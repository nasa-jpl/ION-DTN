/*

	ionadmin.c:	contact list adminstration interface.

									*/
/*	Copyright (c) 2007, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "zco.h"
#include "rfx.h"

#ifdef STRSOE
#include <strsoe_ionadmin.h>
#endif

static time_t	_referenceTime(time_t *newValue)
{
	static time_t	refTime = 0;

	if (newValue)
	{
		refTime = *newValue;
	}

	return refTime;
}

static vast	_regionIdx(int *newValue)
{
	static int	regionIdx = -1;

	if (newValue)
	{
		regionIdx = *newValue;
	}

	return regionIdx;
}

static int	_forecastNeeded(int parm)
{
	static int	needed = 0;
	int		result;

	result = needed;
	if (parm)	/*	Signaling that forecast is needed.	*/
	{
		needed = 1;
	}
	else		/*	Signaling intent to forecast as nec.	*/
	{
		needed = 0;	/*	Forecasting resets flag value.	*/
	}

	return result;
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

static void	handleQuit(int signum)
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
	PUTS("\t   ^ <region number>");
	PUTS("\t\tThe ^ command identifies the region to which subsequent \
'add contact' operations pertain.");
	PUTS("\ta\tAdd");
	PUTS("\t   a contact <from time> <until time> <from node#> <to node#> \
<xmit rate in bytes per second> [confidence in occurrence; default is 1.0]");
	PUTS("\t   a range <from time> <until time> <from node#> <to node#> \
<OWLT, i.e., range in light seconds>");
	PUTS("\t\tTime format is either +ss or yyyy/mm/dd-hh:mm:ss,");
	PUTS("\t\texcept time '0' indicates a hypothetical contact");
	PUTS("\t\tand time '-1' indicates a 'registration' contact.");
	PUTS("\tc\tChange");
	PUTS("\t   c contact <from time> <from node#> <to node#> <xmit rate \
in bytes per second> [confidence in occurrence]");
	PUTS("\t\tNot applicable for hypothetical and registration contacts.");
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
	PUTS("\t   m utcdelta <local time minus correct UTC, in seconds>");
	PUTS("\t   m clockerr <new known maximum clock error, in seconds>");
	PUTS("\t   m clocksync [ { 0 | 1 } ]");
	PUTS("\t   m production <new planned production rate, in bytes/sec>");
	PUTS("\t   m consumption <new planned consumption rate, in bytes/sec>");
	PUTS("\t   m inbound <new inbound ZCO heap occupancy limit, in MB; \
-1 means \"unchanged\"> [<new inbound ZCO file occupancy limit, in MB>]");
	PUTS("\t   m outbound <new outbound ZCO heap occupancy limit, in MB; \
-1 means \"unchanged\"> [<new outbound ZCO file occupancy limit, in MB>]");
	PUTS("\t   m search <max free blocks to search through>");
	PUTS("\t   m horizon { 0 | <end time for congestion forecasts> }");
	PUTS("\t   m alarm '<congestion alarm script>'");
	PUTS("\t   m usage");
	PUTS("\t   m home <home region nbr>");
	PUTS("\t   m outer <outer region nbr>");
	PUTS("\t   m passageway <node number> <home region nbr> <outer region nbr>");
	PUTS("\t\tSetting outer region nbr to -1 makes node a non-passageway.");
	PUTS("\t\tSetting home region nbr to -1 removes the passageway.");
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

static int	initializeNode(int tokenCount, char **tokens)
{
	char		*ownNodeNbrString = tokens[1];
	char		*configFileName = tokens[2];
	IonParms	parms;

	if (tokenCount < 2 || *ownNodeNbrString == '\0')
	{
		writeMemo("[?] No node number, can't initialize node.");
		return 1;
	}

	if (readIonParms(configFileName, &parms) < 0)
	{
		putErrmsg("ionadmin can't get SDR parms.", NULL);
		return 1;
	}

	if (ionInitialize(&parms, strtouvast(ownNodeNbrString)) < 0)
	{
		putErrmsg("ionadmin can't initialize ION.", NULL);
		return 1;
	}

	/*	Default home region is 0.				*/

	if (ionManageRegion(0, 0) < 0)
	{
		putErrmsg("ionadmin can't initialize home region.", NULL);
		return 1;
	}

	return 0;
}

void	executeAdd(int tokenCount, char **tokens)
{
	uvast		ownNodeNbr = getOwnNodeNbr();
	time_t		refTime;
	time_t		fromTime;
	time_t		toTime;
	uvast		fromNodeNbr;
	uvast		toNodeNbr;
	PsmAddress	xaddr;
	unsigned int	xmitRate;
	float		confidence;
	unsigned int	owlt;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	switch (tokenCount)
	{
	case 8:
		confidence = atof(tokens[7]);
		break;

	case 7:
		confidence = 1.0;	/*	Default value.		*/
		break;

	default:
		SYNTAX_ERROR;
		return;
	}

	refTime = _referenceTime(NULL);
	fromNodeNbr = strtouvast(tokens[4]);
	toNodeNbr = strtouvast(tokens[5]);
	if (fromNodeNbr <= 0)
	{
		printText("'From' node number must be greater than zero.");
		return;
	}

	if (toNodeNbr <= 0)
	{
		printText("'To' node number must be greater than zero.");
		return;
	}

	if (confidence < 0.0 || confidence > 1.0)
	{
		printText("Confidence must be in the range 0.0 to 1.0.");
		return;
	}

	if (strcmp(tokens[1], "contact") == 0)
	{
		if (strncmp(tokens[2], "0", 1) == 0)
		{
			fromTime = 0;
		}
		else if (strncmp(tokens[2], "-1", 1) == 0)
		{
			fromTime = (time_t) -1;
		}
		else
		{
			fromTime = readTimestampUTC(tokens[2], refTime);
		}

		if (strncmp(tokens[3], "0", 1) == 0)
		{
			toTime = 0;
		}
		else if (strncmp(tokens[3], "-1", 1) == 0)
		{
			toTime = (time_t) -1;
		}
		else
		{
			toTime = readTimestampUTC(tokens[3], refTime);
		}

		if (fromTime == (time_t) -1)
		{
			/*	Must be a registration contact.		*/

			if (fromNodeNbr != toNodeNbr)
			{
				printText("For registration contact, from \
and to nodes must be identical.");
				return;
			}

			xmitRate = 0;
			confidence = 1.0;
		}
		else if (fromTime == 0)
		{
			/*	Must be a hypothetical contact.		*/

			if (fromNodeNbr == toNodeNbr
			|| (fromNodeNbr != ownNodeNbr
				&& toNodeNbr != ownNodeNbr))
			{
				printText("For hypothetical contact, either \
from or to node must be the local node and the other must not.");
				return;
			}

			xmitRate = 0;
			confidence = 0.0;
		}
		else	/*	Scheduled contact.			*/
		{
			if (toTime <= fromTime)
			{
				printText("Interval end time must be later \
than start time and earlier than 19 January 2038.");
				return;
			}

			xmitRate = strtol(tokens[6], NULL, 0);
		}

		oK(rfx_insert_contact(ionPickRegion(_regionIdx(NULL)),
				fromTime, toTime, fromNodeNbr, toNodeNbr,
				xmitRate, confidence, &xaddr));
		oK(_forecastNeeded(1));
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		fromTime = readTimestampUTC(tokens[2], refTime);
		toTime = readTimestampUTC(tokens[3], refTime);
		if (toTime <= fromTime)
		{
			printText("Interval end time must be later than start \
time and earlier than 19 January 2038.");
			return;
		}

		owlt = strtol(tokens[6], NULL, 0);
		oK(rfx_insert_range(fromTime, toTime, fromNodeNbr,
				toNodeNbr, owlt, &xaddr));
		return;
	}

	SYNTAX_ERROR;
}

void	executeChange(int tokenCount, char **tokens)
{
	time_t		refTime;
	time_t		fromTime;
	uvast		fromNodeNbr;
	uvast		toNodeNbr;
	unsigned int	xmitRate;
	float		confidence;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "contact") != 0)
	{
		SYNTAX_ERROR;
		return;
	}

	switch (tokenCount)
	{
	case 7:
		confidence = atof(tokens[6]);
		break;

	case 6:
		confidence = -1.0;	/*	Meaning "no change".	*/
		break;

	default:
		SYNTAX_ERROR;
		return;
	}

	refTime = _referenceTime(NULL);
	fromTime = readTimestampUTC(tokens[2], refTime);
	if (fromTime == 0)
	{
		SYNTAX_ERROR;
		return;
	}

	fromNodeNbr = strtouvast(tokens[3]);
	toNodeNbr = strtouvast(tokens[4]);
	xmitRate = strtol(tokens[5], NULL, 0);
	oK(rfx_revise_contact(fromTime, fromNodeNbr, toNodeNbr, xmitRate,
			confidence));
}

void	executeDelete(int tokenCount, char **tokens)
{
	time_t	refTime;
	time_t	fromTime;
	time_t	*scope = &fromTime;
	uvast	fromNodeNbr;
	uvast	toNodeNbr;

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

	if (tokens[2][0] == '*')		/*	All.		*/
	{
		scope = NULL;
	}
	else if (strcmp(tokens[2], "0") == 0)	/*	Hypothetical.	*/
	{
		fromTime = 0;
	}
	else if (strcmp(tokens[2], "-1") == 0)	/*	Registration.	*/
	{
		fromTime = (time_t) -1;
	}
	else				/*	Predicted or Scheduled.	*/
	{
		refTime = _referenceTime(NULL);
		fromTime = readTimestampUTC(tokens[2], refTime);
		if (fromTime == 0)
		{
			SYNTAX_ERROR;
			return;
		}
	}

	fromNodeNbr = strtouvast(tokens[3]);
	toNodeNbr = strtouvast(tokens[4]);
	if (strcmp(tokens[1], "contact") == 0)
	{
		oK(rfx_remove_contact(scope, fromNodeNbr, toNodeNbr));
		oK(_forecastNeeded(1));
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		oK(rfx_remove_range(scope, fromNodeNbr, toNodeNbr));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	time_t		refTime;
	time_t		fromTime;
	uvast		fromNode;
	uvast		toNode;
	IonCXref	arg1;
	PsmAddress	elt;
	PsmAddress	addr;
	PsmAddress	nextElt;
	char		buffer[RFX_NOTE_LEN];
	IonRXref	arg2;

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

	refTime = _referenceTime(NULL);
	if (strcmp(tokens[2], "0") == 0)	/*	Hypothetical.	*/
	{
		fromTime = 0;
	}
	else if (strcmp(tokens[2], "-1") == 0)	/*	Registration.	*/
	{
		fromTime = (time_t) -1;
	}
	else				/*	Predicted or Scheduled.	*/
	{
		fromTime = readTimestampUTC(tokens[2], refTime);
	}

	fromNode = strtouvast(tokens[3]);
	toNode = strtouvast(tokens[4]);
	if (strcmp(tokens[1], "contact") == 0)
	{
		memset((char *) &arg1, 0, sizeof(IonCXref));
		arg1.fromNode = fromNode;
		arg1.toNode = toNode;
		arg1.fromTime = fromTime;
		CHKVOID(sdr_begin_xn(sdr));
		elt = sm_rbt_search(ionwm, vdb->contactIndex,
				rfx_order_contacts, &arg1, &nextElt);
		if (elt)
		{
			addr = sm_rbt_data(ionwm, elt);
			oK(rfx_print_contact(addr, buffer));
			printText(buffer);
		}
		else
		{
			printText("Contact not found in database.");
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		memset((char *) &arg2, 0, sizeof(IonRXref));
		arg2.fromNode = fromNode;
		arg2.toNode = toNode;
		arg2.fromTime = fromTime;
		CHKVOID(sdr_begin_xn(sdr));
		elt = sm_rbt_search(ionwm, vdb->rangeIndex,
				rfx_order_ranges, &arg2, &nextElt);
		if (elt)
		{
			addr = sm_rbt_data(ionwm, elt);
			oK(rfx_print_range(addr, buffer));
			printText(buffer);
		}
		else
		{
			printText("Range not found in database.");
		}

		sdr_exit_xn(sdr);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonVdb		*vdb = getIonVdb();
	PsmAddress	elt;
	PsmAddress	addr;
	char		buffer[RFX_NOTE_LEN];

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "contact") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sm_rbt_first(ionwm, vdb->contactIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			addr = sm_rbt_data(ionwm, elt);
			rfx_print_contact(addr, buffer);
			printText(buffer);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "range") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sm_rbt_first(ionwm, vdb->rangeIndex); elt;
				elt = sm_rbt_next(ionwm, elt))
		{
			addr = sm_rbt_data(ionwm, elt);
			rfx_print_range(addr, buffer);
			printText(buffer);
		}

		sdr_exit_xn(sdr);
		return;
	}

	SYNTAX_ERROR;
}

static void	manageUtcDelta(int tokenCount, char **tokens)
{
	int	newDelta;

	if (tokenCount != 3)
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

	if (tokenCount != 3)
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

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.maxClockError = newMaxClockError;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maximum clock error.", NULL);
	}
}

static void	manageClockSync(int tokenCount, char **tokens)
{
	Sdr	sdr;
	Object	iondbObj;
	IonDB	iondb;
	int	newSyncVal;
	char	buffer[128];

	if (tokenCount < 2 || tokenCount > 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (tokenCount == 3)
	{
		newSyncVal = atoi(tokens[2]);
		sdr = getIonsdr();
		iondbObj = getIonDbObject();
		CHKVOID(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		iondb.clockIsSynchronized = (!(newSyncVal == 0));
		sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't change clock sync.", NULL);
		}
	}

	isprintf(buffer, sizeof buffer, "clock sync = %d",
			ionClockIsSynchronized());
	printText(buffer);
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
		newRate = -1;			/*	Not metered.	*/
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.productionRate = newRate;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change bundle production rate.", NULL);
	}

	oK(_forecastNeeded(1));
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
		newRate = -1;			/*	Not metered.	*/
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.consumptionRate = newRate;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change bundle consumption rate.", NULL);
	}

	oK(_forecastNeeded(1));
}

static void	manageOccupancy(int tokenCount, char **tokens, ZcoAcct acct)
{
	Sdr	sdr = getIonsdr();
	Object	iondbObj = getIonDbObject();
	IonDB	iondb;
	vast	newFileLimit = -1;	/*	-1 = "unchanged"	*/
	vast	newHeapLimit = -1;	/*	-1 = "unchanged"	*/
	double	fileLimit;
	double	maxHeapLimit;
	double	heapLimit;

	switch (tokenCount)
	{
	case 4:
		newFileLimit = strtovast(tokens[3]);

		/*	Intentional fall-through to next case.		*/

	case 3:
		newHeapLimit = strtovast(tokens[2]);
		break;

	default:
		SYNTAX_ERROR;
		return;
	}

	if (newFileLimit < -1)
	{
		writeMemo("[?] ZCO file occupancy limit can't be negative.");
		return;
	}

	if (newHeapLimit < -1)
	{
		writeMemo("[?] ZCO heap occupancy limit can't be negative.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	if (newFileLimit != -1)	/*	Overriding current value.	*/
	{
		fileLimit = newFileLimit;

		/*	Convert from MB to bytes.			*/

		fileLimit *= 1000000.0;
		zco_set_max_file_occupancy(sdr, fileLimit, acct);
		writeMemo("[i] ZCO max file space changed.");
	}

	if (newHeapLimit != -1)	/*	Overriding the default.		*/
	{
		maxHeapLimit = (sdr_heap_size(sdr) / 100)
				* (100 - ION_SEQUESTERED);
		if (newHeapLimit > (maxHeapLimit / 1000000))
		{
			writeMemo("[i] New ZCO heap limit invalid!");
		}
		else
		{
			heapLimit = newHeapLimit;

			/*	Convert from MB to bytes.		*/

			heapLimit *= 1000000.0;
			zco_set_max_heap_occupancy(sdr, heapLimit, acct);
			writeMemo("[i] ZCO max heap changed.");
		}
	}

	/*	Revise occupancy ceiling and reserve as needed.		*/

	if (acct == ZcoOutbound)
	{
		fileLimit = zco_get_max_file_occupancy(sdr, ZcoOutbound);
		heapLimit = zco_get_max_heap_occupancy(sdr, ZcoOutbound);
		sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		iondb.occupancyCeiling = fileLimit + heapLimit;
		sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change bundle storage occupancy limit.", NULL);
	}

	oK(_forecastNeeded(1));
}

static void	manageInbound(int tokenCount, char **tokens)
{
	manageOccupancy(tokenCount, tokens, ZcoInbound);
}

static void	manageOutbound(int tokenCount, char **tokens)
{
	manageOccupancy(tokenCount, tokens, ZcoOutbound);
}

static void	manageSearch(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	unsigned int	newLimit;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newLimit = atoi(tokens[2]);
	sdr_set_search_limit(sdr, newLimit);
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

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	iondb.horizon = horizon;
	sdr_write(sdr, iondbObj, (char *) &iondb, sizeof(IonDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change congestion forecast horizon.", NULL);
	}

	oK(_forecastNeeded(1));
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

	CHKVOID(sdr_begin_xn(sdr));
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
	char	buffer[256];
	double	heapOccupancyInbound;
	double	heapSpaceMBInUseInbound;
	double	fileOccupancyInbound;
	double	fileSpaceMBInUseInbound;
	double	heapOccupancyOutbound;
	double	heapSpaceMBInUseOutbound;
	double	fileOccupancyOutbound;
	double	fileSpaceMBInUseOutbound;
	double	occupancyCeiling;	/*	In MBytes.		*/
	double	maxForecastOccupancy;	/*	In MBytes.		*/

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	heapOccupancyInbound = zco_get_heap_occupancy(sdr, ZcoInbound);
	fileOccupancyInbound = zco_get_file_occupancy(sdr, ZcoInbound);
	heapSpaceMBInUseInbound = heapOccupancyInbound / 1000000;
	fileSpaceMBInUseInbound = fileOccupancyInbound / 1000000;
	heapOccupancyOutbound = zco_get_heap_occupancy(sdr, ZcoOutbound);
	fileOccupancyOutbound = zco_get_file_occupancy(sdr, ZcoOutbound);
	heapSpaceMBInUseOutbound = heapOccupancyOutbound / 1000000;
	fileSpaceMBInUseOutbound = fileOccupancyOutbound / 1000000;
	GET_OBJ_POINTER(sdr, IonDB, iondb, getIonDbObject());
	occupancyCeiling = iondb->occupancyCeiling / 1000000;
	maxForecastOccupancy = iondb->maxForecastOccupancy / 1000000;
	sdr_exit_xn(sdr);
	isprintf(buffer, sizeof buffer, "current inbound heap %.2f MB, \
current inbound file space %.2f MB, current outbound heap %.2f MB, \
current outbound file space %.2f MB, limit %.2f MB, max forecast %.2f MB",
			heapSpaceMBInUseInbound, fileSpaceMBInUseInbound,
			heapSpaceMBInUseOutbound, fileSpaceMBInUseOutbound,
			occupancyCeiling, maxForecastOccupancy);
	printText(buffer);
}

static void	manageRegion(int tokenCount, char **tokens, int i)
{
	Sdr	sdr;
	Object	iondbObj;
	IonDB	iondb;
	char	buffer[128];
	vast	regionNbr;

	if (tokenCount == 2)
	{
		sdr = getIonsdr();
		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		isprintf(buffer, sizeof buffer, "home region is "
VAST_FIELDSPEC ", outer region is " VAST_FIELDSPEC ".",
				iondb.regions[0].regionNbr,
				iondb.regions[1].regionNbr);
		printText(buffer);
		return;
	}

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	/*	Manage node's own region membership.			*/

	regionNbr = strtovast(tokens[2]);
	if (regionNbr < -1)
	{
		SYNTAX_ERROR;
		return;
	}

	oK(ionManageRegion(i, regionNbr));
}

static void	managePassageway(int tokenCount, char **tokens)
{
	uvast	nodeNbr;
	vast	homeRegionNbr;
	vast	outerRegionNbr;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	nodeNbr	 = strtouvast(tokens[2]);
	homeRegionNbr = strtovast(tokens[3]);
	outerRegionNbr = strtovast(tokens[4]);
	oK(ionManagePassageway(nodeNbr, homeRegionNbr, outerRegionNbr));
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

	if (strcmp(tokens[1], "clocksync") == 0)
	{
		manageClockSync(tokenCount, tokens);
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

	if (strcmp(tokens[1], "inbound") == 0
	|| strcmp(tokens[1], "in") == 0)
	{
		manageInbound(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "outbound") == 0
	|| strcmp(tokens[1], "out") == 0)
	{
		manageOutbound(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "search") == 0)
	{
		manageSearch(tokenCount, tokens);
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

	if (strcmp(tokens[1], "home") == 0)
	{
		manageRegion(tokenCount, tokens, 0);
		return;
	}

	if (strcmp(tokens[1], "outer") == 0)
	{
		manageRegion(tokenCount, tokens, 1);
		return;
	}

	if (strcmp(tokens[1], "passageway") == 0)
	{
		managePassageway(tokenCount, tokens);
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

static int ion_is_up(int count, int max)
{
	while (count <= max && !rfx_system_is_started())
	{
		microsnooze(250000);
		count++;
	}

	if (count > max)		//ion system is not started
	{
		printText("ION system is not started");
		return 0;
	}

	//ion system is started

	printText("ION system is started");
	return 1;
}

static int	processLine(char *line, int lineLength, int *rc)
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
	vast		regionNbr;
	int		regionIdx;
	int		max = 0;
	int		count = 0;

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
			isprintf(buffer, sizeof buffer, "%s",
					IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			return initializeNode(tokenCount, tokens);

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
						printText("[?] RFX start hung \
up, abandoned.");
						break;
					}
				}

				/*	Advertise the reference time.	*/

				refTime = _referenceTime(NULL);
				ionReferenceTime(&refTime);
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

					currentTime = getCtime();
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

		case '^':
			if (ionAttach() == 0)
			{
				if (tokenCount != 2)
				{
					SYNTAX_ERROR;
				}
				else
				{
					regionNbr = strtovast(tokens[1]);
					regionIdx = ionPickRegion(regionNbr);
					if (regionIdx > 1)
					{
						printText("Node does not \
reside in this region.");
					}
					else
					{
						oK(_regionIdx(&regionIdx));
					}
				}
			}

			return 0;

		case 'a':
			if (ionAttach() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (ionAttach() == 0)
			{
				executeChange(tokenCount, tokens);
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
			while (count <= max && ionAttach() == -1)
			{
				microsnooze(250000);
				count++;
			}

			if (count > max)
			{
				//ion system is not started
				printText("ION system is not started");
				return 1;
			}

			//attached to ion system

			*rc = ion_is_up(count, max);
			return 1;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static int	runIonadmin(char *cmdFileName)
{
	int	rc = 0;
	time_t	currentTime;
	int	regionIdx = 0;			/*	Home region.	*/
	int	cmdFile;
	char	line[256];
	int	len;

	currentTime = getCtime();
	
	oK(_referenceTime(&currentTime));
	oK(_regionIdx(&regionIdx));		/*	Home region.	*/
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

			if (processLine(line, len, &rc))
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

				if (processLine(line, len, &rc))
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
		if (_forecastNeeded(0))
		{
			oK(pseudoshell("ionwarn"));
		}
	}

	printText("Stopping ionadmin.");
	ionDetach();
	return rc;
}

#if defined (ION_LWT)
int	ionadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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
		result = 1;
	}

	return result;
}

#ifdef STRSOE
int	ionadmin_pseudoshell(char *line)
{
	return pseudoshell(line);
}

int	ionadmin_processLine(char *line, int lineLength, int *rc)
{
	return processLine(line, lineLength, rc);
}

void	ionadmin_help(void)
{
	printUsage();
}
#endif
