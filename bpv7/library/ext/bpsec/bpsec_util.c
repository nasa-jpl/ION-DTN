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


/******************************************************************************
 *
 * \par Function Name: bpsec_build_sdr_parm
 *
 * \par Purpose: This utility function builds a parms field and writes it to
 *               the SDR. The parm field is a CBOR arry of type-value pairs,
 *		 each of which is itself a CBOR array of two items..
 *
 * \par Date Written:  2/27/2016
 *
 * \retval SdrObject -- The SDR Object, or 0 on error.
 *
 * \param[in|out] sdr    The SDR Storing the result.
 * \param[in]     parm   The structure holding parameters.
 * \param[out]    len    The length of the data written to the SDR.
 *
 * \par Notes:
 *      1. The SDR object must be freed when no longer needed.
 *      2. This function does not use an SDR transaction.
 *      3. Only non-zero fields in the parm structure will be included
 *      4. We do not support content range.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

SdrObject bpsec_build_sdr_parm(Sdr sdr, csi_cipherparms_t parms, uint32_t *len)
{
	SdrObject result = 0;
	csi_val_t val;

	/* Step 0 - Sanity Check. */
	CHKZERO(len);

	/* Step 1 - Serialize the parameters */
	val = csi_serialize_parms(parms);

	if(val.len == 0)
	{
		BPSEC_DEBUG_ERR("bpsec_build_sdr_parm: Cannot serialize \
parameters.", NULL);
		return 0;
	}

	*len = val.len;


	/* Step 2 - Allocate the SDR space. */
	if((result = sdr_malloc(sdr, *len)) == 0)
	{
		BPSEC_DEBUG_ERR("bpsec_build_sdr_parm: Can't allocate sdr \
result of length %d.", *len);
		*len = 0;
		MRELEASE(val.contents);
		return 0;
	}

	sdr_write(sdr, result, (char *) val.contents, val.len);

	MRELEASE(val.contents);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: bpsec_build_sdr_result
 *
 * \par Purpose: This utility function builds a result field and writes it to
 *               the SDR. The results field is a CBOR arry of type-value pairs,
 *		 each of which is itself a CBOR array of two items..
 *
 * \par Date Written:  2/27/2016
 *
 * \retval SdrObject -- The SDR Object, or 0 on error.
 *
 * \param[in|out] sdr    The SDR Storing the result.
 * \param[in]     id     The type of data being written.
 * \param[in]     value  The length/value of data to be included in the result.
 * \param[out]    len    The length of the data written to the SDR.
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

SdrObject bpsec_build_sdr_result(Sdr sdr, uint8_t id, csi_val_t value, uint32_t *len)
{
	csi_val_t tmp;
	SdrObject result = 0;

	*len = 0;
	tmp = csi_build_tlv(id, value.len, value.contents);

	if(tmp.len == 0)
	{
		BPSEC_DEBUG_ERR("bpsec_build_result: Cannot create TLV.", NULL);
		return 0;
	}

	if((result = sdr_malloc(sdr, tmp.len)) == 0)
	{
		BPSEC_DEBUG_ERR("bpsec_build_result: Can't allocate sdr \
result of length %d.", tmp.len);
		MRELEASE(tmp.contents);
		return 0;
	}

	*len = tmp.len;
	sdr_write(sdr, result, (char *) tmp.contents, tmp.len);
	MRELEASE(tmp.contents);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: bpsec_deserializeASB
 *
 * \par Purpose: This utility function accepts a serialized Abstract Security
 *               Block from a bundle during acquisition and places it in a
 *               bpsecInboundBlock structure stored in the Acquisition Extension
 *               Block's scratchpad area.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int -- 1 - An ASB was successfully deserialized into the scratchpad
 *                0 - The deserialized ASB did not pass its sanity check.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  A pointer to the acquisition block holding the
 *                      serialized abstract security block.
 * \param[in]      wk   Work area holding bundle information.
 *
 * \par Notes: 
 *      1. This function allocates memory using the MTAKE method.  This
 *         scratchpad must be freed by the caller iff the method does
 *         not return -1.  Any system error will release the memory.
 *
 *      2.  If we return a 1, the ASB is considered invalid and not usable.
 *          The block should be discarded. It is still returned, though, so that
 *          the caller may examine it.
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

static void	destroyInboundTarget(LystElt elt, void *userData)
{
	BpsecInboundTarget	*target;

	target = (BpsecInboundTarget *) lyst_data(elt);
<<-- more here
	MRELEASE(target);
}

static void	loseInboundAsb(BpsecInboundBlock *asb)
{
	if (asb->targets)
	{
		lyst_destroy(asb->targets);
	}

<<-- more here
	MRELEASE(asb);
}

int	bpsec_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int			memIdx = getIonMemoryMgr();
	int			result = 1;
	unsigned char		*cursor = NULL;
	unsigned int		unparsedBytes = 0;
	BpsecInboundBlock	asb;
	uvast			arrayLength;
	BpsecInboundTarget	*itarget;
	uvast			uvtemp;
	BpsecInboundTv		*tv;
	unsigned char		buffer[255];
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

	lyst_delete_set(asb->targets, destroyInboundTarget, NULL);
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
		|| lyst_insert_last(asb->targets, target))
		{
			BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for \
ASB targets list", NULL);
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
length %d", asb.contextId, asb.contextFlags, blk->dataLength);

	/*	Next, possibly, is security source.			*/

	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		switch (acquireEid(&(asb->securitySource), &cursor,
					&unparsedBytes))
		{
		case -1:
			BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for \
securityu source EID", NULL);
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
			tv = (BpsecInboundTv *) MTAKE(sizeof(BpsecInboundTv));
			if (tv == NULL
			|| lyst_insert_last(asb->parmsData, tv) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No \
space for ASB parms TV", NULL);
				loseInboundAsb(asb);
				return -1;
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

			tv = (BpsecInboundTv *) MTAKE(sizeof(BpsecInboundTv));
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

			if (cbor_decode_integer(&uvtem, &cursor, &unparsedBytes)
					< 1)
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


/******************************************************************************
 *
 * \par Function Name: bpsec_findAcqBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in
 * 		 an acquisition work area looking for a bpsec block of the
 * 		 indicated type whose target is the block of indicated type
 * 		 and (as relevant) target type.
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

LystElt	bpsec_findAcqBlock(AcqWorkArea *wk,
		                   uint8_t type,
				   uint8_t targetBlockType,
				   uint8_t metatargetBlockType)
{
	LystElt			elt;
	AcqExtBlock		*blk;
	BpsecInboundBlock	*asb;

	CHKZERO(wk);
	for (elt = lyst_first(wk->extBlocks); elt; elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		if (blk->type != type)
		{
			continue;
		}

		asb = (BpsecInboundBlock *) (blk->object);
		if (asb->targetBlockType == targetBlockType
		&& asb->metatargetBlockType == metatargetBlockType)
		{
			return elt;
		}
	}

	return 0;
}


/******************************************************************************
 *
 * \par Function Name: bpsec_findBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in a
 * 			bundle looking for a bpsec block of the indicated type
 *			whose target is the block of indicated type.
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

Object	bpsec_findBlock(Bundle *bundle,
				uint8_t type,
			        uint8_t targetBlockType,
				uint8_t metatargetBlockType)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);
		OBJ_POINTER(BpsecOutboundBlock, asb);

	CHKZERO(bundle);
	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (blk->type != type)
		{
			continue;
		}

		GET_OBJ_POINTER(sdr, BpsecOutboundBlock, asb, blk->object);
		if (asb->targetBlockType == targetBlockType
		&& asb->metatargetBlockType == metatargetBlockType)
		{
			return elt;
		}
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
 *          0 on failure
 *          >0 on Success
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
	int	result;

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

	result = 0;
	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		if (readEid(&(asb->securitySource), fromEid) < 0)
		{
			result = 1;
		}
	}
	else
	{
		if (readEid(&bundle->id.source, fromEid) < 0)
		{
			result = -1;
		}
	}

	return result;
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
		OBJ_POINTER(BpsecOutboundTv, tv);

	CHKVOID(items);
	CHKVOID(tv);
	*tvp = 0;			/*	Default.		*/
	for (elt = sdr_list_first(sdr, items); elt; elt = sdr_next(sdr, elt))
	{
		addr = sdr_list_data(elt);
		GET_OBJ_POINTER(sdr, BpsecOutboundTv, tv, addr);
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
 *          0 on failure
 *          >0 on success
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
	int	result;

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

	result = 0;
	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		if (readEid(&(asb->securitySource), fromEid) < 0)
		{
			result = 1;
		}
	}
	else
	{
		if (readEid(&bundle->id.source, fromEid) < 0)
		{
			result = -1;
		}
	}

	return result;
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
<<--	get the local node's admin EID (for scheme of bundle's destination
<<--	EID, parse it to MetaEid, and writeEid(&eid, &metaEid).
	return;
}


/******************************************************************************
 *
 * \par Function Name: bpsec_retrieveKey
 *
 * \par Purpose: Retrieves the key associated with a particular keyname.
 *
 * \par Date Written:  6/01/09
 *
 * \retval csi_val_t -- The key value. Length 0 indicates error.
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
 *  03/14/16  E. Birrane    Reworked to use csi_val_t [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t bpsec_retrieveKey(char *keyName)
{
	csi_val_t key;
	char stdBuffer[100];
	int  ReqBufLen = 0;

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

	memset(&key, 0, sizeof(csi_val_t));

	if(keyName == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Bad Parms", NULL);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.len);
		return key;
	}

	ReqBufLen = sizeof(stdBuffer);
	key.len = sec_get_key(keyName, &(ReqBufLen), stdBuffer);

	/**
	 *  Step 1 - Check the key length.
	 *           <  0 indicated system failure.
	 *           == 0 indicated key not found.
	 *           > 0  inicates success.
	 */
	if (key.len < 0)	/* Error. */
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't get length of \
key '%s'.", keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.len);
		return key;
	}
	else if(key.len > 0) /*	Key has been retrieved.		*/
	{
		if((key.contents = (unsigned char *) MTAKE(key.len)) == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't allocate \
key of size %d", key.len);
			BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
					key.len);
			return key;
		}

		memcpy(key.contents, stdBuffer, key.len);

#ifdef BSP_DEBUGING
		char *str = NULL;

		if((str = csi_val_print(key)) != NULL)
		{
			BPSEC_DEBUG_INFO("i bpsec_retrieveKey: Key  Len: %d  \
Val: %s...", key.len, str);
			MRELEASE(str);
		}
#endif

		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.len);

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
				key.len);
		return key;
	}

	/* Step 2b - If the buffer was just not big enough, make a
	 * larger buffer and try again.
	 */

	if ((key.contents = MTAKE(ReqBufLen)) == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't allocate key of \
size %d", ReqBufLen);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				ReqBufLen);
		return key;
	}

	/* Step 3 - Call get key again and it should work this time. */

	if (sec_get_key(keyName, &ReqBufLen, (char *) (key.contents)) <= 0)
	{
		MRELEASE(key.contents);
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey:  Can't get key '%s'",
				keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)",
				key.len);
		return key;
	}

	key.len = ReqBufLen;

#ifdef BSP_DEBUGING
		char *str = NULL;

			if((str = csi_val_print(key)) != NULL)
			{
				BPSEC_DEBUG_INFO("i bpsec_retrieveKey: Key  \
Len: %d  Val: %s...", key.len, str);
				MRELEASE(str);
			}
#endif

	BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);
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
 *  03/14/16  E. Birrane    Reworked to use csi_val_t [Secure DTN
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

int	bpsec_requiredBlockExists(AcqWorkArea *wk, uint8_t bpsecBlockType,
			uint8_t targetBlockType, char *secSrcEid)
{
	LystElt			elt;
	AcqExtBlock		*blk;
	BpsecInboundBlock	*asb;
	Bundle			*bundle;
	int			result;
	char			*fromEid;

	CHKZERO(wk);
	bundle = &(wk->bundle);
	for (elt = lyst_first(wk->extBlocks); elt; elt = lyst_next(elt))
	{
		blk = (AcqExtBlock *) lyst_data(elt);
		if (blk->type != bpsecBlockType)
		{
			continue;		/*	Not a BIB.	*/
		}

		asb = (BpsecInboundBlock *) (blk->object);
		if (asb->targetBlockType != targetBlockType)
		{
			continue;		/*	Wrong target.	*/
		}

		/*	Now see if source of BIB matches the source
		 *	EID for this BIB rule.				*/

		result = 0;
		fromEid = NULL;

		/*	No security source in block, so source of BIB
		 *	is the source of the bundle.			*/

		if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
		{
			if (readEid(&(asb->securitySource), fromEid) < 0)
			{
				result = 1;
			}
		}
		else
		{
			if (readEid(&bundle->id.source, fromEid) < 0)
			{
				result = -1;
			}
		}

		if (result == 1)
		{
			if (eidsMatch(secSrcEid, strlen(secSrcEid),
					fromEid, strlen(fromEid)) == 0)
			{
				result = 0;
			}
		}

		if (fromEid)
		{
			MRELEASE(fromEid);
		}

		if (result != 0)		/*	1 or -1.	*/
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
 *               serialized representation in a private working memory buffer..
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

static void	releaseAsbBuffers(char *serializedTargets,
			char *serializedSource, Lyst serializedParms,
			char *serializedParmsBuffer, Lyst serializedResults,
			char *serializedResultsBuffer))
{
	LystElt	elt;
	Lyst	tvs;
	LystElt	elt2;
	Stv	*stv;

	if (serializedTargets) MRELEASE(serializedTargets);
	if (serializedSource) MRELEASE(serializedSource);
	if (serializedParms)
	{
		while ((elt2 = lyst_first(serializedParms)))
		{
			stv = (Stv *) lyst_data(elt2);
			if (stv)
			{
				if (stv->text)
				{
					MRELEASE(stv->text);
				}

				MRELEASE(stv);
			}

			lyst_delete(elt2);
		}

		lyst_destroy(serializedParms);
	}

	if (serializedParmsBuffer) MRELEASE(serializedParmsBuffer);
	if (serializedResults)
	{
		while ((elt = lyst_first(serializedResults)))
		{
			tvs = (Lyst) lyst_data(elt);
			if (tvs)
			{
				while ((elt2 = lyst_first(tvs)))
				{
					stv = (Stv *) lyst_data(elt2);
					if (stv)
					{
						if (stv->text)
						{
							MRELEASE(stv->text);
						}

						MRELEASE(stv);
					}

					lyst_delete(elt2);
				}

				lyst_destroy(tvs);
			}

			lyst_delete(elt);
		}

		lyst_destroy(serializedResults);
	}

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
	unsigned char	serializedResultsBuffer;
	unsigned int	serializedContextIdLen = 0;
	unsigned char	serializedContextId[9];
	unsigned int	serializedContextFlagsLen = 0;
	unsigned char	serializedContextFlags[9];
	unsigned int	serializedSourceLen = 0;
	unsigned char	*serializedSource[300];
	unsigned int	serializedParmsLen = 0;
	Lyst		serializedParms = NULL;
	unsigned char	*serializedParmsBuffer;
	unsigned int	serializedAsbLen = 0;
	unsigned char	*serializedAsb = NULL;
	unsigned char	*cursor;
	uvast		uvtemp;
	Stv		*stv;
	Lyst		tvs;
	LystElt		elt3;
	LystElt		elt4;
	unsigned char	*cursor2;
			OBJ_POINTER(BpsecOutboundTv, tv);

	BPSEC_DEBUG_PROC("+ bpsec_serializeASB (%x, %x)",
			(unsigned long) length, (unsigned long) asb);

	CHKNULL(length);
	CHKNULL(asb);

	itemsCount = 4;	/*	Min. # items in serialized ASB array.	*/

	/*	Serialize the targets and the results in a single
		bpass through the ASB's list of targets.		*/

	serializedResults = lyst_create_using(getIonMemoryMgr());
	if (serializedResults == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB can't create lyst.");
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		return NULL;
	}

	targetsCount = sdr_list_len(sdr, asb->targets);
	serializedTargetsLength = 9 * targetsCount;
	serializedTargets = MTAKE(serializedTargetsLength);
	if (serializedTargets == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB Need %d bytes.",
				serializedTargetsLength);
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		releaseAsbBuffers(serializedTargets, serializedSource,
			serializedParms, serializedResults);
		return NULL;
	}

	/*	Targets is just an array of integers.  Open it here.	*/

	cursor = serializedTargets;
	oK(cbor_encode_array_open(targetsCount, &cursor);

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
		oK(cbor_encode_integer(uvtemp, &cursor);

		/*	But to serialize the results for this target
			block we have to step through them all,
			serializing and accumulating total length
			as we go.

			They will ultimately be serialized inside
			a per-target array of results.			*/

		serializedResultsLen += 9;	/*	Array open.	*/

		/*	We create a temporary list to hold the
			serialized TVs (results) for this target.	*/

		serializedTvs = lyst_create_using(getIonMemoryMgr());
		if (serializedTvs == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_serializeASB no Tvs lyst.");
			BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
			releaseAsbBuffers(serializedTargets, serializedSource,
					serializedParms, serializedResults);
			return NULL;
		}

		for (elt2 = sdr_list_first(sdr, target->results); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			GET_OBJ_POINTER(sdr, BpsecOutboundTv, tv,
					sdr_list_data(sdr, elt));
			stv = (Stv *) MTAKE(sizeof(Stv));
			if (stv == NULL
			|| lyst_insert_last(serializedTvs, stv) == NULL
			|| (stv->text = MTAKE(19 + tv->length)) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_serializeASB no Stv");
				BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
				releaseAsbBuffers(serializedTargets,
						serializedSource,
						serializedParms,
						serializedResults);
				return NULL;
			}

			/*	Serialize this TV.			*/

			cursor2 = stv->text;

			/*	TV array open.				*/

			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor2);

			/*	Result type ID code.			*/

			uvtemp = tv->id;
			oK(cbor_encode_integer(uvtemp, &cursor2);

			/*	Result value byte string length.	*/

			uvtemp = tv->length;
			oK(cbor_encode_byte_string(NULL, uvtemp, &cursor2);

			/*	Result value byte string content.

			sdr_read(sdr, (char *) cursor2, tv->value, tv->length);

			/*	Add length of this serialized TV
				(result) to the total serialized
				results buffer length.			*/

			stv->length = (cursor2 - stv->text) + tv->length;;
			serializedResultsLen += stv->length;

			/*	Append the Lyst of serialized TVs for
				this target block number to the Lyst
				of results lists (one per target)	*/

			if (lyst_insert_last(serializedResults, serializedTvs)
					== NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_serializeASB no Tgt");
				BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
				releaseAsbBuffers(serializedTargets,
						serializedSource,
						serializedParms,
						serializedResults);
				return NULL;
			}
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
		BPSEC_DEBUG_ERR("x bpsec_serializeASB no Results buf");
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		releaseAsbBuffers(serializedTargets,
				serializedSource,
				serializedParms,
				serializedResults);
		return NULL;
	}

	/*	Open the results top-level array.			*/

	cursor = serializedResultsBuffer;
	oK(cbor_encode_array_open(targetsCount, &cursor);

	/*	Now, for each of the result sets (targets) in the
		serializedResults list, concatenate all serialized
		TVs into the serialized results buffer.			*/

	while ((elt3 = lyst_first(serializedResults)))
	{
		stvs = (Lyst) lyst_data(elt3);

		/*	Open the array of TVs for this results set.	*/

		uvtemp = lyst_length(stvs);
		oK(cbor_encode_array_open(uvtemp, &cursor);

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
	serializedResultsLen = cursor - serializedResultsBuffer;

	/*	Whew.  Now serialize something easy: security
		context ID.						*/

	cursor = serializedContactId;
	uvtemp = asb->contextId;
	oK(cbor_encode_integer(uvtemp, &cursor);
	serializedContactIdLength = cursor - serializedContactId;

	/*	And security context flags.				*/

	cursor = serializedContactFlags;
	uvtemp = asb->contextFlags;
	oK(cbor_encode_integer(uvtemp, &cursor);
	serializedContactFlagsLength = cursor - serializedContactFlags;

	/*	Slightly more challenging: security source.		*/

	if (asb->contactFlags & BPSEC_ASB_SEC_SRC)
	{
		itemsCount += 1;
		serializedSourceLen = serializeEid(&asb->securitySource,
				serializedSource);
	}

	/*	Then security context parameters.			*/

	if (asb->contactFlags & BPSEC_ASB_PARM)
	{
		itemsCount += 1;

		/*	Serialize the parameters in a single pass
			through the ASB's list of parameters.		*/

		serializedParmsLen += 9;	/*	Array open.	*/

		/*	We create a temporary list to hold the
			serialized TVs (parms) for this block.		*/

		serializedTvs = lyst_create_using(getIonMemoryMgr());
		if (serializedTvs == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_serializeASB no Tvs lyst.");
			BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
			releaseAsbBuffers(serializedTargets, serializedSource,
					serializedParms, serializedResults);
			return NULL;
		}

		for (elt2 = sdr_list_first(sdr, asb->parmsData); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			GET_OBJ_POINTER(sdr, BpsecOutboundTv, tv,
					sdr_list_data(sdr, elt));
			stv = (Stv *) MTAKE(sizeof(Stv));
			if (stv == NULL
			|| lyst_insert_last(serializedTvs, stv) == NULL
			|| (stv->text = MTAKE(19 + tv->length)) == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_serializeASB no Stv");
				BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
				releaseAsbBuffers(serializedTargets,
						serializedSource,
						serializedParms,
						serializedResults);
				return NULL;
			}

			/*	Serialize this TV.			*/

			cursor2 = stv->text;

			/*	TV array open.				*/

			uvtemp = 2;
			oK(cbor_encode_array_open(uvtemp, &cursor2);

			/*	Result type ID code.			*/

			uvtemp = tv->id;
			oK(cbor_encode_integer(uvtemp, &cursor2);

			/*	Result value byte string length.	*/

			uvtemp = tv->length;
			oK(cbor_encode_byte_string(NULL, uvtemp, &cursor2);

			/*	Result value byte string content.

			sdr_read(sdr, (char *) cursor2, tv->value, tv->length);

			/*	Add length of this serialized TV
				(parm) to the total serialized
				parms buffer length.			*/

			stv->length = (cursor2 - stv->text) + tv->length;;
			serializedParmsLen += stv->length;
		}

		/*	We have now got upper bound on total length
			of the serialized results buffer.		*/

		serializedParmsBuffer = MTAKE(serializedParmsLen);
		if (serializedParmsBuffer == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_serializeASB no Parms buf");
			BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
			releaseAsbBuffers(serializedTargets,
					serializedSource,
					serializedParms,
					serializedResults);
			return NULL;
		}

		/*	Open the parms array.				*/

		cursor = serializedParmsBuffer;
		oK(cbor_encode_array_open(sdr_list_length(sdr, asb->parmsData,
				&cursor);

		/*	Now append to the buffer all pre-serialized
			security context parameters.			*/

		while ((elt4 = lyst_first(serializedTvs)))
		{
			stv = (Stv *) lyst_data(elt4);
			memcpy(cursor, stv->text, stv->length);
			cursor += stv->length;
			MRELEASE(stv->text);
			MRELEASE(stv);
			lyst_delete(elt4);
		}

		lyst_destroy(serializedTvs);
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
		BPSEC_DEBUG_ERR("x bpsec_serializeASB no ASB buffer");
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		releaseAsbBuffers(serializedTargets,
				serializedSource,
				serializedParms,
				serializedResults);
		return NULL;
	}

	cursor = serializedAsb;
	oK(cbor_encode_array_open(itemsCount, &cursor);
	memcpy(cursor, serializedTargets, serializedTargetsLen);
	cursor += serializedTargetsLen;
	memcpy(cursor, serializedContxtId, serializedContxtIdLen);
	cursor += serializedContextIdLen;
	memcpy(cursor, serializedContextFlags, serializedContextFlagsLen);
	cursor += serializedContextFlagsLen;
	if (serializedSourceLength > 0)
	{
		memcpy(cursor, serializedSource, serializedSourceLen);
		cursor += serializedSourceLen;
	}

	if (serializedParmsLength > 0)
	{
		memcpy(cursor, serializedParmsBuffer, serializedParmsLen);
		cursor += serializedParmsLen;
	}

	memcpy(cursor, serializedResultsBuffer, serializedResultsLen);
	cursor += serializedResultsLen;

	/*	Done.  Wrap up.						*/

	*length = cursor - serializedAsb;
	releaseAsbBuffers(serializedTargets, serializedSource,
			serializedParms, serializedResults);

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

	// Write the data to the file
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
