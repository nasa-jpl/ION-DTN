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

#if defined (ION_LWT)
int	amsbenchr(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	AmsModule	me;
	int		subjectNbr;
	AmsEvent	event;
	int		cn, zn, nn, sn, len, ct, pr;
	unsigned char	fl;
	AmsMsgType	mt;
	char		*txt;
	int		msgNbr = 0;
	int		msgs = -1;
	double		bytes = 0.0;
	struct timeval	startTime;
	struct timeval	endTime;
	double		usecElapsed;
	double		seconds;
	double		msgsPerSec;
	double		bytesPerSec;
	double		Mbps;
	char		buf[128];

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

			/*	Messages arrive in reverse nbr order.	*/

			if (msgNbr < 1)
			{
				writeMemoNote("Message number is < 1",
						itoa(msgNbr));
				msgNbr = 1;
			}

			if (msgs < 0)		/*	First message.	*/
			{
				getCurrentTime(&startTime);
				msgs = 0;
			}

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
	isprintf(buf, sizeof buf, "Received %d messages, a total of %.0f bytes,\
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
