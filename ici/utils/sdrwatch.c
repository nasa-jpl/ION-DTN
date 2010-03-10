/*

	sdrwatch.c:	SDR activity monitor.

									*/
/*									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "sdr.h"

static int	sdrwatch_count = 0;

static void	handleQuit()
{
	puts("[Terminated by user.]");
	sdrwatch_count = 1;	/*	Advance to end of last cycle.	*/
}

static int	run_sdrwatch(char *sdrName, int interval, int verbose)
{
	Sdr		sdr;
	SdrUsageSummary	sdrsummary;

	if (interval > 0)
	{
		if (sdrwatch_count < 0)
		{
			puts("count must be >= 0 cycles");
			return 0;
		}
	}

	sdr_initialize(0, NULL, SM_NO_KEY, NULL);
	sdr = sdr_start_using(sdrName);
	if (sdr == NULL)
	{
		putSysErrmsg("Can't attach to sdr.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	/*	Initial state.						*/

	sdr_usage(sdr, &sdrsummary);
	sdr_report(&sdrsummary);
	if (interval == 0)	/*	One-time poll.			*/
	{
		return 0;
	}

	/*	Start watching trace.					*/

	if (sdr_start_trace(sdr, 5000000, NULL) < 0)
	{
		putSysErrmsg("Can't start trace.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	signal(SIGTERM, handleQuit);
	while (sdrwatch_count > 0)
	{
		snooze(interval);
		sdr_print_trace(sdr, verbose);
	       	sdr_clear_trace(sdr);
		sdrwatch_count--;
	}

	sdr_stop_trace(sdr);
	sdr_stop_using(sdr);
	writeErrmsgMemos();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	sdrwatch(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*sdrName = (char *) a1;
	int	interval = a2;
	int	verbose = 0;

	sdrwatch_count = a3;
	if (interval > 1)
	{
		verbose = a4;
	}
#else
int	main(int argc, char **argv)
{
	char	*sdrName;
	int	interval;
	int	verbose = 0;

	if (argc < 4)
	{
		puts("Usage: sdrwatch <sdr name> <interval> <count> [verbose]");
		return 0;
	}

	sdrName = argv[1];
	interval = atoi(argv[2]);
	if (interval < 0)
	{
		puts("interval must be >= 0 seconds");
		return 0;
	}

	sdrwatch_count = atoi(argv[3]);
	if (interval > 0)
	{
		if (argc > 4)
		{
			verbose = 1;
		}
	}
#endif
	return run_sdrwatch(sdrName, interval, verbose);
}
