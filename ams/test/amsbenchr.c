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

#if defined (VXWORKS) || defined (RTEMS)
int	amsbenchr(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	AmsModule		me;
	int		subjectNbr;
	AmsEvent	event;
	int		cn, zn, nn, sn, len, ct, pr;
	unsigned char	fl;
	AmsMsgType	mt;
	char		*txt;
	int		count = -1;
	int		msgs = 0;
	double		bytes = 0.0;
	struct timeval	startTime;
	struct timeval	endTime;
	double		usecElapsed;
	double		seconds;
	double		msgsPerSec;
	double		bytesPerSec;
	double		Mbps;
	char		buf[128];

	if (ams_register("amsmib.xml", NULL, NULL, NULL, 0, "amsdemo", "test",
			"", "benchr", &me) < 0)
	{
		putSysErrmsg("amsbenchr can't register", NULL);
		return -1;
	}

	subjectNbr = ams_lookup_subject_nbr(me, "bench");
	if (subjectNbr < 0)
	{
		putErrmsg("Subject 'bench' is unknown.", NULL);
		return -1;
	}

	if (ams_subscribe(me, 0, 0, 0, subjectNbr, 8, 0, AmsTransmissionOrder,
				AmsAssured) < 0)
	{
		putSysErrmsg("amsbenchr can't subscribe", NULL);
		return -1;
	}

	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &event) < 0)
		{
			putSysErrmsg("amsbenchr can't get event", NULL);
			return -1;
		}

		if (ams_get_event_type(event) == AMS_MSG_EVT)
		{
			ams_parse_msg(event, &cn, &zn, &nn, &sn, &len, &txt,
					&ct, &mt, &pr, &fl);
			if (count < 0)	/*	First message.		*/
			{
				getCurrentTime(&startTime);
				memcpy((char *) &count, txt, sizeof(int));
				msgs = count = ntohl(count);
				if (count < 1)
				{
					putErrmsg("Count in message is < 1.",
							NULL);
					break;
				}
			}

			bytes += len;
			count--;
		}

		ams_recycle_event(event);
		if (count == 0)
		{
			break;
		}
	}

	getCurrentTime(&endTime);
	if (endTime.tv_usec < startTime.tv_usec)
	{
		endTime.tv_usec += 1000000;
		startTime.tv_sec -= 1;
	}

	usecElapsed = (1000000 * (endTime.tv_sec - startTime.tv_sec))
			+ (endTime.tv_usec - startTime.tv_usec);
	seconds = usecElapsed / 1000000;
	msgsPerSec = msgs / seconds;
	bytesPerSec = bytes / seconds;
	Mbps = (bytesPerSec * 8) / (1024 * 1024);
	isprintf(buf, sizeof buf, "Received %d messages, totalling %.0f bytes,\
in %f seconds.", msgs, bytes, seconds);
	PUTS(buf);
	isprintf(buf, sizeof buf, "%10.3f messages per second.", msgsPerSec);
	PUTS(buf);
	isprintf(buf, sizeof buf, "%10.3f Mbps.", Mbps);
	PUTS(buf);
	writeErrmsgMemos();
	ams_unregister(me);
	return 0;
}
