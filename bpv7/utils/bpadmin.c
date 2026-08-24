/*

	bpadmin.c:	BP node adminstration interface.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bpP.h"
#include "cbr.h"
#include "crypto.h"
#include "csi.h"

#ifdef INPUT_HISTORY
#include "linenoise.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef STRSOE
#include <strsoe_bpadmin.h>
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

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of bpadmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage(void)
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION and crypto suite.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a scheme <scheme name> '<forwarder cmd>' '<admin app cmd>'");
	PUTS("\t   a endpoint <endpoint name> {q|x} ['<recv script>']");
	PUTS("\t   a protocol <protocol name> [<protocol class>]");
	PUTS("\t   a induct <protocol name> <duct name> '<CLI command>'");
	PUTS("\t   a outduct <protocol name> <duct name> '<CLO command>' [max \
payload length]");
	PUTS("\t   a plan <endpoint name> [<transmission rate>]");
	PUTS("\ta\tAttach an outduct to an egress plan");
	PUTS("\t   a planduct <endpoint name> <protocol name> <duct name>");
	PUTS("\tc\tChange");
	PUTS("\t   c scheme <scheme name> '<forwarder cmd>' '<admin app cmd>'");
	PUTS("\t   c endpoint <endpoint name> {q|x} ['<recv script>']");
	PUTS("\t   c induct <protocol name> <duct name> '<CLI command>'");
	PUTS("\t   c outduct <protocol name> <duct name> '<CLO command>' [max \
payload length");
	PUTS("\t   c plan <endpoint name> <transmission rate>");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} scheme <scheme name>");
	PUTS("\t   {d|i} endpoint <endpoint name>");
	PUTS("\t   {d|i} protocol <protocol name>");
	PUTS("\t   d induct <protocol name> <duct name>");
	PUTS("\t   i induct <protocol name> [<duct name>]");
	PUTS("\t   {d|i} outduct <protocol name> <duct name>");
	PUTS("\t   {d|i} plan <endpoint name>");
	PUTS("\td\tDetach an outduct from the egress plan that cites it");
	PUTS("\t   d planduct <protocol name> <duct name>");
	PUTS("\tl\tList");
	PUTS("\t   l scheme");
	PUTS("\t   l endpoint");
	PUTS("\t   l protocol");
	PUTS("\t   l induct [<protocol name>]");
	PUTS("\t   l outduct [<protocol name>]");
	PUTS("\t   l plan");
	PUTS("\tb\tBlock an egress plan");
	PUTS("\t   b plan <endpoint name>");
	PUTS("\tu\tUnblock an egress plan");
	PUTS("\t   u plan <endpoint name>");
	PUTS("\tg\tSet gateway 'via' EID for an egress plan");
	PUTS("\t   g plan <endpoint name> <via endpoint name>");
	PUTS("\tm\tManage");
	PUTS("\t   m heapmax <max database heap for any single acquisition>");
	PUTS("\t   m primarycrc <set CRC type for locally sourced primary block, 0, 1, or 2>");
	PUTS("\t   m payloadcrc <set CRC type for locally sourced payload block, 0, 1, or 2>");
	PUTS("\t   m maxcount <max value of bundle ID sequence number>");
	PUTS("\t   m bsl <local ipn eid> <key reg. file path name> <policy config. file pathname>");
	PUTS("\t   m custodymode <custody transfer mode: none | bibe | orangebook>");
	PUTS("\t   m srmode <status report mode: none | traditional | compressed | both>");
	PUTS("\t   m cbraggr <CRS limit> <CCS limit> <timeout seconds>");
	PUTS("\t      Aggregate limits: max bundles before sending signal (0=immediate)");
	PUTS("\t      Timeout: max seconds to wait before sending aggregated signal");
	PUTS("\t      Takes effect at runtime; 'l cbraggr' shows current limits.");
	PUTS("\t   m cbrretx <strategy: none|timer|signal> <interval seconds> <max retransmissions>");
	PUTS("\t      Strategy none: no automatic retransmission (default)");
	PUTS("\t      Strategy timer: retransmit after interval if no custody acceptance received");
	PUTS("\t      Strategy signal: retransmit immediately on custody refusal CCS");
	PUTS("\t      Max retransmissions: 0 means unlimited");
	PUTS("\t   m crebexpliciteid { 0 | 1 }");
	PUTS("\t      Include explicit source EID in CREB blocks (default: 0)");
	PUTS("\t   m crebreportto <eid> | -");
	PUTS("\t      Set or clear the default CREB report-to EID override");
	PUTS("\t      When set, all outgoing CREB blocks carry this EID as element 4");
	PUTS("\t      (arrayLen=5), directing relay/destination nodes to send CRS");
	PUTS("\t      to <eid> instead of the bundle's reportTo field.");
	PUTS("\t      Use '-' to clear the override (revert to bundle's reportTo).");
	PUTS("\t      'l crebreportto' shows the current override.");
	PUTS("\t   m cbrcounterwidth { 16 | 32 | 64 }");
	PUTS("\t      Set the sequence-counter wraparound width (bits) for new");
	PUTS("\t      counters, for interoperability with constrained peers.");
	PUTS("\t      Default 64. 'l cbrcounterwidth' shows the current value.");
	PUTS("\t   a cbraccept { custodian | source } <eid>");
	PUTS("\t      Add EID to custody acceptance whitelist");
	PUTS("\t      custodian: match on the node requesting custody (CTEB custodian EID)");
	PUTS("\t      source: match on the bundle's original source EID");
	PUTS("\t      Empty whitelist means accept from anyone (default)");
	PUTS("\t   d cbraccept { custodian | source } <eid>");
	PUTS("\t      Remove EID from custody acceptance whitelist");
	PUTS("\t   l cbraccept");
	PUTS("\t   a custodyreq <eid>");
	PUTS("\t      Add destination EID to auto custody-request policy list");
	PUTS("\t      Bundles sent to this EID will have custody transfer requested");
	PUTS("\t      automatically, even if the application does not request it");
	PUTS("\t   d custodyreq <eid>");
	PUTS("\t      Remove destination EID from auto custody-request policy list");
	PUTS("\t   l custodyreq");
	PUTS("\t      List all destination EIDs in the auto custody-request policy list");
	PUTS("\t   l crslog [<sender-eid>]");
	PUTS("\t      List received CRS history log, newest first");
	PUTS("\t      Optional sender-eid filters output to entries from that node");
	PUTS("\t   l custodybundle");
	PUTS("\t      List all bundles currently held in custody tracking");
	PUTS("\t      Shows seqId, seqNum, dest EID, acceptance time, last transmit, retransmit count");
	PUTS("\t   i custodybundle <sourceEid> <seqId> <seqNum>");
	PUTS("\t      Report custody status of a specific bundle");
	PUTS("\t      Status: pending (awaiting CCS) or not-found (released or not tracked)");
	PUTS("\tr\tRun another admin program");
	PUTS("\t   r '<admin command>'");
	PUTS("\ts\tStart");
	PUTS("\tt\tStartup-test");
	PUTS("\t   t [p [<timeout>]]");
	PUTS("\tx\tStop");
	PUTS("\t   {s|x}");
	PUTS("\t   {s|x} scheme <scheme name>");
	PUTS("\t   {s|x} protocol <protocol name>");
	PUTS("\t   {s|x} induct <protocol name> <duct name>");
	PUTS("\t   {s|x} outduct <protocol name> <duct name>");
	PUTS("\t   {s|x} plan <endpoint name>");
	PUTS("\tw\tWatch BP activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., acz~.  See man(5) for bprc.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text, ignored by the program>");
}

static void	initializeBp(int tokenCount, char **tokens)
{
	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("bpadmin can't attach to ION.", NULL);
		return;
	}

	if (bpInit() < 0)
	{
		putErrmsg("bpadmin can't initialize BP.", NULL);
		return;
	}
}

static int	attachToBp(void)
{
	if (bpAttach() < 0)
	{
		printText("[?] BP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	executeStart(int tokenCount, char **tokens)
{
	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStartScheme(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStartProtocol(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStartInduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStartOutduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
		}

		bpStartPlan(tokens[2]);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeStop(int tokenCount, char **tokens)
{
	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStopScheme(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStopProtocol(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStopInduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		bpStopOutduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
		}

		bpStopPlan(tokens[2]);
		return;
	}

	SYNTAX_ERROR;
}

static void	addCbrAccept(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	forCustodian;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[2], "custodian") == 0)
	{
		forCustodian = 1;
	}
	else if (strcmp(tokens[2], "source") == 0)
	{
		forCustodian = 0;
	}
	else
	{
		SYNTAX_ERROR;
		return;
	}

	if (cbr_addCustodyAccept(sdr, forCustodian, tokens[3]) < 0)
	{
		putErrmsg("Can't add CBR accept entry.", tokens[3]);
	}
	else
	{
		printText("CBR accept entry added.");
	}
}

static void	deleteCbrAccept(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	forCustodian;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[2], "custodian") == 0)
	{
		forCustodian = 1;
	}
	else if (strcmp(tokens[2], "source") == 0)
	{
		forCustodian = 0;
	}
	else
	{
		SYNTAX_ERROR;
		return;
	}

	if (cbr_removeCustodyAccept(sdr, forCustodian, tokens[3]) < 0)
	{
		putErrmsg("Can't remove CBR accept entry.", tokens[3]);
	}
	else
	{
		printText("CBR accept entry removed.");
	}
}

static void printCbrAcceptList(Sdr sdr, SdrObject list, char *label)
{
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	if (list == 0 || sdr_list_length(sdr, list) == 0)
	{
		isprintf(buf, sizeof buf, "%s (all)", label);
		printText(buf);
		return;
	}

	printText(label);
	for (elt = sdr_list_first(sdr, list); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		if (sdr_string_read(sdr, buf, obj) >= 0)
		{
			printText(buf);
		}
	}
}

static void	listCbrAccept(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject list;

	(void) tokenCount;
	(void) tokens;

	CHKVOID(sdr_begin_xn(sdr));
	list = cbr_getCustodyAcceptList(sdr, 1);
	printCbrAcceptList(sdr, list, "Accept by custodian:");
	list = cbr_getCustodyAcceptList(sdr, 0);
	printCbrAcceptList(sdr, list, "Accept by source:   ");
	sdr_exit_xn(sdr);
}

static void	addCustodyReq(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (cbr_addCustodyReq(sdr, tokens[2]) < 0)
	{
		putErrmsg("Can't add custody-req entry.", tokens[2]);
	}
	else
	{
		printText("Custody-req entry added.");
	}
}

static void	deleteCustodyReq(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (cbr_removeCustodyReq(sdr, tokens[2]) < 0)
	{
		putErrmsg("Can't remove custody-req entry.", tokens[2]);
	}
	else
	{
		printText("Custody-req entry removed.");
	}
}

static void	listCustodyReq(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject list;
	SdrObject elt;
	SdrObject obj;
	char	buf[SDRSTRING_BUFSZ];

	(void) tokenCount;
	(void) tokens;

	CHKVOID(sdr_begin_xn(sdr));
	list = cbr_getCustodyReqList(sdr);
	if (list == 0 || sdr_list_length(sdr, list) == 0)
	{
		printText("Custody-req list: (none)");
	}
	else
	{
		printText("Custody-req destinations:");
		for (elt = sdr_list_first(sdr, list); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			if (sdr_string_read(sdr, buf, obj) >= 0)
			{
				printText(buf);
			}
		}
	}

	sdr_exit_xn(sdr);
}

static void	listCrsLog(int tokenCount, char **tokens)
{
	Sdr			sdr = getIonsdr();
	SdrObject		list;
	SdrObject		elt;
	SdrObject		recObj;
	ReceivedCrsRecord	rec;
	char			eidBuf[SDRSTRING_BUFSZ];
	char			timeBuf[32];
	struct tm		*tm;
	time_t			t;
	char			line[256];
	const char		*filterEid;
	int			printed = 0;

	filterEid = (tokenCount >= 3) ? tokens[2] : NULL;

	CHKVOID(sdr_begin_xn(sdr));
	list = cbr_getCrsHistoryList(sdr);
	if (list == 0 || sdr_list_length(sdr, list) == 0)
	{
		printText("CRS history: (none)");
		sdr_exit_xn(sdr);
		return;
	}

	/*	Walk newest-first (from list tail).			*/
	for (elt = sdr_list_last(sdr, list); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		recObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rec, recObj, sizeof(ReceivedCrsRecord));

		if (rec.senderEid)
		{
			if (sdr_string_read(sdr, eidBuf, rec.senderEid) < 0)
			{
				istrcpy(eidBuf, "?", sizeof eidBuf);
			}
		}
		else
		{
			istrcpy(eidBuf, "(unknown)", sizeof eidBuf);
		}

		if (filterEid && *filterEid
				&& strcmp(eidBuf, filterEid) != 0)
		{
			continue;
		}

		t = rec.receivedAt;
		tm = gmtime(&t);
		strftime(timeBuf, sizeof timeBuf, "%Y-%m-%dT%H:%M:%SZ", tm);
		isprintf(line, sizeof line,
				"  %s  from %-40s  status=%d  bundles=%llu",
				timeBuf, eidBuf,
				rec.statusCode,
				(unsigned long long) rec.bundleCount);
		printText(line);
		printed = 1;
	}

	if (!printed)
	{
		printText("CRS history: (none matching filter)");
	}

	sdr_exit_xn(sdr);
}

static void	printCustodyBundle(CbrCustodyInfo *info, void *userData)
{
	char		acceptBuf[32];
	char		xmitBuf[32];
	struct tm	*tm;
	time_t		t;
	char		line[320];

	(void) userData;

	t = info->custodyAccepted;
	tm = gmtime(&t);
	strftime(acceptBuf, sizeof acceptBuf, "%Y-%m-%dT%H:%M:%SZ", tm);
	t = info->lastTransmit;
	if (t == 0)
	{
		istrcpy(xmitBuf, "(never)", sizeof xmitBuf);
	}
	else
	{
		tm = gmtime(&t);
		strftime(xmitBuf, sizeof xmitBuf, "%Y-%m-%dT%H:%M:%SZ", tm);
	}

	isprintf(line, sizeof line,
			"  seqId=%-6llu seqNum=%-6llu dest=%-40s accepted=%s lastXmit=%s retx=%d",
			(unsigned long long) info->seqId,
			(unsigned long long) info->seqNum,
			info->destEid, acceptBuf, xmitBuf,
			info->retransmitCount);
	printText(line);
}

static void	listCustodyBundles(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	count;
	char	summary[64];

	(void) tokenCount;
	(void) tokens;

	count = cbr_listCustodyBundles(sdr, printCustodyBundle, NULL);
	if (count < 0)
	{
		putErrmsg("Can't list custody bundles.", NULL);
		return;
	}

	if (count == 0)
	{
		printText("Custody bundles: (none)");
		return;
	}

	isprintf(summary, sizeof summary, "Custody bundles: %d total", count);
	printText(summary);
}

static void	infoCustodyBundle(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	uvast		seqId;
	uvast		seqNum;
	int		status;
	const char	*statusName;
	char		line[256];

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	seqId = strtouvast(tokens[3]);
	seqNum = strtouvast(tokens[4]);
	status = cbr_getCustodyStatus(sdr, tokens[2], seqId, seqNum);
	switch (status)
	{
	case CBR_CUSTODY_STATUS_PENDING:
		statusName = "pending";
		break;

	case CBR_CUSTODY_STATUS_NOT_FOUND:
		statusName = "not-found (released or never tracked)";
		break;

	default:
		statusName = "unknown";
		break;
	}

	isprintf(line, sizeof line,
			"Custody status for %s seqId=%llu seqNum=%llu: %s",
			tokens[2],
			(unsigned long long) seqId,
			(unsigned long long) seqNum,
			statusName);
	printText(line);
}

static void	manageCrebReportTo(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	char	buf[MAX_EID_LEN];
	char	line[MAX_EID_LEN + 40];

	if (tokenCount == 2)
	{
		if (cbr_getCrebReportTo(sdr, buf, sizeof buf) < 0)
		{
			putErrmsg("Can't read CREB report-to override.", NULL);
			return;
		}

		if (buf[0] == '\0')
		{
			printText("CREB default report-to: (none -- use bundle's reportTo)");
		}
		else
		{
			isprintf(line, sizeof line,
					"CREB default report-to: %s", buf);
			printText(line);
		}

		return;
	}

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[2], "-") == 0)
	{
		if (cbr_setCrebReportTo(sdr, NULL) < 0)
		{
			putErrmsg("Can't clear CREB report-to override.", NULL);
			return;
		}

		printText("CREB report-to override cleared.");
		return;
	}

	if (cbr_setCrebReportTo(sdr, tokens[2]) < 0)
	{
		putErrmsg("Can't set CREB report-to override.", tokens[2]);
		return;
	}

	printText("CREB report-to override set.");
}

static void	manageCbrCounterWidth(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	maxValue;
	int	width;
	char	line[80];

	if (tokenCount == 2)
	{
		maxValue = cbr_getCounterMaxValue();
		switch (maxValue)
		{
		case CBR_COUNTER_MAX_16BIT:
			width = 16;
			break;

		case CBR_COUNTER_MAX_32BIT:
			width = 32;
			break;

		default:
			width = 64;
		}

		isprintf(line, sizeof line,
				"CBR counter width: %d-bit (max " UVAST_FIELDSPEC ")",
				width, maxValue);
		printText(line);
		return;
	}

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	width = atoi(tokens[2]);
	switch (width)
	{
	case 16:
		maxValue = CBR_COUNTER_MAX_16BIT;
		break;

	case 32:
		maxValue = CBR_COUNTER_MAX_32BIT;
		break;

	case 64:
		maxValue = CBR_COUNTER_MAX_64BIT;
		break;

	default:
		printText("[?] Unknown counter width; use '16', '32', or '64'.");
		return;
	}

	if (cbr_setCounterMaxValue(sdr, maxValue) < 0)
	{
		putErrmsg("Can't set CBR counter width.", NULL);
	}
	else
	{
		printText("CBR counter width set.");
	}
}

static void	manageCbrAggr(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	unsigned int	crsLimit;
	unsigned int	ccsLimit;
	unsigned int	timeout;
	char		line[128];

	if (tokenCount == 2)
	{
		crsLimit = ccsLimit = timeout = 0;
		if (cbr_getConfig(sdr, &crsLimit, &ccsLimit, &timeout) < 0)
		{
			putErrmsg("Can't read CBR aggregate config.", NULL);
			return;
		}

		isprintf(line, sizeof line, "CBR aggregate config: CRS limit %u, "
				"CCS limit %u, timeout %u s",
				crsLimit, ccsLimit, timeout);
		printText(line);
		return;
	}

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	crsLimit = atoi(tokens[2]);
	ccsLimit = atoi(tokens[3]);
	timeout = atoi(tokens[4]);

	if (cbr_configure(sdr, crsLimit, ccsLimit, timeout) < 0)
	{
		putErrmsg("Can't set CBR aggregate config.", NULL);
	}
	else
	{
		printText("CBR aggregate config set.");
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	char		*script;
	BpRecvRule	rule;
	int		protocolClass = 0;
	unsigned int	maxPayloadLength;
	unsigned int	xmitRate;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		addScheme(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		switch (tokenCount)
		{
		case 5:
			script = tokens[4];
			break;

		case 4:
			script = NULL;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		if (*(tokens[3]) == 'q')
		{
			rule = EnqueueBundle;
		}
		else
		{
			rule = DiscardBundle;
		}

		addEndpoint(tokens[2], rule, script);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		if (tokenCount > 4)	/*	Deprecated format.	*/
		{
			if (tokenCount > 6)
			{
				SYNTAX_ERROR;
				return;
			}

			/*	Ignore tokens 3 and 4.			*/

			if (tokenCount == 6)
			{
				protocolClass = atol(tokens[5]);
			}
		}
		else			/*	Current format.		*/
		{
			if (tokenCount < 3)
			{
				SYNTAX_ERROR;
				return;
			}

			if (tokenCount == 4)
			{
				protocolClass = atol(tokens[3]);
			}
		}


		addProtocol(tokens[2], protocolClass);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		addInduct(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		switch (tokenCount)
		{
		case 6:
			maxPayloadLength = strtoul(tokens[5], NULL, 0);
			break;

		case 5:
			maxPayloadLength = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		addOutduct(tokens[2], tokens[3], tokens[4], maxPayloadLength);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		switch (tokenCount)
		{
		case 4:
			xmitRate = strtoul(tokens[3], NULL, 0);
			break;

		case 3:
			xmitRate = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		addPlan(tokens[2], xmitRate);
		return;
	}

	if (strcmp(tokens[1], "planduct") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		findOutduct(tokens[3], tokens[4], &vduct, &vductElt);
		if (vductElt == 0)
		{
			printText("[?] Unknown outduct.");
			return;
		}

		attachPlanDuct(tokens[2], vduct->outductElt);
		return;
	}

	if (strcmp(tokens[1], "cbraccept") == 0)
	{
		addCbrAccept(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "custodyreq") == 0)
	{
		addCustodyReq(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	char		*script;
	BpRecvRule	rule;
	unsigned int	maxPayloadLen;
	unsigned int	xmitRate;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		updateScheme(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		switch (tokenCount)
		{
		case 5:
			script = tokens[4];
			break;

		case 4:
			script = NULL;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		if (*(tokens[3]) == 'q')
		{
			rule = EnqueueBundle;
		}
		else
		{
			rule = DiscardBundle;
		}

		updateEndpoint(tokens[2], rule, script);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		updateInduct(tokens[2], tokens[3], tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		switch (tokenCount)
		{
		case 6:
			maxPayloadLen = strtoul(tokens[5], NULL, 0);
			break;

		case 5:
			maxPayloadLen = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		updateOutduct(tokens[2], tokens[3], tokens[4], maxPayloadLen);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		xmitRate = strtoul(tokens[3], NULL, 0);
		updatePlan(tokens[2], xmitRate);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	VOutduct	*vduct;
	PsmAddress	vductElt;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removeScheme(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removeEndpoint(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removeProtocol(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		removeInduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		removeOutduct(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		removePlan(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "planduct") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		findOutduct(tokens[2], tokens[3], &vduct, &vductElt);
		if (vductElt == 0)
		{
			printText("[?] Unknown outduct.");
			return;
		}

		detachPlanDuct(vduct->outductElt);
		return;
	}

	if (strcmp(tokens[1], "cbraccept") == 0)
	{
		deleteCbrAccept(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "custodyreq") == 0)
	{
		deleteCustodyReq(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	printScheme(VScheme *vscheme)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Scheme, scheme);
	char	fwdCmdBuffer[SDRSTRING_BUFSZ];
	char	*fwdCmd;
	char	admAppCmdBuffer[SDRSTRING_BUFSZ];
	char	*admAppCmd;
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, Scheme, scheme, sdr_list_data(sdr,
			vscheme->schemeElt));
	if (sdr_string_read(sdr, fwdCmdBuffer, scheme->fwdCmd) < 0)
	{
		fwdCmd = "?";
	}
	else
	{
		fwdCmd = fwdCmdBuffer;
	}

	if (sdr_string_read(sdr, admAppCmdBuffer, scheme->admAppCmd) < 0)
	{
		admAppCmd = "?";
	}
	else
	{
		admAppCmd = admAppCmdBuffer;
	}

	isprintf(buffer, sizeof buffer, "%.8s\tfwdpid: %d cmd: %.256s  \
admpid: %d cmd %.256s", scheme->name, vscheme->fwdPid, fwdCmd,
			vscheme->admAppPid, admAppCmd);
	printText(buffer);
}

static void	infoScheme(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	elt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findScheme(tokens[2], &vscheme, &elt);
	if (elt == 0)
	{
		printText("[?] Unknown scheme.");
	}
	else
	{
		printScheme(vscheme);
	}

	sdr_exit_xn(sdr);
}

static void	printEndpoint(VEndpoint *vpoint)
{
	Sdr		sdr = getIonsdr();
		OBJ_POINTER(Endpoint, endpoint);
		OBJ_POINTER(Scheme, scheme);
	char	buffer[512];
	char	recvRule;
	char	recvScriptBuffer[SDRSTRING_BUFSZ];
	char	*recvScript = recvScriptBuffer;

	GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
			vpoint->endpointElt));
	GET_OBJ_POINTER(sdr, Scheme, scheme, endpoint->scheme);
	if (endpoint->recvRule == EnqueueBundle)
	{
		recvRule = 'q';
	}
	else
	{
		recvRule = 'x';
	}

	if (endpoint->recvScript == 0)
	{
		recvScriptBuffer[0] = '\0';
	}
	else
	{
		if (sdr_string_read(sdr, recvScriptBuffer, endpoint->recvScript)
			       	< 0)
		{
			recvScript = "?";
		}
	}

	isprintf(buffer, sizeof buffer, "%.8s:%.128s  %d\trule: %c  script: \
%.256s", scheme->name, endpoint->nss, vpoint->appPid, recvRule, recvScript);
	printText(buffer);
}

static void	infoEndpoint(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	VEndpoint	*vpoint;
	PsmAddress	elt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (parseEidString(tokens[2], &metaEid, &vscheme, &vschemeElt) == 0)
	{
		printText("[?] Endpoint ID unintelligible.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findEndpoint(tokens[2], &metaEid, NULL, &vpoint, &elt);
	if (elt == 0)
	{
		printText("[?] Unknown endpoint.");
	}
	else
	{
		printEndpoint(vpoint);
	}

	sdr_exit_xn(sdr);
	clearMetaEid(&metaEid);
}

static void	printProtocol(ClProtocol *protocol)
{
	printText(protocol->name);
}

static void	infoProtocol(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	elt;
	ClProtocol	clpbuf;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	fetchProtocol(tokens[2], &clpbuf, &elt);
	if (elt == 0)
	{
		printText("[?] Unknown protocol.");
	}
	else
	{
		printProtocol(&clpbuf);
	}

	sdr_exit_xn(sdr);
}

static void	printInduct(VInduct *vduct)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Induct, duct);
		OBJ_POINTER(ClProtocol, clp);
	char	cliCmdBuffer[SDRSTRING_BUFSZ];
	char	*cliCmd;
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, Induct, duct, sdr_list_data(sdr,
			vduct->inductElt));
	GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);
	if (sdr_string_read(sdr, cliCmdBuffer, duct->cliCmd) < 0)
	{
		cliCmd = "?";
	}
	else
	{
		cliCmd = cliCmdBuffer;
	}

	isprintf(buffer, sizeof buffer, "%.8s/%.256s\tpid: %d  cmd: %.256s",
			clp->name, duct->name, vduct->cliPid, cliCmd);
	printText(buffer);
}

static void	infoInduct(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VInduct		*vduct;
	PsmAddress	elt;

	if (tokenCount != 3 && tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findInduct(tokens[2], tokenCount == 4 ? tokens[3] : NULL, &vduct, &elt);
	if (elt == 0)
	{
		printText("[?] Unknown induct.");
	}
	else
	{
		printInduct(vduct);
	}

	sdr_exit_xn(sdr);
}

static void	printOutduct(VOutduct *vduct)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(Outduct, duct);
		OBJ_POINTER(ClProtocol, clp);
	char	cloCmdBuffer[SDRSTRING_BUFSZ];
	char	*cloCmd;
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, Outduct, duct, sdr_list_data(sdr,
			vduct->outductElt));
	GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);

	if (duct->cloCmd == 0)
	{
		cloCmd = "?";
	}
	else if (sdr_string_read(sdr, cloCmdBuffer, duct->cloCmd) < 0)
	{
		cloCmd = "?";
	}
	else
	{
		cloCmd = cloCmdBuffer;
	}

	isprintf(buffer, sizeof buffer,
			"%.8s/%.256s\tpid: %d  cmd: %.256s max: %u", clp->name,
			duct->name, vduct->cloPid, cloCmd, duct->maxPayloadLen);
	printText(buffer);
}

static void	infoOutduct(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VOutduct	*vduct;
	PsmAddress	elt;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findOutduct(tokens[2], tokens[3], &vduct, &elt);
	if (elt == 0)
	{
		printText("[?] Unknown outduct.");
	}
	else
	{
		printOutduct(vduct);
	}

	sdr_exit_xn(sdr);
}

static void	printPlan(VPlan *vplan)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BpPlan, plan);
	char	buffer[1024];

	GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, vplan->planElt));
	isprintf(buffer, sizeof buffer, "%.256s\tpid: %d xmit rate: %u",
			plan->neighborEid, vplan->clmPid, plan->nominalRate);
	printText(buffer);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VPlan		*vplan;
	PsmAddress	vplanElt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	findPlan(tokens[2], &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		printText("[?] Unknown plan.");
	}
	else
	{
		printPlan(vplan);
	}

	sdr_exit_xn(sdr);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (strcmp(tokens[1], "scheme") == 0)
	{
		infoScheme(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		infoEndpoint(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		infoProtocol(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		infoInduct(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		infoOutduct(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		infoPlan(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "custodybundle") == 0)
	{
		infoCustodyBundle(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listSchemes(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VScheme		*vscheme;

	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, elt));
		printScheme(vscheme);
	}

	sdr_exit_xn(sdr);
}

static void	listEndpointsForScheme(VScheme *vscheme)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VEndpoint	*vpoint;

	for (elt = sm_list_first(ionwm, vscheme->endpoints); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, elt));
		printEndpoint(vpoint);
	}
}

static void	listEndpoints(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	VScheme		*vscheme;
	PsmAddress	elt;

	switch (tokenCount)
	{
	case 2:
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
				elt = sm_list_next(ionwm, elt))
		{
			vscheme = (VScheme *) psp(ionwm,
					sm_list_data(ionwm, elt));
			listEndpointsForScheme(vscheme);
		}

		sdr_exit_xn(sdr);
		break;

	case 3:
		CHKVOID(sdr_begin_xn(sdr));
		findScheme(tokens[2], &vscheme, &elt);
		if (elt == 0)
		{
			printText("[?] Unknown scheme.");
		}
		else
		{
			listEndpointsForScheme(vscheme);
		}

		sdr_exit_xn(sdr);
		break;

	default:
		SYNTAX_ERROR;
	}
}

static void	listProtocols(int tokenCount, char **tokens)
{
	/* Parameter intentionally unused. */
	(void)tokens;

	Sdr	sdr = getIonsdr();
	SdrObject elt;
		OBJ_POINTER(ClProtocol, clp);

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, ClProtocol, clp, sdr_list_data(sdr, elt));
		printProtocol(clp);
	}

	sdr_exit_xn(sdr);
}

static void	listInductsForProtocol(char *protocolName)
{
	PsmPartition	ionwm = getIonwm();
	VInduct		*vduct;
	PsmAddress	elt;

	for (elt = sm_list_first(ionwm, (getBpVdb())->inducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, elt));
		if (strcmp(vduct->protocolName, protocolName) == 0)
		{
			printInduct(vduct);
		}
	}
}

static void	listInducts(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	ClProtocol	clpbuf;
	SdrObject	elt;

	switch (tokenCount)
	{
	case 2:
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, (getBpConstants())->protocols);
				elt; elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &clpbuf,
				sdr_list_data(sdr, elt), sizeof(ClProtocol));
			listInductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	case 3:
		CHKVOID(sdr_begin_xn(sdr));
		fetchProtocol(tokens[2], &clpbuf, &elt);
		if (elt == 0)
		{
			printText("[?] Unknown protocol.");
		}
		else
		{
			listInductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	default:
		SYNTAX_ERROR;
	}
}

static void	listOutductsForProtocol(char *protocolName)
{
	PsmPartition	ionwm = getIonwm();
	VOutduct	*vduct;
	PsmAddress	elt;

	for (elt = sm_list_first(ionwm, (getBpVdb())->outducts); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, elt));
		if (strcmp(vduct->protocolName, protocolName) == 0)
		{
			printOutduct(vduct);
		}
	}
}

static void	listOutducts(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	ClProtocol	clpbuf;
	SdrObject	elt;

	switch (tokenCount)
	{
	case 2:
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, (getBpConstants())->protocols);
				elt; elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &clpbuf,
				sdr_list_data(sdr, elt), sizeof(ClProtocol));
			listOutductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	case 3:
		CHKVOID(sdr_begin_xn(sdr));
		fetchProtocol(tokens[2], &clpbuf, &elt);
		if (elt == 0)
		{
			printText("[?] Unknown protocol.");
		}
		else
		{
			listOutductsForProtocol(clpbuf.name);
		}

		sdr_exit_xn(sdr);
		break;

	default:
		SYNTAX_ERROR;
	}
}

static void	listPlans(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VPlan		*vplan;

	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, (getBpVdb())->plans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vplan = (VPlan *) psp(ionwm, sm_list_data(ionwm, elt));
		printPlan(vplan);
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

	if (strcmp(tokens[1], "scheme") == 0)
	{
		listSchemes(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "endpoint") == 0)
	{
		listEndpoints(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "protocol") == 0)
	{
		listProtocols(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "induct") == 0)
	{
		listInducts(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "outduct") == 0)
	{
		listOutducts(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		listPlans(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "cbraccept") == 0)
	{
		listCbrAccept(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "custodyreq") == 0)
	{
		listCustodyReq(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "crslog") == 0)
	{
		listCrsLog(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "custodybundle") == 0)
	{
		listCustodyBundles(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "crebreportto") == 0)
	{
		manageCrebReportTo(2, tokens);
		return;
	}

	if (strcmp(tokens[1], "cbrcounterwidth") == 0)
	{
		manageCbrCounterWidth(2, tokens);
		return;
	}

	if (strcmp(tokens[1], "cbraggr") == 0)
	{
		manageCbrAggr(2, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeBlock(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Block what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		oK(bpBlockPlan(tokens[2]));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeUnblock(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Unblock what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		oK(bpUnblockPlan(tokens[2]));
		return;
	}

	SYNTAX_ERROR;
}

static void	executeGateway(int tokenCount, char **tokens)
{
	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	setPlanViaEid(tokens[2], tokens[3]);
}

static void	manageHeapmax(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	bpdbObj = getBpDbObject();
	BpDB		bpdb;
	unsigned int	heapmax;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	heapmax = strtoul(tokens[2], NULL, 0);
	if (heapmax < 560)
	{
		printText("[?] heapmax must be at least 560.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.maxAcqInHeap = heapmax;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxAcqInHeap.", NULL);
	}
}

static void	managePrimaryCrc(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	bpdbObj = getBpDbObject();
	BpDB		bpdb;
	BpCrcType	crcType;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	crcType = strtoul(tokens[2], NULL, 0);
	if (crcType < 0 || crcType > 2)
	{
		printText("[?] Primary block CRC type must 0, 1 or 2.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.sourcePrimaryCrcType = crcType;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change primary block CRC type.", NULL);
	}
}

static void	managePayloadCrc(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	bpdbObj = getBpDbObject();
	BpDB		bpdb;
	BpCrcType	crcType;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	crcType = strtoul(tokens[2], NULL, 0);
	if (crcType < 0 || crcType > 2)
	{
		printText("[?] Payload CRC type must be 0, 1 or 2.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.sourcePayloadCrcType = crcType;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change payload block CRC type.", NULL);
	}
}


static void	manageMaxcount(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	bpdbObj = getBpDbObject();
	BpDB		bpdb;
	unsigned int	maxcount;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	maxcount = strtoul(tokens[2], NULL, 0);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.maxBundleCount = maxcount;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxBundleCount.", NULL);
	}
}

#if USING_BSL
static void	manageBSL(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	SdrObject bpdbObj = getBpDbObject();
	char	*localEid;
	char	*keyFile;
	char	*policyFile;
	BpDB	bpdb;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr);
	CHKVOID(bpdbObj);
	localEid = tokens[2];
	keyFile = tokens[3];
	policyFile = tokens[4];
	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	if (bpdb.bslLocalEid)
	{
		sdr_free(sdr, bpdb.bslLocalEid);
	}

	if (bpdb.bslKeyFile)
	{
		sdr_free(sdr, bpdb.bslKeyFile);
	}

	if (bpdb.bslPolicyFile)
	{
		sdr_free(sdr, bpdb.bslPolicyFile);
	}

	bpdb.bslLocalEid = sdr_string_create(sdr, localEid);
	bpdb.bslKeyFile = sdr_string_create(sdr, keyFile);
	bpdb.bslPolicyFile = sdr_string_create(sdr, policyFile);
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update BSL config parameters.", NULL);
	}
}
#endif

static void	manageCustodyMode(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	mode;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[2], "none") == 0)
	{
		mode = BP_CUSTODY_NONE;
	}
	else if (strcmp(tokens[2], "bibe") == 0)
	{
		mode = BP_CUSTODY_BIBE;
	}
	else if (strcmp(tokens[2], "orangebook") == 0)
	{
		mode = BP_CUSTODY_ORANGEBOOK;
	}
	else
	{
		printText("Custody mode must be 'none', 'bibe', or 'orangebook'.");
		return;
	}

	if (cbr_setCustodyMode(sdr, mode) < 0)
	{
		putErrmsg("Can't set custody mode.", NULL);
	}
	else
	{
		printText("Custody mode set.");
	}
}

static void	manageSrMode(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	mode;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[2], "none") == 0)
	{
		mode = BP_SR_MODE_NONE;
	}
	else if (strcmp(tokens[2], "traditional") == 0)
	{
		mode = BP_SR_MODE_TRADITIONAL;
	}
	else if (strcmp(tokens[2], "compressed") == 0)
	{
		mode = BP_SR_MODE_COMPRESSED;
	}
	else if (strcmp(tokens[2], "both") == 0)
	{
		mode = BP_SR_MODE_BOTH;
	}
	else
	{
		printText("Status report mode must be 'none', 'traditional', 'compressed', or 'both'.");
		return;
	}

	if (cbr_setStatusReportMode(sdr, mode) < 0)
	{
		putErrmsg("Can't set status report mode.", NULL);
	}
	else
	{
		printText("Status report mode set.");
		if (bp_agent_is_started())
		{
			printText("Note: status report mode is cached by \
running daemons; restart the node for this change to take effect.");
		}
	}
}

static void	manageCrebExplicitEid(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	enable;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	enable = atoi(tokens[2]);
	if (cbr_setCrebExplicitEid(sdr, enable) < 0)
	{
		putErrmsg("Can't set CREB explicit EID mode.", NULL);
	}
	else
	{
		printText("CREB explicit EID mode set.");
	}
}

static void	manageCbrRetx(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	int		strategy;
	unsigned int	intervalSec;
	unsigned int	maxRetransmissions;

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[2], "none") == 0)
	{
		strategy = CBR_RETX_NONE;
	}
	else if (strcmp(tokens[2], "timer") == 0)
	{
		strategy = CBR_RETX_TIMER;
	}
	else if (strcmp(tokens[2], "signal") == 0)
	{
		strategy = CBR_RETX_SIGNAL;
	}
	else
	{
		printText("[?] Unknown retransmission strategy; use 'none', 'timer', or 'signal'.");
		return;
	}

	intervalSec = (unsigned int) atoi(tokens[3]);
	maxRetransmissions = (unsigned int) atoi(tokens[4]);

	if (cbr_configureRetransmission(sdr, strategy, intervalSec,
			maxRetransmissions) < 0)
	{
		putErrmsg("Can't set CBR retransmission config.", NULL);
	}
	else
	{
		printText("CBR retransmission config set.");
	}
}

static void	manageCrsMaxLog(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	unsigned int	max;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	max = (unsigned int) atoi(tokens[2]);
	if (cbr_setCrsHistoryMax(sdr, max) < 0)
	{
		putErrmsg("Can't set CRS history max.", NULL);
	}
	else
	{
		printText("CRS history max set.");
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "primarycrc") == 0)
	{
		managePrimaryCrc(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "payloadcrc") == 0)
	{
		managePayloadCrc(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "heapmax") == 0)
	{
		manageHeapmax(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxcount") == 0)
	{
		manageMaxcount(tokenCount, tokens);
		return;
	}

#if USING_BSL
	if (strcmp(tokens[1], "bsl") == 0)
	{
		manageBSL(tokenCount, tokens);
		return;
	}
#endif

	if (strcmp(tokens[1], "custodymode") == 0)
	{
		manageCustodyMode(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "srmode") == 0)
	{
		manageSrMode(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "cbraggr") == 0)
	{
		manageCbrAggr(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "crebexpliciteid") == 0)
	{
		manageCrebExplicitEid(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "cbrretx") == 0)
	{
		manageCbrRetx(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "crsmaxlog") == 0)
	{
		manageCrsMaxLog(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "crebreportto") == 0)
	{
		manageCrebReportTo(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "cbrcounterwidth") == 0)
	{
		manageCbrCounterWidth(tokenCount, tokens);
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
		printText("[?] pseudoshell failed.");
	}
	else
	{
		snooze(1);	/*	Give script time to finish.	*/
	}
}

static void	noteWatchValue(void)
{
	BpVdb	*vdb = getBpVdb();
	Sdr	sdr = getIonsdr();
	SdrObject dbObj = getBpDbObject();
	BpDB	db;

	if (vdb != NULL && dbObj != 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &db, dbObj, sizeof(BpDB));
		db.watching = vdb->watching;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(BpDB));
		oK(sdr_end_xn(sdr));
	}
}

static void	switchWatch(int tokenCount, char **tokens)
{
	BpVdb	*vdb = getBpVdb();
	char	buffer[80];
	char	*cursor;

	if (tokenCount < 2)
	{
		printText("Switch watch in what way?");
		return;
	}

	if (strcmp(tokens[1], "1") == 0)
	{
		vdb->watching |= WATCH_BP;
		return;
	}

	vdb->watching &= ~(WATCH_BP);
	if (strcmp(tokens[1], "0") == 0)
	{
		return;
	}

	cursor = tokens[1];
	while (*cursor)
	{
		switch (*cursor)
		{
		case 'a':
			vdb->watching |= WATCH_a;
			break;

		case 'b':
			vdb->watching |= WATCH_b;
			break;

		case 'c':
			vdb->watching |= WATCH_c;
			break;

		case 'y':
			vdb->watching |= WATCH_y;
			break;

		case 'z':
			vdb->watching |= WATCH_z;
			break;

		case '~':
			vdb->watching |= WATCH_abandon;
			break;

		case '!':
			vdb->watching |= WATCH_expire;
			break;

		case '#':
			vdb->watching |= WATCH_clfail;
			break;

		case 'j':
			vdb->watching |= WATCH_limbo;
			break;

		case 'k':
			vdb->watching |= WATCH_delimbo;
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

static int bp_is_up(int count, int max)
{
	while (count <= max && !bp_agent_is_started())
	{
		microsnooze(250000);
		count++;
	}

	if (count > max)		//bp agent is not started
	{
		printText("BP agent is not started");
		return 0;
	}

	//bp agent is started

	printText("BP agent is started");
	return 1;
}

static int	processLine(char *line, int lineLength, int *rc)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	struct timeval	done_time;
	struct timeval	cur_time;
	char		buffer[80];

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
			isprintf(buffer, sizeof buffer,
					"%s compiled with crypto suite: %s",
					IONVERSIONNUMBER, CSI_SUITE_NAME);
			printText(buffer);
			return 0;

		case '1':
			initializeBp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToBp() == 0)
			{
				if (tokenCount > 1)
				{
					executeStart(tokenCount, tokens);
				}
				else
				{
					if (bpStart() < 0)
					{
						putErrmsg("Can't start BP.",
								NULL);
						return 0;
					}
				}

				/* Wait for bp to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (bp_agent_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
						done_time.tv_sec
						&& cur_time.tv_usec >=
						done_time.tv_usec)
					{
						printText("[?] BP start hung\
 up, abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (attachToBp() == 0)
			{
				if (tokenCount > 1)
				{
					executeStop(tokenCount, tokens);
				}
				else
				{
					bpStop();
				}
			}

			return 0;

		case 'a':
			if (attachToBp() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attachToBp() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToBp() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToBp() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToBp() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'b':
			if (attachToBp() == 0)
			{
				executeBlock(tokenCount, tokens);
			}

			return 0;

		case 'u':
			if (attachToBp() == 0)
			{
				executeUnblock(tokenCount, tokens);
			}

			return 0;

		case 'g':
			if (attachToBp() == 0)
			{
				executeGateway(tokenCount, tokens);
			}

			return 0;

		case 'm':
			if (attachToBp() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'r':
			executeRun(tokenCount, tokens);
			return 0;

		case 'w':
			if (attachToBp() == 0)
			{
				switchWatch(tokenCount, tokens);
				noteWatchValue();
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
			while (count <= max && attachToBp() == -1)
			{
				microsnooze(250000);
				count++;
			}

			if (count > max)
			{
				//bp agent is not started
				printText("BP agent is not started");
				return 1;
			}

			//attached to bp system

			*rc = bp_is_up(count, max);
			return 1;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	bpadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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
		if (attachToBp() == 0)
		{
			bpStop();
		}
	}
	else if (strcmp(cmdFileName, "!") == 0)	/*	Resume.		*/
	{
		if (attachToBp() == 0)
		{
			bpStart();
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

				if (processLine(line, len, &rc))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping bpadmin.");
	ionDetach();
	return rc;
}

#ifdef STRSOE
int	bpadmin_processLine(char *line, int lineLength, int *rc)
{
	return processLine(line, lineLength, rc);
}

void	bpadmin_help(void)
{
	printUsage();
}
#endif
