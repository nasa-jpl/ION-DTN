/*
	bpecho.c:	receiver for bundle benchmark test.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*	Enhanced by Ryan Metzger (MITRE Corp.) August 2006		*/
/*	Andrew Jenkins <andrew.jenkins@colorado.edu> made it echo received 
		data, March 2009 			*/

#include <bp.h>

#define ADU_LEN	(1024)

#if 0
#define	CYCLE_TRACE
#endif

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

static void	_zcoControl(int *controlPtr)
{
	static int	*ptr = NULL;

	if (controlPtr)	/*	Initializing ZCO request cancellation.	*/
	{
		ptr = controlPtr;
	}
	else		/*	Canceling ZCO request.			*/
	{
		ionCancelZcoSpaceRequest(ptr);
	}
}

static void	handleQuit()
{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	PUTS("BP reception interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpecho(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ownEid = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*ownEid = (argc > 1 ? argv[1] : NULL);
#endif
/*	Indication marks:	"." for BpPayloadPresent (1),
				"*" for BpReceptionTimedOut (2).
 				"!" for BpReceptionInterrupted (3).
				"X" for BpEndpointStopped (4).	*/
	static char	dlvmarks[] = "?.*!X";
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	char		dataToSend[ADU_LEN];
	Object		bundleZco;
	int		controlZco;
	Object		newBundle;
	Object		extent;
	BpDelivery	dlv;
	ZcoReader	reader;
 	char		sourceEid[1024];
	int		bytesToEcho = 0;
	int		result;

	if (ownEid == NULL)
	{
		PUTS("Usage: bpecho <own endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", NULL);
		return 0;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	_zcoControl(&controlZco);
	isignal(SIGINT, handleQuit);
	while (1)
	{
		/*	Wait for a bundle from the driver.		*/

		while (state.running)
		{
			if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
			{
				bp_close(state.sap);
				putErrmsg("bpecho bundle reception failed.",
						NULL);
				return 1;
			}

putchar(dlvmarks[dlv.result]);
fflush(stdout);
			if (dlv.result == BpEndpointStopped
			|| (dlv.result == BpReceptionInterrupted
					&& state.running == 0))
			{
				state.running = 0;
				continue;
			}

			if (dlv.result == BpPayloadPresent)
			{
				istrcpy(sourceEid, dlv.bundleSourceEid,
						sizeof sourceEid);
				bytesToEcho = MIN(zco_source_data_length(sdr,
						dlv.adu), ADU_LEN);
				zco_start_receiving(dlv.adu, &reader);
				CHKZERO(sdr_begin_xn(sdr));
				result = zco_receive_source(sdr, &reader,
						bytesToEcho, dataToSend);
				if (sdr_end_xn(sdr) < 0 || result < 0)
				{
					putErrmsg("Can't receive payload.",
							NULL);
					state.running = 0;
					continue;
				}

				bp_release_delivery(&dlv, 1);
				break;	/*	Out of reception loop.	*/
			}

			bp_release_delivery(&dlv, 1);
		}

		if (state.running == 0)
		{
			/*	Benchmark run terminated.		*/

			break;		/*	Out of main loop.	*/
		}

		/*	Now send acknowledgment bundle.			*/
		if (strcmp(sourceEid, "dtn:none") == 0) continue;
		CHKZERO(sdr_begin_xn(sdr));
		extent = sdr_malloc(sdr, bytesToEcho);
		if (extent)
		{
			sdr_write(sdr, extent, dataToSend, bytesToEcho);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("No space for ZCO extent.", NULL);
			break;		/*	Out of main loop.	*/
		}

		bundleZco = ionCreateZco(ZcoSdrSource, extent, 0,
				bytesToEcho, &controlZco);
		if (bundleZco == 0)
		{
			putErrmsg("Can't create ZCO.", NULL);
			break;		/*	Out of main loop.	*/
		}

		if (bp_send(state.sap, sourceEid, NULL, 300, BP_STD_PRIORITY,
				NoCustodyRequested, 0, 0, NULL, bundleZco,
				&newBundle) < 1)
		{
			putErrmsg("bpecho can't send echo bundle.", NULL);
			break;		/*	Out of main loop.	*/
		}
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
