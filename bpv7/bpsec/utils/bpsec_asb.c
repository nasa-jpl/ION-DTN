/******************************************************************************
#include <sc_util.h>
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bpsec_asb.h
 **
 ** Subsystem:
 **              BP Security Library (BSP) Abstract Security Block (ASB)
 **
 ** Namespace:
 **              bslasb_
 **
 ** Description: This file provides all structures, variables, and function
 **              definitions necessary for operations associated with the
 **              Abstract Security Block (ASB) structure defined by the
 **              BPSec security extensions.
 **
 **              ASBs are logical structures holding security information for
 **              BIB and BCB security blocks. As such, ABSs must be deserialized
 **              from encoded blocks when processing inbound bundles and
 **              serialized into outbound blocks when transmitting
 **              bundles.
 **
 **              All inbound bpsec blocks are extracted from bundles created
 **              "remotely" (though possibly by the local node via BP loopback)
 **              and received locally. All bpsec blocks created locally are
 **              outbound bpsec blocks.  However, not all outbound bpsec blocks
 **              are created locally.  A bpsec block that was created remotely
 **              and acquired locally may need to be copied to an outbound bpsec
 **              block so that it may be forwarded to other nodes, just as locally
 **              created bpsec blocks must be.
 **
 **              For this purpose, ASB serialization is not needed, since the
 **              serialized form of the block is already present in the "bytes"
 **              array of the inbound block.  All that's needed is to copy the
 **              inbound block's "bytes" array to the SDR heap and store the
 **              address of the copy in the outbound block's "bytes" array.  The
 **              inbound block's ASB (scratchpad object) need not be referenced or
 **              re-serialized.  (Note that an inbound bpsec block must not be
 **              modified in any way before it is forwarded, as this would
 **              invalidate the block's sourceEID.)
 **
 **
 ** Assumptions:
 **      1. We assume that this code is not under such tight profiling
 **         constraints that sanity checks are too expensive.
 **
 ** Modification History:
 **
 **  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 **  --------  ------------  -----------------------------------------------
 **  12/31/21  E. Birrane           Extracted ASB functions from bpsec_util and
 **                                 updated for RFC9172 and RFC9173. (JHU/APL)
 *****************************************************************************/


/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_asb.h"
#include "bpsec_util.h"
#include "sc_value.h"

/*****************************************************************************
 *                             PRIVATE FUNCTIONS                             *
 *****************************************************************************/

// TODO: Document function.
// TODO: Fix function name.
/*
 * Create the results array for targets. We don't need to keep a separate targets array
 * just because...
 *
 * Assumed that the asb already has the results lyst created...
 */

static int bslasb_extractTargets(BpsecInboundASB *asb, int memIdx, unsigned char **cursor, unsigned int *unparsedBytes)
{
	BpsecInboundTargetResult *result = NULL;
	uvast arrayLength = 0;
	uvast uvtemp = 0;

	/* Step 0: Sanity Checks. */
	CHKZERO(asb);
	CHKZERO(cursor);
	CHKZERO(unparsedBytes);

	/* Step 1: Open the array which starts the target list. */
    arrayLength = 0;
    if (cbor_decode_array_open(&arrayLength, cursor, unparsedBytes) < 1)
    {
    	BPSEC_DEBUG_ERR("Can't decode bpsec block target array", NULL);
        return 0;
    }

    /* Step 2: For each target in the target list... */
    while (arrayLength > 0)
    {

        if (cbor_decode_integer(&uvtemp, CborAny, cursor, unparsedBytes) < 1)
        {
        	BPSEC_DEBUG_ERR("Can't decode bpsec block target nbr", NULL);
            return 0;
        }

        if((result = bpsec_asb_inboundTargetResultCreate(uvtemp, memIdx)) == NULL)
        {
        	BPSEC_DEBUG_ERR("Can't create target results.", NULL);
            return 0;
        }

        if(lyst_insert_last(asb->scResults, result) == NULL)
        {
        	BPSEC_DEBUG_ERR("Can't add target results.", NULL);
            return 0;
        }

        arrayLength -= 1;
    }

    return 1;
}


// TODO: Document function.
// TODO: Fix function names.

static int bslasb_extractContext(BpsecInboundASB *asb, int memIdx, unsigned char **cursor, unsigned int *unparsedBytes)
{
	uvast uvtemp = 0;
	sc_value *tv = NULL;

	BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
	                 (uaddr)asb, memIdx, (uaddr) cursor, (uaddr) unparsedBytes);

	/* Step 1: Extract the security context Id. */
    if (cbor_decode_integer(&uvtemp, CborAny, cursor, unparsedBytes) < 1)
    {
    	BPSEC_DEBUG_ERR("Can't decode bpsec block context ID", NULL);
    	return 0;
    }
        asb->scId = uvtemp;

    BPSEC_DEBUG_INFO("read context id of %d.", asb->scId);

    /* Step 2: Extract the security context flags. */
    if (cbor_decode_integer(&uvtemp, CborAny, cursor, unparsedBytes) < 1)
    {
    	BPSEC_DEBUG_ERR("Can't decode bpsec block context flags", NULL);
        return 0;
    }
    asb->scFlags = uvtemp;

    BPSEC_DEBUG_INFO("Read security flags %x", asb->scFlags);

    /* Step 3: Extract the Security Source */
    switch (acquireEid(&(asb->scSource), cursor, unparsedBytes))
    {
    	 case -1:
    		 BPSEC_DEBUG_ERR("No space for security source EID", NULL);
    		 return -1;

    	 case 0:
    		 BPSEC_DEBUG_ERR("Can't decode bpsec block security src", NULL);
    		 return 0;
    	 default:
    		 break;
    }

    /* Step 4: Extract security context Parameters, if they exist. */
    if (asb->scFlags & BPSEC_ASB_PARM)
    {
        uvast arrayLength = 0;
        if (cbor_decode_array_open(&arrayLength, cursor, unparsedBytes) < 1)
        {
        	BPSEC_DEBUG_ERR("Can't decode bpsec block parms array", NULL);
            return 0;
        }

        BPSEC_DEBUG_INFO("There are %d parms.", arrayLength);
        while (arrayLength > 0)
        {
        	if( ((tv = bpsec_scv_memDeserialize(asb->scId, SC_VAL_TYPE_PARM, cursor, unparsedBytes)) == NULL) ||
        		(lyst_insert_last(asb->scParms, tv) == NULL))
            {
                BPSEC_DEBUG_ERR("Cannot extract SCI Parm.", NULL);
                return -1;
            }

        	BPSEC_DEBUG_INFO("Deserialized val of id %d type %d and length %d", tv->scValId, tv->scValType, tv->scValLength);

            arrayLength -= 1;
        }
    }
    else
    {
        BPSEC_DEBUG_INFO("No parms noted.", NULL);
    }

    return 1;
}

// TODO: Document function.
// TODO: Fix function names.

/*
 * This is the full set of results, wrapped in 3 arrays, as follows:
 *
 * 1. All Results   - How many target results are there. This must be the same as
 *    Array           the number of targets in the block. Each element of this
 *                    array is a Target Result Array.
 *
 * 2. Target        - This is an array of results per target. Each element of this
 *    Result Array    array is an array of Individual Results.
 *
 * 3. Individual    - This is a 2-tuple of key/values representing an individual
 *    Result Array    security result.
 *
 * This function extracts ALL of these results.
 * We expect the scResults list to have 1 entry for each target in the block, and
 * this function is to extract the results for each.
 */
static int bslasb_extractTargetResults(BpsecInboundASB *asb, int memIdx, unsigned char **cursor, unsigned int *unparsedBytes)
{
	uvast arrayLength = 0;
	sc_value *tv = NULL;
	LystElt tgtElt;
	int i = 1;


	BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",memIdx,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
	                 (uaddr) asb, (uaddr) cursor, (uaddr) unparsedBytes);

	/*
	 * Step 1: Check that the results array is an array with expected length.
	 *         There must be one target result set for each target of the
	 *         security block.
	 *
	 *         Passing the expected number of results to the array decode
	 *         will check that the array has the right number of elements.
	 */

    if (cbor_decode_array_open(&arrayLength, cursor, unparsedBytes) < 1)
    {
    	BPSEC_DEBUG_ERR("Can't decode bpsec block results array", NULL);
        return 0;
    }

    if(arrayLength != lyst_length(asb->scResults))
    {
        BPSEC_DEBUG_ERR("Expecting results for %d targets but instead have results for %d targets.",
                        lyst_length(asb->scResults), arrayLength);
        return 0;
    }
    else
    {
        BPSEC_DEBUG_INFO("Attempting to extract results for %d target(s).", arrayLength);
    }


    /*
     * Step 2: Populate each target result set. Each target result set is
     *         an array of individual results for the target.
     */

    for (tgtElt = lyst_first(asb->scResults); tgtElt; tgtElt = lyst_next(tgtElt))
    {
    	BpsecInboundTargetResult *target = (BpsecInboundTargetResult *) lyst_data(tgtElt);

    	BPSEC_DEBUG_INFO("Extracting target set %d", i);

    	/*
    	 * Step 2.1: Extract number of individual results.
    	 */
    	arrayLength = 0;
    	if (cbor_decode_array_open(&arrayLength, cursor, unparsedBytes) < 1)
    	{
    		BPSEC_DEBUG_ERR("Can't decode bpsec block result set", NULL);
    		return 0;
    	}

    	BPSEC_DEBUG_INFO("Extracting %d individual results for target set %d", arrayLength, i++);

    	while (arrayLength > 0)
    	{
    	    tv = bpsec_scv_memDeserialize(asb->scId, SC_VAL_TYPE_RESULT, cursor, unparsedBytes);

    	    if(tv != NULL)
    	    {
    	        if(lyst_insert_last(target->scIndTargetResults, tv) == NULL)
    	        {
    	            BPSEC_DEBUG_ERR("Failed adding extracted SCI result of type %d.", tv->scValId);
    	            return -1;
    	        }
    	    }
    	    else
    	    {
                BPSEC_DEBUG_ERR("Cannot extract SCI Result.", NULL);
                return -1;
    	    }

    		arrayLength -= 1;
    	}
    	i++;
    }

    return 1;
}

// TODO: Document function.
// TODO: Fix function names.

static Object bslasb_outboundCopyTargetResults(Sdr sdr, Object oldResultList)
{
    Object newResultList = 0;
    Object elt = 0;
    Object curTgtResultObj = 0;
    Object newTgtResultObj = 0;
    BpsecOutboundTargetResult oldTgtResult;
    BpsecOutboundTargetResult newTgtResult;
    int success = 1;


    /* Step 0: Sanity check. We need a list to copy...*/
    if(oldResultList == 0)
    {
        return newResultList;
    }

    /* Step 1: Allocate the new list in the SDR. */
    if((newResultList = sdr_list_create(sdr)) == 0)
    {
        BPSEC_DEBUG_ERR("Cannot allocate SDR list.", NULL);
        return newResultList;
    }

    /*
     * Step 2: For each element of the existing list of results,
     *         allocate a new target result and deep copy it.
     */
    for (elt = sdr_list_first(sdr, oldResultList);
         elt;
         elt = sdr_list_next(sdr, elt))
    {
        /* Step 2.1: Grab the current existing target result. */
        curTgtResultObj = sdr_list_data(sdr, elt);
        sdr_read(sdr, (char *) &oldTgtResult, curTgtResultObj, sizeof(oldTgtResult));

        /* Step 2.2: Allocate space in the SDR for the new result. */
        if((newTgtResultObj = sdr_malloc(sdr, sizeof(BpsecOutboundTargetResult))) == 0)
        {
            success = 0;
            break;
        }

        /* Step 2.2: Shallow copy elements to new result where applicable. */
        newTgtResult.scTargetId = oldTgtResult.scTargetId;

        /* Step 2.3: Copy over the individual results. */
        if((newTgtResult.scIndTargetResults = bpsec_scv_sdrListCopy(sdr, oldTgtResult.scIndTargetResults)) < 0)
        {
            success = 0;
            break;
        }

        /* Step 2.4: Write this new target result to the SDR. */
        sdr_write(sdr, newTgtResultObj, (char *) &newTgtResult, sizeof(newTgtResult));

        if (sdr_list_insert_last(sdr, newResultList, newTgtResultObj) == 0)
        {
            success = 0;
            break;
        }
    }

    if(success == 0)
    {
        sdr_list_destroy(sdr, newResultList, bpsec_asb_outboundTargetResultsRelease, NULL);
        newResultList = 0;
    }

    return newResultList;
}

/*****************************************************************************
 *                             PUBLIC FUNCTIONS                              *
 *****************************************************************************/


/******************************************************************************
 *
 * @brief Create and initialize an ASB in private working memory.
 *
 * This utility function allocates an ASB in private working memory and
 * initializes values and sets callbacks for data structures.
 *
 * @notes
 * 1.  Parameter and result lysts have dedicated delete functions which are
 *     set in this function. There is no need to custom-delete items in
 *     these lysts when destroying the ASB.
 *
 * @retval  NULL  - There was an error creating the ASB.
 * @retval  !NULL - The created ASB.
 *
 *****************************************************************************/

BpsecInboundASB *bpsec_asb_inboundAsbCreate(int memIdx)
{
    BpsecInboundASB *result = NULL;

    if((result = (BpsecInboundASB *) MTAKE(sizeof(BpsecInboundASB))) == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot allocate inbound ASB.", NULL);
    }
    else
    {
        memset((char *) result, 0, sizeof(BpsecInboundASB));
        result->scParms = lyst_create_using(memIdx);
        result->scResults = lyst_create_using(memIdx);

        if((result->scParms == NULL) || (result->scResults == NULL))
        {
            BPSEC_DEBUG_ERR("Cannot allocate inbound ASB lists.", NULL);

        	lyst_destroy(result->scParms);
        	lyst_destroy(result->scResults);
        	MRELEASE(result);
        	return NULL;
        }

        lyst_delete_set(result->scParms, bpsec_scv_lystCbDel, NULL);
        lyst_delete_set(result->scResults, bpsec_asb_inboundTargetResultRelease, NULL);
    }

    BPSEC_DEBUG_PROC("Returning " ADDR_FIELDSPEC, (uaddr) result);
    return result;
}



/******************************************************************************
 *
 * @brief Release resources associated with an ASB from private working memory.
 *
 * This utility function removes memory resources associated with an ASB.
 *
 * @param[in,out] asb - The ASB being deleted.
 *
 * @notes
 * 1.  The lysts used in this structure are presumed to have delete functions
 *     set at the time of creation of the ASB (from the bpsec_asb_inboundAsbCreate
 *     function).
 *
 * 2.  Once this function returns, the ASB MUST NOT be referenced by the
 *     calling function.
 *
 *****************************************************************************/

void bpsec_asb_inboundAsbDelete(BpsecInboundASB *asb)
{
	/* Step 0: Sanity CHecks. */
	if(asb == NULL)
	{
		return;
	}

	/* Step 1: Free resouces associated with the security source. */
	eraseEid(&(asb->scSource));

	/*
	 * Step 2: Free any lists of targets and parameters. It is assumed
	 *         that these lists have appropriate delete functions set.
	 */
	lyst_destroy(asb->scParms);
	lyst_destroy(asb->scResults);

	/* Step 3: Release memory for the ASB. */
	MRELEASE(asb);
}



/******************************************************************************
 *
 * @brief Generate an ASB from an acquired security block.
 *
 * This utility function accepts an Acquisition extension block in the course
 * of bundle acquisition, parses that block's serialized content into an
 * Abstract Security Block structure, and stores that structure as the
 * scratchpad object of the extension block.
 *
 * @param[in,out] blk - The acquisition extension block structure holding the
 *                      serialized security block. Also, the structure that
 *                      will keep a pointer to the deserialized ASB.
 *
 * @param[in]     wk  - The in-memory acquisition structure for the bundle.
 *
 * @notes
 * 1.  This function allocates memory for the scratchpad object using the
 *     MTAKE method.  This memory is released immediately if the function
 *     returns 0 or -1; if the function returns 1 (successful
 *     deserialization) then release of this memory must be ensured by
 *     subsequent processing.
 * \par
 * 2.  If we return a 0, the extension block is considered invalid and not
 *     usable; it should be discarded. The extension block is not discarded,
 *     though, in case the caller wants to examine it.  (The ASB structure,
 *     however, *is* discarded immediately. It never becomes the "object" of
 *     this extension block.)
 *
 * @retval -1 - There was a system error.
 * @retval  0 - The deserialization failed.
 * @retval  1 - The ASB was deserialized and placed in the blk's object.
 *
 *****************************************************************************/

int bpsec_asb_inboundAsbDeserialize(AcqExtBlock *blk, AcqWorkArea *wk)
{
     int              memIdx = getIonMemoryMgr();
     unsigned char   *cursor = NULL;
     unsigned int     unparsedBytes = 0;
     BpsecInboundASB *asb;

     BPSEC_DEBUG_PROC("(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC "%d)", (uaddr) blk, (uaddr) wk);


     /* Step 0: Sanity Checks. */
     CHKERR(blk);
     CHKERR(wk);
     BPSEC_DEBUG_INFO("Blk data length %d", blk->dataLength);


     /* Step 1: Create the Inbound ASB. */
     if((asb = bpsec_asb_inboundAsbCreate(memIdx)) == NULL)
     {
         BPSEC_DEBUG_ERR("Could not create ASB.", NULL);
         return -1;
     }

     /*
      * Step 2: Initialize parsing of the serialized security block.
      *         The cursor is placed at the start of the block-type-specific
      *         data in the extension block. This skips the extension block
      *         header.
      */
     unparsedBytes = blk->dataLength;
     cursor = ((unsigned char *)(blk->bytes)) + (blk->length - blk->dataLength);


     /* Step 3: Decode the list of targets for this security block.*/

     /*
      * Step 3.1: Process the targets array of the ASB. This will
      *           populate the target results array in the ASB.
      */
     if(bslasb_extractTargets(asb, memIdx, &cursor, &unparsedBytes) < 1)
     {
    	 BPSEC_DEBUG_ERR("Cannot extract target results.", NULL);
         bpsec_asb_inboundAsbDelete(asb);
         return 0;
     }

     /*
      * Step 3.2: Extract security context information from the block. This
      *           information will include the security context identifier,
      *           security context flags, and security context parameters.
      */
     if(bslasb_extractContext(asb, memIdx, &cursor, &unparsedBytes) < 1)
     {
    	 BPSEC_DEBUG_ERR("Cannot extract security context info.", NULL);
         bpsec_asb_inboundAsbDelete(asb);
         return 0;
     }

     /*
      * Step 3.3: Extract security results.
      */
     if(bslasb_extractTargetResults(asb, memIdx, &cursor, &unparsedBytes) < 1)
     {
    	 BPSEC_DEBUG_ERR("Cannot extract target results.", NULL);
         bpsec_asb_inboundAsbDelete(asb);
         return 0;
     }

     blk->size = sizeof(BpsecInboundASB);
     blk->object = asb;



     BPSEC_DEBUG_PROC("Returning 1", NULL);
     return 1;
}


/******************************************************************************
 *
 * @brief Create a target result structure to hold security results for a target.
 *
 * This utility function allocates an inbound target results structure. There
 * must beone such structure for each target of a security block.
 *
 * @param[in] tgt_id - The target identifier for this set of target results.
 *
 * @param[in] memIdx - The memory area to use for allocation.
 *
 * @notes
 * 1.  The individual results list is created but left empty. It is assumed
 *     that subsequent calls will populate the individual results for this
 *     target result.
 *
 * @retval  NULL - There was a system error.
 * @retval !NULL - The Created result.
 *
 *****************************************************************************/
BpsecInboundTargetResult *bpsec_asb_inboundTargetResultCreate(uvast tgt_id, int memIdx)
{
	BpsecInboundTargetResult *result = NULL;

	result = (BpsecInboundTargetResult *) MTAKE(sizeof(BpsecInboundTargetResult));
	CHKNULL(result);

	if((result->scIndTargetResults = lyst_create_using(memIdx)) == NULL)
	{
		MRELEASE(result);
		result= NULL;
	}
	else
	{
		result->scTargetId = tgt_id;
		lyst_delete_set(result->scIndTargetResults, bpsec_scv_lystCbDel, NULL);
	}

	return result;
}

/******************************************************************************
 * @brief Remove a target result from a security block.
 *
 * This function removes a security target from a security block. This happens
 * at either a security verifier (if the operation being verified failes) or
 * at a security acceptor.
 *
 * If the security operation being removed is the last security operation in
 * the security block, then the security block should also be removed from the
 * bundle.
 *
 * @param[in/out]  tgtResultElt - The target result entry in the security block
 *                                security results section.
 *
 * @param[in]      secBlkElt       - The security block entry in the incoming bundle
 *                                structure.
 *****************************************************************************/

void bpsec_asb_inboundTargetResultRemove(LystElt tgtResultElt, LystElt secBlkElt)
{
    AcqExtBlock        *secBlk = NULL;
    BpsecInboundASB    *asb = NULL;


    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr) tgtResultElt, (uaddr) secBlkElt);

    /*
     * Step 1: Remove this element from the target result lyst.
     *         We have callbacks for this.
     */
    lyst_delete(tgtResultElt);

    /*
     *  Step 2: If this was the last target in the block, then also remove the
     *         security block.
     */
    if((secBlk = (AcqExtBlock *) lyst_data(secBlkElt)) != NULL)
    {
        if((asb = (BpsecInboundASB *) (secBlk->object)) != NULL)
        {
            if (lyst_length(asb->scResults) == 0)
            {
                /*
                 * This will trigger the "clear" callback on the security block
                 * to free it's ASB.
                 */
                deleteAcqExtBlock(secBlkElt);
            }
        }
    }

    BPSEC_DEBUG_PROC("Returning...", NULL);
}



// TODO: Document function.


/*
 * Records an in-memory ASB into the SDR for storage.
 * 0 is OK
 * -1 is bad.
 * Replaced: // bpsec_recordAsb .. bsu_asb_record
 */
int  bpsec_asb_inboundAsbRecord(ExtensionBlock *newBlk, AcqExtBlock *oldBlk)
{
	Sdr			sdr = getIonsdr();
	BpsecInboundASB	*oldAsb;
	BpsecOutboundASB	newAsb;
	int			result;
	LystElt			elt;
	BpsecInboundTargetResult	*oldTarget;
	BpsecOutboundTargetResult	newTarget;
	Object			obj;

	BPSEC_DEBUG_PROC("(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC ")",
			        (uaddr) newBlk, (uaddr) oldBlk);

	/* Step 0: Sanity Checks. */
	CHKERR(newBlk);
	CHKERR(oldBlk);

	/* Step 1: If there is no object to record, we are done. */
	if (oldBlk->object == NULL || oldBlk->size == 0)
	{
		/*	Nothing to do.					*/
		newBlk->object = 0;
		newBlk->size = 0;
		return 0;
	}

	/* Step 2: Initialize the new ASB. */
	memset((char *) &newAsb, 0, sizeof(newAsb));


	/* Step 3: Copy fixed data from memory to the SDR. */
	oldAsb = (BpsecInboundASB *) (oldBlk->object);

	newAsb.scId = oldAsb->scId;
	newAsb.scFlags = oldAsb->scFlags;
	if((result = bpsec_util_EIDCopy(&newAsb.scSource, &oldAsb->scSource)) < 1)
	{
		BPSEC_DEBUG_ERR("Unable to copy EID.", NULL);
		return -1;
	}

	/* Step 4: Copy over the target results. */
	if((newAsb.scResults = sdr_list_create(sdr)) == 0)
	{
		BPSEC_DEBUG_ERR("Unable to create results list.", NULL);
		return -1;
	}

	for (elt = lyst_first(oldAsb->scResults); elt; elt = lyst_next(elt))
	{
		oldTarget = (BpsecInboundTargetResult *) lyst_data(elt);

		/*	Copy individual results for target to the SDR.	*/
		newTarget.scTargetId = oldTarget->scTargetId;

		if((newTarget.scIndTargetResults = bpsec_scv_memListRecord(sdr, 0, oldTarget->scIndTargetResults)) == 0)
		{
			bpsec_asb_outboundAsbDelete(sdr, &newAsb);
			return -1;
		}

		/* Allocate space for outbound target results. */
		obj = sdr_malloc(sdr, sizeof(newTarget));
		sdr_write(sdr, obj, (char *) &newTarget, sizeof(newTarget));
		if (sdr_list_insert_last(sdr, newAsb.scResults, obj) == 0)
		{
		    bpsec_asb_outboundAsbDelete(sdr, &newAsb);
			return -1;
		}
	}

	/* Step 5: Copy over parameters. */
	if((newAsb.scParms = bpsec_scv_memListRecord(sdr, 0, oldAsb->scParms)) < 1)
	{
		bpsec_asb_outboundAsbDelete(sdr, &newAsb);
		return -1;
	}

	/* Step 6: Allocate space for the new ASB. */
	if((newBlk->object = sdr_malloc(sdr, sizeof(BpsecOutboundASB))) == 0)
	{
	    bpsec_asb_outboundAsbDelete(sdr, &newAsb);
		return -1;
	}
	newBlk->size = sizeof(BpsecOutboundASB);


	/* Step 7: Write the ASB to the SDR. */
	sdr_write(sdr, newBlk->object, (char *) &newAsb, sizeof(newAsb));


	BPSEC_DEBUG_PROC("Returning 0", NULL);
	return 0;
}

// TODO: Document function.
void bpsec_asb_inboundTargetResultRelease(LystElt item, void *tag) // bsu_outTgtReleaseCb
{
    BpsecInboundTargetResult *result = (BpsecInboundTargetResult *) lyst_data(item);

    if(result)
    {
        lyst_destroy(result->scIndTargetResults);
    }

    MRELEASE(result);
}


// TODO: Document function.
Object bpsec_asb_outboundAsbCreate(Sdr sdr, unsigned int *size)
{
    Object obj = 0;

    CHKZERO(size);

    *size = sizeof(BpsecOutboundASB);

    if((obj = sdr_malloc(sdr, *size)) == 0)
    {
        *size = 0;
    }

    return obj;
}
// TODO: Document function.

void bpsec_asb_outboundTargetResultsRelease(Sdr sdr, Object eltData, void *arg)
{
    Object obj = sdr_list_data(sdr, eltData);

    bpsec_asb_outboundTargetResultDelete(sdr, obj);
}

// TODO: Document function.
void bpsec_asb_outboundTargetResultDelete(Sdr sdr, Object obj)
{
   if(obj != 0)
   {
       BpsecOutboundTargetResult result;
       sdr_read(sdr, (char*)&result, obj, sizeof(result));
       sdr_list_destroy(sdr, result.scIndTargetResults, bpsec_scv_sdrListCbDel, NULL);
       sdr_free(sdr, obj);
   }
}


// TODO: Document function.
// Does not free asb.
// deletes them individually.
void bpsec_asb_outboundAsbDelete(Sdr sdr, BpsecOutboundASB *asb)
{
    BPSEC_DEBUG_PROC("(sdr, "ADDR_FIELDSPEC")", (uaddr)asb);

    /* Step 0: Sanity CHecks. */
    if(asb != NULL)
    {

        /* Step 1: Free resources associated with the security source. */
        eraseEid(&(asb->scSource));

        /*
         * Step 2: Free any lists of targets and parameters. It is assumed
         *         that these lists have appropriate delete functions set.
         */
        sdr_list_destroy(sdr, asb->scParms, bpsec_scv_sdrListCbDel, NULL);
        sdr_list_destroy(sdr, asb->scResults, bpsec_asb_outboundTargetResultsRelease, NULL);
    }
    BPSEC_DEBUG_PROC("Returning.", NULL);
}

// TODO: Document function.
void bpsec_asb_outboundAsbDeleteObj(Sdr sdr, Object obj)
{
    BpsecOutboundASB asb;

    BPSEC_DEBUG_PROC("(sdr, %d)", obj);

    if (obj)
    {
        sdr_read(sdr, (char *) &asb, obj, sizeof(BpsecOutboundASB));
        bpsec_asb_outboundAsbDelete(sdr, &asb);

        sdr_free(sdr, obj);
    }
}


// TODO: Document function.
/*
 * Copy a block. Since we copy blocks as part of transmitting (not receiving) these are
 * outbound blocks.
 */
int      bpsec_asb_outboundAsbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
    Sdr sdr = getIonsdr();
    BpsecOutboundASB  oldAsb;
    BpsecOutboundASB  newAsb;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                    (uaddr) newBlk, (uaddr) oldBlk);


    /* Step 0: Sanity Checks. */
    CHKERR(newBlk);
    CHKERR(oldBlk);

    /* Step 1: Allocate the new copy of the block. */
    if((newBlk->object = bpsec_asb_outboundAsbCreate(sdr, &(newBlk->size))) == 0)
    {
        BPSEC_DEBUG_ERR("Failed to allocate outbound ASB.", NULL);
        BPSEC_DEBUG_PROC("Returning -1", NULL);
        return -1;
    }

    /*
     * Step 2: Initialize the ASBs for the copy.
     *         The "old" ASB is read from the SDR.
     *         The "new" ASB is initialized to all 0.
     */
    sdr_read(sdr, (char *) &oldAsb, oldBlk->object, sizeof(oldAsb));
    memset((char*) &newAsb, 0, sizeof(newAsb));

    /* Step 3: Shallow copy that which can be shallow copied. */
    newAsb.scFlags = oldAsb.scFlags;
    newAsb.scId = oldAsb.scId;

    /* Step 4: Deep copy the security source. */
    if(bpsec_util_EIDCopy(&(newAsb.scSource), &(oldAsb.scSource)) < 1)
    {
        BPSEC_DEBUG_ERR("Error copying EID.", NULL);
        bpsec_asb_outboundAsbDelete(sdr, &newAsb);
        sdr_free(sdr, newBlk->object);
        return -1;
    }

    /* Step 5: Deep copy the security target results. */
    if((newAsb.scResults = bslasb_outboundCopyTargetResults(sdr, oldAsb.scResults)) == 0)
    {
        BPSEC_DEBUG_ERR("Error copying target results.", NULL);
        bpsec_asb_outboundAsbDelete(sdr, &newAsb);
        sdr_free(sdr, newBlk->object);
        return -1;
    }

    /* Step 6: Deep copy the security parameters. */
    if((newAsb.scParms = bpsec_scv_sdrListCopy(sdr, oldAsb.scParms)) == 0)
    {
        BPSEC_DEBUG_ERR("Error copying security parameters.", NULL);
        bpsec_asb_outboundAsbDelete(sdr, &newAsb);
        sdr_free(sdr, newBlk->object);
        return -1;
    }

    /* Step 7 - Write copied block to the SDR. */
    sdr_write(sdr, newBlk->object, (char *) &newAsb, sizeof(newAsb));
    BPSEC_DEBUG_PROC("Returning 0", NULL);
    return 0;

}

// TODO: Document function.
/*
 * 1. Serialize the target array in real time.
 * 2. Create a linked lyst of results and serialize them next.
 * 3.
 * The approach here is to create a linked list for each entry to hold the
 * results, and keep a running size, allocate the buffer, and be done.
 *
 * We
 */

int bslasb_encodeTargets(Sdr sdr, BpsecOutboundASB *asb, sc_Def *def, BpsecSerializeData *tgtIds, BpsecSerializeData *tgtResults)
{
    Object elt = 0;
    BpsecSerializeData *tmpData = NULL;
    uint8_t *cursor = NULL;
    int i = 0;
    unsigned char arrayHdr[9];
    int arrayHdrLen = 0;
    OBJ_POINTER(BpsecOutboundTargetResult, target);
    int numTgts = 0;

    BPSEC_DEBUG_PROC("(sdr,"ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                     (uaddr) asb, (uaddr) def, (uaddr) tgtIds, (uaddr) tgtResults);

    /* Step 0: Sanity Checks. */
    CHKERR(asb);
    CHKERR(tgtIds);
    CHKERR(tgtResults);

    tgtResults->scSerializedLength = 0;

    /*
     * Step 1: Allocate space for the target array and temporary space for each
     *         target result.
     *
     *         We can allocate the full target array because we know the maximum size
     *         of the target ID array: # targets * max-encoded-size.
     *
     *         But each target result set can be of (wildly) varying sizes, so we
     *         will need to serialize each target result first and then add up the
     *         total length to see the final length of the results for the
     *         security block.
     */
    if((numTgts = sdr_list_length(sdr, asb->scResults)) <= 0)
    {
        BPSEC_DEBUG_ERR("Invalid number of targets: %d.", numTgts);
        return -1;
    }

    if((tgtIds->scSerializedText = MTAKE(numTgts * CBOR_MAX_UVAST_ENC_LENGTH)) == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", numTgts * CBOR_MAX_UVAST_ENC_LENGTH);
        return -1;
    }
    tgtIds->scSerializedLength = 0;
    cursor = tgtIds->scSerializedText;
    if((tmpData = MTAKE(numTgts * sizeof(BpsecSerializeData))) == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", numTgts * sizeof(BpsecSerializeData));
        MRELEASE(tgtIds->scSerializedText);
        return -1;
    }
    /*
     * Step 2: Start the array encoding for the target ID's list.
     *         We can do this now because we know how many target IDs there
     *         will be AND we have the sized buffer to hold the target id
     *         result list.
     */
    unsigned char *tmp = arrayHdr;
    arrayHdrLen = cbor_encode_array_open(numTgts, &tmp);
    tgtIds->scSerializedLength += arrayHdrLen;
    memcpy(cursor, arrayHdr, arrayHdrLen);
    cursor += arrayHdrLen;
    /*
     * Step 2: For each result, add an entry into the target id list
     *         and the target result list.
     */
    for (elt = sdr_list_first(sdr, asb->scResults); elt; elt = sdr_list_next(sdr, elt))
    {
        GET_OBJ_POINTER(sdr, BpsecOutboundTargetResult, target, sdr_list_data(sdr, elt));

        /* Step 3.1: Serialize the target ID into the target ID buffer. */
        tgtIds->scSerializedLength += cbor_encode_integer(target->scTargetId, &cursor);

        /* Step 3.2: Serialize the target results for this target. */
        if((tmpData[i].scSerializedText = bpsec_scv_sdrListSerialize(sdr, def, target->scIndTargetResults, &(tmpData[i].scSerializedLength))) == NULL)
        {
            BPSEC_DEBUG_ERR("Cannot serialize list.", NULL);
            MRELEASE(tgtIds->scSerializedText);
            MRELEASE(tmpData);
            return -1;
        }
        tgtResults->scSerializedLength += tmpData[i].scSerializedLength;
        i++;
    }

    /*
     * Step 3: Serialize the full set of target results. This involves sizing the target
     *         results buffer, encoding the array header, and copying over the individual
     *         target results sets for each target.
     *
     */

    /*
     * Step 3.1: Size, allocate, and initialize data associated with the
     *           resulting serialized buffer.
     *
     *           Happily, the size of the target result array is the same size
     *           as the target ID array. So, we can reuse the CBOR array header
     *           from above to be the same array header here.
     */

    tgtResults->scSerializedLength += arrayHdrLen;

    if((tgtResults->scSerializedText = MTAKE(tgtResults->scSerializedLength)) == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", tgtResults->scSerializedLength);
        MRELEASE(tgtIds->scSerializedText);
        MRELEASE(tmpData);
        return -1;
    }
    cursor = tgtResults->scSerializedText;

    /*
     * Step 3.2: Populate the result buffer, starting with the CBOR array header
     *           and followed by the individual value results.
     */
    memcpy(cursor, arrayHdr, arrayHdrLen);
    cursor += arrayHdrLen;

    for(i = 0; i < numTgts; i++)
    {
        memcpy(cursor, tmpData[i].scSerializedText, tmpData[i].scSerializedLength);
        cursor += tmpData[i].scSerializedLength;
    }

    MRELEASE(tmpData);

    BPSEC_DEBUG_PROC("-->1", NULL);
    return 1;
}

// TODO: Document function.
uint8_t *bslasb_encodeCtxInfo(BpsecOutboundASB *asb, unsigned int *length)
{
    int maxLen = 9 + 9 + 300;
    uint8_t *cursor = NULL;
    uint8_t *result = NULL;

    CHKNULL(asb);
    CHKNULL(length);

    if((result = MTAKE(maxLen)) == NULL)
    {
        return NULL;
    }
    cursor = result;
    *length = 0;

    *length += cbor_encode_integer(asb->scId, &cursor);
    *length += cbor_encode_integer(asb->scFlags, &cursor);
    *length += serializeEid(&(asb->scSource), cursor);

    return result;
}

// TODO: Update documentation
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
 *  10/20/20  S. Burleigh          Rewrite for BPv7 and BPSec
 *****************************************************************************/
/*
 * Serialize an ASB into a byte string.
 */
uint8_t *bpsec_asb_outboundAsbSerialize(uint32_t *length, BpsecOutboundASB *asb)
{
    Sdr     sdr = getIonsdr();

    BpsecSerializeData tgtIds;
    BpsecSerializeData tgtResults;
    BpsecSerializeData ctxInfo;
    BpsecSerializeData parms;

    uint8_t *serializedAsb = NULL;
    uint8_t *cursor = NULL;

    sc_Def def;
    sc_Def *defPtr = NULL;

    BPSEC_DEBUG_PROC("(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC ")", (uaddr) length, (uaddr) asb);

    /* Step 0: Sanity Checks and Initialization. */
    CHKNULL(length);
    CHKNULL(asb);

    if(bpsec_sci_defFind(asb->scId, &def))
    {
        defPtr = &def;
    }
    else
    {
        BPSEC_DEBUG_WARN("Could not find SC %d.", asb->scId);
    }

    /*
     * Step 1: Serialize the security block target ID array and target results
     *         array. We do this together since the target IDs and target results
     *         are stored in the ASB in a single structure.
     */

    if(bslasb_encodeTargets(sdr, asb, defPtr, &tgtIds, &tgtResults) < 1)
    {
        BPSEC_DEBUG_ERR("Cannot serialize ASB target information.", NULL);
        return NULL;
    }


    /*
     * Step 2: Serialize the security parameters if we have parameters.
     */

    if (sdr_list_length(sdr, asb->scParms) > 0)
    {
        asb->scFlags |= BPSEC_ASB_PARM;

        BPSEC_DEBUG_INFO("Serializing %d parameters.", sdr_list_length(sdr, asb->scParms));

        if((parms.scSerializedText = bpsec_scv_sdrListSerialize(sdr, defPtr, asb->scParms, &(parms.scSerializedLength))) == NULL)
        {
            MRELEASE(tgtIds.scSerializedText);
            MRELEASE(tgtResults.scSerializedText);
            return NULL;
        }
    }
    else
    {
        parms.scSerializedLength = 0;
    }


    /*
     * Step 3: Serialize context information. We do this after looking
     *         at security parms, since that check will set the scFlags.
     */
    if((ctxInfo.scSerializedText = bslasb_encodeCtxInfo(asb, &(ctxInfo.scSerializedLength))) == NULL)
    {
        MRELEASE(tgtIds.scSerializedText);
        MRELEASE(tgtResults.scSerializedText);
        MRELEASE(parms.scSerializedText);
        return NULL;
    }


    /* Step 4: Calculate the total size of the serialized ASB. */
    *length = tgtIds.scSerializedLength + ctxInfo.scSerializedLength + parms.scSerializedLength + tgtResults.scSerializedLength;


    /* Step 5: Allocate the resulting array and populate it. */
    if((serializedAsb = MTAKE(*length)) == NULL)
    {
        MRELEASE(tgtIds.scSerializedText);
        MRELEASE(tgtResults.scSerializedText);
        MRELEASE(parms.scSerializedText);
        MRELEASE(ctxInfo.scSerializedText);
        return NULL;
    }
    cursor = serializedAsb;

    memcpy(cursor, tgtIds.scSerializedText, tgtIds.scSerializedLength);
    cursor += tgtIds.scSerializedLength;

    memcpy(cursor, ctxInfo.scSerializedText, ctxInfo.scSerializedLength);
    cursor += ctxInfo.scSerializedLength;

    memcpy(cursor, parms.scSerializedText, parms.scSerializedLength);
    cursor += parms.scSerializedLength;

    memcpy(cursor, tgtResults.scSerializedText, tgtResults.scSerializedLength);
    cursor += tgtResults.scSerializedLength;

    MRELEASE(tgtIds.scSerializedText);
    MRELEASE(tgtResults.scSerializedText);
    MRELEASE(parms.scSerializedText);
    MRELEASE(ctxInfo.scSerializedText);

    BPSEC_DEBUG_INFO("data: " ADDR_FIELDSPEC ", length %d",
                     (uaddr) serializedAsb, *length);

    BPSEC_DEBUG_PROC(" Returning " ADDR_FIELDSPEC, (uaddr) serializedAsb);

    return serializedAsb;
}

// TODO: Update documentation
/******************************************************************************
 *
 * \par Function Name: bpsec_write_parms
 *
 * \par Purpose: This utility function writes the parms in an
 *       sci_inbound_parms structure to the SDR heap as
 *       BpsecOutboundTlv structures, appending them to the
 *       parmsData sdrlist of a bpsec outbound ASB.
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
 *  10/20/20  S. Burleigh   Update for BPv7 and BPsec.
 *****************************************************************************/
// TODO - note that this appends if the list already exists.

int      bpsec_asb_outboundParmsWrite(Sdr sdr, BpsecOutboundASB *asb, Lyst parms)
{

    BPSEC_DEBUG_PROC("(sdr,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr) asb, (uaddr)parms);

    if(bpsec_scv_memListRecord(sdr, asb->scParms, parms) == 0)
    {
        BPSEC_DEBUG_ERR("Cannot record list.", NULL);
        return -1;
    }

    return 0;
}




/******************************************************************************
 *
 * \par Function Name: bpsec_insertSecuritySource
 *
 * \par Purpose: Inserts security source into the Abstract Security Block.
 *       of an outbound bpsec block.
 *
 * \par Date Written:  TBD
 *
 * \retval  None
 *
 * \param[in]   bundle  The outbound bundle.
 * \param[in]   asb The outbound Abstract Security Block.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  ----------------------------------------
 *****************************************************************************/

void     bpsec_asb_outboundSecuritySourceInsert(Bundle *bundle, BpsecOutboundASB *asb)
{
    char        *eidString;
    char        *adminEid;
    MetaEid     metaEid;
    VScheme     *vscheme;
    PsmAddress  elt;
    int     result;

    /*  Get the local node's admin EID for the scheme by
     *  which the bundle's destination is identified, since
     *  we know the destination understands that scheme.    */

    readEid(&bundle->destination, &eidString);
    CHKVOID(eidString);
    adminEid = bpsec_util_localAdminEIDGet(eidString);
    MRELEASE(eidString);
    /*  Now note that this admin EID is the block's security
     *  source.                         */

    CHKVOID(parseEidString(adminEid, &metaEid, &vscheme, &elt));
    result = writeEid(&(asb->scSource), &metaEid);
    restoreEidString(&metaEid);
    CHKVOID(result == 0);
}




/******************************************************************************
 *
 * \par Function Name: bpsec_insert_target
 *
 * \par Purpose: This utility function inserts a single target for a
 *       BPSEC block.
 *
 * \par Date Written:  11/27/2019
 *
 * \retval int -- 0 on success, -1 on error
 *
 * \param[in|out] sdr    The SDR storing the result.
 * \param[in]     asb    The block to attach the target to.
 * \param[in]     nbr    The target block's number.
 *
 * \par Notes:
 *      1. The SDR object must be freed when no longer needed.
 *      2. This function does not start its own SDR transaction.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  11/27/19  S. Burleigh   Initial Implementation
 *****************************************************************************/
/* TODO: Note that this just makes a placeholder for the target result, it doesn't
 * generate the resule yet.
 */
int bpsec_asb_outboundTargetInsert(Sdr sdr, BpsecOutboundASB *asb, uint8_t nbr)
{

    Object              obj;
    BpsecOutboundTargetResult tgtResult;

    CHKERR(sdr && asb);
    tgtResult.scTargetId = nbr;
    if((tgtResult.scIndTargetResults = sdr_list_create(sdr)) == 0)
    {
        BPSEC_DEBUG_ERR("Can't allocate SDR list.", NULL);
        return -1;
    }

    if((obj = sdr_malloc(sdr, sizeof(BpsecOutboundTargetResult))) == 0)
    {
        sdr_list_destroy(sdr, tgtResult.scIndTargetResults, NULL, NULL);
        BPSEC_DEBUG_ERR("Can't allocate target object.", NULL);
        return -1;
    }

    sdr_write(sdr, obj, (char *) &tgtResult, sizeof(BpsecOutboundTargetResult));
    if (sdr_list_insert_last(sdr, asb->scResults, obj) == 0)
    {
        sdr_free(sdr, obj);
        sdr_list_destroy(sdr, tgtResult.scIndTargetResults, NULL, NULL);
        BPSEC_DEBUG_ERR("Can't append target to ASB.", NULL);
        return -1;
    }

    return 0;
}




