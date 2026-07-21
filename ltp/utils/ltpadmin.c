/*

	ltpadmin.c:	LTP engine adminstration interface.

									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "ltpP.h"
#include "ion.h"

#ifdef INPUT_HISTORY
#include "linenoise.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#endif

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

#ifndef NON_INTERACTIVE
static void	handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	printText("Please enter command 'q' to stop the program.");
}
#endif

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer,
			"Syntax error at line %d of ltpadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage(void)
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1 <est. max number of sessions>");
	PUTS("\ta\tAdd");
	PUTS("\t   a span <engine ID> <max export sessions> \
<max import sessions> <max segment size> <aggregation size threshold> \
<aggregation time limit> '<LSO command>' [queuing latency, in seconds]");
	PUTS("\t\tIf queuing latency is negative, the absolute value of this \
number is used as the actual queuing latency and session purging is enabled.  \
See man(5) for ltprc.");
	PUTS("\t   a seat '<LSI command>'");
	PUTS("\tc\tChange");
	PUTS("\t   c span <engine ID> <max export sessions> \
<max import sessions> <max segment size> <aggregation size threshold> \
<aggregation time limit> '<LSO command>' [queuing latency, in seconds]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} span <engine ID>");
	PUTS("\t   {d|i} seat '<LSI command>'");
	PUTS("\tl\tList");
	PUTS("\t   l span");
	PUTS("\t   l seat");
	PUTS("\tm\tManage");
	PUTS("\t   m heapmax <max database heap for any single inbound block>");
	PUTS("\t   m screening {y | on | n | off}");
	PUTS("\t   m ownqtime <own queuing latency, in seconds>");
	PUTS("\t   m maxber <max expected bit error rate> (DEPRECATED)");
	PUTS("\t   m maxretries <max retransmission attempts; default is 5>");
	PUTS("\t   m maxseglossrate <max segment loss rate; default is 0.01>");
	PUTS("\t   m maxretriesxmit <max retries transmit; default is 5>");
	PUTS("\t   m maxretriesrecv <max retries receive; default is 5>");
	PUTS("\t   m maxseglossratexmit <max seg loss rate xmit; default 0.01>");
	PUTS("\t   m maxseglossraterecv <max seg loss rate recv; default 0.01>");
	PUTS("\t   m span <engine ID> maxretries <value>");
	PUTS("\t   m span <engine ID> maxseglossrate <value>");
	PUTS("\t   m span <engine ID> maxretriesxmit <value>");
	PUTS("\t   m span <engine ID> maxretriesrecv <value>");
	PUTS("\t   m span <engine ID> maxseglossratexmit <value>");
	PUTS("\t   m span <engine ID> maxseglossraterecv <value>");
	PUTS("\t   m span <engine ID> inactivity <seconds>");
	PUTS("\t   m maxbacklog <max block delivery backlog; default is 10>");
	PUTS("\ts\tStart");
	PUTS("\t   s ['<LSI command>']");
	PUTS("\t   s span <engine ID>");
	PUTS("\tt\tStartup-test");
	PUTS("\t   t [p [<timeout>]]");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\t   x span <engine ID>");
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
	uvast		parsed_sessions;
	char		errMsg[256];

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

	if (platform_parse_uvast(tokens[1], &parsed_sessions) < 0
		|| parsed_sessions > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid max sessions: %s", tokens[1]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	estMaxNbrOfSessions = (unsigned int) parsed_sessions;

	if (ionAttach() < 0)
// ...
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

static int	attachToLtp(void)
{
	if (ltpAttach() < 0)
	{
		printText("[?] LTP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	composeLsiCmd(int tokenCount, char **tokens, char *lsiCmd)
{
	switch (tokenCount)
	{
	case 4:
		isprintf(lsiCmd, 256, "%s %s", tokens[2], tokens[3]);
		break;

	case 3:
		istrcpy(lsiCmd, tokens[2], 256);
		break;

	default:
		*lsiCmd = '\0';
	}
}

static void	patchLsiCmd(int tokenCount, char **tokens, char *lsiCmd)
{
	switch (tokenCount)
	{
	case 3:
		isprintf(lsiCmd, 256, "%s %s", tokens[1], tokens[2]);
		break;

	case 2:
		istrcpy(lsiCmd, tokens[1], 256);
		break;

	default:
		*lsiCmd = '\0';
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	char	lsiCmd[256];
	uvast	engineId;
	int	qTime = 1;			/*	Default.	*/
	int	purge = 0;			/*	Default.	*/

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "seat") == 0)
	{
		composeLsiCmd(tokenCount, tokens, lsiCmd);
		if (lsiCmd[0] == '\0')
		{
			SYNTAX_ERROR;
			return;
		}

		oK(addSeat(lsiCmd));
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		/* Hoisted declarations for POSIX/C90 compatibility */
		uvast p_maxExp, p_maxImp, p_maxSeg, p_aggrSize, p_aggrTime;
		char errMsg[256];

		/* Temporary patch for backward compatibility
		* in configuration files.  Ditch tokens [4] and
		* [6], which were maximum block sizes.            */

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

		/* End of temporary patch.                         */

		switch (tokenCount)
		{
		case 10:
			if (platform_parse_int(tokens[9], &qTime) < 0)
			{
				isprintf(errMsg, sizeof(errMsg),
						"[?] Invalid qTime: %s", tokens[9]);
				PUTS(errMsg); writeMemo(errMsg);
				return;
			}

			if (qTime < 0)
			{
				purge = 1;
				qTime = 0 - qTime;
			}

			/* Intentional fall-through to next case.  */

		case 9:
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		engineId = getFqn(tokens[2]);

		if (platform_parse_uvast(tokens[3], &p_maxExp) < 0 || p_maxExp > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid maxExportSessions: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[4], &p_maxImp) < 0 || p_maxImp > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid maxImportSessions: %s", tokens[4]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[5], &p_maxSeg) < 0 || p_maxSeg > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid maxSegmentSize: %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[6], &p_aggrSize) < 0 || p_aggrSize > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid aggrSizeLimit: %s", tokens[6]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[7], &p_aggrTime) < 0 || p_aggrTime > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid aggrTimeLimit: %s", tokens[7]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		oK(addSpan(engineId,
				(unsigned int) p_maxExp,
				(unsigned int) p_maxImp,
				(unsigned int) p_maxSeg,
				(unsigned int) p_aggrSize,
				(unsigned int) p_aggrTime,
				tokens[8],
				(unsigned int) qTime,
				purge));
		return;
	}
}

static void	executeChange(int tokenCount, char **tokens)
{
	uvast	engineId;
	int	qTime = 1;			/* Default.        */
	int	purge = 0;			/* Default.        */

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		/* Hoisted declarations for POSIX/C90 compatibility */
		uvast p_maxExp, p_maxImp, p_maxSeg, p_aggrSize, p_aggrTime;
		char errMsg[256];

		/* Temporary patch for backward compatibility
		* in configuration files.  Ditch tokens [4] and
		* [6], which were maximum block sizes.            */

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

		/* End of temporary patch.                         */

		switch (tokenCount)
		{
		case 10:
			if (platform_parse_int(tokens[9], &qTime) < 0)
			{
				isprintf(errMsg, sizeof(errMsg),
						"[?] Invalid qTime: %s", tokens[9]);
				PUTS(errMsg); writeMemo(errMsg);
				return;
			}

			if (qTime < 0)
			{
				purge = 1;
				qTime = 0 - qTime;
			}

			/* Intentional fall-through to next case.  */

		case 9:
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		engineId = getFqn(tokens[2]);

		if (platform_parse_uvast(tokens[3], &p_maxExp) < 0 || p_maxExp > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid maxExportSessions: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[4], &p_maxImp) < 0 || p_maxImp > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid maxImportSessions: %s", tokens[4]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[5], &p_maxSeg) < 0 || p_maxSeg > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid maxSegmentSize: %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[6], &p_aggrSize) < 0 || p_aggrSize > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid aggrSizeLimit: %s", tokens[6]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[7], &p_aggrTime) < 0 || p_aggrTime > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid aggrTimeLimit: %s", tokens[7]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		oK(updateSpan(engineId,
				(unsigned int) p_maxExp,
				(unsigned int) p_maxImp,
				(unsigned int) p_maxSeg,
				(unsigned int) p_aggrSize,
				(unsigned int) p_aggrTime,
				tokens[8],
				(unsigned int) qTime,
				purge));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	char	lsiCmd[256];
	uvast	engineId;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "seat") == 0)
	{
		composeLsiCmd(tokenCount, tokens, lsiCmd);
		if (lsiCmd[0] == '\0')
		{
			SYNTAX_ERROR;
			return;
		}

		oK(removeSeat(lsiCmd));
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		engineId = getFqn(tokens[2]);
		oK(removeSpan(engineId));
		return;
	}

	SYNTAX_ERROR;
}

static void	printSeat(LtpVseat *vseat)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpSeat, seat);
	char	cmd[SDRSTRING_BUFSZ];
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, LtpSeat, seat, sdr_list_data(sdr, vseat->seatElt));
	sdr_string_read(sdr, cmd, seat->lsiCmd);
	isprintf(buffer, sizeof buffer, "'%.128s' pid: %d", cmd, vseat->lsiPid);
	sdr_exit_xn(sdr);
	printText(buffer);
}

static void	printSpan(LtpVspan *vspan)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpSpan, span);
	char	nbrBuf[FQN_MAX_LENGTH];
	char	cmd[SDRSTRING_BUFSZ];
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));
	GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr, vspan->spanElt));
	putFqn(nbrBuf, vspan->engineId);
	sdr_string_read(sdr, cmd, span->lsoCmd);
	isprintf(buffer, sizeof buffer, "%s pid: %d  cmd: '%.128s'", nbrBuf,
			vspan->lsoPid, cmd);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\tmax export sessions: %u",
			span->maxExportSessions);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\tmax import sessions: %u",
			span->maxImportSessions);
	printText(buffer);
	isprintf(buffer, sizeof buffer, "\taggregation size threshold: %u  \
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

static void	infoSeat(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	char		lsiCmd[256];
	LtpVseat	*vseat;
	PsmAddress	vseatElt;

	composeLsiCmd(tokenCount, tokens, lsiCmd);
	if (lsiCmd[0] == '\0')
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSeat(lsiCmd, &vseat, &vseatElt);
	sdr_exit_xn(sdr);
	if (vseatElt == 0)
	{
		printText("[?] Unknown seat.");
		return;
	}

	printSeat(vseat);
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

	engineId = getFqn(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSpan(engineId, &vspan, &vspanElt);
	sdr_exit_xn(sdr);
	if (vspanElt == 0)
	{
		printText("[?] Unknown span.");
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

	if (strcmp(tokens[1], "seat") == 0)
	{
		infoSeat(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "span") == 0)
	{
		infoSpan(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listSeats(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	LtpVseat	*vseat;

	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	for (elt = sm_list_first(ionwm, vdb->seats); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vseat = (LtpVseat *) psp(ionwm, sm_list_data(ionwm, elt));
		printSeat(vseat);
	}

	sdr_exit_xn(sdr);
}

static void	listSpans(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	PsmPartition	ionwm = getIonwm();
	SdrObject	ltpdbObj = getLtpDbObject();
			OBJ_POINTER(LtpDB, ltpdb);
	char		nbrBuf[FQN_MAX_LENGTH];
	char		buffer[128];
	PsmAddress	elt;
	LtpVspan	*vspan;

	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, LtpDB, ltpdb, ltpdbObj);
	putFqn(nbrBuf, ltpdb->ownEngineId);
	isprintf(buffer, sizeof buffer,"(Engine %s Queuing latency: %u)",
			nbrBuf, ltpdb->ownQtime);
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

	if (strcmp(tokens[1], "seat") == 0)
	{
		listSeats(tokenCount, tokens);
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
	SdrObject	ltpdbObj = getLtpDbObject();
	LtpDB		ltpdb;
	unsigned int	heapmax;
	uvast		parsed_heapmax;
	char		errMsg[256];

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_uvast(tokens[2], &parsed_heapmax) < 0
		|| parsed_heapmax > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid heapmax: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	heapmax = (unsigned int) parsed_heapmax;

	if (heapmax < 560 || heapmax > LTP_MAX_HEAP_LIMIT)
	{
		writeMemoNote("[?] heapmax must be at least 560 and no more than 65536", tokens[2]);
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
	SdrObject ltpdbobj = getLtpDbObject();
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
	SdrObject ltpdbObj = getLtpDbObject();
	LtpDB	ltpdb;
	int	newOwnQtime;
	char	errMsg[256];

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newOwnQtime) < 0)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid own Q time: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}

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
	SdrObject	ltpdbObj = getLtpDbObject();
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

	if (platform_parse_double(tokens[2], &newMaxBER) < 0 || newMaxBER < 0.0)
	{
		writeMemoNote("Max BER invalid", tokens[2]);
		return;
	}

	/*	Deprecation warning.					*/

	writeMemo("[?] Warning: 'maxber' is deprecated.");
	writeMemo("[?] Use 'm maxretries' and 'm maxseglossrate' instead.");

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.maxBER = newMaxBER;
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Switch ALL spans to legacy mode (LAST WRITE WINS).	*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		/*	Log mode switch if applicable.			*/

		if (vspan->useExplicitConfig == 1)
		{
			writeMemo("[i] Switching from explicit to legacy mode");
		}

		vspan->useExplicitConfig = 0;	/*	Legacy mode.	*/
		vspan->hasSpanOverride = 0;	/*	Clear override.	*/
		computeRetransmissionLimits(vspan);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maximum bit error rate.", NULL);
	}
}

static void	manageMaxRetries(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	unsigned int	newMaxRetries;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	uvast	parsed_retries;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[2], &parsed_retries) < 0
		|| parsed_retries > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid maxRetries: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newMaxRetries = (unsigned int) parsed_retries;

	if (newMaxRetries < 3)
	{
		writeMemoNote("[?] maxRetries must be >= 3", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.defaultMaxRetries = newMaxRetries;
	ltpdb.useGlobalSplitMode = 0;		/*	Unified mode.	*/
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Update all spans WITHOUT overrides.			*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		if (vspan->hasSpanOverride == 0)
		{
			if (vspan->useExplicitConfig == 0)
			{
				writeMemo("[i] Switching from legacy to \
explicit mode");
			}

			vspan->useExplicitConfig = 1;
			vspan->useSplitMode = 0;
			vspan->maxRetries = newMaxRetries;
			computeRetransmissionLimits(vspan);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change max retries.", NULL);
	}
}

static void	manageMaxSegLossRate(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	float		newLossRate;
	double		parsed_double;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_double(tokens[2], &parsed_double) < 0
		|| parsed_double < 0.0 || parsed_double >= 1.0)
	{
		writeMemoNote("[?] maxSegmentLossRate must be [0.0, 0.99]",
				tokens[2]);
		return;
	}
	newLossRate = (float) parsed_double;

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.defaultMaxSegLossRate = newLossRate;
	ltpdb.useGlobalSplitMode = 0;		/*	Unified mode.	*/
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Update all spans WITHOUT overrides.			*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		if (vspan->hasSpanOverride == 0)
		{
			vspan->useExplicitConfig = 1;
			vspan->useSplitMode = 0;
			vspan->maxSegmentLossRate = newLossRate;
			computeRetransmissionLimits(vspan);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change max segment loss rate.", NULL);
	}
}

static void	manageMaxRetriesXmit(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	unsigned int	newMaxRetries;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	uvast	parsed_retries;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[2], &parsed_retries) < 0
		|| parsed_retries > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid maxRetriesXmit: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newMaxRetries = (unsigned int) parsed_retries;

	if (newMaxRetries < 3)
	{
		writeMemoNote("[?] maxRetries must be >= 3", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.defaultMaxRetriesXmit = newMaxRetries;
	ltpdb.useGlobalSplitMode = 1;		/*	Split mode.	*/
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Update all spans WITHOUT overrides.			*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		if (vspan->hasSpanOverride == 0)
		{
			if (vspan->useExplicitConfig == 0)
			{
				writeMemo("[i] Switching from legacy to \
explicit mode");
			}

			vspan->useExplicitConfig = 1;
			vspan->useSplitMode = 1;
			vspan->maxRetriesXmit = newMaxRetries;
			computeRetransmissionLimits(vspan);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change max retries xmit.", NULL);
	}
}

static void	manageMaxRetriesRecv(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	unsigned int	newMaxRetries;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	uvast	parsed_retries;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[2], &parsed_retries) < 0
		|| parsed_retries > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid maxRetriesRecv: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newMaxRetries = (unsigned int) parsed_retries;

	if (newMaxRetries < 3)
	{
		writeMemoNote("[?] maxRetries must be >= 3", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.defaultMaxRetriesRecv = newMaxRetries;
	ltpdb.useGlobalSplitMode = 1;		/*	Split mode.	*/
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Update all spans WITHOUT overrides.			*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		if (vspan->hasSpanOverride == 0)
		{
			vspan->useExplicitConfig = 1;
			vspan->useSplitMode = 1;
			vspan->maxRetriesRecv = newMaxRetries;
			computeRetransmissionLimits(vspan);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change max retries recv.", NULL);
	}
}

static void	manageMaxSegLossRateXmit(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	float		newLossRate;
	double		parsed_double;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_double(tokens[2], &parsed_double) < 0
		|| parsed_double < 0.0 || parsed_double >= 1.0)
	{
		writeMemoNote("[?] maxSegmentLossRate must be [0.0, 0.99]",
				tokens[2]);
		return;
	}
	newLossRate = (float) parsed_double;

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.defaultMaxSegLossRateXmit = newLossRate;
	ltpdb.useGlobalSplitMode = 1;		/*	Split mode.	*/
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Update all spans WITHOUT overrides.			*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		if (vspan->hasSpanOverride == 0)
		{
			vspan->useExplicitConfig = 1;
			vspan->useSplitMode = 1;
			vspan->maxSegLossRateXmit = newLossRate;
			computeRetransmissionLimits(vspan);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change max segment loss rate xmit.", NULL);
	}
}

static void	manageMaxSegLossRateRecv(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	LtpDB		ltpdb;
	float		newLossRate;
	double		parsed_double;
	PsmAddress	elt;
	LtpVspan	*vspan;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_double(tokens[2], &parsed_double) < 0
		|| parsed_double < 0.0 || parsed_double >= 1.0)
	{
		writeMemoNote("[?] maxSegmentLossRate must be [0.0, 0.99]",
				tokens[2]);
		return;
	}
	newLossRate = (float) parsed_double;

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &ltpdb, ltpdbObj, sizeof(LtpDB));
	ltpdb.defaultMaxSegLossRateRecv = newLossRate;
	ltpdb.useGlobalSplitMode = 1;		/*	Split mode.	*/
	sdr_write(sdr, ltpdbObj, (char *) &ltpdb, sizeof(LtpDB));

	/*	Update all spans WITHOUT overrides.			*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		if (vspan->hasSpanOverride == 0)
		{
			vspan->useExplicitConfig = 1;
			vspan->useSplitMode = 1;
			vspan->maxSegLossRateRecv = newLossRate;
			computeRetransmissionLimits(vspan);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change max segment loss rate recv.", NULL);
	}
}

/*	Per-span configuration commands (volatile memory only).	*/

static void	manageSpanMaxRetries(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	unsigned int	newMaxRetries;
	int		found = 0;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);
	uvast	parsed_retries;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[4], &parsed_retries) < 0
		|| parsed_retries > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid span maxretries: %s", tokens[4]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newMaxRetries = (unsigned int) parsed_retries;

	if (newMaxRetries < 3)
	{
		writeMemoNote("[?] maxretries must be at least 3", tokens[4]);
		return;
	}

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->hasSpanOverride = 1;
	vspan->useExplicitConfig = 1;
	vspan->useSplitMode = 0;  /* Unified mode. */
	vspan->maxRetries = newMaxRetries;
	computeRetransmissionLimits(vspan);
}

static void	manageSpanMaxRetriesXmit(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	unsigned int	newMaxRetriesXmit;
	int		found = 0;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);
	uvast	parsed_retries;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[4], &parsed_retries) < 0
		|| parsed_retries > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid span maxretriesxmit: %s", tokens[4]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newMaxRetriesXmit = (unsigned int) parsed_retries;

	if (newMaxRetriesXmit < 3)
	{
		writeMemoNote("[?] maxretriesxmit must be at least 3", tokens[4]);
		return;
	}

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->hasSpanOverride = 1;
	vspan->useExplicitConfig = 1;
	vspan->useSplitMode = 1;  /* Split mode. */
	vspan->maxRetriesXmit = newMaxRetriesXmit;
	computeRetransmissionLimits(vspan);
}

static void	manageSpanMaxRetriesRecv(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	unsigned int	newMaxRetriesRecv;
	int		found = 0;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);
	uvast	parsed_retries;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[4], &parsed_retries) < 0
		|| parsed_retries > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid span maxretriesrecv: %s", tokens[4]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newMaxRetriesRecv = (unsigned int) parsed_retries;

	if (newMaxRetriesRecv < 3)
	{
		writeMemoNote("[?] maxretriesrecv must be at least 3", tokens[4]);
		return;
	}

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->hasSpanOverride = 1;
	vspan->useExplicitConfig = 1;
	vspan->useSplitMode = 1;  /* Split mode. */
	vspan->maxRetriesRecv = newMaxRetriesRecv;
	computeRetransmissionLimits(vspan);
}

static void	manageSpanMaxSegLossRate(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	float		newLossRate;
	double		parsed_double;
	int		found = 0;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);

	if (platform_parse_double(tokens[4], &parsed_double) < 0
		|| parsed_double < 0.0 || parsed_double >= 1.0)
	{
		writeMemoNote("[?] maxseglossrate must be in [0.0, 0.99]",
				tokens[4]);
		return;
	}
	newLossRate = (float) parsed_double;

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->hasSpanOverride = 1;
	vspan->useExplicitConfig = 1;
	vspan->useSplitMode = 0;  /* Unified mode. */
	vspan->maxSegmentLossRate = newLossRate;
	computeRetransmissionLimits(vspan);
}

static void	manageSpanMaxSegLossRateXmit(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	float		newLossRateXmit;
	double		parsed_double;
	int		found = 0;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);

	if (platform_parse_double(tokens[4], &parsed_double) < 0
		|| parsed_double < 0.0 || parsed_double >= 1.0)
	{
		writeMemoNote("[?] maxseglossratexmit must be in [0.0, 0.99]",
				tokens[4]);
		return;
	}
	newLossRateXmit = (float) parsed_double;

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->hasSpanOverride = 1;
	vspan->useExplicitConfig = 1;
	vspan->useSplitMode = 1;  /* Split mode. */
	vspan->maxSegLossRateXmit = newLossRateXmit;
	computeRetransmissionLimits(vspan);
}

static void	manageSpanMaxSegLossRateRecv(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	float		newLossRateRecv;
	double		parsed_double;
	int		found = 0;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);

	if (platform_parse_double(tokens[4], &parsed_double) < 0
		|| parsed_double < 0.0 || parsed_double >= 1.0)
	{
		writeMemoNote("[?] maxseglossraterecv must be in [0.0, 0.99]",
				tokens[4]);
		return;
	}
	newLossRateRecv = (float) parsed_double;

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->hasSpanOverride = 1;
	vspan->useExplicitConfig = 1;
	vspan->useSplitMode = 1;  /* Split mode. */
	vspan->maxSegLossRateRecv = newLossRateRecv;
	computeRetransmissionLimits(vspan);
}

static void	manageSpanInactivityLimit(int tokenCount, char **tokens)
{
	PsmPartition	ionwm = getIonwm();
	LtpVdb		*vdb = getLtpVdb();
	PsmAddress	elt;
	LtpVspan	*vspan;
	uvast		engineId;
	unsigned int	newLimit;
	int		found = 0;
	char		nbrBuf[FQN_MAX_LENGTH];
	char		msgBuf[256];

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	engineId = getFqn(tokens[2]);
	uvast	parsed_limit;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[4], &parsed_limit) < 0
		|| parsed_limit > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid inactivity limit: %s", tokens[4]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	newLimit = (unsigned int) parsed_limit;

	/*	Find the span by engineId.				*/

	for (elt = sm_list_first(ionwm, vdb->spans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));
		if (vspan->engineId == engineId)
		{
			found = 1;
			break;
		}
	}

	if (!found)
	{
		writeMemoNote("[?] Span not found for engine", utoa(engineId));
		return;
	}

	/*	Update span-specific configuration (volatile only).	*/

	vspan->sessionInactivityLimit = newLimit;
	putFqn(nbrBuf, engineId);
	isprintf(msgBuf, sizeof msgBuf,
		"[i] Span %s sessionInactivityLimit set to %u seconds",
		nbrBuf, newLimit);
	writeMemo(msgBuf);
}

static void	manageMaxBacklog(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	ltpdbObj = getLtpDbObject();
	LtpDB		ltpdb;
	unsigned int	maxBacklog;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	uvast	parsed_backlog;
	char	errMsg[256];

	if (platform_parse_uvast(tokens[2], &parsed_backlog) < 0
		|| parsed_backlog > UINT_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid maxBacklog: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}
	maxBacklog = (unsigned int) parsed_backlog;

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

	/*	Global unified mode commands.				*/

	if (strcmp(tokens[1], "maxretries") == 0)
	{
		manageMaxRetries(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxseglossrate") == 0)
	{
		manageMaxSegLossRate(tokenCount, tokens);
		return;
	}

	/*	Global split mode commands.				*/

	if (strcmp(tokens[1], "maxretriesxmit") == 0)
	{
		manageMaxRetriesXmit(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxretriesrecv") == 0)
	{
		manageMaxRetriesRecv(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxseglossratexmit") == 0)
	{
		manageMaxSegLossRateXmit(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxseglossraterecv") == 0)
	{
		manageMaxSegLossRateRecv(tokenCount, tokens);
		return;
	}

	/*	Per-span commands (requires 'span <engineId> <param>').	*/

	if (strcmp(tokens[1], "span") == 0)
	{
		if (tokenCount < 4)
		{
			SYNTAX_ERROR;
			return;
		}

		/*	Unified mode per-span commands.		*/

		if (strcmp(tokens[3], "maxretries") == 0)
		{
			manageSpanMaxRetries(tokenCount, tokens);
			return;
		}

		if (strcmp(tokens[3], "maxseglossrate") == 0)
		{
			manageSpanMaxSegLossRate(tokenCount, tokens);
			return;
		}

		/*	Split mode per-span commands.			*/

		if (strcmp(tokens[3], "maxretriesxmit") == 0)
		{
			manageSpanMaxRetriesXmit(tokenCount, tokens);
			return;
		}

		if (strcmp(tokens[3], "maxretriesrecv") == 0)
		{
			manageSpanMaxRetriesRecv(tokenCount, tokens);
			return;
		}

		if (strcmp(tokens[3], "maxseglossratexmit") == 0)
		{
			manageSpanMaxSegLossRateXmit(tokenCount, tokens);
			return;
		}

		if (strcmp(tokens[3], "maxseglossraterecv") == 0)
		{
			manageSpanMaxSegLossRateRecv(tokenCount, tokens);
			return;
		}

		if (strcmp(tokens[3], "inactivity") == 0)
		{
			manageSpanInactivityLimit(tokenCount, tokens);
			return;
		}

		/*	Unknown span parameter.				*/

		SYNTAX_ERROR;
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
			vdb->watching |= WATCH_CLS;
			break;

		case '}':
			vdb->watching |= WATCH_handleCLS;
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
	uvast		engineId;
	char		lsiCmd[256];
	char		buffer[80];
	struct timeval	done_time;
	struct timeval	cur_time;
	int		max = 0;
	int		count = 0;

	/* Parameter intentionally unused. */
	(void)lineLength;

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

	while (isspace((unsigned char) *cursor))
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
				if (tokenCount > 1)
				{
					if (strcmp(tokens[1], "span") == 0)
					{
						if (tokenCount != 3)
						{
							SYNTAX_ERROR;
							return 0;
						}

						if (platform_parse_uvast(tokens[2], &engineId) < 0)
						{
							printText("[?] Invalid engine ID.");
							return 0;
						}
						oK(ltpStartSpan(engineId));
						return 0;
					}

					patchLsiCmd(tokenCount, tokens, lsiCmd);
					oK(addSeat(lsiCmd));
				}

				if (ltpStart() < 0)
				{
					putErrmsg("Can't start LTP.", NULL);
					return 0;
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
				if (tokenCount > 1)
				{
					if (strcmp(tokens[1], "span") == 0)
					{
						if (tokenCount != 3)
						{
							SYNTAX_ERROR;
							return 0;
						}

						if (platform_parse_uvast(tokens[2], &engineId) < 0)
						{
							printText("[?] Invalid engine ID.");
							return 0;
						}
						ltpStopSpan(engineId);
						return 0;
					}

					SYNTAX_ERROR;
					return 0;
				}

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
					int parsed_int;
					if (platform_parse_int(tokens[2], &parsed_int) < 0 || parsed_int < 0)
					{
						printText("[?] Invalid timeout value.");
						return 0;
					}
					max = parsed_int * 4;
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

#ifdef INPUT_HISTORY
	char *input;
#endif

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef NON_INTERACTIVE
		return 0;			/*	No stdin.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
#ifdef INPUT_HISTORY
			/* add input history */
			if ((input = linenoise(": ")) != NULL)
			{
				len = strlen(input);

				if (len == 0)
				{
					linenoiseFree(input);
					continue;
				}

				/* received input */
				if (len > 0)
				{
					linenoiseHistoryAdd(input);
				}

				if ((size_t) len > sizeof(line) - 1)
				{
					printf("\nInput is too long. Ignored.\n");
					fflush(stdout);
					linenoiseFree(input);
					continue;
				}
			}
			else if (errno == EAGAIN)
			{
				/* Ctrl+C pressed */
				printText("Please enter command 'q' to stop \
the program.");
				continue;
			}
			else
			{
				/* input error detected */
				printf("\nInput error detected. Exiting.\n");
				fflush(stdout);
				break;
			}

			/* copy the input to line for processing
			 * input sized already checked */

			strcpy(line, input);

			if (processLine(line, len, &checkNeeded, &rc))
			{
				linenoiseFree(input);
				break;		/*	Out of loop.	*/
			}
			linenoiseFree(input);
#else
			/* original input handling*/
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
#endif
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
	else if (strcmp(cmdFileName, "!") == 0)	/*	Resume.	*/
	{
		if (attachToLtp() == 0)
		{
			ltpStart();
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
			int	echoState = 1;

			oK(_echo(&echoState));
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
