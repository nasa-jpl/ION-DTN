/*

	cfdpadmin.c:	CFDP engine adminstration interface.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdpP.h"

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
			"Syntax error at line %d of cfdpadmin.c", lineNbr);
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
	PUTS("\t   1");
	PUTS("\tm\tManage");
	PUTS("\t   m discard { 0 | 1 }");
	PUTS("\t   m requirecrc { 0 | 1 }");
	PUTS("\t   m fillchar <file fill character in hex, e.g., 0xaa>");
	PUTS("\t   m ckperiod <check cycle period, in seconds>");
	PUTS("\t   m maxtimeouts <max number of check cycle timeouts>");
	PUTS("\t   m maxtrnbr <max transaction number>");
	PUTS("\t   m segsize <max bytes per file data segment>");
	PUTS("\t   m inactivity <inactivity limit, in seconds>");
	PUTS("\ti\tInfo");
	PUTS("\t   i");
	PUTS("\ts\tStart");
	PUTS("\t   s '<UTA command>'");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\tw\tWatch CFDP activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\t\tActivity spec is a string of all requested activity \
indication characters, e.g., pq.  See man(5) for cfdprc.");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeCfdp(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (cfdpInit() < 0)
	{
		putErrmsg("cfdpadmin can't initialize CFDP.", NULL);
	}
}

static int	attachToCfdp()
{
	if (cfdpAttach() < 0)
	{
		printText("CFDP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	manageDiscard(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newDiscard;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newDiscard = atoi(tokens[2]);
	if (newDiscard != 0 && newDiscard != 1)
	{
		putErrmsg("discardIncompleteFile switch invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newRequirecrc;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newRequirecrc = atoi(tokens[2]);
	if (newRequirecrc != 0 && newRequirecrc != 1)
	{
		putErrmsg("crcRequired switch invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newFillchar;
	char	*trailing;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newFillchar = strtol(tokens[2], &trailing, 16);
	if (*trailing != '\0')
	{
		putErrmsg("fillCharacter invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newCkperiod;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newCkperiod = atoi(tokens[2]);
	if (newCkperiod < 1)
	{
		putErrmsg("checkTimerPeriod invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtimeouts;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxtimeouts = atoi(tokens[2]);
	if (newMaxtimeouts < 0)
	{
		putErrmsg("checkTimeoutLimit invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newMaxtrnbr;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newMaxtrnbr = atoi(tokens[2]);
	if (newMaxtrnbr < 0)
	{
		putErrmsg("maxTransactionNbr invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newSegsize;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newSegsize = atoi(tokens[2]);
	if (newSegsize < 0)
	{
		putErrmsg("maxFileDataLength invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.maxFileDataLength = newSegsize;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change maxFileDataLength.", NULL);
	}
}

static void	manageInactivity(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	cfdpdbObj = getCfdpDbObject();
	CfdpDB	cfdpdb;
	int	newLimit;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newLimit = atoi(tokens[2]);
	if (newLimit < 0)
	{
		putErrmsg("transactionInactivityLimit invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &cfdpdb, cfdpdbObj, sizeof(CfdpDB));
	cfdpdb.transactionInactivityLimit = newLimit;
	sdr_write(sdr, cfdpdbObj, (char *) &cfdpdb, sizeof(CfdpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change transactionInactivityLimit.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
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

	if (strcmp(tokens[1], "inactivity") == 0)
	{
		manageInactivity(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo()
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(CfdpDB, db);
	char	buffer[256];

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, CfdpDB, db, getCfdpDbObject());
	isprintf(buffer, sizeof buffer, "xncount=%lu, maxtrnbr=%lu, \
fillchar=0x%x, discard=%hu, requirecrc=%hu, segsize=%hu, inactivity=%u, \
ckperiod=%u, maxtimeouts=%u", db->transactionCounter, db->maxTransactionNbr,
			db->fillCharacter, db->discardIncompleteFile,
			db->crcRequired, db->maxFileDataLength, 
			db->transactionInactivityLimit, db->checkTimerPeriod,
			db->checkTimeoutLimit);
	sdr_exit_xn(sdr);
	printText(buffer);
}

static void	switchWatch(int tokenCount, char **tokens)
{
	CfdpVdb	*vdb = getCfdpVdb();
	char	buffer[80];
	char	*cursor;

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
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	char		buffer[80];
	struct timeval	done_time;
	struct timeval	cur_time;

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
			initializeCfdp(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToCfdp() == 0)
			{
				if (tokenCount < 2)
				{
					printText("Can't start CFDP: no UTA \
command.");
				}
				else
				{
					if (cfdpStart(tokens[1]) < 0)
					{
						putErrmsg("Can't start CFDP.",
								NULL);
					}
				}

				/* Wait for cfdp to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (cfdp_entity_is_started() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
					    done_time.tv_sec 
					    && cur_time.tv_usec >=
					    done_time.tv_usec) {
						printText("[?]  start hung up,\
 abandoned.");
						break;
					}
				}

			}

			return 0;

		case 'x':
			if (attachToCfdp() == 0)
			{
				cfdpStop();
			}

			return 0;

		case 'm':
			if (attachToCfdp() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToCfdp() == 0)
			{
				executeInfo();
			}

			return 0;

		case 'w':
			if (attachToCfdp() == 0)
			{
				switchWatch(tokenCount, tokens);
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

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	cfdpadmin(int a1, int a2, int a3, int a4, int a5,
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
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		if (cfdpAttach() == 0)
		{
			cfdpStop();
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

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping cfdpadmin.");
	ionDetach();

	return 0;
}
