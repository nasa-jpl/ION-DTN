/*

	file2sm.c:	a test producer of shared-memory linked
			list activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <platform.h>
#include <smlist.h>

static int	run_file2sm(char *fileName)
{
	char		*wmspace;
	uaddr		wmid;
	PsmPartition	wm = NULL;
	PsmMgtOutcome	outcome;
	PsmAddress	testlist;
	sm_SemId	semaphore;
	int		inputFile;
	unsigned long	startLineNbr;
	char		line[256];
	char		*eofLine = "*** End of the file ***\n";
	int		lineLen;
	PsmAddress	lineAddress;
	struct timeval	startTime;
	unsigned long	endLineNbr;
	struct timeval	endTime;
	unsigned long	linesProcessed;
	unsigned long	usec;
	int		rate;

	if (sm_ipc_init() < 0)
	{
		return 0;
	}

	if (sm_ShmAttach(0x1108, 10000000, &wmspace, &wmid) < 0)
	{
		PERROR("Can't attach to shared memory");
		return 0;
	}

	if (psm_manage(wmspace, 10000000, "file2sm", &wm, &outcome) < 0
	|| outcome == Refused)
	{
		PUTS("Can't manage shared memory.");
		return 0;
	}

	testlist = psm_get_root(wm);
	if (testlist == 0)
	{
		testlist = sm_list_create(wm);
		if (testlist == 0)
		{
			PUTS("Can't create shared memory list.");
			return 0;
		}

		psm_set_root(wm, testlist);
	}

	semaphore = sm_SemCreate(0x1101, SM_SEM_FIFO);
	if (semaphore < 0)
	{
		PUTS("Can't create semaphore.");
		return 0;
	}

	/*	Establish position at first record of file.		*/

	inputFile = iopen(fileName, O_RDONLY, 0777);
	if (inputFile < 0)
	{
		PERROR("Can't open input file");
		return 0;
	}

	getCurrentTime(&startTime);
	endLineNbr = startLineNbr = 0;
	while (1)
	{
		if (igets(inputFile, line, sizeof line, &lineLen) == NULL)
		{
			if (lineLen == 0)
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
				lineLen = strlen(eofLine) + 1;
				lineAddress = psm_zalloc(wm, lineLen);
				if (lineAddress == 0)
				{
					PUTS("Ran out of memory.");
					return 0;
				}

				istrcpy((char *) psp(wm, lineAddress), eofLine,
						lineLen);
				if (sm_list_insert_last(wm, testlist,
						lineAddress) == 0)
				{
					PUTS("Ran out of memory.");
					return 0;
				}

				sm_SemGive(semaphore);
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

		/*	Append line to shared memory list.		*/

		lineLen = strlen(line) + 1;
		lineAddress = psm_zalloc(wm, lineLen);
		if (lineAddress == 0)
		{
			close(inputFile);
			PUTS("Ran out of memory.");
			return 0;
		}

		memcpy((char *) psp(wm, lineAddress), line, lineLen);
		if (sm_list_insert_last(wm, testlist, lineAddress) == 0)
		{
			close(inputFile);
			PUTS("Ran out of memory.");
			return 0;
		}

		sm_SemGive(semaphore);
		endLineNbr++;
	}
}

#if defined (ION_LWT)
int	file2sm(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*fileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*fileName;

	if (argc < 2)
	{
		PUTS("Usage:  file2sm <name of file to copy>");
		return 0;
	}

	fileName = argv[1];
#endif
	return run_file2sm(fileName);
}
