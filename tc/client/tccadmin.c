/*
	tccadmin.c:	TCC authority adminstration interface.
									*/
/*	Copyright (c) 2020, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Nodeor: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "tccP.h"

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
			"Syntax error at line %d of tccadmin.c", lineNbr);
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
	PUTS("\t   1 <number of authorities in collective> [ <K> [ <R> ]]");
	PUTS("\t\tK is blocks per bulletin, defaulting to 50");
	PUTS("\t\tR is redundancy, 0.0 < R < 1.0, defaulting to .2");
	PUTS("\tm\tManage");
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

static void	initializeTcc(int tokenCount, char **tokens)
{
	int	numAuths;
	int	K = 50;			/*	Default.		*/
	double	R = .2;			/*	Default.		*/

	switch (tokenCount)
	{
	case 4:
		R = atof(tokens[3]);

		/*	Intentional fall-through to next case.		*/
	case 3:
		K = atoi(tokens[2]);

		/*	Intentional fall-through to next case.		*/
	case 2:
		numAuths = atoi(tokens[1]);
		break;

	default:
		SYNTAX_ERROR;
		return;
	}

	if (tccInit(_blocksGroupNbr(NULL), numAuths, K, R) < 0)
	{
		putErrmsg("tccadmin can't initialize TCC.", NULL);
	}
}

static int	attachToTcc()
{
	if (tccAttach(_blocksGroupNbr(NULL)) < 0)
	{
		printText("TCC not initialized yet.");
		return -1;
	}

	return 0;
}

static void	manageAuthority(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		dbobj = getTccDBObj(_blocksGroupNbr(NULL));
	int		idx;
	uvast		nodeNbr;
	TccDB		db;
	int		i;
	Object		elt;
	Object		authObj;
	TccAuthority	auth;

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	idx = atoi(tokens[2]);
	nodeNbr = strtouvast(tokens[3]);
	if (nodeNbr < 1)
	{
		putErrmsg("authority node number invalid.", tokens[3]);
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TccDB));
	if (idx < 0 || idx >= sdr_list_length(sdr, db.authorities))
	{
		sdr_exit_xn(sdr);
		putErrmsg("authority index value invalid.", tokens[2]);
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
	sdr_stage(sdr, (char *) &auth, authObj, sizeof(TccAuthority));
	auth.nodeNbr = nodeNbr;
	sdr_write(sdr, authObj, (char *) &auth, sizeof(TccAuthority));
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
	Object		dbobj = getTccDBObj(_blocksGroupNbr(NULL));
	TccDB		db;
	char		buffer[256];
	int		i;
	Object		elt;
	Object		authObj;
	TccAuthority	auth;

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock heap.	*/
	sdr_read(sdr, (char *) &db, dbobj, sizeof(TccDB));
	printText("Authorities:");
	for (elt = sdr_list_first(sdr, db.authorities), i = 0; elt;
			elt = sdr_list_next(sdr, elt), i++)
	{
		authObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char  *) &auth, authObj, sizeof(TccAuthority));
		isprintf(buffer, sizeof buffer, "\t%d\t" UVAST_FIELDSPEC, i,
				auth.nodeNbr);
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
			initializeTcc(tokenCount, tokens);
			return 0;

		case 's':
			if (attachToTcc() == 0)
			{
				if (tccStart(_blocksGroupNbr(NULL)) < 0)
				{
					putErrmsg("Can't start TCC.", NULL);
				}

				/* Wait for tcc to start up. */
				getCurrentTime(&done_time);
				done_time.tv_sec += STARTUP_TIMEOUT;
				while (tccIsStarted(_blocksGroupNbr(NULL)) == 0)
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
			if (attachToTcc() == 0)
			{
				tccStop(_blocksGroupNbr(NULL));
			}

			return 0;

		case 'm':
			if (attachToTcc() == 0)
			{
				executeManage(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToTcc() == 0)
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
int	tccadmin(int a1, int a2, int a3, int a4, int a5,
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
		if (tccAttach(_blocksGroupNbr(NULL)) == 0)
		{
			tccStop(_blocksGroupNbr(NULL));
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
	printText("Stopping tccadmin.");
	ionDetach();

	return 0;
}
