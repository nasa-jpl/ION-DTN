/*
	tcpclv4_agent.c:	Standalone TCPCLv4 agent for the ION
				TCP4 convergence-layer adapter.

			Replaces the Python bridge_adapter.py +
			dtn-demo-agent stack with a single C process.

			One agent serves one ION duct: it attaches the
			duct's TX/RX shared-memory pools, connects to
			tcp4clo/tcp4cli's control sockets, and runs one
			TCPCLv4 session (RFC 9174) to the peer.

			Data path:
			  TX: CTRL_SLOT_READY -> read TX pool slot ->
			      segment over TCPCL -> XFER_ACK complete ->
			      CTRL_XFER_SUCCESS
			  RX: TCPCL bundle reassembled -> RX pool slot ->
			      CTRL_BUNDLE_RECV

			Usage:
			  tcpclv4_agent --duct <name> --node-id <eid>
				(--connect <host:port> | --listen <host:port>)
				[--keepalive <sec>]
				[--tls-cert <pem> --tls-key <pem>
				 --tls-ca <pem>] [--tls-noverify]

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcpclv4_session.h"
#include "ion_transfer_pool.h"
#include "ion_ctrl_channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define AGENT_CTRL_KEEPALIVE	15	/* seconds */
#define AGENT_CTRL_TIMEOUT	45	/* seconds */
#define AGENT_RECONNECT_SEC	5	/* TCPCL reconnect backoff */
#define AGENT_STARTUP_WAIT_SEC	30	/* Wait for ION daemons */
#define AGENT_POLL_MS		500

static volatile sig_atomic_t	g_running = 1;

static void	handle_signal(int signum)
{
	(void) signum;
	g_running = 0;
}

/*	Timestamped logging to stderr.					*/

static void	logmsg(const char *fmt, ...)
{
	va_list	args;
	time_t	now = time(NULL);
	char	stamp[32];

	strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S",
			localtime(&now));
	fprintf(stderr, "%s tcpclv4_agent: ", stamp);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	fflush(stderr);
}

/*	Agent state.							*/

typedef struct
{
	char			duct_name[64];
	char			node_id[256];

	/*	Peer (active mode) or listen (passive mode).		*/

	char			host[256];
	int			port;
	int			passive;
	int			listen_fd;

	/*	ION side.						*/

	IonTransferPool		tx_pool;
	IonTransferPool		rx_pool;
	CtrlChannel		tx_ctrl;
	CtrlChannel		rx_ctrl;
	uint64_t		next_rx_xfer_id;

	/*	TCPCL side.						*/

	tcpclv4_session_t	*session;
	tcpclv4_session_config_t session_cfg;
	tcpclv4_tls_config_t	tls_cfg;
	int			use_tls;
	time_t			last_connect_attempt;
	int			session_was_up;

	/*	SLOT_READY messages deferred while the session's
	 *	output queue is over its watermark (backpressure).
	 *	Capacity matches the in-flight ceiling; in practice
	 *	at most num_slots can be pending.			*/

	CtrlMsg			deferred[64];
	int			deferred_count;
} agent_t;

/*	==============================================================
 *	SESSION CALLBACKS
 *	==============================================================
 */

static void	notify_session_state(agent_t *agent, int up,
			const char *peer)
{
	CtrlMsg	msg;

	ctrl_msg_init(&msg, up ? CTRL_SESSION_UP : CTRL_SESSION_DOWN,
			0, 0, peer, peer ? strlen(peer) : 0);
	if (ctrl_send(&agent->tx_ctrl, &msg) < 0)
	{
		logmsg("can't send session state to tcp4clo");
	}

	if (ctrl_send(&agent->rx_ctrl, &msg) < 0)
	{
		logmsg("can't send session state to tcp4cli");
	}
}

static void	on_established(void *user, tcpclv4_session_t *sess)
{
	agent_t		*agent = user;
	const char	*peer = tcpclv4_session_peer_node_id(sess);

	logmsg("TCPCL session established, peer=%s%s",
			peer ? peer : "(unknown)",
			tcpclv4_session_is_secure(sess) ? " (TLS)" : "");
	agent->session_was_up = 1;
	notify_session_state(agent, 1, peer);
}

static void	on_terminated(void *user, tcpclv4_session_t *sess,
			int reason)
{
	agent_t	*agent = user;

	(void) sess;
	logmsg("TCPCL session terminated (reason %d)", reason);
	if (agent->session_was_up)
	{
		agent->session_was_up = 0;
		notify_session_state(agent, 0, NULL);
	}
}

static void	on_xfer_result(void *user, tcpclv4_session_t *sess,
			uint64_t xfer_id, int ok)
{
	agent_t	*agent = user;
	CtrlMsg	msg;

	(void) sess;
	ctrl_msg_init(&msg, ok ? CTRL_XFER_SUCCESS : CTRL_XFER_FAILURE,
			0, xfer_id, NULL, 0);
	if (ctrl_send(&agent->tx_ctrl, &msg) < 0)
	{
		logmsg("can't report xfer %llu disposition",
				(unsigned long long) xfer_id);
	}
}

static void	on_bundle(void *user, tcpclv4_session_t *sess,
			const uint8_t *data, uint64_t len)
{
	agent_t		*agent = user;
	int		slot = -1;
	int		i;
	IonPoolSlotHdr	*hdr;
	char		*dst;
	uint64_t	xfer_id;
	CtrlMsg		msg;

	(void) sess;

	/*	Acquire an RX slot, waiting briefly for tcp4cli to
	 *	drain the pool if it is momentarily full.		*/

	for (i = 0; i < 200; i++)
	{
		slot = pool_slot_acquire(&agent->rx_pool);
		if (slot >= 0)
		{
			break;
		}

		usleep(10000);	/* 10ms; up to 2s total */
	}

	if (slot < 0)
	{
		logmsg("RX pool full, dropping %llu-byte bundle",
				(unsigned long long) len);
		return;
	}

	hdr = pool_slot_hdr(&agent->rx_pool, slot);
	if (len > hdr->capacity)
	{
		logmsg("bundle (%llu bytes) exceeds RX slot capacity "
				"(%llu), dropping",
				(unsigned long long) len,
				(unsigned long long) hdr->capacity);
		pool_slot_force_free(&agent->rx_pool, slot);
		return;
	}

	dst = pool_slot_data(&agent->rx_pool, slot);
	memcpy(dst, data, len);

	xfer_id = agent->next_rx_xfer_id++;
	pool_slot_publish(&agent->rx_pool, slot, len, xfer_id);
	logmsg("RX xfer %llu: %llu bytes -> slot %d",
			(unsigned long long) xfer_id,
			(unsigned long long) len, slot);

	ctrl_msg_init(&msg, CTRL_BUNDLE_RECV, (uint16_t) slot, xfer_id,
			NULL, 0);
	if (ctrl_send(&agent->rx_ctrl, &msg) < 0)
	{
		logmsg("can't send BUNDLE_RECV for slot %d", slot);
	}
}

static const tcpclv4_session_callbacks_t	agent_cbs =
{
	on_established, on_terminated, on_xfer_result, on_bundle
};

/*	==============================================================
 *	TX PATH (ION -> peer)
 *	==============================================================
 */

static void	fail_xfer(agent_t *agent, uint64_t xfer_id)
{
	CtrlMsg	msg;

	ctrl_msg_init(&msg, CTRL_XFER_FAILURE, 0, xfer_id, NULL, 0);
	oK(ctrl_send(&agent->tx_ctrl, &msg));
}

static void	handle_slot_ready(agent_t *agent, const CtrlMsg *msg)
{
	uint64_t	data_len;
	uint64_t	xfer_id;
	char		*data;

	data = pool_slot_read(&agent->tx_pool, msg->slot_index,
			&data_len, &xfer_id);
	if (data == NULL)
	{
		logmsg("TX slot %u not in READY state", msg->slot_index);
		fail_xfer(agent, msg->xfer_id);
		return;
	}

	if (agent->session == NULL || tcpclv4_session_state(agent->session)
			!= TCPCLV4_SESS_ESTABLISHED)
	{
		logmsg("no TCPCL session, failing xfer %llu",
				(unsigned long long) xfer_id);
		pool_slot_release(&agent->tx_pool, msg->slot_index);
		fail_xfer(agent, xfer_id);
		return;
	}

	logmsg("TX xfer %llu: %llu bytes", (unsigned long long) xfer_id,
			(unsigned long long) data_len);

	/*	send_bundle copies segment data as it writes, so the
	 *	slot can be released as soon as the call returns.	*/

	if (tcpclv4_session_send_bundle(agent->session, xfer_id,
			(const uint8_t *) data, data_len) < 0)
	{
		logmsg("send_bundle failed: %s",
				tcpclv4_session_error(agent->session));
		pool_slot_release(&agent->tx_pool, msg->slot_index);
		fail_xfer(agent, xfer_id);
		return;
	}

	pool_slot_release(&agent->tx_pool, msg->slot_index);
}

/*	==============================================================
 *	CONTROL CHANNELS
 *	==============================================================
 */

/*	Process one readable control channel.  Returns -1 if the
 *	channel disconnected.						*/

static int	handle_ctrl(agent_t *agent, CtrlChannel *ch, int is_tx)
{
	CtrlMsg	msg;
	int	ret;

	for (;;)
	{
		ret = ctrl_recv(ch, &msg, 0);	/* Non-blocking */
		if (ret < 0)
		{
			return -1;
		}

		if (ret == 0)
		{
			return 0;
		}

		switch (msg.type)
		{
		case CTRL_SLOT_READY:
			if (!is_tx)
			{
				break;
			}

			/*	Defer new transfers while the session's
			 *	output queue is over its watermark; the
			 *	slot stays READY in the pool meanwhile.	*/

			if (agent->session != NULL
			&& !tcpclv4_session_writable(agent->session)
			&& tcpclv4_session_state(agent->session)
					== TCPCLV4_SESS_ESTABLISHED
			&& agent->deferred_count
					< (int) (sizeof(agent->deferred)
						/ sizeof(CtrlMsg)))
			{
				agent->deferred[agent->deferred_count++] =
						msg;
				break;
			}

			handle_slot_ready(agent, &msg);
			break;

		case CTRL_SLOT_RELEASE:

			/*	Informational: tcp4cli (the consumer)
			 *	already released the RX slot.		*/

			break;

		case CTRL_KEEPALIVE:
			break;	/* last_recv_time updated by ctrl_recv */

		default:
			logmsg("unexpected ctrl msg type 0x%02x on %s",
					msg.type, is_tx ? "TX" : "RX");
			break;
		}
	}
}

/*	Connect both control channels, waiting for the ION daemons
 *	to create their sockets.					*/

static int	connect_ctrl_channels(agent_t *agent)
{
	char	tx_path[300];
	char	rx_path[300];
	int	waited;

	snprintf(tx_path, sizeof(tx_path), "/tmp/ion_tcp4/tx_%s.sock",
			agent->duct_name);
	snprintf(rx_path, sizeof(rx_path), "/tmp/ion_tcp4/rx_%s.sock",
			agent->duct_name);

	for (waited = 0; waited < AGENT_STARTUP_WAIT_SEC; waited++)
	{
		if (ctrl_client_connect(&agent->tx_ctrl, tx_path,
				AGENT_CTRL_KEEPALIVE,
				AGENT_CTRL_TIMEOUT) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (waited >= AGENT_STARTUP_WAIT_SEC)
	{
		logmsg("timed out waiting for %s", tx_path);
		return -1;
	}

	logmsg("connected to tcp4clo on %s", tx_path);

	for (waited = 0; waited < AGENT_STARTUP_WAIT_SEC; waited++)
	{
		if (ctrl_client_connect(&agent->rx_ctrl, rx_path,
				AGENT_CTRL_KEEPALIVE,
				AGENT_CTRL_TIMEOUT) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (waited >= AGENT_STARTUP_WAIT_SEC)
	{
		logmsg("timed out waiting for %s", rx_path);
		ctrl_disconnect(&agent->tx_ctrl);
		return -1;
	}

	logmsg("connected to tcp4cli on %s", rx_path);
	return 0;
}

/*	Attach both pools, waiting for the daemons to create them.	*/

static int	attach_pools(agent_t *agent)
{
	char	tx_name[300];
	char	rx_name[300];
	int	waited;

	snprintf(tx_name, sizeof(tx_name), "/ion_tcp4_tx_%s",
			agent->duct_name);
	snprintf(rx_name, sizeof(rx_name), "/ion_tcp4_rx_%s",
			agent->duct_name);

	for (waited = 0; waited < AGENT_STARTUP_WAIT_SEC; waited++)
	{
		if (pool_attach(tx_name, &agent->tx_pool) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (waited >= AGENT_STARTUP_WAIT_SEC)
	{
		logmsg("timed out waiting for pool %s", tx_name);
		return -1;
	}

	for (waited = 0; waited < AGENT_STARTUP_WAIT_SEC; waited++)
	{
		if (pool_attach(rx_name, &agent->rx_pool) == 0)
		{
			break;
		}

		sleep(1);
	}

	if (waited >= AGENT_STARTUP_WAIT_SEC)
	{
		logmsg("timed out waiting for pool %s", rx_name);
		pool_detach(&agent->tx_pool);
		return -1;
	}

	logmsg("attached pools %s (%u slots) and %s (%u slots)",
			tx_name, agent->tx_pool.num_slots,
			rx_name, agent->rx_pool.num_slots);
	return 0;
}

/*	==============================================================
 *	TCPCL SESSION LIFECYCLE
 *	==============================================================
 */

static int	open_listen_socket(agent_t *agent)
{
	struct sockaddr_in	addr;
	int			opt = 1;
	int			attempt;

	agent->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (agent->listen_fd < 0)
	{
		logmsg("listen socket: %s", strerror(errno));
		return -1;
	}

	setsockopt(agent->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
			sizeof(opt));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(agent->port);
	if (inet_pton(AF_INET, agent->host, &addr.sin_addr) != 1)
	{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	/*	A predecessor agent's socket may take a moment to be
	 *	released after it exits; retry the bind briefly.	*/

	for (attempt = 0; ; attempt++)
	{
		if (bind(agent->listen_fd, (struct sockaddr *) &addr,
				sizeof(addr)) == 0)
		{
			break;
		}

		if (errno != EADDRINUSE
		|| attempt >= AGENT_STARTUP_WAIT_SEC || !g_running)
		{
			logmsg("bind %s:%d: %s", agent->host, agent->port,
					strerror(errno));
			close(agent->listen_fd);
			agent->listen_fd = -1;
			return -1;
		}

		sleep(1);
	}

	if (listen(agent->listen_fd, 4) < 0)
	{
		logmsg("listen %s:%d: %s", agent->host, agent->port,
				strerror(errno));
		close(agent->listen_fd);
		agent->listen_fd = -1;
		return -1;
	}

	logmsg("listening on %s:%d", agent->host, agent->port);
	return 0;
}

static void	try_connect_session(agent_t *agent)
{
	time_t	now = time(NULL);

	if (agent->session != NULL)
	{
		return;
	}

	if (now - agent->last_connect_attempt < AGENT_RECONNECT_SEC)
	{
		return;
	}

	agent->last_connect_attempt = now;
	agent->session = tcpclv4_session_connect(agent->host, agent->port,
			&agent->session_cfg, &agent_cbs, agent);
	if (agent->session == NULL)
	{
		logmsg("connect to %s:%d failed, will retry",
				agent->host, agent->port);
	}
	else
	{
		logmsg("connected to %s:%d, negotiating",
				agent->host, agent->port);
	}
}

static void	accept_session(agent_t *agent)
{
	int		conn_fd;
	struct timeval	tv = { 10, 0 };

	conn_fd = accept(agent->listen_fd, NULL, NULL);
	if (conn_fd < 0)
	{
		return;
	}

	/*	Establishment does blocking I/O on this fd (contact
	 *	exchange, TLS handshake, SESS_INIT); bound it so a
	 *	silent peer cannot stall the event loop.		*/

	setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(conn_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	if (agent->session != NULL)
	{
		/*	One session at a time; refuse extras.  Per
		 *	RFC 9174, do so gracefully: complete the
		 *	contact header exchange (and TLS handshake if
		 *	negotiated), send SESS_INIT, then SESS_TERM
		 *	with reason BUSY so the peer knows to back off
		 *	rather than treating this as a network fault.	*/

		tcpclv4_session_t	*extra;

		logmsg("refusing second inbound connection (BUSY)");
		extra = tcpclv4_session_accept(conn_fd,
				&agent->session_cfg, NULL, NULL);
		if (extra != NULL)
		{
			tcpclv4_session_terminate(extra, TCPCLV4_TERM_BUSY);
			tcpclv4_session_close(extra);
		}

		return;
	}

	agent->session = tcpclv4_session_accept(conn_fd,
			&agent->session_cfg, &agent_cbs, agent);
	if (agent->session == NULL)
	{
		logmsg("inbound session establishment failed");
	}
	else
	{
		logmsg("inbound connection accepted, negotiating");
	}
}

/*	Process transfers deferred by backpressure, in arrival order,
 *	for as long as the session will take them.  With no session,
 *	each deferred transfer is failed back to ION (slot released,
 *	CTRL_XFER_FAILURE sent) via the same path handle_slot_ready
 *	uses for that case.						*/

static void	drain_deferred(agent_t *agent)
{
	int	i;

	while (agent->deferred_count > 0)
	{
		if (agent->session != NULL
		&& !tcpclv4_session_writable(agent->session))
		{
			return;	/* Still backed up; try again later. */
		}

		handle_slot_ready(agent, &agent->deferred[0]);
		agent->deferred_count--;
		for (i = 0; i < agent->deferred_count; i++)
		{
			agent->deferred[i] = agent->deferred[i + 1];
		}
	}
}

static void	drop_session(agent_t *agent)
{
	if (agent->session != NULL)
	{
		tcpclv4_session_close(agent->session);
		agent->session = NULL;
	}

	/*	Fail anything still deferred back to ION so the
	 *	bundles can be reforwarded.				*/

	drain_deferred(agent);
}

/*	==============================================================
 *	MAIN LOOP
 *	==============================================================
 */

static int	run_agent(agent_t *agent)
{
	struct pollfd	pfds[4];
	int		nfds;
	int		session_idx;
	int		listen_idx;
	int		ret;

	while (g_running)
	{
		nfds = 0;
		pfds[nfds].fd = agent->tx_ctrl.conn_fd;
		pfds[nfds].events = POLLIN;
		nfds++;
		pfds[nfds].fd = agent->rx_ctrl.conn_fd;
		pfds[nfds].events = POLLIN;
		nfds++;

		listen_idx = -1;
		if (agent->passive && agent->listen_fd >= 0)
		{
			listen_idx = nfds;
			pfds[nfds].fd = agent->listen_fd;
			pfds[nfds].events = POLLIN;
			nfds++;
		}

		session_idx = -1;
		if (agent->session != NULL)
		{
			session_idx = nfds;
			pfds[nfds].fd = tcpclv4_session_fd(agent->session);
			pfds[nfds].events = POLLIN;
			if (tcpclv4_session_wants_write(agent->session))
			{
				pfds[nfds].events |= POLLOUT;
			}

			nfds++;
		}

		ret = poll(pfds, nfds, AGENT_POLL_MS);
		if (ret < 0 && errno != EINTR)
		{
			logmsg("poll: %s", strerror(errno));
			return -1;
		}

		if (ret > 0)
		{
			if (pfds[0].revents)
			{
				if (handle_ctrl(agent, &agent->tx_ctrl, 1)
						< 0)
				{
					logmsg("tcp4clo control channel "
							"lost; exiting");
					return -1;
				}
			}

			if (pfds[1].revents)
			{
				if (handle_ctrl(agent, &agent->rx_ctrl, 0)
						< 0)
				{
					logmsg("tcp4cli control channel "
							"lost; exiting");
					return -1;
				}
			}

			/*	Handle the existing session's I/O BEFORE
			 *	accepting new connections.  When a peer
			 *	drops and immediately reconnects (e.g.
			 *	after an idle timeout), the dead session's
			 *	fd and the listen socket often become
			 *	readable on the same poll wakeup; reaping
			 *	the dead session first clears agent->session
			 *	so the reconnect is accepted rather than
			 *	refused as BUSY.				*/

			if (session_idx >= 0 && pfds[session_idx].revents
			&& agent->session != NULL)
			{
				if ((pfds[session_idx].revents & POLLOUT)
				&& tcpclv4_session_handle_output(agent->
						session) < 0)
				{
					drop_session(agent);
				}

				if (agent->session != NULL
				&& (pfds[session_idx].revents
					& (POLLIN | POLLHUP | POLLERR))
				&& tcpclv4_session_handle_io(agent->session)
						< 0)
				{
					drop_session(agent);
				}
			}

			if (listen_idx >= 0 && pfds[listen_idx].revents)
			{
				accept_session(agent);
			}
		}

		/*	Resume transfers deferred by backpressure once
		 *	the output queue drains below its watermark.	*/

		if (agent->deferred_count > 0)
		{
			drain_deferred(agent);
		}

		/*	Timers.						*/

		if (agent->session != NULL
		&& tcpclv4_session_tick(agent->session) < 0)
		{
			drop_session(agent);
		}

		if (ctrl_keepalive_due(&agent->tx_ctrl))
		{
			oK(ctrl_send_keepalive(&agent->tx_ctrl));
		}

		if (ctrl_keepalive_due(&agent->rx_ctrl))
		{
			oK(ctrl_send_keepalive(&agent->rx_ctrl));
		}

		if (!agent->passive)
		{
			try_connect_session(agent);
		}
	}

	return 0;
}

/*	==============================================================
 *	STARTUP
 *	==============================================================
 */

static void	usage(void)
{
	fprintf(stderr,
"Usage: tcpclv4_agent --duct <name> --node-id <eid>\n"
"           (--connect <host:port> | --listen <host:port>)\n"
"           [--keepalive <seconds>] [--segment-mtu <bytes>]\n"
"           [--tls-cert <pem> --tls-key <pem> --tls-ca <pem>]\n"
"           [--tls-noverify]\n");
}

static int	parse_hostport(const char *arg, char *host, size_t host_len,
			int *port)
{
	const char	*colon = strrchr(arg, ':');

	if (colon == NULL || colon == arg)
	{
		return -1;
	}

	if ((size_t)(colon - arg) >= host_len)
	{
		return -1;
	}

	memcpy(host, arg, colon - arg);
	host[colon - arg] = '\0';
	*port = atoi(colon + 1);
	if (*port <= 0 || *port > 65535)
	{
		return -1;
	}

	return 0;
}

int	main(int argc, char *argv[])
{
	static agent_t	agent;
	int		i;
	int		have_endpoint = 0;
	int		tls_verify = 1;
	int		exit_code;

	agent.listen_fd = -1;
	agent.next_rx_xfer_id = 1;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--duct") == 0 && i + 1 < argc)
		{
			snprintf(agent.duct_name, sizeof(agent.duct_name),
					"%s", argv[++i]);
		}
		else if (strcmp(argv[i], "--node-id") == 0 && i + 1 < argc)
		{
			snprintf(agent.node_id, sizeof(agent.node_id),
					"%s", argv[++i]);
		}
		else if (strcmp(argv[i], "--connect") == 0 && i + 1 < argc)
		{
			if (parse_hostport(argv[++i], agent.host,
					sizeof(agent.host), &agent.port) < 0)
			{
				usage();
				return 1;
			}

			agent.passive = 0;
			have_endpoint = 1;
		}
		else if (strcmp(argv[i], "--listen") == 0 && i + 1 < argc)
		{
			if (parse_hostport(argv[++i], agent.host,
					sizeof(agent.host), &agent.port) < 0)
			{
				usage();
				return 1;
			}

			agent.passive = 1;
			have_endpoint = 1;
		}
		else if (strcmp(argv[i], "--keepalive") == 0 && i + 1 < argc)
		{
			agent.session_cfg.keepalive_interval =
					atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "--segment-mtu") == 0
				&& i + 1 < argc)
		{
			agent.session_cfg.segment_mtu = strtoull(argv[++i],
					NULL, 10);
		}
		else if (strcmp(argv[i], "--tls-cert") == 0 && i + 1 < argc)
		{
			agent.tls_cfg.tls_cert_file = argv[++i];
			agent.use_tls = 1;
		}
		else if (strcmp(argv[i], "--tls-key") == 0 && i + 1 < argc)
		{
			agent.tls_cfg.tls_key_file = argv[++i];
			agent.use_tls = 1;
		}
		else if (strcmp(argv[i], "--tls-ca") == 0 && i + 1 < argc)
		{
			agent.tls_cfg.tls_ca_file = argv[++i];
			agent.use_tls = 1;
		}
		else if (strcmp(argv[i], "--tls-noverify") == 0)
		{
			tls_verify = 0;
		}
		else
		{
			usage();
			return 1;
		}
	}

	if (agent.duct_name[0] == '\0' || agent.node_id[0] == '\0'
	|| !have_endpoint)
	{
		usage();
		return 1;
	}

	agent.session_cfg.node_id = agent.node_id;
	if (agent.use_tls)
	{
		agent.tls_cfg.tls_verify_peer = tls_verify;
		agent.session_cfg.tls = &agent.tls_cfg;
	}

	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGPIPE, SIG_IGN);

	tcpclv4_tls_init();

	logmsg("starting: duct=%s node=%s %s %s:%d%s", agent.duct_name,
			agent.node_id,
			agent.passive ? "listening on" : "connecting to",
			agent.host, agent.port,
			agent.use_tls ? " (TLS)" : "");

	if (attach_pools(&agent) < 0
	|| connect_ctrl_channels(&agent) < 0)
	{
		return 1;
	}

	if (agent.passive && open_listen_socket(&agent) < 0)
	{
		return 1;
	}

	exit_code = run_agent(&agent) < 0 ? 1 : 0;

	/*	Graceful shutdown: terminate the TCPCL session and
	 *	give the peer a moment to reply.			*/

	logmsg("shutting down");
	if (agent.session != NULL && tcpclv4_session_state(agent.session)
			== TCPCLV4_SESS_ESTABLISHED)
	{
		struct pollfd	pfd;
		int		waited = 0;

		tcpclv4_session_terminate(agent.session,
				TCPCLV4_TERM_UNKNOWN);
		pfd.fd = tcpclv4_session_fd(agent.session);
		while (waited < 3000 && tcpclv4_session_state(agent.session)
				!= TCPCLV4_SESS_TERMINATED)
		{
			pfd.events = POLLIN;
			if (tcpclv4_session_wants_write(agent.session))
			{
				pfd.events |= POLLOUT;
			}

			if (poll(&pfd, 1, 100) > 0)
			{
				if ((pfd.revents & POLLOUT)
				&& tcpclv4_session_handle_output(agent.
						session) < 0)
				{
					break;
				}

				if ((pfd.revents & (POLLIN | POLLHUP))
				&& tcpclv4_session_handle_io(agent.session)
						< 0)
				{
					break;
				}
			}
			else
			{
				waited += 100;
			}
		}
	}

	drop_session(&agent);
	if (agent.listen_fd >= 0)
	{
		close(agent.listen_fd);
	}

	ctrl_disconnect(&agent.tx_ctrl);
	ctrl_disconnect(&agent.rx_ctrl);
	pool_detach(&agent.tx_pool);
	pool_detach(&agent.rx_pool);
	tcpclv4_tls_cleanup();
	logmsg("stopped");
	return exit_code;
}
