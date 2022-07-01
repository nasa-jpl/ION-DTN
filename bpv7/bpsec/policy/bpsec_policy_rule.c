/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bpsec_policy_rule.c
 **
 ** Description: This file implements processing specific to BPSec policy rules.
 **              Policy rules are used by the policy engine to associate
 **              security events with security actions.
 **
 ** Notes:
 **  Rules live in shared memory as they must be accessed in the context of the
 **  BPA, network management, and the utility files that use these functions.
 **
 **  Certain functions in this toolkit will collect and report on rules in the
 **  context of local memory (such as with a Lyst) in cases where read-only,
 **  local access is needed.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/YYYY  AUTHOR         DESCRIPTION
 **  -------  ------------   ---------------------------------------------
 **  12/2020  E. Birrane     Initial implementation
 **
 *****************************************************************************/


/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_policy_rule.h"
#include "../../utils/bpsecadmin_config.h"

/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION DEFINITIONS 							  +
 * +--------------------------------------------------------------------------+
 */


/******************************************************************************
 *
 * \par Function Name: eidsMatch
 *
 * \par Purpose: This function accepts two string EIDs and determines if they
 *      match.  Significantly, each EID may contain an end-of-string
 *      wildcard character ("*"). For example, the two EIDs compared
 *      can be "ipn:1.*" and "ipn*".
 *
 * \retval int 1 - The EIDs matched, counting wildcards.
 *             0 - The EIDs did not match.
 *
 * \param[in]   firstEid     - The first EID to compare.
 * \param[in]   firstEidLen  - The length of the first EID string.
 * \param[in]   secondEid    - The second EID to compare.
 * \param[in]   secondEidLen - The length of the second EID string.
 *
 * \par Notes: The wildcard character '*' is interpreted to function as *
 *             (matching 0 or more) NOT + (matching 1 or more).
 *****************************************************************************/

static int eidsMatch(char *firstEid, int firstEidLen, char *secondEid,
        int secondEidLen)
{
    int i;

    /* We do not match NULL strings. */
    if((firstEid == NULL) || (secondEid == NULL))
    {
        return 0;
    }

    for (i = 0; i < MAX(firstEidLen, secondEidLen); i++)
    {
        /* EID length mismatch */
        if ((firstEidLen < i) || (secondEidLen < i))
        {
            return 0;
        }

        /* Perform wildcard matching */
        else if ((firstEid[i] == '*') || (secondEid[i] == '*'))
        {
            return 1;
        }

        /* EIDs do not match */
        else if (firstEid[i] != secondEid[i])
        {
            return 0;
        }
    }

    /* EIDs are an exact match */
    return 1;
}



/******************************************************************************
 * @brief Creates the filter object that matches a rule to an extension block.
 *
 * @param[in] partition - The shared memory partition.
 * @param[in] bsrc  - Optional bundle source EID filter.
 * @param[in] bdest - Optional bundle destination EID filter.
 * @param[in] ssrc  - Optional security source EID filter.
 * @param[in] type  - Optional extension block type filter. -1 if not present.
 * @param[in] role  - Optional security role filter.
 * @param[in] scid  - Optional security context identifier.  0 if not present.
 * @param[in] svc   - Optional security service identifier.  0 if not present.
 *
 * @note
 * A filter must have AT LEAST ONE EID filter associated with it.
 * \par
 * This function will also populate the score associated with the filter.
 * \par
 * On error, a zeroed filter is returned. This can be caught by looking at
 * the filter flag result, which described which EIDs are used in the filter.
 * If the entire filter flag value is 0 then no filter elements are set, which
 * is an error condition.
 * \par
 * An EID is considered "provided" if its string value is not NULL. A block type
 * is considered provided if it's value is >= 0. A role is considered provided if
 * it is any OR'ing of the three role types. An SCID is considered provided if it
 * is any value other than 0 (which is a reserved SCID).
 *
 * @retval The built filter.
 *****************************************************************************/

BpSecFilter bslpol_filter_build(PsmPartition partition, char *bsrc, char *bdest, char *ssrc, int type, int role, int scid, int svc)
{
	BpSecFilter filter;

	/* Step 0: Create and clean a filter structure. */
	memset(&filter, 0, sizeof(filter));

	/* Step 1: Populate EID filter items based on what is passed in. */
	if((bsrc != NULL) && (strlen(bsrc) > 0))
	{
		filter.flags |= BPRF_USE_BSRC;
		filter.bsrc_len = strlen(bsrc);
		filter.bundle_src = bsl_ed_get_ref(partition, bsrc);
	}

	if((bdest != NULL) && (strlen(bdest) > 0))
	{
		filter.flags |= BPRF_USE_BDST;
		filter.bdest_len = strlen(bdest);
		filter.bundle_dest = bsl_ed_get_ref(partition, bdest);
	}

	if((ssrc != NULL) && (strlen(ssrc) > 0))
	{
		filter.flags |= BPRF_USE_SSRC;
		filter.ssrc_len = strlen(ssrc);
		filter.sec_src = bsl_ed_get_ref(partition, ssrc);
	}

	/*
	 * Step 2: If we have at least 1 EID filter condition, then we can
	 *         populate the rest of the filter items.
	 */
	if(filter.flags != 0)
	{
		/* Step 2.1: Populate remaining optional filter elements. */
		if((role & BPRF_SRC_ROLE) ||
		   (role & BPRF_VER_ROLE) ||
		   (role & BPRF_ACC_ROLE))
		{
			/* Make sure only role bits are set in the role value. */
			filter.flags |= (role & (BPRF_SRC_ROLE | BPRF_VER_ROLE | BPRF_ACC_ROLE));
		}

		if(type >= 0)
		{
			filter.flags |= BPRF_USE_BTYP;
			filter.blk_type = type;
		}

		if(scid != BPSEC_UNSUPPORTED_SC)
		{
			filter.flags |= BPRF_USE_SCID;
			filter.scid = scid;
		}

		/* Set the security service value, defaulting to 0 if it has not been provided. */
		if(svc >= 0)
		{
			filter.svc = svc;
		}
		else
		{
			filter.svc = 0;
		}

		/* Step 2.2: Calculate the specificity score for the filter. */
		bslpol_filter_score(partition, &filter);
	}

	/* Step 3: Return filter. */
	return filter;
}



/******************************************************************************
 * @brief Calculate the specificity score of a filter.
 *
 * Filter scores are used to "sort" the applicability of a rule to a given
 * security block.  Filters with higher scores are considered to be more
 * specific.  This function applies a score to a filter. Once calculated,
 * a filter score does not need to be re-calculated unless the filter is
 * changed.
 *
 * @param[in]  partition - The shared memory partition.
 * @param[out] filter    - The filter being scored.
 *
 * @note
 *****************************************************************************/

void  bslpol_filter_score(PsmPartition partition, BpSecFilter *filter)
{
	int len = 0;
	char *curEID = NULL;

	CHKVOID(filter);

	filter->score = 0;

	if((filter->flags & BPRF_USE_BSRC) && (filter->bundle_src != 0))
	{
		curEID = (char *) psp(partition, filter->bundle_src);
		len = istrlen(curEID, MAX_EID_LEN);
		filter->score += (curEID[len-1] == RADIX_PREFIX_WILDCARD) ?
				BPSEC_RULE_SCORE_PARTIAL : BPSEC_RULE_SCORE_FULL;
	}

	if((filter->flags & BPRF_USE_BDST) && (filter->bundle_dest != 0))
	{
		curEID = (char *) psp(partition, filter->bundle_dest);
		len = istrlen(curEID, MAX_EID_LEN);
		filter->score += (curEID[len-1] == RADIX_PREFIX_WILDCARD) ?
				BPSEC_RULE_SCORE_PARTIAL : BPSEC_RULE_SCORE_FULL;
	}

	if((filter->flags & BPRF_USE_SSRC) && (filter->sec_src != 0))
	{
		curEID = (char *) psp(partition, filter->sec_src);
		len = istrlen(curEID, MAX_EID_LEN);
		filter->score += (curEID[len-1] == RADIX_PREFIX_WILDCARD) ?
				BPSEC_RULE_SCORE_PARTIAL : BPSEC_RULE_SCORE_FULL;
	}

	if(filter->flags & BPRF_USE_BTYP)
	{
		filter->score += BPSEC_RULE_SCORE_FULL;
	}

	if(filter->flags & BPRF_USE_SCID)
	{
		filter->score += BPSEC_RULE_SCORE_FULL;
	}
}



// TODO Update these comments.
/******************************************************************************
 * @brief This function will apply the provided policy rule (with a
 *               security policy role of either verifier of acceptor) to the
 *               block identified by its block number.
 *
 * @param[in]  wk      -  Work area holding bundle information.
 * @param[in]  polRule -  The policy rule describing the required security
 *                        operation in the bundle to be verified/processed.
 * @param[in]  tgtNum  -  Block number of the security target block.
 *
 * @retval <0 - An error occurred while applying the policy rule.
 * @retval  1 - The policy rule was successfully applied to the bundle.
 *****************************************************************************/

int bslpol_proc_applyReceiverPolRule(AcqWorkArea *wk, BpSecPolRule *polRule,
        int service,
        AcqExtBlock *secBlk, BpsecInboundASB *asb, BpsecInboundTargetResult *tgtResult,
        sc_Def *def, LystElt *tgtBlkElt, size_t *tgtBlkOrigLen)
{
     PsmPartition wm = getIonwm();
     int result = -1;
     sc_state state;
     LystElt tmp = NULL;

     BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC","ADDR_FIELDSPEC","
                         ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                      (uaddr) wk, (uaddr) polRule, service, (uaddr) secBlk, (uaddr) asb, (uaddr) tgtResult,
                      (uaddr) def, (uaddr) tgtBlkElt, (uaddr) tgtBlkOrigLen);


     /*
      * Step 1 - Make sure the target block exists in the bundle. The only
      *          way the target block would not exist in the bundle is if
      *          it is an extension block, and the extension block does not
      *          appear in the bundle.
      */

     if( (tgtResult->scTargetId != PrimaryBlk) &&
         (tgtResult->scTargetId != PayloadBlk) &&
         ((tmp = getAcqExtensionBlock(wk, tgtResult->scTargetId)) == NULL))
     {
         return -1;
     }

     /*
      * If we caller wants us to return the target block and its original length
      * store that information, unless we don't have it.
      */
     if(tgtBlkElt != NULL)
     {
         /* Store the LystELt of the target block in thew ACQ area. */
         *tgtBlkElt = tmp;
     }

     /* Store the block length, if the caller wanted it. */
     if(tgtBlkOrigLen != NULL)
     {

         if(tgtResult->scTargetId == PrimaryBlk)
         {
             *tgtBlkOrigLen = wk->headerLength; // TODO make sure this is right.
         }
         else if(tgtResult->scTargetId == PayloadBlk)
         {
             *tgtBlkOrigLen = wk->bundle.payload.length;
         }
         else
         {
             AcqExtBlock *tgtBlk = NULL;

             /* If we can't find the block, that's a bigger issue.. */
             if((tgtBlk = (AcqExtBlock *) lyst_data(tmp)) == NULL)
             {
                 *tgtBlkOrigLen = 0;
                 return -1;
             }
             *tgtBlkOrigLen = tgtBlk->length;
         }
     }


     def->scStateInit(wm, &state, secBlk->number, &def, BPSEC_RULE_ROLE_IDX(polRule), service, asb->scSource, polRule->sc_parms, asb->scParms, lyst_length(asb->scResults));

     result = def->scProcInBlk(&state, wk, asb, tmp, tgtResult);

     def->scStateClear(&state);

     BPSEC_DEBUG_PROC("Returning %d", result);
     return result;
}

// TODO update these comments.
/******************************************************************************
 * @brief Apply a security-source policy rule to an outgoing bundle.
 *
 * This function applies the provided policy rule (with a security policy role
 * of source) to the block identified by its block number. The block is either
 * added as a target of an existing security block, or a new security block'
 * is created for that target block.
 *
 * @param[in/out]  bundle  -  Current, working bundle.
 * @param[in]      polRule -  The policy rule describing the required security
 *                            operation in the bundle to be added.
 * @param[in]      tgtNum  -  Block number of the security target block.
 *
 * @retval <=0 - An error occurred while applying the policy rule.
 * @retval  1 - The policy rule was successfully applied to the bundle.
 *****************************************************************************/

int bslpol_proc_applySenderPolRule(Bundle *bundle, BpBlockType secBlkType, BpSecPolRule *polRule, int tgtNum)
{
    Sdr sdr = getIonsdr();
    PsmPartition wm = getIonwm();
    Object blkObj = 0;
    ExtensionBlock blk;
    BpsecOutboundASB asb;
    sc_Def def;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC",%d)",
                     (uaddr)bundle, secBlkType, (uaddr) polRule, tgtNum);

    /* Step 0: Sanity Checks */
    CHKERR(bundle);
    CHKERR(polRule);
    CHKERR(tgtNum >= 0);

    /*
     * Step 1: Retrieve the BIB profile using the security context ID provided
     *         in the policy rule.
     * */

    if(bpsec_sci_defFind(polRule->filter.scid, &def) < 1)
    {
        BPSEC_DEBUG_ERR("Unsupported SC id %d.", polRule->filter.scid);
        return -1;
    }

    /*
     * Step 2: Confirm that we can apply this security operation to the given
     *         security target. If there is an existing BIB we can add to, we
     *         will find that out, too.
     */

    // TODO - when calling this for a BCB we need to get the bibObj back and
    //        add it to the encryption list.

#if 0
// TODO
// We need to add logic that says if we are applying a BCB to a target that is covered by a BIB,
// then we need to cover the BIB too.  This MIGHT mean splitting the BIB apart so it can be encrypted.
// This probably should be done AFTER we have worked through all the rule... but maybe not.
// Leaving this #ifdef 0 here to force discussion of the issue...




    /*    The security block we found earlier must be a BIB.
     *    (If it were a BCB, we wouldn't have added the target
     *    block as a target of the new BCB.)            */

    bibObj = (Object) sdr_list_data(sdr, bibElt);
    sdr_read(sdr, (char *) &bib, bibObj, sizeof(ExtensionBlock));
    sdr_read(sdr, (char *) &bibAsb, bib.object, bib.size);
    if (sdr_list_length(sdr, bibAsb.secResults) == 1)
    {
        /*    The target block is the sole target of this
         *    BIB.  Just add this BIB as a target of the BCB.    */

        if (bpsec_asb_outboundTargetInsert(sdr, asb, bib.number) < 0)
        {
            return -1;
        }

        return 0;
    }

    /*    More complicated.  Must clone this BIB, such that
     *    the original BIB no longer signs the target block
     *    -- only the (new) clone BIB does so.  Then add the
     *    clone BIB as an additional target of the new BCB.    */

    memcpy((char *) &clone, (char *) &bib, sizeof(ExtensionBlock));
    memcpy((char *) &cloneAsb, (char*) &bibAsb, sizeof(BpsecOutboundASB));
    for (resultElt = sdr_list_first(sdr, bibAsb.secResults); resultElt;
            resultElt = sdr_list_next(sdr, resultElt))
    {
        resultObj = sdr_list_data(sdr, resultElt);
        sdr_read(sdr, (char *) &result, resultObj,
                sizeof(BpsecOutboundTargetResult));
        if (result.secTargetId == targetBlockNumber)
        {
            break;
        }
    }

    CHKERR(resultElt);    /*    System error if didn't find it.    */

    /*    Must move this target to the clone BIB.  First
     *    remove it from the original BIB's list of targets
     *    and re-serialize the original BIB.            */

    sdr_list_delete(sdr, resultElt, NULL, NULL);
    serializedAsb = bpsec_asb_outboundAsbSerialize((uint32_t *) &(bib.dataLength),
            &bibAsb);
    CHKERR(serializedAsb);
    if (serializeExtBlk(&bib, (char *) serializedAsb) < 0)
    {
        MRELEASE(serializedAsb);
        putErrmsg("Failed re-serializing cloned BIB.", NULL);
        return -1;
    }

    MRELEASE(serializedAsb);

    /*    Now fix up the clone BIB (its sole target is the
     *    block we're adding as a BCB target) and add it as
     *    an additional target of the new BCB.            */

    //TODO Watch the CHKERR here... they auto-return and we will leak created lists.
    cloneAsb.secResults = sdr_list_create(sdr);
    CHKERR(cloneAsb.secResults);
    sdr_list_insert_last(sdr, cloneAsb.secResults, resultObj);
    cloneAsb.secContextParms = sdr_list_create(sdr);
    CHKERR(cloneAsb.secContextParms);
    clone.object = sdr_malloc(sdr, clone.size);
    CHKERR(clone.object);
    sdr_write(sdr, clone.object, (char *) &cloneAsb, clone.size);
    cloneObj = attachExtensionBlock(BlockIntegrityBlk, &clone, bundle);
    CHKERR(cloneObj);
    if (bibAttach(bundle, &clone, &cloneAsb) < 0)
    {
        putErrmsg("Failed attaching clone BIB.", NULL);
        return -1;
    }

#endif

// TODO might not want to pass in NULL here is we can get the possible bibBlk that we must also encrypt if
    // secBlkType == BCB.  See above #ifdef 0.

    if(bpsec_util_checkOutboundSopTarget(bundle, &def, wm, polRule->sc_parms, secBlkType, tgtNum, NULL, &blkObj) < 1)
    {
        BPSEC_DEBUG_INFO("Unable to apply security to target %d.", tgtNum);
        return 0;
    }

    /*
     * Step 3:  If there isn't a usable security block in the bundle already, we need to
     *          create our own.
     *
     *          TODO: If creating a block, just return the ASB and the blk itself,
     *                 so we don't have to re-read it from the SDR in step 4.
     */
    if(blkObj == 0)
    {
        blkObj = bpsec_util_OutboundBlockCreate(bundle, secBlkType, &def, polRule->sc_parms);
        if(blkObj == 0)
        {
            return 0;
        }
    }

    /*
     * Step 4: We now have a bibBlock, either one we are re-using or one that
     *         was created for us. Read the ExtensionBlock and associated ASB
     *         in from the SDR.
     */
    sdr_read(sdr, (char*) &blk, blkObj, sizeof(ExtensionBlock));
    sdr_read(sdr, (char*) &asb, blk.object, blk.size);


    /*
     * Step 5: Add the target to the block. This doesn't add the result yet,
     *         because we don't have a result. But it will add the target to
     *         the block in the SDR so that when it is time to send the
     *         bundle, a result over the target will be created.
     */
    if (bpsec_asb_outboundTargetInsert(sdr, &asb, tgtNum) < 0)
    {
        return -1;
    }


    return 1;
}




/******************************************************************************
 * @brief Creates a policy rule from components.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in] desc          - The rule description
 * @param[in] id            - The "user" ID of the rule.
 * @param[in] flags         - Special processing flags for the rule.
 * @param[in] filter        - The rule filter.
 * @param[in] sec_parms     - The smlist of security context parameters (or 0);
 * @param[in] events        - The event set for the rule.
 *
 * @note
 * The rule id is expected to be a unique identifier for the rule.
 * \par
 * The filter is expected to be properly filled in, such as from a call to
 * the function bslpol_filter_build.
 * \par
 * If the eventset is *not* a reference into the eventset database, then the
 * flags passed in to this function MUST indicate that the eventset is an
 * anonymous event set associated only with this rule.
 *
 * @retval !0 - The created rule.
 * @retval 0 - Error.
 *****************************************************************************/

PsmAddress bslpol_rule_create(PsmPartition partition, char *desc, uint16_t id, uint8_t flags,
		                      BpSecFilter filter, PsmAddress sec_parms, PsmAddress events)
{
	PsmAddress ruleAddr = 0;
	BpSecPolRule *rulePtr = NULL;
	SecVdb	*secvdb = getSecVdb();

	BPSEC_DEBUG_PROC("(partition,%s,%d,0x%x,filter,%d,%d",desc?desc:"null", id, flags, sec_parms, events);

	/* Step 1: Allocate the security policy rule. */
	if (secvdb == NULL)
	{
	    BPSEC_DEBUG_ERR("Could not get security volitile DB.", NULL);
	    return 0;
	}

	CHKZERO(ruleAddr = psm_zalloc(partition, sizeof(BpSecPolRule)));
	rulePtr = (BpSecPolRule*) psp(partition, ruleAddr);

	/* Step 2: Populate the security policy rule. */
	memset(rulePtr->desc, 0, BPSEC_RULE_DESCR_LEN);
	if(desc)
	{
		istrcpy(rulePtr->desc, desc, BPSEC_RULE_DESCR_LEN);
	}
	rulePtr->idx = sm_list_length(partition, secvdb->bpsecPolicyRules);
	rulePtr->user_id = id;
	rulePtr->flags = flags;
	rulePtr->filter = filter;
	rulePtr->eventSet = events;
	rulePtr->sc_parms = sec_parms;

	BPSEC_DEBUG_INFO("Added rule id %d with %d parameters.", id, sm_list_length(partition, sec_parms));

	/* Step 3: Return the new rule. */
	return ruleAddr;
}



/******************************************************************************
 * @brief Releases resources associated with a rule.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     ruleAddr  - The rule being deleted.
 *
 * @note
 * Deleting a rule is NOT the same as removing it from its data structures. It
 * is assumed that all data structures holding references to this rule have
 * been removed BEFORE this function is called to delete the rule.
 * \par
 * The rule MUST NOT be referenced after this function completes.
 *****************************************************************************/

void bslpol_rule_delete(PsmPartition partition, PsmAddress ruleAddr)
{
	BpSecPolRule *rulePtr = NULL;

	BPSEC_DEBUG_PROC("(partition,"ADDR_FIELDSPEC")", (uaddr) ruleAddr);

	/* Step 0: Sanity checks. */
	CHKVOID(partition);

	/*
	 * Step 1: We grab the rule pointer to reset memory as well as
	 *         to clean up an anonymous eventset if one is associated
	 *         with the rule.
	 */
	if((rulePtr = (BpSecPolRule*) psp(partition, ruleAddr)) == NULL)
	{
		BPSEC_DEBUG_INFO("No rule found at address %d", ruleAddr);
		return;
	}


	BPSEC_DEBUG_INFO("Attempting to remove rule from SDR.", NULL);
	/* Step 2: Remove the rule from the SDR. */
	bslpol_sdr_rule_forget(partition, ruleAddr);

	/*
	 * Step 3: If the rule has an anonymous event set, then the rule must
	 *         clean that up as part of releasing resources. Otherwise, the
	 *         delete function MUST NOT remove the event set as other rules
	 *         could be referencing a shared eventset.
	 */
	if(BPSEC_RULE_ANON_ES(rulePtr))
	{
		bsles_destroy(partition, rulePtr->eventSet, psp(partition, rulePtr->eventSet));
	}

	BPSEC_DEBUG_INFO("Removing rule from shared memory.", NULL);

	/* Step 4: Release memory associated with the rule. */
	memset(rulePtr, 0, sizeof(BpSecPolRule));
	psm_free(partition, ruleAddr);
}



/******************************************************************************
 * @brief Returns the policy rule address associated with a unique user id.
 *
 * @param[in] partition - The shared memory partition.
 * @param[in] user_id  - The user id of the desired rule
 *
 * @note
 * This is a O(n) walk through the rules since we do not index by
 * user ID. Since this is likely called in response to a user
 * action, this is an OK wait time.
 * \par
 * Since rules exist in shared memory, the pointer returned by this function
 * is ONLY valid in the context of the process calling this function.
 *
 * @retval !0 - The rule.
 * @retval 0 - No rules found with that user id.
 *****************************************************************************/

PsmAddress bslpol_rule_get_addr(PsmPartition partition, int user_id)
{
	PsmAddress eltAddr = 0;
	PsmAddress ruleAddr = 0;
	BpSecPolRule *rulePtr = NULL;
	SecVdb	*secvdb = getSecVdb();

	if (secvdb == NULL) return 0;

	/* Step 1: Walk through the list... */
	for(eltAddr = sm_list_first(partition, secvdb->bpsecPolicyRules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{
		ruleAddr = sm_list_data(partition, eltAddr);
		rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);

		/* Step 1.1: If we see a user_id match, return the rule. */
		if((rulePtr) && (rulePtr->user_id == user_id))
		{
			return ruleAddr;
		}
	}

	/* Step 2: if there is no match, return no match. */
	return 0;
}



/******************************************************************************
 * @brief Returns a list of every rule known by the policy engine that would
 *        match the set of provided criteria.
 *
 *  This function is used to provide a user with every known rule that could
 *  match a given set of information for a theoretical extension block.
 *
 * @param[in] partition - The shared memory partition
 * @param[in] tag       - A search tag populated with rule-matching criteria.
 *
 * @note
 * The returned list elements MUST NOT be altered/freed by the calling function.
 * \par
 * The returned list, itself, MUST be destroyed by the calling function.
 * \par
 * The returned list is sorted by highest score (then highest idx when scores are same).
 * \par
 * The returned list contains pointers to rules in shared memory and these
 * pointers are ONLY valid in the context of the calling process.
 *
 * @retval !NULL - List of rules
 * @retval NULL  - No rules match.
 *****************************************************************************/

Lyst bslpol_rule_get_all_match(PsmPartition partition, BpSecPolRuleSearchTag tag)
{
	Lyst rules = NULL;
	PsmAddress eltAddr = 0;
	PsmAddress ruleAddr = 0;
	BpSecPolRule *rulePtr = NULL;
	SecVdb	*secvdb = getSecVdb();

	if (secvdb == NULL) return NULL;

	/* Step 1: Check every rule that we have. */
	for(eltAddr = sm_list_first(partition, secvdb->bpsecPolicyRules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{
		ruleAddr = sm_list_data(partition, eltAddr);
		rulePtr = psp(partition, ruleAddr);

		/* Step 2.1: If the rule is a match...*/
		if(bslpol_rule_matches(partition, rulePtr, &tag) == 1)
		{
			/* Step 2.1.1: Create the list, if this is the first matched rule. */
			if(rules == NULL)
			{
				rules = lyst_create();
				lyst_direction_set(rules, LIST_SORT_DESCENDING);
				lyst_compare_set(rules, (LystCompareFn)bslpol_cb_rulelyst_compare_score);
			}

			/* Step 2.1.2: Insert the rule (sorted). */
			lyst_insert(rules, rulePtr);
		}
	}

	return rules;
}

/******************************************************************************
 * @brief Returns the "best" - rule with highest score indicating specificity -
 *        rule from every rule known by the policy engine that would
 *        match the set of provided criteria.
 *
 *  This function is used to provide a user with the best known rule that could
 *  match a given set of information for a theoretical extension block.
 *
 * @param[in] partition - The shared memory partition
 * @param[in] tag       - A search tag populated with rule-matching criteria.
 *
 * @note
 * The returned pointer is a rule in shared memory that is ONLY valid in the 
 * context of the calling process.
 * @note
 * This function should only be used when performing a bpsec policy FIND
 * for the best rule. If intent is to find the best rule for an actual
 * extension block - not hypothetical - the bslpol_rule_get_best_match
 * function should be used.
 *
 * @retval !NULL - List of rules
 * @retval NULL  - No rules match.
 *****************************************************************************/
BpSecPolRule *bslpol_rule_find_best_match(PsmPartition partition, BpSecPolRuleSearchTag tag)
{
	Lyst match_rules = NULL;
	LystElt elt;

	/* Step 1: Retrieve all security policy rules matching the find criteria. */
	match_rules = bslpol_rule_get_all_match(partition, tag);

	/* Step 2.1: If no matching rules are found, return NULL */
	if(lyst_length(match_rules) <= 0)
	{
		return NULL;
	}

	/* Step 2.2: If one or more rules are found that match the find criteria, 
	 * return the rule with the greatest score (the best rule). This will always
	 * be the first rule in the lyst as they are sorted by score. */
	elt = lyst_first(match_rules);
	BpSecPolRule *rulePtr = (BpSecPolRule *) lyst_data(elt);

	/* Step 2.3: Destroy the rule lyst, created in the call to 
	 * bslpol_rule_get_all_match */
	lyst_destroy(match_rules);

	return rulePtr;
}

/******************************************************************************
 * @brief Returns the policy rule that is the "best match" for a given
 *        extension block.
 *
 * @param[in] partition - The shared memory partition.
 * @param[in] tag       - A search tag populated with rule-matching criteria.
 *
 * @note
 * The returned rule MUST NOT be altered/freed by the calling function.
 * \par
 * Because rules are stored in shared memory, the pointer returned by this
 * function can only be accessed in the context of the caller's process.
 *
 * @retval !NULL - The best rule.
 * @retval NULL - No rules match.
 *****************************************************************************/

BpSecPolRule *bslpol_rule_get_best_match(PsmPartition partition, BpSecPolRuleSearchTag criteria)
{
	BpSecPolRuleSearchBestTag tag;
	char default_key[2] = "*";
	char *search_key = NULL;
	SecVdb	*secvdb = getSecVdb();

	if (secvdb == NULL) return NULL;

	/* Step 1: Populate the search tag. */
	tag.search = criteria;
	tag.best_rule = NULL;

	/* Step 2: Walk through each index... */
	search_key = (criteria.bsrc) ? criteria.bsrc : default_key;
	radix_foreach_match(partition,  secvdb->bpsecRuleIdxBySrc, search_key,  RADIX_MATCH_FULL, (radix_match_fn)bslpol_cb_ruleradix_search_best, &tag);

	search_key = (criteria.bdest) ? criteria.bdest : default_key;
	radix_foreach_match(partition,  secvdb->bpsecRuleIdxByDest, search_key,  RADIX_MATCH_FULL, (radix_match_fn)bslpol_cb_ruleradix_search_best, &tag);

	search_key = (criteria.ssrc) ? criteria.ssrc : default_key;
	radix_foreach_match(partition,  secvdb->bpsecRuleIdxBySSrc, search_key,  RADIX_MATCH_FULL, (radix_match_fn)bslpol_cb_ruleradix_search_best, &tag);

	/* Step 3: Return the "best" rule for this circumstance. */
	return tag.best_rule;
}



/******************************************************************************
 * @brief Returns the policy rule pointer associated with a unique user id.
 *
 * @param[in] partition - The shared memory partition.
 * @param[in] user_id  - The user id of the desired rule
 *
 * @note
 * This is a O(n) walk through the rules since we do not index by
 * user ID. Since this is likely called in response to a user
 * action, this is an OK wait time.
 * \par
 * Since rules exist in shared memory, the pointer returned by this function
 * is ONLY valid in the context of the process calling this function.
 *
 * @retval !NULL - The rule.
 * @retval NULL - No rules found with that user id.
 *****************************************************************************/

BpSecPolRule *bslpol_rule_get_ptr(PsmPartition partition, int user_id)
{
	PsmAddress ruleAddr = bslpol_rule_get_addr(partition, user_id);
	return psp(partition, ruleAddr);
}



/******************************************************************************
 * @brief Inserts a rule into the BPSec Policy Engine.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     ruleAddr  - The rule being inserted
 * @param[in]     remember  - Whether to persist the rule in the SDR
 *
 * @note
 * TODO Check Returns.
 * \par
 * Once inserted, the rule MUST NOT be altered/freed by the calling function.
 *
 * @retval 1  - Success
 * @retval 0  - Error
 * @retval -1 - System Error.
 *****************************************************************************/

int bslpol_rule_insert(PsmPartition partition, PsmAddress ruleAddr, int remember)
{
	BpSecPolRule *rulePtr = NULL;
	BpSecEventSet *esPtr = NULL;
	char *curEID = NULL;
	SecVdb	*secvdb = getSecVdb();

	/* Step 0: Sanity Checks. */
	CHKZERO(partition);
	if (secvdb == NULL) return 0;

	if((rulePtr = (BpSecPolRule *) psp(partition, ruleAddr)) == NULL)
	{
		return 0;
	}

	/* Step 1: insert the rule into the shared memory list of rules. */
	sm_list_insert(partition, secvdb->bpsecPolicyRules, ruleAddr, bslpol_cb_rule_compare_idx, rulePtr);

	/*
	 * Step 2: Track that another rule is using the given event set.
	 *         TODO: When anonymous event sets are supported, this needs to
	 *               be changed to only supporting named event sets.
	 */
	if((esPtr = (BpSecEventSet*) psp(partition, rulePtr->eventSet)) != NULL)
	{
		esPtr->ruleCount++;
	}

	/* Step 3: Based on the rule's filters, add it into various indexers. */
	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_src);
		if(radix_insert(partition, secvdb->bpsecRuleIdxBySrc, curEID, ruleAddr,(radix_insert_fn)bslpol_cb_ruleradix_insert, NULL) < 0)
		{
			bslpol_rule_remove(partition, ruleAddr);
			return 0;
		}
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_dest);
		if(radix_insert(partition, secvdb->bpsecRuleIdxByDest, curEID, ruleAddr,(radix_insert_fn)bslpol_cb_ruleradix_insert, NULL) < 0)
		{
			bslpol_rule_remove(partition, ruleAddr);
			return 0;
		}
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.sec_src);
		if(radix_insert(partition, secvdb->bpsecRuleIdxBySSrc, curEID, ruleAddr,(radix_insert_fn)bslpol_cb_ruleradix_insert, NULL) < 0)
		{
			bslpol_rule_remove(partition, ruleAddr);
			return 0;
		}
	}

	/* Step 4: Persist the rule to the SDR. */
	if(remember)
	{
		bslpol_sdr_rule_persist(partition, ruleAddr);
	}

	return 1;
}



/******************************************************************************
 * @brief Determines if a rule matches a given set of search criteria
 *
 * @param[in] partition - The shared memory partition
 * @param[in] rulePtr   - The rule being evaluated
 * @param[in] tag       - Search criteria representing information associated with
 *                        an extension block
 * 
 * @note To check if the security policy rule provided matches the criteria in 
 *  the search tag, the following steps are taken:
 *  1. If the security role is present in the security policy rule, check that
 *     it matches the role in the search tag.
 *  2. If the target block type is present in the security policy rule, check that
 *     it matches the block type in the search tag.
 *  3. If the security role for the security policy rule is:
 * 			security verifier OR security acceptor
 *     (indicating this is a receive-side security policy rule), check that the
 * 	   security context ID for that rule matches the search tag's sc_id.
 *  4. If the search tag and security policy rule specify the bundle source, check
 *     that these EIDs match (accounting for wildcards).
 *  5. If the search tag and security policy rule specify the bundle destination, check
 *     that these EIDs match (accounting for wildcards).
 *  6. If the search tag and security policy rule specify the security source, check
 *     that these EIDs match (accounting for wildcards).
 * 
 * @retval 1  - The rule matches the search criteria
 * @retval 0  - The rule does not match
 * @retval -1 - There was a system error
 *****************************************************************************/

int bslpol_rule_matches(PsmPartition partition, BpSecPolRule *rulePtr, BpSecPolRuleSearchTag *tag)
{
	char *curEID = NULL;

	/* Step 0: Sanity Checks. */
	CHKERR(partition);
	CHKERR(rulePtr);
	CHKERR(tag);

	/* Step 1: Check the fast cases first, so we can throw away rules early. */

	if ((BPSEC_RULE_ROLE_IDX(rulePtr)) && (tag->role != 0))
	{
		if ((tag->role & rulePtr->filter.flags) == 0)
		{
			return 0;
		}
	}

	if ((BPSEC_RULE_BTYP_IDX(rulePtr)) && (tag->type != -1))
	{
		if (tag->type != rulePtr->filter.blk_type)
		{
			return 0;
		}
	}

	/* The sc_id field in the search tag is set to unsupported if not specified in
	   the find command. */
	if(BPSEC_RULE_SCID_IDX(rulePtr))
	{

		/* If the sc_id in the search tag is set to unsupported, do not use for matching.
		   If the sc_id in the search tag does not match the sc_id associated with
		   the current security policy rule. */
		if((tag->scid != BPSEC_UNSUPPORTED_SC) && (tag->scid != rulePtr->filter.scid))
		{
			return 0;
		}
	}

	if (tag->es_name != NULL)
	{
		BpSecEventSet *esPtr = NULL;
		esPtr = (BpSecEventSet *) psp(partition, rulePtr->eventSet);
	
		if (bsles_match(esPtr, tag->es_name) != 0)
		{
			return 0;
		}
	}

	if (tag->svc > 0)
	{
		if(rulePtr->filter.svc != tag->svc)
		{
			return 0;
		}
	}
	
	/*
	 * Step 2: Check the EIDs last - as strings with wildcards these operations
	 *         are expensive.
	 */
	if((tag->bsrc) && (tag->bsrc_len > 0) && BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_src);
		if(eidsMatch(tag->bsrc, tag->bsrc_len, curEID, rulePtr->filter.bsrc_len) == 0)
		{
			return 0;
		}
	}

	if((tag->bdest) && (tag->bdest_len > 0) && BPSEC_RULE_BDST_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_dest);
		if(eidsMatch(tag->bdest, tag->bdest_len, curEID, rulePtr->filter.bdest_len) == 0)
		{
			return 0;
		}
	}

	if((tag->ssrc) && (tag->ssrc_len > 0) && BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.sec_src);
		if(eidsMatch(tag->ssrc, tag->ssrc_len, curEID, rulePtr->filter.ssrc_len) == 0)
		{
			return 0;
		}
	}

	/* Step 3: If we get here, we have passed all the filters. */
	return 1;
}



/******************************************************************************
 * @brief Removes a rule from the BPSec Policy Engine.
 *
 * @param[in,out] partition - The shared memory partition.
 * @param[in,out] ruleAddr  - The rule being removed
 *
 * @note
 * Upon removal, the rule is deleted and MUST NOT be referenced by the
 * calling function.
 * \par
 * Removing a rule will alter the index of every rule before this one in the
 * shared list.
 *
 * @retval 1  - Success
 * @retval 0  - Error
 * @retval -1 - System Error.
 *****************************************************************************/

int bslpol_rule_remove(PsmPartition partition, PsmAddress ruleAddr)
{
	PsmAddress eltAddr;
	BpSecPolRule *rulePtr = NULL;
	BpSecEventSet *esPtr = NULL;
	char *curEID = NULL;
	SecVdb	*secvdb = getSecVdb();

	PsmAddress curRuleAddr = 0;
	BpSecPolRule *curRulePtr = NULL;

	/* Step 0: Sanity Check */
	CHKZERO(partition);
	if (secvdb == NULL) return 0;

	/* Cannot delete a rule that doesn't exist. */
	if(ruleAddr == 0)
	{
		return 0;
	}

	rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);

	/* Step 1: Remove rule references from every string index. */
	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		if((curEID = (char *) psp(partition, rulePtr->filter.bundle_src)) != NULL)
		{
			radix_foreach_match(partition, secvdb->bpsecRuleIdxBySrc, curEID, 1, (radix_match_fn)bslpol_cb_ruleradix_remove, &ruleAddr);
		}
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		if((curEID = (char *) psp(partition, rulePtr->filter.bundle_dest)) != NULL)
		{
			radix_foreach_match(partition, secvdb->bpsecRuleIdxByDest, curEID, 1, (radix_match_fn)bslpol_cb_ruleradix_remove, &ruleAddr);
		}
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		if((curEID = (char *) psp(partition, rulePtr->filter.sec_src)) != NULL)
		{
			radix_foreach_match(partition, secvdb->bpsecRuleIdxBySSrc, curEID, 1, (radix_match_fn)bslpol_cb_ruleradix_remove, &ruleAddr);
		}
	}

	/*
	 * Step 2: Removing a rule from shared memory involves 2 steps:
	 *         1. Reduce the IDX of every rule whose IDX is > the IDX being removed.
	 *            This is easy because the rule list is sorted by IDX. So every rule
	 *            prior to the one being deleted needs its IDX value decremented.
	 *            If you had rule IDX's: 3  2  1 and removed the rule with IDX 2
	 *            You would process:   3(decrement) 2 (delete) 1 (unchanged)
	 *            After which your rule IDX's would be:  2  1
	 *
	 *         2. Remove the rule from the lyst.
	 */

	for(eltAddr = sm_list_first(partition, secvdb->bpsecPolicyRules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{
		curRuleAddr = sm_list_data(partition, eltAddr);

		if(curRuleAddr != ruleAddr)
		{
			curRulePtr = (BpSecPolRule *) psp(partition, curRuleAddr);
			curRulePtr->idx--;
		}
		else
		{
			if((esPtr = (BpSecEventSet*) psp(partition, rulePtr->eventSet)) != NULL)
			{
				esPtr->ruleCount--;
			}

			/*
			 * The delete callback referenced here will also delete the rule
			 * and clear the shared memory associated with the rule.
			 */
			sm_list_delete(partition, eltAddr, bslpol_cb_smlist_delete, NULL);
			break;
		}
	}

	return 1;
}



/******************************************************************************
 * @brief Removes a rule from the BPSec Policy Engine by its unique user_id
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     user_id   - The user id of the rule being removed
 *
 * @note
 * Upon removal, the rule is deleted and MUST NOT be referenced by the
 * calling function.
 *
 * @retval 1  - Success
 * @retval 0  - Error
 * @retval -1 - System Error.
 *****************************************************************************/

int bslpol_rule_remove_by_id(PsmPartition partition, int user_id)
{
	CHKZERO(partition);
	return bslpol_rule_remove(partition, bslpol_rule_get_addr(partition, user_id));
}



/******************************************************************************
 * @brief Determines if a policy rule exists for a block in a bundle if BPA
 *  role is Security Source.
 *
 * This function populates a BpSecPolRuleSearchTag and finds the bpsec policy
 * rule that is the best match for the block identified by its block type
 * (tgtType) and role of the BPA (security source).
 *
 * @param[in]     bundle   Current bundle.
 * @param[in]     sopType  Block type of security operation to search for.
 * @param[in]     tgtType  Block type of target of policy rule.
 *
 * @retval !NULL - The policy rule to be applied to his bundle.
 * @retval NULL  - No rule is available for this bundle.
 *****************************************************************************/
BpSecPolRule* bslpol_get_sender_rule(Bundle *bundle, BpBlockType sopType,
		BpBlockType tgtType)
{
	PsmPartition wm = getIonwm();
	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));

	BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,%d)", (uaddr)bundle, sopType, tgtType);

	/* Step 1: Populate the policy rule search tag */
	tag.role = BPRF_SRC_ROLE;
	tag.type = tgtType;

	readEid(&bundle->id.source, &(tag.bsrc));
	tag.bsrc_len = strlen(tag.bsrc);

	readEid(&bundle->destination, &(tag.bdest));
	tag.bdest_len = strlen(tag.bdest);

	if(sopType == BlockIntegrityBlk)
	{
	    tag.svc = SC_SVC_BIBINT;
	}
	else if(sopType == BlockConfidentialityBlk)
	{
	    tag.svc = SC_SVC_BCBCONF;
	}

	tag.scid = BPSEC_UNSUPPORTED_SC;

	/* Step 2: Retrieve the rule for the current security operation */
	BpSecPolRule *polRule = bslpol_rule_get_best_match(wm, tag);

	/*
	 * Step 3: Using the scid, determine if the policy rule matches the
	 * offered security operation type.
	 */
	if (polRule != NULL)
	{
	    sc_Def def;

// TODO - Remove the fitler.svc check once we pass the sopType into the filter criteria.
	    if(
//	            (polRule->filter.svc != sopType)                                             ||
	      (bpsec_sci_defFind(polRule->filter.scid, &def) < 1)                           ||
	      ((sopType != BlockIntegrityBlk) && (sopType != BlockConfidentialityBlk))      ||
	      ((sopType != BlockIntegrityBlk) && (sopType != BlockConfidentialityBlk))      ||
	      ((sopType == BlockIntegrityBlk) && !(def.scServices & SC_SVC_BIBINT))         ||
	      ((sopType == BlockConfidentialityBlk) && !(def.scServices & SC_SVC_BCBCONF)))
	    {
            polRule = NULL;
	    }
	}

	MRELEASE(tag.bsrc);
	MRELEASE(tag.bdest);

	return polRule;
}

/******************************************************************************
 * @brief Determines if a policy rule exists for a block in a bundle if BPA
 *  role is Security Verifier or Acceptor.
 *
 * This function populates a BpSecPolRuleSearchTag and finds the bpsec policy
 * rule that is the best match for the block identified by its block number
 * (tgtNum), security context ID for the security block (scid), and role of
 * the BPA (security verifier or acceptor).
 *
 * @param[in]     work     The incoming acquisition work area.
 * @param[in]     tgtNum   Block number of target of policy rule
 * @param[in]     scid     Security context ID of security block
 *
 * @retval !NULL - The policy rule to be applied to his bundle.
 * @retval NULL  - No rule is available for this bundle.
 *****************************************************************************/
BpSecPolRule* bslpol_get_receiver_rule(AcqWorkArea *work, unsigned char tgtNum, int scid)
{
	PsmPartition wm = getIonwm();
	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));

	/* Step 1: Populate the policy rule search tag */
	tag.role = BPRF_VER_ROLE | BPRF_ACC_ROLE;
	tag.scid = scid;

	readEid(&(work->bundle.id.source), &(tag.bsrc));
	tag.bsrc_len = strlen(tag.bsrc);

	readEid(&(work->bundle.destination), &(tag.bdest));
	tag.bdest_len = strlen(tag.bdest);

	if (tgtNum == 0)
	{
		tag.type = PrimaryBlk;
	}
	else if (tgtNum == 1)
	{
		tag.type = PayloadBlk;
	}
	else
	{
		LystElt	elt = getAcqExtensionBlock(work, tgtNum);

		if(elt != NULL)
		{
			AcqExtBlock	*blk = lyst_data(elt);
			tag.type = blk->type;
		}
		else
		{
			BPSEC_DEBUG_INFO("Target %d does not exist in bundle.", tgtNum);
			return NULL;
		}
	}

	/* Step 2: Retrieve the rule for the current security operation */
	BpSecPolRule *polRule = bslpol_rule_get_best_match(wm, tag);

	MRELEASE(tag.bsrc);
	MRELEASE(tag.bdest);

	return polRule;
}



PsmAddress bslpol_scparm_create(PsmPartition partition, int type, int length, void *value)
{
	PsmAddress result = psm_zalloc(partition, sizeof(BpSecCtxParm));
	BpSecCtxParm *parm = psp(partition, result);

	BPSEC_DEBUG_PROC("(partition, %d, %d, "ADDR_FIELDSPEC")",
	                 type, length, (uaddr)value);

	if(parm)
	{
		parm->id = type;
		parm->length = length;
		if(length <= 0)
		{
			parm->addr = 0;
		}
		else if((parm->addr = psm_zalloc(partition, length+1)) <= 0)
		{
			psm_free(partition, result);
			result = 0;
		}
		else
		{
			void *valPtr = psp(partition, parm->addr);
			memset(valPtr, 0, length+1);
			memcpy(valPtr, value, length);
		}
	}

	return result;
}




PsmAddress bslpol_scparm_find(PsmPartition partition, PsmAddress parms, int type)
{
  PsmAddress result = 0;
  PsmAddress elt = 0;
  BpSecCtxParm *parm = NULL;

  for(elt = sm_list_first(partition, parms); elt; elt = sm_list_next(partition, elt))
  {
      result = sm_list_data(partition, elt);
      if((parm = psp(partition, result)) != NULL)
      {
          if(parm->id == type)
          {
              return result;
          }
      }
  }

  return 0;
}




void bslpol_scparms_destroy(PsmPartition partition, PsmAddress addr)
{
    BpSecCtxParm *parm = psp(partition, addr);

    if(parm)
    {
        void *valPtr = psp(partition, parm->addr);
        memset(valPtr, 0, parm->length);
        psm_free(partition, parm->addr);

        memset(parm,0,sizeof(BpSecCtxParm));
        psm_free(partition, addr);
    }
}

/******************************************************************************
 * @brief Serialize a security context parameter into a buffer.
 *
 * @param[in,out] cursor     - The cursor into the buffer being updated.
 * @param[in]     param      - The parameter being serialized.
 * @param[in,out] bytes_left - Space remaining in buffer.
 *
 * @notes
 *
 * @retval  The number of bytes written into the buffer beyond the cursor.
 *****************************************************************************/

Address bslpol_scparms_persist(PsmPartition partition, char *buffer, PsmAddress parms, int *bytes_left)
{

	char *cursor = buffer;
	PsmAddress elt = 0;
	uint16_t num_items = 0;
	BpSecCtxParm *sc_parm = NULL;
	void *valPtr = NULL;

	CHKZERO(cursor);
	CHKZERO(bytes_left);

	num_items = sm_list_length(partition, parms);
	cursor += bsl_bufwrite(cursor, &num_items, sizeof(num_items), bytes_left);

	for(elt = sm_list_first(partition, parms); elt; elt = sm_list_next(partition, elt))
	{
		if((sc_parm = psp(partition, sm_list_data(partition, elt))) != NULL)
		{
			cursor += bsl_bufwrite(cursor, &(sc_parm->id), sizeof(sc_parm->id), bytes_left);

			if((valPtr = psp(partition, sc_parm->addr)) != NULL)
			{
				cursor += bsl_bufwrite(cursor, &(sc_parm->length), sizeof(sc_parm->length), bytes_left);
				cursor += bsl_bufwrite(cursor, valPtr, sc_parm->length, bytes_left);
			}
			else
			{
				sc_parm->length = 0;
				cursor += bsl_bufwrite(cursor, &(sc_parm->length), sizeof(sc_parm->length), bytes_left);
			}
		}
	}

	return cursor - buffer;
}



/******************************************************************************
 * @brief Deserialize a security context parameter from a buffer
 *
 * @param[out]    parm       - The deserialized parameter.
 * @param[in]     cursor     - The cursor into the buffer being read from.
 * @param[in,out] bytes_left - Space remaining in buffer.
 *
 * @notes
 * @retval  The number of bytes read from the buffer beyond the cursor.
 *****************************************************************************/

Address bslpol_scparms_restore(PsmPartition partition, PsmAddress *parms, char *buffer, int *bytes_left)
{
	char *cursor = buffer;
	PsmAddress newParm = 0;
	uint16_t num_items = 0;
	int i = 0;

	CHKZERO(cursor);
	CHKZERO(bytes_left);
	CHKZERO(parms);

	*parms = sm_list_create(partition);
	CHKZERO(*parms);

	cursor += bsl_bufread(&(num_items), cursor, sizeof(num_items), bytes_left);

	for(i = 0; i < num_items; i++)
	{
		int type = 0;
		int length = 0;
		void *valPtr = NULL;

		cursor += bsl_bufread(&(type), cursor, sizeof(type), bytes_left);
		cursor += bsl_bufread(&(length), cursor, sizeof(length), bytes_left);
		if(length > 0)
		{
			if((valPtr = MTAKE(length)) != NULL)
			{
			   cursor += bsl_bufread(&(valPtr), cursor, length, bytes_left);
			}
		}

		if((newParm = bslpol_scparm_create(partition, type, length, valPtr)) != 0)
		{
			sm_list_insert_last(partition, *parms, newParm);
		}

		if(valPtr)
		{
		   MRELEASE(valPtr);
		   valPtr = NULL;
		}
	}

	return cursor - buffer;
}


int bslpol_scparms_size(PsmPartition partition, PsmAddress parms)
{
    int size = 0;
    PsmAddress elt = 0;
    BpSecCtxParm *parm = NULL;

    size += sizeof(uint16_t); // @ items in the list.

    for(elt = sm_list_first(partition, parms); elt; elt = sm_list_next(partition, elt))
    {
        if((parm = psp(partition, sm_list_data(partition, elt))) != NULL)
        {
            size += sizeof(parm->id);
            size += sizeof(parm->length);
            size += parm->length;
        }
    }

    return size;
}




/******************************************************************************
 * @brief Remove a rule from the SDR.
 *
 * @param[in]     wm       - The shared memory partition
 * @param[in|out] ruleAddr - The address of the rule being forgotten
 *
 * @notes
 * TODO: If the rule has an anonymous event set, that needs to be forgotten too
 *       currently anonymous event sets are not supported.
 *
 * @retval  1 - The rule was found in the SDR and removed.
 * @retval  0 - The rule was not found in the SDR.
 * @retval -1 - Error.
 *****************************************************************************/

int bslpol_sdr_rule_forget(PsmPartition wm, PsmAddress ruleAddr)
{
	SecDB *secdb = getSecConstants();
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecPolRule *rulePtr = NULL;
	Object sdrElt = 0;
	Object dataElt = 0;
	uint16_t user_id = 0;
	int success = 0;

	if (secdb == NULL) return -1;
	rulePtr = (BpSecPolRule*) psp(wm, ruleAddr);
	CHKERR(rulePtr);

	CHKERR(sdr_begin_xn(ionsdr));

	for(sdrElt = sdr_list_first(ionsdr, secdb->bpSecPolicyRules);
		sdrElt;
		sdrElt = sdr_list_next(ionsdr, sdrElt))
	{
		if((dataElt = sdr_list_data(ionsdr, sdrElt)) != 0)
		{
			sdr_read(ionsdr, (char *) &entry, dataElt, sizeof(BpSecPolicyDbEntry));
			sdr_read(ionsdr, (char *) &user_id, entry.entryObj, sizeof(user_id));
			if(user_id == rulePtr->user_id)
			{
				sdr_free(ionsdr, entry.entryObj);
				sdr_free(ionsdr, dataElt);
				sdr_list_delete(ionsdr, sdrElt, NULL, NULL);
				success = 1;
				break;
			}
		}
	}

	if (sdr_end_xn(ionsdr) < 0)
	{
		putErrmsg("Can't remove key.", NULL);
		success = -1;
	}

	return success;
}



/******************************************************************************
 * @brief Writes a serialized version of the rule into the SDR.
 *
 * @param[in] wm       - The shared memory partition
 * @param[in] ruleAddr - The address of the rule being persisted
 *
 * @notes
 * TODO: If the rule has an anonymous event set, that needs to be persisted too
 *       currently anonymous event sets are not supported.
 * TODO: Only serialize present security parameters, not all parms.
 *
 * @retval  1 - The rule was written to the SDR
 * @retval  0 - The rule was not written to the SDR.
 * @retval -1 - Error.
 *****************************************************************************/

int bslpol_sdr_rule_persist(PsmPartition wm, PsmAddress ruleAddr)
{
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecPolRule *rule = NULL;
	char *buffer = NULL;
	char *cursor = NULL;
	int bytes_left = 0;
	uint8_t len = 0;
	int success = 0;
	SecDB *secdb = getSecConstants();

	CHKERR(wm);
	if (secdb == NULL) return -1;
	rule = (BpSecPolRule *) psp(wm, ruleAddr);
	CHKERR(rule);

	/*
	 * Step 1: Figure out the size of the serializes rule and allocate
	 *         a buffer to hold the serialzied rule. Serializing the rule
	 *         in memory first allows us to have a single large write into
	 *         the SDR which locks the SDR for a shorter period of time
	 *         and is a little bit faster.
	 */
	entry.size = bslpol_sdr_rule_size(wm, ruleAddr);
	if((buffer = MTAKE(entry.size)) == NULL)
	{
		sdr_cancel_xn(ionsdr);
		putErrmsg("Cannot allocate workspace for rule.", NULL);
		return -1;
	}

	/*
	 * Step 2: Setup a cursor and buffer limits. The cursor will advance
	 *         through the buffer adding rule items.
	 */
	cursor = buffer;
	bytes_left = entry.size;


	/*
	 * Step 3: Serialize the rule user_id. This MUST be the first thing that
	 *         is serialized for the rule. This allows us to peek at the first
	 *         ID of a serialized rule when trying to forget the rule to see
	 *         if the rules is a match without reading the entire thing from
	 *         the SDR.
	 */
	cursor += bsl_bufwrite(cursor, &(rule->user_id), sizeof(rule->user_id), &bytes_left);


	/*
	 * Step 4: Write all other rule components based on what is/is not turned on
	 *         in the rule.
	 */
	if((len = strlen(rule->desc)) > 0)
	{
		len++; /* Note length is 1 more to include NULL terminator. */
		cursor += bsl_bufwrite(cursor, &len, 1, &bytes_left);
		cursor += bsl_bufwrite(cursor, &(rule->desc), len, &bytes_left);
	}
	else
	{
		cursor += bsl_bufwrite(cursor, &len, 1, &bytes_left);
	}

	cursor += bsl_bufwrite(cursor, &(rule->flags), sizeof(rule->flags), &bytes_left);
	cursor += bsl_bufwrite(cursor, &(rule->filter.flags), sizeof(rule->filter.flags), &bytes_left);
	cursor += bsl_bufwrite(cursor, &(rule->filter.score), sizeof(rule->filter.score), &bytes_left);

	if(BPSEC_RULE_BSRC_IDX(rule))
	{
		cursor += bsl_bufwrite(cursor, &(rule->filter.bsrc_len), sizeof(rule->filter.bsrc_len), &bytes_left);
		cursor += bsl_bufwrite(cursor, psp(wm, rule->filter.bundle_src), rule->filter.bsrc_len+1, &bytes_left);
	}

	if(BPSEC_RULE_BDST_IDX(rule))
	{
		cursor += bsl_bufwrite(cursor, &(rule->filter.bdest_len), sizeof(rule->filter.bdest_len), &bytes_left);
		cursor += bsl_bufwrite(cursor, psp(wm, rule->filter.bundle_dest), rule->filter.bdest_len+1, &bytes_left);
	}

	if(BPSEC_RULE_SSRC_IDX(rule))
	{
		cursor += bsl_bufwrite(cursor, &(rule->filter.ssrc_len), sizeof(rule->filter.ssrc_len), &bytes_left);
		cursor += bsl_bufwrite(cursor, psp(wm, rule->filter.sec_src), rule->filter.ssrc_len+1, &bytes_left);
	}

	if(BPSEC_RULE_BTYP_IDX(rule))
	{
		cursor += bsl_bufwrite(cursor, &(rule->filter.blk_type), sizeof(rule->filter.blk_type), &bytes_left);
	}

	if(BPSEC_RULE_SCID_IDX(rule))
	{
		cursor += bsl_bufwrite(cursor, &(rule->filter.scid), sizeof(rule->filter.scid), &bytes_left);
	}

	cursor += bslpol_scparms_persist(wm, cursor, rule->sc_parms, &bytes_left);

	BpSecEventSet *esPtr = (BpSecEventSet *) psp(wm, rule->eventSet);

	if(esPtr == NULL)
	{
		len = 0;
		cursor += bsl_bufwrite(cursor, &len, 1, &bytes_left);
	}
	else
	{
		len = strlen(esPtr->name) + 1;
		cursor += bsl_bufwrite(cursor, &len, 1, &bytes_left);
		cursor += bsl_bufwrite(cursor, &(esPtr->name), len, &bytes_left);
	}

	/*
	 * Step 5: If the cursor position is exactly the entry length away from the start of
	 *         the buffer, then everything was (probably) serialized as expected.
	 */
	if(cursor != (buffer + entry.size))
	{
		putErrmsg("Error persisting rule.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	/* Step 6: Persist, cleanup, and return. */
	success = bsl_sdr_insert(ionsdr, buffer, entry, secdb->bpSecPolicyRules);
	MRELEASE(buffer);

	return success;
}



/******************************************************************************
 * @brief Reads a serialized version of a rule from the SDR.
 *
 * @param[in] wm    - The shared memory partition
 * @param[in] entry - The entry pointing to the rule to restore in the SDR.
 *
 * @notes
 * TODO: If the rule has an anonymous event set, that needs to be restored too
 *       currently anonymous event sets are not supported.
 * TODO: Only restore present security parameters, not all parms.
 * TODO: Revisit how the rule index (idx) is recalculated. This may not preserve
 *       indexes correctly. The index is only used to sort the rule list and to
 *       be a tie-breaker when selecting the "best" rule amongst rules with the
 *       same filter score.
 *
 * @retval  1 - The rule was found in the SDR and restored.
 * @retval  0 - The rule was not found in the SDR.
 * @retval -1 - Error.
 *****************************************************************************/

int bslpol_sdr_rule_restore(PsmPartition wm, BpSecPolicyDbEntry entry)
{
	PsmAddress ruleAddr = 0;
	BpSecPolRule *rulePtr = NULL;
	char *buffer = NULL;
	char *cursor = NULL;
	char *curEid = NULL;
	Sdr ionsdr = getIonsdr();
	int bytes_left = 0;
	uint8_t len = 0;
	SecVdb	*secvdb = getSecVdb();

	bytes_left = entry.size;
	cursor = buffer = MTAKE(entry.size);
	CHKERR(buffer);
	if (secvdb == NULL) return -1;

	/*
	 * Step 1: Inhale the serialied object from the SDR. Reading the rule as a
	 *         single SDR read and then processing in in memory saves time by
	 *         locking the SDR for less time and not going through the overhead
	 *         of SDR ops.
	 */
	sdr_read(ionsdr, buffer, entry.entryObj, entry.size);

	/* Step 2: Allocate and zero out a rule object to hold the new information. */

	if((ruleAddr = psm_zalloc(wm, sizeof(BpSecPolRule))) == 0)
	{
		MRELEASE(buffer);
		return -1;
	}
	rulePtr = (BpSecPolRule*) psp(wm, ruleAddr);
	memset(rulePtr, 0, sizeof(BpSecPolRule));

	/* Step 3: Read rule items from the buffer directly into the rule object. */
	cursor += bsl_bufread(&(rulePtr->user_id), cursor, sizeof(rulePtr->user_id), &bytes_left);

	cursor += bsl_bufread(&len, cursor, 1, &bytes_left);
	if(len > 0)
	{
		cursor += bsl_bufread(&(rulePtr->desc), cursor, len, &bytes_left);
	}

	rulePtr->idx = sm_list_length(wm, secvdb->bpsecPolicyRules);

	cursor += bsl_bufread(&(rulePtr->flags), cursor, sizeof(rulePtr->flags), &bytes_left);
	cursor += bsl_bufread(&(rulePtr->filter.flags), cursor, sizeof(rulePtr->filter.flags), &bytes_left);
	cursor += bsl_bufread(&(rulePtr->filter.score), cursor, sizeof(rulePtr->filter.score), &bytes_left);

	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		cursor += bsl_bufread(&(rulePtr->filter.bsrc_len), cursor, sizeof(rulePtr->filter.bsrc_len), &bytes_left);
		if((curEid = MTAKE(rulePtr->filter.bsrc_len+1)) == NULL)
		{
			MRELEASE(buffer);
			psm_free(wm, ruleAddr);
			return -1;
		}
		cursor += bsl_bufread(curEid, cursor, rulePtr->filter.bsrc_len+1, &bytes_left);
		rulePtr->filter.bundle_src = bsl_ed_get_ref(wm, curEid);
		MRELEASE(curEid);
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		cursor += bsl_bufread(&(rulePtr->filter.bdest_len), cursor, sizeof(rulePtr->filter.bdest_len), &bytes_left);
		if((curEid = MTAKE(rulePtr->filter.bdest_len+1)) == NULL)
		{
			MRELEASE(buffer);
			psm_free(wm, ruleAddr);
			return -1;
		}
		cursor += bsl_bufread(curEid, cursor, rulePtr->filter.bdest_len+1, &bytes_left);
		rulePtr->filter.bundle_dest = bsl_ed_get_ref(wm, curEid);
		MRELEASE(curEid);
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		cursor += bsl_bufread(&(rulePtr->filter.ssrc_len), cursor, sizeof(rulePtr->filter.ssrc_len), &bytes_left);
		if((curEid = MTAKE(rulePtr->filter.ssrc_len+1)) == NULL)
		{
			MRELEASE(buffer);
			psm_free(wm, ruleAddr);
			return -1;
		}
		cursor += bsl_bufread(curEid, cursor, rulePtr->filter.ssrc_len+1, &bytes_left);
		rulePtr->filter.sec_src = bsl_ed_get_ref(wm, curEid);
		MRELEASE(curEid);
	}

	if(BPSEC_RULE_BTYP_IDX(rulePtr))
	{
		cursor += bsl_bufread(&(rulePtr->filter.blk_type), cursor, sizeof(rulePtr->filter.blk_type), &bytes_left);
	}

	if(BPSEC_RULE_SCID_IDX(rulePtr))
	{
		cursor += bsl_bufread(&(rulePtr->filter.scid), cursor, sizeof(rulePtr->filter.scid), &bytes_left);
	}

	cursor += bslpol_scparms_restore(wm, &(rulePtr->sc_parms), cursor, &bytes_left);

	cursor += bsl_bufread(&len, cursor, 1, &bytes_left);
	if(len > 0)
	{
		char tmp[MAX_EVENT_SET_NAME_LEN];
		cursor += bsl_bufread(tmp, cursor, len, &bytes_left);
		if((rulePtr->eventSet = bsles_get_addr(wm, tmp)) == 0)
		{
			writeMemoNote("[w] - Unable to get event set for rule",tmp);
		}
	}

	/*
	 * Step 4: If the cursor position is exactly the entry length away from the start of
	 *         the buffer, then everything was (probably) serialized as expected.
	 */

	if(cursor != (buffer + entry.size))
	{
		putErrmsg("Error restoring rule.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	/* Step 5: Cleanup, save the rule, and return. */

	MRELEASE(buffer);
	return bslpol_rule_insert(wm, ruleAddr, 0);
}



/******************************************************************************
 * @brief Calculate the expected serialized size of a rule object.
 *
 * @param[in] wm       - The shared memory partition
 * @param[in] ruleAddr - The address of the rule being sized.
 *
 * @notes
 * TODO: If the rule has an anonymous event set, that needs to be restored too
 *       currently anonymous event sets are not supported.
 * TODO: Only restore present security parameters, not all parms.
 * TODO: Revisit how the rule index (idx) is recalculated. This may not preserve
 *       indexes correctly. The index is only used to sort the rule list and to
 *       be a tie-breaker when selecting the "best" rule amongst rules with the
 *       same filter score.
 *
 * @retval  >0 - The expected size of the rule
 * @retval   0 - The rule could not be sized
 * @retval  -1 - Error.
 *****************************************************************************/

int bslpol_sdr_rule_size(PsmPartition wm, PsmAddress ruleAddr)
{
	BpSecPolRule *rulePtr = NULL;
	int size = 0;

	CHKERR(wm);
	CHKERR(ruleAddr);
	rulePtr = (BpSecPolRule*) psp(wm, ruleAddr);
	CHKERR(rulePtr);

	/* Step 1: Calculate the storage size of the RULE. */
	size = 3; /* Size of user_id, and flags */
	size += 2; /* Size of static rule filters. */

	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.bsrc_len) + rulePtr->filter.bsrc_len+1;
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.bdest_len) + rulePtr->filter.bdest_len+1;
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.ssrc_len) + rulePtr->filter.ssrc_len+1;
	}

	if(BPSEC_RULE_BTYP_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.blk_type);
	}

	if(BPSEC_RULE_SCID_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.scid);
	}

	size += 1; /* the length of the rule description. */
	if(strlen(rulePtr->desc) > 0)
	{
		size += strlen(rulePtr->desc) + 1; /* The NULL-terminated rule description. */
	}

	size += bslpol_scparms_size(wm, rulePtr->sc_parms);

	size += 1; // Event set name.

	BpSecEventSet *esPtr = (BpSecEventSet *) psp(wm, rulePtr->eventSet);
	if(esPtr)
	{
		size += strlen(esPtr->name)+1;
	}

	return size;
}





/******************************************************************************
 * @brief Determines whether a rule is a better match than the current best
 *        matched rule for an extension block.
 *
 * @param[in]     partition - The shared memory partition
 * @param[in,out] tag  - The search tag holding information about the extension
 *                       block and the best rule matched so far.
 * @param[in]     rule - The current rule, from a list of possible rules, that
 *                       might be a better match for the search tag.
 *
 * @note
 * It is assumed that the current rule list is sorted by rule specificity and
 * then sub-sorted by rule idx. In this way, a call to this function for the
 * cur_rule in the list is the "best" rule in the list if it matches. So, as
 * soon as a match in the list is found, it will be the best for that list.
 *
 * @retval 1  - A better match was found and the current rule list no longer
 *              needs to be searched.
 * @retval 0  - No better match was found and the search must continue.
 * @retval -1 - System Error.
 *****************************************************************************/

int bslpol_search_tag_best(PsmPartition partition, BpSecPolRuleSearchBestTag *tag, PsmAddress ruleAddr)
{
	BpSecPolRule *rulePtr = NULL;

	rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);
	CHKZERO(rulePtr);

	/* Step 1: If this rule has no chance of beating best rule so far, we're done. */
	if(tag->best_rule)
	{
		if((tag->best_rule->filter.score > rulePtr->filter.score) ||
		   ((tag->best_rule->filter.score == rulePtr->filter.score) && (tag->best_rule->idx >= rulePtr->idx)))
		{
			/* If our best rule has a higher score (or same score and higher idx)
			 * then the current rule, the current rule cannot be a better match.
			 * Additionally, since rules in the list we are searching are sorted
			 * by descending score and sub-sorted by descending idx, the cur_rule
			 * (whether it matches or not) is the only chance for a "better" rule.
			 * If it isn't this rule, no rule in the list after this one would have
			 * a better score/idx combo. So we can just stop looking.
			 */
			return 1;

		}
	}

	/*
	 * Step 2: Otherwise, the current rule has a higher (or equal) score and
	 *         a lower id than any subsequent rule in the rule list. So we
	 *         will find no better match in this list.
	 *
	 *         If the rule matches the search tag we can assume this is the
	 *         best rule we will see as part of this search. Return 1 to stop
	 *         searching.
	 */
	if(bslpol_rule_matches(partition, rulePtr, &(tag->search)) == 1)
	{
		tag->best_rule = rulePtr;
		return 1;
	}

	/*
	 * Step 3: If we get here, there is still a possibility there exists a
	 *         more specific rule. So keep searching.
	 */
	return 0;
}



/******************************************************************************
 * @brief Compare function for shorting the master shared memory list of rules
 *
 *  This function is used by the master shared memory list of policy rules to
 *  determine where to insert a new rule. This lisy is sorted by the rule
 *  index.
 *
 * @param[in] partition  - The shared memory partition
 * @param[in] eltData    - The current list entry
 * @param[in] insertData - The data being inserted into the list.
 *
 * @note
 *
 * @retval >0  - The current list entry has a higher index than the new rule.
 * @retval  0  - The rules have the same index. This should not happen.
 * @retval <0  - The new rule has an index lower than the list entry.
 *****************************************************************************/

int bslpol_cb_rule_compare_idx(PsmPartition partition, PsmAddress eltData, void *insertData)
{
	BpSecPolRule *r1 = psp(partition, eltData);
	BpSecPolRule *r2 = (BpSecPolRule *) insertData;

	return r1->idx - r2->idx;
}



/******************************************************************************
 * @brief compares the scores of 2 rules.
 *
 * @param[in] r1 - The first rule being compared.
 * @param[in] r2 - The second rule being compared.
 *
 * @note
 * The comparison is based on rule scores first and, if rule scores are the
 * same, then rule IDX values are compared instead.
 *
 * @retval <0 - r1  <  r2
 * @retval  0 - r1  == r2
 * @retval >0 - r1  >  r2
 *****************************************************************************/

int bslpol_cb_rule_compare_score(PsmPartition partition, PsmAddress eltData, void *dataBuffer)
{
	BpSecPolRule *r1 = psp(partition, eltData);
	BpSecPolRule *r2 = (BpSecPolRule *) dataBuffer;

	int result = 0;

	if((result = r1->filter.score - r2->filter.score) == 0)
	{
		result = r1->idx - r2->idx;
	}

	return result;
}



/******************************************************************************
 * @brief compares the scores of 2 rules.
 *
 *  This callback is used to sort a lyst of rules by their filter score.
 *
 *  Sorts descending by score and descending by index.
 *  using the notation (Score,Idx) then the following rules:
 *
 *  (1,1), (3,4), (3,2), (3,5), (2,2) would be sorted as follows:
 *
 *  (3,5), (3,4), (3,2), (2,2), (1,1)
 *
 *
 * @param[in] r1 - The first rule being compared.
 * @param[in] r2 - The second rule being compared.
 *
 * @note
 * The comparison is based on rule scores first and, if rule scores are the
 * same, then rule IDX values are compared instead.
 *
 * @retval <0 - r1  <  r2
 * @retval  0 - r1  == r2
 * @retval >0 - r1  >  r2
 *****************************************************************************/

int bslpol_cb_rulelyst_compare_score(BpSecPolRule *r1, BpSecPolRule *r2)
{
	int result = 0;

	if((r1 == NULL) || (r2 == NULL))
	{
		return 0;
	}

	if((result = r1->filter.score - r2->filter.score) == 0)
	{
		result = r1->idx - r2->idx;
	}

	return result;
}



/******************************************************************************
 * @brief Inserts a rule into a radix tree node
 *
 * A radix tree normally associated a key->value.  For rules, the key for a
 * particular radix tree is the EID being indexed by that tree, and the value
 * is a SET of all rules which match that EID.
 *
 * Therefore, to insert a rule into an indexing radix tree is to insert the
 * rule into the SET of rules at that node.
 *
 * The set of rules held at a radix node is a shared memory list of the addresses
 * of those rules.
 *
 * @param[in]     partition - The shared memory partition.
 * @param[in,out] entryAddr - The address of the SET of rules for the node.
 * @param[in]     itemAddr  - The address of the rule being added.
 *
 * @retval   1 - The rule was added
 * @retval  -1 - There was an error.
 *****************************************************************************/

int bslpol_cb_ruleradix_insert(PsmPartition partition, PsmAddress *entryAddr, PsmAddress itemAddr)
{
	BpSecPolRuleEntry *entryPtr = NULL;
	BpSecPolRule *rulePtr = NULL;

	CHKZERO(entryAddr);
	CHKZERO(itemAddr);

	/*
	 * If there is no list at this node, then this is the first rule being added
	 * at this node. Make a list.
	 */
	if(*entryAddr == 0)
	{
		*entryAddr = psm_zalloc(partition, sizeof(BpSecPolRuleEntry));
		CHKZERO(*entryAddr);

		entryPtr = (BpSecPolRuleEntry*) psp(partition, *entryAddr);
		memset(entryPtr,0,sizeof(BpSecPolRuleEntry));
		entryPtr->rules = sm_list_create(partition);
	}
	else
	{
		entryPtr = (BpSecPolRuleEntry*) psp(partition, *entryAddr);
	}

	/* Make sure we have the rules list and a good rule to add. */
	CHKERR(entryPtr->rules);

	rulePtr = psp(partition, itemAddr);
	CHKERR(rulePtr);

	/* Insert the rule into the rule list. */
	sm_list_insert(partition, entryPtr->rules, itemAddr, bslpol_cb_rule_compare_score, rulePtr);

	return 1;
}



/******************************************************************************
 * @brief Removes a rule from a radix tree node
 *
 * A radix tree normally associated a key->value.  For rules, the key for a
 * particular radix tree is the EID being indexed by that tree, and the value
 * is a SET of all rules which match that EID.
 *
 * Therefore, to remove a rule from an indexing radix tree is to remove the
 * rule from the SET of rules at that node.
 *
 * The set of rules held at a radix node is a shared memory list of the addresses
 * of those rules.
 *
 * @param[in]     partition - The shared memory partition.
 * @param[in,out] entryAddr - The address of the SET of rules for the node.
 * @param[in]     tag       - The address of the rule being removed.
 *
 * @retval   2 - Rule was deleted and no need to keep searching
 * @retval   1 - The was no rule to remove
 * @retval   0 - There was no rule set to review
 * @retval  -1 - There was an error.
 *****************************************************************************/

int bslpol_cb_ruleradix_remove(PsmPartition partition, PsmAddress entryAddr, void *tag)
{
	PsmAddress *ruleAddr = (PsmAddress*) tag;
	BpSecPolRuleEntry *entryPtr = NULL;
	PsmAddress eltAddr = 0;

	if(entryAddr == 0)
	{
		return 0;
	}

	CHKERR(partition);
	CHKZERO(ruleAddr);

	entryPtr = (BpSecPolRuleEntry*) psp(partition, entryAddr);

	for(eltAddr = sm_list_first(partition, entryPtr->rules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{

		/*
		 * The rule can be matched directly by its address because the
		 * radix tree stores rule by their shared memory address.
		 */
		if(sm_list_data(partition, eltAddr) == *ruleAddr)
		{
			/*
			 * Remove the address from the list of rules at this node. The
			 * address itself is not deleted because an indexing radix
			 * tree only holds addresses to rules that live elsewhere.
			 */
			sm_list_delete(partition, eltAddr, NULL, NULL);

			/*
			 * Since we have removed the rule, there is no reason to keep
			 * traversing the radix tree
			 */
			return RADIX_STOP_FOREACH;
		}
	}
	return 1;
}



/******************************************************************************
 * @brief Searches the rules at a radix tree node for a better match
 *
 * This function searches all of the rules associated with this node in the
 * radix tree to determine whether any rule at the local node is a better
 * match for some criteria than a rule that was passed-in.
 *
 * @param[in]     partition - The shared memory partition.
 * @param[in,out] entryAddr - The address of the SET of rules for the node.
 * @param[in]     tag       - Search critiera and best rule so far
 *
 * @notes
 * Even if we find a better rule match at this node, we must keep searching
 * the radix tree across the subtree rooted at the EID common root of filter
 * criteria.  For example, if we are searching for rules that might best
 * match ipn:12.2 and we have radix tree nodes associated with
 *
 *   ipn:*, ipn:1*, ipn:12*, etc...
 *
 * we would need to look through each of them as we find the best rule match.
 *
 * @retval   1 - A better rule was found at this node.
 * @retval   0 - There was no rule to search
 * @retval  -1 - There was an error.
 *****************************************************************************/

int bslpol_cb_ruleradix_search_best(PsmPartition partition, PsmAddress entryAddr, BpSecPolRuleSearchBestTag *tag)
{
	PsmAddress eltAddr = 0;
	BpSecPolRuleEntry *entryPtr = NULL;
	PsmAddress ruleAddr = 0;

	/*
	 * If there is no entry data, the current node is a split node. While some split nodes also
	 * have entry data, only split nodes have zero entry data.
	 */
	if(entryAddr == 0)
	{
		return 0;
	}
	CHKZERO(tag);

	entryPtr = (BpSecPolRuleEntry *) psp(partition, entryAddr);

	/* Look through each node in the list. */
	for(eltAddr = sm_list_first(partition, entryPtr->rules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{
		ruleAddr = sm_list_data(partition,eltAddr);

		/*
		 * Since these rules by score and index, if we find a better
		 * rule, it will be the first one encountered. There is no
		 * longer a reason to search the list.
		 */
		if(bslpol_search_tag_best(partition, tag, ruleAddr) == 1)
		{
			return 1;
		}
	}
	return 0;
}



/******************************************************************************
 * @brief Removes a rule from the master shared memory list of rules.
 *
 *  This callback is used to auto-delete a rule from the master shared memory
 *  list of rules.
 *
 * @param[in] partition - The shared memory partition
 * @param[in] tag       - Ignored
 *
 * @note
 *****************************************************************************/

void bslpol_cb_smlist_delete(PsmPartition partition, PsmAddress eltAddr, void *tag)
{

	bslpol_rule_delete(partition, sm_list_data(partition, eltAddr));
}

