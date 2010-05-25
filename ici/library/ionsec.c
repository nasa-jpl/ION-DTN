/*

	ionsec.c:	API for managing ION's security database.

	Author:		Scott Burleigh, JPL

	Copyright (c) 2009, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "ionsec.h"

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

	if (db == NULL)
	{
		sdr_read(getIonsdr(), (char *) &buf, _secdbObject(0),
				sizeof(SecDB));
		db = &buf;
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
	sdr_begin_xn(ionsdr);
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
		secdbBuf.babTxRules = sdr_list_create(ionsdr);
		secdbBuf.babRxRules = sdr_list_create(ionsdr);
		secdbBuf.pibTxRules = sdr_list_create(ionsdr);
		secdbBuf.pibRxRules = sdr_list_create(ionsdr);
		secdbBuf.pcbTxRules = sdr_list_create(ionsdr);
		secdbBuf.pcbRxRules = sdr_list_create(ionsdr);
		secdbBuf.esbTxRules = sdr_list_create(ionsdr);
		secdbBuf.esbRxRules = sdr_list_create(ionsdr);
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
		sdr_begin_xn(ionsdr);
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

void	ionClear(char *eid)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BabRxRule, rule);

	if (secAttach() < 0)
	{
		writeMemo("[?] ionClear can't find ION security.");
		return;
	}

	if (eid == NULL || *eid == '\0')
	{
		/*	Function must remove all BAB reception rules,
		 *	effectively disabling BAB reception checking
		 *	altogether.					*/

		sdr_begin_xn(sdr);
		while (1)
		{
			elt = sdr_list_first(sdr, secdb->babRxRules);
			if (elt == 0)
			{
				break;
			}

			ruleObj = sdr_list_data(sdr, elt);
			sdr_list_delete(sdr, elt, NULL, NULL);
			GET_OBJ_POINTER(sdr, BabRxRule, rule, ruleObj);
			sdr_free(sdr, rule->xmitEid);
			sdr_free(sdr, ruleObj);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] ionClear: failed deleting BABRX rules.");
		}
		else
		{
			writeMemo("[i] ionClear: no remaining BAB reception \
rules.");
		}

		return;
	}

	/*	Function must disable the BAB reception rule identified
	 *	by this EID (which may be wild-carded).			*/

	if (sec_updateBabRxRule(eid, "", "") < 1)
	{
		writeMemoNote("[?] ionClear: can't clear BAB reception rule.",
				eid);
	}
	else
	{
		writeMemo("[i] ionClear: cleared BAB reception rule.");
	}
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
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_findKey can't find ION security.");
		return;
	}

	sdr_begin_xn(sdr);
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

	keyfd = open(fileName, O_RDONLY, 0);
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
			writeMemoNote("[?] Key value file truncated.",
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
	Object		elt;

	CHKERR(keyName);
	CHKERR(fileName);
	if (*keyName == '\0' || strlen(keyName) > 31)
	{
		writeMemoNote("[?] Invalid key name", keyName);
		return 0;
	}

	if (stat(fileName, &statbuf) < 0)
	{
		writeMemoNote("[?] Can't stat the key value file.", fileName);
		return 0;
	}

	if (secAttach() < 0)
	{
		writeMemo("[?] sec_addKey can't find ION security.");
		return 0;
	}

	sdr_begin_xn(sdr);
	if (locateKey(keyName, &nextKey) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This key is already defined.", keyName);
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
		elt = sdr_list_insert_before(sdr, nextKey, keyObj);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, secdb->keys, keyObj);
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
	if (*keyName == '\0' || strlen(keyName) > 31)
	{
		writeMemoNote("[?] Invalid key name", keyName);
		return 0;
	}

	if (stat(fileName, &statbuf) < 0)
	{
		writeMemoNote("[?] Can't stat the key value file.", fileName);
		return 0;
	}

	if (secAttach() < 0)
	{
		writeMemo("[?] sec_updateKey can't find ION security.");
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locateKey(keyName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This key is not defined.", keyName);
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
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_removeKey can't find ION security.");
		return 0;
	}

	sdr_begin_xn(sdr);
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
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_get_key can't find ION security.");
		return 0;
	}

	sdr_begin_xn(sdr);
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
	int	eidLength = strlen(inputEid);
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

int	sec_get_babTxRule(char *eid, Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BabTxRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result;
	int	last;

	/*	This function determines the relevant BabTxRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKERR(eid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_get_babTxRule can't find ION security.");
		return 0;
	}

	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, secdb->babTxRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BabTxRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->recvEid);
		result = strcmp(eidBuffer, eid);
		if (result < 0)
		{
			continue;
		}
		
		if (result == 0)
		{
			*eltp = elt;	/*	Exact match.		*/
			break;		/*	Stop searching.		*/
		}

		/*	eid in rule > eid, but it might
		 *	still be a wild-card match.			*/

		last = eidLen - 1;
		if (eidBuffer[last] == '~'	/*	"all endpoints"	*/
		&& strncmp(eidBuffer, eid, eidLen - 1) == 0)
		{
			*eltp = elt;	/*	Wild-card match.	*/
			break;		/*	Stop searching.		*/
		}

		/*	Same as end of list.				*/

		break;
	}

	sdr_exit_xn(sdr);
	return 0;
}

static Object	locateBabTxRule(char *eid, Object *nextBabTxRule)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BabTxRule, rule);
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result;

	/*	This function locates the BabTxRule identified by the
	 *	specified receiving endpoint, if any; must be an exact
	 *	match.  If none, notes the location within the rules
	 *	list at which such a rule should be inserted.		*/

	CHKZERO(ionLocked());
	if (nextBabTxRule) *nextBabTxRule = 0;	/*	Default.	*/
	for (elt = sdr_list_first(sdr, secdb->babTxRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BabTxRule, rule, sdr_list_data(sdr, elt));
		sdr_string_read(sdr, eidBuffer, rule->recvEid);
		result = strcmp(eidBuffer, eid);
		if (result < 0)
		{
			continue;
		}
		
		if (result > 0)
		{
			if (nextBabTxRule) *nextBabTxRule = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	sec_findBabTxRule(char *eid, Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	/*	This function finds the BabTxRule for the specified
	 *	endpoint, if any.					*/

	CHKVOID(eid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_findBabTxRule can't find ION security.");
		return;
	}

	if (filterEid(eid, eid) == 0)
	{
		return;
	}

	sdr_begin_xn(sdr);
	elt = locateBabTxRule(eid, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	*ruleAddr = sdr_list_data(sdr, elt);
	sdr_exit_xn(sdr);
	*eltp = elt;
}

int	sec_addBabTxRule(char *eid, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	Object		nextBabTxRule;
	BabTxRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(eid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_addBabTxRule can't find ION security.");
		return 0;
	}

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

	if (filterEid(eid, eid) == 0)
	{
		return 0;
	}

	sdr_begin_xn(sdr);
	if (locateBabTxRule(eid, &nextBabTxRule) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This rule is already defined.", eid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	rule.recvEid = sdr_string_create(sdr, eid);
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BabTxRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", eid);
		return -1;
	}

	if (nextBabTxRule)
	{
		elt = sdr_list_insert_before(sdr, nextBabTxRule, ruleObj);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, secdb->babTxRules,
				ruleObj);
	}

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BabTxRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", eid);
		return -1;
	}

	return 1;
}

int	sec_updateBabTxRule(char *eid, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BabTxRule	rule;

	CHKERR(eid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_updateBabTxRule can't find ION security.");
		return 0;
	}

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

	if (filterEid(eid, eid) == 0)
	{
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locateBabTxRule(eid, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No rule defined for this endpoint.", eid);
		return 0;
	}

	ruleObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BabTxRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BabTxRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", eid);
		return -1;
	}

	return 1;
}

int	sec_removeBabTxRule(char *eid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BabTxRule, rule);

	CHKERR(eid);
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_removeBabTxRule can't find ION security.");
		return 0;
	}

	if (filterEid(eid, eid) == 0)
	{
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locateBabTxRule(eid, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return 1;
	}

	ruleObj = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BabTxRule, rule, ruleObj);
	sdr_free(sdr, rule->recvEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", eid);
		return -1;
	}

	return 1;
}

int	sec_get_babRxRule(char *eid, Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BabRxRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result;
	int	last;

	/*	This function determines the relevant BabRxRule for
	 *	the specified sending endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKERR(eid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_get_babRxRule can't find ION security.");
		return 0;
	}

	sdr_begin_xn(sdr);
	for (elt = sdr_list_first(sdr, secdb->babRxRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BabRxRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->xmitEid);
		result = strcmp(eidBuffer, eid);
		if (result < 0)
		{
			continue;
		}
		
		if (result == 0)
		{
			*eltp = elt;	/*	Exact match.		*/
			break;		/*	Stop searching.		*/
		}

		/*	eid in rule > eid, but it might
		 *	still be a wild-card match.			*/

		last = eidLen - 1;
		if (eidBuffer[last] == '~'	/*	"all endpoints"	*/
		&& strncmp(eidBuffer, eid, eidLen - 1) == 0)
		{
			*eltp = elt;	/*	Wild-card match.	*/
			break;		/*	Stop searching.		*/
		}

		/*	Same as end of list.				*/

		break;
	}

	sdr_exit_xn(sdr);
	return 0;
}

static Object	locateBabRxRule(char *eid, Object *nextBabRxRule)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BabRxRule, rule);
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result;

	/*	This function locates the BabRxRule identified by the
	 *	specified sending endpoint, if any; must be an exact
	 *	match.  If none, notes the location within the rules
	 *	list at which such a rule should be inserted.		*/

	CHKZERO(ionLocked());
	if (nextBabRxRule) *nextBabRxRule = 0;	/*	Default.	*/
	for (elt = sdr_list_first(sdr, secdb->babRxRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BabRxRule, rule, sdr_list_data(sdr, elt));
		sdr_string_read(sdr, eidBuffer, rule->xmitEid);
		result = strcmp(eidBuffer, eid);
		if (result < 0)
		{
			continue;
		}
		
		if (result > 0)
		{
			if (nextBabRxRule) *nextBabRxRule = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	sec_findBabRxRule(char *eid, Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	/*	This function finds the BabRxRule for the specified
	 *	endpoint, if any.					*/

	CHKVOID(eid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_findBabRxRule can't find ION security.");
		return;
	}

	if (filterEid(eid, eid) == 0)
	{
		return;
	}

	sdr_begin_xn(sdr);
	elt = locateBabRxRule(eid, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	*ruleAddr = sdr_list_data(sdr, elt);
	sdr_exit_xn(sdr);
	*eltp = elt;
}

int	sec_addBabRxRule(char *eid, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	Object		nextBabRxRule;
	BabRxRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(eid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_addBabRxRule can't find ION security.");
		return 0;
	}

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

	if (filterEid(eid, eid) == 0)
	{
		return 0;
	}

	sdr_begin_xn(sdr);
	if (locateBabRxRule(eid, &nextBabRxRule) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This rule is already defined.", eid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	rule.xmitEid = sdr_string_create(sdr, eid);
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BabRxRule));
	if (ruleObj)
	{
		if (nextBabRxRule)
		{
			elt = sdr_list_insert_before(sdr, nextBabRxRule,
					ruleObj);
		}
		else
		{
			elt = sdr_list_insert_last(sdr, secdb->babRxRules,
					ruleObj);
		}

		sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BabRxRule));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", eid);
		return -1;
	}

	return 1;
}

int	sec_updateBabRxRule(char *eid, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BabRxRule	rule;

	CHKERR(eid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_updateBabRxRule can't find ION security.");
		return 0;
	}

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

	if (filterEid(eid, eid) == 0)
	{
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locateBabRxRule(eid, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("No rule defined for this endpoint.", eid);
		return 0;
	}

	ruleObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BabRxRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BabRxRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", eid);
		return -1;
	}

	return 1;
}

int	sec_removeBabRxRule(char *eid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BabRxRule, rule);

	CHKERR(eid);
	if (secAttach() < 0)
	{
		writeMemo("[?] sec_removeBabRxRule can't find ION security.");
		return 0;
	}

	if (filterEid(eid, eid) == 0)
	{
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locateBabRxRule(eid, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return 1;
	}

	ruleObj = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BabRxRule, rule, ruleObj);
	sdr_free(sdr, rule->xmitEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", eid);
		return -1;
	}

	return 1;
}
