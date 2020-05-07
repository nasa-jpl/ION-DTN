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
#include "ams.h"

#define	MAX_SUBJ_NAME	32

static int	_amsshell_running(int *value)
{
	static int	running = 0;

	if (value)
	{
		running = (*value == 0 ? 0 : 1);
	}

	return running;
}

#ifdef mingw
static void	killMainThread()
{
	int	stop = 0;

	oK(_amsshell_running(&stop));

	/*	Must make sure fgets is interrupted.			*/

	fclose(stdin);
}
#else
static pthread_t	_mainThread()
{
	static pthread_t	mainThread;
	static int		haveMainThread = 0;

	if (haveMainThread == 0)
	{
		mainThread = pthread_self();
		haveMainThread = 1;
	}

	return mainThread;
}

static void	killMainThread()
{
	int		stop = 0;
	pthread_t	mainThread = _mainThread();

	oK(_amsshell_running(&stop));

	/*	Must make sure fgets is interrupted.			*/

	if (!pthread_equal(mainThread, pthread_self()))
	{
		pthread_kill(mainThread, SIGINT);
	}
}
#endif

static void	handleQuit(int signum)
{
	isignal(SIGINT, handleQuit);
	killMainThread();
}

static void	handleCommand(AmsModule me, char *mode)
{
	static int	continuumNbr = 0;
	static int	unitNbr = 0;
	static int	moduleNbr = 0;
	static int	roleNbr = 0;
	static char	subjectName[MAX_SUBJ_NAME + 1] = "";
	int		stop = 0;
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
			putSysErrmsg("amsshell can't read line from console",
					NULL);
		}

		oK(_amsshell_running(&stop));
		return;
	}

	newline = line + strlen(line) - 1;
	if (*newline != '\n')
	{
		writeMemo("[?] amsshell input line is too long; max length \
is 255.");
		return;
	}

	*newline = '\0';
	switch (*line)
	{
	case '.':		/*	Quitting.			*/
		oK(_amsshell_running(&stop));
		return;

	case 'r':		/*	Setting role number.		*/
		roleNbr = strtol(line + 1, NULL, 0);
		return;

	case 'c':		/*	Setting continuum number.	*/
		continuumNbr = strtol(line + 1, NULL, 0);
		return;

	case 'u':		/*	Setting unit number.		*/
		unitNbr = strtol(line + 1, NULL, 0);
		return;

	case 'm':		/*	Setting node number.		*/
		moduleNbr = strtol(line + 1, NULL, 0);
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
		if (subjectNameLength > MAX_SUBJ_NAME)
		{
			writeMemoNote("[?] amsshell subject name is too long",
					subjectNameString);
			return;
		}

		istrcpy(subjectName, subjectNameString, MAX_SUBJ_NAME);
		break;

	default:
		content = line;
		contentLength = strlen(content) + 1;
	}

	if (strlen(subjectName) == 0)
	{
		writeMemo("[?] amsshell: must specify subject before \
publishing msg.");
		return;
	}

	subjectNbr = ams_lookup_subject_nbr(me, subjectName);
	if (subjectNbr < 0)
	{
		writeMemoNote("[?] amsshell unknown subject; can't publish \
message", subjectName);
		return;
	}

	switch (*mode)
	{
	case 'a':
		if (ams_announce(me, roleNbr, continuumNbr, unitNbr,
			subjectNbr, 8, 0, contentLength, content, 0) < 0)
		{
			putErrmsg("amsshell can't announce message.", NULL);
		}

		return;

	case 's':
		if (ams_send(me, continuumNbr, unitNbr, moduleNbr,
			subjectNbr, 8, 0, contentLength, content, 0) < 0)
		{
			putErrmsg("amsshell can't send message.", NULL);
		}

		return;

	case 'q':
		if (ams_invite(me, 0, 0, 0, subjectNbr, 8, 0, AmsArrivalOrder,
			AmsBestEffort) < 0)
		{
			putErrmsg("amsshell can't invite reply messages.",
					NULL);
			return;
		}

		snooze(2);
		if (ams_query(me, continuumNbr, unitNbr, moduleNbr,
			subjectNbr, 8, 0, contentLength, content, 43,
			5000000, &event) < 0)
		{
			putErrmsg("amsshell can't send message.", NULL);
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

		fflush(stdout);
		ams_recycle_event(event);
		return;

	default:
		if (ams_publish(me,
			subjectNbr, 0, 0, contentLength, content, 0) < 0)
		{
			putErrmsg("amsshell can't publish message.", NULL);
		}
	}
}

static void	reportError(void *userData, AmsEvent *event)
{
	puts("AMS event loop terminated.");
	killMainThread();
}

#if defined (ION_LWT)
int	amsshell(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*unitName = (char *) a1;
	char	*roleName = (char *) a2;
	char	*applicationName = (char *) a3;
	char	*authorityName = (char *) a4;
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
	AmsModule	me;
	AmsEventMgt	rules;
	int		start = 1;

#ifndef FSWLOGGER	/*	Need stdin/stdout for interactivity.	*/
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

#ifndef mingw
	oK(_mainThread());
#endif
	oK(_amsshell_running(&start));
	isignal(SIGINT, handleQuit);
	if (ams_register("", NULL, applicationName, authorityName, unitName,
			roleName, &me) < 0)
	{
		putErrmsg("amsshell can't register.", NULL);
		return -1;
	}

	memset((char *) &rules, 0, sizeof(AmsEventMgt));
	rules.errHandler = reportError;
	if (ams_set_event_mgr(me, &rules) < 0)
	{
		putErrmsg("amsshell can't set event manager.", NULL);
		ams_unregister(me);
		return -1;
	}

	while (_amsshell_running(NULL))
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
