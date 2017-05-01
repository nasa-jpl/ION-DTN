/*

	1500.loopback-brs/dotest.c:	Loopback test using BRS.

									*/

#include <ion.h>
#include <ionsec.h>
#include <bp.h>
#include <bpP.h>
#include "check.h"
#include "testutil.h"

static BpSAP	rxSap;
static char testEid[] = "ipn:1.1";
static char testLine[] = "Loopback bundle over ION";

void do_brs_startup()
{
	VInduct *vBrscin;
	PsmAddress vBrscinElt;	/* Don't use, but findInduct() requires. */

	sleep(15);

	/* Start the base of the ION node. */
	ionstart_default_config("loopback-brs/loopback.ionrc", 
			 "loopback-brs/loopback.ionsecrc",
			 NULL,
			 NULL, /* Must start bpadmin after adding 1.brs key */
			 NULL, /* Must start ipnadmin after starting bpadmin */
			 NULL);

	/* The ionsecrc file tries to add the key we need, named "1.brs", but when
	 * running with a working directory inside the tests/ tree, it will fail.
	 * We need to provide it a more elaborate path to "1.brs". */
	fail_unless(secAttach() >= 0);
	fail_unless(sec_addKey_default_config("1.brs", "loopback-brs/1.brs") == 1);

	/* Now start BP and IPN portions of node. */
	bpadmin_default_config("loopback-brs/loopback.bprc");
	ionstart_default_config(NULL, NULL, NULL,
			"loopback-brs/loopbackstart.bprc",
			NULL, NULL);
	ipnadmin_default_config("loopback-brs/loopback.ipnrc");

	/* It is possible that the BRS client tried to connect to the BRS server
	 * before the BRS server had started.  We check if the BRS client failed
	 * to startup, and if so, we restart it. */
	sleep(2);
	fail_unless(bp_attach() >= 0);
	findInduct("brsc", "localhost:4556_1", &vBrscin, &vBrscinElt);
	fail_unless(vBrscin != NULL);
	if(! sm_TaskExists(vBrscin->cliPid))
	{
		bpadmin_default_config("loopback-brs/restart-brsc.bprc");
	}
}



int main(int argc, char **argv)
{
	Sdr sdr;
	Object txExtent;
	Object txBundleZco;
	Object txNewBundle;
	BpDelivery rxDlv;
	int rxContentLength;
	ZcoReader rxReader;
	int rxLen;
	char rxContent[sizeof(testLine)];

	do_brs_startup();

	/* Attach to ION */
	fail_unless(bp_attach() >= 0);
	sdr = bp_get_sdr();

	/* Verify our key is installed correctly. */
	{
		char key[20];
		int keyBufLen = sizeof(key);
		fail_unless(sec_get_key("1.brs", &keyBufLen, key) == 20);
	}

	/* Send the loopback bundle */
	sdr_begin_xn(sdr);
	txExtent = sdr_malloc(sdr, sizeof(testLine) - 1);
	fail_unless(txExtent != 0);
	sdr_write(sdr, txExtent, testLine, sizeof(testLine) - 1);
	txBundleZco = ionCreateZco(ZcoSdrSource, txExtent, 0, sizeof(testLine) - 1, 0, 0, 0, NULL);
	fail_unless(sdr_end_xn(sdr) >= 0 && txBundleZco != 0);
	fail_unless(bp_send(NULL, testEid, NULL, 300, BP_STD_PRIORITY,
		NoCustodyRequested, 0, 0, NULL, txBundleZco, &txNewBundle) > 0);

	/* Receive the loopback bundle */
	fail_unless(bp_open(testEid, &rxSap) >= 0);
	fail_unless(bp_receive(rxSap, &rxDlv, IONTEST_DEFAULT_RECEIVE_WAIT) >= 0);
	fail_unless(rxDlv.result == BpPayloadPresent);
	sdr_begin_xn(sdr);
	rxContentLength = zco_source_data_length(sdr, rxDlv.adu);
	fail_unless(rxContentLength == sizeof(testLine) - 1);
	zco_start_receiving(rxDlv.adu, &rxReader);
	rxLen = zco_receive_source(sdr, &rxReader, rxContentLength, 
		rxContent);
	fail_unless(rxLen == rxContentLength);
	fail_unless(sdr_end_xn(sdr) >= 0);
	bp_release_delivery(&rxDlv, 1);
	bp_close(rxSap);

	/* Detach from ION */
	writeErrmsgMemos();
	bp_detach();

	/* Compare the received data */
	rxContent[sizeof(rxContent) - 1] = '\0';
	fail_unless(strncmp(rxContent, testLine, sizeof(testLine)) == 0);

	/* Stop ION */
	ionstop();

	CHECK_FINISH;
}
