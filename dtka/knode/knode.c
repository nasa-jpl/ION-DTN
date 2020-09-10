/*
	knode.c:	public/private key pair generator for ION
			nodes.

			NOTE: this program utilizes functions
			provided by cryptography software that is
			not distributed with ION.  To indicate that
			this supporting software has been installed,
			set the compiler flag
			
				-DCRYPTO_SOFTWARE_INSTALLED
				
			when compiling this program.  Absent that
			flag setting at compile time, knode's
			generateKeyPair() function does nothing.


	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "knode.h"

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#ifdef CRYPTO_SOFTWARE_INSTALLED
#include "polarssl/config.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/bignum.h"
#include "polarssl/rsa.h"
#include "polarssl/x509.h"
#include "polarssl/base64.h"
#include "polarssl/x509write.h"
#endif

#define KEY_SIZE 1024
#define EXPONENT 65537

static long	_running(long *newValue)
{
	void	*value;
	long	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (long) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (long) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown()	/*	Commands knode termination.	*/
{
	long	stop = 0;

	oK(_running(&stop));	/*	Terminates knode.		*/
}

/*	*	*	Clock thread functions	*	*	*	*/

static Object	_knodedbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static DtkaNodeDB	*_knodeConstants()
{
	static DtkaNodeDB	buf;
	static DtkaNodeDB	*db = NULL;
	Sdr			sdr;
	Object			dbObject;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _knodedbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaNodeDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtkaNodeDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

static Object	getKnodeDbObject()
{
	return _knodedbObject(NULL);
}

static DtkaNodeDB	*getKnodeConstants()
{
	return _knodeConstants();
}

static char	*_knodedbName()
{
	return "knodedb";
}

static int	knodeInit()
{
	Sdr		sdr;
	Object		knodedbObject;
	DtkaNodeDB	knodedbBuf;
	char		*knodevdbName = _knodevdbName();
	int		i;
	DtkaAuthority	*auth;
	int		seqLength = DTKA_FEC_Q / 2;
	int		sharenum;

	sdr = getIonsdr();

	/*	Recover the knode database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	switch (knodedbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for knode database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		knodedbObject = sdr_malloc(sdr, sizeof(DtkaNodeDB));
		if (knodedbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for knode database.", NULL);
			return -1;
		}

		/*	Initialize the knode database.			*/

		memset((char *) &knodedbBuf, 0, sizeof(DtkaNodeDB));

		/*	Default values.					*/

		knodedbBuf.nextKeyGenTime = getCtime() + 4;
		knodedbBuf.keyGenInterval = 604800;	/*	Weekly	*/
		knodedbBuf.effectiveLeadTime = 345600;	/*	4 days	*/
		sdr_write(sdr, knodedbObject, (char *) &knodedbBuf,
				sizeof(DtkaNodeDB));
		sdr_catlg(sdr, _knodedbName(), 0, knodedbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create knode database.", NULL);
			return -1;
		}

		break;

	default:			/*	Found DB in the SDR.	*/
		sdr_exit_xn(sdr);
	}

	oK(_knodedbObject(&knodedbObject));/*	Save database location.	*/
	oK(_knodeConstants());
	return 0;		/*	TCC service is available.	*/
}

static int	knodeAttach()
{
	Object		knodedbObject = _knodedbObject(NULL);
	Sdr		sdr;
	char		*knodevdbName = _knodevdbName();

	if (bp_attach() < 0)
	{
		putErrmsg("DTKA can't attach to ION.", NULL);
		return -1;
	}

	if (knodedbObject)
	{
		return 0;		/*	Already attached.	*/
	}

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));	/*	Lock database.		*/
	knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	sdr_exit_xn(sdr);		/*	Unlock database.	*/
	if (knodedbObject == 0)
	{
		if (knodeInit() < 0)
		{
			putErrmsg("Can't find DTKA database.", NULL);
			return -1;
		}

		knodedbObject = sdr_find(sdr, _knodedbName(), NULL);
	}

	oK(_knodedbObject(&knodedbObject));
	oK(_knodeConstants());
	return 0;		/*	DTKA service is available.	*/
}

#ifdef CRYPTO_SOFTWARE_INSTALLED
static int	writeAddPubKeyCmd(time_t effectiveTime,
			unsigned short publicKeyLen, unsigned char *publicKey)
{
	int		fd;
	char		cmdbuf[2048];
	char		*cursor = cmdbuf;
	int		bytesRemaining = sizeof cmdbuf;
	int		cmdLen = 0;
	int		len;
	int		i;
	unsigned char	*keyCursor;
	int		val;

	fd = iopen("kn.ionsecrc", O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd < 0)
	{
		putSysErrmsg("Can't open cmd file", "kn.ionsecrc");
		return -1;
	}

	len = _isprintf(cmdbuf, sizeof cmdbuf, "a pubkey " UVAST_FIELDSPEC 
			" %d %d %d ", getOwnNodeNbr(), effectiveTime,
			getCtime(), publicKeyLen);
	cursor += len;
	bytesRemaining -= len;
	cmdLen += len;
	for (i = 0, keyCursor = publicKey; i < publicKeyLen; i++, keyCursor++)
	{
		val = *keyCursor;
		isprintf(cursor, bytesRemaining, "%02x", val);
		cursor += 2;
		bytesRemaining -= 2;
		cmdLen += 2;
	}

	*cursor = '\n';
	cmdLen += 1;
	if (write(fd, cmdbuf, cmdLen) < 0)
	{
		putSysErrmsg("Can't write command to add key", "kn.ionsecrc");
		return -1;
	}

	close(fd);
	return 0;
}
#endif

static int	generateKeyPair(BpSAP sap, DtkaNodeDB *db)
{
#ifdef CRYPTO_SOFTWARE_INSTALLED
	time_t			currentTime = getCtime();
	Sdr			sdr = getIonsdr();
	time_t			effectiveTime;
	entropy_context		entropy;
	ctr_drbg_context	ctr_drbg;
	const char		*pers = "rsa_genkey";
	rsa_context		rsa;
	int			result;
	unsigned char		pubKeyBuf[16000];
	unsigned short		publicKeyLen;
	unsigned char		*publicKey;
	unsigned char		privKeyBuf[16000];
	unsigned short		privateKeyLen;
	unsigned char		*privateKey;
	unsigned char		recordBuffer[DTKA_MAX_REC];
	int			recordLen;
	Object			extent;
	Object			bundleZco;
	char			destEid[32];
	Object			newBundle;

	effectiveTime = currentTime + db->effectiveLeadTime;
	entropy_init(&entropy);
	if (ctr_drbg_init(&ctr_drbg, entropy_func, &entropy,
			(const unsigned char *) pers, strlen(pers)))
	{
		putErrmsg("ctr_drbg_init failed.", NULL);
		return -1;
	}

	rsa_init(&rsa, RSA_PKCS_V15, 0);
	if (rsa_gen_key(&rsa, ctr_drbg_random, &ctr_drbg, KEY_SIZE, EXPONENT))
	{
		putErrmsg("rsa_gen_key failed.", NULL);
		return -1;
	}

	result = rsa_check_privkey(&rsa);
	if (result != 0)
	{
		putErrmsg("Bad private key.", itoa(result));
		return -1;
	}

	result = rsa_check_pubkey(&rsa);
	if (result != 0)
	{
		putErrmsg("Bad public key.", itoa(result));
		return -1;
	}

	/*	Extract public key from context.			*/

	result = x509_write_pubkey_der(pubKeyBuf, sizeof pubKeyBuf, &rsa);
	if (result < 0)
	{
		putErrmsg("Can't extract public key.", NULL);
		return -1;
	}

	publicKeyLen = result;
	publicKey = (pubKeyBuf + (sizeof pubKeyBuf - 1)) - publicKeyLen;

	/*	Extract private key from context.			*/

	result = x509_write_key_der(privKeyBuf, sizeof privKeyBuf, &rsa);
	if (result < 0)
	{
		putErrmsg("Can't extract private key.", NULL);
		return -1;
	}

	privateKeyLen = result;
	privateKey = (privKeyBuf + (sizeof privKeyBuf - 1)) - privateKeyLen;

	/*	Store public and private keys locally.			*/

	if (sec_addOwnPublicKey(effectiveTime, publicKeyLen, publicKey) < 0)
	{
		putErrmsg("Can't add own public key.", NULL);
		return -1;
	}

	if (sec_addPrivateKey(effectiveTime, privateKeyLen, privateKey) < 0)
	{
		putErrmsg("Can't add own public key.", NULL);
		return -1;
	}

	/*	Write "add public key" command to kn.ionsecrc file.	*/

	if (writeAddPubKeyCmd(effectiveTime, publicKeyLen, publicKey) < 0)
	{
		putErrmsg("Can't write command to add node public key.", NULL);
		return -1;
	}

	/*	Publish new public key declaration record.		*/

	recordLen = dtka_serialize(recordBuffer, sizeof recordBuffer,
			getOwnNodeNbr(), effectiveTime, currentTime,
			publicKeyLen, publicKey);
	if (recordLen < 0)
	{
		putErrmsg("Can't serialize key declaration record.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	extent = sdr_malloc(sdr, recordLen);
	if (extent)
	{
		sdr_write(sdr, extent, (char *) recordBuffer, recordLen);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't create ZCO extent.", NULL);
		return -1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, recordLen,
			BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (Object) -1)
	{
		putErrmsg("Can't create ZCO.", NULL);
		return -1;
	}

	isprintf(destEid, sizeof destEid, "imc:%d.0", DTKA_DECLARE);
	if (bp_send(sap, destEid, NULL, 432000, BP_STD_PRIORITY,
		NoCustodyRequested, 0, 0, NULL, bundleZco, &newBundle) < 1)
	{
		putErrmsg("Can't publish key declaration bundle.", NULL);
		return -1;
	}

	rsa_free(&rsa);
#endif
	return 0;
}

static void	generateKeys(void *parm)
{
	char		ownEid[32];
	char		*procName = "knode";
	BpSAP		sap;
	Sdr		sdr;
	Object		dbobj;
	DtkaNodeDB	db;
	time_t		currentTime;
	unsigned int	secondsToWait;

	/*	Main loop for DTKA key generation.			*/

	snooze(1);	/*	Let main thread become interruptible.	*/
	if (knodeAttach() < 0)
	{
		putErrmsg("knode can't attach to ION.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	dbobj = getKnodeDbObject();
	if (dbobj == 0)
	{
		putErrmsg("No DTKA node database.", NULL);
		ionDetach();
		return 1;
	}

	sdr_read(sdr, (char *) &db, dbobj, sizeof(DtkaNodeDB));
	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
		return 1;
	}

	/*	Clock loop: wait until next key generation time,
	 *	then generate key pair and schedule subsequent key
	 *	generation time.					*/

	while (_running(NULL))
	{
		currentTime = getCtime();
		if (currentTime < db.nextKeyGenTime)
		{
			secondsToWait = db.nextKeyGenTime - currentTime;
			snooze(secondsToWait);
			continue;	/*	In case interrupted.	*/
		}

		if (sdr_begin_xn(sdr) < 0)
		{
			putErrmsg("Can't update DTKA next key gen time.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}

		sdr_stage(sdr, (char *) &db, dbobj, sizeof(DtkaNodeDB));
		db.nextKeyGenTime += db.keyGenInterval;
		sdr_write(sdr, dbobj, (char *) &db, sizeof(DtkaNodeDB));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't update DTKA next key gen time.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}

		if (generateKeyPair(sap, &db) < 0)
		{
			putErrmsg("knode key pair generation failed.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] knode clock thread has ended.");
	return NULL;
}

/*	*	Functions for main loop of knode.	*	*	*/

static void	printRecord(uvast nodeNbr, time_t effectiveTime,
			time_t assertionTime, unsigned short datLength,
			unsigned char *datValue)
{
	char	msgbuf[1024];
	char	*cursor = msgbuf;
	int	bytesRemaining = sizeof msgbuf;
	int	i;
	int	len;

	len = _isprintf(cursor, bytesRemaining, UVAST_FIELDSPEC " %lu %lu ",
			nodeNbr, assertionTime, effectiveTime);
	cursor += len;
	bytesRemaining -= len;
	if (datLength == 0)
	{
		istrcpy(cursor, "[revoke]", bytesRemaining);
		cursor += 8;
		bytesRemaining -= 8;
	}
	else
	{
		for (i = 0; i < datLength; i++)
		{
			len = _isprintf(cursor, bytesRemaining, "%02x",
					datValue[i]);
			cursor += len;
			bytesRemaining -= len;
		}
	}

	PUTS(msgbuf);
}

static int	handleBulletin(unsigned char *buffer, int bufSize)
{
	unsigned char	*cursor = buffer;
	int		bytesRemaining = bufSize;
	uvast		nodeNbr;
	time_t		effectiveTime;
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[DTKA_MAX_DATLEN];
	int		recCount = 0;
	char		msgbuf[72];

	PUTS("\n---Bulletin received---");
	while (bytesRemaining >= 14)
	{
		if (dtka_deserialize(&cursor, &bytesRemaining, DTKA_MAX_DATLEN,
				&nodeNbr, &effectiveTime, &assertionTime,
				&datLength, datValue) < 1)
		{
			writeMemo("[?] DTKA bulletin malformed, discarded.");
			break;
		}

		if (nodeNbr == 0)	/*	Block padding bytes.	*/
		{
			break;
		}

		recCount++;
		printRecord(nodeNbr, effectiveTime, assertionTime, datLength,
				datValue);
		if (datLength == 0)
		{
			if (sec_removePublicKey(nodeNbr, effectiveTime) < 0)
			{
				putErrmsg("Failed handling bulletin.", NULL);
				MRELEASE(buffer);
				return -1;
			}

			continue;
		}

		if (sec_addPublicKey(nodeNbr, effectiveTime, assertionTime,
				datLength, datValue) < 0)
		{
			putErrmsg("Failed handling bulletin.", NULL);
			MRELEASE(buffer);
			return -1;
		}
	}

	MRELEASE(buffer);
	isprintf(msgbuf, sizeof msgbuf, "Number of records received: %d",
			recCount);
	PUTS(msgbuf);
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	knode(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	long		state = 1;
	pthread_t	clockThread;
	int		result;
	char		*bulletinContent;
	int		length;

	oK(_running(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] knode is running.");
	if (pthread_begin(&clockThread, NULL, generateKeys, NULL))
	{
		putSysErrmsg("knode can't start clock thread", NULL);
		return -1;
	}

	/*	Main loop: get bulletin content and apply it to
	 *	the security database.					*/

	while (_running(NULL))
	{
		if (tcc_getBulletin(DTKA_ANNOUNCE, &bulletinContent, &length)
				< 0)
		{
			putErrmsg("Failed getting bulletin content.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}

		if (bulletinContent == NULL || length == 0)
		{
			/*	Normal termination.			*/

			state = 0;
			oK(_running(&state));
			continue;
		}


		result = handleBulletin(bulletinContent, length);
		MRELEASE(bulletinContent);
		if (result < 0)
		{
			putErrmsg("Failed handling bulletin content.", NULL);
			state = 0;
			oK(_running(&state));
		}
	}

	pthread_join(clockThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] knode has ended.");
	ionDetach();
	return 0;
}
