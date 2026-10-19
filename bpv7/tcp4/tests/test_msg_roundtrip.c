/*
	test_msg_roundtrip.c: Unit tests for TCPCLv4 message encode/decode.

			Tests round-trip encode/decode for all message
			types to verify wire format compatibility.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "../tcpclv4_msg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define TEST_BUFFER_SIZE 65536

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
	do { \
		printf("Testing %s... ", name); \
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

/*	Test Contact Header						*/

static int test_contact_header(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_contact_header_t msg;
	tcpclv4_contact_header_t decoded;
	size_t			len;

	TEST("Contact Header");

	/* Encode */
	msg.magic = TCPCLV4_CONTACT_MAGIC;
	msg.flags = TCPCLV4_CONTACT_FLAG_CAN_TLS;

	if (tcpclv4_encode_contact_header(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 4)
	{
		FAIL("Wrong encoded length");
	}

	/* Decode */
	if (tcpclv4_decode_contact_header(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.magic != TCPCLV4_CONTACT_MAGIC)
	{
		FAIL("Magic mismatch");
	}

	PASS();
	return 0;
}

/*	Test SESS_INIT							*/

static int test_sess_init(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_sess_init_t	msg;
	tcpclv4_sess_init_t	decoded;
	size_t			len;
	const char		*node_id = "ipn:2.0";

	TEST("SESS_INIT");

	/* Encode */
	memset(&msg, 0, sizeof(msg));
	msg.keepalive_interval = 30;
	msg.segment_mru = 1048576;	/* 1 MB */
	msg.transfer_mru = 10485760;	/* 10 MB */
	msg.node_id = (char *)node_id;
	msg.node_id_len = strlen(node_id);
	msg.extensions = NULL;
	msg.extensions_len = 0;

	if (tcpclv4_encode_sess_init(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	/* Decode */
	memset(&decoded, 0, sizeof(decoded));
	if (tcpclv4_decode_sess_init(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	/* Verify */
	if (decoded.keepalive_interval != msg.keepalive_interval)
	{
		FAIL("keepalive_interval mismatch");
	}

	if (decoded.segment_mru != msg.segment_mru)
	{
		FAIL("segment_mru mismatch");
	}

	if (decoded.transfer_mru != msg.transfer_mru)
	{
		FAIL("transfer_mru mismatch");
	}

	if (decoded.node_id_len != msg.node_id_len)
	{
		FAIL("node_id_len mismatch");
	}

	if (strcmp(decoded.node_id, msg.node_id) != 0)
	{
		FAIL("node_id mismatch");
	}

	tcpclv4_sess_init_free(&decoded);

	PASS();
	return 0;
}

/*	Test SESS_TERM							*/

static int test_sess_term(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_sess_term_t	msg;
	tcpclv4_sess_term_t	decoded;
	size_t			len;

	TEST("SESS_TERM");

	/* Encode */
	msg.flags = 0;
	msg.reason_code = TCPCLV4_TERM_IDLE_TIMEOUT;

	if (tcpclv4_encode_sess_term(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 3)
	{
		FAIL("Wrong encoded length");
	}

	/* Decode */
	if (tcpclv4_decode_sess_term(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.reason_code != msg.reason_code)
	{
		FAIL("reason_code mismatch");
	}

	PASS();
	return 0;
}

/*	Test XFER_SEGMENT (START)					*/

static int test_xfer_segment_start(void)
{
	uint8_t				buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_segment_t		msg;
	tcpclv4_xfer_segment_t		decoded;
	size_t				len;
	const uint8_t			payload[] = "Test payload data";

	TEST("XFER_SEGMENT (START)");

	/* Encode */
	memset(&msg, 0, sizeof(msg));
	msg.flags = TCPCLV4_XFER_FLAG_START;
	msg.transfer_id = 12345;
	msg.data_offset = 0;
	msg.extensions = NULL;
	msg.extensions_len = 0;
	msg.data = payload;
	msg.data_len = sizeof(payload);

	if (tcpclv4_encode_xfer_segment(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	/* Decode */
	memset(&decoded, 0, sizeof(decoded));
	if (tcpclv4_decode_xfer_segment(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	/* Verify */
	if (decoded.flags != msg.flags)
	{
		FAIL("flags mismatch");
	}

	if (decoded.transfer_id != msg.transfer_id)
	{
		FAIL("transfer_id mismatch");
	}

	if (decoded.data_len != msg.data_len)
	{
		FAIL("data_len mismatch");
	}

	if (memcmp(decoded.data, msg.data, msg.data_len) != 0)
	{
		FAIL("data mismatch");
	}

	tcpclv4_xfer_segment_free(&decoded);

	PASS();
	return 0;
}

/*	Test XFER_SEGMENT (non-START with data_offset)			*/

static int test_xfer_segment_continuation(void)
{
	uint8_t				buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_segment_t		msg;
	tcpclv4_xfer_segment_t		decoded;
	size_t				len;
	const uint8_t			payload[] = "More data";

	TEST("XFER_SEGMENT (continuation)");

	/* Encode */
	memset(&msg, 0, sizeof(msg));
	msg.flags = 0;			/* Not START, not END */
	msg.transfer_id = 12345;
	msg.data_offset = 1024;		/* Continuation at offset 1024 */
	msg.extensions = NULL;
	msg.extensions_len = 0;
	msg.data = payload;
	msg.data_len = sizeof(payload);

	if (tcpclv4_encode_xfer_segment(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	/* Decode */
	memset(&decoded, 0, sizeof(decoded));
	if (tcpclv4_decode_xfer_segment(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	/* Verify */
	if (decoded.data_offset != msg.data_offset)
	{
		FAIL("data_offset mismatch");
	}

	if (decoded.transfer_id != msg.transfer_id)
	{
		FAIL("transfer_id mismatch");
	}

	tcpclv4_xfer_segment_free(&decoded);

	PASS();
	return 0;
}

/*	Test XFER_ACK							*/

static int test_xfer_ack(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_ack_t	msg;
	tcpclv4_xfer_ack_t	decoded;
	size_t			len;

	TEST("XFER_ACK");

	/* Encode */
	msg.flags = 0;
	msg.transfer_id = 12345;
	msg.ack_len = 1024;

	if (tcpclv4_encode_xfer_ack(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 18)
	{
		FAIL("Wrong encoded length");
	}

	/* Decode */
	if (tcpclv4_decode_xfer_ack(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.transfer_id != msg.transfer_id)
	{
		FAIL("transfer_id mismatch");
	}

	if (decoded.ack_len != msg.ack_len)
	{
		FAIL("ack_len mismatch");
	}

	PASS();
	return 0;
}

/*	Test XFER_REFUSE						*/

static int test_xfer_refuse(void)
{
	uint8_t				buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_refuse_t		msg;
	tcpclv4_xfer_refuse_t		decoded;
	size_t				len;

	TEST("XFER_REFUSE");

	/* Encode */
	msg.reason_code = TCPCLV4_REFUSE_NO_RESOURCES;
	msg.transfer_id = 12345;

	if (tcpclv4_encode_xfer_refuse(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 10)
	{
		FAIL("Wrong encoded length");
	}

	/* Decode */
	if (tcpclv4_decode_xfer_refuse(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.reason_code != msg.reason_code)
	{
		FAIL("reason_code mismatch");
	}

	if (decoded.transfer_id != msg.transfer_id)
	{
		FAIL("transfer_id mismatch");
	}

	PASS();
	return 0;
}

/*	Test KEEPALIVE							*/

static int test_keepalive(void)
{
	uint8_t	buf[TEST_BUFFER_SIZE];
	size_t	len;
	int	msg_type;

	TEST("KEEPALIVE");

	/* Encode */
	if (tcpclv4_encode_keepalive(buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 1)
	{
		FAIL("Wrong encoded length");
	}

	/* Verify message type */
	msg_type = tcpclv4_get_msg_type(buf, len);
	if (msg_type != TCPCLV4_MSG_KEEPALIVE)
	{
		FAIL("Message type mismatch");
	}

	PASS();
	return 0;
}

/*	Test MSG_REJECT							*/

static int test_msg_reject(void)
{
	uint8_t				buf[TEST_BUFFER_SIZE];
	tcpclv4_msg_reject_t		msg;
	tcpclv4_msg_reject_t		decoded;
	size_t				len;

	TEST("MSG_REJECT");

	/* Encode */
	msg.reason_code = TCPCLV4_REJECT_VERSION_MISMATCH;
	msg.rejected_msg_header = 0xFF;

	if (tcpclv4_encode_msg_reject(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 3)
	{
		FAIL("Wrong encoded length");
	}

	/* Decode */
	if (tcpclv4_decode_msg_reject(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.reason_code != msg.reason_code)
	{
		FAIL("reason_code mismatch");
	}

	if (decoded.rejected_msg_header != msg.rejected_msg_header)
	{
		FAIL("rejected_msg_header mismatch");
	}

	PASS();
	return 0;
}

/*	Test utility functions						*/

static int test_utilities(void)
{
	const char	*name;

	TEST("Utility functions");

	name = tcpclv4_msg_type_name(TCPCLV4_MSG_SESS_INIT);
	if (strcmp(name, "SESS_INIT") != 0)
	{
		FAIL("msg_type_name failed for SESS_INIT");
	}

	name = tcpclv4_msg_type_name(TCPCLV4_MSG_XFER_SEGMENT);
	if (strcmp(name, "XFER_SEGMENT") != 0)
	{
		FAIL("msg_type_name failed for XFER_SEGMENT");
	}

	name = tcpclv4_msg_type_name(0xFF);
	if (strcmp(name, "UNKNOWN") != 0)
	{
		FAIL("msg_type_name failed for unknown type");
	}

	PASS();
	return 0;
}

/*	Main test runner						*/

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	printf("TCPCLv4 Message Encode/Decode Round-trip Tests\n");
	printf("==============================================\n\n");

	test_contact_header();
	test_sess_init();
	test_sess_term();
	test_xfer_segment_start();
	test_xfer_segment_continuation();
	test_xfer_ack();
	test_xfer_refuse();
	test_keepalive();
	test_msg_reject();
	test_utilities();

	printf("\n==============================================\n");
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
