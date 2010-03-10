/*
	bpcounter.c:	a test bundle counter.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

static BpSAP	sap;
static int	bundlesReceived;
static int	running;

static void	printCount()
{
	signal(SIGALRM, printCount);
	printf("%10d bundles received.\n", bundlesReceived);
	fflush(stdout);
	alarm(5);
}

static void	handleQuit()
{
	running = 0;
	bp_interrupt(sap);
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
	Sdr		sdr;
	BpDelivery	dlv;
	time_t		startTime = 0;
	int		bytesReceived;
	time_t		endTime;
	long		interval;

	if (ownEid == NULL)
	{
		puts("Usage: bpcounter <own endpoint ID> [<max count>]");
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

	sdr = bp_get_sdr();
	bundlesReceived = 0;
	bytesReceived = 0;
	signal(SIGALRM, printCount);
	alarm(5);
	signal(SIGINT, handleQuit);
	running = 1;
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
			bundlesReceived++;
			if (bundlesReceived == 1)	/*	First.	*/
			{
				startTime = time(NULL);
			}

			bytesReceived += zco_length(sdr, dlv.adu);
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
		printf("Time: %ld seconds.\n", interval);
		if (interval > 0)
		{
			printf("Throughput: %lu bytes per second.\n",
					bytesReceived / interval);
		}
	}

	bp_close(sap);
	printf("Stopping bpcounter with %d bundles received.\n",
			bundlesReceived);
	bp_detach();
	return 0;
}
