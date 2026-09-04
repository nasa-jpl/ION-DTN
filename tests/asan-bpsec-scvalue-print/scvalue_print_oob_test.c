/*
 * scvalue_print_oob_test.c -- AddressSanitizer regression for the BPSec
 * security-context value display path (GHSA-74r8-6qvf-vj4f, residual site).
 *
 * bpsec_scvm_byIdIdxFind() returns -1 when a value's (id, type) is not in the
 * security context's value map. bpsec_scv_smListPrint() used that return value
 * directly as an array index -- reading scvm[-1] out of bounds and calling its
 * scValToStr function pointer -- the same defect that was fixed on the
 * deserialize and serialize paths. The fix skips a value whose lookup fails.
 *
 * This harness builds a one-element value list whose id is not in the
 * BIB-HMAC-SHA2 value map and calls bpsec_scv_smListPrint().  On the vulnerable
 * code the out-of-bounds read is reported by AddressSanitizer; on the fixed code
 * the unknown value is skipped and the call returns without an out-of-bounds
 * access.
 *
 * bpsec_scv_smListPrint() allocates from ION working memory, so the harness
 * attaches to a running node; the dotest starts a minimal one.  Meaningful only
 * under --enable-asan; the dotest skips otherwise.
 */

#include <ion.h>
#include <platform.h>
#include <lyst.h>

#include "sci.h"
#include "sc_value.h"
#include "bib_hmac_sha2_sc.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	sc_Def		def;
	Lyst		vals;
	sc_value	val;
	char		*out;

	(void) argc;
	(void) argv;

	if (ionAttach() < 0)
	{
		fprintf(stderr, "SKIP: can't attach to ION.\n");
		return 2;
	}

	if (bpsec_sci_defFind(BPSEC_BIB_HMAC_SHA2_SC_ID, &def) != 1)
	{
		fprintf(stderr, "SKIP: BIB-HMAC-SHA2 security context not found.\n");
		ionDetach();
		return 2;
	}

	/*	A value whose id is not in the BIB-HMAC-SHA2 value map, so the
	 *	map lookup returns -1.						*/

	memset(&val, 0, sizeof val);
	val.scValType = SC_VAL_TYPE_PARM;
	val.scValId = 16962;
	val.scValLength = 0;

	vals = lyst_create_using(getIonMemoryMgr());
	if (vals == NULL)
	{
		fprintf(stderr, "FAIL: can't create value list.\n");
		ionDetach();
		return 1;
	}

	if (lyst_insert_last(vals, &val) == NULL)
	{
		fprintf(stderr, "FAIL: can't populate value list.\n");
		lyst_destroy(vals);
		ionDetach();
		return 1;
	}

	/*	On the vulnerable code this reads scvm[-1] and calls its
	 *	function pointer; on the fixed code the value is skipped.	*/

	out = bpsec_scv_smListPrint(getIonwm(), &def, vals);
	if (out != NULL)
	{
		MRELEASE(out);
	}

	lyst_destroy(vals);
	ionDetach();

	fprintf(stderr, "PASS: unrecognized SC value did not cause an "
			"out-of-bounds access.\n");
	return 0;
}
