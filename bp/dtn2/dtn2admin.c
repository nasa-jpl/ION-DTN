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

static Sdr	sdr;
static int	echo = 0;
static DtnDB	*dtn2Constants;
static char	*huh = "?";

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

	isprintf(buffer, sizeof buffer,
			"Syntax error at line %d of dtn2admin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	puts("Syntax of 'directive' is:");
	puts("\t{ f <endpoint ID> | x <protocol name>/<outduct name>[,dest \
induct name] }");
	puts("Note that, by convention, each node name must start with '//'.");
	puts("Valid commands are:");
	puts("\tq\tQuit");
	puts("\th\tHelp");
	puts("\t?\tHelp");
	puts("\ta\tAdd");
	puts("\t   a plan <node name> <default directive>");
	puts("\t   a rule <node name> <demux name> <directive>");
	puts("\tc\tChange");
	puts("\t   c plan <node name> <default directive>");
	puts("\t   c rule <node name> <demux name> <directive>");
	puts("\td\tDelete");
	puts("\t   d plan <node name>");
	puts("\t   d rule <node name> <demux name>");
	puts("\ti\tInfo");
	puts("\t   i plan <node name>");
	puts("\t   i rule <node name> <demux name>");
	puts("\tl\tList");
	puts("\t   l plan");
	puts("\t   l rule <node name>");
	puts("\t#\tComment");
	puts("\t   # <comment text>");
}

static int	parseDirective(char *actionToken, char *parmToken,
			FwdDirective *dir)
{
	char		*protocolName;
	char		*cursor = NULL;
	char		*outductName;
	char		*destDuctName;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	if (actionToken == NULL || parmToken == NULL || dir == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	switch (*actionToken)
	{
	case 'f':
		if (strlen(parmToken) >= SDRSTRING_BUFSZ)
		{
			putErrmsg("Station EID is too long.", parmToken);
			return -1;
		}

		dir->action = fwd;
		sdr_begin_xn(sdr);
		dir->eid = sdr_string_create(sdr, parmToken);
		if (sdr_end_xn(sdr))
		{
			putSysErrmsg("Can't write station EID.", NULL);
			return 0;
		}

		return 1;

	case 'x':
		dir->action = xmit;
		cursor = parmToken;
		protocolName = cursor;
		cursor = strchr(cursor, '/');
		if (cursor == NULL)
		{
			putErrmsg("Malformed directive: <protocol>/<duct>",
					parmToken);
			return 0;
		}

		*cursor = '\0';
		cursor++;
		outductName = cursor;
		cursor = strchr(cursor, ',');
		if (cursor == NULL)
		{
			destDuctName = NULL;
			dir->destDuctName = 0;
		}
		else
		{
			*cursor = '\0';
			cursor++;
			destDuctName = cursor;
			if (strlen(destDuctName) >= SDRSTRING_BUFSZ)
			{
				putErrmsg("Destination duct name is too long.",
						destDuctName);
				return 0;
			}
		}

		findOutduct(protocolName, outductName, &vduct, &vductElt);
		if (vductElt == 0)
		{
			putErrmsg("Unknown outduct.", outductName);
			return 0;
		}

		dir->outductElt = vduct->outductElt;
		if (destDuctName)
		{
			sdr_begin_xn(sdr);
			dir->destDuctName = sdr_string_create(sdr,
					destDuctName);
			if (sdr_end_xn(sdr))
			{
				putSysErrmsg("Can't write duct name.", NULL);
				return 0;
			}
		}

		return 1;

	default:
		putErrmsg("Invalid action code in directive", cursor);
		return 0;
	}

}

static void	executeAdd(int tokenCount, char **tokens)
{
	FwdDirective	directive;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDirective(tokens[3], tokens[4], &directive) < 1)
		{
			return;
		}

		if (dtn2_addPlan(tokens[2], &directive) < 1)
		{
			dtn2_destroyDirective(&directive);
		}

		return;
	}

	if (strcmp(tokens[1], "rule") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDirective(tokens[4], tokens[5], &directive) < 1)
		{
			return;
		}

		if (dtn2_addRule(tokens[2], tokens[3], &directive) < 1)
		{
			dtn2_destroyDirective(&directive);
		}

		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	FwdDirective	directive;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDirective(tokens[3], tokens[4], &directive) < 1)
		{
			return;
		}

		if (dtn2_updatePlan(tokens[2], &directive) < 1)
		{
			dtn2_destroyDirective(&directive);
		}

		return;
	}

	if (strcmp(tokens[1], "rule") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDirective(tokens[4], tokens[5], &directive) < 1)
		{
			return;
		}

		if (dtn2_updateRule(tokens[2], tokens[3], &directive) < 1)
		{
			dtn2_destroyDirective(&directive);
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

	if (strcmp(tokens[1], "rule") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		dtn2_removeRule(tokens[2], tokens[3]);
		return;
	}

	SYNTAX_ERROR;
}

static void	printDirective(char *context, FwdDirective *dir)
{
	char	eidString[SDRSTRING_BUFSZ + 1];
		OBJ_POINTER(Outduct, duct);
		OBJ_POINTER(ClProtocol, clp);
	char	destDuctName[SDRSTRING_BUFSZ + 1];
	char	buffer[1024];

	switch (dir->action)
	{
	case fwd:
		if (sdr_string_read(sdr, eidString, dir->eid) < 1)
		{
			isprintf(buffer, sizeof buffer, "%.256s f ?", context);
			printText(buffer);
		}
		else
		{
			isprintf(buffer, sizeof buffer, "%.256s f %.256s\n",
					context, eidString);
			printText(buffer);
		}

		return;

	case xmit:
		GET_OBJ_POINTER(sdr, Outduct, duct, sdr_list_data(sdr,
				dir->outductElt));
		GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);
		if (dir->destDuctName == 0)
		{
			destDuctName[0] = '\0';
		}
		else
		{
			destDuctName[0] = ':';
			if (sdr_string_read(sdr, destDuctName + 1,
					dir->destDuctName) < 0)
			{
				istrcpy(destDuctName + 1, huh,
						sizeof destDuctName - 1);
			}
		}

		isprintf(buffer, sizeof buffer, "%.256s x %.8s.%.128s%.128s\n",
				context, clp->name, duct->name, destDuctName);
		printText(buffer);
		return;

	default:
		printText(huh);
	}
}

static void	printPlan(Dtn2Plan *plan)
{
	char	nameBuf[SDRSTRING_BUFSZ];
	char	*nodeName;

	if (sdr_string_read(sdr, nameBuf, plan->nodeName) < 0)
	{
		nodeName = huh;
	}
	else
	{
		nodeName = nameBuf;
	}

	printDirective(nodeName, &plan->defaultDirective);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Object	planAddr;
		OBJ_POINTER(Dtn2Plan, plan);
	Object	elt;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	dtn2_findPlan(tokens[2], &planAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown plan.");
		return;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, planAddr);
	printPlan(plan);
}

static void	printRule(Dtn2Plan *plan, Dtn2Rule *rule)
{
	char	toNodeName[SDRSTRING_BUFSZ];
	char	demux[SDRSTRING_BUFSZ];
	char	context[128];

	if (sdr_string_read(sdr, toNodeName, plan->nodeName) < 0)
	{
		istrcpy(toNodeName, huh, sizeof toNodeName);
	}

	if (sdr_string_read(sdr, demux, rule->demux) < 0)
	{
		istrcpy(demux, huh, sizeof toNodeName);
	}

	isprintf(context, sizeof context, "%.64s, for %.32s =", toNodeName,
			demux);
	printDirective(context, &rule->directive);
}

static void	infoRule(int tokenCount, char **tokens)
{
	Object	planAddr;
		OBJ_POINTER(Dtn2Plan, plan);
	Object	ruleAddr;
		OBJ_POINTER(Dtn2Rule, rule);
	Object	elt;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	dtn2_findPlan(tokens[2], &planAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown plan.");
		return;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, planAddr);
	dtn2_findRule(tokens[2], tokens[3], plan, &ruleAddr, &elt);
	if (elt == 0)
	{
		printText("Unknown rule.");
		return;
	}

	GET_OBJ_POINTER(sdr, Dtn2Rule, rule, ruleAddr);
	printRule(plan, rule);
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

	if (strcmp(tokens[1], "rule") == 0)
	{
		infoRule(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listPlans()
{
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);

	for (elt = sdr_list_first(sdr, dtn2Constants->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
		printPlan(plan);
	}
}

static void	listRules(Dtn2Plan *plan)
{
	Object	elt;
		OBJ_POINTER(Dtn2Rule, rule);

	for (elt = sdr_list_first(sdr, plan->rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Dtn2Rule, rule, sdr_list_data(sdr, elt));
		printRule(plan, rule);
	}
}

static void	executeList(int tokenCount, char **tokens)
{
	Object	planAddr;
		OBJ_POINTER(Dtn2Plan, plan);
	Object	elt;

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

	if (strcmp(tokens[1], "rule") == 0)
	{
		if (tokenCount < 3)
		{
			printText("Must specify node name for rules list.");
			return;
		}

		dtn2_findPlan(tokens[2], &planAddr, &elt);
		if (elt == 0)
		{
			printText("Unknown plan.");
			return;
		}

		GET_OBJ_POINTER(sdr, Dtn2Plan, plan, planAddr);
		listRules(plan);
		return;
	}

	SYNTAX_ERROR;
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

	if (line == NULL) return 0;

	lineLength = strlen(line);
	if (lineLength < 0) return 0;

	if (line[lineLength - 1] == 0x0a)	/*	LF (newline)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength < 0) return 0;
	}

	if (line[lineLength - 1] == 0x0d)	/*	CR (DOS text)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength < 0) return 0;
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
	FILE	*cmdFile;
	char	line[256];

	if (bpAttach() < 0)
	{
		putErrmsg("dtn2admin can't attach to BP", NULL);
		return 1;
	}

	sdr = getIonsdr();
	if (dtn2Init() < 0)
	{
		putErrmsg("dtn2admin can't initialize routing database", NULL);
		return 1;
	}

	dtn2Constants = getDtnConstants();
	if (cmdFileName == NULL)	/*	Interactive.		*/
	{
		signal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			if (fgets(line, sizeof line, stdin) == NULL)
			{
				if (feof(stdin))
				{
					break;
				}

				putSysErrmsg("dtn2admin fgets failed", NULL);
				break;		/*	Out of loop.	*/
			}

			if (processLine(line))
			{
				break;		/*	Out of loop.	*/
			}
		}
	}
	else				/*	Scripted.		*/
	{
		cmdFile = fopen(cmdFileName, "r");
		if (cmdFile == NULL)
		{
			putSysErrmsg("Can't open command file", cmdFileName);
		}
		else
		{
			while (1)
			{
				if (fgets(line, sizeof line, cmdFile) == NULL)
				{
					if (feof(cmdFile))
					{
						break;
					}

					putSysErrmsg("dtn2admin fgets failed",
							NULL);
					break;		/*	Loop.	*/
				}

				if (line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line))
				{
					break;		/*	Loop.	*/
				}
			}

			fclose(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping dtn2admin.");
	ionDetach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	dtn2admin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_dtn2admin(cmdFileName);
}
