/*
 *	bcb_arc4.c:	implementation of the BCB_ARC4 ciphersuite.
 *
 *			Note that in this ciphersuite a session key,
 *			encrypted in a pre-placed shared secret key,
 *			is present in the *results* field of the BCB
 *			and -- when decrypted -- is used to decrypt
 *			the target block.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bcb_arc4.h"
#include "crypto.h"

#if (BCB_DEBUGGING == 1)
extern char	gMsg[];			/*	Debug message buffer.	*/
#endif

#define BCB_ENCRYPTION_CHUNK_SIZE	4096
#define	BSP_BCB_SESSION_KEY_LENGTH	128

int	bcb_arc4_construct(ExtensionBlock *blk, BspOutboundBlock *asb)
{
	asb->ciphersuiteType = BSP_CSTYPE_BCB_ARC4;
	asb->ciphersuiteFlags = BSP_ASB_RES;

	/*	Result length 131 is the length of a type/length/value
	 *	triplet: length of result information item type (1)
	 *	plus length of the length of the security result (the
	 *	length of an SDNV large enough to contain the length
	 *	of the session key, i.e., 2) plus the length of the
	 *	session key itself (BCB_BSP_BCB_SESSION_KEY_LEN = 128).	*/

	asb->resultsLen = 131;
	asb->parmsLen = 0;
	asb->parmsData = 0;
	asb->resultsData = 0;
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: generateSessionKey
 *                                    
 * \par Purpose: Generates a random session key string
 *
 * \retval unsigned char * - A pointer to the session key string generated
 *
 * \param[in]  sessionKeyLen - Pointer to put the length of the session key in
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation.
 *****************************************************************************/

static unsigned char	*generateSessionKey(unsigned int *sessionKeyLen) 
{
	int		i = 0;
	char		possibleChar[] = "abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
			/*	-_+=[]{};:\\|'\",.<>?/!@#$%^&*()~`	*/
	unsigned char	*sessionKey;
	unsigned char	*cursor;

	*sessionKeyLen = BSP_BCB_SESSION_KEY_LENGTH + 1;
	sessionKey = MTAKE(*sessionKeyLen);
	CHKNULL(sessionKey);

	/*	Randomly select characters out of the set of possible
	 *	characters and write into string.			*/

	cursor = sessionKey;
	for (i = 0; i < BSP_BCB_SESSION_KEY_LENGTH; i++, cursor++) 
	{
		*cursor = possibleChar[(rand() % (sizeof(possibleChar) - 1))];
	}

	/*	NULL-terminate the string per ARC4 requirements.	*/

	*cursor = '\0';
	return sessionKey;
}

static int	cryptPayload(Bundle *bundle, unsigned char *sessionKey,
			unsigned int sessionKeyLen, char *opString)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*dataBuffer = NULL;
	arc4_context	arcContext;
	ZcoReader	dataReader;
	unsigned int	bytesRemaining = 0;
	unsigned int	offset;
	unsigned int	chunkSize = BCB_ENCRYPTION_CHUNK_SIZE;
	unsigned int	bytesRetrieved = 0;

	CHKERR(sessionKey);
	BCB_DEBUG_INFO("+ %scryptPayload(0x%x, '%s' %d)", opString,
			(unsigned long) bundle, sessionKey, sessionKeyLen);

	/*	Allocate data buffer for arc4 crypto work area.		*/

	dataBuffer = MTAKE(BCB_ENCRYPTION_CHUNK_SIZE);
	CHKERR(dataBuffer);

	/*	Set up the context for arc4.				*/

	arc4_setup(&arcContext, sessionKey, sessionKeyLen);

	/*	Encrypt/decrypt the bundle's payload ZCO, one chunk
	 *	at a time, writing the results back to the ZCO.		*/

	oK(sdr_begin_xn(bpSdr));
	zco_start_transmitting(bundle->payload.content, &dataReader);
	bytesRemaining = bundle->payload.length;
	offset = 0;
	BCB_DEBUG_INFO("i encryptPayload: size is %d", bytesRemaining);
	while (bytesRemaining > 0)
	{
		memset(dataBuffer, 0, sizeof(BCB_ENCRYPTION_CHUNK_SIZE));
		if (bytesRemaining < chunkSize)
		{
			chunkSize = bytesRemaining;
		}

		bytesRetrieved = zco_transmit(bpSdr, &dataReader, chunkSize,
				(char *) dataBuffer);
		if (bytesRetrieved != chunkSize)
		{
			BCB_DEBUG_ERR("x bsp_pcbCryptPayload: Read %d bytes, \
but expected %d.", bytesRetrieved, chunkSize);
			BCB_DEBUG_PROC("- bsp_pcbCryptPayload--> %d", -1);
			break;		/*	Out of loop.		*/
		}

		arc4_crypt(&arcContext, chunkSize, dataBuffer, dataBuffer);
		if (zco_revise(bpSdr, bundle->payload.content, offset,
					(char *) dataBuffer, chunkSize) < 0)
		{
			putErrmsg("Can't write ARC4 ciphertext.", NULL);
			break;		/*	Out of loop.		*/
		}

		offset += bytesRetrieved;
		bytesRemaining -= bytesRetrieved;
	}

	MRELEASE(dataBuffer);
	if (bytesRemaining != 0)	/*	Error encountered.	*/
	{
		sdr_cancel_xn(bpSdr);
		return -1;
	}

	if (sdr_end_xn(bpSdr) < 0)
	{
		putErrmsg("ARC4 encrypt/decrypt failed.", NULL);
		return -1;
	}

	return 0;
}

int	bcb_arc4_encrypt(Bundle *bundle, ExtensionBlock *blk,
		BspOutboundBlock *asb)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*sessionKey;
	unsigned int	sessionKeyLen;
	unsigned char	*keyValue;	/*	Key from rule base.	*/
	int		keyLen;		/*	Key from rule base.	*/
	unsigned char	*encryptedSessionKey;
	arc4_context	arcContext;
	Sdnv		sessionKeySdnv;
	int		resultsLength;
	unsigned char	*temp;

	/*	First, generate a clear text ARC4 session key and
	 *	use it to encrypt the BCB's target block.		*/

	sessionKey = generateSessionKey(&sessionKeyLen);
	CHKERR(sessionKey);
	switch (asb->targetBlockType)
	{
	case BLOCK_TYPE_PAYLOAD:
		if (cryptPayload(bundle, sessionKey, sessionKeyLen, "en") < 0)
		{
			BCB_DEBUG_ERR("x bcb_arc4_encrypt: Can't encrypt \
payload.", NULL);
			BCB_DEBUG_PROC("- bcb_arc4_encrypt--> NULL", NULL);
			MRELEASE(sessionKey);
			return 0;
		}

		break;

	default:
		BCB_DEBUG_ERR("x bcb_arc4_encrypt: Can't encrypt block type \
%d: canonicalization not implemented.", asb->targetBlockType);
		BCB_DEBUG_PROC("- bcb_arc4_encrypt--> NULL", NULL);
		MRELEASE(sessionKey);
		return 0;
	}

	/*	Now finish attaching the BCB.  Start by retrieving
	 *	the pre-placed shared secret key cited in the BCB
	 *	rule and using that key to encrypt the session key.	*/

	keyValue = bsp_retrieveKey(&keyLen, asb->keyName);
	arc4_setup(&arcContext, keyValue, keyLen); 
	encryptedSessionKey = MTAKE(sessionKeyLen); 
	CHKERR(encryptedSessionKey);
	arc4_crypt(&arcContext, sessionKeyLen, sessionKey, encryptedSessionKey);
	MRELEASE(keyValue);
	MRELEASE(sessionKey);

	/*	Finally, place the encrypted session key in the
	 *	BCB's results field.					*/

	encodeSdnv(&sessionKeySdnv, sessionKeyLen); 
	resultsLength = 1 + sessionKeySdnv.length + sessionKeyLen;
	temp = (unsigned char *) MTAKE(resultsLength);
	CHKERR(temp);
	*temp = BSP_CSPARM_KEY_INFO;
	memcpy(temp + 1, sessionKeySdnv.text, sessionKeySdnv.length);
	memcpy(temp + 1 + sessionKeySdnv.length, encryptedSessionKey,
			sessionKeyLen);
	MRELEASE(encryptedSessionKey);
	asb->resultsLen = resultsLength;
	asb->resultsData = sdr_malloc(bpSdr, resultsLength);
	if (asb->resultsData == 0)
	{
		BCB_DEBUG_ERR("x bcb_arc4_encrypt: Can't allocate heap \
space for ASB result, len %ld.", resultsLength);
		MRELEASE(temp);
		BCB_DEBUG_PROC("- bcb_arc4_encrypt --> %d", -1);
		return -1;
	}

	sdr_write(bpSdr, asb->resultsData, (char *) temp, resultsLength);
	MRELEASE(temp);

	/*	BCB is now ready to be serialized.			*/

	return 1;
}

int	bcb_arc4_decrypt(AcqWorkArea *wk, AcqExtBlock *blk)
{
	BspInboundBlock	*asb;
	unsigned char	*encryptedSessionKey;
	unsigned char	*keyValue;	/*	Key from rule base.	*/
	int		keyLen;		/*	Key from rule base.	*/
	unsigned char	*sessionKey;
	unsigned int	sessionKeyLen;
	arc4_context	arcContext;

	asb = (BspInboundBlock *) (blk->object);

	/*	First, retrieve the encrypted session key from the
	 *	results field of the inbound BCB.  Retrieve the
	 *	pre-placed shared secret key cited in the BCB rule
	 *	and use that key to decrypt the session key.		*/

	bsp_getInboundBspItem(BSP_CSPARM_KEY_INFO, asb->resultsData,
			asb->resultsLen, &encryptedSessionKey, &sessionKeyLen);
	if (sessionKeyLen != BSP_BCB_SESSION_KEY_LENGTH + 1)
	{
		BCB_DEBUG_ERR("x bcb_arc4_decrypt: Wrong length session key \
				in BCB: %d.", sessionKeyLen);
		BCB_DEBUG_PROC("- bcb_arc4_decrypt--> NULL", NULL);
		return 0;
	}

	keyValue = bsp_retrieveKey(&keyLen, asb->keyName);
	arc4_setup(&arcContext, keyValue, keyLen);
	sessionKey = MTAKE(sessionKeyLen);
	CHKERR(sessionKey);
	arc4_crypt(&arcContext, sessionKeyLen, encryptedSessionKey, sessionKey);
	MRELEASE(keyValue);

	/*	Now use the clear text session key to decrypt the
	 *	BCB's target block.					*/

	switch (asb->targetBlockType)
	{
	case BLOCK_TYPE_PAYLOAD:
		if (cryptPayload(&(wk->bundle), sessionKey, sessionKeyLen, "de")
				< 0)
		{
			BCB_DEBUG_ERR("x bcb_arc4_decrypt: Can't decrypt \
payload.", NULL);
			BCB_DEBUG_PROC("- bcb_arc4_decrypt--> NULL", NULL);
			MRELEASE(sessionKey);
			return 0;
		}

		break;

	default:
		BCB_DEBUG_ERR("x bcb_arc4_decrypt: Can't decrypt block type \
%d: canonicalization not implemented.", asb->targetBlockType);
		BCB_DEBUG_PROC("- bcb_arc4_decrypt--> NULL", NULL);
		MRELEASE(sessionKey);
		return 0;
	}

	MRELEASE(sessionKey);
	return 1;
}
