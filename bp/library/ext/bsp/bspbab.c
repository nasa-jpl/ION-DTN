/*
 * bspbab.c
 *
 *  Created on: 1 February 2014, adapted from extbspbab.c created Jul 5, 2010
 *      Author: Scott Burleigh, adapted from extbspbab.c created by birraej1
 */

#include "bspbab.h"

#if (BAB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/*****************************************************************************
 *                     BAB EXTENSIONS INTERFACE FUNCTIONS                    *
 *                                                                           *
 *                                Call Order                                 *
 *                                                                           *
 *       SEND SIDE                                      RECEIVE SIDE         *  
 *                                                                           *
 *   bsp_babOffer (First)                                                    *
 *   bsp_babOffer (Last)                                                     *
 *   bsp_babProcessOnDequeue (First)                                         *
 *   bsp_babProcessOnDequeue (Last)                                          *
 *   bsp_babProcessOnTransmit (First)                                        *
 *   bsp_babProcessOnTransmit (Last)                                         *
 *   bsp_babRelease (First)                                                  *
 *   bsp_babRelease (Last)                                                   *
 *                                                  bsp_babAcquire (First)   *
 *                                                  bsp_babAcquire (Last)    *
 *                                                  bsp_babCheck (First)     *
 *                                                  bsp_babCheck (Last)      *
 *                                                  bsp_babClear (First)     *
 *                                                  bsp_babClear (Last)      *
 *                                                                           *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name:	bsp_babOffer
 *
 * \par Purpose:	This callback creates a BAB block.  Because the rule
 * 			governing BAB production for a bundle depends in
 * 			part on the EID of the node to which the bundle is
 * 			to be transmitted, and that node is not yet known
 * 			at the time the "offer" callback is invoked, we
 * 			always create the BAB block.  At the time the
 * 			bundle is dequeued we know the EID of the receiving
 * 			node and can look up the relevant BAB rule, if any;
 * 			if no BAB rule applies, the BAB block is simply
 * 			discarded.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that is to be added to the bundle.
 * \param[in]      bundle The bundle that will hold this block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *****************************************************************************/

int	bsp_babOffer(ExtensionBlock *blk, Bundle *bundle)
{
	BAB_DEBUG_PROC("+ bsp_babOffer(%x, %x)", (unsigned long) blk,
			(unsigned long) bundle);

	CHKERR(blk);
	CHKERR(bundle);

	/*	We can't create an actual block at this time because
	 *	we don't know which ciphersuite to use.  All we do
	 *	is tell the BP agent that we want this block to be
	 *	considered for construction at the time the bundle
	 *	is dequeued for transmission.  For this purpose,
	 *	we arbitrarily set blk->size to 1 just to ensure that
	 *	the block is attached to the bundle.			*/

	blk->length = 0;
	blk->bytes = 0;
	blk->size = 1;
	blk->object = 0;

	BAB_DEBUG_PROC("- bsp_babOffer -> %d", 1);

	return 1;
}

static void	babGetCiphersuite(char *fromEid, char *toEid,
			BspBabRule *babRule, BabCiphersuite **cs)
{
	Sdr	bpSdr = getIonsdr();
	Object	ruleAddr;
	Object	ruleElt;

	*cs = NULL;		/*	Default: no ciphersuite.	*/
	sec_get_bspBabRule(fromEid, toEid, &ruleAddr, &ruleElt);
	if (ruleElt == 0)	/*	No matching rule.		*/
	{
		BAB_DEBUG_INFO("i babGetCiphersuite: No rule found \
for BABs. No BAB processing for this bundle.", NULL);
		return;
	}

	/*	Given the applicable BAB rule, get the ciphersuite.	*/

	sdr_read(bpSdr, (char *) babRule, ruleAddr, sizeof(BspBabRule));
	*cs = get_bab_cs_by_name(babRule->ciphersuiteName);
	if (*cs == NULL)
	{
		BAB_DEBUG_INFO("i babGetCiphersuite: Ciphersuite \
of BAB rule is unknown '%s'.  No BAB processing for this bundle.",
				babRule->ciphersuiteName);
	}
}

static int	babProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
			void *parm)
{
	Sdr			bpSdr = getIonsdr();
	DequeueContext		*ctxt = (DequeueContext *) parm;
	char			*fromEid;
	char			*toEid;
	BspBabRule		babRule;
	BabCiphersuite		*cs;
	BspOutboundBlock	asb;
	unsigned char		*serializedAsb;
	int			result;

	CHKERR(blk);
	CHKERR(bundle);
	CHKERR(ctxt);

	/*	For BABs, the security source is the sending node
	 *	(the local node) and the security destination is the
	 *	topologically adjacent (neighboring) node to which the
	 *	bundle is being sent.					*/

	fromEid = bsp_getLocalAdminEid(ctxt->proxNodeEid);
	if (fromEid == NULL)
	{
		fromEid = "";
	}

	toEid = ctxt->proxNodeEid;

	/*	Have now got BAB security source and destination.	*/

	babGetCiphersuite(fromEid, toEid, &babRule, &cs);
	if (cs == NULL)
	{
		scratchExtensionBlock(blk);
		return 0;
	}

	if (blk->occurrence == 1)	/*	Last BAB.		*/
	{
		if (!cs->blockPair)	/*	Lone-BAB ciphersuite.	*/
		{
			BAB_DEBUG_INFO("i babProcessOnDequeue: only a \
single BAB for ciphersuite '%s'.", babRule.ciphersuiteName);
			scratchExtensionBlock(blk);
			return 0;
		}

		/*	Must be last block in the bundle.		*/

		blk->blkProcFlags |= BLK_IS_LAST;
	}

	/*	The BAB is to be attached to the bundle, so
	 *	construct it as the extension block's scratchpad.	*/

	memset((char *) &asb, 0, sizeof(BspOutboundBlock));
	asb.targetBlockType = 0;	/*	Entire bundle.		*/
	asb.targetBlockOccurrence = 0;
	memcpy(asb.keyName, babRule.keyName, BSP_KEY_NAME_LEN);
	if (cs->construct(blk, &asb) < 0)
	{
        	BAB_DEBUG_ERR("x babProcessOnDequeue: Can't build ASB.",
				NULL);
		return -1;
	}

	blk->object = sdr_malloc(bpSdr, sizeof(BspOutboundBlock));
	if (blk->object == 0)
	{
		putErrmsg("No space for BSP object.", NULL);
		return -1;
	}

	sdr_write(bpSdr, blk->object, (char *) &asb, sizeof(BspOutboundBlock));
	blk->size = sizeof(BspOutboundBlock) + asb.parmsLen + asb.resultsLen;

	/*	Now generate serialized ASB from the scratchpad.	*/

	serializedAsb = bsp_serializeASB(&(blk->dataLength), &asb);
	if (serializedAsb == NULL)
	{
		BAB_DEBUG_ERR("x babProcessOnDequeue: Unable to \
serialize ASB. blk->dataLength = %d", blk->dataLength);
		return -1;
	}

	/*	Serialized ASB is the block-specific data for the BAB.	*/

	result = serializeExtBlk(blk, NULL, (char *) serializedAsb);
	MRELEASE(serializedAsb);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_babProcessOnDequeue
 *
 * \par Purpose: This callback constructs a BAB block for inclusion
 * 		 in a bundle right before the bundle is transmitted.
 *
 * 		 For the first BAB in the bundle, this means building
 * 		 an abstract security block and serializing it into
 * 		 the block's bytes array, except that if there is no
 * 		 BAB production rule in the ION security database for
 * 		 this bundle's sender/receiver pair then the BAB block
 * 		 is simply destroyed.
 *
 * 		 Fort the last BAB in the bundle, this means building
 * 		 an abstract security block and serializing it into
 * 		 the block's bytes array, except that if there is no
 * 		 BAB production rule in the ION security database for
 * 		 this bundle's sender/receiver pair -- or if that
 * 		 rule cites a ciphersuite that mandates a lone
 *               BAB -- then the BAB block is simply destroyed.
 *
 * \retval int 0 - The block was successfully constructed or destroyed
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk		The block whose abstract security block
 * 				structure may be built and then
 * 				serialized into the block's bytes array.
 * \param[in]      bundle	The bundle holding the block.
 * \param[in]      parm		The dequeue context.
 *
 * \par Notes:
 *****************************************************************************/

int	bsp_babProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *ctxt)
{
	int	result;

	BAB_DEBUG_PROC("+ bsp_babProcessOnDequeue(%d, %x, %x, %x)",
			blk->occurrence, (unsigned long) blk,
			(unsigned long) bundle, (unsigned long) ctxt);
	result = babProcessOnDequeue(blk, bundle, ctxt);
	BAB_DEBUG_PROC("- bsp_babProcessOnDequeue(%d, %d)", blk->occurrence,
			result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_babProcessOnTransmit
 *
 * \par Purpose: This callback inserts the security result to be associated
 *               with a given BAB block for a given bundle, if the BAB is
 *               a lone or last BAB; otherwise the callback simply returns
 *               without revising the BAB.  The security result cannot be
 *               calculated until the entire bundle has been serialized and
 *               is ready to transmit.  This callback will use the BAB(s)
 *               serialized from the bsp_babProcessOnDequeue function to
 *               calculate the security result.
 *
 * \retval int 0 - The security result was successfully inserted if needed
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk     The block whose abstract security block structure
 *                         will be populated and then serialized into the
 *                         bundle.
 * \param[in]      bundle  The bundle holding the block.
 *
 * \par Notes:
 *      1. For a lone or last BAB, this must be the last block modification
 *         performed on the bundle before it is transmitted.
 *****************************************************************************/

int	bsp_babProcessOnTransmit(ExtensionBlock *blk, Bundle *bundle,
		void *ctxt)
{
	Sdr			sdr = getIonsdr();
	BspOutboundBlock	asb;
	BabCiphersuite		*cs;
	int			result;

	CHKERR(blk);
	CHKERR(bundle);

	BAB_DEBUG_PROC("+ bsp_babProcessOnTransmit: %d, %x, %x, %x)",
			blk->occurrence, (unsigned long) blk,
			(unsigned long) bundle, (unsigned long) ctxt);

	if (blk->object == 0)
	{
		BAB_DEBUG_ERR("x bsp_babProcessOnTransmit: No ASB, can't \
process the BAB.", NULL);
		result = 0;
	}
	else
	{
		sdr_read(sdr, (char *) &asb, blk->object,
				sizeof(BspOutboundBlock));
		cs = get_bab_cs_by_number(asb.ciphersuiteType);
		result = cs->sign(bundle, blk, &asb);
	}

	BAB_DEBUG_PROC("- bsp_babProcessOnTransmit--> %d, %d", blk->occurrence,
			result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_babRelease
 *
 * \par Purpose: This callback releases SDR heap space allocated to
 *               a bundle authentication block.
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated SDR heap space must be
 *                      released.
 *
 * \par Notes:
 *      1. This same function is used for the first and last BAB blocks.
 *
 *****************************************************************************/

void    bsp_babRelease(ExtensionBlock *blk)
{
	Sdr			sdr = getIonsdr();
	BspOutboundBlock	asb;

	BAB_DEBUG_PROC("+ bsp_babRelease(%x)", (unsigned long) blk);

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

	BAB_DEBUG_PROC("- bsp_babRelease(%c)", ' ');
}

/******************************************************************************
 *
 * \par Function Name: bsp_babAcquire
 *
 * \par Purpose: This callback is called when a serialized BAB bundle is
 *               encountered during bundle reception.  This callback will
 *               deserialize the block into a scratchpad object.
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

int	bsp_babAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BAB_DEBUG_PROC("+ bsp_babAcquire(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);
	result = bsp_deserializeASB(blk, wk);
	BAB_DEBUG_INFO("i bsp_babAcquire: Deserialize result %d", result);

	BAB_DEBUG_PROC("- bsp_babAcquire -> %d", result);

	return result;
}

static int	babCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
	char		*fromEid;
	char		*toEid;
	BspBabRule	babRule;
	BabCiphersuite	*cs;
	LystElt		lastBab;
	BspInboundBlock	*asb;

	CHKERR(blk);
	CHKERR(wk);

	/*	For BABs, the security destination is the receiving
	 *	node (the local node) and the security source is the
	 *	topologically adjacent (neighboring) node from which
	 *	the bundle was sent.					*/

	toEid = bsp_getLocalAdminEid(wk->senderEid);
	if (toEid == NULL)
	{
		toEid = "";
	}

	fromEid = wk->senderEid;
	if (fromEid == NULL)
	{
		fromEid = "";
	}

	/*	Given sender & receiver EIDs, get applicable BAB rule.	*/

	babGetCiphersuite(fromEid, toEid, &babRule, &cs);
	if (cs == NULL)
	{
		discardExtensionBlock(blk);
		return 1;
	}

	if (blk->occurrence == 1) /*	Not inital BAB for the bundle.	*/
	{
		if (!cs->blockPair)
		{
			BAB_DEBUG_INFO("i babCheck: 'Last' BAB is invalid \
for ciphersuite '%s'.  Discarding the BAB block.", babRule.ciphersuiteName);
			discardExtensionBlock(blk);
			return 0;
		}

		/*	This is a ciphersuite for which a 'Last' BAB
		 *	is valid.  Make sure this BAB is the only
		 *	'Last' BAB for the bundle.			*/ 

		lastBab = findAcqExtensionBlock(wk, EXTENSION_TYPE_BAB, 1);
		if ((AcqExtBlock *) (lyst_data(lastBab)) != blk)
		{
			BAB_DEBUG_INFO("i babCheck: Multiple 'Last' BABs \
in the bundle.  Discarding this one.", NULL);
			discardExtensionBlock(blk);
			return 0;
		}
	}

	/*	Fill in missing information in the scratchpad area. 	*/

	asb = (BspInboundBlock *) (blk->object);
	memcpy((char *) (asb->keyName), babRule.keyName, BSP_KEY_NAME_LEN);

	/*	Invoke ciphersuite-specific check procedure.  For the
	 *	initial BAB of paired-BAB ciphersuite, this may be a
	 *	no-op.							*/

	return cs->verify(wk, blk);
}

/******************************************************************************
 *
 * \par Function Name: bsp_babCheck
 *
 * \par Purpose: This callback checks a BAB received in a bundle.
 *
 *               At this point the entire bundle has been received
 *               into a zero-copy object, so we can compute the security
 *               result for the bundle as received, for comparison with
 *               the security result as transmitted.
 *
 *               Processing depends on the applicable ciphersuite.  If
 *               the ciphersuite mandates a lone BAB then the first BAB
 *               must contain the transmitted security result and BAB
 *               verification takes place at this time.  No second BAB
 *               will have been acquired, as it would have violated the
 *               ciphersuite and been discarded.
 *
 *               Otherwise (the ciphersuite mandates a BAB pair), the
 *               bundle must contain a second BAB.  If it does not, then
 *               the first BAB must be simply discarded.  If it does, then
 *               when the Check callback is invoked for the first BAB the
 *               expected security result must be computed and stored in
 *               the scratchpad area of that initial BAB; this result,
 *               together with any key information encoded in the block's
 *               security parameters, must be retained for use in checking
 * 		 the last BAB, which will contain the transmitted security
 * 		 result.  When the Check callback is invoked for the last
 * 		 BAB, the transmitted security result is compared with
 * 		 the computed security result from the first BAB.
 *
 * \retval int 0 - The BAB is invalid.
 *             1 - The check was inconclusive.
 *             2 - The check failed: bundle inauthentic.
 *             3 - The check succeeded: bundle authentic.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition BAB being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and
 *                  the rest of the received bundle.
 *
 * \par Notes:
 *      1.
 *****************************************************************************/

int	bsp_babCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BAB_DEBUG_PROC("+ bsp_babCheck(%d, %x, %x)", blk->occurrence,
			(unsigned long) blk, (unsigned long) wk);
	result = babCheck(blk, wk);
	BAB_DEBUG_PROC("- bsp_babCheck --> %d, %d", blk->occurrence, result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_babClear
 *
 * \par Purpose: This callback removes all memory allocated by the BSP module
 *               during the block's acquisition process. This function is the
 *               same for both FIRST and LAST BAB blocks.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the BSP module.
 *****************************************************************************/

void	bsp_babClear(AcqExtBlock *blk)
{
	BspInboundBlock	*asb;

	BAB_DEBUG_PROC("+ bsp_babClear(%d, %x)", blk->occurrence,
			(unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
       		asb = (BspInboundBlock *) (blk->object);
		if (asb->parmsData)
		{
			BAB_DEBUG_INFO("i bsp_babClear: Release parms len %ld",
					asb->parmsLen);
			MRELEASE(asb->parmsData);
		}

		if (asb->resultsData)
		{
			BAB_DEBUG_INFO("i bsp_babClear: Release result len %ld",
					asb->resultsLen);
			MRELEASE(asb->resultsData);
		}

		MRELEASE(blk->object);
		blk->object = NULL;
		blk->size = 0;
	}

	BAB_DEBUG_PROC("- bsp_babClear %d", blk->occurrence, NULL);
}
