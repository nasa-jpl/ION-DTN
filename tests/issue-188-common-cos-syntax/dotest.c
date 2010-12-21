/*

	issue-188-common-cos-syntax/dotest.c:	Tests common class-of-service. 

									*/

#include <bp.h>
#include "check.h"
#include "testutil.h"


static void run_cos_case(const char *token, BpExtendedCOS desiredExtendedCOS,
		BpCustodySwitch desiredCustodySwitch, int desiredPriority)
{
	BpExtendedCOS	extendedCOS = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch;
	int				priority;

	fail_unless(bp_parse_class_of_service(token, &extendedCOS, &custodySwitch,
											&priority) == 1);


	fail_unless(extendedCOS.flowLabel == desiredExtendedCOS.flowLabel); 
	fail_unless(extendedCOS.flags == desiredExtendedCOS.flags); 
	fail_unless(extendedCOS.ordinal == desiredExtendedCOS.ordinal); 
	fail_unless(custodySwitch == desiredCustodySwitch);
	fail_unless(priority == desiredPriority);
}

static void run_invalid_cos_case(const char *token)
{
	BpExtendedCOS	extendedCOS = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch;
	int				priority;

	fail_unless(bp_parse_class_of_service(token, &extendedCOS, &custodySwitch,
											&priority) == 0);
}

int main(int argc, char **argv)
{
	BpExtendedCOS desiredExtendedCOS = { 0, 0, 0 };

    /* Only one arg: invalid args. */
    run_invalid_cos_case("0");
    run_invalid_cos_case("1");
    run_invalid_cos_case("2");
    run_invalid_cos_case("X");      /* Not even an int. */
    run_invalid_cos_case("-");      /* Not even an int. */
    run_invalid_cos_case(" ");      /* Not even an int. */
    run_invalid_cos_case("");       /* Not even an int. */

	/* Only two args: custody.priority . */
	run_cos_case("0.2", desiredExtendedCOS, NoCustodyRequested, 2);
	run_cos_case("0.1", desiredExtendedCOS, NoCustodyRequested, 1);
	run_cos_case("0.0", desiredExtendedCOS, NoCustodyRequested, 0);
	run_cos_case("1.2", desiredExtendedCOS, SourceCustodyRequired, 2);
	run_cos_case("1.1", desiredExtendedCOS, SourceCustodyRequired, 1);
	run_cos_case("1.0", desiredExtendedCOS, SourceCustodyRequired, 0);

    /* Custody must be 0 or 1, and priority must be 0, 1, or 2. */
    run_invalid_cos_case("2.0");
    run_invalid_cos_case("1.3");
    run_invalid_cos_case("0.3");

    /* Three args: custody.priority.ordinal . */
    desiredExtendedCOS.ordinal = 0;
    run_cos_case("1.0.0", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0", desiredExtendedCOS, SourceCustodyRequired, 1);
    run_cos_case("1.2.0", desiredExtendedCOS, SourceCustodyRequired, 2);
    desiredExtendedCOS.ordinal = 0;
    run_cos_case("0.0.0", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("0.1.0", desiredExtendedCOS, NoCustodyRequested, 1);
    run_cos_case("0.2.0", desiredExtendedCOS, NoCustodyRequested, 2);
    desiredExtendedCOS.ordinal = 1;
    run_cos_case("1.0.1", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.1", desiredExtendedCOS, SourceCustodyRequired, 1);
    run_cos_case("1.2.1", desiredExtendedCOS, SourceCustodyRequired, 2);
    desiredExtendedCOS.ordinal = 254;
    run_cos_case("1.0.254", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.254", desiredExtendedCOS, SourceCustodyRequired, 1);
    run_cos_case("1.2.254", desiredExtendedCOS, SourceCustodyRequired, 2);

    /* Can't request ordinal 255; it's reserved for admin bundles that can't
     * be sent via the bp_send() interface. */
    run_invalid_cos_case("0.0.255");
    run_invalid_cos_case("0.1.255");
    run_invalid_cos_case("0.2.255");
    run_invalid_cos_case("1.0.255");
    run_invalid_cos_case("1.1.255");
    run_invalid_cos_case("1.2.255");

    /* Can't have exactly 4 args. */
    run_invalid_cos_case("0.0.0.0");
    run_invalid_cos_case("1.0.0.0");
    run_invalid_cos_case("1.1.0.0");
    run_invalid_cos_case("1.1.1.0");
    run_invalid_cos_case("1.1.1.1");

    /* Five args: custody.priority.ordinal.unreliable.critical */
    desiredExtendedCOS.ordinal = 0;
    desiredExtendedCOS.flags = 0;
    run_cos_case("0.0.0.0.0", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_MINIMUM_LATENCY;
    run_cos_case("0.0.0.0.1", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.1", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.1", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_BEST_EFFORT | BP_MINIMUM_LATENCY;
    run_cos_case("0.0.0.1.1", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.1", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.1", desiredExtendedCOS, SourceCustodyRequired, 1);

    /* Critical and unreliable must be 0 or 1. */
    run_invalid_cos_case("0.0.0.0.2");
    run_invalid_cos_case("0.0.0.0.17");
    run_invalid_cos_case("0.0.0.2.0");
    run_invalid_cos_case("0.0.0.17.0");

    /* Six args: custody.priority.ordinal.unreliable.critical.flow-label */
    desiredExtendedCOS.ordinal = 0;
    desiredExtendedCOS.flags = BP_FLOW_LABEL_PRESENT;
    desiredExtendedCOS.flowLabel = 0;
    run_cos_case("0.0.0.0.0.0", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0.0", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0.0", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_FLOW_LABEL_PRESENT | BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0.0", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0.0", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0.0", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_FLOW_LABEL_PRESENT;
    desiredExtendedCOS.flowLabel = 42;
    run_cos_case("0.0.0.0.0.42", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0.42", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0.42", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_FLOW_LABEL_PRESENT | BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0.42", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0.42", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0.42", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_FLOW_LABEL_PRESENT;
    desiredExtendedCOS.flowLabel = 65535;
    run_cos_case("0.0.0.0.0.65535", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0.65535", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0.65535", desiredExtendedCOS, SourceCustodyRequired, 1);
    desiredExtendedCOS.flags = BP_FLOW_LABEL_PRESENT | BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0.65535", desiredExtendedCOS, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0.65535", desiredExtendedCOS, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0.65535", desiredExtendedCOS, SourceCustodyRequired, 1);

	CHECK_FINISH;
}
