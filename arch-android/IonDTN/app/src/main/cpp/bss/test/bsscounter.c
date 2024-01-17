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
	int		dbRecordsCount = 0;
	bssNav		nav;
	time_t		bundleIdTime;
	unsigned long	bundleIdCount;
	long		bytesRead;
	unsigned int	prevDataValue = 0;
	unsigned int	dataValue;

	memset((char *) &nav, 0, sizeof nav);
	if (bssSeek(&nav, 0, &bundleIdTime, &bundleIdCount) < 0)
	{
		putErrmsg("Failed in bssSeek.", NULL);
		return -1;
	}

	while (1)
	{
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

		dbRecordsCount++;
		if (dbRecordsCount == limit)
		{
			return limit;
		}

		switch (bssNext(&nav, &bundleIdTime, &bundleIdCount))
		{
		case -2:		/*	End of database.	*/
			break;		/*	Out of switch.		*/

		case -1:
			putErrmsg("Failed in bssNext.", NULL);
			return -1;

		default:
			continue;
		}

		break;			/*	Out of loop.		*/
	}

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
		switch (checkReceptionStatus(buffer + RCV_LENGTH, limit))
		{
		case -1:
			puts("bss test failed.");
			result = 1;		/*	Failed.		*/
			break;

		case 0:
			continue;		/*	Not done yet.	*/

		default:
			fprintf(stderr, "Received %d real-time frames.\n",
					_count(0));
			fprintf(stderr, "Received %d frames in total.\n",
					limit);
			puts("bss test succeeded.");
			result = 0;		/*	Succeeded.	*/
			break;			/*	Out of switch.	*/
		}

		break;				/*	Out of loop.	*/
	}

	oK(bssExit());
	free(buffer);
	return result;
}
