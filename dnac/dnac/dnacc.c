/*
	dnacc.c:	"child" agent for DTN node auto-configuration.

			The dnacc utility runs as an application
			in an ION node that has been automatically
			configured by the dnacp utility running in
			a separate "sponsor" node on the same
			computer.  Its purpose is to establish the
			new node's public/private key pair and pass
			the public key back to the sponsor node.
			This enables the sponsor node to attest to
			the new node's authenticity by asserting
			its public key to the network's delay-tolerant
			public key infrastructure.

	Author: Scott Burleigh, IPNGROUP

	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
	
									*/
#include "ionsec.h"
#include "crypto.h"
#include "dtka.h"

#if defined (ION_LWT)
int	dnacc(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	unsigned short	portNbr = atoi(a1);
	char		*passwdPathName = (char *) a2;
#else
int	main(int argc, char *argv[])
{
	unsigned short	portNbr = (argc > 1 ? atoi(argv[1]) : 0);
	char		*passwdPathName = (argc > 2 ? argv[2] : NULL);
#endif
	char		socketName[32] = "127.0.0.1:0";
	time_t		currentTime = getCtime();
	Object		dbobj;
	DtkaDB		db;
	Sdr		sdr;
	time_t		effectiveTime;
	unsigned char	pubKeyBuf[16000];
	unsigned short	publicKeyLen;
	unsigned char	*publicKey;
	unsigned char	privKeyBuf[16000];
	unsigned short	privateKeyLen;
	unsigned char	*privateKey;
	int		sock;
	unsigned short	keyLen;

	/*	Immediately connect to local socket identified by
	 *	portNbr, by which new public key will be returned.	*/

	switch (itcp_connect(socketName, portNbr, &sock))
	{
	case -1:
		putErrmsg("dnacc failed connecting to dnacp", NULL);
		return 1;

	case 0:
		writeMemo("dnacc unable to connect to dnacp");
		return 1;

	default:
		break;
	}

	/*	Now compute and return the new public key.		*/

	if (dtkaAttach() < 0)
	{
		putErrmsg("dnacc can't attach to DTKA system.", NULL);
		closesocket(sock);
		return 1;
	}

	dbobj = getDtkaDbObject();
	if (dbobj == 0)
	{
		putErrmsg("No DTKA node database.", NULL);
		closesocket(sock);
		return 1;
	}

	/*	Set initial re-keying time.				*/

	sdr = getIonsdr();
	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Can't start setting initial key gen time.", NULL);
		closesocket(sock);
		return 1;
	}

	sdr_stage(sdr, (char *) &db, dbobj, sizeof(DtkaDB));
	effectiveTime = currentTime + db.effectiveLeadTime;
	db.nextKeyGenTime = currentTime + db.keyGenInterval;
	sdr_write(sdr, dbobj, (char *) &db, sizeof(DtkaDB));

	/*	Generate initial keys.					*/

	if (sec_generate_key_pair(pubKeyBuf, sizeof pubKeyBuf, &publicKey,
			&publicKeyLen, privKeyBuf, sizeof privKeyBuf,
			&privateKey, &privateKeyLen) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Key pair generation failed.", NULL);
		closesocket(sock);
		return 1;
	}

	/*	Store public and private keys locally.  (ION's
	 *	sec_addPrivateKey does not encrypt the stored key, so
	 *	the password path name is not used here.)		*/

	(void)passwdPathName;
	if (sec_addPrivateKey(effectiveTime, privateKeyLen, privateKey) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add own private key.", NULL);
		closesocket(sock);
		return 1;
	}

	if (sec_addOwnPublicKey(effectiveTime, publicKeyLen, publicKey) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add own public key.", NULL);
		closesocket(sock);
		return 1;
	}

	if (sec_addPublicKey(getOwnFqnn(), effectiveTime, currentTime,
			publicKeyLen, publicKey) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't add public key.", NULL);
		closesocket(sock);
		return 1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't set initial DTKA next key gen time.", NULL);
		closesocket(sock);
		return 1;
	}

	/*	Return the newly computed public key.			*/

	effectiveTime = htonl(effectiveTime);
	switch (itcp_send(&sock, (char *) &effectiveTime, sizeof effectiveTime))
	{
	case -1:
		putErrmsg("dnacc failed sending key effective time.", NULL);
		closesocket(sock);
		return 1;

	case 0:
		writeMemo("dnacc unable to send key effective time.");
		closesocket(sock);
		return 1;

	default:
		break;
	}

	currentTime = htonl(currentTime);
	switch (itcp_send(&sock, (char *) &currentTime, sizeof currentTime))
	{
	case -1:
		putErrmsg("dnacc failed sending key assertion time.", NULL);
		closesocket(sock);
		return 1;

	case 0:
		writeMemo("dnacc unable to send key assertion time.");
		closesocket(sock);
		return 1;

	default:
		break;
	}

	keyLen = htons(publicKeyLen);
	switch (itcp_send(&sock, (char *) &keyLen, sizeof keyLen))
	{
	case -1:
		putErrmsg("dnacc failed sending public key length.", NULL);
		closesocket(sock);
		return 1;

	case 0:
		writeMemo("dnacc unable to send public key length.");
		closesocket(sock);
		return 1;

	default:
		break;
	}

	switch (itcp_send(&sock, (char *) publicKey, publicKeyLen))
	{
	case -1:
		putErrmsg("dnacc failed sending public key.", NULL);
		closesocket(sock);
		return 1;

	case 0:
		writeMemo("dnacc unable to send public key.");
		closesocket(sock);
		return 1;

	default:
		break;
	}

	closesocket(sock);
	writeErrmsgMemos();
	writeMemo("[i] dnacc initialization completed.");
	ionDetach();
	return 0;
}
