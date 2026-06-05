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
	static time_t prevSec = 0;
	static unsigned int	prevCount = 0;

	/* Parameter intentionally unused. */
	(void)buf;

	if (bufLength < sizeof(unsigned int))
	{
		writeMemoNote("[?] Received test payload is too small",
				itoa(bufLength));
		return -1;
	}

	/*  Debugging only: we check realtime bundles are in-order.
	*  BSS callback for realtime bundles already perform this
	*  check.
	*  Such check can be turned-off to reduce overhead.     */
	if (sec < prevSec)
	{
		writeMemoNote("[?] bsscounter: Real-time stream out of order, prev bundle time second",
				itoa((unsigned int) prevSec));
		writeMemoNote("[?] bsscounter: Real-time stream out of order, prev bundle seq. number",
				itoa((unsigned int) prevCount));
		writeMemoNote("[?] bsscounter: Real-time stream out of order, received bundle time seccond",
				itoa((unsigned int) sec));
		writeMemoNote("[?] bsscounter: Real-time stream out of order, received bundle seq. number",
				itoa((unsigned int) count));
		return -1;
	}
	else
	{
		if ((sec == prevSec) && ((unsigned int) count < prevCount))
		{
			writeMemoNote("[?] bsscounter: Real-time stream out of order, prev bundle time second",
					itoa((unsigned int) prevSec));
			writeMemoNote("[?] bsscounter: Real-time stream out of order, prev bundle seq. number",
					itoa((unsigned int) prevCount));
			writeMemoNote("[?] bsscounter: Real-time stream out of order, received bundle time seccond",
					itoa((unsigned int) sec));
			writeMemoNote("[?] bsscounter: Real-time stream out of order, received bundle seq. number",
					itoa((unsigned int) count));
			return -1;
		}
	}

	/* received bundle is in-order */
	prevSec = sec;
	prevCount = (unsigned int) count;
	oK(_count(1));
	printf("Real-time in-order bundle count: %d\n",_count(0));
	return 0;
}

static int	checkReceptionStatus(char *buffer, int limit, long playback_wait)
{
	int		dbRecordsCount = 0;
	bssNav		nav;
	static time_t		bundleSec = 0;
	unsigned long	bundleCount = 0;
	long		bytesRead;

	/* This function performs a database read.
	* NOTE: it reads "forward" and will not
	* pick up any delayed bundled. Therefore
	* to make sure *all* bundles are read,
	* the "playback_wait" should be set
	* sufficiently long. */

	/* reset nav data structure */
	memset((char *) &nav, 0, sizeof nav);

	/* Wait for initial data arrival, we
	* assume database is empty, otherwise
	* we will pick up previous data stream. */
	if (bundleSec == 0)
	{
		puts("Waiting for stream...");
		fflush(stdout);
		if (bssSeek(&nav, 0, &bundleSec, &bundleCount) < 0)
		{
			/* not started yet */
			snooze(1);
			return 0;
		}
		else
		{
			puts("Bss data found...");
			fflush(stdout);
			snooze(playback_wait);
		}
	}

	/* Now we reset nav in case the first arrival is
	* not the actual first bundle in the stream */
	if (bssSeek(&nav, 0, &bundleSec, &bundleCount) < 0)
	{
		writeMemo("[?] BSS database is missing data...");
		return -1;
	}

	printf("Recheck Database reception total count....");
	fflush(stdout);

	while (1)
	{
		bytesRead = bssRead(nav, buffer, RCV_LENGTH);

		if (bytesRead < 0)
		{
			putErrmsg("Failed in bssRead.", NULL);
			return -1;
		}

		if (bytesRead < 0 || (size_t)bytesRead < sizeof(unsigned int))
		{
			writeMemoNote("[?] Received test payload is too small",
					itoa(bytesRead));
			return -1;
		}

		/* increment the database read counter */
		dbRecordsCount++;

		if ((dbRecordsCount % 10) == 0)
		{
			printf("Playback Database: Received %d bundles...\n", dbRecordsCount);
			fflush(stdout);
		}

		if (dbRecordsCount == limit)
		{
			return limit;
		}

		switch (bssNext(&nav, &bundleSec, &bundleCount))
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
int     bsscounter(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int     limit = 0;
	char    *bssName = (char *) a2;
	char    *path = (char *) a3;
	char    *eid = (char *) a4;
	int     cmdFile = fileno(stdin);
	char    *buffer;
	int     result;
	long    playback_wait = 5;
#else
int     main(int argc, char **argv)
{
	int     limit = 0;
	char    *bssName = NULL;
	char    *path = NULL;
	char    *eid = NULL;
	char    *buffer;
	int     result;
	long    playback_wait = 5;
#endif
	int     recv_count = 0;
	int     parsed_int;
	char    errMsg[256];

#if defined (ION_LWT)
	/* Tell compiler we aren't using cmdFile (legacy dead code) */
	(void) cmdFile;

	if (a1)
	{
		if (platform_parse_int((char *) a1, &parsed_int) < 0 || parsed_int < 1)
		{
			PUTS("[?] Invalid limit. Must be >= 1.");
			return 1;
		}
		limit = parsed_int;
	}

	if (a5)
	{
		if (platform_parse_int((char *) a5, &parsed_int) < 0 || parsed_int < 0)
		{
			PUTS("[?] Invalid playback delay. Must be >= 0.");
			return 1;
		}
		playback_wait = (long) parsed_int;
	}
#else
	if (argc > 6) argc = 6;
	switch (argc)
	{
	case 6:
		if (platform_parse_int(argv[5], &parsed_int) < 0 || parsed_int < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid playback delay. Must be >= 0: %s", argv[5]);
			PUTS(errMsg);
			return 1;
		}
		playback_wait = (long) parsed_int;
		/* FALLTHROUGH */

	case 5:
		eid = argv[4];
		/* FALLTHROUGH */

	case 4:
		path = argv[3];
		/* FALLTHROUGH */

	case 3:
		bssName = argv[2];
		/* FALLTHROUGH */

	case 2:
		if (platform_parse_int(argv[1], &parsed_int) < 0 || parsed_int < 1)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid limit. Must be >= 1: %s", argv[1]);
			PUTS(errMsg);
			return 1;
		}
		limit = parsed_int;
		/* FALLTHROUGH */

	default:
		break;
	}
#endif

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
		PUTS("Usage: bsscounter <number of bundles to receive> <BSS \
database name> <path for BSS database files> <own endpoint ID> \
<optional: playback delay in seconds> - adding playback delay allows \
for complete accounting of out-or-order delivery, default 5 seconds.");
		fflush(stdout);
		return 1;
	}

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
			result = 1;		/* Failed.         */
			break;

		case 0:
			puts("reception no complete yet...");
			fflush(stdout);
			continue;		/* Not done yet.   */

		default:
			/* in-order real-time reception reported by display */
			fprintf(stderr, "Real-time 'display' callback picked up %d in-order real-time frames.\n", _count(0));

			fprintf(stderr, "Bsscounter terminated after receiving total of %d frames.\n",
					recv_count);
			puts("bss test succeeded.");
			fflush(stdout);
			result = 0;		/* Succeeded.      */
			break;			/* Out of switch.  */
		}

		break;				/* Out of loop.    */
	}

	oK(bssExit());
	free(buffer);
	return result;
}
