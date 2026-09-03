/*
 * ltp_extcount_oob_test.c -- AddressSanitizer regression for the LTP
 * extension-counts over-read.
 *
 * ltpHandleInboundSegment() read the LTP header's extension-counts byte without
 * first checking that any byte of the segment remained.  The SDNV accessors that
 * precede it (_extractSdnv/_extractSmallSdnv) are bounded and leave the
 * remaining-byte count at zero when a segment ends right after the session ID,
 * so on such a segment the extension-counts read went one byte past the end of
 * the caller's buffer and the remaining-byte count was then decremented to -1.
 * The read is now guarded by a remaining-bytes check.
 *
 * This harness feeds exactly that segment to ltpHandleInboundSegment() in an
 * EXACT-size heap buffer.  On the fixed code the segment is ignored before the
 * extension-counts byte is reached; on the vulnerable code the read is one byte
 * past the allocation, which AddressSanitizer reports as a heap-buffer-overflow
 * (so this test fails).
 *
 * Note that the fixed code returns before it needs any ION resource, so this
 * harness does not require a running node.  It deliberately does not assert
 * anything about well-formed segments: proceeding past the guard immediately
 * requires ION's memory manager and the LTP database.  Correct parsing of valid
 * segments -- and therefore that this guard is not off by one -- is covered by
 * the ltp-* regression tests.
 *
 * Meaningful only when ION is built with --enable-asan; the dotest skips
 * otherwise.
 */

#include <ion.h>
#include <platform.h>

#include <stdio.h>
#include <stdlib.h>

/*	ltpHandleInboundSegment() is declared in the private header ltpP.h,
 *	which is not installed, so declare it here.			*/

extern int	ltpHandleInboundSegment(char *buf, int length);

int main(int argc, char **argv)
{
	char	*buf;
	int	rc;

	(void) argc;
	(void) argv;

	/*
	 * A minimal LTP segment truncated immediately after the session ID:
	 *
	 *   byte 0   0x00   version 0 (high nibble), segment type 0 (red data)
	 *   byte 1   0x01   source engine ID, one-byte SDNV, value 1
	 *   byte 2   0x01   session number, one-byte SDNV, value 1
	 *
	 * The session number must be nonzero or the segment is discarded before
	 * reaching the byte under test.  The extension-counts byte would be
	 * byte 3 -- one past this three-byte allocation.
	 */

	buf = malloc(3);
	if (buf == NULL)
	{
		fprintf(stderr, "FAIL: out of memory.\n");
		return 1;
	}

	buf[0] = 0x00;
	buf[1] = 0x01;
	buf[2] = 0x01;

	rc = ltpHandleInboundSegment(buf, 3);
	free(buf);
	if (rc != 0)
	{
		fprintf(stderr, "FAIL: truncated segment not ignored (rc=%d).\n",
				rc);
		return 1;
	}

	fprintf(stderr, "PASS: truncated LTP segment ignored without reading "
			"past the buffer.\n");
	return 0;
}
