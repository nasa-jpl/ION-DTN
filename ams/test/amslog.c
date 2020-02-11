/*
	amslog.c:	a simple AMS test program.  amslog
			registers in a specified message space (named
			in a command-line parameter), subscribes to a
			list of subjects that it reads from stdin, and
			writes to stdout the subject, content length,
			and content of all messages it receives.
									*/
/*									*/
/*	Copyright (c) 2005, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ams.h"

static int	_amslog_running(int *value)
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

	oK(_amslog_running(&stop));

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

	oK(_amslog_running(&stop));

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

static void	logToStderr(char *text)
{
	fprintf(stderr, "%s\n", text);
	fflush(stderr);
}

static void	logMsg(AmsModule me, void *userData, AmsEvent *event,
			int continuumNbr, int unitNbr, int nodeNbr,
			int subjectNbr, int contentLength, char *content,
			int context, AmsMsgType msgType, int priority,
			unsigned char flowLabel)
{
	char	*subjectName;
#ifdef unix
	int	subjectNameLength;
#endif
	char	replyText[256];
	int	replyLength;

	subjectName = ams_lookup_subject_name(me, subjectNbr);
	if (subjectName == NULL)
	{
		fprintf(stderr, "Unknown subject number: %d.\n", subjectNbr);
		return;
	}
#ifndef unix
	if (fprintf(stdout, "subject %d (%s), %d bytes of content: '%s'\n",
			subjectNbr, subjectName, contentLength, content) < 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing subject length");
		}

		killMainThread();
		return;
	}
#else		/*	Assume message is piped, possibly to amslogprt.	*/
	subjectNameLength = strlen(subjectName);
	if (fwrite((char *) &subjectNameLength, sizeof subjectNameLength,
				1, stdout) == 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing subject length");
		}

		killMainThread();
		return;
	}

	if (fwrite(subjectName, subjectNameLength, 1, stdout) == 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing subject name");
		}

		killMainThread();
		return;
	}

	if (fwrite((char *) &contentLength, sizeof contentLength, 1, stdout)
			== 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing content length");
		}

		killMainThread();
		return;
	}

	if (contentLength > 0)
	{
		if (fwrite(content, contentLength, 1, stdout) == 0)
		{
			if (ferror(stdout))
			{
				perror("amslog error writing content");
			}

			killMainThread();
			return;
		}
	}
#endif
	if (msgType == AmsMsgQuery)
	{
		isprintf(replyText, sizeof replyText, "Got '%.128s'.", content);
		replyLength = strlen(replyText);
		if (ams_reply(me, *event, subjectNbr, priority, 0, replyLength,
				replyText) < 0)
		{
			putErrmsg("amslog can't send reply message.", NULL);
			killMainThread();
			return;
		}
	}

	fflush(stdout);
}

static void	interruptAmslog(void *userData, AmsEvent *event)
{
	fputs("AMS event loop terminated.\n", stderr);
	killMainThread();
}

#if defined (ION_LWT)
int	amslog(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownUnitName = (char *) a1;
	char		*ownRoleName = (char *) a2;
	char		*applicationName = (char *) a3;
	char		*authorityName = (char *) a4;
	char		*mode = (a5 == 0 ? "s" : (char *) a5);
#else
int	main(int argc, char **argv)
{
	char		*ownUnitName = (argc > 1 ? argv[1] : NULL);
	char		*ownRoleName = (argc > 2 ? argv[2] : NULL);
	char		*applicationName = (argc > 3 ? argv[3] : NULL);
	char		*authorityName = (argc > 4 ? argv[4] : NULL);
	char		*mode = (argc > 5 ? argv[5] : "s");
#endif
	AmsModule	me;
	AmsEventMgt	rules;
	int		start = 1;
	int		stop = 0;
	int		asserting;
	char		buffer[256];
	char		*cmdString = buffer;
	int		stringLength;
	int		parmCount;
	char		subjectName[33];
	char		unitName[33];
	char		roleName[33];
	char		continName[33];			// CW, 5/1/06
	int		subjectNbr;              
	int		unitNbr;
	int		roleNbr;
	int		continNbr;			// CW, 5/1/06

	if (ownUnitName == NULL || ownRoleName == NULL
	|| applicationName == NULL || authorityName == NULL
	|| (strcmp(mode, "s") && strcmp(mode, "i")))
	{
		fputs("Usage: amslog <unit name> <role name> <application \
name> <authority name> [{ s | i }]\n", stderr);
		fputs("   e.g., amslog \"\" log amsdemo test\n", stderr);
		fputs("   By default, subscribes to subjects.\n", stderr);
		fputs("   To override, use 'i' (invite).\n", stderr);
		fputs("   Takes subject names from stdin, logs all \
messages to stdout.\n", stderr);
		fputs("   Enter '.' to terminate.\n", stderr);
		return 0;
	}

#ifndef mingw
	oK(_mainThread());
#endif
	oK(_amslog_running(&start));
	isignal(SIGINT, handleQuit);
	setLogger(logToStderr);
	if (ams_register("", NULL, applicationName, authorityName, ownUnitName,
			ownRoleName, &me) < 0)
	{
		putErrmsg("amslog can't register.", NULL);
		return -1;
	}

	memset((char *) &rules, 0, sizeof(AmsEventMgt));
	rules.msgHandler = logMsg;
	rules.errHandler = interruptAmslog;
	if (ams_set_event_mgr(me, &rules) < 0)
	{
		putErrmsg("amslog can't set event manager.", NULL);
		ams_unregister(me);
		return -1;
	}

	while (_amslog_running(NULL))
	{
		asserting = 1;
		if (fgets(cmdString, 256, stdin) == NULL)
		{
			if (ferror(stdin))
			{ 
				putSysErrmsg("amslog can't read from stdin",
						NULL);
			}

			oK(_amslog_running(&stop));
			continue;
		}

		switch (*cmdString)
		{
		case '.':		/*	Quitting.		*/
			oK(_amslog_running(&stop));
			continue;

		case '+':		/*	Asserting.		*/
			cmdString++;
			break;		/*	Out of switch.		*/
	
		case '-':		/*	Canceling.		*/
			asserting = 0;
			cmdString++;
			break;		/*	Out of switch.		*/

		default:		/*	Assume no control char.	*/
			break;		/*	Out of switch.		*/
		}
	
		/*	Get rid of terminal newline.			*/

		stringLength = strlen(cmdString);
		if (cmdString[stringLength - 1] == '\n')
		{
			cmdString[stringLength - 1] = '\0';
		}
		else
		{
			writeMemo("[?] amslog input line too long.");
			continue;
		}

		/*	Parse parameters out of command.		*/

		roleNbr = 0;	/*	Default is "all roles".		*/
		unitNbr = 0;	/*	Default is root unit.		*/
		continNbr = 0;	/*	Default is "all continua".	*/
		parmCount = sscanf(cmdString, "%32s %32s %32s %32s",
				subjectName, continName,	// CW, 5/1/06  
				unitName, roleName);                     
		switch (parmCount)
		{
		case 4:						// CW 5/1/06
			roleNbr = ams_lookup_role_nbr(me, roleName);
			if (roleNbr < 0)
			{
				writeMemoNote("[?] amslog unknown role",
						roleName);
				continue;
			}

			/*	Intentional fall-through to next case.	*/

		case 3:						// CW 5/1/06
			if (strcmp(unitName, "_") != 0)
			{

				unitNbr = ams_lookup_unit_nbr(me, unitName);
				if (unitNbr < 0)
				{
					writeMemoNote("[?] amslog unknown unit",
							unitName);
					continue;
				}
			}

			/*	Intentional fall-through to next case.	*/

		case 2:  
			if (strcmp(continName, "_") != 0)	// CW, 5/19/06
			{
				continNbr = ams_lookup_continuum_nbr(me,
						continName);
				if (continNbr < 0)
				{
					writeMemoNote("[?] amslog unknown \
continuum", continName);
					continue;
				}
			}

			/*	Intentional fall-through to next case.	*/

		case 1:
			subjectNbr = ams_lookup_subject_nbr(me, subjectName);
			if (subjectNbr < 0)
			{
				writeMemoNote("[?] amslog unknown subject",
						subjectName);
				continue;
			}

			if (subjectNbr == 0)	/*	All subjects.	*/
			{
				if (continNbr == 0)
				{
					/*	Use different default,
						because subscription to
						"everything" is valid
						only within the local
						continuum.		*/

					continNbr = THIS_CONTINUUM;
				}

				if (continNbr != THIS_CONTINUUM)
				{
					writeMemoNote("[?] amslog unknown \
subject", subjectName);
					continue;
				}
			}

			break;

		default:
			writeMemo("[?] amslog null command ignored.");
			continue;
		}

		if (asserting)
		{
			if (*mode == 's')
			{
				if (ams_subscribe(me, roleNbr,
						continNbr,	// CW, 5/1/06
						unitNbr, subjectNbr,
						8, 0, AmsArrivalOrder,
						AmsBestEffort) < 0)
				{
					putErrmsg("amslog can't subscribe to \
subject", subjectName);
					continue;
				}
			}
			else
			{
				if (ams_invite(me, roleNbr,
						continNbr,
						unitNbr, subjectNbr,
						8, 0, AmsArrivalOrder,
						AmsBestEffort) < 0)
				{
					putErrmsg("amslog can't invite this \
subject", subjectName);
					continue;
				}
			}
		}
		else
		{
			if (*mode == 's')
			{
				if (ams_unsubscribe(me, roleNbr,
						continNbr, // CW 5/1/06
						unitNbr, subjectNbr) < 0)
				{
					putErrmsg("amslog can't unsubscribe \
to subject", subjectName);
					continue;
				}
			}
			else
			{
				if (ams_disinvite(me, roleNbr,
						continNbr, // CW 5/1/06
						unitNbr, subjectNbr) < 0)
				{
					putErrmsg("amslog can't disinvite \
this subject", subjectName);
					continue;
				}
			}
		}
	}

	ams_unregister(me);
	writeErrmsgMemos();
	return 0;
}
