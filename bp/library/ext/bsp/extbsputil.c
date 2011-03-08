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
#include "../../bpP.h"	/** HMAC-SHA1 implementation */ 

/*****************************************************************************
 *                            VARIABLE DEFINITIONS                           *
 *****************************************************************************/

/** \var gMsg
 * Global variable used to hold a constructed error message. NOT RE-ENTRANT! 
 * This is accessed by the BSP_DEBUG macros.
 */
char	gMsg[GMSG_BUFLEN];

/* If we are only in 1 scheme, return custodian EID for that scheme. If we
   are in  multiple schemes, return custodian EID for the peerEid given.
*/

char * getCustodianEid(char *peerEid)
{
  char *temp = NULL;

   VScheme	*vscheme = NULL;
   PsmAddress	vschemeElt;
   MetaEid	metaEid;
   int		len;

   CHKNULL(peerEid);
   len = strlen(peerEid);
   if ((temp = MTAKE(len + 1)) == 0)
   {
     BSP_DEBUG_ERR("x getCustodianEid: Unable to allocate EID of size %d",
		     len + 1);
     return NULL;  
   }

   istrcpy(temp, peerEid, len + 1);
   if (parseEidString(temp, &metaEid, &vscheme, &vschemeElt) == 0)
   {
      BSP_DEBUG_ERR("x getCustodianEid: Cannot find scheme for dest EID: %s",
		      temp);
      MRELEASE(temp);
      return NULL;
   }

   MRELEASE(temp);
   return vscheme->custodianEidString;
}

int	extensionBlockTypeToInt(char *blockType)
{
	ExtensionDef	*extensions;
	int		extensionsCt;
	int		i;
	ExtensionDef	*def;

	CHKERR(blockType);
	if (strcmp("payload", blockType) == 0)
		return PAYLOAD_BLOCK_TYPE;
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
	if (blockType == PAYLOAD_BLOCK_TYPE)
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

int bsp_deserializeASB(AcqExtBlock *blk)
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
		extractSdnv(&(asb->cipher),      &cursor, &unparsedBytes);
		extractSdnv(&(asb->cipherFlags), &cursor, &unparsedBytes);

		BSP_DEBUG_INFO("i bsp_deserializeASB: cipher %ld, flags %ld, length %d",
				asb->cipher, asb->cipherFlags, blk->dataLength);

		if(asb->cipherFlags & BSP_ASB_CORR)
		{
			extractSdnv(&(asb->correlator), &cursor, &unparsedBytes);
			BSP_DEBUG_INFO("i bsp_deserializeASB: corr. = %ld", asb->correlator);
		}

		if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
		{
			/* TODO: Not implemented yet. */
		}

		if(asb->cipherFlags & BSP_ASB_RES)
		{
			extractSdnv(&(asb->resultLen), &cursor, &unparsedBytes);
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


#if 0
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
#endif


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
	encodeSdnv(&resultLen, asb->resultLen);

	*length = cipherFlags.length + cipher.length;

	if(asb->cipherFlags & BSP_ASB_CORR)
	{
		*length += correlator.length;
	}

	if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
	{
		/* TODO: Not implemented yet. */
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
			/* TODO: Not implemented yet. */
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
		unsigned long bspLen, unsigned char **val,
		unsigned long *len)
{
	unsigned char	*cursor = bspBuf;
	unsigned char	itemType;
	int		sdnvLength;
	unsigned long	itemLength;

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

		sdnvLength = decodeSdnv(&itemLength, cursor);
		if (sdnvLength == 0 || sdnvLength > bspLen)
		{
			return;		/*	Malformed result data.	*/
		}

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
			int result;

			result = sec_get_bspBabRule(eidSourceString, eidDestString, &ruleAddr, &eltp);

			if((result == -1) || (eltp == 0))
			{
				BSP_DEBUG_INFO("i bsp_getSecurityInfo: No TX/RX entry for EID %s.", eidSourceString);
			}
			else
			{
				/** \todo: Check ciphersuite name */
				GET_OBJ_POINTER(getIonsdr(), BspBabRule, babRule, ruleAddr);

				if (babRule->ciphersuiteName[0] != '\0')
				{
					istrcpy(secInfo->cipherKeyName, babRule->keyName, sizeof(secInfo->cipherKeyName));
				}
				BSP_DEBUG_INFO("i bsp_getSecurityInfo: get TX/RX key name of '%s'", secInfo->cipherKeyName);
			}
		}
	}

	BSP_DEBUG_PROC("- bsp_getSecurityInfo %c", ' ');
}


