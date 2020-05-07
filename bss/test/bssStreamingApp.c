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

typedef struct
{
	BpSAP	sap;
	int	running;
} BptestState;

static BptestState	*_bptestState(BptestState *newState)
{
	void		*value;
	BptestState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	handleQuit(int signum)
{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	PUTS("BP reception interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static int	run_streamingApp(char *ownEid, char *destEid, char *svcClass)
{
	int		priority = 0;
	BpAncillaryData	ancillaryData = { 0, 10, 0 };
			/*	Note: flag value 10 directs BP to send
			 *	bundles using a streaming protocol.	*/
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	BptestState	state = { NULL, 1 };
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

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	oK(_bptestState(&state));

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	if (sdr_heap_depleted(sdr))
	{
		sdr_exit_xn(sdr);
		bp_close(state.sap);
		putErrmsg("Low on heap space, can't initiate streaming.", NULL);
		return 0;
	}

	sdr_exit_xn(sdr);
	isignal(SIGINT, handleQuit);

	writeMemo("[i] bssStreamingApp is running.");

	while(state.running) {

		i++;
		istrcpy(framePayload, itoa(i), sizeof(framePayload));

		CHKZERO(sdr_begin_xn(sdr));
		bundlePayload = sdr_malloc(sdr, sizeof(framePayload));
		if (bundlePayload == 0)
		{
			sdr_cancel_xn(sdr);
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
		switch (bundleZco)
		{
		case 0:
			writeMemoNote("[?] Not enough ZCO space for BSS \
payload", itoa(i));
			sdr_free(sdr, bundlePayload);
			break;

		case (Object) ERROR:
			bundleZco = 0;
			sdr_free(sdr, bundlePayload);
		}

		if (sdr_end_xn(sdr))
		{
			putErrmsg("bssStreamingApp can't create bundle ZCO.",
					NULL);
			break;
		}

		/* Send the bundle payload. */
		if (bundleZco)
		{
			if (bp_send(state.sap, destEid, NULL, 86400, priority,
				custodySwitch, 0, 0, &ancillaryData, bundleZco,
				&newBundle) <= 0)
			{
				putErrmsg("bssStreamingApp can't send frame.",
						NULL);
				break;
			}

			isprintf(info, sizeof info, "A frame with payload: %s \
and size: %d has been sent\n", framePayload, sizeof(framePayload));
			PUTS(info);
		}

		microsnooze(SNOOZE_INTERVAL);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	PUTS("Stopping bssStreamingApp.");
	bp_detach();
	return 0;
}

#if defined (ION_LWT)
int	bssStreamingApp(ssaddr a1, ssaddr a2, ssaddr a3, ssaddr a4, ssaddr a5,
		ssaddr a6, ssaddr a7, ssaddr a8, ssaddr a9, ssaddr a10)
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
		PUTS("\tclass of service: " BP_PARSE_QUALITY_OF_SERVICE_USAGE);
		return 0;
	}

	return run_streamingApp(ownEid, destEid, classOfService);
}
