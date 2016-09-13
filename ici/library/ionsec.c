/*

	ionsec.c:	API for managing ION's security database.

	Copyright (c) 2009, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author:		Scott Burleigh, JPL
	Modifications:	TCSASSEMBLER, TopCoder

	Modification History:
	Date       Who     What
	9-24-13    TC      Added functions (find, add, remove, change) to
			   manage ltpXmitAuthRule and ltpRecvAuthRule
			   Updated secInitialize to initialize SecDB's 
			   ltpXmitAuthRule and ltpRecvAuthRule lists
			   Added writeRuleMessage to print rule-related message
	11-15-13  romanoTC Check for valid ciphersuite values (0,1,255)

									*/
#include "ionsec.h"

static int	eidsMatch(char *firstEid, int firstEidLen, char *secondEid,
			int secondEidLen);

static char	*_secDbName()
{
	return "secdb";
}

static char	*_secVdbName()
{
	return "secvdb";
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

static void	writeRuleMessage(char* ruleMessage, uvast engineId, 
			unsigned char ciphersuiteNbr, char *keyName)
{
	char	buf[512];

	isprintf(buf, sizeof buf, "ltp engine id " UVAST_FIELDSPEC, engineId);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" ciphersuite_nbr %c ", ciphersuiteNbr);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			"key name '%.31s'", keyName);
	writeMemoNote(ruleMessage, buf);
}

static int	orderKeyRefs(PsmPartition wm, PsmAddress refData,
			void *dataBuffer)
{
	PubKeyRef	*ref;
	PubKeyRef	*argRef;

	ref = (PubKeyRef *) psp(wm, refData);
	argRef = (PubKeyRef *) dataBuffer;
	if (ref->nodeNbr < argRef->nodeNbr)
	{
		return -1;
	}

	if (ref->nodeNbr > argRef->nodeNbr)
	{
		return 1;
	}

	/*	Matching node number.					*/

	if (ref->effectiveTime.seconds < argRef->effectiveTime.seconds)
	{
		return -1;
	}

	if (ref->effectiveTime.seconds > argRef->effectiveTime.seconds)
	{
		return 1;
	}

	if (ref->effectiveTime.count < argRef->effectiveTime.count)
	{
		return -1;
	}

	if (ref->effectiveTime.count > argRef->effectiveTime.count)
	{
		return 1;
	}

	/*	Matching effective time.				*/

	return 0;
}

static void	eraseKeyRef(PsmPartition wm, PsmAddress refData, void *arg)
{
	psm_free(wm, refData);
}

static int	loadPublicKey(PsmPartition wm, PsmAddress rbt, PublicKey *key,
			Object elt)
{
	PsmAddress	refAddr;
	PubKeyRef	*ref;

	refAddr = psm_zalloc(wm, sizeof(PubKeyRef));
	if (refAddr == 0)
	{
		return -1;
	}

	ref = (PubKeyRef *) psp(wm, refAddr);
	CHKERR(ref);
	ref->nodeNbr = key->nodeNbr;
	ref->effectiveTime.seconds = key->effectiveTime.seconds;
	ref->effectiveTime.count = key->effectiveTime.count;
	ref->publicKeyElt = elt;
	if (sm_rbt_insert(wm, rbt, refAddr, orderKeyRefs, ref) == 0)
	{
		psm_free(wm, refAddr);
		return -1;
	}

	return 0;
}

static int	loadPublicKeys(PsmAddress rbt)
{
	PsmPartition	wm = getIonwm();
	Sdr		sdr = getIonsdr();
	SecDB		db;
	Object		elt;
			OBJ_POINTER(PublicKey, nodeKey);

	sdr_read(sdr, (char *) &db, _secdbObject(NULL), sizeof(SecDB));
	for (elt = sdr_list_first(sdr, db.publicKeys); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, PublicKey, nodeKey,
				sdr_list_data(sdr, elt));
		if (loadPublicKey(wm, rbt, nodeKey, elt) < 0)
		{
			putErrmsg("Can't add public key reference.", NULL);
			return -1;
		}
	}

	return 0;
}

static SecVdb	*_secvdb(char **name)
{
	static SecVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		wm = getIonwm();
		if (psm_locate(wm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (SecVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	Security volatile database doesn't exist yet.	*/

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/

		/*	Create and catalogue the SecVdb object.	*/

		vdbAddress = psm_zalloc(wm, sizeof(SecVdb));
		if (vdbAddress == 0)
		{
			putErrmsg("No space for volatile database.", NULL);
			sdr_exit_xn(sdr);
			return NULL;
		}

		if (psm_catlg(wm, *name, vdbAddress) < 0)
		{
			putErrmsg("Can't catalogue volatile database.", NULL);
			psm_free(wm, vdbAddress);
			sdr_exit_xn(sdr);
			return NULL;
		}

		vdb = (SecVdb *) psp(wm, vdbAddress);
		vdb->publicKeys = sm_rbt_create(wm);
		if (vdb->publicKeys == 0)
		{
			putErrmsg("Can't initialize volatile database.", NULL);
			vdb = NULL;
			oK(psm_uncatlg(wm, *name));
			psm_free(wm, vdbAddress);
			sdr_exit_xn(sdr);
			return NULL;
		}

		if (loadPublicKeys(vdb->publicKeys) < 0)
		{
			putErrmsg("Can't load volatile database.", NULL);
			oK(sm_rbt_destroy(wm, vdb->publicKeys, eraseKeyRef,
					NULL));
			vdb = NULL;
			oK(psm_uncatlg(wm, *name));
			psm_free(wm, vdbAddress);
		}

		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
	}

	return vdb;
}

int	secInitialize()
{
	Sdr	ionsdr;
	Object	secdbObject;
	SecDB	secdbBuf;
	char	*secvdbName = _secVdbName();

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
		secdbBuf.publicKeys = sdr_list_create(ionsdr);
		secdbBuf.ownPublicKeys = sdr_list_create(ionsdr);
		secdbBuf.privateKeys = sdr_list_create(ionsdr);
		secdbBuf.keys = sdr_list_create(ionsdr);
		secdbBuf.bspBabRules = sdr_list_create(ionsdr);
#ifdef ORIGINAL_BSP
		secdbBuf.bspPibRules = sdr_list_create(ionsdr);
		secdbBuf.bspPcbRules = sdr_list_create(ionsdr);
#else
		secdbBuf.bspBibRules = sdr_list_create(ionsdr);
		secdbBuf.bspBcbRules = sdr_list_create(ionsdr);
#endif
		secdbBuf.ltpRecvAuthRules = sdr_list_create(ionsdr);
		secdbBuf.ltpXmitAuthRules = sdr_list_create(ionsdr);
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
	if (_secvdb(&secvdbName) == NULL)
	{
		putErrmsg("Can't initialize ION security vdb.", NULL);
		return -1;
	}

	return 0;
}

int	secAttach()
{
	Sdr	ionsdr;
	Object	secdbObject;
	SecVdb	*secvdb = _secvdb(NULL);
	char	*secvdbName = _secVdbName();

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
	if (secvdb == NULL)
	{
		if (_secvdb(&secvdbName) == NULL)
		{
			putErrmsg("Can't initialize ION security vdb.", NULL);
			return -1;
		}
	}

	return 0;
}

Object	getSecDbObject()
{
	return _secdbObject(NULL);
}

SecVdb	*getSecVdb()
{
	return _secvdb(NULL);
}

static Object	locatePublicKey(uvast nodeNbr, BpTimestamp *effectiveTime,
			PubKeyRef *argRef)
{
	SecDB		*secdb = _secConstants();
	PsmPartition	wm = getIonwm();
	SecVdb		*vdb = getSecVdb();
	char		keyId[32];
	PsmAddress	rbtNode;
	PsmAddress	successor;
	PsmAddress	refAddr;
	PubKeyRef	*ref;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKZERO(vdb);
	isprintf(keyId, sizeof keyId, UVAST_FIELDSPEC ":%u.%u", nodeNbr,
			effectiveTime->seconds, effectiveTime->count);
	argRef->nodeNbr = nodeNbr;
	argRef->effectiveTime.seconds = effectiveTime->seconds;
	argRef->effectiveTime.count = effectiveTime->count;
	rbtNode = sm_rbt_search(wm, vdb->publicKeys, orderKeyRefs, argRef,
			&successor);
	if (rbtNode == 0)
	{
		writeMemoNote("[?] This key is not defined", keyId);
		return 0;
	}

	/*	Key has been located.					*/

	refAddr = sm_rbt_data(wm, rbtNode);
	ref = (PubKeyRef *) psp(wm, refAddr);
	return ref->publicKeyElt;
}

void	sec_findPublicKey(uvast nodeNbr, BpTimestamp *effectiveTime,
		Object *keyAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	PubKeyRef	argRef;

	/*	This function finds the PublicKey for the specified
	 *	node and time, if any.					*/

	CHKVOID(effectiveTime);
	CHKVOID(keyAddr);
	CHKVOID(eltp);
	*eltp = 0;
	CHKVOID(sdr_begin_xn(sdr));
	elt = locatePublicKey(nodeNbr, effectiveTime, &argRef);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return;
	}

	*keyAddr = sdr_list_data(sdr, elt);
	sdr_exit_xn(sdr);
	*eltp = elt;
}

int	sec_addPublicKey(uvast nodeNbr, BpTimestamp *effectiveTime,
		time_t assertionTime, int keyLen, unsigned char *keyValue)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	PsmPartition	wm = getIonwm();
	uvast		localNodeNbr = getOwnNodeNbr();
	SecVdb		*vdb = getSecVdb();
	char		keyId[32];
	PubKeyRef	argRef;
	PsmAddress	rbtNode;
	PsmAddress	successor;
	Object		keyObj;
	PublicKey	newPublicKey;
	PsmAddress	successorRefAddr;
	PubKeyRef	*successorRef;
	PsmAddress	addr;
	PubKeyRef	*newRef;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	if (nodeNbr == localNodeNbr)
	{
		return 0;	/*	Own public key added elsewhere.	*/
	}

	CHKERR(vdb);
	CHKERR(nodeNbr > 0);
	CHKERR(effectiveTime);
	CHKERR(keyLen > 0);
	CHKERR(keyValue);
	isprintf(keyId, sizeof keyId, UVAST_FIELDSPEC ":%u.%u", nodeNbr,
			effectiveTime->seconds, effectiveTime->count);
	argRef.nodeNbr = nodeNbr;
	argRef.effectiveTime.seconds = effectiveTime->seconds;
	argRef.effectiveTime.count = effectiveTime->count;
	CHKERR(sdr_begin_xn(sdr));
	rbtNode = sm_rbt_search(wm, vdb->publicKeys, orderKeyRefs, &argRef,
			&successor);
	if (rbtNode)
	{
		writeMemoNote("[?] This key is already defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	New key may be added.					*/

	newPublicKey.nodeNbr = nodeNbr;
	newPublicKey.effectiveTime.seconds = effectiveTime->seconds;
	newPublicKey.effectiveTime.count = effectiveTime->count;
	newPublicKey.assertionTime = assertionTime;
	newPublicKey.length = keyLen;
	newPublicKey.value = sdr_malloc(sdr, keyLen);
	keyObj = sdr_malloc(sdr, sizeof(PublicKey));
	if (keyObj == 0 || newPublicKey.value == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add public key.", keyId);
		return -1;
	}

	sdr_write(sdr, newPublicKey.value, (char *) keyValue, keyLen);
	sdr_write(sdr, keyObj, (char *) &newPublicKey, sizeof(PublicKey));
	if (successor)
	{
		successorRefAddr = sm_rbt_data(wm, successor);
		successorRef = (PubKeyRef *) psp(wm, successorRefAddr);
		argRef.publicKeyElt = sdr_list_insert_before(sdr,
				successorRef->publicKeyElt, keyObj);
	}
	else
	{
		argRef.publicKeyElt = sdr_list_insert_last(sdr,
				secdb->publicKeys, keyObj);
	}

	addr = psm_zalloc(wm, sizeof(PubKeyRef));
	if (addr)
	{
		newRef = (PubKeyRef *) psp(wm, addr);
		memcpy((char *) newRef, (char *) &argRef, sizeof(PubKeyRef));
		oK(sm_rbt_insert(wm, vdb->publicKeys, addr, orderKeyRefs,
				newRef));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add public key.", NULL);
		return -1;
	}

	return 1;
}

int	sec_removePublicKey(uvast nodeNbr, BpTimestamp *effectiveTime)
{
	Sdr		sdr = getIonsdr();
	SecVdb		*vdb = getSecVdb();
	Object		elt;
	PubKeyRef	argRef;
	Object		keyObj;
	PublicKey	publicKey;

	CHKERR(effectiveTime);
	CHKERR(sdr_begin_xn(sdr));
	elt = locatePublicKey(nodeNbr, effectiveTime, &argRef);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	keyObj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &publicKey, keyObj, sizeof(PublicKey));
	sdr_free(sdr, publicKey.value);
	sdr_free(sdr, keyObj);
	sdr_list_delete(sdr, elt, NULL, NULL);
	sm_rbt_delete(getIonwm(), vdb->publicKeys, orderKeyRefs, &argRef, NULL,
			NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove public key.", NULL);
		return -1;
	}

	return 0;
}

static Object	locateOwnPublicKey(BpTimestamp *effectiveTime, Object *nextKey)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(OwnPublicKey, key);

	/*	This function locates the OwnPublicKey identified
	 *	by effectiveTime, if any.  If none, notes the location
	 *	within the list of own public keys at which such
	 *	a key should be inserted.				*/

	CHKZERO(ionLocked());
	if (nextKey) *nextKey = 0;	/*	Default.		*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, secdb->ownPublicKeys); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, OwnPublicKey, key,
				sdr_list_data(sdr, elt));
		if (key->effectiveTime.seconds < effectiveTime->seconds)
		{
			continue;
		}

		if (key->effectiveTime.seconds > effectiveTime->seconds)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		if (key->effectiveTime.count < effectiveTime->count)
		{
			continue;
		}

		if (key->effectiveTime.count > effectiveTime->count)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

int	sec_addOwnPublicKey(BpTimestamp *effectiveTime, int keyLen, 
		unsigned char *keyValue)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	char		keyId[32];
	Object		nextKey;
	Object		keyObj;
	OwnPublicKey	newOwnPublicKey;

	CHKERR(secdb);
	CHKERR(effectiveTime);
	CHKERR(keyLen > 0);
	CHKERR(keyValue);
	isprintf(keyId, sizeof keyId, ":%lu.%lu", effectiveTime->seconds,
			effectiveTime->count);
	CHKERR(sdr_begin_xn(sdr));
	if (locateOwnPublicKey(effectiveTime, &nextKey) != 0)
	{
		writeMemoNote("[?] This public key is already defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	New key may be added.					*/

	newOwnPublicKey.effectiveTime.seconds = effectiveTime->seconds;
	newOwnPublicKey.effectiveTime.count = effectiveTime->count;
	newOwnPublicKey.length = keyLen;
	newOwnPublicKey.value = sdr_malloc(sdr, keyLen);
	keyObj = sdr_malloc(sdr, sizeof(OwnPublicKey));
	if (keyObj == 0 || newOwnPublicKey.value == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add own public key.", keyId);
		return -1;
	}

	sdr_write(sdr, newOwnPublicKey.value, (char *) keyValue, keyLen);
	sdr_write(sdr, keyObj, (char *) &newOwnPublicKey, sizeof(OwnPublicKey));
	if (nextKey)
	{
		oK(sdr_list_insert_before(sdr, nextKey, keyObj));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, secdb->ownPublicKeys, keyObj));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add own public key.", NULL);
		return -1;
	}

	return 1;
}

int	sec_removeOwnPublicKey(BpTimestamp *effectiveTime)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	char		keyId[32];
	Object		keyElt;
	Object		keyObj;
	OwnPublicKey	ownPublicKey;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(effectiveTime);
	isprintf(keyId, sizeof keyId, ":%lu.%lu", effectiveTime->seconds,
			effectiveTime->count);
	CHKERR(sdr_begin_xn(sdr));
	keyElt = locateOwnPublicKey(effectiveTime, NULL);
	if (keyElt == 0)
	{
		writeMemoNote("[?] This public key is not defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	Key may be removed.					*/

	keyObj = sdr_list_data(sdr, keyElt);
	sdr_read(sdr, (char *) &ownPublicKey, keyObj, sizeof(OwnPublicKey));
	sdr_free(sdr, ownPublicKey.value);
	sdr_free(sdr, keyObj);
	sdr_list_delete(sdr, keyElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove own public key.", NULL);
		return -1;
	}

	return 0;
}

static Object	locatePrivateKey(BpTimestamp *effectiveTime, Object *nextKey)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(PrivateKey, key);

	/*	This function locates the PrivateKey identified by
	 *	effectiveTime, if any.  If none, notes the location
	 *	within the list of private keys at which such
	 *	a key should be inserted.				*/

	CHKZERO(ionLocked());
	if (nextKey) *nextKey = 0;	/*	Default.		*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	for (elt = sdr_list_first(sdr, secdb->privateKeys); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, PrivateKey, key, sdr_list_data(sdr, elt));
		if (key->effectiveTime.seconds < effectiveTime->seconds)
		{
			continue;
		}

		if (key->effectiveTime.seconds > effectiveTime->seconds)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		if (key->effectiveTime.count < effectiveTime->count)
		{
			continue;
		}

		if (key->effectiveTime.count > effectiveTime->count)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

int	sec_addPrivateKey(BpTimestamp *effectiveTime, int keyLen,
		unsigned char *keyValue)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	char		keyId[32];
	Object		nextKey;
	Object		keyObj;
	OwnPublicKey	newPrivateKey;

	CHKERR(secdb);
	CHKERR(effectiveTime);
	CHKERR(keyLen > 0);
	CHKERR(keyValue);
	isprintf(keyId, sizeof keyId, ":%lu.%lu", effectiveTime->seconds,
			effectiveTime->count);
	CHKERR(sdr_begin_xn(sdr));
	if (locatePrivateKey(effectiveTime, &nextKey) != 0)
	{
		writeMemoNote("[?] This private key is already defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	New key may be added.					*/

	newPrivateKey.effectiveTime.seconds = effectiveTime->seconds;
	newPrivateKey.effectiveTime.count = effectiveTime->count;
	newPrivateKey.length = keyLen;
	newPrivateKey.value = sdr_malloc(sdr, keyLen);
	keyObj = sdr_malloc(sdr, sizeof(PrivateKey));
	if (keyObj == 0 || newPrivateKey.value == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add private key.", keyId);
		return -1;
	}

	sdr_write(sdr, newPrivateKey.value, (char *) keyValue, keyLen);
	sdr_write(sdr, keyObj, (char *) &newPrivateKey, sizeof(PrivateKey));
	if (nextKey)
	{
		oK(sdr_list_insert_before(sdr, nextKey, keyObj));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, secdb->privateKeys, keyObj));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add private key.", NULL);
		return -1;
	}

	return 1;
}

int	sec_removePrivateKey(BpTimestamp *effectiveTime)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	char		keyId[32];
	Object		keyElt;
	Object		keyObj;
	PrivateKey	privateKey;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(effectiveTime);
	isprintf(keyId, sizeof keyId, ":%lu.%lu", effectiveTime->seconds,
			effectiveTime->count);
	CHKERR(sdr_begin_xn(sdr));
	keyElt = locatePrivateKey(effectiveTime, NULL);
	if (keyElt == 0)
	{
		writeMemoNote("[?] This private key is not defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	Key may be removed.					*/

	keyObj = sdr_list_data(sdr, keyElt);
	sdr_read(sdr, (char *) &privateKey, keyObj, sizeof(PrivateKey));
	sdr_free(sdr, privateKey.value);
	sdr_free(sdr, keyObj);
	sdr_list_delete(sdr, keyElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove private key.", NULL);
		return -1;
	}

	return 0;
}

int	sec_get_public_key(uvast nodeNbr, BpTimestamp *effectiveTime,
		int *keyBufferLen, unsigned char *keyValueBuffer)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	PsmPartition	wm = getIonwm();
	SecVdb		*vdb = getSecVdb();
	PubKeyRef	argRef;
	PsmAddress	rbtNode;
	PsmAddress	successor;
	PsmAddress	refAddr;
	PubKeyRef	*ref;
	Object		keyObj;
	PublicKey	publicKey;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(vdb);
	CHKERR(effectiveTime);
	CHKERR(keyBufferLen);
	CHKERR(*keyBufferLen > 0);
	CHKERR(keyValueBuffer);
	argRef.nodeNbr = nodeNbr;
	argRef.effectiveTime.seconds = effectiveTime->seconds;
	argRef.effectiveTime.count = effectiveTime->count;
	CHKERR(sdr_begin_xn(sdr));
	rbtNode = sm_rbt_search(wm, vdb->publicKeys, orderKeyRefs, &argRef,
			&successor);
	if (rbtNode == 0)	/*	No exact match (normal).	*/
	{
		if (successor == 0)
		{
			rbtNode = sm_rbt_last(wm, vdb->publicKeys);
		}
		else
		{
			rbtNode = sm_rbt_prev(wm, successor);
		}

		if (rbtNode == 0)
		{
			sdr_exit_xn(sdr);
			return 0;	/*	No such key.		*/
		}
	}

	refAddr = sm_rbt_data(wm, rbtNode);
	ref = (PubKeyRef *) psp(wm, refAddr);
	if (ref->nodeNbr != nodeNbr)
	{
		sdr_exit_xn(sdr);
		return 0;		/*	No such key.		*/
	}

	/*	Ref now points to the last-effective public key for
	 *	this node that was in effect at a time at or before
	 *	the indicated effective time.				*/

	keyObj = sdr_list_data(sdr, ref->publicKeyElt);
	sdr_read(sdr, (char *) &publicKey, keyObj, sizeof(PublicKey));
	if (publicKey.length > *keyBufferLen)
	{
		/*	Buffer is too small for this key value.		*/

		sdr_exit_xn(sdr);
		*keyBufferLen = publicKey.length;
		return 0;
	}

	sdr_read(sdr, (char *) keyValueBuffer, publicKey.value,
			publicKey.length);
	sdr_exit_xn(sdr);
	return publicKey.length;
}

int	sec_get_own_public_key(BpTimestamp *effectiveTime, int *keyBufferLen,
		unsigned char *keyValueBuffer)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	Object		keyElt;
	Object		nextKey;
	Object		keyObj;
	OwnPublicKey	ownPublicKey;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(effectiveTime);
	CHKERR(keyBufferLen);
	CHKERR(*keyBufferLen > 0);
	CHKERR(keyValueBuffer);
	keyElt = locateOwnPublicKey(effectiveTime, &nextKey);
	if (keyElt == 0)	/*	No exact match (normal).	*/
	{
		if (nextKey == 0)
		{
			keyElt = sdr_list_last(sdr, secdb->ownPublicKeys);
		}
		else
		{
			keyElt = sdr_list_prev(sdr, nextKey);
		}

		if (keyElt == 0)
		{
			sdr_exit_xn(sdr);
			return 0;	/*	No such key.		*/
		}
	}

	/*	keyElt now points to the last-effective public key
	 *	for the local node that was in effect at a time at
	 *	or before the indicated effective time.			*/

	keyObj = sdr_list_data(sdr, keyElt);
	sdr_read(sdr, (char *) &ownPublicKey, keyObj, sizeof(OwnPublicKey));
	if (ownPublicKey.length > *keyBufferLen)
	{
		/*	Buffer is too small for this key value.		*/

		sdr_exit_xn(sdr);
		*keyBufferLen = ownPublicKey.length;
		return 0;
	}

	sdr_read(sdr, (char *) keyValueBuffer, ownPublicKey.value,
			ownPublicKey.length);
	sdr_exit_xn(sdr);
	return ownPublicKey.length;
}

int	sec_get_private_key(BpTimestamp *effectiveTime, int *keyBufferLen,
		unsigned char *keyValueBuffer)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	Object		keyElt;
	Object		nextKey;
	Object		keyObj;
	PrivateKey	privateKey;

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(effectiveTime);
	CHKERR(keyBufferLen);
	CHKERR(*keyBufferLen > 0);
	CHKERR(keyValueBuffer);
	keyElt = locatePrivateKey(effectiveTime, &nextKey);
	if (keyElt == 0)	/*	No exact match (normal).	*/
	{
		if (nextKey == 0)
		{
			keyElt = sdr_list_last(sdr, secdb->privateKeys);
		}
		else
		{
			keyElt = sdr_list_prev(sdr, nextKey);
		}

		if (keyElt == 0)
		{
			sdr_exit_xn(sdr);
			return 0;	/*	No such key.		*/
		}
	}

	/*	keyElt now points to the last-effective private key
	 *	for the local node that was in effect at a time at
	 *	or before the indicated effective time.			*/

	keyObj = sdr_list_data(sdr, keyElt);
	sdr_read(sdr, (char *) &privateKey, keyObj, sizeof(PrivateKey));
	if (privateKey.length > *keyBufferLen)
	{
		/*	Buffer is too small for this key value.		*/

		sdr_exit_xn(sdr);
		*keyBufferLen = privateKey.length;
		return 0;
	}

	sdr_read(sdr, (char *) keyValueBuffer, privateKey.value,
			privateKey.length);
	sdr_exit_xn(sdr);
	return privateKey.length;
}

/******************************************************************************
 *
 * \par Function Name: sec_clearBspRules 
 *
 * \par Purpose: Clears bsp rules configured on the node.  Rules are 
 *		identified by their security source and security destination
 *		and targeted BSP block type.  For EIDs, ~ is accepted as a
 *		end-of-string wildcard.
 *
 * \param[in]	srcEid  The security source of all rules being cleared. This
 *			may be "~" to match all security sources.
 *		destEid The security destination of all rules being cleared.
 *			This may be "~" to match all security destinations.
 *		blockType This is the BSP block of the rule to be cleared. 
 *			This is one of "2", "3", "4", or "~" to match all
 *			block types.
 * \par Notes:
 *****************************************************************************/

void	sec_clearBspRules(char *srcEid, char *destEid, char *blockType)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
	Object	temp;
	Object	ruleObj;
	int	rmCount;
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
	if (blockType[0] == '~' || blockType[0] == '2')	/*	BAB	*/
	{
		/*	Remove matching authentication rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
		OBJ_POINTER(BspBabRule, rule);
#ifdef ORIGINAL_BSP
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
#else
		for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->senderEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->receiverEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->senderEid);
					sdr_free(sdr, rule->receiverEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#endif
		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] authentication rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBspRules: failed deleting \
authentication rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBspRules: matching \
authentication rules cleared.");
		}
	}

	if (blockType[0] == '~' || blockType[0] == '3')	/*	PIB/BIB	*/
	{
		/*	Remove matching integrity rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
#ifdef ORIGINAL_BSP
		OBJ_POINTER(BspPibRule, rule);
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
#else
		OBJ_POINTER(BspBibRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->destEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->destEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#endif
		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] integrity rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBspRules: failed deleting \
integrity rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBspRules: matching integrity \
rules cleared.");
		}
	}

	if (blockType[0] == '~' || blockType[0] == '4')	/*	PCB/BCB	*/
	{
		/*	Remove matching confidentiality rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
#ifdef ORIGINAL_BSP
		OBJ_POINTER(BspPcbRule, rule);
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
#else
		OBJ_POINTER(BspBcbRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->destEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->destEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#endif

		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] confidentiality rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBspRules: failed deleting \
confidentiality rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBspRules: matching \
confidentiality rules cleared.");
		}

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
		close(keyfd);
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

/*
 * -1: System Error.
 * 0: Bad key or Duplicate
 * 1: Success
 */
int sec_addKeyValue(char *keyName, char *keyVal, uint32_t keyLen)
{
	Sdr	sdr = getIonsdr();
	SecDB*	secdb = _secConstants();
	Object	nextKey;
	SecKey	key;
	Object	keyObj;

	CHKERR(keyName);
	CHKERR(keyVal);
	CHKERR(secdb);

	/* Check that a key has been passed in. */
	if (*keyName == '\0' || istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid key name", keyName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));

	/* Make sure key does not already exist. */
	if (locateKey(keyName, &nextKey) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This key is already defined", keyName);
		return 0;
	}

	/*	Store key in the SDR.		*/

	istrcpy(key.name, keyName, sizeof key.name);
	key.length = keyLen;
	key.value = sdr_malloc(sdr, key.length);
	if (key.value == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Failed loading key value.", key.name);
		return -1;
	}

	sdr_write(sdr, key.value, keyVal, keyLen);

	/* Add key object to the key database. */
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

int	sec_activeKey(char *keyName)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBabRule, babRule);
		OBJ_POINTER(BspBibRule, bibRule);
		OBJ_POINTER(BspBcbRule, bcbRule);

	CHKERR(sdr_begin_xn(sdr));

	for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBabRule, babRule, ruleObj);
		if ((strncmp(babRule->keyName, keyName, 32)) == 0)
		{
			sdr_end_xn(sdr);
			return 1;
		}
	}

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBibRule, bibRule, ruleObj);
		if ((strncmp(bibRule->keyName, keyName, 32)) == 0)
		{
			sdr_end_xn(sdr);
			return 1;
		}
	}

	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBcbRule, bcbRule, ruleObj);
		if ((strncmp(bcbRule->keyName, keyName, 32)) == 0)
		{
			sdr_end_xn(sdr);
			return 1;
		}
	}

	sdr_end_xn(sdr);

	return 0;
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

static int	filterEid(char *outputEid, char *inputEid, int eidIsInRule)
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

#ifndef ORIGINAL_BSP
	if (eidIsInRule && outputEid[last] != '~')
	{
		writeMemoNote("[?] Security rule not for entire node, rejected",
				outputEid);
		return 0;
	}
#endif

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: eidsMatch 
 *
 * \par Purpose: This function accepts two string EIDs and determines if they
 *		match.  Significantly, each EID may contain an end-of-string
 *		wildcard character ("~"). For example, the two EIDs compared
 *		can be "ipn:1.~" and "ipn~".
 *
 * \retval int -- 1 - The EIDs matched, counting wildcards.
 *		0 - The EIDs did not match.
 *
 * \param[in]	firstEid	The first EID to compare. 
 *		firstEidLen	The length of the first EID string.
 *		secondEid	The second EID to compare.
 *		secondEidLen	The length of the second EID string.
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

#ifdef ORIGINAL_BSP
void	sec_get_bspBabRule(char *srcEid, char *destEid, Object *ruleAddr,
		Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspBabRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBabRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(srcEid);
	CHKVOID(destEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
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
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}
	}

	sdr_exit_xn(sdr);
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
	if ((filterEid(srcEid, srcEid, 0) == 0)
	|| (filterEid(destEid, destEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBabRule(srcEid, destEid, ruleAddr, eltp);
	return (*eltp != 0);
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

	if ((filterEid(srcEid, srcEid, 1) == 0)
	|| (filterEid(destEid, destEid, 1) == 0))
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

	if ((filterEid(srcEid, srcEid, 1) == 0)
	|| (filterEid(destEid, destEid, 1) == 0))
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
	if ((filterEid(srcEid, srcEid, 1) == 0)
	|| (filterEid(destEid, destEid, 1) == 0))
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
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
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
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
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

int	sec_get_bspPcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspPcbRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 0;

	/*	This function determines the relevant BspPcbRule for
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
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching. */
			}
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/* 1 if found. 0 if not. -1 on error. */
int	sec_findBspPcbRule(char *secSrcEid, char *secDestEid, int BlockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspPcbRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	return sec_get_bspPcbRule(secSrcEid, secDestEid, BlockTypeNbr,
			ruleAddr, eltp);
}

int	sec_addBspPcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	BspPcbRule	rule;
	Object		ruleObj;
	Object		elt;

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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
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

	/*	Okay to add this rule to the database.			*/

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

int	sec_updateBspPcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspPcbRule	rule;

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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
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

int	sec_removeBspPcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspPcbRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
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
#else
/*		Bundle Authentication Block Support			*/

void	sec_get_bspBabRule(char *senderEid, char *receiverEid, Object *ruleAddr,
		Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspBabRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBabRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(senderEid);
	CHKVOID(receiverEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBabRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->receiverEid);
		/* If destinations match... */
		if (eidsMatch(eidBuffer, eidLen, receiverEid,
				strlen(receiverEid)))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->senderEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, senderEid,
					strlen(senderEid)))
			{
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}
	}

	sdr_exit_xn(sdr);
}

int	sec_findBspBabRule(char *senderEid, char *receiverEid, Object *ruleAddr,
		Object *eltp)
{
	CHKERR(senderEid);
	CHKERR(receiverEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(senderEid, senderEid, 0) == 0)
	|| (filterEid(receiverEid, receiverEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBabRule(senderEid, receiverEid, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBspBabRule(char *senderEid, char *receiverEid,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	BspBabRule	rule;
	Object		ruleObj;
	Object		elt;
	Object		ruleAddr;

	CHKERR(senderEid);
	CHKERR(receiverEid);
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

	if ((filterEid(senderEid, senderEid, 1) == 0)
	|| (filterEid(receiverEid, receiverEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBabRule(senderEid, receiverEid, &ruleAddr, &elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", receiverEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.senderEid = sdr_string_create(sdr, senderEid);
	rule.receiverEid = sdr_string_create(sdr, receiverEid);
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBabRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", receiverEid);
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
		putErrmsg("Can't add rule.", receiverEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBabRule(char *senderEid, char *receiverEid,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspBabRule	rule;

	CHKERR(senderEid);
	CHKERR(receiverEid);
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

	if ((filterEid(senderEid, senderEid, 1) == 0)
	|| (filterEid(receiverEid, receiverEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBabRule(senderEid, receiverEid, &ruleObj, &elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this node",
				receiverEid);
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
		putErrmsg("Can't update rule.", receiverEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBabRule(char *senderEid, char *receiverEid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBabRule, rule);

	CHKERR(senderEid);
	CHKERR(receiverEid);
	if ((filterEid(senderEid, senderEid, 1) == 0)
	|| (filterEid(receiverEid, receiverEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBabRule(senderEid, receiverEid, &ruleObj, &elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this node",
				receiverEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleObj);
	sdr_free(sdr, rule->senderEid);
	sdr_free(sdr, rule->receiverEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", receiverEid);
		return -1;
	}

	return 1;
}

/*		Block Integrity Block Support				*/

void	sec_get_bspBibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspBibRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBibRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(secSrcEid);
	CHKVOID(secDestEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBibRule, rule, *ruleAddr);

		if (blockTypeNbr != 0 && rule->blockTypeNbr != blockTypeNbr)
		{
			continue;	/*	Wrong target blk type.	*/
		}

		eidLen = sdr_string_read(sdr, eidBuffer, rule->destEid);

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
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}
	}

	sdr_exit_xn(sdr);
}

int	sec_findBspBibRule(char *secSrcEid, char *secDestEid, int blkType,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspBibRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBibRule(secSrcEid, secDestEid, blkType, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBspBibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	BspBibRule	rule;
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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}


	if (sec_findBspBibRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.destEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBibRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspBibRules,ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspBibRule	rule;

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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspBibRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBibRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->destEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

/*		Block Confidentiality Block Support			*/

void	sec_get_bspBcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
		OBJ_POINTER(BspBcbRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBcbRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(secSrcEid);
	CHKVOID(secDestEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBcbRule, rule, *ruleAddr);
		if (blockTypeNbr != 0 && rule->blockTypeNbr != blockTypeNbr)
		{
			continue;	/*	Wrong target blk type.	*/
		}

		eidLen = sdr_string_read(sdr, eidBuffer, rule->destEid);
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
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching. */
			}
		}
	}

	sdr_exit_xn(sdr);
}

int	sec_findBspBcbRule(char *secSrcEid, char *secDestEid, int blkType,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspBcbRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBcbRule(secSrcEid, secDestEid, blkType, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBspBcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	BspBcbRule	rule;
	Object		ruleObj;
	Object		elt;

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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Don't expect a rule here already...*/
	if (sec_findBspBcbRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined.", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.destEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBcbRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspBcbRules,ruleObj);

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspBcbRule	rule;

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

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspBcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspBcbRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBcbRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspBcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->destEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

Object sec_get_bspBibRuleList()
{
	SecDB	*secdb = _secConstants();

	if(secdb == NULL)
	{
		return 0;
	}

	return secdb->bspBibRules;
}

Object sec_get_bspBcbRuleList()
{
	SecDB	*secdb = _secConstants();

	if(secdb == NULL)
	{
		return 0;
	}

	return secdb->bspBcbRules;
}

/* Size is the maximum size of a key name. */
int	sec_get_bpsecNumKeys(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
/*		OBJ_POINTER(SecDB, db);		*/
	int result = 0;

	CHKERR(size);
	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->keys);
	/* TODO: This should be a #define. */
	*size = 32;
	sdr_exit_xn(sdr);

	return result;
}

/*
 * Generates NULL-terminated, CSV of key names
 * in the PRE-ALLOCATED buffer of given length.
 */
void sec_get_bpsecKeys(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
/*		OBJ_POINTER(SecDB, db);		*/
	SecDB	*secdb = _secConstants();
		OBJ_POINTER(SecKey, key);
	Object	elt;
	Object	obj;

	char *cursor = NULL;
	int idx = 0;
	int key_len = 0;

	CHKVOID(buffer);

	memset(buffer, 0, length);
	cursor = buffer;

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->keys); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, SecKey, key, obj);

		if ((key != NULL) && ((key_len = strlen(key->name)) > 0))
		{
			/* Make sure there is room in the buffer to
			 * hold the key name.
			 */
			if ((idx + key_len + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_cancel_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, key->name, key_len);
			cursor += key_len;
			*cursor = ',';
			cursor += 1;
			idx += key_len + 1;
		}
	}

	/* If we put anything in the buffer, there is now
	 * a trailing ",". Replace it with a NULL terminator.
	 */
	if (buffer != cursor)
	{
		cursor--;
		cursor[0] = '\0';
	}

	sdr_end_xn(sdr);
}

int  sec_get_bpsecNumCSNames(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	/*	OBJ_POINTER(SecDB, db);		*/
	int	result = 0;

	CHKERR(size);

	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->bspBibRules);
	result += sdr_list_length(sdr, secdb->bspBcbRules);

	/* TODO: This should be a #define. */
	*size = 32;
	sdr_exit_xn(sdr);

	return result;
}

void sec_get_bpsecCSNames(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	/*	OBJ_POINTER(SecDB, db);		*/
	SecDB	*secdb = _secConstants();
		OBJ_POINTER(BspBibRule, bibRule);
		OBJ_POINTER(BspBcbRule, bcbRule);
	Object	elt;
	Object	obj;

	char *cursor = NULL;
	int idx = 0;
	int size = 0;

	CHKVOID(buffer);

	memset(buffer, 0, length);

	CHKVOID(sdr_begin_xn(sdr));
	cursor = buffer;

	/* Walk through the BIB rules and gather ciphersuite names. */

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBibRule, bibRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */
		size = strlen(bibRule->ciphersuiteName);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, bibRule->ciphersuiteName, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}

	/* Walk through the BCB rules and gather ciphersuite names. */
	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBcbRule, bcbRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */
		size = strlen(bcbRule->ciphersuiteName);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, bcbRule->ciphersuiteName, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}


	/* If we put anything in the buffer, there is now
	 * a trailing ",". Replace it with a NULL terminator.
	 */
	if (buffer != cursor)
	{
		cursor--;
		cursor[0] = '\0';
	}

	sdr_exit_xn(sdr);
}

int  sec_get_bpsecNumSrcEIDs(int *size)
{
	Sdr	sdr = getIonsdr();
	/*	OBJ_POINTER(SecDB, db);		*/
	SecDB	*secdb = _secConstants();
	int	result = 0;

	CHKERR(size);

	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->bspBibRules);
	result += sdr_list_length(sdr, secdb->bspBcbRules);

	/* TODO: This should be a #define. */
	*size = SDRSTRING_BUFSZ;
	sdr_exit_xn(sdr);

	return result;
}

void sec_get_bpsecSrcEIDs(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	/*	OBJ_POINTER(SecDB, db);		*/
	SecDB	*secdb = _secConstants();
		OBJ_POINTER(BspBibRule, bibRule);
		OBJ_POINTER(BspBcbRule, bcbRule);
	Object	elt;
	Object	obj;

	char	*cursor = NULL;
	int	idx = 0;
	int	size = 0;
	char	eidBuffer[SDRSTRING_BUFSZ];

	CHKVOID(buffer);

	memset(buffer, 0, length);

	CHKVOID(sdr_begin_xn(sdr));
	cursor = buffer;

	/* Walk through the BIB rules and gather ciphersuite names. */

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBibRule, bibRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */

		size = sdr_string_read(sdr, eidBuffer, bibRule->securitySrcEid);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, eidBuffer, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}

	/* Walk through the BCB rules and gather ciphersuite names. */
	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBcbRule, bcbRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */
		size = sdr_string_read(sdr, eidBuffer, bcbRule->securitySrcEid);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, eidBuffer, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}


	/* If we put anything in the buffer, there is now
	 * a trailing ",". Replace it with a NULL terminator.
	 */
	if (buffer != cursor)
	{
		cursor--;
		cursor[0] = '\0';
	}

	sdr_exit_xn(sdr);

}

#endif

/******************************************************************************
 *
 * \par Function Name: sec_findLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to find an LTP signing rule.
 * 		 There is a match if there is a rule in Sdr with the
 *		 same ltpEngineId. 
 *
 *		 Return 1 if there is a match, return 0 if not, return -1 on 
 *		 error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \param[out]	ruleAddr the pointer to the matched rule object
 *		eltp the pointer to the matched rule iterator
 * \par Notes:
 *****************************************************************************/

int	sec_findLtpXmitAuthRule(uvast ltpEngineId, Object *ruleAddr,
		Object * eltp)
{
	CHKERR(ruleAddr);
	CHKERR(eltp);

	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
	int	result = 0;
		OBJ_POINTER(LtpXmitAuthRule, rule);	

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}
	
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpXmitAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, *ruleAddr);
		if (rule->ltpEngineId == ltpEngineId)
		{
			result = 1;
			*eltp = elt;
			break;
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: sec_addLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to add an LTP signing rule. 
 *
 *		 Return 1 if added successfully, return 0 if not, return -1 on 
 *		 error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_addLtpXmitAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	LtpXmitAuthRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secdb);	
	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Don't expect a rule here already...*/
	if (sec_findLtpXmitAuthRule(ltpEngineId, &ruleObj, &elt) != 0)
	{
		writeRuleMessage("[?] This rule is already defined", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.ltpEngineId = ltpEngineId;
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(LtpXmitAuthRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", NULL);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->ltpXmitAuthRules, ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpXmitAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_updateLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to update an LTP signing rule. 
 *
 *		 Return 1 if updated successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_updateLtpXmitAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	Object		ruleObj;
	Object		elt;
	LtpXmitAuthRule	rule;

	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Need to have a rule to update it. */
	if (sec_findLtpXmitAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(LtpXmitAuthRule));
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpXmitAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_removeLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to remove an LTP signing rule. 
 *
 *		 Return 1 if removed successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \par Notes:
 *****************************************************************************/

int	sec_removeLtpXmitAuthRule(uvast ltpEngineId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;

	/* Need to have a rule to delete it. */
	if (sec_findLtpXmitAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine.", 
				ltpEngineId, 0, "");
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_findLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to find an LTP authentication 
 *		 rule.  There is a match if there is a rule in Sdr with the
 *		 same ltpEngineId. 
 *
 *		 Return 1 if there is a match, return 0 if not, return -1 on 
 *		 error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \param[out]	ruleAddr the pointer to the matched rule object
 *		eltp the pointer to the matched rule iterator
 * \par Notes:
 *****************************************************************************/

int	sec_findLtpRecvAuthRule(uvast ltpEngineId, Object *ruleAddr,
		Object * eltp)
{
	CHKERR(ruleAddr);
	CHKERR(eltp);

	Sdr	sdr = getIonsdr();
	SecDB	*secdb = _secConstants();
	Object	elt;
	int	result = 0;
		OBJ_POINTER(LtpRecvAuthRule, rule);	

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}
	
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpRecvAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpRecvAuthRule, rule, *ruleAddr);
		if (rule->ltpEngineId == ltpEngineId)
		{
			result = 1;
			*eltp = elt;
			break;
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: sec_addLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to add an LTP authentication rule. 
 *
 *		Return 1 if added successfully, return 0 if not, return -1 on 
 *		error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_addLtpRecvAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	LtpRecvAuthRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secdb);	
	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Don't expect a rule here already...*/
	if (sec_findLtpRecvAuthRule(ltpEngineId, &ruleObj, &elt) != 0)
	{
		writeRuleMessage("[?] This rule is already defined", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.ltpEngineId = ltpEngineId;
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(LtpRecvAuthRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", NULL);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->ltpRecvAuthRules, ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpRecvAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_updateLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to update an LTP authentication 
 *		 rule. 
 *
 *		 Return 1 if updated successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_updateLtpRecvAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	Object		ruleObj;
	Object		elt;
	LtpRecvAuthRule	rule;

	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Need to have a rule to update it. */
	if (sec_findLtpRecvAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(LtpRecvAuthRule));
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpRecvAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_removeLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to remove an LTP authentication 
 *		 rule. 
 *
 *		 Return 1 if removed successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \par Notes:
 *****************************************************************************/

int	sec_removeLtpRecvAuthRule(uvast ltpEngineId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;

	/* Need to have a rule to delete it. */
	if (sec_findLtpRecvAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine.", 
				ltpEngineId, 0, "");
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}
