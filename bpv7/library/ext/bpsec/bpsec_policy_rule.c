/*****************************************************************************
 **
 ** File Name: bpsec_policy_rule.c
 **
 ** Description:
 **
 ** Notes:
 **
 **  TODO: Document searching occurs in the context of the calling process so
 **        we work with local pointers whenever possible.
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


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION DEFINITIONS 							  +
 * +--------------------------------------------------------------------------+
 */



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

BpSecFilter bslpol_filter_build(PsmPartition partition, char *bsrc, char *bdest, char *ssrc, int type, int role, int scid)
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

		if(scid != 0)
		{
			filter.flags |= BPRF_USE_SCID;
			filter.scid = scid;
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
 * @param[in,out] filter - The filter being scored.
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



/******************************************************************************
 * @brief Creates a policy rule from components.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in] id            - The "user" ID of the rule.
 * @param[in] flags         - Special processing flags for the rule.
 * @param[in] filter        - The rule filter.
 * @param[in] sec_parms     - The security context for the rule.
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
 * \par
 * The passed-in sec_parms MUST be destroyed by the calling function. These
 * parms are deep-copied into the security context.
 *
 * @retval !0 - The created rule.
 * @retval 0 - Error.
 *****************************************************************************/

PsmAddress bslpol_rule_create(PsmPartition partition, char *desc, uint16_t id, uint8_t flags,
		                      BpSecFilter filter, Lyst sec_parms, PsmAddress events)
{
	PsmAddress ruleAddr = 0;
	BpSecPolRule *rulePtr = NULL;

	CHKZERO(ruleAddr = psm_zalloc(partition, sizeof(BpSecPolRule)));
	rulePtr = (BpSecPolRule*) psp(partition, ruleAddr);

	memset(rulePtr->desc, 0, BPSEC_RULE_DESCR_LEN+1);
	if(desc)
	{
		istrcpy(rulePtr->desc, desc, BPSEC_RULE_DESCR_LEN);
	}
	rulePtr->idx = sm_list_length(partition, getSecVdb()->bpsecPolicyRules);
	rulePtr->user_id = id;
	rulePtr->flags = flags;
	rulePtr->filter = filter;
	rulePtr->eventSet = events;
	rulePtr->sc_cfgs.parms = sci_build_parms(sec_parms);

	/* Step 2: Return the new rule. */
	return ruleAddr;
}



/******************************************************************************
 * @brief Releases resources associated with a rule.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in,out] ruleAddr  - The rule being deleted.
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

	/* Step 0: Sanity checks. */
	CHKVOID(partition);
	CHKVOID(ruleAddr);

	rulePtr = (BpSecPolRule*) psp(partition, ruleAddr);

	/* Step 1: Remove the rule from the SDR. */
	bslpol_sdr_rule_forget(partition, ruleAddr);

	/*
	 * Step 2: If the rule has an anonymous event set, then the rule must
	 *         clean that up as part of releasing resources. Otherwise, the
	 *         delete function MUST NOT remove the event set as other rules
	 *         could be referencing a shared eventset.
	 */
	if(BPSEC_RULE_ANON_ES(rulePtr))
	{
		bsles_destroy(partition, rulePtr->eventSet, psp(partition, rulePtr->eventSet));
	}

	/* Step 3: Release memory associated with the rule. */
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

	/* Step 1: Walk through the list... */
	for(eltAddr = sm_list_first(partition, getSecVdb()->bpsecPolicyRules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{
		ruleAddr = sm_list_data(partition, eltAddr);
		rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);

		/* Step 1.1: If we see a user_id match, return the rule. */
		if(rulePtr->user_id == user_id)
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
 * @param[in] bsrc      - Theoretical bundle source (or NULL)
 * @param[in] bdest     - Theoretical bundle destination (or NULL)
 * @param[in] ssrc      - Theoretical security source (or NULL)
 * @param[in] type      - Theoretical type of the extension block  (or < 0)
 * @param[in] role      - Theoretical role of the BPA when evaluating rules (or 0)
 * @param[in] sc_id     - Theoretical security context ID (or 0)
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

	/* Step 1: Populate the search tag. */

	/* Step 2: Check every rule that we have. */
	for(eltAddr = sm_list_first(partition, getSecVdb()->bpsecPolicyRules);
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
 * @brief Returns the policy rule that is the "best match' for a given
 *        extension block.
 *
 * @param[in] partition - The shared memory partition.
 * @param[in] bsrc      - The bundle source of the bundle holding the block.
 * @param[in] bdest     - The bundle destination of the bundle holding the block.
 * @param[in] ssrc      - The optional security source of a security operation on the block.
 * @param[in] type      - The type of the extension block.
 * @param[in] role      - The role of the BPA when evaluating rules.
 * @param[in] scid      - The security context ID used when verifying/accepting security.
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
	char default_key[2] = "~";
	char *search_key = NULL;

	/* Step 1: Populate the search tag. */
	tag.search = criteria;
	tag.best_rule = NULL;

	/* Step 2: Walk through each index... */
	search_key = (criteria.bsrc) ? criteria.bsrc : default_key;
	radix_foreach_match(partition,  getSecVdb()->bpsecRuleIdxBySrc, search_key,  RADIX_MATCH_FULL, (radix_match_fn)bslpol_cb_ruleradix_search_best, &tag);

	search_key = (criteria.bdest) ? criteria.bdest : default_key;
	radix_foreach_match(partition,  getSecVdb()->bpsecRuleIdxByDest, search_key,  RADIX_MATCH_FULL, (radix_match_fn)bslpol_cb_ruleradix_search_best, &tag);

	search_key = (criteria.ssrc) ? criteria.ssrc : default_key;
	radix_foreach_match(partition,  getSecVdb()->bpsecRuleIdxBySSrc, search_key,  RADIX_MATCH_FULL, (radix_match_fn)bslpol_cb_ruleradix_search_best, &tag);

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

int bslpol_rule_insert(PsmPartition partition, PsmAddress ruleAddr)
{
	int success = 1;
	BpSecPolRule *rulePtr = NULL;
	char *curEID = NULL;

	/* Step 0: Sanity Checks. */
	CHKZERO(partition);
	CHKZERO(ruleAddr);

	rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);

	/* Step 1: insert the rule in the "prime" storage lyst. */
	sm_list_insert(partition, getSecVdb()->bpsecPolicyRules, ruleAddr, bslpol_cb_rule_compare_idx, rulePtr);

	/* Step 2: Based on the rule's filters, add it into various indexers. */
	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_src);
		success = radix_insert(partition, getSecVdb()->bpsecRuleIdxBySrc, curEID, ruleAddr);
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_dest);
		success = radix_insert(partition, getSecVdb()->bpsecRuleIdxByDest, curEID, ruleAddr);
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.sec_src);
		success = radix_insert(partition, getSecVdb()->bpsecRuleIdxBySSrc, curEID, ruleAddr);
	}

	/* Step 3: Persist the rule to the SDR. */
	bslpol_sdr_rule_persist(partition, ruleAddr);

	return success;
}


/******************************************************************************
 * @brief Determines if a rule matches a given set of search criteria
 *
 * @param[in] partition - The shared memory partition
 * @param[in] rulePtr      - The rule being evaluated
 * @param[in] tag       - Search criteria representing information associated with
 *                        an extension block
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

	if((BPSEC_RULE_ROLE_IDX(rulePtr)) && ((tag->role & rulePtr->filter.flags) == 0))
	{
		return 0;
	}

	if((BPSEC_RULE_BTYP_IDX(rulePtr)) && (tag->type != rulePtr->filter.blk_type))
	{
		return 0;
	}

	if((BPSEC_RULE_SCID_IDX(rulePtr)) && (tag->scid != rulePtr->filter.scid))
	{
		return 0;
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
 *
 * @retval 1  - Success
 * @retval 0  - Error
 * @retval -1 - System Error.
 *****************************************************************************/

int bslpol_rule_remove(PsmPartition partition, PsmAddress ruleAddr)
{
	PsmAddress eltAddr;
	BpSecPolRule *rulePtr = NULL;
	char *curEID = NULL;

	PsmAddress curRuleAddr = 0;
	BpSecPolRule *curRulePtr = NULL;

	/* Step 0: Sanity Check */
	CHKZERO(partition);
	CHKZERO(ruleAddr);

	rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);

	void radix_foreach_match(PsmPartition partition, PsmAddress radixAddr, char *key, int flags, radix_match_fn match_fn, void *tag);

	/* Step 1: Remove rule references from every string index. */
	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_src);
		radix_foreach_match(partition, getSecVdb()->bpsecRuleIdxBySrc, curEID, 1, (radix_match_fn)bslpol_cb_ruleradix_remove, &ruleAddr);
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.bundle_dest);
		radix_foreach_match(partition, getSecVdb()->bpsecRuleIdxByDest, curEID, 1, (radix_match_fn)bslpol_cb_ruleradix_remove, &ruleAddr);
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		curEID = (char *) psp(partition, rulePtr->filter.sec_src);
		radix_foreach_match(partition, getSecVdb()->bpsecRuleIdxBySSrc, curEID, 1, (radix_match_fn)bslpol_cb_ruleradix_remove, &ruleAddr);
	}

	/*
	 * Step 2: Removing a rule from the lyst involves 2 steps:
	 *         1. Reduce the IDX of every rule whose IDX is > the IDX being removed.
	 *            This is easy because the lyst is sorted by IDX. So every rule
	 *            prior to the one being deleted needs its IDX value decremented.
	 *            If you had rule IDX's: 3  2  1 and removed the rule with IDX 2
	 *            You would process:   3(decrement) 2 (delete) 1 (unchanged)
	 *            After which your rule IDX's would be:  2  1
	 *
	 *         2. Remove the rule from the lyst.
	 */
	for(eltAddr = sm_list_first(partition, getSecVdb()->bpsecPolicyRules);
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
			sm_list_delete(partition, eltAddr, bslpol_cb_rulelyst_delete, NULL);
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

int bslpol_rule_get_best_match_at_src(Bundle *bundle, BpSecPolRule *polRule,
		BpBlockType tgtType)
{
	PsmPartition wm = getIonwm();
	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));

	/* Step 1: Populate the policy rule search tag */

	readEid(&bundle->id.source, &(tag.bsrc));
	tag.bsrc_len = strlen(tag.bsrc);

	readEid(&bundle->destination, &(tag.bdest));
	tag.bdest_len = strlen(tag.bdest);

	tag.role = BPRF_SRC_ROLE;
	tag.type = tgtType;

	/* Step 2: Retrieve the rule for the current security operation */
	polRule = bslpol_rule_get_best_match(wm, tag);

	MRELEASE(tag.bsrc);
	MRELEASE(tag.bdest);

	if (polRule != NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int bslpol_rule_get_best_match_at_receiver(Bundle *bundle, BpSecPolRule *polRule,
		BpBlockType tgtType)
{
	PsmPartition wm = getIonwm();
	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));

	/* Step 1: Populate the policy rule search tag */

	readEid(&bundle->id.source, &(tag.bsrc));
	tag.bsrc_len = strlen(tag.bsrc);

	readEid(&bundle->destination, &(tag.bdest));
	tag.bdest_len = strlen(tag.bdest);

	tag.role = BPRF_SRC_ROLE;
	tag.type = tgtType;

	/* Step 2: Retrieve the rule for the current security operation */
	polRule = bslpol_rule_get_best_match(wm, tag);

	MRELEASE(tag.bsrc);
	MRELEASE(tag.bdest);

	if (polRule != NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int bslpol_sdr_rule_forget(PsmPartition wm, PsmAddress ruleAddr)
{
	SecDB *secdb = getSecConstants();
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecPolRule *rulePtr = NULL;
	Object sdrElt = 0;
	Object dataElt = 0;
	uint16_t user_id;

	CHKERR(ruleAddr);
	rulePtr = (BpSecPolRule*) psp(wm, ruleAddr);
	CHKERR(rulePtr);

	CHKERR(sdr_begin_xn(ionsdr));

	for(sdrElt = sdr_list_first(ionsdr, secdb->bpSecPolicyRules);
		sdrElt;
		sdrElt = sdr_list_next(ionsdr, sdrElt))
	{
		dataElt = sdr_list_data(ionsdr, sdrElt);
		sdr_read(ionsdr, (char *) &entry, dataElt, sizeof(BpSecPolicyDbEntry));
		sdr_read(ionsdr, (char *) &user_id, entry.entryObj, sizeof(user_id));
		if(user_id == rulePtr->user_id)
		{
			if(rulePtr->eventSet != 0)
			{
				BpSecEventSet *esPtr = psp(wm, rulePtr->eventSet);
				if(esPtr)
				{
					bsles_sdr_forget(wm, esPtr->name);
				}
			}
			sdr_free(ionsdr, entry.entryObj);
			sdr_free(ionsdr, dataElt);
			sdr_list_delete(ionsdr, sdrElt, NULL, NULL);
			break;
		}
	}

	if (sdr_end_xn(ionsdr) < 0)
	{
		putErrmsg("Can't remove key.", NULL);
		return -1;
	}

	return 0;
}

/*
 * Faster to build in buffer and write to SDR once.
 */
int bslpol_sdr_rule_persist(PsmPartition wm, PsmAddress ruleAddr)
{
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecPolRule *rule = NULL;
	char *buffer = NULL;
	char *cursor = NULL;
	int bytes_left = 0;
	uint8_t len = 0;

	CHKERR(wm);
	rule = (BpSecPolRule *) psp(wm, ruleAddr);
	CHKERR(rule);

	entry.size = bslpol_sdr_rule_size(wm, ruleAddr);


	if((buffer = MTAKE(entry.size)) == NULL)
	{
		sdr_cancel_xn(ionsdr);
		putErrmsg("Cannot allocate workspace for rule.", NULL);
		return -1;
	}

	cursor = buffer;
	bytes_left = entry.size;

	if((len = strlen(rule->desc)) > 0)
	{
		len++; /* Note length is 1 more to include NULL terminator. */
		cursor += bsl_sdr_bufwrite(cursor, &len, 1, &bytes_left);
		cursor += bsl_sdr_bufwrite(cursor, &(rule->desc), len, &bytes_left);
	}
	else
	{
		cursor += bsl_sdr_bufwrite(cursor, &len, 1, &bytes_left);
	}

	cursor += bsl_sdr_bufwrite(cursor, &(rule->user_id), sizeof(rule->user_id), &bytes_left);
	cursor += bsl_sdr_bufwrite(cursor, &(rule->flags), sizeof(rule->flags), &bytes_left);
	cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.flags), sizeof(rule->filter.flags), &bytes_left);
	cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.score), sizeof(rule->filter.score), &bytes_left);

	if(BPSEC_RULE_BSRC_IDX(rule))
	{
		cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.bsrc_len), sizeof(rule->filter.bsrc_len), &bytes_left);
		cursor += bsl_sdr_bufwrite(cursor, psp(wm, rule->filter.bundle_src), rule->filter.bsrc_len, &bytes_left);
	}

	if(BPSEC_RULE_BDST_IDX(rule))
	{
		cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.bdest_len), sizeof(rule->filter.bdest_len), &bytes_left);
		cursor += bsl_sdr_bufwrite(cursor, psp(wm, rule->filter.bundle_dest), rule->filter.bdest_len, &bytes_left);
	}

	if(BPSEC_RULE_SSRC_IDX(rule))
	{
		cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.ssrc_len), sizeof(rule->filter.ssrc_len), &bytes_left);
		cursor += bsl_sdr_bufwrite(cursor, psp(wm, rule->filter.sec_src), rule->filter.ssrc_len, &bytes_left);
	}

	if(BPSEC_RULE_BTYP_IDX(rule))
	{
		cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.blk_type), sizeof(rule->filter.blk_type), &bytes_left);
	}

	if(BPSEC_RULE_SCID_IDX(rule))
	{
		cursor += bsl_sdr_bufwrite(cursor, &(rule->filter.scid), sizeof(rule->filter.scid), &bytes_left);
	}

	cursor += bslpol_sdr_scparm_persist(cursor, rule->sc_cfgs.parms.aad, &bytes_left);
	cursor += bslpol_sdr_scparm_persist(cursor, rule->sc_cfgs.parms.icv, &bytes_left);
	cursor += bslpol_sdr_scparm_persist(cursor, rule->sc_cfgs.parms.intsig, &bytes_left);
	cursor += bslpol_sdr_scparm_persist(cursor, rule->sc_cfgs.parms.iv, &bytes_left);
	cursor += bslpol_sdr_scparm_persist(cursor, rule->sc_cfgs.parms.keyinfo, &bytes_left);
	cursor += bslpol_sdr_scparm_persist(cursor, rule->sc_cfgs.parms.salt, &bytes_left);

	if(BPSEC_RULE_ANON_ES(rule))
	{
		BpSecEventSet *eventSetPtr = psp(wm, rule->eventSet);
		cursor += bsles_sdr_serialize_buffer(wm, eventSetPtr, cursor, &bytes_left);
	}

	if(cursor != (buffer + entry.size))
	{
		putErrmsg("Error persisting rule.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	return bsl_sdr_insert(ionsdr, buffer, entry, getSecConstants()->bpSecPolicyRules);
}


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

	bytes_left = entry.size;
	cursor = buffer = MTAKE(entry.size);
	CHKERR(buffer);

	sdr_read(ionsdr, buffer, entry.entryObj, entry.size);

	if((ruleAddr = psm_zalloc(wm, sizeof(BpSecPolRule))) == 0)
	{
		MRELEASE(buffer);
		return -1;
	}

	rulePtr = (BpSecPolRule*) psp(wm, ruleAddr);
	memset(rulePtr, 0, sizeof(BpSecPolRule));


	cursor += bsl_sdr_bufread(&len, cursor, 1, &bytes_left);
	if(len > 0)
	{
		cursor += bsl_sdr_bufread(&(rulePtr->desc), cursor, len+1, &bytes_left);
	}

	rulePtr->idx = sm_list_length(wm, getSecVdb()->bpsecPolicyRules);

	cursor += bsl_sdr_bufread(&(rulePtr->flags), cursor, sizeof(rulePtr->flags), &bytes_left);
	cursor += bsl_sdr_bufread(&(rulePtr->flags), cursor, sizeof(rulePtr->flags), &bytes_left);
	cursor += bsl_sdr_bufread(&(rulePtr->filter.flags), cursor, sizeof(rulePtr->filter.flags), &bytes_left);
	cursor += bsl_sdr_bufread(&(rulePtr->filter.score), cursor, sizeof(rulePtr->filter.score), &bytes_left);

	if(BPSEC_RULE_BSRC_IDX(rulePtr))
	{
		cursor += bsl_sdr_bufread(&(rulePtr->filter.bsrc_len), cursor, sizeof(rulePtr->filter.bsrc_len), &bytes_left);
		if((curEid = MTAKE(rulePtr->filter.bsrc_len)) == NULL)
		{
			MRELEASE(buffer);
			psm_free(wm, ruleAddr);
			return -1;
		}
		cursor += bsl_sdr_bufread(curEid, cursor, rulePtr->filter.bsrc_len, &bytes_left);
		rulePtr->filter.bundle_src = bsl_ed_get_ref(wm, curEid);
		MRELEASE(curEid);
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		cursor += bsl_sdr_bufread(&(rulePtr->filter.bdest_len), cursor, sizeof(rulePtr->filter.bdest_len), &bytes_left);
		if((curEid = MTAKE(rulePtr->filter.bdest_len)) == NULL)
		{
			MRELEASE(buffer);
			psm_free(wm, ruleAddr);
			return -1;
		}
		cursor += bsl_sdr_bufread(curEid, cursor, rulePtr->filter.bdest_len, &bytes_left);
		rulePtr->filter.bundle_dest = bsl_ed_get_ref(wm, curEid);
		MRELEASE(curEid);
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		cursor += bsl_sdr_bufread(&(rulePtr->filter.ssrc_len), cursor, sizeof(rulePtr->filter.ssrc_len), &bytes_left);
		if((curEid = MTAKE(rulePtr->filter.ssrc_len)) == NULL)
		{
			MRELEASE(buffer);
			psm_free(wm, ruleAddr);
			return -1;
		}
		cursor += bsl_sdr_bufread(curEid, cursor, rulePtr->filter.ssrc_len, &bytes_left);
		rulePtr->filter.sec_src = bsl_ed_get_ref(wm, curEid);
		MRELEASE(curEid);
	}

	if(BPSEC_RULE_BTYP_IDX(rulePtr))
	{
		cursor += bsl_sdr_bufread(&(rulePtr->filter.blk_type), cursor, sizeof(rulePtr->filter.blk_type), &bytes_left);
	}

	if(BPSEC_RULE_SCID_IDX(rulePtr))
	{
		cursor += bsl_sdr_bufread(&(rulePtr->filter.scid), cursor, sizeof(rulePtr->filter.scid), &bytes_left);
	}

	cursor += bslpol_sdr_scparm_restore(&(rulePtr->sc_cfgs.parms.aad),     cursor, &bytes_left);
	cursor += bslpol_sdr_scparm_restore(&(rulePtr->sc_cfgs.parms.icv),     cursor, &bytes_left);
	cursor += bslpol_sdr_scparm_restore(&(rulePtr->sc_cfgs.parms.intsig),  cursor, &bytes_left);
	cursor += bslpol_sdr_scparm_restore(&(rulePtr->sc_cfgs.parms.iv),      cursor, &bytes_left);
	cursor += bslpol_sdr_scparm_restore(&(rulePtr->sc_cfgs.parms.keyinfo), cursor, &bytes_left);
	cursor += bslpol_sdr_scparm_restore(&(rulePtr->sc_cfgs.parms.salt),    cursor, &bytes_left);

	if(BPSEC_RULE_ANON_ES(rulePtr))
	{
		cursor += bsles_sdr_deserialize(wm, &(rulePtr->eventSet), cursor, &bytes_left);
	}

	if(cursor != (buffer + entry.size))
	{
		putErrmsg("Error restoring rule.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	MRELEASE(buffer);

	return bslpol_rule_insert(wm, ruleAddr);
}

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
		size += sizeof(rulePtr->filter.bsrc_len) + rulePtr->filter.bsrc_len;
	}

	if(BPSEC_RULE_BDST_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.bdest_len) + rulePtr->filter.bdest_len;
	}

	if(BPSEC_RULE_SSRC_IDX(rulePtr))
	{
		size += sizeof(rulePtr->filter.ssrc_len) + rulePtr->filter.ssrc_len;
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

	size += rulePtr->sc_cfgs.parms.aad.length     + sizeof(uvast) + sizeof(uint32_t);
	size += rulePtr->sc_cfgs.parms.icv.length     + sizeof(uvast) + sizeof(uint32_t);
	size += rulePtr->sc_cfgs.parms.intsig.length  + sizeof(uvast) + sizeof(uint32_t);
	size += rulePtr->sc_cfgs.parms.iv.length      + sizeof(uvast) + sizeof(uint32_t);
	size += rulePtr->sc_cfgs.parms.keyinfo.length + sizeof(uvast) + sizeof(uint32_t);
	size += rulePtr->sc_cfgs.parms.salt.length    + sizeof(uvast) + sizeof(uint32_t);
	size += BPSEC_RULE_ANON_ES(rulePtr) ? bsles_sdr_size(wm, rulePtr->eventSet) : 0;

	return size;
}


Address bslpol_sdr_scparm_persist(char *cursor, sci_inbound_tlv parm, int *bytes_left)
{
	Address total = 0;

	CHKZERO(cursor);
	CHKZERO(bytes_left);

	total = bsl_sdr_bufwrite(cursor, &(parm.id), sizeof(parm.id), bytes_left);
	total += bsl_sdr_bufwrite(cursor + total, &(parm.length), sizeof(parm.length), bytes_left);
	total += bsl_sdr_bufwrite(cursor + total, parm.value, parm.length, bytes_left);

	return total;
}

Address bslpol_sdr_scparm_restore(sci_inbound_tlv *parm, char *cursor, int *bytes_left)
{
	Address total = 0;

	CHKZERO(parm);
	CHKZERO(cursor);
	CHKZERO(bytes_left);

	total = bsl_sdr_bufread(&(parm->id), cursor, sizeof(parm->id), bytes_left);
	total += bsl_sdr_bufread(&(parm->length), cursor + total, sizeof(parm->length), bytes_left);
	total += bsl_sdr_bufread(&(parm->value), cursor + total, parm->length, bytes_left);

	return total;

}







/******************************************************************************
 * @brief Determines whether a rule is a better match than the current best
 *        matched rule for an extension block.
 *
 * @param[in] partition - The shared memory partition
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

	CHKZERO(ruleAddr);
	rulePtr = (BpSecPolRule *) psp(partition, ruleAddr);

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


int bslpol_cb_rulelyst_compare_score(BpSecPolRule *r1, BpSecPolRule *r2)
{
	int result = 0;

	if((result = r1->filter.score - r2->filter.score) == 0)
	{
		result = r1->idx - r2->idx;
	}

	return result;
}


void bslpol_cb_rulelyst_delete(PsmPartition partition, PsmAddress eltAddr, void *tag)
{

	bslpol_rule_delete(partition, sm_list_data(partition, eltAddr));
}



// TODO: Check return values.
/*
 * Sorts descending by score and descending by index.
 * using the notation (Score,Idx) then the following rules:
 * (1,1), (3,4), (3,2), (3,5), (2,2) would be sorted as follows:
 *
 * (3,5), (3,4), (3,2), (2,2), (1,1)
 *
 * This compare function
 *
 */

int bslpol_cb_ruleradix_insert(PsmPartition partition, PsmAddress *entryAddr, PsmAddress itemAddr)
{
	BpSecPolRuleEntry *entryPtr = NULL;
	BpSecPolRule *rulePtr = NULL;

	CHKZERO(entryAddr);
	CHKZERO(itemAddr);

	if(*entryAddr == 0)
	{
		*entryAddr = psm_zalloc(partition, sizeof(BpSecPolRuleEntry));
		CHKZERO(*entryAddr);
	}

	entryPtr = (BpSecPolRuleEntry*) psp(partition, *entryAddr);

	if(entryPtr->rules == 0)
	{
		entryPtr->rules = sm_list_create(partition);
		CHKZERO(entryPtr->rules);
	}

	rulePtr = psp(partition, itemAddr);

	sm_list_insert(partition, entryPtr->rules, itemAddr, bslpol_cb_rule_compare_score, rulePtr);

	return 1;
}



int bslpol_cb_ruleradix_remove(PsmPartition partition, PsmAddress entryAddr, void *tag)
{
	PsmAddress *ruleAddr = (PsmAddress*) tag;
	BpSecPolRuleEntry *entryPtr = NULL;
	PsmAddress eltAddr = 0;

	CHKZERO(partition);
	CHKZERO(entryAddr);
	CHKZERO(ruleAddr);

	entryPtr = (BpSecPolRuleEntry*) psp(partition, entryAddr);

	for(eltAddr = sm_list_first(partition, entryPtr->rules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{

		if(sm_list_data(partition, eltAddr) == *ruleAddr)
		{
			sm_list_delete(partition, eltAddr, NULL, NULL);
			return RADIX_STOP_FOREACH;
		}
	}
	return 1;
}



/*
 * Search the data for a better rule...
 */
int bslpol_cb_ruleradix_search_best(PsmPartition partition, PsmAddress entryAddr, BpSecPolRuleSearchBestTag *tag)
{
	PsmAddress eltAddr = 0;
	BpSecPolRuleEntry *entryPtr = NULL;
	PsmAddress ruleAddr = 0;

	CHKZERO(entryAddr);
	CHKZERO(tag);

	entryPtr = (BpSecPolRuleEntry *) psp(partition, entryAddr);

	for(eltAddr = sm_list_first(partition, entryPtr->rules);
		eltAddr;
		eltAddr = sm_list_next(partition, eltAddr))
	{
		ruleAddr = sm_list_data(partition,eltAddr);

		if(bslpol_search_tag_best(partition, tag, ruleAddr) == 1)
		{
			return 1;
		}
	}
	return 0;
}
