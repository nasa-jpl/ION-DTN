/*
 *	bib_hmac_sha256.c:	implementation of the BIB_HMAC_SHA256
 *				ciphersuite.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bib_hmac_sha256.h"
#include "crypto.h"

#define BSP_BIB_BLOCKING_SIZE		4096
#define BIB_HMAC_SHA256_RESULT_LEN	32

int	bib_hmac_sha256_construct(ExtensionBlock *blk, BspOutboundBlock *asb)
{
	asb->ciphersuiteType = BSP_CTYPE_BIB_HMAC_SHA256;
	asb.ciphersuiteFlags = BSP_ASB_RES;

	/*	Result length 34 is the length of a
	 *	type/length/value triplet: length of result
	 *	information item type (1) plus length of the
	 *	length of the security result (the length of
	 *	an SDNV large enough to contain the length of
 	*	the result, i.e., 1) plus the length of the
	*	result itself (BIB_HMAC_SHA256_RESULT_LEN = 32).	*/

	asb->resultsLength = 34; 
	asb->parmsLength = 0;
	asb->parmsData = 0;
	asb->resultsData = 0;
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: computePayloadDigest
 *
 * \par Purpose: Calculates HMAC SHA256 digest for a payload block given a
 * 		 key value.
 *
 * \retval unsigned char *	- The security result.
 *
 * \param[in]  dataObj		- The payload object to hash.
 * \param[in]  keyValue		- Value of the key to use.
 * \param[in]  keyLen		- Length of the key to use.
 * \param[out] hashLen		- Length of the computed digest.
 *
 * \par Notes:
 *****************************************************************************/

static unsigned char	*computePayloadDigest(Object dataObj, char *keyValue,
				unsigned int keyLen, unsigned int *hashLen)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*hashData = NULL;
	char		*dataBuffer;
	ZcoReader	dataReader;
	char		*authContext;
	int		authCtxLen = 0;
	unsigned int	bytesRemaining = 0;
	unsigned int	chunkSize = BSP_BIB_BLOCKING_SIZE;
	unsigned int	bytesRetrieved = 0;

	*hashLen = 0;		/*	Default, indicating failure.	*/

	/*	Allocate a data buffer.					*/

	dataBuffer = MTAKE(BSP_BIB_BLOCKING_SIZE);
	CHKNULL(dataBuffer);

	/*	Grab a context for the hmac. A local context allows
	 *	re-entrant calls to the HMAC libraries.			*/

	if ((authCtxLen = hmac_sha256_context_length()) <= 0)
	{
		BIB_DEBUG_ERR("x computeDigest: Bad context length (%d)",
				authCtxLen);
		MRELEASE(dataBuffer);
		BIB_DEBUG_PROC("- computeDigest--> NULL", NULL);
		return NULL;
	}

	BIB_DEBUG_INFO("i computeDigest: context length is %d", authCtxLen);

	if ((authContext = MTAKE(authCtxLen)) == NULL)
	{
		BIB_DEBUG_ERR("x computeDigest: Can't allocate %ld bytes",
				authCtxLen);
		MRELEASE(dataBuffer);
		BIB_DEBUG_PROC("- computeDigest--> NULL", NULL);
		return NULL;
	}

	/*	Initialize the digest computation.			*/

	hmac_sha256_init(authContext,(unsigned char *) keyValue, keyLen);
	bytesRemaining = zco_length(bpSdr, dataObj);
	CHKNULL(sdr_begin_xn(bpSdr));
	zco_start_transmitting(dataObj, &dataReader);

	BIB_DEBUG_INFO("i computeDigest: size is %d", bytesRemaining);
	BIB_DEBUG_INFO("i computeDigest: Key value = %s, key length = %d",
			keyValue, keyLen);

	while (bytesRemaining > 0)
	{
		if (bytesRemaining < chunkSize)
		{
			chunkSize = bytesRemaining;
		}

		bytesRetrieved = zco_transmit(bpSdr, &dataReader, chunkSize,
				dataBuffer);
		if (bytesRetrieved != chunkSize)
		{
			BIB_DEBUG_ERR("x computeDigest: Read %d bytes, but \
expected %d.", bytesRetrieved, chunkSize);
			MRELEASE(authContext);
			sdr_exit_xn(bpSdr);
			MRELEASE(dataBuffer);
			BIB_DEBUG_PROC("- computeDigest--> NULL", NULL);
			return NULL;
		}

		/*	Add the data to the hmac_sha1 context.		*/

		hmac_sha256_update(authContext, (unsigned char *)dataBuffer,
				chunkSize);
		bytesRemaining -= bytesRetrieved;
	}

	sdr_exit_xn(bpSdr);
	MRELEASE(dataBuffer);

	/*	Allocate a buffer for the hash result.			*/

	if ((hashData = MTAKE(BIB_HMAC_SHA256_RESULT_LEN)) == NULL)
	{
		putErrmsg("Failed allocating buffer for hash result.", NULL);
		MRELEASE(authContext);
		BIB_DEBUG_PROC("- computeDigest--> NULL", NULL);
		return NULL;
	}

	/*	Initialize the buffer to all zeros in case the
	 *	hmac_sha1_final function doesn't fill up the buffer.	*/

	memset(hashData, 0, BIB_HMAC_SHA256_RESULT_LEN);

	BIB_DEBUG_INFO("i computeDigest: allocated hash data.",NULL);

	/*	Calculate the hash result.				*/

	hmac_sha256_final(authContext, hashData, BIB_HMAC_SHA256_RESULT_LEN);
	hmac_sha256_reset(authContext);
	MRELEASE(authContext);
	*hashLen = BIB_HMAC_SHA256_RESULT_LEN;

	BIB_DEBUG_PROC("- computeDigest(%x)", (unsigned long) hashData);

	return (unsigned char *) hashData;
}

int	bib_hmac_sha256_sign(Bundle *bundle, ExtensionBlock *blk,
		BspOutboundBlock *asb)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*keyValue;
	int		keyLen;
	unsigned char	*digest;
	unsigned int	digestLen;
	int		outcome;
	Sdnv		digestSdnv;
	int		resultsLength;
	unsigned char	*temp;

	keyValue = bsp_retrieveKey(&keyLen, asb->keyName);
	switch (asb->targetBlockType)
	{
	case BLOCK_TYPE_PRIMARY:
		BIB_DEBUG_ERR("x bib_hmac_sha256_sign: Can't compute digest \
for primary block: canonicalization not implemented.", NULL);
		BIB_DEBUG_PROC("- bib_hmac_sha256_sign--> NULL", NULL);
		MRELEASE(keyValue);
		return 0;

	case BLOCK_TYPE_PAYLOAD:
		digest = computePayloadDigest(bundle->payload.content,
				keyValue, keyLen, &digestLen);
		break;

	default:
		BIB_DEBUG_ERR("x bib_hmac_sha256_sign: Can't compute digest \
for block type %d: canonicalization not implemented.", asb->targetBlockType);
		BIB_DEBUG_PROC("- bib_hmac_sha256_sign--> NULL", NULL);
		MRELEASE(keyValue);
		return 0;
	}

	MRELEASE(keyValue);
	if (digestLen != BIB_HMAC_SHA256_RESULT_LEN)   
	{
		if (digest != NULL)
		{
			MRELEASE(digest);
		}

		BAB_DEBUG_ERR("x bib_hmac_sha256_sign: Bad hash. hashData \
is 0x%x and length is %d.", digest, digestLen);
		BAB_DEBUG_PROC("- bib_hmac_sha256_sign--> 0", NULL);
		return 0;
	}

	encodeSdnv(&digestSdnv, digestLen); 
	resultsLength = 1 + digestSdnv.length + digestLen;
	temp = (unsigned char *) MTAKE(resultsLength);
	if (temp == NULL)
	{
		BAB_DEBUG_ERR("x bib_hmac_sha256_sign: Can't allocate \
memory for ASB result, len %ld.", resultsLength);
		MRELEASE(digest);
		BAB_DEBUG_PROC("- bib_hmac_sha256_sign --> %d", -1);
		return -1;
	}

	*temp = BSP_CSPARM_INT_SIG;
	memcpy(temp + 1, digestSdnv.text, digestSdnv.length);
	memcpy(temp + 1 + digestSdnv.length, digest, digestLen);
	MRELEASE(digest);
	asb.resultsLen = resultsLength;
	asb.resultsData = sdr_malloc(bpSdr, resultsLength);
	if (asb.resultsData == 0)
	{
		BAB_DEBUG_ERR("x bib_hmac_sha256_sign: Can't allocate heap \
space for ASB result, len %ld.", resultsLength);
		MRELEASE(temp);
		BAB_DEBUG_PROC("- bib_hmac_sha256_sign --> %d", -1);
		return -1;
	}

	sdr_write(bpSdr, asb.resultsData, temp, resultsLength);
	MRELEASE(temp);
	return 0;
}

int	bib_hmac_sha256_verify(AcqWorkArea *wk, AcqExtBlock *blk)
{
	Sdr		sdr = getIonsdr();
	BspInboundBlock	*asb;
	unsigned char	*keyValue;
	Object		payloadObj;
	int		keyLen;
	unsigned char	*digest;
	unsigned int	digestLen;
	unsigned char	*assertedDigest;
	unsigned int	assertedDigestLen;
	int		outcome;

	asb = (BspInboundBlock *) (blk->object);
	keyValue = bsp_retrieveKey(&keyLen, asb->keyName);
	switch (asb->targetBlockType)
	{
	case BLOCK_TYPE_PRIMARY:
		BIB_DEBUG_ERR("x bib_hmac_sha256_sign: Can't compute digest \
for primary block: canonicalization not implemented.", NULL);
		BIB_DEBUG_PROC("- bib_hmac_sha256_sign--> NULL", NULL);
		MRELEASE(keyValue);
		return 0;

	case BLOCK_TYPE_PAYLOAD:
		/*	Currently the wk->bundle.payload.content ZCO
		 *	is the entire serialized bundle.  Snap off
		 *	a clone of just the portion of that serialized
		 *	bundle that is the payload content.		*/

		CHKERR(sdr_begin_xn(sdr));
		payloadObj = zco_clone(sdr, wk->bundle.payload.content,
				wk->headerLength, wk->bundle.payload.length);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Transaction failed.", NULL);
			return -1;
		}

		/*	Now compute the digest for just the payload,
		 *	then discard the temporary payload content ZCO.	*/

		digest = computePayloadDigest(payloadObj, keyValue, keyLen,
				&digestLen);
		CHKERR(sdr_begin_xn(sdr));
		zco_destroy(sdr, payloadObj);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Transaction failed.", NULL);
			return -1;
		}

		break;

	default:
		BIB_DEBUG_ERR("x bib_hmac_sha256_sign: Can't compute digest \
for block type %d: canonicalization not implemented.", asb->targetBlockType);
		BIB_DEBUG_PROC("- bib_hmac_sha256_sign--> NULL", NULL);
		MRELEASE(keyValue);
		return 0;
	}

	MRELEASE(keyValue);
	if (digestLen != BIB_HMAC_SHA256_RESULT_LEN)   
	{
		if (digest != NULL)
		{
			MRELEASE(digest);
		}

		BAB_DEBUG_ERR("x bib_hmac_sha256_sign: Bad hash. hashData is \
0x%x and length is %d.", digest, digestLen);
		BAB_DEBUG_PROC("- bib_hmac_sha256_sign--> 0", NULL);
		return -1;
	}

	getInboundBspItem(BSP_CSPARM_INT_SIG, asb->resultsData,
			asb->resultsLen, &assertedDigest, &assertedDigestLen);
	if (assertedDigestLen != BIB_HMAC_SHA256_RESULT_LEN)
	{
		BAB_DEBUG_ERR("x bib_hmac_sha256_verify: Wrong length digest \
in BIB: %d.", assertedDigestLen);
		MRELEASE(digest);
		return 0;
	}

	/*	Compare the digests to verify bundle authenticity.	*/

	if (memcmp(digest, assertedDigest, BIB_HMAC_SHA256_RESULT_LEN) == 0)
	{
		outcome = 1;
	}
	else
	{
		BAB_DEBUG_WARN("x bsp_hmac_verify: digests don't match.");
		outcome = 0;
	}

	MRELEASE(digest);
	return outcome;
}
