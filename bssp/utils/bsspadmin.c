/*
 *	bsspadmin.c:	BSSP engine adminstration interface.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *		 Scott Burleigh, JPL
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 */

#include "bsspP.h"
#include "ion.h"

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
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	handleQuit(int signum)
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer,
			"Syntax error at line %d of bsspadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1 <est. max number of sessions>");
	PUTS("\ta\tAdd");
	PUTS("\t   a span <engine ID#> <max number of sessions> <max-block-\
size> '<BE-BSO command>' '<RL-BSO command>' [queuing latency, in seconds]");
	PUTS("\t\tIf queuing latency is negative, the absolute value of this \
number is used as the actual queuing latency and session purging is enabled.  \
See man(5) for bssprc.");
	PUTS("\tc\tChange");
	PUTS("\t   c span <engine ID#> <max export sessions> <max-block-size> \
'<BE-BSO command>' '<RL-BSO command>' [queuing latency, in seconds]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} span <engine ID#>");
	PUTS("\tl\tList");
	PUTS("\t   l span");
	PUTS("\tm\tManage");
	PUTS("\t   m ownqtime <own queuing latency, in seconds>");
	PUTS("\ts\tStart");
	PUTS("\t   s '<BE-BSI command>' '<RL-BSI command>'");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\tw\tWatch BSSP activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., df{].  See man(5) for bssprc.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeBssp(int tokenCount, char **tokens)
{
	unsigned int	estMaxNbrOfSessions;
	
	if (tokenCount == 3)
	{
		tokenCount = 2;
	}

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}
	estMaxNbrOfSessions = strtol(tokens[1], NULL, 0);
	if (ionAttach() < 0)
	{
		putErrmsg("bssppadmin can't attach to ION.", NULL);
		return;
	}
	if (bsspInit(estMaxNbrOfSessions) < 0)
	{
		putErrmsg("bssppadmin can't initialize BSSP.", NULL);
		return;
	}
}

static int	attachToBssp()
{
	if (bsspAttach() < 0)
	{
		printText("BSSP not initialized yet.");
		return -1;
	}
	return 0;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	uvast	engineId;
	int	qTime = 1;			/*	Default.	*/
	int	purge = 0;			/*	Default.	*/
	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{

		switch (tokenCount)
			{
			case 8:
				qTime = strtol(tokens[7], NULL, 0);
				if (qTime < 0)
				{
					purge = 1;
					qTime = 0 - qTime;
				}

			/*	Intentional fall-through to next case.	*/

			case 7:
				break;

			default:
				SYNTAX_ERROR;
				return;
			}

			engineId = strtouvast(tokens[2]);
			oK(addBsspSpan(engineId, strtol(tokens[3], NULL, 0),
				strtol(tokens[4], NULL, 0), tokens[5],
				tokens[6], (unsigned int) qTime, purge));
			return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
		uvast	engineId;
	int	qTime = 1;			/*	Default.	*/
	int	purge = 0;			/*	Default.	*/

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{

		switch (tokenCount)
			{
			case 8:
				qTime = strtol(tokens[7], NULL, 0);
				if (qTime < 0)
				{
					purge = 1;
					qTime = 0 - qTime;
				}

			/*	Intentional fall-through to next case.	*/

			case 7:
				break;

			default:
				SYNTAX_ERROR;
				return;
			}

			engineId = strtouvast(tokens[2]);
			oK(updateBsspSpan(engineId, strtol(tokens[3], NULL, 0),
				strtol(tokens[4], NULL, 0), tokens[5], 
				tokens[6], (unsigned int) qTime, purge));
			return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	uvast	engineId;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		engineId = strtouvast(tokens[2]);
		removeBsspSpan(engineId);
		return;
	}

	SYNTAX_ERROR;
}

static void	printSpan(BsspVspan *vspan)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BsspSpan, span);
	char	cmd[SDRSTRING_BUFSZ];
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, BsspSpan, span,
			sdr_list_data(sdr, vspan->spanElt));
	sdr_string_read(sdr, cmd, span->bsoBECmd);
	isprintf(buffer, sizeof buffer,
			UVAST_FIELDSPEC "  pid: %d  cmd: %.128s",
			vspan->engineId, vspan->bsoBEPid, cmd);
	printText(buffer);

	sdr_string_read(sdr, cmd, span->bsoRLCmd);
	isprintf(buffer, sizeof buffer,
			UVAST_FIELDSPEC "  pid: %d  cmd: %.128s",
			vspan->engineId, vspan->bsoRLPid, cmd);
	printText(buffer);

	isprintf(buffer, sizeof buffer, "\tmax export sessions: %u",
			span->maxExportSessions);
	printText(buffer);

	isprintf(buffer, sizeof buffer, "\tmax block size: %u  queuing \
			latency: %u  purge: %d", span->maxBlockSize, 
			span->remoteQtime, span->purge);
	printText(buffer);

	isprintf(buffer, sizeof buffer, "\towltOutbound: %u  localXmit: %u  \
			owltInbound: %u  remoteXmit: %u", vspan->owltOutbound, 
			vspan->localXmitRate, vspan->owltInbound,
			vspan->remoteXmitRate);
	sdr_exit_xn(sdr);
	printText(buffer);
}

static void	infoSpan(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	uvast		engineId;
	BsspVspan	*vspan;
	PsmAddress	vspanElt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findBsspSpan(engineId, &vspan, &vspanElt);
	sdr_exit_xn(sdr);
	if (vspanElt == 0)
	{
		printText("Unknown span.");
		return;
	}

	printSpan(vspan);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		infoSpan(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listSpans(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	BsspVdb		*vdb = getBsspVdb();
	PsmPartition	ionwm = getIonwm();
	Object		bsspdbObj = getBsspDbObject();
			OBJ_POINTER(BsspDB, bsspdb);
	char		buffer[128];
	PsmAddress	elt;
	BsspVspan	*vspan;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, BsspDB, bsspdb, bsspdbObj);
	isprintf(buffer, sizeof buffer,"(Engine " UVAST_FIELDSPEC "  BE-BSI \
pid: %d)", bsspdb->ownEngineId, vdb->beBsiPid);
	printText(buffer);
	isprintf(buffer, sizeof buffer,"(Engine " UVAST_FIELDSPEC "  RL-BSI \
pid: %d)", bsspdb->ownEngineId, vdb->rlBsiPid);
	printText(buffer);
	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (BsspVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		printSpan(vspan);
	}

	sdr_exit_xn(sdr);
}

static void	executeList(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		listSpans(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	manageOwnqtime(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	bsspdbObj = getBsspDbObject();
	BsspDB	bsspdb;
	int	newOwnQtime;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newOwnQtime = strtol(tokens[2], NULL, 0);
	if (newOwnQtime < 0)
	{
		putErrmsg("Own Q time invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bsspdb, bsspdbObj, sizeof(BsspDB));
	bsspdb.ownQtime = newOwnQtime;
	sdr_write(sdr, bsspdbObj, (char *) &bsspdb, sizeof(BsspDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change own BSSP queuing time.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "ownqtime") == 0)
	{
		manageOwnqtime(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	switchWatch(int tokenCount, char **tokens)
{
	BsspVdb	*vdb = getBsspVdb();
	char	buffer[80];
	char	*cursor;

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
		case 'd':
			vdb->watching |= WATCH_d;
			break;

		case 'e':
			vdb->watching |= WATCH_e;
			break;

		case 'g':
			vdb->watching |= WATCH_g;
			break;

		case 's':
			vdb->watching |= WATCH_s;
			break;

		case '=':
			vdb->watching |= WATCH_resendBlk;
			break;

		default:
			isprintf(buffer, sizeof buffer,
					"Invalid watch char %c.", *cursor);
			printText(buffer);
		}

		cursor++;
	}
}

static void	switchEcho(int tokenCount, char **tokens)
{
	int	state;

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

static int bssp_is_up(int count, int max)
{
	while (count <= max && !bssp_engine_is_started())
	{
		microsnooze(250000);
		count++;
	}

	if (count > max)		//bssp engine is not started
	{
		printText("BSSP engine is not started");
		return 0;
	}

	//bssp engine is started

	printText("BSSP engine is started");
	return 1;
}

static int	processLine(char *line, int lineLength, int *rc)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[12];
	char		buffer[80];
	struct timeval	done_time;
	struct timeval	cur_time;
	int		max = 0;
	int		count = 0;

	tokenCount = 0;
	for (cursor = line, i = 0; i < 12; i++)
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
			isprintf(buffer, sizeof buffer, "%s",
					IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			initializeBssp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToBssp() == 0)
			{
				if (tokenCount < 3)
				{
					printText("Can't start BSSP: no \
best-effort or reliable BSI command.");
				}
				else
				{
					if (bsspStart(tokens[1], tokens[2]) < 0)
					{
						putErrmsg("Can't start BSSP.",
								NULL);
					}
				}
				/* Wait for bssp to start up */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (bssp_engine_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
					    done_time.tv_sec 
					    && cur_time.tv_usec >=
					    done_time.tv_usec)
					{
						printText("[?] BSSP start hung\
 up, abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (attachToBssp() == 0)
			{
				bsspStop();
			}

			return 0;

		case 'a':
			if (attachToBssp() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attachToBssp() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToBssp() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToBssp() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToBssp() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'm':
			if (attachToBssp() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'w':
			if (attachToBssp() == 0)
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
				if (tokenCount < 3)	//use default timeout
				{
					max = DEFAULT_CHECK_TIMEOUT;
				}
				else
				{
					max = atoi(tokens[2]) * 4;
				}

				count = 1;
				while (count <= max && attachToBssp() == -1)
				{
					microsnooze(250000);
					count++;
				}

				if (count > max)
				{
					//bssp engine is not started
					printText("BSSP engine is not started");
					return 1;
				}

				//attached to bssp system

				*rc = bssp_is_up(count, max);
				return 1;
			}

			//check once

			*rc = bssp_engine_is_started();
			if (*rc)
			{
				printText("BSSP engine is started");
			}
			else
			{
				printText("BSSP engine is not started");
			}

			return 1;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	bsspadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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
	char	line[256];
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
		if (attachToBssp() == 0)
		{
			bsspStop();
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
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
	printText("Stopping bsspadmin.");
	ionDetach();
	return rc;
}
