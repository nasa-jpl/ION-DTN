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
#ifdef ORIGINAL_BSP
	PUTS("\t   a bspbabrule <sender eid expression> <receiver eid \
expression> { '' |  <ciphersuite name> <key name> }");
	PUTS("\t\tAn eid expression may be either an EID or a wild card, \
i.e., a partial eid expression ending in '*'.");
	PUTS("\t   a bsppibrule <sender eid expression> <receiver eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   a bsppcbrule <sender eid expression> <receiver eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
#else
	PUTS("\t   a bspbibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t\tEvery eid expression must be a node identification \
expression, i.e., a partial eid expression ending in '*' or '~'.");
	PUTS("\t   a bspbcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
#endif
	PUTS("\tc\tChange");
#ifdef ORIGINAL_BSP
	PUTS("\t   c bspbabrule <sender eid expression> <receiver eid \
expression> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c bsppibrule <sender eid expression> <receiver eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c bsppcbrule <sender eid expression> <receiver eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
#else
	PUTS("\t   c bspbibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   c bspbcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
#endif
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
#ifdef ORIGINAL_BSP
	PUTS("\t   {d|i} bspbabrule <sender eid expression> <receiver eid \
expression>");
	PUTS("\t   {d|i} bsppibrule <sender eid expression> <receiver eid \
expression> <block type number>");
	PUTS("\t   {d|i} bsppcbrule <sender eid expression> <receiver eid \
expression> <block type number>");
#else
	PUTS("\t   {d|i} bspbibrule <source eid expression> <destination eid \
expression> <block type number>");
	PUTS("\t   {d|i} bspbcbrule <source eid expression> <destination eid \
expression> <block type number>");
#endif
	PUTS("\tl\tList");
#ifdef ORIGINAL_BSP
	PUTS("\t   l bspbabrule");
	PUTS("\t   l bsppibrule");
	PUTS("\t   l bsppcbrule");
#else
	PUTS("\t   l bspbibrule");
	PUTS("\t   l bspbcbrule");
#endif
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

#ifdef ORIGINAL_BSP
	if (strcmp(tokens[1], "bspbabrule") == 0)
	{
		switch (tokenCount)
		{
		case 6:
			keyName = tokens[5];
			break;

		case 5:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_addBspBabRule(tokens[2], tokens[3], tokens[4], keyName);
		return;
	}

	if (strcmp(tokens[1], "bsppibrule") == 0)
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

		sec_addBspPibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bsppcbrule") == 0)
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

                sec_addBspPcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
                return;
        }
#else
	if (strcmp(tokens[1], "bspbibrule") == 0)
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

		sec_addBspBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bspbcbrule") == 0)
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

                sec_addBspBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
                return;
        }
#endif
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

#ifdef ORIGINAL_BSP
	if (strcmp(tokens[1], "bspbabrule") == 0)
	{
		switch (tokenCount)
		{
		case 6:
			keyName = tokens[5];
			break;

		case 5:
			keyName = _omitted();
			break;

		default:
			SYNTAX_ERROR;
			return;
		}

		sec_updateBspBabRule(tokens[2], tokens[3], tokens[4], keyName);
		return;
	}

	if (strcmp(tokens[1], "bsppibrule") == 0)
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

		sec_updateBspPibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bsppcbrule") == 0)
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

                sec_updateBspPcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
                return;
        }
#else
	if (strcmp(tokens[1], "bspbibrule") == 0)
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

		sec_updateBspBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				tokens[5], keyName);
		return;
	}

        if (strcmp(tokens[1], "bspbcbrule") == 0)
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

                sec_updateBspBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                tokens[5], keyName);
                return;
        }
#endif

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	if (tokenCount < 3)
	{
		printText("Delete what?");
		return;
	}

#ifdef ORIGINAL_BSP
	if (strcmp(tokens[1], "bspbabrule") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		sec_removeBspBabRule(tokens[2], tokens[3]);
		return;
	}

        if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "bsppibrule") == 0)
	{
		sec_removeBspPibRule(tokens[2], tokens[3], atoi(tokens[4]));
		return;
	}

        if (strcmp(tokens[1], "bsppcbrule") == 0)
        {
                sec_removeBspPcbRule(tokens[2], tokens[3], atoi(tokens[4]));
                return;
        }
#else
        if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "bspbibrule") == 0)
	{
		sec_removeBspBibRule(tokens[2], tokens[3], atoi(tokens[4]));
		return;
	}

        if (strcmp(tokens[1], "bspbcbrule") == 0)
        {
                sec_removeBspBcbRule(tokens[2], tokens[3], atoi(tokens[4]));
                return;
        }
#endif
	SYNTAX_ERROR;
}

#ifdef ORIGINAL_BSP
static void	printBspBabRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BspBabRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
	sdr_string_read(sdr, destEidBuf, rule->securityDestEid);
	isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void	printBspPibRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BspPibRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BspPibRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
	sdr_string_read(sdr, destEidBuf, rule->securityDestEid);
	isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockTypeNbr, rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void     printBspPcbRule(Object ruleAddr)
{
        Sdr     sdr = getIonsdr();
                OBJ_POINTER(BspPcbRule, rule);
        char    srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
        char    buf[512];

        GET_OBJ_POINTER(sdr, BspPcbRule, rule, ruleAddr);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->securityDestEid);
        isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockTypeNbr, rule->ciphersuiteName, rule->keyName);
        printText(buf);
}
#else
static void	printBspBibRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BspBibRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
	sdr_string_read(sdr, destEidBuf, rule->destEid);
	isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockTypeNbr, rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void     printBspBcbRule(Object ruleAddr)
{
        Sdr     sdr = getIonsdr();
                OBJ_POINTER(BspBcbRule, rule);
        char    srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
        char    buf[512];

        GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleAddr);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->destEid);
        isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockTypeNbr, rule->ciphersuiteName, rule->keyName);
        printText(buf);
}
#endif

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

#ifdef ORIGINAL_BSP
	if (strcmp(tokens[1], "bspbabrule") == 0)
	{
        	if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		sec_findBspBabRule(tokens[2], tokens[3], &addr, &elt);
		if (elt == 0)
		{
			printText("BAB rule not found.");
		}
		else
		{
			printBspBabRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bsppibrule") == 0)
	{
        	if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
		sec_findBspPibRule(tokens[2], tokens[3], atoi(tokens[4]),
				&addr, &elt);
		if (elt == 0)
		{
			printText("PIB rule not found.");
		}
		else
		{
			printBspPibRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

        if (strcmp(tokens[1], "bsppcbrule") == 0)
        {
        	if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		CHKVOID(sdr_begin_xn(sdr));
                sec_findBspPcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                &addr, &elt);
                if (elt == 0)
                {
                        printText("PCB rule not found.");
                }
		else
		{
                	printBspPcbRule(addr);
		}

		sdr_exit_xn(sdr);
                return;
        }
#else
       	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	if (strcmp(tokens[1], "bspbibrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sec_findBspBibRule(tokens[2], tokens[3], atoi(tokens[4]),
				&addr, &elt);
		if (elt == 0)
		{
			printText("BIB rule not found.");
		}
		else
		{
			printBspBibRule(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

        if (strcmp(tokens[1], "bspbcbrule") == 0)
        {
		CHKVOID(sdr_begin_xn(sdr));
                sec_findBspBcbRule(tokens[2], tokens[3], atoi(tokens[4]),
                                &addr, &elt);
                if (elt == 0)
                {
                        printText("BCB rule not found.");
                }
		else
		{
                	printBspBcbRule(addr);
		}

		sdr_exit_xn(sdr);
                return;
        }
#endif
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
#ifdef ORIGINAL_BSP
	if (strcmp(tokens[1], "bspbabrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bspBabRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBspBabRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bsppibrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bspPibRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBspPibRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bsppcbrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bspPcbRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBspPcbRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
        }
#else
	if (strcmp(tokens[1], "bspbibrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bspBibRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBspBibRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bspbcbrule") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, db->bspBcbRules); elt;
				elt = sdr_list_next(sdr, elt))
		{
			obj = sdr_list_data(sdr, elt);
			printBspBcbRule(obj);
		}

		sdr_exit_xn(sdr);
		return;
        }
#endif
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
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];
	char	buffer[80];

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
					sec_clearBspRules(tokens[1], tokens[2],
							tokens[3]);
				}
				else if (tokenCount == 3)
				{
					sec_clearBspRules(tokens[1], tokens[2],
							"~");
				}
				else if (tokenCount == 2)
				{
					sec_clearBspRules(tokens[1], "~", "~");
				}
				else
				{
					sec_clearBspRules("~", "~", "~");
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
