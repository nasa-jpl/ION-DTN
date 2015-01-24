/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 ** \file extbsp.c
 ** 
 ** File Name: extbsputil.c
 **
 **
 ** Subsystem:
 **          Extensions: BSP
 **
 ** Description: This file provides a partial implementation of the Bundle
 **              Security Protocol (BSP) Specification, Version 0.8. This
 **              implementation utilizes the ION Extension Interface to
 **              manage the creation, modification, evaluation, and removal
 **              of BSP blocks from Bundle Protocol (RFC 5050) bundles.
 **
 ** Notes:  The current implementation of this file (6/2009) only supports
 **         the Bundle Authentication Block (BAB) with the following
 **         constraints:
 **         - Bundle fragmentation is not considered
 **         - Only the HMAC-SHA1 ciphersuite for BAB is considered
 **         - No ciphersuite parameters are utilized or supported.
 **         - All BAB blocks will utilize both the pre- and post-payload block.
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
 **  06/08/09  E. Birrane           Initial Implementation of BAB blocks.
 **  06/15/09  E. Birrane           BAB Unit Testing and Documentation updates.
 **  06/20/09  E. Birrane           Documentation updates for initial release.
 **  12/04/09  S. Burleigh          Revisions per DINET and DEN testing.
 **  01/14/11  B. Van Besien        Revised to use old security syntax.
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "extbsputil.h"	/** BSP structures and enumerations */
#include "bpP.h"	/** HMAC-SHA1 implementation */ 

/*****************************************************************************
 *                            VARIABLE DEFINITIONS                           *
 *****************************************************************************/

/** \var gMsg
 * Global variable used to hold a constructed error message. NOT RE-ENTRANT! 
 * This is accessed by the BSP_DEBUG macros.
 */
char	gMsg[GMSG_BUFLEN];

int	bspTypeToString(int bspType, char *s, int buflen)
{
	CHKERR(s);
	switch (bspType)
	{
	case BSP_BAB_TYPE:
		istrcat(s, "BAB", buflen);
		break;

	case BSP_PIB_TYPE:
		istrcat(s, "PIB", buflen);
		break;

	case BSP_PCB_TYPE:
		istrcat(s, "PCB", buflen);
		break;

	case BSP_ESB_TYPE:
		istrcat(s, "ESB", buflen);
		break;

	default:
		istrcat(s, " ", buflen);
		return -1;
	}

	return 0;
}

int	bspTypeToInt(char *bspType)
{
	CHKERR(bspType);
	if (strncmp(bspType, "BAB", 3) == 0)
		return BSP_BAB_TYPE;
	else if (strncmp(bspType, "bab", 3) == 0)
		return BSP_BAB_TYPE;
	else if (strncmp(bspType, "PIB", 3) == 0)
		return BSP_PIB_TYPE;
	else if (strncmp(bspType, "pib", 3) == 0)
		return BSP_PIB_TYPE;
	else if (strncmp(bspType, "PCB", 3) == 0)
		return BSP_PCB_TYPE;
	else if (strncmp(bspType, "pcb", 3) == 0)
		return BSP_PCB_TYPE;
	else if (strncmp(bspType, "ESB", 3) == 0)
		return BSP_ESB_TYPE;
	else if (strncmp(bspType, "esb", 3) == 0)
		return BSP_ESB_TYPE;
	return -1;
}

int	extensionBlockTypeToInt(char *blockType)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;

	CHKERR(blockType);
	if (strcmp("payload", blockType) == 0)
		return BLOCK_TYPE_PAYLOAD;
	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (strcmp(def->name, blockType) == 0)
		{
			return def->type;
		}
	}

	return -1;
}

int	extensionBlockTypeToString(unsigned char blockType, char *s,
		unsigned int buflen)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;

	if (blockType == 0) return -1;
	CHKERR(s);
	if (blockType == BLOCK_TYPE_PAYLOAD)
	{
		istrcat(s, "payload", buflen);
		return 0;
	}

	getExtensionDefs(&extensions, &extensionsCt);
	for (i = 0, def = extensions; i < extensionsCt; i++, def++)
	{
		if (def->type == blockType)
		{
			istrcat(s, def->name, buflen);
			return 0;
		}
	}

	return -1;
}

/*****************************************************************************
 *                           GENERAL BSP FUNCTIONS                           *
 *****************************************************************************/


/******************************************************************************
 *
 * \par Function Name: bsp_addSdnvToStream
 *
 * \par Purpose: This utility function adds the contents of an SDNV to a
 *               character stream and then returns the updated stream pointer.
 *
 * \retval unsigned char * -- The updated stream pointer.
 *
 * \param[in]  stream  The current position of the stream pointer.
 * \param[in]  value   The SDNV value to add to the stream.
 *
 * \par Notes: 
 *      1. Input parameters are passed as pointers to prevent wasted copies.
 *         Therefore, this function must be careful not to modify them.
 *      2. This function assumes that the stream is a character stream.
 *      3. We assume that we are not under such tight profiling constraints
 *         that sanity checks are too expensive.
 *****************************************************************************/

unsigned char *bsp_addSdnvToStream(unsigned char *stream, Sdnv* value)
{
	BSP_DEBUG_PROC("+ bsp_addSdnvToStream(%x, %x)",
			(unsigned long) stream, (unsigned long) value);

	if((stream != NULL) && (value != NULL) && (value->length > 0))
	{
		BSP_DEBUG_INFO("i bsp_addSdnvToStream: Adding %d bytes", value->length);
		memcpy(stream, value->text, value->length);
		stream += value->length;
	}

	BSP_DEBUG_PROC("- bsp_addSdnvToStream --> %x", (unsigned long) stream);

	return stream;
}


/******************************************************************************
 *
 * \par Function Name: bsp_deserializeASB
 *
 * \par Purpose: This utility function accepts a serialized Abstract Security
 *               Block from a bundle during acquisition and places it in a
 *               AbstractSecurityBlock structure stored in the Acquisition
 *               Block's scratchpad area.
 *
 * \retval int -- 1 - An ASB was successfully deserialized into the scratchpad
 *                0 - The deserialized ASB did not pass its sanity check.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  A pointer to the acquisition block holding the
 *                      serialized abstract security block.
 *
 * \par Notes: 
 *      1. This function allocates memory using the MTAKE method.  This
 *         scratchpad must be freed by the caller iff the method does
 *         not return -1.  Any system error will release the memory.
 *
 *      2.  If we return a 1, the ASB is considered corrupted and not usable.
 *          The block should be discarded. It is still returned, though, so that
 *          the caller may examine it.
 *****************************************************************************/

int bsp_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk, int blockType)
{
	int result = 1;

	BSP_DEBUG_PROC("+ bsp_deserializeASB(%x)", (unsigned long)blk);

	/* Sanity Check 1: We have a block. */
	if(blk == NULL)
	{
		BSP_DEBUG_ERR("x bsp_deserializeASB: Bad parms: blk = %x", (unsigned long) blk);
		result = -1;
	}
	/* Sanity Check 2: The block has a scratchpad. */
	else if((blk->object == NULL) || (blk->size == 0))
	{
		BSP_DEBUG_ERR("x bsp_deserializeASB: No Scratchpad: blk->size %d",
				blk->size);
		result = -1;
	}
	/* Sanity Check 3: The block has a known type. */
	else if((blk->type != BSP_BAB_TYPE) &&
			(blk->type != BSP_PIB_TYPE) &&
			(blk->type != BSP_PCB_TYPE) &&
			(blk->type != BSP_ESB_TYPE))
	{
		BSP_DEBUG_WARN("? bsp_deserializeASB: Not a BSP block: %d", blk->type);
		result = 0;
	}
	/* If we pass sanity checks, we are good to deserialize. */
	else
	{
		BspAbstractSecurityBlock *asb = (BspAbstractSecurityBlock *) blk->object;
		unsigned char *cursor = NULL;
		int unparsedBytes = blk->length;

		/*
		 * Position cursor to start of security-specific part of the block, which
		 * is after the canonical block header area.  The security-specific part
		 * of the block has length blk->dataLength, so it is easy to find the
		 * start of the part given the overall block length.
		 */
		cursor = ((unsigned char*)(blk->bytes)) + (blk->length - blk->dataLength);

		/* Extract block specifics, using cipherFlags as necessary. */
		extractSmallSdnv(&(asb->cipher),      &cursor, &unparsedBytes);
		extractSmallSdnv(&(asb->cipherFlags), &cursor, &unparsedBytes);

		BSP_DEBUG_INFO("i bsp_deserializeASB: cipher %ld, flags %ld, length %d",
				asb->cipher, asb->cipherFlags, blk->dataLength);

                if(setSecPointsRecv(blk, wk, blockType) != 0)
                {
			BSP_DEBUG_ERR("x bsp_deserializeASB: Failed to set ASB sec src/dest", NULL);
                	result = -1;
		}

		if(asb->cipherFlags & BSP_ASB_CORR)
		{
			extractSmallSdnv(&(asb->correlator), &cursor,
					&unparsedBytes);
			BSP_DEBUG_INFO("i bsp_deserializeASB: corr. = %ld", asb->correlator);
		}

		if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
		{
			/* Not implemented yet. */
		}

		if(asb->cipherFlags & BSP_ASB_RES)
		{
			extractSmallSdnv(&(asb->resultLen), &cursor,
					&unparsedBytes);
			if(asb->resultLen == 0)
			{
				BSP_DEBUG_ERR("x bsp_deserializeASB: ResultLen is 0 with flags %ld",
						asb->cipherFlags);
				result = 0;
			}

			else if((asb->resultData = MTAKE(asb->resultLen)) == NULL)
			{
				BSP_DEBUG_ERR("x bsp_deserializeASB: Failed allocating %ld bytes.",
						asb->resultLen);
				result = -1;
			}

			else
			{
				BSP_DEBUG_INFO("i bsp_deserializeASB: resultLen %ld",
						asb->resultLen);
				memcpy((char *)asb->resultData, cursor, asb->resultLen);
				cursor += asb->resultLen;

				result = 1;
			}
		}
	}

	BSP_DEBUG_PROC("- bsp_deserializeASB -> %d", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: bsp_eidNil
 *
 * \par Purpose: This utility function determines whether a given EID is
 *               "nil".  Nil in this case means that the EID is uninitialized
 *               or will otherwise resolve to the default nullEID. 
 *
 * \retval int - Whether the EID is nil (1) or not (0).
 *
 * \param[in]  eid - The EndpointID being checked.
 *
 * \par Notes: 
 *      1. Nil check is pulled from whether the ION library printEid function
 *         will use the default nullEID when given this EID. 
 *****************************************************************************/

int bsp_eidNil(EndpointId *eid)
{
	int result = 0;

	BSP_DEBUG_PROC("+ bsp_eidNil(%x)", (unsigned long) eid);

	/*
	 * EIDs have two mutually exclusive representations, so pick right one to
	 * check.
	 */
	if(eid->cbhe == 0)
	{
		BSP_DEBUG_INFO("i bsp_eidNil: scheme %d, nss %d",
				eid->d.schemeNameOffset, eid->d.nssOffset);

		result = (eid->d.schemeNameOffset == 0) &&
				(eid->d.nssOffset == 0);
	}
	else
	{
		BSP_DEBUG_INFO("i bsp_eidNil: node %ld, service %ld",
				eid->c.nodeNbr, eid->c.serviceNbr);

		result = (eid->c.nodeNbr == 0);
	}

	BSP_DEBUG_PROC("- bsp_eidNil --> %d", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: bsp_findAcqExtBlk
 *
 * \par Purpose: This utility function finds an acquisition extension block
 *               from within the work area.
 *
 * \retval AcqExtBlock * -- The found block, or NULL.
 *
 * \param[in]  wk      - The work area holding the blocks.
 * \param[in]  listIdx - Whether we want to look in the pre- or post- payload
 *                       area for the block.
 * \param[in[  type    - The block type.
 * 
 * \par Notes: 
 *      1. This function should be moved into libbpP.c
 *****************************************************************************/

AcqExtBlock *bsp_findAcqExtBlk(AcqWorkArea *wk, int listIdx, int type)
{
	LystElt      elt;
	AcqExtBlock  *result = NULL;

	BSP_DEBUG_PROC("+ bsp_findAcqExtBlk(%x, %d, %d",
			(unsigned long) wk, listIdx, type);
	CHKNULL(wk);
	for (elt = lyst_first(wk->extBlocks[listIdx]); elt; elt = lyst_next(elt))
	{
		result = (AcqExtBlock *) lyst_data(elt);

		BSP_DEBUG_INFO("i bsp_findAcqExtBlk: Found type %d", result->type);
		if(result->type == type)
		{
			break;
		}

		result = NULL;
	}

	BSP_DEBUG_PROC("- bsp_findAcqExtBlk -- %x", (unsigned long) result);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_retrieveKey
 *
 * \par Purpose: Retrieves the key associated with a particular keyname.
 *
 * \retval char * -- !NULL - A pointer to a buffer holding the key value.
 *                    NULL - There was a system error.
 *
 * \param[out] keyLen   The length of the key value that was found.
 * \param[in]  keyName  The name of the key to find.
 *
 * \par Notes: 
 *****************************************************************************/

unsigned char *bsp_retrieveKey(int *keyLen, char *keyName)
{
	unsigned char *keyValueBuffer = NULL;
	char c = ' ';

	BSP_DEBUG_PROC("+ bsp_retrieveKey(%d, %s)", *keyLen, keyName);

	/*
	 * We pass in a key length of zero.  This should result in the sec-get_key
	 * function populating this value with the key length.  We cannot pass in a
	 * value of NULL for the keyValueBuffer, so we pass in a single char pointer.
	 */
	CHKNULL(keyLen);
	CHKNULL(keyName);
	*keyLen = 0;
	if(sec_get_key(keyName, keyLen, &c) != 0)
	{
		BSP_DEBUG_ERR("bsp_retrieveKey:  Unable to return length of key %s.",
				keyName);
	}

	/* If no key length, the key must not have been found.*/
	else if(*keyLen == 0)
	{
		BSP_DEBUG_ERR("x bsp_retrieveKey: Unable to find key %s", keyName);
	}

	else if((keyValueBuffer = (unsigned char *) MTAKE(*keyLen)) == NULL)
	{
		BSP_DEBUG_ERR("x bsp_retrieveKey: Failed to allocate %d bytes", *keyLen);
	}

	/* Now we have key length and allocated buffer, so get key. */
	else if(sec_get_key(keyName, keyLen, (char *) keyValueBuffer) != *keyLen)
	{
		BSP_DEBUG_ERR("bsp_retrieveKey:  Can't get key %s", keyName);
		MRELEASE(keyValueBuffer);
		keyValueBuffer = NULL;
	}

	BSP_DEBUG_PROC("- bsp_retrieveKey - (begins with) %.*s", MIN(128, *keyLen),
			keyValueBuffer);

	return keyValueBuffer;
}


/******************************************************************************
 *
 * \par Function Name: bsp_serializeASB
 *
 * \par Purpose: Serializes an abstract security block and returns the 
 *               serialized representation.
 *
 * \retval unsigned char * - the serialized Abstract Security Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  asb    The BspAbstractSecurityBlock to serialize.
 *
 * \par Notes: 
 *      1. This function uses MTAKE to allocate space for the result. This
 *         result (if not NULL) must be freed using MRELEASE. 
 *      2. This function only serializes the "security specific" ASB, not the
 *         canonical header information of the encapsulating BP extension block.
 *****************************************************************************/

unsigned char *bsp_serializeASB(unsigned int *length, BspAbstractSecurityBlock *asb)
{
	Sdnv cipherFlags;
	Sdnv cipher;
	Sdnv correlator;
	Sdnv resultLen;
	unsigned char *result = NULL;

	BSP_DEBUG_PROC("+ bsp_serializeASB(%x, %x)",
			(unsigned long) length, (unsigned long) asb);

	/*
	 * Sanity check. We should have an ASB and the block should not already
	 * have a serialized version of itself.
	 */
	if((asb == NULL) || (length == NULL))
	{
		BSP_DEBUG_ERR("x bsp_serializeASB:  Parameters are missing. Asb is %x",
				(unsigned long) asb);
		BSP_DEBUG_PROC("- bsp_serializeASB --> %s","NULL");
		return NULL;
	}

	/***************************************************************************
	 *                       Calculate the BLock Length                        *
	 ***************************************************************************/

	/*
	 * We need to assign all of our SDNV values first so we know how many
	 * bytes they will take up. We don't want a separate function to calcuate
	 * this length as it would result in generating multiple SDNV values
	 * needlessly.
	 */

	encodeSdnv(&cipherFlags, asb->cipherFlags);
	encodeSdnv(&cipher, asb->cipher);
	encodeSdnv(&correlator, asb->correlator);
	BSP_DEBUG_PROC("+ bsp_serializeASB KEY LENGTH IS CURRENTLY (%d)", asb->resultLen);
	encodeSdnv(&resultLen, asb->resultLen);

	*length = cipherFlags.length + cipher.length;

	if(asb->cipherFlags & BSP_ASB_CORR)
	{
		*length += correlator.length;
	}

	if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
	{
		/* Not implemented yet. */
	}

	if(asb->cipherFlags & BSP_ASB_RES)
	{
		*length += resultLen.length + asb->resultLen;
	}

	/***************************************************************************
	 *                Serialize the ASB into the allocated buffer              *
	 ***************************************************************************/
	if((result = MTAKE(*length)) == NULL)
	{
		BSP_DEBUG_ERR("x bsp_serializeBlock:  Unable to malloc %d bytes.",
				*length);
		*length = 0;
		result = NULL;
	}
	else
	{
		unsigned char *cursor = result;

		cursor = bsp_addSdnvToStream(cursor, &cipher);
		cursor = bsp_addSdnvToStream(cursor, &cipherFlags);

		if(asb->cipherFlags & BSP_ASB_CORR)
		{
			cursor = bsp_addSdnvToStream(cursor, &correlator);
		}

		if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
		{
			/* Not implemented yet. */
		}

		if(asb->cipherFlags & BSP_ASB_RES)
		{
			cursor = bsp_addSdnvToStream(cursor, &resultLen);
			BSP_DEBUG_INFO("i bsp_serializeASB: cursor %x, data %x, length %ld",
					(unsigned long)cursor, (unsigned long) asb->resultData,
					asb->resultLen);
			if(asb->resultData != NULL)
			{
				memcpy(cursor, (char *)asb->resultData, asb->resultLen);
			}
			else
			{
				memset(cursor,0, asb->resultLen);
			}
			cursor += asb->resultLen;
		}

		/* Sanity check */
		if( (unsigned long)(cursor - result) > (*length + 1))
		{
			BSP_DEBUG_ERR("x bsp_serializeASB: Check failed. Length is %d not %d",
					cursor-result, *length + 1);
			MRELEASE(result);
			*length = 0;
			result = NULL;
		}
	}

	BSP_DEBUG_PROC("- bsp_serializeASB -> data: %x, length %d",
			(unsigned long) result, *length);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: getBspItem
 *
 * \par Purpose: This function searches within a BSP buffer (a ciphersuite
 *		 parameters field or a security result field) for an
 *		 information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in bsp.h
 *                         as BSP_CSPARM_xxx macros.
 * \param[in]  bspBuf      The data buffer in which to search for the item.
 * \param[in]  bspLen      The length of the data buffer.
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

void	getBspItem(int itemNeeded, unsigned char *bspBuf,
		unsigned int bspLen, unsigned char **val,
		unsigned int *len)
{
	unsigned char	*cursor = bspBuf;
	unsigned char	itemType;
	int		sdnvLength;
	uvast		longNumber;
	unsigned int	itemLength;

	CHKVOID(bspBuf);
	CHKVOID(val);
	CHKVOID(len);
	*val = NULL;		/*	Default.			*/
	*len = 0;		/*	Default.			*/

	/*	Walk through all items in the buffer (either a security
	 *	parameters field or a security result field, searching
	 *	for an item of the indicated type.			*/

	while (bspLen > 0)
	{
		itemType = *cursor;
		cursor++;
		bspLen--;
		if (bspLen == 0)	/*	No item length.		*/
		{
			return;		/*	Malformed result data.	*/
		}

		sdnvLength = decodeSdnv(&longNumber, cursor);
		if (sdnvLength == 0 || sdnvLength > bspLen)
		{
			return;		/*	Malformed result data.	*/
		}

		itemLength = longNumber;
		cursor += sdnvLength;
		bspLen -= sdnvLength;

		if (itemLength == 0)	/*	Empty item.		*/
		{
			continue;
		}

		if (itemType == itemNeeded)
		{
			*val = cursor;
			*len = itemLength;
			return;
		}

		/*	Look at next item in result data.		*/
		cursor += itemLength;
		bspLen -= itemLength;
	}
}

/******************************************************************************
 *
 * \par Function Name: bsp_getSecurityInfo
 *
 * \par Purpose: This utility function retrieves security information for a
 *               given block from the ION security manager.
 *
 * \retval void 
 *
 * \param[in]  bundle    The bundle that holding the block whose security
 *                       information is being requested.
 * \param[in]  bspType   The type of BSP block whose key is being requested.
 * \param[in]  eidSourceString The name of the source endpoint.
 * \param[in]  eidDestString The name of the destination endpoint.
 * \param[out] secInfo   The block scratchpad holding security information for 
 *                       this block.
 *****************************************************************************/

void bsp_getSecurityInfo(Bundle *bundle, 
		int bspType,
		int blockType,
		char *eidSourceString,
		char *eidDestString,
		BspSecurityInfo *secInfo)
{
	BSP_DEBUG_PROC("+ bsp_getSecurityInfo(0x%08x, %d, %s, %s, 0x%08x)",
			(unsigned long) bundle, bspType, eidSourceString, eidDestString,(unsigned long) secInfo);

	CHKVOID(secInfo);
	secInfo->cipherKeyName[0] = '\0';

	/* Since we look up key information by EndPointID, if we do not have the
	 * EID, we cannot get any security information.  We will assume, then, that
	 * we are not using the BAB.
	 */
	if(eidSourceString == NULL)
	{
		BSP_DEBUG_WARN("? bsp_getSecurityInfo: Can't get EID from bundle \
				%x.", (unsigned long) bundle);
	}
	else
	{
		Object ruleAddr;
		Object eltp;


		if(bspType == BSP_BAB_TYPE)
		{
			OBJ_POINTER(BspBabRule, babRule);

			sec_get_bspBabRule(eidSourceString, eidDestString, &ruleAddr, &eltp);

			if (eltp == 0)
			{
				BSP_DEBUG_INFO("i bsp_getSecurityInfo: No TX/RX entry for EID %s.", eidSourceString);
			}
			else
			{
				GET_OBJ_POINTER(getIonsdr(), BspBabRule, babRule, ruleAddr);

				if (babRule->ciphersuiteName[0] != '\0')
				{
					istrcpy(secInfo->cipherKeyName, babRule->keyName, sizeof(secInfo->cipherKeyName));
				}
				BSP_DEBUG_INFO("i bsp_getSecurityInfo: get TX/RX key name of '%s'", secInfo->cipherKeyName);
			}
		}

		if(bspType == BSP_PIB_TYPE)
		{
			OBJ_POINTER(BspPibRule, pibRule);
			int result;

			result = sec_get_bspPibRule(eidSourceString, eidDestString, blockType, &ruleAddr, &eltp);

			if((result == -1) || (eltp == 0))
			{
				BSP_DEBUG_INFO("i bsp_getSecurityInfo: No TX/RX entry for EID %s.", eidSourceString);
			}
			else
			{
				GET_OBJ_POINTER(getIonsdr(), BspPibRule, pibRule, ruleAddr);

				if (pibRule->ciphersuiteName[0] != '\0')
				{
					istrcpy(secInfo->cipherKeyName, pibRule->keyName, sizeof(secInfo->cipherKeyName));
				}
				BSP_DEBUG_INFO("i bsp_getSecurityInfo: get TX/RX key name of '%s'", secInfo->cipherKeyName);
			}
		}

                if(bspType == BSP_PCB_TYPE)
                {
                        OBJ_POINTER(BspPcbRule, pcbRule);
                        int result;

                        result =  sec_get_bspPcbRule(eidSourceString, eidDestString, blockType, &ruleAddr, &eltp);

                        if((result == -1) || (eltp == 0))
                        {
                                BSP_DEBUG_INFO("i bsp_getSecurityInfo: No TX/RX entry for EID %s.", eidSourceString);
                        }
                        else
                        {
                                GET_OBJ_POINTER(getIonsdr(), BspPcbRule, pcbRule, ruleAddr);

                                if (pcbRule->ciphersuiteName[0] != '\0')
                                {
                                        istrcpy(secInfo->cipherKeyName, pcbRule->keyName, sizeof(secInfo->cipherKeyName));
                                }
                                BSP_DEBUG_INFO("i bsp_getSecurityInfo: get TX/RX key name of '%s'", secInfo->cipherKeyName);
                        }
                }


	}

	BSP_DEBUG_PROC("- bsp_getSecurityInfo %c", ' ');
}

char *getLocalAdminEid(DequeueContext *ctxt)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	/*
	 * Look at scheme we are delivering to, as that will be the
	 * scheme of our local admin EID, as we don't cross schemes
	 * in transmit.
	 */
	if (parseEidString(ctxt->proxNodeEid, &metaEid, &vscheme,
			&vschemeElt) == 0)
	{
		/*	Can't know which admin EID to use.		*/
		return NULL;
	}

	restoreEidString(&metaEid);
   	return vscheme->adminEid;
}

/******************************************************************************
 *
 * \par Function Name: setSecPointsRecv
 *
 * \par Purpose: This utility function takes care of all addressing operations
 *               in deserialize. Most importantly, it sets the sec Src and dest
 *               of the asb block in blk->object. This is dependent on the given
 *               block type, the presence of any already existing eid references in blk,               
 *               and the availiability of convergence layer given sender eid (wk->senderEid)
 *
 * \retval int - 0 indicates success, -1 is an error
 *
 * \param[out]  blk       our extension block, needed to get any existing eid refs
 *                        also holds our asb in blk->object
 * \param]in]  wk         our acquired work area, holds info on bundle like src/dest addr
 * \param[in]  blockType  BAB, PCB, PIB
 *****************************************************************************/

int setSecPointsRecv(AcqExtBlock *blk, AcqWorkArea *wk, int blockType)
{
    BspAbstractSecurityBlock *asb = (BspAbstractSecurityBlock *) blk->object;
    LystElt eidElt = NULL;
    unsigned long   schemeOffset;
    unsigned long   nssOffset;
    VScheme      *vscheme = NULL;        
    PsmAddress   vschemeElt;
    MetaEid      metaEid;
    char *tmp = MTAKE(MAX_EID_LEN + 1);

    // SECURITY POLICY: If a security source or destination are not present,
    // then we will assume they are the bundle's source and destination.
    // The exception is BAB block, where we check for convergence layer
    // given eid addr before trying to use bundle source/dest

    // Extract EID references if they are present. If security source is
    // present, it will be first, so set up an eidReferences iterator and
    // let it run first looking for source EID and then for dest EID.
    if(asb->cipherFlags & BSP_ASB_SEC_SRC || asb->cipherFlags & BSP_ASB_SEC_DEST)
	eidElt = lyst_first(blk->eidReferences);

    if(asb->cipherFlags & BSP_ASB_SEC_SRC)
    {
	// Grab the security source and stuff it in the sec src EID 
	schemeOffset = (unsigned long) lyst_data(eidElt);
	eidElt = lyst_next(eidElt);
	nssOffset = (unsigned long) lyst_data(eidElt);
	// In case theres a destination too:
	eidElt = lyst_next(eidElt);

	asb->secSrc.unicast = 1;
	asb->secSrc.cbhe = (wk->dictionary == NULL);
	if(asb->secSrc.cbhe)
	{
		asb->secSrc.c.nodeNbr = schemeOffset;
		asb->secSrc.c.serviceNbr = nssOffset;
	}
	else
	{
		asb->secSrc.d.schemeNameOffset = schemeOffset;
		asb->secSrc.d.nssOffset = nssOffset;
	}
    }
    else
    {
        // No given sec src
	// if a bab block, try to use convergence layer sender addr
        if(blockType == BSP_BAB_TYPE && wk->senderEid != NULL)
        {
	    istrcpy(tmp, wk->senderEid, MAX_EID_LEN + 1);
	    // parseEidString will mess up the char * given to it..
	    // so just copy it to a temp variable
            if (parseEidString(tmp, &metaEid, &vscheme, &vschemeElt) != 0
	    && metaEid.cbhe == 1)
            {
		// It's CBHE, so we can use it as security source
		asb->secSrc.unicast = 1;
		asb->secSrc.cbhe = 1;  
		asb->secSrc.c.nodeNbr = metaEid.nodeNbr;
		asb->secSrc.c.serviceNbr = 0;
            }
            else
            {
                // failed
	        asb->secSrc = wk->bundle.id.source;
            }
        }
	else 
	{
	    // Not a BAB block, or sender EID unknown
	    asb->secSrc = wk->bundle.id.source;
        }
    }

    if(asb->cipherFlags & BSP_ASB_SEC_DEST)
    {
	// Grab the security destination and stuff it in the sec dest EID 
	schemeOffset = (unsigned long) lyst_data(eidElt);
	eidElt = lyst_next(eidElt);
	nssOffset = (unsigned long) lyst_data(eidElt);

	asb->secDest.unicast = 1;
	asb->secDest.cbhe = (wk->dictionary == NULL);
	if(asb->secDest.cbhe)
	{
	    asb->secDest.c.nodeNbr = schemeOffset;
	    asb->secDest.c.serviceNbr = nssOffset;
	}
	else
	{
	    asb->secDest.d.schemeNameOffset = schemeOffset;
	    asb->secDest.d.nssOffset = nssOffset;
        }
    }
    else
    {
        // No given sec dest
        // if a bab block, assume security destination is the local node
        if(blockType == BSP_BAB_TYPE && wk->senderEid != NULL)
        {   
	    istrcpy(tmp, wk->senderEid, MAX_EID_LEN + 1);
	    // parseEidString will mess up the char * given to it..
	    // so just copy it to a temp variable
            if (parseEidString(tmp, &metaEid, &vscheme, &vschemeElt) != 0
	    && metaEid.cbhe == 1)
            {
                // It's CBHE, so we can use local node as security destination
		asb->secDest.unicast = 1;
                asb->secDest.cbhe = 1;
                asb->secDest.c.nodeNbr = getOwnNodeNbr();
                asb->secDest.c.serviceNbr = 0;
            }
            else
            {
                // failed
                asb->secDest = wk->bundle.destination;
            }
	} 
	else 
	{  
            // Not a BAB block
            asb->secDest = wk->bundle.destination;
	} 

    }
    MRELEASE(tmp);
    return 0;
}

/******************************************************************************
 *
 * \par Function Name: setSecPointsTrans
 *
 * \par Purpose: This utility function takes care of all addressing operations
 *               in the bsp dequeue callback. Most importantly, it sets the given
 *               srcNode and destNode strings to the correct eids for finding the
 *               correct security key for the given block type. Additionally,
 *               this function will set the given eidRefs if it sees that a sec Src
 *               and dest will be needed in the ASB block for future hops.
 *
 * \retval int - 0 indicates success, -1 is an error
 *
 * \param[in]  blk        our extension block, needed to set flags
 * \param]in]  bundle     the attained bundle, needed to get bundle src/dest addr
 * \param[in]  asb        our security block, may need to set sec src/dest
 * \param[out] eidRefs    list that will hold any eid references
 * \param[in]  blockType  post BAB, PCB, PIB, 0 for undefined (pre BAB)
 * \param[in]  ctxt       object relevant to this node, needed for next hop, the admin
 *                        node addr of this hop
 * \param[out] srcNode    string that will hold eid for getting the right sec key 
 * \param[out] destNode   string that will hold eid for getting the right sec key 
 *****************************************************************************/

int setSecPointsTrans(ExtensionBlock *blk, Bundle *bundle, BspAbstractSecurityBlock *asb,
                      Lyst *eidRefs, int blockType, DequeueContext *ctxt, char *srcNode, char *destNode) 
{
    VScheme      *vscheme = NULL;        
    PsmAddress   vschemeElt;
    MetaEid      srcEid, destEid;
    char *dictionary = NULL;
    unsigned long tmp = 0;
    char *tmp2 = NULL;
    if(blockType != 0)
    {
        CHKERR(eidRefs);
    }
    CHKERR(blk); CHKERR(bundle); CHKERR(asb); CHKERR(ctxt);

    // No matter what, we need to set the srcNode and destNode to find the right key:
    // We wouldn't be here if we aren't adding a block. For all block types,
    // the source is this current node
    if(blockType == BSP_BAB_TYPE || blockType == 0)
    {
        // BAB is hop to hop, so the security source is our current node.
        tmp2 = getLocalAdminEid(ctxt);
        memcpy(srcNode, tmp2, strlen(tmp2));

        // For pre bab blocks or blocks without defined type (likely post bab), 
        // dest will be next hop
        memcpy(destNode, ctxt->proxNodeEid, strlen(ctxt->proxNodeEid));
    }
    else
    {
        dictionary = retrieveDictionary(bundle);

	// By policy, PIB/PCB security src/destination is currently just the
        // bundle src/destination. So, if we have a bundle src, use it, else
        // use the local admin endpoint for this node.
	if(bsp_eidNil(&(bundle->id.source)))
	{
	  tmp2 = getLocalAdminEid(ctxt);
	  memcpy(srcNode, tmp2, strlen(tmp2));
	}
        else
        {
	  if (printEid(&(bundle->id.source), dictionary, &tmp2) < 0)
	  {
		  putErrmsg("Can't print source EID.", NULL);
		  releaseDictionary(dictionary);
		  return -1;
	  }

          memcpy(srcNode, tmp2, strlen(tmp2));
          MRELEASE(tmp2);
	}

        // For pib/pcb destination will be the bundle destination
	if (printEid(&(bundle->destination), dictionary, &tmp2) < 0)
	{
		putErrmsg("Can't print destination EID.", NULL);
		releaseDictionary(dictionary);
		return -1;
	}

        memcpy(destNode, tmp2, strlen(tmp2));
        MRELEASE(tmp2);

	// retrieveDictionary() does an MTAKE
        releaseDictionary(dictionary);
    }

    if(blockType == 0)
    {
        // No real blockType defined here according to RFC spec (likely a post bab block)
        // don't mess with sec src/dest; do nothing more
        return 0;
    }

    // Assign some memory for some operations below
    tmp2 = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
    // We need the Eids, copy our src/dest node strings to tmp vars
    // as parseEidString will mess with them in the process
    // We need the MetaEid objects from srcNode and destNode
    memcpy(tmp2, srcNode, strlen(srcNode));
    if (parseEidString(tmp2, &srcEid, &vscheme, &vschemeElt) == 0)
    {
	MRELEASE(tmp2);
	BSP_DEBUG_ERR("x setSecPointsTrans: Cannot get src EID:", NULL);
	return -1;
    } 
    memcpy(tmp2, destNode, strlen(destNode));
    if(parseEidString(tmp2, &destEid, &vscheme, &vschemeElt) == 0) 
    {
	MRELEASE(tmp2);
	BSP_DEBUG_ERR("x setSecPointsTrans: Cannot get dest EID:", NULL);
	return -1;
    }
    MRELEASE(tmp2);



    // if true: This bundle is from anonymous, and the next node is going to need a 
    // sec src/dest since there won't be enough info to figure them out. 
    if(bsp_eidNil(&bundle->id.source))
    {
	if((*eidRefs = lyst_create_using(getIonMemoryMgr())) == NULL)
	{
	    BSP_DEBUG_ERR("x setSecPointsTrans: Can't allocate eidRefs%c.", ' ');
	    return -1;
	}
	else
	{
            //  Every block needs the src since its an anonymous bundle
	    blk->blkProcFlags |= BLK_HAS_EID_REFERENCES;
	    asb->cipherFlags |= (BSP_ASB_SEC_SRC);
	    lyst_insert_last(*eidRefs, (void *) (tmp = srcEid.nodeNbr));
	    lyst_insert_last(*eidRefs, (void *) (tmp = srcEid.serviceNbr));
            // Only bab needs the destination, since for that block only it
            // will be the next hop

	   // Always assume destination inferred on the receiving side from the
 	   // convergence layer for bab, and always assume this is the bundle dest
	   // for pib/pcb, so no need to waste space in the block for a security
  	   // dest.

	   /* if(blockType == BSP_BAB_TYPE) 
	    { 
                asb->cipherFlags |= (BSP_ASB_SEC_DEST);
		lyst_insert_last(*eidRefs, (void *) (tmp = destEid.nodeNbr));
		lyst_insert_last(*eidRefs, (void *) (tmp = destEid.serviceNbr));
	    }*/
	}
    }
    
    return 0;
}

/******************************************************************************
 *
 * \par Function Name: transferToZcoFileSource
 *
 * \par Purpose: This utility function attains a zco object, a file reference, a 
 *               character string and appends the string to a file. A file
 *               reference to the new data is appended to the zco object. If given
 *               an empty zco object- it will create a new one on the empty pointer.
 *               If given an empty file reference, it will create a new file.
 *
 * \retval int - 0 indicates success, -1 is an error
 *
 * \param[in]  sdr        ion sdr
 * \param]in]  resultZco  Object where the file references will go
 * \param[in]  acqFileRef A file references pointing to the file
 * \param[in]  fname      A string to be used as the base of the filename
 * \param[in]  bytes      The string data to write in the file
 * \param[in]  length     Length of the string data
 *****************************************************************************/

int     transferToZcoFileSource(Sdr sdr, Object *resultZco, Object *acqFileRef, char *fname, 
                                char *bytes, int length)
{
        static unsigned int    acqCount = 0;
        char                    cwd[200];
        char                    fileName[SDRSTRING_BUFSZ];
        int                     fd;
        int                    fileLength;

        CHKERR(bytes);
        CHKERR(length >= 0);

        CHKERR(sdr_begin_xn(sdr));
        if (*resultZco == 0)     /*      First extent of acquisition.    */
        {
                *resultZco = zco_create(sdr, ZcoSdrSource, 0, 0, 0, ZcoInbound);
                if (*resultZco == (Object) ERROR)
                {
                        putErrmsg("extbsputil: Can't start file source ZCO.",
					NULL);
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
                        putErrmsg("extbsputil: Can't get CWD for acq file \
name.", NULL);
                        sdr_cancel_xn(sdr);
                        return -1;
                }

                acqCount++;
                isprintf(fileName, sizeof fileName, "%s%c%s.%u", cwd,
                                ION_PATH_DELIMITER, fname, acqCount);
                fd = open(fileName, O_WRONLY | O_CREAT, 0666);
                if (fd < 0)
                {
                        putSysErrmsg("extbsputil: Can't create acq file",
					fileName);
                        sdr_cancel_xn(sdr);
                        return -1;
                }

                fileLength = 0;
                *acqFileRef = zco_create_file_ref(sdr, fileName, "",
				ZcoInbound);
        }
	else				/*	Writing more to file.	*/
	{
		oK(zco_file_ref_path(sdr, *acqFileRef, fileName,
				sizeof fileName));
		fd = open(fileName, O_WRONLY, 0666);
		if (fd < 0)
		{
			putSysErrmsg("extbsputil: Can't reopen acq file",
					fileName);
                        sdr_cancel_xn(sdr);
                        return -1;
		}

		if ((fileLength = lseek(fd, 0, SEEK_END)) < 0)
		{
			putSysErrmsg("extbsputil: Can't get acq file length",
					fileName);
			sdr_cancel_xn(sdr);
			close(fd);
			return -1;
		}
	}

        // Write the data to the file
        if (write(fd, bytes, length) < 0)
        {
                putSysErrmsg("extbsputil: Can't append to acq file", fileName);
                sdr_cancel_xn(sdr);
		close(fd);
                return -1;
        }

        close(fd);
        if (zco_append_extent(sdr, *resultZco, ZcoFileSource, *acqFileRef,
                        fileLength, length) <= 0)
	{
		putErrmsg("extbsputil: Can't append extent to ZCO.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

        /*      Flag file reference for deletion as soon as the last
         *      ZCO extent that references it is deleted.               */
        zco_destroy_file_ref(sdr, *acqFileRef);
        if (sdr_end_xn(sdr) < 0)
        {
                putErrmsg("extbsputil: Can't acquire extent into file.", NULL);
                return -1;
        }

        return 0;
}
