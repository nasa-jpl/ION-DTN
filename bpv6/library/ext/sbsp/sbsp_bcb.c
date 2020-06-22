/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: sbsp_bcb.c
 **
 ** Description: Definitions supporting generic processing of BCB blocks.
 **              This includes both the BCB Interface to the ION SBSP
 **              API as well as a default implementation of the BCB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BCB SBSP Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              sbsp_bcbOffer
 **              sbsp_bcbProcessOnDequeue
 **              sbsp_bcbRelease
 **              sbsp_bcbCopy
 **                                                  sbsp_bcbAcquire
 **                                                  sbsp_bcbReview
 **                                                  sbsp_bcbDecrypt
 **                                                  sbsp_bcbRecord
 **                                                  sbsp_bcbClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    ENCRYPT SIDE                     DECRYPT SIDE
 **
 **              sbsp_bcbDefaultConstruct
 **              sbsp_bcbDefaultEncrypt
 **
 **                                              sbsp_bcbDefaultConstruct
 **                                              sbsp_bcbDefaultDecrypt
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
 **            S. Burleigh    Implementation as sbspbcb.c for Ssbsp
 **  11/07/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
#include "zco.h"
#include "sbsp_bcb.h"
#include "sbsp_instr.h"

#if (BCB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/* 2/27 */


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbAcquire
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

int	sbsp_bcbAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BCB_DEBUG_PROC("+ sbsp_bcbAcquire(0x%x, 0x%x)", (unsigned long) blk,
			(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);

	result = sbsp_deserializeASB(blk, wk);
	BCB_DEBUG_INFO("i sbsp_bcbAcquire: Deserialize result %d", result);

	BCB_DEBUG_PROC("- sbsp_bcbAcquire -> %d", result);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbClear
 *
 * \par Purpose: This callback removes all memory allocated by the sbsp module
 *               during the block's acquisition process.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the sbsp module.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/04/09  E. Birrane           Initial Implementation.
 *            S. Burleigh    Port from sbsp_pibCopy
 *****************************************************************************/

void	sbsp_bcbClear(AcqExtBlock *blk)
{
	SbspInboundBlock	*asb;

	BCB_DEBUG_PROC("+ sbsp_bcbClear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		asb = (SbspInboundBlock *) (blk->object);
		if (asb->parmsData)
		{
			BCB_DEBUG_INFO("i sbsp_bcbClear: Release result len %ld",
					asb->parmsLen);
			MRELEASE(asb->parmsData);
		}

		if (asb->resultsData)
		{
			BCB_DEBUG_INFO("i sbsp_bcbClear: Release result len %ld",
					asb->resultsLen);
			MRELEASE(asb->resultsData);
		}

		BCB_DEBUG_INFO("i sbsp_bcbClear: Release ASB len %d", blk->size);

		MRELEASE(blk->object);
		blk->object = NULL;
		blk->size = 0;
	}

	BCB_DEBUG_PROC("- sbsp_bcbClear", NULL);
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbCopy
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
 *  04/02/12  S. Burleigh    Port from sbsp_pibCopy
 *  11/07/15  E. Birrane     Comments. [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	sbsp_bcbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr			bpSdr = getIonsdr();
	SbspOutboundBlock	asb;
	int			result = 0;
	char		*buffer = NULL;

	BCB_DEBUG_PROC("+ sbsp_bcbCopy(0x%x, 0x%x)", (unsigned long) newBlk,
			(unsigned long) oldBlk);

	/* Step 1 - Sanity Checks. */
	CHKERR(newBlk);
	CHKERR(oldBlk);

	/* Step 2 - Allocate the new destination BCB. */
	newBlk->size = sizeof(asb);
	if((newBlk->object = sdr_malloc(bpSdr, sizeof(asb))) == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbCopy: Failed to allocate: %d",
				sizeof asb);
		return -1;
	}

	/*  Step 3 - Copy the source BCB into the destination. */

	/* Step 3.1 - Read the source BCB from the SDR. */
	sdr_read(bpSdr, (char *) &asb, oldBlk->object, sizeof asb);

	/*
	 * Step 3.2 - Copy parameters by reading entire parameter list
	 *            into a buffer and copying that buffer into the
	 *            destination block. Then write the copied parameters
	 *            back to the SDR.
	 */
	if (asb.parmsData)
	{
		buffer = MTAKE(asb.parmsLen);
		if (buffer == NULL)
		{
			BCB_DEBUG_ERR("x sbsp_bcbCopy: Failed to allocate: %d",
					asb.parmsLen);
			return -1;
		}

		sdr_read(bpSdr, buffer, asb.parmsData, asb.parmsLen);
		asb.parmsData = sdr_malloc(bpSdr, asb.parmsLen);
		if (asb.parmsData == 0)
		{
			BCB_DEBUG_ERR("x sbsp_bcbCopy: Failed to allocate: %d",
					asb.parmsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.parmsData, buffer, asb.parmsLen);
		MRELEASE(buffer);
	}

	/*
	 * Step 3.3 - Copy results by reading result field
	 *            into a buffer and copying that buffer into the
	 *            destination block. Then write the copied results
	 *            to the SDR.
	 */

	if (asb.resultsData)
	{
		buffer = MTAKE(asb.resultsLen);
		if (buffer == NULL)
		{
			BCB_DEBUG_ERR("x sbsp_bcbCopy: Failed to allocate: %d",
					asb.resultsLen);
			return -1;
		}

		sdr_read(bpSdr, buffer, asb.resultsData, asb.resultsLen);
		asb.resultsData = sdr_malloc(bpSdr, asb.resultsLen);
		if (asb.resultsData == 0)
		{
			BCB_DEBUG_ERR("x sbsp_bcbCopy: Failed to allocate: %d",
					asb.resultsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.resultsData, buffer, asb.resultsLen);
		MRELEASE(buffer);
	}

	/* Step 4 - Write copied block to the SDR. */
	sdr_write(bpSdr, newBlk->object, (char *) &asb, sizeof asb);

	BCB_DEBUG_PROC("- sbsp_bcbCopy -> %d", result);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbDecrypt
 *
 * \par Purpose: This callback decrypts the target block of a BCB.
 *
 * \retval int 1 - Decryption was unnecessary (not destination) or successful
 * 	       0 - The target block could not be decrypted
 *            -1 - There was a system error
 *
 * \param[in]  blk  The BCB whose target must be decrypted.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/03/09  E. Birrane     Initial Implementation.
 *            S. Burleigh    Port from sbsp_pibCopy
 *  11/10/15  E. Birrane     Update to profiles. [Secure DTN implementation
 *                           (NASA: NNX14CS58P)]
 *****************************************************************************/

int	sbsp_bcbDecrypt(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle;
	char		*dictionary;
	SbspInboundBlock	*asb = NULL;
	char		*fromEid;
	char		*toEid;
	BspBcbRule	bcbRule;
	BcbProfile	*prof = NULL;
	int		result;
	uvast bytes = 0;

	BCB_DEBUG_PROC("+ sbsp_bcbDecrypt(0x%x, 0x%x)", (unsigned long) blk,
			(unsigned long) wk);

	if (blk == NULL || blk->object == NULL || wk == NULL)
	{
		ADD_BCB_RX_FAIL(NULL, 1, 0);

		BCB_DEBUG_ERR("x sbsp_bcbDecrypt:  Blocks are NULL. %x",
				(unsigned long) blk);
		result = 0;
		BCB_DEBUG_PROC("- sbsp_bcbDecrypt --> %d", result);
		return result;
	}

	/*	For BCBs, the security destination is always the final
	 *	destination of the bundle.  The security source is
	 *	normally the original source of the bundle, but a BCB
	 *	can alternatively be inserted at any point in the
	 *	bundle's end-to-end path.				*/

	bundle = &(wk->bundle);
	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)	/*	No space.	*/
	{
		ADD_BCB_RX_FAIL(NULL, 1, 0);

		return 0;
	}

	asb = (SbspInboundBlock *) (blk->object);
	if (asb->securitySource.unicast)	/*	Waypoint source.*/
	{
		if (printEid(&(asb->securitySource), dictionary, &fromEid) < 0)
		{
			ADD_BCB_RX_FAIL(NULL, 1, 0);

			releaseDictionary(dictionary);
			return 0;
		}
	}
	else
	{
		if (printEid(&(bundle->id.source), dictionary, &fromEid) < 0)
		{
			ADD_BCB_RX_FAIL(NULL, 1, 0);
			releaseDictionary(dictionary);
			return 0;
		}
	}

	if (printEid(&(bundle->destination), dictionary, &toEid) < 0)
	{
		ADD_BCB_RX_FAIL(fromEid, 1, 0);

		MRELEASE(fromEid);
		releaseDictionary(dictionary);
		return 0;
	}

	releaseDictionary(dictionary);

	/*	Given sender & receiver EIDs, get applicable BCB rule.	*/

	prof = sbsp_bcbGetProfile(fromEid, toEid, asb->targetBlockType,
			&bcbRule);
	MRELEASE(toEid);

	if (prof == NULL)
	{
		/*	We can't decrypt this block.			*/

		if (bcbRule.destEid == 0)	/*	No rule.	*/
		{
			/*	We don't care about decrypting the
			 *	target block for this BCB, but we
			 *	preserve the BCB in case somebody
			 *	else does.				*/

			BCB_DEBUG_INFO("- sbsp_bcbDecrypt - No rule.", NULL);

			ADD_BCB_RX_MISS(fromEid, 1, bytes);
			MRELEASE(fromEid);

			result = 1;
			BCB_DEBUG_PROC("- sbsp_bcbDecrypt --> %d", result);
			blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
			return result;		/*	No information.	*/
		}

		/*	Rule is found, but we don't have this CS.
		 *	We cannot decrypt the block, so the block
		 *	is -- in effect -- malformed.			*/

		ADD_BCB_RX_FAIL(fromEid, 1, bytes);
		MRELEASE(fromEid);

		discardExtensionBlock(blk);
	 	BCB_DEBUG_ERR("- sbsp_bcbDecrypt - Profile missing!", NULL);
		result = 0;
		BCB_DEBUG_PROC("- sbsp_bcbDecrypt --> %d", result);
		return 0;			/*	Block malformed.*/
	}

	/*	Fill in missing information in the scratchpad area.	*/
	memcpy(asb->keyName, bcbRule.keyName, SBSP_KEY_NAME_LEN);

	/*	Invoke ciphersuite-specific check procedure.		*/
	result = (prof->decrypt == NULL) ?
				 sbsp_bcbDefaultDecrypt(prof->suiteId, wk, blk, &bytes) :
				 prof->decrypt(wk, blk, &bytes);

	/*	Discard the BCB if the local node is the destination
	 *	of the bundle or if decryption failed (meaning the
	 *	block is malformed and therefore the bundle is
	 *	malformed); otherwise make sure the BCB is retained.	*/

	if (result == 0 || sbsp_destinationIsLocal(&(wk->bundle)))
	{
		if(result == 0)
		{
			ADD_BCB_RX_FAIL(fromEid, 1, bytes);
		}
		else
		{
			BCB_DEBUG(2,"BCB Passed Decrypt", NULL);
			ADD_BCB_RX_PASS(fromEid, 1, bytes);
		}

		discardExtensionBlock(blk);
	}
	else
	{
		ADD_BCB_FWD(fromEid, 1, bytes);
		blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
	}

	MRELEASE(fromEid);
	BCB_DEBUG_PROC("- sbsp_bcbDecrypt --> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbDefaultConstruct
 *
 * \par Given a pre-allocated extension block and abstract security block,
 *      initialize the abstract security block for subsequent processing.
 *
 * \param[in]   suite  The ciphersuite used by this profile.
 * \param[in]     blk  The pre-allocated extension block.
 * \param[in/out] asb  The abstract security block being initialized.
 *
 * \par Notes:
 *  - The ASB will be attached to the extension block and passed back
 *    to another function (encrypt) to help create the contents of the
 *    serialized extension block.
 *
 *  - It is assumed that other functions populate non-ciphersuite related
 *    portions of the ASB, such as the security source, target block info,
 *    ASB instance, and key information.
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/05/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int	sbsp_bcbDefaultConstruct(uint32_t suite, ExtensionBlock *blk, SbspOutboundBlock *asb)
{

	CHKERR(blk);
	CHKERR(asb);

	/* Step 1: Populate block-instance-agnostic parts of the ASB. */
	asb->ciphersuiteType = suite;
	asb->parmsLen = 0;
	asb->parmsData = 0;
	asb->resultsData = 0;

	/* Step 2: Populate instance-specific parts of the ASB. */
#if 0
	asb->ciphersuiteFlags = SBSP_ASB_RES | SBSP_ASB_PARM;
#endif
	asb->ciphersuiteFlags = SBSP_ASB_PARM;
	asb->resultsLen = 0;

	return 0;
}



/*
 * Assume the asb key has been udpated with the key to use for decrypt.
 */
int sbsp_bcbDefaultDecrypt(uint32_t suite, AcqWorkArea *wk, AcqExtBlock *blk, uvast *bytes)
{

	SbspInboundBlock	*asb;
	csi_val_t longtermKey;
	csi_val_t sessionKeyInfo;
	csi_val_t sessionKeyClear;
	csi_cipherparms_t parms;

	BCB_DEBUG_INFO("+ sbsp_bcbDefaultDecrypt(%d, 0x%x, 0x%x)",
			       suite, (unsigned long) wk, (unsigned long) blk);

	/* Step 0 - Sanity Checks. */
	CHKERR(wk);
	CHKERR(blk);
	CHKERR(bytes);

	memset(&longtermKey, 0, sizeof(csi_val_t));
	memset(&sessionKeyInfo, 0, sizeof(csi_val_t));
	memset(&sessionKeyClear, 0, sizeof(csi_val_t));
	*bytes = 0;


	/* Step 1 - Initialization */
	asb = (SbspInboundBlock *) (blk->object);

	/* Step 2 - Grab any ciphersuite parameters in the received BCB. */
	parms = csi_build_parms(asb->parmsData, asb->parmsLen);

	/*
	 * Step 3 - Decrypt the encrypted session key. We need it to decrypt
	 *          the target block.
	 */

	/* Step 3.1 - Grab the long-term key used to protect the session key. */
	longtermKey = sbsp_retrieveKey(asb->keyName);
	if(longtermKey.len == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultDecrypt: Can't get longterm key for %s", asb->keyName);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

	/*
	 * Step 3.2 -Grab the encrypted session key from the BCB itself. This
	 *           session key has been encrypted with the long-term key.
	 */
	sessionKeyInfo = csi_extract_tlv(CSI_PARM_KEYINFO, asb->resultsData, asb->resultsLen);

	/*
	 * Step 3.3 - Decrypt the session key. We assume that the encrypted session
	 *            key fits into memory and we can do the encryption all at once.
	 */
	if((csi_crypt_key(suite, CSI_SVC_DECRYPT, &parms, longtermKey, sessionKeyInfo, &sessionKeyClear)) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultDecrypt: Could not decrypt session key", NULL);
		MRELEASE(longtermKey.contents);
		MRELEASE(sessionKeyInfo.contents);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

	/* Step 3.4 - Release unnecessary key-related memory. */

	if((sessionKeyClear.contents == NULL) || (sessionKeyClear.len == 0))
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultDecrypt: Could not decrypt session key", NULL);
		MRELEASE(sessionKeyClear.contents);
		MRELEASE(longtermKey.contents);
		MRELEASE(sessionKeyInfo.contents);

		BCB_DEBUG_PROC("- sbsp_bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

	MRELEASE(longtermKey.contents);
	MRELEASE(sessionKeyInfo.contents);

	/* Step 4 -Decrypt the target block payload in place. */

	switch (asb->targetBlockType)
	{
		case BLOCK_TYPE_PAYLOAD:
			*bytes = wk->bundle.payload.length;

			if (sbsp_bcbHelper(&(wk->bundle.payload.content),
							   csi_blocksize(suite),
							   suite,
							   sessionKeyClear,
							   parms, 0, 0,
							   CSI_SVC_DECRYPT) < 0)
			{
				BCB_DEBUG_ERR("x sbsp_bcbDefaultDecrypt: Can't decrypt payload.", NULL);
				BCB_DEBUG_PROC("- sbsp_bcbDefaultDecrypt--> NULL", NULL);
				MRELEASE(sessionKeyClear.contents);
				return 0;
			}

		break;

		default:
			BCB_DEBUG_ERR("x sbsp_bcbDefaultDecrypt: Can't decrypt block type \
%d: canonicalization not implemented.", asb->targetBlockType);
			BCB_DEBUG_PROC("- sbsp_bcbDefaultDecrypt--> NULL", NULL);
			MRELEASE(sessionKeyClear.contents);
			return 0;
	}

	MRELEASE(sessionKeyClear.contents);
	return 1;
}



uint32_t sbsp_bcbDefaultEncrypt(uint32_t suite,
			Bundle *bundle,
			ExtensionBlock *blk,
			SbspOutboundBlock *asb,
			size_t xmitRate,
			uvast *bytes)
{
	Sdr		bpSdr = getIonsdr();
	csi_val_t	sessionKey;
	csi_val_t	encryptedSessionKey;
	csi_val_t	longtermKey;
	csi_cipherparms_t parms;

	BCB_DEBUG_INFO("+ sbsp_bcbDefaultEncrypt(%d, 0x%x, 0x%x, 0x%x",
			suite, (unsigned long) bundle, (unsigned long) blk,
			(unsigned long) asb);

	/* Step 0 - Sanity Checks. */
	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(asb);
	CHKERR(bytes);

	memset(&sessionKey, 0, sizeof(csi_val_t));
	memset(&encryptedSessionKey, 0, sizeof(csi_val_t));
	memset(&longtermKey, 0, sizeof(csi_val_t));
	*bytes = 0;

	/*
	 * Step 1 - Make sure we have a long-term key that we can use to
	 * protect the session key.
	 */
	longtermKey = sbsp_retrieveKey(asb->keyName);
	if (longtermKey.len == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt - Can't get longterm \
key.", NULL);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt--> 0", NULL);
		return 0;
	}

	/* Step 2 - Grab session key to use for the encryption. */
	sessionKey = csi_crypt_parm_get(suite, CSI_PARM_BEK);

	if ((sessionKey.contents == NULL) || (sessionKey.len == 0))
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt - Can't get session \
key.", NULL);
		MRELEASE(longtermKey.contents);
		MRELEASE(sessionKey.contents);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt--> NULL", NULL);
		return 0;
	}

	/* Step 3 - Grab cipher parms to seed encryption.*/
	memset(&parms, 0, sizeof(parms));
	parms.iv = csi_crypt_parm_get(suite, CSI_PARM_IV);
	parms.salt = csi_crypt_parm_get(suite, CSI_PARM_SALT);

	/*
	 * Step 4 - Use the long-term key to encrypt the
	 *          session key. We assume session key sizes fit into
	 *          memory and do not need to be chunked. We want to
	 *          make sure we can encrypt all the keys before doing
	 *          surgery on the target block itself.
	 */
	if ((csi_crypt_key(suite, CSI_SVC_ENCRYPT, &parms, longtermKey,
			sessionKey, &encryptedSessionKey)) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt: Could not decrypt \
session key", NULL);
		MRELEASE(longtermKey.contents);
		MRELEASE(sessionKey.contents);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt--> 0", NULL);
		return 0;

	}

	if ((encryptedSessionKey.contents == NULL)
	|| (encryptedSessionKey.len == 0))
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt - Can't get encrypted \
session key.", NULL);
		MRELEASE(longtermKey.contents);
		MRELEASE(sessionKey.contents);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt--> NULL", NULL);
		csi_cipherparms_free(parms);
		return 0;
	}

	/* Step 5 - Encrypt the target block. */
	switch (asb->targetBlockType)
	{
	case BLOCK_TYPE_PAYLOAD:

		*bytes = bundle->payload.length;

		if (sbsp_bcbHelper(&(bundle->payload.content),
				csi_blocksize(suite), suite, sessionKey, parms,
			       	asb->encryptInPlace, xmitRate, CSI_SVC_ENCRYPT)
				< 0)
		{
			BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt: Can't \
encrypt payload.", NULL);
			BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt--> NULL",
					NULL);
			MRELEASE(longtermKey.contents);
			MRELEASE(sessionKey.contents);
			MRELEASE(encryptedSessionKey.contents);
			csi_cipherparms_free(parms);
			return 0;
		}

		break;

	default:
		BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt: Can't encrypt block \
type %d: canonicalization not implemented.", asb->targetBlockType);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt--> NULL", NULL);
		MRELEASE(longtermKey.contents);
		MRELEASE(sessionKey.contents);
		MRELEASE(encryptedSessionKey.contents);
		csi_cipherparms_free(parms);
		return 0;
	}

	/* Step 6 - Free plaintext keys post-encryption. */
	MRELEASE(longtermKey.contents);
	MRELEASE(sessionKey.contents);

	/*
	 * Step 7 - Place the encrypted session key in the BCB
	 *         results field.
	 */

	asb->resultsData = sbsp_build_sdr_result(bpSdr, CSI_PARM_KEYINFO,
			encryptedSessionKey, &(asb->resultsLen));
	MRELEASE(encryptedSessionKey.contents);

	if (asb->resultsData == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbDefaultEncrypt: Can't allocate heap \
space for ASB result.", NULL);
		BCB_DEBUG_PROC("- sbsp_bcbDefaultEncrypt --> %d", 0);
		csi_cipherparms_free(parms);
		return 0;
	}

	/* Step 8 - Place the parameters in the appropriate BCB field. */
	asb->parmsData = sbsp_build_sdr_parm(bpSdr, parms, &(asb->parmsLen));
	csi_cipherparms_free(parms);

	if (asb->parmsData == 0)
	{
		BCB_DEBUG_WARN("x sbsp_bcbDefaultEncrypt: Can't write cipher \
parameters.", NULL);
	}

	/*	BCB is now ready to be serialized.			*/

	return 1;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbGetProfile
 *
 * \par Purpose: Find the profile associated with a potential confidentiality
 *		 service.  The confidentiality security service within a
 *		 bundle is uniquely identified as OP(confidentiality, target),
 *		 where target is the identifier of the bundle block receiving
 *		 the protection.
 *
 *               The ciphersuite profile captures all function implementations
 *		 associated with the application of confidentiality for the
 *		 given target block from the given security source to the
 *		 security destination. If no profile exists, then there is no
 *		 implementation of BCB for the given block between the source
 *		 and destination; any securty policy rule requiring such an
 *		 implementation has been violated..
 *
 * \retval BcbProfile *  NULL  - No profile found.
 *            -          !NULL - The appropriate BCB Profile
 *
 * \param[in]  secSrc     The EID of the node that creates the BCB
 * \param[in]  secDest    The EID of the node that uses the BCB.
 * \param[in]  secTgtType The block type of the target block.
 * \param[out] secBcbRule The BCB rule capturing security policy.
 *
 * \par Notes:
 *      1. \todo Update to handle target identifier beyond just block type.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/07/15  E. Birrane     Update to profiles, error checks [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

BcbProfile	*sbsp_bcbGetProfile(char *secSrc, char *secDest, int secTgtType,
			BspBcbRule *bcbRule)
{
	Sdr		bpSdr = getIonsdr();
	Object		ruleAddr;
	Object		ruleElt;
	BcbProfile 	*prof = NULL;

	BCB_DEBUG_PROC("+ sbsp_bcbGetProfile(%s, %s, %d, 0x%x)",
				   (secSrc == NULL) ? "NULL" : secSrc,
				   (secDest == NULL) ? "NULL" : secDest,
				   secTgtType, (unsigned long) bcbRule);

	/* Step 0 - Sanity Checks. */
	CHKNULL(bcbRule);

	/* Step 1 - Find the BCB Rule capturing policy */
	sec_get_bspBcbRule(secSrc, secDest, secTgtType,	&ruleAddr, &ruleElt);

	/*
	 * Step 1.1 - If there is no matching rule, there is no policy
	 * and without policy we do not apply a BCB.
	 */

	if (ruleElt == 0)
	{
		memset((char *) bcbRule, 0, sizeof(BspBcbRule));
		BCB_DEBUG_INFO("i sbsp_bcbGetProfile: No rule found \
for BCBs. No BCB processing for this bundle.", NULL);
		return NULL;
	}

	/* Step 2 - Retrieve the Profile associated with this policy. */

	sdr_read(bpSdr, (char *) bcbRule, ruleAddr, sizeof(BspBcbRule));
	if( (prof = get_bcb_prof_by_name(bcbRule->ciphersuiteName)) == NULL)
	{
		BCB_DEBUG_INFO("i sbsp_bcbGetProfile: Profile of BCB rule is \
unknown '%s'.  No BCB processing for this bundle.", bcbRule->ciphersuiteName);
	}

	BCB_DEBUG_PROC("- sbsp_bcbGetProfile -> 0x%x", (unsigned long) prof);

	return prof;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbAttach
 *
 * \par Purpose: Construct, compute, and attach a BCB block within the bundle.
 *               This function determines, through a policy lookup, whether
 *               a BCB should be applied to a particular block in a bundle and,
 *               if so, constructs the appropriate BCB block, using the
 *               appropriate ciphersuite, and generates a serialized version of
 *               the block appropriate for transmission.
 * *
 * \retval int -1  - System Error.
 *              0  - Failure (such as No BCB Policy)
 *             >0  - BCB Attached
 *
 * \param[in|out]  bundle  The bundle to which a BCB might be attached.
 * \param[out]     bcbBlk  The serialized BCB extension block.
 * \param[out]     bcbAsb  The ASB for this BCB.
 *
 * \todo Update to handle target identifier beyond just block type.
 * \todo This function assumes bcb asb and blk are partially initialized
 *       by other functions. Clean up/document this, removing
 *       such assumptions where practical.
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

static int	sbsp_bcbAttach(Bundle *bundle, ExtensionBlock *bcbBlk,
			SbspOutboundBlock *bcbAsb, size_t xmitRate)
{
	int		result = 0;
	char		*fromEid = NULL;
	char		*toEid = NULL;
	char		eidBuf[32];
	BspBcbRule	bcbRule;
	BcbProfile	*prof = NULL;
	unsigned char	*serializedAsb = NULL;
	uvast		bytes = 0;

	BCB_DEBUG_PROC("+ sbsp_bcbAttach (0x%x, 0x%x, 0x%x)",
			(unsigned long) bundle, (unsigned long) bcbBlk,
			(unsigned long) bcbAsb);

	/* Step 0 - Sanity checks. */
	CHKERR(bundle);
	CHKERR(bcbBlk);
	CHKERR(bcbAsb);

	/* Step 1 -	Grab Policy for the candidate block. 		*/

	/* Step 1.1 -	Retrieve the from/to EIDs that bound the
			confidentiality service. 			*/

	if ((result = sbsp_getOutboundSecurityEids(bundle, bcbBlk, bcbAsb,
			&fromEid, &toEid)) <= 0)
	{
		ADD_BCB_TX_FAIL(NULL, 1, 0);

		BCB_DEBUG_ERR("x sbsp_bcbAttach: Can't get security EIDs.",
				NULL);
		result = -1;
		BCB_DEBUG_PROC("- sbsp_bcbAttach -> %d", result);
		return result;
	}

	/*	We only attach a BCB per a rule for which the local
		node is the security source.				*/

	MRELEASE(fromEid);
	isprintf(eidBuf, sizeof eidBuf, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	fromEid = eidBuf;

	/*
	 * Step 1.2 -	Grab the profile for encryption for the target
	 *		block from the EIDs. If there is no rule,
	 *		then there is no policy for attaching BCBs
	 *		in this instance.  If there is a rule but no
	 *		matching profile then the bundle must not be
	 *		forwarded.
	 */

	prof = sbsp_bcbGetProfile(fromEid, toEid, bcbAsb->targetBlockType,
			&bcbRule);
	MRELEASE(toEid);
	if (prof == NULL)
	{
		if (bcbRule.destEid == 0)	/*	No rule.	*/
		{
			BCB_DEBUG(2,"NOT adding BCB; no rule.", NULL);

			/*	No applicable valid construction rule.	*/

			result = 0;
			scratchExtensionBlock(bcbBlk);
			BCB_DEBUG_PROC("- sbsp_bcbAttach -> %d", result);
			return result;
		}

		BCB_DEBUG(2,"NOT adding BCB; no profile.", NULL);

		/*	No applicable valid construction rule.		*/

		result = 0;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- sbsp_bcbAttach -> %d", result);
		return result;
	}

	BCB_DEBUG(2, "Adding BCB", NULL);

	/* Step 2 - Populate the BCB ASB. */

	/* Step 2.1 - Grab the key name for this operation. */

	memcpy(bcbAsb->keyName, bcbRule.keyName, SBSP_KEY_NAME_LEN);

	/* Step 2.2 - Initialize the BCB ASB. */

	result = (prof->construct == NULL) ?
			sbsp_bcbDefaultConstruct(prof->suiteId, bcbBlk, bcbAsb)
			: prof->construct(bcbBlk, bcbAsb);

	if (result < 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbAttach: Can't construct ASB.", NULL);

		ADD_BCB_TX_FAIL(fromEid, 1, 0);

		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- sbsp_bcbAttach --> %d", result);
		return result;
	}

	/* Step 2.2 - Encrypt the target block and attach it. 		*/

	result = (prof->encrypt == NULL) ?
			sbsp_bcbDefaultEncrypt(prof->suiteId, bundle, bcbBlk,
			bcbAsb, xmitRate, &bytes) : prof->encrypt(bundle,
			bcbBlk, bcbAsb, xmitRate, &bytes);

	if (result < 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbAttach: Can't encrypt target block.",
				NULL);

		ADD_BCB_TX_FAIL(fromEid, 1, bytes);

		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- sbsp_bcbAttach --> %d", result);
		return result;
	}

	/* Step 3 - serialize the BCB ASB into the BCB blk. */

	/* Step 3.1 - Create a serialized version of the BCB ASB. */

	if ((serializedAsb = sbsp_serializeASB((uint32_t *)
			&(bcbBlk->dataLength), bcbAsb)) == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbAttach: Unable to serialize ASB.  \
bcbBlk->dataLength = %d", bcbBlk->dataLength);

		ADD_BCB_TX_FAIL(fromEid, 1, bytes);

		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- sbsp_bcbAttach --> %d", result);
		return result;
	}

	/* Step 3.2 - Copy serializedBCB ASB into the BCB extension block. */

	if ((result = serializeExtBlk(bcbBlk, NULL, (char *) serializedAsb))
			< 0)
	{
		bundle->corrupt = 1;
	}

	MRELEASE(serializedAsb);

	ADD_BCB_TX_PASS(fromEid, 1, bytes);

	BCB_DEBUG_PROC("- sbsp_bcbAttach --> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbOffer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		 a BCB for the block identified by tags 1 and 2 of
 * 		 the proposed BCB contents.  If the bundle already
 * 		 contains such a BCB (inserted by an upstream node)
 * 		 then the function simply returns 0.  Otherwise the
 * 		 function creates a BCB for the target block identified
 * 		 by blk->tag1 and tag2.  Note that only a placeholder
 * 		 BCB is constructed at this time; in effect, the
 * 		 placeholder BCB signals to later processing that
 * 		 such a BCB may or may not need to be attached to
 * 		 the bundle, depending on the final contents of other
 * 		 bundle blocks.  (Even encryption of the payload block is
 * 		 deferred, because in order ot make accurate decisions
 * 		 about in-place encryption we need the current data
 * 		 rate, which is only available at the time the bundle
 * 		 is dequeued.)
 *
 * \retval int 0 - The BCB was successfully created, or not needed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this new block.
 *
 * \par Assumptions:
 *      1. The blk has been pre-initialized such that the tag1-2 values
 *         are initialized sufficiently to identify the target block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *      2. tag3 is used to request destructive encryption (i.e., in-place).
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/07/15  E. Birrane     Update to profiles, Target Blocks, error checks
 *                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	sbsp_bcbOffer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr			bpSdr = getIonsdr();
	SbspOutboundBlock	asb;
	int			result = 0;

	BCB_DEBUG_PROC("+ sbsp_bcbOffer(%x, %x)",
                  (unsigned long) blk, (unsigned long) bundle);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure we have parameters...*/
	CHKERR(blk);
	CHKERR(bundle);

	blk->length = 0;	/*	Default.			*/
	blk->bytes = 0;		/*	Default.			*/

	/* Step 1.2 - Make sure that we are not trying an invalid security OP. */

	if ((blk->tag1 == BLOCK_TYPE_PRIMARY)||
		(blk->tag1 == BLOCK_TYPE_BCB))
	{
		/*	Can't have a BCB for these types of block.	*/
		BCB_DEBUG_ERR("x sbsp_bcbOffer - BCB can't target type %d",
				blk->tag1);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BCB_DEBUG_PROC("- sbsp_bcbOffer -> %d", result);
		return result;
	}

    /* Step 1.3 - Make sure OP(confidentiality, target) isn't already there. */
	if(sbsp_findBlock(bundle, BLOCK_TYPE_BCB, blk->tag1, blk->tag2,
			blk->tag3))
	{
		/*	Don't create a placeholder BCB for this block.	*/
		BCB_DEBUG_ERR("x sbsp_bcbOffer - BCB already exists for tgt %d",
				blk->tag1);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BCB_DEBUG_PROC("- sbsp_bcbOffer -> %d", result);
		return result;
	}

	/* Step 2 - Initialize BCB structures. */

	/* Step 2.1 - Populate the BCB ASB. */
	memset((char *) &asb, 0, sizeof(SbspOutboundBlock));
	sbsp_insertSecuritySource(bundle, &asb);
	asb.targetBlockType = blk->tag1;
	asb.encryptInPlace = blk->tag3;

	CHKERR(sdr_begin_xn(bpSdr));

	/* Step 2.2 Populate the BCB Extension Block. */
	blk->size = sizeof(SbspOutboundBlock);
	if((blk->object = sdr_malloc(bpSdr, blk->size)) == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbOffer: Failed to SDR allocate object \
of size: %d", blk->size);
		result = -1;
		sdr_cancel_xn(bpSdr);
		BCB_DEBUG_PROC("- sbsp_bcbOffer -> %d", result);
		return result;
	}

	/* Step 3 - Write the ASB into the block. */

	sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);

	sdr_end_xn(bpSdr);


	/* Step 4 - We always defer encryption until dequeue time. */

	/*	We can't construct the block at this time because we
	 *	can't assume that the target block exists in final
	 *	form yet.  (Except for the payload block, and we defer
	 *	encryption of the payload as well because we need the
	 *	current xmit data rate in order to encrypt that block
	 *	efficiently, and that rate is only known at dequeue
	 *	time.)  All we do is tell the BP agent that we want
	 *	this BCB to be considered for construction at the
	 *	time the bundle is dequeued for transmission.  For
	 *	this purpose, we stop after initializing the block's
	 *	scratchpad area, resulting in insertion of a
	 *	placeholder BCB for the target block.			*/

	result = 0;
	BCB_DEBUG_PROC("- sbsp_bcbOffer -> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbProcessOnDequeue
 *
 * \par Purpose: This callback determines whether or not a block of the
 * 		 type that is the target of this proposed BCB exists in
 * 		 the bundle and discards the BCB if it does not.  If the
 * 		 target block exists, the BCB is constructed.
 *
 * \retval int 0 - The block was successfully constructed or deleted.
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk       The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 * \param[in]      parm   The dequeue context.
 *
 * \par Notes:
 *      1. The target block must not have its content changed after this point.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/07/15  E. Birrane     Update to profiles, Target Blocks, error checks
 *                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	sbsp_bcbProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
		void *parm)
{
	SbspOutboundBlock	asb;
	int			result = 0;
	DequeueContext		*context = (DequeueContext *) parm;

	BCB_DEBUG_PROC("+ sbsp_bcbProcessOnDequeue(0x%x, 0x%x, 0x%x)",
			(unsigned long) blk, (unsigned long) bundle,
			(unsigned long) parm);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure arguments are valid. */

	if (bundle == NULL || parm == NULL || blk == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbProcessOnDequeue: Bad Args.", NULL);
		BCB_DEBUG_INFO("i sbsp_bcbProcessOnDequeue bundle 0x%x, \
parm 0x%x, blk 0x%x, blk->size %d", (unsigned long) bundle,
		(unsigned long) parm, (unsigned long)blk,
		(blk == NULL) ? 0 : blk->size);
		scratchExtensionBlock(blk);
		BCB_DEBUG_PROC("- sbsp_bcbProcessOnDequeue --> %d", -1);
		return -1;
	}

	/* Step 1.2 - Make sure we do not process someone else's BCB. */

	if (blk->blkProcFlags & BLK_FORWARDED_OPAQUE)
	{
		/*	Do nothing; the block's bytes are correct
		 *	and ready for transmission.  The block was
		 *	received when the bundle was received; it
		 *	was serialized in the recordExtensionBlocks()
		 *	function.					*/

		BCB_DEBUG_PROC("- sbsp_bcbProcessOnDequeue(%d)", result);
		return 0;
	} 

	/*
	 * Step 2 - Calculate the BCB for the target and attach it
	 *          to the bundle.
	 */

	sdr_read(getIonsdr(), (char *) &asb, blk->object, blk->size);
	result = sbsp_bcbAttach(bundle, blk, &asb, context->xmitRate);
	BCB_DEBUG_PROC("- sbsp_bcbProcessOnDequeue(%d)", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_bcbRelease
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

void    sbsp_bcbRelease(ExtensionBlock *blk)
{
	Sdr			sdr = getIonsdr();
	SbspOutboundBlock	asb;

	BCB_DEBUG_PROC("+ sbsp_bcbRelease(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		sdr_read(sdr, (char *) &asb, blk->object,
				sizeof(SbspOutboundBlock));
		if (asb.parmsData)
		{
			sdr_free(sdr, asb.parmsData);
		}

		if (asb.resultsData)
		{
			sdr_free(sdr, asb.resultsData);
		}

		sdr_free(sdr, blk->object);
	}

	BCB_DEBUG_PROC("- sbsp_bcbRelease(%c)", ' ');
}


Object sbsp_bcbStoreOverflow(uint32_t suite,
							  uint8_t	 *context,
		                      ZcoReader *dataReader,
							  uvast readOffset,
							  uvast writeOffset,
							  uvast cipherBufLen,
							  uvast cipherOverflow,
							  csi_val_t plaintext,
							  csi_val_t ciphertext,
							  csi_blocksize_t *blocksize)
{
	uvast chunkSize = 0;
	Sdr bpSdr = getIonsdr();
	Object cipherBuffer = 0;

	/* Step 4: Create SDR space and store any extra encryption that won't fit in the payload. */
	ciphertext.len = 0;
	ciphertext.contents = NULL;
	if((readOffset < blocksize->plaintextLen) || (cipherOverflow > 0))
	{
		Object cipherBuffer = 0;
		uvast length = cipherBufLen - blocksize->plaintextLen;

		writeOffset = 0;
		if((cipherBuffer = sdr_malloc(bpSdr, length * 2)) == 0)
		{

		}

		if(cipherOverflow > 0)
		{
			sdr_write(bpSdr, cipherBuffer + writeOffset, (char *) ciphertext.contents + ciphertext.len - cipherOverflow, cipherOverflow);
			writeOffset += cipherOverflow;
		}

		while(readOffset < blocksize->plaintextLen)
		{
			if((readOffset + chunkSize) < blocksize->plaintextLen)
			{
				plaintext.len = zco_transmit(bpSdr, dataReader, chunkSize, (char *) plaintext.contents);
			}
			else
			{
				plaintext.len = zco_transmit(bpSdr, dataReader, blocksize->plaintextLen - readOffset, (char*) plaintext.contents);
			}
			readOffset += plaintext.len;

			ciphertext = csi_crypt_update(suite, context, CSI_SVC_ENCRYPT, plaintext);

			if((ciphertext.contents == NULL) || (ciphertext.len == 0))
			{
				BCB_DEBUG_ERR("x sbsp_bcbCiphertextToSdr: Could not encrypt.", plaintext.len, chunkSize);
				BCB_DEBUG_PROC("- sbsp_bcbCiphertextToSdr--> -1", NULL);
				return 0;
			}

			sdr_write(bpSdr, cipherBuffer + writeOffset, (char *) ciphertext.contents, ciphertext.len);
			writeOffset += ciphertext.len;
			MRELEASE(ciphertext.contents);
		}
	}

	return cipherBuffer;
}



/*
 * 		 *  Step 3.2 - Write ciphertext to the payload. We assume the ciphertext length will never be
		 *             more than 2x the payload length. If the ciphertext is beyond what can be
		 *             stored in the existing payload allocation, capture it or later use in the
		 *             BCB.
 *
 */


int32_t	sbsp_bcbUpdatePayloadInPlace(uint32_t suite, csi_cipherparms_t parms,
		uint8_t	 *context, csi_blocksize_t *blocksize, Object dataObj,
		ZcoReader *dataReader, uvast cipherBufLen, Object *cipherBuffer,
		uint8_t function)
{
	Sdr		bpSdr = getIonsdr();
	csi_val_t	plaintext[2];
	csi_val_t	ciphertext;
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
	ciphertext.len = 0;
	ciphertext.contents = NULL;

	/* Step 1: Allocate read buffers. */
	if((plaintext[0].contents = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadInPlace - Can't allocate buffer of size %d.",
				chunkSize);
		return -1;
	}
	if((plaintext[1].contents = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadInPlace - Can't allocate buffer of size %d.",
				chunkSize);
		MRELEASE(plaintext[0].contents);
		return -1;
	}


	/* Step 2 - Perform priming read of payload to prep for encryption. */
	chunkSize = blocksize->chunkSize;

/* \todo: Check return value for zco_transmit */
	/* Step 2.1: Perform priming read. */
	if(blocksize->plaintextLen <= blocksize->chunkSize)
	{
		plaintext[0].len = zco_transmit(bpSdr, dataReader, blocksize->plaintextLen, (char *) plaintext[0].contents);
		plaintext[1].len = 0;
		readOffset = plaintext[0].len;
		writeOffset = 0;
	}
	else if(blocksize->plaintextLen <= (2*(blocksize->chunkSize)))
	{
		plaintext[0].len = zco_transmit(bpSdr, dataReader, chunkSize, (char *) plaintext[0].contents);
		plaintext[1].len = zco_transmit(bpSdr, dataReader, blocksize->plaintextLen - chunkSize, (char *) plaintext[1].contents);
		readOffset = plaintext[0].len + plaintext[1].len;
		writeOffset = 0;
	}
	else
	{
		plaintext[0].len = zco_transmit(bpSdr, dataReader, chunkSize, (char *) plaintext[0].contents);
		plaintext[1].len = zco_transmit(bpSdr, dataReader, chunkSize, (char *) plaintext[0].contents);
		readOffset = plaintext[0].len + plaintext[1].len;
		writeOffset = 0;
	}

	/* Step 3: Walk through payload writing ciphertext. */

	if((csi_crypt_start(suite, context, parms)) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadInPlace: Could not start context.", NULL);
		MRELEASE(plaintext[0].contents);
		MRELEASE(plaintext[1].contents);
		MRELEASE(ciphertext.contents);
		return -1;
	}

 	while (writeOffset < blocksize->plaintextLen)
	{

 		/* Step 3.1: Generate ciphertext from earliest plaintext
		 * buffer. */

 		/* Step 3.1: If there is no data left to encrypt... */
 		if(plaintext[cur_idx].len == 0)
 		{
 			break;
 		}

 		ciphertext = csi_crypt_update(suite, context, function,
				plaintext[cur_idx]);
		if((ciphertext.contents == NULL) || (ciphertext.len == 0))
		{
			BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadInPlace: Could not encrypt.", NULL);
			MRELEASE(plaintext[0].contents);
			MRELEASE(plaintext[1].contents);
			MRELEASE(ciphertext.contents);
			BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadInPlace--> -1", NULL);
			return -1;
		}

		/*
		 * Step 3.2: If the ciphertext will no longer fit into the
		 * existing payload, then just copy the bits of ciphertext
		 * that will fit and save the rest for later.
		 */
		if((writeOffset + ciphertext.len) > blocksize->plaintextLen)
		{
			if((zco_revise(bpSdr, dataObj, writeOffset,
					(char *) ciphertext.contents,
					blocksize->plaintextLen - writeOffset))
				       	== -1)
			{
				BCB_DEBUG_ERR("sbsp_bcbUpdatePayloadInPlace: Failed call to zco_revise.", NULL);
				break;
			}

			cipherOverflow = ciphertext.len -
					(blocksize->plaintextLen - writeOffset);
			writeOffset = blocksize->plaintextLen;
		}
		else
		{
			if((zco_revise(bpSdr, dataObj, writeOffset,
					(char *) ciphertext.contents,
					ciphertext.len)) == -1)
			{
				BCB_DEBUG_ERR("sbsp_bcbUpdatePayloadInPlace: Failed call to zco_revise.", NULL);
				break;
			}

			writeOffset += ciphertext.len;

			/* Fill up the next read buffer */
			if(readOffset >= blocksize->plaintextLen)
			{
				plaintext[cur_idx].len = 0;
			}
			else if((readOffset + chunkSize)
					< blocksize->plaintextLen)
			{
				plaintext[cur_idx].len = zco_transmit(bpSdr,
						dataReader, chunkSize, (char *)
						plaintext[cur_idx].contents);
			}
			else
			{
				plaintext[cur_idx].len = zco_transmit(bpSdr,
						dataReader,
						blocksize->plaintextLen
						- readOffset, (char*)
						plaintext[cur_idx].contents);
			}

			readOffset += plaintext[cur_idx].len;
			cur_idx = 1 - cur_idx;
		}

		MRELEASE(ciphertext.contents);
	}

	/* Step 4: Create SDR space and store any extra encryption that won't fit in the payload. */
	*cipherBuffer = sbsp_bcbStoreOverflow(suite, context, dataReader,
			readOffset, writeOffset, cipherBufLen, cipherOverflow,
			plaintext[0], ciphertext, blocksize);

	MRELEASE(plaintext[0].contents);
	MRELEASE(plaintext[1].contents);

	if((csi_crypt_finish(suite, context, function, &parms)) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadInPlace: Could not finish CSI context.", NULL);
		if (ciphertext.contents)
		{
			MRELEASE(ciphertext.contents);
		}

		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadInPlace--> -1", NULL);
		return -1;
	}

	BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadInPlace--> 0", NULL);

	return 0;
}

/**
 * > 0 Success
 * 0 BCB error
 * -1 System error
 */
int32_t sbsp_bcbUpdatePayloadFromSdr(uint32_t suite, csi_cipherparms_t parms,
		uint8_t *context, csi_blocksize_t *blocksize, Object dataObj,
		ZcoReader *dataReader, uvast cipherBufLen, Object *cipherZco,
		uint8_t function)
{
	Sdr bpSdr = getIonsdr();
	csi_val_t plaintext;
	csi_val_t ciphertext;
	uvast chunkSize = 0;
	uvast bytesRemaining = 0;
	Object cipherBuffer = 0;
	uvast    writeOffset = 0;
	SdrUsageSummary summary;
	uvast memmax = 0;

	CHKERR(context);
	CHKERR(blocksize);
	CHKERR(dataReader);
	CHKERR(cipherZco);

	BCB_DEBUG_PROC("+ sbsp_bcbUpdatePayloadFromSdr(%d," ADDR_FIELDSPEC
			"," ADDR_FIELDSPEC "," UVAST_FIELDSPEC ","
			ADDR_FIELDSPEC "," UVAST_FIELDSPEC "," ADDR_FIELDSPEC
			", %d)", suite, (uaddr) context, (uaddr) blocksize,
			(uvast) dataObj, (uaddr) dataReader, cipherBufLen,
			(uaddr) cipherZco, function);

	/* Step 1 - Get information about the SDR storage space. */
	sdr_usage(bpSdr, &summary);

	// Set the maximum buffer len to 1/2 of the available space.
	// Note: >> 1 means divide by 2.
	memmax = (summary.largePoolFree + summary.unusedSize) >> (uvast) 1;

	/* Step 2 - See if the ciphertext will fit into the existing SDR space. */
	if(cipherBufLen > memmax)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr: Buffer len \
will not fit. " UVAST_FIELDSPEC " > " UVAST_FIELDSPEC, cipherBufLen, memmax);
		sdr_report(&summary);
		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> 0", NULL);

		return 0;
	}

	if((cipherBuffer = sdr_malloc(bpSdr, cipherBufLen)) == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr: Cannot \
allocate " UVAST_FIELDSPEC " from SDR.", NULL);
		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);

		return -1;
	}

	/* Pass additive inverse of cipherBufLen to tell ZCO that space has already been
	 * allocated.
	 */
	if((*cipherZco = zco_create(bpSdr, ZcoSdrSource, cipherBuffer, 0, 0 - cipherBufLen, ZcoOutbound)) == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr: Cannot create zco.", NULL);

		sdr_free(bpSdr, cipherBuffer);

		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);

		return -1;
	}


	chunkSize = blocksize->chunkSize;
	bytesRemaining = blocksize->plaintextLen;

	/* Step 1: Allocate read buffers. */
	if((plaintext.contents = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr - Can't allocate buffer of size %d.",
				chunkSize);

		sdr_free(bpSdr, cipherBuffer);
		zco_destroy(bpSdr, *cipherZco);
		*cipherZco = 0;
		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);

		return -1;
	}

	chunkSize = blocksize->chunkSize;

	/* Step 2 - Perform priming read of payload to prep for encryption. */
	if((csi_crypt_start(suite, context, parms)) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr - Can't start context.", NULL);

		MRELEASE(plaintext.contents);
		sdr_free(bpSdr, cipherBuffer);
		zco_destroy(bpSdr, *cipherZco);
		*cipherZco = 0;

		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);
		return -1;
	}

	/* Step 3: Walk through payload writing ciphertext. */
 	while (bytesRemaining > 0)
	{

 		/* Step 3.1: Prep for this encrypting iteration. */
 		memset(plaintext.contents, 0, chunkSize);
 		if(bytesRemaining < chunkSize)
 		{
 			chunkSize = bytesRemaining;
 		}

 		/* Step 3.2: Generate ciphertext from earliest plaintext buffer. */
 		plaintext.len = zco_transmit(bpSdr, dataReader, chunkSize, (char *) plaintext.contents);
 		if(plaintext.len <= 0)
 		{
 			BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr - Can't do priming read of length %d.",
 						  chunkSize);

 			MRELEASE(plaintext.contents);
 			sdr_free(bpSdr, cipherBuffer);
 			zco_destroy(bpSdr, *cipherZco);
 			*cipherZco = 0;

			BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);

 			return -1;
 		}

 		bytesRemaining -= plaintext.len;

 		ciphertext = csi_crypt_update(suite, context, function, plaintext);

		if((ciphertext.contents == NULL) || (ciphertext.len == 0))
		{
			BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr: Could not encrypt.", plaintext.len, chunkSize);
			MRELEASE(plaintext.contents);
			MRELEASE(ciphertext.contents);
			sdr_free(bpSdr, cipherBuffer);
			zco_destroy(bpSdr, *cipherZco);
			*cipherZco = 0;

			BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);
			return -1;
		}

		sdr_write(bpSdr, cipherBuffer + writeOffset, (char *) ciphertext.contents, ciphertext.len);
		writeOffset += ciphertext.len;

		MRELEASE(ciphertext.contents);
	}

	MRELEASE(plaintext.contents);

	if(csi_crypt_finish(suite, context, function, &parms) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromSdr: Could not finish context.", NULL);
		sdr_free(bpSdr, cipherBuffer);
		zco_destroy(bpSdr, *cipherZco);
		*cipherZco = 0;

		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromSdr--> -1", NULL);
		return -1;
	}

	BCB_DEBUG_PROC("- sbsp_bcbCiphertextToSdr--> 1", NULL);

	return 1;
}

/**
 * > 0 Success
 * 0 processing error
 * -1 system error
 */
int32_t sbsp_bcbUpdatePayloadFromFile(uint32_t suite, csi_cipherparms_t parms,
		uint8_t *context, csi_blocksize_t *blocksize, Object dataObj,
		ZcoReader *dataReader, uvast cipherBufLen, Object *cipherZco,
		uint8_t function)
{
	Sdr bpSdr = getIonsdr();
	csi_val_t plaintext;
	csi_val_t ciphertext;
	uvast chunkSize = 0;
	uvast bytesRemaining = 0;
	Object fileRef = 0;

	BCB_DEBUG_PROC("+ sbsp_bcbUpdatePayloadFromFile(%d," ADDR_FIELDSPEC
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
	if((plaintext.contents = MTAKE(chunkSize)) == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromFile: Can't allocate buffer of size %d.",
				chunkSize);
		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> -1", NULL);

		return -1;
	}

	if(csi_crypt_start(suite, context, parms) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromFile: Can't start context", NULL);
		MRELEASE(plaintext.contents);
		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> -1", NULL);
		return -1;
	}

	/* Step 2: Walk through payload writing ciphertext. */
 	while (bytesRemaining > 0)
	{

 		/* Step 3.1: Prep for this encrypting iteration. */
 		memset(plaintext.contents, 0, chunkSize);
 		if(bytesRemaining < chunkSize)
 		{
 			chunkSize = bytesRemaining;
 		}

 		/* Step 3.2: Generate ciphertext from earliest plaintext buffer. */
 		plaintext.len = zco_transmit(bpSdr, dataReader, chunkSize, (char *) plaintext.contents);
 		if(plaintext.len <= 0)
 		{
 			BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromFile: Can't do priming read of length %d.",
 						  chunkSize);

 			MRELEASE(plaintext.contents);
			BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> -1", NULL);
 			return -1;
 		}

 		bytesRemaining -= plaintext.len;

 		ciphertext = csi_crypt_update(suite, context, function, plaintext);

		if((ciphertext.contents == NULL) || (ciphertext.len == 0))
		{
			BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromFile: Could not encrypt.", plaintext.len, chunkSize);
			MRELEASE(plaintext.contents);
			MRELEASE(ciphertext.contents);
			BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> -1", NULL);
			return -1;
		}

		if(sbsp_transferToZcoFileSource(bpSdr, cipherZco, &fileRef, BCB_FILENAME,
									     (char *) ciphertext.contents, ciphertext.len) <= 0)
		{
			BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromFile: Transfer of chunk has failed..", NULL);
			MRELEASE(ciphertext.contents);
			MRELEASE(plaintext.contents);
			BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> -1", NULL);
			return -1;
		}

		/*
		 * \todo: Consider if a sleep here will help the filesystem catch up.
		 *
		 * microsnooze(1000);
		 */

		MRELEASE(ciphertext.contents);
	}

	MRELEASE(plaintext.contents);


	if(csi_crypt_finish(suite, context, function, &parms) == ERROR)
	{
		BCB_DEBUG_ERR("x sbsp_bcbUpdatePayloadFromFile: Could not finish context.", NULL);

		BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> -1", NULL);
		return -1;
	}


	BCB_DEBUG_PROC("- sbsp_bcbUpdatePayloadFromFile--> 1", NULL);

	return 1;
}


/**
 * Step 6: Replace plaintext with ciphertext.
 *         If we are encrypting, we have to make a decision on how we
 *         generate the ciphertext. We will need to generate ciphertext
 *         separate from the existing user payload, as it is possible
 *         that payload ZCO is a ZCO to a file on the user system and
 *         encrypting the payload in place could, actually, encrypt the
 *         data on the user's system. That's bad.
 *
 *         So we have three choices for housing the ciphertext:
 *         1. Encrypt in place, IF plaintext is not in a file.
 *         1. Built a ZCO out of SDR dataspace (fast, but space limited)
 *         2. Built a ZCO to a temp file (slow, but accomodate large data)
 *
 *         We select which option based on the estimated size of the
 *         ciphertext and the space available in the SDR. If the ciphertext
 *         is less than 50% of the free SDR spade, we will use a ZCO to the
 *         SDR heap. 50% is selected arbitrarily. If method 1 fails due to
 *         an sdr_malloc call, we will, instead, default to method 2. Method 1
 *         could fail due to SDR heap fragmentation even if the SDR heap would
 *         otherwise have sufficient space to store the ciphertext.
 */


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbHelper
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
int	sbsp_bcbHelper(Object *dataObj, uint32_t chunkSize, uint32_t suite,
		csi_val_t key, csi_cipherparms_t parms, uint8_t encryptInPlace,
	       	size_t xmitRate, uint8_t function)
{
	Sdr		bpSdr = getIonsdr();
	csi_blocksize_t blocksize;
	uint32_t	cipherBufLen = 0;
	uint32_t	bytesRemaining = 0;
	ZcoReader	dataReader;
	uint8_t		*context = NULL;
	Object		cipherBuffer = 0;

	BCB_DEBUG_INFO("+ sbsp_bcbHelper(0x%x, %d, %d, [0x%x, %d])",
			       (unsigned long) dataObj, chunkSize, suite,
				   (unsigned long) key.contents, key.len);

	/* Step 0 - Sanity Check. */
	CHKERR(key.contents);

	if ((sdr_begin_xn(bpSdr)) == 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbHelper - Can't start txn.", NULL);
		BCB_DEBUG_PROC("- sbsp_bcbHelper--> NULL", NULL);
		return -1;
	}

	/*
	 * Step 3 - Setup playback of data from the data object.
	 *          The data object is the target block.
	 */

	if ((bytesRemaining = zco_length(bpSdr, *dataObj)) <= 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbHelper - data object has no length.",
				NULL);
		sdr_cancel_xn(bpSdr);
		BCB_DEBUG_PROC("- sbsp_bcbHelper--> NULL", NULL);
		return -1;
	}

	zco_start_transmitting(*dataObj, &dataReader);

	BCB_DEBUG_INFO("i sbsp_bcbHelper: bundle size is %d", bytesRemaining);

	/* Step 4 - Grab and initialize a crypto context. */
	if ((context = csi_ctx_init(suite, key, function)) == NULL)
	{
		BCB_DEBUG_ERR("x sbsp_bcbHelper - Can't get context.", NULL);
		sdr_cancel_xn(bpSdr);
		BCB_DEBUG_PROC("- sbsp_bcbHelper--> NULL", NULL);
		return -1;
	}

	/* Step 5: Calculate the maximum size of the ciphertext. */
	memset(&blocksize, 0, sizeof(blocksize));
	blocksize.chunkSize = chunkSize;
	blocksize.keySize = key.len;
	blocksize.plaintextLen = bytesRemaining;

	if ((cipherBufLen = csi_crypt_res_len(suite, context, blocksize,
			function)) <= 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbHelper: Predicted bad ciphertext \
length: %d", cipherBufLen);
		csi_ctx_free(suite, context);
		sdr_cancel_xn(bpSdr);

		BCB_DEBUG_PROC("- sbsp_bcbHelper --> %d", -1);
		return -1;
	}

	BCB_DEBUG_INFO("i sbsp_bcbHelper - CipherBufLen is %d", cipherBufLen);

	if ((function == CSI_SVC_ENCRYPT) || (function == CSI_SVC_DECRYPT))
	{
		if (encryptInPlace || xmitRate == 0)
		{
			if ((sbsp_bcbUpdatePayloadInPlace(suite, parms, context,
					&blocksize, *dataObj, &dataReader,
					cipherBufLen, &cipherBuffer, function))
					!= 0)
			{
				BCB_DEBUG_ERR("x sbsp_bcbHelper: Cannot update \
ciphertext in place.", NULL);
				csi_ctx_free(suite, context);
				sdr_cancel_xn(bpSdr);
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
				result = sbsp_bcbUpdatePayloadFromSdr(suite,
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

				result = sbsp_bcbUpdatePayloadFromFile(suite,
						parms, context, &blocksize,
						*dataObj, &dataReader,
						cipherBufLen, &cipherZco,
						function);
			}

			if (result <= 0)
			{
				BCB_DEBUG_ERR("x sbsp_bcbHelper: Cannot \
allocate ZCO for ciphertext of size " UVAST_FIELDSPEC, cipherBufLen);
				csi_ctx_free(suite, context);
				sdr_cancel_xn(bpSdr);

				BCB_DEBUG_PROC("- sbsp_bcbHelper --> %d", -1);
				return -1;
			}

			zco_destroy(bpSdr, *dataObj);
			*dataObj = cipherZco;
		}
	}
	else
	{
		BCB_DEBUG_ERR("x sbsp_bcbHelper: Invalid service: %d",
				function);
		csi_ctx_free(suite, context);
		sdr_cancel_xn(bpSdr);

		BCB_DEBUG_PROC("- sbsp_bcbHelper --> %d", -1);
		return -1;
	}

	csi_ctx_free(suite, context);

	if (sdr_end_xn(bpSdr) < 0)
	{
		BCB_DEBUG_ERR("x sbsp_bcbHelper: Can't end encrypt txn.", NULL);
		return -1;
	}

	return 0;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bcbReview
 *
 * \par Purpose: This callback is called once for each acquired bundle.
 *		 It scans through all BCB security rules and ensures
		 that each required BCB is included among the bundle's
		 extension blocks.
 *
 * \retval int -- 1 - All required BCBs are present.
 *                0 - At least one required BCB is missing.
 *               -1 - There was a system error.
 *
 * \param[in]      wk   The work area associated with this bundle acquisition.
 *
 * \par Notes:
 *****************************************************************************/

int	sbsp_bcbReview(AcqWorkArea *wk)
{
	Sdr	sdr = getIonsdr();
	char	secDestEid[32];
	Object	rules;
	Object	elt;
	Object	ruleAddr;
		OBJ_POINTER(BspBcbRule, rule);
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 1;	/*	Default: no problem.		*/

	BCB_DEBUG_PROC("+ sbsp_bcbReview(%x)", (unsigned long) wk);

	CHKERR(wk);

	isprintf(secDestEid, sizeof secDestEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());

	/* Check if security datbase exists */
	rules = sec_get_bspBcbRuleList();
	if (rules == 0)
	{
		BCB_DEBUG_PROC("- sbsp_bcbReview -> no security database");
		return result;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleAddr);
		oK(sdr_string_read(sdr, eidBuffer, rule->destEid));
		if (strcmp(eidBuffer, secDestEid) != 0)
		{
			/*	No requirement against local node.	*/

			continue;
		}

		/*	A block satisfying this rule is required.	*/

		oK(sdr_string_read(sdr, eidBuffer, rule->securitySrcEid));
		result = sbsp_requiredBlockExists(wk, BLOCK_TYPE_BCB,
				rule->blockTypeNbr, eidBuffer);
		if (result != 1)
		{
			break;
		}
	}

	sdr_exit_xn(sdr);

	BCB_DEBUG_PROC("- sbsp_bcbReview -> %d", result);

	return result;
}
