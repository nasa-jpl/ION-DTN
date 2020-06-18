/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 ** \file bpsec_util.c
 ** 
 ** File Name: bpsec_util.c (originally extbsputil.c)
 **
 **
 ** Subsystem:
 **          Extensions: bpsec
 **
 ** Description: This file provides a partial implementation of
 **		 Bundle Security Protocol (bpsec).
 **
 **              This implementation utilizes the ION Extension Interface to
 **              manage the creation, modification, evaluation, and removal
 **              of bpsec blocks in Bundle Protocol bundles.
 **
 ** Notes:
 **         As of November 2019, the ION bpsec implementation supports
 **         the Block Integrity Block (BIB) and Block Confidentiality Block
 **         (BCB) with the following constraints:
 **         - Only the contexts implemented in the profiles.c file
 **           are supported.
 **         - There is no support for the use of multiple contexts to
 **           offer or acquire bpsec blocks of a given type in the bundle
 **           traffic between a given security source and a given bundle
 **           destination.  That is, the context to be used for offering
 **           or acquiring a bpsec block is a function of the block type,
 **           the security source node, and the bundle destination node.
 **           When a bundle arrives at its destination node, any bpsec
 **	      block in that bundle that was offered in the context of a
 **	      context other than the one that ION would select for
 **	      that block type's type and security source, and that
 **	      bundle's destination, is silently discarded.
 **
 ** Assumptions:
 **      1. We assume that this code is not under such tight profiling
 **         constraints that sanity checks are too expensive.
 **
 **      2. As a general rule, abstract security block structures are created
 **         and used as the unit of scratchpad information for the extension
 **         block.  While less efficient, this provides easy maintainability.
 **         As such, we assume that the time and space necessary to use the
 **         scratchpad in this way does not exceed available margin.
 **      
 **      3. We assume that the extensions interface never passes us a NULL 
 **         value.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 **  --------  ------------  -----------------------------------------------
 **  06/08/09  E. Birrane           Initial Implementation of BAB blocks. (JHU/APL)
 **  06/15/09  E. Birrane           BAB Unit Testing and Documentation updates. (JHU/APL)
 **  06/20/09  E. Birrane           Documentation updates for initial release. (JHU/APL)
 **  12/04/09  S. Burleigh          Revisions per DINET and DEN testing.
 **  01/14/11  B. Van Besien        Revised to use old security syntax. (JHU/APL)
 **  01/14/14  S. Burleigh          Revised for "streamlined" BSP.
 **  01/23/16  E. Birrane           Update to SBSP
 **  09/02/19  S. Burleigh          Rename everything for bpsec
 **                                 [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_util.h"

/*****************************************************************************
 *                            VARIABLE DEFINITIONS                           *
 *****************************************************************************/

/** \var gMsg
 * Global variable used to hold a constructed error message. NOT RE-ENTRANT! 
 * This is accessed by the BPSEC_DEBUG macros.
 */
char	gMsg[GMSG_BUFLEN];

/*****************************************************************************
 *                              GENERAL FUNCTIONS                            *
 *****************************************************************************/

void	bpsec_releaseInboundTlvs(Lyst tlvs)
{
	LystElt			elt;
	sci_inbound_tlv		*tlv;

	if (tlvs)
	{
		while ((elt = lyst_first(tlvs)))
		{
			tlv = (sci_inbound_tlv *) lyst_data(elt);
			MRELEASE(tlv->value);
			MRELEASE(tlv);
			lyst_delete(elt);
		}

		lyst_destroy(tlvs);
	}
}

void	bpsec_releaseInboundTargets(Lyst targets)
{
	LystElt			elt;
	BpsecInboundTarget	*target;

	if (targets)
	{
		while ((elt = lyst_first(targets)))
		{
			target = (BpsecInboundTarget *) lyst_data(elt);
			bpsec_releaseInboundTlvs(target->results);
			MRELEASE(target);
			lyst_delete(elt);
		}

		lyst_destroy(targets);
	}
}

void	bpsec_releaseInboundAsb(BpsecInboundBlock *asb)
{
	if (asb)
	{
		eraseEid(&asb->securitySource);
		bpsec_releaseInboundTargets(asb->targets);
		bpsec_releaseInboundTlvs(asb->parmsData);
		MRELEASE(asb);
	}
}

void	bpsec_releaseOutboundTlvs(Sdr sdr, Object items)
{
	Object	elt;
	Object	obj;
		OBJ_POINTER(BpsecOutboundTlv, tlv);

	CHKVOID(sdr);
	if (items)
	{
		while ((elt = sdr_list_first(sdr, items)))
		{
			obj = sdr_list_data(sdr, elt);
			if (obj)
			{
				GET_OBJ_POINTER(sdr, BpsecOutboundTlv, tlv,
						obj);
				sdr_free(sdr, tlv->value);
				sdr_free(sdr, obj);
			}

			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		sdr_list_destroy(sdr, items, NULL, NULL);
	}
}

void	bpsec_releaseOutboundTargets(Sdr sdr, Object targets)
{
	Object	elt;
	Object	obj;
		OBJ_POINTER(BpsecOutboundTarget, target);

	CHKVOID(sdr);
	if (targets)
	{
		while ((elt = sdr_list_first(sdr, targets)))
		{
			obj = sdr_list_data(sdr, elt);
			if (obj)
			{
				GET_OBJ_POINTER(sdr, BpsecOutboundTarget,
						target, obj);
				bpsec_releaseOutboundTlvs(sdr, target->results);
				sdr_free(sdr, obj);
			}

			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		sdr_list_destroy(sdr, targets, NULL, NULL);
	}
}

void	bpsec_releaseOutboundAsb(Sdr sdr, Object obj)
{
	BpsecOutboundBlock	asb;

	if (obj)
	{
		sdr_read(sdr, (char *) &asb, obj, sizeof(BpsecOutboundBlock));
		eraseEid(&asb.securitySource);
		if (asb.targets)
		{
			bpsec_releaseOutboundTargets(sdr, asb.targets);
		}

		if (asb.parmsData)
		{
			bpsec_releaseOutboundTlvs(sdr, asb.parmsData);
		}
	
		sdr_free(sdr, obj);
	}
}

int	bpsec_recordAsb(ExtensionBlock *newBlk, AcqExtBlock *oldBlk)
{
	Sdr			sdr = getIonsdr();
	BpsecInboundBlock	*oldAsb;
	BpsecOutboundBlock	newAsb;
	char			*eidBuf;
	MetaEid			meid;
	int			result;
	VScheme			*vscheme;
	PsmAddress		schemeElt;
	LystElt			elt;
	BpsecInboundTarget	*oldTarget;
	BpsecOutboundTarget	newTarget;
	LystElt			elt2;
	sci_inbound_tlv		*oldTlv;
	BpsecOutboundTlv	newTlv;
	Object			obj;

	BPSEC_DEBUG_PROC("+ bpsec_record(0x%x, 0x%x)", (unsigned long) newBlk,
			(unsigned long) oldBlk);

	/* Step 1 - Sanity Checks.					*/
	CHKERR(newBlk);
	CHKERR(oldBlk);
	if (oldBlk->object == NULL || oldBlk->size == 0)
	{
		/*	Nothing to do.					*/

		newBlk->object = 0;
		newBlk->size = 0;
		return 0;
	}

	/* Step 2 - Allocate the new in-heap ASB.			*/
	newBlk->size = sizeof(newAsb);
	if ((newBlk->object = sdr_malloc(sdr, sizeof newAsb)) == 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_record: Failed to allocate: %d",
				sizeof(newAsb));
		BPSEC_DEBUG_PROC("- bpsec_record -> -1", NULL);
		return -1;
	}

	/*  Step 3 - Copy the received ASB into the in-heap block.	*/

	/* Step 3.1 - Copy the fixed data.				*/
	oldAsb = (BpsecInboundBlock *) (oldBlk->object);
	memset((char *) &newAsb, 0, sizeof newAsb);
	newAsb.contextId = oldAsb->contextId;
	memcpy(newAsb.keyName, oldAsb->keyName, BPSEC_KEY_NAME_LEN);
	newAsb.contextFlags = oldAsb->contextFlags;

	/* Step 3.2 - Copy non-fixed data from received ASB.		*/

	/*	First, security source if it's not the bundle source.	*/

	if (oldAsb->securitySource.schemeCodeNbr)	/*	Waypoint*/
	{
		if (readEid(&oldAsb->securitySource, &eidBuf) < 0)
		{
			return -1;
		}

		CHKERR(parseEidString(eidBuf, &meid, &vscheme, &schemeElt));
		result = writeEid(&newAsb.securitySource, &meid);
		restoreEidString(&meid);
		MRELEASE(eidBuf);
       		if (result < 0)
		{
			return -1;
		}
	}

	/*	Next, targets.						*/

	newAsb.targets = sdr_list_create(sdr);
	if (newAsb.targets == 0) return -1;
	for (elt = lyst_first(oldAsb->targets); elt; elt = lyst_next(elt))
	{
		oldTarget = (BpsecInboundTarget *) lyst_data(elt);
		newTarget.targetBlockNumber = oldTarget->targetBlockNumber;
		newTarget.targetBlockType = oldTarget->targetBlockType;
		newTarget.metatargetBlockNumber =
				oldTarget->metatargetBlockNumber;
		newTarget.metatargetBlockType = oldTarget->metatargetBlockType;

		/*	Pending full implementation of target
		 *	multiplicity:					*/

		newBlk->tag1 = newTarget.targetBlockType;
		newBlk->tag2 = newTarget.metatargetBlockType;
		newBlk->tag3 = 0;

		/*	For each target, record all results (TLVs).	*/

		newTarget.results = sdr_list_create(sdr);
		if (newTarget.results == 0)
		{
			return -1;
		}

		for (elt2 = lyst_first(oldTarget->results); elt2;
				elt2 = lyst_next(elt2))
		{
			oldTlv = (sci_inbound_tlv *) lyst_data(elt2);
			newTlv.id = oldTlv->id;
			newTlv.length = oldTlv->length;
			newTlv.value = sdr_malloc(sdr, newTlv.length);
			if (newTlv.value == 0)
			{
				return -1;
			}

			sdr_write(sdr, newTlv.value, oldTlv->value,
					newTlv.length);
			obj = sdr_malloc(sdr, sizeof newTlv);
			if (obj == 0)
			{
				return -1;
			}

			sdr_write(sdr, obj, (char *) &newTlv, sizeof newTlv);
			if (sdr_list_insert_last(sdr, newTarget.results, obj)
					== 0)
			{
				return -1;
			}
		}

		obj = sdr_malloc(sdr, sizeof newTarget);
		if (obj == 0)
		{
			return -1;
		}

		sdr_write(sdr, obj, (char *) &newTarget, sizeof newTarget);
		if (sdr_list_insert_last(sdr, newAsb.targets, obj) == 0)
		{
			return -1;
		}
	}

	/*	Finally, parms.						*/

	newAsb.parmsData = sdr_list_create(sdr);
	if (newAsb.parmsData == 0)
	{
		return -1;
	}

	if (oldAsb->parmsData)
	{
		for (elt2 = lyst_first(oldAsb->parmsData); elt2;
				elt2 = lyst_next(elt2))
		{
			oldTlv = (sci_inbound_tlv *) lyst_data(elt2);
			newTlv.id = oldTlv->id;
			newTlv.length = oldTlv->length;
			newTlv.value = sdr_malloc(sdr, newTlv.length);
			if (newTlv.value == 0)
			{
				return -1;
			}

			sdr_write(sdr, newTlv.value, oldTlv->value,
					newTlv.length);
			obj = sdr_malloc(sdr, sizeof newTlv);
			if (obj == 0)
			{
				return -1;
			}

			sdr_write(sdr, obj, (char *) &newTlv, sizeof newTlv);
			if (sdr_list_insert_last(sdr, newTarget.results, obj)
					== 0)
			{
				return -1;
			}
		}
	}

	/* Step 4 - Write copied block to the SDR. */

	sdr_write(sdr, newBlk->object, (char *) &newAsb, sizeof newAsb);
	BPSEC_DEBUG_PROC("- bpsec_record -> 0", NULL);
	return 0;
}

int	bpsec_copyAsb(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr			sdr = getIonsdr();
	BpsecOutboundBlock	oldAsb;
	BpsecOutboundBlock	newAsb;
	char			*eidBuf;
	MetaEid			meid;
	int			result;
	VScheme			*vscheme;
	PsmAddress		schemeElt;
	Object			elt;
	Object			obj;
	BpsecOutboundTarget	oldTarget;
	BpsecOutboundTarget	newTarget;
	Object			elt2;
	Object			obj2;
	BpsecOutboundTlv	oldTlv;
	BpsecOutboundTlv	newTlv;
	char			*valBuf;

	BPSEC_DEBUG_PROC("+ bpsec_copy(0x%x, 0x%x)", (unsigned long) newBlk,
			(unsigned long) oldBlk);

	/* Step 1 - Sanity Checks. */
	CHKERR(newBlk);
	CHKERR(oldBlk);

	/* Step 2 - Allocate the new destination ASb. */
	newBlk->size = sizeof(newAsb);
	if ((newBlk->object = sdr_malloc(sdr, sizeof newAsb)) == 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_copy: Failed to allocate: %d",
				sizeof(newAsb));
		BPSEC_DEBUG_PROC("- bpsec_copy -> -1", NULL);
		return -1;
	}

	/*  Step 3 - Copy the source ASB into the destination.		*/

	/* Step 3.1 - Read the old ASB from the SDR, copy fixed data.	*/
	sdr_read(sdr, (char *) &oldAsb, oldBlk->object, sizeof(oldAsb));
	memcpy((char *) &newAsb, (char *) &oldAsb, sizeof newAsb);

	/* Step 3.2 - Copy non-fixed data from old ASB to new one.	*/

	/*	First, security source.					*/

	if (oldAsb.securitySource.schemeCodeNbr)	/*	Waypoint*/
	{
		if (readEid(&oldAsb.securitySource, &eidBuf) < 0)
		{
			return -1;
		}

		CHKERR(parseEidString(eidBuf, &meid, &vscheme, &schemeElt));
		result = writeEid(&newAsb.securitySource, &meid);
		restoreEidString(&meid);
		MRELEASE(eidBuf);
		if (result < 0)
		{
			return -1;
		}
	}

	/*	Next, targets.						*/

	newAsb.targets = sdr_list_create(sdr);
	if (newAsb.targets == 0) return -1;
	for (elt = sdr_list_first(sdr, oldAsb.targets); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &oldTarget, obj, sizeof oldTarget);
		memcpy((char *) &newTarget, (char *) &oldTarget,
				sizeof newTarget);

		/*	For each target, copy all results (TLVs).	*/

		newTarget.results = sdr_list_create(sdr);
		if (newTarget.results == 0) return -1;
		for (elt2 = sdr_list_first(sdr, oldTarget.results); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			obj2 = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &oldTlv, obj2, sizeof oldTlv);
			memcpy((char *) &newTlv, (char *) &oldTlv,
					sizeof newTlv);
			newTlv.value = sdr_malloc(sdr, newTlv.length);
			if (newTlv.value == 0) return -1;
			valBuf = MTAKE(oldTlv.length);
			if (valBuf == NULL) return -1;
			sdr_read(sdr, valBuf, oldTlv.value, oldTlv.length);
			sdr_write(sdr, newTlv.value, valBuf, newTlv.length);
			MRELEASE(valBuf);
			obj2 = sdr_malloc(sdr, sizeof newTlv);
			if (obj2 == 0) return -1;
			sdr_write(sdr, obj2, (char *) &newTlv, sizeof newTlv);
			if (sdr_list_insert_last(sdr, newTarget.results, obj2)
					== 0) return -1;
		}

		obj = sdr_malloc(sdr, sizeof newTarget);
		if (obj == 0) return -1;
		sdr_write(sdr, obj, (char *) &newTarget, sizeof newTarget);
		if (sdr_list_insert_last(sdr, newAsb.targets, obj) == 0)
		{
			return -1;
		}
	}

	/*	Finally, parms.						*/

	newAsb.parmsData = sdr_list_create(sdr);
	if (newAsb.parmsData == 0) return -1;
	for (elt2 = sdr_list_first(sdr, oldAsb.parmsData); elt2;
			elt2 = sdr_list_next(sdr, elt2))
	{
		obj2 = sdr_list_data(sdr, elt2);
		sdr_read(sdr, (char *) &oldTlv, obj2, sizeof oldTlv);
		memcpy((char *) &newTlv, (char *) &oldTlv, sizeof newTlv);
		newTlv.value = sdr_malloc(sdr, newTlv.length);
		if (newTlv.value == 0) return -1;
		valBuf = MTAKE(oldTlv.length);
		if (valBuf == NULL) return -1;
		sdr_read(sdr, valBuf, oldTlv.value, oldTlv.length);
		sdr_write(sdr, newTlv.value, valBuf, newTlv.length);
		MRELEASE(valBuf);
		obj2 = sdr_malloc(sdr, sizeof newTlv);
		if (obj2 == 0) return -1;
		sdr_write(sdr, obj2, (char *) &newTlv, sizeof newTlv);
		if (sdr_list_insert_last(sdr, newAsb.parmsData, obj2) == 0)
		{
			return -1;
		}
	}

	/* Step 4 - Write copied block to the SDR. */

	sdr_write(sdr, newBlk->object, (char *) &newAsb, sizeof newAsb);
	BPSEC_DEBUG_PROC("- bpsec_copy -> 0", NULL);
	return 0;
}

int	bpsec_getInboundTarget(Lyst targets, BpsecInboundTarget **target)
{
	LystElt	elt;

	/*	NOTE: at this time we are assuming that each BPSEC
	 *	block has only a single target.  We don't yet know
	 *	how to implement BPSEC for multi-target blocks.		*/

	CHKERR(targets && target);
	elt = lyst_first(targets);
	CHKERR(elt);
	*target = (BpsecInboundTarget *) lyst_data(elt);
	return 0;
}

int	bpsec_getOutboundTarget(Sdr sdr, Object targets,
		BpsecOutboundTarget *target)
{
	Object	elt;
	Object	obj;

	/*	NOTE: at this time we are assuming that each BPSEC
	 *	block has only a single target.  We don't yet know
	 *	how to implement BPSEC for multi-target blocks.		*/

	CHKERR(sdr && targets && target);
	elt = sdr_list_first(sdr, targets);
	CHKERR(elt);
	obj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) target, obj, sizeof(BpsecOutboundTarget));
	return 0;
}

static int	appendItem(Sdr sdr, Object items, sci_inbound_tlv *itlv)
{
	BpsecOutboundTlv	otlv;
	Object			tlvAddr;

	otlv.id = itlv->id;
	otlv.length = itlv->length;
	tlvAddr = sdr_malloc(sdr, sizeof(BpsecOutboundTlv));
	if (tlvAddr == 0
	|| ((otlv.value = sdr_malloc(sdr, otlv.length)) == 0)
	|| sdr_list_insert_last(sdr, items, tlvAddr) == 0)
	{
		BPSEC_DEBUG_ERR("appendItem: Can't allocate sdr item of \
length %d.", itlv->length);
		return -1;
	}

	sdr_write(sdr, otlv.value, (char *) (itlv->value), otlv.length);
	sdr_write(sdr, tlvAddr, (char *) &otlv, sizeof(BpsecOutboundTlv));
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_write_parms
 *
 * \par Purpose: This utility function writes the parms in an
 *		 sci_inbound_parms structure to the SDR heap as
 *		 BpsecOutboundTlv structures, appending them to the
 *		 parmsData sdrlist of a bpsec outbound ASB.
 *
 * \par Date Written:  2/27/2016
 *
 * \retval SdrObject -- The number of parms written, or -1 on error.
 *
 * \param[in|out] sdr    The SDR Storing the result.
 * \param[in]     parms  The structure holding parameters.
 * \param[in]     items  The parmsData list
 *
 * \par Notes:
 *      1. The SDR objects must be freed when no longer needed.
 *      2. Only non-zero fields in the parm structure will be included
 *      3. We do not support content range.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_write_parms(Sdr sdr, BpsecOutboundBlock *asb,
		sci_inbound_parms *parms)
{
	int	result = 0;

	/* Step 0 - Sanity Check. */

	CHKERR(sdr && asb && parms);

	/* Step 2 - Allocate the SDR space. */

	if (parms->iv.length > 0)
	{
		if (appendItem(sdr, asb->parmsData, &parms->iv) < 0)
		{
			return -1;
		}
	}

	if (parms->salt.length > 0)
	{
		if (appendItem(sdr, asb->parmsData, &parms->salt) < 0)
		{
			return -1;
		}
	}

	if (parms->icv.length > 0)
	{
		if (appendItem(sdr, asb->parmsData, &parms->icv) < 0)
		{
			return -1;
		}
	}

	if (parms->intsig.length > 0)
	{
		if (appendItem(sdr, asb->parmsData, &parms->intsig) < 0)
		{
			return -1;
		}
	}

	if (parms->aad.length > 0)
	{
		if (appendItem(sdr, asb->parmsData, &parms->aad) < 0)
		{
			return -1;
		}
	}

	if (parms->keyinfo.length > 0)
	{
		if (appendItem(sdr, asb->parmsData, &parms->keyinfo) < 0)
		{
			return -1;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_write_one_result
 *
 * \par Purpose: This utility function builds a result item and appends it
 *		 to the non-volatile linked list of result items for the
 *		 block's initial target.
 *
 * \par Date Written:  2/27/2016
 *
 * \retval int -- 0 on success, -1 on error
 *
 * \param[in|out] sdr    The SDR storing the result.
 * \param[in]     asb    The block to append the result for.
 * \param[in]     tlv    The item to write.
 *
 * \par Notes:
 *      1. The SDR object must be freed when no longer needed.
 *      2. This function does not use an SDR transaction.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_write_one_result(Sdr sdr, BpsecOutboundBlock *asb,
		sci_inbound_tlv *tlv)
{
	BpsecOutboundTarget	target;

	CHKERR(sdr && asb && tlv);
	if (bpsec_getOutboundTarget(sdr, asb->targets, &target) < 0)
	{
		putErrmsg("Can't retrieve outbound target", NULL);
		return -1;
	}

	return appendItem(sdr, target.results, tlv);
}

/******************************************************************************
 *
 * \par Function Name: bpsec_insert_target
 *
 * \par Purpose: This utility function inserts a single target for a
 * 		 BPSEC block.
 *
 * \par Date Written:  11/27/2019
 *
 * \retval int -- 0 on success, -1 on error
 *
 * \param[in|out] sdr    The SDR storing the result.
 * \param[in]     asb    The block to attach the target to.
 * \param[in]     nbr    The target block's number.
 * \param[in]     typ    The target block's type.
 * \param[in]     mnbr   The target block's own target's block number.
 * \param[in]     mtyp   The target block's own target's block type.
 *
 * \par Notes:
 *      1. The SDR object must be freed when no longer needed.
 *      2. This function does not use an SDR transaction.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  11/27/19  S. Burleigh   Initial Implementation
 *****************************************************************************/

int	bpsec_insert_target(Sdr sdr, BpsecOutboundBlock *asb, uint8_t nbr,
		uint8_t typ, uint8_t mnbr, uint8_t mtyp)
{
	Object			obj;
	BpsecOutboundTarget	target;

	CHKERR(sdr && asb);
	target.targetBlockNumber = nbr;
	target.targetBlockType = typ;
	target.metatargetBlockNumber = mnbr;
	target.metatargetBlockType = mtyp;
	target.results = sdr_list_create(sdr);
	if (target.results == 0)
	{
		putErrmsg("Can't allocate results list.", NULL);
		return -1;
	}

	obj = sdr_malloc(sdr, sizeof(BpsecOutboundTarget));
	if (obj == 0)
	{
		sdr_free(sdr, target.results);
		putErrmsg("Can't allocate target object.", NULL);
		return -1;
	}

	sdr_write(sdr, obj, (char *) &target, sizeof(BpsecOutboundTarget));
	if (sdr_list_insert_last(sdr, asb->targets, obj) == 0)
	{
		sdr_free(sdr, obj);
		sdr_free(sdr, target.results);
		putErrmsg("Can't append target to ASB.", NULL);
		return -1;
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_deserializeASB
 *
 * \par Purpose: This utility function accepts an Acquisition extension
 * 		 block in the course of bundle acquisition, parses that
 * 		 block's serialized content into an Abstract Security
 * 		 Block structure, and stores that structure as the
 * 		 scratchpad object of the extension block.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int -- 1 - An ASB was successfully deserialized into the scratchpad
 *                0 - The deserialized ASB did not pass its sanity check.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  A pointer to the acquisition extension block holding
 * 			the serialized abstract security block.
 * \param[in]      wk   Work area holding bundle information.
 *
 * \par Notes: 
 *      1.  This function allocates memory for the scratchpad object
 *          using the MTAKE method.  This memory is released immediately
 *          if the function returns 0 or -1; if the function returns 1
 *          (successful deserialization) then release of this memory must
 *          be ensured by subsequent processing.
 *
 *      2.  If we return a 0, the extension block is considered invalid
 *          and not usable; it should be discarded. The extension block
 *          is not discarded, though, in case the caller wants to examine
 *          it.  (The ASB structure, however, *is* discarded immediately.
 *          It never becomes the "object" of this extension block.)
 *
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Cmts and code cleanup for initial release.
 *  07/26/11  E. Birrane           Added useCbhe and EID ref/deref
 *****************************************************************************/

static void	loseInboundAsb(BpsecInboundBlock *asb)
{
	eraseEid(&(asb->securitySource));
	bpsec_releaseInboundTargets(asb->targets);
	bpsec_releaseInboundTlvs(asb->parmsData);
	MRELEASE(asb);
}

int	bpsec_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int			memIdx = getIonMemoryMgr();
	int			result = 1;
	unsigned char		*cursor = NULL;
	unsigned int		unparsedBytes = 0;
	BpsecInboundBlock	*asb;
	uvast			arrayLength;
	BpsecInboundTarget	*target;
	uvast			uvtemp;
	sci_inbound_tlv		*tv;
	unsigned char		buffer[4096];
	LystElt			elt;

	BPSEC_DEBUG_PROC("+ bpsec_deserializeASB(" ADDR_FIELDSPEC \
"," ADDR_FIELDSPEC "%d)", (uaddr) blk, (uaddr) wk);
	CHKERR(blk);
	CHKERR(wk);
	BPSEC_DEBUG_INFO("i bpsec_deserializeASB blk data length %d",
			blk->dataLength);
	unparsedBytes = blk->dataLength;
       	asb = (BpsecInboundBlock *) MTAKE(sizeof(BpsecInboundBlock));
	if (asb == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for ASB \
scratchpad", NULL);
		return -1;
	}

	memset((char *) asb, 0, sizeof(BpsecInboundBlock));

	/*
	 * Position cursor at start of block-type-specific data of the
	 * extension block, by skipping over the extension block header.
	*/

	cursor = ((unsigned char *)(blk->bytes))
			+ (blk->length - blk->dataLength);

	/*	Get top-level array.					*/

	arrayLength = 0;	/*	May be 4, 5, or 6.		*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode bpsec block");
		loseInboundAsb(asb);
		return 0;
	}

	/*	First item in array is array of target block numbers.	*/

	asb->targets = lyst_create_using(memIdx);
	if (asb->targets == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for ASB \
targets list", NULL);
		loseInboundAsb(asb);
		return -1;
	}

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode bpsec block target array");
		loseInboundAsb(asb);
		return 0;
	}

	while (arrayLength > 0)
	{
		target = (BpsecInboundTarget *)
				MTAKE(sizeof(BpsecInboundTarget));
		if (target == NULL
		|| (target->results = lyst_create_using(memIdx)) == NULL 
		|| lyst_insert_last(asb->targets, target) == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for \
ASB target", NULL);
			loseInboundAsb(asb);
			return -1;
		}

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode bpsec block target nbr");
			loseInboundAsb(asb);
			return 0;
		}

		target->targetBlockNumber = uvtemp;

		/*	NOTE!!!  The next line is a TEMPORARY hack,
		 *	which works for the moment because the only
		 *	types of blocks bpsec can currently operate
		 *	on are Primary and Payload, for which block
		 *	type and block number are the same.  For
		 *	next release, this function or some other
		 *	bpsec function MUST find the block identified
		 *	by targetBlockNumber, obtain its type, and
		 *	set target->targetBlockType to that block
		 *	type number, before any operations are
		 *	performed on this ASB.				*/

		target->targetBlockType = uvtemp;	/*	HACK!	*/
		arrayLength -= 1;
	}

	/*	Second item is security context ID.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode bpsec block context ID");
		loseInboundAsb(asb);
		return 0;
	}

	asb->contextId = uvtemp;

	/*	Third item is security context flags.			*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode bpsec block context flags");
		loseInboundAsb(asb);
		return 0;
	}

	asb->contextFlags = uvtemp;

	BPSEC_DEBUG_INFO("i bpsec_deserializeASB: context %ld, flags %ld, \
length %d", asb->contextId, asb->contextFlags, blk->dataLength);

	/*	Next, possibly, is security source.			*/

	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		switch (acquireEid(&(asb->securitySource), &cursor,
					&unparsedBytes))
		{
		case -1:
			BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for \
security source EID", NULL);
			loseInboundAsb(asb);
			return -1;

		case 0:
			writeMemo("[?] Can't decode bpsec block security src");
			loseInboundAsb(asb);
			return 0;

		default:
			break;
		}
	}

	/*	Then, possibly, context parameters.			*/

	if (asb->contextFlags & BPSEC_ASB_PARM)
	{
		asb->parmsData = lyst_create_using(memIdx);
		if (asb->parmsData == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for \
ASB parms list", NULL);
			loseInboundAsb(asb);
			return -1;
		}

		arrayLength = 0;
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode bpsec block parms array");
			loseInboundAsb(asb);
			return 0;
		}

		while (arrayLength > 0)
		{
			tv = (sci_inbound_tlv *) MTAKE(sizeof(sci_inbound_tlv));
			if (tv == NULL
			|| lyst_insert_last(asb->parmsData, tv) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No \
space for ASB parms TV", NULL);
				loseInboundAsb(asb);
				return -1;
			}

			uvtemp = 2;
			if (cbor_decode_array_open(&uvtemp, &cursor,
					&unparsedBytes) < 1)
			{
				writeMemo("[?] Can't decode parm TV array");
				loseInboundAsb(asb);
				return 0;
			}

			if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
					&unparsedBytes) < 1)
			{
				writeMemo("[?] Can't decode bpsec block \
context parm ID");
				loseInboundAsb(asb);
				return 0;
			}

			tv->id = uvtemp;
			uvtemp = sizeof buffer;
			if (cbor_decode_byte_string(buffer, &uvtemp, &cursor,
					&unparsedBytes) < 1)
			{
				writeMemo("[?] Can't decode bpsec block \
context parm value");
				loseInboundAsb(asb);
				return 0;
			}

			tv->length = uvtemp;
			tv->value = MTAKE(tv->length);
			if (tv->value == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No \
space for ASB parm value", NULL);
				loseInboundAsb(asb);
				return -1;
			}

			memcpy(tv->value, buffer, tv->length);
			arrayLength -= 1;
		}
	}

	/*	Finally, security results.				*/

	arrayLength = 0;
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode bpsec block results array");
		loseInboundAsb(asb);
		return 0;
	}

	/*	arrayLength is the number of targets for which security
	 *	results are provided in this bpsec block.		*/

	if (arrayLength != lyst_length(asb->targets))
	{
		writeMemo("[?] Mismatch between targets and results.");
		loseInboundAsb(asb);
		return 0;
	}

	for (elt = lyst_first(asb->targets); elt; elt = lyst_next(elt))
	{
		target = (BpsecInboundTarget *) lyst_data(elt);

		/*	Each result set (1 per target) is an array
		 *	of results.					*/

		arrayLength = 0;
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode bpsec block result set");
			loseInboundAsb(asb);
			return 0;
		}

		/*	arrayLength is the number of results in the
		 *	result set for this target.			*/

		while (arrayLength > 0)
		{
			/*	Each result is a TV, i.e., an array
			 *	of two items: ID and value.		*/

			tv = (sci_inbound_tlv *) MTAKE(sizeof(sci_inbound_tlv));
			if (tv == NULL
			|| lyst_insert_last(target->results, tv) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No \
space for ASB results TV", NULL);
				loseInboundAsb(asb);
				return -1;
			}

			uvtemp = 2;
			if (cbor_decode_array_open(&uvtemp, &cursor,
				&unparsedBytes) < 1)
			{
				writeMemo("[?] Can't decode bpsec result");
				loseInboundAsb(asb);
				return 0;
			}

			if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
					&unparsedBytes) < 1)
			{
				writeMemo("[?] Can't decode bpsec result ID");
				loseInboundAsb(asb);
				return 0;
			}

			tv->id = uvtemp;
			uvtemp = sizeof buffer;
			if (cbor_decode_byte_string(buffer, &uvtemp, &cursor,
					&unparsedBytes) < 1)
			{
				writeMemo("[?] Can't decode bpsec result \
value");
				loseInboundAsb(asb);
				return 0;
			}

			tv->length = uvtemp;
			tv->value = MTAKE(tv->length);
			if (tv->value == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No \
space for ASB results value", NULL);
				loseInboundAsb(asb);
				return -1;
			}

			memcpy(tv->value, buffer, tv->length);
			arrayLength -= 1;
		}
	}

	blk->size = sizeof(BpsecInboundBlock);
	blk->object = asb;
	BPSEC_DEBUG_PROC("- bpsec_deserializeASB -> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_destinationIsLocal
 *
 * \par Purpose: Determines if the destination of the bundle is the local node.
 *
 * \retval 1 - The bundle destination is local.
 *         0 - The bundle destination is not local.
 *
 * \param[in] The bundle whose destination locality is being checked.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_destinationIsLocal(Bundle *bundle)
{
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	int		result = 0;

	lookUpEidScheme(&(bundle->destination), &vscheme);
	if (vscheme)	/*	EID scheme is known on this node.	*/
	{
		lookUpEndpoint(&(bundle->destination), vscheme, &vpoint);
		if (vpoint)	/*	Node is registered in endpoint.	*/
		{
			result = 1;
		}
	}

	return result;
}

#if 0
/******************************************************************************
 *
 * \par Function Name: bpsec_findAcqBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in
 * 		 an acquisition work area looking for a bpsec block of the
 * 		 indicated type, among whose targets is a block of indicated
 * 		 type and (as relevant) target type.
 *
 * \retval Object
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of bpsec block to look for.
 * \param[in]  targetBlockType		Identifies target of the bpsec block.
 * \param[in]  metatargetBlockType	Identifies target of the target block.
 *
 * \par Notes:
 *****************************************************************************/

LystElt	bpsec_findAcqBlock(AcqWorkArea *wk, BpBlockType type,
		BpBlockType targetBlockType, BpBlockType metatargetBlockType)
{
	LystElt			elt;
	AcqExtBlock		*blk;
	BpsecInboundBlock	*asb;
	LystElt			elt2;
	BpsecInboundTarget	*target;

	CHKZERO(wk);
	for (elt = lyst_first(wk->extBlocks); elt; elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		if (blk->type != type)
		{
			continue;
		}

		asb = (BpsecInboundBlock *) (blk->object);
		for (elt2 = lyst_first(asb->targets); elt2;
				elt2 = lyst_next(elt2))
		{
			target = (BpsecInboundTarget *) lyst_data(elt2);
			if (target->targetBlockType == targetBlockType
			&& target->metatargetBlockType == metatargetBlockType)
			{
				return elt;
			}
		}
	}

	return 0;
}
#endif

/******************************************************************************
 *
 * \par Function Name: bpsec_findBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in a
 * 			bundle looking for a bpsec block of the indicated type,
 *			among whose targets is a block of indicated type and
 *			(as relevant) target type.
 *
 * \retval Object
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of bpsec block to look for.
 * \param[in]  targetBlockType		Identifies target of the bpsec block.
 * \param[in]  metatargetBlockType	Identifies target of the target block.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

Object	bpsec_findBlock(Bundle *bundle, BpBlockType type,
		BpBlockType targetBlockType, BpBlockType metatargetBlockType)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	CHKZERO(bundle);
	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (blk->type == type
		&& blk->tag1 == targetBlockType
		&& blk->tag2 == metatargetBlockType)
		{
			return elt;
		}
#if 0
		/*	Note: when we get target multiplicity fully
		 *	implemented, we will need to use this sort
		 *	of logic to find the requested block.  But
		 *	initially we continue to use the tag fields
		 *	in the ExtensionBlock as search fields.		*/

		Object	elt2;
			OBJ_POINTER(BpsecOutboundBlock, asb);
			OBJ_POINTER(BpsecOutboundTarget, target);

		if (blk->type != type)
		{
			continue;
		}

		GET_OBJ_POINTER(sdr, BpsecOutboundBlock, asb, blk->object);
		for (elt2 = sdr_list_first(sdr, asb->targets); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			addr = sdr_list_data(sdr, elt2);
			GET_OBJ_POINTER(sdr, BpsecOutboundTarget, target, addr);
			if (target->targetBlockType == targetBlockType
			&& target->metatargetBlockType == metatargetBlockType)
			{
				return elt;
			}
		}
#endif
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_getInboundSecurityEids
 *
 * \par Purpose: This function retrieves the inbound and outbound security
 *               EIDs associated with an inbound abstract security block.
 *
 * \retval -1 on fatal error
 *          0 on success (but the returned From and/or To eid may be NULL)
 *
 * \param[in]   bundle   The inbound bundle.
 * \param[in]   blk      The inbound block.
 * \param[in]   asb	 The inbound Abstract Security Block.
 * \param[out]  fromEid  The security source EID to populate.
 * \param[out]  toEid    The bundle destination EID to populate
 *
 * \par Notes:
 *    - The fromEid will be populated from the security block, if the security
 *      block houses such an EID.  Otherwise the bundle source EID will be used.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *  07/14/16  E. Birrane    Update Return Codes.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_getInboundSecurityEids(Bundle *bundle, AcqExtBlock *blk,
		BpsecInboundBlock *asb, char **fromEid, char **toEid)
{
	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(asb);
	CHKERR(fromEid);
	CHKERR(toEid);

	*fromEid = NULL;	/*	Default.			*/
	*toEid = NULL;		/*	Default.			*/

	if (readEid(&(bundle->destination), toEid) < 0)
	{
		return -1;
	}

	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		if (readEid(&(asb->securitySource), fromEid) < 0)
		{
			return -1;
		}
	}
	else
	{
		if (readEid(&bundle->id.source, fromEid) < 0)
		{
			return -1;
		}
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_getLocalAdminEid
 *
 * \par Purpose: Return the administrative endpoint ID for a given EID.
 *
 * \retval The EID, or NULL.
 *
 * \param[in] peerEId - The EID whose administrative endpoint is being requested.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

char	*bpsec_getLocalAdminEid(char *peerEid)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	/*	Look at scheme of peer node, as that will be the
	 *	scheme of our local admin EID, as we don't cross
	 *	schemes in transmit.					*/

	if (peerEid == NULL)
	{
		return NULL;
	}

	if (parseEidString(peerEid, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		/*	Can't know which admin EID to use.		*/
		return NULL;
	}

	restoreEidString(&metaEid);
   	return vscheme->adminEid;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_getOutboundItem
 *
 * \par Purpose: This function searches either within the parmsData list
 *               of an outbound bpsec block for a parameter of
 *               specified type (ID) or within the results list of
 *               one target of an outbound bpsec block for a
 *               result of specified type (ID).
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in
 *                         bpsec_util.h as BPSEC_CSPARM_xxx macros.
 * \param[in]  items       The sdrlist in which to search for the item.
 * \param[in]  tvp         A pointer to a variable in which the function
 *                         should place the address of the first item of
 *                         specified type that is found within the buffer.
 *                         On return, this variable contains 0 if no such
 *                         item was found.
 *
 * \par Notes:
 *****************************************************************************/

void	bpsec_getOutboundItem(uint8_t itemNeeded, Object items, Object *tvp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(BpsecOutboundTlv, tv);

	CHKVOID(items);
	CHKVOID(tvp);
	*tvp = 0;			/*	Default.		*/
	for (elt = sdr_list_first(sdr, items); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BpsecOutboundTlv, tv, addr);
		if (tv->id == itemNeeded)
		{
			*tvp = addr;
			return;
		}
	}
}

/******************************************************************************
 *
 * \par Function Name: bpsec_getOutboundSecurityEids
 *
 * \par Purpose: This function retrieves the inbound and outbound security
 *               EIDs associated with an outbound abstract security block.
 *
 * \retval -1 on Fatal error
 *          0 on success (but the returned From and/or To eid may be NULL)
 *
 * \param[in]   bundle   The outbound bundle.
 * \param[in]   blk      The outbound block.
 * \param[in]   asb	 The outbound Abstract Security Block.
 * \param[out]  fromEid  The security source EID to populate.
 * \param[out]  toEid    The bundle destination EID to populate
 *
 * \par Notes:
 *    - The fromEid will be populated from the security block, if the security
 *      block houses such an EID.  Otherwise, the bundle EID will be used.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *  07/14/16  E. Birrane    Updated return codes.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_getOutboundSecurityEids(Bundle *bundle, ExtensionBlock *blk,
		BpsecOutboundBlock *asb, char **fromEid, char **toEid)
{
	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(asb);
	CHKERR(fromEid);
	CHKERR(toEid);

	*fromEid = NULL;	/*	Default.			*/
	*toEid = NULL;		/*	Default.			*/
	if (readEid(&(bundle->destination), toEid) < 0)
	{
		return -1;
	}

	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		if (readEid(&(asb->securitySource), fromEid) < 0)
		{
			return -1;
		}
	}
	else
	{
		if (readEid(&bundle->id.source, fromEid) < 0)
		{
			return -1;
		}
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_insertSecuritySource
 *
 * \par Purpose: Inserts security source into the Abstract Security Block.
 *		 of an outbound bpsec block.
 *
 * \par Date Written:  TBD
 *
 * \retval	None
 *
 * \param[in]   bundle  The outbound bundle.
 * \param[in]   asb	The outbound Abstract Security Block.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  ----------------------------------------
 *****************************************************************************/

void	bpsec_insertSecuritySource(Bundle *bundle, BpsecOutboundBlock *asb)
{
	char		*eidString;
	char		*adminEid;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	elt;
	int		result;

	CHKVOID(readEid(&bundle->destination, &eidString) == 0);
	adminEid = bpsec_getLocalAdminEid(eidString);
	MRELEASE(eidString);
	CHKVOID(parseEidString(adminEid, &metaEid, &vscheme, &elt));
	result = writeEid(&(asb->securitySource), &metaEid);
	restoreEidString(&metaEid);
       	CHKVOID(result == 0);
}

/******************************************************************************
 *
 * \par Function Name: bpsec_retrieveKey
 *
 * \par Purpose: Retrieves the key associated with a particular keyname.
 *
 * \par Date Written:  6/01/09
 *
 * \retval sci_inbound_tlv -- The key value. Length 0 indicates error.
 *
 * \param[in]  keyName  The name of the key to find.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  ----------------------------------------
 *  06/01/09  E. Birrane    Initial Implementation.
 *  06/06/09  E. Birrane    Documentation Pass.
 *  06/13/09  E. Birrane    Added debugging statements.
 *  06/15/09  E. Birrane    Formatting and comment updates.
 *  06/20/09  E. Birrane    Change to use ION primitives, Added cmts for
 *                          initial release.
 *  03/14/16  E. Birrane    Reworked to use sci_inbound_tlv [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

sci_inbound_tlv	bpsec_retrieveKey(char *keyName)
{
	int		keyLength;
	sci_inbound_tlv	key;
	char		stdBuffer[100];
	int		ReqBufLen = 0;

	BPSEC_DEBUG_PROC("+ bpsec_retrieveKey(0x" ADDR_FIELDSPEC ")",
			(uaddr) keyName);
	/*
	 * We first guess that the key will normally be no more than 100
	 * bytes long, so we call sec_get_key with a buffer of that size.
	 * If this works, great; we make a copy of the retrieved key
	 * value and pass it back.  If not, then the function has told
	 * us what the actual length of this key is; we allocate a new
	 * buffer of sufficient length and call sec_get_key again to
	 * retrieve the key value into that buffer.
	 */

	memset(&key, 0, sizeof(sci_inbound_tlv));

	if(keyName == NULL || strlen(keyName) == 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Bad Parms", NULL);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.length);
		return key;
	}

	ReqBufLen = sizeof(stdBuffer);
	key.length = keyLength = sec_get_key(keyName, &(ReqBufLen), stdBuffer);

	/**
	 *  Step 1 - Check the key length.
	 *           <  0 indicated system failure.
	 *           == 0 indicated key not found.
	 *           > 0  inicates success.
	 */
	if (keyLength < 0)	/* Error. */
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't get length of \
key '%s'.", keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				keyLength);
		return key;
	}
	else if(key.length > 0) /*	Key has been retrieved.		*/
	{
		if((key.value = (unsigned char *) MTAKE(key.length)) == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't allocate \
key of size %d", key.length);
			BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
					key.length);
			return key;
		}

		memcpy(key.value, stdBuffer, key.length);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.length);

		return key;
	}

	/**
	 *  Step 2 - At this point, if we did not find a key and did
	 *  not have a system error, either the key was not found or
	 *  it was found and was larger than the standard buffer.
	 *
	 *  If we ran out of space, the neededBufLen will be less than
	 *  the provided buffer. Otherwise, the neededBufLen will be
	 *  the required size of the buffer to hold the key.		*/

	/* Step 2a - If we did not find a key... */
	if(ReqBufLen <= sizeof(stdBuffer))
	{
		BPSEC_DEBUG_WARN("? bpsec_retrieveKey: Unable to find key '%s'",
				keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey", NULL);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.length);
		return key;
	}

	/* Step 2b - If the buffer was just not big enough, make a
	 * larger buffer and try again.
	 */

	if ((key.value = MTAKE(ReqBufLen)) == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't allocate key of \
size %d", ReqBufLen);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				ReqBufLen);
		return key;
	}

	/* Step 3 - Call get key again and it should work this time. */

	if (sec_get_key(keyName, &ReqBufLen, (char *) (key.value)) <= 0)
	{
		MRELEASE(key.value);
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey:  Can't get key '%s'",
				keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.length);
		return key;
	}

	key.length = ReqBufLen;
	BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.length);
	return key;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_securityPolicyViolated
 *
 * \par Purpose: Determines if an incoming bundle/block has violated security
 *               policy.
 *
 * \retval 1 if the policy was violated.
 *         0 if the policy was not violated.
 *
 * \param[in]  wk  The incominb bundle work area.
 *
 * \par Notes:
 *    - This function is not implemented at this time.
 *    - \todo For each block in the bundle, find the matching BIB rule
 *            If rule found, find BIB for this block. If BIB not found,
 *            return 1.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  ----------------------------------------
 *  03/14/16  E. Birrane    Reworked to use sci_inbound_tlv [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_securityPolicyViolated(AcqWorkArea *wk)
{
	/*	TODO: eventually this function should do something like:
	 *		1.  For each block in the bundle, find matching
	 *		    BIB rule.  If rule found, find BIB for this
	 *		    block.  If BIB not found, return 1.		*/

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_requiredBlockExists
 *
 * \par Purpose: This function searches for the bpsec block that satisfies
		 some bpsec security rule at the time of bundle acquisition..
 *
 * \retval  -1 on Fatal Error.
 *           0 on Failure (block not found).
 *           1 on Success (block was found).
 *
 * \param[in]   wk      	The bundle acquisition structure.
 * \param[in]   bpsecBlockType	The type of block to search for.
 * \param[in]   targetBlockType	The type of block the block must target.
 * \param[in]   secSrcEid	The EID of the node that must have attached
 *				the block to the bundle.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  07/19/18  S. Burleigh   Initial implementation.
 *
 *****************************************************************************/

int	bpsec_requiredBlockExists(AcqWorkArea *wk, BpBlockType bpsecBlockType,
			BpBlockType targetBlockType, char *secSrcEid)
{
	LystElt			elt;
	AcqExtBlock		*blk;
	BpsecInboundBlock	*asb;
	LystElt			elt2;
	BpsecInboundTarget	*target;
	Bundle			*bundle;
	int			result;
	char			*fromEid;

	CHKZERO(wk);
	bundle = &(wk->bundle);
	for (elt = lyst_first(wk->extBlocks); elt; elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		CHKERR(blk);
		if (blk->type != bpsecBlockType)
		{
			continue;		/*	No type match.	*/
		}

		asb = (BpsecInboundBlock *) (blk->object);
		CHKERR(asb);
		for (elt2 = lyst_first(asb->targets); elt2;
				elt2 = lyst_next(elt2))
		{
			target = (BpsecInboundTarget *) lyst_data(elt2);
			CHKERR(target);
			if (target->targetBlockType == targetBlockType)
			{
				break;		/*	Target matches.	*/
			}
		}

		if (elt2 == NULL)	/*	Reached end of list.	*/
		{
			continue;	/*	No target match.	*/
		}

		/*	Now see if source of BIB matches the source
		 *	EID for this BIB rule.				*/

		result = 0;
		fromEid = NULL;
		if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
		{
			if (readEid(&(asb->securitySource), &fromEid) < 0)
			{
				return -1;
			}
		}
		else
		{
			/*	No security source in block, so source
			 *	of BIB is the source of the bundle.	*/

			if (readEid(&bundle->id.source, &fromEid) < 0)
			{
				return -1;
			}
		}

		if (fromEid == NULL)
		{
			continue;	/*	No source EID match.	*/
		}

		if (eidsMatch(secSrcEid, strlen(secSrcEid),
				fromEid, strlen(fromEid)))
		{
			result = 1;
		}

		MRELEASE(fromEid);
		if (result == 1)
		{
			return result;
		}
	}

	/*	Did not find any BIB that satisfies this rule.		*/

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_serializeASB
 *
 * \par Purpose: Serializes an outbound bundle security block and returns the
 *               serialized representation in a private working memory buffer.
 *
 * \par Date Written:  6/03/09
 *
 * \retval unsigned char * - the serialized outbound bundle Security Block.
 *
 * \param[out] length The length of the serialized block, a returned value.
 * \param[in]  asb    The BpsecOutboundBlock to serialize.
 *
 * \par Notes:
 *      1. This function uses MTAKE to allocate space for the serialized ASB.
 *         This serialized ASB (if not NULL) must be freed using MRELEASE.
 *      2. This function only serializes the block-type-specific data of
 *         a bpsec extension block, not the extension block header.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/03/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/13/09  E. Birrane           Added debugging statements.
 *  06/15/09  E. Birrane           Comment updates for DINET-2 release.
 *  06/20/09  E. Birrane           Fixed Debug stmts, pre for initial release.
 *****************************************************************************/

typedef struct
{
	unsigned int	length;
	unsigned char	*text;
} Stv;					/*	Serialized Type/Value	*/

static void	releaseAsbBuffers(unsigned char *serializedTargets,
			unsigned char *serializedSource, Lyst serializedParms,
			unsigned char *serializedParmsBuffer,
			Lyst serializedTlvs, Lyst serializedResults,
			unsigned char *serializedResultsBuffer)
{
	if (serializedTargets) MRELEASE(serializedTargets);
	if (serializedSource) MRELEASE(serializedSource);
	bpsec_releaseInboundTlvs(serializedParms);
	bpsec_releaseInboundTlvs(serializedTlvs);
	if (serializedParmsBuffer) MRELEASE(serializedParmsBuffer);
	bpsec_releaseInboundTlvs(serializedResults);
	if (serializedResultsBuffer) MRELEASE(serializedResultsBuffer);
}

unsigned char	*bpsec_serializeASB(uint32_t *length, BpsecOutboundBlock *asb)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		elt2;
			OBJ_POINTER(BpsecOutboundTarget, target);
	uvast		itemsCount;
	uvast		targetsCount;
	unsigned int	serializedTargetsLen = 0;
	unsigned char	*serializedTargets = NULL;
	unsigned int	serializedResultsLen = 0;
	Lyst		serializedResults = NULL;
	unsigned char	*serializedResultsBuffer = NULL;
	unsigned int	serializedContextIdLen = 0;
	unsigned char	serializedContextId[9];
	unsigned int	serializedContextFlagsLen = 0;
	unsigned char	serializedContextFlags[9];
	unsigned int	serializedSourceLen = 0;
	unsigned char	serializedSource[300];
	unsigned int	serializedParmsLen = 0;
	Lyst		serializedParms = NULL;
	unsigned char	*serializedParmsBuffer = NULL;
	unsigned char	*serializedAsb = NULL;
	Lyst		serializedTlvs = NULL;
	unsigned char	*cursor;
	uvast		uvtemp;
	Lyst		stvs;
	Stv		*stv;
	LystElt		elt3;
	LystElt		elt4;
	unsigned char	*cursor2;
			OBJ_POINTER(BpsecOutboundTlv, tv);

	BPSEC_DEBUG_PROC("+ bpsec_serializeASB (%x, %x)",
			(unsigned long) length, (unsigned long) asb);

	CHKNULL(length);
	CHKNULL(asb);

	itemsCount = 4;	/*	Min. # items in serialized ASB array.	*/

	/*	Serialize the targets and the results in a single
		pass through the ASB's list of targets.			*/

	serializedResults = lyst_create_using(getIonMemoryMgr());
	if (serializedResults == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB can't create lyst.",
				NULL);
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		return NULL;
	}

	targetsCount = sdr_list_length(sdr, asb->targets);
	serializedTargetsLen = 9 * targetsCount;
	serializedTargets = MTAKE(serializedTargetsLen);
	if (serializedTargets == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB Need %d bytes.",
				serializedTargetsLen);
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		releaseAsbBuffers(serializedTargets, serializedSource,
			serializedParms, serializedParmsBuffer,
			serializedTlvs, serializedResults,
			serializedResultsBuffer);
		return NULL;
	}

	/*	Targets is just an array of integers.  Open it here.	*/

	cursor = serializedTargets;
	oK(cbor_encode_array_open(targetsCount, &cursor));

	/*	Results is an array of result structures, each of
		which is a block number paired with an array of
		security results -- each of which is an array
		comprising result type ID and result text.

		We can't open the top-level results array yet
		because we don't know how many bytes of buffer
		we'll need to encode the whole array into.		*/

	serializedResultsLen = 9;	/*	Top-level array open.	*/
	for (elt = sdr_list_first(sdr, asb->targets); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BpsecOutboundTarget, target,
				sdr_list_data(sdr, elt));

		/*	We can serialize the target block number
			right away.					*/

		uvtemp = target->targetBlockNumber;
		oK(cbor_encode_integer(uvtemp, &cursor));

		/*	But to serialize the results for this target
			block we have to step through them all,
			serializing and accumulating total length
			as we go.

			They will ultimately be serialized inside
			a per-target array of results.			*/

		serializedResultsLen += 9;	/*	Array open.	*/

		/*	We create a temporary list to hold the
			serialized TVs (results) for this target.	*/

		serializedTlvs = lyst_create_using(getIonMemoryMgr());
		if (serializedTlvs == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_serializeASB no Tvs lyst.",
					NULL);
			BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
			releaseAsbBuffers(serializedTargets, serializedSource,
				serializedParms, serializedParmsBuffer,
				serializedTlvs, serializedResults,
				serializedResultsBuffer);
			return NULL;
		}

		for (elt2 = sdr_list_first(sdr, target->results); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			GET_OBJ_POINTER(sdr, BpsecOutboundTlv, tv,
					sdr_list_data(sdr, elt2));
			stv = (Stv *) MTAKE(sizeof(Stv));
			if (stv == NULL
			|| lyst_insert_last(serializedTlvs, stv) == NULL
			|| (stv->text = MTAKE(19 + tv->length)) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_serializeASB no Stv",
						NULL);
				BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
				releaseAsbBuffers(serializedTargets,
					serializedSource, serializedParms,
					serializedParmsBuffer,
					serializedTlvs, serializedResults,
					serializedResultsBuffer);
				return NULL;
			}

			/*	Serialize this TV.			*/

			cursor2 = stv->text;

			/*	TV array open.				*/

			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor2));

			/*	Result type ID code.			*/

			uvtemp = tv->id;
			oK(cbor_encode_integer(uvtemp, &cursor2));

			/*	Result value byte string length.	*/

			uvtemp = tv->length;
			oK(cbor_encode_byte_string(NULL, uvtemp, &cursor2));

			/*	Result value byte string content.	*/

			sdr_read(sdr, (char *) cursor2, tv->value, tv->length);

			/*	Add length of this serialized TV
				(result) to the total serialized
				results buffer length.			*/

			stv->length = (cursor2 - stv->text) + tv->length;;
			serializedResultsLen += stv->length;

			/*	Append the Lyst of serialized TVs for
				this target block number to the Lyst
				of results lists (one per target)	*/

			if (lyst_insert_last(serializedResults, serializedTlvs)
					== NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_serializeASB no Tgt",
						NULL);
				BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
				releaseAsbBuffers(serializedTargets,
						serializedSource,
						serializedParms,
						serializedParmsBuffer,
						serializedTlvs,
						serializedResults,
					       	serializedResultsBuffer);
				return NULL;
			}

			serializedTlvs = NULL;
		}
	}

	/*	The buffer of serialized targets is now ready to go.
		Reduce length from maximum length to actual length.	*/

	serializedTargetsLen = cursor - serializedTargets;

	/*	And we have now got upper bound on total length of
		the serialized results buffer.				*/

	serializedResultsBuffer = MTAKE(serializedResultsLen);
	if (serializedResultsBuffer == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB no Results buf", NULL);
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		releaseAsbBuffers(serializedTargets,
				serializedSource, serializedParms,
				serializedParmsBuffer, serializedTlvs,
				serializedResults, serializedResultsBuffer);
		return NULL;
	}

	/*	Open the results top-level array.			*/

	cursor = serializedResultsBuffer;
	oK(cbor_encode_array_open(targetsCount, &cursor));

	/*	Now, for each of the result sets (targets) in the
		serializedResults list, concatenate all serialized
		TVs into the serialized results buffer.			*/

	while ((elt3 = lyst_first(serializedResults)))
	{
		stvs = (Lyst) lyst_data(elt3);

		/*	Open the array of TVs for this results set.	*/

		uvtemp = lyst_length(stvs);
		oK(cbor_encode_array_open(uvtemp, &cursor));

		/*	Now append all pre-serialized results in this
			result set (for one target).			*/

		while ((elt4 = lyst_first(stvs)))
		{
			stv = (Stv *) lyst_data(elt4);
			memcpy(cursor, stv->text, stv->length);
			cursor += stv->length;
			MRELEASE(stv->text);
			MRELEASE(stv);
			lyst_delete(elt4);
		}

		lyst_destroy(stvs);
		lyst_delete(elt3);
	}

	lyst_destroy(serializedResults);
	serializedResults = NULL;
	serializedResultsLen = cursor - serializedResultsBuffer;

	/*	Whew.  Now serialize something easy: security
		context ID.						*/

	cursor = serializedContextId;
	uvtemp = asb->contextId;
	oK(cbor_encode_integer(uvtemp, &cursor));
	serializedContextIdLen = cursor - serializedContextId;

	/*	And security context flags.				*/

	cursor = serializedContextFlags;
	uvtemp = asb->contextFlags;
	oK(cbor_encode_integer(uvtemp, &cursor));
	serializedContextFlagsLen = cursor - serializedContextFlags;

	/*	Slightly more challenging: security source.		*/

	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		itemsCount += 1;
		serializedSourceLen = serializeEid(&asb->securitySource,
				serializedSource);
	}

	/*	Then security context parameters.			*/

	if (asb->contextFlags & BPSEC_ASB_PARM)
	{
		itemsCount += 1;

		/*	Serialize the parameters in a single pass
			through the ASB's list of parameters.		*/

		serializedParmsLen += 9;	/*	Array open.	*/

		/*	We create a temporary list to hold the
			serialized TVs (parms) for this block.		*/

		serializedTlvs = lyst_create_using(getIonMemoryMgr());
		if (serializedTlvs == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_serializeASB no Tvs lyst.",
					NULL);
			BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
			releaseAsbBuffers(serializedTargets,
				serializedSource, serializedParms,
				serializedParmsBuffer, serializedTlvs,
				serializedResults, serializedResultsBuffer);
			return NULL;
		}

		for (elt2 = sdr_list_first(sdr, asb->parmsData); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			GET_OBJ_POINTER(sdr, BpsecOutboundTlv, tv,
					sdr_list_data(sdr, elt2));
			stv = (Stv *) MTAKE(sizeof(Stv));
			if (stv == NULL
			|| lyst_insert_last(serializedTlvs, stv) == NULL
			|| (stv->text = MTAKE(19 + tv->length)) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_serializeASB no Stv",
						NULL);
				BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
				releaseAsbBuffers(serializedTargets,
					serializedSource, serializedParms,
					serializedParmsBuffer, serializedTlvs,
					serializedResults,
					serializedResultsBuffer);
				return NULL;
			}

			/*	Serialize this TV.			*/

			cursor2 = stv->text;

			/*	TV array open.				*/

			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor2));

			/*	Result type ID code.			*/

			uvtemp = tv->id;
			oK(cbor_encode_integer(uvtemp, &cursor2));

			/*	Result value byte string length.	*/

			uvtemp = tv->length;
			oK(cbor_encode_byte_string(NULL, uvtemp, &cursor2));

			/*	Result value byte string content.	*/

			sdr_read(sdr, (char *) cursor2, tv->value, tv->length);

			/*	Add length of this serialized TV
				(parm) to the total serialized
				parms buffer length.			*/

			stv->length = (cursor2 - stv->text) + tv->length;;
			serializedParmsLen += stv->length;
		}

		/*	We have now got upper bound on total length
			of the serialized parms buffer.		*/

		serializedParmsBuffer = MTAKE(serializedParmsLen);
		if (serializedParmsBuffer == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_serializeASB no Parms buf",
					NULL);
			BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
			releaseAsbBuffers(serializedTargets,
				serializedSource, serializedParms,
				serializedParmsBuffer, serializedTlvs,
				serializedResults, serializedResultsBuffer);
			return NULL;
		}

		/*	Open the parms array.				*/

		cursor = serializedParmsBuffer;
		oK(cbor_encode_array_open(sdr_list_length(sdr, asb->parmsData),
				&cursor));

		/*	Now append to the buffer all pre-serialized
			security context parameters.			*/

		while ((elt4 = lyst_first(serializedTlvs)))
		{
			stv = (Stv *) lyst_data(elt4);
			memcpy(cursor, stv->text, stv->length);
			cursor += stv->length;
			MRELEASE(stv->text);
			MRELEASE(stv);
			lyst_delete(elt4);
		}

		lyst_destroy(serializedTlvs);
		serializedTlvs = NULL;
		serializedParmsLen = cursor - serializedParmsBuffer;
	}

	/*	Finally, concatenate the whole shebang into one
		buffer.							*/

	serializedAsb = MTAKE(1		/*	Top-level array.	*/
			+ serializedTargetsLen
			+ serializedContextIdLen
			+ serializedContextFlagsLen
			+ serializedSourceLen
			+ serializedParmsLen
			+ serializedResultsLen);
	if (serializedAsb == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB no ASB buffer", NULL);
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		releaseAsbBuffers(serializedTargets,
			serializedSource, serializedParms,
			serializedParmsBuffer, serializedTlvs,
			serializedResults, serializedResultsBuffer);
		return NULL;
	}

	cursor = serializedAsb;
	oK(cbor_encode_array_open(itemsCount, &cursor));
	memcpy(cursor, serializedTargets, serializedTargetsLen);
	cursor += serializedTargetsLen;
	memcpy(cursor, serializedContextId, serializedContextIdLen);
	cursor += serializedContextIdLen;
	memcpy(cursor, serializedContextFlags, serializedContextFlagsLen);
	cursor += serializedContextFlagsLen;
	if (serializedSourceLen > 0)
	{
		memcpy(cursor, serializedSource, serializedSourceLen);
		cursor += serializedSourceLen;
	}

	if (serializedParmsLen > 0)
	{
		memcpy(cursor, serializedParmsBuffer, serializedParmsLen);
		cursor += serializedParmsLen;
	}

	memcpy(cursor, serializedResultsBuffer, serializedResultsLen);
	cursor += serializedResultsLen;

	/*	Done.  Wrap up.						*/

	*length = cursor - serializedAsb;
	releaseAsbBuffers(serializedTargets, serializedSource,
			serializedParms, serializedParmsBuffer,
			serializedTlvs, serializedResults,
			serializedResultsBuffer);
	BPSEC_DEBUG_INFO("i bpsec_serializeASB -> data: " ADDR_FIELDSPEC
			", length %d", (uaddr) serializedAsb, *length);
	BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);

	return serializedAsb;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_transferToZcoFileSource
 *
 * \par Purpose: This utility function takes a zco object, a file reference,
 *		 and a character string, and it appends the string to a file.
 *		 A file reference to the new data is appended to the zco
 *		 object. If given an empty zco object, it will create a new
 *		 zco and return its location.  If given an empty file
 *		 reference, it will create a new file.
 *
 * \par Date Written:  8/15/11
 *
 * \retval -1 on Fatal Error
 *          0 on failure
 *          >0 on Success
 *
 * \param[in]  sdr        ion sdr
 * \param]in]  resultZco  Object where the file references will go
 * \param[in]  acqFileRef A file references pointing to the file
 * \param[in]  fname      A string to be used as the base of the filename
 * \param[in]  bytes      The string data to write in the file
 * \param[in]  length     Length of the string data
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/20/11  R. Brown      Initial Implementation.
 *  01/31/16  E. Birrane    Update to SBSP
 *****************************************************************************/

int	bpsec_transferToZcoFileSource(Sdr sdr, Object *resultZco,
		Object *acqFileRef, char *fname, char *bytes, uvast length)
{
	static uint32_t	acqCount = 0;
	char		cwd[200];
	char		fileName[SDRSTRING_BUFSZ];
	int		fd;
	vast		fileLength;

	CHKERR(bytes);

	BPSEC_DEBUG_PROC("+bpsec_transferToZcoFileSource(sdr, 0x"
			         ADDR_FIELDSPEC ", 0x"
			         ADDR_FIELDSPEC ", 0x"
				 ADDR_FIELDSPEC ", 0x"
				 ADDR_FIELDSPEC ","
				 UVAST_FIELDSPEC ")",
			         (uaddr) resultZco, (uaddr) acqFileRef,
				 (uaddr) fname, (uaddr) bytes, length);


	CHKERR(sdr_begin_xn(sdr));

	/* Step 1: If we don't have a ZCO, we need to make one. */
	if (*resultZco == 0)     /*      First extent of acquisition.    */
	{
		*resultZco = zco_create(sdr, ZcoSdrSource, 0, 0, 0,
				ZcoOutbound);
		if (*resultZco == (Object) ERROR)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: \
Can't start file source ZCO.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*      This extent of this acquisition must be acquired into
	 *      a file.                                                 */

	if (*acqFileRef == 0)      /*      First file extent.      */
	{
		if (igetcwd(cwd, sizeof cwd) == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: \
Can't get CWD for acq file name.", NULL);
			sdr_cancel_xn(sdr);
			return 0;
		}

		acqCount++;
		isprintf(fileName, sizeof fileName, "%s%c%s.%u", cwd,
				ION_PATH_DELIMITER, fname, acqCount);
		fd = open(fileName, O_WRONLY | O_CREAT, 0666);
		if (fd < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: \
Can't create acq file %s.", fileName);
			sdr_cancel_xn(sdr);
			return 0;
		}

		fileLength = 0;
		*acqFileRef = zco_create_file_ref(sdr, fileName, "",ZcoInbound);
	}
	else				/*	Writing more to file.	*/
	{
		oK(zco_file_ref_path(sdr, *acqFileRef, fileName,
				sizeof fileName));
		fd = open(fileName, O_WRONLY, 0666);
		if (fd < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: \
Can't reopen acq file %s.", fileName);
			sdr_cancel_xn(sdr);
			return 0;
		}

		if ((fileLength = lseek(fd, 0, SEEK_END)) < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: \
Can't get acq file length %s.", fileName);
			sdr_cancel_xn(sdr);
			close(fd);
			return 0;
		}
	}

	/*	Write the data to the file.				*/

	if (write(fd, bytes, length) < 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't append \
to acq file %s.", fileName);
		sdr_cancel_xn(sdr);
		close(fd);
		return 0;
	}

	close(fd);
	if (zco_append_extent(sdr, *resultZco, ZcoFileSource, *acqFileRef,
					      fileLength, length) <= 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't append \
extent to ZCO.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*      Flag file reference for deletion as soon as the last
	 *      ZCO extent that references it is deleted.               */

	zco_destroy_file_ref(sdr, *acqFileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't \
acquire extent into file..", NULL);
		return -1;
	}

	return 1;
}
