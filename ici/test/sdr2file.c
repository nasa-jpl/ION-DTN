/*

	sdr2file.c:	a test consumer of SDR activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2sdr.h>

#if 0
#include "sdrP.h"	/*	TEMPORARY for sptrace test.		*/
#endif

static int	sdr2file_stopped(int *newState)
{
	static int	state = 0;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static void	handleQuit(int signum)
{
	int	stop = 1;

	oK(sdr2file_stopped(&stop));
}

static int	run_sdr2file(int configFlags)
{
	char		sdrName[256];
	Sdr		sdr;
	sm_SemId	semaphore;
	Object		cycleList;
	Object		cycleListElt;
	Object		cycleObj;
	Cycle		currentCycle;
	char		fileName[256];
	int		outputFile;
	Object		lineListElt;
	Object		lineObj;	/*	An SDR string.		*/
	char		line[SDRSTRING_BUFSZ];
#if 0
char		*region = NULL;
int		sdrwmId;
PsmPartition	sdrwm;
int		lineCount = 0;
int		clearCount = 0;
PsmUsageSummary	psmsummary;
SdrUsageSummary	sdrsummary;
#endif
	isprintf(sdrName, sizeof sdrName, "%s%d", TEST_SDR_NAME, configFlags);
	sdr_initialize(TEST_WM_SIZE, NULL, SM_NO_KEY, NULL);
	sdr_load_profile(sdrName, configFlags, TEST_HEAP_WORDS, SM_NO_KEY,
			0, SM_NO_KEY, TEST_PATH_NAME, NULL);
	sdr = sdr_start_using(sdrName);
	if (sdr == NULL)
	{
		PUTS("Can't use sdr.");
		return 0;
	}

#if 0
sdr_start_trace(sdr, 5000000, NULL);
sm_ShmAttach(SDR_SM_KEY, 0, &region, &sdrwmId);
psm_manage((u_char *) region, 0, SDR_SM_NAME, &sdrwm);
psm_start_trace(sdrwm, 5000000, NULL);
#endif
	semaphore = sm_SemCreate(TEST_SEM_KEY * configFlags, SM_SEM_FIFO);
	if (semaphore < 0)
	{
		PUTS("Can't create semaphore.");
		return 0;
	}

	/*	Get list of cycles, creating it if necessary.		*/

	cycleList = sdr_find(sdr, CYCLE_LIST_NAME, 0);
	if (cycleList == 0)
	{
		CHKZERO(sdr_begin_xn(sdr));
		cycleList = sdr_list_create(sdr);
		sdr_catlg(sdr, CYCLE_LIST_NAME, 0, cycleList);
		if (sdr_end_xn(sdr))
		{
			PUTS("SDR transaction failed.");
			return 0;
		}
	}

	/*	Establish position at oldest cycle.  If none, wait
	 *	for one to be created.					*/

	while (sdr_list_length(sdr, cycleList) == 0)
	{
		sm_SemTake(semaphore);
	}

	cycleListElt = sdr_list_first(sdr, cycleList);
	cycleObj = sdr_list_data(sdr, cycleListElt);
	sdr_read(sdr, (char *) &currentCycle, cycleObj, sizeof(Cycle));
	PUTMEMO("Working on cycle", utoa(currentCycle.cycleNbr));
	isprintf(fileName, sizeof fileName, "file_copy_%d",
			currentCycle.cycleNbr);
	outputFile = iopen(fileName, O_WRONLY | O_APPEND, 0666);
	if (outputFile < 0)
	{
		PERROR("Can't open output file");
		return 0;
	}

	/*	Copy text lines from SDR to file.			*/

	isignal(SIGINT, handleQuit);
#if 0
psm_print_trace(sdrwm, 0);
psm_clear_trace(sdrwm);
sdr_print_trace(sdr, 0);
sdr_clear_trace(sdr);
#endif
	while (1)
	{
		if (sdr2file_stopped(NULL))
		{
			break;
		}

		while (sdr_list_length(sdr, currentCycle.lines) == 0)
		{
			if (sm_SemTake(semaphore) < 0)
			{
				break;
			}
		}

		lineListElt = sdr_list_first(sdr, currentCycle.lines);
		if (lineListElt == 0)
		{
			break;
		}

		lineObj = sdr_list_data(sdr, lineListElt);
		sdr_string_read(sdr, line, lineObj);

		/*	Delete line from SDR.				*/

		if (!sdr_begin_xn(sdr))
		{
			close(outputFile);
			return 0;
		}

		sdr_free(sdr, lineObj);
		sdr_list_delete(sdr, lineListElt, (SdrListDeleteFn) NULL, NULL);
		if (sdr_end_xn(sdr))
		{
			close(outputFile);
			PUTS("SDR transaction failed.");
			return 0;
		}

		/*	Process text of line.				*/

#if 0
lineCount++;
if (lineCount > 100)
{
	lineCount = 1;
/*
	psm_usage(sdrwm, &psmsummary);
	psm_report(&psmsummary);
	sdr_usage(sdr, &sdrsummary);
	sdr_report(&sdrsummary);
*/
	clearCount++;
	if (clearCount > 50)
	{
		clearCount = 1;
		psm_print_trace(sdrwm, 0);
		sdr_print_trace(sdr, 0);
	}

	psm_clear_trace(sdrwm);
	sdr_clear_trace(sdr);
}
#endif
		if (strcmp(line, EOF_LINE_TEXT) == 0)
		{
			/*	Delete cycle from SDR, close file.	*/

			close(outputFile);
			outputFile = -1;
			CHKZERO(sdr_begin_xn(sdr));
			sdr_list_destroy(sdr, currentCycle.lines,
					(SdrListDeleteFn) NULL, NULL);
			sdr_free(sdr, cycleObj);
			sdr_list_delete(sdr, cycleListElt,
					(SdrListDeleteFn) NULL, NULL);
			if (sdr_end_xn(sdr))
			{
				PUTS("SDR transaction failed.");
				return 0;
			}

			/*	Get next cycle, open new output file.	*/

			while (sdr_list_length(sdr, cycleList) == 0)
			{
				if (sm_SemTake(semaphore) < 0)
				{
					break;
				}
			}

			cycleListElt = sdr_list_first(sdr, cycleList);
			if (cycleListElt == 0)
			{
				break;
			}

			cycleObj = sdr_list_data(sdr, cycleListElt);
			sdr_read(sdr, (char *) &currentCycle, cycleObj,
					sizeof(Cycle));
			PUTMEMO("Working on cycle",
					utoa(currentCycle.cycleNbr));
			isprintf(fileName, sizeof fileName, "file_copy_%d",
					currentCycle.cycleNbr);
			outputFile = iopen(fileName, O_WRONLY | O_APPEND, 0666);
			if (outputFile < 0)
			{
				PERROR("Can't open output file");
				return 0;
			}
		}
		else	/*	Just write line to output file.		*/
		{
			if (iputs(outputFile, line) < 0)
			{
				close(outputFile);
				PERROR("Can't write to output file");
				return 0;
			}
		}
	}

	if (outputFile != -1)
	{
		close(outputFile);
	}
#if 0
psm_stop_trace(sdrwm);
sdr_stop_trace(sdr);
#endif
	sdr_shutdown();
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	sdr2file(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	configFlags = a1;
#else
int	main(int argc, char **argv)
{
	int	configFlags;

	if (argc < 2)
	{
		PUTS("Usage:  sdr2file <config flags>");
		return 0;
	}

	configFlags = atoi(argv[1]);
#endif
	return run_sdr2file(configFlags);
}
