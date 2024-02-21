/*

	bibeadmin.c:	Bundle-in-bundle encapsulation adminstration
			interface.
									*/
/*									*/
/*	Copyright (c) 2019, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include "bibeP.h"

#ifdef INPUT_HISTORY
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

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

	isprintf(buffer, sizeof buffer, "Syntax error @ line %d of bibeadmin.c",
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
	PUTS("\tv\tPrint version of ION and crypto suite.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a bcla <peer EID> <fwd> <rtn> <rptTo> <bsrFlags> <lifespan> \
<priority> <ordinal> <qosFlags> [<data label>]");
	PUTS("\tc\tChange");
	PUTS("\t   c bcla <peer EID> <fwd> <rtn> <rptTo> <bsrFlags> <lifespan> \
<priority> <ordinal> <qosFlags> [<data label>]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} bcla <peer EID>");
	PUTS("\tl\tList");
	PUTS("\t   l bcla <scheme name>");
	PUTS("\tw\tWatch BIBE custody transfer activity");
	PUTS("\t   w { 0 | 1 | <activity spec> }");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text, ignored by the program>");
}

static int	attachToBp()
{
	if (bpAttach() < 0)
	{
		printText("BP not initialized yet.");
		return -1;
	}

	return 0;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	unsigned int	fwdLatency;
	unsigned int	rtnLatency;
	unsigned char	bsrFlags;
	int		lifespan;
	unsigned char	priority;
	unsigned char	ordinal;
	unsigned char	qosFlags;
	unsigned int	label;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "bcla") == 0)
	{
		switch (tokenCount)
		{
		case 12:
			label = strtoul(tokens[11], NULL, 0);
			break;

		case 11:
			label = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		fwdLatency = strtoul(tokens[3], NULL, 0);
		rtnLatency = strtoul(tokens[4], NULL, 0);
		bsrFlags = strtoul(tokens[6], NULL, 0);
		lifespan = strtoul(tokens[7], NULL, 0);
		priority = strtoul(tokens[8], NULL, 0);
		ordinal = strtoul(tokens[9], NULL, 0);
		qosFlags = strtoul(tokens[10], NULL, 0);
		bibeAdd(tokens[2], fwdLatency, rtnLatency, tokens[5],
				bsrFlags, lifespan, priority, ordinal,
				qosFlags, label);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	unsigned int	fwdLatency;
	unsigned int	rtnLatency;
	unsigned char	bsrFlags;
	int		lifespan;
	unsigned char	priority;
	unsigned char	ordinal;
	unsigned char	qosFlags;
	unsigned int	label;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "bcla") == 0)
	{
		switch (tokenCount)
		{
		case 12:
			label = strtoul(tokens[11], NULL, 0);
			break;

		case 11:
			label = 0;
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		fwdLatency = strtoul(tokens[3], NULL, 0);
		rtnLatency = strtoul(tokens[4], NULL, 0);
		bsrFlags = strtoul(tokens[6], NULL, 0);
		lifespan = strtoul(tokens[7], NULL, 0);
		priority = strtoul(tokens[8], NULL, 0);
		ordinal = strtoul(tokens[9], NULL, 0);
		qosFlags = strtoul(tokens[10], NULL, 0);
		bibeChange(tokens[2], fwdLatency, rtnLatency, tokens[5],
				bsrFlags, lifespan, priority, ordinal,
				qosFlags, label);
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

	if (strcmp(tokens[1], "bcla") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		bibeDelete(tokens[2]);
		return;
	}

	SYNTAX_ERROR;
}

static void	printBcla(Bcla *bcla)
{
	Sdr	sdr = getIonsdr();
	char	peerEid[SDRSTRING_BUFSZ];
	char	reportToEid[SDRSTRING_BUFSZ];
	char	buffer[1024];

	sdr_string_read(sdr, peerEid, bcla->dest);
	if (bcla->reportTo)
	{
		sdr_string_read(sdr, reportToEid, bcla->reportTo);
	}
	else
	{
		reportToEid[0] = '\0';
	}

	isprintf(buffer, sizeof buffer, "%.255s\tfwd: %lu  rtn: %lu  \
reportTo: '%s'  bsrFlags: %c  lifespan: %d  priority: %c  ordinal: %c  \
qosFlags: %c  data label: %u  count: " UVAST_FIELDSPEC, peerEid,
			bcla->fwdLatency, bcla->rtnLatency,
			reportToEid, bcla->bsrFlags,
			bcla->lifespan, bcla->classOfService,
			bcla->ancillaryData.ordinal,
			bcla->ancillaryData.flags,
			bcla->ancillaryData.dataLabel, bcla->count);

	printText(buffer);
}

static void	infoBcla(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		obj;
	Object		elt;
	Bcla		bcla;

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	bibeFind(tokens[2], &obj, &elt);
	if (elt == 0)
	{
		printText("Unknown BIBE CLA.");
	}
	else
	{
		sdr_read(sdr, (char *) &bcla, obj, sizeof(Bcla));
		printBcla(&bcla);
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

	if (strcmp(tokens[1], "bcla") == 0)
	{
		infoBcla(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listBclas(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Scheme		scheme;
	Object		elt;
	Bcla		bcla;

	CHKVOID(sdr_begin_xn(sdr));
	findScheme(tokens[2], &vscheme, &vschemeElt);
	if (vschemeElt == 0)
	{
		printText("Unknown scheme.");
	}
	else
	{
		sdr_read(sdr, (char *) &scheme, sdr_list_data(sdr,
				vscheme->schemeElt), sizeof(Scheme));
		for (elt = sdr_list_first(sdr, scheme.bclas); elt;
				elt = sdr_list_next(sdr, elt))
		{
			sdr_read(sdr, (char *) &bcla, sdr_list_data(sdr, elt),
					sizeof(Bcla));
			printBcla(&bcla);
		}
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

	if (strcmp(tokens[1], "bcla") == 0)
	{
		listBclas(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	noteWatchValue()
{
	BpVdb	*vdb = getBpVdb();
	Sdr	sdr = getIonsdr();
	Object	dbObj = getBpDbObject();
	BpDB	db;

	if (vdb != NULL && dbObj != 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &db, dbObj, sizeof(BpDB));
		db.watching = vdb->watching;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(BpDB));
		oK(sdr_end_xn(sdr));
	}
}

static void	switchWatch(int tokenCount, char **tokens)
{
	BpVdb	*vdb = getBpVdb();
	char	buffer[80];
	char	*cursor;

	if (tokenCount < 2)
	{
		printText("Switch watch in what way?");
		return;
	}

	if (strcmp(tokens[1], "1") == 0)
	{
		vdb->watching |= WATCH_BIBE;
		return;
	}

	vdb->watching &= ~(WATCH_BIBE);
	if (strcmp(tokens[1], "0") == 0)
	{
		return;
	}

	cursor = tokens[1];
	while (*cursor)
	{
		switch (*cursor)
		{
		case 'w':
			vdb->watching |= WATCH_w;
			break;

		case 'm':
			vdb->watching |= WATCH_m;
			break;

		case 'x':
			vdb->watching |= WATCH_x;
			break;

		case '&':
			vdb->watching |= WATCH_refusal;
			break;

		case '$':
			vdb->watching |= WATCH_timeout;
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

static int	processLine(char *line, int lineLength, int *rc)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[15];

	tokenCount = 0;
	for (cursor = line, i = 0; i < 15; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			if (tokens[i])
			{
				tokenCount++;
			}
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
			if (attachToBp() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attachToBp() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToBp() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToBp() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToBp() == 0)
			{
				executeList(tokenCount, tokens);
			}

			return 0;

		case 'w':
			if (attachToBp() == 0)
			{
				switchWatch(tokenCount, tokens);
				noteWatchValue();
			}

			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 'q':
			return 1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
int	bibeadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	rc = 0;
	int	cmdFile;
	char	line[256];
	int	len;

#ifdef INPUT_HISTORY
	char *input;
#endif
	
	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef FSWLOGGER
		return 0;			/*	No stdout.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
#ifdef INPUT_HISTORY
			/* add input history */
			if ((input = readline(": ")) != NULL)
			{
				len = strlen(input);
				
				if (len == 0)
				{
					continue;
				}

				/* received input */
				if (len > 0) 
				{
            		add_history(input);
        		}
				
				if (len > sizeof(line) - 1 ) 
				{
					printf("\nInput is too long. Ignored.\n");
					fflush(stdout);
					continue;
				}
			}
			else
			{
				/* input error detected */
				printf("\nInput error detected. Exiting.\n");
				fflush(stdout);
				free(input);
				break;
			}

			/* copy the input to line for processing
			 * input sized already checked */

			strcpy(line, input);

			if (processLine(line, len, &rc))
			{
				free(input);
				break;		/*	Out of loop.	*/
			}
#else
			/* original input handling*/
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

			if (processLine(line, len, &rc))
			{
				break;		/*	Out of loop.	*/
			}
#endif
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

				if (processLine(line, len, &rc))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping bibeadmin.");
	ionDetach();
	return rc;
}

#ifdef STRSOE
int	bibeadmin_processLine(char *line, int lineLength, int *rc)
{
	return processLine(line, lineLength, rc);
}

void	bibeadmin_help(void)
{
	printUsage();
}
#endif
