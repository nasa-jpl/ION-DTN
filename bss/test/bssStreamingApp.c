/*
 *	bssStreamingApp.c:	a test program that creates and sends a 
 *				stream of bundles.
 *									
 *	BSS Streaming Application Specifications			
 *	Simulated Compression: H.264/MPEG-4
 *	Resolution: 1280×720 @ 30fps. Constant Bit Rate: 3Mbps
 *								
 *	Copyright (c) 2011, California Institute of Technology.	
 *	Copyright (c) 2011, Space Internetworking Center,
 *	Democritus University of Thrace.
 *
 *	All rights reserved.						
 *	
 *	Authors: Sotirios-Angelos Lenas, SPICE	 
 */			

#include "bp.h"
#include "bsstest.h"

static int 	running = 1;

static BpSAP	_bpsap(BpSAP *newSAP)
{
	void	*value;
	BpSAP	sap;

	if (newSAP)			/*	Add task variable.	*/
	{
		value = (void *) (*newSAP);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static void	handleQuit()
{
	void	*erase = NULL;

	bp_interrupt(_bpsap(NULL));
	oK(sm_TaskVar(&erase));
	running = 0;
}

static int	run_streamingApp(char *ownEid, char *destEid, char *svcClass)
{
	int		priority = 0;
	BpExtendedCOS	extendedCOS = { 0, 0, 0 };

	/*
         * ******************************************************
	 * BSS traffic bundles must always be marked as custodial
         * ******************************************************
	 */ 
	BpCustodySwitch	custodySwitch = SourceCustodyRequired;
	BpSAP		sap;
	Sdr		sdr;
	Object		bundlePayload;
	Object		bundleZco;
	Object		newBundle;
	int 		i=0;

	/*	bitrate = 3Mbps, CBR = 20866 bytes per 55642 usec	*/    

	char		framePayload[RCV_LENGTH];
	char		info[100];

	if (svcClass == NULL)
	{
		priority = BP_STD_PRIORITY;
	}
	else
	{
		if (!bp_parse_class_of_service(svcClass, &extendedCOS,
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

	oK(_bpsap(&sap));

	sdr = bp_get_sdr();
	if (sdr_heap_depleted(sdr))
	{
		bp_close(sap);
		putErrmsg("Low on heap space, can't initiate streaming.", NULL);
		return 0;
	}

	isignal(SIGINT, handleQuit);

	writeMemo("[i] bssStreamingApp is running.");

	while(running) {

		i++;
		istrcpy(framePayload, itoa(i), sizeof(framePayload));

		sdr_begin_xn(sdr);
		bundlePayload = sdr_malloc(sdr, sizeof(framePayload));
		if(bundlePayload == 0) {
			sdr_cancel_xn(sdr);
			bp_close(sap);
			putErrmsg("No space for frame payload.", NULL);
			break;
		}
		
		sdr_write(sdr, bundlePayload, framePayload, sizeof(framePayload));
		bundleZco = zco_create(sdr, ZcoSdrSource, bundlePayload, 0, 
				sizeof(framePayload));
		if(sdr_end_xn(sdr) < 0 || bundleZco == 0)
		{
			bp_close(sap);
			putErrmsg("bssStreamingApp can't create bundle ZCO.", NULL);
			break;
		}

		/* Send the bundle payload. */
		if(bp_send(sap, BP_BLOCKING, destEid, NULL, 86400,
		   priority, custodySwitch, 0, 0, &extendedCOS, bundleZco, 
		   &newBundle) <= 0)
		{
			putErrmsg("bssStreamingApp can't send frame.", NULL);
			break;
		}

		isprintf(info, sizeof info, "A frame with payload: %s and \
size: %d has been sent\n", framePayload, sizeof(framePayload));
		PUTS(info);
		microsnooze(SNOOZE_INTERVAL);
	}

	bp_close(sap);
	writeErrmsgMemos();
	PUTS("Stopping bssStreamingApp.");
	bp_detach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	bssStreamingApp(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ownEid = (char *) a2;
	char	*destEid = (char *) a3;
	char	*classOfService = (char *) a4;
#else
int	main(int argc, char **argv)
{
	char	*ownEid = NULL;
	char	*destEid = NULL;
	char	*classOfService = NULL;

	if (argc > 4) argc = 4;
	switch (argc)
	{
	case 4:
		classOfService = argv[3];
	case 3:
		destEid = argv[2];
	case 2:
		ownEid = argv[1];
	default:
		break;
	}
#endif
	if (ownEid == NULL || destEid == NULL)
	{
		PUTS("Usage: bssStreamingApp <own endpoint ID> <destination \
endpoint ID> [<class of service>]");
		PUTS("\tclass of service: " BP_PARSE_CLASS_OF_SERVICE_USAGE);
		return 0;
	}

	return run_streamingApp(ownEid, destEid, classOfService);
}