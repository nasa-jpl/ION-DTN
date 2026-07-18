/*
 * sdnv_oob_test.c -- AddressSanitizer regression for GHSA-85pw-28vw-2jf7.
 *
 * decodeSdnv() walks up to 10 bytes looking for the SDNV terminator with no
 * knowledge of the caller's buffer length, so a truncated SDNV at the end of a
 * received buffer is read past its end.  The extractSdnv() wrappers now decode
 * with decodeSdnvBounded(), bounded by the caller's remaining byte count, so
 * the over-read cannot occur.
 *
 * This harness feeds a truncated SDNV to _extractSdnv() in an EXACT-size heap
 * buffer.  On the fixed code the decode stops at the buffer end and the call
 * is rejected; on the vulnerable code it reads one byte past the allocation,
 * which AddressSanitizer reports as a heap-buffer-overflow (so this test
 * fails).  A well-formed SDNV must still decode correctly.
 *
 * Meaningful only when ION is built with --enable-asan; the dotest skips
 * otherwise.
 */

#include <ion.h>
#include <platform.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	unsigned char *buf;
	uvast	       val;
	unsigned char *cursor;
	int	       remnant;
	int	       rc;

	(void) argc;
	(void) argv;

	/*
	 * Case 1: truncated SDNV.  Both bytes have the continuation bit (0x80)
	 * set, so the SDNV is never terminated within the 2-byte buffer.  The
	 * bounded decode must stop at the buffer end rather than read buf[2].
	 */

	buf = malloc(2);
	buf[0] = 0x81;
	buf[1] = 0x81;
	cursor = buf;
	remnant = 2;
	rc = _extractSdnv(&val, &cursor, &remnant, __LINE__);
	if (rc != 0)
	{
		fprintf(stderr, "FAIL: truncated SDNV not rejected (rc=%d).\n",
				rc);
		free(buf);
		return 1;
	}

	free(buf);

	/*
	 * Case 2: well-formed 2-byte SDNV must still decode. 0x81 =
	 * continuation, low 7 bits = 1; 0x2c = terminator, low 7 bits = 0x2c.
	 * Value = (1 << 7) | 0x2c = 172.
	 */

	buf = malloc(2);
	buf[0] = 0x81;
	buf[1] = 0x2c;
	cursor = buf;
	remnant = 2;
	rc = _extractSdnv(&val, &cursor, &remnant, __LINE__);
	if (rc != 2 || val != (uvast) ((1 << 7) | 0x2c))
	{
		fprintf(stderr,
				"FAIL: valid SDNV mis-decoded "
				"(rc=%d val=" UVAST_FIELDSPEC ").\n",
				rc, val);
		free(buf);
		return 1;
	}

	free(buf);

	fprintf(stderr, "PASS: SDNV decode stays within the input buffer.\n");
	return 0;
}
