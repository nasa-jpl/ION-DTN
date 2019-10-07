/*

	1400.loopback/dotest.c:	Loopback test using simple TCP.

									*/

#include <bp.h>
#include "check.h"
#include "testutil.h"

static BpSAP	rxSap;
static char testEid[] = "ipn:1.1";
static char testLine[] = "Loopback bundle over ION";

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

	/* Start ION */
	ionstart_default_config("loopback-stcp/loopback.ionrc", 
			 NULL,
			 NULL,
			 "loopback-stcp/loopback.bprc",
			 "loopback-stcp/loopback.ipnrc",
			 NULL);

	/* Attach to ION */
	fail_unless(bp_attach() >= 0);
	sdr = bp_get_sdr();

	/* Send the loopback bundle */
	sdr_begin_xn(sdr);
	txExtent = sdr_malloc(sdr, sizeof(testLine) - 1);
	fail_unless(txExtent != 0);
	sdr_write(sdr, txExtent, testLine, sizeof(testLine) - 1);
	txBundleZco = zco_create(sdr, ZcoSdrSource, txExtent, 0, sizeof(testLine) - 1, ZcoOutbound);
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
