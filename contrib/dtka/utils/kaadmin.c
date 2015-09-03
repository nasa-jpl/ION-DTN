/*
	kaadmin.c:	DTKA authority adminstration interface.
									*/
/*	Copyright (c) 2013, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "kauth.h"

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
			"Syntax error at line %d of kaadmin.c", lineNbr);
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
	PUTS("\t   m compiletime <time>");
	PUTS("\t\tTime format is yyyy/mm/dd-hh:mm:ss.");
	PUTS("\t   m interval <bulletin compilation interval, in seconds>");
	PUTS("\t   m grace <bulletin consensus grace time, in seconds>");
	PUTS("\t   m hijack { 0 | 1 }");
	PUTS("\t\tSets 'hijacked' flag, for testing purposes only.");
	PUTS("\t+\tEnable authority");
	PUTS("\t   + <authority array index> <node number>");
	PUTS("\t-\tDisable authority");
	PUTS("\t   - <authority array index>");
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

static void	initializeKauth(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (kauthInit() < 0)
	{
		putErrmsg("kaadmin can't initialize DTKA.", NULL);
	}
}

static int	attachToKauth()
{
	if (kauthAttach() < 0)
	{
		printText("DTKA not initialized yet.");
		return -1;
	}

	return 0;
}

static void	manageCompileTime(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		kauthdbObj = getKauthDbObject();
	time_t		newCompileTime;
	DtkaAuthDB	kauthdb;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newCompileTime = readTimestampUTC(tokens[2], 0);
	if (newCompileTime == 0)
	{
		putErrmsg("compiletime invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &kauthdb, kauthdbObj, sizeof(DtkaAuthDB));
	kauthdb.nextCompilationTime = newCompileTime;
	sdr_write(sdr, kauthdbObj, (char *) &kauthdb, sizeof(DtkaAuthDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change compiletime.", NULL);
	}
}

static void	manageInterval(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		kauthdbObj = getKauthDbObject();
	DtkaAuthDB	kauthdb;
	int		newInterval;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newInterval = atoi(tokens[2]);
	if (newInterval < 1)
	{
		putErrmsg("interval invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &kauthdb, kauthdbObj, sizeof(DtkaAuthDB));
	kauthdb.compilationInterval = newInterval;
	sdr_write(sdr, kauthdbObj, (char *) &kauthdb, sizeof(DtkaAuthDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change interval.", NULL);
	}
}

static void	manageGrace(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		kauthdbObj = getKauthDbObject();
	DtkaAuthDB	kauthdb;
	int		newGrace;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newGrace = atoi(tokens[2]);
	if (newGrace < 1)
	{
		putErrmsg("grace invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &kauthdb, kauthdbObj, sizeof(DtkaAuthDB));
	kauthdb.consensusInterval = newGrace;
	sdr_write(sdr, kauthdbObj, (char *) &kauthdb, sizeof(DtkaAuthDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change grace.", NULL);
	}
}

static void	manageHijack(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		kauthdbObj = getKauthDbObject();
	DtkaAuthDB	kauthdb;
	int		newHijacked;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newHijacked = ((atoi(tokens[2])) == 0 ? 0 : 1);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &kauthdb, kauthdbObj, sizeof(DtkaAuthDB));
	kauthdb.hijacked = newHijacked;
	sdr_write(sdr, kauthdbObj, (char *) &kauthdb, sizeof(DtkaAuthDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change authority hijack state.", NULL);
	}
}

static void	executeManage(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Manage what?");
		return;
	}

	if (strcmp(tokens[1], "compiletime") == 0)
	{
		manageCompileTime(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "interval") == 0)
	{
		manageInterval(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "grace") == 0)
	{
		manageGrace(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "hijack") == 0)
	{
		manageHijack(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeEnable(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		kauthdbObj = getKauthDbObject();
	DtkaAuthDB	kauthdb;
	int		idx;
	uvast		nodeNbr;

	if (tokenCount < 3)
	{
		if (tokenCount < 2)
		{
			printText("Enable which authority?");
		}
		else
		{
			printText("What node number for this authority?");
		}

		return;
	}

	idx = atoi(tokens[1]);
	if (idx < 0 || idx >= DTKA_NUM_AUTHS)
	{
		putErrmsg("authority index value invalid.", tokens[1]);
		return;
	}

	nodeNbr = strtouvast(tokens[2]);
	if (nodeNbr < 1)
	{
		putErrmsg("authority node number invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &kauthdb, kauthdbObj, sizeof(DtkaAuthDB));
	if (kauthdb.authorities[idx].nodeNbr == getOwnNodeNbr())
	{
		kauthdb.ownAuthIdx = -1;
	}

	kauthdb.authorities[idx].inService = 1;
	kauthdb.authorities[idx].nodeNbr = nodeNbr;
	if (kauthdb.authorities[idx].nodeNbr == getOwnNodeNbr())
	{
		kauthdb.ownAuthIdx = idx;
	}

	sdr_write(sdr, kauthdbObj, (char *) &kauthdb, sizeof(DtkaAuthDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't enable authority.", NULL);
	}
}

static void	executeDisable(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		kauthdbObj = getKauthDbObject();
	DtkaAuthDB	kauthdb;
	int		idx;

	if (tokenCount < 2)
	{
		printText("Disable which authority?");
		return;
	}

	idx = atoi(tokens[1]);
	if (idx < 0 || idx >= DTKA_NUM_AUTHS)
	{
		putErrmsg("authority index value invalid.", tokens[1]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &kauthdb, kauthdbObj, sizeof(DtkaAuthDB));
	if (kauthdb.authorities[idx].nodeNbr == getOwnNodeNbr())
	{
		kauthdb.ownAuthIdx = -1;
	}

	kauthdb.authorities[idx].inService = 0;
	sdr_write(sdr, kauthdbObj, (char *) &kauthdb, sizeof(DtkaAuthDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't disable authority.", NULL);
	}
}

static void	executeInfo()
{
	Sdr		sdr = getIonsdr();
			OBJ_POINTER(DtkaAuthDB, db);
	char		buffer[256];
	char		current[TIMESTAMPBUFSZ];
	char		next[TIMESTAMPBUFSZ];
	int		i;
	DtkaAuthority	*auth;

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, DtkaAuthDB, db, getKauthDbObject());
	writeTimestampUTC(db->currentCompilationTime, current);
	writeTimestampUTC(db->nextCompilationTime, next);
	isprintf(buffer, sizeof buffer, "currentCompilationTime=%s, \
nextCompilationTime=%s, compilationInterval=%u, consensusInterval=%u",
			current, next, db->compilationInterval,
			db->consensusInterval);
	printText(buffer);
	printText("Authorities:");
	for (i = 0, auth = db->authorities; i < DTKA_NUM_AUTHS; i++, auth++)
	{
		isprintf(buffer, sizeof buffer, "\t%d\t%d\t" UVAST_FIELDSPEC,
				i, auth->inService, auth->nodeNbr);
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
			initializeKauth(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToKauth() == 0)
			{
				if (kauthStart(tokens[1]) < 0)
				{
					putErrmsg("Can't start DTKA.", NULL);
				}

				/* Wait for kauth to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (kauthIsStarted() == 0)
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
			if (attachToKauth() == 0)
			{
				kauthStop();
			}

			return 0;

		case 'm':
			if (attachToKauth() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case '+':
			if (attachToKauth() == 0)
			{
				executeEnable(tokenCount, tokens);
			}

			return 0;

		case '-':
			if (attachToKauth() == 0)
			{
				executeDisable(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToKauth() == 0)
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
int	kaadmin(int a1, int a2, int a3, int a4, int a5,
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
		if (kauthAttach() == 0)
		{
			kauthStop();
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
	printText("Stopping kaadmin.");
	ionDetach();

	return 0;
}
