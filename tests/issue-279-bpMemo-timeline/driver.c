/*

	issue-279-bpMemo-timeline/driver.c:	bpMemo test

									*/

#include <bpP.h>
#include "check.h"
#include "testutil.h"

static char myEid[] = "ipn:1.1";
static char testEid[] = "ipn:2.1";
static char testLine[] = "Hello";

int main(int argc, char **argv)
{
	Sdr sdr;
	BpSAP sap;
	int payloadSize = strlen(testLine) + 1;
	Object txExtent;
	Object txBundleZco;
	Object txNewBundle;

	/* Attach to ION */
	fail_unless(bp_attach() == 0);
	fail_unless(bp_open_source(myEid, &sap, 1) == 0);
	sdr = bp_get_sdr();

	/* Send the dummy bundle */
	sdr_begin_xn(sdr);
	txExtent = sdr_malloc(sdr, payloadSize);
	fail_unless(txExtent != 0);
	sdr_write(sdr, txExtent, testLine, payloadSize);
	txBundleZco = ionCreateZco(ZcoSdrSource, txExtent, 0, payloadSize, 0, 0, 0, NULL);
	fail_unless(sdr_end_xn(sdr) == 0 && txBundleZco != 0);
	fail_unless(bp_send(sap, testEid, NULL, 60, BP_STD_PRIORITY,
		SourceCustodyRequired, 0, 0, NULL, txBundleZco, &txNewBundle) == 1);

	/* Post a custody acceptance timeout event for this bundle */
	fail_unless(bpMemo(txNewBundle, 4) == 0);

	/* Sleep for 2 sec, then post a second custody acceptance timeout */
	snooze(2);
	fail_unless(bpMemo(txNewBundle, 4) == 0);
	bp_release(txNewBundle);

	/* Detach from ION */
	writeErrmsgMemos();
	bp_detach();
	exit(0);
}
