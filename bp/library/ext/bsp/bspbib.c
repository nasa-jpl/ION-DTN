#include "bspbib.h"

#if (BIB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

#include "crypto.h"

/*****************************************************************************
 *                     BIB EXTENSIONS INTERFACE FUNCTIONS                    *
 *                                                                           *
 *                                Call Order                                 *
 *                                                                           *
 *       SEND SIDE                                      RECEIVE SIDE         *  
 *                                                                           *
 *   bsp_bibOffer                                                            *
 *   bsp_bibProcessOnDequeue                                                 *
 *   bsp_bibRelease                                                          *
 *   bsp_bibCopy                                                             *
 *                                                  bsp_bibParse             *
 *                                                  bsp_bibCheck             *
 *                                                  bsp_bibRecord            *
 *                                                  bsp_bibClear             *
 *                                                                           *
 *****************************************************************************/

static void	bibGetCiphersuite(char *securitySource, char *securityDest,
			int targetBlkType, BspBibRule *bibRule,
			BibCiphersuite **cs)
{
	Sdr	bpSdr = getIonsdr();
	Object	ruleAddr;
	Object	ruleElt;

	*cs = NULL;		/*	Default: no ciphersuite.	*/
	sec_get_bspBibRule(securitySource, securityDest, targetBlkType,
			&ruleAddr, &ruleElt);
	if (ruleElt == 0)	/*	No matching rule.		*/
	{
		memset((char *) bibRule, 0, sizeof(BspBibRule));
		BIB_DEBUG_INFO("i bspGetCiphersuite: No rule found \
for BIBs. No BIB processing for this bundle.", NULL);
		return;
	}

	/*	Given the applicable BIB rule, get the ciphersuite.	*/

	sdr_read(bpSdr, (char *) bibRule, ruleAddr, sizeof(BspBibRule));
	*cs = get_bib_cs_by_name(bibRule->ciphersuiteName);
	if (*cs == NULL)
	{
		BIB_DEBUG_INFO("i bspGetCiphersuite: Ciphersuite \
of BIB rule is unknown '%s'.  No BIB processing for this bundle.",
				bibRule->ciphersuiteName);
	}
}

static int	attachBib(Bundle *bundle, ExtensionBlock *blk,
			BspOutboundBlock *asb)
{
	int		result = 0;
	char		*fromEid;
	char		*toEid;
	BspBibRule	bibRule;
	BibCiphersuite	*cs;
	unsigned char	*serializedAsb;

	BIB_DEBUG_PROC("+ attachBib", NULL);

	if (bsp_getOutboundSecurityEids(bundle, blk, asb, &fromEid, &toEid))
	{
		BIB_DEBUG_ERR("x attachBib: out of space.", NULL);
		result = -1;
		BIB_DEBUG_PROC("- attachBib -> %d", result);
		return result;
	}

	bibGetCiphersuite(fromEid, toEid, asb->targetBlockType, &bibRule, &cs);
	MRELEASE(fromEid);
	MRELEASE(toEid);
	if (cs == NULL)
	{
		/*	No applicable valid construction rule.		*/

		scratchExtensionBlock(blk);
		BIB_DEBUG_PROC("- attachBib -> %d", result);
		return result;
	}

	memcpy(asb->keyName, bibRule.keyName, BSP_KEY_NAME_LEN);
	if (cs->construct(blk, asb) < 0)
	{
		BIB_DEBUG_ERR("x attachBib: Can't construct ASB.", NULL);
		result = -1;
		scratchExtensionBlock(blk);
		BIB_DEBUG_PROC("- attachBib --> %d", result);
		return result;
	}

	if (cs->sign(bundle, blk, asb) < 0)
	{
		BIB_DEBUG_ERR("x attachBib: Can't sign target block.", NULL);
		result = -1;
		scratchExtensionBlock(blk);
		BIB_DEBUG_PROC("- attachBib --> %d", result);
		return result;
	}

	serializedAsb = bsp_serializeASB(&(blk->dataLength), asb);
	if (serializedAsb == NULL)
	{
		BIB_DEBUG_ERR("x attachBib: Unable to \
serialize ASB.  blk->dataLength = %d", blk->dataLength);
		result = -1;
		scratchExtensionBlock(blk);
		BIB_DEBUG_PROC("- attachBib --> %d", result);
		return result;
	}

	/*	Serialized ASB is the block-specific data for the BIB.	*/

	result = serializeExtBlk(blk, NULL, (char *) serializedAsb);
	MRELEASE(serializedAsb);
	BIB_DEBUG_PROC("- attachBib --> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibOffer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		 a BIB for the block identified by tag1 of the proposed
 * 		 BIB contents.  If the bundle already contains such a
 * 		 BIB (inserted by an upstream node) then the function
 * 		 simply returns 0.  Otherwise the function creates a
 * 		 BIB for the target block identified by blk->tag1.  If
 * 		 the target block is the payload block, then the BIB is
 * 		 fully constructed at this time (because the final
 * 		 content of the payload block is complete).  Otherwise,
 * 		 only a placeholder BIB is constructed; in effect, the
 * 		 placeholder BIB signals to later processing that such
 * 		 a BIB may or may not need to be attached to the bundle,
 * 		 depending on the final contents of other bundle blocks.
 *
 * \retval int 0 - The BIB was successfully created, or not needed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this new block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *      2. The payload block must not have its content changed after this point.
 *         Note that if the payload block is both signed and encrypted, the ION
 *         implementation of BSP will assume that the payload block must be
 *         decrypted before its integrity can be checked (in fact, before any
 *         operation at all can be performed on the payload block).  So the
 *         payload block's BIB must be computed on the clear text of the
 *         payload block, i.e., the BIB must be constructed before the BCB is
 *         constructed.
 *****************************************************************************/

int	bsp_bibOffer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr			bpSdr = getIonsdr();
	Object			existingBib;
	BspOutboundBlock	asb;
	int			result = 0;

	BIB_DEBUG_PROC("+ bsp_bibOffer(%x, %x)",
                  (unsigned long) blk, (unsigned long) bundle);

	CHKERR(blk);
	CHKERR(bundle);
	blk->length = 0;	/*	Default.			*/
	blk->bytes = 0;		/*	Default.			*/
	if (blk->tag1 == EXTENSION_TYPE_BAB
	|| blk->tag1 == EXTENSION_TYPE_BIB
	|| blk->tag1 == EXTENSION_TYPE_BCB)
	{
		/*	Can't have a BIB for these types of block.	*/

		blk->size = 0;
		blk->object = 0;
		BIB_DEBUG_PROC("- bsp_bibOffer -> %d", result);
		return result;
	}

	existingBib = bsp_findBspBlock(bundle, EXTENSION_TYPE_BIB, blk->tag1, 
			0, 0);
	if (existingBib)	/*	Bundle already has this BIB.	*/
	{
		/*	Don't create a placeholder BIB for this block.	*/

		blk->size = 0;
		blk->object = 0;
		BIB_DEBUG_PROC("- bsp_bibOffer -> %d", result);
		return result;
	}

	memset((char *) &asb, 0, sizeof(BspOutboundBlock));
	bsp_insertSecuritySource(bundle, &asb);
	asb.targetBlockType = blk->tag1;
	blk->size = sizeof(BspOutboundBlock);
	blk->object = sdr_malloc(bpSdr, blk->size);
	if (blk->object == 0)
	{
		BIB_DEBUG_ERR("x bsp_bibOffer: Failed to SDR allocate object \
of size: %d", blk->size);
		result = -1;
		BIB_DEBUG_PROC("- bsp_bibOffer -> %d", result);
		return result;
	}

	sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);
	if (asb.targetBlockType != BLOCK_TYPE_PAYLOAD)
	{
		/*	We can't construct the block at this time
		 *	because we can't assume that the target block
		 *	exists in final form yet.  All we do is tell
		 *	the BP agent that we want this BIB to be
		 *	considered for construction at the time the
		 *	bundle is dequeued for transmission.  For
		 *	this purpose, we stop after initializing
		 *	the block's scratchpad area, resulting in
		 *	insertion of a placeholder BIB for the target
		 *	block.						*/

		BIB_DEBUG_PROC("- bsp_bibOffer -> %d", result);
		return result;
	}

	/*	Construct the BIB for the payload block now.		*/

	result = attachBib(bundle, blk, &asb);
	BIB_DEBUG_PROC("- bsp_bibOffer -> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibProcessOnDequeue
 *
 * \par Purpose: This callback determines whether or not a block of the
 * 		 type that is the target of this proposed BIB exists in
 * 		 the bundle and discards the BIB if it does not.  If the
 * 		 target block exists and no BIB for that block has yet
 * 		 been constructed, the BIB is constructed.
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
 *      1. All block memory is allocated using sdr_malloc.
 *      2. The target block must not have its content changed after this point.
 *         Note that if the target block is both signed and encrypted, the ION
 *         implementation of BSP will assume that the target block must be
 *         decrypted before its integrity can be checked (in fact, before any
 *         operation at all can be performed on the target block).  So the
 *         target block's BIB must be computed on the clear text of the
 *         target block, i.e., the BIB must be constructed before the BCB is
 *         constructed.
 *****************************************************************************/

int	bsp_bibProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *parm)
{
	BspOutboundBlock	asb;
	int			result = 0;

	BIB_DEBUG_PROC("+ bsp_bibProcessOnDequeue(%x, %x, %x)",
			(unsigned long) blk, (unsigned long) bundle,
			(unsigned long) parm);
	if (bundle == NULL || parm == NULL || blk == NULL)
	{
		BIB_DEBUG_ERR("x bsp_bibProcessOnDequeue: Bundle or ASB were \
not as expected.", NULL);
		BIB_DEBUG_INFO("i bsp_bibProcessOnDequeue bundle %d, parm %d, \
blk %d, blk->size %d", bundle, parm, blk, blk->size);
		BIB_DEBUG_PROC("- bsp_bibProcessOnDequeue --> %d", -1);
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

		BIB_DEBUG_PROC("- bsp_bibProcessOnDequeue(%d)", result);
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

		BIB_DEBUG_PROC("- bsp_bibProcessOnDequeue(%d)", result);
		return 0;
	}

	result = attachBib(bundle, blk, &asb);
	BIB_DEBUG_PROC("- bsp_bibProcessOnDequeue(%d)", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibRelease
 *
 * \par Purpose: This callback releases SDR heap space  allocated to
 * 		 a block integrity block.
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated SDR heap space must be
 *                      released.
 *
 * \par Notes:
 *
 *****************************************************************************/

void    bsp_bibRelease(ExtensionBlock *blk)
{
	Sdr			sdr = getIonsdr();
	BspOutboundBlock	asb;

	BIB_DEBUG_PROC("+ bsp_bibRelease(%x)", (unsigned long) blk);

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

	BIB_DEBUG_PROC("- bsp_bibRelease(%c)", ' ');
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a BIB
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

int	bsp_bibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr			bpSdr = getIonsdr();
	BspOutboundBlock	asb;
	int			result = 0;
	char			*buffer = NULL;

	BIB_DEBUG_PROC("+ bsp_bibCopy(%x, %x)", (unsigned long) newBlk,
			(unsigned long) oldBlk);

	CHKERR(newBlk);
	CHKERR(oldBlk);
	newBlk->size = sizeof asb;
	newBlk->object = sdr_malloc(bpSdr, sizeof asb);
	if (newBlk->object == 0)
	{
		BIB_DEBUG_ERR("x bsp_bibCopy: Failed to allocate: %d",
				sizeof asb);
		return -1;
	}

	sdr_read(bpSdr, (char *) &asb, oldBlk->object, sizeof asb);
	if (asb.parmsData)
	{
		buffer = MTAKE(asb.parmsLen);
		if (buffer == NULL)
		{
			BIB_DEBUG_ERR("x bsp_bibCopy: Failed to allocate: %d",
					asb.parmsLen);
			return -1;
		}

		sdr_read(bpSdr, buffer, asb.parmsData, asb.parmsLen);
		asb.parmsData = sdr_malloc(bpSdr, asb.parmsLen);
		if (asb.parmsData == 0)
		{
			BIB_DEBUG_ERR("x bsp_bibCopy: Failed to allocate: %d",
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
			BIB_DEBUG_ERR("x bsp_bibCopy: Failed to allocate: %d",
					asb.resultsLen);
			return -1;
		}

		sdr_read(bpSdr, buffer, asb.resultsData, asb.resultsLen);
		asb.resultsData = sdr_malloc(bpSdr, asb.resultsLen);
		if (asb.resultsData == 0)
		{
			BIB_DEBUG_ERR("x bsp_bibCopy: Failed to allocate: %d",
					asb.resultsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.resultsData, buffer, asb.resultsLen);
		MRELEASE(buffer);
	}

	sdr_write(bpSdr, newBlk->object, (char *) &asb, sizeof asb);
	BIB_DEBUG_PROC("- bsp_bibCopy -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibParse
 *
 * \par Purpose: This callback is called for each serialized BIB that was
 * 		 encountered during bundle reception and subsequently
 * 		 decrypted (as necessary).  The callback will deserialize
 * 		 the block into a scratchpad object.
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

int	bsp_bibParse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BIB_DEBUG_PROC("+ bsp_bibParse(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);
	result = bsp_deserializeASB(blk, wk);
	BIB_DEBUG_INFO("i bsp_bibParse: Deserialize result %d", result);

	BIB_DEBUG_PROC("- bsp_bibParse -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibCheck
 *
 * \par Purpose: This callback determine whether or not a BIB's security
 * 		 target block has been corrupted end route.  Specifically,
 * 		 a newly computed hash for the security target block
 * 		 must match the security result encoded in the BIB.
 *
 * \retval int 0 - The block was not found to be corrupt.
 *             3 - The block was found to be corrupt.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The BIB whose target must be checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *****************************************************************************/

int	bsp_bibCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle		*bundle;
	char		*dictionary;
	BspInboundBlock	*asb = NULL;
	char		*fromEid;
	char		*toEid;
	BspBibRule	bibRule;
	BibCiphersuite	*cs;
	int		result;

	BIB_DEBUG_PROC("+ bsp_bibCheck(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	if (blk == NULL || blk->object == NULL || wk == NULL)
	{
		BIB_DEBUG_ERR("x bsp_bibCheck:  Blocks are NULL. %x",
				(unsigned long) blk);
		BIB_DEBUG_PROC("- bsp_bibCheck --> %d", -1);
		return -1;
	}

	/*	For BIBs, the security destination is always the final
	 *	destination of the bundle.  The security source is
	 *	normally the original source of the bundle, but a BIB
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

	/*	Given sender & receiver EIDs, get applicable BIB rule.	*/

	bibGetCiphersuite(fromEid, toEid, asb->targetBlockType, &bibRule, &cs);
	MRELEASE(fromEid);
	MRELEASE(toEid);
	if (cs == NULL)
	{
		/*	We can't verify this signature.			*/
	       
		if (bibRule.destEid == 0)	/*	No rule.	*/
		{
			/*	We don't care about verifying the
			 *	signature on the target block for
			 *	this BIB, but we preserve the BIB
			 *	in case somebody else does.		*/

			BIB_DEBUG_INFO("- bsp_bibCheck - No rule.", NULL);
			BIB_DEBUG_PROC("- bsp_bibCheck", NULL);
			blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
			return 1;		/*	No information.	*/
		}

		/*	Rule is found, but we don't have this CS.
		 *	We have to conclude that the BIB is invalid,
		 *	in which case the bundle is judged corrupt.	*/

		discardExtensionBlock(blk);
	 	BIB_DEBUG_ERR("- bsp_bibCheck - Ciphersuite missing!", NULL);
		BIB_DEBUG_PROC("- bsp_bibCheck", NULL);
		return 0;			/*	Bundle corrupt.	*/
	}

	/*	Fill in missing information in the scratchpad area.	*/

	memcpy(asb->keyName, bibRule.keyName, BSP_KEY_NAME_LEN);

	/*	Invoke ciphersuite-specific check procedure.		*/

	result = cs->verify(wk, blk);

	/*	Discard the BIB if the local node is the destination
	 *	of the bundle or the BIB is invalid or verification
	 *	failed (meaning the block is altered and therefore
	 *	the bundle is altered); otherwise make sure the BIB
	 *	is retained.						*/

	if (result == 0 || result == 4 || bsp_destinationIsLocal(&(wk->bundle)))
	{
		discardExtensionBlock(blk);
	}
	else
	{
		blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
	}

	BIB_DEBUG_PROC("- bsp_bibCheck(%d)", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_bibClear
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

void	bsp_bibClear(AcqExtBlock *blk)
{
	BspInboundBlock *asb;

	BIB_DEBUG_PROC("+ bsp_bibClear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		asb = (BspInboundBlock *) (blk->object);
		if (asb->parmsData)
		{
			BIB_DEBUG_INFO("i bsp_bibClear: Release parms len %ld",
					asb->parmsLen);
			MRELEASE(asb->parmsData);
		}

		if (asb->resultsData)
		{
			BIB_DEBUG_INFO("i bsp_bibClear: Release result len %ld",
					asb->resultsLen);
			MRELEASE(asb->resultsData);
		}

		BIB_DEBUG_INFO("i bsp_bibClear: Release ASB len %d", blk->size);

		MRELEASE(blk->object);
		blk->object = NULL;
		blk->size = 0;
	}

	BIB_DEBUG_PROC("- bsp_bibClear", NULL);

	return;
}
