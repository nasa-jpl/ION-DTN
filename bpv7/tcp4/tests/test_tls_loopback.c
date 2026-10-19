/*
	test_tls_loopback.c: Unit test for TCPCLv4 TLS library.

			Tests TCP connection, contact header exchange,
			and message send/recv on localhost loopback.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "../tcpclv4_tls.h"
#include "../tcpclv4_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TEST_PORT 14556
#define TEST_BUFFER_SIZE 65536

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

/*	Simple TCP server for testing					*/

static int create_listen_socket(int port)
{
	int			listen_fd;
	struct sockaddr_in	addr;
	int			opt = 1;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		return -1;
	}

	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		close(listen_fd);
		return -1;
	}

	if (listen(listen_fd, 1) < 0)
	{
		close(listen_fd);
		return -1;
	}

	return listen_fd;
}

/*	Server thread for loopback test				*/

typedef struct
{
	int	port;
	int	ready;
	int	result;
} server_args_t;

static void* server_thread(void *arg)
{
	server_args_t		*args = (server_args_t *)arg;
	int			listen_fd;
	int			client_fd;
	tcpclv4_tls_session_t	*sess;
	uint8_t			buf[TEST_BUFFER_SIZE];
	size_t			len;
	int			ret;

	/* Create listen socket */
	listen_fd = create_listen_socket(args->port);
	if (listen_fd < 0)
	{
		args->result = -1;
		return NULL;
	}

	args->ready = 1;	/* Signal client we're ready */

	/* Accept connection */
	client_fd = accept(listen_fd, NULL, NULL);
	close(listen_fd);

	if (client_fd < 0)
	{
		args->result = -1;
		return NULL;
	}

	/* Create session (no TLS for this basic test) */
	sess = tcpclv4_tls_accept(client_fd, "ipn:3.0", NULL);
	if (sess == NULL)
	{
		close(client_fd);
		args->result = -1;
		return NULL;
	}

	/* Receive SESS_INIT */
	ret = tcpclv4_tls_recv_msg(sess, buf, sizeof(buf), &len);
	if (ret != 1)
	{
		tcpclv4_tls_close(sess);
		args->result = -1;
		return NULL;
	}

	/* Verify it's a SESS_INIT */
	if (tcpclv4_get_msg_type(buf, len) != TCPCLV4_MSG_SESS_INIT)
	{
		tcpclv4_tls_close(sess);
		args->result = -1;
		return NULL;
	}

	/* Send KEEPALIVE response */
	ret = tcpclv4_encode_keepalive(buf, sizeof(buf), &len);
	if (ret < 0)
	{
		tcpclv4_tls_close(sess);
		args->result = -1;
		return NULL;
	}

	ret = tcpclv4_tls_send_msg(sess, buf, len);
	if (ret < 0)
	{
		tcpclv4_tls_close(sess);
		args->result = -1;
		return NULL;
	}

	/* Close session */
	tcpclv4_tls_close(sess);

	args->result = 0;
	return NULL;
}

/*	Test: Library initialization					*/

static int test_initialization(void)
{
	TEST("Library initialization");

	/* These should be safe to call multiple times */
	tcpclv4_tls_init();
	tcpclv4_tls_init();

	/* Test error handling with NULL session */
	if (tcpclv4_tls_get_fd(NULL) != -1)
	{
		FAIL("get_fd should return -1 for NULL");
	}

	if (tcpclv4_tls_get_peer_nodeid(NULL) != NULL)
	{
		FAIL("get_peer_nodeid should return NULL for NULL");
	}

	if (tcpclv4_tls_is_secure(NULL) != 0)
	{
		FAIL("is_secure should return 0 for NULL");
	}

	/* Close with NULL should not crash */
	tcpclv4_tls_close(NULL);

	PASS();
	return 0;
}

/*	Test: Message send/recv						*/

static int test_message_exchange(void)
{
	tcpclv4_tls_session_t	*sess;
	server_args_t		server_args;
	pthread_t		server_tid;
	uint8_t			buf[TEST_BUFFER_SIZE];
	size_t			len;
	int			ret;
	int			i;

	TEST("Message send/recv");

	/* Start server thread */
	memset(&server_args, 0, sizeof(server_args));
	server_args.port = TEST_PORT + 1;
	server_args.ready = 0;

	if (pthread_create(&server_tid, NULL, server_thread,
			   &server_args) != 0)
	{
		FAIL("Failed to create server thread");
	}

	/* Wait for server */
	for (i = 0; i < 50; i++)
	{
		if (server_args.ready)
		{
			break;
		}

		usleep(10000);
	}

	if (!server_args.ready)
	{
		FAIL("Server not ready");
	}

	/* Connect */
	sess = tcpclv4_tls_connect("127.0.0.1", TEST_PORT + 1,
				    "ipn:2.0", NULL);
	if (sess == NULL)
	{
		pthread_join(server_tid, NULL);
		FAIL("Failed to connect");
	}

	/* Send SESS_INIT */
	ret = tcpclv4_encode_sess_init(
		&(tcpclv4_sess_init_t){
			.keepalive_interval = 30,
			.segment_mru = 1048576,
			.transfer_mru = 10485760,
			.node_id = "ipn:2.0",
			.node_id_len = 8,
			.extensions = NULL,
			.extensions_len = 0
		},
		buf, sizeof(buf), &len);

	if (ret < 0)
	{
		tcpclv4_tls_close(sess);
		pthread_join(server_tid, NULL);
		FAIL("Failed to encode SESS_INIT");
	}

	ret = tcpclv4_tls_send_msg(sess, buf, len);
	if (ret < 0)
	{
		tcpclv4_tls_close(sess);
		pthread_join(server_tid, NULL);
		FAIL("Failed to send SESS_INIT");
	}

	/* Receive KEEPALIVE response */
	ret = tcpclv4_tls_recv_msg(sess, buf, sizeof(buf), &len);
	if (ret != 1)
	{
		tcpclv4_tls_close(sess);
		pthread_join(server_tid, NULL);
		FAIL("Failed to receive KEEPALIVE");
	}

	/* Verify message type */
	if (tcpclv4_get_msg_type(buf, len) != TCPCLV4_MSG_KEEPALIVE)
	{
		tcpclv4_tls_close(sess);
		pthread_join(server_tid, NULL);
		FAIL("Wrong message type");
	}

	tcpclv4_tls_close(sess);
	pthread_join(server_tid, NULL);

	if (server_args.result != 0)
	{
		FAIL("Server thread failed");
	}

	PASS();
	return 0;
}

/*	Test: Non-blocking mode						*/

static int test_nonblocking(void)
{
	tcpclv4_tls_session_t	*sess;
	server_args_t		server_args;
	pthread_t		server_tid;
	int			i;

	TEST("Non-blocking mode");

	/* Start server thread */
	memset(&server_args, 0, sizeof(server_args));
	server_args.port = TEST_PORT + 2;
	server_args.ready = 0;

	if (pthread_create(&server_tid, NULL, server_thread,
			   &server_args) != 0)
	{
		FAIL("Failed to create server thread");
	}

	/* Wait for server */
	for (i = 0; i < 50; i++)
	{
		if (server_args.ready)
		{
			break;
		}

		usleep(10000);
	}

	if (!server_args.ready)
	{
		FAIL("Server not ready");
	}

	/* Connect */
	sess = tcpclv4_tls_connect("127.0.0.1", TEST_PORT + 2,
				    "ipn:2.0", NULL);
	if (sess == NULL)
	{
		pthread_join(server_tid, NULL);
		FAIL("Failed to connect");
	}

	/* Set non-blocking */
	if (tcpclv4_tls_set_nonblocking(sess, 1) < 0)
	{
		tcpclv4_tls_close(sess);
		pthread_join(server_tid, NULL);
		FAIL("Failed to set non-blocking");
	}

	/* Close immediately (server will handle gracefully) */
	tcpclv4_tls_close(sess);
	pthread_join(server_tid, NULL);

	PASS();
	return 0;
}

/*	Main test runner						*/

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	printf("TCPCLv4 TLS Library Tests (Plain TCP mode)\n");
	printf("===========================================\n\n");

	tcpclv4_tls_init();

	test_initialization();
	test_message_exchange();
	test_nonblocking();

	tcpclv4_tls_cleanup();

	printf("\n===========================================\n");
	printf("Tests: %d/%d passed\n", tests_passed, tests_run);

	if (tests_passed == tests_run)
	{
		printf("ALL TESTS PASSED\n");
		return 0;
	}
	else
	{
		printf("SOME TESTS FAILED\n");
		return 1;
	}
}
