/*

	dtpcadmin:	DTPC protocol administration interface.


	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.

									*/

#include "dtpcP.h"

static int	_echo(int *newValue)
{
	static int	state = 0;

	if (newValue)
	{
		if (*newValue == 1)
		{
			state = 1;
		}
		else
		{
			state = 0;
		}
	}

	return state;
}

static void	printText(char *text)
{
	if(_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	handleQuit(int signum)
{
	printText("Please enter command 'q' to stop the program.");
}

static void     printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer,
			"Syntax error at line %d of dtpcadmin.c", lineNbr);
	printText(buffer);
}

#define SYNTAX_ERROR	printSyntaxError(__LINE__)

static void     printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint ION version number.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
        PUTS("\t   a profile <profileID> <maxRtx> <aggrSizeLimit> \
<aggrTimeLimit> <ttl> <custody>.<priority>[.<ordinal>\
[.<unreliable>.<critical>[.<flow_label>]]] <report-to-endpoint> \
[<status_report_flags>]");
	
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} profile <profileID>");
	PUTS("\tl\tList");
	PUTS("\t   l profile");
	PUTS("\ts\tStart");
        PUTS("\tx\tStop");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
        PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., pq.  See man(5) for dtpcrc.");
        PUTS("\te\tEnable or disable echo of printed output to log file");
        PUTS("\t   e { 0 | 1 }");
        PUTS("\t#\tComment");
        PUTS("\t   # <comment text>");
}

static void	initializeDtpc(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (dtpcInit() < 0)
	{
		putErrmsg("dtpcadmin can't initialize DTPC.", NULL);
	}
}

static int	attachToDtpc()
{
	if (dtpcAttach() < 0)
	{
		printText("DTPC not initialized yet.");
		return -1;
	}

	return 0;
}

static void	printProfile(Profile *vprofile)
{
	Sdr	sdr = getIonsdr();
	char	buffer[256];
	char	sdrBuf[SDRSTRING_BUFSZ];
	
	sdr_string_read(sdr, sdrBuf, vprofile->reportToEid);
	
	isprintf(buffer, sizeof buffer, "profileID: %u maxRtx: %u lifespan: %u",
			vprofile->profileID, vprofile->maxRtx,
			vprofile->lifespan);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "aggrSizeLimit: %u, aggTimeLimit: \
%u Priority: %d Custody: %d", vprofile->aggrSizeLimit, vprofile->aggrTimeLimit,
			vprofile->classOfService, vprofile->custodySwitch);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "reportToEid: %s", sdrBuf);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "Ordinal: %d Unreliable: %d  Critical: \
%d", vprofile->ancillaryData.ordinal, 
		vprofile->ancillaryData.flags & BP_BEST_EFFORT ? 1 : 0,
		vprofile->ancillaryData.flags & BP_MINIMUM_LATENCY ? 1 : 0);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "rcvReport: %d ctReport: %d fwdReport: \
%d dlvReport: %d delReport: %d", vprofile->srrFlags & BP_RECEIVED_RPT ? 1 : 0,
		vprofile->srrFlags & BP_CUSTODY_RPT? 1 : 0,
		vprofile->srrFlags & BP_FORWARDED_RPT? 1 : 0,
		vprofile->srrFlags & BP_DELIVERED_RPT? 1 : 0,
		vprofile->srrFlags & BP_DELETED_RPT? 1 : 0);
	printText(buffer);
	return;
}

static void	infoProfile(int tokenCount, char **tokens)
{
	DtpcVdb		*vdb = getDtpcVdb();
	Profile		*vprofile;
	PsmAddress	elt;
	PsmPartition	wm = getIonwm();
	unsigned int	profileID;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}
	
	profileID = atoi(tokens[2]);
	for (elt = sm_list_first(wm, vdb->profiles); elt;
			elt = sm_list_next(wm, elt))
	{
		vprofile = (Profile *) psp(wm, sm_list_data(wm, elt));
		if (vprofile->profileID == profileID)
		{
			break;
		}
	}

	if (elt == 0)
	{
		printText("Unknown profile.");
		return;
	}

	printProfile(vprofile);
}

static void	executeAdd(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "profile") == 0)
	{
		if (tokenCount != 10 && tokenCount != 9)
		{
			SYNTAX_ERROR;
			return;
		}

		oK(addProfile(strtol(tokens[2], NULL, 0),
				strtol(tokens[3], NULL, 0),
				strtol(tokens[4], NULL, 0),
				strtol(tokens[5], NULL, 0),
				strtol(tokens[6], NULL, 0),
				tokens[7], tokens[8], tokens[9]));
		return; 
	}
	
	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "profile") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		oK(removeProfile(strtol(tokens[2], NULL, 0)));
		return;
	}
	
	SYNTAX_ERROR;
}	

static void	executeInfo(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (strcmp(tokens[1], "profile") == 0)
	{
		infoProfile(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listProfiles(int tokenCount, char **tokens)
{
	PsmPartition	wm = getIonwm();
	PsmAddress	elt;
	Profile		*vprofile;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}	

	for (elt = sm_list_first(wm, (getDtpcVdb())->profiles); elt;
			elt = sm_list_next(wm, elt))
	{
		vprofile = (Profile *) psp(wm, sm_list_data(wm, elt));
		printProfile(vprofile);
	}
}

static void	executeList(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "profile") == 0)
	{
		listProfiles(tokenCount, tokens);
		return; 
	}

	SYNTAX_ERROR;
}

static void	switchWatch(int tokenCount, char **tokens)
{
	DtpcVdb		*vdb = getDtpcVdb();
	char		buffer[80];
	char		*cursor;

	if (tokenCount < 2)
	{
		printText("Switch watch in what way?");
		return;
	}

	if (strcmp(tokens[1], "1") == 0)
	{
		vdb->watching = -1;
		return;
	}

	vdb->watching = 0;
	if (strcmp(tokens[1], "0") == 0)
	{
		return;
	}

	cursor = tokens[1];
	while (*cursor)
	{
		switch (*cursor)
		{
		case 'o':
			vdb->watching |= WATCH_o;
			break;

		case '<':
			vdb->watching |= WATCH_newItem;
			break;

		case 'r':
			vdb->watching |= WATCH_r;
			break;

		case '>':
			vdb->watching |= WATCH_complete;
			break;

		case '-':
			vdb->watching |= WATCH_send;
			break;

		case 'l':
			vdb->watching |= WATCH_l;
			break;

		case 'm':
			vdb->watching |= WATCH_m;
			break;

		case 'n':
			vdb->watching |= WATCH_n;
			break;

		case 'i':
			vdb->watching |= WATCH_i;
			break;
		
		case 'u':
			vdb->watching |= WATCH_u;
			break;

		case 'v':
			vdb->watching |= WATCH_v;
			break;

		case '?':
			vdb->watching |= WATCH_discard;
			break;

		case '*':
			vdb->watching |= WATCH_expire;
			break;

		case '$':
			vdb->watching |= WATCH_reset;
			break;

		default:
			isprintf(buffer, sizeof buffer,
					"Invalid watch char %c.", *cursor);
			printText(buffer);
		}

		cursor++;
	}
}

static void 	switchEcho(int tokenCount, char **tokens)
{
	int     state;

	if (tokenCount < 2)
        {
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		state = 0;
		oK(_echo(&state));
		break;

	case '1':
		state = 1;
		oK(_echo(&state));
		break;

	default:
		printText("Echo on or off?");
	}
}

static int dtpc_is_up(int count, int max)
{
	while (count <= max && !dtpc_entity_is_started())
	{
		microsnooze(250000);
		count++;
	}

	if (count > max)		//dtpc entity is not started
	{
		printText("DTPC entity is not started");
		return 0;
	}

	//dtpc entity is started

	printText("DTPC entity is started");
	return 1;
}

static int	processLine(char *line, int lineLength, int *rc)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[10];
	char		buffer[80];
	struct timeval	done_time;
	struct timeval	cur_time;

	int max = 0;
	int count = 0;

	tokenCount = 0;
	for (cursor = line, i = 0; i < 10; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			if (tokens[i])
			{
				tokenCount++;
			}
		}
	}

	if (tokenCount == 0)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		printText("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;

		case 'v':
			isprintf(buffer, sizeof buffer, "%s", IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			initializeDtpc(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToDtpc() == 0)
			{
				if (tokenCount > 1)
				{
					printText("Too many tokens for start \
command.");
				}
				else
				{
					if (dtpcStart() < 0)
					{
						putErrmsg("Can't start DTPC.",
								NULL);
						return 0;
					}
				}

				/*	Wait for dtpc to start up.	*/

				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (dtpc_entity_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >= done_time.tv_sec 
					&& cur_time.tv_usec >=
							done_time.tv_usec)
					{
						printText("[?] DTPC start hung \
up, abandoned.");
						break;
					}
				}
			}

			return 0;

		case 'x':
			if (attachToDtpc() == 0)
			{
				if (tokenCount > 1)
				{
					printText("Too many tokens for stop \
command.");
				}
				else
				{
					dtpcStop();
				}
			}

			return 0;

		case 'a':
			if (attachToDtpc() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToDtpc() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToDtpc() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToDtpc() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'w':
			if (attachToDtpc() == 0)
			{
				switchWatch(tokenCount, tokens);
			}

			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 't':
			if (tokenCount > 1
			&& strcmp(tokens[1], "p") == 0) //poll
			{
				if (tokenCount < 3) //use default timeout
				{
					max = DEFAULT_CHECK_TIMEOUT;
				}
				else
				{
					max = atoi(tokens[2]) * 4;
				}
			}
			else
			{
				max = 1;
			}

			count = 1;
			while (count <= max && attachToDtpc() == -1)
			{
				microsnooze(250000);
				count++;
			}

			if (count > max)
			{
				//dtpc entity is not started
				printText("DTPC entity is not started");
				return 1;
			}

			//attached to dtpc system

			*rc = dtpc_is_up(count, max);
			return 1;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	dtpcadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	rc = 0;
	int	cmdFile;
	char	line[512];
	int	len;
	
	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdout.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0)
				{
					break;
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Out of loop.	*/
			}
			
			if (len == 0)
			{
				continue;
			}

			if (processLine(line, len, &rc))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		if (attachToDtpc() == 0)
		{
			dtpcStop();
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = open(cmdFileName, O_RDONLY, 0777);
		if (cmdFile < 0)
		{
			PERROR("Can't open command file");
		}
		else
		{
			while (1)
			{
				if (igets(cmdFile, line, sizeof line, &len)
						== NULL)
				{
					if (len == 0)
					{
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line, len, &rc))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping dtpcadmin.");
	ionDetach();
	return rc;
}
