/*

	ionadmin.c:	contact list adminstration interface.

									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "rfx.h"

static Sdr	sdr = NULL;
static int	echo = 0;
static IonDB	iondb;
static time_t	referenceTime = 0;

static void	printText(char *text)
{
	if (echo)
	{
		writeMemo(text);
	}

	puts(text);
}

static void	handleQuit()
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	sprintf(buffer, "Syntax error at line %d of ionadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	puts("Valid commands are:");
	puts("\tq\tQuit");
	puts("\th\tHelp");
	puts("\t?\tHelp");
	puts("\tv\tPrint version of ION.");
	puts("\t1\tInitialize");
	puts("\t   1 <own node number> { \"\" | <configuration file name> }");
	puts("\t@\tAt");
	puts("\t   @ <reference time>");
	puts("\t\tTime format is either +ss or yyyy/mm/dd-hh:mm:ss.");
	puts("\t\tThe @ command sets the reference time from which subsequent \
relative times (+ss) are computed.");
	puts("\ta\tAdd");
	puts("\t   a contact <from time> <until time> <from node#> <to node#> \
<xmit rate in bytes per second>");
	puts("\t   a range <from time> <until time> <from node#> <to node#> \
<OWLT, i.e., range in light seconds>");
	puts("\t\tTime format is either +ss or yyyy/mm/dd-hh:mm:ss.");
	puts("\td\tDelete");
	puts("\ti\tInfo");
	puts("\t   {d|i} contact <from time> <from node#> <to node#>");
	puts("\t   {d|i} range <from time> <from node#> <to node#>");
	puts("\tl\tList");
	puts("\t   l contact");
	puts("\t   l range");
	puts("\tm\tManage ION database: clock, space occupancy");
	puts("\t   m utcdelta <local clock time minus UTC, in seconds>");
	puts("\t   m clockerr <new known maximum clock error, in seconds>");
	puts("\t   m production <new planned production rate, in bytes/sec>");
	puts("\t   m consumption <new planned consumption rate, in bytes/sec>");
	puts("\t   m occupancy <new occupancy limit value, in bytes>");
	puts("\t   m horizon { 0 | <end time for congestion forecasts> }");
	puts("\t   m alarm '<congestion alarm script>'");
	puts("\t   m usage");
	puts("\tr\tRun a script or another program, such as an admin progrm");
	puts("\t   r '<command>'");
	puts("\ts\tStart");
	puts("\t   s");
	puts("\tx\tStop");
	puts("\t   x");
	puts("\te\tEnable or disable echo of printed output to log file");
	puts("\t   e { 0 | 1 }");
	puts("\t#\tComment");
	puts("\t   # <comment text>");
}

static void	initializeNode(int tokenCount, char **tokens)
{
	char		*ownNodeNbrString = tokens[1];
	char		*configFileName = tokens[2];
	IonParms	parms;

	if (*configFileName == '\0')	/*	Zero-length string.	*/
	{
		configFileName = NULL;	/*	Use built-in defaults.	*/
	}

	if (readIonParms(configFileName, &parms) < 0)
	{
		putErrmsg("ionadmin can't get SDR parms.", NULL);
		return;
	}

	if (ionInitialize(&parms, atol(ownNodeNbrString)) < 0)
	{
		putErrmsg("ionadmin can't initialize ION.", NULL);
		return;
	}

	sdr = getIonsdr();
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
}

static int	attachToNode()
{
	if (sdr == NULL)
	{
		if (ionAttach() < 0)
		{
			printText("Node not initialized yet.");
			return -1;
		}

		sdr = getIonsdr();
		sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	}

	return 0;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	time_t		fromTime;
	time_t		toTime;
	unsigned long	fromNodeNbr;
	unsigned long	toNodeNbr;
	unsigned long	xmitRate;
	unsigned int	owlt;
	Object		elt;

	if (attachToNode() < 0) return;
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

	if (referenceTime == 0)
	{
		referenceTime = getUTCTime();
	}

	fromTime = readTimestampUTC(tokens[2], referenceTime);
	toTime = readTimestampUTC(tokens[3], referenceTime);
	if (toTime <= fromTime)
	{
		printText("Interval end time must be later than start time.");
		return;
	}

	fromNodeNbr = atol(tokens[4]);
	toNodeNbr = atol(tokens[5]);
	if (strcmp(tokens[1], "contact") == 0)
	{
		xmitRate = atol(tokens[6]);
		elt = rfx_insert_contact(fromTime, toTime, fromNodeNbr,
				toNodeNbr, xmitRate);
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		owlt = atoi(tokens[6]);
		elt = rfx_insert_range(fromTime, toTime, fromNodeNbr,
				toNodeNbr, owlt);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	time_t		timestamp;
	unsigned long	fromNodeNbr;
	unsigned long	toNodeNbr;

	if (attachToNode() < 0) return;
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

	if (referenceTime == 0)
	{
		referenceTime = getUTCTime();
	}

	timestamp = readTimestampUTC(tokens[2], referenceTime);
	fromNodeNbr = atol(tokens[3]);
	toNodeNbr = atol(tokens[4]);
	if (strcmp(tokens[1], "contact") == 0)
	{
		rfx_remove_contact(timestamp, fromNodeNbr, toNodeNbr);
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		rfx_remove_range(timestamp, fromNodeNbr, toNodeNbr);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo(int tokenCount, char **tokens)
{
	time_t		timestamp;
	unsigned long	fromNode;
	unsigned long	toNode;
	Object		elt;
	Object		contactObj;
	IonContact	contact;
	char		buffer[RFX_NOTE_LEN];
	Object		rangeObj;
	IonRange	range;

	if (attachToNode() < 0) return;
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

	if (referenceTime == 0)
	{
		referenceTime = getUTCTime();
	}

	timestamp = readTimestampUTC(tokens[2], referenceTime);
	fromNode = atol(tokens[3]);
	toNode = atol(tokens[4]);
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
	Object	elt;
	Object	obj;
	char	buffer[RFX_NOTE_LEN];

	if (attachToNode() < 0) return;
	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

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
	setDeltaFromUTC(newDelta);
}

static void	manageClockError(int tokenCount, char **tokens)
{
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
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	char	*horizonString;
	time_t	horizon;

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
		if (referenceTime == 0)
		{
			referenceTime = getUTCTime();
		}

		horizon = readTimestampUTC(horizonString, referenceTime);
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
		OBJ_POINTER(IonDB, iondb);
	char	buffer[128];

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	GET_OBJ_POINTER(sdr, IonDB, iondb, getIonDbObject());
	sprintf(buffer, "current = %ld  limit = %ld  max forecast = %ld",
			iondb->currentOccupancy, iondb->occupancyCeiling,
			iondb->maxForecastOccupancy);
	printText(buffer);
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (attachToNode() < 0) return;
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
	if (tokenCount < 2)
	{
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		echo = 0;
		break;

	case '1':
		echo = 1;
		break;

	default:
		printText("Echo on or off?");
	}
}

static int	processLine(char *line)
{
	int	lineLength;
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];
	char	buffer[80];

	if (line == NULL) return 0;

	lineLength = strlen(line);
	if (lineLength <= 0) return 0;

	if (line[lineLength - 1] == 0x0a)	/*	LF (newline)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength <= 0) return 0;
	}

	if (line[lineLength - 1] == 0x0d)	/*	CR (DOS text)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength <= 0) return 0;
	}

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
			sprintf(buffer, "ION version %s.", IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			initializeNode(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToNode() < 0)
			{
				return 0;
			}

			if (rfx_start() < 0)
			{
				putErrmsg("Can't start RFX.", NULL);
			}

			return 0;

		case 'x':
			if (attachToNode() < 0)
			{
				return 0;
			}

			rfx_stop();
			return 0;

		case '@':
			if (attachToNode() < 0)
			{
				return 0;
			}

			if (tokenCount < 2)
			{
				printText("Can't set reference time: no time.");
			}
			else
			{
				if (referenceTime == 0)
				{
					referenceTime = getUTCTime();
				}

				referenceTime = readTimestampUTC(tokens[1],
						referenceTime);
			}

			return 0;

		case 'a':
			executeAdd(tokenCount, tokens);
			return 0;

		case 'd':
			executeDelete(tokenCount, tokens);
			return 0;

		case 'i':
			executeInfo(tokenCount, tokens);
			return 0;

		case 'l':
			executeList(tokenCount, tokens);
			return 0;

		case 'm':
			executeManage(tokenCount, tokens);
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
	FILE	*cmdFile;
	char	line[256];

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
		isignal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			if (fgets(line, sizeof line, stdin) == NULL)
			{
				if (feof(stdin))
				{
					break;
				}

				perror("ionadmin fgets failed");
				break;		/*	Out of loop.	*/
			}

			if (processLine(line))
			{
				break;		/*	Out of loop.	*/
			}
		}
	}
	else if (strcmp(cmdFileName, ".") == 0) /*	Shutdown.	*/
	{
		if (attachToNode() == 0)
		{
			rfx_stop();
		}
	}
	else					/*	Scripted.	*/
	{
		cmdFile = fopen(cmdFileName, "r");
		if (cmdFile == NULL)
		{
			perror("Can't open command file");
		}
		else
		{
			while (1)
			{
				if (fgets(line, sizeof line, cmdFile) == NULL)
				{
					if (feof(cmdFile))
					{
						break;	/*	Loop.	*/
					}

					perror("ionadmin fgets failed");
					break;		/*	Loop.	*/
				}

				if (line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line))
				{
					break;	/*	Out of loop.	*/
				}
			}

			fclose(cmdFile);
		}
	}

	writeErrmsgMemos();
	if (attachToNode() == 0)
	{
		checkForCongestion();
	}
	else
	{
		printText("Can't check for congestion.");
	}

	printText("Stopping ionadmin.");
	ionDetach();
	return 0;
}
