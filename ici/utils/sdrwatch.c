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
#include "ion.h"
#include "zco.h"
#include "sdrxn.h"

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

	if (-1 == interval)
	{
		sdr_stats(sdr);
		interval = 0;
	}
	else if (-2 == interval)
	{
		sdr_stats(sdr);
		CHKERR(sdr_begin_xn(sdr));
		printf("\n");
		zco_status(sdr);
		sdr_exit_xn(sdr);
		interval = 0;
	}
	else if (-3 == interval)
	{
		sdr_reset_stats(sdr);
		interval = 0;
	}
	else
	{
		CHKERR(sdr_begin_xn(sdr));
		sdr_usage(sdr, &sdrsummary);
		sdr_report(&sdrsummary);
		sdr_exit_xn(sdr);
	}

	if (interval == 0)	/*	One-time poll.			*/
	{
		sdr_stop_using(sdr);
		return 0;
	}

	/*	Start watching trace.					*/

	if (sdr_start_trace(sdr, 20000000, NULL) < 0)
	{
		putErrmsg("Can't start trace.", NULL);
		sdr_stop_using(sdr);
		writeErrmsgMemos();
		return 0;
	}

	isignal(SIGINT, handleQuit);
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

	PUTS("Stopping bptrace.");
	sdr_stop_trace(sdr);
	sdr_stop_using(sdr);
	writeErrmsgMemos();
	return 0;
}

static void	printUsage()
{
	PUTS("Usage: sdrwatch <sdr name> [ -s | -r | -z | <interval> <count> \
[verbose] ]");
	PUTS("\t-s to print stats for current transaction");
	PUTS("\t-s to reset log length high-water mark and then print stats \
for current transaction");
	PUTS("\t-z to print stats for current transaction and print ZCO \
status after that transaction ends");
}

#if defined (ION_LWT)
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

	if (argc < 2)
	{
		printUsage();
	}

	sdrName = argv[1];

	if (!strcmp ( argv[2], "-s" ))
	{
		interval = -1;
		count = 0;
	}
	else if (!strcmp(argv[2], "-z"))
	{
		interval = -2;
		count = 0;
	}
	else if (!strcmp(argv[2], "-r"))
	{
		interval = -3;
		count = 0;
	}
	else
	{
		if (argc < 4)
		{
			printUsage();
			return 0;
		}

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
	}
#endif
	oK(sdrwatch_count(&count));
	return run_sdrwatch(sdrName, interval, verbose);
}
