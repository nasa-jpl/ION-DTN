/*

	file2sdr.c:	a test producer of SDR activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2sdr.h>

static int	file2sdr_stopped(int *newState)
{
	static int	state = 0;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

static Object	newCycle(Sdr sdr, Object cycleList, Cycle *currentCycle,
			sm_SemId file2sdr_semaphore)
{
	Object	cycleObj;

	CHKZERO(sdr_begin_xn(sdr));
	currentCycle->cycleNbr++;
	currentCycle->lineCount = 0;
	currentCycle->lines = sdr_list_create(sdr);
	cycleObj = sdr_malloc(sdr, sizeof(Cycle));
	sdr_write(sdr, cycleObj, (char *) currentCycle, sizeof(Cycle));
	sdr_list_insert_last(sdr, cycleList, cycleObj);
	if (sdr_end_xn(sdr))
	{
		putErrmsg("SDR transaction failed", NULL);
		writeErrmsgMemos();
		return 0;
	}

	PUTMEMO("Appended cycle to list", utoa(currentCycle->cycleNbr));
	sm_SemGive(file2sdr_semaphore);
	return cycleObj;
}

static Object	endCycle(Sdr sdr, Object cycleList, Cycle *currentCycle,
			sm_SemId file2sdr_semaphore)
{
	CHKZERO(sdr_begin_xn(sdr));
	sdr_list_insert_last(sdr, currentCycle->lines,
			sdr_string_create(sdr, EOF_LINE_TEXT));
	if (sdr_end_xn(sdr))
	{
		putErrmsg("SDR transaction failed", NULL);
		writeErrmsgMemos();
		return 0;
	}

	return newCycle(sdr, cycleList, currentCycle, file2sdr_semaphore);
}

static void	handleQuit(int signum)
{
	int	stopped = 1;

	oK(file2sdr_stopped(&stopped));
}

static int	run_file2sdr(int configFlags, char *fileName)
{
	char		sdrName[256];
	Sdr		sdr;
	sm_SemId	file2sdr_semaphore;
	Object		cycleList;
	Object		cycleListElt;
	Object		cycleObj;
	Cycle		currentCycle = { 0, 0, 0 };
	int		inputFile;
	unsigned long	startLineNbr;
	char		line[SDRSTRING_BUFSZ];
	int		len;
	struct timeval	startTime;
	unsigned long	endLineNbr;
	struct timeval	endTime;
	unsigned long	linesProcessed;
	unsigned long	usec;
	int		rate;

	isprintf(sdrName, sizeof sdrName, "%s%d", TEST_SDR_NAME, configFlags);
	sdr_initialize(TEST_WM_SIZE, NULL, SM_NO_KEY, NULL);
	sdr_load_profile(sdrName, configFlags, TEST_HEAP_WORDS, SM_NO_KEY,
			0, SM_NO_KEY, TEST_PATH_NAME, NULL);
	sdr = sdr_start_using(sdrName);
	if (sdr == NULL)
	{
		putErrmsg("Can't use sdr.", sdrName);
		writeErrmsgMemos();
		return 0;
	}

	file2sdr_semaphore = sm_SemCreate(TEST_SEM_KEY * configFlags,
			SM_SEM_FIFO);
	if (file2sdr_semaphore < 0)
	{
		putErrmsg("Can't create semaphore.", NULL);
		writeErrmsgMemos();
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
			putErrmsg("SDR transaction failed.", NULL);
			writeErrmsgMemos();
			return 0;
		}
	}

	/*	Create initial cycle if necessary.  In any case,
		establish position at latest cycle.			*/

	if (sdr_list_length(sdr, cycleList) == 0)
	{
		cycleObj = newCycle(sdr, cycleList, &currentCycle,
				file2sdr_semaphore);
		if (cycleObj == 0)
		{
			return 0;
		}
	}
	else
	{
		cycleListElt = sdr_list_last(sdr, cycleList);
		cycleObj = sdr_list_data(sdr, cycleListElt);
		sdr_read(sdr, (char *) &currentCycle, cycleObj, sizeof(Cycle));
	}

	/*	Establish position at first unread record of file.	*/

	inputFile = iopen(fileName, O_RDONLY, 0777);
	if (inputFile < 0)
	{
		PERROR("Can't open input file");
		return 0;
	}

	startLineNbr = 0;
	while (startLineNbr < currentCycle.lineCount)
	{
		if (igets(inputFile, line, sizeof line, &len) == NULL)
		{
			if (len == 0)
			{
				close(inputFile);
				cycleObj = endCycle(sdr, cycleList,
					&currentCycle, file2sdr_semaphore);
				if (cycleObj == 0)
				{
					return 0;
				}

				inputFile = iopen(fileName, O_RDONLY, 0777);
				if (inputFile < 0)
				{
					PERROR("Can't reopen input file");
					return 0;
				}
			}
			else
			{
				close(inputFile);
				PERROR("Can't read from input file");
				return 0;
			}
		}

		startLineNbr++;
	}

	/*	Copy text lines from file to SDR.			*/

	isignal(SIGINT, handleQuit);
	getCurrentTime(&startTime);
	endLineNbr = startLineNbr;
	while (1)
	{
#if 0
microsnooze(10000);
#endif
		if (file2sdr_stopped(NULL))
		{
			break;
		}

		if (igets(inputFile, line, sizeof line, &len) == NULL)
		{
			if (len == 0)
			{
				close(inputFile);
				getCurrentTime(&endTime);
				linesProcessed = endLineNbr - startLineNbr;
				if (endTime.tv_usec < startTime.tv_usec)
				{
					endTime.tv_sec--;
					endTime.tv_usec += 1000000;
				}

				usec = ((endTime.tv_sec - startTime.tv_sec)
						* 1000000) + (endTime.tv_usec -
						startTime.tv_usec);
				rate = (linesProcessed * 1000) / (usec / 1000);
				PUTMEMO("Lines per second processed",
						utoa(rate));
				cycleObj = endCycle(sdr, cycleList,
					&currentCycle, file2sdr_semaphore);
				if (cycleObj == 0)
				{
					return 0;
				}

				inputFile = iopen(fileName, O_RDONLY, 0777);
				if (inputFile < 0)
				{
					PERROR("Can't reopen input file");
					return 0;
				}

				endLineNbr = startLineNbr = 0;
				getCurrentTime(&startTime);
				continue;
			}
			else
			{
				close(inputFile);
				PERROR("Can't read from input file");
				return 0;
			}
		}

		/*	Append line to list in SDR.			*/

		CHKZERO(sdr_begin_xn(sdr));
		sdr_list_insert_last(sdr, currentCycle.lines,
				sdr_string_create(sdr, line));
		sdr_stage(sdr, (char *) &currentCycle, cycleObj, sizeof(Cycle));
		currentCycle.lineCount++;
		sdr_write(sdr, cycleObj, (char *) &currentCycle, sizeof(Cycle));
		if (sdr_end_xn(sdr))
		{
			close(inputFile);
			putErrmsg("SDR transaction failed", NULL);
			writeErrmsgMemos();
			return 0;
		}

		sm_SemGive(file2sdr_semaphore);
		endLineNbr++;
	}

	close(inputFile);
	sm_SemEnd(file2sdr_semaphore);
	microsnooze(50000);
	sm_SemDelete(file2sdr_semaphore);
	sdr_shutdown();
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	file2sdr(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int	configFlags = a1;
	char	*fileName = (char *) a2;
#else
int	main(int argc, char **argv)
{
	int	configFlags;
	char	*fileName;

	if (argc < 3)
	{
		PUTS("Usage:  file2sdr <config flags> <name of file to copy>");
		PUTS("\tNote -- file is expected to be lines of text.");
		return 0;
	}

	configFlags = atoi(argv[1]);
	fileName = argv[2];
#endif
	return run_file2sdr(configFlags, fileName);
}
