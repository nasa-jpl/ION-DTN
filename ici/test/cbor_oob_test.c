/*
	cbor_oob_test.c:	regression test for VDP-2945, an out-of-bounds
			read in the ICI CBOR string decoders.

	cbor_decode_text_string() and cbor_decode_byte_string() must
	reject a string whose declared length exceeds the number of bytes
	actually buffered, instead of memcpy()ing past the end of the
	input (CWE-125) and underflowing the unsigned "bytes buffered"
	counter (CWE-191).

	The truncated inputs are heap-allocated to exactly the number of
	bytes "on the wire", so an AddressSanitizer build aborts on the
	over-read if the bounds check is ever removed.  A non-ASAN build
	verifies the decoders return 0 (reject) and leave the counter
	un-underflowed, plus that well-formed strings still decode.

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
									*/

#include "platform.h"
#include "cbor.h"

static int	failures = 0;

static void	check(int cond, const char *msg)
{
	if (cond)
	{
		printf("ok   - %s\n", msg);
	}
	else
	{
		printf("FAIL - %s\n", msg);
		failures++;
	}
}

/*	Copies "len" bytes into a heap region sized exactly "len" so that
 *	an over-read past the supplied input is flagged under ASAN.	*/
static unsigned char	*wireCopy(const unsigned char *bytes, size_t len)
{
	unsigned char	*buf = (unsigned char *) malloc(len);

	if (buf == NULL)
	{
		fprintf(stderr, "out of memory\n");
		exit(2);
	}

	memcpy(buf, bytes, len);
	return buf;
}

static void	testTruncatedText(void)
{
	/*	0x78 0xC8: text string, 1-byte length = 200; followed by
	 *	only 2 body bytes -- far short of the declared 200.	*/
	const unsigned char	wire[] = { 0x78, 0xC8, 'a', 'b' };
	unsigned char		*buf = wireCopy(wire, sizeof wire);
	unsigned char		*cursor = buf;
	unsigned int		bytesBuffered = sizeof wire;
	char			value[256];
	uvast			size = sizeof value;
	int			rc;

	rc = cbor_decode_text_string(value, &size, &cursor, &bytesBuffered);
	check(rc == 0, "truncated text string is rejected");
	check(bytesBuffered <= sizeof wire,
			"text decode does not underflow bytesBuffered");
	free(buf);
}

static void	testTruncatedBytes(void)
{
	/*	0x58 0xC8: byte string, 1-byte length = 200; followed by
	 *	only 2 body bytes.					*/
	const unsigned char	wire[] = { 0x58, 0xC8, 0x01, 0x02 };
	unsigned char		*buf = wireCopy(wire, sizeof wire);
	unsigned char		*cursor = buf;
	unsigned int		bytesBuffered = sizeof wire;
	unsigned char		value[256];
	uvast			size = sizeof value;
	int			rc;

	rc = cbor_decode_byte_string(value, &size, &cursor, &bytesBuffered);
	check(rc == 0, "truncated byte string is rejected");
	check(bytesBuffered <= sizeof wire,
			"byte decode does not underflow bytesBuffered");
	free(buf);
}

static void	testWellFormedText(void)
{
	/*	0x63 'a' 'b' 'c': text string of length 3 = "abc".	*/
	const unsigned char	wire[] = { 0x63, 'a', 'b', 'c' };
	unsigned char		*buf = wireCopy(wire, sizeof wire);
	unsigned char		*cursor = buf;
	unsigned int		bytesBuffered = sizeof wire;
	char			value[256];
	uvast			size = sizeof value;
	int			rc;

	rc = cbor_decode_text_string(value, &size, &cursor, &bytesBuffered);
	check(rc > 0, "well-formed text string still decodes");
	check(size == 3, "well-formed text string length is 3");
	check(memcmp(value, "abc", 3) == 0, "well-formed text content matches");
	free(buf);
}

static void	testWellFormedBytes(void)
{
	/*	0x43 0x01 0x02 0x03: byte string of length 3.		*/
	const unsigned char	wire[] = { 0x43, 0x01, 0x02, 0x03 };
	unsigned char		*buf = wireCopy(wire, sizeof wire);
	unsigned char		*cursor = buf;
	unsigned int		bytesBuffered = sizeof wire;
	unsigned char		value[256];
	uvast			size = sizeof value;
	int			rc;

	rc = cbor_decode_byte_string(value, &size, &cursor, &bytesBuffered);
	check(rc > 0, "well-formed byte string still decodes");
	check(size == 3, "well-formed byte string length is 3");
	free(buf);
}

#if defined (ION_LWT)
int	cbor_oob_test(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	(void)a6; (void)a7; (void)a8; (void)a9; (void)a10;
#else
int	main(int argc, char **argv)
{
	(void)argc; (void)argv;
#endif
	testTruncatedText();
	testTruncatedBytes();
	testWellFormedText();
	testWellFormedBytes();

	if (failures == 0)
	{
		printf("cbor_oob_test: PASSED\n");
		return 0;
	}

	printf("cbor_oob_test: FAILED (%d)\n", failures);
	return 1;
}
