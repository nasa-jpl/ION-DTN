/*
 *	bab_hmac_sha1.c:	implementation of the BAB_HMAC_SHA1
 *				ciphersuite.
 *
 *	Copyright (c) 2014, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "bab_hmac_sha1.h"
#include "crypto.h"

#if (BAB_DEBUGGING == 1)
extern char	gMsg[];			/*	Debug message buffer.	*/
#endif

#define BSP_BAB_BLOCKING_SIZE		4096
#define BAB_HMAC_SHA1_RESULT_LEN	20
#define BAB_HMAC_SHA1_ASB_RESULTS_LEN	(BAB_HMAC_SHA1_RESULT_LEN + 2)
	/*	ASB results field length 22 is the length of a
	 *	type/length/value triplet: length of result information
	 *	item type (1) plus length of the length of the security
	 *	result (the length of an SDNV large enough to contain
	 *	the length of the digest, i.e., 1) plus the length of
	 *	the security result itself.				*/

int	bab_hmac_sha1_construct(ExtensionBlock *blk, BspOutboundBlock *asb)
{
	asb->ciphersuiteType = BSP_CSTYPE_BAB_HMAC_SHA1;
	asb->parmsLen = 0;
	asb->parmsData = 0;
	if (blk->occurrence == 0)
	{
		asb->ciphersuiteFlags = 0;
	}
	else
	{
		asb->ciphersuiteFlags = BSP_ASB_RES;
		asb->resultsLen = BAB_HMAC_SHA1_ASB_RESULTS_LEN; 
	}

	asb->resultsData = 0;
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: computeDigest
 *
 * \par Purpose: Calculates HMAC SHA1 digest for a bundle given a key value.
 *
 * \retval unsigned char * - The security result.
 *
 * \param[in]  dataObj     - The serialized bundle to hash, a ZCO.
 * \param[in]  keyValue    - Value of the key to use.
 * \param[in]  keyLen      - Length of the key to use.
 * \param[out] hashLen     - Length of the computed digest.
 *
 * \par Notes:
 *      1. 
 *****************************************************************************/

static unsigned char	*computeDigest(Object dataObj, unsigned char *keyValue,
				unsigned int keyLen, unsigned int *hashLen)
{
	Sdr		bpSdr = getIonsdr();
	unsigned char	*hashData = NULL;
	char		*dataBuffer;
	ZcoReader	dataReader;
	char		*authContext;
	int		authCtxLen;
	unsigned int	bytesRemaining;
	unsigned int	chunkSize = BSP_BAB_BLOCKING_SIZE;
	unsigned int	bytesRetrieved = 0;
	unsigned int	asbResultsLength = BAB_HMAC_SHA1_ASB_RESULTS_LEN;

	BAB_DEBUG_INFO("+ computeDigest(0x%x, '%s', %d, 0x%x)",
			(unsigned long) dataObj, keyValue, keyLen,
			(unsigned long) hashLen);

	CHKNULL(keyValue);
	CHKNULL(hashLen);
	*hashLen = 0;		/*	Default, indicating failure.	*/

	/*	Allocate a data buffer.					*/

	dataBuffer = MTAKE(BSP_BAB_BLOCKING_SIZE);
	CHKNULL(dataBuffer);

	/*	Grab a context for the hmac. A local context allows
	 *	re-entrant calls to the HMAC libraries.			*/

	if ((authCtxLen = hmac_sha1_context_length()) <= 0)
	{
		BAB_DEBUG_ERR("x computeDigest: Bad context length (%d)",
				authCtxLen);
		MRELEASE(dataBuffer);
		BAB_DEBUG_PROC("- computeDigest--> NULL", NULL);
		return NULL;
	}

	BAB_DEBUG_INFO("i computeDigest: context length is %d", authCtxLen);

	if ((authContext = MTAKE(authCtxLen)) == NULL)
	{
		putErrmsg("Failed allocating buffer for HMAC context.", NULL);
		MRELEASE(dataBuffer);
		BAB_DEBUG_PROC("- computeDigest--> NULL", NULL);
		return NULL;
	}

	/*	Initialize the digest computation.  Compute over
	 *	entire bundle except the results of the digest
	 *	computation.						*/

	hmac_sha1_init(authContext, (unsigned char *) keyValue, keyLen);
	bytesRemaining = zco_length(bpSdr, dataObj) - asbResultsLength;
	CHKNULL(sdr_begin_xn(bpSdr));
	zco_start_transmitting(dataObj, &dataReader);

	BAB_DEBUG_INFO("i computeDigest: bundle size is %d", bytesRemaining);
	BAB_DEBUG_INFO("i computeDigest: Key value = %s, key length = %d",
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
			BAB_DEBUG_ERR("x computeDigest: Read %d bytes, but \
expected %d.", bytesRetrieved, chunkSize);
			MRELEASE(authContext);
			sdr_exit_xn(bpSdr);
			MRELEASE(dataBuffer);
			BAB_DEBUG_PROC("- computeDigest--> NULL", NULL);
			return NULL;
		}

		/*	Add the data to the hmac_sha1 context.		*/

		hmac_sha1_update(authContext, (unsigned char *) dataBuffer,
				chunkSize);
		bytesRemaining -= bytesRetrieved;
	}

	sdr_exit_xn(bpSdr);
	MRELEASE(dataBuffer);

	/*	Allocate a buffer for the hash result.			*/

	if ((hashData = MTAKE(BAB_HMAC_SHA1_RESULT_LEN)) == NULL)
	{
		putErrmsg("Failed allocating buffer for hash result.", NULL);
		MRELEASE(authContext);
		BAB_DEBUG_PROC("- computeDigest--> NULL", NULL);
		return NULL;
	}

	BAB_DEBUG_INFO("i computeDigest: allocated hash data.", NULL);

	/*	Calculate the hash result.				*/

	hmac_sha1_final(authContext, hashData, BAB_HMAC_SHA1_RESULT_LEN);
	hmac_sha1_reset(authContext);
	MRELEASE(authContext);
	*hashLen = BAB_HMAC_SHA1_RESULT_LEN;

	BAB_DEBUG_PROC("- computeDigest(%x)", (unsigned long) hashData);

	return (unsigned char *) hashData;
}

int	bab_hmac_sha1_sign(Bundle *bundle, ExtensionBlock *blk,
		BspOutboundBlock *asb)
{
	Sdr		bpSdr = getIonsdr();
	int		result = 0;
	unsigned char	*keyValue;
	int		keyLen;
	unsigned char	*digest;
	unsigned int	digestLen;
	Sdnv		digestSdnv;
	Object		trailerAddr;
	vast		trailerLength;
	Object		babAddr;
	vast		babLength;
	Object		results;
	unsigned char	asbType;

	BAB_DEBUG_INFO("+ bab_hmac_sha1_sign", NULL);

	if (blk->occurrence == 0)	/*	First BAB.		*/
	{
		/*	For this BAB ciphersuite, only the last BAB
		 *	block participates in signing the bundle.	*/

		BAB_DEBUG_PROC("- bab_hmac_sha1_sign--> %d", result);
		return result;
	}

	/*	For this BAB ciphersuite, the last BAB is required
	 *	to be the last block of the bundle.  That block has
	 *	previously been serialized with correct security
	 *	result length but only filler in result data, and
	 *	the last bytes of the final serialized bundle are
	 *	the bytes of that serialized last BAB.  Since
	 *	the block's security result data is the last field
	 *	of the last BAB (the last block of the bundle), we
	 *	can insert the security result TLV into the serialized
	 *	bundle by simply overwriting those last bytes.		*/

	keyValue = bsp_retrieveKey(&keyLen, asb->keyName);
	digest = computeDigest(bundle->payload.content, keyValue, keyLen,
			&digestLen);
	MRELEASE(keyValue);
	if (digestLen != BAB_HMAC_SHA1_RESULT_LEN)   
	{
		if (digest != NULL)
		{
			MRELEASE(digest);
		}

		BAB_DEBUG_ERR("x bab_hmac_sha1_sign: Bad hash. hashData is \
0x%x and length is %d.", digest, digestLen);
		BAB_DEBUG_PROC("- bab_hmac_sha1_sign--> %d", result);
		return result;
	}

	encodeSdnv(&digestSdnv, digestLen); 

	/*	At this point the bundle has been serialized in the
	 *	payload ZCO, with all pre-payload blocks concatenated
	 *	into a single ZCO header and all post-payload blocks
	 *	concatenated into a single ZCO trailer.  So we need
	 *	to overwrite the final 22 bytes of that trailer.	*/

	trailerAddr = zco_trailer_text(bpSdr, bundle->payload.content, 0,
			&trailerLength);
	sdr_stage(bpSdr, NULL, trailerAddr, 0);

	/*	NOTE (TODO): this function MUST be generalized to
	 *	skip over all post-payload extension blocks that
	 *	precede the final BAB in the ZCO trailer.  But at
	 *	this time ION supports no post-payload extension
	 *	blocks other than the final BAB, so this is left to
	 *	a future update.					*/

	babAddr = trailerAddr;
	babLength = trailerLength;

	/*	The BAB's results data field is the last 22 bytes of
	 *	the serialized BAB.					*/

	results = (babAddr + babLength) - BAB_HMAC_SHA1_ASB_RESULTS_LEN; 
	asbType = BSP_CSPARM_INT_SIG;
	sdr_write(bpSdr, results, (char *) &asbType, 1); 
	sdr_write(bpSdr, results + 1, (char *) digestSdnv.text,
			digestSdnv.length);
	sdr_write(bpSdr, results + 1 + digestSdnv.length, (char *) digest,
			digestLen);
	MRELEASE(digest);
	BAB_DEBUG_PROC("- bab_hmac_sha1_sign--> %d", 1);
	return result;
}

int	bab_hmac_sha1_verify(AcqWorkArea *wk, AcqExtBlock *blk)
{
	int		outcome = 0;	/*	Default: fails.		*/
	BspInboundBlock	*asb;
	LystElt		elt;
	AcqExtBlock	*otherBab;
	unsigned char	*keyValue;
	int		keyLen;
	unsigned char	*digest;
	unsigned int	digestLen;
	Sdnv		digestSdnv;
	int		resultsLen;
	unsigned char	*temp;
	BspInboundBlock	*firstAsb;
	unsigned char	*assertedDigest;
	unsigned int	assertedDigestLen;

	asb = (BspInboundBlock *) (blk->object);
	if (asb->instance == 0)
	{
		elt = findAcqExtensionBlock(wk, EXTENSION_TYPE_BAB, 1);
		if (elt == NULL)
		{
			BAB_DEBUG_ERR("x bab_hmac_sha1_verify: no Last BAB.",
					NULL);
			discardExtensionBlock(blk);
			return 0;
		}

		/*	We can now compute the expected digest for
		 *	this bundle and simply stash it in this BAB's
		 *	resultsData for use when Last BAB is checked.	*/

		keyValue = bsp_retrieveKey(&keyLen, asb->keyName);
		digest = computeDigest(wk->rawBundle, keyValue, keyLen,
				&digestLen);
		MRELEASE(keyValue);
		if (digestLen != BAB_HMAC_SHA1_RESULT_LEN)   
		{
			if (digest != NULL)
			{
				MRELEASE(digest);
			}

			BAB_DEBUG_ERR("x bab_hmac_sha1_verify: Bad hash. \
hashData is 0x%x and length is %d.", digest, digestLen);
			discardExtensionBlock(blk);
			return 0;
		}

		/*	Use the resultsData field of the First BAB's
		 *	scratchpad to store the computed digest.
		 *	(Note that, for this ciphersuite, it is not
		 *	used for any other purpose.)			*/

		encodeSdnv(&digestSdnv, digestLen); 
		resultsLen = 1 + digestSdnv.length + digestLen;
		temp = (unsigned char *) MTAKE(resultsLen);
		if (temp == NULL)
		{
			BAB_DEBUG_ERR("x bab_hmac_sha1_verify: Can't allocate \
memory for ASB result, len %ld.", resultsLen);
			MRELEASE(digest);
			discardExtensionBlock(blk);
			return -1;
		}

		*temp = BSP_CSPARM_INT_SIG;
		memcpy(temp + 1, digestSdnv.text, digestSdnv.length);
		memcpy(temp + 1 + digestSdnv.length, digest, digestLen);
		MRELEASE(digest);
		if (asb->resultsData)	/*	Should be none.		*/
		{
			MRELEASE(asb->resultsData);
		}

		asb->resultsLen = resultsLen;
		asb->resultsData = temp;
		BAB_DEBUG_INFO("i bab_hmac_sha1_verify: computed signature \
of length %d has been stored in first BAB.", digestLen);
		return 1;
	}

	/*	This is the last BAB; check its security result.	*/

	if ((asb->ciphersuiteFlags & BSP_ASB_RES) == 0)
	{
		BAB_DEBUG_ERR("x bab_hmac_sha1_verify: No security result in \
last BAB.", NULL);
		discardExtensionBlock(blk);
		return 0;
	}

	bsp_getInboundBspItem(BSP_CSPARM_INT_SIG, asb->resultsData,
			asb->resultsLen, &assertedDigest, &assertedDigestLen);
	if (assertedDigestLen != BAB_HMAC_SHA1_RESULT_LEN)
	{
		BAB_DEBUG_ERR("x bab_hmac_sha1_verify: Wrong length digest in \
last BAB: %d.", assertedDigestLen);
		discardExtensionBlock(blk);
		return 0;
	}

	/*	Asserted digest seems okay.  Retrieve computed digest.	*/

	elt = findAcqExtensionBlock(wk, EXTENSION_TYPE_BAB, 0);
	if (elt == NULL)
	{
		BAB_DEBUG_ERR("x bsp_hmac_sha1_verify: no First BAB.", NULL);
		discardExtensionBlock(blk);
		return 0;
	}

	otherBab = (AcqExtBlock *) lyst_data(elt);
	firstAsb = (BspInboundBlock *) (otherBab->object);
	bsp_getInboundBspItem(BSP_CSPARM_INT_SIG, firstAsb->resultsData,
			firstAsb->resultsLen, &digest, &digestLen);

	/*	Compare the digests to verify bundle authenticity.	*/

	if (memcmp(digest, assertedDigest, BAB_HMAC_SHA1_RESULT_LEN) == 0)
	{
		outcome = 3;
	}
	else
	{
		BAB_DEBUG_ERR("x bsp_hmac_sha1_verify: digests don't match.",
				NULL);
		outcome = 2;
	}

	/*	Now delete both the First and Last BABs and return.	*/

	deleteAcqExtBlock(elt);
	discardExtensionBlock(blk);
	return outcome;
}
