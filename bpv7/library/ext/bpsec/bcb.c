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
 **              bcbOffer
 **              bcbProcessOnDequeue
 **              bcbRelease
 **              bcbCopy
 **                                                  bcbAcquire
 **                                                  bcbReview
 **                                                  bcbDecrypt
 **                                                  bcbRecord
 **                                                  bcbClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    ENCRYPT SIDE                     DECRYPT SIDE
 **
 **              bcbDefaultConstruct
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
 *****************************************************************************/

#include "zco.h"
#include "csi.h"
#include "bpsec_util.h"
#include "bcb.h"
#include "bpsec_instr.h"

#if (BCB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

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
 *  06/04/09  E. Birrane           Initial Implementation.
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
 * \par Function Name: bcbDecrypt
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
 *            S. Burleigh    Port from pibCopy
 *  11/10/15  E. Birrane     Update to profiles. [Secure DTN implementation
 *                           (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bcbDecrypt(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle			*bundle;
	BpsecInboundBlock	*asb = NULL;
	BpsecInboundTarget	*target;
	char			*fromEid;
	char			*toEid;
	BPsecBcbRule		bpsecRule;
	BcbProfile		*prof = NULL;
	int			result;
	uvast			bytes = 0;

	BCB_DEBUG_PROC("+ bcbDecrypt(0x%x, 0x%x)", (unsigned long) blk,
			(unsigned long) wk);

	if (blk == NULL || blk->object == NULL || wk == NULL)
	{
		ADD_BCB_RX_FAIL(NULL, 1, 0);

		BCB_DEBUG_ERR("x bcbDecrypt:  Blocks are NULL. %x",
				(unsigned long) blk);
		result = 0;
		BCB_DEBUG_PROC("- bcbDecrypt --> %d", result);
		return result;
	}

	/*	The security destination is always the final
	 *	destination of the bundle.  The security source is
	 *	normally the original source of the bundle, but a BCB
	 *	can alternatively be inserted at any point in the
	 *	bundle's end-to-end path.				*/

	bundle = &(wk->bundle);
	asb = (BpsecInboundBlock *) (blk->object);
	if (bpsec_getInboundTarget(asb->targets, &target) < 0)
	{
		BCB_DEBUG_ERR("x bcbDecrypt - Can't get target.", NULL);
		BCB_DEBUG_PROC("- bcbDecrypt--> 0", NULL);
		return 0;
	}

	if (asb->securitySource.schemeCodeNbr)	/*	Waypoint source.*/
	{
		if (readEid(&(asb->securitySource), &fromEid) < 0)
		{
			ADD_BCB_RX_FAIL(NULL, 1, 0);

			return 0;
		}
	}
	else
	{
		if (readEid(&(bundle->id.source), &fromEid) < 0)
		{
			ADD_BCB_RX_FAIL(NULL, 1, 0);
			return 0;
		}
	}

	if (readEid(&(bundle->destination), &toEid) < 0)
	{
		ADD_BCB_RX_FAIL(fromEid, 1, 0);

		MRELEASE(fromEid);
		return 0;
	}

	/*	Given sender & receiver EIDs, get applicable BCB rule.	*/

	prof = bcbGetProfile(fromEid, toEid, target->targetBlockType,
			&bpsecRule);
	MRELEASE(toEid);

	if (prof == NULL)
	{
		/*	We can't decrypt this block.			*/

		if (bpsecRule.destEid == 0)	/*	No rule.	*/
		{
			/*	We don't care about decrypting the
			 *	target block for this BCB, but we
			 *	preserve the BCB in case somebody
			 *	else does.				*/

			BCB_DEBUG_INFO("- bcbDecrypt - No rule.", NULL);

			ADD_BCB_RX_MISS(fromEid, 1, bytes);
			MRELEASE(fromEid);

			result = 1;
			BCB_DEBUG_PROC("- bcbDecrypt --> %d", result);
			return result;		/*	No information.	*/
		}

		/*	Rule is found, but we don't have this CS.
		 *	We cannot decrypt the block, so the block
		 *	is -- in effect -- malformed.			*/

		ADD_BCB_RX_FAIL(fromEid, 1, bytes);
		MRELEASE(fromEid);

		discardExtensionBlock(blk);
	 	BCB_DEBUG_ERR("- bcbDecrypt - Profile missing!", NULL);
		result = 0;
		BCB_DEBUG_PROC("- bcbDecrypt --> %d", result);
		return 0;			/*	Block malformed.*/
	}

	/*	Fill in missing information in the scratchpad area.	*/
	memcpy(asb->keyName, bpsecRule.keyName, BPSEC_KEY_NAME_LEN);

	/*	Invoke ciphersuite-specific check procedure.		*/
	result = (prof->decrypt == NULL) ?
		 bcbDefaultDecrypt(prof->suiteId, wk, blk, &bytes, fromEid) :
		 prof->decrypt(prof->suiteId, wk, blk, &bytes, fromEid);

	/*	Discard the BCB if the local node is the destination
	 *	of the bundle or if decryption failed (meaning the
	 *	block is malformed and therefore the bundle is
	 *	malformed); otherwise make sure the BCB is retained.	*/

	if (result == 0 || bpsec_destinationIsLocal(&(wk->bundle)))
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
	}

	MRELEASE(fromEid);
	BCB_DEBUG_PROC("- bcbDecrypt --> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bcbDefaultConstruct
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
int	bcbDefaultConstruct(uint32_t suite, ExtensionBlock *blk,
		BpsecOutboundBlock *asb)
{
	Sdr	sdr = getIonsdr();

	CHKERR(blk);
	CHKERR(asb);

	/* Step 1: Populate block-instance-agnostic parts of the ASB. */

	if (asb->targets == 0)
	{
		asb->targets = sdr_list_create(sdr);
	}

	asb->contextId = suite;
	if (asb->parmsData == 0)
	{
		asb->parmsData = sdr_list_create(sdr);
	}

	/* Step 2: Populate instance-specific parts of the ASB. */

	asb->contextFlags = BPSEC_ASB_PARM;
	if (asb->targets == 0 || asb->parmsData == 0)
	{
		putErrmsg("Can't construct BCB.", NULL);
		return -1;
	}

	return 0;
}

/*
 * Assume the asb key has been udpated with the key to use for decrypt.
 */
int	bcbDefaultDecrypt(uint32_t suite, AcqWorkArea *wk, AcqExtBlock *blk,
		uvast *bytes, char *fromEid)
{
	BpsecInboundBlock	*asb;
	BpsecInboundTarget	*target;
	sci_inbound_tlv		longtermKey;
	sci_inbound_tlv		sessionKeyInfo;
	sci_inbound_tlv		sessionKeyClear;
	sci_inbound_parms	parms;

	BCB_DEBUG_INFO("+ bcbDefaultDecrypt(%d, 0x%x, 0x%x)", suite,
			(unsigned long) wk, (unsigned long) blk);

	/* Step 0 - Sanity Checks. */
	CHKERR(wk && blk && bytes);
	*bytes = 0;

	memset(&longtermKey, 0, sizeof(sci_inbound_tlv));
	memset(&sessionKeyInfo, 0, sizeof(sci_inbound_tlv));
	memset(&sessionKeyClear, 0, sizeof(sci_inbound_tlv));

	/* Step 1 - Initialization */
	asb = (BpsecInboundBlock *) (blk->object);
	if (bpsec_getInboundTarget(asb->targets, &target) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultDecrypt - Can't get target.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultDecrypt--> 0", NULL);
		return 0;
	}

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

	switch (target->targetBlockType)
	{
	case PayloadBlk:
		*bytes = wk->bundle.payload.length;
		if (bcbHelper(&(wk->bundle.payload.content),
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

	default:
		BCB_DEBUG_ERR("x bcbDefaultDecrypt: Can't decrypt block \
type %d: canonicalization not implemented.", target->targetBlockType);
		BCB_DEBUG_PROC("- bcbDefaultDecrypt--> NULL", NULL);
		MRELEASE(sessionKeyClear.value);
		return 0;
	}

	MRELEASE(sessionKeyClear.value);
	return 1;
}

uint32_t	bcbDefaultEncrypt(uint32_t suite, Bundle *bundle,
			ExtensionBlock *blk, BpsecOutboundBlock *asb,
			size_t xmitRate, uvast *bytes, char *toEid)
{
	Sdr			sdr = getIonsdr();
	BpsecOutboundTarget	target;
	sci_inbound_tlv		sessionKey;
	sci_inbound_tlv		encryptedSessionKey;
	sci_inbound_tlv		longtermKey;
	sci_inbound_parms	parms;

	BCB_DEBUG_INFO("+ bcbDefaultEncrypt(%d, 0x%x, 0x%x, 0x%x", suite,
			(unsigned long) bundle, (unsigned long) blk,
			(unsigned long) asb);

	/* Step 0 - Sanity Checks. */
	CHKERR(bundle && blk && asb && bytes);
	*bytes = 0;

	memset(&sessionKey, 0, sizeof(sci_inbound_tlv));
	memset(&encryptedSessionKey, 0, sizeof(sci_inbound_tlv));
	memset(&longtermKey, 0, sizeof(sci_inbound_tlv));
	if (bpsec_getOutboundTarget(sdr, asb->targets, &target) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt - Can't get target.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> 0", NULL);
		return 0;
	}

	/*
	 * Step 1 - Make sure we have a long-term key that we can use to
	 * protect the session key.
	 */

	longtermKey = bpsec_retrieveKey(asb->keyName);
	if (longtermKey.length == 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt - Can't get longterm \
key.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> 0", NULL);
		return 0;
	}

	/* Step 2 - Grab session key to use for the encryption. */
	sessionKey = sci_crypt_parm_get(suite, CSI_PARM_BEK);
	if ((sessionKey.value == NULL) || (sessionKey.length == 0))
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt - Can't get session \
key.", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
		return 0;
	}

	/* Step 3 - Grab cipher parms to seed encryption.*/
	memset(&parms, 0, sizeof(parms));
	parms.iv = sci_crypt_parm_get(suite, CSI_PARM_IV);
	parms.salt = sci_crypt_parm_get(suite, CSI_PARM_SALT);

	/*
	 * Step 4 - Use the long-term key to encrypt the
	 *          session key. We assume session key sizes fit into
	 *          memory and do not need to be chunked. We want to
	 *          make sure we can encrypt all the keys before doing
	 *          surgery on the target block itself.
	 */

	if ((sci_crypt_key(suite, CSI_SVC_ENCRYPT, &parms, longtermKey,
			sessionKey, &encryptedSessionKey)) == ERROR)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Could not decrypt \
session key", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> 0", NULL);
		return 0;
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
		return 0;
	}

	/* Step 5 - Encrypt the target block. */
	switch (target.targetBlockType)
	{
	case PayloadBlk:
		*bytes = bundle->payload.length;
		if (bcbHelper(&(bundle->payload.content), csi_blocksize(suite),
				suite, sessionKey, parms, asb->encryptInPlace,
				xmitRate, CSI_SVC_ENCRYPT) < 0)
		{
			BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't encrypt \
payload.", NULL);
			BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
			MRELEASE(longtermKey.value);
			MRELEASE(sessionKey.value);
			MRELEASE(encryptedSessionKey.value);
			sci_cipherparms_free(parms);
			return 0;
		}

		break;

	default:
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't encrypt block \
type %d: canonicalization not implemented.", target.targetBlockType);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt--> NULL", NULL);
		MRELEASE(longtermKey.value);
		MRELEASE(sessionKey.value);
		MRELEASE(encryptedSessionKey.value);
		sci_cipherparms_free(parms);
		return 0;
	}

	/* Step 6 - Free plaintext keys post-encryption. */
	MRELEASE(longtermKey.value);
	MRELEASE(sessionKey.value);

	/*
	 * Step 7 - Place the encrypted session key in the
	 *         results field of the BCB's first target.
	 */

	encryptedSessionKey.id = CSI_PARM_KEYINFO;
	if (bpsec_write_one_result(sdr, asb, &encryptedSessionKey) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't allocate heap \
space for ASB target's result.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt --> %d", 0);
		MRELEASE(encryptedSessionKey.value);
		sci_cipherparms_free(parms);
		return 0;
	}

	MRELEASE(encryptedSessionKey.value);

	/* Step 8 - Place the parameters in the appropriate BCB field. */

	if (bpsec_write_parms(sdr, asb, &parms) < 0)
	{
		BCB_DEBUG_ERR("x bcbDefaultEncrypt: Can't allocate heap \
space for ASN parms data.", NULL);
		BCB_DEBUG_PROC("- bcbDefaultEncrypt --> %d", 0);
		sci_cipherparms_free(parms);
		return 0;
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
 * \par Function Name: bcbGetProfile
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

BcbProfile	*bcbGetProfile(char *secSrc, char *secDest,
			BpBlockType secTgtType, BPsecBcbRule *bpsecRule)
{
	Sdr		sdr = getIonsdr();
	Object		ruleAddr;
	Object		ruleElt;
	BcbProfile 	*prof = NULL;

	BCB_DEBUG_PROC("+ bcbGetProfile(%s, %s, %d, 0x%x)",
				   (secSrc == NULL) ? "NULL" : secSrc,
				   (secDest == NULL) ? "NULL" : secDest,
				   secTgtType, (unsigned long) bpsecRule);

	/* Step 0 - Sanity Checks. */
	CHKNULL(bpsecRule);

	/* Step 1 - Find the BCB Rule capturing policy */
	sec_get_bpsecBcbRule(secSrc, secDest, &secTgtType, &ruleAddr, &ruleElt);

	/*
	 * Step 1.1 - If there is no matching rule, there is no policy
	 * and without policy we do not apply a BCB.
	 */

	if (ruleElt == 0)
	{
		memset((char *) bpsecRule, 0, sizeof(BPsecBcbRule));
		BCB_DEBUG_INFO("i bcbGetProfile: No rule found \
for BCBs. No BCB processing for this bundle.", NULL);
		return NULL;
	}

	/* Step 2 - Retrieve the Profile associated with this policy. */

	sdr_read(sdr, (char *) bpsecRule, ruleAddr, sizeof(BPsecBcbRule));
	if( (prof = get_bcb_prof_by_name(bpsecRule->ciphersuiteName)) == NULL)
	{
		BCB_DEBUG_INFO("i bcbGetProfile: Profile of BCB rule is \
unknown '%s'.  No BCB processing for this bundle.", bpsecRule->ciphersuiteName);
	}

	BCB_DEBUG_PROC("- bcbGetProfile -> 0x%x", (unsigned long) prof);

	return prof;
}

/******************************************************************************
 *
 * \par Function Name: bcbAttach
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

static int	bcbAttach(Bundle *bundle, ExtensionBlock *bcbBlk,
			BpsecOutboundBlock *bcbAsb, size_t xmitRate)
{
	Sdr			sdr = getIonsdr();
	int			result = 0;
	char			eidBuf[32];
	char			*fromEid = NULL;
	char			*toEid = NULL;
	BpsecOutboundTarget	target;
	BPsecBcbRule		bpsecRule;
	BcbProfile		*prof = NULL;
	unsigned char		*serializedAsb = NULL;
	uvast			bytes = 0;

	BCB_DEBUG_PROC("+ bcbAttach (0x%x, 0x%x, 0x%x)",
			(unsigned long) bundle, (unsigned long) bcbBlk,
			(unsigned long) bcbAsb);

	/* Step 0 - Sanity checks. */
	CHKERR(bundle);
	CHKERR(bcbBlk);
	CHKERR(bcbAsb);

	/* Step 1 -	Grab Policy for the candidate block. 		*/

	/* Step 1.1 -	Retrieve the from/to EIDs that bound the
			confidentiality service. 			*/

	if ((result = bpsec_getOutboundSecurityEids(bundle, bcbBlk, bcbAsb,
			&fromEid, &toEid)) < 0 || toEid == NULL)
	{
		ADD_BCB_TX_FAIL(NULL, 1, 0);

		BCB_DEBUG_ERR("x bcbAttach: Can't get security EIDs.",
				NULL);
		result = -1;
		BCB_DEBUG_PROC("- bcbAttach -> %d", result);
		return result;
	}

	/*	We only attach a BCB per a rule for which the local
		node is the security source.				*/

	if (fromEid) MRELEASE(fromEid);
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
	 *
	 *		NOTE: for now we are assuming that the block
	 *		has only a single target.  We don't yet know
	 *		how to do this if the block has multiple
	 *		targets.
	 */

	if (bpsec_getOutboundTarget(sdr, bcbAsb->targets, &target) < 0)
	{
		BCB_DEBUG(2,"NOT adding BCB; no target.", NULL);

		MRELEASE(toEid);
		result = 0;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach -> %d", result);
		return result;
	}

	prof = bcbGetProfile(fromEid, toEid, target.targetBlockType,
			&bpsecRule);
	MRELEASE(toEid);
	if (prof == NULL)
	{
		if (bpsecRule.destEid == 0)	/*	No rule.	*/
		{
			BCB_DEBUG(2,"NOT adding BCB; no rule.", NULL);

			/*	No applicable valid construction rule.	*/

			result = 0;
			scratchExtensionBlock(bcbBlk);
			BCB_DEBUG_PROC("- bcbAttach -> %d", result);
			return result;
		}

		BCB_DEBUG(2,"NOT adding BCB; no profile.", NULL);

		/*	No applicable valid construction rule.		*/

		result = 0;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach -> %d", result);
		return result;
	}

	BCB_DEBUG(2, "Adding BCB", NULL);

	/* Step 2 - Populate the BCB ASB. */

	/* Step 2.1 - Grab the key name for this operation. */

	memcpy(bcbAsb->keyName, bpsecRule.keyName, BPSEC_KEY_NAME_LEN);

	/* Step 2.2 - Initialize the BCB ASB. */

	result = (prof->construct == NULL) ?
			bcbDefaultConstruct(prof->suiteId, bcbBlk, bcbAsb)
			: prof->construct(prof->suiteId, bcbBlk, bcbAsb);

	if (result < 0)
	{
		BCB_DEBUG_ERR("x bcbAttach: Can't construct ASB.", NULL);

		ADD_BCB_TX_FAIL(fromEid, 1, 0);

		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach --> %d", result);
		return result;
	}

	/* Step 2.2 - Encrypt the target block and attach it. 		*/

	result = (prof->encrypt == NULL) ?
		bcbDefaultEncrypt(prof->suiteId, bundle, bcbBlk, bcbAsb,
				xmitRate, &bytes, toEid)
		: prof->encrypt(prof->suiteId, bundle, bcbBlk, bcbAsb,
				xmitRate, &bytes, toEid);
	if (result < 0)
	{
		BCB_DEBUG_ERR("x bcbAttach: Can't encrypt target block.",
				NULL);

		ADD_BCB_TX_FAIL(fromEid, 1, bytes);

		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bcbBlk);
		BCB_DEBUG_PROC("- bcbAttach --> %d", result);
		return result;
	}

	/* Step 3 - serialize the BCB ASB into the BCB blk. */

	/* Step 3.1 - Create a serialized version of the BCB ASB. */

	if ((serializedAsb = bpsec_serializeASB((uint32_t *)
			&(bcbBlk->dataLength), bcbAsb)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbAttach: Unable to serialize ASB.  \
bcbBlk->dataLength = %d", bcbBlk->dataLength);

		ADD_BCB_TX_FAIL(fromEid, 1, bytes);

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

	ADD_BCB_TX_PASS(fromEid, 1, bytes);

	BCB_DEBUG_PROC("- bcbAttach --> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bcbOffer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		 a BCB for the block identified by the tag1 and tag2
 * 		 values loaded into the proposed-block structure.
 * 		 If the bundle already contains such a BCB (inserted
 * 		 by an upstream node) then the function simply
 * 		 returns 0.  Otherwise the function creates a BCB
 * 		 for the block identified by tag1 and tag2.  Note
 * 		 that only a placeholder BCB is constructed at this
 * 		 time; in effect, the placeholder BCB signals to
 * 		 later processing that such a BCB may or may not
 * 		 need to be attached to the bundle, depending on
 * 		 the final contents of other bundle blocks.  (Even
 * 		 encryption of the payload block is deferred,
 * 		 because in order ot make accurate decisions
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
 *      1. The blk has been pre-initialized with correct block type
 *         (BCB) and tag1 and tag2 values that identify the target
 *         of the proposed BCB.
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

int	bcbOffer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr			sdr = getIonsdr();
	BpsecOutboundBlock	asb;
	int			result = 0;

//<<--	Must attach block, even if only as placeholder.

	BCB_DEBUG_PROC("+ bcbOffer(%x, %x)",
                  (unsigned long) blk, (unsigned long) bundle);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure we have parameters...*/
	CHKERR(blk);
	CHKERR(bundle);

	blk->length = 0;	/*	Default.			*/
	blk->bytes = 0;		/*	Default.			*/

	/*	Step 1.2 - 	Make sure that we are not trying an
	 *			invalid security OP.			*/

	if ((blk->tag1 == PrimaryBlk)||
		(blk->tag1 == BlockConfidentialityBlk))
	{
		/*	Can't have a BCB for these types of block.	*/
		BCB_DEBUG_ERR("x bcbOffer - BCB can't target type %d",
				blk->tag1);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BCB_DEBUG_PROC("- bcbOffer -> %d", result);
		return result;
	}

	/* Step 1.3 - Make sure OP(confidentiality, target) isn't
	 * already there. */

	if (bpsec_findBlock(bundle, BlockConfidentialityBlk, blk->tag1,
				blk->tag2))
	{
		/*	Don't create a placeholder BCB for this block.	*/
		BCB_DEBUG_ERR("x bcbOffer - BCB already exists for tgt %d, %d",
				blk->tag1, blk->tag2);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BCB_DEBUG_PROC("- bcbOffer -> %d", result);
		return result;
	}

	/* Step 2 - Initialize BCB structures. */

	/* Step 2.1 - Populate the BCB ASB. */
	memset((char *) &asb, 0, sizeof(BpsecOutboundBlock));

	CHKERR(sdr_begin_xn(sdr));
	asb.targets = sdr_list_create(sdr);
	asb.parmsData = sdr_list_create(sdr);
	if (asb.targets == 0 || asb.parmsData == 0)
	{
		sdr_cancel_xn(sdr);
		BCB_DEBUG_ERR("x bcbOffer: Failed to initialize BCB ASB.",
				NULL);
		result = -1;
		BCB_DEBUG_PROC("- bcbOffer -> %d", result);
		return result;
	}

#if 0
	bpsec_insertSecuritySource(bundle, &asb);
#endif
	if (bpsec_insert_target(sdr, &asb, (blk->tag1 == PayloadBlk ? 1 : 0),
			blk->tag1, 0, blk->tag2))
	{
		sdr_cancel_xn(sdr);
		BCB_DEBUG_ERR("x bcbOffer: Failed to insert target.", NULL);
		result = -1;
		BCB_DEBUG_PROC("- bcbOffer -> %d", result);
		return result;
	}

	asb.encryptInPlace = blk->tag3;

	/* Step 2.2 Populate the BCB Extension Block. */

	blk->size = sizeof(BpsecOutboundBlock);
	if ((blk->object = sdr_malloc(sdr, blk->size)) == 0)
	{
		sdr_cancel_xn(sdr);
		BCB_DEBUG_ERR("x bcbOffer: Failed to SDR allocate object \
of size: %d", blk->size);
		result = -1;
		BCB_DEBUG_PROC("- bcbOffer -> %d", result);
		return result;
	}

	/* Step 3 - Write the ASB into the block. */

	sdr_write(sdr, blk->object, (char *) &asb, blk->size);
	sdr_end_xn(sdr);

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
	BCB_DEBUG_PROC("- bcbOffer -> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bcbProcessOnDequeue
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

int	bcbProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
		void *parm)
{
	BpsecOutboundBlock	asb;
	int			result = 0;
	DequeueContext		*context = (DequeueContext *) parm;

	BCB_DEBUG_PROC("+ bcbProcessOnDequeue(0x%x, 0x%x, 0x%x)",
			(unsigned long) blk, (unsigned long) bundle,
			(unsigned long) parm);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure arguments are valid. */

	if (bundle == NULL || parm == NULL || blk == NULL)
	{
		BCB_DEBUG_ERR("x bcbProcessOnDequeue: Bad Args.", NULL);
		BCB_DEBUG_INFO("i bcbProcessOnDequeue bundle 0x%x, \
parm 0x%x, blk 0x%x, blk->size %d", (unsigned long) bundle,
		(unsigned long) parm, (unsigned long)blk,
		(blk == NULL) ? 0 : blk->size);
		scratchExtensionBlock(blk);
		BCB_DEBUG_PROC("- bcbProcessOnDequeue --> %d", -1);
		return -1;
	}

	/* Step 1.1.1 - If block was received from elsewhere, nothing
	 * to do; it's already attached to the bundle.			*/

	if (blk->bytes)
	{
		BCB_DEBUG_PROC("- bcbProcessOnDequeue(%d) no-op", result);
		return 0;
	}

	/*
	 * Step 2 - Calculate the BCB for the target and attach it
	 *          to the bundle.
	 */

	sdr_read(getIonsdr(), (char *) &asb, blk->object, blk->size);
	result = bcbAttach(bundle, blk, &asb, context->xmitRate);
	BCB_DEBUG_PROC("- bcbProcessOnDequeue(%d)", result);
	return result;
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
 *         2. Built a ZCO to a temp file (slow, but accomodates large data)
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
 * \par Function Name: bcbHelper
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

int	bcbHelper(Object *dataObj, uint32_t chunkSize, uint32_t suite,
		sci_inbound_tlv key, sci_inbound_parms parms,
		uint8_t encryptInPlace, size_t xmitRate, uint8_t function)
{
	Sdr		sdr = getIonsdr();
	csi_blocksize_t blocksize;
	uint32_t	cipherBufLen = 0;
	uint32_t	bytesRemaining = 0;
	ZcoReader	dataReader;
	uint8_t		*context = NULL;
	Object		cipherBuffer = 0;

	BCB_DEBUG_INFO("+ bcbHelper(0x%x, %d, %d, [0x%x, %d])",
			       (unsigned long) dataObj, chunkSize, suite,
				   (unsigned long) key.value, key.length);

	/* Step 0 - Sanity Check. */
	CHKERR(key.value);

	if ((sdr_begin_xn(sdr)) == 0)
	{
		BCB_DEBUG_ERR("x bcbHelper - Can't start txn.", NULL);
		BCB_DEBUG_PROC("- bcbHelper--> NULL", NULL);
		return -1;
	}

	/*
	 * Step 3 - Setup playback of data from the data object.
	 *          The data object is the target block.
	 */

	if ((bytesRemaining = zco_length(sdr, *dataObj)) <= 0)
	{
		BCB_DEBUG_ERR("x bcbHelper - data object has no length.",
				NULL);
		sdr_cancel_xn(sdr);
		BCB_DEBUG_PROC("- bcbHelper--> NULL", NULL);
		return -1;
	}

	zco_start_transmitting(*dataObj, &dataReader);

	BCB_DEBUG_INFO("i bcbHelper: bundle size is %d", bytesRemaining);

	/* Step 4 - Grab and initialize a crypto context. */
	if ((context = sci_ctx_init(suite, key, function)) == NULL)
	{
		BCB_DEBUG_ERR("x bcbHelper - Can't get context.", NULL);
		sdr_cancel_xn(sdr);
		BCB_DEBUG_PROC("- bcbHelper--> NULL", NULL);
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
		BCB_DEBUG_ERR("x bcbHelper: Predicted bad ciphertext \
length: %d", cipherBufLen);
		csi_ctx_free(suite, context);
		sdr_cancel_xn(sdr);

		BCB_DEBUG_PROC("- bcbHelper --> %d", -1);
		return -1;
	}

	BCB_DEBUG_INFO("i bcbHelper - CipherBufLen is %d", cipherBufLen);

	if ((function == CSI_SVC_ENCRYPT) || (function == CSI_SVC_DECRYPT))
	{
		if (encryptInPlace || xmitRate == 0)
		{
			if ((bcbUpdatePayloadInPlace(suite, parms, context,
					&blocksize, *dataObj, &dataReader,
					cipherBufLen, &cipherBuffer, function))
					!= 0)
			{
				BCB_DEBUG_ERR("x bcbHelper: Cannot update \
ciphertext in place.", NULL);
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
				BCB_DEBUG_ERR("x bcbHelper: Cannot \
allocate ZCO for ciphertext of size " UVAST_FIELDSPEC, cipherBufLen);
				csi_ctx_free(suite, context);
				sdr_cancel_xn(sdr);

				BCB_DEBUG_PROC("- bcbHelper --> %d", -1);
				return -1;
			}

			zco_destroy(sdr, *dataObj);
			*dataObj = cipherZco;
		}
	}
	else
	{
		BCB_DEBUG_ERR("x bcbHelper: Invalid service: %d",
				function);
		csi_ctx_free(suite, context);
		sdr_cancel_xn(sdr);

		BCB_DEBUG_PROC("- bcbHelper --> %d", -1);
		return -1;
	}

	csi_ctx_free(suite, context);

	if (sdr_end_xn(sdr) < 0)
	{
		BCB_DEBUG_ERR("x bcbHelper: Can't end encrypt txn.", NULL);
		return -1;
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bcbReview
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

int	bcbReview(AcqWorkArea *wk)
{
	Sdr	sdr = getIonsdr();
	char	secDestEid[32];
	Object	rules;
	Object	elt;
	Object	ruleAddr;
		OBJ_POINTER(BPsecBcbRule, rule);
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 1;	/*	Default: no problem.		*/

	BCB_DEBUG_PROC("+ bcbReview(%x)", (unsigned long) wk);

	CHKERR(wk);

	isprintf(secDestEid, sizeof secDestEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	rules = sec_get_bpsecBcbRuleList();
	if (rules == 0)
	{
		BCB_DEBUG_PROC("- bcbReview -> no security database", NULL);
		return result;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, ruleAddr);
		oK(sdr_string_read(sdr, eidBuffer, rule->destEid));
		if (strcmp(eidBuffer, secDestEid) != 0)
		{
			/*	No requirement against local node.	*/

			continue;
		}

		/*	A block satisfying this rule is required.	*/

		oK(sdr_string_read(sdr, eidBuffer, rule->securitySrcEid));
		result = bpsec_requiredBlockExists(wk, BlockConfidentialityBlk,
				rule->blockType, eidBuffer);
		if (result != 1)
		{
			break;
		}
	}

	sdr_exit_xn(sdr);

	BCB_DEBUG_PROC("- bcbReview -> %d", result);

	return result;
}
