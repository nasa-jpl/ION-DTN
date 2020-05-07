/*

	cfdpadmin.c:	CFDP engine adminstration interface.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdpP.h"

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

static void	handleQuit(int signum)
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer,
			"Syntax error at line %d of cfdpadmin.c", lineNbr);
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
	PUTS("\t   l entity");
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
	PUTS("\ti\tInfo");
	PUTS("\t   i entity <entity nbr>");
	PUTS("\t   i");
	PUTS("\ts\tStart");
	PUTS("\t   s '<UTA command>'");
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

static int	attachToCfdp()
{
	if (cfdpAttach() < 0)
	{
		printText("CFDP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	entityNbr;

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

		CHKVOID(sdr_begin_xn(sdr));
		entityNbr = strtouvast(tokens[2]);
		oK(addEntity(entityNbr, tokens[3], tokens[4],
				strtol(tokens[5], NULL, 0),
				strtol(tokens[6], NULL, 0),
				strtol(tokens[7], NULL, 0)));
		oK(sdr_end_xn(sdr));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	entityNbr;

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

		CHKVOID(sdr_begin_xn(sdr));
		entityNbr = strtouvast(tokens[2]);
		oK(changeEntity(entityNbr, tokens[3], tokens[4],
				strtol(tokens[5], NULL, 0),
				strtol(tokens[6], NULL, 0),
				strtol(tokens[7], NULL, 0)));
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
		entityNbr = strtouvast(tokens[2]);
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
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));
	isprintf(buffer, sizeof buffer, UVAST_FIELDSPEC, entity->entityId);
	printText(buffer);
	switch (entity->utLayer)
	{
	case  UtBp:
		isprintf(buffer, sizeof buffer,
				"\tBP node number " UVAST_FIELDSPEC,
				entity->bpNodeNbr);
		printText(buffer);
		break;

	case  UtLtp:
		isprintf(buffer, sizeof buffer,
				"\tLTP engine number " UVAST_FIELDSPEC,
				entity->ltpEngineNbr);
		printText(buffer);
		break;

	case  UtTcp:
		printDottedString(entity->ipAddress, dottedString);
		isprintf(buffer, sizeof buffer, "\tTCP address %s port %hu",
				dottedString, entity->portNbr);
		printText(buffer);
	}

	isprintf(buffer, sizeof buffer, "\trtt %lu\tinCkType %d outCkType %d",
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

	entityNbr = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (findEntity(entityNbr, &entity) == 0)
	{
		printText("Unknown entity.");
	}
	else
	{
		printEntity(&entity);
	}

	sdr_exit_xn(sdr);
}

static void	printCfdpInfo()
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(CfdpDB, db);
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, CfdpDB, db, getCfdpDbObject());
	isprintf(buffer, sizeof buffer, "xncount=%lu, maxtrnbr=%lu, \
fillchar=0x%x, discard=%hu, requirecrc=%hu, segsize=%hu, inactivity=%u, \
ckperiod=%u, maxtimeouts=%u, maxevents=%u", db->transactionCounter,
			db->maxTransactionNbr, db->fillCharacter,
		       	db->discardIncompleteFile, db->crcRequired,
			db->maxFileDataLength, db->transactionInactivityLimit,
			db->checkTimerPeriod, db->checkTimeoutLimit,
			db->maxQueuedEvents);
	sdr_exit_xn(sdr);
	printText(buffer);
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
	Object	elt;
	Object	entityObj;
	Entity	entity;
	char	buffer[128];

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	isprintf(buffer, sizeof buffer,"(Entity " UVAST_FIELDSPEC "  Check \
timer period: %u  Check timeout limit: %u)", db->ownEntityId,
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

static void	executeList(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newDiscard;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newDiscard = atoi(tokens[2]);
	if (newDiscard != 0 && newDiscard != 1)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newRequirecrc;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newRequirecrc = atoi(tokens[2]);
	if (newRequirecrc != 0 && newRequirecrc != 1)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newFillchar;
	char	*trailing;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newFillchar = strtol(tokens[2], &trailing, 16);
	if (*trailing != '\0')
	{
		putErrmsg("fillCharacter invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.fillCharacter = newFillchar;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change fillCharacter.", NULL);
	}
}

static void	manageCkperiod(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newCkperiod;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newCkperiod = atoi(tokens[2]);
	if (newCkperiod < 1)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtimeouts;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxtimeouts = atoi(tokens[2]);
	if (newMaxtimeouts < 0)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxevents;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxevents = atoi(tokens[2]);
	if (newMaxevents < 0)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtrnbr;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxtrnbr = atoi(tokens[2]);
	if (newMaxtrnbr < 0)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newSegsize;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newSegsize = atoi(tokens[2]);
	if (newSegsize < 0)
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
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newLimit;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newLimit = atoi(tokens[2]);
	if (newLimit < 0)
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
			initializeCfdp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToCfdp() == 0)
			{
				if (tokenCount < 2)
				{
					printText("Can't start CFDP: no UTA \
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
				cfdpStop();
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
					max = atoi(tokens[2]) * 4;
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
		if (cfdpAttach() == 0)
		{
			cfdpStop();
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
