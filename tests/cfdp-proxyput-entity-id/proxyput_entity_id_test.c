/*
 * proxyput_entity_id_test.c -- regression for the CFDP Proxy Put Request
 * entity-ID out-of-bounds write (GHSA-fh7m-mf49-m786).
 *
 * parseProxyPutRequest() reads the destination entity-ID length as a raw byte
 * (0..255) and bounds it only against the bytes remaining in the message, never
 * against the 8-byte CfdpNumber value buffer.  For any length greater than 8,
 * pad = 8 - length is negative; converted to size_t it becomes enormous, so
 *   memset(buffer, 0, pad)
 * writes far past the buffer -- a remote out-of-bounds write from one received
 * Proxy Put Request.  The fix rejects a length greater than the buffer size.
 *
 * The harness calls parseProxyPutRequest() with a destination entity-ID length
 * of 9.  On the vulnerable code the memset crashes the process; on the fixed
 * code the message is rejected and the call returns.  The function is a pure
 * parser (no ION runtime), so no node is needed; it uses the private CFDP
 * structures, so it compiles against the ION source tree ($IONDIR).
 */

#include "cfdpP.h"

#include <stdio.h>
#include <string.h>

extern void	parseProxyPutRequest(char *text, int bytesRemaining,
			CfdpUserOpsData *opsData);

int	main(int argc, char **argv)
{
	CfdpUserOpsData	opsData;
	unsigned char	text[10];

	(void) argc;
	(void) argv;

	memset(&opsData, 0, sizeof opsData);

	/*	One-byte length prefix of 9 (greater than the 8-byte
	 *	CfdpNumber buffer), followed by nine entity-ID bytes.	*/

	memset(text, 0x41, sizeof text);
	text[0] = 9;

	parseProxyPutRequest((char *) text, (int) sizeof text, &opsData);

	fprintf(stderr, "PASS: oversized Proxy Put entity-ID length was rejected "
			"without an out-of-bounds write.\n");
	return 0;
}
