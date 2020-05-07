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
 ** \file bsputil.c
 ** 
 ** File Name: bsputil.c (originally extbsputil.c)
 **
 **
 ** Subsystem:
 **          Extensions: BSP
 **
 ** Description: This file provides a partial implementation of the
 **              "Streamlined" Bundle Security Protocol (SBSP) Specification.
 **              This implementation utilizes the ION Extension Interface to
 **              manage the creation, modification, evaluation, and removal
 **              of BSP blocks from Bundle Protocol (RFC 5050) bundles.
 **
 ** Notes:  The original implementation of this file (6/2009) only supported
 **         the Bundle Authentication Block (BAB) with the following
 **         constraints:
 **         - Bundle fragmentation is not considered.
 **         - Only the HMAC-SHA1 ciphersuite for BAB is considered.
 **         - No ciphersuite parameters are utilized or supported.
 **         - All BAB blocks will utilize both the pre- and post-payload block.
 **
 **         As of 14 January 2014, the ION SBSP implementation supports
 **         the Bundle Authentication Block (BAB), Block Integrity Block
 **         (BIB), and Block Confidentiality Block (BCB) with the
 **         following constraints:
 **         - Only the ciphersuites implemented in the ciphersuites.c file
 **           are supported.
 **         - There is currently no support for BIBs and BCBs comprising
 **           First and Last Blocks.  First and Last BABs are supported,
 **           and reception of lone BABs is supported, but ION currently
 **           supports only Lone BIBs and BCBs.
 **         - There is no support for the use of multiple ciphersuites to
 **           offer or acquire SBSP blocks of a given type in the bundle
 **           traffic between a given security source and a given security
 **           destination.  That is, the ciphersuite to be used for offering
 **           or acquiring an SBSP block is a function of the block type,
 **           the security source node, and the security destination node.
 **           When an SBSP block whose security destination is the local
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
 **  06/08/09  E. Birrane           Initial Implementation of BAB blocks.
 **  06/15/09  E. Birrane           BAB Unit Testing and Documentation updates.
 **  06/20/09  E. Birrane           Documentation updates for initial release.
 **  12/04/09  S. Burleigh          Revisions per DINET and DEN testing.
 **  01/14/11  B. Van Besien        Revised to use old security syntax.
 **  01/14/14  S. Burleigh          Revised for "streamlined" BSP.
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bsputil.h"	/** BSP structures and enumerations */

/*****************************************************************************
 *                            VARIABLE DEFINITIONS                           *
 *****************************************************************************/

/** \var gMsg
 * Global variable used to hold a constructed error message. NOT RE-ENTRANT! 
 * This is accessed by the BSP_DEBUG macros.
 */
char	gMsg[GMSG_BUFLEN];

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

unsigned char	*bsp_addSdnvToStream(unsigned char *stream, Sdnv* value)
{
	BSP_DEBUG_PROC("+ bsp_addSdnvToStream(%x, %x)",
			(unsigned long) stream, (unsigned long) value);

	if ((stream != NULL) && (value != NULL) && (value->length > 0))
	{
		BSP_DEBUG_INFO("i bsp_addSdnvToStream: Adding %d bytes",
				value->length);
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
 *               BspInboundBlock structure stored in the Acquisition Extension
 *               Block's scratchpad area.
 *
 * \retval int -- 1 - An ASB was successfully deserialized into the scratchpad
 *                0 - The deserialized ASB did not pass its sanity check.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  A pointer to the acquisition extension block
 *                      containing the serialized abstract security block.
 *                 wk   Work area holding bundle information.
 *
 * \par Notes: 
 *      1.  This function allocates memory using the MTAKE method.  This
 *          scratchpad must be freed by the caller iff the method does
 *          not return -1.  Any system error will release the memory.
 *      2.  If we return a 1, the ASB is considered invalid and not usable.
 *          The block should be discarded. It is still returned, though, so that
 *          the caller may examine it.
 *****************************************************************************/

int	bsp_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int		result = 1;
	BspInboundBlock	asb;
	unsigned char	*cursor = NULL;
	int		unparsedBytes = blk->length;
	LystElt		elt;
	uaddr		ltemp;
	unsigned int	itemp;

	BSP_DEBUG_PROC("+ bsp_deserializeASB(%x)", (unsigned long) blk);
	BSP_DEBUG_INFO("i bsp_deserializeASB blk length %d", blk->length);

	CHKERR(blk);
	CHKERR(wk);
	memset((char *) &asb, 0, sizeof(BspInboundBlock));
	if (blk->eidReferences)
	{
		asb.securitySource.unicast = 1;
		elt = lyst_first(blk->eidReferences);
		ltemp = (uaddr) lyst_data(elt);
		asb.securitySource.d.schemeNameOffset = ltemp;
		elt = lyst_next(elt);
		ltemp = (uaddr) lyst_data(elt);
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
	 *	target field of the inbound BSP block is the
	 *	occurrence number of the target block.			*/

	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.targetBlockOccurrence = itemp;

	/*	For now, ION does not recognize any multi-block
	 *	security operations except BAB.  For BABs, if payload
	 *	has already been acquired at the time this BAB is
	 *	being deserialized then the BAB instance is 1; else
	 *	it is 0.						*/

	if (blk->type == EXTENSION_TYPE_BAB)
	{
		if (wk->bundle.payload.length > 0)
		{
			asb.instance = 1;	/*	Last.		*/
		}
		else
		{
			asb.instance = 0;	/*	Lone or 1st.	*/
		}
	}
	else
	{
		asb.instance = 0;
	}

	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.ciphersuiteType = itemp;
	extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
	asb.ciphersuiteFlags = itemp;

	BSP_DEBUG_INFO("i bsp_deserializeASB: cipher %ld, flags %ld, length %d",
		asb.ciphersuiteType, asb.ciphersuiteFlags, blk->dataLength);

	if (asb.ciphersuiteFlags & BSP_ASB_PARM)
	{
		extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
		asb.parmsLen = itemp;
		if (asb.parmsLen > 0)
		{
			if (asb.parmsLen > unparsedBytes)
			{
				BSP_DEBUG_WARN("? bsp_deserializeASB: \
parmsLen %u, unparsedBytes %u.", asb.parmsLen, unparsedBytes);
				result = 0;
				BSP_DEBUG_PROC("- bsp_deserializeASB -> %d",
						result);
				return result;
			}

			asb.parmsData = MTAKE(asb.parmsLen);
			if (asb.parmsData == NULL)
			{
				putErrmsg("No space for ASB parms.",
						utoa(asb.parmsLen));
				return -1;
			}

			memcpy(asb.parmsData, cursor, asb.parmsLen);
			cursor += asb.parmsLen;
			unparsedBytes -= asb.parmsLen;
			BSP_DEBUG_INFO("i bsp_deserializeASB: parmsLen %ld",
					asb.parmsLen);
		}
	}

	if (asb.ciphersuiteFlags & BSP_ASB_RES)
	{
		extractSmallSdnv(&itemp, &cursor, &unparsedBytes);
		asb.resultsLen = itemp;
		if (asb.resultsLen > 0)
		{
			if (asb.resultsLen > unparsedBytes)
			{
				BSP_DEBUG_WARN("? bsp_deserializeASB: \
resultsLen %u, unparsedBytes %u.", asb.resultsLen, unparsedBytes);
				if (asb.parmsData)
				{
					MRELEASE(asb.parmsData);
				}

				result = 0;
				BSP_DEBUG_PROC("- bsp_deserializeASB -> %d",
						result);
				return result;
			}

			asb.resultsData = MTAKE(asb.resultsLen);
			if (asb.resultsData == NULL)
			{
				putErrmsg("No space for ASB results.",
						utoa(asb.resultsLen));
				return -1;
			}

			memcpy(asb.resultsData, cursor, asb.resultsLen);
			cursor += asb.resultsLen;
			unparsedBytes -= asb.resultsLen;
			BSP_DEBUG_INFO("i bsp_deserializeASB: resultsLen %ld",
					asb.resultsLen);
		}
	}

	blk->size = sizeof(BspInboundBlock);
	blk->object = MTAKE(sizeof(BspInboundBlock));
	if (blk->object == NULL)
	{
		putErrmsg("No space for ASB scratchpad.", NULL);
		return -1;
	}

	memcpy((char *) (blk->object), (char *) &asb, sizeof(BspInboundBlock));

	BSP_DEBUG_PROC("- bsp_deserializeASB -> %d", result);

	return result;
}

void	bsp_insertSecuritySource(Bundle *bundle, BspOutboundBlock *asb)
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
BSP block is not yet implemented.");
	releaseDictionary(dictionary);
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

unsigned char	*bsp_retrieveKey(int *keyLen, char *keyName)
{
	unsigned char	*keyValueBuffer = NULL;
	int		keyBufferLength;
	char		stdBuffer[100];
	int		keyLength;

	BSP_DEBUG_PROC("+ bsp_retrieveKey(%s)", keyName);

	/*
	 * We first guess that the key will normally be no more than 100
	 * bytes long, so we call sec_get_key with a buffer of that size.
	 * If this works, great; we make a copy of the retrieved key
	 * value and pass it back.  If not, then the function has told
	 * us what the actual length of this key is; we allocate a new
	 * buffer of sufficient length and call sec_get_key again to
	 * retrieve the key value into that buffer.
	 */

	CHKNULL(keyLen);
	CHKNULL(keyName);
	keyBufferLength = sizeof stdBuffer;
	keyLength = sec_get_key(keyName, &keyBufferLength, stdBuffer);
	if (keyLength < 0)	/*	System failure.			*/
	{
		BSP_DEBUG_ERR("bsp_retrieveKey: Can't get length of key '%s'.",
				keyName);
		return NULL;
	}

	if (keyLength > 0)	/*	Key has been retrieved.		*/
	{
		keyValueBuffer = (unsigned char *) MTAKE(keyLength);
		if (keyValueBuffer == NULL)
		{
			putErrmsg("No space for key value.", itoa(keyLength));
		}
		else
		{
			memcpy(keyValueBuffer, stdBuffer, keyLength);
			BSP_DEBUG_INFO("i bsp_retrieveKey - length %d, begins \
with %.*s", keyLength, MIN(128, keyLength), keyValueBuffer);
			BSP_DEBUG_PROC("- bsp_retrieveKey", NULL);
		}

		*keyLen = keyLength;
		return keyValueBuffer;
	}

	/*	keyLength == 0; key has not yet been retrieved.		*/

	if (keyBufferLength == sizeof stdBuffer)/*	Key not found.	*/
	{
		BSP_DEBUG_WARN("? bsp_retrieveKey: Unable to find key '%s'",
				keyName);
		BSP_DEBUG_PROC("- bsp_retrieveKey", NULL);
		return NULL;
	}

	/*	Keylen > stdBuffer len; length is in keyBufferLength.	*/

	if ((keyValueBuffer = (unsigned char *) MTAKE(keyBufferLength)) == NULL)
	{
		putErrmsg("No space for key value.", itoa(keyBufferLength));
		return NULL;
	}

	/*	Call sec_get_key again, this time providing a buffer
	 *	of adequate size.					*/

	if (sec_get_key(keyName, &keyBufferLength, (char *) keyValueBuffer) < 0)
	{
		MRELEASE(keyValueBuffer);
		BSP_DEBUG_ERR("bsp_retrieveKey:  Can't get key '%s'", keyName);
		return NULL;
	}

	keyLength = keyBufferLength;
	BSP_DEBUG_INFO("i bsp_retrieveKey - length %d, begins with %.*s",
			keyLength, MIN(128, keyLength), keyValueBuffer);
	BSP_DEBUG_PROC("- bsp_retrieveKey", NULL);

	*keyLen = keyLength;
	return keyValueBuffer;
}

/******************************************************************************
 *
 * \par Function Name: bsp_serializeASB
 *
 * \par Purpose: Serializes an outbound bundle security block and returns the 
 *               serialized representation.
 *
 * \retval unsigned char * - the serialized Abstract Security Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  asb    The BspOutboundBlock to serialize.
 *
 * \par Notes: 
 *      1. This function uses MTAKE to allocate space for the serialized ASB.
 *         This serialized ASB (if not NULL) must be freed using MRELEASE. 
 *      2. This function only serializes the block-type-specific data of
 *         a BSP extension block, not the extension block header.
 *****************************************************************************/

unsigned char	*bsp_serializeASB(unsigned int *length, BspOutboundBlock *asb)
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

	BSP_DEBUG_PROC("+ bsp_serializeASB (%x, %x)",
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
	if (asb->ciphersuiteFlags & BSP_ASB_PARM)
	{
		encodeSdnv(&parmsLen, asb->parmsLen);
		*length += parmsLen.length;
		*length += asb->parmsLen;
	} 

	BSP_DEBUG_INFO("i bsp_serializeASB RESULT LENGTH IS CURRENTLY (%d)",
			asb->resultsLen);

	if (asb->ciphersuiteFlags & BSP_ASB_RES)
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
		BSP_DEBUG_ERR("x bsp_serializeASB Need %d bytes.", *length);
		BSP_DEBUG_PROC("- bsp_serializeASB", NULL);
		return NULL;
	}

	cursor = serializedAsb;
	cursor = bsp_addSdnvToStream(cursor, &targetBlockType);
	cursor = bsp_addSdnvToStream(cursor, &targetBlockOccurrence);
	cursor = bsp_addSdnvToStream(cursor, &ciphersuiteType);
	cursor = bsp_addSdnvToStream(cursor, &ciphersuiteFlags);

	if (asb->ciphersuiteFlags & BSP_ASB_PARM)
	{
		cursor = bsp_addSdnvToStream(cursor, &parmsLen);
		BSP_DEBUG_INFO("i bsp_serializeASB: cursor %x, parms data \
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

	if (asb->ciphersuiteFlags & BSP_ASB_RES)
	{
		cursor = bsp_addSdnvToStream(cursor, &resultsLen);
		BSP_DEBUG_INFO("i bsp_serializeASB: cursor %x, results data \
%u, results length %ld", (unsigned long) cursor, (unsigned long)
				asb->resultsData, asb->resultsLen);
		if (asb->resultsData != 0)
		{
			sdr_read(sdr, (char *) cursor, asb->resultsData,
					asb->resultsLen);
			cursor += asb->resultsLen;
		}
	}

	BSP_DEBUG_INFO("i bsp_serializeASB -> data: %x, length %d",
			(unsigned long) serializedAsb, *length);
	BSP_DEBUG_PROC("- bsp_serializeASB", NULL);

	return serializedAsb;
}

/******************************************************************************
 *
 * \par Function Name: bsp_getInboundBspItem
 *
 * \par Purpose: This function searches within a buffer (a ciphersuite
 *		 parameters field or a security results field) of an
 *		 inbound BSP block for an information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in
 *                         bsputil.h as BSP_CSPARM_xxx macros.
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

void	bsp_getInboundBspItem(int itemNeeded, unsigned char *bspBuf,
		unsigned int bspBufLen, unsigned char **val, unsigned int *len)
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

	/*	Walk through all items in the buffer (either a
	 *	security parameters field or a security results
	 *	field), searching for an item of the indicated type.	*/

	while (bspBufLen > 0)
	{
		itemType = *cursor;
		cursor++;
		bspBufLen--;
		if (bspBufLen == 0)	/*	No item length.		*/
		{
			return;		/*	Malformed buffer data.	*/
		}

		sdnvLength = decodeSdnv(&longNumber, cursor);
		if (sdnvLength == 0 || sdnvLength > bspBufLen)
		{
			return;		/*	Malformed result data.	*/
		}

		itemLength = longNumber;
		cursor += sdnvLength;
		bspBufLen -= sdnvLength;

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

		/*	Look at next item in buffer.			*/

		cursor += itemLength;
		bspBufLen -= itemLength;
	}
}

/******************************************************************************
 *
 * \par Function Name: bsp_getOutboundBspItem
 *
 * \par Purpose: This function searches within a buffer (a ciphersuite
 *		 parameters field or a security results field) of an
 *		 outbound BSP block for an information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in
 *                         bsputil.h as BSP_CSPARM_xxx macros.
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

void	bsp_getOutboundBspItem(int itemNeeded, Object bspBuf,
		unsigned int bspBufLen, Address *val, unsigned int *len)
{
	unsigned char	*temp;
	unsigned char	*cursor;
	unsigned char	itemType;
	int		sdnvLength;
	uvast		longNumber;
	unsigned int	itemLength;
	unsigned int	offset;

	CHKVOID(bspBuf);
	CHKVOID(val);
	CHKVOID(len);
	*val = 0;			/*	Default.		*/
	*len = 0;			/*	Default.		*/

	/*	Walk through all items in the buffer (either a
	 *	security parameters field or a security results
	 *	field), searching for an item of the indicated type.	*/

	temp = MTAKE(bspBufLen);
	if (temp == NULL)
	{
		putErrmsg("No space for temporary memory buffer.",
				utoa(bspBufLen));
		return;
	}

	memset(temp, 0, bspBufLen);
	sdr_read(getIonsdr(), (char *) temp, bspBuf, bspBufLen);
	cursor = temp;
	while (bspBufLen > 0)
	{
		itemType = *cursor;
		cursor++;
		bspBufLen--;
		if (bspBufLen == 0)	/*	No item length.		*/
		{
			break;		/*	Malformed result data.	*/
		}

		sdnvLength = decodeSdnv(&longNumber, cursor);
		if (sdnvLength == 0 || sdnvLength > bspBufLen)
		{
			break;		/*	Malformed result data.	*/
		}

		itemLength = longNumber;
		cursor += sdnvLength;
		bspBufLen -= sdnvLength;

		if (itemLength == 0)	/*	Empty item.		*/
		{
			continue;
		}

		if (itemType == itemNeeded)
		{
			offset = cursor - temp;
			*val = (Address) (bspBuf + offset);
			*len = itemLength;
			break;
		}

		/*	Look at next item in buffer.			*/

		cursor += itemLength;
		bspBufLen -= itemLength;
	}

	MRELEASE(temp);
}

/******************************************************************************
 *
 * \par Function Name: bsp_findBspBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in
 * 		 a bundle looking for a BSP block of the indicated type
 * 		 and ordinality whose target is the block of indicated
 * 		 type and ordinality.
 *
 * \retval Object
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of BSP block to look for.
 * \param[in]  targetBlockType		Identifies target of the BSP block.
 * \param[in]  targetBlockOccurrence		"
 * \param[in]  instance			The BSP block instance to look for.
 *
 * \par Notes: 
 *****************************************************************************/

Object	bsp_findBspBlock(Bundle *bundle, unsigned char type,
		unsigned char targetBlockType,
		unsigned char targetBlockOccurrence,
		unsigned char instance)
{
	Sdr	sdr = getIonsdr();
	int	idx;
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);
		OBJ_POINTER(BspOutboundBlock, asb);

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

			GET_OBJ_POINTER(sdr, BspOutboundBlock, asb,
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
 * \par Function Name: bsp_findAcqBspBlock
 *
 * \par Purpose: This function searches the lists of extension blocks in
 * 		 an acquisition work area looking for a BSP block of the
 * 		 indicated type and ordinality whose target is the block
 * 		 of indicated type and ordinality.
 *
 * \retval Object
 *
 * \param[in]  bundle      		The bundle within which to search.
 * \param[in]  type        		The type of BSP block to look for.
 * \param[in]  targetBlockType		Identifies target of the BSP block.
 * \param[in]  targetBlockOccurrence		"
 * \param[in]  instance			The BSP block instance to look for.
 *
 * \par Notes: 
 *****************************************************************************/

LystElt	bsp_findAcqBspBlock(AcqWorkArea *wk, unsigned char type,
		unsigned char targetBlockType,
		unsigned char targetBlockOccurrence,
		unsigned char instance)
{
	int		idx;
	LystElt		elt;
	AcqExtBlock	*blk;
	BspInboundBlock	*asb;

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

			asb = (BspInboundBlock *) (blk->object);
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

char	*getLocalAdminEid(char *peerEid)
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

static int	getInboundSecuritySource(AcqExtBlock *blk, char *dictionary,
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
	securitySource.d.schemeNameOffset = (uaddr) lyst_data(elt1);
	securitySource.d.nssOffset = (uaddr) lyst_data(elt2);
	if (printEid(&securitySource, dictionary, fromEid) < 0)
	{
		return -1;
	}

	return 0;
}

int	bsp_getInboundSecurityEids(Bundle *bundle, AcqExtBlock *blk,
		BspInboundBlock *asb, char **fromEid, char **toEid)
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
		return -1;
	}

	if (printEid(&(bundle->destination), dictionary, toEid) < 0)
	{
		if (dictionary)
		{
			releaseDictionary(dictionary);
		}

		return -1;
	}

	if (asb->ciphersuiteFlags & BSP_ASB_SEC_SRC)
	{
		result = getInboundSecuritySource(blk, dictionary, fromEid);
	}
	else
	{
		result = printEid(&bundle->id.source, dictionary, fromEid);
	}

	if (dictionary)
	{
		releaseDictionary(dictionary);
	}

	return result;
}

static int	getOutboundSecuritySource(ExtensionBlock *blk, char *dictionary,
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

	return 0;
}

int	bsp_getOutboundSecurityEids(Bundle *bundle, ExtensionBlock *blk,
		BspOutboundBlock *asb, char **fromEid, char **toEid)
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
		return -1;
	}

	if (printEid(&(bundle->destination), dictionary, toEid) < 0)
	{
		if (dictionary)
		{
			releaseDictionary(dictionary);
		}

		return -1;
	}

	if (asb->ciphersuiteFlags & BSP_ASB_SEC_SRC)
	{
		result = getOutboundSecuritySource(blk, dictionary, fromEid);
	}
	else
	{
		result = printEid(&bundle->id.source, dictionary, fromEid);
	}

	if (dictionary)
	{
		releaseDictionary(dictionary);
	}

	return result;
}

int	bsp_destinationIsLocal(Bundle *bundle)
{
	char		*dictionary;
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	int		result = 0;

	dictionary = retrieveDictionary(bundle);
	if (dictionary == (char *) bundle)
	{
		putErrmsg("Can't retrieve dictionary.", NULL);
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

char	*bsp_getLocalAdminEid(char *eid)
{
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;

	/*
	 * Look at EID scheme we are working in: that will be the
	 * scheme of our local admin EID, as we don't cross schemes
	 * in transmit.
	 */

	if (parseEidString(eid, &metaEid, &vscheme, &vschemeElt) == 0)
	{
		/*	Can't know which admin EID to use.		*/

		return NULL;
	}

	restoreEidString(&metaEid);
   	return vscheme->adminEid;
}

int	bsp_securityPolicyViolated(AcqWorkArea *wk)
{
	/*	TODO: eventually this function should do something like:
	 *		1.  For each block in the bundle, find matching
	 *		    BIB rule.  If rule found, find BIB for this
	 *		    block.  If BIB not found, return 1.		*/

	return 0;
}
