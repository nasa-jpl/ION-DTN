/*
	tcpclv4_session.c:	TCPCLv4 session state machine (RFC 9174).

			See tcpclv4_session.h for the model.  All
			state lives in the session object; there are
			no threads and no global state, so multiple
			sessions can coexist in one event loop.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "tcpclv4_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/*	Defaults applied to zeroed config fields.			*/

#define DEFAULT_KEEPALIVE	15		/* seconds */
#define DEFAULT_SEGMENT_MRU	(1ULL << 20)	/* 1MB */
#define DEFAULT_TRANSFER_MRU	(100ULL << 20)	/* 100MB */
#define DEFAULT_SEGMENT_MTU	(1ULL << 20)	/* 1MB */

/*	Fixed overhead of an XFER_SEGMENT header: type(1) + flags(1)
 *	+ xfer_id(8) + ext_len(4, START only) + data_len(8).		*/

#define SEGMENT_HDR_OVERHEAD	22

/*	Output-queue watermark: the session reports itself
 *	non-writable once this many bytes are queued, so the owner
 *	defers starting new transfers (backpressure).  The queue can
 *	exceed this transiently by up to one bundle.			*/

#define OUT_QUEUE_WATERMARK	(1UL << 20)	/* 1MB */

/*	Headroom added to the preallocated output queue above the
 *	maximum single outbound bundle: covers per-segment header
 *	overhead plus control messages (acks/keepalives) that may
 *	queue behind a bundle while the socket is briefly stalled.	*/

#define OUT_QUEUE_HEADROOM	(128UL * 1024)	/* 128KB */

/*	Outbound transfer awaiting acknowledgment.			*/

typedef struct
{
	uint64_t	xfer_id;
	uint64_t	total_len;
	uint64_t	acked_len;
	int		in_use;
} tx_transfer_t;

struct tcpclv4_session_s
{
	tcpclv4_tls_session_t		*transport;
	tcpclv4_session_state_t		state;
	tcpclv4_session_callbacks_t	cbs;
	void				*user;

	char		local_node_id[256];
	char		*peer_node_id;
	int		auth_required;	/* Authenticate peer Node ID vs cert */

	/*	Our parameters (offered in SESS_INIT).			*/

	uint16_t	keepalive_interval;
	uint64_t	segment_mru;
	uint64_t	transfer_mru;
	uint64_t	segment_mtu;

	/*	Negotiated with peer's SESS_INIT.			*/

	uint16_t	session_keepalive;	/* min(ours, theirs) */
	uint64_t	peer_segment_mru;
	uint64_t	peer_transfer_mru;

	/*	Keepalive timers.					*/

	time_t		last_recv_time;
	time_t		last_send_time;

	/*	Outbound transfers in flight.				*/

	tx_transfer_t	inflight[TCPCLV4_SESSION_MAX_INFLIGHT];

	/*	Inbound transfer reassembly.  RFC 9174 transfers are
	 *	sequential within a session: a new transfer cannot
	 *	start until the previous one ends.			*/

	int		rx_active;
	uint64_t	rx_xfer_id;
	uint8_t		*rx_buf;
	uint64_t	rx_len;
	uint64_t	rx_cap;

	/*	Scratch buffers.					*/

	uint8_t		*tx_msg_buf;	/* One segment + header	*/
	size_t		tx_msg_cap;
	uint8_t		*rx_msg_buf;	/* One inbound message	*/
	size_t		rx_msg_cap;

	/*	Output queue.  All post-establishment sends append
	 *	here and drain via non-blocking writes, so a peer
	 *	that is itself busy writing can never deadlock us
	 *	(both sides blocked in write, neither reading).		*/

	uint8_t		*out_buf;
	size_t		out_head;	/* First unsent byte	*/
	size_t		out_len;	/* Unsent bytes		*/
	size_t		out_cap;

	int		term_sent;
	int		term_reason;
	char		error_msg[512];
};

/*	==============================================================
 *	INTERNAL HELPERS
 *	==============================================================
 */

static void set_error(tcpclv4_session_t *s, const char *msg)
{
	snprintf(s->error_msg, sizeof(s->error_msg), "%s", msg);
}

static void set_transport_error(tcpclv4_session_t *s, const char *prefix)
{
	const char	*terr = tcpclv4_tls_get_error(s->transport);

	snprintf(s->error_msg, sizeof(s->error_msg), "%s%s%s", prefix,
			terr ? ": " : "", terr ? terr : "");
}

/*	Drain the output queue with non-blocking writes.  Returns 0
 *	on success (possibly with bytes still queued), -1 on
 *	transport error.					*/

static int flush_output(tcpclv4_session_t *s)
{
	int	n;

	while (s->out_len > 0)
	{
		n = tcpclv4_tls_write(s->transport,
				s->out_buf + s->out_head, s->out_len);
		if (n < 0)
		{
			set_transport_error(s, "send failed");
			return -1;
		}

		if (n == 0)
		{
			return 0;	/* Socket full; poll POLLOUT. */
		}

		s->out_head += n;
		s->out_len -= n;
	}

	s->out_head = 0;
	return 0;
}

/*	Append bytes to the output queue and opportunistically flush.
 *	Never blocks: what the socket will not take stays queued and
 *	drains from tcpclv4_session_handle_output() when the owner's
 *	poll reports the fd writable.				*/

static int send_raw(tcpclv4_session_t *s, const uint8_t *buf, size_t len)
{
	if (s->out_head + s->out_len + len > s->out_cap)
	{
		/*	Compact.  The output queue is preallocated (see
		 *	session_create) and never grown on the data path,
		 *	so after compaction the append must fit.		*/

		if (s->out_head > 0)
		{
			memmove(s->out_buf, s->out_buf + s->out_head,
					s->out_len);
			s->out_head = 0;
		}

		if (s->out_len + len > s->out_cap)
		{
			/*	Bound violation (bundle larger than the
			 *	configured maximum, or excessive control
			 *	backlog behind a stalled socket): fail the
			 *	session rather than allocate.			*/

			set_error(s, "Output queue overflow (exceeds "
					"preallocated capacity)");
			return -1;
		}
	}

	memcpy(s->out_buf + s->out_head + s->out_len, buf, len);
	s->out_len += len;
	s->last_send_time = time(NULL);

	return flush_output(s);
}

static int send_sess_term(tcpclv4_session_t *s, uint8_t reason,
		int is_reply)
{
	tcpclv4_sess_term_t	term;
	uint8_t			buf[8];
	size_t			len;

	term.flags = is_reply ? TCPCLV4_TERM_FLAG_REPLY : 0;
	term.reason_code = reason;

	if (tcpclv4_encode_sess_term(&term, buf, sizeof(buf), &len) < 0)
	{
		return -1;
	}

	return send_raw(s, buf, len);
}

/*	Enter TERMINATED state and notify the owner exactly once.	*/

static void terminate_session(tcpclv4_session_t *s, int reason)
{
	int	i;

	if (s->state == TCPCLV4_SESS_TERMINATED)
	{
		return;
	}

	s->state = TCPCLV4_SESS_TERMINATED;

	/*	Fail any transfers still awaiting acknowledgment.	*/

	for (i = 0; i < TCPCLV4_SESSION_MAX_INFLIGHT; i++)
	{
		if (s->inflight[i].in_use)
		{
			s->inflight[i].in_use = 0;
			if (s->cbs.on_xfer_result)
			{
				s->cbs.on_xfer_result(s->user, s,
						s->inflight[i].xfer_id, 0);
			}
		}
	}

	if (s->cbs.on_terminated)
	{
		s->cbs.on_terminated(s->user, s, reason);
	}
}

static tx_transfer_t *find_inflight(tcpclv4_session_t *s, uint64_t xfer_id)
{
	int	i;

	for (i = 0; i < TCPCLV4_SESSION_MAX_INFLIGHT; i++)
	{
		if (s->inflight[i].in_use
		&& s->inflight[i].xfer_id == xfer_id)
		{
			return &s->inflight[i];
		}
	}

	return NULL;
}

/*	==============================================================
 *	MESSAGE HANDLERS
 *	==============================================================
 */

static int handle_sess_init(tcpclv4_session_t *s, const uint8_t *buf,
		size_t len)
{
	tcpclv4_sess_init_t	init;

	if (s->state != TCPCLV4_SESS_NEGOTIATING)
	{
		set_error(s, "Unexpected SESS_INIT");
		return -1;
	}

	if (tcpclv4_decode_sess_init(buf, len, &init) < 0)
	{
		set_error(s, "Malformed SESS_INIT from peer");
		return -1;
	}

	s->peer_node_id = init.node_id;	/* Take ownership */
	init.node_id = NULL;

	/*	Authenticate the peer's Node ID against its certificate.
	 *	On failure the caller sends SESS_TERM (Contact Failure)
	 *	and tears the session down.				*/

	if (tcpclv4_tls_authenticate_nodeid(s->transport, s->peer_node_id,
			s->auth_required) < 0)
	{
		set_error(s, "Peer Node ID not authenticated by certificate");
		return -1;
	}

	s->peer_segment_mru = init.segment_mru;
	s->peer_transfer_mru = init.transfer_mru;
	s->session_keepalive = (init.keepalive_interval
			< s->keepalive_interval)
			? init.keepalive_interval : s->keepalive_interval;
	free(init.extensions);

	/*	Inbound messages can now be as large as our segment
	 *	MRU plus header overhead; bound transport framing
	 *	accordingly.						*/

	tcpclv4_tls_set_max_msg_len(s->transport,
			s->segment_mru + SEGMENT_HDR_OVERHEAD + 1024);

	s->state = TCPCLV4_SESS_ESTABLISHED;
	if (s->cbs.on_established)
	{
		s->cbs.on_established(s->user, s);
	}

	return 0;
}

static int send_xfer_ack(tcpclv4_session_t *s, uint8_t seg_flags,
		uint64_t xfer_id, uint64_t ack_len)
{
	tcpclv4_xfer_ack_t	ack;
	uint8_t			buf[32];
	size_t			len;

	ack.flags = seg_flags;
	ack.transfer_id = xfer_id;
	ack.ack_len = ack_len;

	if (tcpclv4_encode_xfer_ack(&ack, buf, sizeof(buf), &len) < 0)
	{
		return -1;
	}

	return send_raw(s, buf, len);
}

static int send_xfer_refuse(tcpclv4_session_t *s, uint8_t reason,
		uint64_t xfer_id)
{
	tcpclv4_xfer_refuse_t	refuse;
	uint8_t			buf[16];
	size_t			len;

	refuse.reason_code = reason;
	refuse.transfer_id = xfer_id;

	if (tcpclv4_encode_xfer_refuse(&refuse, buf, sizeof(buf), &len) < 0)
	{
		return -1;
	}

	return send_raw(s, buf, len);
}

static int handle_xfer_segment(tcpclv4_session_t *s, const uint8_t *buf,
		size_t len)
{
	tcpclv4_xfer_segment_t	seg;

	if (s->state != TCPCLV4_SESS_ESTABLISHED
	&& s->state != TCPCLV4_SESS_ENDING)
	{
		set_error(s, "XFER_SEGMENT before session established");
		return -1;
	}

	if (tcpclv4_decode_xfer_segment(buf, len, &seg) < 0)
	{
		set_error(s, "Malformed XFER_SEGMENT");
		return -1;
	}

	tcpclv4_xfer_segment_free(&seg);	/* Frees extensions only;
						 * seg.data stays valid. */

	if (seg.flags & TCPCLV4_XFER_FLAG_START)
	{
		if (s->rx_active)
		{
			set_error(s, "START segment inside active transfer");
			return -1;
		}

		s->rx_active = 1;
		s->rx_xfer_id = seg.transfer_id;
		s->rx_len = 0;
	}
	else
	{
		if (!s->rx_active || seg.transfer_id != s->rx_xfer_id)
		{
			set_error(s, "Segment for unknown transfer");
			return -1;
		}
	}

	/*	Enforce our transfer MRU.				*/

	if (s->rx_len + seg.data_len > s->transfer_mru)
	{
		s->rx_active = 0;
		if (send_xfer_refuse(s, TCPCLV4_REFUSE_NO_RESOURCES,
				seg.transfer_id) < 0)
		{
			return -1;
		}

		return 0;
	}

	/*	Append segment data to the reassembly buffer.		*/

	if (s->rx_len + seg.data_len > s->rx_cap)
	{
		/*	Cannot happen: the reassembly buffer is
		 *	preallocated to transfer_mru and the MRU check
		 *	above already refused anything larger.  Defensive
		 *	only -- never grown on the data path.		*/

		set_error(s, "Reassembly buffer overflow");
		return -1;
	}

	if (seg.data_len > 0)
	{
		memcpy(s->rx_buf + s->rx_len, seg.data, seg.data_len);
		s->rx_len += seg.data_len;
	}

	/*	Acknowledge cumulative reception (§5.2.3).		*/

	if (send_xfer_ack(s, seg.flags, seg.transfer_id, s->rx_len) < 0)
	{
		return -1;
	}

	if (seg.flags & TCPCLV4_XFER_FLAG_END)
	{
		s->rx_active = 0;
		if (s->cbs.on_bundle)
		{
			s->cbs.on_bundle(s->user, s, s->rx_buf, s->rx_len);
		}
	}

	return 0;
}

static int handle_xfer_ack(tcpclv4_session_t *s, const uint8_t *buf,
		size_t len)
{
	tcpclv4_xfer_ack_t	ack;
	tx_transfer_t		*xfer;

	if (tcpclv4_decode_xfer_ack(buf, len, &ack) < 0)
	{
		set_error(s, "Malformed XFER_ACK");
		return -1;
	}

	xfer = find_inflight(s, ack.transfer_id);
	if (xfer == NULL)
	{
		/*	ACK for an unknown transfer; harmless.		*/

		return 0;
	}

	xfer->acked_len = ack.ack_len;
	if (xfer->acked_len >= xfer->total_len)
	{
		xfer->in_use = 0;
		if (s->cbs.on_xfer_result)
		{
			s->cbs.on_xfer_result(s->user, s, ack.transfer_id, 1);
		}
	}

	return 0;
}

static int handle_xfer_refuse(tcpclv4_session_t *s, const uint8_t *buf,
		size_t len)
{
	tcpclv4_xfer_refuse_t	refuse;
	tx_transfer_t		*xfer;

	if (tcpclv4_decode_xfer_refuse(buf, len, &refuse) < 0)
	{
		set_error(s, "Malformed XFER_REFUSE");
		return -1;
	}

	xfer = find_inflight(s, refuse.transfer_id);
	if (xfer != NULL)
	{
		xfer->in_use = 0;
		if (s->cbs.on_xfer_result)
		{
			s->cbs.on_xfer_result(s->user, s,
					refuse.transfer_id, 0);
		}
	}

	return 0;
}

static int handle_sess_term(tcpclv4_session_t *s, const uint8_t *buf,
		size_t len)
{
	tcpclv4_sess_term_t	term;

	if (tcpclv4_decode_sess_term(buf, len, &term) < 0)
	{
		set_error(s, "Malformed SESS_TERM");
		return -1;
	}

	if (s->state == TCPCLV4_SESS_ENDING)
	{
		/*	Peer's reply to our SESS_TERM; both sides are
		 *	done.						*/

		terminate_session(s, s->term_reason);
		return 0;
	}

	/*	Peer-initiated termination: reply, then finish.  Any
	 *	transfers the peer has fully acked already fired their
	 *	callbacks; the rest fail in terminate_session().	*/

	oK(send_sess_term(s, term.reason_code, 1));
	terminate_session(s, term.reason_code);
	return 0;
}

static int handle_message(tcpclv4_session_t *s, const uint8_t *buf,
		size_t len)
{
	switch (tcpclv4_get_msg_type(buf, len))
	{
	case TCPCLV4_MSG_SESS_INIT:
		return handle_sess_init(s, buf, len);

	case TCPCLV4_MSG_KEEPALIVE:
		return 0;	/* last_recv_time already updated */

	case TCPCLV4_MSG_XFER_SEGMENT:
		return handle_xfer_segment(s, buf, len);

	case TCPCLV4_MSG_XFER_ACK:
		return handle_xfer_ack(s, buf, len);

	case TCPCLV4_MSG_XFER_REFUSE:
		return handle_xfer_refuse(s, buf, len);

	case TCPCLV4_MSG_SESS_TERM:
		return handle_sess_term(s, buf, len);

	case TCPCLV4_MSG_MSG_REJECT:
		/*	Peer rejected one of our messages.  Log and
		 *	continue; the transfer-level timeouts and the
		 *	owner's policy decide what to do next.		*/

		set_error(s, "Peer sent MSG_REJECT");
		return 0;

	default:
		set_error(s, "Unknown message type from peer");
		return -1;
	}
}

/*	==============================================================
 *	ESTABLISHMENT
 *	==============================================================
 */

static tcpclv4_session_t *session_create(
		const tcpclv4_session_config_t *cfg,
		const tcpclv4_session_callbacks_t *cbs, void *user)
{
	tcpclv4_session_t	*s;

	if (cfg == NULL || cfg->node_id == NULL)
	{
		return NULL;
	}

	s = calloc(1, sizeof(*s));
	if (s == NULL)
	{
		return NULL;
	}

	snprintf(s->local_node_id, sizeof(s->local_node_id), "%s",
			cfg->node_id);

	/*	When peer verification is on, the peer's Node ID must be
	 *	authenticated against its certificate once SESS_INIT
	 *	arrives (RFC 9174 §4.4.3).				*/

	s->auth_required = (cfg->tls != NULL && cfg->tls->tls_verify_peer);

	if (cbs != NULL)
	{
		s->cbs = *cbs;
	}

	s->user = user;
	s->keepalive_interval = cfg->keepalive_interval
			? cfg->keepalive_interval : DEFAULT_KEEPALIVE;
	s->segment_mru = cfg->segment_mru
			? cfg->segment_mru : DEFAULT_SEGMENT_MRU;
	s->transfer_mru = cfg->transfer_mru
			? cfg->transfer_mru : DEFAULT_TRANSFER_MRU;
	s->segment_mtu = cfg->segment_mtu
			? cfg->segment_mtu : DEFAULT_SEGMENT_MTU;
	s->state = TCPCLV4_SESS_NEGOTIATING;
	s->last_recv_time = time(NULL);
	s->last_send_time = time(NULL);

	/*	Preallocate every data-path buffer to its maximum, so the
	 *	session performs no heap allocation while forwarding
	 *	bundles (flight memory discipline).  Bounds:
	 *	  rx_buf     : one full inbound transfer  (transfer_mru)
	 *	  rx_msg_buf : one inbound message        (segment_mru + hdr)
	 *	  tx_msg_buf : one outbound segment       (segment_mtu + hdr)
	 *	  out_buf    : one outbound bundle + control headroom
	 *	max_tx_transfer (the sender's slot capacity) bounds the
	 *	output queue; it defaults to transfer_mru when unset.	*/

	{
		uint64_t	tx_bound = cfg->max_tx_transfer
					? cfg->max_tx_transfer : s->transfer_mru;

		s->rx_cap = s->transfer_mru;
		s->rx_msg_cap = s->segment_mru + SEGMENT_HDR_OVERHEAD + 1024;
		s->tx_msg_cap = s->segment_mtu + SEGMENT_HDR_OVERHEAD;
		s->out_cap = tx_bound + OUT_QUEUE_HEADROOM;

		s->rx_buf = malloc(s->rx_cap);
		s->rx_msg_buf = malloc(s->rx_msg_cap);
		s->tx_msg_buf = malloc(s->tx_msg_cap);
		s->out_buf = malloc(s->out_cap);
		if (s->rx_buf == NULL || s->rx_msg_buf == NULL
				|| s->tx_msg_buf == NULL || s->out_buf == NULL)
		{
			free(s->rx_buf);
			free(s->rx_msg_buf);
			free(s->tx_msg_buf);
			free(s->out_buf);
			free(s);
			return NULL;
		}
	}

	return s;
}

static int send_sess_init(tcpclv4_session_t *s)
{
	tcpclv4_sess_init_t	init;
	uint8_t			buf[512];
	size_t			len;

	memset(&init, 0, sizeof(init));
	init.keepalive_interval = s->keepalive_interval;
	init.segment_mru = s->segment_mru;
	init.transfer_mru = s->transfer_mru;
	init.node_id = s->local_node_id;
	init.node_id_len = strlen(s->local_node_id);

	if (tcpclv4_encode_sess_init(&init, buf, sizeof(buf), &len) < 0)
	{
		set_error(s, "SESS_INIT encode failed");
		return -1;
	}

	return send_raw(s, buf, len);
}

tcpclv4_session_t *tcpclv4_session_connect(const char *host, int port,
		const tcpclv4_session_config_t *cfg,
		const tcpclv4_session_callbacks_t *cbs, void *user)
{
	tcpclv4_session_t	*s;

	s = session_create(cfg, cbs, user);
	if (s == NULL)
	{
		return NULL;
	}

	s->transport = tcpclv4_tls_connect(host, port, s->local_node_id,
			cfg->tls);
	if (s->transport == NULL
	|| tcpclv4_tls_set_nonblocking(s->transport, 1) < 0
	|| send_sess_init(s) < 0)
	{
		tcpclv4_session_close(s);
		return NULL;
	}

	return s;
}

tcpclv4_session_t *tcpclv4_session_accept(int connected_fd,
		const tcpclv4_session_config_t *cfg,
		const tcpclv4_session_callbacks_t *cbs, void *user)
{
	tcpclv4_session_t	*s;

	s = session_create(cfg, cbs, user);
	if (s == NULL)
	{
		close(connected_fd);	/* Accept always owns the fd. */
		return NULL;
	}

	s->transport = tcpclv4_tls_accept(connected_fd, s->local_node_id,
			cfg->tls);
	if (s->transport == NULL
	|| tcpclv4_tls_set_nonblocking(s->transport, 1) < 0
	|| send_sess_init(s) < 0)
	{
		tcpclv4_session_close(s);
		return NULL;
	}

	return s;
}

/*	==============================================================
 *	EVENT LOOP INTEGRATION
 *	==============================================================
 */

int	tcpclv4_session_fd(tcpclv4_session_t *s)
{
	if (s == NULL || s->transport == NULL)
	{
		return -1;
	}

	return tcpclv4_tls_get_fd(s->transport);
}

int	tcpclv4_session_handle_io(tcpclv4_session_t *s)
{
	size_t	len;
	int	ret;
	int	drained_one = 0;

	if (s == NULL || s->transport == NULL
	|| s->state == TCPCLV4_SESS_TERMINATED)
	{
		return -1;
	}

	/*	Size the message buffer to the largest inbound
	 *	message we accept.					*/

	if (s->rx_msg_cap < s->segment_mru + SEGMENT_HDR_OVERHEAD + 1024)
	{
		/*	Cannot happen: rx_msg_buf is preallocated to this
		 *	size in session_create.  Defensive only.	*/

		set_error(s, "Message buffer under-provisioned");
		terminate_session(s, TCPCLV4_TERM_RESOURCE_EXHAUSTION);
		return -1;
	}

	/*	Drain every complete message currently available.	*/

	for (;;)
	{
		ret = tcpclv4_tls_recv_msg(s->transport, s->rx_msg_buf,
				s->rx_msg_cap, &len);
		if (ret < 0)
		{
			/*	Transport error or connection closed.
			 *	An orderly close after SESS_TERM
			 *	exchange is normal termination.	*/

			set_transport_error(s, "recv failed");
			terminate_session(s, s->term_sent
					? s->term_reason
					: TCPCLV4_TERM_UNKNOWN);
			return -1;
		}

		if (ret == 0)
		{
			/*	No complete message waiting.  If the
			 *	first call would have blocked, the
			 *	caller's poll was spurious; fine.	*/

			return drained_one ? 0
				: (s->state == TCPCLV4_SESS_TERMINATED
					? -1 : 0);
		}

		drained_one = 1;
		s->last_recv_time = time(NULL);

		if (handle_message(s, s->rx_msg_buf, len) < 0)
		{
			/*	Protocol violation: bail out.		*/

			oK(send_sess_term(s, TCPCLV4_TERM_CONTACT_FAILURE,
					0));
			terminate_session(s, TCPCLV4_TERM_CONTACT_FAILURE);
			return -1;
		}

		if (s->state == TCPCLV4_SESS_TERMINATED)
		{
			return -1;
		}

		/*	More buffered messages?				*/

		if (!tcpclv4_tls_pending(s->transport))
		{
			return 0;
		}
	}
}

int	tcpclv4_session_tick(tcpclv4_session_t *s)
{
	time_t	now;
	uint8_t	buf[4];
	size_t	len;

	if (s == NULL || s->state == TCPCLV4_SESS_TERMINATED)
	{
		return -1;
	}

	if (s->session_keepalive == 0
	|| s->state == TCPCLV4_SESS_NEGOTIATING)
	{
		return 0;
	}

	now = time(NULL);

	/*	Idle timeout: no traffic for 2x keepalive interval.	*/

	if (now - s->last_recv_time >= (time_t)(2 * s->session_keepalive))
	{
		set_error(s, "Session idle timeout");
		oK(send_sess_term(s, TCPCLV4_TERM_IDLE_TIMEOUT, 0));
		terminate_session(s, TCPCLV4_TERM_IDLE_TIMEOUT);
		return -1;
	}

	/*	Send KEEPALIVE when the interval elapses (§5.1.2).
	 *	Skip it if the output queue is already backed up: a
	 *	keepalive cannot help a peer that is not draining us,
	 *	and the idle timeout above is the real liveness check.	*/

	if (now - s->last_send_time >= (time_t) s->session_keepalive
	&& s->out_len < OUT_QUEUE_WATERMARK)
	{
		if (tcpclv4_encode_keepalive(buf, sizeof(buf), &len) == 0)
		{
			if (send_raw(s, buf, len) < 0)
			{
				terminate_session(s, TCPCLV4_TERM_UNKNOWN);
				return -1;
			}
		}
	}

	return 0;
}

/*	==============================================================
 *	OUTPUT QUEUE (event loop integration)
 *	==============================================================
 */

int	tcpclv4_session_wants_write(tcpclv4_session_t *s)
{
	return (s != NULL && s->out_len > 0) ? 1 : 0;
}

int	tcpclv4_session_handle_output(tcpclv4_session_t *s)
{
	if (s == NULL || s->transport == NULL
	|| s->state == TCPCLV4_SESS_TERMINATED)
	{
		return -1;
	}

	if (flush_output(s) < 0)
	{
		terminate_session(s, TCPCLV4_TERM_UNKNOWN);
		return -1;
	}

	return 0;
}

int	tcpclv4_session_writable(tcpclv4_session_t *s)
{
	if (s == NULL || s->state != TCPCLV4_SESS_ESTABLISHED)
	{
		return 0;
	}

	return (s->out_len < OUT_QUEUE_WATERMARK) ? 1 : 0;
}

/*	==============================================================
 *	TRANSFERS
 *	==============================================================
 */

int	tcpclv4_session_send_bundle(tcpclv4_session_t *s,
		uint64_t xfer_id, const uint8_t *data, uint64_t len)
{
	uint64_t	seg_limit;
	uint64_t	offset;
	uint64_t	chunk;
	tx_transfer_t	*slot = NULL;
	int		i;

	if (s == NULL || data == NULL)
	{
		return -1;
	}

	if (s->state != TCPCLV4_SESS_ESTABLISHED)
	{
		set_error(s, "Session not established");
		return -1;
	}

	if (s->peer_transfer_mru && len > s->peer_transfer_mru)
	{
		set_error(s, "Bundle exceeds peer transfer MRU");
		return -1;
	}

	for (i = 0; i < TCPCLV4_SESSION_MAX_INFLIGHT; i++)
	{
		if (!s->inflight[i].in_use)
		{
			slot = &s->inflight[i];
			break;
		}
	}

	if (slot == NULL)
	{
		set_error(s, "Too many transfers in flight");
		return -1;
	}

	seg_limit = s->segment_mtu;
	if (s->peer_segment_mru && s->peer_segment_mru < seg_limit)
	{
		seg_limit = s->peer_segment_mru;
	}

	if (seg_limit == 0)
	{
		set_error(s, "Zero segment limit");
		return -1;
	}

	/*	Size the segment scratch buffer.			*/

	if (s->tx_msg_cap < seg_limit + SEGMENT_HDR_OVERHEAD)
	{
		/*	Cannot happen: tx_msg_buf is preallocated to
		 *	segment_mtu + header, and seg_limit is capped at
		 *	segment_mtu.  Defensive only.			*/

		set_error(s, "Segment buffer under-provisioned");
		return -1;
	}

	slot->xfer_id = xfer_id;
	slot->total_len = len;
	slot->acked_len = 0;
	slot->in_use = 1;

	offset = 0;
	do
	{
		tcpclv4_xfer_segment_t	seg;
		size_t			msg_len;

		chunk = len - offset;
		if (chunk > seg_limit)
		{
			chunk = seg_limit;
		}

		memset(&seg, 0, sizeof(seg));
		seg.transfer_id = xfer_id;
		seg.flags = 0;
		if (offset == 0)
		{
			seg.flags |= TCPCLV4_XFER_FLAG_START;
		}

		if (offset + chunk == len)
		{
			seg.flags |= TCPCLV4_XFER_FLAG_END;
		}

		seg.data = data + offset;
		seg.data_len = chunk;

		if (tcpclv4_encode_xfer_segment(&seg, s->tx_msg_buf,
				s->tx_msg_cap, &msg_len) < 0)
		{
			set_error(s, "Segment encode failed");
			slot->in_use = 0;
			return -1;
		}

		if (send_raw(s, s->tx_msg_buf, msg_len) < 0)
		{
			slot->in_use = 0;
			terminate_session(s, TCPCLV4_TERM_UNKNOWN);
			return -1;
		}

		offset += chunk;
	} while (offset < len);

	return 0;
}

int	tcpclv4_session_inflight_count(tcpclv4_session_t *s)
{
	int	i;
	int	count = 0;

	if (s == NULL)
	{
		return 0;
	}

	for (i = 0; i < TCPCLV4_SESSION_MAX_INFLIGHT; i++)
	{
		if (s->inflight[i].in_use)
		{
			count++;
		}
	}

	return count;
}

/*	==============================================================
 *	TERMINATION
 *	==============================================================
 */

int	tcpclv4_session_terminate(tcpclv4_session_t *s, uint8_t reason_code)
{
	if (s == NULL || s->state == TCPCLV4_SESS_TERMINATED)
	{
		return -1;
	}

	if (s->state == TCPCLV4_SESS_ENDING)
	{
		return 0;	/* Already sent */
	}

	s->term_sent = 1;
	s->term_reason = reason_code;

	if (send_sess_term(s, reason_code, 0) < 0)
	{
		terminate_session(s, reason_code);
		return -1;
	}

	s->state = TCPCLV4_SESS_ENDING;
	return 0;
}

void	tcpclv4_session_close(tcpclv4_session_t *s)
{
	if (s == NULL)
	{
		return;
	}

	if (s->transport != NULL)
	{
		tcpclv4_tls_close(s->transport);
	}

	free(s->peer_node_id);
	free(s->rx_buf);
	free(s->tx_msg_buf);
	free(s->rx_msg_buf);
	free(s->out_buf);
	free(s);
}

/*	==============================================================
 *	INTROSPECTION
 *	==============================================================
 */

tcpclv4_session_state_t	tcpclv4_session_state(tcpclv4_session_t *s)
{
	return s ? s->state : TCPCLV4_SESS_TERMINATED;
}

const char	*tcpclv4_session_peer_node_id(tcpclv4_session_t *s)
{
	return s ? s->peer_node_id : NULL;
}

int	tcpclv4_session_is_secure(tcpclv4_session_t *s)
{
	if (s == NULL || s->transport == NULL)
	{
		return 0;
	}

	return tcpclv4_tls_is_secure(s->transport);
}

const char	*tcpclv4_session_error(tcpclv4_session_t *s)
{
	if (s == NULL)
	{
		return NULL;
	}

	return s->error_msg[0] ? s->error_msg : NULL;
}
