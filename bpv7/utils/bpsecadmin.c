/*
	bpsecadmin.c:	security database adminstration interface.


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
	6-27-19	    SB	   Extracted from ionsecadmin.
									*/
#include "bpsec.h"
#include "bpP.h"

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

static void	handleQuit(int signum)
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of \
bpsecadmin.c", lineNbr);
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
	PUTS("\ta\tAdd");
	PUTS("\t   a bibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t\tEvery eid expression must be a node identification \
expression, i.e., a partial eid expression ending in '*' or '~'.");
	PUTS("\t   a bcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\tc\tChange");
	PUTS("\t   c bibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c bcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} bibrule <source eid expression> <destination eid \
expression> <block type number>");
	PUTS("\t   {d|i} bcbrule <source eid expression> <destination eid \
expression> <block type number>");
	PUTS("\tl\tList");
	PUTS("\t   l bibrule");
	PUTS("\t   l bcbrule");
	PUTS("\tx\tClear BSP security rules.");
	PUTS("\t   x <security source eid> <security destination eid> \
{ 2 | 3 | 4 | ~ }");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	executeAdd(int tokenCount, char **tokens)
{
	char	*keyName = "";

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		switch (tokenCount)
		{
		case 7:
			keyName = tokens[6];
			break;

		case 6:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_addBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bcbrule") == 0)
        {
                switch (tokenCount)
                {
                case 7:
                        keyName = tokens[6];
			break;

                case 6:
			keyName = _omitted();
			break;

                default:
                        SYNTAX_ERROR;
                        return;
		}

                sec_addBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
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

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		switch (tokenCount)
		{
		case 7:
			keyName = tokens[6];
			break;

		case 6:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_updateBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bcbrule") == 0)
        {
                switch (tokenCount)
                {
                case 7:
                        keyName = tokens[6];
                        break;

                case 6:
                        keyName = _omitted();
                        break;

                default:
                        SYNTAX_ERROR;
                        return;
		}

                sec_updateBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
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

        if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		sec_removeBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]));
		return;
	}

        if (strcmp(tokens[1], "bcbrule") == 0)
        {
                sec_removeBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]));
                return;
        }

	SYNTAX_ERROR;
}

static void	printBPsecBibRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BPsecBibRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BPsecBibRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
	sdr_string_read(sdr, destEidBuf, rule->destEid);
	isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockType, rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void     printBPsecBcbRule(Object ruleAddr)
{
        Sdr     sdr = getIonsdr();
                OBJ_POINTER(BPsecBcbRule, rule);
        char    srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
        char    buf[512];

        GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, ruleAddr);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->destEid);
        isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockType, rule->ciphersuiteName, rule->keyName);
        printText(buf);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	Sdr		sdr = getIonsdr();
	Object		addr;
	Object		elt;

	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (tokenCount > 5)
	{
		SYNTAX_ERROR;
		return;
	}

       	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "bibrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sec_findBPsecBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				&addr, &elt);
		if (elt == 0)
		{
			printText("BIB rule not found.");
		}
		else
		{
			printBPsecBibRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

        if (strcmp(tokens[1], "bcbrule") == 0)
        {
		CHKVOID(sdr_begin_xn(sdr));
                sec_findBPsecBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                &addr, &elt);
                if (elt == 0)
                {
                        printText("BCB rule not found.");
                }
		else
		{
                	printBPsecBcbRule(addr);
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
	if (strcmp(tokens[1], "bibrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bpsecBibRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBPsecBibRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bcbrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bpsecBcbRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBPsecBcbRule(obj);
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

int	bpsecadmin_processLine(char *line, int lineLength)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	char		buffer[80];
	BpBlockType	blockType;

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

		case 'x':
			if (secAttach() == 0)
			{
			   	if (tokenCount > 4)
				{
					SYNTAX_ERROR;
				}
				else if (tokenCount == 4)
				{
					blockType = (BpBlockType)
							atoi(tokens[3]);
					sec_clearBPsecRules(tokens[1],
							tokens[2], &blockType);
				}
				else if (tokenCount == 3)
				{
					sec_clearBPsecRules(tokens[1],
							tokens[2], NULL);
				}
				else if (tokenCount == 2)
				{
					sec_clearBPsecRules(tokens[1], "~",
							NULL);
				}
				else
				{
					sec_clearBPsecRules("~", "~", NULL);
				}
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
int	bpsecadmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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

			if (bpsecadmin_processLine(line, len))
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

				if (bpsecadmin_processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping bpsecadmin.");
	ionDetach();
	return 0;
}
