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
	static unsigned int	prevCount = 0;

	if (bufLength < sizeof(unsigned int))
	{
		writeMemoNote("[?] Received test payload is too small",
				itoa(bufLength));
		return -1;
	}

	if ((unsigned int) count < prevCount)
	{
		writeMemoNote("[?] bsscounter: Real-time stream out of order per bundle seq. number",
				itoa((unsigned int) count));
		return -1;
	}

	prevCount = (unsigned int) count;
	oK(_count(1));
	printf("Real-time in-order bundle count: %d\n",_count(0));
	return 0;
}

static int	checkReceptionStatus(char *buffer, int limit, long playback_wait)
{
	static int		dbRecordsCount = 0;
	bssNav		nav;
	static time_t		bundleIdTime = 0;
	static unsigned long	bundleIdCount = 0;
	static unsigned long 	waitForBundleIdCount = 0;
	long		bytesRead;

	/* reset nav data structure */
	memset((char *) &nav, 0, sizeof nav);

	/* 	check for data arrival */
	if (bundleIdTime == 0)
	{
		if (bssSeek(&nav, 0, &bundleIdTime, &bundleIdCount) < 0)
		{
			puts("Waiting...");
			fflush(stdout);

			/* not started yet */
			return 0;
		}
		else
		{
			puts("Bss data found...");
			fflush(stdout);
			snooze(playback_wait);
		}
	}
	else 
	{
		/* reset nav content by seeking to last known bundle time */
		if (bssSeek(&nav, bundleIdTime, &bundleIdTime, &bundleIdCount) < 0)
		{
			writeMemo("[?] bsscounter: Failed bssSeek");
			fflush(stdout);
			return -1;
		}	
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

		/* Check if new real-time bssp bundle(s) were read. 
		 * Note: bundle sequence number may leap due to multiplexing
		 * with non-bss traffic. 		*/

		/* TO DO: generalize order checking to accommodate 
		 * 		  implementation that may reset seq. number
		 *        on second boundary. Currently ION does 
		 *        not reset bundle seq. number. 		*/

		if (bundleIdCount >= waitForBundleIdCount)
		{
			waitForBundleIdCount = bundleIdCount + 1;
		}
		dbRecordsCount++;
		if ((dbRecordsCount % 10) == 0)
		{
			printf("Playback Databased: Received %d bundles...\n", dbRecordsCount);
			fflush(stdout);
		}

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
	long playback_wait = strtol((char *) a5, NULL, 0);
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
	long playback_wait = 0;

	if (argc > 6) argc = 6;
	switch (argc)
	{
	case 6:
		playback_wait = strtol(argv[5], NULL, 0);
	
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
database name> <path for BSS database files> <own endpoint ID> \
<optional: playback delay in seconds> - adding playback delay allows \
for complete accounting of out-or-order delivery.");
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
		recv_count = checkReceptionStatus(buffer + RCV_LENGTH, limit, playback_wait);
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
			fprintf(stderr, "Real-time 'display' callback picked up %d in-order real-time frames.\n", _count(0));

			fprintf(stderr, "Bsscounter terminated after receiving total of %d frames.\n",
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
