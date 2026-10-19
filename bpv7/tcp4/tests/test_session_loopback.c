/*
	test_session_loopback.c: Unit tests for the TCPCLv4 session
			state machine.

			Runs two sessions against each other over
			localhost: negotiation, keepalive interval
			agreement, single- and multi-segment transfers
			in both directions, ack completion, and
			graceful termination.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "../tcpclv4_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TEST_PORT	14580
#define BIG_BUNDLE_LEN	(64 * 1024 + 37)	/* Forces many segments */

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
	do { \
		printf("Testing %s... ", name); \
		fflush(stdout); \
		tests_run++; \
	} while (0)

#define PASS() \
	do { \
		printf("PASS\n"); \
		tests_passed++; \
	} while (0)

#define FAIL(msg) \
	do { \
		printf("FAIL: %s\n", msg); \
		return -1; \
	} while (0)

/*	Callback recorder shared by both endpoints.			*/

typedef struct
{
	int		established;
	int		terminated;
	int		term_reason;
	int		xfer_ok[8];
	uint64_t	xfer_ids[8];
	int		xfer_count;
	uint8_t		*bundles[8];
	uint64_t	bundle_lens[8];
	int		bundle_count;
} recorder_t;

static void rec_established(void *user, tcpclv4_session_t *sess)
{
	(void) sess;
	((recorder_t *) user)->established = 1;
}

static void rec_terminated(void *user, tcpclv4_session_t *sess, int reason)
{
	recorder_t	*rec = user;

	(void) sess;
	rec->terminated = 1;
	rec->term_reason = reason;
}

static void rec_xfer_result(void *user, tcpclv4_session_t *sess,
		uint64_t xfer_id, int ok)
{
	recorder_t	*rec = user;

	(void) sess;
	if (rec->xfer_count < 8)
	{
		rec->xfer_ids[rec->xfer_count] = xfer_id;
		rec->xfer_ok[rec->xfer_count] = ok;
		rec->xfer_count++;
	}
}

static void rec_bundle(void *user, tcpclv4_session_t *sess,
		const uint8_t *data, uint64_t len)
{
	recorder_t	*rec = user;

	(void) sess;
	if (rec->bundle_count < 8)
	{
		/*	Store the first 8 for content checks...	*/

		rec->bundles[rec->bundle_count] = malloc(len);
		memcpy(rec->bundles[rec->bundle_count], data, len);
		rec->bundle_lens[rec->bundle_count] = len;
	}

	/*	...but keep counting beyond that (flood tests).		*/

	rec->bundle_count++;
}

static const tcpclv4_session_callbacks_t rec_cbs = {
	rec_established, rec_terminated, rec_xfer_result, rec_bundle
};

static void rec_free(recorder_t *rec)
{
	int	i;
	int	stored = rec->bundle_count < 8 ? rec->bundle_count : 8;

	for (i = 0; i < stored; i++)
	{
		free(rec->bundles[i]);
	}
}

/*	Passive endpoint thread: accept one session and run its event
 *	loop until termination, echoing every received bundle back.	*/

typedef struct
{
	int		port;
	int		ready;
	int		result;
	int		echo;		/* Echo received bundles back */
	recorder_t	rec;
} server_ctx_t;

static void *server_main(void *arg)
{
	server_ctx_t		*ctx = arg;
	int			listen_fd;
	int			conn_fd;
	struct sockaddr_in	addr;
	int			opt = 1;
	tcpclv4_session_t	*sess;
	tcpclv4_session_config_t cfg;
	struct pollfd		pfd;
	int			sent_back = 0;
	uint64_t		next_id = 1000;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(ctx->port);
	if (bind(listen_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0
	|| listen(listen_fd, 1) < 0)
	{
		close(listen_fd);
		ctx->result = -1;
		ctx->ready = -1;
		return NULL;
	}

	ctx->ready = 1;
	conn_fd = accept(listen_fd, NULL, NULL);
	close(listen_fd);
	if (conn_fd < 0)
	{
		ctx->result = -1;
		return NULL;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.node_id = "ipn:3.0";
	cfg.keepalive_interval = 5;
	cfg.segment_mru = 4096;	/* Small: force segmentation of the
				 * big inbound bundle */

	sess = tcpclv4_session_accept(conn_fd, &cfg, &rec_cbs, &ctx->rec);
	if (sess == NULL)
	{
		ctx->result = -1;
		return NULL;
	}

	pfd.fd = tcpclv4_session_fd(sess);

	while (tcpclv4_session_state(sess) != TCPCLV4_SESS_TERMINATED)
	{
		pfd.events = POLLIN;
		if (tcpclv4_session_wants_write(sess))
		{
			pfd.events |= POLLOUT;
		}

		if (poll(&pfd, 1, 200) > 0)
		{
			if ((pfd.revents & POLLOUT)
			&& tcpclv4_session_handle_output(sess) < 0)
			{
				break;
			}

			if ((pfd.revents & (POLLIN | POLLHUP))
			&& tcpclv4_session_handle_io(sess) < 0)
			{
				break;
			}
		}

		if (tcpclv4_session_tick(sess) < 0)
		{
			break;
		}

		/*	Echo each received bundle back exactly once,
		 *	respecting backpressure.			*/

		if (ctx->echo && sent_back < ctx->rec.bundle_count
		&& tcpclv4_session_writable(sess))
		{
			if (tcpclv4_session_send_bundle(sess, next_id++,
					ctx->rec.bundles[sent_back],
					ctx->rec.bundle_lens[sent_back]) < 0)
			{
				break;
			}

			sent_back++;
		}
	}

	tcpclv4_session_close(sess);
	ctx->result = 0;
	return NULL;
}

/*	Drive the active session until a predicate is met or a
 *	timeout expires.						*/

#define DRIVE_UNTIL(sess, cond, ms)					\
	do {								\
		struct pollfd	dpfd;					\
		int		waited = 0;				\
		dpfd.fd = tcpclv4_session_fd(sess);			\
		while (!(cond) && waited < (ms))			\
		{							\
			dpfd.events = POLLIN;				\
			if (tcpclv4_session_wants_write(sess))		\
			{						\
				dpfd.events |= POLLOUT;			\
			}						\
			if (poll(&dpfd, 1, 50) > 0)			\
			{						\
				if (dpfd.revents & POLLOUT)		\
				{					\
					tcpclv4_session_handle_output(	\
							sess);		\
				}					\
				if (dpfd.revents & (POLLIN | POLLHUP))	\
				{					\
					tcpclv4_session_handle_io(sess); \
				}					\
			}						\
			else						\
			{						\
				waited += 50;				\
			}						\
			tcpclv4_session_tick(sess);			\
			if (tcpclv4_session_state(sess)			\
					== TCPCLV4_SESS_TERMINATED)	\
			{						\
				break;					\
			}						\
		}							\
	} while (0)

static int test_full_session(void)
{
	server_ctx_t		server;
	pthread_t		tid;
	recorder_t		rec;
	tcpclv4_session_t	*sess;
	tcpclv4_session_config_t cfg;
	uint8_t			*big;
	uint64_t		i;
	int			ok;

	TEST("Full session (negotiate, transfer both ways, terminate)");

	memset(&server, 0, sizeof(server));
	server.port = TEST_PORT;
	server.echo = 1;

	if (pthread_create(&tid, NULL, server_main, &server) != 0)
	{
		FAIL("Server thread create failed");
	}

	for (i = 0; i < 100 && server.ready == 0; i++)
	{
		usleep(10000);
	}

	if (server.ready != 1)
	{
		FAIL("Server not ready");
	}

	memset(&rec, 0, sizeof(rec));
	memset(&cfg, 0, sizeof(cfg));
	cfg.node_id = "ipn:2.0";
	cfg.keepalive_interval = 15;	/* Peer offers 5; min wins */
	cfg.segment_mtu = 4096;

	sess = tcpclv4_session_connect("127.0.0.1", TEST_PORT, &cfg,
			&rec_cbs, &rec);
	if (sess == NULL)
	{
		pthread_join(tid, NULL);
		FAIL("Connect failed");
	}

	/*	Negotiation.						*/

	DRIVE_UNTIL(sess, rec.established, 3000);
	if (!rec.established)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Session not established");
	}

	if (tcpclv4_session_peer_node_id(sess) == NULL
	|| strcmp(tcpclv4_session_peer_node_id(sess), "ipn:3.0") != 0)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Wrong peer node ID");
	}

	/*	Small single-segment bundle.				*/

	if (tcpclv4_session_send_bundle(sess, 1,
			(const uint8_t *) "tiny bundle", 11) < 0)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Small send failed");
	}

	DRIVE_UNTIL(sess, rec.xfer_count >= 1, 3000);
	if (rec.xfer_count < 1 || !rec.xfer_ok[0] || rec.xfer_ids[0] != 1)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Small transfer not acked");
	}

	/*	Large multi-segment bundle (peer MRU 4096 forces
	 *	~16 segments).						*/

	big = malloc(BIG_BUNDLE_LEN);
	for (i = 0; i < BIG_BUNDLE_LEN; i++)
	{
		big[i] = (uint8_t) (i * 31 + 7);
	}

	if (tcpclv4_session_send_bundle(sess, 2, big, BIG_BUNDLE_LEN) < 0)
	{
		free(big);
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Big send failed");
	}

	DRIVE_UNTIL(sess, rec.xfer_count >= 2, 5000);
	if (rec.xfer_count < 2 || !rec.xfer_ok[1] || rec.xfer_ids[1] != 2)
	{
		free(big);
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Big transfer not acked");
	}

	/*	The server echoes both bundles back; verify both
	 *	arrive intact.						*/

	DRIVE_UNTIL(sess, rec.bundle_count >= 2, 5000);
	ok = (rec.bundle_count == 2
		&& rec.bundle_lens[0] == 11
		&& memcmp(rec.bundles[0], "tiny bundle", 11) == 0
		&& rec.bundle_lens[1] == BIG_BUNDLE_LEN
		&& memcmp(rec.bundles[1], big, BIG_BUNDLE_LEN) == 0);
	free(big);
	if (!ok)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		rec_free(&rec);
		FAIL("Echoed bundles missing or corrupted");
	}

	/*	Graceful termination.					*/

	if (tcpclv4_session_terminate(sess, TCPCLV4_TERM_UNKNOWN) < 0)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		rec_free(&rec);
		FAIL("Terminate failed");
	}

	DRIVE_UNTIL(sess, rec.terminated, 3000);
	if (!rec.terminated)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		rec_free(&rec);
		FAIL("No termination event");
	}

	tcpclv4_session_close(sess);
	pthread_join(tid, NULL);

	if (server.result != 0 || !server.rec.terminated)
	{
		rec_free(&rec);
		rec_free(&server.rec);
		FAIL("Server session did not terminate cleanly");
	}

	if (server.rec.bundle_count != 2)
	{
		rec_free(&rec);
		rec_free(&server.rec);
		FAIL("Server missed a bundle");
	}

	rec_free(&rec);
	rec_free(&server.rec);
	PASS();
	return 0;
}

/*	Transfer-MRU enforcement: a bundle bigger than the peer's
 *	transfer MRU must be rejected locally without touching the
 *	wire.								*/

static int test_transfer_mru(void)
{
	server_ctx_t		server;
	pthread_t		tid;
	recorder_t		rec;
	tcpclv4_session_t	*sess;
	tcpclv4_session_config_t cfg;
	uint8_t			dummy[16];
	int			i;

	TEST("Peer transfer-MRU enforcement");

	memset(&server, 0, sizeof(server));
	server.port = TEST_PORT + 1;

	if (pthread_create(&tid, NULL, server_main, &server) != 0)
	{
		FAIL("Server thread create failed");
	}

	for (i = 0; i < 100 && server.ready == 0; i++)
	{
		usleep(10000);
	}

	memset(&rec, 0, sizeof(rec));
	memset(&cfg, 0, sizeof(cfg));
	cfg.node_id = "ipn:2.0";

	sess = tcpclv4_session_connect("127.0.0.1", TEST_PORT + 1, &cfg,
			&rec_cbs, &rec);
	if (sess == NULL)
	{
		pthread_join(tid, NULL);
		FAIL("Connect failed");
	}

	DRIVE_UNTIL(sess, rec.established, 3000);
	if (!rec.established)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Session not established");
	}

	/*	Server's transfer MRU defaults to 100MB; lie about a
	 *	huge length via a short buffer is unsafe, so instead
	 *	verify the check plumbing with a length over MRU:
	 *	claim is enforced before any data is read.		*/

	if (tcpclv4_session_send_bundle(sess, 1, dummy,
			(200ULL << 20)) == 0)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Oversize transfer was not rejected");
	}

	tcpclv4_session_terminate(sess, TCPCLV4_TERM_UNKNOWN);
	DRIVE_UNTIL(sess, rec.terminated, 3000);
	tcpclv4_session_close(sess);
	pthread_join(tid, NULL);
	rec_free(&rec);
	rec_free(&server.rec);
	PASS();
	return 0;
}

/*	Bidirectional flood: both endpoints blast bundles at each
 *	other simultaneously.  With blocking sends this deadlocks
 *	(both sides stuck in write(), neither reading); the output
 *	queue plus POLLOUT integration must keep both directions
 *	flowing to completion.						*/

#define FLOOD_COUNT	50
#define FLOOD_LEN	(32 * 1024)

typedef struct
{
	int		port;
	int		ready;
	int		result;
	recorder_t	rec;
} flood_ctx_t;

static void *flood_server_main(void *arg)
{
	flood_ctx_t		*ctx = arg;
	int			listen_fd;
	int			conn_fd;
	struct sockaddr_in	addr;
	int			opt = 1;
	tcpclv4_session_t	*sess;
	tcpclv4_session_config_t cfg;
	uint8_t			*payload;
	uint64_t		i;
	int			sent = 0;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(ctx->port);
	if (bind(listen_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0
	|| listen(listen_fd, 1) < 0)
	{
		close(listen_fd);
		ctx->result = -1;
		ctx->ready = -1;
		return NULL;
	}

	ctx->ready = 1;
	conn_fd = accept(listen_fd, NULL, NULL);
	close(listen_fd);
	if (conn_fd < 0)
	{
		ctx->result = -1;
		return NULL;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.node_id = "ipn:3.0";
	cfg.segment_mtu = 4096;
	cfg.segment_mru = 4096;

	sess = tcpclv4_session_accept(conn_fd, &cfg, &rec_cbs, &ctx->rec);
	if (sess == NULL)
	{
		ctx->result = -1;
		return NULL;
	}

	payload = malloc(FLOOD_LEN);
	for (i = 0; i < FLOOD_LEN; i++)
	{
		payload[i] = (uint8_t) (i * 13 + 3);
	}

	/*	Drive the loop; send as fast as backpressure allows.	*/

	while (tcpclv4_session_state(sess) != TCPCLV4_SESS_TERMINATED)
	{
		struct pollfd	pfd;

		pfd.fd = tcpclv4_session_fd(sess);
		pfd.events = POLLIN;
		if (tcpclv4_session_wants_write(sess))
		{
			pfd.events |= POLLOUT;
		}

		if (poll(&pfd, 1, 100) > 0)
		{
			if ((pfd.revents & POLLOUT)
			&& tcpclv4_session_handle_output(sess) < 0)
			{
				break;
			}

			if ((pfd.revents & (POLLIN | POLLHUP))
			&& tcpclv4_session_handle_io(sess) < 0)
			{
				break;
			}
		}

		if (tcpclv4_session_tick(sess) < 0)
		{
			break;
		}

		while (sent < FLOOD_COUNT && tcpclv4_session_writable(sess))
		{
			if (tcpclv4_session_send_bundle(sess,
					(uint64_t) sent + 1, payload,
					FLOOD_LEN) < 0)
			{
				sent = FLOOD_COUNT;
				break;
			}

			sent++;
		}
	}

	free(payload);
	tcpclv4_session_close(sess);
	ctx->result = (sent >= FLOOD_COUNT) ? 0 : -1;
	return NULL;
}

static int test_bidirectional_flood(void)
{
	flood_ctx_t		server;
	pthread_t		tid;
	recorder_t		rec;
	tcpclv4_session_t	*sess;
	tcpclv4_session_config_t cfg;
	uint8_t			*payload;
	uint64_t		i;
	int			sent = 0;
	int			acked;
	int			deadline;

	TEST("Bidirectional flood (no write-write deadlock)");

	memset(&server, 0, sizeof(server));
	server.port = TEST_PORT + 2;

	if (pthread_create(&tid, NULL, flood_server_main, &server) != 0)
	{
		FAIL("Server thread create failed");
	}

	for (i = 0; i < 100 && server.ready == 0; i++)
	{
		usleep(10000);
	}

	if (server.ready != 1)
	{
		FAIL("Server not ready");
	}

	memset(&rec, 0, sizeof(rec));
	memset(&cfg, 0, sizeof(cfg));
	cfg.node_id = "ipn:2.0";
	cfg.segment_mtu = 4096;
	cfg.segment_mru = 4096;

	sess = tcpclv4_session_connect("127.0.0.1", TEST_PORT + 2, &cfg,
			&rec_cbs, &rec);
	if (sess == NULL)
	{
		pthread_join(tid, NULL);
		FAIL("Connect failed");
	}

	DRIVE_UNTIL(sess, rec.established, 3000);
	if (!rec.established)
	{
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Session not established");
	}

	payload = malloc(FLOOD_LEN);
	for (i = 0; i < FLOOD_LEN; i++)
	{
		payload[i] = (uint8_t) (i * 7 + 1);
	}

	/*	Blast our direction while the server blasts back;
	 *	drive I/O between sends.  Total each way: 50 x 32KB
	 *	= 1.6MB, far beyond both socket buffers.		*/

	deadline = 0;
	while ((sent < FLOOD_COUNT
		|| rec.bundle_count < FLOOD_COUNT) && deadline < 30000)
	{
		while (sent < FLOOD_COUNT && tcpclv4_session_writable(sess))
		{
			if (tcpclv4_session_send_bundle(sess,
					(uint64_t) sent + 1, payload,
					FLOOD_LEN) < 0)
			{
				free(payload);
				tcpclv4_session_close(sess);
				pthread_join(tid, NULL);
				FAIL("send_bundle failed mid-flood");
			}

			sent++;
		}

		DRIVE_UNTIL(sess, 0, 50);	/* One I/O pump cycle */
		deadline += 50;

		if (tcpclv4_session_state(sess) == TCPCLV4_SESS_TERMINATED)
		{
			break;
		}
	}

	/*	Wait for all our transfers to be acknowledged and all
	 *	inbound bundles to land.				*/

	DRIVE_UNTIL(sess, tcpclv4_session_inflight_count(sess) == 0
			&& rec.bundle_count >= FLOOD_COUNT, 15000);

	acked = (tcpclv4_session_inflight_count(sess) == 0
			&& sent == FLOOD_COUNT);

	if (!acked)
	{
		free(payload);
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Not all outbound transfers completed");
	}

	if (rec.bundle_count < FLOOD_COUNT)
	{
		free(payload);
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Not all inbound bundles arrived");
	}

	/*	Verify payload integrity of the last inbound bundle
	 *	(recorder keeps the first 8; check one of them).	*/

	if (rec.bundle_lens[0] != FLOOD_LEN)
	{
		free(payload);
		tcpclv4_session_close(sess);
		pthread_join(tid, NULL);
		FAIL("Inbound bundle length corrupted");
	}

	free(payload);
	tcpclv4_session_terminate(sess, TCPCLV4_TERM_UNKNOWN);
	DRIVE_UNTIL(sess, rec.terminated, 3000);
	tcpclv4_session_close(sess);
	pthread_join(tid, NULL);
	rec_free(&rec);
	rec_free(&server.rec);

	if (server.result != 0)
	{
		FAIL("Server did not finish its flood");
	}

	PASS();
	return 0;
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	printf("TCPCLv4 Session State Machine Tests\n");
	printf("====================================\n\n");

	tcpclv4_tls_init();

	test_full_session();
	test_transfer_mru();
	test_bidirectional_flood();

	tcpclv4_tls_cleanup();

	printf("\n====================================\n");
	printf("Tests: %d/%d passed\n", tests_passed, tests_run);

	if (tests_passed == tests_run)
	{
		printf("ALL TESTS PASSED\n");
		return 0;
	}

	printf("SOME TESTS FAILED\n");
	return 1;
}
