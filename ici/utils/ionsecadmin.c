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
			   authentication rules
									*/
#include "ionsec.h"

static char	*_omitted()
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
	PUTS("\tv\tPrint version of ION.");
	PUTS("\t1\tInitialize");
	PUTS("\t   1");
	PUTS("\ta\tAdd");
	PUTS("\t   a key <key name> <name of file containing key value>");
	PUTS("\t   a pubkey <node nbr> <eff. time sec> <key len> <key>");
	PUTS("\t   a bspbabrule <sender eid expression> <receiver eid \
expression> { '' |  <ciphersuite name> <key name> }");
#ifdef ORIGINAL_BSP
	PUTS("\t\tAn eid expression may be either an EID or a wild card, \
i.e., a partial eid expression ending in '*'.");
	PUTS("\t   a bsppibrule <sender eid expression> <receiver eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   a bsppcbrule <sender eid expression> <receiver eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
#else
	PUTS("\t\tEvery eid expression must be a node identification \
expression, i.e., a partial eid expression ending in '*' or '~'.");
	PUTS("\t   a bspbibrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
	PUTS("\t   a bspbcbrule <source eid expression> <destination eid \
expression> <block type number> { '' | <ciphersuite name> <key name> }");
#endif
	PUTS("\t   a ltprecvauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\t\tValid ciphersuite numbers:");
	PUTS("\t\t\t  0: HMAC-SHA1-80");
	PUTS("\t\t\t  1: RSA-SHA256");
	PUTS("\t\t\t255: NULL");
	PUTS("\t   a ltpxmitauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\tc\tChange");
	PUTS("\t   c key <key name> <name of file containing key value>");
	PUTS("\t   c bspbabrule <sender eid expression> <receiver eid \
expression> { '' | <ciphersuite name> <key name> }");
#ifdef ORIGINAL_BSP
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
	PUTS("\t   c ltprecvauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\t   c ltpxmitauthrule <ltp engine id> <ciphersuite_nbr> \
[<key name>]");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} key <key name>");
	PUTS("\t   {d|i} pubkey <node nbr> <eff. time sec>");
	PUTS("\t   {d|i} bspbabrule <sender eid expression> <receiver eid \
expression>");
#ifdef ORIGINAL_BSP
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
	PUTS("\t   {d|i} ltprecvauthrule <ltp engine id> ");
	PUTS("\t   {d|i} ltpxmitauthrule <ltp engine id> ");
	PUTS("\tl\tList");
	PUTS("\t   l key");
	PUTS("\t   l pubkey");
	PUTS("\t   l bspbabrule");
	PUTS("\t   l bsppibrule");
	PUTS("\t   l bsppcbrule");
	PUTS("\t   l ltprecvauthrule");
	PUTS("\t   l ltpxmitauthrule");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\tx\tClear BSP security rules.");
	PUTS("\t   x <security source eid> <security destination eid> \
{ 2 | 3 | 4 | ~ }");
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
	uvast		nodeNbr;
	BpTimestamp	effectiveTime;
	time_t		assertionTime;
	unsigned short	datLen;
	unsigned char	datValue[1024];
	char		*cursor;
	int		i;
	char		buf[3];
	int		val;
	char		*keyName = "";

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

	if (strcmp(tokens[1], "pubkey") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		nodeNbr = strtouvast(tokens[2]);
		effectiveTime.seconds = strtoul(tokens[3], NULL, 0);
		effectiveTime.count = 0;
		assertionTime = strtoul(tokens[4], NULL, 0);
		datLen = atoi(tokens[5]);
		cursor = tokens[6];
		if (strlen(cursor) != (datLen * 2))
		{
			SYNTAX_ERROR;
			return;
		}

		for (i = 0; i < datLen; i++)
		{
			memcpy(buf, cursor, 2);
			buf[2] = '\0';
			sscanf(buf, "%x", &val);
			datValue[i] = val;
			cursor += 2;
		}

		sec_addPublicKey(nodeNbr, &effectiveTime, assertionTime,
				datLen, datValue);
		return;
	}

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

#ifdef ORIGINAL_BSP
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
		}

		sec_addLtpRecvAuthRule(atoi(tokens[2]), atouc(tokens[3]),
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

		sec_addLtpXmitAuthRule(atoi(tokens[2]), atouc(tokens[3]),
				keyName);
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

#ifdef ORIGINAL_BSP
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
	uvast		nodeNbr;
	BpTimestamp	effectiveTime;

	if (tokenCount < 3)
	{
		printText("Delete what?");
		return;
	}

	if (tokenCount > 5)
	{
		SYNTAX_ERROR;
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

	if (strcmp(tokens[1], "pubkey") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		nodeNbr = strtouvast(tokens[2]);
		effectiveTime.seconds = strtoul(tokens[3], NULL, 0);
		effectiveTime.count = 0;
		sec_removePublicKey(nodeNbr, &effectiveTime);
		return;
	}

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

#ifdef ORIGINAL_BSP
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

static void	printPubKey(Object keyAddr)
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
	char		buf[(sizeof datValueDisplay) * 2];

	GET_OBJ_POINTER(sdr, PublicKey, key, keyAddr);
	writeTimestampUTC(key->effectiveTime.seconds, effectiveTime);
	writeTimestampUTC(key->assertionTime, assertionTime);
	len = key->length;
	if (len < 0)
	{
		len = 0;
	}
	else
	{
		if (len > sizeof datValue)
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

	isprintf(buf, sizeof buf, "node " UVAST_FIELDSPEC " effective %s \
asserted %s data length %d data %s", key->nodeNbr, effectiveTime, assertionTime,
			key->length, datValueDisplay);
	printText(buf);
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
static void	printBspBabRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BspBabRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->senderEid);
	sdr_string_read(sdr, destEidBuf, rule->receiverEid);
	isprintf(buf, sizeof buf, "rule sender eid '%.255s' receiver eid \
'%.255s' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

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

static void	printLtpRecvAuthRule(Object ruleAddr)
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

static void	printLtpXmitAuthRule(Object ruleAddr)
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
	Object		addr;
	Object		elt;
	uvast		nodeNbr;
	BpTimestamp	effectiveTime;

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

	if (strcmp(tokens[1], "key") == 0)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sec_findKey(tokens[2], &addr, &elt);
		if (elt == 0)
		{
			printText("Key not found.");
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
		nodeNbr = strtouvast(tokens[2]);
		effectiveTime.seconds = strtoul(tokens[3], NULL, 0);
		effectiveTime.count = 0;
		sec_findPublicKey(nodeNbr, &effectiveTime, &addr, &elt);
		if (elt == 0)
		{
			printText("Public key not found.");
		}
		else
		{
			printPubKey(addr);
		}

		sdr_exit_xn(sdr);
		return;
	}

	if (strcmp(tokens[1], "bspbabrule") == 0)
	{
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
#ifdef ORIGINAL_BSP
	if (strcmp(tokens[1], "bsppibrule") == 0)
	{
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
#ifdef ORIGINAL_BSP
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

int	ionsecadmin_processLine(char *line, int lineLength)
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
	
		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

#if defined (ION_LWT)
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

			if (ionsecadmin_processLine(line, len))
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
