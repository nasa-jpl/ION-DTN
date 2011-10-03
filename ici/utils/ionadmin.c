/*

	ionadmin.c:	contact list adminstration interface.

									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "rfx.h"

static time_t	_referenceTime(time_t *newValue)
{
	static time_t	reftime = 0;
	
	if (newValue)
	{
		reftime = *newValue;
	}

	return reftime;
}

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

static void	handleQuit()
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of ionadmin.c",
			lineNbr);
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
	PUTS("\t   1 <own node number> { \"\" | <configuration file name> }");
	PUTS("\t@\tAt");
	PUTS("\t   @ <reference time>");
	PUTS("\t\tTime format is either +ss or yyyy/mm/dd-hh:mm:ss,");
	PUTS("\t\tor to set reference time to the current time use '@ 0'.");
	PUTS("\t\tThe @ command sets the reference time from which subsequent \
relative times (+ss) are computed.");
	PUTS("\ta\tAdd");
	PUTS("\t   a contact <from time> <until time> <from node#> <to node#> \
<xmit rate in bytes per second>");
	PUTS("\t   a range <from time> <until time> <from node#> <to node#> \
<OWLT, i.e., range in light seconds>");
	PUTS("\t\tTime format is either +ss or yyyy/mm/dd-hh:mm:ss.");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} contact <from time> <from node#> <to node#>");
	PUTS("\t   {d|i} range <from time> <from node#> <to node#>");
	PUTS("\t\tTo delete all contacts or ranges for some pair of nodes,");
	PUTS("\t\tuse '*' as <from time>.");
	PUTS("\tl\tList");
	PUTS("\t   l contact");
	PUTS("\t   l range");
	PUTS("\tm\tManage ION database: clock, space occupancy");
	PUTS("\t   m utcdelta <local clock time minus UTC, in seconds>");
	PUTS("\t   m clockerr <new known maximum clock error, in seconds>");
	PUTS("\t   m production <new planned production rate, in bytes/sec>");
	PUTS("\t   m consumption <new planned consumption rate, in bytes/sec>");
	PUTS("\t   m occupancy <new occupancy limit value, in bytes>");
	PUTS("\t   m horizon { 0 | <end time for congestion forecasts> }");
	PUTS("\t   m alarm '<congestion alarm script>'");
	PUTS("\t   m usage");
	PUTS("\tr\tRun a script or another program, such as an admin progrm");
	PUTS("\t   r '<command>'");
	PUTS("\ts\tStart");
	PUTS("\t   s");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeNode(int tokenCount, char **tokens)
{
	char		*ownNodeNbrString = tokens[1];
	char		*configFileName = tokens[2];
	IonParms	parms;

	if (tokenCount < 2 || *ownNodeNbrString == '\0')
	{
		writeMemo("[?] No node number, can't initialize node.");
		return;
	}

	if (tokenCount < 3 || *configFileName == '\0')
	{
		configFileName = NULL;	/*	Use built-in defaults.	*/
	}

	if (readIonParms(configFileName, &parms) < 0)
	{
		putErrmsg("ionadmin can't get SDR parms.", NULL);
		return;
	}

	if (ionInitialize(&parms, strtol(ownNodeNbrString, NULL, 0)) < 0)
	{
		putErrmsg("ionadmin can't initialize ION.", NULL);
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	time_t		refTime;
	time_t		fromTime;
	time_t		toTime;
	unsigned long	fromNodeNbr;
	unsigned long	toNodeNbr;
	unsigned long	xmitRate;
	unsigned int	owlt;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (tokenCount != 7)
	{
		SYNTAX_ERROR;
		return;
	}

	refTime = _referenceTime(NULL);
	fromTime = readTimestampUTC(tokens[2], refTime);
	toTime = readTimestampUTC(tokens[3], refTime);
	if (toTime <= fromTime)
	{
		printText("Interval end time must be later than start time.");
		return;
	}

	fromNodeNbr = strtol(tokens[4], NULL, 0);
	toNodeNbr = strtol(tokens[5], NULL, 0);
	if (strcmp(tokens[1], "contact") == 0)
	{
		xmitRate = strtol(tokens[6], NULL, 0);
		oK(rfx_insert_contact(fromTime, toTime, fromNodeNbr,
				toNodeNbr, xmitRate));
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		owlt = atoi(tokens[6]);
		oK(rfx_insert_range(fromTime, toTime, fromNodeNbr,
				toNodeNbr, owlt));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	time_t		refTime;
	time_t		timestamp;
	unsigned long	fromNodeNbr;
	unsigned long	toNodeNbr;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (tokens[2][0] == '*')
	{
		timestamp = 0;
	}
	else
	{
		refTime = _referenceTime(NULL);
		timestamp = readTimestampUTC(tokens[2], refTime);
		if (timestamp == 0)
		{
			SYNTAX_ERROR;
			return;
		}
	}

	fromNodeNbr = strtol(tokens[3], NULL, 0);
	toNodeNbr = strtol(tokens[4], NULL, 0);
	if (strcmp(tokens[1], "contact") == 0)
	{
		oK(rfx_remove_contact(timestamp, fromNodeNbr, toNodeNbr));
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		oK(rfx_remove_range(timestamp, fromNodeNbr, toNodeNbr));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	IonDB		iondb;
	time_t		refTime;
	time_t		timestamp;
	unsigned long	fromNode;
	unsigned long	toNode;
	Object		elt;
	Object		contactObj;
	IonContact	contact;
	char		buffer[RFX_NOTE_LEN];
	Object		rangeObj;
	IonRange	range;

	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	refTime = _referenceTime(NULL);
	timestamp = readTimestampUTC(tokens[2], refTime);
	fromNode = strtol(tokens[3], NULL, 0);
	toNode = strtol(tokens[4], NULL, 0);
	if (strcmp(tokens[1], "contact") == 0)
	{
		for (elt = sdr_list_first(sdr, iondb.contacts); elt;
				elt = sdr_list_next(sdr, elt))
		{
			contactObj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &contact, contactObj,
					sizeof(IonContact));
			if (contact.fromTime < timestamp)
			{
				continue;
			}

			if (contact.fromTime > timestamp)
			{
				break;
			}

			if (contact.fromNode < fromNode)
			{
				continue;
			}

			if (contact.fromNode > fromNode)
			{
				break;
			}

			if (contact.toNode < toNode)
			{
				continue;
			}

			if (contact.toNode > toNode)
			{
				break;
			}

			/*	Contact has been located in database.	*/

			rfx_print_contact(contactObj, buffer);
			printText(buffer);
			return;
		}

		putErrmsg("Contact not found in database.", NULL);
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		for (elt = sdr_list_first(sdr, iondb.ranges); elt;
				elt = sdr_list_next(sdr, elt))
		{
			rangeObj = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &range, rangeObj,
					sizeof(IonRange));
			if (range.fromTime < timestamp)
			{
				continue;
			}

			if (range.fromTime > timestamp)
			{
				break;
			}

			if (range.fromNode < fromNode)
			{
				continue;
			}

			if (range.fromNode > fromNode)
			{
				break;
			}

			if (range.toNode < toNode)
			{
				continue;
			}

			if (range.toNode > toNode)
			{
				break;
			}

			/*	Range has been located in database.	*/

			rfx_print_range(rangeObj, buffer);
			printText(buffer);
			return;
		}

		putErrmsg("Range not found in database.", NULL);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	IonDB	iondb;
	Object	elt;
	Object	obj;
	char	buffer[RFX_NOTE_LEN];

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	if (strcmp(tokens[1], "contact") == 0)
	{
		for (elt = sdr_list_first(sdr, iondb.contacts); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			rfx_print_contact(obj, buffer);
			printText(buffer);
		}

		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		for (elt = sdr_list_first(sdr, iondb.ranges); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			rfx_print_range(obj, buffer);
			printText(buffer);
		}

		return;
	}

	SYNTAX_ERROR;
}

static void	manageUtcDelta(int tokenCount, char **tokens)
{
	int	newDelta;

	if (tokenCount!= 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newDelta = atoi(tokens[2]);
	CHKVOID(setDeltaFromUTC(newDelta) == 0);
}

static void	manageClockError(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	int	newMaxClockError;

	if (tokenCount!= 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxClockError = atoi(tokens[2]);
	if (newMaxClockError < 0 || newMaxClockError > 60)
	{
		putErrmsg("Maximum clock error out of range (0-60).", NULL);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.maxClockError = newMaxClockError;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maximum clock error.", NULL);
	}
}

static void	manageProduction(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	int	newRate;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newRate = atoi(tokens[2]);
	if (newRate < 0)
	{
		putErrmsg("Planned bundle production rate can't be negative.",
				itoa(newRate));
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.productionRate = newRate;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change bundle production rate.", NULL);
	}
}

static void	manageConsumption(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	int	newRate;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newRate = atoi(tokens[2]);
	if (newRate < 0)
	{
		putErrmsg("Planned bundle consumption rate can't be negative.",
				itoa(newRate));
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.consumptionRate = newRate;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change bundle consumption rate.", NULL);
	}
}

static void	manageOccupancy(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	int	newLimit;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newLimit = atoi(tokens[2]);
	if (newLimit <= 0)
	{
		putErrmsg("Bundle storage occupancy limit must be greater \
than zero.", itoa(newLimit));
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.occupancyCeiling = newLimit;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change bundle storage occupancy limit.",
				NULL);
	}
}

static void	manageHorizon(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	char	*horizonString;
	time_t	refTime;
	time_t	horizon;
	IonDB	iondb;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	horizonString = tokens[2];
	if (*horizonString == '0' && *(horizonString + 1) == 0)
	{
		horizon = 0;	/*	Remove horizon from database.	*/
	}
	else
	{
		refTime = _referenceTime(NULL);
		horizon = readTimestampUTC(horizonString, refTime);
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.horizon = horizon;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change congestion forecast horizon.", NULL);
	}
}

static void	manageAlarm(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	char	*newAlarmScript;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newAlarmScript = tokens[2];
	if (strlen(newAlarmScript) > 255)
	{
		putErrmsg("New congestion alarm script too long, limit is \
255 chars.", newAlarmScript);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	if (iondb.alarmScript != 0)
	{
		sdr_free(sdr, iondb.alarmScript);
	}

	iondb.alarmScript = sdr_string_create(sdr, newAlarmScript);
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change congestion alarm script.", NULL);
	}
}

static void	manageUsage(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(IonDB, iondb);
	char	buffer[128];

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	GET_OBJ_POINTER(sdr, IonDB, iondb, getIonDbObject());
	isprintf(buffer, sizeof buffer, "current = %ld  limit = %ld  max \
forecast = %ld", iondb->currentOccupancy, iondb->occupancyCeiling,
			iondb->maxForecastOccupancy);
	printText(buffer);
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "utcdelta") == 0)
	{
		manageUtcDelta(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "clockerr") == 0)
	{
		manageClockError(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "production") == 0
	|| strcmp(tokens[1], "prod") == 0)
	{
		manageProduction(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "consumption") == 0
	|| strcmp(tokens[1], "consum") == 0)
	{
		manageConsumption(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "occupancy") == 0
	|| strcmp(tokens[1], "occ") == 0)
	{
		manageOccupancy(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "horizon") == 0)
	{
		manageHorizon(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "alarm") == 0)
	{
		manageAlarm(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "usage") == 0)
	{
		manageUsage(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeRun(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Run what?");
		return;
	}
	
	if (pseudoshell(tokens[1]) < 0)
	{
		printText("pseudoshell failed.");
	}
	else
	{
		snooze(2);	/*	Give script time to finish.	*/
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

static int	processLine(char *line, int lineLength)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	char		buffer[80];
	time_t		refTime;
	time_t		currentTime;
	struct timeval	done_time;
	struct timeval	cur_time;

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
			tokenCount++;
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
			isprintf(buffer, sizeof buffer, "ION version %s.",
					IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			initializeNode(tokenCount, tokens);
			return 0;

		case 's':
			if (ionAttach() == 0)
			{
				if (rfx_start() < 0)
				{
					putErrmsg("Can't start RFX.", NULL);
				}

				/* Wait for rfx to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (rfx_system_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
					    done_time.tv_sec
					    && cur_time.tv_usec >=
					    done_time.tv_usec)
					{
						printText("[?] RFX start hung\
 up, abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (ionAttach() == 0)
			{
				rfx_stop();
			}

			return 0;

		case '@':
			if (ionAttach() == 0)
			{
				if (tokenCount < 2)
				{
					printText("Can't set reference time: \
no time.");
				}
				else if (strcmp(tokens[1], "0") == 0)
				{
					/*	Set reference time to
					 *	the current time.	*/

					currentTime = getUTCTime();
					oK(_referenceTime(&currentTime));
				}
				else
				{
					/*	Get current ref time.	*/

					refTime = _referenceTime(NULL);

					/*	Get new ref time, which
					 *	may be an offset from
					 *	the current ref time.	*/

					refTime = readTimestampUTC
						(tokens[1], refTime);

					/*	Record new ref time
					 *	for use by subsequent
					 *	command lines.		*/

					oK(_referenceTime(&refTime));
				}
			}

			return 0;

		case 'a':
			if (ionAttach() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (ionAttach() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (ionAttach() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (ionAttach() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'm':
			if (ionAttach() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'r':
			executeRun(tokenCount, tokens);
			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static int	runIonadmin(char *cmdFileName)
{
	time_t	currentTime;
	int	cmdFile;
	char	line[256];
	int	len;

	currentTime = getUTCTime();
	oK(_referenceTime(&currentTime));
	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdin.	*/
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

			if (processLine(line, len))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else if (strcmp(cmdFileName, ".") == 0) /*	Shutdown.	*/
	{
		if (ionAttach() == 0)
		{
			rfx_stop();
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

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	if (ionAttach() == 0)
	{
		oK(checkForCongestion());
	}
	else
	{
		printText("Can't check for congestion.");
	}

	printText("Stopping ionadmin.");
	ionDetach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	ionadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	result;

	result = runIonadmin(cmdFileName);
	if (result < 0)
	{
		puts("ionadmin failed.");
		return 1;
	}

	return 0;
}
