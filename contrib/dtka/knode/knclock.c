/*
	knclock.c:	public/private key pair generator for ION
			nodes.

			NOTE: this program utilizes functions
			provided by cryptography software that is
			not distributed with ION.  To indicate that
			this supporting software has been installed,
			set the compiler flag
			
				-DCRYPTO_SOFTWARE_INSTALLED
				
			when compiling this program.  Absent that
			flag setting at compile time, knclock's
			generateKeyPair() function does nothing.


	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "knode.h"
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

static void	shutDown()	/*	Commands knclock termination.	*/
{
	long	stop = 0;

	oK(_running(&stop));	/*	Terminates knclock.		*/
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

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	knclock(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	char		ownEid[32];
	BpSAP		sap;
	Sdr		sdr;
	Object		dbobj;
	DtkaNodeDB	db;
	long		state = 1;
	time_t		currentTime;
	unsigned int	secondsToWait;

	if (knodeAttach() < 0)
	{
		putErrmsg("knclock can't attach to DTKA.", NULL);
		return 1;
	}

	isprintf(ownEid, sizeof ownEid, "ipn:" UVAST_FIELDSPEC ".%d",
			getOwnNodeNbr(), DTKA_DECLARE);
	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		ionDetach();
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

	/*	Main loop: wait until next key generation time, then
	 *	generate key pair and schedule subsequent key
	 *	generation time.					*/

	oK(_running(&state));
	isignal(SIGTERM, shutDown);
	writeMemo("[i] knclock is running.");
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
			putErrmsg("knclock key pair generation failed.", NULL);
			state = 0;
			oK(_running(&state));
			continue;
		}
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] knclock has ended.");
	ionDetach();
	return 0;
}
