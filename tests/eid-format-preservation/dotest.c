/*

	eid-format-preservation/dotest.c:	Verify that ION preserves the
	CBOR array cardinality (2-element [fqnn, service] vs 3-element
	[allocator, node, service]) of an ipn EID across acquisition and
	re-serialization, rather than transcoding it.

	RFC 9758 6.4 permits both encodings of the same ipn EID but notes
	that syntax-based integrity checks treat the two byte strings
	differently, so a forwarding node that re-encodes an EID into a
	different form breaks any covering BPSec integrity block.  Prior to
	the EidFormat change, serializeEid re-derived the cardinality solely
	from the fully-qualified node number (3-element iff the allocator
	bits were non-zero), so a bundle received as [0,N,S] left as [N,S]
	and a non-zero-allocator EID received as 2-element left as 3-element.

	This test feeds each of the four wire forms through acquireEid and
	asserts that serializeEid reproduces byte-identical output.  A live
	node is required because acquireEid resolves the scheme name through
	the BP database.

									*/

#include <bp.h>
#include <bpP.h>
#include <string.h>
#include "check.h"
#include "testutil.h"

static void	check_roundtrip(const char *label, unsigned char *in, int inlen)
{
	EndpointId	eid;
	unsigned char	*cursor = in;
	unsigned int	remaining = (unsigned int) inlen;
	unsigned char	out[300];
	int		got;
	int		outlen;

	memset(&eid, 0, sizeof eid);
	got = acquireEid(&eid, &cursor, &remaining);
	fail_unless(got == inlen, "acquireEid failed", label);
	outlen = serializeEid(&eid, out);
	fail_unless(outlen == inlen && memcmp(in, out, inlen) == 0,
			"EID CBOR form was not preserved (transcoded)", label);
}

int	main(int argc, char **argv)
{
	/*	Full EID encodings: [schemeCode(2), <ipn-ssp>].		*/

	unsigned char	v2elemAlloc0[] =
		{0x82, 0x02, 0x82, 0x0A, 0x02};			/*	[10,2]		*/
	unsigned char	v3elemAlloc0[] =
		{0x82, 0x02, 0x83, 0x00, 0x0A, 0x02};		/*	[0,10,2]	*/
	unsigned char	v3elemAllocN[] =
		{0x82, 0x02, 0x83, 0x1A, 0x00, 0x0E, 0xE8, 0x68,
			0x18, 0x64, 0x01};			/*	[977000,100,1]	*/
	unsigned char	v2elemAllocN[] =
		{0x82, 0x02, 0x82, 0x1B, 0x00, 0x0E, 0xE8, 0x68,
			0x00, 0x00, 0x00, 0x64, 0x01};		/*	[fqnn,1]	*/

	(void) argc;

	/*	Bring up the shared single-node ipn loopback configuration
	 *	so that acquireEid can resolve the ipn scheme.		*/

	synch_killm(60);
	ionstart_default_config("loopback-ltp/loopback.ionrc",
			"loopback-ltp/loopback.ionsecrc",
			"loopback-ltp/loopback.ltprc",
			"loopback-ltp/loopback.bprc",
			"loopback-ltp/loopback.ipnrc",
			NULL);
	ionstart_default_config(NULL, NULL, NULL,
			"loopback-ltp/loopbackstart.bprc",
			NULL, NULL);

	fail_unless(bp_attach() >= 0);

	/*	Continue through all four cases for full diagnostics.	*/

	failure_mode = CONTINUE_ON_FAIL;

	check_roundtrip("2-element, allocator 0 (canonical)",
			v2elemAlloc0, sizeof v2elemAlloc0);
	check_roundtrip("3-element, allocator 0 (non-canonical)",
			v3elemAlloc0, sizeof v3elemAlloc0);
	check_roundtrip("3-element, non-zero allocator (canonical)",
			v3elemAllocN, sizeof v3elemAllocN);
	check_roundtrip("2-element, non-zero allocator (non-canonical)",
			v2elemAllocN, sizeof v2elemAllocN);

	writeErrmsgMemos();
	bp_detach();

	ionstop();

	return check_summary(argv[0]);
}
