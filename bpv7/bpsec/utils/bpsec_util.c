/******************************************************************************
#include <sc_util.h>
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

//TODO: update documentation
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
 **                                 [Secure DTN implementation (NASA: NNX14CS58P)]
 **  09/02/19  S. Burleigh          Rename everything for bpsec
 **  10/20/20  S. Burleigh          Major update for BPv7
 **
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_util.h"
#include "sc_value.h"
#include "sci_valmap.h"

//#include "../sci/sci_valmap.h"

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

// TODO Document function
/*
 * *         TODO: This method is from original code, but here
	 *               must be a better way to copy an EID...
 */
int bpsec_util_EIDCopy(EndpointId *toEID, EndpointId *fromEID)
{
	char			*eidBuf;
	MetaEid			meid;
	VScheme			*vscheme;
	PsmAddress		schemeElt;
	int result = 0;

	CHKERR(toEID);
	CHKERR(fromEID);

	readEid(fromEID, &eidBuf);

	if(parseEidString(eidBuf, &meid, &vscheme, &schemeElt) <= 0)
	{
		MRELEASE(eidBuf);
		return -1;
	}

	result = writeEid(toEID, &meid);
	restoreEidString(&meid);
	MRELEASE(eidBuf);
	if (result < 0)
	{
		return -1;
	}

	return 1;
}

int bpsec_util_eidIsLocalCheck(EndpointId eid)
{
    VScheme     *vscheme;
    VEndpoint   *vpoint;
    int     result = 0;

    lookUpEidScheme(&(eid), &vscheme);
    if (vscheme)    /*  EID scheme is known on this node.   */
    {
        lookUpEndpoint(&(eid), vscheme, &vpoint);
        if (vpoint) /*  Node is registered in endpoint. */
        {
            result = 1;
        }
    }

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

int	bpsec_util_destIsLocalCheck(Bundle *bundle)
{
    CHKZERO(bundle);
    return bpsec_util_eidIsLocalCheck(bundle->destination);
}

/******************************************************************************
 *
 * \par Function Name: bpsec_util_localAdminEIDGet
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

char	*bpsec_util_localAdminEIDGet(char *peerEid)
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
 * \par Function Name: bpsec_util_zcoFileSourceTransferTo
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

int	bpsec_util_zcoFileSourceTransferTo(Sdr sdr, Object *resultZco,
		Object *acqFileRef, char *fname, char *bytes, uvast length)
{
	static uint32_t	acqCount = 0;
	char		cwd[200];
	char		fileName[SDRSTRING_BUFSZ];
	int		fd;
	vast		fileLength;

	CHKERR(bytes);

	BPSEC_DEBUG_PROC("+bpsec_util_zcoFileSourceTransferTo(sdr, 0x"
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
			BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: \
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
			BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: \
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
			BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: \
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
			BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: \
Can't reopen acq file %s.", fileName);
			sdr_cancel_xn(sdr);
			return 0;
		}

		if ((fileLength = lseek(fd, 0, SEEK_END)) < 0)
		{
			BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: \
Can't get acq file length %s.", fileName);
			sdr_cancel_xn(sdr);
			close(fd);
			return 0;
		}
	}

	/*	Write the data to the file.				*/

	if (write(fd, bytes, length) < 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: Can't append \
to acq file %s.", fileName);
		sdr_cancel_xn(sdr);
		close(fd);
		return 0;
	}

	close(fd);
	if (zco_append_extent(sdr, *resultZco, ZcoFileSource, *acqFileRef,
					      fileLength, length) <= 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: Can't append \
extent to ZCO.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*      Flag file reference for deletion as soon as the last
	 *      ZCO extent that references it is deleted.               */

	zco_destroy_file_ref(sdr, *acqFileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		BPSEC_DEBUG_ERR("x bpsec_util_zcoFileSourceTransferTo: Can't \
acquire extent into file..", NULL);
		return -1;
	}

	return 1;
}

/*****************************************************************************
 *            CANONICALIZATION FUNCTIONS (for BIB processing)                *
 *****************************************************************************/
// TODO Document function
// TODO: Can we move this into BEI.h/BEI.c? Or LibbpP.c? Does ION already canonicalize blocks??

unsigned char *bpsec_util_primaryBlkSerialize(Bundle *bundle, int *length)
{
    unsigned char dstEid[300];
    int           dstEidLength;
    unsigned char srcEid[300];
    int           srcEidLength;
    unsigned char rptToEid[300];
    int           rptToEidLength;
    int           maxBlockLength;
    unsigned char *buffer = NULL;
    unsigned char *cursor = NULL;

    CHKNULL(bundle);
    CHKNULL(length);

    *length = 0;

    /*  Can now compute max primary block length: EID lengths
     *  plus 50 for remainder of primary block.         */

    CHKNULL((dstEidLength = serializeEid(&(bundle->destination), dstEid))   > 0);
    CHKNULL((srcEidLength = serializeEid(&(bundle->id.source),   srcEid))   > 0);
    CHKNULL((rptToEidLength = serializeEid(&(bundle->reportTo),  rptToEid)) > 0);

    maxBlockLength = dstEidLength + srcEidLength + rptToEidLength + 50;

    if((buffer = MTAKE(maxBlockLength)) == NULL)
    {
        BPSEC_DEBUG_ERR("Can't allocate %d bytes.", maxBlockLength);
        return NULL;
    }

    cursor = buffer;
    serializePrimaryBlock(bundle, &cursor, dstEid, dstEidLength, srcEid, srcEidLength,  rptToEid, rptToEidLength);

    *length = (cursor - buffer);
    return buffer;
}

static int	canonicalizePrimaryBlock(Bundle *bundle, Object *zcoOut)
{
	Sdr		sdr = getIonsdr();
	Object  blkBytes = 0;
	Object  zco = 0;
	int     blkLength = 0;
	unsigned char *buffer = NULL;

	*zcoOut = 0;

	/*	Wrap canonicalized block data in a ZCO for processing.	*/

	if((buffer = bpsec_util_primaryBlkSerialize(bundle, &blkLength)) == NULL)
	{
        putErrmsg("Can't serialize primary block.", NULL);
        return -1;
	}

	blkBytes = sdr_malloc(sdr, blkLength);
	if (blkBytes == 0)
	{
		MRELEASE(buffer);
		putErrmsg("Can't serialize primary block.", NULL);
		return -1;
	}

	sdr_write(sdr, blkBytes, (char *) buffer, blkLength);
	MRELEASE(buffer);
	zco = zco_create(sdr, ZcoSdrSource, blkBytes, 0, 0 - blkLength,
			ZcoOutbound);
	switch (zco)
	{
	case ((Object) ERROR):
		return -1;

	case 0:
		putErrmsg("ZCO failure in primary block canonicalization.",
				NULL);
		return 0;	/*	No canonicalization.		*/

	default:
		*zcoOut = zco;
		return blkLength;
	}
}

// TODO Document function
// TODO: Can we move this into BEI.h/BEI.c? Or LibbpP.c? Does ION already canonicalize blocks??
static int	canonicalizePayloadBlock(Bundle *bundle, Object *zcoOut)
{
	Sdr	sdr = getIonsdr();
	Payload	payload;

	/*	Note: to canonicalize the bundle's payload block,
	 *	we first create a ZCO that is a copy of the payload
	 *	content.  We use that ZCO as the content of a
	 *	temporary Payload structure.  We pass that structure
	 *	to the serializePayloadBlock function along with a
	 *	blkProcFlags value of 0 (required for canonicalization).
	 *	That function creates the serialized payload block
	 *	header, prepends the header to the content object,
	 *	computes the applicable CRC, and appends the CRC to
	 *	the content object.  We then simply pass that
	 *	content object back as the canonicalization of the
	 *	payload block.						*/

	*zcoOut = 0;
	payload.length = bundle->payload.length;
	payload.crcType = bundle->payload.crcType;
	payload.content = zco_clone(sdr, bundle->payload.content, 0,
			payload.length);
	switch (payload.content)
	{
	case ((Object) ERROR):
		return -1;

	case 0:
		putErrmsg("ZCO failure in payload canonicalization.", NULL);
		return 0;	/*	No canonicalization.		*/

	default:
		break;
	}

	if (serializePayloadBlock(&payload, 0) < 0)
	{
		putErrmsg("Failed serializing payload.", NULL);
		return -1;
	}

	*zcoOut = payload.content;
	return zco_length(sdr, payload.content);
}


// TODO Document function
// TODO: Can we move this into BEI.h/BEI.c? Or LibbpP.c? Does ION already canonicalize blocks??

static int	canonicalizeExtensionBlock(Bundle *bundle, uint8_t blkNbr,
			Object *zcoOut)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		blkObj;
	ExtensionBlock	blk;
	ExtensionDef	*def;
	Object		zco;

	/*	Note: to canonicalize this block we read the block
	 *	into a buffer, tweak the buffer, and call the
	 *	serialization function for outbound extension blocks
	 *	of the indicated type.  This function constructs a
	 *	serialized extension block, writes that serialized
	 *	block to the SDR heap, and stores the address of
	 *	that SDR heap object in the "bytes" field of our
	 *	extension block buffer, but the extension block
	 *	itself is not modified.  So we need not repair any
	 *	modification to the extension block, since no such
	 *	modification was recorded; we merely wrap the
	 *	results of serialization (in the buffer's "bytes"
	 *	field) in a temporary ZCO and return that ZCO.		*/

	*zcoOut = 0;
	elt = getExtensionBlock(bundle, blkNbr);
	if (elt == 0)
	{
		return 0;	/*	Not found, can't canonicalize.	*/
	}

	blkObj = sdr_list_data(sdr, elt);
	sdr_read(sdr, (char *) &blk, blkObj, sizeof(ExtensionBlock));

	/*	Generate canonicalized form of block.			*/

	blk.blkProcFlags = 0;	/*	Required for canonicalization.	*/
	blk.bytes = 0;		/*	Ignore previous serialization.	*/
	blk.length = 0;		/*	Ignore previous serialization.	*/
	def = findExtensionDef(blk.type);

	if (def && def->serialize)
	{
		if (def->serialize(&blk, bundle) < 0)
		{
			putErrmsg("Can't serialize extension block.", NULL);
			return -1;
		}

		/*	Result of extension block serialization is
		 *	written to SDR heap; pointer to that object
		 *	is placed in blk->bytes, and its length is
		 *	placed in blk->length.				*/
	}

	if (blk.bytes == 0)
	{
		return 0;	/*	No canonicalization.		*/
	}

	/*	Wrap canonicalized block data in a ZCO for processing.	*/

	zco = zco_create(sdr, ZcoSdrSource, blk.bytes, 0, 0 - blk.length, ZcoOutbound);
	switch (*zcoOut)
	{
	case ((Object) ERROR):
		return -1;

	case 0:
		putErrmsg("ZCO failure in extension block canonicalization.",
				NULL);
		return 0;	/*	No canonicalization.		*/

	default:
		*zcoOut = zco;
		return blk.length;
	}
}


int bpsec_util_acqBlkDataAsZco(AcqExtBlock *blk, Object *zco)
{
    Sdr    sdr = getIonsdr();
    Object bytesObj = 0;
    int    offset = 0;

    CHKERR(blk);
    CHKERR(zco);

    offset = blk->length - blk->dataLength;

    bytesObj = sdr_malloc(sdr, blk->dataLength);

    CHKERR(bytesObj);
    sdr_write(sdr, bytesObj, (char *) (blk->bytes + offset), blk->dataLength);

    if((*zco = zco_create(sdr, ZcoSdrSource, bytesObj, 0, 0 - blk->dataLength,  ZcoInbound)) <= 0)
    {
        BPSEC_DEBUG_ERR("Cannot make ZCO out of block data.", NULL);
        sdr_free(sdr, bytesObj);
        return ERROR;
    }

    return blk->dataLength;
}

// TODO Document function
// TODO: Can we move this into BEI.h/BEI.c? Or LibbpP.c? Does ION already canonicalize blocks??

static int	canonicalizeAcqExtensionBlock(AcqWorkArea *work, uint8_t blkNbr,
			Object *zcoOut)
{
	Sdr		sdr = getIonsdr();
	LystElt		elt;
	AcqExtBlock	*blk;
	Object		bytesObj;
	Object		zco;

	/*	Note: to canonicalize this extension block we first
	 *	find the block within the acquisition work area.  The
	 *	"bytes" array of the block contains precisely the
	 *	canonicalized form of the block that we are looking
	 *	for: the only difference between the received
	 *	serialized block and the canonicalized block is
	 *	that blkProcFlags must be set to zero in the latter,
	 *	and that value within the received serialized block
	 *	was changed to zero in the course of acquiring the
	 *	block.  So now all we need to do is wrap the block's
	 *	(preemptively tweaked) "bytes" array in a temporary
	 *	ZCO and return that ZCO.				*/

	*zcoOut = 0;
	elt = getAcqExtensionBlock(work, blkNbr);
	if (elt == NULL)
	{
		return 0;	/*	Not found, can't verify.	*/
	}

	blk = (AcqExtBlock *) lyst_data(elt);
	CHKERR(blk);

	/*	Wrap canonicalized block data in a ZCO for processing.	*/

	bytesObj = sdr_malloc(sdr, blk->length);
	CHKERR(bytesObj);
	sdr_write(sdr, bytesObj, (char *) (blk->bytes), blk->length);
	zco = zco_create(sdr, ZcoSdrSource, bytesObj, 0, 0 - blk->length,
			ZcoInbound);

	switch (zco)
	{
	case ((Object) ERROR):
		sdr_free(sdr, bytesObj);
		return -1;

	case 0:
		sdr_free(sdr, bytesObj);
		putErrmsg("ZCO failure in extension block canonicalization.",
				NULL);
		return 0;	/*	No canonicalization.		*/

	default:
		*zcoOut = zco;
		return blk->length;
	}
}


// TODO Update function documentation
// TODO: Can we move this into BEI.h/BEI.c? Or LibbpP.c? Does ION already canonicalize blocks??

/******************************************************************************
 *
 * \par Function Name: bpsec_util_canonicalizeOut
 *
 * \par Purpose: This utility function generates a zero copy object
 * 		 instantiating the canonicalized form of an outbound
 * 		 BP block (primary, payload, or extension) for the
 * 		 purpose of BIB signature computation.  The ZCO MUST
 * 		 be destroyed after it has been used for signing the
 * 		 block.
 *
 * \par Date Written:  8/15/11
 *
 * \retval -1 on Fatal Error
 *          0 on Failure
 *          length of canonicalized block on Success
 *
 * \param[in]  bundle     The bundle in which the block resides.
 * \param]in]  blkNbr     The block number of the block to canonicalize.
 * \param[out] zcoOut     Where to deliver the canonicalized block.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  10/06/20  S. Burleigh   Initial implementation
 *****************************************************************************/

int	bpsec_util_canonicalizeOut(Bundle *bundle, uint8_t blkNbr, Object *zcoOut)
{
	Sdr	sdr = getIonsdr();
	int	result;

	CHKERR(bundle);
	CHKERR(zcoOut);
	CHKERR(sdr_begin_xn(sdr));
	switch (blkNbr)
	{
	case 0:
		result = canonicalizePrimaryBlock(bundle, zcoOut);
		break;

	case 1:
		result = canonicalizePayloadBlock(bundle, zcoOut);
		break;

	default:
		result = canonicalizeExtensionBlock(bundle, blkNbr, zcoOut);
	}

	if (result < 0)
	{
		sdr_cancel_xn(sdr);
	}
	else
	{
		if (sdr_end_xn(sdr) < 0)
		{
			result = -1;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bpsec_util_canonicalizeIn
 *
 * \par Purpose: This utility function generates a zero copy object
 * 		 instantiating the canonicalized form of an inbound
 * 		 BP block (primary, payload, or extension) for the
 * 		 purpose of BIB signature computation.  The ZCO MUST
 * 		 be destroyed after it has been used for verifying
 * 		 the block.
 *
 * \par Date Written:  8/15/11
 *
 * \retval -1 on Fatal Error
 *          0 on Failure
 *          length of canonicalized block on Success
 *
 * \param[in]  bundle     The bundle in which the block resides.
 * \param]in]  blkNbr     The block number of the block to canonicalize.
 * \param[out] zcoOut     Where to deliver the canonicalized block.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  10/06/20  S. Burleigh   Initial implementation
 *****************************************************************************/

int	bpsec_util_canonicalizeIn(AcqWorkArea *work, uint8_t blkNbr, Object *zcoOut)
{
	Sdr	sdr = getIonsdr();
	int	result;

	CHKERR(work);
	CHKERR(zcoOut);
	CHKERR(sdr_begin_xn(sdr));
	switch (blkNbr)
	{
	case 0:
		result = canonicalizePrimaryBlock(&(work->bundle), zcoOut);
		break;

	case 1:
		result = canonicalizePayloadBlock(&(work->bundle), zcoOut);
		break;

	default:
		result = canonicalizeAcqExtensionBlock(work, blkNbr, zcoOut);
	}

	if (result < 0)
	{
		sdr_cancel_xn(sdr);
	}
	else
	{
		if (sdr_end_xn(sdr) < 0)
		{
			result = -1;
		}
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: bsl_findOutboundBpsecBlock
 *
 * \par Purpose: This function returns the security block of indicated type
 *               that targets the provided block (identified by block number)
 *               if such a security block exists.
 *
 * \param[in]  bundle     Current, working bundle.
 * \param[in]  tgtBlkNum  Block number of the security target block.
 * \param[in]  sopType    Block type of the security block to find.
 *
 * \Note This function has been adapted from S. Burleigh's
 *        findOutboundTarget functions in bib.c and bcb.c
 *****************************************************************************/
Object bspsec_util_findOutboundBpsecTargetBlock(Bundle *bundle, int tgtBlkNum, BpBlockType sopType)
{
    /* Step 0: Sanity checks. */
    CHKERR(bundle);
    CHKERR(tgtBlkNum);
    CHKERR(sopType);

    Sdr                 sdr = getIonsdr();
    Object              elt;
    Object              blockObj;
    ExtensionBlock      block;
    BpsecOutboundASB    asb;
    Object              elt2;
    Object              targetObj;
    BpsecOutboundTargetResult   target;

    /*
     * Step 1: Check each extension block in the bundle, looking for a
     * bpsec block whose type (BIB or BCB) matches the provided sopType.
     */
    for (elt = sdr_list_first(sdr, bundle->extensions); elt;
            elt = sdr_list_next(sdr, elt))
    {
        blockObj = sdr_list_data(sdr, elt);
        sdr_read(sdr, (char *) &block, blockObj,
                sizeof(ExtensionBlock));
        if (block.type != sopType)
        {
            continue;
        }

        sdr_read(sdr, (char *) &asb, block.object,
                sizeof(BpsecOutboundASB));

        /*
         * Step 2: Check the targets of the bpsec block, looking for a
         * match to the tgtBlkNum provided.
         */
        for (elt2 = sdr_list_first(sdr, asb.scResults); elt2;
                elt2 = sdr_list_next(sdr, elt2))
        {
            targetObj = sdr_list_data(sdr, elt2);
            sdr_read(sdr, (char *) &target, targetObj,
                    sizeof(BpsecOutboundTargetResult));
            if (target.scTargetId == tgtBlkNum)
            {
                return elt; /* bpsec block with target found */
            }
        }
    }

    return 0;   /* bpsec block with specified target not found */
}

// TODO Document function

/*
 * 0 == disallowed.
 * 1 == allowed.
 */
int bpsec_util_checkSop(BpBlockType target, BpBlockType sec)
{
    /* No one can ever target a BCB. */
    if(target == BlockConfidentialityBlk)
    {
        return 0;
    }

    /* Cannot encrypt primary block. */
    if((sec == BlockConfidentialityBlk) && (target == PrimaryBlk))
    {
        return 0;
    }

    /* Security blocks cannot target their own types. */
    else if(sec == target)
    {
        return 0;
    }

    return 1;
}

/******************************************************************************
 * @brief Creates a placeholder security block in a bundle.
 *
 * This function allocates space for a security block in a given bundle, and
 * populates non-target-related fields associated with it.  The list of
 * targets and associated security results will be added after security
 * block creation and ultimately serialized when the bundle holding this
 * block is ready for transmission.
 *
 *
 * @param[in/out] bundle The bundle holding this BIB.
 * @param[in]     type   The type of block (BIB or BCB)
 * @param[in]     def    The Security Context populated this block.
 * @param[in]     parms  SC parms that apply to this block instance.
 *
 * @note
 *  - The block is added to the SDR.
 *
 * @retval 0  - The block could not be added to the bundle.
 * @retval !0 - The block that was added (and stored in the SDR).
 *****************************************************************************/

Object bpsec_util_OutboundBlockCreate(Bundle *bundle, BpBlockType type, sc_Def *def, PsmAddress parms)
{
    Sdr sdr = getIonsdr();
    PsmPartition wm = getIonwm();
    ExtensionBlock blk;
    BpsecOutboundASB asb;
    Object result = 0;


    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC",%d",
                     (uaddr) bundle, type, (uaddr)def, parms);

    /* Step 0: Sanity Checks. */
    CHKZERO(bundle);
    CHKZERO(def);

    /* Step 1: Initialize the extension block. . */
    memset((char*) &blk, 0, sizeof(ExtensionBlock));
    blk.type = type;
    blk.tag = 0;
    blk.crcType = NoCRC;
    blk.size = sizeof(BpsecOutboundASB);
    if ((blk.object = sdr_malloc(sdr, blk.size)) == 0)
    {
        BPSEC_DEBUG_ERR("Cannot SDR allocate %d bytes", blk.size);
        return 0;
    }

    /*
     * Step 2: Initialize the ASB comprising the block-type-specific
     *         portion of the extension block.
     */
    if(def->scInitOutboundASB(def, bundle, &asb, sdr, wm, parms) < 1)
    {
        BPSEC_DEBUG_ERR("Failed to initialize BIB ASB.", NULL);
        bpsec_asb_outboundAsbDelete(sdr, &asb);
        sdr_free(sdr, blk.object);
        return 0;
    }

    /* Step 3: Write the block to the SDR. */
    sdr_write(sdr, blk.object, (char* ) &asb, blk.size);

    /* Step 4: Attach the block to the list of blocks for the given bundle. */
    if((result = attachExtensionBlock(type, &blk, bundle)) == 0)
    {
    	BPSEC_DEBUG_ERR("Cannot SDR allocate %d bytes", blk.size);
        sdr_free(sdr, blk.object);
        bpsec_asb_outboundAsbDelete(sdr, &asb);
    }


    return result;
}


/******************************************************************************
 * @brief Populate the security results of a security block.
 *
 *
 * @param[in]  bundle    The bundle holding all the blocks.
 * @param[out] secBlk    Existing security block being applied
 * @param[out] secAsb    ASB for the security block
 *
 * @note
 *   1. The blkAsb MUST be pre-allocated and of the correct size to hold
 *      the created BIB ASB.
 *   2. The passed-in asb MUST be pre-initialized with both the target
 *      block type and the security source.
 *   3. The bibBlk MUST be pre-allocated and initialized with a size,
 *      a target block type, and the object within the block MUST be
 *      allocated in the SDR.
 *
 * @retval -1 - System error
 * @retval  0 - Nothing to do
 * @retval  1 - Results computed for the block.
 *****************************************************************************/
int bpsec_util_generateSecurityResults(Bundle *bundle, char *fromEid, ExtensionBlock *secBlk, BpsecOutboundASB *secAsb, sc_action action)
{
    Sdr sdr = getIonsdr();
    int8_t result = 1;


    Object elt = 0;
    Object targetObj = 0;
    BpsecOutboundTargetResult tgtResult;
    sc_Def def;
    sc_state state;
    PsmPartition wm = getIonwm();

    LystElt sopResultElt = NULL;
    sc_value *sopResult = NULL;

    int numTgts = 0;

    BPSEC_DEBUG_PROC("(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC "," ADDR_FIELDSPEC ")",
                     (uaddr ) bundle, (uaddr ) secBlk, (uaddr ) secAsb);

    /* Step 0 - Sanity checks. */
    CHKERR(bundle);
    CHKERR(secBlk);
    CHKERR(secAsb);
    CHKERR(fromEid);


    /*
     * Step 1: Make sure there are security targets associated with this security
     *         block. The actual results should be empty, but the target block ID
     *         for each needs to be populated.
     *
     *         If this list is empty, it is not necessarily a processing error
     *         but maybe a misconfiguration?
     *
     *         TODO: Sarah Heiner - is this something that requires a sop_event?
     *
     */
    if ((numTgts = sdr_list_length(sdr, secAsb->scResults)) == 0)
    {
        result = 0;
        BPSEC_DEBUG_WARN("BIB block %d has no targets", secBlk->number);
        scratchExtensionBlock(secBlk);
        BPSEC_DEBUG_PROC("--> %d", result);
        return result;
    }

    BPSEC_DEBUG_INFO("Block has %d targets.", numTgts);
    /*
     * Step 2 - Grab the security context that will generate the results for
     *          the security operations for this BIB block.
     */
    if(bpsec_sci_defFind(secAsb->scId, &def) < 1)
    {
        BPSEC_DEBUG_ERR("SCI %d not supported.", secAsb->scId);
        result = -1;
        bundle->corrupt = 1;
        scratchExtensionBlock(secBlk);
        BPSEC_DEBUG_PROC("--> %d", result);
        return result;
    }

    /*
     * Step 3 - Initialize the security context state. We will be using
     *          it to process every security operation in the bundle.
     */
    Lyst blkParms = bpsec_scv_sdrListRead(sdr, secAsb->scParms);
    Lyst extraParms = lyst_create(); // TODO what is this call fails?

    lyst_delete_set(extraParms, bpsec_scv_lystCbDel, NULL);

    def.scStateInit(wm, &state, secBlk->number, &def, SC_ROLE_SOURCE, action, secAsb->scSource, 0, blkParms, numTgts);


    /* Step 4 - For each target in the security block... */
    for(elt = sdr_list_first(sdr, secAsb->scResults); elt; elt = sdr_list_next(sdr, elt))
    {
        /* Step 4.1 - extract the target ID */

        // TODO - check return codes.
        targetObj = sdr_list_data(sdr, elt);
        sdr_read(sdr, (char *) &tgtResult, targetObj, sizeof(BpsecOutboundTargetResult));

        /* Step 4.2 - Calculate the security result. */
        // TODO - check return codes.

        if(def.scProcOutBlk(&state, extraParms, bundle, secAsb, &tgtResult) < 1)
        {
        	BPSEC_DEBUG_ERR("Failed processing target number %d", tgtResult.scTargetId);
        	result = 0;
        }


        /* Step 4.3 - Write the result back to the SDR. */
        for(sopResultElt = lyst_first(state.scStResults); sopResultElt; sopResultElt = lyst_next(sopResultElt))
        {
            sopResult = lyst_data(sopResultElt);
            bpsec_scv_memSdrListAppend(sdr, tgtResult.scIndTargetResults, sopResult);
        }

        if(def.scStateIncr(&state) < 1)
        {
            BPSEC_DEBUG_ERR("Unable to advance SC state.", NULL);
            // TODO: Panic.
        }
    }

    bpsec_scv_memListRecord(sdr, secAsb->scParms, extraParms);

    /* Step 5: Clean up any remaining state. */
    def.scStateClear(&state);
    lyst_destroy(blkParms);
    lyst_destroy(extraParms);

    BPSEC_DEBUG_PROC("--> %d", result);
    return result;
}



/******************************************************************************
 * @brief Confirms that a security block can use a target.
 *
 * This function determines that a block in a bundle can be the correct
 * recipient of a given security operation. There are a few cases when the
 * target CANNOT be considered a correct recipient of security:
 *
 * 1. If the target does not exist in the bundle.
 * 2. If the target is already the target of an existing BCB.
 * 3. If the target is already the target of a BIB and the requested operation
 *    is also a BIB.
 * 4. If the target is a BCB.
 * 5. If the target is a BIB and the requested operation is also a BIB.
 *
 * If the target is appropriate, this function ALSO returns:
 * - The BIB security block targeting the target, if the sopType is BCB and
 *   the target is already signed by a BIB.
 *
 * @param[in]  bundle    The bundle holding the target block.
 * @param[in]  def       The Security Context definition used to populate the block.
 * @parms[in]  parms     Policy parameters used to condifure this secuirty block.
 * @param[in]  sopType   The security operation being requested.
 * @param[in]  tgtBlkNum The block being targeted.
 * @param[out] bibBlk    Existing BIB that holds a signature over this target.
 * @param[out] secBlk    The existing secBlk to use for this target.
 *
 * @note
 *  - All of these concerns are together because checking these items involves
 *    iterating through the bundle's list of extension blocks, which is a
 *    time-intensive procedure. So we do not want to do that multiple times
 *    if we can avoid it.
 *  - If bibBlk is not 0 then the sopType is BCB and the target has a BIB on it
 *    such that the BCB must encrypt both the bibBlk and the target block.
 *  - If the secBlk is not 0 then it points to a security block that can be
 *    re-used to hold the target without creating a new security block.
 *
 * @retval -1 - System error
 * @retval  0 - The target may not be added to a security block.
 * @retval  1 - The target may be added to a security block.
 *****************************************************************************/

int bpsec_util_checkOutboundSopTarget(Bundle *bundle, sc_Def *def, PsmPartition wm, PsmAddress parms,
                                      BpBlockType sopType, int tgtBlkNum, Object *bibBlk, Object *secBlk)
{
    Sdr                 sdr = getIonsdr();
    Object              elt;
    Object              blockObj;
    ExtensionBlock      block;
    BpsecOutboundASB    asb;
    Object              elt2;
    Object              targetObj;
    BpsecOutboundTargetResult   target;

    int targetFound = 0;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC",wm,%d,%d,%d,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                     (uaddr) bundle, (uaddr) def, parms, sopType, tgtBlkNum, (uaddr)bibBlk, (uaddr) secBlk);

    /* Step 0: Sanity checks. */
    CHKERR(bundle);
    CHKERR(def);

    /*
     * Step 1: If the tgtBlockNum is not an extension block, then we
     *         need to do a quick sop check before looping through
     *         extension blocks.
     */
    if((tgtBlkNum == PrimaryBlk) || (tgtBlkNum == PayloadBlk))
    {
        if(bpsec_util_checkSop(tgtBlkNum, sopType) < 1)
        {
            BPSEC_DEBUG_ERR("Security block type %d cannot target a block of type %d.", sopType, tgtBlkNum);
            return 0;
        }
        targetFound = 1;
    }

    /* Step 2: Loop through known extension blocks in the bundle. */
    for (elt = sdr_list_first(sdr, bundle->extensions); elt; elt = sdr_list_next(sdr, elt))
    {
        /* Step 2.1: Read the block from the SDR. */
        blockObj = sdr_list_data(sdr, elt);
        sdr_read(sdr, (char *) &block, blockObj, sizeof(ExtensionBlock));

        BPSEC_DEBUG_INFO("Checking block type %d.", block.type);

        /*
         * Step 2.2: If this block is the target block, note that the target block
         *           exists in this bundle.
         */
        if(block.number == tgtBlkNum)
        {
            targetFound = 1;
            if(bpsec_util_checkSop(block.type, sopType) < 1)
            {
                BPSEC_DEBUG_ERR("Security block type %d cannot target a block of type %d.", sopType, block.type);
                return 0;
            }
        }

        /* Step 2.3: Skip non security blocks. */
        if ((block.type != BlockIntegrityBlk) &&
           (block.type != BlockConfidentialityBlk))
        {
            continue;   /*  Not a BPSec block.  */
        }


        /*
         * Step 2.4: This is a security block! We need to check a few things:
         *           1. Does this security block have our target block as a target?
         *              1a. If it is a BIB and sopType is a BCB, that's OK.
         *              1b. In any other case, it isn't OK.
         *           2. Can we use this security block to hold the target?
         */

        sdr_read(sdr, (char *) &asb, block.object, sizeof(BpsecOutboundASB));

        BPSEC_DEBUG_INFO("Security block has %d results.", sdr_list_length(sdr, asb.scResults));

        /*
         * Step 2.5: Check the targets of this security block to be sure that we
         *           don't have any disallowed re-targeting of our tgtBlkNum.
         */
        for (elt2 = sdr_list_first(sdr, asb.scResults); elt2; elt2 = sdr_list_next(sdr, elt2))
        {

            targetObj = sdr_list_data(sdr, elt2);
            sdr_read(sdr, (char *) &target, targetObj, sizeof(BpsecOutboundTargetResult));

            BPSEC_DEBUG_INFO("Security result target is %d.", target.scTargetId);

            /*
             * Step 2.5.1 - Our target is also the target of this security block.
             *              This is ONLY ok if the current block is a BIB and we
             *              are applying a BCB. Otherwise, this is a problem.
             */
            if (target.scTargetId == tgtBlkNum)
            {
                if((sopType == BlockConfidentialityBlk) && (block.type == BlockIntegrityBlk))
                {
                    if(bibBlk)
                    {
                        *bibBlk = blockObj;
                    }
                    else
                    {
                        BPSEC_DEBUG_ERR("BIB on BCB target %d but no bibBlk?.", tgtBlkNum);
                        return -1;
                    }
                }
                else
                {
                    return 0;
                }
            }
        }

        /*
         * Step 2.6: See if we can re-use this security block to hold the result of
         *           applying security to the tgtBlkNum.
         */
        if(bpsec_sci_multCheck(sdr, &asb, def, wm, parms) > 0)
        {
            *secBlk = blockObj;
        }

    }

    /*
     * Step 3: If we get here, there is either no security block that targets our tgtBlkNum
     *         or our sopType is BCB and a BIB is targeting our tgtBlkNum
     *
     *         Now the success of this function lies on whether or not we found the target
     *         block number in the bundle at all. If we found the target then we can
     *         proceed. If the target is missing, it cannot be targeted by a SOP!
     */
    return targetFound;
}


// TODO Document function


/*
 * Calculate the security result for each operation in each block. Then,
 * serialize each block.
 */

int bpsec_util_attachSecurityBlocks(Bundle *bundle, BpBlockType secBlkType, sc_action action)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object blockObj;
    ExtensionBlock block;
    BpsecOutboundASB asb;
    uint8_t *serializedAsb = NULL;
    char *fromEid = NULL; /*    Instrumentation.*/
    int result = 0;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,%d)", (uaddr)bundle, secBlkType, action);

    for (elt = sdr_list_first(sdr, bundle->extensions); elt; elt = sdr_list_next(sdr, elt))
    {
        blockObj = sdr_list_data(sdr, elt);

        sdr_read(sdr, (char*) &block, blockObj, sizeof(ExtensionBlock));

        if (block.bytes) /*    Already serialized.    */
        {
            continue; /*    Not newly sourced.    */
        }

        if (block.type != secBlkType) {
            continue; /*    Doesn't apply.        */
        }

        /*    This is a new BIB: compute all signatures,
         *    insert all security results, serialize.        */

        sdr_read(sdr, (char*) &asb, block.object, sizeof(BpsecOutboundASB));

        /* TODO - can we pull this out of the for loop? */
        readEid(&(asb.scSource), &fromEid);

        BPSEC_DEBUG_INFO("Attaching security block with source %s", fromEid);
        if(bpsec_util_generateSecurityResults(bundle, fromEid, &block, &asb, action) <= 0)
        {
        	BPSEC_DEBUG_ERR("Unable to populate security block (type %d, id %d) with source %s.", block.type, block.number, fromEid);
            MRELEASE(fromEid);
            return -1;
        }

        /* Step 6 - serialize the BIB ASB into the BIB blk.  */
        /* Step 6.1 - Create a serialized version of the BIB ASB. */
         if ((serializedAsb = bpsec_asb_outboundAsbSerialize((uint32_t*) &(block.dataLength), &asb)) == NULL)
         {
             BPSEC_DEBUG_ERR("Unable to serialize ASB. bibBlk->dataLength = %d", block.dataLength);
             // TODO: Issue #74 ADD_BIB_TX_FAIL(fromEid, 1, length);
             MRELEASE(fromEid);
             result = -1;
             bundle->corrupt = 1;
             scratchExtensionBlock(&block);
             BPSEC_DEBUG_PROC("--> %d", result);
             return result;
         }

         /* Step 6.2 - Copy serializedBIB ASB into the BIB extension block. */
         if ((result = serializeExtBlk(&block, (char*) serializedAsb)) < 0)
         {
             BPSEC_DEBUG_ERR("Unable to serialize the extension block.", NULL);
             bundle->corrupt = 1;
             // TODO: Should we scratch the extension block here like we do in the
             //        failure case above?
         }

        sdr_write(sdr, block.object, (char* ) &asb, sizeof(BpsecOutboundASB));
        sdr_write(sdr, blockObj, (char* ) &block, sizeof(ExtensionBlock));

        MRELEASE(serializedAsb);


       // TODO: Issue #74 ADD_BIB_TX_PASS(fromEid, 1, length);
        MRELEASE(fromEid);
    }



    BPSEC_DEBUG_PROC("-->0", NULL);
    return 0;
}




// TODO I think we can delete this function... verify..
LystElt bpsec_util_findInboundTarget(AcqWorkArea *work, int blockNumber, LystElt *bibElt)
{
    LystElt elt;
    AcqExtBlock *block;
    BpsecInboundASB *asb;
    LystElt elt2;
    BpsecInboundTargetResult *target;

    for (elt = lyst_first(work->extBlocks); elt; elt = lyst_next(elt))
    {
        block = (AcqExtBlock*) lyst_data(elt);

        if (block->type != BlockIntegrityBlk)
        {
            continue;
        }

        /*    This is a BIB.  See if the indicated
         *    non-BPSec block is one of its targets.        */

        asb = (BpsecInboundASB*) (block->object);
        for (elt2 = lyst_first(asb->scResults); elt2; elt2 = lyst_next(elt2)) {
            target = (BpsecInboundTargetResult*) lyst_data(elt2);
            if (target->scTargetId== blockNumber) {
                *bibElt = elt;
                return elt2;
            }
        }
    }

    return NULL; /*    No such target.                */
}


// TODO Document function
void bpsec_util_inboundBlkClear(AcqExtBlock *blk)
{
    BpsecInboundASB    *asb = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC")", (uaddr) blk);

    if(blk == NULL)
    {
        BPSEC_DEBUG_WARN("Attempt to clear NULL blk?", NULL);
        return;
    }

    if (blk->object)
    {
        asb = (BpsecInboundASB *) (blk->object);
        bpsec_asb_inboundAsbDelete(asb);
        blk->object = NULL;
        blk->size = 0;
    }
}
// TODO Document function
void bpsec_util_outboundBlkRelease(ExtensionBlock *blk)
{
    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC")", (uaddr) blk);

    if(blk)
    {
       bpsec_asb_outboundAsbDeleteObj(getIonsdr(), blk->object);
    }
}
// TODO Document function
/*
 * THis is used to remove a SOP from one security block and place it in another security block.
 * This is currently used in cases where you need to pull a BIB signature out of a BIB to allow
 * another BCB to target it.
 */
int bpsec_util_swapOutboundSop(Bundle *bundle, ExtensionBlock *src, ExtensionBlock *dest, int tgtBlkNum)
{
	BPSEC_DEBUG_ERR("Not implemented yet.", NULL);
	return -1;
}



/******************************************************************************
 *
 * \par Function Name: bpsec_asb_keyRetrieve
 *
 * \par Purpose: Retrieves the key associated with a particular keyname.
 *
 * \par Date Written:  6/01/09
 *
 * \retval sc_value -- The key value. Length 0 indicates error.
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
 *  10/20/20  S. Burleigh   Rewrite for BPv7 and BPSec
 *****************************************************************************/

sc_value bpsec_util_keyRetrieve(char *keyName)
{
    int     keyLength;
    sc_value   key;
    char        stdBuffer[100];
    int     ReqBufLen = 0;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC")", (uaddr) keyName);

    /*
     * We first guess that the key will normally be no more than 100
     * bytes long, so we call sec_get_key with a buffer of that size.
     * If this works, great; we make a copy of the retrieved key
     * value and pass it back.  If not, then the function has told
     * us what the actual length of this key is; we allocate a new
     * buffer of sufficient length and call sec_get_key again to
     * retrieve the key value into that buffer.
     */

    memset(&key, 0, sizeof(sc_value));

    if(keyName == NULL || strlen(keyName) == 0)
    {
        BPSEC_DEBUG_ERR("Bad Parms", NULL);
        BPSEC_DEBUG_PROC("-> key (len=%d)", key.scValLength);
        return key;
    }

    ReqBufLen = sizeof(stdBuffer);
    keyLength = sec_get_key(keyName, &ReqBufLen, stdBuffer);

    /**
     *  Step 1 - Check the key length.
     *           <  0 indicated system failure.
     *           == 0 indicated key not found.
     *           > 0  inicates success.
     */
    if (keyLength < 0)  /* Error. */
    {
        BPSEC_DEBUG_ERR("Can't get length of key '%s'.", keyName);
        BPSEC_DEBUG_PROC("-> key (len=%d)", keyLength);
        return key;
    }
    else if(keyLength > 0) /*   Key has been retrieved.     */
    {
        if(bpsec_scv_memCreate(&key, SC_VAL_TYPE_PARM, 0, keyLength) < 1)
        {
            BPSEC_DEBUG_ERR("Can't allocate key of size %d", keyLength);
            BPSEC_DEBUG_PROC("-> key (len=%d)", key.scValLength);
            return key;
        }

        memcpy(key.scRawValue.asPtr, stdBuffer, key.scValLength);
        BPSEC_DEBUG_PROC("-> key (len=%d)", key.scValLength);

        return key;
    }

    /**
     *  Step 2 - At this point, if we did not find a key and did
     *  not have a system error, either the key was not found or
     *  it was found and was larger than the standard buffer.
     *
     *  If we ran out of space, the ReqBufLen will be less than
     *  the provided buffer. Otherwise, the neededBufLen will be
     *  the required size of the buffer to hold the key.        */

    /* Step 2a - If we did not find a key... */
    if(ReqBufLen <= sizeof(stdBuffer))
    {
        BPSEC_DEBUG_WARN("Unable to find key '%s'", keyName);
        BPSEC_DEBUG_PROC("--> key (len=%d)", key.scValLength);
        return key;
    }

    /* Step 2b - If the buffer was just not big enough, make a
     * larger buffer and try again.
     */

    if(bpsec_scv_memCreate(&key, SC_VAL_TYPE_PARM, 0, ReqBufLen) < 1)
    {
        BPSEC_DEBUG_ERR("Can't allocate key of size %d", ReqBufLen);
        BPSEC_DEBUG_PROC("--> key (len=%d)", ReqBufLen);
        return key;
    }

    /* Step 3 - Call get key again and it should work this time. */
    if (sec_get_key(keyName, &ReqBufLen, (char *) (key.scRawValue.asPtr)) <= 0)
    {
    	bpsec_scv_clear(0, &key);
        BPSEC_DEBUG_ERR("Can't get key '%s'", keyName);
        BPSEC_DEBUG_PROC("--> key (len=%d)", key.scValLength);
        return key;
    }

    BPSEC_DEBUG_PROC("--> key (len=%d)", key.scValLength);
    return key;
}



/******************************************************************************
 * @brief Encrypt/Decrypt a block held in the SDR.
 *
 * @param[in]     suite        The cipher suite to use for the encrypt/decrypt.
 * @param[in]     csi_ctx      The CSI context to use for the encrypt/decrypt.
 * @param[in]     blocksize    Information about how to chunk the data.
 * @param[in|out] dataReader   The ZCO reader.
 * @param[in]     outputBufLen The length of the cipher suite output
 * @param[out]    outputZco    The ZCO holding the cipher suite output.
 * @param[in]     function     Whether we are encrypting or decrypting
 *
 * Replace the current contents of a ZCO with the output of a cipher suite. This
 * can be used for both encryption and decryption.
 *
 * The dataObj must point to the block-type-specific portions of the block.
 *
 * The terms input and output are used instead of plaintext and ciphertext
 * because this function can be called for both encryption and decryption.
 * When performing encryption, the input is the plaintext and the output
 * is the ciphertext. When performing decryption, the input is the ciphertext
 * and the output is the plaintext.
 *
 * @todo See if some parms need to be passed in, or if they can be calculated
 *       in the function.
 *
 *
 * @retval 1  - The security block was processed
 * @retval 0  - There was a logic error
 * @retval -1 - System error
 *****************************************************************************/

int32_t bpsec_util_sdrBlkConvert(uint32_t suite, uint8_t *csi_ctx,
		                         csi_blocksize_t *blocksize, ZcoReader *dataReader,
							     uvast outputBufLen, Object *outputZco, uint8_t function)
{
    Sdr          sdr = getIonsdr();
    csi_val_t    csiInputChunk;
    csi_val_t    csiOutputChunk;
    uvast        chunkSize = 0;
    uvast        bytesRemaining = 0;
    Object       outputBuffer = 0;
    uvast        writeOffset = 0;
    SdrUsageSummary    summary;
    uvast        memmax = 0;
    int          result = 1;

    BPSEC_DEBUG_PROC("(%d," ADDR_FIELDSPEC"," ADDR_FIELDSPEC ","
                      ADDR_FIELDSPEC "," UVAST_FIELDSPEC "," ADDR_FIELDSPEC", %d)",
			          suite, (uaddr) csi_ctx, (uaddr) blocksize, (uaddr) dataReader,
			          outputBufLen, (uaddr) outputZco, function);


    /* Step 0 - Sanity Checks. */
    CHKERR(csi_ctx);
    CHKERR(blocksize);
    CHKERR(dataReader);
    CHKERR(outputZco);

    /*
     * Step 1 - Get information about the SDR storage space. If the expected
     *          cipher text length is less than half the available space, we
     *          will attempt the conversion using the SDR.
     *
     *          Note, ">> 1" means divide by 2.
     */
    sdr_usage(sdr, &summary);
    memmax = (summary.largePoolFree + summary.unusedSize) >> (uvast) 1;

    if (outputBufLen > memmax)
    {
        BPSEC_DEBUG_ERR("Buffer len will not fit. " UVAST_FIELDSPEC " > " UVAST_FIELDSPEC, outputBufLen, memmax);
        sdr_report(&summary);
        BPSEC_DEBUG_PROC("--> 0", NULL);
        return 0;
    }

    /*
     * Step 2 - Allocate space in the SDR to hold the converted text.
     *
     *          Also, create a ZCO to this allocated space. When creating
     *          the ZCO, we pass the additive inverse of the length to
     *          zco_create as that tells the ZCO library that space has
     *          already been allocated.
     *
     */
    if ((outputBuffer = sdr_malloc(sdr, outputBufLen)) == 0)
    {
        BPSEC_DEBUG_ERR("Cannot allocate" UVAST_FIELDSPEC " from SDR.", NULL);
        BPSEC_DEBUG_PROC("--> -1", NULL);
        return -1;
    }

    if ((*outputZco = zco_create(sdr, ZcoSdrSource, outputBuffer, 0,
                                 0 - outputBufLen, ZcoOutbound)) == 0)
    {
        BPSEC_DEBUG_ERR("Cannot create zco.", NULL);
        sdr_free(sdr, outputBuffer);
        BPSEC_DEBUG_PROC("--> -1", NULL);
        return -1;
    }


    /*
     * Step 3 - Set up read buffers to read input text in chunk sizes
     *          and pass them to the cipher suite until there are no
     *          more chunks remaining.
     */

    chunkSize = blocksize->chunkSize;
    bytesRemaining = blocksize->plaintextLen;

    csiInputChunk.len = chunkSize;
    if((csiInputChunk.contents = MTAKE(chunkSize)) == NULL)
    {
        BPSEC_DEBUG_ERR("Can't allocate buffer of size %d.", chunkSize);

        sdr_free(sdr, outputBuffer);
        zco_destroy(sdr, *outputZco);
        *outputZco = 0;
        BPSEC_DEBUG_PROC("--> -1", NULL);
        return -1;
    }


    /*
     * Step 4: Walk through the data object converting input chunks to
     *         output chunks. We will read an input chunk, pass it to
     *         the cipher suite for conversion, and then write the
     *         output to the output buffer.
     */
    while (bytesRemaining > 0)
    {
         /* Step 4.1 - Catch "final" iteration. */
         if (bytesRemaining < chunkSize)
         {
             chunkSize = bytesRemaining;
         }

         /* Step 4.2 - Read an input chunk. */
         csiInputChunk.len = zco_transmit(sdr, dataReader, chunkSize, (char *) csiInputChunk.contents);
         if (csiInputChunk.len <= 0)
         {
             BPSEC_DEBUG_ERR("Can't do priming read of length %d.", chunkSize);
        	 break;
         }

         /* Step 4.3 - Pass input to the cipher suite and generate output. */
         csiOutputChunk = csi_crypt_update(suite, csi_ctx, function, csiInputChunk);
         if (csiOutputChunk.contents == NULL)
         {
            BPSEC_DEBUG_ERR("Could not encrypt input of %d with chunk size of %d.", csiInputChunk.len, chunkSize);
            break;
         }

         /* Step 4.4 - Write output chunk to the output buffer. */
        sdr_write(sdr, outputBuffer + writeOffset, (char *) csiOutputChunk.contents, csiOutputChunk.len);
        MRELEASE(csiOutputChunk.contents);

        /* Step 4.5 - Prep for next iteration. */
        bytesRemaining -= csiInputChunk.len;
        writeOffset += csiOutputChunk.len;
    }

    MRELEASE(csiInputChunk.contents);

    if(bytesRemaining > 0)
    {
    	result = ERROR;
    	sdr_free(sdr, outputBuffer);
    	zco_destroy(sdr, *outputZco);
    	*outputZco = 0;
    }

    BPSEC_DEBUG_PROC("--> %d", result);
    return result;
}




/******************************************************************************
 * @brief Encrypt/Decrypt a large  block by using the file system.
 *
 * @param[in]     suite        The cipher suite to use for the encrypt/decrypt.
 * @param[in]     csi_ctx      The CSI context to use for the encrypt/decrypt.
 * @param[in]     blocksize    Information about how to chunk the data.
 * @param[in|out] dataReader   The ZCO reader.
 * @param[in]     outputBufLen The length of the cipher suite output
 * @param[out]    outputZco    The ZCO holding the cipher suite output.
 * @param[in]     filename     Name of the temp filename.
 * @param[in]     function     Whether we are encrypting or decrypting
 *
 * Replace the current contents of a ZCO with the output of a cipher suite. This
 * can be used for both encryption and decryption.
 *
 * The dataObj must point to the block-type-specific portions of the block.
 *
 * The terms input and output are used instead of plaintext and ciphertext
 * because this function can be called for both encryption and decryption.
 * When performing encryption, the input is the plaintext and the output
 * is the ciphertext. When performing decryption, the input is the ciphertext
 * and the output is the plaintext.
 *
 * @todo See if some parms need to be passed in, or if they can be calculated
 *       in the function.
 *
 *
 * @retval 1  - The security block was processed
 * @retval 0  - There was a logic error
 * @retval -1 - System error
 *****************************************************************************/
int32_t bpsec_util_fileBlkConvert(uint32_t suite, uint8_t *csi_ctx,
		                          csi_blocksize_t *blocksize, ZcoReader *dataReader,
								  uvast outputBufLen, Object *outputZco, char *filename, uint8_t function)
{
    Sdr        sdr = getIonsdr();
    csi_val_t  csiInputChunk;
    csi_val_t  csiOutputChunk;
    uvast      chunkSize = 0;
    uvast      bytesRemaining = 0;
    Object     fileRef = 0;
    int        result = 1;

    BPSEC_DEBUG_PROC("(%d," ADDR_FIELDSPEC"," ADDR_FIELDSPEC "," ADDR_FIELDSPEC ","
                          UVAST_FIELDSPEC "," ADDR_FIELDSPEC","ADDR_FIELDSPEC", %d)",
    			          suite, (uaddr) csi_ctx, (uaddr) blocksize, (uaddr) dataReader,
    			          outputBufLen, (uaddr) outputZco, (uaddr) filename, function);

    /* Step 0 - Sanity Checks. */
    CHKERR(csi_ctx);
    CHKERR(blocksize);
    CHKERR(dataReader);
    CHKERR(outputZco);


    /* Step 1 - Initialization */
    chunkSize = blocksize->chunkSize;
    bytesRemaining = blocksize->plaintextLen;
    *outputZco = 0;


    /*
     * Step 2 - Set up read buffers to read input text in chunk sizes
     *          and pass them to the cipher suite until there are no
     *          more chunks remaining.
     */
    csiInputChunk.len = chunkSize;
    if((csiInputChunk.contents = MTAKE(chunkSize)) == NULL)
    {
        BPSEC_DEBUG_ERR("Can't allocate buffer of size %d.", chunkSize);
        BPSEC_DEBUG_PROC("--> -1", NULL);
        return -1;
    }


    /*
     * Step 3: Walk through the data object converting input chunks to
     *         output chunks. We will read an input chunk, pass it to
     *         the cipher suite for conversion, and then write the
     *         output to the output buffer.
     */
     while (bytesRemaining > 0)
     {
         /* Step 3.1 - Catch "final" iteration. */
         if (bytesRemaining < chunkSize)
         {
             chunkSize = bytesRemaining;
         }

         /* Step 3.2 - Read an input chunk. */
         csiInputChunk.len = zco_transmit(sdr, dataReader, chunkSize, (char *) csiInputChunk.contents);
         if (csiInputChunk.len <= 0)
         {
             BPSEC_DEBUG_ERR("Can't do priming read of length %d.", chunkSize);
             break;
         }

         /* Step 3.3 - Pass input to the cipher suite and generate output. */
         csiOutputChunk = csi_crypt_update(suite, csi_ctx, function, csiInputChunk);
         if (csiOutputChunk.contents == NULL)
         {
            BPSEC_DEBUG_ERR("Could not encrypt.", csiInputChunk.len, chunkSize);
            break;
         }

         /* Step 3.4 - Write output chunk to file. */
         if (bpsec_util_zcoFileSourceTransferTo(sdr, outputZco, &fileRef,
        		filename, (char *) csiOutputChunk.contents, csiOutputChunk.len) <= 0)
         {
            BPSEC_DEBUG_ERR("Transfer of chunk has failed..", NULL);
            MRELEASE(csiOutputChunk.contents);
            break;
         }

         /* Step 3.5 - Prep for next iteration. */
         bytesRemaining -= csiInputChunk.len;
         //microsnooze(1000); // TODO: COnsider if a sleep here will help filesystem catch up.
         MRELEASE(csiOutputChunk.contents);
    }

    MRELEASE(csiInputChunk.contents);

    if(bytesRemaining > 0)
    {
    	result = ERROR;
    }

    BPSEC_DEBUG_PROC("--> %d", result);
    return result;
}


// TODO Everything below here should probably be removed.


int bpsec_util_numKeysGet(int *size)
{
    // TODO - remove calls to this function.
    BPSEC_DEBUG_ERR("DEPRECATED FUNCTION CALL", NULL)
    CHKERR(0);
    return -1;
}

void bpsec_util_keysGet(char *buffer, int length)
{
    // TODO - remove calls to this function.
    BPSEC_DEBUG_ERR("DEPRECATED FUNCTION CALL", NULL)
    CHKVOID(0);
}

int  bpsec_util_numCSNamesGet(int *size)
{
    // TODO - remove calls to this function.
    BPSEC_DEBUG_ERR("DEPRECATED FUNCTION CALL", NULL)
    CHKERR(0);
    return -1;
}

void bpsec_util_cSNamesGet(char *buffer, int length)
{
    // TODO - remove calls to this function.
    BPSEC_DEBUG_ERR("DEPRECATED FUNCTION CALL", NULL)
    CHKVOID(0);
}
