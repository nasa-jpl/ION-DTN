/*

	ionsecadmin.c:	security database adminstration interface.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "ionsec.h"

static Sdr	sdr = NULL;
static int	echo = 0;
static SecDB	secdb;
static char	*omitted = "";

static void	printText(char *text)
{
	if (echo)
	{
		writeMemo(text);
	}

	puts(text);
}

static void	handleQuit()
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	sprintf(buffer, "Syntax error at line %d of ionsecadmin.c", lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	puts("Valid commands are:");
	puts("\tq\tQuit");
	puts("\th\tHelp");
	puts("\t?\tHelp");
	puts("\t1\tInitialize");
	puts("\t   1");
	puts("\ta\tAdd");
	puts("\t   a key <key name> <name of file containing key value>");
	puts("\t   a babtxrule <receiver eid expression> { '' | \
<ciphersuite name> <key name> }");
	puts("\t\tAn eid expression may be either an EID or a wild card, \
i.e., a partial eid expression ending in '*'.");
	puts("\t   a babrxrule <sender eid expression> { '' | \
<ciphersuite name> <key name> }");
	puts("\tc\tChange");
	puts("\t   c key <key name> <name of file containing key value>");
	puts("\t   c babtxrule <receiver eid expression> { '' | \
<ciphersuite name> <key name> }");
	puts("\t   c babrxrule <sender eid expression> { '' | \
<ciphersuite name> <key name> }");
	puts("\td\tDelete");
	puts("\ti\tInfo");
	puts("\t   {d|i} key <key name>");
	puts("\t   {d|i} babtxrule <receiver eid expression>");
	puts("\t   {d|i} babrxrule <sender eid expression>");
	puts("\tl\tList");
	puts("\t   l key");
	puts("\t   l babtxrule");
	puts("\t   l babrxrule");
	puts("\te\tEnable or disable echo of printed output to log file");
	puts("\t   e { 0 | 1 }");
	puts("\t#\tComment");
	puts("\t   # <comment text>");
}

static int	initializeIonSecurity()
{
	if (secInitialize() < 0)
	{
		printText("Can't initialize the ION security system.");
		return -1;
	}

	sdr = getIonsdr();
	sdr_read(sdr, (char *) &secdb, getSecDbObject(), sizeof(SecDB));
	return 0;
}

static int	attachToIonSecurity()
{
	if (secAttach() < 0)
	{
		printText("Can't attach to the ION security system.");
		return -1;
	}

	sdr = getIonsdr();
	sdr_read(sdr, (char *) &secdb, getSecDbObject(), sizeof(SecDB));
	return 0;
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
			keyName = omitted;
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
			keyName = omitted;
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
			keyName = omitted;
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
			keyName = omitted;
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
		OBJ_POINTER(SecKey, key);
	char	buf[128];

	GET_OBJ_POINTER(sdr, SecKey, key, keyAddr);
	sprintf(buf, "key name '%.31s' length %d", key->name, key->length);
	printText(buf);
}

static void	printBabTxRule(Object ruleAddr)
{
		OBJ_POINTER(BabTxRule, rule);
	char	eidbuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BabTxRule, rule, ruleAddr);
	sdr_string_read(sdr, eidbuf, rule->recvEid);
	sprintf(buf, "txrule eid '%.255s' ciphersuite '%.31s' key name '%.31s'",
			eidbuf, rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void	printBabRxRule(Object ruleAddr)
{
		OBJ_POINTER(BabRxRule, rule);
	char	eidbuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BabRxRule, rule, ruleAddr);
	sdr_string_read(sdr, eidbuf, rule->xmitEid);
	sprintf(buf, "rxrule eid '%.255s' ciphersuite '%.31s' key name '%.31s'",
			eidbuf, rule->ciphersuiteName, rule->keyName);
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

	if (strcmp(tokens[1], "key") == 0)
	{
		for (elt = sdr_list_first(sdr, secdb.keys); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printKey(obj);
		}

		return;
	}

	if (strcmp(tokens[1], "babtxrule") == 0)
	{
		for (elt = sdr_list_first(sdr, secdb.babTxRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBabTxRule(obj);
		}

		return;
	}

	if (strcmp(tokens[1], "babrxrule") == 0)
	{
		for (elt = sdr_list_first(sdr, secdb.babRxRules); elt;
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

static int	processLine(char *line)
{
	int	lineLength;
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];

	if (line == NULL) return 0;

	lineLength = strlen(line);
	if (lineLength == 0) return 0;

	if (line[lineLength - 1] == 0x0a)	/*	LF (newline)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength == 0) return 0;
	}

	if (line[lineLength - 1] == 0x0d)	/*	CR (DOS text)	*/
	{
		line[lineLength - 1] = '\0';	/*	lose it		*/
		lineLength--;
		if (lineLength == 0) return 0;
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
			if (initializeIonSecurity() < 0)
			{
				putErrmsg("Can't initialize security database.",
						NULL);
			}

			return 0;

		case 'a':
			if (attachToIonSecurity() < 0)
			{
				putErrmsg("Can't update security database.",
						NULL);
			}
			else
			{
				executeAdd(tokenCount, tokens);
			}

			return 0;

		case 'c':
			if (attachToIonSecurity() < 0)
			{
				putErrmsg("Can't update security database.",
						NULL);
			}
			else
			{
				executeChange(tokenCount, tokens);
			}

			return 0;

		case 'd':
			if (attachToIonSecurity() < 0)
			{
				putErrmsg("Can't update security database.",
						NULL);
			}
			else
			{
				executeDelete(tokenCount, tokens);
			}

			return 0;

		case 'i':
			if (attachToIonSecurity() < 0)
			{
				putErrmsg("Can't query security database.",
						NULL);
			}
			else
			{
				executeInfo(tokenCount, tokens);
			}

			return 0;

		case 'l':
			if (attachToIonSecurity() < 0)
			{
				putErrmsg("Can't list security database.",
						NULL);
			}
			else
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
	FILE	*cmdFile;
	char	line[256];

	if (cmdFileName == NULL)		/*	Interactive.	*/
	{
		isignal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			if (fgets(line, sizeof line, stdin) == NULL)
			{
				if (feof(stdin))
				{
					break;
				}

				perror("ionsecadmin fgets failed");
				break;		/*	Out of loop.	*/
			}

			if (processLine(line))
			{
				break;		/*	Out of loop.	*/
			}
		}
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

					perror("ionsecadmin fgets failed");
					break;		/*	Loop.	*/
				}

				if (line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line))
				{
					break;	/*	Out of loop.	*/
				}
			}

			fclose(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping ionsecadmin.");
	ionDetach();
	return 0;
}
