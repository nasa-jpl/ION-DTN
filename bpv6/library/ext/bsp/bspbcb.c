#include "bspbcb.h"

#if (BCB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

#include "crypto.h"

/*****************************************************************************
 *                     BCB EXTENSIONS INTERFACE FUNCTIONS                    *
 *                                                                           *
 *                                Call Order                                 *
 *                                                                           *
 *       SEND SIDE                                      RECEIVE SIDE         *  
 *                                                                           *
 *   bsp_bcbOffer                                                            *
 *   bsp_bcbProcessOnDequeue                                                 *
 *   bsp_bcbRelease                                                          *
 *   bsp_bcbCopy                                                             *
 *                                                  bsp_bcbAcquire           *
 *                                                  bsp_bcbDecrypt           *
 *                                                  bsp_bcbRecord            *
 *                                                  bsp_bcbClear             *
 *                                                                           *
 *****************************************************************************/

static void	bcbGetCiphersuite(char *securitySource, char *securityDest,
			int targetBlkType, BspBcbRule *bcbRule,
			BcbCiphersuite **cs)
{
	Sdr	bpSdr = getIonsdr();
	Object	ruleAddr;
	Object	ruleElt;

	*cs = NULL;		/*	Default: no ciphersuite.	*/
	sec_get_bspBcbRule(securitySource, securityDest, targetBlkType,
			&ruleAddr, &ruleElt);
	if (ruleElt == 0)	/*	No matching rule.		*/
	{
		memset((char *) bcbRule, 0, sizeof(BspBcbRule));
		BCB_DEBUG_INFO("i bsp_bcbGetCiphersuite: No rule found \
for BCBs. No BCB processing for this bundle.", NULL);
		return;
	}

	/*	Given the applicable BCB rule, get the ciphersuite.	*/

	sdr_read(bpSdr, (char *) bcbRule, ruleAddr, sizeof(BspBcbRule));
	*cs = get_bcb_cs_by_name(bcbRule->ciphersuiteName);
	if (*cs == NULL)
	{
		BCB_DEBUG_INFO("i bsp_bcbGetCiphersuite: Ciphersuite \
of BCB rule is unknown '%s'.  No BCB processing for this bundle.",
				bcbRule->ciphersuiteName);
	}
}

static int	attachBcb(Bundle *bundle, ExtensionBlock *blk,
			BspOutboundBlock *asb)
{
	int		result = 0;
	char		*fromEid;
	char		*toEid;
	BspBcbRule	bcbRule;
	BcbCiphersuite	*cs;
	unsigned char	*serializedAsb;

	BCB_DEBUG_PROC("+ attachBcb (%x, %x, %x)", (unsigned long) bundle,
			(unsigned long) blk, (unsigned long) asb);

	if (bsp_getOutboundSecurityEids(bundle, blk, asb, &fromEid, &toEid))
	{
		BCB_DEBUG_ERR("x attachBcb: out of space.", NULL);
		result = -1;
		BCB_DEBUG_PROC("- attachBcb -> %d", result);
		return result;
	}

	bcbGetCiphersuite(fromEid, toEid, asb->targetBlockType, &bcbRule, &cs);
	MRELEASE(fromEid);
	MRELEASE(toEid);
	if (cs == NULL)
	{
		/*	No applicable valid construction rule.		*/

		scratchExtensionBlock(blk);
		BCB_DEBUG_PROC("- attachBcb -> %d", result);
		return result;
	}

	memcpy(asb->keyName, bcbRule.keyName, BSP_KEY_NAME_LEN);
	if (cs->construct(blk, asb) < 0)
	{
		BCB_DEBUG_ERR("x attachBcb: Can't construct ASB.", NULL);
		result = -1;
		scratchExtensionBlock(blk);
		BCB_DEBUG_PROC("- attachBcb --> %d", result);
		return result;
	}

	if (cs->encrypt(bundle, blk, asb) < 0)
	{
		BCB_DEBUG_ERR("x attachBcb: Can't encrypt target block.",
				NULL);
		result = -1;
		scratchExtensionBlock(blk);
		BCB_DEBUG_PROC("- attachBcb --> %d", result);
		return result;
	}

	serializedAsb = bsp_serializeASB(&(blk->dataLength), asb);
	if (serializedAsb == NULL)
	{
		BCB_DEBUG_ERR("x attachBcb: Unable to \
serialize ASB.  blk->dataLength = %d", blk->dataLength);
		result = -1;
		scratchExtensionBlock(blk);
		BCB_DEBUG_PROC("- attachBcb --> %d", result);
		return result;
	}

	/*	Serialized ASB is the block-specific data for the BCB.	*/

	result = serializeExtBlk(blk, NULL, (char *) serializedAsb);
	MRELEASE(serializedAsb);
	BCB_DEBUG_PROC("- attachBcb --> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbOffer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		 a BCB for the block identified by tags 1, 2, and 3 of
 * 		 the proposed BCB contents.  If the bundle already
 * 		 contains such a BCB (inserted by an upstream node)
 * 		 then the function simply returns 0.  Otherwise the
 * 		 function creates a BCB for the target block identified
 * 		 by blk->tag1, tag2, and tag2.  If the target block is
 * 		 the payload block, then the BCB is fully constructed
 * 		 -- and its target encrypted -- at this time (because
 * 		 the final content of the payload block is complete).
 * 		 Otherwise, only a placeholder BCB is constructed; in
 * 		 effect, the placeholder BCB signals to later processing
 * 		 that such a BCB may or may not need to be attached to
 * 		 the bundle, depending on the final contents of other
 * 		 bundle blocks.
 *
 * \retval int 0 - The BCB was successfully created, or not needed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this new block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *****************************************************************************/

int	bsp_bcbOffer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr			bpSdr = getIonsdr();
	Object			existingBcb;
	BspOutboundBlock	asb;
	int			result = 0;

	BCB_DEBUG_PROC("+ bsp_bcbOffer(%x, %x)",
                  (unsigned long) blk, (unsigned long) bundle);

	CHKERR(blk);
	CHKERR(bundle);
	blk->length = 0;	/*	Default.			*/
	blk->bytes = 0;		/*	Default.			*/
	if (blk->tag1 == BLOCK_TYPE_PRIMARY
	|| blk->tag1 == EXTENSION_TYPE_BAB
	|| blk->tag1 == EXTENSION_TYPE_BCB)
	{
		/*	Can't have a BCB for these types of block.	*/

		blk->size = 0;
		blk->object = 0;
		BCB_DEBUG_PROC("- bsp_bcbOffer -> %d", result);
		return result;
	}

	existingBcb = bsp_findBspBlock(bundle, EXTENSION_TYPE_BCB, blk->tag1, 
			blk->tag2, blk->tag3);
	if (existingBcb)	/*	Bundle already has this BCB.	*/
	{
		/*	Don't create a placeholder BCB for this block.	*/

		blk->size = 0;
		blk->object = 0;
		BCB_DEBUG_PROC("- bsp_bcbOffer -> %d", result);
		return result;
	}

	memset((char *) &asb, 0, sizeof(BspOutboundBlock));
	bsp_insertSecuritySource(bundle, &asb);
	asb.targetBlockType = blk->tag1;
	blk->size = sizeof(BspOutboundBlock);
	blk->object = sdr_malloc(bpSdr, blk->size);
	if (blk->object == 0)
	{
		BCB_DEBUG_ERR("x bsp_bcbOffer: Failed to SDR allocate object \
of size: %d", blk->size);
		result = -1;
		BCB_DEBUG_PROC("- bsp_bcbOffer -> %d", result);
		return result;
	}

	sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);
	if (asb.targetBlockType != BLOCK_TYPE_PAYLOAD)
	{
		/*	We can't construct the block at this time
		 *	because we can't assume that the target block
		 *	exists in final form yet.  All we do is tell
		 *	the BP agent that we want this BCB to be
		 *	considered for construction at the time the
		 *	bundle is dequeued for transmission.  For
		 *	this purpose, we stop after initializing
		 *	the block's scratchpad area, resulting in
		 *	insertion of a placeholder BCB for the target
		 *	block.						*/

		BCB_DEBUG_PROC("- bsp_bcbOffer -> %d", result);
		return result;
	}

	/*	Construct the BCB for the payload block now, and
	 *	encrypt the payload in the process.			*/

	result = attachBcb(bundle, blk, &asb);
	BCB_DEBUG_PROC("- bsp_bcbOffer -> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbProcessOnDequeue
 *
 * \par Purpose: This callback determines whether or not a block of the
 * 		 type that is the target of this proposed BCB exists in
 * 		 the bundle and discards the BCB if it does not.  If the
 * 		 target block exists and no BCB for that block has yet
 * 		 been constructed, the BCB is constructed.
 *
 * \retval int 0 - The block was successfully constructed or deleted.
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk       The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 *
 * \par Notes:
 *      1. The target block must not have its content changed after this point.
 *****************************************************************************/

int	bsp_bcbProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *parm)
{
	BspOutboundBlock	asb;
	int			result = 0;

	BCB_DEBUG_PROC("+ bsp_bcbProcessOnDequeue(%x, %x, %x)",
			(unsigned long) blk, (unsigned long) bundle,
			(unsigned long) parm);
	if (bundle == NULL || parm == NULL || blk == NULL)
	{
		BCB_DEBUG_ERR("x bsp_bcbProcessOnDequeue: Bundle or ASB were \
not as expected.", NULL);
		BCB_DEBUG_INFO("i bsp_bcbProcessOnDequeue bundle %d, parm %d, \
blk %d, blk->size %d", bundle, parm, blk, blk->size);
		BCB_DEBUG_PROC("- bsp_bcbProcessOnDequeue --> %d", -1);
		scratchExtensionBlock(blk);
		return -1;
	}

	if (blk->blkProcFlags & BLK_FORWARDED_OPAQUE)
	{
		/*	Do nothing; the block's bytes are correct
		 *	and ready for transmission.  The block was
		 *	received when the bundle was received; it
		 *	was serialized in the recordExtensionBlocks()
		 *	function.					*/

		BCB_DEBUG_PROC("- bsp_bcbProcessOnDequeue(%d)", result);
		return 0;
	} 

	sdr_read(getIonsdr(), (char *) &asb, blk->object, blk->size);
	if (asb.targetBlockType == BLOCK_TYPE_PAYLOAD)
	{
		/*	Do nothing; the block's bytes are correct
		 *	and ready for transmission.  The block was
		 *	constructed by the offer() function, because
		 *	the payload block content was already final
		 *	at that time.					*/

		BCB_DEBUG_PROC("- bsp_bcbProcessOnDequeue(%d)", result);
		return 0;
	}

	result = attachBcb(bundle, blk, &asb);
	BCB_DEBUG_PROC("- bsp_bcbProcessOnDequeue(%d)", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbRelease
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
 *****************************************************************************/

void    bsp_bcbRelease(ExtensionBlock *blk)
{
	Sdr			sdr = getIonsdr();
	BspOutboundBlock	asb;

	BCB_DEBUG_PROC("+ bsp_bcbRelease(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		sdr_read(sdr, (char *) &asb, blk->object,
				sizeof(BspOutboundBlock));
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

	BCB_DEBUG_PROC("- bsp_bcbRelease(%c)", ' ');
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbCopy
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
 *****************************************************************************/

int	bsp_bcbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr			bpSdr = getIonsdr();
	BspOutboundBlock	asb;
	int			result = 0;
	char			*buffer = NULL;

	BCB_DEBUG_PROC("+ bsp_bcbCopy(%x, %x)", (unsigned long) newBlk,
			(unsigned long) oldBlk);

	CHKERR(newBlk);
	CHKERR(oldBlk);
	newBlk->size = sizeof asb;
	newBlk->object = sdr_malloc(bpSdr, sizeof asb);
	if (newBlk->object == 0)
	{
		BCB_DEBUG_ERR("x bsp_bcbCopy: Failed to allocate: %d",
				sizeof asb);
		return -1;
	}

	sdr_read(bpSdr, (char *) &asb, oldBlk->object, sizeof asb);
	if (asb.parmsData)
	{
		buffer = MTAKE(asb.parmsLen);
		if (buffer == NULL)
		{
			BCB_DEBUG_ERR("x bsp_bcbCopy: Failed to allocate: %d",
					asb.parmsLen);
			return -1;
		}

		sdr_read(bpSdr, buffer, asb.parmsData, asb.parmsLen);
		asb.parmsData = sdr_malloc(bpSdr, asb.parmsLen);
		if (asb.parmsData == 0)
		{
			BCB_DEBUG_ERR("x bsp_bcbCopy: Failed to allocate: %d",
					asb.parmsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.parmsData, buffer, asb.parmsLen);
		MRELEASE(buffer);
	}

	if (asb.resultsData)
	{
		buffer = MTAKE(asb.resultsLen);
		if (buffer == NULL)
		{
			BCB_DEBUG_ERR("x bsp_bcbCopy: Failed to allocate: %d",
					asb.resultsLen);
			return -1;
		}

		sdr_read(bpSdr, buffer, asb.resultsData, asb.resultsLen);
		asb.resultsData = sdr_malloc(bpSdr, asb.resultsLen);
		if (asb.resultsData == 0)
		{
			BCB_DEBUG_ERR("x bsp_bcbCopy: Failed to allocate: %d",
					asb.resultsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.resultsData, buffer, asb.resultsLen);
		MRELEASE(buffer);
	}

	sdr_write(bpSdr, newBlk->object, (char *) &asb, sizeof asb);
	BCB_DEBUG_PROC("- bsp_bcbCopy -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbAcquire
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
 *****************************************************************************/

int	bsp_bcbAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BCB_DEBUG_PROC("+ bsp_bcbAcquire(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);
	result = bsp_deserializeASB(blk, wk);
	BCB_DEBUG_INFO("i bsp_bcbAcquire: Deserialize result %d", result);

	BCB_DEBUG_PROC("- bsp_bcbAcquire -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbDecrypt
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
 *****************************************************************************/

int	bsp_bcbDecrypt(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle;
	char		*dictionary;
	BspInboundBlock	*asb = NULL;
	char		*fromEid;
	char		*toEid;
	BspBcbRule	bcbRule;
	BcbCiphersuite	*cs;
	int		result;

	BCB_DEBUG_PROC("+ bsp_bcbDecrypt(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	if (blk == NULL || blk->object == NULL || wk == NULL)
	{
		BCB_DEBUG_ERR("x bsp_bcbDecrypt:  Blocks are NULL. %x",
				(unsigned long) blk);
		result = -1;
		BCB_DEBUG_PROC("- bsp_bcbDecrypt --> %d", result);
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
		return -1;
	}

	asb = (BspInboundBlock *) (blk->object);
	if (asb->securitySource.unicast)	/*	Waypoint source.*/
	{
		if (printEid(&(asb->securitySource), dictionary, &fromEid) < 0)
		{
			releaseDictionary(dictionary);
			return -1;
		}
	}
	else
	{
		if (printEid(&(bundle->id.source), dictionary, &fromEid) < 0)
		{
			releaseDictionary(dictionary);
			return -1;
		}
	}

	if (printEid(&(bundle->destination), dictionary, &toEid) < 0)
	{
		MRELEASE(fromEid);
		releaseDictionary(dictionary);
		return -1;
	}

	releaseDictionary(dictionary);

	/*	Given sender & receiver EIDs, get applicable BCB rule.	*/

	bcbGetCiphersuite(fromEid, toEid, asb->targetBlockType, &bcbRule, &cs);
	MRELEASE(fromEid);
	MRELEASE(toEid);
	if (cs == NULL)
	{
		/*	We can't decrypt this block.			*/
	       
		if (bcbRule.destEid == 0)	/*	No rule.	*/
		{
			/*	We don't care about decrypting the
			 *	target block for this BCB, but we
			 *	preserve the BCB in case somebody
			 *	else does.				*/

			BCB_DEBUG_INFO("- bsp_bcbDecrypt - No rule.", NULL);
			result = 1;
			BCB_DEBUG_PROC("- bsp_bcbDecrypt --> %d", result);
			blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
			return result;		/*	No information.	*/
		}

		/*	Rule is found, but we don't have this CS.
		 *	We cannot decrypt the block, so the block
		 *	is -- in effect -- malformed.			*/

		discardExtensionBlock(blk);
	 	BCB_DEBUG_ERR("- bsp_bcbDecrypt - Ciphersuite missing!", NULL);
		result = 0;
		BCB_DEBUG_PROC("- bsp_bcbDecrypt --> %d", result);
		return 0;			/*	Block malformed.*/
	}

	/*	Fill in missing information in the scratchpad area.	*/

	memcpy(asb->keyName, bcbRule.keyName, BSP_KEY_NAME_LEN);

	/*	Invoke ciphersuite-specific check procedure.		*/

	result = cs->decrypt(wk, blk);

	/*	Discard the BCB if the local node is the destination
	 *	of the bundle or if decryption failed (meaning the
	 *	block is malformed and therefore the bundle is
	 *	malformed); otherwise make sure the BCB is retained.	*/

	if (result == 0 || bsp_destinationIsLocal(&(wk->bundle)))
	{
		discardExtensionBlock(blk);
	}
	else
	{
		blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
	}

	BCB_DEBUG_PROC("- bsp_bcbDecrypt --> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bcbClear
 *
 * \par Purpose: This callback removes all memory allocated by the BSP module
 *               during the block's acquisition process.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the BSP module.
 *****************************************************************************/

void	bsp_bcbClear(AcqExtBlock *blk)
{
	BspInboundBlock	*asb;

	BCB_DEBUG_PROC("+ bsp_bcbClear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		asb = (BspInboundBlock *) (blk->object);
		if (asb->parmsData)
		{
			BCB_DEBUG_INFO("i bsp_bcbClear: Release result len %ld",
					asb->parmsLen);
			MRELEASE(asb->parmsData);
		}

		if (asb->resultsData)
		{
			BCB_DEBUG_INFO("i bsp_bcbClear: Release result len %ld",
					asb->resultsLen);
			MRELEASE(asb->resultsData);
		}

		BCB_DEBUG_INFO("i bsp_bcbClear: Release ASB len %d", blk->size);

		MRELEASE(blk->object);
		blk->object = NULL;
		blk->size = 0;
	}

	BCB_DEBUG_PROC("- bsp_bcbClear", NULL);
}
