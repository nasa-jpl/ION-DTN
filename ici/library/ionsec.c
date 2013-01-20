/*

	ionsec.c:	API for managing ION's security database.

	Author:		Scott Burleigh, JPL

	Copyright (c) 2009, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "ionsec.h"


static int	eidsMatch(char *firstEid, int firstEidLen, char *secondEid,
			int secondEidLen);

static char	*_secDbName()
{
	return "secdb";
}

static Object	_secdbObject(Object *newDbObj)
{
	static Object	obj = 0;
	
	if (newDbObj)
	{
		obj = *newDbObj;
	}
	
	return obj;
}

static SecDB	*_secConstants()
{
	static SecDB	buf;
	static SecDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;

	if (db == NULL)
	{
		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _secdbObject(0);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(SecDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(SecDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

int	secInitialize()
{
	Sdr	ionsdr;
	Object	secdbObject;
	SecDB	secdbBuf;

	if (ionAttach() < 0)
	{
		putErrmsg("Can't attach to ION.", NULL);
		return -1;
	}

	ionsdr = getIonsdr();
	CHKERR(sdr_begin_xn(ionsdr));
	secdbObject = sdr_find(ionsdr, _secDbName(), NULL);
	switch (secdbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(ionsdr);
		putErrmsg("Can't seek security database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found must create new DB.	*/
		memset((char *) &secdbBuf, 0, sizeof(SecDB));
		secdbBuf.keys = sdr_list_create(ionsdr);
		secdbBuf.bspBabRules = sdr_list_create(ionsdr);
		secdbBuf.bspPibRules = sdr_list_create(ionsdr);
		secdbBuf.bspPcbRules = sdr_list_create(ionsdr);
		secdbBuf.bspEsbRules = sdr_list_create(ionsdr);
		secdbObject = sdr_malloc(ionsdr, sizeof(SecDB));
		if (secdbObject == 0)
		{
			sdr_cancel_xn(ionsdr);
			putErrmsg("No space for database.", NULL);
			return -1;
		}

		sdr_write(ionsdr, secdbObject, (char *) &secdbBuf,
				sizeof(SecDB));
		sdr_catlg(ionsdr, _secDbName(), 0, secdbObject);
		if (sdr_end_xn(ionsdr))
		{
			putErrmsg("Can't create security database.", NULL);
			return -1;
		}

		break;

	default:
		sdr_exit_xn(ionsdr);
	}

	oK(_secdbObject(&secdbObject));
	oK(_secConstants());
	return 0;
}

int	secAttach()
{
	Sdr	ionsdr;
	Object	secdbObject;

	if (ionAttach() < 0)
	{
		putErrmsg("Bundle security can't attach to ION.", NULL);
		return -1;
	}

	ionsdr = getIonsdr();
	secdbObject = _secdbObject(NULL);
	if (secdbObject == 0)
	{
		CHKERR(sdr_begin_xn(ionsdr));
		secdbObject = sdr_find(ionsdr, _secDbName(), NULL);
		sdr_exit_xn(ionsdr);
		if (secdbObject == 0)
		{
			writeMemo("[?] Can't find ION security database.");
			return -1;
		}

		oK(_secdbObject(&secdbObject));
	}

	oK(_secConstants());
	return 0;

}

Object	getSecDbObject()
{
	return _secdbObject(NULL);
}

int	bspTypeToString(int bspType, char *s, int buflen)
{
	CHKERR(s);
	switch (bspType)
	{
	case BSP_BAB_TYPE:
		istrcat(s, "BAB", buflen);
		break;

	case BSP_PIB_TYPE:
		istrcat(s, "PIB", buflen);
		break;

	case BSP_PCB_TYPE:
		istrcat(s, "PCB", buflen);
		break;

	case BSP_ESB_TYPE:
		istrcat(s, "ESB", buflen);
		break;

	default:
		istrcat(s, " ", buflen);
		return -1;
	}

	return 0;
}

int bspTypeToInt(char *bspType)
{
	CHKERR(bspType);
	if (strncmp(bspType, "BAB", 3) == 0)
		return BSP_BAB_TYPE;
	else if (strncmp(bspType, "bab", 3) == 0)
		return BSP_BAB_TYPE;
	else if (strncmp(bspType, "PIB", 3) == 0)
		return BSP_PIB_TYPE;
	else if (strncmp(bspType, "pib", 3) == 0)
		return BSP_PIB_TYPE;
	else if (strncmp(bspType, "PCB", 3) == 0)
		return BSP_PCB_TYPE;
	else if (strncmp(bspType, "pcb", 3) == 0)
		return BSP_PCB_TYPE;
	else if (strncmp(bspType, "ESB", 3) == 0)
		return BSP_ESB_TYPE;
	else if (strncmp(bspType, "esb", 3) == 0)
		return BSP_ESB_TYPE;
	return -1;
}

/******************************************************************************
 *
 * \par Function Name: ionClear 
 *
 * \par Purpose: Clears bsp rules configured on the node.  Rules are 
 *               identified by their security source and security destination
 *               and targetted BSP block type.  For EIDs, ~ is accepted as a
 *               end-of-string wildcard.
 *
 * \param[in]  srcEid  The security source of all rules being cleared. This
 *                     may be "~" to match all security sources.
 *             destEid The security destination of all rules being cleared.
 *                     This may be "~" to match all security destinations.
 *             blockType This is the BSP block of the rule to be cleared. 
 *                       This is one of "bab", "pib", "pcb", "esb" or 
 *                       "~" to match all block types.
 * \par Notes:
 *****************************************************************************/

void	ionClear(char *srcEid, char *destEid, char *blockType)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt, temp;
	Object	ruleObj;
	int	rmCount = 0;
	char	rmStr [5];
	int	srcEidLen;
	int	destEidLen;
        int	curEidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

        CHKVOID(srcEid);
        CHKVOID(destEid);
        CHKVOID(blockType);
	CHKVOID(secdb);
	srcEidLen = istrlen(srcEid, SDRSTRING_BUFSZ);
	destEidLen = istrlen(destEid, SDRSTRING_BUFSZ);
       	if ((blockType[0] == '~') || (bspTypeToInt(blockType) == BSP_BAB_TYPE))
       	{
        	// For each bab rule, if src/dest match, delete it.  
         	OBJ_POINTER(BspBabRule, rule);
		CHKVOID(sdr_begin_xn(sdr));
		for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
			elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleObj);
		        curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
                        temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
		        	curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->securityDestEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->securityDestEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}

		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] BAB rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] ionClear: failed deleting BAB rules.");
		}
		else
		{
			writeMemo("[i] ionClear: matching BAB rules cleared.");
		}
	}

	/* TODO: Implement as we add additional security block types. */
       	if (blockType[0] == '~' || (strncmp(blockType,"pib",3) == 0))
        {
                // For each pib rule, if src/dest match, delete it.  
                OBJ_POINTER(BspPibRule, rule);
                CHKVOID(sdr_begin_xn(sdr));
                for (elt = sdr_list_first(sdr, secdb->bspPibRules); elt;
                        elt = temp)
                {
                        ruleObj = sdr_list_data(sdr, elt);
                        GET_OBJ_POINTER(sdr, BspPibRule, rule, ruleObj);
                        curEidLen = sdr_string_read(sdr, eidBuffer,
                                        rule->securitySrcEid);
                        temp = sdr_list_next(sdr, elt);
                        if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
                        {
                                curEidLen = sdr_string_read(sdr, eidBuffer,
                                                rule->securityDestEid);
                                if (eidsMatch(destEid, destEidLen, eidBuffer,
                                                curEidLen))
                                {
                                        sdr_list_delete(sdr, elt, NULL, NULL);
                                        sdr_free(sdr, rule->securitySrcEid);
                                        sdr_free(sdr, rule->securityDestEid);
                                        sdr_free(sdr, ruleObj);
                                        rmCount++;
                                }
                        }
                }

                isprintf(rmStr, 5, "%d", rmCount);
                writeMemoNote("[i] PIB rules removed", rmStr);
                if (sdr_end_xn(sdr) < 0)
                {
                        writeMemo("[?] ionClear: failed deleting PIB rules.");
                }
                else
                {
                        writeMemo("[i] ionClear: matching PIB rules cleared.");
                }
        }
	
       	if (blockType[0] == '~' || (strncmp(blockType,"pcb",3) == 0))
        {
                // For each pib rule, if src/dest match, delete it.  
                OBJ_POINTER(BspPcbRule, rule);
                CHKVOID(sdr_begin_xn(sdr));
                for (elt = sdr_list_first(sdr, secdb->bspPcbRules); elt;
                        elt = temp)
                {
                        ruleObj = sdr_list_data(sdr, elt);
                        GET_OBJ_POINTER(sdr, BspPcbRule, rule, ruleObj);
                        curEidLen = sdr_string_read(sdr, eidBuffer,
                                        rule->securitySrcEid);
                        temp = sdr_list_next(sdr, elt);
                        if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
                        {
                                curEidLen = sdr_string_read(sdr, eidBuffer,
                                                rule->securityDestEid);
                                if (eidsMatch(destEid, destEidLen, eidBuffer,
                                                curEidLen))
                                {
                                        sdr_list_delete(sdr, elt, NULL, NULL);
                                        sdr_free(sdr, rule->securitySrcEid);
                                        sdr_free(sdr, rule->securityDestEid);
                                        sdr_free(sdr, ruleObj);
                                        rmCount++;
                                }
                        }
                }

                isprintf(rmStr, 5, "%d", rmCount);
                writeMemoNote("[i] PCB rules removed", rmStr);
                if (sdr_end_xn(sdr) < 0)
                {
                        writeMemo("[?] ionClear: failed deleting PCB rules.");
                }
                else
                {
                        writeMemo("[i] ionClear: matching PCB rules cleared.");
                }

        }
 
       	if (blockType[0] == '~' || (strncmp(blockType,"esb",3) == 0))
        {
        }

	return;
}

static Object	locateKey(char *keyName, Object *nextKey)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(SecKey, key);
	int	result;

	/*	This function locates the SecKey identified by the
	 *	specified name, if any.  If none, notes the
	 *	location within the keys list at which such a key
	 *	should be inserted.					*/

	CHKZERO(ionLocked());
	if (nextKey) *nextKey = 0;	/*	Default.		*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, secdb->keys); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, SecKey, key, sdr_list_data(sdr, elt));
		result = strcmp(key->name, keyName);
		if (result < 0)
		{
			continue;
		}

		if (result > 0)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	sec_findKey(char *keyName, Object *keyAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	/*	This function finds the SecKey for the specified
	 *	node, if any.						*/

	CHKVOID(keyName);
	CHKVOID(keyAddr);
	CHKVOID(eltp);
	*eltp = 0;
	CHKVOID(sdr_begin_xn(sdr));
	elt = locateKey(keyName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	*keyAddr = sdr_list_data(sdr, elt);
	sdr_exit_xn(sdr);
	*eltp = elt;
}

static int	loadKeyValue(SecKey *key, char *fileName)
{
	Sdr	sdr = getIonsdr();
	char	*keybuf;
	int	offset;
	char	*cursor;
	int	length;
	int	bytesRead;
	int	keyfd;

	keyfd = iopen(fileName, O_RDONLY, 0);
	if (keyfd < 0)
	{
		putSysErrmsg("Can't open key value file", fileName);
		return 0;
	}

	keybuf = MTAKE(key->length);
	key->value = sdr_malloc(sdr, key->length);
	if (keybuf == NULL || key->value == 0)
	{
		putErrmsg("Failed loading key value.", key->name);
		return -1;
	}

	cursor = keybuf;
	offset = 0;
	length = key->length;
	while (length > 0)
	{
		bytesRead = read(keyfd, cursor, length);
		switch (bytesRead)
		{
		case -1:
			putSysErrmsg("Failed reading key value file", fileName);
			return -1;

		case 0:
			MRELEASE(keybuf);
			close(keyfd);
			writeMemoNote("[?] Key value file truncated",
					fileName);
			return 0;
		}

		sdr_write(sdr, key->value + offset, cursor, bytesRead);
		cursor += bytesRead;
		offset += bytesRead;
		length -= bytesRead;
	}

	MRELEASE(keybuf);
	close(keyfd);
	return 1;
}

int	sec_addKey(char *keyName, char *fileName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	Object		nextKey;
	struct stat	statbuf;
	SecKey		key;
	Object		keyObj;

	CHKERR(keyName);
	CHKERR(fileName);
	CHKERR(secdb);
	if (*keyName == '\0' || istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid key name", keyName);
		return 0;
	}

	if (stat(fileName, &statbuf) < 0)
	{
		writeMemoNote("[?] Can't stat the key value file", fileName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	if (locateKey(keyName, &nextKey) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This key is already defined", keyName);
		return 0;
	}

	/*	Okay to add this key to the database.			*/

	istrcpy(key.name, keyName, sizeof key.name);
	key.length = statbuf.st_size;
	switch (loadKeyValue(&key, fileName))
	{
	case -1:
		sdr_cancel_xn(sdr);
		putErrmsg("Failed loading key value.", keyName);
		return -1;

	case 0:
		sdr_cancel_xn(sdr);
		putErrmsg("Can't load key value.", keyName);
		return -1;
	}

	keyObj = sdr_malloc(sdr, sizeof(SecKey));
	if (keyObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create key.", keyName);
		return -1;
	}

	if (nextKey)
	{
		oK(sdr_list_insert_before(sdr, nextKey, keyObj));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, secdb->keys, keyObj));
	}

	sdr_write(sdr, keyObj, (char *) &key, sizeof(SecKey));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add key.", NULL);
		return -1;
	}

	return 1;
}

int	sec_updateKey(char *keyName, char *fileName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		keyObj;
	SecKey		key;
	struct stat	statbuf;

	CHKERR(keyName);
	CHKERR(fileName);
	if (*keyName == '\0' || istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid key name", keyName);
		return 0;
	}

	if (stat(fileName, &statbuf) < 0)
	{
		writeMemoNote("[?] Can't stat the key value file", fileName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locateKey(keyName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This key is not defined", keyName);
		return 0;
	}

	keyObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &key, keyObj, sizeof(SecKey));
	if (key.value)
	{
		sdr_free(sdr, key.value);
	}

	key.length = statbuf.st_size;
	switch (loadKeyValue(&key, fileName))
	{
	case -1:
		sdr_cancel_xn(sdr);
		putErrmsg("Failed loading key value.", keyName);
		return -1;

	case 0:
		sdr_cancel_xn(sdr);
		putErrmsg("Can't load key value.", keyName);
		return -1;
	}

	sdr_write(sdr, keyObj, (char *) &key, sizeof(SecKey));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update key.", keyName);
		return -1;
	}

	return 1;
}

int	sec_removeKey(char *keyName)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	keyObj;
		OBJ_POINTER(SecKey, key);

	CHKERR(keyName);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateKey(keyName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return 1;
	}

	keyObj = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, SecKey, key, keyObj);
	if (key->value)
	{
		sdr_free(sdr, key->value);
	}

	sdr_free(sdr, keyObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove key.", NULL);
		return -1;
	}

	return 1;
}

int	sec_get_key(char *keyName, int *keyBufferLength, char *keyValueBuffer)
{
	Sdr	sdr = getIonsdr();
	Object	keyAddr;
	Object	elt;
		OBJ_POINTER(SecKey, key);

	CHKERR(keyName);
	CHKERR(keyBufferLength);
	CHKERR(keyValueBuffer);
	CHKERR(sdr_begin_xn(sdr));
	sec_findKey(keyName, &keyAddr, &elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	GET_OBJ_POINTER(sdr, SecKey, key, keyAddr);
	if (key->length > *keyBufferLength)
	{
		sdr_exit_xn(sdr);
		*keyBufferLength = key->length;
		return 0;
	}

	sdr_read(sdr, keyValueBuffer, key->value, key->length);
	sdr_exit_xn(sdr);
	return key->length;
}

static int	filterEid(char *outputEid, char *inputEid)
{
	int	eidLength = istrlen(inputEid, MAX_SDRSTRING + 1);
	int	last = eidLength - 1;

	if (eidLength == 0 || eidLength > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Invalid eid length", inputEid);
		return 0;
	}

	/*	Note: the '~' character is used internally to
	 *	indicate "all others" (wild card) because it's the
	 *	last printable ASCII character and therefore always
	 *	sorts last in any list.  If the user wants to use
	 *	'*' instead, we just change it silently.		*/

	memcpy(outputEid, inputEid, eidLength);
	outputEid[eidLength] = '\0';
	if (outputEid[last] == '*')
	{
		outputEid[last] = '~';
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: eidsMatch 
 *
 * \par Purpose: This function accepts two string EIDs and determines if they
 *               match.  Significantly, each EID may contain an end-of-string
 *               wildcard character ("~"). For example, the two EIDs compared
 *               can be "ipn:1.~" and "ipn~".
 *
 * \retval int -- 1 - The EIDs matched, counting wildcards.
 *                0 - The EIDs did not match.
 *
 * \param[in]  firstEid     The first EID to compare. 
 *             firstEidLen  The length of the first EID string.
 *             secondEid    The second EID to compare.
 *             secondEidLen The length of the second EID string.
 *
 * \par Notes:
 *****************************************************************************/

static int	eidsMatch(char *firstEid, int firstEidLen, char *secondEid,
			int secondEidLen)
{
	int	result = 1;
	int	firstPos = -1;
	int	secondPos = -1;

	CHKZERO(firstEid);
	CHKZERO(firstEidLen > 0);
	CHKZERO(secondEid);
	CHKZERO(secondEidLen > 0);

	/*
	 * First, determine if (and the pos of) end-of-line wildcards.
	 */
	if (firstEid[firstEidLen - 1] == '~')
	{
		firstPos = firstEidLen - 1;
	}

	if (secondEid[secondEidLen - 1] == '~')
	{
		secondPos = secondEidLen-1;
	}

	/* If either or both EIDs are simply '~', this is a match. */
	if ((firstPos == 0) || (secondPos == 0))
	{
		result = 0;
	}

	/* If one/the other/both EIDs have wildcards, do a compare
	 * up to the shortest length. */
	else if ((firstPos > 0) && (secondPos > 0))
	{
		result = strncmp(firstEid, secondEid, MIN(firstPos, secondPos));
	}
	else if (firstPos > 0)
	{
		result = strncmp(firstEid, secondEid, MIN(firstPos,
				secondEidLen));
	}
	else if (secondPos > 0)
	{
		result = strncmp(firstEid, secondEid, MIN(firstEidLen,
				secondPos));
	}

	/* If no wildcards are used, do a regular compare. */
	else
	{
		result = strncmp(firstEid, secondEid, MIN(firstEidLen,
				secondEidLen));
	}

	/* If we have no differences, the EIDs must match. */
	return (result == 0);
}

int	sec_get_bspBabRule(char *srcEid, char *destEid, Object *ruleAddr,
		Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspBabRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 0;

	/*	This function determines the relevant BspBabRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBabRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->securityDestEid);
		/* If destinations match... */
		if (eidsMatch(eidBuffer, eidLen, destEid, strlen(destEid)))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, srcEid,
					strlen(srcEid)))
			{
				result = 1;
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/* 1 if found. 0 if not. and -1 on error. */
int	sec_findBspBabRule(char *srcEid, char *destEid, Object *ruleAddr,
		Object *eltp)
{
	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(srcEid, srcEid) == 0)
	|| (filterEid(destEid, destEid) == 0))
	{
		return -1;
	}

	return sec_get_bspBabRule(srcEid, destEid, ruleAddr, eltp);
}

int	sec_addBspBabRule(char *srcEid, char *destEid, char *ciphersuiteName,
		char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	BspBabRule	rule;
	Object		ruleObj;
	Object		elt;
	Object		ruleAddr;
	int		last;

	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(srcEid, srcEid) == 0)
	|| (filterEid(destEid, destEid) == 0))
	{
		return 0;
	}

	/* Don't expect a rule here already...*/
	if (sec_findBspBabRule(srcEid, destEid, &ruleAddr, &elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", destEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	last = istrlen(srcEid, 32) - 1;
	if (*(srcEid + last) != '~')
	{
		writeMemoNote("[?] Warning: this BAB rule authenticates only a \
single sender endpoint, not all endpoints on the sending node", srcEid);
	}

	last = istrlen(destEid, 32) - 1;
	if (*(destEid + last) != '~')
	{
		writeMemoNote("[?] Warning: this BAB rule authenticates only a \
single receiver endpoint, not all endpoints on the receiving node", destEid);
	}

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, srcEid);
	rule.securityDestEid = sdr_string_create(sdr, destEid);
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBabRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", destEid);
		return -1;
	}

	/*	Note that the rules list is not sorted, as there
	 *	is no obviously correct sorting order: rules are
	 *	symmetrical, specifying both security source and
	 *	security destination, and the local node might be
	 *	neither.						*/

	elt = sdr_list_insert_last(sdr, secdb->bspBabRules,ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBabRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", destEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBabRule(char *srcEid, char *destEid, char *ciphersuiteName,
		char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspBabRule	rule;

	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (strlen(keyName) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(srcEid, srcEid) == 0)
	|| (filterEid(destEid, destEid) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspBabRule(srcEid, destEid, &ruleObj, &elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				destEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspBabRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBabRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", destEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBabRule(char *srcEid, char *destEid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBabRule, rule);

	CHKERR(srcEid);
	CHKERR(destEid);
	if ((filterEid(srcEid, srcEid) == 0)
	|| (filterEid(destEid, destEid) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspBabRule(srcEid, destEid, &ruleObj, &elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				destEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->securityDestEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", destEid);
		return -1;
	}

	return 1;
}

/* Payload Integrity Block Support */

int	sec_get_bspPibTxRule(char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	return sec_get_bspPibRule("~", secDestEid, blockTypeNbr, ruleAddr,
			eltp);
}

int	sec_get_bspPibRxRule(char *secSrcEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	return sec_get_bspPibRule(secSrcEid, "~", blockTypeNbr, ruleAddr, eltp);
}

int	sec_get_bspPibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspPibRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 0;

	/*	This function determines the relevant BspPibRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspPibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspPibRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->securityDestEid);

		/* If destinations match... */
		if ((rule->blockTypeNbr == blockTypeNbr)
		&& (eidsMatch(eidBuffer, eidLen, secDestEid,
				strlen(secDestEid))))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, secSrcEid,
					strlen(secSrcEid)) == 1)
			{
				result = 1;
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/* 1 if found. 0 if not. -1 on error. */
int	sec_findBspPibRule(char *secSrcEid, char *secDestEid, int BlockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspPibRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid) == 0)
	|| (filterEid(secDestEid, secDestEid) == 0))
	{
		return -1;
	}

	return sec_get_bspPibRule(secSrcEid, secDestEid, BlockTypeNbr,
			ruleAddr, eltp);
}

int	sec_addBspPibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	BspPibRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(secSrcEid, secSrcEid) == 0)
	|| (filterEid(secDestEid, secDestEid) == 0))
	{
		return 0;
	}


	/* Don't expect a rule here already...*/
	if (sec_findBspPibRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.securityDestEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspPibRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspPibRules,ruleObj);

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspPibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspPibRule	rule;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(secSrcEid, secSrcEid) == 0)
	|| (filterEid(secDestEid, secDestEid) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspPibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspPibRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspPibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspPibRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid) == 0)
	|| (filterEid(secDestEid, secDestEid) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspPibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspPibRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->securityDestEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

int    sec_get_bspPcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
                Object *ruleAddr, Object *eltp)
{
        Sdr     sdr = getIonsdr();
        SecDB   *secdb = _secConstants();
        Object  elt;
                OBJ_POINTER(BspPcbRule, rule);
        int     eidLen;
        char    eidBuffer[SDRSTRING_BUFSZ];
        int     result = 0;

        /*      This function determines the relevant BspPcbRule for
         *      the specified receiving endpoint, if any.  Wild card
         *      match is okay.                                          */

        CHKERR(secSrcEid);
        CHKERR(secDestEid);
        CHKERR(ruleAddr);
        CHKERR(eltp);
        *eltp = 0;
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

        CHKERR(sdr_begin_xn(sdr));
        for (elt = sdr_list_first(sdr, secdb->bspPcbRules); elt;
                        elt = sdr_list_next(sdr, elt))
        {
                *ruleAddr = sdr_list_data(sdr, elt);
                GET_OBJ_POINTER(sdr, BspPcbRule, rule, *ruleAddr);
                eidLen = sdr_string_read(sdr, eidBuffer, rule->securityDestEid);

                /* If destinations match... */
                if ((rule->blockTypeNbr == blockTypeNbr)
                && (eidsMatch(eidBuffer, eidLen, secDestEid,
                                strlen(secDestEid))))
                {
                        eidLen = sdr_string_read(sdr, eidBuffer,
                                        rule->securitySrcEid);
                        /* If sources match ... */
                        if (eidsMatch(eidBuffer, eidLen, secSrcEid,
                                        strlen(secSrcEid)) == 1)
                        {
                                result = 1;
                                *eltp = elt;    /*      Exact match.    */
                                break;          /*      Stop searching. */
                        }
                }

                *ruleAddr = 0;
        }

        sdr_exit_xn(sdr);
        return result;
}

/* 1 if found. 0 if not. -1 on error. */
int     sec_findBspPcbRule(char *secSrcEid, char *secDestEid, int BlockTypeNbr,
                Object *ruleAddr, Object *eltp)
{
        /*      This function finds the BspPcbRule for the specified
         *      endpoint, if any.                                       */

        CHKERR(secSrcEid);
        CHKERR(secDestEid);
        CHKERR(ruleAddr);
        CHKERR(eltp);
        *eltp = 0;
        if ((filterEid(secSrcEid, secSrcEid) == 0)
        || (filterEid(secDestEid, secDestEid) == 0))
        {
                return -1;
        }

        return sec_get_bspPcbRule(secSrcEid, secDestEid, BlockTypeNbr,
                        ruleAddr, eltp);
}

int     sec_addBspPcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
                char *ciphersuiteName, char *keyName)
{
        Sdr             sdr = getIonsdr();
        SecDB           *secdb = _secConstants();
        BspPcbRule      rule;
        Object          ruleObj;
        Object          elt;            

        CHKERR(secSrcEid);
        CHKERR(secDestEid);
        CHKERR(ciphersuiteName);
        CHKERR(keyName);
        CHKERR(secdb);
        if (strlen(ciphersuiteName) > 31)
        {
                writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
                return 0;
        }               
        
        if (strlen(keyName) > 31)
        {
                writeMemoNote("[?] Invalid keyName", keyName);
                return 0;
        }

        if ((filterEid(secSrcEid, secSrcEid) == 0)
        || (filterEid(secDestEid, secDestEid) == 0))
        {
                return 0;
        }
    
    
        /* Don't expect a rule here already...*/
        if (sec_findBspPcbRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
                        &elt) != 0)
        {
                writeMemoNote("[?] This rule is already defined.", secDestEid);
                return 0;
        }

        /*      Okay to add this rule to the database.                  */

        CHKERR(sdr_begin_xn(sdr));
        rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
        rule.securityDestEid = sdr_string_create(sdr, secDestEid);
        rule.blockTypeNbr = blockTypeNbr;
        istrcpy(rule.ciphersuiteName, ciphersuiteName,
                        sizeof rule.ciphersuiteName);
        istrcpy(rule.keyName, keyName, sizeof rule.keyName);
        ruleObj = sdr_malloc(sdr, sizeof(BspPcbRule));
        if (ruleObj == 0)
        {
                sdr_cancel_xn(sdr);
                putErrmsg("Can't create rule.", secDestEid);
                return -1;
        }

        elt = sdr_list_insert_last(sdr, secdb->bspPcbRules,ruleObj);

        sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPcbRule));
        if (sdr_end_xn(sdr) < 0)
        {
                putErrmsg("Can't add rule.", secDestEid);
                return -1;
        }

        return 1;
}

int     sec_updateBspPcbRule(char *secSrcEid, char *secDestEid,
                int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
        Sdr             sdr = getIonsdr();
        Object          elt;
        Object          ruleObj;
        BspPcbRule      rule;

        CHKERR(secSrcEid);
        CHKERR(secDestEid);
        CHKERR(ciphersuiteName);
        CHKERR(keyName);
        if (strlen(ciphersuiteName) > 31)
        {
                writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
                return 0;
        }

        if (strlen(keyName) > 31)
        {
                writeMemoNote("[?] Invalid keyName", keyName);
                return 0;
        }

        if ((filterEid(secSrcEid, secSrcEid) == 0)
        || (filterEid(secDestEid, secDestEid) == 0))
        {
                return 0;
        }

        /* Need to have a rule to update it. */
        if (sec_findBspPcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
                        &elt) == 0)
        {
                writeMemoNote("[?] No rule defined for this endpoint.",
                                secDestEid);
                return 0;
        }

        CHKERR(sdr_begin_xn(sdr));
        sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspPcbRule));
        istrcpy(rule.ciphersuiteName, ciphersuiteName,
                        sizeof rule.ciphersuiteName);
        istrcpy(rule.keyName, keyName, sizeof rule.keyName);
        sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPcbRule));
        if (sdr_end_xn(sdr) < 0)
        {
                putErrmsg("Can't update rule.", secDestEid);
                return -1;
        }

        return 1;
}

int     sec_removeBspPcbRule(char *secSrcEid, char *secDestEid,
                int BlockTypeNbr)
{
        Sdr     sdr = getIonsdr();
        Object  elt;
        Object  ruleObj;
                OBJ_POINTER(BspPcbRule, rule);

        CHKERR(secSrcEid);
        CHKERR(secDestEid);
        if ((filterEid(secSrcEid, secSrcEid) == 0)
        || (filterEid(secDestEid, secDestEid) == 0))
        {
                return 0;
        }

        /* Need to have a rule to delete it. */
        if (sec_findBspPcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
                        &elt) == 0)
        {
                writeMemoNote("[?] No rule defined for this endpoint.",
                                secDestEid);
                return 0;
        }

        CHKERR(sdr_begin_xn(sdr));
        sdr_list_delete(sdr, elt, NULL, NULL);
        GET_OBJ_POINTER(sdr, BspPcbRule, rule, ruleObj);
        sdr_free(sdr, rule->securitySrcEid);
        sdr_free(sdr, rule->securityDestEid);
        sdr_free(sdr, ruleObj);
        if (sdr_end_xn(sdr) < 0)
        {
                putErrmsg("Can't remove rule.", secDestEid);
                return -1;
        }

        return 1;
}
