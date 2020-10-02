/*
	tcaadmin.c:	TCA authority adminstration interface.
									*/
/*	Copyright (c) 2020, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "tcaP.h"

static int	_blocksGroupNbr(int *newValue)
{
	static int	blocksGroupNbr = -1;

	if (newValue)
	{
		blocksGroupNbr = *newValue;
	}

	return blocksGroupNbr;
}

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
			"Syntax error at line %d of tcaadmin.c", lineNbr);
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
	PUTS("\t   1 <multicast group number for TC bulletins> <multicast \
group number for TC records> <nbr of authorities in collective> [ <K> [ <R> ]]");
	PUTS("\t\tK is blocks per bulletin, defaulting to 50");
	PUTS("\t\tR is redundancy, 0.0 < R < 1.0, defaulting to .2");
	PUTS("\tm\tManage");
	PUTS("\t   m compiletime <scheduled time of next compilation>");
	PUTS("\t\tTime format is yyyy/mm/dd-hh:mm:ss.");
	PUTS("\t   m interval <bulletin compilation interval, in seconds>");
	PUTS("\t   m grace <bulletin consensus grace period, in seconds>");
	PUTS("\t   m hijack { 0 | 1 }");
	PUTS("\t\tSets 'hijacked' flag, for testing purposes only.");
	PUTS("\t+\tEnable authority");
	PUTS("\t   + <authority array index> <node number>");
	PUTS("\t-\tDisable authority");
	PUTS("\t   - <authority array index>");
	PUTS("\ta\tAdd authorized client");
	PUTS("\t   a <node number>");
	PUTS("\td\tDelete authorized client");
	PUTS("\t   d <node number>");
	PUTS("\tl\tList authorized clients");
	PUTS("\t   l");
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

static void	initializeTca(int tokenCount, char **tokens)
{
	int	bulletinsGroupNbr;
	int	recordsGroupNbr;
	int	numAuths;
	int	K = 50;			/*	Default.		*/
	double	R = .2;			/*	Default.		*/

	switch (tokenCount)
	{
	case 6:
		R = atof(tokens[5]);

		/*	Intentional fall-through to next case.		*/
	case 5:
		K = atoi(tokens[4]);

		/*	Intentional fall-through to next case.		*/
	case 4:
		numAuths = atoi(tokens[3]);
		recordsGroupNbr = atoi(tokens[2]);
		bulletinsGroupNbr = atoi(tokens[1]);
		break;

	default:
		SYNTAX_ERROR;
		return;
	}

	if (tcaInit(_blocksGroupNbr(NULL), bulletinsGroupNbr, recordsGroupNbr,
			numAuths, K, R) < 0)
	{
		putErrmsg("tcaadmin can't initialize TCA.", NULL);
	}
}

static int	attachToTca()
{
	if (tcaAttach(_blocksGroupNbr(NULL)) < 0)
	{
		printText("TCA not initialized yet.");
		return -1;
	}

	return 0;
}

static void	manageCompileTime(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	time_t	newCompileTime;
	TcaDB	db;

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
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	db.nextCompilationTime = newCompileTime;
	sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change compiletime.", NULL);
	}
}

static void	manageInterval(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	TcaDB	db;
	int	newInterval;

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
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	db.compilationInterval = newInterval;
	sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change interval.", NULL);
	}
}

static void	manageGrace(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	TcaDB	db;
	int	newGrace;

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
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	db.consensusInterval = newGrace;
	sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't change grace.", NULL);
	}
}

static void	manageHijack(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	Object	dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	TcaDB	db;
	int	newHijacked;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	newHijacked = ((atoi(tokens[2])) == 0 ? 0 : 1);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	db.hijacked = newHijacked;
	sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
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
	Object		dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	int		idx;
	int		i;
	uvast		nodeNbr;
	TcaDB		db;
	Object		elt;
	Object		authObj;
	TcaAuthority	auth;

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
	nodeNbr = strtouvast(tokens[2]);
	if (nodeNbr < 1)
	{
		putErrmsg("authority node number invalid.", tokens[2]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	if (idx < 0 || idx >= sdr_list_length(sdr, db.authorities))
	{
		putErrmsg("authority index value invalid.", tokens[1]);
		return;
	}

	for (i = 0, elt = sdr_list_first(sdr, db.authorities); elt;
			i++, elt = sdr_list_next(sdr, elt))
	{
		if (i == idx)
		{
			break;
		}
	}

	authObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &auth, authObj, sizeof(TcaAuthority));
	auth.inService = 1;
	auth.nodeNbr = nodeNbr;
	if (auth.nodeNbr == getOwnNodeNbr())
	{
		db.ownAuthIdx = idx;
		sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
	}

	sdr_write(sdr, authObj, (char *) &auth, sizeof(TcaAuthority));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't enable authority.", NULL);
	}
}

static void	executeDisable(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	int		idx;
	int		i;
	TcaDB		db;
	Object		elt;
	Object		authObj;
	TcaAuthority	auth;

	if (tokenCount < 2)
	{
		printText("Disable which authority?");
		return;
	}

	idx = atoi(tokens[1]);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	if (idx < 0 || idx >= sdr_list_length(sdr, db.authorities))
	{
		putErrmsg("authority index value invalid.", tokens[1]);
		return;
	}

	for (i = 0, elt = sdr_list_first(sdr, db.authorities); elt;
			i++, elt = sdr_list_next(sdr, elt))
	{
		if (i == idx)
		{
			break;
		}
	}

	authObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &auth, authObj, sizeof(TcaAuthority));
	auth.inService = 0;
	if (auth.nodeNbr == getOwnNodeNbr())
	{
		db.ownAuthIdx = -1;
		sdr_write(sdr, dbobj, (char *) &db, sizeof(TcaDB));
	}

	sdr_write(sdr, authObj, (char *) &auth, sizeof(TcaAuthority));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't disable authority.", NULL);
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	uvast		nodeNbr;
	TcaDB		db;
	Object		elt;
	uvast		client;

	if (tokenCount < 2)
	{
		printText("Add which authorized client?");
		return;
	}

	nodeNbr = strtouvast(tokens[1]);
	if (nodeNbr < 1)
	{
		putErrmsg("client node number invalid.", tokens[1]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	if (db.validClients == 0)	/*	No valid clients yet.	*/
	{
		db.validClients = sdr_list_create(sdr);
		if (db.validClients == 0)
		{
			putErrmsg("Can't add authorized clients list.", NULL);
			sdr_cancel_xn(sdr);
			return;
		}
	}

	for (elt = sdr_list_first(sdr, db.validClients); elt;
			elt = sdr_list_next(sdr, elt))
	{
		client = (uvast) sdr_list_data(sdr, elt);
		if (client < nodeNbr)
		{
			continue;
		}

		break;
	}

	if (elt)
	{
		if (client == nodeNbr)
		{
			printText("Client is already authorized.");
			return;
		}

		/*	Client must be > nodeNbr.			*/

		sdr_list_insert_before(sdr, elt, nodeNbr);
	}
	else
	{
		sdr_list_insert_last(sdr, db.validClients, nodeNbr);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add authorized client.", NULL);
	}
}

static void	executeDelete(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	uvast		nodeNbr;
	TcaDB		db;
	Object		elt;
	uvast		client;

	if (tokenCount < 2)
	{
		printText("Delete which authorized client?");
		return;
	}

	nodeNbr = strtouvast(tokens[1]);
	if (nodeNbr < 1)
	{
		putErrmsg("client node number invalid.", tokens[1]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	if (db.validClients == 0)
	{
		printText("Unlimited client authorization for this authority.");
		sdr_exit_xn(sdr);
		return;
	}

	for (elt = sdr_list_first(sdr, db.validClients); elt;
			elt = sdr_list_next(sdr, elt))
	{
		client = (uvast) sdr_list_data(sdr, elt);
		if (client < nodeNbr)
		{
			continue;
		}

		break;
	}

	if (elt && client == nodeNbr)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}
	else
	{
		printText("Not a currently authorized client.");
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't delete authorized client.", NULL);
	}
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	TcaDB		db;
	Object		elt;
	uvast		client;
	char		buffer[32];

	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	if (db.validClients == 0)
	{
		printText("Unlimited client authorization for this authority.");
		sdr_exit_xn(sdr);
		return;
	}

	printText("Authorized clients:");
	for (elt = sdr_list_first(sdr, db.validClients); elt;
			elt = sdr_list_next(sdr, elt))
	{
		client = (uvast) sdr_list_data(sdr, elt);
		isprintf(buffer, sizeof buffer, "\t" UVAST_FIELDSPEC, client);
		printText(buffer);
	}

	sdr_exit_xn(sdr);
}

static void	executeInfo()
{
	Sdr		sdr = getIonsdr();
	Object		dbobj;
	TcaDB		db;
	char		buffer[256];
	char		current[TIMESTAMPBUFSZ];
	char		next[TIMESTAMPBUFSZ];
	int		i;
	Object		elt;
	Object		authObj;
	TcaAuthority	auth;

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	dbobj = getTcaDBObject(_blocksGroupNbr(NULL));
	CHKVOID(dbobj);
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TcaDB));
	writeTimestampUTC(db.currentCompilationTime, current);
	writeTimestampUTC(db.nextCompilationTime, next);
	isprintf(buffer, sizeof buffer, "\nbulletins group = %d, records group \
= %d, current compilationTime=%s, next compilationTime=%s, compilation \
interval=%u, consensus interval=%u", db.bulletinsGroupNbr, db.recordsGroupNbr,
			current, next, db.compilationInterval,
			db.consensusInterval);
	printText(buffer);
	printText("Authorities:");
	for (elt = sdr_list_first(sdr, db.authorities), i = 0; elt;
			elt = sdr_list_next(sdr, elt), i++)
	{
		authObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &auth, authObj, sizeof(TcaAuthority));
		isprintf(buffer, sizeof buffer, "\t%d\t%d\t" UVAST_FIELDSPEC,
				i, auth.inService, auth.nodeNbr);
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
			initializeTca(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToTca() == 0)
			{
				if (tcaStart(_blocksGroupNbr(NULL)) < 0)
				{
					putErrmsg("Can't start TCA.", NULL);
				}

				/* Wait for tca to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (tcaIsStarted(_blocksGroupNbr(NULL)) == 0)
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
			if (attachToTca() == 0)
			{
				tcaStop(_blocksGroupNbr(NULL));
			}

			return 0;

		case 'm':
			if (attachToTca() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case '+':
			if (attachToTca() == 0)
			{
				executeEnable(tokenCount, tokens);
			}

			return 0;

		case '-':
			if (attachToTca() == 0)
			{
				executeDisable(tokenCount, tokens);
			}

			return 0;

		case 'a':
			if (attachToTca() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToTca() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToTca() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToTca() == 0)
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
int	tcaadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	int	blocksGroupNbr = (a1 ? atoi((char *) a1) : -1);
	char	*cmdFileName = (a2 ? (char *) a2 : NULL);
#else
int	main(int argc, char **argv)
{
	int	blocksGroupNbr = (argc > 1 ? atoi(argv[1]) : -1);
	char	*cmdFileName = (argc > 2 ? argv[2] : NULL);
#endif
	int	cmdFile;
	char	line[256];
	int	len;

	_blocksGroupNbr(&blocksGroupNbr);
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
		if (tcaAttach(_blocksGroupNbr(NULL)) == 0)
		{
			tcaStop(_blocksGroupNbr(NULL));
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
	printText("Stopping tcaadmin.");
	ionDetach();

	return 0;
}
