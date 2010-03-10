/*
	bpecho.c:	receiver for bundle benchmark test.
									*/
/*									*/
/*	Copyright (c) 2004, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*	Enhanced by Ryan Metzger (MITRE Corp.) August 2006		*/
/*									*/

#include <bp.h>

#define ADU_LEN	(2)

#if 0
#define	CYCLE_TRACE
#endif

/*	Indication marks:	"." for BpPayloadPresent (1),
				"*" for BpReceptionTimedOut (2).
 				"!" for BpReceptionInterrupted (3).	*/

static char	dlvmarks[] = "?.*!";

static int	running;
static BpSAP	sap;

static void	handleQuit()
{
	running = 0;
	bp_interrupt(sap);
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
	Sdr		sdr;
	char		dataToSend[ADU_LEN] = "x";
	Object		bundleZco;
	Object		newBundle;
	Object		extent;
	BpDelivery	dlv;
	char		sourceEid[1024];

	if (ownEid == NULL)
	{
		puts("Usage: bpecho <own endpoint ID>");
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
	
	sdr = bp_get_sdr();
	signal(SIGINT, handleQuit);
	running = 1;
	while (1)
	{
		/*	Wait for a bundle from the driver.		*/

		while (running)
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
			if (dlv.result == BpPayloadPresent)
			{
				strcpy(sourceEid, dlv.bundleSourceEid);
				bp_release_delivery(&dlv, 1);
				break;
			}

			bp_release_delivery(&dlv, 1);
		}

		if (!running)	/*	Benchmark run terminated.	*/
		{
			break;
		}

		/*	Now send acknowledgment bundle.			*/

		sdr_begin_xn(sdr);
		extent = sdr_malloc(sdr, ADU_LEN);
		if (extent == 0)
		{
			sdr_cancel_xn(sdr);
			putSysErrmsg("No space for ZCO extent", NULL);
			break;
		}

		sdr_write(sdr, extent, dataToSend, ADU_LEN);
		bundleZco = zco_create(sdr, ZcoSdrSource, extent, 0, ADU_LEN);
		if (sdr_end_xn(sdr) < 0 || bundleZco == 0)
		{
			putErrmsg("Can't create ZCO.", NULL);
			break;
		}

		if (bp_send(sap, BP_BLOCKING, sourceEid, NULL, 300,
				BP_STD_PRIORITY, NoCustodyRequested,
				0, 0, NULL, bundleZco, &newBundle) < 1)
		{
			putSysErrmsg("bpecho can't send echo bundle", NULL);
			break;
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
