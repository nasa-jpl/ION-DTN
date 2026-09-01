/*
 * dgr_oob_test.c -- AddressSanitizer regression for the DGR receiver
 * out-of-bounds read.
 *
 * The DGR receiver decodes a segment's client service-data length and copies
 * that many bytes out of the received datagram.  _dgrParseInboundSegment()
 * bounds that declared length against the bytes actually present, so the copy
 * cannot run past the datagram.  This harness builds DGR data segments in
 * EXACT-size heap buffers and performs the same content copy the receiver
 * does, using the offset and length the parser returns:
 *
 *   - A well-formed segment must parse (type 3) with the exact content.
 *   - A segment that overstates its service-data length must be rejected, so
 *     no copy occurs.  On vulnerable code the parser returns the overstated
 *     length and the copy below reads past the exact-size allocation, which
 *     AddressSanitizer reports as a heap-buffer-overflow (the test fails).
 *
 * The over-read, if any, happens in this harness's own copy of a malloc'd
 * buffer, so ASan catches it whenever the harness itself is built with
 * -fsanitize=address -- no AddressSanitizer build of ION is required.
 */

#include <ion.h>
#include <platform.h>
#include "dgrP.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void putSdnv(char *buf, int *len, uvast value)
{
	Sdnv sdnv;

	encodeSdnv(&sdnv, value);
	memcpy(buf + *len, sdnv.text, sdnv.length);
	*len += sdnv.length;
}

/*
 * Builds a DGR red-data (type 3, EOB) segment header into buf, declaring
 * svcDataLength bytes of content, and returns the header length.  The caller
 * appends the actual content bytes.
 */
static int buildDataHeader(char *buf, uvast engineId, uvast sessionNbr,
		uvast svcDataLength)
{
	int len = 0;

	buf[len++] = 0x03;     /* Version 0, segment type 3. */
	putSdnv(buf, &len, engineId);
	putSdnv(buf, &len, sessionNbr);
	buf[len++] = 0x00;     /* Extension counts. */
	putSdnv(buf, &len, 1); /* Client service id. */
	putSdnv(buf, &len, 0); /* Service data offset (0). */
	putSdnv(buf, &len, svcDataLength);
	putSdnv(buf, &len, 1); /* Checkpoint serial number. */
	putSdnv(buf, &len, 0); /* Report serial number (0). */
	return len;
}

int main(int argc, char **argv)
{
	uvast	    engineId;
	uvast	    sessionNbr;
	uvast	    ckptSerialNbr;
	uvast	    rptSerialNbr;
	uvast	    contentLength;
	int	    headerLength;
	int	    contentOffset;
	int	    segmentType;
	char	    scratch[128];
	int	    hdrLen;
	char	   *buf;
	char	   *dst;
	const char *payload = "HELLO-DGR";
	int	    payloadLen = (int) strlen(payload);

	(void) argc;
	(void) argv;

	/*
	 * Case 1: well-formed segment (declared length == bytes present).
	 * Must parse as type 3 with the exact content.
	 */

	hdrLen = buildDataHeader(scratch, 2, 1, (uvast) payloadLen);
	buf = malloc((size_t) (hdrLen + payloadLen)); /*	Exact size.	*/
	memcpy(buf, scratch, (size_t) hdrLen);
	memcpy(buf + hdrLen, payload, (size_t) payloadLen);
	segmentType = _dgrParseInboundSegment(buf, hdrLen + payloadLen,
			&engineId, &sessionNbr, &ckptSerialNbr, &rptSerialNbr,
			&headerLength, &contentOffset, &contentLength);
	if (segmentType != 3)
	{
		fprintf(stderr,
				"FAIL: well-formed data segment not parsed "
				"(type=%d).\n",
				segmentType);
		free(buf);
		return 1;
	}

	if (contentLength != (uvast) payloadLen)
	{
		fprintf(stderr,
				"FAIL: wrong content length (got " UVAST_FIELDSPEC
				", want %d).\n",
				contentLength, payloadLen);
		free(buf);
		return 1;
	}

	/* The copy the receiver performs -- must stay within buf. */

	dst = malloc((size_t) contentLength);
	memcpy(dst, buf + contentOffset, (size_t) contentLength);
	if (memcmp(dst, payload, (size_t) payloadLen) != 0)
	{
		fprintf(stderr, "FAIL: delivered content does not match.\n");
		free(dst);
		free(buf);
		return 1;
	}

	free(dst);
	free(buf);

	/*
	 * Case 2: overstated service-data length.  The header claims 4096
	 * content bytes but only payloadLen are present in an exact-size
	 * buffer.  The parser must reject the segment; on vulnerable code it
	 * returns the overstated length and the copy below reads past the
	 * allocation (ASan heap-buffer-overflow).
	 */

	hdrLen = buildDataHeader(scratch, 2, 1, (uvast) 4096);
	buf = malloc((size_t) (hdrLen + payloadLen)); /*	Exact size.	*/
	memcpy(buf, scratch, (size_t) hdrLen);
	memcpy(buf + hdrLen, payload, (size_t) payloadLen);
	segmentType = _dgrParseInboundSegment(buf, hdrLen + payloadLen,
			&engineId, &sessionNbr, &ckptSerialNbr, &rptSerialNbr,
			&headerLength, &contentOffset, &contentLength);
	if (segmentType == 3)
	{
		/*
		 * Vulnerable path: the receiver would copy contentLength bytes
		 * starting at buf + contentOffset.  Touch the last of those
		 * bytes -- on vulnerable code contentLength overstates the
		 * datagram, so this reads past the exact-size allocation and
		 * AddressSanitizer reports a heap-buffer-overflow.
		 */

		volatile char sink;

		sink = buf[contentOffset + (int) contentLength - 1];
		(void) sink;
		free(buf);
		fprintf(stderr,
				"FAIL: overstated service-data length accepted "
				"(len=" UVAST_FIELDSPEC ").\n",
				contentLength);
		return 1;
	}

	free(buf);

	fprintf(stderr, "PASS: DGR content length is bounded to the datagram.\n");
	return 0;
}
