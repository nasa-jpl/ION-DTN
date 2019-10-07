/*
	amspubsub.c:	an AMS test program for VxWorks.  Contains
			two functions: amssub(), which subscribes to
			messages on a given subject, and amspub(),
			which publishes one message on a given subject.
 									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Amalaye Oyake, Jet Propulsion Laboratory		*/

#include "ams.h"

int	amssub(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*applicationName = (char *) a1;
	char		*authorityName = (char *) a2;
	char		*subjectName = (char *) a3;
	AmsModule	me;
	int		subjectNbr;
	AmsEvent	event;
	int		cn, un, nn, sn, len, ct, pr, fl;
	char		*txt;
	AmsMsgType	mt;
	char		buf[256];

	if (applicationName == NULL
	|| authorityName == NULL
	|| subjectName == NULL)
	{
		PUTS("Usage: amssub \"<application name>\", \"<authority \
name>\", \"<subject name>\"");
		return 0;
	}

	if (ams_register("", NULL, applicationName, authorityName, "", "log",
			&me) < 0)
	{
		putErrmsg("amssub can't register.", NULL);
		return -1;
	}

	subjectNbr = ams_lookup_subject_nbr(me, subjectName);
	if (subjectNbr < 0)
	{
		ams_unregister(me);
		putErrmsg("amssub: subject is unknown", subjectName);
		return -1;
	}

	if (ams_subscribe(me, 0, 0, 0, subjectNbr, 8, 0, AmsTransmissionOrder,
				AmsAssured) < 0)
	{
		ams_unregister(me);
		putErrmsg("amssub can't subscribe.", NULL);
		return -1;
	}

	while (1)
	{
		if (ams_get_event(me, AMS_BLOCKING, &event) < 0)
		{
			ams_unregister(me);
			putErrmsg("amssub can't get event.", NULL);
			return -1;
		}

		if (ams_get_event_type(event) == AMS_MSG_EVT)
		{
			ams_parse_msg(event, &cn, &un, &nn, &sn, &len, &txt,
					&ct, &mt, &pr, &fl);
			isprintf(buf, sizeof buf, "msg length %d content '%s'",
					len, txt);
			PUTS(buf);
		}

		ams_recycle_event(event);
	}
}

int	amspub(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*applicationName = (char *) a1;
	char		*authorityName = (char *) a2;
	char		*subjectName = (char *) a3;
	char		*msgText = (char *) a4;
	AmsModule	me;
	int		subjectNbr;

	if (applicationName == NULL
	|| authorityName == NULL
	|| subjectName == NULL
	|| msgText == NULL)
	{
		PUTS("Usage: amspub \"<application name>\", \"<authority \
name>\", \"<subject name>\", \"<message text>\"");
		return 0;
	}

	if (ams_register("", NULL, applicationName, authorityName, "", "shell",
			&me) < 0)
	{
		putErrmsg("amspub can't register.", NULL);
		return -1;
	}

	subjectNbr = ams_lookup_subject_nbr(me, subjectName);
	if (subjectNbr < 0)
	{
		putErrmsg("amspub: subject is unknown", subjectName);
		ams_unregister(me);
		return -1;
	}

	snooze(1);	/*	Wait for subscriptions to arrive.	*/
	if (ams_publish(me, subjectNbr, 0, 0, strlen(msgText) + 1, msgText, 0)
			< 0)
	{
		putErrmsg("amspub can't publish message.", NULL);
	}

	snooze(1);
	writeErrmsgMemos();
	ams_unregister(me);
	return 0;
}
