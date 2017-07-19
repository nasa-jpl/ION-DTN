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
 **          Extensions: BPSEC
 **
 ** Description: This file provides a partial implementation of Bundle Protocol
 ** 			 Security (BPSEC).
 **              This implementation utilizes the ION Extension Interface to
 **              manage the creation, modification, evaluation, and removal
 **              of BPSEC blocks from Bundle Protocol (RFC 5050) bundles.
 **
 ** Notes:
 **         As of January 2016, the ION BPSEC implementation supports
 **         the Block Integrity Block (BIB) and Block Confidentiality Block
 **         (BCB) with the following constraints:
 **         - Only the ciphersuites implemented in the profiles.c file
 **           are supported.
 **         - There is currently no support for BIBs and BCBs comprising
 **           First and Last Blocks.
 **         - There is no support for the use of multiple ciphersuites to
 **           offer or acquire BPSEC blocks of a given type in the bundle
 **           traffic between a given security source and a given security
 **           destination.  That is, the ciphersuite to be used for offering
 **           or acquiring a BPSEC block is a function of the block type,
 **           the security source node, and the security destination node.
 **           When an BPSEC block whose security destination is the local
 **           node is received, if that block was offered in the context
 **           of a ciphersuite other than the one that ION would select
 **           for that block type's type and security source and destination
 **           then the block is silently discarded.
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
 **  01/23/16  E. Birrane           Update to BPSEC
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
 * \par Function Name: bpsec_addSdnvToStream
 *
 * \par Purpose: This utility function adds the contents of an SDNV to a
 *               character stream and then returns the updated stream pointer.
 *
 * \par Date Written:  5/29/09
 *
 * \retval unsigned char * -- The updated stream pointer.
 *
 * \param[in]  stream  The current position of the stream pointer.
 * \param[in]  val     The SDNV value to add to the stream.
 *
 * \par Notes: 
 *      1. Input parameters are passed as pointers to prevent wasted copies.
 *         Therefore, this function must be careful not to modify them.
 *      2. This function assumes that the stream is a character stream.
 *      3. We assume that we are not under such tight profiling constraints
 *         that sanity checks are too expensive.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/29/09  E. Birrane           Initial Implementation.
 *  06/08/09  E. Birrane           Documentation updates.
 *  06/20/09  E. Birrane           Added Debugging Stmt, cmts for initial rel.
 *****************************************************************************/
unsigned char	*bpsec_addSdnvToStream(unsigned char *stream, Sdnv* value)
{
	BPSEC_DEBUG_PROC("+ bpsec_addSdnvToStream(%x, %x)",
			(unsigned long) stream, (unsigned long) value);

	if ((stream != NULL) && (value != NULL) && (value->length > 0))
	{
		BPSEC_DEBUG_INFO("i bpsec_addSdnvToStream: Adding %d bytes",
				value->length);
		memcpy(stream, value->text, value->length);
		stream += value->length;
	}

	BPSEC_DEBUG_PROC("- bpsec_addSdnvToStream --> %x", (unsigned long) stream);

	return stream;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_build_sdr_parm
 *
 * \par Purpose: This utility function builds a parms field and writes it to
 *               the SDR. The parm field is a set of type-length-value fields
 *               where the type is a byte, length is an SDNV-encoded integer,
 *               and value is the value itself.
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
		BPSEC_DEBUG_ERR("bpsec_build_sdr_parm: Cannot serialize parameters.", NULL);
		return 0;
	}

	*len = val.len;


	/* Step 2 - Allocate the SDR space. */
	if((result = sdr_malloc(sdr, *len)) == 0)
	{
		BPSEC_DEBUG_ERR("bpsec_build_sdr_parm: Can't allocate sdr result of length %d.",
				*len);
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
 *               the SDR. Result fields are type-length-value fields where the
 *               type is a byte, length is an SDNV-encoded integer, and value
 *               is the value itself.
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
		BPSEC_DEBUG_ERR("bpsec_build_result: Can't allocate sdr result of length %d.",
				tmp.len);
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

int	bpsec_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int		result = 1;
	BpsecInboundBlock	asb;
	unsigned char	*cursor = NULL;
	int	unparsedBytes = 0;
	LystElt		elt;
	uvast	 ltemp;
	unsigned int itemp;

	BPSEC_DEBUG_PROC("+ bpsec_deserializeASB("ADDR_FIELDSPEC","ADDR_FIELDSPEC"%d)", (uaddr) blk, (uaddr) wk);

	CHKERR(blk);
	CHKERR(wk);

	BPSEC_DEBUG_INFO("i bpsec_deserializeASB blk length %d", blk->length);
	unparsedBytes = blk->length;

	memset((char *) &asb, 0, sizeof(BpsecInboundBlock));

	if (blk->eidReferences)
	{
		asb.securitySource.unicast = 1;
		elt = lyst_first(blk->eidReferences);
		ltemp = (unsigned long) lyst_data(elt);
		asb.securitySource.d.schemeNameOffset = ltemp;
		elt = lyst_next(elt);
		ltemp = (unsigned long) lyst_data(elt);
		asb.securitySource.d.nssOffset = ltemp;
	}

	/*
	 * Position cursor to start of block-type-specific data of the
	 * extension block, by skipping over the extension block header.
	 */

	cursor = ((unsigned char *)(blk->bytes))
			+ (blk->length - blk->dataLength);

	/* Extract block specifics, using ciphersuiteFlags as necessary. */

	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.targetBlockType = itemp;

	/*	The "subscript" portion of the compound security-
	 *	target field of the inbound BPSEC block is the
	 *	occurrence number of the target block.			*/

	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.targetBlockOccurrence = itemp;

	/*	For now, ION does not recognize any multi-block
	 *	security operations.
	 */
	asb.instance = 0;

	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.ciphersuiteType = itemp;
	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.ciphersuiteFlags = itemp;

	BPSEC_DEBUG_INFO("i bpsec_deserializeASB: cipher %ld, flags %ld, length %d",
		asb.ciphersuiteType, asb.ciphersuiteFlags, blk->dataLength);

	if (asb.ciphersuiteFlags & BPSEC_ASB_PARM)
	{
		extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
		asb.parmsLen = itemp;
		if (asb.parmsLen > 0)
		{
			if (asb.parmsLen > unparsedBytes)
			{
				BPSEC_DEBUG_WARN("? bpsec_deserializeASB: \
parmsLen %u, unparsedBytes %u.", asb.parmsLen, unparsedBytes);

				result = 0;

				BPSEC_DEBUG_PROC("- bpsec_deserializeASB -> %d",
						         result);
				return result;
			}

			asb.parmsData = MTAKE(asb.parmsLen);
			if (asb.parmsData == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for ASB parms %d",
						        utoa(asb.parmsLen));
				return -1;
			}

			memcpy(asb.parmsData, cursor, asb.parmsLen);
			cursor += asb.parmsLen;
			unparsedBytes -= asb.parmsLen;
			BPSEC_DEBUG_INFO("i bpsec_deserializeASB: parmsLen %ld",
					asb.parmsLen);
		}
	}

	if (asb.ciphersuiteFlags & BPSEC_ASB_RES)
	{
		extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
		asb.resultsLen = itemp;
		if (asb.resultsLen > 0)
		{
			if (asb.resultsLen > unparsedBytes)
			{
				BPSEC_DEBUG_WARN("? bpsec_deserializeASB: \
resultsLen %u, unparsedBytes %u.", asb.resultsLen, unparsedBytes);
				if (asb.parmsData)
				{
					MRELEASE(asb.parmsData);
				}

				result = 0;
				BPSEC_DEBUG_PROC("- bpsec_deserializeASB -> %d",
						result);
				return result;
			}

			asb.resultsData = MTAKE(asb.resultsLen);
			if (asb.resultsData == NULL)
			{
				BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for ASB results %d",
						        utoa(asb.resultsLen));
				return -1;
			}

			memcpy(asb.resultsData, cursor, asb.resultsLen);
			cursor += asb.resultsLen;
			unparsedBytes -= asb.resultsLen;
			BPSEC_DEBUG_INFO("i bpsec_deserializeASB: resultsLen %ld",
					asb.resultsLen);
		}
	}

	blk->size = sizeof(BpsecInboundBlock);
	blk->object = MTAKE(sizeof(BpsecInboundBlock));
	if (blk->object == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_deserializeASB: No space for ASB scratchpad", NULL);
		return -1;
	}

	memcpy((char *) (blk->object), (char *) &asb, sizeof(BpsecInboundBlock));

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
	char		*dictionary;
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	int		result = 0;

	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)
	{
		BPSEC_DEBUG_ERR("x bpsec_destinationIsLocal: Can't retrieve dictionary.", NULL);
		return result;
	}

	lookUpEidScheme(bundle->destination, dictionary, &vscheme);
	if (vscheme)	/*	EID scheme is known on this node.	*/
	{
		lookUpEndpoint(bundle->destination, dictionary, vscheme,
				&vpoint);
		if (vpoint)	/*	Node is registered in endpoint.	*/
		{
			result = 1;
		}
	}

	releaseDictionary(dictionary);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_findAcqBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in
 * 		 an acquisition work area looking for a BPSEC block of the
 * 		 indicated type and ordinality whose target is the block
 * 		 of indicated type and ordinality.
 *
 * \retval Object
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of BPSEC block to look for.
 * \param[in]  targetBlockType		Identifies target of the BPSEC block.
 * \param[in]  targetBlockOccurrence		"
 * \param[in]  instance			The BPSEC block instance to look for.
 *
 * \par Notes:
 *****************************************************************************/

LystElt	bpsec_findAcqBlock(AcqWorkArea *wk,
		                   uint8_t type,
						   uint8_t targetBlockType,
						   uint8_t targetBlockOccurrence,
						   uint8_t instance)
{
	uint32_t	 idx;
	LystElt		elt;
	AcqExtBlock	*blk;
	BpsecInboundBlock	*asb;

	CHKZERO(wk);
	for (idx = 0; idx < 2; idx++)
	{
		for (elt = lyst_first(wk->extBlocks[idx]); elt;
				elt = lyst_next(elt))
		{
			blk = (AcqExtBlock *) lyst_data(elt);
			if (blk->type != type)
			{
				continue;
			}

			asb = (BpsecInboundBlock *) (blk->object);
			if (asb->targetBlockType == targetBlockType
			&& asb->targetBlockOccurrence == targetBlockOccurrence
			&& asb->instance == instance)
			{
				return elt;
			}
		}
	}

	return 0;
}




/******************************************************************************
 *
 * \par Function Name: bpsec_findBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in
 * 		         a bundle looking for a BPSEC block of the indicated type
 * 		         and ordinality whose target is the block of indicated
 * 		         type and ordinality.
 *
 * \retval Object
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of BPSEC block to look for.
 * \param[in]  targetBlockType		Identifies target of the BPSEC block.
 * \param[in]  targetBlockOccurrence		"
 * \param[in]  instance			The BPSEC block instance to look for.
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
						uint8_t targetBlockOccurrence,
						uint8_t instance)
{
	Sdr	sdr = getIonsdr();
	int	idx;
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);
		OBJ_POINTER(BpsecOutboundBlock, asb);

	CHKZERO(bundle);
	for (idx = 0; idx < 2; idx++)
	{
		for (elt = sdr_list_first(sdr, bundle->extensions[idx]); elt;
				elt = sdr_list_next(sdr, elt))
		{
			addr = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
			if (blk->type != type)
			{
				continue;
			}

			GET_OBJ_POINTER(sdr, BpsecOutboundBlock, asb,
					blk->object);
			if (asb->targetBlockType == targetBlockType
			&& asb->targetBlockOccurrence == targetBlockOccurrence
			&& asb->instance == instance)
			{
				return elt;
			}
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
 * \param[in]   asb		 The inbound Abstract Security Block.
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
 *  07/14/16  E. Birrane    Update Return Codes.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int bpsec_getInboundSecurityEids(Bundle *bundle,
		                            AcqExtBlock *blk,
									BpsecInboundBlock *asb,
									char **fromEid,
									char **toEid)
{
	char	*dictionary;
	int	result;

	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(asb);
	CHKERR(fromEid);
	CHKERR(toEid);
	*fromEid = NULL;	/*	Default.			*/
	*toEid = NULL;		/*	Default.			*/
	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)
	{
		return 0;
	}

	if (printEid(&(bundle->destination), dictionary, toEid) < 0)
	{
		if (dictionary)
		{
			releaseDictionary(dictionary);
		}

		return -1;
	}

	if (asb->ciphersuiteFlags & BPSEC_ASB_SEC_SRC)
	{
		result = bpsec_getInboundSecuritySource(blk, dictionary, fromEid);
	}
	else
	{
		if(printEid(&bundle->id.source, dictionary, fromEid) < 0)
		{
			result = -1;
		}
		else
		{
			result = 1;
		}
	}

	if (dictionary)
	{
		releaseDictionary(dictionary);
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_getInboundSecuritySource
 *
 * \par Purpose: This function finds the security source associated with a
 *               given in-bound extension block.
 *
 * \retval  -1 on System Error
 *           0 on failure.
 *           >0 Success
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of BPSEC block to look for.
 * \param[in]  targetBlockType		Identifies target of the BPSEC block.
 * \param[in]  targetBlockOccurrence		"
 * \param[in]  instance			The BPSEC block instance to look for.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *  07/14/16  E. Birrane    Update return values [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bpsec_getInboundSecuritySource(AcqExtBlock *blk,
		                                       char *dictionary,
			                                   char **fromEid)
{
	EndpointId	securitySource;
	LystElt		elt1;
	LystElt		elt2;

	if (dictionary == NULL
	|| (elt1 = lyst_first(blk->eidReferences)) == NULL
	|| (elt2 = lyst_next(elt1)) == NULL)
	{
		return 0;
	}

	securitySource.cbhe = 0;
	securitySource.unicast = 1;
	securitySource.d.schemeNameOffset = (long) lyst_data(elt1);
	securitySource.d.nssOffset = (long) lyst_data(elt2);
	if (printEid(&securitySource, dictionary, fromEid) < 0)
	{
		return -1;
	}

	return 1;
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
 * \par Purpose: This function searches within a buffer (a ciphersuite
 *               parameters field or a security result field) of an outbound
 *               bpsec block for an information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in
 *                         bpsec_util.h as BPSEC_CSPARM_xxx macros.
 * \param[in]  bpsecBuf      The data buffer in which to search for the item.
 * \param[in]  bpsecLen      The length of the data buffer.
 * \param[in]  val         A pointer to a variable in which the function
 *                         should place the location (within the buffer)
 *                         of the first item of specified type that is
 *                         found within the buffer.  On return, this
 *                         variable contains NULL if no such item was found.
 * \param[in]  len         A pointer to a variable in which the function
 *                         should place the length of the first item of
 *                         specified type that is found within the buffer.
 *                         On return, this variable contains 0 if no such
 *                         item was found.
 *
 * \par Notes:
 *****************************************************************************/

void	bpsec_getOutboundItem(uint8_t itemNeeded, Object buf,
		                      uint32_t bufLen, Address *val, uint32_t *len)
{
	unsigned char *temp;
	unsigned char *cursor;
	uint8_t       itemType;
	unsigned int      sdnvLength;
	uvast		  longNumber;
	uint32_t	  itemLength;
	uint32_t	  offset;

	CHKVOID(buf);
	CHKVOID(val);
	CHKVOID(len);
	*val = 0;			/*	Default.		*/
	*len = 0;			/*	Default.		*/

	/*	Walk through all items in the buffer (either a
	 *	security parameters field or a security results
	 *	field), searching for an item of the indicated type.	*/

	temp = MTAKE(bufLen);
	if (temp == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_getOutboundItem: No space for temporary memory buffer %d.",
					    utoa(bufLen));
		return;
	}

	memset(temp, 0, bufLen);
	sdr_read(getIonsdr(), (char *) temp, buf, bufLen);
	cursor = temp;
	while (bufLen > 0)
	{
		itemType = *cursor;
		cursor++;
		bufLen--;
		if (bufLen == 0)	/*	No item length.		*/
		{
			break;		/*	Malformed result data.	*/
		}

		sdnvLength = decodeSdnv(&longNumber, cursor);
		if (sdnvLength == 0 || sdnvLength > bufLen)
		{
			break;		/*	Malformed result data.	*/
		}

		itemLength = longNumber;
		cursor += sdnvLength;
		bufLen -= sdnvLength;

		if (itemLength == 0)	/*	Empty item.		*/
		{
			continue;
		}

		if (itemType == itemNeeded)
		{
			offset = cursor - temp;
			*val = (Address) (buf + offset);
			*len = itemLength;
			break;
		}

		/*	Look at next item in buffer.			*/

		cursor += itemLength;
		bufLen -= itemLength;
	}

	MRELEASE(temp);
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
 * \param[in]   asb		 The outbound Abstract Security Block.
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

int	bpsec_getOutboundSecurityEids(Bundle *bundle,
		                              ExtensionBlock *blk,
									  BpsecOutboundBlock *asb,
									  char **fromEid,
									  char **toEid)
{
	char	*dictionary;
	int	result;

	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(asb);
	CHKERR(fromEid);
	CHKERR(toEid);
	*fromEid = NULL;	/*	Default.			*/
	*toEid = NULL;		/*	Default.			*/

	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)
	{
		return 0;
	}

	if (printEid(&(bundle->destination), dictionary, toEid) < 0)
	{
		if (dictionary)
		{
			releaseDictionary(dictionary);
		}

		return -1;
	}

	if (asb->ciphersuiteFlags & BPSEC_ASB_SEC_SRC)
	{
		result = bpsec_getOutboundSecuritySource(blk, dictionary, fromEid);
	}
	else
	{
		if(printEid(&bundle->id.source, dictionary, fromEid) < 0)
		{
			result = -1;
		}
		else
		{
			result = 1;
		}
	}

	if (dictionary)
	{
		releaseDictionary(dictionary);
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_getOutboundSecuritySource
 *
 * \par Purpose: This function finds the security source associated with a
 *               given outbound extension block.
 *
 * \retval  -1 on Fatal Error.
 *           0 on Failure.
 *           >0 on Success.
 *
 * \param[in]   blk      	The outbound block.
 * \param[in]   dictionary  The bundle dictionary (or null).
 * \param[out]  fromEid		The calculated security source.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  03/14/16  E. Birrane    Documentation.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *  07/14/16  E. Birrane    Update return codes.[Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int bpsec_getOutboundSecuritySource(ExtensionBlock *blk,
		                                      char *dictionary,
				  				 	          char **fromEid)
{
	Sdr		sdr = getIonsdr();
	EndpointId	securitySource;
	Object		elt1;
	Object		elt2;

	if (dictionary == NULL
	|| (elt1 = sdr_list_first(sdr, blk->eidReferences)) == 0
	|| (elt2 = sdr_list_next(sdr, elt1)) == 0)
	{
		return 0;
	}

	securitySource.cbhe = 0;
	securitySource.unicast = 1;
	securitySource.d.schemeNameOffset = sdr_list_data(sdr, elt1);
	securitySource.d.nssOffset = sdr_list_data(sdr, elt2);
	if (printEid(&securitySource, dictionary, fromEid) < 0)
	{
		return -1;
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_insertSecuritySource
 *
 * \par Purpose: This function inserts the local node as the security
 *               source of an ASB.
 *
 * \retval  None.
 *
 * \param[in]     bundle  The bundle whose block will have a new security source.
 * \param[in|out] asb     The ASB receiving the security source
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

void	bpsec_insertSecuritySource(Bundle *bundle, BpsecOutboundBlock *asb)
{
	char		*dictionary;
	VEndpoint	*vpoint;
	PsmAddress	elt;

	CHKVOID(bundle);
	CHKVOID(asb);
	if (bundle->destination.cbhe)
	{
		/*	Can't add EID references to extension blocks
		 *	when using CBHE -- no dictionary.		*/

		return;
	}

	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)	/*	No space.	*/
	{
		return;
	}

	findEndpoint(dictionary + bundle->id.source.d.schemeNameOffset,
			dictionary + bundle->id.source.d.nssOffset, NULL,
			&vpoint, &elt);
	if (elt)
	{
		/*	The bundle source is the local node, so
		 *	no need to insert an EID reference for the
		 *	security source.				*/

		releaseDictionary(dictionary);
		return;
	}

	/*	Note that local node is the security source.
	 *
	 *	Right, except that this is extremely complicated
	 *	and error-prone.  See insertNonCbheCustodian() in
	 *	libbpP.c.  If possible, never implement this at all.	*/

	writeMemo("[!] Insertion of local node as security source in outbound \
BPSEC block is not yet implemented.");
	releaseDictionary(dictionary);
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
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);
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
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't get length of key '%s'.",
				keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);
		return key;
	}
	else if(key.len > 0) /*	Key has been retrieved.		*/
	{
		if((key.contents = (unsigned char *) MTAKE(key.len)) == NULL)
		{
			BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't allocate key of size %d", key.len);
			BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);
			return key;
		}

		memcpy(key.contents, stdBuffer, key.len);

#ifdef BSP_DEBUGING
		char *str = NULL;

		if((str = csi_val_print(key)) != NULL)
		{
			BPSEC_DEBUG_INFO("i bpsec_retrieveKey: Key  Len: %d  Val: %s...", key.len, str);
			MRELEASE(str);
		}
#endif

		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);

		return key;
	}

	/**
	 *  Step 2 - At this point, if we did not find a key and did not have a system error,
	 *          either the key was not found, or it was found and larger than the
	 *          standard buffer.
	 *
	 *          If we ran out of space, the neededBufLen will be less than the
	 *          provided buffer. Otherwise, the neededBufLen will be the required size
	 *          of the buffer to hold the key.
	 */

	/* Step 2a - If we did not find a key... */
	if(ReqBufLen <= sizeof(stdBuffer))
	{
		BPSEC_DEBUG_WARN("? bpsec_retrieveKey: Unable to find key '%s'", keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey", NULL);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);
		return key;
	}

	/* Step 2b - If the buffer was just not big enough, make a larger buffer and
	 *           try again.
	 */


	if ((key.contents = MTAKE(ReqBufLen)) == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey: Can't allocate key of size %d", ReqBufLen);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", ReqBufLen);
		return key;
	}

	/* Step 3 - Call get key again and it should work this time. */

	if (sec_get_key(keyName, &ReqBufLen, (char *) (key.contents)) <= 0)
	{
		MRELEASE(key.contents);
		BPSEC_DEBUG_ERR("x bpsec_retrieveKey:  Can't get key '%s'", keyName);
		BPSEC_DEBUG_PROC("- bpsec_retrieveKey -> key (len=%d)", key.len);
		return key;
	}

	key.len = ReqBufLen;

#ifdef BSP_DEBUGING
		char *str = NULL;

			if((str = csi_val_print(key)) != NULL)
			{
				BPSEC_DEBUG_INFO("i bpsec_retrieveKey: Key  Len: %d  Val: %s...", key.len, str);
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
 *    - This function is not implementated at this time.
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
 * \par Function Name: bpsec_serializeASB
 *
 * \par Purpose: Serializes an outbound bundle security block and returns the
 *               serialized representation.
 *
 * \par Date Written:  6/03/09
 *
 * \retval unsigned char * - the serialized outbound bundle Security Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  asb    The BpsecOutboundBlock to serialize.
 *
 * \par Notes:
 *      1. This function uses MTAKE to allocate space for the serialized ASB.
 *         This serialized ASB (if not NULL) must be freed using MRELEASE.
 *      2. This function only serializes the block-type-specific data of
 *         a BPSEC extension block, not the extension block header.
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

unsigned char	*bpsec_serializeASB(uint32_t *length, BpsecOutboundBlock *asb)
{
	Sdr		sdr = getIonsdr();
	Sdnv		targetBlockType;
	Sdnv		targetBlockOccurrence;
	Sdnv		ciphersuiteType;
	Sdnv		ciphersuiteFlags;
	Sdnv		parmsLen;
	Sdnv		resultsLen;
	unsigned char	*serializedAsb;
	unsigned char	*cursor;

	BPSEC_DEBUG_PROC("+ bpsec_serializeASB (%x, %x)",
			(unsigned long) length, (unsigned long) asb);

	CHKNULL(length);
	CHKNULL(asb);

	/*********************************************************************
	 *         Calculate the length of the block-type-specific data      *
	 *********************************************************************/

	/*	We need to assign all of our SDNV values first so we
	 *	know how many bytes they will take up. We don't want a
	 *	separate function to calculate this length as it would
	 *	result in generating multiple SDNV values needlessly.	*/

	encodeSdnv(&targetBlockType, asb->targetBlockType);
	*length = targetBlockType.length;
	encodeSdnv(&targetBlockOccurrence, asb->targetBlockOccurrence);
	*length += targetBlockOccurrence.length;
	encodeSdnv(&ciphersuiteType, asb->ciphersuiteType);
	*length += ciphersuiteType.length;
	encodeSdnv(&ciphersuiteFlags, asb->ciphersuiteFlags);
	*length += ciphersuiteFlags.length;
	if (asb->ciphersuiteFlags & BPSEC_ASB_PARM)
	{
		encodeSdnv(&parmsLen, asb->parmsLen);
		*length += parmsLen.length;
		*length += asb->parmsLen;
	}

	BPSEC_DEBUG_INFO("i bpsec_serializeASB RESULT LENGTH IS CURRENTLY (%d)",
			asb->resultsLen);

	if (asb->ciphersuiteFlags & BPSEC_ASB_RES)
	{
		encodeSdnv(&resultsLen, asb->resultsLen);
		*length += resultsLen.length;

		/*	The resultsLen field may be hypothetical; the
		 *	resultsData may not yet be present.  But it
		 *	will be provided eventually (even if only as
		 *	filler bytes), so the block's serialized
		 *	length must include resultsLen.			*/

		*length += asb->resultsLen;
	}

	/*********************************************************************
	 *             Serialize the ASB into the allocated buffer           *
	 *********************************************************************/

	if ((serializedAsb = MTAKE(*length)) == NULL)
	{
		BPSEC_DEBUG_ERR("x bpsec_serializeASB Need %d bytes.", *length);
		BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);
		return NULL;
	}

	cursor = serializedAsb;
	cursor = bpsec_addSdnvToStream(cursor, &targetBlockType);
	cursor = bpsec_addSdnvToStream(cursor, &targetBlockOccurrence);
	cursor = bpsec_addSdnvToStream(cursor, &ciphersuiteType);
	cursor = bpsec_addSdnvToStream(cursor, &ciphersuiteFlags);

	if (asb->ciphersuiteFlags & BPSEC_ASB_PARM)
	{
		cursor = bpsec_addSdnvToStream(cursor, &parmsLen);
		BPSEC_DEBUG_INFO("i bpsec_serializeASB: cursor %x, parms data \
%u, parms length %ld", (unsigned long) cursor, (unsigned long) asb->parmsData,
				asb->parmsLen);
		if (asb->parmsData == 0)
		{
			memset(cursor, 0, asb->parmsLen);
		}
		else
		{
			sdr_read(sdr, (char *) cursor, asb->parmsData,
					asb->parmsLen);
		}

		cursor += asb->parmsLen;
	}

	if (asb->ciphersuiteFlags & BPSEC_ASB_RES)
	{
		cursor = bpsec_addSdnvToStream(cursor, &resultsLen);
		BPSEC_DEBUG_INFO("i bpsec_serializeASB: cursor " ADDR_FIELDSPEC
			", results data  0x%x, results length %d",
			(uaddr) cursor, asb->resultsData, asb->resultsLen);
		if (asb->resultsData != 0)
		{
			sdr_read(sdr, (char *) cursor, asb->resultsData,
					asb->resultsLen);
			cursor += asb->resultsLen;
		}
	}

	BPSEC_DEBUG_INFO("i bpsec_serializeASB -> data: " ADDR_FIELDSPEC
			", length %d", (uaddr) serializedAsb, *length);
	BPSEC_DEBUG_PROC("- bpsec_serializeASB", NULL);

	return serializedAsb;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_transferToZcoFileSource
 *
 * \par Purpose: This utility function attains a zco object, a file reference, a
 *               character string and appends the string to a file. A file
 *               reference to the new data is appended to the zco object. If given
 *               an empty zco object- it will create a new one on the empty pointer.
 *               If given an empty file reference, it will create a new file.
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
 *  01/31/16  E. Birrane    Update to BPSEC
 *****************************************************************************/
int bpsec_transferToZcoFileSource(Sdr sdr, Object *resultZco,
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
		*resultZco = zco_create(sdr, ZcoSdrSource, 0, 0, 0, ZcoOutbound);
		if (*resultZco == (Object) ERROR)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't start file source ZCO.", NULL);
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
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't get CWD for acq file name.", NULL);
			sdr_cancel_xn(sdr);
			return 0;
		}

		acqCount++;
		isprintf(fileName, sizeof fileName, "%s%c%s.%u", cwd,
				ION_PATH_DELIMITER, fname, acqCount);
		fd = open(fileName, O_WRONLY | O_CREAT, 0666);
		if (fd < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't create acq file %s.", fileName);
			sdr_cancel_xn(sdr);
			return 0;
		}

		fileLength = 0;
		*acqFileRef = zco_create_file_ref(sdr, fileName, "",ZcoInbound);
	}
	else				/*	Writing more to file.	*/
	{
		oK(zco_file_ref_path(sdr, *acqFileRef, fileName, sizeof fileName));
		fd = open(fileName, O_WRONLY, 0666);
		if (fd < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't reopen acq file %s.", fileName);
			sdr_cancel_xn(sdr);
			return 0;
		}

		if ((fileLength = lseek(fd, 0, SEEK_END)) < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't get acq file length %s.", fileName);
			sdr_cancel_xn(sdr);
			close(fd);
			return 0;
		}
	}

	// Write the data to the file
	if (write(fd, bytes, length) < 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't append to acq file %s.", fileName);
		sdr_cancel_xn(sdr);
		close(fd);
		return 0;
	}

	close(fd);


	if (zco_append_extent(sdr, *resultZco, ZcoFileSource, *acqFileRef,
					      fileLength, length) <= 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't append extent to ZCO.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*      Flag file reference for deletion as soon as the last
	 *      ZCO extent that references it is deleted.               */
	zco_destroy_file_ref(sdr, *acqFileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_transferToZcoFileSource: Can't acquire extent into file..", NULL);
		return -1;
	}

	return 1;
}


