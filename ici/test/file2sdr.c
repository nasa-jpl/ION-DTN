/*

	file2sdr.c:	a test producer of SDR activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2sdr.h>

static sm_SemId	file2sdr_semaphore;
static int	file2sdr_stopped = 0;

static Object	newCycle(Sdr sdr, Object cycleList, Cycle *currentCycle)
{
	Object	cycleObj;

	sdr_begin_xn(sdr);
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

	printf("appended cycle %d to list.\n", currentCycle->cycleNbr);
	sm_SemGive(file2sdr_semaphore);
	return cycleObj;
}

static Object	endCycle(Sdr sdr, Object cycleList, Cycle *currentCycle)
{
	sdr_begin_xn(sdr);
	sdr_list_insert_last(sdr, currentCycle->lines,
			sdr_string_create(sdr, EOF_LINE_TEXT));
	if (sdr_end_xn(sdr))
	{
		putErrmsg("SDR transaction failed", NULL);
		writeErrmsgMemos();
		return 0;
	}

	return newCycle(sdr, cycleList, currentCycle);
}

static void	handleQuit()
{
	file2sdr_stopped = 1;
}

static int	run_file2sdr(int configFlags, char *fileName)
{
	char		sdrName[256];
	Sdr		sdr;
	Object		cycleList;
	Object		cycleListElt;
	Object		cycleObj;
	Cycle		currentCycle = { 0, 0, 0 };
	FILE		*inputFile;
	unsigned long	startLineNbr;
	char		line[SDRSTRING_BUFSZ];
	struct timeval	startTime;
	unsigned long	endLineNbr;
	struct timeval	endTime;
	unsigned long	linesProcessed;
	unsigned long	usec;
	int		rate;

	sprintf(sdrName, "%s%d", TEST_SDR_NAME, configFlags);
	sdr_initialize(TEST_WM_SIZE, NULL, SM_NO_KEY, NULL);
	sdr_load_profile(sdrName, configFlags, TEST_HEAP_WORDS, SM_NO_KEY,
			TEST_PATH_NAME);
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
		sdr_begin_xn(sdr);
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
		cycleObj = newCycle(sdr, cycleList, &currentCycle);
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

	inputFile = fopen(fileName, "r");
	if (inputFile == NULL)
	{
		putSysErrmsg("Can't open input file", NULL);
		return 0;
	}

	startLineNbr = 0;
	while (startLineNbr < currentCycle.lineCount)
	{
		if (fgets(line, SDRSTRING_BUFSZ, inputFile) == NULL)
		{
			if (feof(inputFile))
			{
				fclose(inputFile);
				cycleObj = endCycle(sdr, cycleList,
						&currentCycle);
				if (cycleObj == 0)
				{
					return 0;
				}

				inputFile = fopen(fileName, "r");
				if (inputFile == NULL)
				{
					putSysErrmsg("Can't reopen input file",
							NULL);
					return 0;
				}
			}
			else
			{
				fclose(inputFile);
				putSysErrmsg("Can't read from input file",
						NULL);
				return 0;
			}
		}

		startLineNbr++;
	}

	/*	Copy text lines from file to SDR.			*/

	signal(SIGINT, handleQuit);
	getCurrentTime(&startTime);
	endLineNbr = startLineNbr;
	while (1)
	{
#if 0
microsnooze(10000);
#endif
		if (file2sdr_stopped)
		{
			break;
		}

		if (fgets(line, SDRSTRING_BUFSZ, inputFile) == NULL)
		{
			if (feof(inputFile))
			{
				fclose(inputFile);
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
				printf("Processing %d lines per second.\n",
						rate);
				cycleObj = endCycle(sdr, cycleList,
						&currentCycle);
				if (cycleObj == 0)
				{
					return 0;
				}

				inputFile = fopen(fileName, "r");
				if (inputFile == NULL)
				{
					putSysErrmsg("Can't reopen input file",
							NULL);
					return 0;
				}

				endLineNbr = startLineNbr = 0;
				getCurrentTime(&startTime);
				continue;
			}
			else
			{
				fclose(inputFile);
				putSysErrmsg("Can't read from input file",
						NULL);
				return 0;
			}
		}

		/*	Append line to list in SDR.			*/

		sdr_begin_xn(sdr);
		sdr_list_insert_last(sdr, currentCycle.lines,
				sdr_string_create(sdr, line));
		sdr_stage(sdr, (char *) &currentCycle, cycleObj, sizeof(Cycle));
		currentCycle.lineCount++;
		sdr_write(sdr, cycleObj, (char *) &currentCycle, sizeof(Cycle));
		if (sdr_end_xn(sdr))
		{
			fclose(inputFile);
			putErrmsg("SDR transaction failed", NULL);
			writeErrmsgMemos();
			return 0;
		}

		sm_SemGive(file2sdr_semaphore);
		endLineNbr++;
	}

	fclose(inputFile);
	sm_SemDelete(file2sdr_semaphore);
	sdr_shutdown();
	ionDetach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	file2sdr(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
		puts("Usage:  file2sdr <config flags> <name of file to copy>");
		puts("\tNote -- file is expected to be lines of text.");
		return 0;
	}

	configFlags = atoi(argv[1]);
	fileName = argv[2];
#endif
	return run_file2sdr(configFlags, fileName);
}
