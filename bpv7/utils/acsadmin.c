/*

	acsadmin.c:	Aggregate Custody Signal adminstration interface.

	Authors: Andrew Jenkins, Sebastian Kuzminsky, 
				University of Colorado at Boulder
	Based on bpadmin.c, by Scott Burleigh/JPL

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/

#include "acs.h"

static int		echo = 0;

static void	printText(const char *text)
{
	char *textScratch = strdup(text);
	if (echo && textScratch != NULL)
	{
		writeMemo(textScratch);
	}
	free(textScratch);

	puts(text);
}

static void	handleQuit()
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of bpadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	{ printSyntaxError(__LINE__); return; }

static void	printUsage()
{
	puts("Valid commands are:");
	puts("\tq\tQuit");
	puts("\th\tHelp");
	puts("\t?\tHelp");
	puts("\tv\tPrint version of ION.");
	puts("\t1\tInitialize");
	puts("\t   1 <logLevel> [<heapWords>]");
	puts("\ta\tAdd custodian information");
	puts("\t   a <custodianEid> <acsSize> [<acsDelay>]");
	puts("\ts\tSet minimum custody ID");
	puts("\t   s <minimumCustodyId>");
	puts("\te\tEnable or disable echo of printed output to log file");
	puts("\t   e { 0 | 1 }");
	puts("\t#\tComment");
	puts("\t   # <comment text, ignored by the program>");
}

static void	initializeAcs(int tokenCount, char **tokens)
{
	long heapWordsRequested;
	int logLevelRequested;

	if (tokenCount < 2)
	{
		SYNTAX_ERROR;
	}

	logLevelRequested = atoi(tokens[1]);

	if (tokenCount == 2)
	{
		heapWordsRequested = 0;     /* means "use default" */
	} else {
		heapWordsRequested = atol(tokens[2]);
	}

	if (acsInitialize(heapWordsRequested, logLevelRequested) < 0)
	{
		printText("Couldn't initialize ACS");
		return;
	}
}

static void	executeList(int tokenCount, char **tokens)
{
	if (acsAttach() < 0) return;
	listCustodianInfo(printText);
}

static void	executeAdd(int tokenCount, char **tokens)
{
	long        acsSize;
	long        acsDelay;

        char text[200];

	if (acsAttach() < 0) return;
	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}
	if (tokenCount < 3)
	{
		printText("Does custodian support ACS?");
		return;
	}

	acsSize = atol(tokens[2]);
	if (acsSize < 0)
	{
		printText("acsSize must be >=0");
		return;
	}
	updateCustodianAcsSize(tokens[1], (unsigned long)(acsSize));

	if (tokenCount > 3)
	{
		acsDelay = atol(tokens[3]);
		if(acsDelay < 0) {
			printText("acsDelay must be non-negative, or not specified.");
			return;
		}
		updateCustodianAcsDelay(tokens[1], (unsigned long)(acsDelay));

                isprintf( text, sizeof text, "added acs rule; %s %ld %ld", tokens[1], acsSize, acsDelay );
	}
        else
                isprintf( text, sizeof text, "added acs rule; %s %ld (no delay)", tokens[1], acsSize );

        writeMemo( text );
}

static void executeSetMinimumCustodyId(int tokenCount, char **tokens)
{
	long minimumCustodyId;

	if (acsAttach() < 0) return;
	if (tokenCount < 2)
	{
		printText("Set minimum custody ID to what?");
		return;
	}

	minimumCustodyId = atol(tokens[1]);
	if (minimumCustodyId < 0)
	{
		printText("minimumCustodyId must be positive");
		return;
	}

	updateMinimumCustodyId(minimumCustodyId);
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

static int	acsadmin_processLine(char *line)
{
	int		lineLength;
	int		tokenCount;
	char	        *cursor;
	int		i;
	char		*tokens[9];
	char	buffer[80];

	if (line == NULL) return 0;

	lineLength = strlen(line);
	if (lineLength <= 0) return 0;

	if (line[lineLength - 1] == 0x0a)	/*	LF (newline)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength <= 0) return 0;
	}

	if (line[lineLength - 1] == 0x0d)	/*	CR (DOS text)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength <= 0) return 0;
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

		case 'v':
			isprintf(buffer, sizeof buffer, "%s",
					IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case '1':
			initializeAcs(tokenCount, tokens);
			return 0;

		case 'l':
			executeList(tokenCount, tokens);
			return 0;

		case 'a':
			executeAdd(tokenCount, tokens);
			return 0;

		case 's':
			executeSetMinimumCustodyId(tokenCount, tokens);
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
int	acsadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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

				perror("acsadmin fgets failed");
				break;		/*	Out of loop.	*/
			}

			if (acsadmin_processLine(line))
			{
				break;		/*	Out of loop.	*/
			}
		}
	}
	else if (strcmp(cmdFileName, ".") == 0)	/*	Shutdown.	*/
	{
		
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

					perror("acsadmin fgets failed");
					break;		/*	Loop.	*/
				}

				if (line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (acsadmin_processLine(line))
				{
					break;	/*	Out of loop.	*/
				}
			}

			fclose(cmdFile);
		}
	}

	writeErrmsgMemos();
	acsDetach();
	bp_detach();
	printText("Stopping acsadmin.");
	return 0;
}
