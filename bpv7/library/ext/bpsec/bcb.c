/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: bcb.c
 **
 ** Description: Definitions supporting generic processing of BCB blocks.
 **              This includes both the BCB Interface to the ION bpsec
 **              API as well as a default implementation of the BCB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BCB bpsec Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              bpsec_encrypt
 **              bcbSerialize
 **              bcbRelease
 **              bcbCopy
 **                                                  bpsec_decrypt
 **                                                  bcbAcquire
 **                                                  bcbRecord
 **                                                  bcbClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    ENCRYPT SIDE                     DECRYPT SIDE
 **
 **              bcbDefaultEncrypt
 **
 **                                              bcbDefaultDecrypt
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/05/10  E. Birrane     Implementation as extsbspbcb.c (JHU/APL)
 **            S. Burleigh    Implementation as sbspbcb.c for Sbsp
 **  11/07/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 **  09/02/19  S. Burleigh    Rename everything for bpsec
 **  10/14/20  S. Burleigh    Restructure for target multiplicity
 *****************************************************************************/

#include "zco.h"
#include "csi.h"
#include "bpsec_util.h"
#include "bcb.h"
#include "bpsec_instr.h"

extern int	bibAttach(Bundle *bundle, ExtensionBlock *bibBlk,
			BpsecOutboundBlock *bibAsb);
extern LystElt	bibFindInboundTarget(AcqWorkArea *work, int blockNumber,
			LystElt *bibElt);

#if (BCB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/*****************************************************************************
 *                         BCB COMPUTATION FUNCTIONS                         *
 *****************************************************************************/

Object	bcbStoreOverflow(uint32_t suite, uint8_t *context,
		ZcoReader *dataReader, uvast readOffset, uvast writeOffset,
		uvast cipherBufLen, uvast cipherOverflow,
		sci_inbound_tlv plaintext, sci_inbound_tlv ciphertext,
		csi_blocksize_t *blocksize)
{
	uvast	chunkSize = 0;
	Sdr	sdr = getIonsdr();
	Object	cipherBuffer = 0;

	/*	Step 4: Create SDR space and store any extra encryption
	 *	that won't fit in the payload.				*/

	ciphertext.length = 0;
	ciphertext.value = NULL;
	if ((readOffset < blocksize->plaintextLen) || (cipherOverflow > 0))
	{
		Object	cipherBuffer = 0;
		uvast	length = cipherBufLen - blocksize->plaintextLen;

		writeOffset = 0;
		if ((cipherBuffer = sdr_malloc(sdr, length * 2)) == 0)
		{
			/*	Something happens here?			*/
		}

		if (cipherOverflow > 0)
		{
			sdr_write(sdr, cipherBuffer + writeOffset,
					(char *) ciphertext.value
						+ ciphertext.length
						- cipherOverflow,
					cipherOverflow);
			writeOffset += cipherOverflow;
		}

		while (readOffset < blocksize->plaintextLen)
		{
			if ((readOffset + chunkSize) < blocksize->plaintextLen)
			{
				plaintext.length = zco_transmit(sdr,
						dataReader, chunkSize,
						(char *) plaintext.value);
			}
			else
			{
				plaintext.length = zco_transmit(sdr, dataReader,
						blocksize->plaintextLen
							- readOffset,
						(char*) plaintext.value);
			}

			readOffset += plaintext.length;
			ciphertext = sci_crypt_update(suite, context,
					CSI_SVC_ENCRYPT, plaintext);

			if ((ciphertext.value == NULL)
			|| (ciphertext.length == 0))
			{
				BCB_DEBUG_ERR("x bcbCiphertextToSdr: Could \
not encrypt.", plaintext.length, chunkSize);
				BCB_DEBUG_PROC("- bcbCiphertextToSdr--> -1",
						NULL);
				return 0;
			}

			sdr_write(sdr, cipherBuffer + writeOffset,
					(char *) ciphertext.value,
					ciphertext.length);
			writeOffset += ciphertext.length;
			MRELEASE(ciphertext.value);
		}
	}

	return cipherBuffer;
}

/*
 * 		 Step 3.2 - Write ciphertext to the payload. We
 * 		 assume the ciphertext length will never be more
 * 		 than 2x the payload length. If the ciphertext is
 * 		 beyond what can be stored in the existing payload
 * 		 allocation, capture it for later use in the BCB.
 */

int32_t	bcbUpdatePayloadInPlace(uint32_t suite, sci_inbound_parms parms,
		uint8_t	*context, csi_blocksize_t *blocksize, Object dataObj,
		ZcoReader *dataReader, uvast cipherBufLen, Object *cipherBuffer,
		uint8_t function)
{
	Sdr		sdr = getIonsdr();
	sci_inbound_tlv	plaintext[2];
	sci_inbound_tlv	ciphertext;
	uint8_t		cur_idx = 0;
	uvast		chunkSize = 0;

	uvast		readOffset = 0;
	uvast		writeOffset = 0;
	uvast		cipherOverflow = 0;

	CHKERR(context);
	CHKERR(blocksize);
	CHKERR(cipherBuffer);

	*cipherBuffer = 0;
	chunkSize = blocksize->chunkSize;
	ciphertext.length = 0;
	ciphertext.value = NULL;

	/* Step 1: Allocate read buffers. */
	if ((plaintext[0].value = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadInPlace - Can't allocate buffer of size %d.", chunkSize);
		return -1;
	}

	if ((plaintext[1].value = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadInPlace - Can't allocate buffer of size %d.", chunkSize);
		MRELEASE(plaintext[0].value);
		return -1;
	}

	/* Step 2 - Perform priming read of payload to prep for encryption. */
	chunkSize = blocksize->chunkSize;

/* \todo: Check return value for zco_transmit */
	/* Step 2.1: Perform priming read. */
	if(blocksize->plaintextLen <= blocksize->chunkSize)
	{
		plaintext[0].length = zco_transmit(sdr, dataReader,
				blocksize->plaintextLen,
				(char *) plaintext[0].value);
		plaintext[1].length = 0;
		readOffset = plaintext[0].length;
		writeOffset = 0;
	}
	else if (blocksize->plaintextLen <= (2*(blocksize->chunkSize)))
	{
		plaintext[0].length = zco_transmit(sdr, dataReader, chunkSize,
				(char *) plaintext[0].value);
		plaintext[1].length = zco_transmit(sdr, dataReader,
				blocksize->plaintextLen - chunkSize,
				(char *) plaintext[1].value);
		readOffset = plaintext[0].length + plaintext[1].length;
		writeOffset = 0;
	}
	else
	{
		plaintext[0].length = zco_transmit(sdr, dataReader,
				chunkSize, (char *) plaintext[0].value);
		plaintext[1].length = zco_transmit(sdr, dataReader,
				chunkSize, (char *) plaintext[0].value);
		readOffset = plaintext[0].length + plaintext[1].length;
		writeOffset = 0;
	}

	/* Step 3: Walk through payload writing ciphertext. */

	if ((sci_crypt_start(suite, context, parms)) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadInPlace: Could not start \
context.", NULL);
		MRELEASE(plaintext[0].value);
		MRELEASE(plaintext[1].value);
		MRELEASE(ciphertext.value);
		return -1;
	}

 	while (writeOffset < blocksize->plaintextLen)
	{
 		/* Step 3.1: Generate ciphertext from earliest plaintext
		 * buffer. */

 		/* Step 3.1: If there is no data left to encrypt... */
 		if(plaintext[cur_idx].length == 0)
 		{
 			break;
 		}

 		ciphertext = sci_crypt_update(suite, context, function,
				plaintext[cur_idx]);
		if ((ciphertext.value == NULL) || (ciphertext.length == 0))
		{
			BCB_DEBUG_ERR("x bcbUpdatePayloadInPlace: Could not \
encrypt.", NULL);
			MRELEASE(plaintext[0].value);
			MRELEASE(plaintext[1].value);
			MRELEASE(ciphertext.value);
			BCB_DEBUG_PROC("- bcbUpdatePayloadInPlace--> -1", NULL);
			return -1;
		}

		/*
		 * Step 3.2: If the ciphertext will no longer fit into the
		 * existing payload, then just copy the bits of ciphertext
		 * that will fit and save the rest for later.
		 */
		if ((writeOffset + ciphertext.length) > blocksize->plaintextLen)
		{
			if ((zco_revise(sdr, dataObj, writeOffset,
					(char *) ciphertext.value,
					blocksize->plaintextLen - writeOffset))
				       	== -1)
			{
				BCB_DEBUG_ERR("bcbUpdatePayloadInPlace: \
Failed call to zco_revise.", NULL);
				break;
			}

			cipherOverflow = ciphertext.length -
					(blocksize->plaintextLen - writeOffset);
			writeOffset = blocksize->plaintextLen;
		}
		else
		{
			if ((zco_revise(sdr, dataObj, writeOffset,
					(char *) ciphertext.value,
					ciphertext.length)) == -1)
			{
				BCB_DEBUG_ERR("bcbUpdatePayloadInPlace: \
Failed call to zco_revise.", NULL);
				break;
			}

			writeOffset += ciphertext.length;

			/* Fill up the next read buffer */
			if (readOffset >= blocksize->plaintextLen)
			{
				plaintext[cur_idx].length = 0;
			}
			else if ((readOffset + chunkSize)
					< blocksize->plaintextLen)
			{
				plaintext[cur_idx].length = zco_transmit(sdr,
						dataReader, chunkSize, (char *)
						plaintext[cur_idx].value);
			}
			else
			{
				plaintext[cur_idx].length = zco_transmit(sdr,
						dataReader,
						blocksize->plaintextLen
						- readOffset, (char*)
						plaintext[cur_idx].value);
			}

			readOffset += plaintext[cur_idx].length;
			cur_idx = 1 - cur_idx;
		}

		MRELEASE(ciphertext.value);
	}

	/* Step 4: Create SDR space and store any extra encryption that
	 * won't fit in the payload. */
	*cipherBuffer = bcbStoreOverflow(suite, context, dataReader,
			readOffset, writeOffset, cipherBufLen, cipherOverflow,
			plaintext[0], ciphertext, blocksize);

	MRELEASE(plaintext[0].value);
	MRELEASE(plaintext[1].value);

	if ((sci_crypt_finish(suite, context, function, &parms)) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadInPlace: Could not finish \
CSI context.", NULL);
		if (ciphertext.value)
		{
			MRELEASE(ciphertext.value);
		}

		BCB_DEBUG_PROC("- bcbUpdatePayloadInPlace--> -1", NULL);
		return -1;
	}

	BCB_DEBUG_PROC("- bcbUpdatePayloadInPlace--> 0", NULL);

	return 0;
}

/**
 * > 0 Success
 * 0 BCB error
 * -1 System error
 */
int32_t bcbUpdatePayloadFromSdr(uint32_t suite, sci_inbound_parms parms,
		uint8_t *context, csi_blocksize_t *blocksize, Object dataObj,
		ZcoReader *dataReader, uvast cipherBufLen, Object *cipherZco,
		uint8_t function)
{
	Sdr		sdr = getIonsdr();
	sci_inbound_tlv	plaintext;
	sci_inbound_tlv	ciphertext;
	uvast		chunkSize = 0;
	uvast		bytesRemaining = 0;
	Object		cipherBuffer = 0;
	uvast		writeOffset = 0;
	SdrUsageSummary	summary;
	uvast		memmax = 0;

	CHKERR(context);
	CHKERR(blocksize);
	CHKERR(dataReader);
	CHKERR(cipherZco);

	BCB_DEBUG_PROC("+ bcbUpdatePayloadFromSdr(%d," ADDR_FIELDSPEC
			"," ADDR_FIELDSPEC "," UVAST_FIELDSPEC ","
			ADDR_FIELDSPEC "," UVAST_FIELDSPEC "," ADDR_FIELDSPEC
			", %d)", suite, (uaddr) context, (uaddr) blocksize,
			(uvast) dataObj, (uaddr) dataReader, cipherBufLen,
			(uaddr) cipherZco, function);

	/* Step 1 - Get information about the SDR storage space. */
	sdr_usage(sdr, &summary);

	// Set the maximum buffer len to 1/2 of the available space.
	// Note: >> 1 means divide by 2.
	memmax = (summary.largePoolFree + summary.unusedSize) >> (uvast) 1;

	/* Step 2 - See if ciphertext will fit into the existing SDR space. */
	if (cipherBufLen > memmax)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr: Buffer len will \
not fit. " UVAST_FIELDSPEC " > " UVAST_FIELDSPEC, cipherBufLen, memmax);
		sdr_report(&summary);
		BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> 0", NULL);
		return 0;
	}

	if ((cipherBuffer = sdr_malloc(sdr, cipherBufLen)) == 0)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr: Cannot allocate "
				UVAST_FIELDSPEC " from SDR.", NULL);
		BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);

		return -1;
	}

	/* Pass additive inverse of cipherBufLen to tell ZCO that space
	 * has already been allocated.
	 */
	if ((*cipherZco = zco_create(sdr, ZcoSdrSource, cipherBuffer, 0,
			0 - cipherBufLen, ZcoOutbound)) == 0)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr: Cannot create zco.",
				NULL);
		sdr_free(sdr, cipherBuffer);
		BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);

		return -1;
	}

	chunkSize = blocksize->chunkSize;
	bytesRemaining = blocksize->plaintextLen;

	/* Step 1: Allocate read buffers. */
	if ((plaintext.value = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr - Can't allocate \
buffer of size %d.", chunkSize);

		sdr_free(sdr, cipherBuffer);
		zco_destroy(sdr, *cipherZco);
		*cipherZco = 0;
		BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);

		return -1;
	}

	chunkSize = blocksize->chunkSize;

	/* Step 2 - Perform priming read of payload to prep for encryption. */
	if ((sci_crypt_start(suite, context, parms)) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr - Can't start \
context.", NULL);

		MRELEASE(plaintext.value);
		sdr_free(sdr, cipherBuffer);
		zco_destroy(sdr, *cipherZco);
		*cipherZco = 0;

		BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);
		return -1;
	}

	/* Step 3: Walk through payload writing ciphertext. */
 	while (bytesRemaining > 0)
	{
 		/* Step 3.1: Prep for this encrypting iteration. */
 		memset(plaintext.value, 0, chunkSize);
 		if (bytesRemaining < chunkSize)
 		{
 			chunkSize = bytesRemaining;
 		}

 		/* Step 3.2: Generate ciphertext from earliest plaintext
		 * buffer. */
 		plaintext.length = zco_transmit(sdr, dataReader, chunkSize,
				(char *) plaintext.value);
 		if (plaintext.length <= 0)
 		{
 			BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr - Can't \
do priming read of length %d.", chunkSize);

 			MRELEASE(plaintext.value);
 			sdr_free(sdr, cipherBuffer);
 			zco_destroy(sdr, *cipherZco);
 			*cipherZco = 0;

			BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);

 			return -1;
 		}

 		bytesRemaining -= plaintext.length;
 		ciphertext = sci_crypt_update(suite, context, function,
				plaintext);

		if ((ciphertext.value == NULL) || (ciphertext.length == 0))
		{
			BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr: Could not \
encrypt.", plaintext.length, chunkSize);
			MRELEASE(plaintext.value);
			MRELEASE(ciphertext.value);
			sdr_free(sdr, cipherBuffer);
			zco_destroy(sdr, *cipherZco);
			*cipherZco = 0;

			BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);
			return -1;
		}

		sdr_write(sdr, cipherBuffer + writeOffset,
				(char *) ciphertext.value, ciphertext.length);
		writeOffset += ciphertext.length;
		MRELEASE(ciphertext.value);
	}

	MRELEASE(plaintext.value);
	if (sci_crypt_finish(suite, context, function, &parms) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromSdr: Could not finish \
context.", NULL);
		sdr_free(sdr, cipherBuffer);
		zco_destroy(sdr, *cipherZco);
		*cipherZco = 0;

		BCB_DEBUG_PROC("- bcbUpdatePayloadFromSdr--> -1", NULL);
		return -1;
	}

	BCB_DEBUG_PROC("- bcbCiphertextToSdr--> 1", NULL);

	return 1;
}

/**
 * > 0 Success
 * 0 processing error
 * -1 system error
 */
int32_t bcbUpdatePayloadFromFile(uint32_t suite, sci_inbound_parms parms,
		uint8_t *context, csi_blocksize_t *blocksize, Object dataObj,
		ZcoReader *dataReader, uvast cipherBufLen, Object *cipherZco,
		uint8_t function)
{
	Sdr		sdr = getIonsdr();
	sci_inbound_tlv	plaintext;
	sci_inbound_tlv	ciphertext;
	uvast		chunkSize = 0;
	uvast		bytesRemaining = 0;
	Object		fileRef = 0;

	BCB_DEBUG_PROC("+ bcbUpdatePayloadFromFile(%d," ADDR_FIELDSPEC
			"," ADDR_FIELDSPEC ",%d," ADDR_FIELDSPEC ","
			UVAST_FIELDSPEC "," ADDR_FIELDSPEC ",%d)",
			suite, (uaddr)context, (uaddr)blocksize, dataObj,
			(uaddr) dataReader, cipherBufLen, (uaddr) cipherZco,
			function);
	CHKERR(context);
	CHKERR(blocksize);
	CHKERR(dataReader);
	CHKERR(cipherZco);

	/* Step 0 - Initialization */
	chunkSize = blocksize->chunkSize;
	bytesRemaining = blocksize->plaintextLen;
	*cipherZco = 0;

	/* Step 1: Allocate read buffers. */
	if ((plaintext.value = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromFile: Can't allocate \
buffer of size %d.", chunkSize);
		BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> -1", NULL);

		return -1;
	}

	if (sci_crypt_start(suite, context, parms) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromFile: Can't start \
context", NULL);
		MRELEASE(plaintext.value);
		BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> -1", NULL);
		return -1;
	}

	/* Step 2: Walk through payload writing ciphertext. */
 	while (bytesRemaining > 0)
	{
 		/* Step 3.1: Prep for this encrypting iteration. */
 		memset(plaintext.value, 0, chunkSize);
 		if (bytesRemaining < chunkSize)
 		{
 			chunkSize = bytesRemaining;
 		}

 		/* Step 3.2: Generate ciphertext from earliest plaintext
		 * buffer. */
 		plaintext.length = zco_transmit(sdr, dataReader, chunkSize,
				(char *) plaintext.value);
 		if (plaintext.length <= 0)
 		{
 			BCB_DEBUG_ERR("x bcbUpdatePayloadFromFile: Can't \
do priming read of length %d.", chunkSize);

 			MRELEASE(plaintext.value);
			BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> -1",
					NULL);
 			return -1;
 		}

 		bytesRemaining -= plaintext.length;
 		ciphertext = sci_crypt_update(suite, context, function,
				plaintext);
		if ((ciphertext.value == NULL) || (ciphertext.length == 0))
		{
			BCB_DEBUG_ERR("x bcbUpdatePayloadFromFile: Could not \
encrypt.", plaintext.length, chunkSize);
			MRELEASE(plaintext.value);
			MRELEASE(ciphertext.value);
			BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> -1",
					NULL);
			return -1;
		}

		if (bpsec_transferToZcoFileSource(sdr, cipherZco, &fileRef,
				BCB_FILENAME, (char *) ciphertext.value,
				ciphertext.length) <= 0)
		{
			BCB_DEBUG_ERR("x bcbUpdatePayloadFromFile: Transfer \
of chunk has failed..", NULL);
			MRELEASE(ciphertext.value);
			MRELEASE(plaintext.value);
			BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> -1",
					NULL);
			return -1;
		}

		/*
		 * \todo: Consider if a sleep here will help the filesystem
		 * catch up.
		 *
		 * microsnooze(1000);
		 */

		MRELEASE(ciphertext.value);
	}

	MRELEASE(plaintext.value);
	if (sci_crypt_finish(suite, context, function, &parms) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbUpdatePayloadFromFile: Could not finish \
context.", NULL);

		BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> -1", NULL);
		return -1;
	}

	BCB_DEBUG_PROC("- bcbUpdatePayloadFromFile--> 1", NULL);

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: bcbDefaultCompute
 *
 * \par Calculate ciphertext given a set of data and associated key information
 *      in accordance with this ciphersuite specification.
 *
 * \retval unsigned char * - The security result.
 *
 * \param[in]  dataObj     - The serialized data to hash, a ZCO.
 * \param[in]  chunkSize   - The chunking size for going through the bundle
 * \param[in]  suite       - Which ciphersuite to use to caluclate the value.
 * \param[in]  key         - The key to use.
 * \param[in]  parms       - Ciphersuite Parameters
 * \param[in]  destructive - Boolean: encrypt plaintext in place.
 * \param[in]  xmitRate    - Transmission data rate, for buffer selection
 * \param[in]  function    - Encrypt or Decrypt
 *
 * \par Assumptions
 *      - The dataObj points to a ZCO.
 *
 * \par Notes:
 *		- The dataObj is encrypted in place. This avoids cases where the
 *		  dataObject is so large that it will not fit into memory.
 *		- Once encrypted, the dataObject MUST NOT change.
 *      - Note, ciphertext may be larger than the original plaintext. So, this
 *        function MUST account for the fact that overwriting the plaintext in
 *        place must account for size deltas.
 *      - any ciphertext larger than the original target block plaintext will be
 *        stored in the BCB as additional data.
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/10/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  02/27/16  E. Birrane     Added ciphersuite parms [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *
 *****************************************************************************/

static int	bcbDefaultCompute(Object *dataObj, uint32_t chunkSize,
			uint32_t suite, sci_inbound_tlv key,
			sci_inbound_parms parms, uint8_t encryptInPlace,
			size_t xmitRate, uint8_t function)
{
	Sdr		sdr = getIonsdr();
	csi_blocksize_t blocksize;
	uint32_t	cipherBufLen = 0;
	uint32_t	bytesRemaining = 0;
	ZcoReader	dataReader;
	uint8_t		*context = NULL;
	Object		cipherBuffer = 0;

	BCB_DEBUG_INFO("+ bcbDefaultCompute(0x%x, %d, %d, [0x%x, %d])",
			       (unsigned long) dataObj, chunkSize, suite,
				   (unsigned long) key.value, key.length);

	/* Step 0 - Sanity Check. */
	CHKERR(key.value);

	if ((sdr_begin_xn(sdr)) == 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultCompute - Can't start txn.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultCompute--> NULL", NULL);
		return -1;
	}

	/*
	 * Step 3 - Setup playback of data from the data object.
	 *          The data object is the target block.
	 */

	if ((bytesRemaining = zco_length(sdr, *dataObj)) <= 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultCompute - data object has no data.",
				NULL);
		sdr_cancel_xn(sdr);
		BCB_DEBUG_PROC("- bcbDefaultCompute--> NULL", NULL);
		return -1;
	}

	zco_start_transmitting(*dataObj, &dataReader);

	BCB_DEBUG_INFO("i bcbDefaultCompute: bundle size is %d",
			bytesRemaining);

	/* Step 4 - Grab and initialize a crypto context. */
	if ((context = sci_ctx_init(suite, key, function)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbDefaultCompute - Can't get context.", NULL);
		sdr_cancel_xn(sdr);
		BCB_DEBUG_PROC("- bcbDefaultCompute--> NULL", NULL);
		return -1;
	}

	/* Step 5: Calculate the maximum size of the ciphertext. */
	memset(&blocksize, 0, sizeof(blocksize));
	blocksize.chunkSize = chunkSize;
	blocksize.keySize = key.length;
	blocksize.plaintextLen = bytesRemaining;

	if ((cipherBufLen = csi_crypt_res_len(suite, context, blocksize,
			function)) <= 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultCompute: Predicted bad ciphertext \
length: %d", cipherBufLen);
		csi_ctx_free(suite, context);
		sdr_cancel_xn(sdr);

		BCB_DEBUG_PROC("- bcbDefaultCompute --> %d", -1);
		return -1;
	}

	BCB_DEBUG_INFO("i bcbDefaultCompute - CipherBufLen is %d",
			cipherBufLen);

	/**
 	* Step 6: Replace plaintext with ciphertext.
 	*         If we are encrypting, we have to make a decision on how we
 	*         generate the ciphertext; if we are decrypting, we similarly
 	*         have to decide how to generate the plaintext.
 	*
 	*         We will need to generate ciphertext separate from the
	*         existing user payload, as it is possible that payload ZCO
	*         is a ZCO to a file on the user system and encrypting the
	*         payload in place could, actually, encrypt the data on the
	*         user's system. That's bad.
 	*
 	*         So we have three choices for housing the ciphertext:
 	*         1. Encrypt in place, IF plaintext is not in a file.
 	*         1. Built a ZCO out of SDR dataspace (fast, but space limited)
 	*         2. Built a ZCO to a temp file (slow, but accomodates large
	*         	data)
 	*
 	*         We select which option based on the estimated size of the
 	*         ciphertext and the space available in the SDR.  If the
	*         ciphertext is less than 50% of the free SDR space, we will
	*         use a ZCO to the SDR heap. 50% is selected arbitrarily.
	*         If method 2 fails due to an sdr_malloc call, we will,
	*         instead, default to method 3. Method 2 could fail due to
	*         SDR heap fragmentation even if the SDR heap would otherwise
	*         have sufficient space to store the ciphertext.
 	*/

	if ((function == CSI_SVC_ENCRYPT) || (function == CSI_SVC_DECRYPT))
	{
		if (encryptInPlace || xmitRate == 0)
		{
			if ((bcbUpdatePayloadInPlace(suite, parms, context,
					&blocksize, *dataObj, &dataReader,
					cipherBufLen, &cipherBuffer, function))
					!= 0)
			{
				BCB_DEBUG_ERR("x bcbDefaultCompute: Cannot \
update ciphertext in place.", NULL);
				csi_ctx_free(suite, context);
				sdr_cancel_xn(sdr);
			}
		}
		else
		{
			int32_t	result;
			Object	cipherZco = 0;
			uvast	minFileBuffer;
			double	siestaBytes;
			double	siestaUsec;

			minFileBuffer = xmitRate / MAX_TEMP_FILES_PER_SECOND;
			result = 0;
			if (cipherBufLen < minFileBuffer)
			{
				result = bcbUpdatePayloadFromSdr(suite,
						parms, context, &blocksize,
						*dataObj, &dataReader,
						cipherBufLen, &cipherZco,
						function);
			}

			if (result <= 0)
			{
				if (cipherBufLen < minFileBuffer)
				{
					/*	Slow down to avoid
					 *	over-stressing the
					 *	file system.		*/

					siestaBytes = minFileBuffer
							- cipherBufLen;
					siestaUsec = (1000000.0 * siestaBytes)
							/ xmitRate;
					microsnooze((unsigned int) siestaUsec);
				}

				result = bcbUpdatePayloadFromFile(suite,
						parms, context, &blocksize,
						*dataObj, &dataReader,
						cipherBufLen, &cipherZco,
						function);
			}

			if (result <= 0)
			{
				BCB_DEBUG_ERR("x bcbDefaultCompute: Cannot \
allocate ZCO for ciphertext of size " UVAST_FIELDSPEC, cipherBufLen);
				csi_ctx_free(suite, context);
				sdr_cancel_xn(sdr);

				BCB_DEBUG_PROC("- bcbDefaultCompute --> %d",
						-1);
				return -1;
			}

			zco_destroy(sdr, *dataObj);
			*dataObj = cipherZco;
		}
	}
	else
	{
		BCB_DEBUG_ERR("x bcbDefaultCompute: Invalid service: %d",
				function);
		csi_ctx_free(suite, context);
		sdr_cancel_xn(sdr);

		BCB_DEBUG_PROC("- bcbDefaultCompute --> %d", -1);
		return -1;
	}

	csi_ctx_free(suite, context);

	if (sdr_end_xn(sdr) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultCompute: Can't end encrypt txn.",
				NULL);
		return -1;
	}

	return 0;
}

/*****************************************************************************
 *                      BCB BLOCK MANAGEMENT FUNCTIONS                       *
 *****************************************************************************/

int	bcbSerialize(ExtensionBlock *blk, Bundle *bundle)
{
	/*	NOTE: BCBs are automatically serialized at the time
	 *	they are attached to a bundle, and they are not
	 *	subject to canonicalization (a BCB cannot be the
	 *	target of a BIB).  Nothing to do here.			*/

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bcbAcquire
 *
 * \par Purpose: This callback is called when a serialized BCB is
 *               encountered during bundle reception.  The callback
 *               will deserialize the block into a scratchpad object.
 *
 * \retval int -- 1 - The block was deserialized into a structure in the
 *                    scratchpad
 *                0 - The block was deserialized but does not appear valid.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  The block whose serialized bytes will be deserialized
 *                      in the block's scratchpad.
 * \param[in]      wk   The work area associated with this bundle acquisition.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/15/09  E. Birrane     Initial Implementation.
 *            S. Burleigh    Port from bsp_pibAcquire
 *  01/23/16  E. Birrane     SBSP Update
 *****************************************************************************/

int	bcbAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BCB_DEBUG_PROC("+ bcbAcquire(0x%x, 0x%x)", (unsigned long) blk,
			(unsigned long) wk);
	CHKERR(blk);
	CHKERR(wk);

	result = bpsec_deserializeASB(blk, wk);
	BCB_DEBUG_INFO("i bcbAcquire: Deserialize result %d", result);

	BCB_DEBUG_PROC("- bcbAcquire -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bcbRecord
 *
 * \par Purpose:	This callback copies an acquired BCB block's object
 *			into the object of a non-volatile BCB in heap space.
 *
 * \retval   0 - Recording was successful.
 *          -1 - There was a system error.
 *
 * \param[in]  oldBlk	The acquired BCB in working memory.
 * \param[in]  newBlk	The non-volatle BCB in heap space.
 *
 *****************************************************************************/

int	bcbRecord(ExtensionBlock *new, AcqExtBlock *old)
{
	int	result;

	BCB_DEBUG_PROC("+ bcbRecord(%x, %x)", (unsigned long) new,
			(unsigned long) old);

	result = bpsec_recordAsb(new, old);
	new->tag = 0;
	BCB_DEBUG_PROC("- bcbRecord", NULL);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bcbClear
 *
 * \par Purpose: This callback removes all memory allocated by the bpsec module
 *               during the block's acquisition process.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the bpsec module.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/04/09  E. Birrane     Initial Implementation.
 *            S. Burleigh    Port from pibCopy
 *****************************************************************************/

void	bcbClear(AcqExtBlock *blk)
{
	BpsecInboundBlock	*asb;

	BCB_DEBUG_PROC("+ bcbClear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		asb = (BpsecInboundBlock *) (blk->object);
		bpsec_releaseInboundAsb(asb);
		blk->object = NULL;
		blk->size = 0;
	}

	BCB_DEBUG_PROC("- bcbClear", NULL);
}

/******************************************************************************
 *
 * \par Function Name: bcbCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a BCB
 * 		 to a new block that is a copy of the original.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  newBlk The new copy of this extension block.
 * \param[in]      oldBlk The original extension block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/02/12  S. Burleigh    Port from pibCopy
 *  11/07/15  E. Birrane     Comments. [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bcbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return bpsec_copyAsb(newBlk, oldBlk);
}

/******************************************************************************
 *
 * \par Function Name: bcbRelease
 *
 * \par Purpose: This callback releases SDR heap space allocated to
 *               a block confidentiality block.
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated SDR heap space must be
 *                      released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/30/09  E. Birrane     Initial Implementation.
 *            S. Burleigh    SBSP Update
 *  12/15/15  E. Birrane     SBSP Update
 *****************************************************************************/

void    bcbRelease(ExtensionBlock *blk)
{
	BCB_DEBUG_PROC("+ bcbRelease(%x)", (unsigned long) blk);

	CHKVOID(blk);
	bpsec_releaseOutboundAsb(getIonsdr(), blk->object);
	BCB_DEBUG_PROC("- bcbRelease(%c)", ' ');
}

/*****************************************************************************
 *                      BPSEC ENCRYPTION FUNCTIONS                           *
 *****************************************************************************/

static Object	bcbFindNew(Bundle *bundle, uint16_t profNbr, char *keyName)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			blockObj;
	ExtensionBlock		block;
	BpsecOutboundBlock	asb;

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blockObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &block, blockObj,
				sizeof(ExtensionBlock));
		if (block.bytes)	/*	Already serialized.	*/
		{
			continue;	/*	Not locally sourced.	*/
		}

		if (block.type != BlockConfidentialityBlk)
		{
			continue;	/*	Doesn't apply.		*/
		}

		sdr_read(sdr, (char *) &asb, block.object,
				sizeof(BpsecOutboundBlock));
		if (asb.contextId != profNbr)
		{
			continue;	/*	For a different rule.	*/
		}

		if (strlen(keyName) != 0 && strcmp(keyName, asb.keyName) != 0)
		{
			continue;	/*	For a different rule.	*/
		}

		return blockObj;
	}

	return 0;
}

static Object	bcbFindOutboundTarget(Bundle *bundle, int blockNumber)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			blockObj;
	ExtensionBlock		block;
	BpsecOutboundBlock	asb;
	Object			elt2;
	Object			targetObj;
	BpsecOutboundTarget	target;

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blockObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &block, blockObj,
				sizeof(ExtensionBlock));
		if (block.type != BlockConfidentialityBlk)
		{
			continue;	/*	Not a BCB.		*/
		}

		/*	This is a BCB.  See if the indicated non-BPSec
		 *	block is one of its targets.			*/

		sdr_read(sdr, (char *) &asb, block.object,
				sizeof(BpsecOutboundBlock));
		for (elt2 = sdr_list_first(sdr, asb.targets); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			targetObj = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &target, targetObj,
					sizeof(BpsecOutboundTarget));
			if (target.targetBlockNumber == blockNumber)
			{
				return elt2;
			}
		}
	}

	return 0;	/*	No such target.				*/
}

static Object	bcbCreate(Bundle *bundle, BcbProfile *prof, char *keyName)
{
	Sdr			sdr = getIonsdr();
	ExtensionBlock		blk;
	BpsecOutboundBlock	asb;

	memset((char *) &blk, 0, sizeof(ExtensionBlock));
	blk.type = BlockConfidentialityBlk;
	blk.tag = 0;
	blk.crcType = NoCRC;
	memset((char *) &asb, 0, sizeof(BpsecOutboundBlock));
	bpsec_insertSecuritySource(bundle, &asb);
	asb.contextId = prof->profNbr;
	memcpy(asb.keyName, keyName, BPSEC_KEY_NAME_LEN);
	asb.targets = sdr_list_create(sdr);
	asb.parmsData = sdr_list_create(sdr);
	if (asb.targets == 0 || asb.parmsData == 0)
	{
		BCB_DEBUG_ERR("x bcbCreate: Failed to initialize BCB ASB.",
				NULL);
		return 0;
	}

	blk.size = sizeof(BpsecOutboundBlock);
	if ((blk.object = sdr_malloc(sdr, blk.size)) == 0)
	{
		BCB_DEBUG_ERR("x bcbCreate: Failed to SDR allocate object of \
size %d bytes", blk.size);
		return 0;
	}

	sdr_write(sdr, blk.object, (char *) &asb, blk.size);
	return attachExtensionBlock(BlockConfidentialityBlk, &blk, bundle);
}

static int	bcbAddTarget(Sdr sdr, Bundle *bundle, Object *bcbObj,
			ExtensionBlock *bcbBlk, BpsecOutboundBlock *asb,
			BcbProfile *prof, char *keyName, int targetBlockNumber)
{
	Object			secBlockElt;
	Object			bibObj;
	ExtensionBlock		bib;
	BpsecOutboundBlock	bibAsb;
	ExtensionBlock		clone;
	BpsecOutboundBlock	cloneAsb;
	Object			targetElt;
	Object			targetObj = 0;
	BpsecOutboundTarget	target;
	unsigned char		*serializedAsb;
	Object			cloneObj;

	/*	If the block is already signed by a BIB, then that
	 *	BIB must also be encrypted.  Prepare for that.		*/

	secBlockElt = bcbFindOutboundTarget(bundle, targetBlockNumber);

	/*	Is the block already encrypted?				*/

	if (bcbFindOutboundTarget(bundle, targetBlockNumber) != 0)
	{
		/*	Block is already encrypted by a BCB.		*/

		return 0;	/*	Nothing to do.			*/
	}

	/*	Block needs to be added as a target to the applicable
	 *	newly inserted BCB.					*/

	if (*bcbObj == 0)	/*	New BCB doesn't exist yet.	*/
	{
		*bcbObj = bcbCreate(bundle, prof, keyName);
		if (*bcbObj == 0)
		{
			return -1;
		}

		sdr_read(sdr, (char *) bcbBlk, *bcbObj, sizeof(ExtensionBlock));
		sdr_read(sdr, (char *) asb, bcbBlk->object, bcbBlk->size);
	}

	if (bpsec_insert_target(sdr, asb, targetBlockNumber) < 0)
	{
		return -1;
	}

	/*	Now determine what additional encryption is needed.	*/

	if (secBlockElt == 0)
	{
		return 0;	/*	Nothing more to do.		*/
	}

	/*	The security block we found earlier must be a BIB.
	 *	(If it were a BCB, we wouldn't have added the target
	 *	block as a target of the new BCB.)			*/

	bibObj = (Object) sdr_list_data(sdr, secBlockElt);
	sdr_read(sdr, (char *) &bib, bibObj, sizeof(ExtensionBlock));
	sdr_read(sdr, (char *) &bibAsb, bib.object, bib.size);
	if (sdr_list_length(sdr, bibAsb.targets) == 1)
	{
		/*	The target block is the sole target of this
		 *	BIB.  Just add this BIB as a target of the BCB.	*/

		if (bpsec_insert_target(sdr, asb, bib.number) < 0)
		{
			return -1;
		}

		return 0;
	}

	/*	More complicated.  Must clone this BIB, such that
	 *	the original BIB no longer signs the target block
	 *	-- only the (new) clone BIB does so.  Then add the
	 *	clone BIB as an additional target of the new BCB.	*/

	memcpy((char *) &clone, (char *) &bib, sizeof(ExtensionBlock));
	memcpy((char *) &cloneAsb, (char*) &bibAsb, sizeof(BpsecOutboundBlock));
	for (targetElt = sdr_list_first(sdr, bibAsb.targets); targetElt;
			targetElt = sdr_list_next(sdr, targetElt))
	{
		targetObj = sdr_list_data(sdr, targetElt);
		sdr_read(sdr, (char *) &target, targetObj,
				sizeof(BpsecOutboundTarget));
		if (target.targetBlockNumber == targetBlockNumber)
		{
			break;
		}
	}

	CHKERR(targetElt);	/*	System error if didn't find it.	*/

	/*	Must move this target to the clone BIB.  First
	 *	remove it from the original BIB's list of targets
	 *	and re-serialize the original BIB.			*/

	sdr_list_delete(sdr, targetElt, NULL, NULL);
	serializedAsb = bpsec_serializeASB((uint32_t *) &(bib.dataLength),
			&bibAsb);
	CHKERR(serializedAsb);
	if (serializeExtBlk(&bib, (char *) serializedAsb) < 0)
	{
		MRELEASE(serializedAsb);
		putErrmsg("Failed re-serializing cloned BIB.", NULL);
		return -1;
	}
	
	MRELEASE(serializedAsb);

	/*	Now fix up the clone BIB (its sole target is the
	 *	block we're adding as a BCB target) and add it as
	 *	an additional target of the new BCB.			*/

	sdr_free(sdr, target.results);
	target.results = sdr_list_create(sdr);
	CHKERR(target.results);
	cloneAsb.targets = sdr_list_create(sdr);
	CHKERR(cloneAsb.targets);
	sdr_list_insert_last(sdr, cloneAsb.targets, targetObj);
	cloneAsb.parmsData = sdr_list_create(sdr);
	CHKERR(cloneAsb.parmsData);
	clone.object = sdr_malloc(sdr, clone.size);
	CHKERR(clone.object);
	sdr_write(sdr, clone.object, (char *) &cloneAsb, clone.size);
	cloneObj = attachExtensionBlock(BlockIntegrityBlk, &clone, bundle);
	CHKERR(cloneObj);
	if (bibAttach(bundle, &clone, &cloneAsb) < 0)
	{
		putErrmsg("Failed attaching clone BIB.", NULL);
		return -1;
	}

	if (bpsec_insert_target(sdr, asb, clone.number) < 0)
	{
		return -1;
	}

	return 0;
}

int	bcbDefaultEncrypt(uint32_t suite, Bundle *bundle, ExtensionBlock *blk,
		BpsecOutboundBlock *asb, BpsecOutboundTarget *target,
		size_t xmitRate, uvast *length, char *toEid)
{
	Sdr			sdr = getIonsdr();
	sci_inbound_tlv		sessionKey;
	sci_inbound_tlv		encryptedSessionKey;
	sci_inbound_tlv		longtermKey;
	sci_inbound_parms	parms;

	BCB_DEBUG_INFO("+ bcbDefaultEncrypt(%d, 0x%x, 0x%x, 0x%x", suite,
			(unsigned long) bundle, (unsigned long) blk,
			(unsigned long) asb);

	/*	Sanity Checks.						*/

	CHKERR(bundle && blk && asb && target && length);
	CHKERR(target->targetBlockNumber > 0);
	longtermKey = bpsec_retrieveKey(asb->keyName);
	if (longtermKey.length == 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt - Can't get longterm \
key.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> 0", NULL);
		return ERROR;
	}

	*length = 0;

	/*	Initialize for keying.					*/

	memset(&sessionKey, 0, sizeof(sci_inbound_tlv));
	memset(&encryptedSessionKey, 0, sizeof(sci_inbound_tlv));
	memset(&longtermKey, 0, sizeof(sci_inbound_tlv));

	/*	Grab session key to use for the encryption.		*/

	sessionKey = sci_crypt_parm_get(suite, CSI_PARM_BEK);
	if ((sessionKey.value == NULL) || (sessionKey.length == 0))
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt - Can't get session key.",
				NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
		return ERROR;
	}

	/*	Grab cipher parms to seed encryption.			*/

	memset(&parms, 0, sizeof(parms));
	parms.iv = sci_crypt_parm_get(suite, CSI_PARM_IV);
	parms.salt = sci_crypt_parm_get(suite, CSI_PARM_SALT);

	/*	Now use the long-term key to encrypt the session key.
	 *	We assume session key sizes fit into memory and do
	 *	not need to be chunked. We want to make sure we can
	 *	encrypt all the keys before doing surgery on the
	 *	target block itself.					*/

	if ((sci_crypt_key(suite, CSI_SVC_ENCRYPT, &parms, longtermKey,
			sessionKey, &encryptedSessionKey)) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Could not decrypt \
session key", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> 0", NULL);
		return ERROR;
	}

	if ((encryptedSessionKey.value == NULL)
	|| (encryptedSessionKey.length == 0))
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt - Can't get encrypted \
session key.", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
		sci_cipherparms_free(parms);
		return ERROR;
	}

	/*	Finally, encrypt the target block's block-specific
	 *	data.							*/

	switch (target->targetBlockNumber)
	{
	case 1:		/*	Target block is the payload block.	*/
		*length = bundle->payload.length;
		if (bcbDefaultCompute(&(bundle->payload.content),
				csi_blocksize(suite), suite, sessionKey,
				parms, asb->encryptInPlace,
				xmitRate, CSI_SVC_ENCRYPT) < 0)
		{
			BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't encrypt \
payload.", NULL);
			BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
			MRELEASE(longtermKey.value);
			MRELEASE(sessionKey.value);
			MRELEASE(encryptedSessionKey.value);
			sci_cipherparms_free(parms);
			return ERROR;
		}

		break;

	default:	/*	Target block is an extension block.	*/
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: encryption of extension \
blocks is not yet implemented.", target->targetBlockNumber);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		MRELEASE(encryptedSessionKey.value);
		sci_cipherparms_free(parms);
		return ERROR;
	}

	/*	Free the plaintext keys post-encryption.		*/

	MRELEASE(longtermKey.value);
	MRELEASE(sessionKey.value);

	/*	Place the encrypted session key in the results field
	 *	of the target.						*/

	encryptedSessionKey.id = CSI_PARM_KEYINFO;
	if (bpsec_appendItem(sdr, target->results, &encryptedSessionKey) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't allocate heap \
space for ASB target's result.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt --> %d", 0);
		MRELEASE(encryptedSessionKey.value);
		sci_cipherparms_free(parms);
		return ERROR;
	}

	MRELEASE(encryptedSessionKey.value);

	/* Step 8 - Place the parameters in the appropriate BCB field. */

	if (bpsec_write_parms(sdr, asb, &parms) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't allocate heap \
space for ASN parms data.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt --> %d", 0);
		sci_cipherparms_free(parms);
		return ERROR;
	}

	sci_cipherparms_free(parms);
	if (asb->parmsData == 0)
	{
		BCB_DEBUG_WARN("x bcbDefaultEncrypt: Can't write cipher \
parameters.", NULL);
	}

	/*	BCB is now ready to be serialized.			*/

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: bcbAttach
 *
 * \par Purpose: Construct, compute, and attach a BCB block within the bundle.
 *               This function completes construction of the ASB for a BCB,
 *               using the appropriate ciphersuite, and generates a serialized
 *               version of the block appropriate for transmission.
 * *
 * \retval int -1  - System Error.
 *              0  - Failure (such as No BCB Policy)
 *             >0  - BCB Attached
 *
 * \param[in|out]  bundle	The bundle to which a BCB is to be attached.
 * \param[out]     bcbBlk	The BCB extension block.
 * \param[out]     bcbAsb	The initialized ASB for this BCB.
 * \param[in]      xmitRate	For selecting encryption mode.
 *
 * \par Notes:
 *	    1. The blkAsb MUST be pre-allocated and of the correct size to hold
 *	       the created BCB ASB.
 *	    2. The passed-in asb MUST be pre-initialized with both the target
 *	       block type and the security source.
 *	    3. The bcbBlk MUST be pre-allocated and initialized with a size,
 *	       a target block type, and the object within the block MUST be
 *	       allocated in the SDR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/07/15  E. Birrane     Update to profiles, error checks [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  07/20/18  S. Burleigh    Abandon bundle if can't attach BCB
 *****************************************************************************/

static int	bcbAttach(Bundle *bundle, ExtensionBlock *bcbBlk,
			BpsecOutboundBlock *bcbAsb, size_t xmitRate)
{
	Sdr			sdr = getIonsdr();
	int			result = 0;
	BcbProfile		*prof = NULL;
	char			*fromEid;	/*	Instrumentation.*/
	char			*toEid;		/*	For whatever.	*/
	Object			elt;
	Object			targetObj;
	BpsecOutboundTarget	target;
	unsigned char		*serializedAsb = NULL;
	uvast			length = 0;

	BCB_DEBUG_PROC("+ bcbAttach (0x%x, 0x%x, 0x%x)",
			(unsigned long) bundle, (unsigned long) bcbBlk,
			(unsigned long) bcbAsb);

	/* Step 0 - Sanity checks. */
	CHKERR(bundle);
	CHKERR(bcbBlk);
	CHKERR(bcbAsb);
	if (sdr_list_length(sdr, bcbAsb->targets) == 0)
	{
		BCB_DEBUG(2, "NOT attaching BCB: no targets.", NULL);

		result = 0;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach -> %d", result);
		return result;
	}

	BCB_DEBUG(2, "Attaching BCB.", NULL);
	if (bpsec_getOutboundSecuritySource(bundle, bcbAsb, &fromEid) < 0)
	{
		BCB_DEBUG_ERR("x bcbAttach: Can't get security source.", NULL);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach --> %d", result);
		return result;
	}

	/* Step 1 - Finish populating the BCB ASB.			*/

	prof = get_bcb_prof_by_number(bcbAsb->contextId);
	CHKERR(prof);
	if (prof->construct)
	{
		if (prof->construct(prof->suiteId, bcbBlk, bcbAsb) < 0)
		{
			ADD_BCB_TX_FAIL(fromEid, 1, 0);
			MRELEASE(fromEid);

			BCB_DEBUG_ERR("x bcbAttach: Can't construct ASB.",
					NULL);
			result = -1;
			bundle->corrupt = 1;
			scratchExtensionBlock(bcbBlk);
			BCB_DEBUG_PROC("- bcbAttach --> %d", result);
			return result;
		}
	}

	/* Step 2 - Encrypt the target blocks and store session keys.	*/

	readEid(&(bundle->destination), &toEid);
	CHKERR(toEid);
	for (elt = sdr_list_first(sdr, bcbAsb->targets); elt;
			elt = sdr_list_next(sdr, elt))
	{
		targetObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &target, targetObj,
				sizeof(BpsecOutboundTarget));
		result = (prof->encrypt == NULL)
			?  bcbDefaultEncrypt(prof->suiteId, bundle, bcbBlk,
			bcbAsb, &target, xmitRate, &length, toEid)
			: prof->encrypt(prof->suiteId, bundle, bcbBlk,
			bcbAsb, &target, xmitRate, &length, toEid);
		if (result < 0)
		{
			MRELEASE(toEid);
			ADD_BCB_TX_FAIL(fromEid, 1, length);
			MRELEASE(fromEid);

			BCB_DEBUG_ERR("x bcbAttach: Can't encrypt target.",
				       NULL);
			result = -1;
			bundle->corrupt = 1;
			scratchExtensionBlock(bcbBlk);
			BCB_DEBUG_PROC("- bcbAttach --> %d", result);
			return result;
		}
	}

	MRELEASE(toEid);

	/* Step 3 - serialize the BCB ASB into the BCB blk. */

	/* Step 3.1 - Create a serialized version of the BCB ASB. */

	if ((serializedAsb = bpsec_serializeASB((uint32_t *)
			&(bcbBlk->dataLength), bcbAsb)) == NULL)
	{
		ADD_BCB_TX_FAIL(fromEid, 1, length);
		MRELEASE(fromEid);

		BCB_DEBUG_ERR("x bcbAttach: Unable to serialize ASB.  \
bcbBlk->dataLength = %d", bcbBlk->dataLength);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach --> %d", result);
		return result;
	}

	/* Step 3.2 - Copy serializedBCB ASB into the BCB extension block. */

	if ((result = serializeExtBlk(bcbBlk, (char *) serializedAsb)) < 0)
	{
		bundle->corrupt = 1;
	}

	MRELEASE(serializedAsb);

	ADD_BCB_TX_PASS(fromEid, 1, length);
	MRELEASE(fromEid);

	BCB_DEBUG_PROC("- bcbAttach --> %d", result);
	return result;
}

static int	bcbAttachAll(Bundle *bundle, size_t xmitRate)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			blockObj;
	ExtensionBlock		block;
	BpsecOutboundBlock	asb;

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blockObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &block, blockObj,
				sizeof(ExtensionBlock));
		if (block.bytes)	/*	Already serialized.	*/
		{
			continue;	/*	Not newly sourced.	*/
		}

		if (block.type != BlockConfidentialityBlk)
		{
			continue;	/*	Doesn't apply.		*/
		}

		/*	This is a new BCB: perform all encryption,
		 *	insert all security results, serialize.		*/

		sdr_read(sdr, (char *) &asb, block.object,
				sizeof(BpsecOutboundBlock));
		if (bcbAttach(bundle, &block, &asb, xmitRate) < 0)
		{
			return -1;
		}
	}

	return 0;
}

int	bpsec_encrypt(Bundle *bundle)
{
	Sdr			sdr = getIonsdr();
	Object			rules;
	Object			elt;
	Object			ruleObj;
	BPsecBcbRule		rule;
	BcbProfile		*prof;
	char			keyBuffer[32];
	int			keyBuflen = sizeof keyBuffer;
	Object			bcbObj;
	ExtensionBlock		bcbBlk;
	BpsecOutboundBlock	asb;
	Object			elt2;
	Object			blockObj;
	ExtensionBlock		block;
	size_t			xmitRate = 125000;

	/*	NOTE: need to reinstate a processOnDequeue method
	 *	for BCB extension blocks: extracts xmitRate from
	 *	DequeueContext and stashes it in the Bundle so that
	 *	bpsec_encrypt can retrieve it.  Or else libbpP.c
	 *	could pass that value directly to bpsec_encrypt
	 *	as an API parameter.					*/

	rules = sec_get_bpsecBcbRuleList();

	/*	Apply all applicable BCB rules.				*/

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rule, ruleObj, sizeof(BPsecBcbRule));
		if (rule.blockType == PrimaryBlk
		|| rule.blockType == BlockIntegrityBlk
		|| rule.blockType == BlockConfidentialityBlk)
		{
			/*	This is an error in the rule.  No
			 *	target of a BCB can be a Primary
			 *	block or another BIB, and a BCB can
			 *	target a BIB only implicitly (when
			 *	targeting a block that is already
			 *	the target of a BIB).			*/

			continue;
		}

		if (!bpsec_BcbRuleApplies(bundle, &rule))
		{
			continue;
		}

		prof = get_bcb_prof_by_name(rule.profileName);
		if (prof == NULL)
		{
			/*	This is an error in the rule; profile
			 *	may have been deleted after rule was
			 *	added.					*/

			continue;
		}

		if (strlen(rule.keyName) > 0
		&& sec_get_key(rule.keyName, &keyBuflen, keyBuffer) == 0)
		{
			/*	Again, an error in the rule; key may
			 *	have been deleted after rule was added.	*/

			continue;
		}

		/*	Need to enforce this rule on all applicable
		 *	blocks.  First find the newly sourced BCB
		 *	that applies the rule's mandated profile and
		 *	(if noted) key.					*/

		bcbObj = bcbFindNew(bundle, prof->profNbr, rule.keyName);
		if (bcbObj)
		{
			sdr_read(sdr, (char *) &bcbBlk, bcbObj,
					sizeof(ExtensionBlock));
			sdr_read(sdr, (char *) &asb, bcbBlk.object,
					bcbBlk.size);
		}

		/*	(If this BCB doesn't exist, it will be created
		 *	as soon as its first target is identified.)
		 *
		 *	Now look for blocks to which this rule must
		 *	be applied.					*/

		if (rule.blockType == PayloadBlk)
		{
			if (bcbAddTarget(sdr, bundle, &bcbObj, &bcbBlk, &asb,
					prof, rule.keyName, 1) < 0)
			{
				return -1;
			}

			continue;
		}

		for (elt2 = sdr_list_first(sdr, bundle->extensions); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			blockObj = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &block, blockObj,
					sizeof(ExtensionBlock));
			if (block.type != rule.blockType)
			{
				continue;	/*	Doesn't apply.	*/
			}

			/*	This rule would apply to this block.	*/

			if (bcbAddTarget(sdr, bundle, &bcbObj, &bcbBlk, &asb,
					prof, rule.keyName, block.number) < 0)
			{
				return -1;
			}
		}
	}

	/*	Now attach all new BCBs, encrypting all targets.	*/

	if (bcbAttachAll(bundle, xmitRate) < 0)
	{
		return -1;
	}

	return 0;
}

/*****************************************************************************
 *                    	    BCB DECRYPTION FUNCTIONS                         *
 *****************************************************************************/

static void	discardTarget(LystElt targetElt, LystElt bcbElt)
{
	BpsecInboundTarget	*target;
	AcqExtBlock		*bcb;
	BpsecInboundBlock	*asb;

	target = (BpsecInboundTarget *) lyst_data(targetElt);
	bpsec_releaseInboundTlvs(target->results);
	MRELEASE(target);
	lyst_delete(targetElt);
	bcb = (AcqExtBlock *) lyst_data(bcbElt);
	asb = (BpsecInboundBlock *) (bcb->object);
	if (lyst_length(asb->targets) == 0)
	{
		deleteAcqExtBlock(bcbElt);
	}
}

static LystElt	bcbFindInboundTarget(AcqWorkArea *work, int blockNumber,
			LystElt *bcbElt)
{
	LystElt			elt;
	AcqExtBlock		*block;
	BpsecInboundBlock	*asb;
	LystElt			elt2;
	BpsecInboundTarget	*target;

	for (elt = lyst_first(work->extBlocks); elt; elt = lyst_next(elt))
	{
		block = (AcqExtBlock *) lyst_data(elt);
		if (block->type != BlockConfidentialityBlk)
		{
			continue;
		}

		/*	This is a BCB.  See if the indicated
		 *	non-BCB block is one of its targets.		*/

		asb = (BpsecInboundBlock *) (block->object);
		for (elt2 = lyst_first(asb->targets); elt2;
				elt2 = lyst_next(elt2))
		{
			target = (BpsecInboundTarget *) lyst_data(elt2);
			if (target->targetBlockNumber == blockNumber)
			{
				*bcbElt = elt;
				return elt2;
			}
		}
	}

	return NULL;	/*	No such target.				*/
}

int	bcbDefaultDecrypt(uint32_t suite, AcqWorkArea *wk, AcqExtBlock *blk,
		BpsecInboundBlock *asb, BpsecInboundTarget *target,
		char *fromEid)
{
	sci_inbound_tlv		longtermKey;
	sci_inbound_tlv		sessionKeyInfo;
	sci_inbound_tlv		sessionKeyClear;
	sci_inbound_parms	parms;

	BCB_DEBUG_INFO("+ bcbDefaultDecrypt(%d, 0x%x, 0x%x)", suite,
			(unsigned long) wk, (unsigned long) blk);

	/* Step 0 - Sanity Checks. */
	CHKERR(wk && blk && asb && target);

	memset(&longtermKey, 0, sizeof(sci_inbound_tlv));
	memset(&sessionKeyInfo, 0, sizeof(sci_inbound_tlv));
	memset(&sessionKeyClear, 0, sizeof(sci_inbound_tlv));

	/* Step 1 - Initialization */

	/* Step 2 - Grab any ciphersuite parameters in the received BCB. */
	parms = sci_build_parms(asb->parmsData);

	/*
	 * Step 3 - Decrypt the encrypted session key. We need it to decrypt
	 *          the target block.
	 */

	/* Step 3.1 - Grab the long-term key used to protect the session key. */
	longtermKey = bpsec_retrieveKey(asb->keyName);
	if (longtermKey.length == 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultDecrypt: Can't get longterm key \
for %s", asb->keyName);
		BCB_DEBUG_PROC("- bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

	/*
	 * Step 3.2 -Grab the encrypted session key from the BCB itself. This
	 *           session key has been encrypted with the long-term key.
	 */
	sessionKeyInfo = sci_extract_tlv(CSI_PARM_KEYINFO, target->results);

	/*
	 * Step 3.3 - Decrypt the session key. We assume that the encrypted
	 * session key fits into memory and we can do the encryption all
	 * at once.
	 */

	if ((sci_crypt_key(suite, CSI_SVC_DECRYPT, &parms, longtermKey,
			sessionKeyInfo, &sessionKeyClear)) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbDefaultDecrypt: Could not decrypt session \
key", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKeyInfo.value);
		BCB_DEBUG_PROC("- bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

	/* Step 3.4 - Release unnecessary key-related memory. */

	if ((sessionKeyClear.value == NULL) || (sessionKeyClear.length == 0))
	{
		BCB_DEBUG_ERR("x bcbDefaultDecrypt: Could not decrypt session \
key", NULL);
		MRELEASE(sessionKeyClear.value);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKeyInfo.value);
		BCB_DEBUG_PROC("- bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

	MRELEASE(longtermKey.value);
	MRELEASE(sessionKeyInfo.value);

	/* Step 4 -Decrypt the target block payload in place. */

	switch (target->targetBlockNumber)
	{
	case 1:		/*	Target block is the payload block.	*/
		if (bcbDefaultCompute(&(wk->bundle.payload.content),
				csi_blocksize(suite), suite, sessionKeyClear,
				parms, 0, 0, CSI_SVC_DECRYPT) < 0)
		{
			BCB_DEBUG_ERR("x bcbDefaultDecrypt: Can't decrypt \
payload.", NULL);
			BCB_DEBUG_PROC("- bcbDefaultDecrypt--> NULL", NULL);
			MRELEASE(sessionKeyClear.value);
			return 0;
		}

		break;

	default:	/*	Target block is an extension block.	*/
		BCB_DEBUG_ERR("x bcbDefaultDecrypt: decryption of extension \
blocks is not yet implemented.", target->targetBlockNumber);
		BCB_DEBUG_PROC("- bcbDefaultDecrypt--> NULL", NULL);
		MRELEASE(sessionKeyClear.value);
		return 0;
	}

	MRELEASE(sessionKeyClear.value);
	return 1;
}

int	bpsec_decrypt(AcqWorkArea *work)
{
	Sdr			sdr = getIonsdr();
	Bundle			*bundle = &(work->bundle);
	Object			rules;
	Object			elt;
	Object			ruleObj;
	BPsecBcbRule		rule;
	BcbProfile		*prof;
	char			keyBuffer[32];
	int			keyBuflen = sizeof keyBuffer;
	LystElt			elt2;
	AcqExtBlock		*blk;
	BpsecInboundBlock	*asb;
	char			*fromEid;	/*	Instrumentation.*/
	LystElt			targetElt;
	LystElt			bcbElt;
	AcqExtBlock		*bcb;
	LystElt			bibElt;
	BpsecInboundTarget	*target;
	unsigned int		oldLength;
	int			result;
	AcqExtBlock		*bib;

	rules = sec_get_bpsecBcbRuleList();

	/*	Apply all applicable BCB rules.				*/

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rule, ruleObj, sizeof(BPsecBibRule));
		if (rule.blockType == PrimaryBlk
		|| rule.blockType == BlockIntegrityBlk
		|| rule.blockType == BlockConfidentialityBlk)
		{
			/*      This is an error in the rule.  No
			 *      target of a BCB can be a Primary
			 *      block or another BIB, and a BCB can
			 *      target a BIB only implicitly (when
			 *      targeting a block that is already
			 *      the target of a BIB).                   */

			continue;
		}

		if (!bpsec_BcbRuleApplies(bundle, &rule))
		{
			continue;
		}

		prof = get_bcb_prof_by_name(rule.profileName);
		if (prof == NULL)
		{
			/*	This is an error in the rule; profile
			 *	may have been deleted after rule was
			 *	added.					*/

			continue;
		}

		if (strlen(rule.keyName) > 0
		&& sec_get_key(rule.keyName, &keyBuflen, keyBuffer) == 0)
		{
			/*	Again, an error in the rule; key may
			 *	have been deleted after rule was added.	*/

			continue;
		}

		for (elt2 = lyst_first(work->extBlocks); elt2;
				elt2 = lyst_next(elt2))
		{
			blk = (AcqExtBlock *) lyst_data(elt2);
			if (blk->type != rule.blockType)
			{
				continue;	/*	Doesn't apply.	*/
			}

			/*	This rule would apply to this block.	*/

			oldLength = blk->length;
			targetElt = bcbFindInboundTarget(work, blk->number,
					&bcbElt);
			if (targetElt == NULL)
			{
				/*	Block is not encrypted.  No
				 *	need to decrypt.		*/

				continue;
			}

			/*	Block needs to be decrypted.		*/

			target = (BpsecInboundTarget *) lyst_data(targetElt);
			bcb = (AcqExtBlock *) lyst_data(bcbElt);
			asb = (BpsecInboundBlock *) (bcb->object);
			if (strlen(rule.keyName) > 0)
			{
				memcpy(asb->keyName, rule.keyName,
						BPSEC_KEY_NAME_LEN);
			}

			if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
			{
				/*	Waypoint source.		*/

				readEid(&(asb->securitySource), &fromEid);
				if (fromEid == NULL)
				{
					ADD_BCB_RX_FAIL(NULL, 1, 0);
					return -1;
				}
			}
			else	/*	Bundle source.			*/
			{
				readEid(&(bundle->id.source), &fromEid);
				if (fromEid == NULL)
				{
					ADD_BCB_RX_FAIL(NULL, 1, 0);
					return -1;
				}
			}

			result = (prof->decrypt == NULL)
				?  bcbDefaultDecrypt(prof->suiteId, work, blk,
				asb, target, fromEid)
				: prof->decrypt(prof->suiteId, work, blk,
				asb, target, fromEid);

			BCB_DEBUG_INFO("i bpsec_decrypt: Decrypt result was %d",
					result);
			switch (result)
			{
			case 0:	/*	Malformed block.		*/
				work->malformed = 1;

				/*	Intentional fall-through.	*/
			case -1:
				MRELEASE(fromEid);
				ADD_BCB_RX_FAIL(fromEid, 1, 0);
				continue;

			default:
				break;
			}

			/*	Decryption completed.			*/

			if (blk->length == 0)	/*	Discarded.	*/
			{
				deleteAcqExtBlock(elt2);
				bundle->extensionsLength -= oldLength;
				discardTarget(targetElt, bcbElt);
			}
			else	/*	Target decrypted.		*/
			{
				if (bpsec_destinationIsLocal(&(work->bundle)))
				{
					BCB_DEBUG(2, "BCB target decrypted.",
							NULL);
					ADD_BCB_RX_PASS(fromEid, 1, 0);
					discardTarget(targetElt, bcbElt);
				}
				else
				{
					ADD_BCB_FWD(fromEid, 1, 0);
				}

				if (blk->length != oldLength)
				{
					bundle->extensionsLength -= oldLength;
					bundle->extensionsLength += blk->length;
				}
			}

			/*	Is this block also signed by a BIB?	*/

			targetElt = bibFindInboundTarget(work, blk->number,
					&bibElt);
			if (targetElt == NULL)
			{
				/*	Block not signed by a BIB.	*/

				MRELEASE(fromEid);
				continue;
			}

			/*	Block is signed by a BIB, so we must
			 *	decrypt that BIB as well.		*/

			bib = (AcqExtBlock *) lyst_data(bibElt);
			oldLength = bib->length;
			targetElt = bcbFindInboundTarget(work, bib->number,
					&bcbElt);
			if (targetElt == NULL)
			{
				/*	BIB is not encrypted, can't
				 *	decrypt it.			*/

				MRELEASE(fromEid);
				continue;
			}

			/*	BIB must be decrypted.			*/

			target = (BpsecInboundTarget *) lyst_data(targetElt);
			result = (prof->decrypt == NULL)
				?  bcbDefaultDecrypt(prof->suiteId, work, bib,
				asb, target, fromEid)
				: prof->decrypt(prof->suiteId, work, bib,
				asb, target, fromEid);

			BCB_DEBUG_INFO("i bpsec_decrypt: Decrypt result was %d",
					result);
			switch (result)
			{
			case 0:	/*	Malformed BIB.			*/
				work->malformed = 1;

				/*	Intentional fall-through.	*/
			case -1:
				MRELEASE(fromEid);
				ADD_BCB_RX_FAIL(fromEid, 1, 0);
				continue;

			default:
				break;
			}

			if (bib->length == 0)	/*	Discarded.	*/
			{
				deleteAcqExtBlock(bibElt);
				bundle->extensionsLength -= oldLength;
				discardTarget(targetElt, bcbElt);
			}
			else	/*	Target decrypted.		*/
			{
				if (bpsec_destinationIsLocal(&(work->bundle)))
				{
					BCB_DEBUG(2, "BIB decrypted.", NULL);
					ADD_BCB_RX_PASS(fromEid, 1, 0);
					discardTarget(targetElt, bcbElt);
				}
				else
				{
					ADD_BCB_FWD(fromEid, 1, 0);
				}

				if (bib->length != oldLength)
				{
					bundle->extensionsLength -= oldLength;
					bundle->extensionsLength += bib->length;
				}
			}

			MRELEASE(fromEid);
		}
	}

	return 0;
}
