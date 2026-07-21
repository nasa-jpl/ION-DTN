/*
	ionsecadmin.c:	security database adminstration interface.


	Copyright (c) 2009, California Institute of Technology.
	All rights reserved.
	Author: Scott Burleigh, Jet Propulsion Laboratory
	Modifications: TCSASSEMBLER, TopCoder

	Modification History:
	Date       Who     What
	9-24-13    TC      Added atouc helper function to convert char* to
			   unsigned char
			   Updated printUsage function to print usage for
			   newly added ltp authentication rules
			   Updated executeAdd, executeChange, executeDelete,
			   executeInfo, and executeList functions to process
			   newly added ltp authentication rules
			   Added printLtpRecvAuthRule and
			   printXmitRecvAuthRule functions to print ltp
			   authentication rule
	6-27-19    SB	   ltpsecadmin and bpsecadmin extracted.
									*/
#include "ionsec.h"
#include "ion.h"
#include "platform.h"
#include "sdrlist.h"
#include "sdrxn.h"
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef INPUT_HISTORY
#include "linenoise.h"
#include <errno.h>
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

#ifndef NON_INTERACTIVE
static void	handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	printText("Please enter command 'q' to stop the program.");
}
#endif

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of \
ionsecadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage(void)
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a key <key name> <name of file containing key value>");
	PUTS("\t   a pubkey <node> <effective time sec> <assertion time \
sec> <key len> <key>");
	PUTS("\tc\tChange");
	PUTS("\t   c key <key name> <name of file containing key value>");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} key <key name>");
	PUTS("\t   {d|i} pubkey <node> <effective time sec>");
	PUTS("\tl\tList");
	PUTS("\t   l key");
	PUTS("\t   l pubkey");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeIonSecurity(int tokenCount, char **tokens)
{
	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (secInitialize() < 0)
	{
		printText("[?] Can't initialize the ION security system.");
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	uvast		fqnn;
	time_t		effectiveTime;
	time_t		assertionTime;
	unsigned short	datLen;
	unsigned char	datValue[1024];
	char		*cursor;
	int		i;
	char		buf[3];
	unsigned int	val;
	uvast		parsed_uvast;
	int		parsed_int;
	char		errMsg[256];

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "key") == 0)
	{
		/* Support implicit key length syntax: a key <name> <file> */
		if (tokenCount == 4)
		{
			/* Pass 0 length to auto-detect */
			sec_addKey(tokens[2], tokens[3], 0);
			return;
		}
		/* Support FIFO syntax: a key <name> <file> <length> */
		else if (tokenCount == 5)
		{
			if (platform_parse_int(tokens[4], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid key length: %s", tokens[4]);
				PUTS(errMsg); writeMemo(errMsg);
				return;
			}
			sec_addKey(tokens[2], tokens[3], parsed_int);
			return;
		}
		else
		{
			SYNTAX_ERROR;
			return;
		}
	}

	if (strcmp(tokens[1], "pubkey") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		fqnn = getFqn(tokens[2]);

		if (platform_parse_uvast(tokens[3], &parsed_uvast) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid effective time: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		effectiveTime = (time_t) parsed_uvast;

		if (platform_parse_uvast(tokens[4], &parsed_uvast) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid assertion time: %s", tokens[4]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		assertionTime = (time_t) parsed_uvast;

		if (platform_parse_int(tokens[5], &parsed_int) < 0 || parsed_int < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid key length: %s", tokens[5]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		datLen = (unsigned short) parsed_int;

		if (datLen > sizeof(datValue))
		{
			printText("datLen out of range.");
			return;
		}

		cursor = tokens[6];
		if (strlen(cursor) != (size_t)(datLen * 2))
		{
			SYNTAX_ERROR;
			return;
		}

		for (i = 0; i < datLen; i++)
		{
			memcpy(buf, cursor, 2);
			buf[2] = '\0';
			if (sscanf(buf, "%x", &val) < 1)
			{
				printText("Invalid hex digit in pubkey data.");
				return;
			}

			datValue[i] = val;
			cursor += 2;
		}

		sec_addPublicKey(fqnn, effectiveTime, assertionTime,
				datLen, datValue);
		return;
	}

	SYNTAX_ERROR;
}

static void executeChange(int tokenCount, char **tokens)
{
	int	parsed_int;
	char	errMsg[256];

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "key") == 0)
	{
		/* Support implicit key length syntax: c key <name> <file> */
		if (tokenCount == 4)
		{
			/* Pass 0 for auto-detect */
			sec_updateKey(tokens[2], tokens[3], 0);
			return;
		}
		/* Support FIFO syntax: c key <name> <file> <length> */
		else if (tokenCount == 5)
		{
			if (platform_parse_int(tokens[4], &parsed_int) < 0)
			{
				isprintf(errMsg, sizeof(errMsg), "[?] Invalid key length: %s", tokens[4]);
				PUTS(errMsg); writeMemo(errMsg);
				return;
			}
			sec_updateKey(tokens[2], tokens[3], parsed_int);
			return;
		}
		else
		{
			SYNTAX_ERROR;
			return;
		}
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	uvast	fqnn;
	time_t	effectiveTime;
	uvast	parsed_uvast;
	char	errMsg[256];

	if (tokenCount < 3)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "key") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		sec_removeKey(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "pubkey") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		fqnn = getFqn(tokens[2]);

		if (platform_parse_uvast(tokens[3], &parsed_uvast) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid effective time: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}
		effectiveTime = (time_t) parsed_uvast;

		sec_removePublicKey(fqnn, effectiveTime);
		return;
	}

	SYNTAX_ERROR;
}

static void printKey(SdrObject keyAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(SecKey, key);
	char	buf[128];

	GET_OBJ_POINTER(sdr, SecKey, key, keyAddr);
	isprintf(buf, sizeof buf, "key name '%.31s' length %d", key->name,
			key->length);
	printText(buf);
}

static void printPubKey(SdrObject keyAddr)
{
	Sdr		sdr = getIonsdr();
			OBJ_POINTER(PublicKey, key);
	char		effectiveTime[TIMESTAMPBUFSZ];
	char		assertionTime[TIMESTAMPBUFSZ];
	int		len;
	unsigned char	datValue[1024];
	char		datValueDisplay[(sizeof datValue * 2)];
	char		*cursor = datValueDisplay;
	int		bytesRemaining = sizeof datValueDisplay;
	int		i;
	char		fqnBuf[FQN_MAX_LENGTH];
	char		buf[(sizeof datValueDisplay) * 2];

	GET_OBJ_POINTER(sdr, PublicKey, key, keyAddr);
	writeTimestampUTC(key->effectiveTime, effectiveTime);
	writeTimestampUTC(key->assertionTime, assertionTime);
	len = key->length;
	if (len < 0)
	{
		len = 0;
	}
	else
	{
		if ((size_t)len > sizeof datValue)
		{
			len = sizeof datValue;
		}
	}

	sdr_read(sdr, (char *) datValue, key->value, len);
	for (i = 0; i < len; i++)
	{
		isprintf(cursor, bytesRemaining, "%02x", datValue[i]);
		cursor += 2;
		bytesRemaining -= 2;
	}

	putFqn(fqnBuf, key->fqnn);
	isprintf(buf, sizeof buf, "node %s effective %s asserted %s data \
length %d data %s", fqnBuf, effectiveTime, assertionTime, key->length,
			datValueDisplay);
	printText(buf);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	addr;
	SdrObject	elt;
	uvast		fqnn;
	time_t		effectiveTime;
	uvast		parsed_uvast;
	char		errMsg[256];

	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (strcmp(tokens[1], "key") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		sec_findKey(tokens[2], &addr, &elt);
		if (elt == 0)
		{
			printText("[?] Key not found.");
		}
		else
		{
			printKey(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "pubkey") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		fqnn = getFqn(tokens[2]);

		if (platform_parse_uvast(tokens[3], &parsed_uvast) < 0)
		{
			isprintf(errMsg, sizeof(errMsg), "[?] Invalid effective time: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			sdr_exit_xn(sdr); /* Safely close transaction before returning */
			return;
		}
		effectiveTime = (time_t) parsed_uvast;

		sec_findPublicKey(fqnn, effectiveTime, &addr, &elt);
		if (elt == 0)
		{
			printText("[?] Public key not found.");
		}
		else
		{
			printPubKey(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	  sdr = getIonsdr();
	SdrObject elt;
	SdrObject obj;
	OBJ_POINTER(SecDB, db);

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());
	if (strcmp(tokens[1], "key") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->keys); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printKey(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "pubkey") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->publicKeys); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printPubKey(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	SYNTAX_ERROR;
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

int	ionsecadmin_processLine(char *line, int lineLength)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];
	char	buffer[80];

	/* Parameter intentionally unused. */
	(void)lineLength;

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

	while (isspace((unsigned char) *cursor))
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
			initializeIonSecurity(tokenCount, tokens);
			return 0;

		case 'a':
			if (secAttach() == 0)
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (secAttach() == 0)
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (secAttach() == 0)
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (secAttach() == 0)
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (secAttach() == 0)
			{
				executeList(tokenCount, tokens);
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
int	ionsecadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = (argc > 1 ? argv[1] : NULL);
#endif
	int	cmdFile;
	char	line[1024];
	int	len;

#ifdef INPUT_HISTORY
	char *input;
#endif

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef NON_INTERACTIVE
		return 0;			/*	No stdin.	*/
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
#ifdef INPUT_HISTORY
			/* add input history */
			if ((input = linenoise(": ")) != NULL)
			{
				len = strlen(input);

				if (len == 0)
				{
					linenoiseFree(input);
					continue;
				}

				/* received input */
				if (len > 0)
				{
					linenoiseHistoryAdd(input);
				}

				if ((size_t) len > sizeof(line) - 1)
				{
					printf("\nInput is too long. Ignored.\n");
					fflush(stdout);
					linenoiseFree(input);
					continue;
				}
			}
			else if (errno == EAGAIN)
			{
				/* Ctrl+C pressed */
				printText("Please enter command 'q' to stop \
the program.");
				continue;
			}
			else
			{
				/* input error detected */
				printf("\nInput error detected. Exiting.\n");
				fflush(stdout);
				break;
			}

			/* copy the input to line for processing
			 * input sized already checked */

			strcpy(line, input);

			if (ionsecadmin_processLine(line, len))
			{
				linenoiseFree(input);
				break;		/*	Out of loop.	*/
			}
			linenoiseFree(input);
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

			if (ionsecadmin_processLine(line, len))
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
			int	echoState = 1;

			oK(_echo(&echoState));
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

				if (ionsecadmin_processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping ionsecadmin.");
	ionDetach();
	return 0;
}
