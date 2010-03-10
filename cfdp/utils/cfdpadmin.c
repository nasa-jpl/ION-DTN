/*

	cfdpadmin.c:	CFDP engine adminstration interface.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdpP.h"

static Sdr	sdr = NULL;
static int	echo = 0;
static CfdpDB	*cfdpConstants;
static CfdpVdb	*vdb;

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

	sprintf(buffer, "Syntax error at line %d of cfdpadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	puts("Valid commands are:");
	puts("\tq\tQuit");
	puts("\th\tHelp");
	puts("\t?\tHelp");
	puts("\t1\tInitialize");
	puts("\t   1");
	puts("\tm\tManage");
	puts("\t   m discard { 0 | 1 }");
	puts("\t   m requirecrc { 0 | 1 }");
	puts("\t   m fillchar <file fill character in hex, e.g., 0xaa>");
	puts("\t   m ckperiod <check cycle period, in seconds>");
	puts("\t   m maxtimeouts <max number of check cycle timeouts>");
	puts("\t   m maxtrnbr <max transaction number>");
	puts("\t   m segsize <max bytes per reliable file data segment>");
	puts("\t   m mtusize <max bytes per best-efforts file data segment>");
	puts("\ti\tInfo");
	puts("\t   i");
	puts("\ts\tStart");
	puts("\t   s '<UTA command>'");
	puts("\tx\tStop");
	puts("\t   x");
	puts("\tw\tWatch CFDP activity");
	puts("\t   w { 0 | 1 | <activity spec> }");
	puts("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., pq.  See man(5) for cfdprc.");
	puts("\te\tEnable or disable echo of printed output to log file");
	puts("\t   e { 0 | 1 }");
	puts("\t#\tComment");
	puts("\t   # <comment text>");
}

static void	initializeCfdp(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (ionAttach() < 0)
	{
		putErrmsg("cfdpadmin can't attach to ION.", NULL);
		return;
	}

	if (cfdpInit() < 0)
	{
		putErrmsg("cfdpadmin can't initialize CFDP.", NULL);
		return;
	}

	sdr = getIonsdr();
	cfdpConstants = getCfdpConstants();
	vdb = getCfdpVdb();
}

static int	attachToCfdp()
{
	if (sdr == NULL)
	{
		if (cfdpAttach() < 0)
		{
			printText("CFDP not initialized yet.");
			return -1;
		}

		sdr = getIonsdr();
		cfdpConstants = getCfdpConstants();
		vdb = getCfdpVdb();
	}

	return 0;
}

static void	manageDiscard(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newDiscard;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newDiscard = atoi(tokens[2]);
	if (newDiscard != 0 && newDiscard != 1)
	{
		putErrmsg("discardIncompleteFile switch invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.discardIncompleteFile = newDiscard;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change discardIncompleteFile switch.", NULL);
	}
}

static void	manageRequirecrc(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newRequirecrc;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newRequirecrc = atoi(tokens[2]);
	if (newRequirecrc != 0 && newRequirecrc != 1)
	{
		putErrmsg("crcRequired switch invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.crcRequired = newRequirecrc;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change crcRequired switch.", NULL);
	}
}

static void	manageFillchar(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newFillchar;
	char	*trailing;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newFillchar = strtol(tokens[2], &trailing, 16);
	if (*trailing != '\0')
	{
		putErrmsg("fillCharacter invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.fillCharacter = newFillchar;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change fillCharacter.", NULL);
	}
}

static void	manageCkperiod(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newCkperiod;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newCkperiod = atoi(tokens[2]);
	if (newCkperiod < 1)
	{
		putErrmsg("checkTimerPeriod invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.checkTimerPeriod = newCkperiod;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change checkTimerPeriod.", NULL);
	}
}

static void	manageMaxtimeouts(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtimeouts;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newMaxtimeouts = atoi(tokens[2]);
	if (newMaxtimeouts < 0)
	{
		putErrmsg("checkTimeoutLimit invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.checkTimeoutLimit = newMaxtimeouts;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change checkTimerPeriod.", NULL);
	}
}

static void	manageMaxtrnbr(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtrnbr;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newMaxtrnbr = atoi(tokens[2]);
	if (newMaxtrnbr < 0)
	{
		putErrmsg("maxTransactionNbr invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.maxTransactionNbr = newMaxtrnbr;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxTransactionNbr.", NULL);
	}
}

static void	manageSegsize(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newSegsize;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newSegsize = atoi(tokens[2]);
	if (newSegsize < 0)
	{
		putErrmsg("maxFileDataLength invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.maxFileDataLength = newSegsize;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxFileDataLength.", NULL);
	}
}

static void	manageMtusize(int tokenCount, char **tokens)
{
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMtusize;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
	}

	newMtusize = atoi(tokens[2]);
	if (newMtusize < 0)
	{
		putErrmsg("mtuSize invalid.", tokens[2]);
		return;
	}

	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.mtuSize = newMtusize;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change mtuSize.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (attachToCfdp() < 0) return;
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "discard") == 0)
	{
		manageDiscard(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "requirecrc") == 0)
	{
		manageRequirecrc(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "fillchar") == 0)
	{
		manageFillchar(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "ckperiod") == 0)
	{
		manageCkperiod(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxtimeouts") == 0)
	{
		manageMaxtimeouts(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "maxtrnbr") == 0)
	{
		manageMaxtrnbr(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "segsize") == 0)
	{
		manageSegsize(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "mtusize") == 0)
	{
		manageMtusize(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo()
{
		OBJ_POINTER(CfdpDB, db);
	char	buffer[256];

	if (attachToCfdp() < 0) return;
	sdr_begin_xn(sdr);	/*	Just to lock memory.		*/
	GET_OBJ_POINTER(sdr, CfdpDB, db, getCfdpDbObject());
	sprintf(buffer, "xncount=%lu, maxtrnbr=%lu, fillchar=0x%x, \
discard=%hu, requirecrc=%hu, segsize=%hu, mtusize = %hu, inactivity=%u, \
ckperiod=%u, maxtimeouts=%u", db->transactionCounter, db->maxTransactionNbr,
			db->fillCharacter, db->discardIncompleteFile,
			db->crcRequired, db->maxFileDataLength, db->mtuSize,
			db->transactionInactivityLimit, db->checkTimerPeriod,
			db->checkTimeoutLimit);
	sdr_exit_xn(sdr);
	printText(buffer);
}

static void	switchWatch(int tokenCount, char **tokens)
{
	char	buffer[80];
	char	*cursor;

	if (attachToCfdp() < 0) return;
	if (tokenCount < 2)
	{
		printText("Switch watch in what way?");
		return;
	}

	if (strcmp(tokens[1], "1") == 0)
	{
		vdb->watching = -1;
		return;
	}

	vdb->watching = 0;
	if (strcmp(tokens[1], "0") == 0)
	{
		return;
	}

	cursor = tokens[1];
	while (*cursor)
	{
		switch (*cursor)
		{
		case 'p':
			vdb->watching |= WATCH_p;
			break;

		case 'q':
			vdb->watching |= WATCH_q;
			break;

		default:
			sprintf(buffer, "Invalid watch char %c.", *cursor);
			printText(buffer);
		}

		cursor++;
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

	if (line == NULL) return 0;

	lineLength = strlen(line);
	if (lineLength == 0) return 0;

	if (line[lineLength - 1] == 0x0a)	/*	LF (newline)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength == 0) return 0;
	}

	if (line[lineLength - 1] == 0x0d)	/*	CR (DOS text)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength == 0) return 0;
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

		case '1':
			initializeCfdp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToCfdp() < 0)
			{
				return 0;
			}

			if (tokenCount < 2)
			{
				printText("Can't start CFDP: no UTA command.");
				return 0;
			}

			if (cfdpStart(tokens[1]) < 0)
			{
				putErrmsg("can't start CFDP.", NULL);
			}

			return 0;

		case 'x':
			if (attachToCfdp() < 0)
			{
				return 0;
			}

			cfdpStop();
			return 0;

		case 'm':
			executeManage(tokenCount, tokens);
			return 0;

		case 'i':
			executeInfo();
			return 0;

		case 'w':
			switchWatch(tokenCount, tokens);
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
int	cfdpadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char		*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	FILE		*cmdFile;
	char		line[256];

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

				perror("cfdpadmin fgets failed");
				break;		/*	Out of loop.	*/
			}

			if (processLine(line))
			{
				break;		/*	Out of loop.	*/
			}
		}
	}
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		if (attachToCfdp() == 0)
		{
			cfdpStop();
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

					perror("cfdpadmin fgets failed");
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
	printText("Stopping cfdpadmin.");
	ionDetach();
	return 0;
}
