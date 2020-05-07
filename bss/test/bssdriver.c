/*
 *	bssdriver.c:	a test program that creates and sends a
 *			delimited stream of bundles.
 *
 *	BSS Driver Specifications			
 *	Simulated Compression: H.264/MPEG-4
 *	Resolution: 1280×720 @ 30fps. Constant Bit Rate: 3Mbps
 *
 *	Adapted from bssStreamingApp.c, written by Sotirios-Angelos
 *	Lenas, Democritus University of Thrace.
 *								
 *	Copyright (c) 2012, California Institute of Technology.	
 *
 *	All rights reserved.						
 *	
 *	Author: Scott Burleigh	 
 */			

#include "bp.h"
#include "bsstest.h"

static int	run_bssdriver(char *ownEid, char *destEid, long bundlesToSend,
			char *svcClass)
{
	int		priority = 0;
	BpAncillaryData	ancillaryData = { 0, 10, 0 };
			/*	Note: flag value 10 directs BP to send
			 *	bundles using a streaming protocol.	*/
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	BpSAP		sap;
	Sdr		sdr;
	unsigned int	i = 0;
	unsigned int	dataValue;
	Object		bundlePayload;
	Object		bundleZco;
	Object		newBundle;

	/*	bitrate = 3Mbps, CBR = 20866 bytes per 55642 usec	*/    

	char		framePayload[RCV_LENGTH];

	if (svcClass == NULL)
	{
		priority = BP_STD_PRIORITY;
	}
	else
	{
		if (!bp_parse_quality_of_service(svcClass, &ancillaryData,
				&custodySwitch, &priority))
		{
			putErrmsg("Invalid class of service for bpsendfile.",
					svcClass);
			return 0;
		}
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	if (sdr_heap_depleted(sdr))
	{
		sdr_exit_xn(sdr);
		bp_close(sap);
		putErrmsg("Low on heap space, can't initiate streaming.", NULL);
		return 0;
	}

	sdr_exit_xn(sdr);
	writeMemo("[i] bssdriver is running.");
	while (bundlesToSend > 0)
	{
		i++;
		dataValue = htonl(i);
		memcpy(framePayload, (char *) &dataValue, sizeof(unsigned int));
		CHKZERO(sdr_begin_xn(sdr));
		bundlePayload = sdr_malloc(sdr, sizeof(framePayload));
		if (bundlePayload == 0)
		{
			sdr_cancel_xn(sdr);
			bp_close(sap);
			putErrmsg("No space for frame payload.", NULL);
			break;
		}
		
		sdr_write(sdr, bundlePayload, framePayload,
				sizeof(framePayload));

		/*	Note: we don't use blocking ionCreateZco here
		 *	because we don't want to block in admission
		 *	control.  The transmission loop is metered by
		 *	time.						*/

		bundleZco = ionCreateZco(ZcoSdrSource, bundlePayload, 0, 
				sizeof(framePayload), priority,
				ancillaryData.ordinal, ZcoOutbound, NULL);
		if (sdr_end_xn(sdr) < 0 || bundleZco == (Object) ERROR
		|| bundleZco == 0)
		{
			bp_close(sap);
			putErrmsg("bssdriver can't create bundle ZCO.", NULL);
			break;
		}

		/*	Send the bundle payload.	*/

		if (bp_send(sap, destEid, NULL, 86400, priority, custodySwitch,
			0, 0, &ancillaryData, bundleZco, &newBundle) <= 0)
		{
			putErrmsg("bssdriver can't send frame.", NULL);
			break;
		}

		bundlesToSend--;
		microsnooze(SNOOZE_INTERVAL);
	}

	bp_close(sap);
	writeErrmsgMemos();
	puts("Stopping bssdriver.");
	bp_detach();
	return 0;
}

#if defined (ION_LWT)
int	bssdriver(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ownEid = (char *) a1;
	char	*destEid = (char *) a2;
	long	nbrOfBundles = strtol((char *) a3, NULL, 0);
	char	*classOfService = (char *) a4;
#else
int	main(int argc, char **argv)
{
	char	*ownEid = NULL;
	char	*destEid = NULL;
	long	nbrOfBundles = 0;
	char	*classOfService = NULL;

	if (argc > 5) argc = 5;
	switch (argc)
	{
	case 5:
		classOfService = argv[4];
	case 4:
		nbrOfBundles = strtol(argv[3], NULL, 0);
	case 3:
		destEid = argv[2];
	case 2:
		ownEid = argv[1];
	default:
		break;
	}
#endif
	if (ownEid == NULL || destEid == NULL || nbrOfBundles < 1)
	{
		puts("Usage: bssdriver <own endpoint ID> <destination \
endpoint ID> <number of bundles> [<class of service>]");
		puts("\tclass of service: " BP_PARSE_QUALITY_OF_SERVICE_USAGE);
		return 0;
	}

	return run_bssdriver(ownEid, destEid, nbrOfBundles, classOfService);
}
