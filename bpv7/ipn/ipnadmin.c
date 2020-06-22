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

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of ipnadmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Syntax of 'duct expression' is:");
	PUTS("\t<protocol name>/<outduct name>");
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\ta\tAdd");
	PUTS("\t   a plan <node nbr> <duct expression> [<xmit rate>]");
	PUTS("\t   a exit <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\t   a rtovrd <data label> <dest node> <source node> <neighbor>");
	PUTS("\t\tRouting override: <neighbor> is a node number.");
	PUTS("\t\tFor all destinations or all sources use node number 0.");
	PUTS("\t   a cosovrd <data label> <dest node> <source node> <p#> <o#>");
	PUTS("\t\tClass of service override: <p#> is overriding CoS");
	PUTS("\t\t\tand <o#> is overriding ordinal");
	PUTS("\t\tFor all destinations or all sources use node number 0.");
	PUTS("\tc\tChange");
	PUTS("\t   c plan <node nbr> <xmit rate>");
	PUTS("\t   c exit <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\t   c rtovrd <data label> <dest node> <source node> <neighbor>");
	PUTS("\t   c cosovrd <data label> <dest node> <source node> <p#> <o#>");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} plan <node nbr>");
	PUTS("\t   {d|i} exit <first node nbr> <last node nbr>");
	PUTS("\t   {d|i} ovrd <data label> <dest node> <source node>");
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
	uvast		destNodeNbr;
	uvast		sourceNodeNbr;
	uvast		neighbor;
	unsigned char	priority;
	unsigned char	ordinal;

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
				nominalRate = atoi(tokens[4]);
			}
		}

		spec = tokens[3];
		ipn_addPlan(strtouvast(tokens[2]), nominalRate);
		ipn_addPlanDuct(strtouvast(tokens[2]), spec);
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

		ipn_addExit(strtouvast(tokens[2]), strtouvast(tokens[3]),
				tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "rtovrd") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		destNodeNbr = strtouvast(tokens[3]);
		if (destNodeNbr == 0)
		{
			destNodeNbr = (uvast) -1;
		}

		sourceNodeNbr = strtouvast(tokens[4]);
		if (sourceNodeNbr == 0)
		{
			sourceNodeNbr = (uvast) -1;
		}

		neighbor = strtouvast(tokens[5]);
		ipn_setOvrd(strtouvast(tokens[2]), destNodeNbr, sourceNodeNbr,
				neighbor, (unsigned char) -2, 0);
		return;
	}

	if (strcmp(tokens[1], "cosovrd") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		destNodeNbr = strtouvast(tokens[3]);
		if (destNodeNbr == 0)
		{
			destNodeNbr = (uvast) -1;
		}

		sourceNodeNbr = strtouvast(tokens[4]);
		if (sourceNodeNbr == 0)
		{
			sourceNodeNbr = (uvast) -1;
		}

		priority = atoi(tokens[5]);
		ordinal = atoi(tokens[6]);
		ipn_setOvrd(strtouvast(tokens[2]), destNodeNbr, sourceNodeNbr,
				(unsigned char) -2, priority, ordinal);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	unsigned int	nominalRate = 0;
	uvast		destNodeNbr;
	uvast		sourceNodeNbr;
	uvast		neighbor;
	unsigned char	priority;
	unsigned char	ordinal;

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

		nominalRate = atoi(tokens[3]);
		ipn_updatePlan(strtouvast(tokens[2]), nominalRate);
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

		ipn_updateExit(strtouvast(tokens[2]), strtouvast(tokens[3]),
				tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "rtovrd") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		destNodeNbr = strtouvast(tokens[3]);
		if (destNodeNbr == 0)
		{
			destNodeNbr = (uvast) -1;
		}

		sourceNodeNbr = strtouvast(tokens[4]);
		if (sourceNodeNbr == 0)
		{
			sourceNodeNbr = (uvast) -1;
		}

		neighbor = strtouvast(tokens[5]);
		ipn_setOvrd(strtouvast(tokens[2]), destNodeNbr, sourceNodeNbr,
				neighbor, (unsigned char) -2, 0);
		return;
	}

	if (strcmp(tokens[1], "cosovrd") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		destNodeNbr = strtouvast(tokens[3]);
		if (destNodeNbr == 0)
		{
			destNodeNbr = (uvast) -1;
		}

		sourceNodeNbr = strtouvast(tokens[4]);
		if (sourceNodeNbr == 0)
		{
			sourceNodeNbr = (uvast) -1;
		}

		priority = atoi(tokens[5]);
		ordinal = atoi(tokens[6]);
		ipn_setOvrd(strtouvast(tokens[2]), destNodeNbr, sourceNodeNbr,
				(unsigned char) -2, priority, ordinal);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	uvast		destNodeNbr;
	uvast		sourceNodeNbr;
	uvast		neighbor = (uvast) -1;;
	unsigned char	priority = (unsigned char) -1;

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

		ipn_removePlan(strtouvast(tokens[2]));
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

		ipn_removeExit(strtouvast(tokens[2]), strtouvast(tokens[3]));
		return;
	}

	if (strcmp(tokens[1], "ovrd") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		destNodeNbr = strtouvast(tokens[3]);
		if (destNodeNbr == 0)
		{
			destNodeNbr = (uvast) -1;
		}

		sourceNodeNbr = strtouvast(tokens[4]);
		if (sourceNodeNbr == 0)
		{
			sourceNodeNbr = (uvast) -1;
		}

		ipn_setOvrd(strtouvast(tokens[2]), destNodeNbr, sourceNodeNbr,
				neighbor, priority, 0);
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
	Object	ductElt;
	Object	outductElt;
	Outduct	outduct;
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

	isprintf(buffer, sizeof buffer, UVAST_FIELDSPEC " %s %s xmit rate: %lu",
			plan->neighborNodeNbr, action, spec, plan->nominalRate);
	printText(buffer);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	int	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(BpPlan, plan);

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	nodeNbr = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));
	ipn_findPlan(nodeNbr, &planAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown node.");
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
			exit->firstNodeNbr, exit->lastNodeNbr, eidString);
	printText(buffer);
}

static void	infoExit(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	firstNodeNbr;
	uvast	lastNodeNbr;
	Object	elt;
		OBJ_POINTER(IpnExit, exit);

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	firstNodeNbr = strtouvast(tokens[2]);
	lastNodeNbr = strtouvast(tokens[3]);
	if (lastNodeNbr < firstNodeNbr)
	{
		printText("Unknown exit.");
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->exits); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
		if (exit->firstNodeNbr == firstNodeNbr
		&& exit->lastNodeNbr == lastNodeNbr)
		{
			printExit(exit);
			break;
		}
	}

	if (elt == 0)
	{
		printText("Unknown exit.");
	}

	sdr_exit_xn(sdr);
}

static void	printOverride(IpnOverride *ovrd)
{
	char	buffer[384];
	uvast	destNodeNbr;
	uvast	sourceNodeNbr;
	char	neighbor[32];
	char	priority[8];
	char	ordinal[8];

	if (ovrd->destNodeNbr == (uvast) -1)
	{
		destNodeNbr = 0;
	}
	else
	{
		destNodeNbr = ovrd->destNodeNbr;
	}

	if (ovrd->sourceNodeNbr == (uvast) -1)
	{
		sourceNodeNbr = 0;
	}
	else
	{
		sourceNodeNbr = ovrd->sourceNodeNbr;
	}

	if (ovrd->neighbor == (uvast) -1)
	{
		istrcpy(neighbor, "<none>", sizeof neighbor);
	}
	else
	{
		isprintf(neighbor, sizeof neighbor, UVAST_FIELDSPEC,
				ovrd->neighbor);
	}

	if (ovrd->priority == (unsigned char) -1)
	{
		istrcpy(priority, "<none>", sizeof(priority));
		istrcpy(ordinal, "<none>", sizeof(ordinal));
	}
	else
	{
		isprintf(priority, sizeof priority, "%u",
				(unsigned int) (ovrd->priority));
		isprintf(ordinal, sizeof ordinal, "%u",
				(unsigned int) (ovrd->ordinal));
	}

	isprintf(buffer, sizeof buffer, "For data label %u, destination node "
UVAST_FIELDSPEC ", source node " UVAST_FIELDSPEC ", overrides are: neighbor \
%s, priority %s, ordinal %s.", ovrd->dataLabel, destNodeNbr, sourceNodeNbr,
			neighbor, priority, ordinal);
	printText(buffer);
}

static void	infoOverride(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	unsigned int	dataLabel;
	uvast		destNodeNbr;
	uvast		sourceNodeNbr;
	Object		elt;
			OBJ_POINTER(IpnOverride, ovrd);

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	dataLabel = strtouvast(tokens[2]);
	destNodeNbr = strtouvast(tokens[3]);
	if (destNodeNbr == 0)
	{
		destNodeNbr = (uvast) -1;
	}

	sourceNodeNbr = strtouvast(tokens[4]);
	if (sourceNodeNbr == 0)
	{
		sourceNodeNbr = (uvast) -1;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->overrides); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnOverride, ovrd,
				sdr_list_data(sdr, elt));
		if (ovrd->dataLabel == dataLabel
		&& ovrd->destNodeNbr == destNodeNbr
		&& ovrd->sourceNodeNbr == sourceNodeNbr)
		{
			printOverride(ovrd);
			break;
		}
	}

	if (elt == 0)
	{
		printText("Unknown override.");
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

static void	listPlans()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(BpPlan, plan);

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, elt));
		if (plan->neighborNodeNbr == 0)	/*	Not CBHE.	*/
		{
			continue;
		}

		printPlan(plan);
	}

	sdr_exit_xn(sdr);
}

static void	listExits()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
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

static void	listOverrides()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
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
#ifdef FSWLOGGER
		return 0;
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
