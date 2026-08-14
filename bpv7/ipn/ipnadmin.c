/*
	ipnadmin.c:	BP routing adminstration interface for
			the IPN endpoint ID scheme.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "ipnfw.h"

#ifdef INPUT_HISTORY
#include "linenoise.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
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

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of ipnadmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage(void)
{
	PUTS("Syntax of 'duct expression' is:");
	PUTS("\t<protocol name>/<outduct name>");
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\ta\tAdd");
	PUTS("\t   a plan <node identifier> <duct expression> [<xmit rate>]");
	PUTS("\t   a exit <first node identifier> <last node identifier> \
<endpoint ID>");
	PUTS("\t   a rtovrd <data label> <dest identifier> <src identifier> \
<neighbor identifier> [<ductExpression>]");
	PUTS("\t\tFor all destinations or all sources use node identifier 0.");
	PUTS("\t   a cosovrd <data label> <dest node identifier> <src node \
identifier> <p#> <o#> [<QoS flags>]");
	PUTS("\t\tClass of service override: <p#> is overriding CoS");
	PUTS("\t\t\tand <o#> is overriding ordinal");
	PUTS("\t\tFor all destinations or all sources use node identifier 0.");
	PUTS("\tc\tChange");
	PUTS("\t   c plan <node identifier> <xmit rate>");
	PUTS("\t   c exit <first node identifier> <last node identifier> \
<endpoint ID>");
	PUTS("\t   c rtovrd <data label> <dest identifier> <src identifier> \
<neighbor node identifier> [<ductExpression>]");
	PUTS("\t   c cosovrd <data label> <dest node identifier> <src node \
identifier> <p#> <o#> [<QoS flags>]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} plan <node identifier>");
	PUTS("\t   {d|i} exit <first node identifier> <last node identifier>");
	PUTS("\t   {d|i} ovrd <data label> <dest node identifier> <source \
node identifier>");
	PUTS("\tl\tList");
	PUTS("\t   l exit");
	PUTS("\t   l plan");
	PUTS("\t   l ovrd");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	executeAdd(int tokenCount, char **tokens)
{
	unsigned int	nominalRate = 0;
	char		*spec;
	char		*ovrdDuctExpression;
	unsigned int	dataLabel;
	uvast		destFqnn;
	uvast		sourceFqnn;
	uvast		neighborFqnn;
	int		maxQosFlags =  (BP_MINIMUM_LATENCY |
					BP_BEST_EFFORT |
					BP_RELIABLE);
	int		flags;
	unsigned char	priority;
	unsigned char	ordinal;
	uvast		parsed_uvast;
	int		parsed_int;
	char		errMsg[256];

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount < 4 || tokenCount > 5)
		{
			SYNTAX_ERROR;
			return;
		}

		if (tokenCount == 5)
		{
			if (isdigit((int) tokens[4][0]))
			{
				if (platform_parse_uvast(tokens[4], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
				{
					isprintf(errMsg, sizeof(errMsg), "[?] Invalid xmit rate: %s", tokens[4]);
					PUTS(errMsg); writeMemo(errMsg);
					return;
				}
				nominalRate = (unsigned int) parsed_uvast;
			}
		}

		spec = tokens[3];
		ipn_addPlan(getFqn(tokens[2]), nominalRate);
		ipn_addPlanDuct(getFqn(tokens[2]), spec);
		return;
	}

	if (strcmp(tokens[1], "exit") == 0
	|| strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_addExit(getFqn(tokens[2]), getFqn(tokens[3]), tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "rtovrd") == 0)
	{
		if (tokenCount == 7)
		{
			ovrdDuctExpression = tokens[6];
		}
		else
		{
			if (tokenCount == 6)
			{
				ovrdDuctExpression = NULL;
			}
			else
			{
				SYNTAX_ERROR;
				return;
			}
		}

		if (platform_parse_uvast(tokens[2], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid data label: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		dataLabel = (unsigned int) parsed_uvast;

		if (dataLabel == 0)
		{
			dataLabel = (unsigned int) -1;
		}

		destFqnn = getFqn(tokens[3]);
		if (destFqnn == 0)
		{
			destFqnn = (uvast) -1;
		}

		sourceFqnn = getFqn(tokens[4]);
		if (sourceFqnn == 0)
		{
			sourceFqnn = (uvast) -1;
		}

		neighborFqnn = getFqn(tokens[5]);
		ipn_setOvrd(dataLabel, destFqnn, sourceFqnn, neighborFqnn,
				ovrdDuctExpression, (unsigned char) -2, 0, 0);
		return;
	}

	if (strcmp(tokens[1], "cosovrd") == 0)
	{
		if (tokenCount == 8)
		{
			if (platform_parse_int(tokens[7], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid QoS flags: %s", tokens[7]);
				PUTS(errMsg); writeMemo(errMsg);
				return;
			}
			flags = parsed_int;
			/*	Limit QoS configuration.		*/
			flags &= maxQosFlags;
		}
		else
		{
			if (tokenCount == 7)
			{
				flags = 0;
			}
			else
			{
				SYNTAX_ERROR;
				return;
			}
		}

		if (platform_parse_uvast(tokens[2], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid data label: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		dataLabel = (unsigned int) parsed_uvast;

		if (dataLabel == 0)
		{
			dataLabel = (unsigned int) -1;
		}

		destFqnn = getFqn(tokens[3]);
		if (destFqnn == 0)
		{
			destFqnn = (uvast) -1;
		}

		sourceFqnn = getFqn(tokens[4]);
		if (sourceFqnn == 0)
		{
			sourceFqnn = (uvast) -1;
		}

		if (platform_parse_uvast(tokens[5], &parsed_uvast) < 0 || parsed_uvast > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid priority (0-255): %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		priority = (unsigned char) parsed_uvast;

		if (platform_parse_uvast(tokens[6], &parsed_uvast) < 0 || parsed_uvast > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid ordinal (0-255): %s", tokens[6]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		ordinal = (unsigned char) parsed_uvast;

		ipn_setOvrd(dataLabel, destFqnn, sourceFqnn, (uvast) -2,
				NULL, priority, ordinal, flags);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	unsigned int	nominalRate = 0;
	char		*ovrdDuctExpression;
	unsigned int	dataLabel;
	uvast		destFqnn;
	uvast		sourceFqnn;
	uvast		neighborFqnn;
	int		maxQosFlags =  (BP_MINIMUM_LATENCY |
					BP_BEST_EFFORT |
					BP_RELIABLE);
	int		flags;
	unsigned char	priority;
	unsigned char	ordinal;
	uvast		parsed_uvast;
	int		parsed_int;
	char		errMsg[256];

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		if (platform_parse_uvast(tokens[3], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid xmit rate: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		nominalRate = (unsigned int) parsed_uvast;

		ipn_updatePlan(getFqn(tokens[2]), nominalRate);
		return;
	}

	if (strcmp(tokens[1], "exit") == 0
	|| strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_updateExit(getFqn(tokens[2]), getFqn(tokens[3]), tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "rtovrd") == 0)
	{
		if (tokenCount == 7)
		{
			ovrdDuctExpression = tokens[6];
		}
		else
		{
			if (tokenCount == 6)
			{
				ovrdDuctExpression = NULL;
			}
			else
			{
				SYNTAX_ERROR;
				return;
			}
		}

		if (platform_parse_uvast(tokens[2], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid data label: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		dataLabel = (unsigned int) parsed_uvast;

		if (dataLabel == 0)
		{
			dataLabel = (unsigned int) -1;
		}

		destFqnn = getFqn(tokens[3]);
		if (destFqnn == 0)
		{
			destFqnn = (uvast) -1;
		}

		sourceFqnn = getFqn(tokens[4]);
		if (sourceFqnn == 0)
		{
			sourceFqnn = (uvast) -1;
		}

		neighborFqnn = getFqn(tokens[5]);
		ipn_setOvrd(dataLabel, destFqnn, sourceFqnn, neighborFqnn,
				ovrdDuctExpression, (unsigned char) -2, 0, 0);
		return;
	}

	if (strcmp(tokens[1], "cosovrd") == 0)
	{
		if (tokenCount == 8)
		{
			if (platform_parse_int(tokens[7], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid QoS flags: %s", tokens[7]);
				PUTS(errMsg); writeMemo(errMsg);
				return;
			}
			flags = parsed_int;

			/*	Limit QoS configuration.		*/
			flags &= maxQosFlags;
		}
		else
		{
			if (tokenCount == 7)
			{
				flags = 0;
			}
			else
			{
				SYNTAX_ERROR;
				return;
			}
		}

		if (platform_parse_uvast(tokens[2], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid data label: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		dataLabel = (unsigned int) parsed_uvast;

		if (dataLabel == 0)
		{
			dataLabel = (unsigned int) -1;
		}

		destFqnn = getFqn(tokens[3]);
		if (destFqnn == 0)
		{
			destFqnn = (uvast) -1;
		}

		sourceFqnn = getFqn(tokens[4]);
		if (sourceFqnn == 0)
		{
			sourceFqnn = (uvast) -1;
		}

		if (platform_parse_uvast(tokens[5], &parsed_uvast) < 0 || parsed_uvast > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid priority (0-255): %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		priority = (unsigned char) parsed_uvast;

		if (platform_parse_uvast(tokens[6], &parsed_uvast) < 0 || parsed_uvast > 255)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid ordinal (0-255): %s", tokens[6]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		ordinal = (unsigned char) parsed_uvast;

		ipn_setOvrd(dataLabel, destFqnn, sourceFqnn,
				(uvast) -2, NULL, priority, ordinal, flags);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	unsigned int	dataLabel;
	uvast		destFqnn;
	uvast		sourceFqnn;
	uvast		neighborFqnn = (uvast) -1;;
	unsigned char	priority = (unsigned char) -1;
	uvast		parsed_uvast;
	char		errMsg[256];

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_removePlan(getFqn(tokens[2]));
		return;
	}

	if (strcmp(tokens[1], "exit") == 0
	|| strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		ipn_removeExit(getFqn(tokens[2]), getFqn(tokens[3]));
		return;
	}

	if (strcmp(tokens[1], "ovrd") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		if (platform_parse_uvast(tokens[2], &parsed_uvast) < 0 || parsed_uvast > UINT_MAX)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid data label: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		dataLabel = (unsigned int) parsed_uvast;

		if (dataLabel == 0)
		{
			dataLabel = (unsigned int) -1;
		}

		destFqnn = getFqn(tokens[3]);
		if (destFqnn == 0)
		{
			destFqnn = (uvast) -1;
		}

		sourceFqnn = getFqn(tokens[4]);
		if (sourceFqnn == 0)
		{
			sourceFqnn = (uvast) -1;
		}

		ipn_setOvrd((uvast) dataLabel, destFqnn, sourceFqnn,
				neighborFqnn, NULL, priority, 0, 0);
		return;
	}

	SYNTAX_ERROR;
}

static void	printPlan(BpPlan *plan)
{
	Sdr	sdr = getIonsdr();
	char	*action = "none";
	char	viaEid[SDRSTRING_BUFSZ];
	char	*spec = "none";
	SdrObject ductElt;
	SdrObject outductElt;
	Outduct	outduct;
	char	neighborBuf[FQN_MAX_LENGTH];
	char	buffer[1024];

	if (plan->viaEid)
	{
		action = "relay";
		sdr_string_read(sdr, viaEid, plan->viaEid);
		spec = viaEid;
	}
	else
	{
		action = "xmit";
		ductElt = sdr_list_first(sdr, plan->ducts);
		if (ductElt)
		{
			outductElt = sdr_list_data(sdr, ductElt);
			sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
					outductElt), sizeof(Outduct));
			spec = outduct.name;
		}
	}

	putFqn(neighborBuf, plan->neighborFqnn);
	isprintf(buffer, sizeof buffer, "%s %s %s xmit rate: %u", neighborBuf,
			action, spec, plan->nominalRate);
	printText(buffer);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	fqnn;
	SdrObject planAddr;
	SdrObject elt;
		OBJ_POINTER(BpPlan, plan);

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	fqnn = getFqn(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));
	ipn_findPlan(fqnn, &planAddr, &elt);
	if (elt == 0)
	{
		printText("[?] Unknown node.");
	}
	else
	{
		GET_OBJ_POINTER(getIonsdr(), BpPlan, plan, planAddr);
		printPlan(plan);
	}

	sdr_exit_xn(sdr);
}

static void	printExit(IpnExit *exit)
{
	char	eidString[SDRSTRING_BUFSZ];
	char	buffer[384];

	sdr_string_read(getIonsdr(), eidString, exit->eid);
	isprintf(buffer, sizeof buffer, "From " UVAST_FIELDSPEC " \
through " UVAST_FIELDSPEC ", forward via %.256s.",
			exit->firstFqnn, exit->lastFqnn, eidString);
	printText(buffer);
}

static void	infoExit(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	firstFqnn;
	uvast	lastFqnn;
	SdrObject elt;
		OBJ_POINTER(IpnExit, exit);

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	firstFqnn = getFqn(tokens[2]);
	lastFqnn = getFqn(tokens[3]);
	if (lastFqnn < firstFqnn)
	{
		printText("[?] Unknown exit.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->exits); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
		if (exit->firstFqnn == firstFqnn
		&& exit->lastFqnn == lastFqnn)
		{
			printExit(exit);
			break;
		}
	}

	if (elt == 0)
	{
		printText("[?] Unknown exit.");
	}

	sdr_exit_xn(sdr);
}

static void	printOverride(IpnOverride *ovrd)
{
	char	buffer[384];
	uvast	destFqnn;
	uvast	sourceFqnn;
	char	neighborIdentifier[32];
	char	priority[8];
	char	ordinal[8];
	char	destBuf[FQN_MAX_LENGTH];
	char	sourceBuf[FQN_MAX_LENGTH];

	if (ovrd->destFqnn == (uvast) -1)
	{
		destFqnn = 0;
	}
	else
	{
		destFqnn = ovrd->destFqnn;
	}

	if (ovrd->sourceFqnn == (uvast) -1)
	{
		sourceFqnn = 0;
	}
	else
	{
		sourceFqnn = ovrd->sourceFqnn;
	}

	if (ovrd->neighborFqnn == (uvast) -1)
	{
		strcpy(neighborIdentifier, "<none>");
	}
	else
	{
		putFqn(neighborIdentifier, ovrd->neighborFqnn);
	}

	if (ovrd->priority == (unsigned char) -1)
	{
		strcpy(priority, "<none>");
		strcpy(ordinal, "<none>");
	}
	else
	{
		isprintf(priority, sizeof priority, "%u",
				(unsigned int) (ovrd->priority));
		isprintf(ordinal, sizeof ordinal, "%u",
				(unsigned int) (ovrd->ordinal));
	}

	putFqn(destBuf, destFqnn);
	putFqn(sourceBuf, sourceFqnn);
	isprintf(buffer, sizeof buffer, "For data label %u, destination \
node %s, source node %s, overrides are: neighbor %s, priority %s, ordinal %s.",
			ovrd->dataLabel, destBuf, sourceBuf, neighborIdentifier,
			priority, ordinal);
	printText(buffer);
}

static void	infoOverride(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	unsigned int	dataLabel;
	uvast		destFqnn;
	uvast		sourceFqnn;
	SdrObject	elt;
			OBJ_POINTER(IpnOverride, ovrd);

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	dataLabel = strtouvast(tokens[2]);
	destFqnn = getFqn(tokens[3]);
	if (destFqnn == 0)
	{
		destFqnn = (uvast) -1;
	}

	sourceFqnn = getFqn(tokens[4]);
	if (sourceFqnn == 0)
	{
		sourceFqnn = (uvast) -1;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->overrides); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnOverride, ovrd,
				sdr_list_data(sdr, elt));
		if (ovrd->dataLabel == dataLabel
		&& ovrd->destFqnn == destFqnn
		&& ovrd->sourceFqnn == sourceFqnn)
		{
			printOverride(ovrd);
			break;
		}
	}

	if (elt == 0)
	{
		printText("[?] Unknown override.");
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

	if (strcmp(tokens[1], "plan") == 0)
	{
		infoPlan(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "exit") == 0
	|| strcmp(tokens[1], "group") == 0)
	{
		infoExit(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "ovrd") == 0)
	{
		infoOverride(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listPlans(void)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
		OBJ_POINTER(BpPlan, plan);

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, elt));
		if (plan->neighborFqnn == 0)	/*	Not ipn-scheme.	*/
		{
			continue;
		}

		printPlan(plan);
	}

	sdr_exit_xn(sdr);
}

static void	listExits(void)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
		OBJ_POINTER(IpnExit, exit);

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->exits); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
		printExit(exit);
	}

	sdr_exit_xn(sdr);
}

static void	listOverrides(void)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
		OBJ_POINTER(IpnOverride, ovrd);

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->overrides); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnOverride, ovrd,
				sdr_list_data(sdr, elt));
		printOverride(ovrd);
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

	if (strcmp(tokens[1], "plan") == 0)
	{
		listPlans();
		return;
	}

	if (strcmp(tokens[1], "exit") == 0
	|| strcmp(tokens[1], "group") == 0)
	{
		listExits();
		return;
	}

	if (strcmp(tokens[1], "ovrd") == 0)
	{
		listOverrides();
		return;
	}

	SYNTAX_ERROR;
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

static int	processLine(char *line, int lineLength)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];
	char	buffer[80];

	/* Parameter intentionally unused. */
	(void)lineLength;

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

		case 'a':
			executeAdd(tokenCount, tokens);
			return 0;

		case 'c':
			executeChange(tokenCount, tokens);
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

static int	run_ipnadmin(char *cmdFileName)
{
	int	cmdFile;
	char	line[256];
	int	len;

#ifdef INPUT_HISTORY
	char *input;
#endif

	if (bpAttach() < 0)
	{
		putErrmsg("ipnadmin can't attach to BP", NULL);
		return -1;
	}

	if (ipnInit() < 0)
	{
		putErrmsg("ipnadmin can't initialize routing database", NULL);
		return -1;
	}

	if (cmdFileName == NULL)	/*	Interactive.		*/
	{
#ifdef NON_INTERACTIVE
		return 0;		/*	No stdin.		*/
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

			if (processLine(line, len))
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

			if (processLine(line, len))
			{
				break;		/*	Out of loop.	*/
			}
#endif
		}
#endif
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

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping ipnadmin.");
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	ipnadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_ipnadmin(cmdFileName);
}
