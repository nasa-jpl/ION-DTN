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

static void	handleQuit(int signum)
{
	int	newCount = 1;	/*	Advance to end of last cycle.	*/

	PUTS("[Terminated by user.]");
	oK(sdrwatch_count(&newCount));
}

static int	run_sdrwatch(char *sdrName, char *mode, int interval,
			int verbose)
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
	switch (*mode)
	{
	case 's':
		sdr_stats(sdr);
		break;

	case 'r':
		sdr_reset_stats(sdr);
		break;

	case 'z':
		sdr_stats(sdr);
		printf("\n");
		zco_status(sdr);
		break;

	case 't':
		sdr_usage(sdr, &sdrsummary);
		sdr_report(&sdrsummary);
		break;

	default:
		putErrmsg("Invalid sdrwatch mode.", mode);
		interval = 0;	/*	Force immediate return.		*/
	}

	sdr_exit_xn(sdr);
	if (interval == 0)	/*	One-time poll.			*/
	{
		sdr_stop_using(sdr);
		return 0;
	}

	/*	Start watching trace.					*/

	if (*mode == 't')
	{
		if (sdr_start_trace(sdr, 20000000, NULL) < 0)
		{
			putErrmsg("Can't start trace.", NULL);
			sdr_stop_using(sdr);
			writeErrmsgMemos();
			return 0;
		}
	}

	isignal(SIGINT, handleQuit);
	while (sdrwatch_count(NULL) > 0)
	{
		secRemaining = interval;
		while (secRemaining > 0)
		{
			snooze(1);
			secRemaining--;
		}

		CHKERR(sdr_begin_xn(sdr));
		switch (*mode)
		{
		case 's':
			sdr_stats(sdr);
			break;

		case 'r':
			sdr_reset_stats(sdr);
			break;

		case 'z':
			sdr_stats(sdr);
			printf("\n");
			zco_status(sdr);
			break;

		default:
			if (!verbose)
			{
				sdr_clear_trace(sdr);
			}

			sdr_print_trace(sdr, verbose);
		}

		sdr_exit_xn(sdr);
		oK(sdrwatch_count(&decrement));
	}

	PUTS("Stopping sdrwatch.");
	if (*mode == 't')
	{
		sdr_stop_trace(sdr);
	}

	sdr_stop_using(sdr);
	writeErrmsgMemos();
	return 0;
}

static void	printUsage()
{
	PUTS("Usage: sdrwatch <sdr name> [ -t | -s | -r | -z ] [<interval> \
[<count> [verbose]]]");
	PUTS("\t-t to print a trace of space allocation and release (default)");
	PUTS("\t-s to print stats for current transaction");
	PUTS("\t-r to reset log length high-water mark and then print stats \
for current transaction");
	PUTS("\t-z to print stats for current transaction and print ZCO \
status after that transaction ends");
}

#if defined (ION_LWT)
int	sdrwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*sdrName = (char *) a1;
	char	*modeToken = (char *) a2;
	int	interval = 0;
	int	count = 0;
	int	verbose = 0;
	char	*mode = "t";

	if (*modeToken == '-')
	{
		mode = modeToken + 1;
		interval = strtol((char *) a3, NULL, 0);
		count = strtol((char *) a4, NULL, 0);
		verbose = a5;
	}
	else
	{
		interval = strtol((char *) a2, NULL, 0);
		count = strtol((char *) a3, NULL, 0);
		verbose = a4;
	}
#else
int	main(int argc, char **argv)
{
	char	*sdrName;
	int	interval = 0;
	int	count = 0;
	int	verbose = 0;
	char	*mode = "t";

	if (argc < 2)
	{
		printUsage();
		return 0;
	}

	sdrName = argv[1];
	if (argc > 2)
	{
		if (*(argv[2]) == '-')
		{
			mode = argv[2] + 1;
			if (argc > 3)
			{
				interval = strtol(argv[3], NULL, 0);
				if (argc > 4)
				{
					count = strtol(argv[4], NULL, 0);
					if (argc > 5)
					{
						verbose = 1;
					}
				}
			}
		}
		else
		{
			interval = strtol(argv[2], NULL, 0);
			if (argc > 3)
			{
				count = strtol(argv[3], NULL, 0);
				if (argc > 4)
				{
					verbose = 1;
				}
			}
		}
	}
#endif
	if (interval < 0)
	{
		interval = 0;
	}

	if (count < 1)
	{
		count = 1;
	}

	oK(sdrwatch_count(&count));
	return run_sdrwatch(sdrName, mode, interval, verbose);
}
