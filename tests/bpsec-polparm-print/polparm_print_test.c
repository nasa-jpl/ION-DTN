/*
 * polparm_print_test.c -- regression for the BPSec policy-parameter display
 * path (bpsec_sci_polParmPrint).
 *
 * The function walked a list of security-context parameters, rendered each to
 * a string, and joined the strings.  It advanced its array index twice per
 * item: once when storing the rendered string and again when measuring it.
 * Measuring therefore read the next, never-written slot -- a garbage or null
 * pointer passed to strlen() -- and, with more than one recognized parameter,
 * the next store landed past the end of the array.  A rendered parameter was
 * also dropped from the joined output because half the slots were skipped.
 *
 * This harness builds a list of two recognized integer parameters (the SHA
 * variant and the integrity scope flags of the BIB-HMAC-SHA2 context) whose
 * rendered values are known, calls bpsec_sci_polParmPrint(), and checks that
 * both values appear in the joined result.  On the defective code the call
 * crashes on the garbage pointer or drops the second value; on the fixed code
 * it returns "7 42".
 *
 * bpsec_sci_polParmPrint() reads a shared-memory list, so the harness attaches
 * to a running node; the dotest starts a minimal one.
 */

#include <ion.h>
#include <platform.h>
#include <psm.h>
#include <smlist.h>

#include "sci.h"
#include "sci_structs.h"
#include "sc_value.h"
#include "bib_hmac_sha2_sc.h"

#include <stdio.h>
#include <string.h>

/*	Two integer parameters whose (id, type) are in the BIB-HMAC-SHA2
 *	value map, so each is rendered rather than skipped.		*/

static int	shaVariant = 7;
static int	scopeFlags = 42;

static PsmAddress	makeIntParm(PsmPartition wm, int id, int *value)
{
	PsmAddress	addr;
	sc_value	*val;

	addr = psm_zalloc(wm, sizeof(sc_value));
	if (addr == 0)
	{
		return 0;
	}

	val = (sc_value *) psp(wm, addr);
	val->scValType = SC_VAL_TYPE_PARM;
	val->scValLoc = SC_VAL_STORE_MEM;
	val->scValId = id;
	val->scValLength = sizeof(int);
	val->scRawValue.asPtr = value;
	return addr;
}

int	main(int argc, char **argv)
{
	PsmPartition	wm;
	sc_Def		def;
	PsmAddress	parms;
	PsmAddress	addr;
	char		*out;
	int		rc = 0;

	(void) argc;
	(void) argv;

	if (ionAttach() < 0)
	{
		fprintf(stderr, "SKIP: can't attach to ION.\n");
		return 2;
	}

	wm = getIonwm();

	if (bpsec_sci_defFind(BPSEC_BIB_HMAC_SHA2_SC_ID, &def) != 1)
	{
		fprintf(stderr, "SKIP: BIB-HMAC-SHA2 security context not found.\n");
		ionDetach();
		return 2;
	}

	parms = sm_list_create(wm);
	if (parms == 0)
	{
		fprintf(stderr, "FAIL: can't create parameter list.\n");
		ionDetach();
		return 1;
	}

	addr = makeIntParm(wm, BPSEC_BHSSC_PARM_SHA_VAR_ID, &shaVariant);
	if (addr == 0 || sm_list_insert_last(wm, parms, addr) == 0)
	{
		fprintf(stderr, "FAIL: can't add the first parameter.\n");
		ionDetach();
		return 1;
	}

	addr = makeIntParm(wm, BPSEC_BHSSC_PARM_SCOPE_FLAGS, &scopeFlags);
	if (addr == 0 || sm_list_insert_last(wm, parms, addr) == 0)
	{
		fprintf(stderr, "FAIL: can't add the second parameter.\n");
		ionDetach();
		return 1;
	}

	/*	On the defective code this reads a never-written array slot
	 *	(garbage pointer to strlen) and drops the second value; on the
	 *	fixed code it renders both.				*/

	out = bpsec_sci_polParmPrint(wm, &def, parms);
	if (out == NULL)
	{
		fprintf(stderr, "FAIL: no output from bpsec_sci_polParmPrint.\n");
		ionDetach();
		return 1;
	}

	fprintf(stderr, "INFO: bpsec_sci_polParmPrint returned \"%s\".\n", out);

	if (strstr(out, "7") == NULL || strstr(out, "42") == NULL)
	{
		fprintf(stderr, "FAIL: a rendered parameter is missing from the "
				"output (expected both 7 and 42).\n");
		rc = 1;
	}
	else
	{
		fprintf(stderr, "PASS: both parameters rendered without an "
				"out-of-bounds access.\n");
	}

	MRELEASE(out);
	ionDetach();
	return rc;
}
