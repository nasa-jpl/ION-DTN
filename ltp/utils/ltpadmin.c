/*

	ltpadmin.c:	LTP engine adminstration interface.

									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "ltpP.h"
#include "ion.h"

#ifdef STRSOE
#include <strsoe_ltpadmin.h>
#endif

static int		_echo(int *newValue)
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
			"Syntax error at line %d of ltpadmin.c", lineNbr);
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
	PUTS("\t   a span <engine ID#> <max export sessions> \
<max import sessions> <max segment size> <aggregation size limit> \
<aggregation time limit> '<LSO command>' [queuing latency, in seconds]");
	PUTS("\t\tIf queuing latency is negative, the absolute value of this \
number is used as the actual queuing latency and session purging is enabled.  \
See man(5) for ltprc.");
	PUTS("\tc\tChange");
	PUTS("\t   c span <engine ID#> <max export sessions> \
<max import sessions> <max segment size> <aggregation size limit> \
<aggregation time limit> '<LSO command>' [queuing latency, in seconds]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} span <engine ID#>");
	PUTS("\tl\tList");
	PUTS("\t   l span");
	PUTS("\tm\tManage");
	PUTS("\t   m heapmax <max database heap for any single inbound block>");
	PUTS("\t   m screening {y | on | n | off}");
	PUTS("\t   m ownqtime <own queuing latency, in seconds>");
	PUTS("\t   m maxber <max expected bit error rate; default is .000001>");
	PUTS("\t   m maxbacklog <max block delivery backlog; default is 10>");
	PUTS("\ts\tStart");
	PUTS("\t   s '<LSI command>'");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\tw\tWatch LTP activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., df{].  See man(5) for ltprc.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeLtp(int tokenCount, char **tokens)
{
	unsigned int	estMaxNbrOfSessions;

	/*	For backward compatibility, if second argument is
	 *	provided it is simply ignored.				*/

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
		putErrmsg("ltpadmin can't attach to ION.", NULL);
		return;
	}

	if (ltpInit(estMaxNbrOfSessions) < 0)
	{
		putErrmsg("ltpadmin can't initialize LTP.", NULL);
		return;
	}
}

static int	attachToLtp()
{
	if (ltpAttach() < 0)
	{
		printText("LTP not initialized yet.");
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
		/*	Temporary patch for backward compatibility
		 *	in configuration files.  Ditch tokens [4] and
		 *	[6], which were maximum block sizes.		*/

		if (tokenCount > 10)
		{
			tokens[4] = tokens[5];
			tokens[5] = tokens[7];
			tokens[6] = tokens[8];
			tokens[7] = tokens[9];
			tokens[8] = tokens[10];
			if (tokenCount > 11)
			{
				tokens[9] = tokens[11];
			}

			tokenCount -= 2;
		}

		/*	End of temporary patch.				*/

		switch (tokenCount)
		{
		case 10:
			qTime = strtol(tokens[9], NULL, 0);
			if (qTime < 0)
			{
				purge = 1;
				qTime = 0 - qTime;
			}

			/*	Intentional fall-through to next case.	*/

		case 9:
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		engineId = strtouvast(tokens[2]);
		oK(addSpan(engineId, strtol(tokens[3], NULL, 0),
				strtol(tokens[4], NULL, 0),
				strtol(tokens[5], NULL, 0),
				strtol(tokens[6], NULL, 0),
				strtol(tokens[7], NULL, 0),
				tokens[8], (unsigned int) qTime, purge));
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
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		/*	Temporary patch for backward compatibility
		 *	in configuration files.  Ditch tokens [4] and
		 *	[6], which were maximum block sizes.		*/

		if (tokenCount > 10)
		{
			tokens[4] = tokens[5];
			tokens[5] = tokens[7];
			tokens[6] = tokens[8];
			tokens[7] = tokens[9];
			tokens[8] = tokens[10];
			if (tokenCount > 11)
			{
				tokens[9] = tokens[11];
			}

			tokenCount -= 2;
		}

		/*	End of temporary patch.				*/

		switch (tokenCount)
		{
		case 10:
			qTime = strtol(tokens[9], NULL, 0);
			if (qTime < 0)
			{
				purge = 1;
				qTime = 0 - qTime;
			}

			/*	Intentional fall-through to next case.	*/

		case 9:
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		engineId = strtouvast(tokens[2]);
		oK(updateSpan(engineId, strtol(tokens[3], NULL, 0),
				strtol(tokens[4], NULL, 0),
				strtol(tokens[5], NULL, 0),
				strtol(tokens[6], NULL, 0),
				strtol(tokens[7], NULL, 0),
				tokens[8], (unsigned int) qTime, purge));
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
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		engineId = strtouvast(tokens[2]);
		removeSpan(engineId);
		return;
	}

	SYNTAX_ERROR;
}

static void	printSpan(LtpVspan *vspan)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpSpan, span);
	char	cmd[SDRSTRING_BUFSZ];
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr, vspan->spanElt));
	sdr_string_read(sdr, cmd, span->lsoCmd);
	isprintf(buffer, sizeof buffer,
			UVAST_FIELDSPEC "  pid: %d  cmd: %.128s",
			vspan->engineId, vspan->lsoPid, cmd);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\tmax export sessions: %u",
			span->maxExportSessions);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\tmax import sessions: %u",
			span->maxImportSessions);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\taggregation size limit: %u  \
aggregation time limit: %u", span->aggrSizeLimit, span->aggrTimeLimit);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\tmax segment size: %u  queuing \
latency: %u  purge: %d", span->maxSegmentSize, span->remoteQtime, span->purge);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\towltOutbound: %u  localXmit: %u  \
owltInbound: %u  remoteXmit: %u", vspan->owltOutbound, vspan->localXmitRate,
			vspan->owltInbound, vspan->remoteXmitRate);
	sdr_exit_xn(sdr);
	printText(buffer);
}

static void	infoSpan(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	uvast		engineId;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSpan(engineId, &vspan, &vspanElt);
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
	LtpVdb		*vdb = getLtpVdb();
	PsmPartition	ionwm = getIonwm();
	Object		ltpdbObj = getLtpDbObject();
			OBJ_POINTER(LtpDB, ltpdb);
	char		buffer[128];
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, LtpDB, ltpdb, ltpdbObj);
	isprintf(buffer, sizeof buffer,"(Engine " UVAST_FIELDSPEC "  Queuing \
latency: %u  LSI pid: %d)", ltpdb->ownEngineId, ltpdb->ownQtime, vdb->lsiPid);
	printText(buffer);
	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
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

static void	manageHeapmax(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		ltpdbObj = getLtpDbObject();
	LtpDB		ltpdb;
	unsigned int	heapmax;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	heapmax = strtoul(tokens[2], NULL, 0);
	if (heapmax < 560)
	{
		writeMemoNote("[?] heapmax must be at least 560", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.maxAcqInHeap = heapmax;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxAcqInHeap.", NULL);
	}
}

static void	manageScreening(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	ltpdbobj = getLtpDbObject();
	LtpDB	ltpdb;
	int	newEnforceSchedule;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	switch (*(tokens[2]))
	{
	case 'Y':
	case 'y':
	case '1':
		newEnforceSchedule = 1;
		break;

	case 'N':
	case 'n':
	case '0':
		newEnforceSchedule = 0;
		break;

	default:
		if (strncmp(tokens[2], "on", 2) == 0)
		{
			newEnforceSchedule = 1;
			break;
		}

		if (strncmp(tokens[2], "off", 3) == 0)
		{
			newEnforceSchedule = 0;
			break;
		}

		writeMemoNote("Screening must be 'y' or 'n'.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbobj, sizeof(LtpDB));
	ltpdb.enforceSchedule = newEnforceSchedule;
	sdr_write(sdr, ltpdbobj, (char *) &ltpdb, sizeof(LtpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change LTP screening control.", NULL);
	}
}

static void	manageOwnqtime(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	ltpdbObj = getLtpDbObject();
	LtpDB	ltpdb;
	int	newOwnQtime;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newOwnQtime = strtol(tokens[2], NULL, 0);
	if (newOwnQtime < 0)
	{
		writeMemoNote("Own Q time invalid", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.ownQtime = newOwnQtime;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change own LTP queuing time.", NULL);
	}
}

static void	manageMaxBER(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	double		newMaxBER;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxBER = atof(tokens[2]);
	if (newMaxBER < 0.0)
	{
		writeMemoNote("Max BER invalid", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.maxBER = newMaxBER;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		computeRetransmissionLimits(vspan);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maximum bit error rate.", NULL);
	}
}

static void	manageMaxBacklog(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		ltpdbObj = getLtpDbObject();
	LtpDB		ltpdb;
	unsigned int	maxBacklog;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	maxBacklog = strtoul(tokens[2], NULL, 0);
	if (maxBacklog < 2)
	{
		writeMemoNote("[?] maxbacklog must be at least 2", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.maxBacklog = maxBacklog;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxBacklog.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "heapmax") == 0)
	{
		manageHeapmax(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "screening") == 0)
	{
		manageScreening(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "ownqtime") == 0)
	{
		manageOwnqtime(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxber") == 0)
	{
		manageMaxBER(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxbacklog") == 0)
	{
		manageMaxBacklog(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	switchWatch(int tokenCount, char **tokens)
{
	LtpVdb	*vdb = getLtpVdb();
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

		case 'f':
			vdb->watching |= WATCH_f;
			break;

		case 'g':
			vdb->watching |= WATCH_g;
			break;

		case 'h':
			vdb->watching |= WATCH_h;
			break;

		case 's':
			vdb->watching |= WATCH_s;
			break;

		case 't':
			vdb->watching |= WATCH_t;
			break;

		case '@':
			vdb->watching |= WATCH_nak;
			break;

		case '{':
			vdb->watching |= WATCH_CS;
			break;

		case '}':
			vdb->watching |= WATCH_handleCS;
			break;

		case '[':
			vdb->watching |= WATCH_CR;
			break;

		case ']':
			vdb->watching |= WATCH_handleCR;
			break;

		case '=':
			vdb->watching |= WATCH_resendCP;
			break;

		case '+':
			vdb->watching |= WATCH_resendRS;
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

static int ltp_is_up(int count, int max)
{
	while (count <= max && !ltp_engine_is_started())
	{
		microsnooze(250000);
		count++;
	}

	if (count > max)		//ltp engine is not started
	{
		printText("LTP engine is not started");
		return 0;
	}

	//ltp engine is started

	printText("LTP engine is started");
	return 1;
}

static int	processLine(char *line, int lineLength, int *checkNeeded,
			int *rc)
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
			initializeLtp(tokenCount, tokens);
			*checkNeeded = 1;
			return 0;

		case 's':
			if (attachToLtp() == 0)
			{
				if (tokenCount < 2)
				{
					printText("Can't start LTP: no LSI \
command.");
					return 0;
				}
				else
				{
					if (ltpStart(tokens[1]) < 0)
					{
						putErrmsg("Can't start LTP.",
								NULL);
						return 0;
					}
				}

				/* Wait for ltp to start up */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (ltp_engine_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
					    done_time.tv_sec 
					    && cur_time.tv_usec >=
					    done_time.tv_usec)
					{
						printText("[?] LTP start hung \
up, abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (attachToLtp() == 0)
			{
				ltpStop();
			}

			return 0;

		case 'a':
			if (attachToLtp() == 0)
			{
				executeAdd(tokenCount, tokens);
				*checkNeeded = 1;
			}

			return 0;

		case 'c':
			if (attachToLtp() == 0)
			{
				executeChange(tokenCount, tokens);
				*checkNeeded = 1;
			}

			return 0;

		case 'd':
			if (attachToLtp() == 0)
			{
				executeDelete(tokenCount, tokens);
				*checkNeeded = 1;
			}

			return 0;

		case 'i':
			if (attachToLtp() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToLtp() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'm':
			if (attachToLtp() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'w':
			if (attachToLtp() == 0)
			{
				switchWatch(tokenCount, tokens);
			}

			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 't':
			if (tokenCount > 1
			&& strcmp(tokens[1], "p") == 0)	//poll
			{
				if (tokenCount < 3)	//use default timeout
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
			while (count <= max && attachToLtp() == -1)
			{
				microsnooze(250000);
				count++;
			}

			if (count > max)
			{
				//ltp engine is not started
				printText("LTP engine is not started");
				return 1;
			}

			//attached to ltp system
				
			*rc = ltp_is_up(count, max);
			return 1;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	ltpadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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
	int	checkNeeded = 0;

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

			if (processLine(line, len, &checkNeeded, &rc))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		if (attachToLtp() == 0)
		{
			ltpStop();
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

				if (processLine(line, len, &checkNeeded, &rc))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	if (checkNeeded)
	{
		if (attachToLtp() == 0)
		{
			checkReservationLimit();
		}
	}

	printText("Stopping ltpadmin.");
	ionDetach();
	return rc;
}

#ifdef STRSOE
int	ltpadmin_processLine(char *line, int lineLength, int *checkNeeded,
		int *rc)
{
	return processLine(line, lineLength, checkNeeded, rc);
}

void	ltpadmin_help(void)
{
	printUsage();
}
#endif
