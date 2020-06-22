/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: sbsp_bib.c
 **
 ** Description: Definitions supporting generic processing of BIB blocks.
 **              This includes both the BIB Interface to the ION SBSP
 **              API as well as a default implementation of the BIB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BIB SBSP Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              sbsp_bibOffer
 **              sbsp_bibProcessOnDequeue
 **              sbsp_bibRelease
 **              sbsp_bibCopy
 **                                                  sbsp_bibReview
 **                                                  sbsp_bibParse
 **                                                  sbsp_bibCheck
 **                                                  sbsp_bibRecord
 **                                                  sbsp_bibClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    SIGN SIDE                     VERIFY SIDE
 **
 **              sbsp_bibDefaultConstruct
 **              sbsp_bibDefaultSign
 **
 **                                              sbsp_bibDefaultConstruct
 **                                              sbsp_bibDefaultSign
 **                                              sbsp_bibDefaultVerify
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
 **            S. Burleigh    Implementation as sbspbib.c for Ssbsp
 **  11/02/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
#include "sbsp_bib.h"
#include "csi.h"
#include "sbsp_instr.h"

#if (BIB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif


/******************************************************************************
 *
 * \par Function Name: sbsp_bibAttach
 *
 * \par Purpose: Construct, compute, and attach a BIB block within the bundle.
 *               This function determines, through a policy lookup, whether
 *               a BIB should be applied to a particular block in a bundle and,
 *               if so, constructs the appropriate BIB block, using the
 *               appropriate ciphersuite, and generates a serialized version of
 *               the block appropriate for transmission.
 * *
 * \retval int -1  - Error.
 *              0  - No BIB Policy
 *             >0  - BIB Attached
 *
 * \param[in|out]  bundle  The bundle to which a BIB might be attached.
 * \param[out]     bibBlk  The serialized BIB extension block.
 * \param[out]     bibAsb  The ASB for this BIB.
 *
 * \todo Update to handle target identifier beyond just block type.
 * \todo This function assumes bib asb and blk are partially initialized
 *       by other functions. Clean up/document this, removing
 *       such assumptions where practical.
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

static int	sbsp_bibAttach(Bundle *bundle, ExtensionBlock *bibBlk,
			SbspOutboundBlock *bibAsb)
{
	int8_t		result = 0;
	char		*fromEid;
	char		*toEid;
	char		eidBuf[32];
	BspBibRule	bibRule;
	BibProfile	*prof;
	uint8_t		*serializedAsb;
	uvast		bytes = 0;

	BIB_DEBUG_PROC("+ sbsp_bibAttach(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC
			"," ADDR_FIELDSPEC ")", (uaddr) bundle, (uaddr) bibBlk,
			(uaddr) bibAsb);

	/* Step 0 - Sanity checks. */
	CHKERR(bundle);
	CHKERR(bibBlk);
	CHKERR(bibAsb);

	/* Step 1 -	Grab Policy for the candidate block. 		*/

	/* Step 1.1 -   Retrieve the from/to EIDs that bound the
			integrity service.				*/

	if (sbsp_getOutboundSecurityEids(bundle, bibBlk, bibAsb, &fromEid,
			&toEid) <= 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibAttach: Can't get security EIDs.",
				NULL);
		result = -1;
		BIB_DEBUG_PROC("- sbsp_bibAttach -> %d", result);
		return result;
	}

	/*	We only attach a BIB per a rule for which the local
		node is the security source.				*/

	MRELEASE(fromEid);
	isprintf(eidBuf, sizeof eidBuf, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	fromEid = eidBuf;

	/*
	 * Step 1.2 -	Grab the profile for integrity for the target
	 *		block from the EIDs. If there is no rule,
	 *		then there is no policy for attaching BIBs
	 *		in this instance.  If there is a rule but
	 *		no matching profile then the bundle must not
	 *		be forwarded.
	 */

	prof = sbsp_bibGetProfile(fromEid, toEid, bibAsb->targetBlockType,
			&bibRule);
	MRELEASE(toEid);
	if (prof == NULL)
	{
		if (bibRule.destEid == 0)	/*	No rule.	*/
		{
			BIB_DEBUG(2, "NOT Attaching BIB; no rule.", NULL);

			/*	No applicable valid construction rule.	*/

			result = 0;
			scratchExtensionBlock(bibBlk);
			BIB_DEBUG_PROC("- sbsp_bibAttach -> %d", result);
			return result;
		}

		BIB_DEBUG(2, "NOT Attaching BIB; no profile.", NULL);

		/*	No applicable ciphersuite profile.		*/

		result = 0;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- sbsp_bibAttach -> %d", result);
		return result;
	}

	BIB_DEBUG(2, "Attaching BIB.", NULL);

	/* Step 2 - Populate the BIB ASB. */

	/* Step 2.1 - Grab the key name for this operation. */

	memcpy(bibAsb->keyName, bibRule.keyName, SBSP_KEY_NAME_LEN);

	/* Step 2.2 - Initialize the BIB ASB. */

	result = (prof->construct == NULL) ?
			sbsp_bibDefaultConstruct(prof->suiteId, bibBlk, bibAsb)
			: prof->construct(bibBlk, bibAsb);
	if (result < 0)
	{
		ADD_BIB_TX_FAIL(fromEid, 1, 0);

		BIB_DEBUG_ERR("x sbsp_bibAttach: Can't construct ASB.", NULL);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- sbsp_bibAttach --> %d", result);
		return result;
	}

	/* Step 2.2 - Sign the target block and store the signature. */

	result = (prof->sign == NULL) ?
			sbsp_bibDefaultSign(prof->suiteId, bundle, bibBlk,
			bibAsb, &bytes)
			: prof->sign(bundle, bibBlk, bibAsb, &bytes);

	if (result < 0)
	{
		ADD_BIB_TX_FAIL(fromEid, 1, bytes);

		BIB_DEBUG_ERR("x sbsp_bibAttach: Can't sign target block.",
				NULL);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- sbsp_bibAttach --> %d", result);
		return result;
	}

	/* Step 3 - serialize the BIB ASB into the BIB blk. */

	/* Step 3.1 - Create a serialized version of the BIB ASB. */

	if ((serializedAsb = sbsp_serializeASB((uint32_t *)
			&(bibBlk->dataLength), bibAsb)) == NULL)
	{
		ADD_BIB_TX_FAIL(fromEid, 1, bytes);

		BIB_DEBUG_ERR("x sbsp_bibAttach: Unable to serialize ASB.  \
bibBlk->dataLength = %d", bibBlk->dataLength);
		result = -1;
		bundle->corrupt = 1;
		scratchExtensionBlock(bibBlk);
		BIB_DEBUG_PROC("- sbsp_bibAttach --> %d", result);
		return result;
	}

	/* Step 3.2 - Copy serializedBIB ASB into the BIB extension block. */

	if ((result = serializeExtBlk(bibBlk, NULL, (char *) serializedAsb))
			< 0)
	{
		bundle->corrupt = 1;
	}

	MRELEASE(serializedAsb);

	ADD_BIB_TX_PASS(fromEid, 1, bytes);

	BIB_DEBUG_PROC("- sbsp_bibAttach --> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibCheck
 *
 * \par Purpose: This callback determine whether or not a BIB's security
 * 		 target block has been corrupted end route.  Specifically,
 * 		 a newly computed hash for the security target block
 * 		 must match the security result encoded in the BIB.
 *
 * \retval   0 - The block was not found to be corrupt.
 *           3 - The block was found to be corrupt.
 *          -1 - There was a system error.
 *
 * \param[in]  blk  The BIB whose target must be checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *****************************************************************************/

int sbsp_bibCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
	Bundle			*bundle;
	char			*dictionary;
	SbspInboundBlock	*asb = NULL;
	char			*fromEid;
	char			*toEid;
	BspBibRule		bibRule;
	BibProfile		*prof;
	int8_t			result;
	uvast			bytes = 0;

	BIB_DEBUG_PROC("+ sbsp_bibCheck(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	if (blk == NULL || blk->object == NULL || wk == NULL)
	{
		BIB_DEBUG_ERR("x sbsp_bibCheck:  Blocks are NULL. %x",
				(unsigned long) blk);
		BIB_DEBUG_PROC("- sbsp_bibCheck --> %d", -1);
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
		ADD_BIB_RX_FAIL(NULL, 1, 0);
		return -1;
	}

	asb = (SbspInboundBlock *) (blk->object);
	if (asb->securitySource.unicast)	/*	Waypoint source.*/
	{
		if (printEid(&(asb->securitySource), dictionary, &fromEid) < 0)
		{
			ADD_BIB_RX_FAIL(NULL, 1, 0);

			releaseDictionary(dictionary);
			return -1;
		}
	}
	else
	{
		if (printEid(&(bundle->id.source), dictionary, &fromEid) < 0)
		{
			ADD_BIB_RX_FAIL(NULL, 1, 0);
			releaseDictionary(dictionary);
			return -1;
		}
	}

	if (printEid(&(bundle->destination), dictionary, &toEid) < 0)
	{
		ADD_BIB_RX_FAIL(fromEid, 1, 0);
		MRELEASE(fromEid);
		releaseDictionary(dictionary);
		return -1;
	}

	releaseDictionary(dictionary);

	/*	Given sender & receiver EIDs, get applicable BIB rule.	*/

	prof = sbsp_bibGetProfile(fromEid, toEid, asb->targetBlockType,
			&bibRule);
	MRELEASE(toEid);

	if (prof == NULL)
	{
		/*	We can't verify this signature.			*/

		if (bibRule.destEid == 0)	/*	No rule.	*/
		{
			/*	We don't care about verifying the
			 *	signature on the target block for
			 *	this BIB, but we preserve the BIB
			 *	in case somebody else does.		*/

			ADD_BIB_RX_MISS(fromEid, 1, 0);
			MRELEASE(fromEid);

			BIB_DEBUG_INFO("- sbsp_bibCheck - No rule.", NULL);
			BIB_DEBUG_PROC("- sbsp_bibCheck", NULL);
			blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
			return 1;		/*	No information.	*/
		}

		/*	Rule is found, but we don't have this CS.
		 *	We have to conclude that the BIB is invalid,
		 *	in which case the bundle is judged corrupt.	*/

		ADD_BIB_RX_FAIL(fromEid, 1, 0);
		MRELEASE(fromEid);

		discardExtensionBlock(blk);
	 	BIB_DEBUG_ERR("- sbsp_bibCheck - Profile missing!", NULL);
		BIB_DEBUG_PROC("- sbsp_bibCheck", NULL);
		return 0;			/*	Bundle corrupt.	*/
	}

	/*	Fill in missing information in the scratchpad area.	*/

	memcpy(asb->keyName, bibRule.keyName, SBSP_KEY_NAME_LEN);

	/*	Invoke ciphersuite-specific check procedure.		*/

	result = (prof->verify == NULL) ?
			sbsp_bibDefaultVerify(prof->suiteId, wk, blk, &bytes)
			: prof->verify(wk, blk, &bytes);

	/*	Discard the BIB if the local node is the destination
	 *	of the bundle or the BIB is invalid or verification
	 *	failed (meaning the block is altered and therefore
	 *	the bundle is altered); otherwise make sure the BIB
	 *	is retained.						*/

	BIB_DEBUG_INFO("i sbsp_bibCheck: Verify result was %d", result);
	if ((result == 0) || (result == 4))
	{
		ADD_BIB_RX_FAIL(fromEid, 1, bytes);
		discardExtensionBlock(blk);
	}
	else if (sbsp_destinationIsLocal(&(wk->bundle)))
	{
		BIB_DEBUG(2, "BIB check passed.", NULL);
		ADD_BIB_RX_PASS(fromEid, 1, bytes);
		discardExtensionBlock(blk);
	}
	else
	{
		ADD_BIB_FWD(fromEid, 1, bytes);
		blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
	}

	MRELEASE(fromEid);

	BIB_DEBUG_PROC("- sbsp_bibCheck(%d)", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibClear
 *
 * \par Purpose: This callback removes all memory allocated by the sbsp module
 *               during the block's acquisition process.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the sbsp module.
 *****************************************************************************/

void	sbsp_bibClear(AcqExtBlock *blk)
{
	SbspInboundBlock *asb;

	BIB_DEBUG_PROC("+ sbsp_bibClear(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		asb = (SbspInboundBlock *) (blk->object);
		if (asb->parmsData)
		{
			BIB_DEBUG_INFO("i sbsp_bibClear: Release parms len %ld",
					asb->parmsLen);
			MRELEASE(asb->parmsData);
		}

		if (asb->resultsData)
		{
			BIB_DEBUG_INFO("i sbsp_bibClear: Release result len %ld",
					asb->resultsLen);
			MRELEASE(asb->resultsData);
		}

		BIB_DEBUG_INFO("i sbsp_bibClear: Release ASB len %d", blk->size);

		MRELEASE(blk->object);
		blk->object = NULL;
		blk->size = 0;
	}

	BIB_DEBUG_PROC("- sbsp_bibClear", NULL);

	return;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibCopy
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
 *            S. Burleigh    Port from sbsp_pibCopy
 *  11/04/15  E. Birrane     Comments. [Secure DTN implementation (NASA: NNX14CS58P)]
 *
 *****************************************************************************/

int	sbsp_bibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr			        bpSdr = getIonsdr();
	SbspOutboundBlock	asb;
	int8_t			    result = 0;
	uint8_t            *buffer = NULL;

	BIB_DEBUG_PROC("+ sbsp_bibCopy(0x%x, 0x%x)", (unsigned long) newBlk,
			(unsigned long) oldBlk);

	/* Step 1 - Sanity Checks. */
	CHKERR(newBlk);
	CHKERR(oldBlk);

	/* Step 2 - Allocate the new destination BIB. */
	newBlk->size = sizeof(asb);
	if ((newBlk->object = sdr_malloc(bpSdr, sizeof asb)) == 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibCopy: Failed to allocate: %d",
				sizeof(asb));
		BIB_DEBUG_PROC("- sbsp_bibCopy -> -1", NULL);
		return -1;
	}

	/*  Step 3 - Copy the source BIB into the destination. */

	/* Step 3.1 - Read the source BIB from the SDR. */
	sdr_read(bpSdr, (char *) &asb, oldBlk->object, sizeof(asb));

	/*
	 * Step 3.2 - Copy parameters by reading entire parameter list
	 *            into a buffer and copying that buffer into the
	 *            destination block. Then write the copied parameters
	 *            back to the SDR.
	 */

	if (asb.parmsData)
	{
		if ((buffer = MTAKE(asb.parmsLen)) == NULL)
		{
			BIB_DEBUG_ERR("x sbsp_bibCopy: Failed to allocate: %d",
					      asb.parmsLen);
			sdr_free(bpSdr, newBlk->object);
			newBlk->object = 0;
			return -1;
		}

		sdr_read(bpSdr, (char *) buffer, asb.parmsData, asb.parmsLen);

		if ((asb.parmsData = sdr_malloc(bpSdr, asb.parmsLen)) == 0)
		{
			BIB_DEBUG_ERR("x sbsp_bibCopy: Failed to allocate: %d",
  					      asb.parmsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.parmsData, (char *) buffer, asb.parmsLen);
		MRELEASE(buffer);
	}


	/*
	 * Step 3.3 - Copy results by reading result field
	 *            into a buffer and copying that buffer into the
	 *            destination block. Then write the copied results
	 *            to the SDR.
	 */

	if (asb.resultsData)
	{
		if ((buffer = MTAKE(asb.resultsLen)) == NULL)
		{
			BIB_DEBUG_ERR("x sbsp_bibCopy: Failed to allocate: %d",
					       asb.resultsLen);
			return -1;
		}

		sdr_read(bpSdr, (char *) buffer, asb.resultsData, asb.resultsLen);
		if ((asb.resultsData = sdr_malloc(bpSdr, asb.resultsLen)) == 0)
		{
			BIB_DEBUG_ERR("x sbsp_bibCopy: Failed to allocate: %d",
					asb.resultsLen);
			MRELEASE(buffer);
			return -1;
		}

		sdr_write(bpSdr, asb.resultsData, (char *) buffer, asb.resultsLen);
		MRELEASE(buffer);
	}


	/* Step 4 - Write copied block to the SDR. */
	sdr_write(bpSdr, newBlk->object, (char *) &asb, sizeof asb);

	BIB_DEBUG_PROC("- sbsp_bibCopy -> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibDefaultCompute
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

int sbsp_bibDefaultCompute(Object dataObj,
							   uint32_t chunkSize,
						       uint32_t suite,
						       void *context,
						       csi_svcid_t svc)
{
	Sdr		bpSdr = getIonsdr();
	char		*dataBuffer;
	ZcoReader	dataReader;
	unsigned int	bytesRemaining = 0;
	unsigned int	bytesRetrieved = 0;


	BIB_DEBUG_INFO("+ sbsp_bibDefaultCompute(0x%x, %d, %d, 0x%x)",
			       (unsigned long) dataObj, chunkSize, suite, (unsigned long) context);

	CHKERR(context);

	/* Step 2 - Allocate a working buffer. */
	if ((dataBuffer = MTAKE(chunkSize)) == NULL)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultCompute - Can't allocate buffer of size %d.",
				chunkSize);
		return ERROR;
	}

	/*
	 * Step 3 - Setup playback of data from the data object.
	 *          The data object is the target block.
	 */

	if ((bytesRemaining = zco_length(bpSdr, dataObj)) <= 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultCompute - data object has no length.", NULL);
		MRELEASE(dataBuffer);
		BIB_DEBUG_PROC("- sbsp_bibDefaultCompute--> ERROR", NULL);
		return ERROR;
	}

	BIB_DEBUG_INFO("i sbsp_bibDefaultCompute bytesRemaining: %d", bytesRemaining);

	if ((sdr_begin_xn(bpSdr)) == 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultCompute - Can't start txn.", NULL);
		MRELEASE(dataBuffer);
		BIB_DEBUG_PROC("- sbsp_bibDefaultCompute--> ERROR", NULL);
		return ERROR;
	}

	zco_start_transmitting(dataObj, &dataReader);

	BIB_DEBUG_INFO("i sbsp_bibDefaultCompute: bundle size is %d", bytesRemaining);

	/* Step 5 - Loop through the data in chunks, updating the context. */
	while (bytesRemaining > 0)
	{
		csi_val_t val;

		if (bytesRemaining < chunkSize)
		{
			chunkSize = bytesRemaining;
		}

		bytesRetrieved = zco_transmit(bpSdr, &dataReader, chunkSize,
				                      dataBuffer);

		if (bytesRetrieved != chunkSize)
		{
			BIB_DEBUG_ERR("x sbsp_bibDefaultCompute: Read %d bytes, but \
                          expected %d.", bytesRetrieved, chunkSize);
			sdr_exit_xn(bpSdr);
			MRELEASE(dataBuffer);

			BIB_DEBUG_PROC("- sbsp_bibDefaultCompute--> ERROR", NULL);
			return ERROR;
		}

		/*	Add the data to the context.		*/
		val.contents = (uint8_t *) dataBuffer;
		val.len = chunkSize;
		csi_sign_update(suite, context, val, svc);

		bytesRemaining -= bytesRetrieved;
	}

	sdr_exit_xn(bpSdr);
	MRELEASE(dataBuffer);

	return 1;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibDefaultConstruct
 *
 * \par Given a pre-allocated extension block and abstract security block,
 *      initialize the abstract security block for subsequent processing.
 *
 * \param[in]   suite  The ciphersuite used by this profile.
 * \param[in]     blk  The pre-allocated extension block.
 * \param[in/out] asb  The abstract security block being initialized.
 *
 * \par Notes:
 *  - The ASB will be attached to the extension block and passed back
 *    to another function (sign) to help create the contents of the
 *    serialized extension block.
 *
 *  - It is assumed that other functions populate non-ciphersuite related
 *    portions of the ASB, such as the security source, target block info,
 *    ASB instance, and key information.
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/05/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int sbsp_bibDefaultConstruct(uint32_t suite, ExtensionBlock *blk, SbspOutboundBlock *asb)
{

	CHKERR(blk);
	CHKERR(asb);

	/* Step 1: Populate block-instance-agnostic parts of the ASB. */
	asb->ciphersuiteType = suite;
	asb->parmsLen = 0;
	asb->parmsData = 0;
	asb->resultsData = 0;

	/* Step 2: Populate instance-specific parts of the ASB. */
#if 0
	asb->ciphersuiteFlags = SBSP_ASB_RES;
#endif
	asb->ciphersuiteFlags = 0;
	asb->resultsLen = sbsp_bibDefaultResultLen(asb->ciphersuiteType, 1);

	return 0;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibDefaultResultLen
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
 *  02/27/16  E. Birrane     Update to CSI Interface [Secure DNULL, NULL, CSI_SVC_SIGNTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t sbsp_bibDefaultResultLen(uint32_t suite, uint8_t tlv)
{
	void *context = NULL;
	csi_val_t key;
	uint32_t result = 0;

	memset(&key, 0, sizeof(csi_val_t));

	context = csi_ctx_init(suite, key, CSI_SVC_SIGN);
	result = csi_sign_res_len(suite, context);

	if (result == 0)
	{
		csi_ctx_free(suite, context);
		return 0;
	}

	if (tlv != 0)
	{
		Sdnv resultSdnv;
		encodeSdnv(&resultSdnv, result);

		/* Add 1 byte for the "type" and N bytes for the Length as an SDNV. */
		result += resultSdnv.length + 1;
	}

	csi_ctx_free(suite, context);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibSign
 *
 * \par Calculates a digest for the given block and adds it to the provided
 *      abstract security block and also provides a serialized version of
 *      the BIB.
 *
 * \param[in]  suite   - THe ciphersuite to be used for signing.
 * \param[in]  bundle  - The serialized bundle.
 * \param[in]  blk     - The BIB extension block instance.
 * \param[in]  asb     - The abstract security block for the BIB.
 * \param[out] bytes   - Number of bytes signed.
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

int sbsp_bibDefaultSign(uint32_t suite,
     	                      Bundle *bundle,
		                      ExtensionBlock *blk,
		                      SbspOutboundBlock *asb,
							  uvast *bytes)
{
	Sdr		bpSdr = getIonsdr();
	int8_t retval = 0;
	csi_val_t key;
	csi_val_t digest;
	Sdnv		digestSdnv;
	int		resultsLen;
	unsigned char	*temp;
	uint8_t *context = NULL;

	BIB_DEBUG_INFO("+ sbsp_bibDefaultSign(%d, 0x%x, 0x%x, 0x%x",
			       suite, (unsigned long) bundle, (unsigned long) blk,
				   (unsigned long) asb);

	/* Step 0 - Sanity Checks. */
	CHKERR(bundle);
	CHKERR(blk);
	CHKERR(asb);
	CHKERR(bytes);

	*bytes = 0;

	/* Step 1 - Compute the security result for the target block. */
	key = sbsp_retrieveKey(asb->keyName);

	/* Step 2 - Grab and initialize a crypto context. */
	if ((context = csi_ctx_init(suite, key, CSI_SVC_SIGN)) == NULL)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultSign - Can't get context.", NULL);
		BIB_DEBUG_PROC("- sbsp_bibDefaultSign--> NULL", NULL);
		return ERROR;
	}

	if (csi_sign_start(suite, context) == ERROR)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultSign - Can't start context.", NULL);
		csi_ctx_free(suite, context);
		BIB_DEBUG_PROC("- sbsp_bibDefaultSign--> NULL", NULL);
		return ERROR;
	}

	switch (asb->targetBlockType)
	{
		case BLOCK_TYPE_PAYLOAD:
			*bytes = bundle->payload.length;
			retval = sbsp_bibDefaultCompute(bundle->payload.content,
									  csi_blocksize(suite),
									  suite,
									  context,
									  CSI_SVC_SIGN);
		break;

		case BLOCK_TYPE_PRIMARY:
		default:
			BIB_DEBUG_ERR("x sbsp_bibDefaultSign: Block type %d not supported.",
					      asb->targetBlockType);
			MRELEASE(key.contents);
			csi_ctx_free(suite, context);
			BIB_DEBUG_PROC("- sbsp_bibDefaultSign--> NULL", NULL);
			return 0;
	}

	MRELEASE(key.contents);

	if (retval == ERROR)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultSign: Can't compute hash.", NULL);
		csi_ctx_free(suite, context);
		BIB_DEBUG_PROC("- sbsp_bibDefaultSign--> ERROR", NULL);
		return ERROR;
	}

	if ((csi_sign_finish(suite, context, &digest, CSI_SVC_SIGN)) == ERROR)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultSign - Can't Finalize.", NULL);
		csi_ctx_free(suite, context);

		BIB_DEBUG_PROC("- sbsp_bibDefaultSign--> ERROR", NULL);
		return ERROR;
	}

	csi_ctx_free(suite, context);

	/* Step 2 - Build the security result. */

	/* Step 2.1 - Encode digest length as an SDNV. */
	encodeSdnv(&digestSdnv, digest.len);

	/* Step 2.2 Allocate space for the encoded result TLV */
	resultsLen = 1 + digestSdnv.length + digest.len;
	if ((temp = (unsigned char *) MTAKE(resultsLen)) == NULL)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultSign: Can't allocate result of len %ld.", resultsLen);
		MRELEASE(digest.contents);
		BIB_DEBUG_PROC("- sbsp_bibDefaultSign --> %d", -1);
		return -1;
	}

	/* Step 2.3 Populate the result TLV. */
	*temp = SBSP_CSPARM_INT_SIG;
	memcpy(temp + 1, digestSdnv.text, digestSdnv.length);
	memcpy(temp + 1 + digestSdnv.length, digest.contents, digest.len);
	MRELEASE(digest.contents);

	/* Step 3 - Store the security result in the ASB and on the SDR. */
	asb->resultsLen = resultsLen;
	if ((asb->resultsData = sdr_malloc(bpSdr, resultsLen)) == 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultSign: Can't allocate heap \
space for ASB result, len %ld.", resultsLen);
		MRELEASE(temp);
		BIB_DEBUG_PROC("- bib_hmac_sha256_sign --> %d", -1);
		return -1;
	}

	sdr_write(bpSdr, asb->resultsData, (char *) temp, resultsLen);
	MRELEASE(temp);


	return 0;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibDefaultVerify
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
int sbsp_bibDefaultVerify(uint32_t suite,
		                     AcqWorkArea *wk,
							 AcqExtBlock *blk,
							 uvast *bytes)
{
	SbspInboundBlock	*asb;
	csi_val_t key;
	csi_val_t assertedDigest;
	uint8_t *context = NULL;
	int8_t retval = 0;

	BIB_DEBUG_INFO("+ sbsp_bibDefaultVerify(%d, 0x%x, 0x%x",
			       suite, (unsigned long) wk, (unsigned long) blk);

	/* Step 0 - Sanity Checks. */
	CHKERR(wk);
	CHKERR(blk);
	CHKERR(bytes);

	*bytes = 0;
	asb = (SbspInboundBlock *) (blk->object);

#if 0
	if ((asb->ciphersuiteFlags & SBSP_ASB_RES) == 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultVerify: No security result.", NULL);
		return 0;
	}
#endif

	/* Step 1 - Compute the security result for the target block. */
	key = sbsp_retrieveKey(asb->keyName);

	/* Step 2 - Grab and initialize a crypto context. */
	if ((context = csi_ctx_init(suite, key, CSI_SVC_VERIFY)) == NULL)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultVerify - Can't get context.", NULL);
		BIB_DEBUG_PROC("- sbsp_bibDefaultVerify--> NULL", NULL);
		return ERROR;
	}

	if (csi_sign_start(suite, context) == ERROR)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultVerify - Can't start context.", NULL);
		csi_ctx_free(suite, context);
		BIB_DEBUG_PROC("- sbsp_bibDefaultVerify--> ERROR", NULL);
		return ERROR;
	}


	switch (asb->targetBlockType)
	{
		case BLOCK_TYPE_PAYLOAD:
			*bytes = wk->bundle.payload.length;

			retval = sbsp_bibDefaultCompute(wk->bundle.payload.content,
									  csi_blocksize(suite),
									  suite,
									  context,
  									  CSI_SVC_VERIFY);
		break;

		case BLOCK_TYPE_PRIMARY:
		default:
			BIB_DEBUG_ERR("x sbsp_bibDefaultVerify: Block type %d not supported.",
					      asb->targetBlockType);
			csi_ctx_free(suite, context);
			MRELEASE(key.contents);
			BIB_DEBUG_PROC("- sbsp_bibDefaultVerify--> NULL", NULL);
			return 0;
	}

	MRELEASE(key.contents);


	if (retval == ERROR)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultVerify: Can't compute hash.", NULL);
		csi_ctx_free(suite, context);
		BIB_DEBUG_PROC("- sbsp_bibDefaultVerify--> ERROR", NULL);
		return ERROR;
	}


	assertedDigest = csi_extract_tlv(CSI_PARM_INTSIG, asb->resultsData, asb->resultsLen);

	if ((retval = csi_sign_finish(suite, context, &assertedDigest, CSI_SVC_VERIFY)) != 1)
	{
		BIB_DEBUG_ERR("x sbsp_bibDefaultVerify - Can't Finalize and Verify.", NULL);
	}

	csi_ctx_free(suite, context);
	MRELEASE(assertedDigest.contents);

	BIB_DEBUG_PROC("- sbsp_bibDefaultVerify--> %d", retval);
	return retval;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibGetProfile
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

BibProfile	*sbsp_bibGetProfile(char *securitySource, char *securityDest,
			int8_t targetBlkType, BspBibRule *bibRule)
{
	Sdr		bpSdr = getIonsdr();
	Object		ruleAddr;
	Object		ruleElt;
	BibProfile	*prof = NULL;

	BIB_DEBUG_PROC("+ sbsp_bibGetProfile(%s, %s, %d, 0x%x)",
			(securitySource == NULL) ? "NULL" : securitySource,
			(securityDest == NULL) ? "NULL" : securityDest,
			targetBlkType, (unsigned long) bibRule);

	/* Step 1 - Sanity Checks. */
	CHKNULL(bibRule);

	/* Step 2 - Find the BIB Rule capturing policy */
	sec_get_bspBibRule(securitySource, securityDest, targetBlkType,
			&ruleAddr, &ruleElt);

	if (ruleElt == 0)	/*	No matching rule.		*/
	{
		memset((char *) bibRule, 0, sizeof(BspBibRule));
		BIB_DEBUG_INFO("i sbsp_bibGetProfile: No rule found for BIBs. \
No BIB processing for this bundle.", NULL);
		return NULL;
	}

	/*	Given applicable BIB rule, get the ciphersuite profile.	*/

	sdr_read(bpSdr, (char *) bibRule, ruleAddr, sizeof(BspBibRule));
	prof = get_bib_prof_by_name(bibRule->ciphersuiteName);
	if (prof == NULL)
	{
		BIB_DEBUG_INFO("i sbsp_bibGetProfile: Profile of BIB rule is \
unknown '%s'.  No BIB processing for this bundle.", bibRule->ciphersuiteName);
	}

	BIB_DEBUG_PROC("- sbsp_bibGetProfile -> 0x%x", (unsigned long) prof);

	return prof;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibOffer
 *
 * \par Purpose: This callback aims to ensure that the bundle contains
 * 		         a BIB for the block identified by tag1 of the proposed
 * 		         BIB contents.  If the bundle already contains such a
 * 		         BIB (inserted by an upstream node) then the function
 * 		         simply returns 0.  Otherwise the function creates a
 * 		         BIB for the target block identified by blk->tag1.  If
 * 		         the target block is the payload block, then the BIB is
 * 		         fully constructed at this time (because the final
 * 		         content of the payload block is complete).  Otherwise,
 * 		         only a placeholder BIB is constructed; in effect, the
 * 		         placeholder BIB signals to later processing that such
 * 		         a BIB may or may not need to be attached to the bundle,
 * 		         depending on the final contents of other bundle blocks.
 *
 * \retval int 0 - The BIB was successfully created, or not needed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this new block.
 *
 * \todo Update to be used with any block, not just payload.
 *
 * \par Assumptions:
 *      1. The blk has been pre-initialized such that the tag1 value
 *         is initialized with the target block type.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *      2. The target block must not have its content changed after this point.
 *         Note that if the target block is both signed and encrypted, the ION
 *         implementation of sbsp will assume that the target block must be
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

int	sbsp_bibOffer(ExtensionBlock *blk, Bundle *bundle)
{
	Sdr			        bpSdr = getIonsdr();
	SbspOutboundBlock	asb;
	int8_t			    result = 0;


	BIB_DEBUG_PROC("+ sbsp_bibOffer(0x%x, 0x%x)",
                  (unsigned long) blk, (unsigned long) bundle);


	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure we have parameters...*/
	CHKERR(blk);
	CHKERR(bundle);

	blk->length = 0;	/*	Default.			*/
	blk->bytes = 0;		/*	Default.			*/


	/* Step 1.2 - Make sure that we are not trying an invalid security OP. */
	if (blk->tag1 == BLOCK_TYPE_BIB ||
	    blk->tag1 == BLOCK_TYPE_BCB)
	{
		/*	Can't have a BIB for these types of block.	*/
		BIB_DEBUG_ERR("x sbsp_bibOffer - BIB can't target type %d", blk->tag1);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BIB_DEBUG_PROC("- sbsp_bibOffer -> %d", result);
		return result;
	}

    /* Step 1.3 - Make sure OP(integrity, target) isn't already there. */
	if (sbsp_findBlock(bundle, BLOCK_TYPE_BIB, blk->tag1, 0, 0))
	{
		/*	Don't create a placeholder BIB for this block.	*/
		BIB_DEBUG_ERR("x sbsp_bibOffer - BIB already exists for tgt %d", blk->tag1);
		blk->size = 0;
		blk->object = 0;
		result = 0;
		BIB_DEBUG_PROC("- sbsp_bibOffer -> %d", result);
		return result;
	}

	/* Step 2 - Initialize BIB structures. */

	/* Step 2.1 - Populate the BIB ASB. */
	memset((char *) &asb, 0, sizeof(SbspOutboundBlock));
	sbsp_insertSecuritySource(bundle, &asb);
	asb.targetBlockType = blk->tag1;

	/* Step 2.2 Populate the BIB Extension Block. */

	CHKERR(sdr_begin_xn(bpSdr));

	blk->size = sizeof(SbspOutboundBlock);
	if ((blk->object = sdr_malloc(bpSdr, blk->size)) == 0)
	{
		BIB_DEBUG_ERR("x sbsp_bibOffer: Failed to SDR allocate %d bytes",
				      blk->size);
		result = -1;
		BIB_DEBUG_PROC("- sbsp_bibOffer -> %d", result);
		return result;
	}

	/* Step 3 - Write the ASB into the block. */

	sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);

	sdr_end_xn(bpSdr);

	/* Step 4 - Attach BIB if possible. */

	/*
	 * Step 4.1 - If the target is not the payload, we have
	 *            to defer integrity until a later date.
	 */
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
		result = 0;

		BIB_DEBUG_WARN("i sbsp_bibOffer: Integrity for type %d not supported.", asb.targetBlockType);
		BIB_DEBUG_PROC("- sbsp_bibOffer -> %d", result);
		return result;
	}

	/*
	 * Step 4.2 If target is the payload, sign the target
	 * and attach the BIB.
	 */

	if ((result = sbsp_bibAttach(bundle, blk, &asb)) <= 1)
	{
		CHKERR(sdr_begin_xn(bpSdr));
		sdr_free(bpSdr, blk->object);
		sdr_end_xn(bpSdr);

		blk->object = 0;
		blk->size = 0;
	}


	BIB_DEBUG_PROC("- sbsp_bibOffer -> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibReview
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

int	sbsp_bibReview(AcqWorkArea *wk)
{
	Sdr	sdr = getIonsdr();
	char	*secDestEid = NULL;
	char    *secSrcEid = NULL;
	Object	rules;
	Object	elt;
	Object	ruleAddr;
		OBJ_POINTER(BspBibRule, rule);
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 1;	/*	Default: no problem.		*/
	Bundle  *bundle;
	char    *dictionary;

	BIB_DEBUG_PROC("+ sbsp_bibReview(%x)", (unsigned long) wk);

	CHKERR(wk);

	/* Check if security database exists */
	rules = sec_get_bspBibRuleList();
	if (rules == 0)
	{
		BIB_DEBUG_PROC("- sbsp_bibReview -> no security database");
		return result;
	}

	/* Retrieve secDestEid */
	bundle = &(wk->bundle);
	dictionary = retrieveDictionary(bundle);
	if ( dictionary == (char*) bundle )
	{
		BIB_DEBUG_ERR("x sbsp_Review: Unable to obtain dictionary");
		return -1;
	}

	if (printEid(&bundle->destination, dictionary, &secDestEid) < 0
	|| printEid(&bundle->id.source, dictionary, &secSrcEid) < 0)
	{
		BIB_DEBUG_ERR("x sbsp_Review: Unable to identify bundle \
src/destination");
		if (secDestEid)
		{
			MRELEASE(secDestEid);
		}

		if (secSrcEid)
		{
			MRELEASE(secSrcEid);
		}

		return -1;
	}

	/* Cleanup dictionary here since we do not currently reuse it.
	 *  Optimization note: An identical dictionary is MTAKE'd and
	 *  released for each call to sbsp_requiredBlockExists() below.
	 *  Retrieving it once here and adding as a parameter may be
	 *  beneficial.
	 */
	if (dictionary)
	{
		releaseDictionary(dictionary);
	}
	
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Validate security destination */
		ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleAddr);
		oK(sdr_string_read(sdr, eidBuffer, rule->destEid));

		if (eidsMatch(eidBuffer, strlen(eidBuffer), secDestEid,
				strlen(secDestEid)) != 1)
		{
			/*	No requirement against local node.	*/

			continue;
		}
			
		/*	Validate security source */
		oK(sdr_string_read(sdr, eidBuffer, rule->securitySrcEid));

		if (eidsMatch(eidBuffer, strlen(eidBuffer), secSrcEid,
				strlen(secSrcEid)) != 1)
		{
			continue;
		}

		/* A block satisfying this rule is required.	*/		
		result = sbsp_requiredBlockExists(wk, BLOCK_TYPE_BIB,
				rule->blockTypeNbr, eidBuffer);
		if (result != 1)
		{
			break;
		}
	}

	sdr_exit_xn(sdr);

	if (secDestEid)
	{
		MRELEASE(secDestEid);
	}
	if (secSrcEid)
	{
		MRELEASE(secSrcEid);
	}

	BIB_DEBUG_PROC("- sbsp_bibReview -> %d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibParse
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

int	sbsp_bibParse(AcqExtBlock *blk, AcqWorkArea *wk)
{
	int	result;

	BIB_DEBUG_PROC("+ sbsp_bibParse(%x, %x)", (unsigned long) blk,
			(unsigned long) wk);

	CHKERR(blk);
	CHKERR(wk);

	result = sbsp_deserializeASB(blk, wk);
	BIB_DEBUG_INFO("i sbsp_bibParse: Deserialize result %d", result);

	BIB_DEBUG_PROC("- sbsp_bibParse -> %d", result);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibProcessOnDequeue
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
 *         implementation of sbsp will assume that the target block must be
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

int	sbsp_bibProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *parm)
{
	SbspOutboundBlock	asb;
	int8_t		    	result = 0;

	BIB_DEBUG_PROC("+ sbsp_bibProcessOnDequeue(%x, %x, %x)",
			       (unsigned long) blk, (unsigned long) bundle,
			       (unsigned long) parm);

	/* Step 1 - Sanity Checks. */

	/* Step 1.1 - Make sure arguments are valid. */
	if (bundle == NULL || parm == NULL || blk == NULL)
	{
		BIB_DEBUG_ERR("x sbsp_bibProcessOnDequeue: Bad Args.", NULL);
		BIB_DEBUG_INFO("i sbsp_bibProcessOnDequeue bundle %d, parm %d, \
blk %d, blk->size %d", (unsigned long) bundle, (unsigned long) parm, (unsigned long) blk,
                       (blk == NULL) ? 0 : blk->size);
		BIB_DEBUG_PROC("- sbsp_bibProcessOnDequeue --> %d", -1);
		scratchExtensionBlock(blk);
		return -1;
	}

	/* Step 1.2 - Make sure we do not process someone else's BIB. */
	if (blk->blkProcFlags & BLK_FORWARDED_OPAQUE)
	{
		/*	Do nothing; the block's bytes are correct
		 *	and ready for transmission.  The block was
		 *	received when the bundle was received; it
		 *	was serialized in the recordExtensionBlocks()
		 *	function.					*/

		BIB_DEBUG_PROC("- sbsp_bibProcessOnDequeue(%d)", result);
		return 0;
	} 

	/*
	 * Step 1.3 - If the target is the payload, the BIB was already
	 *            handled in sbsp_bibOffer. Nothing left to do.
	 */
	sdr_read(getIonsdr(), (char *) &asb, blk->object, blk->size);
	if (asb.targetBlockType == BLOCK_TYPE_PAYLOAD)
	{
		/*	Do nothing; the block's bytes are correct
		 *	and ready for transmission.  The block was
		 *	constructed by the offer() function, because
		 *	the payload block content was already final
		 *	at that time.					*/

		BIB_DEBUG_PROC("- sbsp_bibProcessOnDequeue(%d)", result);
		return 0;
	}

	/*
	 * Step 2 - Calculate the BIB for the target and attach it
	 *          to the bundle.
	 */
	result = sbsp_bibAttach(bundle, blk, &asb);

	BIB_DEBUG_PROC("- sbsp_bibProcessOnDequeue(%d)", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_bibRelease
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
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            S. Burleigh    Initial Implementation
 *****************************************************************************/

void    sbsp_bibRelease(ExtensionBlock *blk)
{
	Sdr			sdr = getIonsdr();
	SbspOutboundBlock	asb;

	BIB_DEBUG_PROC("+ sbsp_bibRelease(%x)", (unsigned long) blk);

	CHKVOID(blk);
	if (blk->object)
	{
		sdr_read(sdr, (char *) &asb, blk->object,
				sizeof(SbspOutboundBlock));
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

	BIB_DEBUG_PROC("- sbsp_bibRelease(%c)", ' ');
}
