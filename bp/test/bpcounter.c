/*
	bpcounter.c:	a test bundle counter.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

static BpSAP	_bpsap(BpSAP *newSAP)
{
	static BpSAP	sap = NULL;
	
	if (newSAP)
	{
		sap = *newSAP;
		sm_TaskVarAdd((int *) &sap);
	}

	return sap;
}

static void	handleQuit()
{
	bp_interrupt(_bpsap(NULL));
}

static int	_bundleCount(int increment)
{
	static int	count = 0;

	if (increment)
	{
		count++;
	}

	return count;
}

static void	printCount()
{
	signal(SIGALRM, printCount);
	PUTMEMO("Bundles received", itoa(_bundleCount(0)));
	fflush(stdout);
	alarm(5);
}

#if defined (VXWORKS) || defined (RTEMS)
int	bpcounter(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*ownEid = (char *) a1;
	int		maxCount = a2;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
	int		maxCount = (argc > 2 ? atoi(argv[2]) : 0);
#endif
	BpSAP		sap;
	Sdr		sdr;
	int		running = 1;
	BpDelivery	dlv;
	time_t		startTime = 0;
	int		bytesReceived;
	int		bundlesReceived = 0;
	time_t		endTime;
	long		interval;

	if (ownEid == NULL)
	{
		PUTS("Usage: bpcounter <own endpoint ID> [<max count>]");
		return 0;
	}

	if (maxCount < 1)
	{
		maxCount = 2000000000;
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
	bundlesReceived = 0;
	bytesReceived = 0;
	isignal(SIGALRM, printCount);
	alarm(5);
	isignal(SIGINT, handleQuit);
	while (running)
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpcounter bundle reception failed.", NULL);
			running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
//putchar('.');
//fflush(stdout);
			if ((bundlesReceived = _bundleCount(1)) == 1)
			{
				startTime = time(NULL);
			}

			bytesReceived += zco_length(sdr, dlv.adu);
		}
		else if (dlv.result == BpReceptionInterrupted)
		{
			running = 0;	/*	Shutdown by ^C.		*/
		}

		bp_release_delivery(&dlv, 1);
		if (bundlesReceived == maxCount)
		{
			running = 0;
		}
	}

	if (bundlesReceived > 0)
	{
		endTime = time(NULL);
		interval = endTime - startTime;
		PUTMEMO("Time (seconds)", itoa(interval));
		if (interval > 0)
		{
			PUTMEMO("Throughput (bytes per second)",
					itoa(bytesReceived / interval));
		}
	}

	bp_close(sap);
	PUTMEMO("Stopping bpcounter; bundles received", itoa(bundlesReceived));
	bp_detach();
	return 0;
}
