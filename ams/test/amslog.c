/*
	amslog.c:	an AMS utility program for UNIX.  amslog
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

static int	amslog_running = 0;

static void	handleQuit()
{
	fputs("Please enter '.' to quit the program.\n", stderr);
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
	int	subjectNameLength;
	char	replyText[256];
	int	replyLength;

	subjectName = ams_lookup_subject_name(me, subjectNbr);
	if (subjectName == NULL)
	{
		fprintf(stderr, "Unknown subject number: %d.\n", subjectNbr);
		return;
	}
#if defined (VXWORKS) || defined (RTEMS)
	if (fprintf(stdout, "subject %d (%s), %d bytes of content: '%s'\n",
			subjectNbr, subjectName, contentLength, content) < 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing subject length");
		}

		kill(sm_TaskIdSelf(), SIGINT);
		return;
	}
#else
	subjectNameLength = strlen(subjectName);
	if (fwrite((char *) &subjectNameLength, sizeof subjectNameLength,
				1, stdout) == 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing subject length");
		}

		kill(sm_TaskIdSelf(), SIGINT);
		return;
	}

	if (fwrite(subjectName, subjectNameLength, 1, stdout) == 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing subject name");
		}

		kill(sm_TaskIdSelf(), SIGINT);
		return;
	}

	if (fwrite((char *) &contentLength, sizeof contentLength, 1, stdout)
			== 0)
	{
		if (ferror(stdout))
		{
			perror("amslog error writing content length");
		}

		kill(sm_TaskIdSelf(), SIGINT);
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

			kill(sm_TaskIdSelf(), SIGINT);
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
			putErrmsg("Can't send reply message.", NULL);
			kill(sm_TaskIdSelf(), SIGINT);
			return;
		}
	}

	fflush(stdout);
}

static void	interruptAmslog(void *userData, AmsEvent *event)
{
	fputs("AMS event loop crashed.  Please enter '.' to quit.\n", stderr);
}

#if defined (VXWORKS) || defined (RTEMS)
int	amslog(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*applicationName = (char *) a1;
	char		*authorityName = (char *) a2;
	char		*mode = (a3 == 0 ? "s" : (char *) a3);
#else
int	main(int argc, char **argv)
{
	char		*applicationName = (argc > 1 ? argv[1] : NULL);
	char		*authorityName = (argc > 2 ? argv[2] : NULL);
	char		*mode = (argc > 3 ? argv[3] : "s");
#endif
	AmsModule		me;
	AmsEventMgt	rules;
	int		asserting;
	char		buffer[256];
	char		*cmdString = buffer;
	int		stringLength;
	int		parmCount;
	char		subjectName[256];
	char		unitName[256];
	char		roleName[256];
	char		cntName[256];		// CW, 5/1/06
	int		subjectNbr;              
	int		unitNbr;
	int		roleNbr;
	int		cntNbr;			// CW, 5/1/06

	if (applicationName == NULL || authorityName == NULL
	|| (strcmp(mode, "s") && strcmp(mode, "i")))
	{
		fputs("Usage: amslog <application name> <authority name>\
[{ s | i }]\n", stderr);
		fputs("   By default, subscribes to subjects.\n", stderr);
		fputs("   To override, use 'i' (invite).\n", stderr);
		fputs("   Takes subject names from stdin, logs all \
messages to stdout.\n", stderr);
		fputs("   Enter '.' to terminate.\n", stderr);
		return 0;
	}

	signal(SIGINT, handleQuit);
	setLogger(logToStderr);
	if (ams_register("amsmib.xml", NULL, NULL, NULL, 0, applicationName,
			authorityName, "", "log", &me) < 0)
	{
		putSysErrmsg("amslog can't register", NULL);
		return -1;
	}

	memset((char *) &rules, 0, sizeof(AmsEventMgt));
	rules.msgHandler = logMsg;
	rules.errHandler = interruptAmslog;
	if (ams_set_event_mgr(me, &rules) < 0)
	{
		putSysErrmsg("amslog can't set event manager", NULL);
		ams_unregister(me);
		return -1;
	}

	amslog_running = 1;
	while (amslog_running)
	{
		asserting = 1;
		if (fgets(cmdString, 256, stdin) == NULL)
		{
			break;		/*	Out of loop.		*/
		}

		switch (*cmdString)
		{
		case '.':		/*	Quitting.		*/
			amslog_running = 0;
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
			writeMemo("amslog input line too long");
			continue;
		}

		/*	Parse parameters out of command.		*/

		roleNbr = 0;	/*	Default is "all roles".		*/
		unitNbr = 0;	/*	Default is root unit.		*/
		cntNbr = 0;	/*	Default is "all continua".	*/	// CW, 5/1/06
		parmCount = sscanf(cmdString, "%255s %255s %255s %255s",
				subjectName,
				cntName,	// CW, 5/1/06  
				unitName, roleName);                     
		switch (parmCount)
		{
		case 4:						// CW 5/1/06
			roleNbr = ams_lookup_role_nbr(me, roleName);
			if (roleNbr < 0)
			{
				writeMemo("Unknown role.");
				continue;
			}

		case 3:						// CW 5/1/06
			if (strcmp(unitName, "_") != 0)
			{

				unitNbr = ams_lookup_unit_nbr(me, unitName);
				if (unitNbr < 0)
				{
					writeMemo("Unknown unit.");
					continue;
				}
			}

		case 2:  
			if (strcmp(cntName, "_") != 0)		// CW, 5/19/06
			{
				cntNbr = ams_lookup_continuum_nbr(me, cntName);
				if (cntNbr < 0)
				{
					writeMemo("Unknown Continuum.");
					continue;
				}
			}

			/*	Intentional fall-through to next case.	*/

		case 1:
			subjectNbr = ams_lookup_subject_nbr(me, subjectName);
			if (subjectNbr < 0)
			{
				writeMemo("Unknown subject.");
				continue;
			}

			if (subjectNbr == 0)	/*	All subjects.	*/
			{
				if (cntNbr == 0)
				{
					/*	Use different default,
						because subscription to
						"everything" is valid
						only within the local
						continuum.		*/

					cntNbr = THIS_CONTINUUM;
				}

				if (cntNbr != THIS_CONTINUUM)
				{
					writeMemo("Unknown subject.");
					continue;
				}
			}

			break;

		default:
			writeMemo("Null command ignored.");
			continue;
		}

		if (asserting)
		{
			if (*mode == 's')
			{
				if (ams_subscribe(me, roleNbr,
						cntNbr,	// CW, 5/1/06
						unitNbr, subjectNbr,
						8, 0, AmsArrivalOrder,
						AmsBestEffort) < 0)
				{
					writeMemo("Can't subscribe to this \
subject.");
					continue;
				}
			}
			else
			{
				if (ams_invite(me, roleNbr,
						cntNbr,
						unitNbr, subjectNbr,
						8, 0, AmsArrivalOrder,
						AmsBestEffort) < 0)
				{
					writeMemo("Can't invite this \
subject.");
					continue;
				}
			}
		}
		else
		{
			if (*mode == 's')
			{
				if (ams_unsubscribe(me, roleNbr,
						cntNbr, // CW 5/1/06
						unitNbr, subjectNbr) < 0)
				{
					writeMemo("Can't unsubscribe to this \
subject.");
					continue;
				}
			}
			else
			{
				if (ams_disinvite(me, roleNbr,
						cntNbr, // CW 5/1/06
						unitNbr, subjectNbr) < 0)
				{
					writeMemo("Can't disinvite this \
subject.");
					continue;
				}
			}
		}
	}

	ams_unregister(me);
	writeErrmsgMemos();
	return 0;
}
