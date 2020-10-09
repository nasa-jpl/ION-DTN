/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: bib.c
 **
 ** Description: Definitions supporting generic processing of BIB blocks.
 **              This includes both the BIB Interface to the ION bpsec
 **              API as well as a default implementation of the BIB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BIB bpsec Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              bibOffer
 **              bibSerialize
 **              bibProcessOnDequeue
 **              bibRelease
 **              bibCopy
 **                                                  bibReview
 **                                                  bibParse
 **                                                  bibCheck
 **                                                  bibRecord
 **                                                  bibClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    SIGN SIDE                     VERIFY SIDE
 **
 **              bibDefaultSign
 **
 **                                              bibDefaultVerify
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/05/10  E. Birrane     Implementation as extsbspbib.c (JHU/APL)
 **            S. Burleigh    Implementation as sbspbib.c for Bpsec
 **  11/02/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 **  09/02/19  S. Burleigh    Rename everything for bpsec
 *****************************************************************************/

#include "csi.h"
#include "bpsec_util.h"
#include "bib.h"
#include "bpsec_instr.h"

#if (BIB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/******************************************************************************
 *
 * \par Function Name: bibAttach
 *
 * \par Purpose: Complete, compute, and attach a BIB block within the bundle.
 *               This function completes construction of the ASB for a BIB,
 *               using the appropriate ciphersuite, and generates a serialized
 *               version of the block appropriate for transmission.
 * *
 * \retval int -1  - Error.
 *              0  - Nothing to do
 *             >0  - BIB Attached
 *
 * \param[in|out]  bundle  The bundle to which a BIB is to be attached.
 * \param[out]     bibBlk  The BIB extension block.
 * \param[out]     bibAsb  The initialized ASB for this BIB.
 *
 * \par Notes:
 *	    1. The blkAsb MUST be pre-allocated and of the correct size to hold
 *	       the created BIB ASB.
 *	    2. The passed-in asb MUST be pre-initialized with both the target
 *	       block type and the security source.
 *	    3. The bibBlk MUST be pre-allocated and initialized with a size,
 *	       a target block type, and the object within the block MUST be
 *	       allocated in the SDR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/03/15  E. Birrane     Update to profiles, error checks [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  07/19/18  S. Burleigh    Abandon bundle if can't attach BIB
 *****************************************************************************/

static int	bibAttach(Bundle *bundle, ExtensionBlock *bibBlk,
			BpsecOutboundBlock *bibAsb)
{
	Sdr			sdr = getIonsdr();
	int8_t			result = 0;
	BibProfile		*prof;
	char			*fromEid;	/*	Instrumentation.*/
	char			*toEid;		/*	For whatever.	*/
	uint8_t			*serializedAsb;
	uvast			bytes = 0;

	BIB_DEBUG_PROC("+ bibAttach(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC
			"," ADDR_FIELDSPEC ")", (uaddr) bundle, (uaddr) bibBlk,
			(uaddr) bibAsb);

	/* Step 0 - Sanity checks. */
	CHKERR(bundle);
	CHKERR(bibBlk);
	CHKERR(bibAsb);
	if (sdr_list_length(sdr, bibAsb->targets) == 0)
	{
		BIB_DEBUG(2, "NOT Attaching BIB; no targets.", NULL);

		result = 0;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- bibAttach -> %d", result);
		return result;
	}

	BIB_DEBUG(2, "Attaching BIB.", NULL);
	if (bpsec_getOutboundSecuritySource(bundle, blk, bibAsb, &fromEid) < 0)
	{
		BIB_DEBUG_ERR("x bibAttach: Can't get security source.", NULL);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- bibAttach --> %d", result);
		return result;
	}

	/* Step 1 - Finish populating the BIB ASB.			*/

	prof = get_bib_prof_by_number(bibAsb->contextId);
	CHKERR(prof);
	if (prof->construct)
	{
		if (prof->construct(prof->suiteId, bibBlk, bibAsb) < 0)
		{
			ADD_BIB_TX_FAIL(fromEid, 1, 0);
			MRELEASE(fromEid);

			BIB_DEBUG_ERR("x bibAttach: Can't construct ASB.",
					NULL);
			result = -1;
			bundle->corrupt = 1;
			scratchExtensionBlock(bibBlk);
			BIB_DEBUG_PROC("- bibAttach --> %d", result);
			return result;
		}
	}

	/* Step 2 - Sign the target blocks and store the signatures.	*/

	readEid(&(bundle->destination, &toEid));
	CHKERR(toEid);
	result = (prof->sign == NULL)
			? bibDefaultSign(prof->suiteId, bundle, bibBlk,
				bibAsb, &bytes, toEid)
			: prof->sign(prof->suiteId, bundle, bibBlk,
				bibAsb, &bytes, toEid);
	MRELEASE(toEid);
	if (result < 0)
	{
		ADD_BIB_TX_FAIL(fromEid, 1, bytes);
		MRELEASE(fromEid);

		BIB_DEBUG_ERR("x bibAttach: Can't sign target blocks.", NULL);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- bibAttach --> %d", result);
		return result;
	}

	/* Step 3 - serialize the BIB ASB into the BIB blk. */

	/* Step 3.1 - Create a serialized version of the BIB ASB. */

	if ((serializedAsb = bpsec_serializeASB((uint32_t *)
			&(bibBlk->dataLength), bibAsb)) == NULL)
	{
		ADD_BIB_TX_FAIL(fromEid, 1, bytes);
		MRELEASE(fromEid);

		BIB_DEBUG_ERR("x bibAttach: Unable to serialize ASB.  \
bibBlk->dataLength = %d", bibBlk->dataLength);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- bibAttach --> %d", result);
		return result;
	}

	/* Step 3.2 - Copy serializedBIB ASB into the BIB extension block. */

	if ((result = serializeExtBlk(bibBlk, (char *) serializedAsb)) < 0)
	{
		bundle->corrupt = 1;
	}

	MRELEASE(serializedAsb);

	ADD_BIB_TX_PASS(fromEid, 1, bytes);
	MRELEASE(fromEid);

	BIB_DEBUG_PROC("- bibAttach --> %d", result);
	return result;
}

int	bibSerialize(ExtensionBlock *blk, Bundle *bundle)
{
	/*	NOTE: BIBs are automatically serialized at the time
	 *	they are attached to a bundle, and they are not
	 *	subject to canonicalization (a BIB cannot be the
	 *	target of another BIB).  Nothing to do here.		*/

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bibCheck
 *
 * \par Purpose: This callback determine whether or not a BIB's
 * 		 security target blocks have been corrupted en route.
 * 		 Specifically, a newly computed hash for each target
 * 		 block must match that block's security result as
 * 		 encoded in the BIB.
 *
 * \retval   0 - No blocks were found to be corrupt.
 *           3 - At least one block was found to be corrupt.
 *          -1 - There was a system error.
 *
 * \param[in]  blk  The BIB whose targets must be checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *****************************************************************************/

int	bibCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle			*bundle;
	BpsecInboundBlock	*asb = NULL;
	BpsecInboundTarget	*target = NULL;
	char			*fromEid;	/*	Instrumentation	*/
	BibProfile		*prof;
	BibVerifyFn		verify = NULL;
	int8_t			result;
	uvast			bytes = 0;

	BIB_DEBUG_PROC("+ bibCheck(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	if (blk == NULL || blk->object == NULL || wk == NULL)
	{
		BIB_DEBUG_ERR("x bibCheck: Blocks are NULL. %x",
				(unsigned long) blk);
		BIB_DEBUG_PROC("- bibCheck --> %d", -1);
		return -1;
	}

	/*	The security destination is always the final
	 *	destination of the bundle.  The security source is
	 *	normally the original source of the bundle, but a BIB
	 *	can alternatively be inserted at any point in the
	 *	bundle's end-to-end path.				*/

	bundle = &(wk->bundle);
	asb = (BpsecInboundBlock *) (blk->object);
	if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
	{
		/*	Waypoint source.				*/

		readEid(&(asb->securitySource), &fromEid);
		if (fromEid == NULL)
		{
			ADD_BIB_RX_FAIL(NULL, 1, 0);
			return -1;
		}
	}
	else	/*	Bundle source is security source.		*/
	{
		readEid(&(bundle->id.source), &fromEid);
		if (fromEid == NULL)
		{
			ADD_BIB_RX_FAIL(NULL, 1, 0);
			return -1;
		}
	}

	/*	Given sender & receiver EIDs, get applicable BIB rule.	*/

	prof = bib_bib_prof_by_number(asb->contextId);
	if (prof == NULL)
	{
		/*	We can't verify any of this BIB's signatures.
		 *	If any of this BIB's targets blocks are
		 *	subject to local BIB verification rules,
		 *	clearly they can't be verified and must
		 *	therefore be considered corrupted.  The
		 *	bibDefaultVerify function will make this
		 *	determination.					*/

			ADD_BIB_RX_MISS(fromEid, 1, 0);
			MRELEASE(fromEid);

			BIB_DEBUG_INFO("- bibCheck - No rule.", NULL);
			BIB_DEBUG_PROC("- bibCheck", NULL);
	}
	else
	{
		verify = prof->verify;
	}

	/*	Invoke ciphersuite-specific check procedure.		*/

	result = (prof->verify == NULL) ?
		bibDefaultVerify(prof->suiteId, wk, blk, &bytes, fromEid)
		: prof->verify(prof->suiteId, wk, blk, &bytes, fromEid);

	/*	Discard the BIB if the local node is the destination
	 *	of the bundle or the BIB is invalid or verification
	 *	failed (meaning the block is altered and therefore
	 *	the bundle is altered); otherwise make sure the BIB
	 *	is retained.						*/

		/*	Rule is found, but we don't have this CS.
		 *	We have to conclude that the BIB is invalid,
		 *	in which case the bundle is judged corrupt.	*/

		ADD_BIB_RX_FAIL(fromEid, 1, 0);
		MRELEASE(fromEid);

		discardExtensionBlock(blk);
	 	BIB_DEBUG_ERR("- bibCheck - Profile missing!", NULL);
		BIB_DEBUG_PROC("- bibCheck", NULL);
		return 0;			/*	Bundle corrupt.	*/

	BIB_DEBUG_INFO("i bibCheck: Verify result was %d", result);
	if ((result == 0) || (result == 4))
	{
		ADD_BIB_RX_FAIL(fromEid, 1, bytes);
		discardExtensionBlock(blk);
	}
	else if (bpsec_destinationIsLocal(&(wk->bundle)))
	{
		BIB_DEBUG(2, "BIB check passed.", NULL);
		ADD_BIB_RX_PASS(fromEid, 1, bytes);
		discardExtensionBlock(blk);
	}
	else
	{
		ADD_BIB_FWD(fromEid, 1, bytes);
	}

	MRELEASE(fromEid);

	BIB_DEBUG_PROC("- bibCheck(%d)", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bibRecord
 *
 * \par Purpose:	This callback copies an acquired BIB block's object
 *			into the object of a non-volatile BIB in heap space.
 *
 * \retval   0 - Recording was successful.
 *          -1 - There was a system error.
 *
 * \param[in]  oldBlk	The acquired BIB in working memory.
 * \param[in]  newBlk	The non-volatle BIB in heap space.
 *
 *****************************************************************************/

int	bibRecord(ExtensionBlock *new, AcqExtBlock *old)
{
	int	result;

	BIB_DEBUG_PROC("+ bibRecord(%x, %x)", (unsigned long) new,
			(unsigned long) old);

	result = bpsec_recordAsb(new, old);
	new->tag = -1;
	
	/*	Tag value -1 indicates "unknown".  Negative tags are
	 *	undefined for BIBs.					*/

	BIB_DEBUG_PROC("- bibRecord", NULL);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bibClear
 *
 * \par Purpose: This callback removes all memory allocated by the bpsec module
 *               during the block's acquisition process.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the bpsec module.
 *****************************************************************************/

void	bibClear(AcqExtBlock *blk)
{
	BpsecInboundBlock	*asb;

	BIB_DEBUG_PROC("+ bibClear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		asb = (BpsecInboundBlock *) (blk->object);
		bpsec_releaseInboundAsb(asb);
		blk->object = NULL;
		blk->size = 0;
	}

	BIB_DEBUG_PROC("- bibClear", NULL);
}

/******************************************************************************
 *
 * \par Function Name: bibCopy
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
 * \par Assumptions
 *      1. The newBlk is preallocated to the correct size.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Port from pibCopy
 *  11/04/15  E. Birrane     Comments. [Secure DTN implementation (NASA:
 *  			     NNX14CS58P)]
 *
 *****************************************************************************/

int	bibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	return bpsec_copyAsb(newBlk, oldBlk);
}

/******************************************************************************
 *
 * \par Function Name: bibDefaultCompute
 *
 * \par Calculate a digest given a set of data and associated key information
 *      in accordance with this ciphersuite specification.
 *
 * \retval unsigned char * - The security result.
 *
 * \param[in]  dataObj     - The serialized data to hash, a ZCO.
 * \param[in]  chunkSize   - The chunking size for going through the bundle
 * \param[in]  suite       - Which ciphersuite to use to caluclate the value.
 * \param[in|out] context  - The CSI context for signing/verifying.
 * \param[in]  svc         - Service being performed: SIGN or VERIFY
 *
 * \par Notes:
 *   - This version of the function uses the ION Cryptographic Interface.
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/05/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bibDefaultCompute(Object dataObj, uint32_t chunkSize, uint32_t suite,
		void *context, csi_svcid_t svc)
{
	Sdr		sdr = getIonsdr();
	char		*dataBuffer;
	ZcoReader	dataReader;
	unsigned int	bytesRemaining = 0;
	unsigned int	bytesRetrieved = 0;

	BIB_DEBUG_INFO("+ bibDefaultCompute(0x%x, %d, %d, 0x%x)",
		       (unsigned long) dataObj, chunkSize, suite,
		       (unsigned long) context);

	CHKERR(context);

	/* Step 2 - Allocate a working buffer. */
	if ((dataBuffer = MTAKE(chunkSize)) == NULL)
	{
		BIB_DEBUG_ERR("x bibDefaultCompute - Can't allocate buffer of \
size %d.", chunkSize);
		return ERROR;
	}

	/*
	 * Step 3 - Setup playback of data from the data object.
	 *          The data object is the target block.
	 */

	if ((bytesRemaining = zco_length(sdr, dataObj)) <= 0)
	{
		BIB_DEBUG_ERR("x bibDefaultCompute - data object has no \
length.", NULL);
		MRELEASE(dataBuffer);
		BIB_DEBUG_PROC("- bibDefaultCompute--> ERROR", NULL);
		return ERROR;
	}

	BIB_DEBUG_INFO("i bibDefaultCompute bytesRemaining: %d",
			bytesRemaining);

	if ((sdr_begin_xn(sdr)) == 0)
	{
		BIB_DEBUG_ERR("x bibDefaultCompute - Can't start txn.", NULL);
		MRELEASE(dataBuffer);
		BIB_DEBUG_PROC("- bibDefaultCompute--> ERROR", NULL);
		return ERROR;
	}

	zco_start_transmitting(dataObj, &dataReader);

	BIB_DEBUG_INFO("i bibDefaultCompute: bundle size is %d",
			bytesRemaining);

	/* Step 5 - Loop through the data in chunks, updating the context. */
	while (bytesRemaining > 0)
	{
		sci_inbound_tlv val;

		if (bytesRemaining < chunkSize)
		{
			chunkSize = bytesRemaining;
		}

		bytesRetrieved = zco_transmit(sdr, &dataReader, chunkSize,
				dataBuffer);

		if (bytesRetrieved != chunkSize)
		{
			BIB_DEBUG_ERR("x bibDefaultCompute: Read %d bytes, \
but expected %d.", bytesRetrieved, chunkSize);
			sdr_exit_xn(sdr);
			MRELEASE(dataBuffer);

			BIB_DEBUG_PROC("- bibDefaultCompute--> ERROR", NULL);
			return ERROR;
		}

		/*	Add the data to the context.		*/
		val.value = (uint8_t *) dataBuffer;
		val.length = chunkSize;
		sci_sign_update(suite, context, val, svc);

		bytesRemaining -= bytesRetrieved;
	}

	sdr_exit_xn(sdr);
	MRELEASE(dataBuffer);
	return 1;
}

#if 0
/******************************************************************************
 *
 * \par Function Name: bibDefaultResultLen
 *
 * \par Calculate the result length as a function of the length of the
 *      ciphersuite result and the overhead capturing that result in
 *      a BIB. BIB encodes results as a TLV (type, length, value) 3-tuple.
 *
 * \param[in]  suite   - THe ciphersuite to be used for signing.
 * \param[in]  TLV     - Whether to ignore space for the type/length in the
 *                       TLV (0) or not (!0)
 *
 * \return >0 - The size of the result length
 *        <=0 - Error.
 *
 *  \Notes
 *   - The csi call requires a context, which we usually do not have when
 *     quering for the security result length, so we create a context for
 *     this call.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/05/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  02/27/16  E. Birrane     Update to CSI Interface [Secure DNULL, NULL,
 *  			     CSI_SVC_SIGNTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t bibDefaultResultLen(uint32_t suite, uint8_t tlv)
{
	void		*context = NULL;
	sci_inbound_tlv	key;
	uint32_t	result = 0;
	unsigned char	serializedResult[16];
	unsigned char	*cursor;
	uvast		uvtemp;
	int		length;

	memset(&key, 0, sizeof(sci_inbound_tlv));
	context = sci_ctx_init(suite, key, CSI_SVC_SIGN);
	result = csi_sign_res_len(suite, context);
	if (result == 0)
	{
		csi_ctx_free(suite, context);
		return 0;
	}

	if (tlv != 0)
	{
		cursor = serializedResult;
		uvtemp = result;
		oK(cbor_encode_integer(uvtemp, &cursor));
		length = cursor - serializedResult;

		/*	Return value is the length returned by signing
		 *	(the original value of result), plus the 
		 *	encoded length of that length, plus 1 for
		 *	result type.					*/

		result += (length + 1);
	}

	csi_ctx_free(suite, context);
	return result;
}
#endif

/******************************************************************************
 *
 * \par Function Name: bibDefaultSign
 *
 * \par For each of the BIB's target blocks, calculates a digest and
 *      inserts the digest into the results list for that target.
 *
 *
 * \param[in]  suite   - THe ciphersuite to be used for signing.
 * \param[in]  bundle  - The serialized bundle.
 * \param[in]  blk     - The BIB extension block instance.
 * \param[in]  asb     - The abstract security block for the BIB.
 * \param[out] bytes   - Number of bytes signed.
 * \param[out] toEid   - Destination EID (not used).
 *
 * \par Notes:
 *   - The target block MUST NOT be modified after this function is called.
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/06/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  04/26/16  E. Birrane     Added bytes [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bibDefaultSign(uint32_t suite, Bundle *bundle, ExtensionBlock *blk,
		BpsecOutboundBlock *asb, uvast *bytes, char *toEid)
{
	Sdr			sdr = getIonsdr();
	int8_t			retval = 0;
	Object			elt;
	Object			targetObj;
	BpsecOutboundTarget	target;
	sci_inbound_tlv		key;
	uint8_t			*context = NULL;
	uvast			bytesSigned;
	Object			targetZco;
	sci_inbound_tlv		digest;

	BIB_DEBUG_INFO("+ bibDefaultSign(%d, 0x%x, 0x%x, 0x%x", suite,
			(unsigned long) bundle, (unsigned long) blk,
			(unsigned long) asb);

	/* Step 0 - Sanity Checks. */
	CHKERR(bundle && blk && asb && bytes);
	*bytes = 0;			/*	Initialize.		*/

	key = bpsec_retrieveKey(asb->keyName);
	for (elt = sdr_list_first(sdr, asb->targets); elt;
			elt = sdr_list_next(sdr, elt))
	{
		targetObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &target, targetObj,
				sizeof(BpsecOutboundTarget));

		/*	Grab and initialize a crypto context.		*/

		if ((context = sci_ctx_init(suite, key, CSI_SVC_SIGN)) == NULL)
		{
			BIB_DEBUG_ERR("x bibDefaultSign - Can't get context.",
					NULL);
			BIB_DEBUG_PROC("- bibDefaultSign--> NULL", NULL);
			MRELEASE(key.value);
			return ERROR;
		}

		if (sci_sign_start(suite, context) == ERROR)
		{
			BIB_DEBUG_ERR("x bibDefaultSign - Can't start context.",
					NULL);
			csi_ctx_free(suite, context);
			MRELEASE(key.value);
			BIB_DEBUG_PROC("- bibDefaultSign--> NULL", NULL);
			return ERROR;
		}

		bytesSigned = bpsec_canonicalize(bundle,
				target.targetBlockNumber, &targetZco);
		*bytes += bytesSigned;
		retval = bibDefaultCompute(targetZco, csi_blocksize(suite),
				suite, context, CSI_SVC_SIGN);
		zco_destroy(sdr, targetZco);
		if (retval == ERROR)
		{
			BIB_DEBUG_ERR("x bibDefaultSign: Can't compute hash.",
					NULL);
			csi_ctx_free(suite, context);
			MRELEASE(key.value);
			BIB_DEBUG_PROC("- bibDefaultSign--> ERROR", NULL);
			return ERROR;
		}

		if ((sci_sign_finish(suite, context, &digest, CSI_SVC_SIGN))
				== ERROR)
		{
			BIB_DEBUG_ERR("x bibDefaultSign - Can't Finalize.",
					NULL);
			csi_ctx_free(suite, context);
			MRELEASE(key.value);
			BIB_DEBUG_PROC("- bibDefaultSign--> ERROR", NULL);
			return ERROR;
		}

		csi_ctx_free(suite, context);

		/*	Insert the security result.			*/

		digest.id = BPSEC_CSPARM_INT_SIG;
		if (bpsec_appendItem(sdr, target.results, &digest) < 0)
		{
			BIB_DEBUG_ERR("x bibDefaultSign: Can't insert result.",
					NULL);
			MRELEASE(digest.value);
			MRELEASE(key.value);
			BIB_DEBUG_PROC("- bibDefaultSign --> %d", -1);
			return ERROR;
		}

		MRELEASE(digest.value);
	}

	MRELEASE(key.value);
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bibDefaultVerify
 *
 * \par Verify that the signature within a BIB matches the signature computed
 *      by this receiving node.
 *
 * \param[in]    suite - The ciphersuite used for verification.
 * \param[in]      wk  - The received, serialized bundle being verified
 * \param[in\out]  blk - The local extension block being reconstructed.
 * \param[out]    bytes - The number of bytes verified.
 *
 * \par Notes:
 *   - This function must be called AFTER the received bundle has been
 *     stored on the node.
 *
 * \return -1  - System Error
 *          0  - Formatting Failure
 *          1  - Successful Processing
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/05/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  04/26/16  E. Birrane     Added bytes. [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int	bibDefaultVerify(uint32_t suite, AcqWorkArea *wk, AcqExtBlock *blk,
		uvast *bytes, char *fromEid)
{
	BpsecInboundBlock	*asb;
	BpsecInboundTarget	*target;
	sci_inbound_tlv		key;
	sci_inbound_tlv		assertedDigest;
	uint8_t			*context = NULL;
	int8_t			retval = 0;

	BIB_DEBUG_INFO("+ bibDefaultVerify(%d, 0x%x, 0x%x",
			suite, (unsigned long) wk, (unsigned long) blk);

	/* Step 0 - Sanity Checks. */
	CHKERR(wk);
	CHKERR(blk);
	CHKERR(bytes);

	*bytes = 0;
	asb = (BpsecInboundBlock *) (blk->object);

	/*	Fill in missing information in the scratchpad area.	*/

For each target:
	prof = bibGetProfile(fromEid, destinationEid, target->targetBlockType,
			&bibRule)
	memcpy(asb->keyName, bibRule.keyName, BPSEC_KEY_NAME_LEN);






	if (bpsec_getInboundTarget(asb->targets, &target) < 0)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify - Can't get target.", NULL);
		BIB_DEBUG_PROC("- bibDefaultVerify--> NULL", NULL);
		return ERROR;
	}

	/* Step 1 - Compute the security result for the target block. */
	key = bpsec_retrieveKey(asb->keyName);

	/* Step 2 - Grab and initialize a crypto context. */
	if ((context = sci_ctx_init(suite, key, CSI_SVC_VERIFY)) == NULL)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify - Can't get context.", NULL);
		BIB_DEBUG_PROC("- bibDefaultVerify--> NULL", NULL);
		return ERROR;
	}

	if (sci_sign_start(suite, context) == ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify - Can't start context.",
				NULL);
		csi_ctx_free(suite, context);
		BIB_DEBUG_PROC("- bibDefaultVerify--> ERROR", NULL);
		return ERROR;
	}

	switch (target->targetBlockType)
	{
	case PrimaryBlk:
#if 0	//	This code is yet to be developed.
		*bytes = wk->bundle.payload.length;
		retval = bibDefaultCompute(wk->bundle.payload.content,
				csi_blocksize(suite), suite, context,
				CSI_SVC_VERIFY);
#endif
		break;

	case PayloadBlk:
		*bytes = wk->bundle.payload.length;
		retval = bibDefaultCompute(wk->bundle.payload.content,
				csi_blocksize(suite), suite, context,
				CSI_SVC_VERIFY);
		break;

	default:
		BIB_DEBUG_ERR("x bibDefaultVerify: Block type %d \
not supported.", target->targetBlockType);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		BIB_DEBUG_PROC("- bibDefaultVerify--> NULL", NULL);
		return 0;
	}

	MRELEASE(key.value);
	if (retval == ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify: Can't compute hash.", NULL);
		csi_ctx_free(suite, context);
		BIB_DEBUG_PROC("- bibDefaultVerify--> ERROR", NULL);
		return ERROR;
	}

	assertedDigest = sci_extract_tlv(CSI_PARM_INTSIG, target->results);
	if ((retval = sci_sign_finish(suite, context, &assertedDigest,
			CSI_SVC_VERIFY)) != 1)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify - Can't Finalize and \
Verify.", NULL);
	}

	csi_ctx_free(suite, context);
	MRELEASE(assertedDigest.value);

	BIB_DEBUG_PROC("- bibDefaultVerify--> %d", retval);
	return retval;
}

/******************************************************************************
 *
 * \par Function Name: bibGetProfile
 *
 * \par Purpose: Find the profile associated with a potential integrity service.
 *               The integrity security service within a bundle is uniquely
 *               identified as OP(integrity, target), where target is the
 *               identifier of the bundle block receiving the integrity
 *               protection.
 *
 *               The ciphersuite profile captures all function implementations
 *		 associated with the application of integrity for the given
 *		 target block from the given security source to the security
 *		 destination. If no profile exists, then there is no implemen-
 *		 tation of BIB for the given block between the source and
 *		 destination; any security policy rule requiring such an
 *		 implementation has been violated.
 *
 * \retval BibProfile *  NULL  - No profile found.
 *            -          !NULL - The appropriate BIB Profile
 *
 * \param[in]  secSrc     The EID of the node that creates the BIB.
 * \param[in]  secDest    The EID of the node that verifies the BIB.
 * \param[in]  secTgtType The block type of the target block.
 * \param[out] secBibRule The BIB rule capturing security policy.
 *
 * \par Notes:
 *      1. \todo Update to handle target identifier beyond just block type.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/01/15  E. Birrane     Update to profiles, error checks [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

BibProfile	*bibGetProfile(char *securitySource, char *securityDest,
			BpBlockType targetBlkType, BPsecBibRule *bibRule)
{
	Sdr		sdr = getIonsdr();
	Object		ruleAddr;
	Object		ruleElt;
	BibProfile	*prof = NULL;

	BIB_DEBUG_PROC("+ bibGetProfile(%s, %s, %d, 0x%x)",
			(securitySource == NULL) ? "NULL" : securitySource,
			(securityDest == NULL) ? "NULL" : securityDest,
			targetBlkType, (unsigned long) bibRule);

	/* Step 1 - Sanity Checks. */
	CHKNULL(bibRule);

	/* Step 2 - Find the BIB Rule capturing policy */
	sec_get_bpsecBibRule(securitySource, securityDest, &targetBlkType,
			&ruleAddr, &ruleElt);

	if (ruleElt == 0)	/*	No matching rule.		*/
	{
		memset((char *) bibRule, 0, sizeof(BPsecBibRule));
		BIB_DEBUG_INFO("i bibGetProfile: No rule found for BIBs. \
No BIB processing for this bundle.", NULL);
		return NULL;
	}

	/*	Given applicable BIB rule, get the ciphersuite profile.	*/

	sdr_read(sdr, (char *) bibRule, ruleAddr, sizeof(BPsecBibRule));
	prof = get_bib_prof_by_name(bibRule->ciphersuiteName);
	if (prof == NULL)
	{
		BIB_DEBUG_INFO("i bibGetProfile: Profile of BIB rule is \
unknown '%s'.  No BIB processing for this bundle.", bibRule->ciphersuiteName);
	}

	BIB_DEBUG_PROC("- bibGetProfile -> 0x%x", (unsigned long) prof);

	return prof;
}

/******************************************************************************
 *
 * \par Function Name: bibOffer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		         a BIB for the block(s) identified by the tag
 * 		         value loaded into the proposed-block structure.
 * 		         If the bundle already contains such a BIB
 * 		         (inserted by an upstream node) then the
 * 		         function simply returns 0.  Otherwise the
 * 		         function creates a BIB for the block(s)
 * 		         identified by the tag.  If the target block
 * 		         is the payload block, then the BIB is fully
 * 		         constructed at this time (because the final
 * 		         content of the payload block is complete).
 * 		         Otherwise, only a placeholder BIB is constructed;
 * 		         in effect, the placeholder BIB signals to later
 * 		         processing that such a BIB may or may not need
 * 		         to be attached to the bundle, depending on the
 * 		         final contents of other bundle blocks.
 *
 * \retval int 0 - The BIB was successfully created, or not needed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this new block.
 *
 * \par Assumptions:
 *      1. The blk has been pre-initialized with correct block type (BIB)
 *         and a tag value that identifies the target of the proposed BIB.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *      2. The target block must not have its content changed after this
 *         point.  Note that if the target block is both signed and
 *         encrypted, the bpsec standard requires that the target block
 *         must be decrypted before its integrity can be checked (in fact,
 *         before any operation at all can be performed on the target
 *         block).  So the target block's BIB must be computed on the
 *         clear text of the target block, i.e., the BIB must be
 *         constructed before the BCB is constructed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/04/15  E. Birrane     Update to profiles, Target Blocks, error checks
 *                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bibOffer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr			sdr = getIonsdr();
	int			memIdx = getIonMemoryMgr();
	ExtensionTargetScope	*scope;
	Lyst			targets;
	char			*eidString;
	char			destinationEid[MAX_EID_LEN];
	char			*sourceEid;
	uint16_t		contextId = 0;
	int			i;
	ExtensionTargetSpec	*targetSpec;
	BPsecBibRule		bibRule;
	Object			targetElt;
				OBJ_POINTER(ExtensionBlock, target);
	unsigned char		blkNumber;
	BpsecOutboundBlock	asb;
	LystElt			elt;
	LystElt			nextElt;
	int8_t			result = 0;

	/*	Block must be offered as a placeholder to enable
	 *	later extension block processing.			*/

	BIB_DEBUG_PROC("+ bibOffer(0x%x, 0x%x)",
                  (unsigned long) blk, (unsigned long) bundle);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure we have parameters...*/
	CHKERR(blk);
	CHKERR(bundle);

	blk->length = 0;	/*	Default.			*/
	blk->bytes = 0;		/*	Default.			*/

	if (blk->tag < 0 || blk->tag >= targetScopesCount)
	{
		/*	Invalid tag in BIB specification.		*/
		BIB_DEBUG_ERR("x bibOffer - BIB tag %d not valid", blk->tag);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BIB_DEBUG_PROC("- bibOffer -> %d", result);
		return result;
	}

	scope = targetScopes + blk->tag;
	readEid(&(bundle->destination), &eidString);
	CHKERR(eidString);
	istrcpy(destinationEid, eidString, sizeof(destinationEid));
	MRELEASE(eidString);
	sourceEid = bpsec_getLocalAdminEid(destinationEid);
	CHKERR(sourceEid);
	targets = lyst_create_using(memIdx);
	CHKERR(targets);

	/* Step 1.2 - Identify the target(s)				*/

	for (i = 0; i < scope->count; i++)
	{
		targetSpec = scope->targets + i;
		if (targetSpec->blockType == BlockIntegrityBlk
		|| targetSpec->blockType == BlockConfidentialityBlk)
		{
			/*	Can't have a BIB for such a block.	*/
			BIB_DEBUG_WARN("w bibOffer - BIB can't target type %d",
					targetSpec->blockType);
			continue;
		}

		prof = bibGetProfile(sourceEid, destinationEid,
				targetSpec->blockType, &bibRule);
		if (prof == NULL)
		{
			if (bibRule.destEid == 0)	/*	No rule.*/
			{
				BIB_DEBUG_WARN("w bibOffer - No rule for %d",
					targetSpec->blockType);
				continue;
			}

			/*	Found rule but no profile.		*/

			BIB_DEBUG_WARN("w bibOffer - No profile for %d",
					targetSpec->blockType);
			continue;
		}

		if (contextId == 0)
		{
			contextId = prof->profNbr;
		}
		else
		{
			if (prof->profNbr != contextId)
			{
				BIB_DEBUG_WARN("w bibOffer - profile conflict \
for %d", targetSpec->blockType);
				continue;
			}
		}

		targetBlockElt = findExtensionBlock(bundle,
				targetSpec->blockType, targetSpec->tag);
		if (targetBlockElt == 0)
		{
			BIB_DEBUG_WARN("i bibOffer - BIB target %d:%d unknown",
					targetSpec->blockType, targetSpec->tag);
			continue;
		}

		GET_OBJ_POINTER(sdr, ExtensionBlock, target,
				sdr_list_data(sdr, targetBlockElt));
		blkNumber = target->number;
		if (bpsec_findBlock(bundle, BlockIntegrityBlk, blkNumber))
		{
			/*	A BIB for this target already exists.	*/
			BIB_DEBUG_WARN("i bibOffer - BIB already exists for %d",
					blkNumber);
			continue;
		}

		if (!(lyst_insert_last(targets, (void *) blkNumber)))
		{
			putErrmsg("Can't insert BIB target into list.", NULL);
			lyst_destroy(targets);
			blk->size = 0;
			blk->object = 0;
			result = -1;
			BIB_DEBUG_PROC("- bibOffer -> %d", result);
			return result;
		}
	}

	/* Step 2 - Initialize BIB structures. */

	/* Step 2.1 - Populate the BIB ASB. */

	memset((char *) &asb, 0, sizeof(BpsecOutboundBlock));
	CHKERR(sdr_begin_xn(sdr));
	bpsec_insertSecuritySource(bundle, &asb);
	asb.contextId = contextId;
	memcpy(asb.keyName, bibRule.keyName, BPSEC_KEY_NAME_LEN);
	asb.targets = sdr_list_create(sdr);
	asb.parmsData = sdr_list_create(sdr);
	if (asb.targets == 0 || asb.parmsData == 0)
	{
		sdr_cancel_xn(sdr);
		BIB_DEBUG_ERR("x bibOffer: Failed to initialize BIB ASB.",
				NULL);
		lyst_destroy(targets);
		result = -1;
		BIB_DEBUG_PROC("- bibOffer -> %d", result);
		return result;
	}

	for (elt = lyst_first(targets); elt; elt = lyst_next(elt))
	{
		blkNumber = lyst_data(elt);
		if (bpsec_insert_target(sdr, &asb, blkNumber) < 0)
		{
			sdr_cancel_xn(sdr);
			BIB_DEBUG_ERR("x bibOffer: Failed to insert target.",
					NULL);
			lyst_destroy(targets);
			result = -1;
			BIB_DEBUG_PROC("- bibOffer -> %d", result);
			return result;
		}
	}

	lyst_destroy(targets);

	/* Step 2.2 Populate the BIB Extension Block. */

	blk->size = sizeof(BpsecOutboundBlock);
	if ((blk->object = sdr_malloc(sdr, blk->size)) == 0)
	{
		sdr_cancel_xn(sdr);
		BIB_DEBUG_ERR("x bibOffer: Failed to SDR allocate object of \
size %d bytes", blk->size);
		result = -1;
		BIB_DEBUG_PROC("- bibOffer -> %d", result);
		return result;
	}

	sdr_write(sdr, blk->object, (char *) &asb, blk->size);

	/*	That's all we can do for now, as we can't assume
	 *	that all of the BIB's target blocks exist in final
	 *	form yet.  All we do is tell the BP agent that we
	 *	want this BIB to be considered for attachment at
	 *	the time the bundle is dequeued for transmission.
	 *	For this purpose, we stop after initializing the
	 *	block's scratchpad area (loading the ASB into it),
	 *	resulting in insertion of a placeholder BIB.		*/

	result = sdr_end_xn(sdr);
	BIB_DEBUG_PROC("- bibOffer -> %d", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bibReview
 *
 * \par Purpose: This callback is called once for each acquired bundle.
 *		 It scans through all BIB security rules and ensures
		 that each required BIB is included among the bundle's
		 extension blocks.
 *
 * \retval int -- 1 - All required BIBs are present.
 *                0 - At least one required BIB is missing.
 *               -1 - There was a system error.
 *
 * \param[in]      wk   The work area associated with this bundle acquisition.
 *
 * \par Notes:
 *****************************************************************************/

int	bibReview(AcqWorkArea *wk)
{
	Sdr	sdr = getIonsdr();
	int	result = 1;	/*	Default: no problem.		*/
	Bundle	*bundle;
	char	*destinationEid;
	char	*sourceEid;
	Object	rules;
	Object	elt;
	Object	ruleAddr;
		OBJ_POINTER(BPsecBibRule, rule);
	char	ruleDestinationEid[SDRSTRING_BUFSZ];
	char	ruleSourceEid[SDRSTRING_BUFSZ];

	BIB_DEBUG_PROC("+ bibReview(%x)", (unsigned long) wk);

	CHKERR(wk);
	rules = sec_get_bpsecBibRuleList();
	if (rules == 0)
	{
		BIB_DEBUG_PROC("- bibReview -> no security database", NULL);
		return result;
	}

	bundle = &(wk->bundle);
	readEid(&(bundle->destination), &destinationEid);
	if (destinationEid == NULL)
	{
		BIB_DEBUG_PROC("- bibReview -> no bundle destination", NULL);
		return -1;
	}

	readEid(&(bundle->id.source), &sourceEid);
	if (sourceEid == NULL)
	{
		BIB_DEBUG_PROC("- bibReview -> no bundle source", NULL);
		MRELEASE(destinationEid);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BPsecBibRule, rule, ruleAddr);
		oK(sdr_string_read(sdr, ruleDestinationEid, rule->destEid));
		if (eidsMatch(ruleDestinationEid, strlen(ruleDestinationEid),
				destinationEid, strlen(destinationEid)) != 1)
		{
			/*	Rule n/a for bundle's destination.	*/

			continue;
		}

		oK(sdr_string_read(sdr, ruleSourceEid, rule->securitySrcEid));
		if (eidsMatch(ruleSourceEid, strlen(ruleSourceEid),
				sourceEid, strlen(sourceEid)) != 1)
		{
			/*	Rule n/a for bundle's source.		*/

			continue;
		}

		/*	Rule applies to this bundle.  A block that
		 *	satisfies this rule is required.		*/

		result = bpsec_requiredBlockExists(wk, BlockIntegrityBlk,
				rule->blockType, ruleSourceEid);
		if (result != 1)
		{
			break;
		}
	}

	MRELEASE(destinationEid);
	MRELEASE(sourceEid);
	sdr_exit_xn(sdr);

	BIB_DEBUG_PROC("- bibReview -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bibParse
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

int	bibParse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BIB_DEBUG_PROC("+ bibParse(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);

	result = bpsec_deserializeASB(blk, wk);
	BIB_DEBUG_INFO("i bibParse: Deserialize result %d", result);

	BIB_DEBUG_PROC("- bibParse -> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: bibProcessOnDequeue
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
 *         implementation of bpsec will assume that the target block must be
 *         decrypted before its integrity can be checked (in fact, before any
 *         operation at all can be performed on the target block).  So the
 *         target block's BIB must be computed on the clear text of the
 *         target block, i.e., the BIB must be constructed before the BCB is
 *         constructed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *  11/04/15  E. Birrane     Update to profiles, Target Blocks, error checks
 *                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bibProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *parm)
{
	Sdr			sdr = getIonsdr();
	BpsecOutboundBlock	asb;
	int8_t		    	result = 0;

	BIB_DEBUG_PROC("+ bibProcessOnDequeue(%x, %x, %x)",
			       (unsigned long) blk, (unsigned long) bundle,
			       (unsigned long) parm);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure arguments are valid. */
	if (bundle == NULL || parm == NULL || blk == NULL)
	{
		BIB_DEBUG_ERR("x bibProcessOnDequeue: Bad Args.", NULL);
		BIB_DEBUG_INFO("i bibProcessOnDequeue bundle %d, parm %d, \
blk %d, blk->size %d", (unsigned long) bundle, (unsigned long) parm,
			(unsigned long) blk, (blk == NULL) ? 0 : blk->size);
		BIB_DEBUG_PROC("- bibProcessOnDequeue --> %d", -1);
		scratchExtensionBlock(blk);
		return -1;
	}

	/* Step 1.1.1 - If block was received from elsewhere, nothing
	 * to do; it's already attached to the bundle.			*/

	if (blk->bytes)
	{
		BIB_DEBUG_PROC("- bibProcessOnDequeue(%d) no-op", result);
		return 0;
	}

	/*
	 * Step 2 - Calculate the digests for the BIB's targets and
	 *          attach the BIB to the bundle.
	 */

	sdr_read(sdr, (char *) &asb, blk->object, blk->size);
	result = bibAttach(bundle, blk, &asb);
	BIB_DEBUG_PROC("- bibProcessOnDequeue(%d)", result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: bibRelease
 *
 * \par Purpose: This callback releases SDR heap space allocated to
 * 		 a block integrity block.
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated SDR heap space must be
 *                      released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *****************************************************************************/

void    bibRelease(ExtensionBlock *blk)
{
	BIB_DEBUG_PROC("+ bibRelease(%x)", (unsigned long) blk);

	CHKVOID(blk);
	bpsec_releaseOutboundAsb(getIonsdr(), blk->object);
	BIB_DEBUG_PROC("- bibRelease(%c)", ' ');
}
