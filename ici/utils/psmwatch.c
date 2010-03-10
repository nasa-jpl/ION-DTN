/*

	psmwatch.c:	PSM memory activity monitor.

									*/
/*									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "platform.h"
#include "psm.h"

static int	psmwatch_count = 0;

static void	handleQuit()
{
	puts("[Terminated by user.]");
	psmwatch_count = 1;	/*	Advance to end of last cycle.	*/
}

static int	run_psmwatch(char *partitionName, int interval, int verbose)
{
	PsmPartition	psm;
	PsmUsageSummary	psmsummary;

	if (interval > 0)
	{
		if (psmwatch_count < 0)
		{
			puts("count must be >= 0 cycles");
			return 0;
		}
	}

	if (sm_ipc_init() < 0)
	{
		writeErrmsgMemos();
		puts("IPC initialization failed.");
		return 0;
	}

	if (psm_manage(NULL, 0, partitionName, &psm) == Refused)
	{
		putErrmsg("Can't attach to psm.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	/*	Initial state.						*/

	psm_usage(psm, &psmsummary);
	psm_report(&psmsummary);
	if (interval == 0)	/*	One-time poll.			*/
	{
		return 0;
	}

	/*	Start watching trace.					*/

	if (psm_start_trace(psm, 5000000, NULL) < 0)
	{
		putErrmsg("Can't start trace.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	signal(SIGTERM, handleQuit);
	while (psmwatch_count > 0)
	{
		snooze(interval);
		psm_print_trace(psm, verbose);
	       	psm_clear_trace(psm);
		psmwatch_count--;
	}

	psm_stop_trace(psm);
	psm_unmanage(psm);
	writeErrmsgMemos();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	psmwatch(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*partitionName = (char *) a1;
	int	interval = a2;
	int	verbose = 0;

	psmwatch_count = a3;
	if (interval > 1)
	{
		verbose = a4;
	}
#else
int	main(int argc, char **argv)
{
	char	*partitionName;
	int	interval;
	int	verbose = 0;

	if (argc < 4)
	{
		puts("Usage: psmwatch <partition name> <interval> <count> \
[verbose]");
		return 0;
	}

	partitionName = argv[1];
	interval = atoi(argv[2]);
	if (interval < 0)
	{
		puts("interval must be >= 0 seconds");
		return 0;
	}

	psmwatch_count = atoi(argv[3]);
	if (interval > 0)
	{
		if (argc > 4)
		{
			verbose = 1;
		}
	}
#endif
	return run_psmwatch(partitionName, interval, verbose);
}
