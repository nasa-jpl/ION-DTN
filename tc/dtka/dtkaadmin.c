/*
	dtkaadmin.c:	DTKA adminstration interface.
									*/
/*	Copyright (c) 2020, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "dtka.h"

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

	isprintf(buffer, sizeof buffer,
			"Syntax error at line %d of dtkaadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\tm\tManage");
	PUTS("\t   m keygentime <time>");
	PUTS("\t\tTime format is yyyy/mm/dd-hh:mm:ss.");
	PUTS("\t   m interval <new key pair generation interval, in seconds>");
	PUTS("\t   m leadtime <key pair effectiveness lead time, in seconds>");
	PUTS("\ti\tInfo");
	PUTS("\t   i");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeDtka(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (dtkaInit() < 0)
	{
		putErrmsg("dtkaadmin can't initialize DTKA.", NULL);
	}
}

static int	attachToDtka()
{
	if (dtkaAttach() < 0)
	{
		printText("DTKA not initialized yet.");
		return -1;
	}

	return 0;
}

static void	manageKeyGenTime(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dtkadbObj = getDtkaDbObject();
	time_t		newKeyGenTime;
	DtkaDB	dtkadb;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newKeyGenTime = readTimestampUTC(tokens[2], 0);
	if (newKeyGenTime == 0)
	{
		putErrmsg("keygentime invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &dtkadb, dtkadbObj, sizeof(DtkaDB));
	dtkadb.nextKeyGenTime = newKeyGenTime;
	sdr_write(sdr, dtkadbObj, (char *) &dtkadb, sizeof(DtkaDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change keygentime.", NULL);
	}
}

static void	manageInterval(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dtkadbObj = getDtkaDbObject();
	DtkaDB	dtkadb;
	int		newInterval;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newInterval = atoi(tokens[2]);
	if (newInterval < 60)
	{
		putErrmsg("interval invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &dtkadb, dtkadbObj, sizeof(DtkaDB));
	dtkadb.keyGenInterval = newInterval;
	sdr_write(sdr, dtkadbObj, (char *) &dtkadb, sizeof(DtkaDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change interval.", NULL);
	}
}

static void	manageLeadTime(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dtkadbObj = getDtkaDbObject();
	DtkaDB	dtkadb;
	int		newLeadTime;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newLeadTime = atoi(tokens[2]);
	if (newLeadTime < 20)
	{
		putErrmsg("leadtime invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &dtkadb, dtkadbObj, sizeof(DtkaDB));
	dtkadb.effectiveLeadTime = newLeadTime;
	sdr_write(sdr, dtkadbObj, (char *) &dtkadb, sizeof(DtkaDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change leadtime.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "keygentime") == 0)
	{
		manageKeyGenTime(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "interval") == 0)
	{
		manageInterval(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "leadtime") == 0)
	{
		manageLeadTime(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo()
{
	Sdr		sdr = getIonsdr();
			OBJ_POINTER(DtkaDB, db);
	char		buffer[256];
	char		next[TIMESTAMPBUFSZ];

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, DtkaDB, db, getDtkaDbObject());
	writeTimestampUTC(db->nextKeyGenTime, next);
	isprintf(buffer, sizeof buffer, "\nnextKeyGentime=%s, keyGenInterval=\
%u, effectiveLeadTime=%u", next, db->keyGenInterval, db->effectiveLeadTime);
	printText(buffer);
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

		case '1':
			initializeDtka(tokenCount, tokens);
			return 0;

		case 'm':
			if (attachToDtka() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToDtka() == 0)
			{
				executeInfo();
			}

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

#if defined (ION_LWT)
int	dtkaadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	cmdFile;
	char	line[256];
	int	len;

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdout.	*/
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
	printText("Stopping dtkaadmin.");
	ionDetach();

	return 0;
}
