/*
 *	bsscounter.c:	A test application that demonstrates the 
 *			functionality of BSS API.  It receives and
 *			processes a stream of bundles.
 *
 *	Adapted from bsscounter.c, written by Sotiris-Angelos Lenas,
 *	Democritus University of Thrace.
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *
 *	All rights reserved.						
 *	
 *	Author: Scott Burleigh
 */

#include "bsstest.h"
//JG
//#include <time.h>

static int	_count(int increment)
{
	static int	count = 0;

	count += increment;
	return count;
}

/* Implementation of the callback function */

static int	display(time_t sec, unsigned long count, char *buf,
			unsigned long bufLength)
{
	static unsigned int	prevDataValue = 0;
	unsigned int		dataValue;

	if (bufLength < sizeof(unsigned int))
	{
		writeMemoNote("[?] Received test payload is too small",
				itoa(bufLength));
		return -1;
	}

	memcpy((char *) &dataValue, buf, sizeof(unsigned int));
	dataValue = ntohl(dataValue);
	if (dataValue < prevDataValue)
	{
		writeMemoNote("[?] Real-time stream out of order",
				itoa(dataValue));
		return -1;
	}

	prevDataValue = dataValue;
	oK(_count(1));
	return 0;
}

static int	checkReceptionStatus(char *buffer, int limit)
{
	static int		dbRecordsCount = 0;
	bssNav		nav;
	static time_t		bundleIdTime = 0;
	static unsigned long	bundleIdCount = 0;
	static unsigned long 	waitForBundleIdCount = 0;
	long		bytesRead;
	unsigned int	prevDataValue = 0;
	unsigned int	dataValue;

	/* reset nav data structure */
	memset((char *) &nav, 0, sizeof nav);

	// JG
	printf("...entered checkRecptionStatus... time = %ld sec.\n", time(NULL));
	printf("......record count = %d. \n", dbRecordsCount);
	printf("......wait for bundle Id Count = %ld. \n", waitForBundleIdCount);
	printf("......bundle Id Count = %ld. \n", bundleIdCount);
	printf("......bundle Id Time = %ld. \n", bundleIdTime);
	fflush(stdout);
	

	/* 	check for data arrival */
	if (bundleIdTime == 0)
	{
		if (bssSeek(&nav, 0, &bundleIdTime, &bundleIdCount) < 0)
		{
			puts("...waiting...");
			fflush(stdout);
			//putErrmsg("Failed in bssSeek.", NULL);
			//JG
			//puts("......failed bssSeek...");
			//fflush(stdout);

			/* not started yet */
			return 0;
		}
		else
		{
			puts("...data arrived...");
			fflush(stdout);
		}
	}
	else //JG this section tries to interrupted Reception
	{
		/* reset nav content by seeking to last know bundle time */
		if (bssSeek(&nav, bundleIdTime, &bundleIdTime, &bundleIdCount) < 0)
		{
			//JG
			puts("......failed bssSeek...");
			fflush(stdout);
			return -1;
		}	
	}

	//JG
	puts("...another round ....");
	printf("......resetted nav current position(%ld), prev(%ld), datOffset(%ld), next(%ld) \n", nav.curPosition, nav.prevOffset, nav.datOffset, nav.nextOffset);
	printf("......current bundle time... %ld \n", bundleIdTime);
	fflush(stdout);

	while (1)
	{
		/* JG
		puts("....running bssRead...");
		fflush(stdout);
		*/

		bytesRead = bssRead(nav, buffer, RCV_LENGTH);
		
		if (bytesRead < 0)
		{
			putErrmsg("Failed in bssRead.", NULL);
			return -1;
		}

		if (bytesRead < sizeof(unsigned int))
		{
			writeMemoNote("[?] Received test payload is too small",
					itoa(bytesRead));
			return -1;
		}

		memcpy((char *) &dataValue, buffer, sizeof(unsigned int));
		dataValue = ntohl(dataValue);
		if (dataValue < prevDataValue)
		{
			writeMemoNote("[?] BSS database out of order",
					itoa(dataValue));
			return -1;
		}

		/* check if new bssp bundle(s) were read */
		if (bundleIdCount >= waitForBundleIdCount)
		{
			waitForBundleIdCount = bundleIdCount + 1;
			dbRecordsCount++;
			if ((dbRecordsCount % 10) == 0)
			{
				printf("...received %d bundles.\n", dbRecordsCount);
				fflush(stdout);
			}


			//* JG
			puts("...new bundle was read......");
			printf("......nav current position(%ld), prev(%ld), datOffset(%ld), next(%ld) \n", nav.curPosition, nav.prevOffset, nav.datOffset, nav.nextOffset);
			printf("......record count = %d. \n", dbRecordsCount);
			printf("......wait for bundle Id Count = %ld. \n", waitForBundleIdCount);
			printf("......bundle Id Count = %ld. \n", bundleIdCount);
			printf("......bundle Id Time = %ld. \n", bundleIdTime);
			fflush(stdout);
			
		}

		if (dbRecordsCount >= limit)
		{
			/* JG
			puts("...exiting checkReceptionStatus (reach limit)...");
			printf("......record count = %d. \n", dbRecordsCount);
			fflush(stdout);
			*/
			return dbRecordsCount;
		}

		switch (bssNext(&nav, &bundleIdTime, &bundleIdCount))
		{
		case -2:		/*	End of database.	*/
			//JG
			puts("...exiting checkReceptionStatus (bssNext: end of database)");
			printf("......nav current position(%ld), prev(%ld), datOffset(%ld), next(%ld) \n", nav.curPosition, nav.prevOffset, nav.datOffset, nav.nextOffset);			printf("......record count = %d. \n", dbRecordsCount);
			printf("......wait for bundle Id Count = %ld. \n", waitForBundleIdCount);
			printf("......bundle Id Count = %ld. \n", bundleIdCount);
			printf("......bundle Id Time = %ld. \n", bundleIdTime);
			fflush(stdout);
			
			break;		/*	Out of switch.		*/

		case -1:
			/* JG
			puts("...exiting checkReceptionStatus (failed bssNext)");
			fflush(stdout);
			*/
			putErrmsg("Failed in bssNext.", NULL);
			return -1;

		default:
			continue;
		}

		break;			/*	Out of loop.		*/
	}

	/* JG
	puts("...exiting checkReceptionStatus (not done yet; return 0)");
	fflush(stdout);
	*/

	return 0;			/*	Not done yet.		*/
}

#if defined (ION_LWT)
int	bsscounter(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int 	limit = strtol((char *) a1, NULL, 0);
	char	*bssName = (char *) a2;
	char	*path = (char *) a3;
	char	*eid = (char *) a4;
	int	cmdFile = fileno(stdin);
	char	*buffer;
	int	result;
#else
int	main(int argc, char **argv)
{
	int 	limit=0;
	char	*bssName = NULL;
	char	*path = NULL;
	char	*eid = NULL;
	char	*buffer;
	int	result;
	int recv_count = 0;

	if (argc > 5) argc = 5;
	switch (argc)
	{
	case 5:
		eid = argv[4];

	case 4:
		path = argv[3];

	case 3:
		bssName = argv[2];

	case 2:
		limit = strtol(argv[1], NULL, 0);

	default:
		break;
	}

	if (bssName == NULL)
	{
		bssName = "bssDB";
	}

	if (path == NULL)
	{
		path = ".";
	}

	if (eid == NULL || limit < 1)
	{
		puts("Usage: bsscounter <number of bundles to receive> <BSS \
database name> <path for BSS database files> <own endpoint ID>");
		fflush(stdout);
		return 1;
	}
#endif
	/*
         * ********************************************************
	 * In order for the BSS receiving thread to work properly,
	 * the receiving application must always allocate a buffer
	 * of a certain size and provide its address and its length 
	 * to bssRun or bssStart function.
         * ********************************************************
	 */ 

	buffer = calloc(2 * RCV_LENGTH, sizeof(char));
	if (buffer == NULL)
	{
		putErrmsg("Memory allocation error in bsscounter.", NULL);
		return 1;
	}

	if (bssRun(bssName, path, eid, buffer, RCV_LENGTH, display) == -1)
	{
		putErrmsg("Failed in bssRun.", NULL);
		free(buffer);
		return 1;
	}

	while (1)
	{
		snooze(5);
		recv_count = checkReceptionStatus(buffer + RCV_LENGTH, limit);
		switch (recv_count)
		{
		case -1:
			puts("bss test failed.");
			fflush(stdout);
			result = 1;		/*	Failed.		*/
			break;

		case 0:
			continue;		/*	Not done yet.	*/

		default:
			/* in-order real-time reception reported by display */
			fprintf(stderr, "Received %d in-order, real-time frames.\n",
					_count(0));

			fprintf(stderr, "Received %d frames in total.\n",
					recv_count);
			puts("bss test succeeded.");
			fflush(stdout);
			result = 0;		/*	Succeeded.	*/
			break;			/*	Out of switch.	*/
		}

		break;				/*	Out of loop.	*/
	}

	oK(bssExit());
	free(buffer);
	return result;
}
