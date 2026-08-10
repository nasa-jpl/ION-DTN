/*
	ambenchr.c:	an AMS benchmark program for UNIX.  Subscribes
			to subject "bench", counts the number of such
			messages received and their agregate size, and
			computes and prints performance statistics.
			The first 4 bytes of each message contain the
			total number of messages that will be published,
			including this one.
								*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "ams.h"
#include <stdlib.h>
#include <string.h>

static int	compare_double(const void *a, const void *b)
{
	double	da = *(const double *) a;
	double	db = *(const double *) b;

	if (da < db) return -1;
	if (da > db) return 1;
	return 0;
}

static double	tvdiff_usec(struct timeval *start, struct timeval *end)
{
	return (double)(end->tv_sec - start->tv_sec) * 1000000.0
		+ (double)(end->tv_usec - start->tv_usec);
}

#if defined (ION_LWT)
int	amsbenchr(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	AmsModule	me;
	short		subjectNbr;
	AmsEvent	event;
	short		cn, sn;
	int		zn, nn, len, ct, pr;
	unsigned char	fl;
	AmsMsgType	mt;
	char		*txt;
	int		msgNbr = 0;
	int		msgs = -1;
	double		bytes = 0.0;
	struct timeval	startTime;
	struct timeval	endTime;
	struct timeval	prevTime;
	struct timeval	nowTime;
	double		usecElapsed;
	double		seconds;
	double		msgsPerSec;
	double		bytesPerSec;
	double		Mbps;
	char		buf[256];

	/*	Per-message timing arrays (allocated after first msg).	*/

	double		*iatUsec = NULL;	/*	Inter-arrival times.	*/
	int		*recvOrder = NULL;	/*	Message numbers.	*/
	int		capacity = 0;
	int		outOfOrder = 0;
	int		prevMsgNbr = -1;
	int		minLen = 0;
	int		maxLen = 0;

	if (ams_register("", NULL, "amsdemo", "test", "", "benchr", &me) < 0)
	{
		putErrmsg("amsbenchr can't register.", NULL);
		return -1;
	}

	subjectNbr = ams_lookup_subject_nbr(me, "bench");
	if (subjectNbr < 0)
	{
		putErrmsg("amsbenchr: subject 'bench' is unknown.", NULL);
		return -1;
	}

	if (ams_subscribe(me, 0, 0, 0, subjectNbr, 8, 0, AmsTransmissionOrder,
				AmsAssured) < 0)
	{
		putErrmsg("amsbenchr can't subscribe.", NULL);
		return -1;
	}

	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &event) < 0)
		{
			putErrmsg("amsbenchr can't get event.", NULL);
			return -1;
		}

		if (ams_get_event_type(event) == AMS_MSG_EVT)
		{
			ams_parse_msg(event, &cn, &zn, &nn, &sn, &len, &txt,
					&ct, &mt, &pr, &fl);
			memcpy((char *) &msgNbr, txt, sizeof(int));
			msgNbr = ntohl(msgNbr);

			if (msgNbr < 1)
			{
				writeMemoNote("Message number is < 1",
						itoa(msgNbr));
				msgNbr = 1;
			}

			if (msgs < 0)		/*	First message.	*/
			{
				getCurrentTime(&startTime);
				prevTime = startTime;
				msgs = 0;

				/*	Allocate arrays based on first
				 *	message's msgNbr (= total count).*/

				capacity = msgNbr;
				iatUsec = (double *) calloc(capacity,
						sizeof(double));
				recvOrder = (int *) calloc(capacity,
						sizeof(int));
				minLen = len;
				maxLen = len;
			}

			getCurrentTime(&nowTime);

			/*	Record inter-arrival time.		*/

			if (iatUsec != NULL && msgs < capacity)
			{
				iatUsec[msgs] = tvdiff_usec(&prevTime,
						&nowTime);
			}

			prevTime = nowTime;

			/*	Record receive order for reorder check.	*/

			if (recvOrder != NULL && msgs < capacity)
			{
				recvOrder[msgs] = msgNbr;
			}

			/*	Track out-of-order (sender counts down).*/

			if (prevMsgNbr >= 0 && msgNbr > prevMsgNbr)
			{
				outOfOrder++;
			}

			prevMsgNbr = msgNbr;

			/*	Track min/max message size.		*/

			if (len < minLen) minLen = len;
			if (len > maxLen) maxLen = len;

			bytes += len;
			msgs += 1;
		}

		ams_recycle_event(event);
		if (msgNbr == 1)		/*	Last message.	*/
		{
			break;
		}
	}

	getCurrentTime(&endTime);
	usecElapsed = tvdiff_usec(&startTime, &endTime);
	seconds = usecElapsed / 1000000.0;
	msgsPerSec = msgs / seconds;
	bytesPerSec = bytes / seconds;
	Mbps = (bytesPerSec * 8) / (1024 * 1024);

	/*	Basic throughput stats (original).			*/

	isprintf(buf, sizeof buf,
		"Received %d messages, a total of %.0f bytes,"
		"in %f seconds.", msgs, bytes, seconds);
	PUTS(buf);
	isprintf(buf, sizeof buf,
		"%10.3f messages per second.", msgsPerSec);
	PUTS(buf);
	isprintf(buf, sizeof buf,
		"%10.3f Mbps.", Mbps);
	PUTS(buf);

	/*	Message size stats.					*/

	isprintf(buf, sizeof buf,
		"Message size: min=%d  max=%d  avg=%.0f bytes.",
		minLen, maxLen, msgs > 0 ? bytes / msgs : 0.0);
	PUTS(buf);

	/*	Ordering stats.						*/

	isprintf(buf, sizeof buf,
		"Out-of-order: %d of %d messages (%.2f%%).",
		outOfOrder, msgs,
		msgs > 0 ? 100.0 * outOfOrder / msgs : 0.0);
	PUTS(buf);

	/*	Inter-arrival time stats.				*/

	if (iatUsec != NULL && msgs > 1)
	{
		double	iatMin;
		double	iatMax;
		double	iatSum = 0.0;
		double	iatMean;
		double	iatP50;
		double	iatP95;
		double	iatP99;
		double	iatJitterSum = 0.0;
		double	iatJitter;
		int	iatCount = msgs - 1;	/*	n-1 intervals.	*/
		int	i;

		/*	Skip first IAT (includes subscription setup).	*/

		iatMin = iatUsec[1];
		iatMax = iatUsec[1];
		for (i = 1; i < msgs && i < capacity; i++)
		{
			if (iatUsec[i] < iatMin) iatMin = iatUsec[i];
			if (iatUsec[i] > iatMax) iatMax = iatUsec[i];
			iatSum += iatUsec[i];
			if (i > 1)
			{
				double diff = iatUsec[i] - iatUsec[i - 1];
				if (diff < 0) diff = -diff;
				iatJitterSum += diff;
			}
		}

		iatMean = iatSum / iatCount;
		iatJitter = (iatCount > 1)
				? iatJitterSum / (iatCount - 1) : 0.0;

		/*	Sort for percentiles (copy to avoid disturbing
		 *	order; reuse iatUsec since we're done with it).	*/

		qsort(iatUsec + 1, iatCount, sizeof(double),
				compare_double);
		iatP50 = iatUsec[1 + iatCount / 2];
		iatP95 = iatUsec[1 + (int)(iatCount * 0.95)];
		iatP99 = iatUsec[1 + (int)(iatCount * 0.99)];

		isprintf(buf, sizeof buf,
			"Inter-arrival (us): "
			"min=%.0f  max=%.0f  avg=%.0f  "
			"p50=%.0f  p95=%.0f  p99=%.0f",
			iatMin, iatMax, iatMean,
			iatP50, iatP95, iatP99);
		PUTS(buf);
		isprintf(buf, sizeof buf,
			"Mean jitter (us): %.1f", iatJitter);
		PUTS(buf);
	}

	fflush(stdout);
	writeErrmsgMemos();

	if (iatUsec != NULL) free(iatUsec);
	if (recvOrder != NULL) free(recvOrder);

	ams_unregister(me);
	return 0;
}
