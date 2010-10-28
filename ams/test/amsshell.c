/*
	amsshell.c:	an AMS utility program for UNIX.  amsshell
			listens for keyboard input and, whenever a
			line of keyboard input arrives, parses it
			into subject and ASCII text content and
			issues a message accordingly.
									*/
/*									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#include <ams.h>

static int	amsshell_running = 0;
static char	subjectName[33];
static int	continuumNbr = 0;
static int	unitNbr = 0;
static int	nodeNbr = 0;
static int	roleNbr = 0;

static void	handleQuit()
{
	puts("Please enter '.' to quit the program.");
}

static void	handleCommand(AmsModule me, char *mode)
{
	char		line[256];
	char		*newline;
	char		*delimiter;
	char		*subjectNameString;
	int		subjectNameLength;
	char		*content;
	int		contentLength;
	int		subjectNbr;
	AmsEvent	event;
	int		cn;
	int		un;
	int		nn;
	int		sn;
	int		cl;
	char		*rc;
	int		context;
	AmsMsgType	msgType;
	int		priority;
	unsigned char	flowLabel;

	if (fgets(line, 256, stdin) == NULL)
	{
		if (ferror(stdin))
		{
			writeErrMemo("Failed reading line from console");
		}

		amsshell_running = 0;
		return;
	}

	newline = line + strlen(line) - 1;
	if (*newline != '\n')
	{
		writeMemo("Input line is too long; max length is 255.");
		return;
	}

	*newline = '\0';
	switch (*line)
	{
	case '.':		/*	Quitting.			*/
		amsshell_running = 0;
		return;

	case 'r':		/*	Setting role number.		*/
		roleNbr = atoi(line + 1);
		return;

	case 'c':		/*	Setting continuum number.	*/
		continuumNbr = atoi(line + 1);
		return;

	case 'u':		/*	Setting unit number.		*/
		unitNbr = atoi(line + 1);
		return;

	case 'n':		/*	Setting node number.		*/
		nodeNbr = atoi(line + 1);
		return;

	case '=':		/*	Setting subject name.		*/
		subjectNameString = line + 1;
		delimiter = strchr(subjectNameString, ' ');
		if (delimiter)
		{
			*delimiter = '\0';
			content = delimiter + 1;
			contentLength = strlen(content) + 1;
		}
		else
		{
			content = NULL;
			contentLength = 0;
		}

		subjectNameLength = strlen(subjectNameString);
		if (subjectNameLength >= sizeof subjectName)
		{
			writeMemo("Subject name is too long.");
			return;
		}

		istrcpy(subjectName, subjectNameString, sizeof subjectName);
		break;

	default:
		content = line;
		contentLength = strlen(content) + 1;
	}

	if (strlen(subjectName) == 0)
	{
		writeMemo("Must specify subject before publishing message.");
		return;
	}

	subjectNbr = ams_lookup_subject_nbr(me, subjectName);
	if (subjectNbr < 0)
	{
		writeMemo("Unknown subject; can't publish message.");
		return;
	}

	switch (*mode)
	{
	case 'a':
		if (ams_announce(me, roleNbr, continuumNbr, unitNbr,
			subjectNbr, 8, 0, contentLength, content, 0) < 0)
		{
			putErrmsg("Unable to announce message.", NULL);
		}

		return;

	case 's':
		if (ams_send(me, continuumNbr, unitNbr, nodeNbr,
			subjectNbr, 8, 0, contentLength, content, 0) < 0)
		{
			putErrmsg("Unable to send message.", NULL);
		}

		return;

	case 'q':
		if (ams_invite(me, 0, 0, 0, subjectNbr, 8, 0, AmsArrivalOrder,
			AmsBestEffort) < 0)
		{
			putErrmsg("Unable to invite reply messages.", NULL);
			return;
		}

		snooze(2);
		if (ams_query(me, continuumNbr, unitNbr, nodeNbr,
			subjectNbr, 8, 0, contentLength, content, 43,
			5000000, &event) < 0)
		{
			putSysErrmsg("Unable to send message", NULL);
			return;
		}

		if (event == NULL)
		{
			return;
		}

		switch (ams_get_event_type(event))
		{
		case TIMEOUT_EVT:
			printf("Query timed out.\n");
			break;

		case AMS_MSG_EVT:
			ams_parse_msg(event, &cn, &un, &nn, &sn, &cl, &rc,
				&context, &msgType, &priority, &flowLabel);
			printf("Reply is '%s'.\n", rc);
			break;

		default:
			printf("Got event of type %d.\n",
					ams_get_event_type(event));
		}

		ams_recycle_event(event);
		return;

	default:
		if (ams_publish(me,
			subjectNbr, 0, 0, contentLength, content, 0) < 0)
		{
			putErrmsg("Unable to publish message.", NULL);
		}
	}
}

static void	reportError(void *userData, AmsEvent *event)
{
	puts("AMS event loop crashed.  Please enter '.' to quit.");
}

#if defined (VXWORKS) || defined (RTEMS)
int	amsshell(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*unitName = (char *) a1;
	char	*roleName = (char *) a2;
	char	*applicationName = (char *) a3;
	char	*authorityName = a4;
	char	*mode = (a5 == 0 ? "p" : (char *) a4);
#else
int	main(int argc, char **argv)
{
	char	*unitName = (argc > 1 ? argv[1] : NULL);
	char	*roleName = (argc > 2 ? argv[2] : NULL);
	char	*applicationName = (argc > 3 ? argv[3] : NULL);
	char	*authorityName = (argc > 4 ? argv[4] : NULL);
	char	*mode = (argc > 5 ? argv[5] : "p");
#endif
	AmsModule		me;
	AmsEventMgt	rules;

#ifndef FSWLOGGER	/*	Need stdin/stdout for interactivity.	*/
	signal(SIGINT, handleQuit);
	if (unitName == NULL || roleName == NULL
	|| applicationName == NULL || authorityName == NULL
	|| (strcmp(mode, "p") && strcmp(mode, "s") && strcmp(mode, "q")
			&& strcmp(mode, "a")))
	{
		fputs("Usage: amsshell <unit name> <role name> <application \
name> <authority name> [{ p | s | q | a }]\n", stderr);
		fputs("   e.g.:   amsshell \"\" shell amsdemo test\n", stderr);
		fputs("   By default, publishes messages.\n", stderr);
		fputs("   To override, use 's' (send) or 'q' (query) or 'a' \
(announce).\n", stderr);
		fputs("   Takes subject name & content from stdin, issues \
messages.\n", stderr);
		fputs("   Enter '.' to terminate.\n", stderr);
		return 0;
	}

	if (ams_register("amsmib.xml", NULL, NULL, NULL, 0, applicationName,
			authorityName, unitName, roleName, &me) < 0)
	{
		putSysErrmsg("amsshell can't register", NULL);
		return -1;
	}

	memset((char *) &rules, 0, sizeof(AmsEventMgt));
	rules.errHandler = reportError;
	if (ams_set_event_mgr(me, &rules) < 0)
	{
		putSysErrmsg("amsshell can't set event manager", NULL);
		ams_unregister(me);
		return -1;
	}

	subjectName[0] = '\0';
	amsshell_running = 1;
	while (amsshell_running)
	{
		printf(": ");
		fflush(stdout);
		handleCommand(me, mode);
	}

	writeErrmsgMemos();
	ams_unregister(me);
#endif
	return 0;
}
