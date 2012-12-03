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

static BpSAP	_bpsap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;
	
	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static int	_running(int *newState)
{
	void	*value = NULL;
	BpSAP	sap;

	if (newState)			/*	Only used for Stop.	*/
	{
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return (sap == NULL ? 0 : 1);
}

static void	handleQuit()
{
	int	stop = 0;

	bp_interrupt(_bpsap(NULL));
	oK(_running(&stop));
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
	BpSAP		sap;
	Sdr		sdr;
	char		dataToSend[ADU_LEN];
	Object		bundleZco;
	Object		newBundle;
	Object		extent;
	BpDelivery	dlv;
	int		stop = 0;
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

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", NULL);
		return 0;
	}

	oK(_bpsap(&sap));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	while (1)
	{
		/*	Wait for a bundle from the driver.		*/

		while (_running(NULL))
		{
			if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
			{
				bp_close(sap);
				putErrmsg("bpecho bundle reception failed.",
						NULL);
				return 1;
			}

putchar(dlvmarks[dlv.result]);
fflush(stdout);
			if (dlv.result == BpEndpointStopped
			|| (dlv.result == BpReceptionInterrupted
					&& _running(NULL) == 0))
			{
				oK(_running(&stop));
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
					oK(_running(&stop));
					continue;
				}

				bp_release_delivery(&dlv, 1);
				break;	/*	Out of reception loop.	*/
			}

			bp_release_delivery(&dlv, 1);
		}

		if (_running(NULL) == 0)
		{
			/*	Benchmark run terminated.		*/

			break;		/*	Out of main loop.	*/
		}

		/*	Now send acknowledgment bundle.			*/
		if(strcmp(sourceEid, "dtn:none") == 0) continue;
		CHKZERO(sdr_begin_xn(sdr));
		extent = sdr_malloc(sdr, bytesToEcho);
		if (extent == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for ZCO extent.", NULL);
			break;		/*	Out of main loop.	*/
		}

		sdr_write(sdr, extent, dataToSend, bytesToEcho);
		bundleZco = zco_create(sdr, ZcoSdrSource, extent, 0,
				bytesToEcho);
		if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
		{
			putErrmsg("Can't create ZCO.", NULL);
			break;		/*	Out of main loop.	*/
		}

		if (bp_send(sap, BP_BLOCKING, sourceEid, NULL, 300,
				BP_STD_PRIORITY, NoCustodyRequested,
				0, 0, NULL, bundleZco, &newBundle) < 1)
		{
			putErrmsg("bpecho can't send echo bundle.", NULL);
			break;		/*	Out of main loop.	*/
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
