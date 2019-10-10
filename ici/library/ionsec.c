/*

	ionsec.c:	API for managing ION's global security database.

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
	06-27-19  SB	   Extracted LTP and BP functions.

									*/
#include "ionsec.h"

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

	if (ref->effectiveTime < argRef->effectiveTime)
	{
		return -1;
	}

	if (ref->effectiveTime > argRef->effectiveTime)
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
	ref->effectiveTime = key->effectiveTime;
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
		secdbBuf.rules[0] = sdr_list_create(ionsdr);
		secdbBuf.rules[1] = sdr_list_create(ionsdr);
		secdbBuf.rules[2] = sdr_list_create(ionsdr);
		secdbBuf.rules[3] = sdr_list_create(ionsdr);
		secdbBuf.rules[4] = sdr_list_create(ionsdr);
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

SecDB	*getSecConstants()
{
	return _secConstants();
}

SecVdb	*getSecVdb()
{
	return _secvdb(NULL);
}

static Object	locatePublicKey(uvast nodeNbr, time_t effectiveTime,
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
	isprintf(keyId, sizeof keyId, UVAST_FIELDSPEC ":%lu", nodeNbr,
			effectiveTime);
	argRef->nodeNbr = nodeNbr;
	argRef->effectiveTime = effectiveTime;
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

void	sec_findPublicKey(uvast nodeNbr, time_t effectiveTime, Object *keyAddr,
		Object *eltp)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	PubKeyRef	argRef;

	/*	This function finds the PublicKey for the specified
	 *	node and time, if any.					*/

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

int	sec_addPublicKey(uvast nodeNbr, time_t effectiveTime,
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
	CHKERR(keyLen > 0);
	CHKERR(keyValue);
	isprintf(keyId, sizeof keyId, UVAST_FIELDSPEC ":%lu", nodeNbr,
			effectiveTime);
	argRef.nodeNbr = nodeNbr;
	argRef.effectiveTime = effectiveTime;
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
	newPublicKey.effectiveTime = effectiveTime;
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

int	sec_removePublicKey(uvast nodeNbr, time_t effectiveTime)
{
	Sdr		sdr = getIonsdr();
	SecVdb		*vdb = getSecVdb();
	Object		elt;
	PubKeyRef	argRef;
	Object		keyObj;
	PublicKey	publicKey;

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

static Object	locateOwnPublicKey(time_t effectiveTime, Object *nextKey)
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
		if (key->effectiveTime < effectiveTime)
		{
			continue;
		}

		if (key->effectiveTime > effectiveTime)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

int	sec_addOwnPublicKey(time_t effectiveTime, int keyLen, 
		unsigned char *keyValue)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	char		keyId[32];
	Object		nextKey;
	Object		keyObj;
	OwnPublicKey	newOwnPublicKey;

	CHKERR(secdb);
	CHKERR(keyLen > 0);
	CHKERR(keyValue);
	isprintf(keyId, sizeof keyId, ":%lu", effectiveTime);
	CHKERR(sdr_begin_xn(sdr));
	if (locateOwnPublicKey(effectiveTime, &nextKey) != 0)
	{
		writeMemoNote("[?] This public key is already defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	New key may be added.					*/

	newOwnPublicKey.effectiveTime = effectiveTime;
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

int	sec_removeOwnPublicKey(time_t effectiveTime)
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

	isprintf(keyId, sizeof keyId, ":%lu", effectiveTime);
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

static Object	locatePrivateKey(time_t effectiveTime, Object *nextKey)
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
		if (key->effectiveTime < effectiveTime)
		{
			continue;
		}

		if (key->effectiveTime > effectiveTime)
		{
			if (nextKey) *nextKey = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

int	sec_addPrivateKey(time_t effectiveTime, int keyLen,
		unsigned char *keyValue)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = _secConstants();
	char		keyId[32];
	Object		nextKey;
	Object		keyObj;
	OwnPublicKey	newPrivateKey;

	CHKERR(secdb);
	CHKERR(keyLen > 0);
	CHKERR(keyValue);
	isprintf(keyId, sizeof keyId, ":%lu", effectiveTime);
	CHKERR(sdr_begin_xn(sdr));
	if (locatePrivateKey(effectiveTime, &nextKey) != 0)
	{
		writeMemoNote("[?] This private key is already defined", keyId);
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	New key may be added.					*/

	newPrivateKey.effectiveTime = effectiveTime;
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

int	sec_removePrivateKey(time_t effectiveTime)
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

	isprintf(keyId, sizeof keyId, ":%lu", effectiveTime);
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

int	sec_get_public_key(uvast nodeNbr, time_t effectiveTime,
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
	CHKERR(keyBufferLen);
	CHKERR(*keyBufferLen > 0);
	CHKERR(keyValueBuffer);
	argRef.nodeNbr = nodeNbr;
	argRef.effectiveTime = effectiveTime;
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

int	sec_get_own_public_key(time_t effectiveTime, int *keyBufferLen,
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

int	sec_get_private_key(time_t effectiveTime, int *keyBufferLen,
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

	/*	This function finds the SecKey identified by the
	 *	specified name, if any.					*/

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

int	sec_addKeyValue(char *keyName, char *keyVal, uint32_t keyLen)
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

int	sec_get_key(char *keyName, int *keyBufferLength, char *keyValueBuffer)
{
	Sdr	sdr = getIonsdr();
	Object	keyAddr;
	Object	elt;
		OBJ_POINTER(SecKey, key);

	CHKERR(keyName);
	CHKERR(keyBufferLength);
	CHKERR(*keyBufferLength > 0);
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
