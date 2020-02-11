/*
	imcadmin.c:	BP routing adminstration interface for
			the IMC multicast endpoint ID scheme.
									*/
/*									*/
/*	Copyright (c) 2012, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "imcfw.h"

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

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of imcadmin.c",
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
	PUTS("\ta\tAdd");
	PUTS("\t   a <node nbr> { 0 | 1 }");
	PUTS("\t\t1 if parent, 0 if child");
	PUTS("\tc\tChange");
	PUTS("\t   c <node nbr> { 0 | 1 }");
	PUTS("\t\t1 if now parent, 0 if now child");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} <node nbr>");
	PUTS("\tl\tList");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	executeAdd(int tokenCount, char **tokens)
{
	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	imc_addKin(strtouvast(tokens[1]), atoi(tokens[2]));
}

static void	executeChange(int tokenCount, char **tokens)
{
	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	imc_updateKin(strtouvast(tokens[1]), atoi(tokens[2]));
}

static void	executeDelete(int tokenCount, char **tokens)
{
	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	imc_removeKin(strtouvast(tokens[1]));
}

static void	printKin(uvast kin, uvast parent)
{
	char	buffer[32];

	isprintf(buffer, sizeof buffer, UVAST_FIELDSPEC " %s", kin,
			kin == parent ? "parent" : "");
	printText(buffer);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	ImcDB	imcdb;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(getIonsdr(), (char *) &imcdb, getImcDbObject(), sizeof(ImcDB));
	printKin(strtouvast(tokens[1]), imcdb.parent);
	sdr_exit_xn(sdr);
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	ImcDB	imcdb;
	Object	elt;
		OBJ_POINTER(NodeId, node);

	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(getIonsdr(), (char *) &imcdb, getImcDbObject(), sizeof(ImcDB));
	for (elt = sdr_list_first(sdr, imcdb.kin); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, NodeId, node, sdr_list_data(sdr, elt));
		printKin(node->nbr, imcdb.parent);
	}

	sdr_exit_xn(sdr);
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

static int	run_imcadmin(char *cmdFileName)
{
	int	cmdFile;
	char	line[256];
	int	len;

	if (bpAttach() < 0)
	{
		putErrmsg("imcadmin can't attach to BP.", NULL);
		return -1;
	}

	if (imcInit() < 0)
	{
		putErrmsg("imcadmin can't initialize IMC database.", NULL);
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
	printText("Stopping imcadmin.");
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	imcadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_imcadmin(cmdFileName);
}
