/*
	test_msg_roundtrip.c: Unit tests for TCPCLv4 message encode/decode.

			Verifies the wire format against hand-computed
			RFC 9174 byte vectors (not just encode/decode
			round-trips, which cannot catch systematic
			deviations from the spec), plus stream framing.

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

/*	Test Contact Header (RFC 9174 §4.2)				*/

static int test_contact_header(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_contact_header_t msg;
	tcpclv4_contact_header_t decoded;
	size_t			len;

	/*	"dtn!" + version 4 + CAN_TLS				*/
	static const uint8_t	expected[6] =
		{ 0x64, 0x74, 0x6E, 0x21, 0x04, 0x01 };
	uint8_t			bad[6];

	TEST("Contact Header");

	msg.version = TCPCLV4_PROTOCOL_VERSION;
	msg.flags = TCPCLV4_CONTACT_FLAG_CAN_TLS;

	if (tcpclv4_encode_contact_header(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 6 || memcmp(buf, expected, 6) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	if (tcpclv4_decode_contact_header(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.version != 4 || decoded.flags != 1)
	{
		FAIL("Field mismatch");
	}

	/*	Wrong magic must be rejected.				*/

	memcpy(bad, expected, 6);
	bad[0] = 'x';
	if (tcpclv4_decode_contact_header(bad, 6, &decoded) == 0)
	{
		FAIL("Accepted bad magic");
	}

	/*	Wrong version must be rejected.				*/

	memcpy(bad, expected, 6);
	bad[4] = 3;
	if (tcpclv4_decode_contact_header(bad, 6, &decoded) == 0)
	{
		FAIL("Accepted bad version");
	}

	/*	Truncated header must be rejected.			*/

	if (tcpclv4_decode_contact_header(expected, 5, &decoded) == 0)
	{
		FAIL("Accepted truncated header");
	}

	PASS();
	return 0;
}

/*	Test SESS_INIT (RFC 9174 §4.7)					*/

static int test_sess_init(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_sess_init_t	msg;
	tcpclv4_sess_init_t	decoded;
	size_t			len;

	/*	keepalive=60, seg MRU=1MB, xfer MRU=100MB,
	 *	node "ipn:2.0", no extensions.				*/
	static const uint8_t	expected[32] = {
		0x07,				/* type */
		0x00, 0x3C,			/* keepalive 60 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
						/* seg MRU 1048576 */
		0x00, 0x00, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00,
						/* xfer MRU 104857600 */
		0x00, 0x07,			/* node ID len 7 */
		0x69, 0x70, 0x6E, 0x3A, 0x32, 0x2E, 0x30,
						/* "ipn:2.0" */
		0x00, 0x00, 0x00, 0x00		/* ext items len 0 */
	};

	TEST("SESS_INIT");

	memset(&msg, 0, sizeof(msg));
	msg.keepalive_interval = 60;
	msg.segment_mru = 1048576;
	msg.transfer_mru = 104857600;
	msg.node_id = "ipn:2.0";
	msg.node_id_len = 7;

	if (tcpclv4_encode_sess_init(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != sizeof(expected) || memcmp(buf, expected, len) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	memset(&decoded, 0, sizeof(decoded));
	if (tcpclv4_decode_sess_init(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.keepalive_interval != 60
	|| decoded.segment_mru != 1048576
	|| decoded.transfer_mru != 104857600
	|| decoded.node_id_len != 7
	|| strcmp(decoded.node_id, "ipn:2.0") != 0
	|| decoded.extensions_len != 0)
	{
		tcpclv4_sess_init_free(&decoded);
		FAIL("Field mismatch");
	}

	tcpclv4_sess_init_free(&decoded);

	/*	Truncated message must be rejected.			*/

	if (tcpclv4_decode_sess_init(expected, sizeof(expected) - 1,
			&decoded) == 0)
	{
		tcpclv4_sess_init_free(&decoded);
		FAIL("Accepted truncated message");
	}

	PASS();
	return 0;
}

/*	Test SESS_TERM (RFC 9174 §6.1)					*/

static int test_sess_term(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_sess_term_t	msg;
	tcpclv4_sess_term_t	decoded;
	size_t			len;

	static const uint8_t	expected[3] = { 0x05, 0x00, 0x03 };

	TEST("SESS_TERM");

	msg.flags = 0;
	msg.reason_code = TCPCLV4_TERM_BUSY;

	if (tcpclv4_encode_sess_term(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 3 || memcmp(buf, expected, 3) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	if (tcpclv4_decode_sess_term(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.flags != 0 || decoded.reason_code != TCPCLV4_TERM_BUSY)
	{
		FAIL("Field mismatch");
	}

	PASS();
	return 0;
}

/*	Test XFER_SEGMENT START (RFC 9174 §5.2.2)			*/

static int test_xfer_segment_start(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_segment_t	msg;
	tcpclv4_xfer_segment_t	decoded;
	size_t			len;

	/*	START|END, id 1, no extensions, data "hello".		*/
	static const uint8_t	expected[27] = {
		0x01,				/* type */
		0x03,				/* flags START|END */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
						/* transfer id 1 */
		0x00, 0x00, 0x00, 0x00,		/* ext items len 0 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
						/* data len 5 */
		'h', 'e', 'l', 'l', 'o'
	};

	TEST("XFER_SEGMENT (START)");

	memset(&msg, 0, sizeof(msg));
	msg.flags = TCPCLV4_XFER_FLAG_START | TCPCLV4_XFER_FLAG_END;
	msg.transfer_id = 1;
	msg.data = (const uint8_t *) "hello";
	msg.data_len = 5;

	if (tcpclv4_encode_xfer_segment(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != sizeof(expected) || memcmp(buf, expected, len) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	memset(&decoded, 0, sizeof(decoded));
	if (tcpclv4_decode_xfer_segment(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.flags != 0x03 || decoded.transfer_id != 1
	|| decoded.extensions_len != 0 || decoded.data_len != 5
	|| memcmp(decoded.data, "hello", 5) != 0)
	{
		tcpclv4_xfer_segment_free(&decoded);
		FAIL("Field mismatch");
	}

	tcpclv4_xfer_segment_free(&decoded);
	PASS();
	return 0;
}

/*	Test XFER_SEGMENT continuation (no ext field, no START flag)	*/

static int test_xfer_segment_continuation(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_segment_t	msg;
	tcpclv4_xfer_segment_t	decoded;
	size_t			len;

	/*	END only, id 1, data "world".  Continuation segments
	 *	carry NO extension item length field.			*/
	static const uint8_t	expected[23] = {
		0x01,				/* type */
		0x01,				/* flags END */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
						/* transfer id 1 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
						/* data len 5 */
		'w', 'o', 'r', 'l', 'd'
	};

	TEST("XFER_SEGMENT (continuation)");

	memset(&msg, 0, sizeof(msg));
	msg.flags = TCPCLV4_XFER_FLAG_END;
	msg.transfer_id = 1;
	msg.data = (const uint8_t *) "world";
	msg.data_len = 5;

	if (tcpclv4_encode_xfer_segment(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != sizeof(expected) || memcmp(buf, expected, len) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	memset(&decoded, 0, sizeof(decoded));
	if (tcpclv4_decode_xfer_segment(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.flags != 0x01 || decoded.transfer_id != 1
	|| decoded.data_len != 5
	|| memcmp(decoded.data, "world", 5) != 0)
	{
		tcpclv4_xfer_segment_free(&decoded);
		FAIL("Field mismatch");
	}

	tcpclv4_xfer_segment_free(&decoded);

	/*	Extensions on a non-START segment must be rejected.	*/

	memset(&msg, 0, sizeof(msg));
	msg.flags = TCPCLV4_XFER_FLAG_END;
	msg.transfer_id = 1;
	msg.extensions = (uint8_t *) "x";
	msg.extensions_len = 1;
	msg.data = (const uint8_t *) "world";
	msg.data_len = 5;

	if (tcpclv4_encode_xfer_segment(&msg, buf, sizeof(buf), &len) == 0)
	{
		FAIL("Accepted extensions on non-START segment");
	}

	PASS();
	return 0;
}

/*	Test XFER_ACK (RFC 9174 §5.2.3)				*/

static int test_xfer_ack(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_ack_t	msg;
	tcpclv4_xfer_ack_t	decoded;
	size_t			len;

	/*	Flags echo the acked segment's flags (§5.2.3).		*/
	static const uint8_t	expected[18] = {
		0x02,				/* type */
		0x03,				/* flags START|END */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
						/* transfer id 1 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05
						/* ack len 5 */
	};

	TEST("XFER_ACK");

	msg.flags = 0x03;
	msg.transfer_id = 1;
	msg.ack_len = 5;

	if (tcpclv4_encode_xfer_ack(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 18 || memcmp(buf, expected, 18) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	if (tcpclv4_decode_xfer_ack(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.flags != 0x03 || decoded.transfer_id != 1
	|| decoded.ack_len != 5)
	{
		FAIL("Field mismatch");
	}

	PASS();
	return 0;
}

/*	Test XFER_REFUSE (RFC 9174 §5.2.3)				*/

static int test_xfer_refuse(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_refuse_t	msg;
	tcpclv4_xfer_refuse_t	decoded;
	size_t			len;

	static const uint8_t	expected[10] = {
		0x03,				/* type */
		0x01,				/* reason COMPLETED */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07
						/* transfer id 7 */
	};

	TEST("XFER_REFUSE");

	msg.reason_code = TCPCLV4_REFUSE_COMPLETED;
	msg.transfer_id = 7;

	if (tcpclv4_encode_xfer_refuse(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 10 || memcmp(buf, expected, 10) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	if (tcpclv4_decode_xfer_refuse(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.reason_code != TCPCLV4_REFUSE_COMPLETED
	|| decoded.transfer_id != 7)
	{
		FAIL("Field mismatch");
	}

	PASS();
	return 0;
}

/*	Test KEEPALIVE (RFC 9174 §5.1.2)				*/

static int test_keepalive(void)
{
	uint8_t		buf[TEST_BUFFER_SIZE];
	size_t		len;

	TEST("KEEPALIVE");

	if (tcpclv4_encode_keepalive(buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 1 || buf[0] != 0x04)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	PASS();
	return 0;
}

/*	Test MSG_REJECT (RFC 9174 §5.1.1)				*/

static int test_msg_reject(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_msg_reject_t	msg;
	tcpclv4_msg_reject_t	decoded;
	size_t			len;

	static const uint8_t	expected[3] = { 0x06, 0x02, 0x99 };

	TEST("MSG_REJECT");

	msg.reason_code = TCPCLV4_REJECT_UNSUPPORTED;
	msg.rejected_msg_header = 0x99;

	if (tcpclv4_encode_msg_reject(&msg, buf, sizeof(buf), &len) < 0)
	{
		FAIL("Encode failed");
	}

	if (len != 3 || memcmp(buf, expected, 3) != 0)
	{
		FAIL("Wire bytes do not match RFC 9174 vector");
	}

	if (tcpclv4_decode_msg_reject(buf, len, &decoded) < 0)
	{
		FAIL("Decode failed");
	}

	if (decoded.reason_code != TCPCLV4_REJECT_UNSUPPORTED
	|| decoded.rejected_msg_header != 0x99)
	{
		FAIL("Field mismatch");
	}

	PASS();
	return 0;
}

/*	Test stream framing						*/

static int test_frame_len(void)
{
	uint8_t			buf[TEST_BUFFER_SIZE];
	tcpclv4_xfer_segment_t	seg;
	tcpclv4_sess_init_t	init;
	size_t			msg_total;
	size_t			frame;
	size_t			i;
	int			ret;

	TEST("Stream framing");

	/*	Fixed-size messages.					*/

	buf[0] = TCPCLV4_MSG_KEEPALIVE;
	if (tcpclv4_msg_frame_len(buf, 1, 0, &frame) != 1 || frame != 1)
	{
		FAIL("KEEPALIVE framing");
	}

	buf[0] = TCPCLV4_MSG_XFER_ACK;
	if (tcpclv4_msg_frame_len(buf, 1, 0, &frame) != 0)
	{
		FAIL("Partial XFER_ACK not detected");
	}

	memset(buf + 1, 0, 17);
	if (tcpclv4_msg_frame_len(buf, 18, 0, &frame) != 1 || frame != 18)
	{
		FAIL("XFER_ACK framing");
	}

	/*	Unknown message type.					*/

	buf[0] = 0xAA;
	if (tcpclv4_msg_frame_len(buf, 1, 0, &frame) != -1)
	{
		FAIL("Unknown type not rejected");
	}

	/*	XFER_SEGMENT: every prefix must report incomplete,
	 *	the full message must report complete.			*/

	memset(&seg, 0, sizeof(seg));
	seg.flags = TCPCLV4_XFER_FLAG_START | TCPCLV4_XFER_FLAG_END;
	seg.transfer_id = 42;
	seg.data = (const uint8_t *) "some payload bytes";
	seg.data_len = 18;

	if (tcpclv4_encode_xfer_segment(&seg, buf, sizeof(buf),
			&msg_total) < 0)
	{
		FAIL("Segment encode failed");
	}

	for (i = 1; i < msg_total; i++)
	{
		ret = tcpclv4_msg_frame_len(buf, i, 0, &frame);
		if (ret != 0)
		{
			FAIL("Partial segment misframed");
		}
	}

	if (tcpclv4_msg_frame_len(buf, msg_total, 0, &frame) != 1
	|| frame != msg_total)
	{
		FAIL("Complete segment misframed");
	}

	/*	Oversize guard: data_len larger than max_msg_len.	*/

	if (tcpclv4_msg_frame_len(buf, msg_total, 10, &frame) != -1)
	{
		FAIL("Oversize segment not rejected");
	}

	/*	SESS_INIT framing across every prefix.			*/

	memset(&init, 0, sizeof(init));
	init.keepalive_interval = 60;
	init.segment_mru = 1048576;
	init.transfer_mru = 104857600;
	init.node_id = "ipn:2.0";
	init.node_id_len = 7;

	if (tcpclv4_encode_sess_init(&init, buf, sizeof(buf),
			&msg_total) < 0)
	{
		FAIL("SESS_INIT encode failed");
	}

	for (i = 1; i < msg_total; i++)
	{
		ret = tcpclv4_msg_frame_len(buf, i, 0, &frame);
		if (ret != 0)
		{
			FAIL("Partial SESS_INIT misframed");
		}
	}

	if (tcpclv4_msg_frame_len(buf, msg_total, 0, &frame) != 1
	|| frame != msg_total)
	{
		FAIL("Complete SESS_INIT misframed");
	}

	/*	Two messages back to back: frame length must cover
	 *	only the first.						*/

	buf[msg_total] = TCPCLV4_MSG_KEEPALIVE;
	if (tcpclv4_msg_frame_len(buf, msg_total + 1, 0, &frame) != 1
	|| frame != msg_total)
	{
		FAIL("Back-to-back messages misframed");
	}

	PASS();
	return 0;
}

/*	Test utility functions						*/

static int test_utilities(void)
{
	uint8_t		buf[4];

	TEST("Utility functions");

	buf[0] = TCPCLV4_MSG_XFER_SEGMENT;
	if (tcpclv4_get_msg_type(buf, 1) != TCPCLV4_MSG_XFER_SEGMENT)
	{
		FAIL("get_msg_type failed");
	}

	if (tcpclv4_get_msg_type(buf, 0) != -1)
	{
		FAIL("get_msg_type accepted empty buffer");
	}

	if (strcmp(tcpclv4_msg_type_name(TCPCLV4_MSG_SESS_INIT),
			"SESS_INIT") != 0
	|| strcmp(tcpclv4_msg_type_name(TCPCLV4_MSG_XFER_SEGMENT),
			"XFER_SEGMENT") != 0
	|| strcmp(tcpclv4_msg_type_name(0xEE), "UNKNOWN") != 0)
	{
		FAIL("msg_type_name failed");
	}

	PASS();
	return 0;
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	printf("TCPCLv4 Message Library Unit Tests (RFC 9174)\n");
	printf("==============================================\n");

	test_contact_header();
	test_sess_init();
	test_sess_term();
	test_xfer_segment_start();
	test_xfer_segment_continuation();
	test_xfer_ack();
	test_xfer_refuse();
	test_keepalive();
	test_msg_reject();
	test_frame_len();
	test_utilities();

	printf("\n==============================================\n");
	printf("Tests: %d/%d passed\n", tests_passed, tests_run);

	if (tests_passed == tests_run)
	{
		printf("ALL TESTS PASSED\n");
		return 0;
	}

	printf("SOME TESTS FAILED\n");
	return 1;
}
