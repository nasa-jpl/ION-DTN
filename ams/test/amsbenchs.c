/*
	amsbenchs.c:	an AMS benchmark program for UNIX.  Publishes
			N messages of size M, each containing the
			total number of messages to be published at
			this point (including this one), then stops.
									*/
/*									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#include "ams.h"

static void	reportError(void *userData, AmsEvent *event)
{
	PUTS("AMS event loop crashed.");
}

static void	handleQuit(int signum)
{
	PUTS("Terminating amsbenchs.");
}

#if defined (ION_LWT)
int	amsbenchs(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	int		count = a1;
	int		size = a2;
#else
int	main(int argc, char **argv)
{
	int		count = (argc > 1 ? atoi(argv[1]) : 0);
	int		size = (argc > 2 ? atoi(argv[2]) : 0);
#endif
	char		*buffer;
	AmsModule	me;
	AmsEventMgt	rules;
	int		subjectNbr;
	int		content;

	if (count < 1 || size < sizeof(int) || size > 65535)
	{
		PUTS("Usage: amsbenchs <# of msgs to send> <msg length>");
		return 0;
	}

	buffer = malloc(size);
	if (buffer == NULL)
	{
		putErrmsg("No memory for amsbenchs.", NULL);
		return 0;
	}

	if (size > sizeof(int))
	{
		memset(buffer, ' ', size - 1);
		buffer[size - 1] = '\0';
	}

	if (ams_register("", NULL, "amsdemo", "test", "", "benchs", &me) < 0)
	{
		putErrmsg("amsbenchs can't register.", NULL);
		return -1;
	}

	memset((char *) &rules, 0, sizeof(AmsEventMgt));
	rules.errHandler = reportError;
	if (ams_set_event_mgr(me, &rules) < 0)
	{
		ams_unregister(me);
		putErrmsg("amsbenchs can't set event manager.", NULL);
		return -1;
	}

	subjectNbr = ams_lookup_subject_nbr(me, "bench");
	if (subjectNbr < 0)
	{
		ams_unregister(me);
		writeMemo("[?] amsbenchs: subject 'bench' is unknown.");
		return -1;
	}

	snooze(1);	/*	Wait for subscriptions to arrive.	*/
	while (count > 0)
	{
		content = htonl(count);
		memcpy(buffer, (char *) &content, sizeof(int));
		if (ams_publish(me, subjectNbr, 0, 0, size, buffer, 0) < 0)
		{
			putErrmsg("amsbenchs can't publish message.", NULL);
			break;
		}

		count--;
	}

	writeErrmsgMemos();
	PUTS("Message publication ended; press ^C when test is done.");
	isignal(SIGINT, handleQuit);
	snooze(3600);
	ams_unregister(me);
	return 0;
}
