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

static unsigned int	psmwatch_count(int *newValue)
{
	unsigned int	count = 1;

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

	puts("[Terminated by user.]");
	oK(psmwatch_count(&newCount));
}

static int	run_psmwatch(char *partitionName, int interval, int verbose)
{
	PsmPartition	psm;
	PsmMgtOutcome	outcome;
	PsmUsageSummary	psmsummary;
	int		decrement = 0;

	if (sm_ipc_init() < 0)
	{
		writeErrmsgMemos();
		puts("IPC initialization failed.");
		return 0;
	}

	if (psm_manage(NULL, 0, partitionName, &psm, &outcome) < 0
	|| outcome == Refused)
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

	isignal(SIGTERM, handleQuit);
	while (psmwatch_count(NULL) > 0)
	{
		snooze(interval);
		psm_print_trace(psm, verbose);
	       	psm_clear_trace(psm);
		oK(psmwatch_count(&decrement));
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
	int	interval = a2 >= 0 ? a2 : 0;
	int	count = a3 > 0 ? a3 : 1;
	int	verbose = a4 ? 1 : 0;
#else
int	main(int argc, char **argv)
{
	char	*partitionName;
	int	interval;
	int	count;
	int	verbose = 0;

	if (argc < 4)
	{
		puts("Usage: psmwatch <partition name> <interval> <count> \
[verbose]");
		return 0;
	}

	partitionName = argv[1];
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
	oK(psmwatch_count(&count));
	return run_psmwatch(partitionName, interval, verbose);
}
