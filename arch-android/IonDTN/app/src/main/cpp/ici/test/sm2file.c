/*

	sm2file.c:	a test consumer of shared-memory linked
			list activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <platform.h>
#include <smlist.h>

#if defined (ION_LWT)
int	sm2file(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
#else
int	main(int argc, char **argv)
#endif
{
	char		*wmspace;
	uaddr		wmid;
	PsmPartition	wm = NULL;
	PsmMgtOutcome	outcome;
	PsmAddress	testlist;
	sm_SemId	semaphore;
	int		cycleNbr = 1;
	char		fileName[256];
	int		outputFile;
	PsmAddress	lineListElt;
	PsmAddress	lineAddress;
	char		*line;

	if (sm_ipc_init() < 0)
	{
		return 0;
	}

	if (sm_ShmAttach(0x1108, 10000000, &wmspace, &wmid) < 0)
	{
		PERROR("can't attach to shared memory");
		return 0;
	}
	
	if (psm_manage(wmspace, 10000000, "file2sm", &wm, &outcome) < 0
	|| outcome == Refused)
	{
		PUTS("can't manage shared memory");
		return 0;
	}
	
	testlist = psm_get_root(wm);
	if (testlist == 0)
	{
		testlist = sm_list_create(wm);
		if (testlist == 0)
		{
			PUTS("can't create shared memory list");
			return 0;
		}
		
		psm_set_root(wm, testlist);
	}

	semaphore = sm_SemCreate(0x1101, SM_SEM_FIFO);
	if (semaphore < 0)
	{
		PUTS("can't create semaphore");
		return 0;
	}

	PUTMEMO("Working on cycle", utoa(cycleNbr));
	isprintf(fileName, sizeof fileName, "file_copy_%d", cycleNbr);
	outputFile = iopen(fileName, O_WRONLY | O_APPEND, 0666);
	if (outputFile < 0)
	{
		PERROR("can't open output file");
		return 0;
	}

	while (1)
	{
		while (sm_list_length(wm, testlist) == 0)
		{
			sm_SemTake(semaphore);	/*	Wait for line.	*/
		}

		lineListElt = sm_list_first(wm, testlist);
		lineAddress = sm_list_data(wm, lineListElt);
		line = psp(wm, lineAddress);

		/*	Process text of line.				*/

		if (strcmp(line, "*** End of the file ***\n") == 0)
		{
			/*	Close file, open next one.		*/

			close(outputFile);
			cycleNbr++;
			PUTMEMO("Working on cycle", utoa(cycleNbr));
			isprintf(fileName, sizeof fileName, "file_copy_%d",
					cycleNbr);
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

		/*	Delete line from shared memory list.		*/

		psm_free(wm, lineAddress);
		CHKZERO(sm_list_delete(wm, lineListElt, (SmListDeleteFn) NULL,
				NULL) == 0);
	}
}
