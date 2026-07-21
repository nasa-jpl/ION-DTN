/*
	ltpsecadmin.c:	security database adminstration interface.


	Copyright (c) 2019, California Institute of Technology.
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
			   authentication rules
	6-27-19    SB	   Extracted from ionsecadmin.
									*/
#include "ltpsec.h"
#include "ion.h"
#include "ionsec.h"
#include "platform.h"
#include "sdrlist.h"
#include "sdrxn.h"
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char	*_omitted(void)
{
	return "";
}

static unsigned char	atouc(char *input)
{
	unsigned char	result;

	result = strtol(input, NULL, 0);
	return result;
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
ltpsecadmin.c", lineNbr);
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
	PUTS("\ta\tAdd");
	PUTS("\t   a ltprecvauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\t\tValid ciphersuite numbers:");
	PUTS("\t\t\t  0: HMAC-SHA1-80");
	PUTS("\t\t\t  1: RSA-SHA256");
	PUTS("\t\t\t255: NULL");
	PUTS("\t   a ltpxmitauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\tc\tChange");
	PUTS("\t   c ltprecvauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\t   c ltpxmitauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} ltprecvauthrule <ltp engine id> ");
	PUTS("\t   {d|i} ltpxmitauthrule <ltp engine id> ");
	PUTS("\tl\tList");
	PUTS("\t   l ltprecvauthrule");
	PUTS("\t   l ltpxmitauthrule");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void     executeAdd(int tokenCount, char **tokens)
{
	char    *keyName = "";

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "ltprecvauthrule") == 0)
	{
		int     engineId;
		uvast   cipherNbr;
		char    errMsg[256];

		switch (tokenCount)
		{
		case 5:
			keyName = tokens[4];
			break;

		case 4:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		if (platform_parse_int(tokens[2], &engineId) < 0)
		{
			isprintf(errMsg, sizeof(errMsg),
					"[?] Invalid engine ID: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[3], &cipherNbr) < 0
			|| cipherNbr > UCHAR_MAX)
		{
			isprintf(errMsg, sizeof(errMsg),
					"[?] Invalid ciphersuite number: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		sec_addLtpRecvAuthRule(engineId, (unsigned char) cipherNbr, keyName);
		return;
	}

	if (strcmp(tokens[1], "ltpxmitauthrule") == 0)
	{
		int     engineId;
		uvast   cipherNbr;
		char    errMsg[256];

		switch (tokenCount)
		{
		case 5:
			keyName = tokens[4];
			break;

		case 4:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		if (platform_parse_int(tokens[2], &engineId) < 0)
		{
			isprintf(errMsg, sizeof(errMsg),
					"[?] Invalid engine ID: %s", tokens[2]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		if (platform_parse_uvast(tokens[3], &cipherNbr) < 0
			|| cipherNbr > UCHAR_MAX)
		{
			isprintf(errMsg, sizeof(errMsg),
					"[?] Invalid ciphersuite number: %s", tokens[3]);
			PUTS(errMsg); writeMemo(errMsg);
			return;
		}

		sec_addLtpXmitAuthRule(engineId, (unsigned char) cipherNbr, keyName);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	char	*keyName;

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "ltprecvauthrule") == 0)
	{
		switch (tokenCount)
		{
		case 5:
			keyName = tokens[4];
			break;

		case 4:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_updateLtpRecvAuthRule(atoi(tokens[2]), atouc(tokens[3]),
				keyName);
		return;
	}

	if (strcmp(tokens[1], "ltpxmitauthrule") == 0)
	{
		switch (tokenCount)
		{
		case 5:
			keyName = tokens[4];
			break;

		case 4:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_updateLtpXmitAuthRule(atoi(tokens[2]), atouc(tokens[3]),
				keyName);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	if (tokenCount < 3)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "ltprecvauthrule") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		sec_removeLtpRecvAuthRule(atoi(tokens[2]));
		return;
	}

	if (strcmp(tokens[1], "ltpxmitauthrule") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		sec_removeLtpXmitAuthRule(atoi(tokens[2]));
		return;
	}

	SYNTAX_ERROR;
}

static void printLtpRecvAuthRule(SdrObject ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpRecvAuthRule, rule);
	char	buf[512];
	int	temp;

	GET_OBJ_POINTER(sdr, LtpRecvAuthRule, rule, ruleAddr);
	temp = rule->ciphersuiteNbr;
	isprintf(buf, sizeof buf, "LTPrecv rule: engine id " UVAST_FIELDSPEC,
			rule->ltpEngineId);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" ciphersuite_nbr %d", temp);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" key name '%.31s'", rule->keyName);
	printText(buf);
}

static void printLtpXmitAuthRule(SdrObject ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpXmitAuthRule, rule);
	char	buf[512];
	int	temp;

	GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, ruleAddr);
	temp = rule->ciphersuiteNbr;
	isprintf(buf, sizeof buf, "LTPxmit rule: engine id " UVAST_FIELDSPEC,
			rule->ltpEngineId);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" ciphersuite_nbr %d", temp);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" key name '%.31s'", rule->keyName);
	printText(buf);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	SdrObject	addr;
	SdrObject	elt;

	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "ltprecvauthrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sec_findLtpRecvAuthRule(atoi(tokens[2]), &addr, &elt);
		if (elt == 0)
		{
			printText("LTP segment authentication rule not found.");
		}
		else
		{
			printLtpRecvAuthRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "ltpxmitauthrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sec_findLtpXmitAuthRule(atoi(tokens[2]), &addr, &elt);
		if (elt == 0)
		{
			printText("LTP segment signing rule not found.");
		}
		else
		{
			printLtpXmitAuthRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(SecDB, db);
	SdrObject	elt;
	SdrObject	obj;

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
	if (strcmp(tokens[1], "ltprecvauthrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->ltpRecvAuthRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printLtpRecvAuthRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "ltpxmitauthrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->ltpXmitAuthRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printLtpXmitAuthRule(obj);
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

int	ltpsecadmin_processLine(char *line, int lineLength)
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
int	ltpsecadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
#ifdef NON_INTERACTIVE
		return 0;			/*	No stdin.	*/
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

			if (ltpsecadmin_processLine(line, len))
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

				if (ltpsecadmin_processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping ltpsecadmin.");
	ionDetach();
	return 0;
}
