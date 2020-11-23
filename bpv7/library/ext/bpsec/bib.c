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
 **		 bpsec_sign
 **              bibSerialize
 **              bibRelease
 **              bibCopy
 **                                                  bpsec_verify
 **                                                  bibParse
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
 **  10/14/20  S. Burleigh    Restructure for target multiplicity
 *****************************************************************************/

#include "zco.h"
#include "csi.h"
#include "bpsec_util.h"
#include "bib.h"
#include "bpsec_instr.h"
#include "bei.h"

#if (BIB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/*****************************************************************************
 *                       BIB COMPUTATION FUNCTIONS                           *
 *****************************************************************************/

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

/*****************************************************************************
 *                     BIB BLOCK MANAGEMENTFUNCTIONS                         *
 *****************************************************************************/

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
	new->tag = 0;
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
 * \param[in]      work The work area associated with this bundle acquisition.
 *
 * \par Notes:
 *****************************************************************************/

int	bibParse(AcqExtBlock *blk, AcqWorkArea *work)
{
	int	result;

	BIB_DEBUG_PROC("+ bibParse(%x, %x)", (unsigned long) blk,
			(unsigned long) work);

	CHKERR(blk);
	CHKERR(work);

	result = bpsec_deserializeASB(blk, work);
	BIB_DEBUG_INFO("i bibParse: Deserialize result %d", result);

	BIB_DEBUG_PROC("- bibParse -> %d", result);

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

/*****************************************************************************
 *                          BPSEC SIGNING FUNCTIONS                           *
 *****************************************************************************/

static Object	bibFindNew(Bundle *bundle, uint16_t profNbr, char *keyName)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			blockObj;
	ExtensionBlock		block;
	BpsecOutboundBlock	asb;

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blockObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &block, blockObj,
				sizeof(ExtensionBlock));
		if (block.bytes)	/*	Already serialized.	*/
		{
			continue;	/*	Not locally sourced.	*/
		}

		if (block.type != BlockIntegrityBlk)
		{
			continue;	/*	Doesn't apply.		*/
		}

		sdr_read(sdr, (char *) &asb, block.object,
				sizeof(BpsecOutboundBlock));
		if (asb.contextId != profNbr)
		{
			continue;	/*	For a different rule.	*/
		}

		if (strlen(keyName) != 0 && strcmp(keyName, asb.keyName) != 0)
		{
			continue;	/*	For a different rule.	*/
		}

		return blockObj;
	}

	return 0;
}

static Object	bibFindOutboundTarget(Bundle *bundle, int blockNumber)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			blockObj;
	ExtensionBlock		block;
	BpsecOutboundBlock	asb;
	Object			elt2;
	Object			targetObj;
	BpsecOutboundTarget	target;

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blockObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &block, blockObj,
				sizeof(ExtensionBlock));
		if (block.type != BlockIntegrityBlk
		&& block.type != BlockConfidentialityBlk)
		{
			continue;	/*	Not a BPSec block.	*/
		}

		/*	This is a BPSec block.  See if the indicated
		 *	non-BPSec block is one of its targets.		*/

		sdr_read(sdr, (char *) &asb, block.object,
				sizeof(BpsecOutboundBlock));
		for (elt2 = sdr_list_first(sdr, asb.targets); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			targetObj = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &target, targetObj,
					sizeof(BpsecOutboundTarget));
			if (target.targetBlockNumber == blockNumber)
			{
				return elt2;
			}
		}
	}

	return 0;	/*	No such target.				*/
}

static Object	bibCreate(Bundle *bundle, BibProfile *prof, char *keyName)
{
	Sdr			sdr = getIonsdr();
	ExtensionBlock		blk;
	BpsecOutboundBlock	asb;

	memset((char *) &blk, 0, sizeof(ExtensionBlock));
	blk.type = BlockIntegrityBlk;
	blk.tag = 0;
	blk.crcType = NoCRC;
	memset((char *) &asb, 0, sizeof(BpsecOutboundBlock));
	bpsec_insertSecuritySource(bundle, &asb);
	asb.contextId = prof->profNbr;
	memcpy(asb.keyName, keyName, BPSEC_KEY_NAME_LEN);
	asb.targets = sdr_list_create(sdr);
	asb.parmsData = sdr_list_create(sdr);
	if (asb.targets == 0 || asb.parmsData == 0)
	{
		BIB_DEBUG_ERR("x bibCreate: Failed to initialize BIB ASB.",
				NULL);
		return 0;
	}

	blk.size = sizeof(BpsecOutboundBlock);
	if ((blk.object = sdr_malloc(sdr, blk.size)) == 0)
	{
		BIB_DEBUG_ERR("x bibCreate: Failed to SDR allocate object of \
size %d bytes", blk->size);
		return 0;
	}

	sdr_write(sdr, blk.object, (char *) &asb, blk.size);
	return attachExtensionBlock(BlockIntegrityBlk, &blk, bundle);
}

static int	bibAddTarget(Sdr sdr, Bundle *bundle, Object *bibObj,
			ExtensionBlock *bibBlk, BpsecOutboundBlock *asb,
			BibProfile *prof, char *keyName, int targetBlockNumber)
{
	if (bibFindOutboundTarget(bundle, targetBlockNumber) != 0)
	{
		/*	Block is already secured by a BIB or a BCB.	*/

		return 0;	/*	Nothing to do.			*/
	}

	/*	Block needs to be added as a target to the applicable
	 *	newly inserted BIB.					*/

	if (*bibObj == 0)	/*	New BIB doesn't exist yet.	*/
	{
		*bibObj = bibCreate(bundle, prof, keyName);
		if (*bibObj == 0)
		{
			return -1;
		}

		sdr_read(sdr, (char *) bibBlk, *bibObj, sizeof(ExtensionBlock));
		sdr_read(sdr, (char *) asb, bibBlk->object, bibBlk->size);
	}

	if (bpsec_insert_target(sdr, asb, targetBlockNumber) < 0)
	{
		return -1;
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bibDefaultSign
 *
 * \par For one of the indicated BIB's target blocks, calculates a
 * 	digest that will be inserted into the results list for that target.
 *
 *
 * \param[in]  suite   		- The ciphersuite to be used for signing.
 * \param[in]  bundle		- The serialized bundle.
 * \param[in]  blk		- The BIB extension block.
 * \param[in]  asb		- The abstract security block for the BIB.
 * \param[in]  target		- The target block.
 * \param[in]  targetZco	- The target block data to sign.
 * \param[out] toEid		- Destination EID (not used).
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
 *  04/26/16  E. Birrane     Added length [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bibDefaultSign(uint32_t suite, Bundle *bundle, ExtensionBlock *blk,
		BpsecOutboundBlock *asb, BpsecOutboundTarget *target,
		Object targetZco, char *toEid)
{
	Sdr			sdr = getIonsdr();
	int8_t			retval = 0;
	sci_inbound_tlv		digest;
	sci_inbound_tlv		key;
	uint8_t			*context = NULL;

	BIB_DEBUG_INFO("+ bibDefaultSign(%d, 0x%x, 0x%x, 0x%x", suite,
			(unsigned long) bundle, (unsigned long) blk,
			(unsigned long) asb);

	/*	Sanity Checks.						*/

	CHKERR(bundle && blk && asb && targetZco && target);
	key = bpsec_retrieveKey(asb->keyName);

	/*	Grab and initialize a crypto context.		*/

	if ((context = sci_ctx_init(suite, key, CSI_SVC_SIGN)) == NULL)
	{
		BIB_DEBUG_ERR("x bibDefaultSign - Can't get context.", NULL);
		MRELEASE(key.value);
		BIB_DEBUG_PROC("- bibDefaultSign--> NULL", NULL);
		return ERROR;
	}

	if (sci_sign_start(suite, context) == ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultSign - Can't start context.", NULL);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		BIB_DEBUG_PROC("- bibDefaultSign--> NULL", NULL);
		return ERROR;
	}

	retval = bibDefaultCompute(targetZco, csi_blocksize(suite), suite,
			context, CSI_SVC_SIGN);
	if (retval == ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultSign: Can't compute hash.", NULL);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		BIB_DEBUG_PROC("- bibDefaultSign--> ERROR", NULL);
		return ERROR;
	}

	if ((sci_sign_finish(suite, context, &digest, CSI_SVC_SIGN))
			== ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultSign - Can't Finalize.", NULL);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		BIB_DEBUG_PROC("- bibDefaultSign--> ERROR", NULL);
		return ERROR;
	}

	/*	Insert the security result.			*/

	digest.id = BPSEC_CSPARM_INT_SIG;
	if (bpsec_appendItem(sdr, target->results, &digest) < 0)
	{
		BIB_DEBUG_ERR("x bibDefaultSign--> Can't insert hash.", NULL);
		MRELEASE(digest.value);
		bundle->corrupt = 1;
		scratchExtensionBlock(blk);
		BIB_DEBUG_PROC("- bibDefaultSign --> %d", -1);
		return ERROR;
	}

	MRELEASE(digest.value);
	csi_ctx_free(suite, context);
	MRELEASE(key.value);
	BIB_DEBUG_PROC("- bibDefaultSign--> %d", retval);
	return 0;
}

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

int	bibAttach(Bundle *bundle, ExtensionBlock *bibBlk,
		BpsecOutboundBlock *bibAsb)
{
	Sdr			sdr = getIonsdr();
	int8_t			result = 0;
	BibProfile		*prof;
	char			*fromEid;	/*	Instrumentation.*/
	char			*toEid;		/*	For whatever.	*/
	Object			elt;
	Object			targetObj;
	BpsecOutboundTarget	target;
	Object			targetZco;
	uint8_t			*serializedAsb;
	uvast			length = 0;

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
	if (bpsec_getOutboundSecuritySource(bundle, bibAsb, &fromEid) < 0)
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

	readEid(&(bundle->destination), &toEid);
	CHKERR(toEid);
	for (elt = sdr_list_first(sdr, bibAsb->targets); elt;
			elt = sdr_list_next(sdr, elt))
	{
		targetObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &target, targetObj,
				sizeof(BpsecOutboundTarget));
		length = bpsec_canonicalizeOut(bundle, target.targetBlockNumber,
				&targetZco);
		if (length < 1)
		{
			zco_destroy(sdr, targetZco);
			MRELEASE(toEid);
			ADD_BIB_TX_FAIL(fromEid, 1, 0);
			MRELEASE(fromEid);

			BIB_DEBUG_ERR("x bibAttach: Can't canonicalize.", NULL);
			result = -1;
			bundle->corrupt = 1;
			scratchExtensionBlock(bibBlk);
			BIB_DEBUG_PROC("- bibAttach --> %d", result);
			return result;
		}

		result = (prof->sign == NULL)
			? bibDefaultSign(prof->suiteId, bundle, bibBlk,
			bibAsb, &target, targetZco, toEid)
			: prof->sign(prof->suiteId, bundle, bibBlk,
			bibAsb, &target, targetZco, toEid);
		zco_destroy(sdr, targetZco);
		if (result < 0)
		{
			MRELEASE(toEid);
			ADD_BIB_TX_FAIL(fromEid, 1, length);
			MRELEASE(fromEid);

			BIB_DEBUG_ERR("x bibAttach: Can't sign target block.",
					NULL);
			result = -1;
			bundle->corrupt = 1;
			scratchExtensionBlock(bibBlk);
			BIB_DEBUG_PROC("- bibAttach --> %d", result);
			return result;
		}
	}

	MRELEASE(toEid);

	/* Step 3 - serialize the BIB ASB into the BIB blk. */

	/* Step 3.1 - Create a serialized version of the BIB ASB. */

	if ((serializedAsb = bpsec_serializeASB((uint32_t *)
			&(bibBlk->dataLength), bibAsb)) == NULL)
	{
		ADD_BIB_TX_FAIL(fromEid, 1, length);
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

	ADD_BIB_TX_PASS(fromEid, 1, length);
	MRELEASE(fromEid);

	BIB_DEBUG_PROC("- bibAttach --> %d", result);
	return result;
}

static int	bibAttachAll(Bundle *bundle)
{
	Sdr			sdr = getIonsdr();
	Object			elt;
	Object			blockObj;
	ExtensionBlock		block;
	BpsecOutboundBlock	asb;

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		blockObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &block, blockObj,
				sizeof(ExtensionBlock));
		if (block.bytes)	/*	Already serialized.	*/
		{
			continue;	/*	Not newly sourced.	*/
		}

		if (block.type != BlockIntegrityBlk)
		{
			continue;	/*	Doesn't apply.		*/
		}

		/*	This is a new BIB: compute all signatures,
		 *	insert all security results, serialize.		*/

		sdr_read(sdr, (char *) &asb, block.object,
				sizeof(BpsecOutboundBlock));
		if (bibAttach(bundle, &block, &asb) < 0)
		{
			return -1;
		}
	}

	return 0;
}

int	bpsec_sign(Bundle *bundle)
{
	Sdr			sdr = getIonsdr();
	Object			rules;
	Object			elt;
	Object			ruleObj;
	BPsecBibRule		rule;
	BibProfile		*prof;
	char			keyBuffer[32];
	int			keyBuflen = sizeof keyBuffer;
	Object			bibObj;
	ExtensionBlock		bibBlk;
	BpsecOutboundBlock	asb;
	Object			elt2;
	Object			blockObj;
	ExtensionBlock		block;

	rules = sec_get_bpsecBibRuleList();

	/*	Apply all applicable BIB rules.				*/

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rule, ruleObj, sizeof(BPsecBibRule));
		if (rule.blockType == BlockIntegrityBlk
		|| rule.blockType == BlockConfidentialityBlk)
		{
			/*	This is an error in the rule.  No
			 *	target of a BIB can be a BCB or
			 *	another BIB.				*/

			continue;
		}

		if (!bpsec_BibRuleApplies(bundle, &rule))
		{
			continue;
		}

		prof = get_bib_prof_by_name(rule.profileName);
		if (prof == NULL)
		{
			/*	This is an error in the rule; profile
			 *	may have been deleted after rule was
			 *	added.					*/

			continue;
		}

		if (strlen(rule.keyName) > 0
		&& sec_get_key(rule.keyName, &keyBuflen, keyBuffer) == 0)
		{
			/*	Again, an error in the rule; key may
			 *	have been deleted after rule was added.	*/

			continue;
		}

		/*	Need to enforce this rule on all applicable
		 *	blocks.  First find the newly sourced BIB
		 *	that applies the rule's mandated profile and
		 *	(if noted) key.					*/

		bibObj = bibFindNew(bundle, prof->profNbr, rule.keyName);
		if (bibObj)
		{
			sdr_read(sdr, (char *) &bibBlk, bibObj,
					sizeof(ExtensionBlock));
			sdr_read(sdr, (char *) &asb, bibBlk.object,
					bibBlk.size);
		}

		/*	(If this BIB doesn't exist, it will be created
		 *	as soon as its first target is identified.)
		 *
		 *	Now look for blocks to which this rule must
		 *	be applied.					*/

		if (rule.blockType == PrimaryBlk)
		{
			if (bibAddTarget(sdr, bundle, &bibObj, &bibBlk, &asb,
					prof, rule.keyName, 0) < 0)
			{
				return -1;
			}

			continue;
		}

		if (rule.blockType == PayloadBlk)
		{
			if (bibAddTarget(sdr, bundle, &bibObj, &bibBlk, &asb,
					prof, rule.keyName, 1) < 0)
			{
				return -1;
			}

			continue;
		}

		for (elt2 = sdr_list_first(sdr, bundle->extensions); elt2;
				elt2 = sdr_list_next(sdr, elt2))
		{
			blockObj = sdr_list_data(sdr, elt2);
			sdr_read(sdr, (char *) &block, blockObj,
					sizeof(ExtensionBlock));
			if (block.type != rule.blockType)
			{
				continue;	/*	Doesn't apply.	*/
			}

			/*	This rule would apply to this block.	*/

			if (bibAddTarget(sdr, bundle, &bibObj, &bibBlk, &asb,
					prof, rule.keyName, block.number) < 0)
			{
				return -1;
			}
		}
	}


	/*	Now attach all new BIBs, signing all targets.		*/

	if (bibAttachAll(bundle) < 0)
	{
		return -1;
	}

	return 0;
}

/*****************************************************************************
 *                      BPSEC VERIFICATION FUNCTIONS                         *
 *****************************************************************************/

static void	discardTarget(LystElt targetElt, LystElt bibElt)
{
	BpsecInboundTarget	*target;
	AcqExtBlock		*bib;
	BpsecInboundBlock	*asb;

	target = (BpsecInboundTarget *) lyst_data(targetElt);
	bpsec_releaseInboundTlvs(target->results);
	MRELEASE(target);
	lyst_delete(targetElt);
	bib = (AcqExtBlock *) lyst_data(bibElt);
	asb = (BpsecInboundBlock *) (bib->object);
	if (lyst_length(asb->targets) == 0)
	{
		deleteAcqExtBlock(bibElt);
	}
}

static void	discardTargetBlock(AcqExtBlock *block, LystElt targetElt,
			LystElt bibElt)
{
	discardAcqExtensionBlock(block);
	if (targetElt)
	{
		discardTarget(targetElt, bibElt);
	}
}

LystElt	bibFindInboundTarget(AcqWorkArea *work, int blockNumber,
		LystElt *bibElt)
{
	LystElt			elt;
	AcqExtBlock		*block;
	BpsecInboundBlock	*asb;
	LystElt			elt2;
	BpsecInboundTarget	*target;

	for (elt = lyst_first(work->extBlocks); elt; elt = lyst_next(elt))
	{
		block = (AcqExtBlock *) lyst_data(elt);
		if (block->type != BlockIntegrityBlk)
		{
			continue;
		}

		/*	This is a BIB.  See if the indicated
		 *	non-BPSec block is one of its targets.		*/

		asb = (BpsecInboundBlock *) (block->object);
		for (elt2 = lyst_first(asb->targets); elt2;
				elt2 = lyst_next(elt2))
		{
			target = (BpsecInboundTarget *) lyst_data(elt2);
			if (target->targetBlockNumber == blockNumber)
			{
				*bibElt = elt;
				return elt2;
			}
		}
	}

	return NULL;	/*	No such target.				*/
}

/******************************************************************************
 *
 * \par Function Name: bibDefaultVerify
 *
 * \par For one of the indicated BIB's target blocks, calculates a
 * 	digest and compares it to the digest asserted (in the BIB)
 * 	for that target.
 *
 * \param[in]	suite		- The ciphersuite used for verification.
 * \param[in]	wk		- Contains the serialized bundle being verified
 * \param[in]	blk 		- The BIB extension block.
 * \param[in]	asb 		- The abstract security block for the BIB.
 * \param[in]	target	 	- The target block.
 * \param[in]	targetZco 	- The target data to verify.
 * \param[in]	fromEid		- Block source EID (not used).
 *
 * \par Notes:
 *   - This function must be called AFTER all blocks of the bundle have
 *     been decrypted.
 *
 * \return -1  - System Error
 *          0  - Protocol Error
 *          1  - Successful Processing
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/05/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  04/26/16  E. Birrane     Added length. [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int	bibDefaultVerify(uint32_t suite, AcqWorkArea *wk, AcqExtBlock *blk,
		BpsecInboundBlock *asb, BpsecInboundTarget *target,
		Object targetZco, char *fromEid)
{
	sci_inbound_tlv assertedDigest;
	sci_inbound_tlv	key;
	uint8_t		*context = NULL;
	int8_t		retval = 0;

	BIB_DEBUG_INFO("+ bibDefaultVerify(%d, 0x%x, 0x%x",
			suite, (unsigned long) wk, (unsigned long) blk);

	/* Step 0 - Sanity Checks. */
	CHKERR(wk && blk && asb && targetZco && target);

	assertedDigest = sci_extract_tlv(CSI_PARM_INTSIG, target->results);
	key = bpsec_retrieveKey(asb->keyName);

	/*	Grab and initialize a crypto context.			*/

	if ((context = sci_ctx_init(suite, key, CSI_SVC_VERIFY)) == NULL)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify - Can't get context.", NULL);
		MRELEASE(key.value);
		MRELEASE(assertedDigest.value);
		BIB_DEBUG_PROC("- bibDefaultVerify--> NULL", NULL);
		return ERROR;
	}

	if (sci_sign_start(suite, context) == ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify - Can't start context.",
				NULL);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		MRELEASE(assertedDigest.value);
		BIB_DEBUG_PROC("- bibDefaultVerify--> ERROR", NULL);
		return ERROR;
	}

	retval = bibDefaultCompute(targetZco, csi_blocksize(suite), suite,
			context, CSI_SVC_VERIFY);
	if (retval == ERROR)
	{
		BIB_DEBUG_ERR("x bibDefaultVerify: Can't compute hash.", NULL);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		MRELEASE(assertedDigest.value);
		BIB_DEBUG_PROC("- bibDefaultVerify--> ERROR", NULL);
		return ERROR;
	}

	retval = sci_sign_finish(suite, context, &assertedDigest,
			CSI_SVC_VERIFY);
	MRELEASE(assertedDigest.value);
	switch (retval)
	{
	case ERROR:
		BIB_DEBUG_ERR("x bibDefaultVerify: Can't finalize.", NULL);
		csi_ctx_free(suite, context);
		MRELEASE(key.value);
		BIB_DEBUG_PROC("- bibDefaultVerify--> ERROR", NULL);
		return ERROR;

	case 0:		/*	Digests do not match.			*/
		retval = 4;
		break;

	default:
		break;
	}

	csi_ctx_free(suite, context);
	MRELEASE(key.value);
	BIB_DEBUG_PROC("- bibDefaultVerify--> %d", retval);
	return retval;
}

int	bpsec_verify(AcqWorkArea *work)
{
	Sdr			sdr = getIonsdr();
	Bundle			*bundle = &(work->bundle);
	Object			rules;
	Object			elt;
	Object			ruleObj;
	BPsecBibRule		rule;
	BibProfile		*prof;
	char			keyBuffer[32];
	int			keyBuflen = sizeof keyBuffer;
	LystElt			elt2;
	AcqExtBlock		*block;
	AcqExtBlock		*bib;
	BpsecInboundBlock	*asb;
	char			*fromEid;	/*	Instrumentation.*/
	LystElt			targetElt;
	LystElt			bibElt;
	BpsecInboundTarget	*target;
	Object			targetZco;
	int			result;
	uvast			length = 0;

	rules = sec_get_bpsecBibRuleList();

	/*	Apply all applicable BIB rules.				*/

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rule, ruleObj, sizeof(BPsecBibRule));
		if (rule.blockType == BlockIntegrityBlk
		|| rule.blockType == BlockConfidentialityBlk)
		{
			/*	This is an error in the rule.  No
			 *	target of a BIB can be a BCB or
			 *	another BIB.				*/

			continue;
		}

		if (!bpsec_BibRuleApplies(bundle, &rule))
		{
			continue;
		}

		prof = get_bib_prof_by_name(rule.profileName);
		if (prof == NULL)
		{
			/*	This is an error in the rule; profile
			 *	may have been deleted after rule was
			 *	added.					*/

			continue;
		}

		if (strlen(rule.keyName) > 0
		&& sec_get_key(rule.keyName, &keyBuflen, keyBuffer) == 0)
		{
			/*	Again, an error in the rule; key may
			 *	have been deleted after rule was added.	*/

			continue;
		}

		for (elt2 = lyst_first(work->extBlocks); elt2;
				elt2 = lyst_next(elt2))
		{
			block = (AcqExtBlock *) lyst_data(elt2);
			if (block->type != rule.blockType)
			{
				continue;	/*	Doesn't apply.	*/
			}

			/*	This rule would apply to this block.	*/

			targetElt = bibFindInboundTarget(work, block->number,
					&bibElt);
			if (targetElt == NULL)
			{
				/*	No BIB; block is not signed.
				 *	A security policy violation.	*/

				if (block->type == PrimaryBlk)
				{
					work->authentic = 0;
				}
				else	/*	Assume compromised.	*/
				{
					bundle->altered = 1;
				}

				continue;
			}

			/*	Block's signature needs to be verified.	*/

			target = (BpsecInboundTarget *) lyst_data(targetElt);
			bib = (AcqExtBlock *) lyst_data(bibElt);
			asb = (BpsecInboundBlock *) (bib->object);
			if (asb->contextFlags & BPSEC_ASB_SEC_SRC)
			{
				/*	Waypoint source.		*/

				readEid(&(asb->securitySource), &fromEid);
				if (fromEid == NULL)
				{
					ADD_BIB_RX_FAIL(NULL, 1, 0);
					return -1;
				}
			}
			else	/*	Bundle source.			*/
			{
				readEid(&(bundle->id.source), &fromEid);
				if (fromEid == NULL)
				{
					ADD_BIB_RX_FAIL(NULL, 1, 0);
					return -1;
				}
			}

			length = bpsec_canonicalizeIn(work, block->number,
					&targetZco);
			if (length < 1)
			{
				ADD_BIB_RX_FAIL(fromEid, 1, 0);
				MRELEASE(fromEid);
				return -1;
			}

			result = (prof->verify == NULL)
				?  bibDefaultVerify(prof->suiteId, work, block,
				asb, target, targetZco, fromEid)
				: prof->verify(prof->suiteId, work, block,
				asb, target, targetZco, fromEid);
			zco_destroy(sdr, targetZco);
			MRELEASE(fromEid);

			BIB_DEBUG_INFO("i bpsec_verify: Verify result was %d",
					result);
			switch (result)
			{
			case -1:
				bundle->corrupt = 1;
				ADD_BIB_RX_FAIL(fromEid, 1, length);
				continue;

			case 0:
				if (work->authentic == 1)
				{
					continue;
				}

				switch (block->type)
				{
				case PrimaryBlk:
					work->authentic = 0;
					ADD_BIB_RX_FAIL(fromEid, 1, length);
					break;

				case PayloadBlk:
					bundle->altered = 1;
					ADD_BIB_RX_FAIL(fromEid, 1, length);
					break;

				default:
					discardTargetBlock(block, targetElt,
							bibElt);
				}

				continue;

			default:	/*	Verified.		*/
				if (work->authentic == -1
				&& block->type == PrimaryBlk)
				{
					work->authentic = 1;
				}
			}

			/*	Target signature verified.		*/

			if (bpsec_destinationIsLocal(&(work->bundle)))
			{
				BIB_DEBUG(2, "BIB check passed.", NULL);
				ADD_BIB_RX_PASS(fromEid, 1, length);
				discardTarget(targetElt, bibElt);
			}
			else
			{
				ADD_BIB_FWD(fromEid, 1, length);
			}
		}
	}

	if (work->authentic == 0)
	{
		bundle->clDossier.authentic = 0;
	}
	else
	{
		bundle->clDossier.authentic = 1;
	}

	return 0;
}
