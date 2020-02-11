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
#include "memmgr.h"

static unsigned int	psmwatch_count(int *newValue)
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
	oK(psmwatch_count(&newCount));
}

static int	run_psmwatch(int memKey, long memSize, char *partitionName,
				int interval, int verbose)
{
	char		*memory = NULL;
	uaddr		smId = 0;
	PsmView		memView;
	PsmPartition	psm = &memView;
	int		memmgrIdx;
	PsmUsageSummary	psmsummary;
	int		secRemaining;
	int		decrement = 0;

	if (sm_ipc_init() < 0)
	{
		writeErrmsgMemos();
		PUTS("IPC initialization failed.");
		return 0;
	}

	if (memmgr_open(memKey, memSize, &memory, &smId, partitionName, &psm,
				&memmgrIdx, NULL, NULL, NULL, NULL) < 0)
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

	if (psm_start_trace(psm, 20000000, NULL) < 0)
	{
		putErrmsg("Can't start trace.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	isignal(SIGTERM, handleQuit);
	while (psmwatch_count(NULL) > 0)
	{
		secRemaining = interval;
		while (secRemaining > 0)
		{
			snooze(1);
			secRemaining--;
			if (!verbose)
			{
	       			psm_clear_trace(psm);
			}
		}

		psm_print_trace(psm, verbose);
		oK(psmwatch_count(&decrement));
	}

	psm_stop_trace(psm);
	writeErrmsgMemos();
	return 0;
}

#if defined (ION_LWT)
int	psmwatch(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	memKey = a1 ? strtol((char *) a1, NULL, 0) : 0;
	int	memSize = a2 ? strtol((char *) a2, NULL, 0) : 0;
	char	*partitionName = (char *) a3;
	int	interval = a4 ? strtol((char *) a4, NULL, 0) : 0;
	int	count = a5 ? strtol((char *) a5, NULL, 0) : 0;
	int	verbose = a6 ? 1 : 0;
#else
int	main(int argc, char **argv)
{
	int	memKey;
	long	memSize;
	char	*partitionName;
	int	interval;
	int	count;
	int	verbose = 0;

	if (argc < 6)
	{
		PUTS("Usage: psmwatch <shared memory key> <memory size> \
<partition name> <interval> <count> [verbose]");
		return 0;
	}

	memKey = strtol(argv[1], NULL, 0);
	memSize = strtol(argv[2], NULL, 0);
	partitionName = argv[3];
	interval = strtol(argv[4], NULL, 0);
	if (interval < 0)
	{
		interval = 0;
	}

	count = strtol(argv[5], NULL, 0);
	if (count < 1)
	{
		count = 1;
	}

	if (argc > 6)
	{
		verbose = 1;
	}
#endif
	oK(psmwatch_count(&count));
	return run_psmwatch(memKey, memSize, partitionName, interval, verbose);
}
