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

static void	handleQuit()
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
	PUTS("\t   a plan <node nbr> [<duct expression>] [<xmit rate>]");
	PUTS("\t   a exit <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\tc\tChange");
	PUTS("\t   c plan <node nbr> [<duct expression>] [<xmit rate>]");
	PUTS("\t   c exit <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} plan <node nbr>");
	PUTS("\t   {d|i} exit <first node nbr> <last node nbr>");
	PUTS("\tl\tList");
	PUTS("\t   l exit");
	PUTS("\t   l plan");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	executeAdd(int tokenCount, char **tokens)
{
	unsigned int	nominalRate = 0;
	char		*spec = NULL;

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
			else
			{
				spec = tokens[4];
			}
		}

		if (tokenCount == 4)
		{
			if (isdigit((int) tokens[3][0]))
			{
				nominalRate = atoi(tokens[3]);
			}
			else
			{
				spec = tokens[3];
			}
		}

		ipn_addPlan(strtouvast(tokens[2]), nominalRate);
		if (spec)
		{
			ipn_addPlanDuct(strtouvast(tokens[2]), spec);
		}

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

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	unsigned int	nominalRate;
	int		rateChanged = 0;
	char		*spec = NULL;

	if (tokenCount < 2)
	{
		printText("Change what?");
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
				rateChanged = 1;
			}
			else
			{
				spec = tokens[4];
			}
		}

		if (tokenCount == 4)
		{
			if (isdigit((int) tokens[3][0]))
			{
				nominalRate = atoi(tokens[3]);
				rateChanged = 1;
			}
			else
			{
				spec = tokens[3];
			}
		}

		if (rateChanged)
		{
			ipn_updatePlan(strtouvast(tokens[2]), nominalRate);
		}

		if (spec)
		{
			ipn_removePlanDuct(strtouvast(tokens[2]), NULL);
			ipn_addPlanDuct(strtouvast(tokens[2]), spec);
		}

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

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
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

	isprintf(buffer, sizeof buffer, UVAST_FIELDSPEC " %s %s",
			plan->neighborNodeNbr, action, spec);
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
int	ipnadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_ipnadmin(cmdFileName);
}
