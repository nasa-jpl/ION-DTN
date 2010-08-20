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
#include <ams.h>

static void	reportError(void *userData, AmsEvent *event)
{
	PUTS("AMS event loop crashed.");
}

static void	handleQuit()
{
	PUTS("Terminating amsbenchs.");
}

#if defined (VXWORKS) || defined (RTEMS)
int	amsbenchs(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int		count = a1;
	int		size = a2;
#else
int	main(int argc, char **argv)
{
	int		count = (argc > 1 ? atoi(argv[1]) : 0);
	int		size = (argc > 2 ? atoi(argv[2]) : 0);
#endif
	char		*application = "amsdemo";
	char		*authority = "test";
	char		*buffer;
	AmsModule		me;
	AmsEventMgt	rules;
	int		subjectNbr;
	int		content;

	if (count < 1 || size < 4 || size > 65535)
	{
		PUTS("Usage: amsbenchs <# of msgs to send> <msg length>");
		return 0;
	}

	buffer = malloc(size);
	if (buffer == NULL)
	{
		putSysErrmsg("No memory for amsbenchs.", NULL);
		return 0;
	}

	memset(buffer, ' ', size);
	if (ams_register("amsmib.xml", NULL, NULL, NULL, 0, application,
			authority, "", "benchs", &me) < 0)
	{
		putSysErrmsg("amsbenchs can't register", NULL);
		return -1;
	}

	memset((char *) &rules, 0, sizeof(AmsEventMgt));
	rules.errHandler = reportError;
	if (ams_set_event_mgr(me, &rules) < 0)
	{
		ams_unregister(me);
		putSysErrmsg("amsbenchs can't set event manager", NULL);
		return -1;
	}

	subjectNbr = ams_lookup_subject_nbr(me, "bench");
	if (subjectNbr < 0)
	{
		ams_unregister(me);
		putErrmsg("Subject 'bench' is unknown.", NULL);
		return -1;
	}

	snooze(1);	/*	Wait for subscriptions to arrive.	*/
	while (count > 0)
	{
		content = htonl(count);
		memcpy(buffer, (char *) &content, sizeof(int));
		if (ams_publish(me, subjectNbr, 0, 0, size, buffer, 0) < 0)
		{
			putErrmsg("Unable to publish message.", NULL);
			break;
		}

		count--;
	}

	writeErrmsgMemos();
	PUTS("Message publication ended; press ^C when test is done.");
	signal(SIGINT, handleQuit);
	snooze(3600);
	ams_unregister(me);
	return 0;
}
