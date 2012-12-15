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

static unsigned int	sdrwatch_count(int *newValue)
{
	static unsigned int	count = 1;

	if (newValue)
	{
		if (*newValue == 0)	/*	Decrement.		*/
		{
			count--;
		}
		else			/*	Initialize.		*/
		{
			count = *newValue;
		}
	}

	return count;
}

static void	handleQuit()
{
	int	newCount = 1;	/*	Advance to end of last cycle.	*/

	PUTS("[Terminated by user.]");
	oK(sdrwatch_count(&newCount));
}

static int	run_sdrwatch(char *sdrName, int interval, int verbose)
{
	Sdr		sdr;
	SdrUsageSummary	sdrsummary;
	int		secRemaining;
	int		decrement = 0;

	sdr_initialize(0, NULL, SM_NO_KEY, NULL);
	sdr = sdr_start_using(sdrName);
	if (sdr == NULL)
	{
		putErrmsg("Can't attach to sdr.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	/*	Initial state.						*/

	CHKERR(sdr_begin_xn(sdr));
	sdr_usage(sdr, &sdrsummary);
	sdr_report(&sdrsummary);
	sdr_exit_xn(sdr);
	if (interval == 0)	/*	One-time poll.			*/
	{
		return 0;
	}

	/*	Start watching trace.					*/

	if (sdr_start_trace(sdr, 20000000, NULL) < 0)
	{
		putErrmsg("Can't start trace.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	isignal(SIGTERM, handleQuit);
	while (sdrwatch_count(NULL) > 0)
	{
		secRemaining = interval;
		while (secRemaining > 0)
		{
			snooze(1);
			secRemaining--;
			if (!verbose)
			{
				sdr_clear_trace(sdr);
			}
		}

		sdr_print_trace(sdr, verbose);
		oK(sdrwatch_count(&decrement));
	}

	sdr_stop_trace(sdr);
	sdr_stop_using(sdr);
	writeErrmsgMemos();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	sdrwatch(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*sdrName = (char *) a1;
	int	interval = a2 ? strtol((char *) a2, NULL, 0) : 0;
	int	count = a3 ? strtol((char *) a3, NULL, 0) : 0;
	int	verbose = a4 ? 1 : 0;

	if (interval > 1)
	{
		verbose = a4;
	}
#else
int	main(int argc, char **argv)
{
	char	*sdrName;
	int	interval;
	int	count;
	int	verbose = 0;

	if (argc < 4)
	{
		PUTS("Usage: sdrwatch <sdr name> <interval> <count> [verbose]");
		return 0;
	}

	sdrName = argv[1];
	interval = strtol(argv[2], NULL, 0);
	if (interval < 0)
	{
		interval = 0;
	}

	count = strtol(argv[3], NULL, 0);
	if (count < 1)
	{
		count = 1;
	}

	if (argc > 4)
	{
		verbose = 1;
	}
#endif
	oK(sdrwatch_count(&count));
	return run_sdrwatch(sdrName, interval, verbose);
}
