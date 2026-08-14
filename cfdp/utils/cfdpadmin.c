/*

	cfdpadmin.c:	CFDP engine adminstration interface.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdpP.h"

#ifdef INPUT_HISTORY
#include "linenoise.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef STRSOE
#include <strsoe_cfdpadmin.h>
#endif

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
			"Syntax error at line %d of cfdpadmin.c", lineNbr);
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
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a entity <entity nbr> <UT protocol name> <UT endpoint name> \
<rtt> <incstype> <outcstype>");
	PUTS("\t\tValid UT protocol names are bp and tcp.");
	PUTS("\t\tEndpoint name is EID for bp, socket spec for tcp.");
	PUTS("\t\tRTT is round-trip time, used to set acknowledgment timers.");
	PUTS("\t\tincstype is type of checksum for data rec'd from entity.");
	PUTS("\t\toutcstype is type of checksum for data sent to entity.");
	PUTS("\tc\tChange");
	PUTS("\t   c entity <entity nbr> <UT protocol name> <UT endpoint name> \
<rtt> <incstype> <outcstype>");
	PUTS("\td\tDelete");
	PUTS("\t   d entity <entity nbr>");
	PUTS("\tl\tList");
	PUTS("\t   l		- show general CFDP status including daemons");
	PUTS("\t   l entity - show all configured CFDP entities");
	PUTS("\tm\tManage");
	PUTS("\t   m discard { 0 | 1 }");
	PUTS("\t   m requirecrc { 0 | 1 }");
	PUTS("\t   m fillchar <file fill character in hex, e.g., 0xaa>");
	PUTS("\t   m ckperiod <check cycle period, in seconds>");
	PUTS("\t   m maxtimeouts <max number of check cycle timeouts>");
	PUTS("\t   m maxevents <max number of queued service indications>");
	PUTS("\t   m maxtrnbr <max transaction number>");
	PUTS("\t   m segsize <max bytes per file data segment>");
	PUTS("\t   m inactivity <inactivity limit, in seconds>");
	PUTS("\t   m throttle <max transmit rate in bits per second>");
	PUTS("\ti\tInfo");
	PUTS("\t   i entity <entity nbr>");
	PUTS("\t   i");
	PUTS("\ts\tStart");
	PUTS("\t   s '<UTA command>'");
	PUTS("\t   s 'bputa'              - Start bputa only");
	PUTS("\t   s 'bputa proxy'        - Start bputa with bpcpd proxy daemon");
	PUTS("\tt\tStartup-test");
	PUTS("\t   t [p [<timeout>]]");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\tw\tWatch CFDP activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., pq.  See man(5) for cfdprc.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeCfdp(int tokenCount, char **tokens)
{
	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (cfdpInit() < 0)
	{
		putErrmsg("cfdpadmin can't initialize CFDP.", NULL);
	}
}

static int	attachToCfdp(void)
{
	if (cfdpAttach() < 0)
	{
		printText("[?] CFDP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	entityNbr;
	uvast	parsed_rtt, parsed_inCk, parsed_outCk;
	char	errMsg[256];

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "entity") == 0)
	{
		if (tokenCount != 8)
		{
			SYNTAX_ERROR;
			return;
		}

		if (platform_parse_uvast(tokens[5], &parsed_rtt) < 0 || parsed_rtt > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid rtt: %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[6], &parsed_inCk) < 0 || parsed_inCk > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid incstype: %s", tokens[6]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[7], &parsed_outCk) < 0 || parsed_outCk > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid outcstype: %s", tokens[7]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		entityNbr = getFqn(tokens[2]);
		oK(addEntity(entityNbr, tokens[3], tokens[4],
				(unsigned int) parsed_rtt,
				(unsigned char) parsed_inCk,
				(unsigned char) parsed_outCk));
		oK(sdr_end_xn(sdr));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	entityNbr;
	uvast	parsed_rtt, parsed_inCk, parsed_outCk;
	char	errMsg[256];

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "entity") == 0)
	{
		if (tokenCount != 8)
		{
			SYNTAX_ERROR;
			return;
		}

		if (platform_parse_uvast(tokens[5], &parsed_rtt) < 0 || parsed_rtt > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid rtt: %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[6], &parsed_inCk) < 0 || parsed_inCk > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid incstype: %s", tokens[6]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[7], &parsed_outCk) < 0 || parsed_outCk > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid outcstype: %s", tokens[7]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		entityNbr = getFqn(tokens[2]);
		oK(changeEntity(entityNbr, tokens[3], tokens[4],
				(unsigned int) parsed_rtt,
				(unsigned char) parsed_inCk,
				(unsigned char) parsed_outCk));
		oK(sdr_end_xn(sdr));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	entityNbr;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "entity") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		entityNbr = getFqn(tokens[2]);
		oK(removeEntity(entityNbr));
		oK(sdr_end_xn(sdr));
		return;
	}

	SYNTAX_ERROR;
}

static void	printEntity(Entity *entity)
{
	Sdr	sdr = getIonsdr();
	char	dottedString[16];
	char	nbrBuf[FQN_MAX_LENGTH];
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));
	putFqn(nbrBuf, entity->entityId);
	isprintf(buffer, sizeof buffer, "%s", nbrBuf);
	printText(buffer);
	switch (entity->utLayer)
	{
	case  UtBp:
		putFqn(nbrBuf, entity->bpFqnn);
		isprintf(buffer, sizeof buffer, "\tBP node identifier %s",
				nbrBuf);
		printText(buffer);
		break;

	case  UtLtp:
		putFqn(nbrBuf, entity->ltpEngineNbr);
		isprintf(buffer, sizeof buffer, "\tLTP engine number %s",
				nbrBuf);
		printText(buffer);
		break;

	case  UtTcp:
		printDottedString(entity->ipAddress, dottedString);
		isprintf(buffer, sizeof buffer, "\tTCP address %s port %hu",
				dottedString, entity->portNbr);
		printText(buffer);
	}

	isprintf(buffer, sizeof buffer, "\trtt %u\tinCkType %d outCkType %d",
		entity->ackTimerInterval, entity->inCkType, entity->outCkType);
	printText(buffer);
	sdr_exit_xn(sdr);
}

static void	infoEntity(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	entityNbr;
	Entity	entity;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	entityNbr = getFqn(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (findEntity(entityNbr, &entity) == 0)
	{
		printText("[?] Unknown entity.");
	}
	else
	{
		printEntity(&entity);
	}

	sdr_exit_xn(sdr);
}

static void	printCfdpInfo(void)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(CfdpDB, db);
	char	buffer[256];
	char	throttleBuffer[128];

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, CfdpDB, db, getCfdpDbObject());
	isprintf(buffer, sizeof buffer, "xncount=%u, maxtrnbr=%u, \
fillchar=0x%x, discard=%hu, requirecrc=%hu, segsize=%hu, inactivity=%u, \
ckperiod=%u, maxtimeouts=%u, maxevents=%u", db->transactionCounter,
			db->maxTransactionNbr, db->fillCharacter,
			db->discardIncompleteFile, db->crcRequired,
			db->maxFileDataLength, db->transactionInactivityLimit,
			db->checkTimerPeriod, db->checkTimeoutLimit,
			db->maxQueuedEvents);
	printText(buffer);

	if (db->maxTransmitRate == 0)
	{
		printText("\tThrottle: unlimited");
	}
	else
	{
		isprintf(throttleBuffer, sizeof throttleBuffer,
			"\tThrottle: " UVAST_FIELDSPEC " bps",
			db->maxTransmitRate);
		printText(throttleBuffer);
	}

	sdr_exit_xn(sdr);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printCfdpInfo();
		return;
	}

	if (strcmp(tokens[1], "entity") == 0)
	{
		infoEntity(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listEntities(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	CfdpDB	*db = getCfdpConstants();
	SdrObject elt;
	SdrObject entityObj;
	Entity	entity;
	char	nbrBuf[FQN_MAX_LENGTH];
	char	buffer[128];

	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	putFqn(nbrBuf, db->ownEntityId);
	isprintf(buffer, sizeof buffer,"(Entity %s  Check \
timer period: %u  Check timeout limit: %u)", nbrBuf,
			db->checkTimerPeriod, db->checkTimeoutLimit);
	printText(buffer);
	for (elt = sdr_list_first(sdr, db->entities); elt;
			elt = sdr_list_next(sdr, elt))
	{
		entityObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &entity, entityObj, sizeof(Entity));
		printEntity(&entity);
	}

	sdr_exit_xn(sdr);
}

static void executeList(int tokenCount, char **tokens)
{
	Sdr     sdr;
	CfdpVdb *vdb;
	CfdpDB  *db;
	char    nbrBuf[FQN_MAX_LENGTH];
	char    buffer[256];
	char    utaCmd[256];

	/* Handle different list commands based on token count */
	if (tokenCount < 2)
	{
		/* No subcommand - show general CFDP status including daemons */
		sdr = getIonsdr();
		vdb = getCfdpVdb();
		db = getCfdpConstants();

		if (!vdb || !db)
		{
			printText("CFDP not initialized.");
			return;
		}

		/* Show CFDP system status header */
		printText("\n==== CFDP System Status ====");

		/* Show basic CFDP info */
		CHKVOID(sdr_begin_xn(sdr));
		putFqn(nbrBuf, db->ownEntityId);
		isprintf(buffer, sizeof buffer,
			"Local entity: %s", nbrBuf);
		printText(buffer);

		isprintf(buffer, sizeof buffer,
			"Check timer period: %u sec, Check timeout limit: %u",
			db->checkTimerPeriod, db->checkTimeoutLimit);
		printText(buffer);

		/* Get UTA command from database */
		SdrObject cfdpdbobj = getCfdpDbObject();
		CfdpDB cfdpdb;
		sdr_read(sdr, (char *) &cfdpdb, cfdpdbobj, sizeof(CfdpDB));
		istrcpy(utaCmd, cfdpdb.utaCmd, sizeof(utaCmd));
		sdr_exit_xn(sdr);

		/* Show daemon status section */
		printText("\n==== Daemon Status ====");

		/* CFDP Clock status */
		if (vdb->clockPid != ERROR && vdb->clockPid != 0
			&& sm_TaskExists(vdb->clockPid))
		{
			isprintf(buffer, sizeof buffer,
				"cfdpclock     : RUNNING (pid %d)", vdb->clockPid);
		}
		else
		{
			istrcpy(buffer, "cfdpclock     : NOT RUNNING", sizeof buffer);
		}
		printText(buffer);

		/* UTA daemon status (bputa or tcputa) */
		if (vdb->utaPid != ERROR && vdb->utaPid != 0
			&& sm_TaskExists(vdb->utaPid))
		{
			isprintf(buffer, sizeof buffer,
				"%-14s: RUNNING (pid %d)", utaCmd, vdb->utaPid);
		}
		else
		{
			isprintf(buffer, sizeof buffer,
				"%-14s: NOT RUNNING",
				strlen(utaCmd) > 0 ? utaCmd : "UTA daemon");
		}
		printText(buffer);

		/* bpcpd proxy daemon status */
		if (vdb->bpcpdPid != ERROR && vdb->bpcpdPid != 0
			&& sm_TaskExists(vdb->bpcpdPid))
		{
			isprintf(buffer, sizeof buffer,
				"bpcpd         : RUNNING (pid %d) [proxy daemon]",
				vdb->bpcpdPid);
		}
		else
		{
			istrcpy(buffer, "bpcpd         : NOT RUNNING", sizeof buffer);
		}
		printText(buffer);

		/* Show watch flags status */
		printText("\n==== Watch Status ====");
		if (vdb->watching == 0)
		{
			printText("Watching      : DISABLED");
		}
		else if (vdb->watching == -1)
		{
			printText("Watching      : ALL (p q)");
		}
		else
		{
			char watchFlags[32];
			int pos = 0;

			istrcpy(watchFlags, "Watching      : ", sizeof(watchFlags));
			pos = strlen(watchFlags);

			if (vdb->watching & WATCH_p)
			{
				watchFlags[pos++] = 'p';
				watchFlags[pos++] = ' ';
			}
			if (vdb->watching & WATCH_q)
			{
				watchFlags[pos++] = 'q';
				watchFlags[pos++] = ' ';
			}
			watchFlags[pos] = '\0';
			printText(watchFlags);
		}

		/* Show entity count */
		printText("\n==== Entities ====");
		CHKVOID(sdr_begin_xn(sdr));
		int entityCount = sdr_list_length(sdr, db->entities);
		sdr_exit_xn(sdr);

		isprintf(buffer, sizeof buffer,
			"Configured remote entities: %d", entityCount);
		printText(buffer);
		printText("(use 'l entity' to list all entities)");

		return;
	}

	/* Original behavior for subcommands */
	if (strcmp(tokens[1], "entity") == 0)
	{
		listEntities(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	manageDiscard(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newDiscard;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newDiscard) < 0 || (newDiscard != 0 && newDiscard != 1))
	{
		putErrmsg("discardIncompleteFile switch invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.discardIncompleteFile = newDiscard;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change discardIncompleteFile switch.", NULL);
	}
}

static void	manageRequirecrc(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newRequirecrc;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newRequirecrc) < 0 || (newRequirecrc != 0 && newRequirecrc != 1))
	{
		putErrmsg("crcRequired switch invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.crcRequired = newRequirecrc;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change crcRequired switch.", NULL);
	}
}

static void	manageFillchar(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	uvast	parsed_val;
	char	errMsg[256];

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_uvast(tokens[2], &parsed_val) < 0 || parsed_val > 255)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid fillCharacter (must be 0-255 or hex): %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.fillCharacter = (unsigned char) parsed_val;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change fillCharacter.", NULL);
	}
}

static void	manageCkperiod(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newCkperiod;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newCkperiod) < 0 || newCkperiod < 1)
	{
		putErrmsg("checkTimerPeriod invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.checkTimerPeriod = newCkperiod;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change checkTimerPeriod.", NULL);
	}
}

static void	manageMaxtimeouts(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtimeouts;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newMaxtimeouts) < 0 || newMaxtimeouts < 0)
	{
		putErrmsg("checkTimeoutLimit invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.checkTimeoutLimit = newMaxtimeouts;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change checkTimeoutLimit.", NULL);
	}
}

static void	manageMaxevents(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxevents;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newMaxevents) < 0 || newMaxevents < 0)
	{
		putErrmsg("eventQueueLimit invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.maxQueuedEvents = newMaxevents;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change eventQueueLimit.", NULL);
	}
}

static void	manageMaxtrnbr(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtrnbr;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newMaxtrnbr) < 0 || newMaxtrnbr < 0)
	{
		putErrmsg("maxTransactionNbr invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.maxTransactionNbr = newMaxtrnbr;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxTransactionNbr.", NULL);
	}
}

static void	manageSegsize(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newSegsize;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newSegsize) < 0 || newSegsize < 0)
	{
		putErrmsg("maxFileDataLength invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.maxFileDataLength = newSegsize;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxFileDataLength.", NULL);
	}
}

static void	manageInactivity(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newLimit;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_int(tokens[2], &newLimit) < 0 || newLimit < 0)
	{
		putErrmsg("transactionInactivityLimit invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.transactionInactivityLimit = newLimit;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change transactionInactivityLimit.", NULL);
	}
}

static void	manageThrottle(int tokenCount, char **tokens)
{
	uvast	newThrottle;
	char	errMsg[256];

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (platform_parse_uvast(tokens[2], &newThrottle) < 0)
	{
		isprintf(errMsg, sizeof(errMsg), "[?] Invalid throttle rate: %s", tokens[2]);
		PUTS(errMsg); writeMemo(errMsg);
		return;
	}

	if (cfdp_set_throttle(newThrottle) < 0)
	{
		putErrmsg("Can't set CFDP throttle.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "discard") == 0)
	{
		manageDiscard(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "requirecrc") == 0)
	{
		manageRequirecrc(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "fillchar") == 0)
	{
		manageFillchar(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "ckperiod") == 0)
	{
		manageCkperiod(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxtimeouts") == 0)
	{
		manageMaxtimeouts(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxevents") == 0)
	{
		manageMaxevents(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxtrnbr") == 0)
	{
		manageMaxtrnbr(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "segsize") == 0)
	{
		manageSegsize(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "inactivity") == 0)
	{
		manageInactivity(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "throttle") == 0)
	{
		manageThrottle(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	switchWatch(int tokenCount, char **tokens)
{
	CfdpVdb	*vdb = getCfdpVdb();
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
		case 'p':
			vdb->watching |= WATCH_p;
			break;

		case 'q':
			vdb->watching |= WATCH_q;
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
		break;

	case '1':
		state = 1;
		break;

	default:
		printText("Echo on or off?");
		return;
	}

	oK(_echo(&state));
}

static int cfdp_is_up(int count, int max)
{
	while (count <= max && !cfdp_entity_is_started())
	{
		microsnooze(250000);
		count++;
	}

	if (count > max)		//cfdp entity is not started
	{
		printText("CFDP entity is not started");
		return 0;
	}

	//cfdp entity is started

	printText("CFDP entity is started");
	return 1;
}

static int	processLine(char *line, int lineLength, int *rc)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	char		buffer[80];
	struct timeval	done_time;
	struct timeval	cur_time;

	/* Parameter intentionally unused. */
	(void)lineLength;

	int max = 0;
	int count = 0;

	tokenCount = 0;
	for (cursor = line, i = 0; i < 9; i++)
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
			isprintf(buffer, sizeof buffer, "%s", IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			initializeCfdp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToCfdp() == 0)
			{
				if (tokenCount < 2)
				{
					printText("[?] Can't start CFDP: no UTA \
command.");
				}
				else
				{
					if (cfdpStart(tokens[1]) < 0)
					{
						putErrmsg("Can't start CFDP.",
								NULL);
					}
				}

				/* Wait for cfdp to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (cfdp_entity_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
						done_time.tv_sec
						&& cur_time.tv_usec >=
						done_time.tv_usec) {
						printText("[?]  start hung up,\
 abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (attachToCfdp() == 0)
			{
				if (tokenCount > 1
				&& (strcmp(tokens[1], "-f") == 0
					|| strcmp(tokens[1], "f") == 0))
				{
					cfdpStop();	/*	Forced.	*/
				}
				else if (cfdpHasActiveTransactions())
				{
					printText("[?] CFDP has active \
transactions; refusing to stop.  Use 'x f' to force.");
				}
				else
				{
					cfdpStop();
				}
			}

			return 0;

		case 'a':
			if (attachToCfdp() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attachToCfdp() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToCfdp() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;


		case 'l':
			if (attachToCfdp() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'm':
			if (attachToCfdp() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToCfdp() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'w':
			if (attachToCfdp() == 0)
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
				if (tokenCount < 3) //use default timeout
				{
					max = DEFAULT_CHECK_TIMEOUT;
				}
				else
				{
					int parsed_max;
					if (platform_parse_int(tokens[2], &parsed_max) < 0 || parsed_max < 0)
					{
						printText("[?] Invalid timeout.");
						return 0;
					}
					max = parsed_max * 4;
				}
			}
			else
			{
				max = 1;
			}

			count = 1;
			while (count <= max && attachToCfdp() == -1)
			{
				microsnooze(250000);
				count++;
			}

			if (count > max)
			{
				//cfdp entity is not started
				printText("CFDP entity is not started");
				return 1;
			}

			//attached to cfdp system

			*rc = cfdp_is_up(count, max);
			return 1;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	cfdpadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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

			if (processLine(line, len, &rc))
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

			if (processLine(line, len, &rc))
			{
				break;		/*	Out of loop.	*/
			}
#endif
		}
#endif
	}
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		if (cfdpAttach() == 0)
		{
			cfdpStop();
		}
	}
	else if (strcmp(cmdFileName, "!") == 0)	/*	Resume.		*/
	{
		if (cfdpAttach() == 0)
		{
			cfdpStart(NULL);
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = ifopen(cmdFileName, O_RDONLY, 0777);
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

				if (processLine(line, len, &rc))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping cfdpadmin.");
	ionDetach();
	return rc;
}

#ifdef STRSOE
int	cfdpadmin_processLine(char *line, int lineLength, int *rc)
{
	return processLine(line, lineLength, rc);
}

void	cfdpadmin_help(void)
{
	printUsage();
}
#endif
