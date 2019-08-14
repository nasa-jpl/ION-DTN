/*

	issue-188-common-cos-syntax/dotest.c:	Tests common class-of-service. 

									*/

#include <bp.h>
#include "check.h"
#include "testutil.h"


static void run_cos_case(const char *token, BpAncillaryData desiredAncillaryData,
		BpCustodySwitch desiredCustodySwitch, int desiredPriority)
{
	//BpExtendedCOS	extendedCOS = { 0, 0, 0 };
	BpAncillaryData	ancillaryData = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch;
	int				priority;

	fail_unless(bp_parse_quality_of_service(token, &ancillaryData, &custodySwitch,
											&priority) == 1);


	fail_unless(ancillaryData.dataLabel == desiredAncillaryData.dataLabel); 
	fail_unless(ancillaryData.flags == desiredAncillaryData.flags); 
	fail_unless(ancillaryData.ordinal == desiredAncillaryData.ordinal); 
	fail_unless(custodySwitch == desiredCustodySwitch);
	fail_unless(priority == desiredPriority);
}

static void run_invalid_cos_case(const char *token)
{
	//BpExtendedCOS	extendedCOS = { 0, 0, 0 };
BpAncillaryData	ancillaryData = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch;
	int				priority;

	fail_unless(bp_parse_quality_of_service(token, &ancillaryData, &custodySwitch,
											&priority) == 0);
}

int main(int argc, char **argv)
{
	//BpExtendedCOS desiredExtendedCOS = { 0, 0, 0 };
	BpAncillaryData	desiredAncillaryData = { 0, 0, 0 };

    /* Only one arg: invalid args. */
    run_invalid_cos_case("0");
    run_invalid_cos_case("1");
    run_invalid_cos_case("2");
    run_invalid_cos_case("X");      /* Not even an int. */
    run_invalid_cos_case("-");      /* Not even an int. */
    run_invalid_cos_case(" ");      /* Not even an int. */
    run_invalid_cos_case("");       /* Not even an int. */

	/* Only two args: custody.priority . */
	run_cos_case("0.2", desiredAncillaryData, NoCustodyRequested, 2);
	run_cos_case("0.1", desiredAncillaryData, NoCustodyRequested, 1);
	run_cos_case("0.0", desiredAncillaryData, NoCustodyRequested, 0);
	run_cos_case("1.2", desiredAncillaryData, SourceCustodyRequired, 2);
	run_cos_case("1.1", desiredAncillaryData, SourceCustodyRequired, 1);
	run_cos_case("1.0", desiredAncillaryData, SourceCustodyRequired, 0);

    /* Custody must be 0 or 1, and priority must be 0, 1, or 2. */
    run_invalid_cos_case("2.0");
    run_invalid_cos_case("1.3");
    run_invalid_cos_case("0.3");

    /* Three args: custody.priority.ordinal . */
    desiredAncillaryData.ordinal = 0;
    run_cos_case("1.0.0", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0", desiredAncillaryData, SourceCustodyRequired, 1);
    run_cos_case("1.2.0", desiredAncillaryData, SourceCustodyRequired, 2);
    desiredAncillaryData.ordinal = 0;
    run_cos_case("0.0.0", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("0.1.0", desiredAncillaryData, NoCustodyRequested, 1);
    run_cos_case("0.2.0", desiredAncillaryData, NoCustodyRequested, 2);
    desiredAncillaryData.ordinal = 1;
    run_cos_case("1.0.1", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.1", desiredAncillaryData, SourceCustodyRequired, 1);
    run_cos_case("1.2.1", desiredAncillaryData, SourceCustodyRequired, 2);
    desiredAncillaryData.ordinal = 254;
    run_cos_case("1.0.254", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.254", desiredAncillaryData, SourceCustodyRequired, 1);
    run_cos_case("1.2.254", desiredAncillaryData, SourceCustodyRequired, 2);

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
    desiredAncillaryData.ordinal = 0;
    desiredAncillaryData.flags = 0;
    run_cos_case("0.0.0.0.0", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_MINIMUM_LATENCY;
    run_cos_case("0.0.0.0.1", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.1", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.1", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_BEST_EFFORT | BP_MINIMUM_LATENCY;
    run_cos_case("0.0.0.1.1", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.1", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.1", desiredAncillaryData, SourceCustodyRequired, 1);

    /* Critical and unreliable must be 0 or 1. */
    run_invalid_cos_case("0.0.0.0.2");
    run_invalid_cos_case("0.0.0.0.17");
    run_invalid_cos_case("0.0.0.2.0");
    run_invalid_cos_case("0.0.0.17.0");

    /* Six args: custody.priority.ordinal.unreliable.critical.flow-label */
    desiredAncillaryData.ordinal = 0;
    desiredAncillaryData.flags = BP_DATA_LABEL_PRESENT;
    desiredAncillaryData.dataLabel = 0;
    run_cos_case("0.0.0.0.0.0", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0.0", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0.0", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_DATA_LABEL_PRESENT | BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0.0", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0.0", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0.0", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_DATA_LABEL_PRESENT;
    desiredAncillaryData.dataLabel = 42;
    run_cos_case("0.0.0.0.0.42", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0.42", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0.42", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_DATA_LABEL_PRESENT | BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0.42", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0.42", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0.42", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_DATA_LABEL_PRESENT;
    desiredAncillaryData.dataLabel = 65535;
    run_cos_case("0.0.0.0.0.65535", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.0.0.65535", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.0.0.65535", desiredAncillaryData, SourceCustodyRequired, 1);
    desiredAncillaryData.flags = BP_DATA_LABEL_PRESENT | BP_BEST_EFFORT;
    run_cos_case("0.0.0.1.0.65535", desiredAncillaryData, NoCustodyRequested, 0);
    run_cos_case("1.0.0.1.0.65535", desiredAncillaryData, SourceCustodyRequired, 0);
    run_cos_case("1.1.0.1.0.65535", desiredAncillaryData, SourceCustodyRequired, 1);

	CHECK_FINISH;
}
