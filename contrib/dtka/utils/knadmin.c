/*
	knadmin.c:	DTKA authority adminstration interface.
									*/
/*	Copyright (c) 2013, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Nodeor: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "knode.h"

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
			"Syntax error at line %d of knadmin.c", lineNbr);
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
	PUTS("\t   m keygentime <time>");
	PUTS("\t\tTime format is yyyy/mm/dd-hh:mm:ss.");
	PUTS("\t   m interval <new key pair generation interval, in seconds>");
	PUTS("\t   m leadtime <key pair effectiveness lead time, in seconds>");
	PUTS("\t   m authority <authority array index> <node number>");
	PUTS("\ti\tInfo");
	PUTS("\t   i");
	PUTS("\ts\tStart");
	PUTS("\t   s");
	PUTS("\tx\tStop");
	PUTS("\t   x");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeKnode(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (knodeInit() < 0)
	{
		putErrmsg("knadmin can't initialize DTKA.", NULL);
	}
}

static int	attachToKnode()
{
	if (knodeAttach() < 0)
	{
		printText("DTKA not initialized yet.");
		return -1;
	}

	return 0;
}

static void	manageKeyGenTime(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		knodedbObj = getKnodeDbObject();
	time_t		newKeyGenTime;
	DtkaNodeDB	knodedb;

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
	sdr_stage(sdr, (char *) &knodedb, knodedbObj, sizeof(DtkaNodeDB));
	knodedb.nextKeyGenTime = newKeyGenTime;
	sdr_write(sdr, knodedbObj, (char *) &knodedb, sizeof(DtkaNodeDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change keygentime.", NULL);
	}
}

static void	manageInterval(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		knodedbObj = getKnodeDbObject();
	DtkaNodeDB	knodedb;
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
	sdr_stage(sdr, (char *) &knodedb, knodedbObj, sizeof(DtkaNodeDB));
	knodedb.keyGenInterval = newInterval;
	sdr_write(sdr, knodedbObj, (char *) &knodedb, sizeof(DtkaNodeDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change interval.", NULL);
	}
}

static void	manageLeadTime(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		knodedbObj = getKnodeDbObject();
	DtkaNodeDB	knodedb;
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
	sdr_stage(sdr, (char *) &knodedb, knodedbObj, sizeof(DtkaNodeDB));
	knodedb.effectiveLeadTime = newLeadTime;
	sdr_write(sdr, knodedbObj, (char *) &knodedb, sizeof(DtkaNodeDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change leadtime.", NULL);
	}
}

static void	manageAuthority(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		knodedbObj = getKnodeDbObject();
	DtkaNodeDB	knodedb;
	int		idx;
	uvast		nodeNbr;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	idx = atoi(tokens[2]);
	if (idx < 0 || idx >= DTKA_NUM_AUTHS)
	{
		putErrmsg("authority index value invalid.", tokens[2]);
		return;
	}

	nodeNbr = strtouvast(tokens[3]);
	if (nodeNbr < 1)
	{
		putErrmsg("authority node number invalid.", tokens[3]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &knodedb, knodedbObj, sizeof(DtkaNodeDB));
	knodedb.authorities[idx].nodeNbr = nodeNbr;
	sdr_write(sdr, knodedbObj, (char *) &knodedb, sizeof(DtkaNodeDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't manage authority.", NULL);
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

	if (strcmp(tokens[1], "authority") == 0)
	{
		manageAuthority(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeInfo()
{
	Sdr		sdr = getIonsdr();
			OBJ_POINTER(DtkaNodeDB, db);
	char		buffer[256];
	char		next[TIMESTAMPBUFSZ];
	int		i;
	DtkaAuthority	*auth;

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, DtkaNodeDB, db, getKnodeDbObject());
	writeTimestampUTC(db->nextKeyGenTime, next);
	isprintf(buffer, sizeof buffer, "nextKeyGentime=%s, keyGenInterval=%u, \
effectiveLeadTime=%u", next, db->keyGenInterval, db->effectiveLeadTime);
	printText(buffer);
	printText("Authorities:");
	for (i = 0, auth = db->authorities; i < DTKA_NUM_AUTHS; i++, auth++)
	{
		isprintf(buffer, sizeof buffer, "\t%d\t" UVAST_FIELDSPEC, i,
				auth->nodeNbr);
		printText(buffer);
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
			initializeKnode(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToKnode() == 0)
			{
				if (knodeStart(tokens[1]) < 0)
				{
					putErrmsg("Can't start DTKA.", NULL);
				}

				/* Wait for knode to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (knodeIsStarted() == 0)
				{
					snooze(1);
					getCurrentTime(&cur_time);
					if (cur_time.tv_sec >=
						done_time.tv_sec 
					&& cur_time.tv_usec >=
						done_time.tv_usec)
					{
						printText("[?] start hung up, \
abandoned.");
						break;
					}
				}
			}

			return 0;

		case 'x':
			if (attachToKnode() == 0)
			{
				knodeStop();
			}

			return 0;

		case 'm':
			if (attachToKnode() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToKnode() == 0)
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

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	knadmin(int a1, int a2, int a3, int a4, int a5,
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
		if (knodeAttach() == 0)
		{
			knodeStop();
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
	printText("Stopping knadmin.");
	ionDetach();

	return 0;
}
