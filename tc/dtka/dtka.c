/*
	dtka.c:	public/private key pair generator for ION nodes.

		NOTE: this program utilizes functions provided by
		cryptography software that is not distributed with
		ION.  To indicate that this supporting software
		has been installed, set the compiler flag

			-DCRYPTO_SOFTWARE_INSTALLED

		when compiling this program.  Absent that flag
		setting at compile time, dtka's generateKeyPair()
		function simply uses the rand() function to generate
		pseudo keys for test purposes only.


	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "dtka.h"
#include "ionsec.h"
#include "crypto.h"

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#ifdef CRYPTO_SOFTWARE_INSTALLED
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/md.h"
#endif

#define KEY_SIZE 1024
#define EXPONENT 65537

static saddr _running(saddr *newValue)
{
	void *value;
	saddr state;

	if (newValue) /*	Changing state.		*/
	{
		value = (void *)(*newValue);
		state = (saddr)sm_TaskVar(&value);
	}
	else /*	Just check.		*/
	{
		state = (saddr)sm_TaskVar(NULL);
	}

	return state;
}

static void shutDown(int signum) /*	Commands dtka termination.	*/
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;
	saddr stop = 0;

	oK(_running(&stop)); /*	Terminates dtka.		*/
}

/*	*	*	Clock thread functions	*	*	*	*/

static int	writeAddPubKeyCmd(time_t effectiveTime, unsigned short
			publicKeyLen, unsigned char *publicKey)
{
	int		fd;
	char		nbrBuf[FQN_MAX_LENGTH];
	char		cmdbuf[2048];
	char		*cursor = cmdbuf;
	int		bytesRemaining = sizeof cmdbuf;
	int		cmdLen = 0;
	int		len;
	int		i;
	unsigned char	*keyCursor;
	int		val;

	fd = iopen("dtka.ionsecrc", O_WRONLY | O_CREAT | O_APPEND, 0777);
	if (fd < 0)
	{
		putSysErrmsg("Can't open cmd file", "dtka.ionsecrc");
		return -1;
	}

	putFqn(nbrBuf, getOwnFqnn());
	len = _isprintf(__FILE__, __LINE__, cmdbuf, sizeof cmdbuf,
			"a pubkey %s " VAST_FIELDSPEC " " VAST_FIELDSPEC " %d ",
			nbrBuf, (vast) effectiveTime, (vast) getCtime(),
			publicKeyLen);
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
		putSysErrmsg("Can't write command to add key", "dtka.ionsecrc");
		return -1;
	}

	close(fd);
	return 0;
}

#ifdef CRYPTO_SOFTWARE_INSTALLED
int	generateAESKey(int keysize, unsigned char *buf)
{
	mbedtls_ctr_drbg_context	ctr_drbg;
	mbedtls_entropy_context		entropy;
	int				result;
	const char			*pers = "aes_genkey";

	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
			(const unsigned char *)pers, strlen(pers));

	result = mbedtls_ctr_drbg_random(&ctr_drbg, buf, keysize);

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
	return result;
}

int	generateHMACKey(int keysize, unsigned char *buf)
{
	mbedtls_entropy_context		entropy;
	mbedtls_hmac_drbg_context	hmac_drbg;
	const mbedtls_md_info_t		*md_info;
	mbedtls_md_type_t		md_type;
	int				result;
	const char			*pers = "hmac_genkey";

	if (keysize == 32)
	{
		md_type = MBEDTLS_MD_SHA256;
	}
	else if (keysize == 48)
	{
		md_type = MBEDTLS_MD_SHA384;
	}
	else if (keysize == 64)
	{
		md_type = MBEDTLS_MD_SHA512;
	}
	else
	{
		putErrmsg("Unsupported key size", itoa(keysize));
	}

	md_info = mbedtls_md_info_from_type(md_type);
	mbedtls_entropy_init(&entropy);
	mbedtls_hmac_drbg_init(&hmac_drbg);
	mbedtls_hmac_drbg_seed(&hmac_drbg, md_info, mbedtls_entropy_func,
			&entropy, (const unsigned char *)pers, strlen(pers));

	result = mbedtls_hmac_drbg_random(&hmac_drbg, buf, keysize);

	mbedtls_hmac_drbg_free(&hmac_drbg);
	mbedtls_entropy_free(&entropy);
	return result;
}

/* buf and private_buf should be at least keysize bytes in size */
int	generateECDSAKey(int keysize, unsigned char *buf,
		unsigned char *private_buf)
{
	mbedtls_ecdsa_context		ecdsa_context;
	mbedtls_ctr_drbg_context	ctr_drbg;
	mbedtls_entropy_context		entropy;
	mbedtls_ecp_group_id		curve;
	int				result;
	size_t				len;
	const char			*pers = "ecdsa_genkey";

	if (keysize == 32)
	{
		curve = MBEDTLS_ECP_DP_SECP256R1;
	}
	else if (keysize == 48)
	{
		curve = MBEDTLS_ECP_DP_SECP384R1;
	}
	else
	{
		putErrmsg("Unsupported key size", itoa(keysize));
	}

	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
			(const unsigned char *)pers, strlen(pers));

	mbedtls_ecdsa_init(&ecdsa_context);
	result = mbedtls_ecdsa_genkey(&ecdsa_context, curve,
			mbedtls_ctr_drbg_random, &ctr_drbg);

	// Extract keys from context
	mbedtls_ecp_point_write_binary(&ecdsa_context.grp, &ecdsa_context.Q,
			MBEDTLS_ECP_PF_UNCOMPRESSED, &len, buf, keysize);
	mbedtls_ecp_write_key(&ecdsa_context, private_buf, keysize);

	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_ecdsa_free(&ecdsa_context);
	return result;
}
#endif

static int	generateKeyPair(BpSAP sap, DtkaDB *db, char *keyType,
			int keySize)
{
	time_t	currentTime = getCtime();
	Sdr	sdr = getIonsdr();
	time_t	effectiveTime;
	int status = 0; /* Track execution state for single exit point */
	int pubBufAllocSize = keySize;
	int privBufAllocSize = keySize;

	(void)keyType;

#ifdef CRYPTO_SOFTWARE_INSTALLED
	int	result = -1;
#else
	int	key;
#endif
	unsigned char	*pubKeyBuf = malloc(keySize);
	unsigned short	publicKeyLen = 0;
	unsigned char	*publicKey = NULL;

	unsigned char	*privKeyBuf = malloc(keySize);
	unsigned short	privateKeyLen = 0;
	unsigned char	*privateKey = NULL;

	char		recordBuffer[TC_MAX_REC];
	int		recordLen;
	SdrObject 	extent = 0;
	SdrObject 	bundleZco = 0;
	char		destEid[32];
	SdrObject	newBundle;

	/* Enforce strict memory availability before proceeding */
	if (pubKeyBuf == NULL || privKeyBuf == NULL)
	{
		putErrmsg("Can't allocate initial key buffers.", NULL);
		status = -1;
	}

	if (status == 0)
	{
		effectiveTime = currentTime + db->effectiveLeadTime;

#ifdef CRYPTO_SOFTWARE_INSTALLED
		if (strcmp(keyType, "hmac") == 0)
		{
			result = generateHMACKey(keySize, pubKeyBuf);
		}
		else if (strcmp(keyType, "aes") == 0)
		{
			result = generateAESKey(keySize, pubKeyBuf);
		}
		else if (strcmp(keyType, "ecdsa") == 0)
		{
			result = generateECDSAKey(keySize, pubKeyBuf, privKeyBuf);
		}
		else
		{
			putErrmsg("Unrecognized key type.", keyType);
			status = -1;
		}

		if (status == 0 && result != 0)
		{
			putErrmsg("Error generating key.", NULL);
			status = -1;
		}

		if (status == 0)
		{
			publicKeyLen = keySize;
			publicKey = pubKeyBuf;

			if (strcmp(keyType, "ecdsa") != 0)
			{
				if (db->keySize == 0)
				{
					putErrmsg("Configured key size is zero.", NULL);
					status = -1;
				}
				else
				{
					unsigned char *newPriv = realloc(privKeyBuf, db->keySize);
					if (newPriv == NULL)
					{
						putErrmsg("Error reallocating privKeyBuf.", NULL);
						status = -1;
					}
					else
					{
						privKeyBuf = newPriv;
						privBufAllocSize = db->keySize;
					}
				}
			}

			if (status == 0)
			{
				privateKeyLen = keySize;
				privateKey = privKeyBuf;
				sdr_exit_xn(sdr);
			}
		}
#else
		{
			uvast	ownFqnn = getOwnFqnn();

			if (ownFqnn == 0)
			{
				ownFqnn = 1;
			}

			srand((unsigned int)currentTime / ownFqnn);
			key = rand();
			memcpy(pubKeyBuf, (char *)&key, sizeof key);
			publicKey = pubKeyBuf;
			publicKeyLen = sizeof key;
			srand((unsigned int)key);
			key = rand();
			memcpy(privKeyBuf, (char *)&key, sizeof key);
			privateKey = privKeyBuf;
			privateKeyLen = sizeof key;
		}
#endif
	}

	if (status == 0 && sec_addOwnPublicKey(effectiveTime, publicKeyLen, publicKey) < 0)
	{
		putErrmsg("Can't add own public key.", NULL);
		status = -1;
	}

	if (status == 0 && sec_addPrivateKey(effectiveTime, privateKeyLen, privateKey) < 0)
	{
		putErrmsg("Can't add own private key.", NULL);
		status = -1;
	}

	if (status == 0 && writeAddPubKeyCmd(effectiveTime, publicKeyLen, publicKey) < 0)
	{
		putErrmsg("Can't write command to add node public key.", NULL);
		status = -1;
	}

	if (status == 0)
	{
		if (sap == NULL)
		{
#if TC_DEBUG
			writeMemo("dtka: recorded initial keys.");
#endif
		}
		else
		{
			recordLen = tc_serialize(recordBuffer, sizeof recordBuffer,
					 getOwnFqnn(), effectiveTime, currentTime,
					 publicKeyLen, publicKey);
			if (recordLen < 0)
			{
				putErrmsg("Can't serialize key declaration record.", NULL);
				status = -1;
			}

			if (status == 0)
			{
				if (sdr_begin_xn(sdr) < 0)
				{
					putErrmsg("Can't start SDR transaction.", NULL);
					status = -1;
				}
				else
				{
					extent = sdr_malloc(sdr, recordLen);
					if (extent)
					{
						sdr_write(sdr, extent, (char *)recordBuffer, recordLen);
					}

					if (sdr_end_xn(sdr) < 0)
					{
						putErrmsg("Can't create ZCO extent.", NULL);
						status = -1;
					}
				}
			}

			if (status == 0)
			{
				bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, recordLen,
					BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
				if (bundleZco == 0 || bundleZco == (SdrObject)-1) /* <-- Fixed upstream cast */
				{
					putErrmsg("Can't create ZCO.", NULL);
					status = -1;
				}
			}

			if (status == 0)
			{
				isprintf(destEid, sizeof destEid, "imc:%d.0", DTKA_DECLARE);
				if (bp_send(sap, destEid, NULL, 432000, BP_STD_PRIORITY,
					NoCustodyRequested, 0, 0, NULL, bundleZco, &newBundle) < 1)
				{
					putErrmsg("Can't publish key declaration bundle.", NULL);
					status = -1;
				}
			}

			if (status == 0)
			{
#if TC_DEBUG
				writeMemo("dtka: published key declaration bundle.");
#endif
			}
		}
	}

	/* * SINGLE EXIT POINT
	 * Strict Flight Rule: Cryptographic material MUST be explicitly zeroized
	 * before releasing memory to the heap. Size is tracked dynamically.
	 */
	if (pubKeyBuf != NULL)
	{
		memset(pubKeyBuf, 0, pubBufAllocSize);
		free(pubKeyBuf);
		pubKeyBuf = NULL;
	}

	if (privKeyBuf != NULL)
	{
		memset(privKeyBuf, 0, privBufAllocSize);
		free(privKeyBuf);
		privKeyBuf = NULL;
	}

	return status;
}

static void	*generateKeys(void *parm)
{
	char	*procName = "dtka";
	Sdr	sdr;
	SdrObject dbobj;
	DtkaDB	db;
	time_t	currentTime;
	char	nbrBuf[FQN_MAX_LENGTH];
	char	ownEid[32];
	char	keyType[6];
	int	keySize;
	BpSAP	sap;
	saddr	state = 1;

	/* Parameter intentionally unused. */
	(void)parm;

	/*	Main loop for DTKA key generation.			*/

	snooze(1); /*	Let main thread become interruptible.	*/
	sdr = getIonsdr();
	dbobj = getDtkaDbObject();
	if (dbobj == 0)
	{
		putErrmsg("No DTKA node database.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Can't look up key parameters.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	sdr_stage(sdr, (char *)&db, dbobj, sizeof(DtkaDB));
	istrcpy(keyType, db.keyType, sizeof(db.keyType) + 1);
	keySize = db.keySize;
	sdr_exit_xn(sdr);

	/*	Generate initial keys and initial re-keying interval.	*/

	if (generateKeyPair(NULL, &db, keyType, keySize) < 0)
	{
		putErrmsg("dtka initial key pair generation failed.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	currentTime = getCtime();
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Can't start setting initial key gen time.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	sdr_stage(sdr, (char *)&db, dbobj, sizeof(DtkaDB));
	if (db.nextKeyGenTime == 0)
	{
		db.nextKeyGenTime = currentTime + db.keyGenInterval;
		sdr_write(sdr, dbobj, (char *)&db, sizeof(DtkaDB));
	}
	#if TC_DEBUG
	writeMemoNote("Initial next key gen time", itoa(db.nextKeyGenTime));
	#endif

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't set initial DTKA next key gen time.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Now prepare for re-keying cycle.			*/

	putFqn(nbrBuf, getOwnFqnn());
	isprintf(ownEid, sizeof ownEid, "ipn:%s.0", nbrBuf);
	if (bp_open_source(ownEid, &sap, 0) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Clock loop: wait until next key generation time,
	 *	then generate key pair and schedule subsequent key
	 *	generation time.					*/

	writeMemo("[i] dtka clock thread is running.");
	while (_running(NULL))
	{
		currentTime = getCtime();
		if (sdr_begin_xn(sdr) < 0)
		{
			putErrmsg("Can't start key gen time update.", NULL);
			state = 0;
			oK(_running(&state));
			ionKillMainThread(procName);
			continue;
		}

		sdr_stage(sdr, (char *)&db, dbobj, sizeof(DtkaDB));
		if (currentTime < db.nextKeyGenTime)
		{
			sdr_exit_xn(sdr);
			snooze(1);
			continue;
		}

#if TC_DEBUG
		writeMemo("dtka: Re-keying.");
#endif
		db.nextKeyGenTime = currentTime + db.keyGenInterval;
		sdr_write(sdr, dbobj, (char *)&db, sizeof(DtkaDB));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't update DTKA next key gen time.", NULL);
			state = 0;
			oK(_running(&state));
			ionKillMainThread(procName);
			continue;
		}

		if (generateKeyPair(sap, &db, keyType, keySize) < 0)
		{
			putErrmsg("dtka key pair generation failed.", NULL);
			state = 0;
			oK(_running(&state));
			ionKillMainThread(procName);
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] dtka clock thread has ended.");
	return NULL;
}

/*	*	Functions for main loop of dtka.	*	*	*/

#if TC_DEBUG
static void	printRecord(uvast fqnn, time_t effectiveTime,
			time_t assertionTime, unsigned short datLength,
			unsigned char *datValue)
{
	char	nbrBuf[FQN_MAX_LENGTH];
	char	msgbuf[1024];
	char	*cursor = msgbuf;
	int	bytesRemaining = sizeof msgbuf;
	int	i;
	int	len;

	putFqn(nbrBuf, fqnn);
	len = _isprintf(cursor, bytesRemaining, "%s %lu %lu ", nbrBuf,
			assertionTime, effectiveTime);
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

	writeMemo(msgbuf);
}
#endif

static int	handleBulletin(char *buffer, int bufSize)
{
	char		*cursor = buffer;
	int		bytesRemaining = bufSize;
	uvast		fqnn;
	time_t		effectiveTime;
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[TC_MAX_DATLEN];
#if TC_DEBUG
	int		recCount = 0;
	char		msgbuf[72];
	writeMemo("---DTKA: Bulletin received---");
#endif
	while (bytesRemaining >= 14)
	{
		if (tc_deserialize(&cursor, &bytesRemaining, TC_MAX_DATLEN,
				&fqnn, &effectiveTime, &assertionTime,
				&datLength, datValue) == 0)
		{
			writeMemo("[?] DTKA bulletin malformed, discarded.");
			break;
		}

		if (fqnn == 0) /*	Block padding bytes.	*/
		{
			break;
		}

#if TC_DEBUG
		recCount++;
		printRecord(fqnn, effectiveTime, assertionTime, datLength,
					datValue);
#endif
		if (datLength == 0)
		{
			if (sec_removePublicKey(fqnn, effectiveTime) < 0)
			{
				putErrmsg("Failed handling bulletin.", NULL);
				MRELEASE(buffer);
				return -1;
			}

			continue;
		}

		if (sec_addPublicKey(fqnn, effectiveTime, assertionTime,
				datLength, datValue) < 0)
		{
			putErrmsg("Failed handling bulletin.", NULL);
			MRELEASE(buffer);
			return -1;
		}
	}

	MRELEASE(buffer);
#if TC_DEBUG
	isprintf(msgbuf, sizeof msgbuf, "DTKA: Number of records received: %d",
			 recCount);
	writeMemo(msgbuf);
#endif
	return 0;
}

#if defined(ION_LWT)
int dtka(int a1, int a2, int a3, int a4, int a5,
		 int a6, int a7, int a8, int a9, int a10)
{
#else
int main(void)
{
#endif
	saddr		state = 1;
	pthread_t	clockThread;
	int		result;
	char		*bulletinContent;
	int		length;

	oK(_running(&state));
	if (dtkaAttach() < 0)
	{
		putErrmsg("dtka can't attach to ION.", NULL);
		return -1;
	}

	isignal(SIGTERM, shutDown);
	writeMemo("[i] dtka is running.");
	if (pthread_begin(&clockThread, NULL, generateKeys, NULL))
	{
		putSysErrmsg("dtka can't start clock thread", NULL);
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
	writeMemo("[i] dtka has ended.");
	ionDetach();
	return 0;
}
