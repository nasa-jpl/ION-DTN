/*

	ionsecadmin.c:	security database adminstration interface.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "ionsec.h"

static char	*_omitted()
{
	return "";
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

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of \
ionsecadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a key <key name> <name of file containing key value>");
	PUTS("\t   a babtxrule <receiver eid expression> { '' | \
<ciphersuite name> <key name> }");
	PUTS("\t\tAn eid expression may be either an EID or a wild card, \
i.e., a partial eid expression ending in '*'.");
	PUTS("\t   a babrxrule <sender eid expression> { '' | \
<ciphersuite name> <key name> }");
	PUTS("\tc\tChange");
	PUTS("\t   c key <key name> <name of file containing key value>");
	PUTS("\t   c babtxrule <receiver eid expression> { '' | \
<ciphersuite name> <key name> }");
	PUTS("\t   c babrxrule <sender eid expression> { '' | \
<ciphersuite name> <key name> }");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} key <key name>");
	PUTS("\t   {d|i} babtxrule <receiver eid expression>");
	PUTS("\t   {d|i} babrxrule <sender eid expression>");
	PUTS("\tl\tList");
	PUTS("\t   l key");
	PUTS("\t   l babtxrule");
	PUTS("\t   l babrxrule");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	initializeIonSecurity(int tokenCount, char **tokens)
{
	if (tokenCount != 1)
	{
		SYNTAX_ERROR;
		return;
	}

	if (secInitialize() < 0)
	{
		printText("Can't initialize the ION security system.");
	}
}

static void	executeAdd(int tokenCount, char **tokens)
{
	char	*keyName;

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "key") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		sec_addKey(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "babtxrule") == 0)
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

		sec_addBabTxRule(tokens[2], tokens[3], keyName);
		return;
	}

	if (strcmp(tokens[1], "babrxrule") == 0)
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

		sec_addBabRxRule(tokens[2], tokens[3], keyName);
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

	if (strcmp(tokens[1], "key") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		sec_updateKey(tokens[2], tokens[3]);
		return;
	}

	if (strcmp(tokens[1], "babtxrule") == 0)
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

		sec_updateBabTxRule(tokens[2], tokens[3], keyName);
		return;
	}

	if (strcmp(tokens[1], "babrxrule") == 0)
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

		sec_updateBabRxRule(tokens[2], tokens[3], keyName);
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

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "key") == 0)
	{
		sec_removeKey(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "babtxrule") == 0)
	{
		sec_removeBabTxRule(tokens[2]);
		return;
	}

	if (strcmp(tokens[1], "babrxrule") == 0)
	{
		sec_removeBabRxRule(tokens[2]);
		return;
	}

	SYNTAX_ERROR;
}

static void	printKey(Object keyAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(SecKey, key);
	char	buf[128];

	GET_OBJ_POINTER(sdr, SecKey, key, keyAddr);
	isprintf(buf, sizeof buf, "key name '%.31s' length %d", key->name,
			key->length);
	printText(buf);
}

static void	printBabTxRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BabTxRule, rule);
	char	eidbuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BabTxRule, rule, ruleAddr);
	sdr_string_read(sdr, eidbuf, rule->recvEid);
	isprintf(buf, sizeof buf, "txrule eid '%.255s' ciphersuite '%.31s' \
			key name '%.31s'", eidbuf, rule->ciphersuiteName,
			rule->keyName);
	printText(buf);
}

static void	printBabRxRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BabRxRule, rule);
	char	eidbuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BabRxRule, rule, ruleAddr);
	sdr_string_read(sdr, eidbuf, rule->xmitEid);
	isprintf(buf, sizeof buf, "rxrule eid '%.255s' ciphersuite '%.31s' \
key name '%.31s'", eidbuf, rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Object	addr;
	Object	elt;

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

	if (strcmp(tokens[1], "key") == 0)
	{
		sec_findKey(tokens[2], &addr, &elt);
		if (elt == 0)
		{
			printText("Key not found.");
			return;
		}

		printKey(addr);
		return;
	}

	if (strcmp(tokens[1], "babtxrule") == 0)
	{
		sec_findBabTxRule(tokens[2], &addr, &elt);
		if (elt == 0)
		{
			printText("Key not found.");
			return;
		}

		printBabTxRule(addr);
		return;
	}

	if (strcmp(tokens[1], "babrxrule") == 0)
	{
		sec_findBabRxRule(tokens[2], &addr, &elt);
		if (elt == 0)
		{
			printText("Key not found.");
			return;
		}

		printBabRxRule(addr);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(SecDB, db);
	Object	elt;
	Object	obj;

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
		for (elt = sdr_list_first(sdr, db->keys); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printKey(obj);
		}

		return;
	}

	if (strcmp(tokens[1], "babtxrule") == 0)
	{
		for (elt = sdr_list_first(sdr, db->babTxRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBabTxRule(obj);
		}

		return;
	}

	if (strcmp(tokens[1], "babrxrule") == 0)
	{
		for (elt = sdr_list_first(sdr, db->babRxRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBabRxRule(obj);
		}

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

static int	processLine(char *line, int lineLength)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];

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

#if defined (VXWORKS) || defined (RTEMS)
int	ionsecadmin(int a1, int a2, int a3, int a4, int a5,
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
	else					/*	Scripted.	*/
	{
		cmdFile = open(cmdFileName, O_RDONLY, 0777);
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
	printText("Stopping ionsecadmin.");
	ionDetach();
	return 0;
}
