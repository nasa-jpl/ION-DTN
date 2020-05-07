/*
	dtn2admin.c:	BP routing adminstration interface for
			the DTN endpoint ID scheme.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "dtn2fw.h"

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
			"Syntax error at line %d of dtn2admin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Syntax of 'duct expression' is:");
	PUTS("\t<protocol name>/<outduct name>");
	PUTS("Syntax of 'directive' is:");
	PUTS("\t{ f <endpoint ID> | x <duct expression>");
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\ta\tAdd");
	PUTS("\t   a plan <node ID> <directive> [xmit rate]");
	PUTS("\tc\tChange");
	PUTS("\t   c plan <node ID> [<directive>] [xmit rate]");
	PUTS("\td\tDelete");
	PUTS("\t   d plan <node ID>");
	PUTS("\ti\tInfo");
	PUTS("\t   i plan <node ID>");
	PUTS("\tl\tList");
	PUTS("\t   l plan");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static int	parseDirective(char *actionToken, char *parmToken,
			char **viaEid, char **ductExpression)
{
	switch (actionToken[0])
	{
	case 'f':
		*viaEid = parmToken;
		return 1;

	case 'x':
		*ductExpression = parmToken;
		return 1;

	default:
		return 0;
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	unsigned int	nominalRate = 0;
	char		*viaEid = NULL;
	char		*ductExpression = NULL;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount < 4 || tokenCount > 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (tokenCount == 6)
		{
			if (isdigit(tokens[5][0]))
			{
				nominalRate = atoi(tokens[5]);
				if (parseDirective(tokens[3], tokens[4],
					&viaEid, &ductExpression) == 0)
				{
					SYNTAX_ERROR;
					return;
				}
			}
			else
			{
				nominalRate = atoi(tokens[3]);
				if (parseDirective(tokens[4], tokens[5],
					&viaEid, &ductExpression) == 0)
				{
					SYNTAX_ERROR;
					return;
				}
			}
		}

		if (tokenCount == 5)
		{
			if (parseDirective(tokens[3], tokens[4],
				&viaEid, &ductExpression) == 0)
			{
				SYNTAX_ERROR;
				return;
			}
		}

		if (tokenCount == 4)
		{
			nominalRate = atoi(tokens[3]);
		}

		dtn2_addPlan(tokens[2], nominalRate);
		if (ductExpression)
		{
			dtn2_addPlanDuct(tokens[2], ductExpression);
		}

		if (viaEid)
		{
			dtn2_setPlanViaEid(tokens[2], viaEid);
		}

		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	unsigned int	nominalRate;
	int		rateChanged = 0;
	char		*viaEid = NULL;
	char		*ductExpression = NULL;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount < 4 || tokenCount > 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (tokenCount == 6)
		{
			if (isdigit(tokens[5][0]))
			{
				nominalRate = atoi(tokens[5]);
				rateChanged = 1;
				if (parseDirective(tokens[3], tokens[4],
					&viaEid, &ductExpression) == 0)
				{
					SYNTAX_ERROR;
					return;
				}
			}
			else
			{
				nominalRate = atoi(tokens[3]);
				rateChanged = 1;
				if (parseDirective(tokens[4], tokens[5],
					&viaEid, &ductExpression) == 0)
				{
					SYNTAX_ERROR;
					return;
				}
			}
		}

		if (tokenCount == 5)
		{
			if (parseDirective(tokens[3], tokens[4],
				&viaEid, &ductExpression) == 0)
			{
				SYNTAX_ERROR;
				return;
			}
		}

		if (tokenCount == 4)
		{
			nominalRate = atoi(tokens[3]);
			rateChanged = 1;
		}

		if (rateChanged)
		{
			dtn2_updatePlan(tokens[2], nominalRate);
		}

		if (ductExpression)
		{
			dtn2_removePlanDuct(tokens[2]);
			dtn2_addPlanDuct(tokens[2], ductExpression);
		}

		if (viaEid)
		{
			dtn2_setPlanViaEid(tokens[2], viaEid);
		}

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

		dtn2_removePlan(tokens[2]);
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

	isprintf(buffer, sizeof buffer, "%s %s %s", plan->neighborEid,
			action, spec);
	printText(buffer);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	planAddr;
		OBJ_POINTER(BpPlan, plan);
	Object	elt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	dtn2_findPlan(tokens[2], &planAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown plan.");
	}
	else
	{
		GET_OBJ_POINTER(sdr, BpPlan, plan, planAddr);
		printPlan(plan);
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
		printPlan(plan);
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

static int	run_dtn2admin(char *cmdFileName)
{
	int	cmdFile;
	char	line[256];
	int	len;

	if (bpAttach() < 0)
	{
		putErrmsg("dtn2admin can't attach to BP.", NULL);
		return -1;
	}

	if (cmdFileName == NULL)	/*	Interactive.		*/
	{
#ifdef FSWLOGGER
		return 0;
#endif
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
	}
	else				/*	Scripted.		*/
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
	printText("Stopping dtn2admin.");
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	dtn2admin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_dtn2admin(cmdFileName);
}
