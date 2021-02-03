/*****************************************************************************
 **
 ** File Name: bpsec_policy.c
 **
 ** Description:
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **                           Initial implementation
 **
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_policy.h"
#include "bpsec_policy_rule.h"
#include "bpsec_policy_eventset.h"

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/

/******************************************************************************
 * @brief Initializes all parts of the BPSec Policy Engine.
 *
 * @note
 *   - TODO: Check all return codes.
 *   - TODO: Check to see if we need to allocate storage structs in shared
 *           memory...
 *
 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int bsl_all_init(PsmPartition partition)
{
	CHKERR(partition);

	bsl_vdb_init(partition);
	bsl_sdr_bootstrap(partition);

	return 1;
}


/*
 * Build the BSL Policy SDR DB and Initialize it.
 */
int bsl_sdr_bootstrap(PsmPartition wm)
{
	Sdr ionsdr = getIonsdr();
	SecDB *secdb = getSecConstants();
	SecVdb *secvdb = getSecVdb();
	Object sdrElt = 0;
	BpSecPolicyDbEntry entry;

	CHKERR(secdb);
	CHKERR(secvdb);

	if(sm_rbt_length(wm, getSecVdb()->bpsecEventSet) == 0)
	{
		CHKERR(sdr_begin_xn(ionsdr));
		for(sdrElt = sdr_list_first(ionsdr, secdb->bpSecEventSets);
			sdrElt;
			sdrElt = sdr_list_next(ionsdr, sdrElt))
		{
			sdr_read(ionsdr, (char *) &entry, sdr_list_data(ionsdr, sdrElt), sizeof(BpSecPolicyDbEntry));
			bsles_sdr_restore(wm, entry);
		}
		sdr_exit_xn(ionsdr);
	}


	if(sm_list_length(wm, getSecVdb()->bpsecPolicyRules) == 0)
	{
		CHKERR(sdr_begin_xn(ionsdr));
		for(sdrElt = sdr_list_first(ionsdr, secdb->bpSecPolicyRules);
			sdrElt;
			sdrElt = sdr_list_next(ionsdr, sdrElt))
		{
			sdr_read(ionsdr, (char *) &entry, sdr_list_data(ionsdr, sdrElt), sizeof(BpSecPolicyDbEntry));
			bslpol_sdr_rule_restore(wm, entry);
		}
		sdr_exit_xn(ionsdr);
	}

	return 0;
}



/******************************************************************************
 * @brief Generates a pointer into the EID Dictionary for a given EID.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     eid - The EID whose dictionary reference is being queried.
 *
 * @note
 * If the EID does not exist in the dictionary then a new entry will be created
 * in the EID dictionary for that EID.
 * \par
 * The passed-in EID MUST be managed/freed by the calling function.
 * \par
 * The returned EID dictionary reference MUST NOT be altered/freed by the
 * calling function.
 *
 * @retval  !0 - The EID Dictionary reference (treat this as constant)
 * @retval  0 - Failure
 *****************************************************************************/

PsmAddress bsl_ed_get_ref(PsmPartition partition, char *eid)
{
	PsmAddress userDataAddr = 0;

	/* Step 0: Sanity Checks. */
	CHKZERO(partition);
	CHKZERO(eid);

	/* Step 1: See if we have this EID in the dictionary. */
	userDataAddr = radix_find(partition, getSecVdb()->bpsecEidDictionary, eid);

	/* Step 2: If we have no ref, make a ref. */
	if(userDataAddr == 0)
	{
		/* Step 2.1: Allocate and init new EID. */
		int len = istrlen(eid, MAX_EID_LEN);
		char *dataPtr = NULL;

		userDataAddr = psm_zalloc(partition, len + 1);
		CHKZERO(userDataAddr);
		dataPtr = (char *) psp(partition, userDataAddr);
		istrcpy(dataPtr, eid, len);

		/* Step 2.2: Insert it into the EID dictionary. */
		if(radix_insert(partition, getSecVdb()->bpsecEidDictionary, eid, userDataAddr) != 1)
		{
			psm_free(partition, userDataAddr);
			userDataAddr = 0;
		}
	}

	/* Step 3: Return either the found or newly created ref. */
	return userDataAddr;
}


int bsl_sdr_insert(Sdr ionsdr, char *buffer, BpSecPolicyDbEntry entry, Object list)
{
	Object itemObj = 0;

	CHKERR(sdr_begin_xn(ionsdr));

	if((itemObj = sdr_malloc(ionsdr, sizeof(BpSecPolicyDbEntry))) == 0)
	{
		sdr_cancel_xn(ionsdr);
		MRELEASE(buffer);
		putErrmsg("Cannot persist policy entry.", NULL);
		return -1;
	}

	if((entry.entryObj = sdr_malloc(ionsdr, entry.size)) == 0)
	{
		sdr_cancel_xn(ionsdr);
		MRELEASE(buffer);
		putErrmsg("Cannot persist serialized item.", NULL);
		return -1;
	}

	sdr_write(ionsdr, entry.entryObj, buffer, entry.size);
	sdr_write(ionsdr, itemObj, (char *) &entry, sizeof(BpSecPolicyDbEntry));
	sdr_list_insert_last(ionsdr, list, itemObj);

	sdr_end_xn(ionsdr);

	MRELEASE(buffer);

	return 0;
}

Address bsl_sdr_bufread(void *value, char *cursor, int length, int *bytes_left)
{
	CHKZERO(*bytes_left >= length);
	memcpy(value, cursor, length);
	*bytes_left -= length;
	return length;
}

Address bsl_sdr_bufwrite(char *cursor, void *value, int length, int *bytes_left)
{
	CHKZERO(*bytes_left >= length);

	memcpy(cursor, value, length);
	*bytes_left -= length;
	return length;
}




int bsl_vdb_init(PsmPartition partition)
{
	SecVdb *secvdb = getSecVdb();
	if(secvdb->bpsecPolicyRules != 0)
	{
		return 1;
	}

	/* Step 1 - Create data structures used to store policy information */

	/* Step 1.1: Make the sm_list that stores policy rules. */

	secvdb->bpsecPolicyRules = sm_list_create(partition);

	/* Step 1.2: Make radix trees to look up rules by various EIDs. */
	secvdb->bpsecRuleIdxBySrc  = radix_create((radix_insert_fn)bslpol_cb_ruleradix_insert, NULL, partition);
	secvdb->bpsecRuleIdxByDest = radix_create((radix_insert_fn)bslpol_cb_ruleradix_insert, NULL, partition);
	secvdb->bpsecRuleIdxBySSrc = radix_create((radix_insert_fn)bslpol_cb_ruleradix_insert, NULL, partition);

	/* Step 1.3: Make radix tree to store EID dictionary. */

	secvdb->bpsecEidDictionary = radix_create(NULL, bsl_cb_ed_delete, partition);

	/* Step 1.4: Make red-black tree to store named eventsets. */
	secvdb->bpsecEventSet = sm_rbt_create(partition);

	return 1;
}

/******************************************************************************
 * @brief Cleans up the BPSec Policy Engine resources.
 *
 * @note
 *   - TODO: Check all return codes.
 *   - TODO: Check to see if we need to allocate storage structs in shared
 *           memory...
 *****************************************************************************/

void bsl_vdb_teardown(PsmPartition partition)
{
	SecVdb *secvdb = getSecVdb();

	radix_destroy(partition, secvdb->bpsecRuleIdxBySrc);
	radix_destroy(partition, secvdb->bpsecRuleIdxByDest);
	radix_destroy(partition, secvdb->bpsecRuleIdxBySSrc);

	sm_list_destroy(partition, secvdb->bpsecPolicyRules, bslpol_cb_rulelyst_delete, NULL);

	radix_destroy(partition, secvdb->bpsecEidDictionary);
	//TODO: Delete RBT?
}



void bsl_cb_ed_delete(PsmPartition partition, PsmAddress user_data)
{
	CHKVOID(partition);
	CHKVOID(user_data);
	psm_free(partition, user_data);
}


/******************************************************************************
 *
 * \par Function Name: bslpol_remove_sop_at_sender
 *
 * \par Purpose: This function removes the described security operation from
 *               a given bundle. The security operation is identified by the
 *               security block type, target block type, and meta target
 *               block type.
 *
 * \param[in]  bundle        Current, working bundle.
 * \param[in]  sopBlk        Security block representing the security operation.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_remove_sop_at_sender(Bundle *bundle, ExtensionBlock *sopBlk)
{
	/* Step 0: Sanity checks. */
	CHKVOID(bundle);

	/* Step 1: Find security block representing the security operation */
	Object sop = getExtensionBlock(bundle, sopBlk->number);

	if (sop)
	{
		/* Step 2: If found, remove the security block from the bundle */
		deleteExtensionBlock(sop, &(bundle->extensionsLength));
	}
}

/******************************************************************************
 *
 * \par Function Name: bslpol_remove_sop_at_receiver
 *
 * \par Purpose: This function removes the described security operation from
 *               a given bundle. The security operation is identified by the
 *               security block type and its block number.
 *
 * \param[in]  wk         Work area holding bundle information.
 * \param[in]  sopElt     Lyst element of the security block representing
 *                        the security operation.
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_remove_sop_at_receiver(AcqWorkArea *wk, LystElt sopElt)
{
	/* Step 0: Sanity checks */
	CHKVOID(wk);
	CHKVOID(sopElt);

	AcqExtBlock *secBlk = (AcqExtBlock *) lyst_data(sopElt);
	Bundle *bundle = &(wk->bundle);

	/* Step 1: Find security block representing the security operation */
	Object sop = getExtensionBlock(bundle, secBlk->number);

	if (sop)
	{
		/* Step 2: If found, remove the security block from the bundle */
		deleteExtensionBlock(sop, &(bundle->extensionsLength));
	}
}

/* NOTE: Adapted from bib/bcb.c */
static void	bsl_discardTarget(LystElt targetElt, LystElt sopElt)
{
	BpsecInboundTarget	*target;
	AcqExtBlock		*sop;
	BpsecInboundBlock	*asb;

	target = (BpsecInboundTarget *) lyst_data(targetElt);
	bpsec_releaseInboundTlvs(target->results);
	MRELEASE(target);
	lyst_delete(targetElt);
	sop = (AcqExtBlock *) lyst_data(sopElt);
	asb = (BpsecInboundBlock *) (sop->object);
	if (lyst_length(asb->targets) == 0)
	{
		deleteAcqExtBlock(sopElt);
	}
}

/* NOTE: Adapted from bib/bcb.c */
static Object bsl_findOutboundTarget(Bundle *bundle, int blockNumber, BpBlockType sopType)
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
		if (block.type != sopType)
		{
			continue;
		}

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
/******************************************************************************
 *
 * \par Function Name: bslpol_remove_sop_target_at_sender
 *
 * \par Purpose: This function removes the security target of the security
 *               block provided.
 *
 * \param[in]  bundle       Current, working bundle.
 * \param[in]  sopBlk       Security block representing the security operation.
 * \param[in]  tgtNum       Block number of the security target block.
 *
 * \TODO Handle primary block and payload block as targets. Function currently
 *  only removes extension blocks from the bundle.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_remove_sop_target_at_sender(Bundle *bundle, ExtensionBlock *sopBlk,
		unsigned char tgtNum)
{
	/* Step 0: Sanity checks. */
	CHKVOID(bundle);
	CHKVOID(tgtNum);

	/* Step 1: Search for the security target block */
	Object tgt = bsl_findOutboundTarget(bundle, tgtNum, sopBlk->type);

	if (tgt)
	{
		/* Step 2: Remove the target block from the bundle */
		deleteExtensionBlock(tgt, &(bundle->extensionsLength));
	}
}

/******************************************************************************
 *
 * \par Function Name: bslpol_remove_sop_target_at_receiver
 *
 * \par Purpose: This function removes the security target of the security
 *               block provided.
 *
 * \param[in]  tgtElt   Lyst element representing the security target block.
 * \param[in]  sopElt   Lyst element: the security block representing the
 *                      security operation.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_remove_sop_target_at_receiver(LystElt tgtElt, LystElt sopElt)
{
	/* Step 0: Sanity checks. */
	CHKVOID(tgtElt);
	CHKVOID(sopElt);

	/* Step 1: Discard target block */
	bsl_discardTarget(tgtElt, sopElt);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_remove_all_target_sops_at_sender
 *
 * \par Purpose: This function removes all security operations targeting a
 *               specified block.
 *
 * \param[in]  bundle       Current, working bundle.
 * \param[in]  tgtNum       Block number of the security operation's target.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_remove_all_target_sops_at_sender(Bundle *bundle, unsigned char tgtNum)
{
	/* Step 0: Sanity checks. */
	CHKVOID(bundle);
	CHKVOID(tgtNum);

	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
	Object tgtTypeElt;
	BpBlockType blkTgtType;
	BpsecOutboundBlock	asb;
	ExtensionBlock tgt;
	OBJ_POINTER(ExtensionBlock, blk);

	Object tgtObj = getExtensionBlock(bundle, tgtNum);
	sdr_read(sdr, (char *) &tgt, tgtObj, sizeof(ExtensionBlock));

	for (elt = sdr_list_first(sdr, bundle->extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (blk->type == BlockIntegrityBlk || (blk->type == BlockConfidentialityBlk))
		{
			sdr_read(sdr, (char *) &asb, blk->object, sizeof(BpsecOutboundBlock));
			for (tgtTypeElt = sdr_list_first(sdr, asb.targets); tgtTypeElt; tgtTypeElt = sdr_list_next(sdr, tgtTypeElt))
			{
				blkTgtType = (BpBlockType) sdr_list_data(sdr, tgtTypeElt);
				if (blkTgtType == tgt.type)
				{
					bsl_remove_sop_at_sender(bundle, blk);
				}
			}
		}
	}
}

/******************************************************************************
 *
 * \par Function Name: bslpol_remove_all_target_sops_at_receiver
 *
 * \par Purpose: This function removes all security operations targeting a
 *               specified block.
 *
 * \param[in]  wk         Work area holding bundle information.
 * \param[in]  sopElt     Security block representing the security operation.
 * \param[in]  tgtNum     Block number of the security target block.
 *
 * \note When target multiplicity is complete, the bslpol_remove_sop_target
 *       function should be used to remove just the target block, rather than
 *       assume the security block has a single target and may always be
 *       removed from the bundle.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_remove_all_target_sops_at_receiver(AcqWorkArea *wk, LystElt sopElt,
		unsigned char tgtNum)
{
	/* Step 0: Sanity checks. */
	CHKVOID(wk);
	CHKVOID(sopElt);
	CHKVOID(tgtNum);

	Sdr	sdr = getIonsdr();
	AcqExtBlock *sopBlk = (AcqExtBlock *) lyst_data(sopElt);
	BpsecInboundBlock *asb = (BpsecInboundBlock *) (sopBlk->object);
	AcqExtBlock	*block;
	LystElt blkTypeElt;
	LystElt elt;
	BpBlockType blkTgtType;
	ExtensionBlock tgt;

	Object tgtObj = getExtensionBlock(&wk->bundle, tgtNum);
	sdr_read(sdr, (char *) &tgt, tgtObj, sizeof(ExtensionBlock));


	for (elt = lyst_first(wk->extBlocks); elt; elt = lyst_next(elt))
	{
		block = (AcqExtBlock *) lyst_data(elt);

		if ((block->type == BlockIntegrityBlk) || (block->type == BlockConfidentialityBlk))
		{
			for (blkTypeElt = lyst_first(asb->targets); blkTypeElt; blkTypeElt = lyst_next(blkTypeElt))
			{
				blkTgtType = (BpBlockType) lyst_data(blkTypeElt);
				if (blkTgtType == tgt.type)
				{
					/* Remove the security block */
					bsl_remove_sop_at_receiver(wk, elt);
				}
			}
		}
	}
}

/******************************************************************************
 *
 * \par Function Name: bslpol_do_not_forward_at_sender
 *
 * \par Purpose: This function ends bundle transmission at the current node.
 *
 * \param[in]  bundle     Current, working bundle.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *
 *****************************************************************************/
void bsl_do_not_forward_at_sender(Bundle *bundle)
{
	CHKVOID(bundle);

	//TODO: Suspend without raw bundle? bp_suspend(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_do_not_forward_at_receiver
 *
 * \par Purpose: This function ends bundle transmission at the current node.
 *
 * \param[in]  wk         Work area holding bundle information.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_do_not_forward_at_receiver(AcqWorkArea *wk)
{
	CHKVOID(wk);

	bp_suspend(wk->rawBundle);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_request_storage
 *
 * \par Purpose: This function will store the bundle in its current state at
 *               time of calling.
 *
 * \param[in]  bundle     Current, working bundle.
 *
 * \TODO: This feature is not implemented for the ION 4.0.2 point release and
 *        will be completed at a later date.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *
 *****************************************************************************/
void bslpol_request_storage(Bundle *bundle)
{
	CHKVOID(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_report_reason_code_at_sender
 *
 * \par Purpose: This function will send a bundle status report with the
 *               provided reason code.
 *
 * \param[in]  bundle   Current, working bundle.
 * \param[in]  reason   The reason code to include in the bundle status report.
 *
 * \TODO: In a later release, this function must accumulate reason codes during
 *        security processing and send a single status report for each reason
 *        code. Currently, the function will send a status report each time it
 *        is called without eliminating duplicate reports.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/26/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_report_reason_code_at_sender(Bundle *bundle, BpSrReason reason)
{
	/* Step 0: Sanity checks. */
	CHKVOID(bundle);
	CHKVOID(reason);

	bundle->statusRpt.reasonCode = reason;

	if (bundle->bundleProcFlags & BDL_STATUS_TIME_REQ)
	{
		getCurrentDtnTime(&(bundle->statusRpt.deletionTime));
	}

	sendStatusRpt(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_report_reason_code_at_receiver
 *
 * \par Purpose: This function will send a bundle status report with the
 *               provided reason code.
 *
 * \param[in]  wk       Work area holding bundle information.
 * \param[in]  reason   The reason code to include in the bundle status report.
 *
 * \TODO: In a later release, this function must accumulate reason codes during
 *        security processing and send a single status report for each reason
 *        code. Currently, the function will send a status report each time it
 *        is called without eliminating duplicate reports.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_report_reason_code_at_receiver(AcqWorkArea *wk, BpSrReason reason)
{
	/* Step 0: Sanity checks. */
	CHKVOID(wk);

	Bundle *bundle = &(wk->bundle);

	bsl_report_reason_code_at_sender(bundle, reason);


}
/******************************************************************************
 *
 * \par Function Name: bslpol_override_target_bpcf
 *
 * \par Purpose: This function will override the block processing control
 *               flags of a security target block.
 *
 * \param[in]  bundle     Current, working bundle.
 *
 * \TODO: This feature is not implemented for the ION 4.0.2 point release and
 *        will be completed at a later date.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *
 *****************************************************************************/
void bslpol_override_target_bpcf(Bundle *bundle)
{
	CHKVOID(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_override_sop_bpcf
 *
 * \par Purpose: This function will override the block processing control
 *               flags of a security block.
 *
 * \param[in]  bundle     Current, working bundle.
 *
 * \TODO: This feature is not implemented for the ION 4.0.2 point release and
 *        will be completed at a later date.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *
 *****************************************************************************/
void bslpol_override_sop_bpcf(Bundle *bundle)
{
	CHKVOID(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bslpol_handle_sender_sop_event
 *
 * \par Purpose: This function is to be called each time a security operation
 *               event occurence is identified by a BPA taking on the role
 *               of Security Source. Bundle characteristics and identifying
 *               information from the security operation itself are used to
 *               determine the security policy rule which is applicable.
 *               If such a security policy rule is found, the function then
 *               determines if any optional processing events have been
 *               configured for the current security operation event and
 *               executes them.
 *
 * \param[in]  bundle        Current, working bundle.
 * \param[in]  sopEvent      The security operation event to respond to.
 * \param[in]  sop           Security block representing the security operation.
 * \param[in]  asb           Abstract security block for the security operation.
 * \param[in]  tgtType       Block type of the security operation's target.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
int bsl_handle_sender_sop_event(Bundle *bundle, BpSecEventId sopEvent,
		ExtensionBlock *sop, BpsecOutboundBlock *asb, unsigned char tgtNum)
{
	/* Step 0: Sanity checks */
	CHKERR(bundle);
	CHKERR(sopEvent >= 0);
	CHKERR(sop);
	CHKERR(tgtNum);

	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));
	PsmPartition wm = getIonwm();

	/* Step 1: Populate the policy rule search tag */
	if (tgtNum == 0)
	{
		tag.type = PrimaryBlk;
	}
	else if (tgtNum == PayloadBlk)
	{
		tag.type = PayloadBlk;
	}
	else
	{
		ExtensionBlock tgt;
		Sdr	sdr = getIonsdr();
		Object tgtObj = getExtensionBlock(bundle, tgtNum);
		sdr_read(sdr, (char *) &tgt, tgtObj,sizeof(ExtensionBlock));
		tag.type = tgt.type;
	}

	tag.scid = asb->contextId;

	readEid(&asb->securitySource, &(tag.ssrc));
	tag.ssrc_len = strlen(tag.ssrc);

	readEid(&bundle->id.source, &(tag.bsrc));
	tag.bsrc_len = strlen(tag.bsrc);

	readEid(&bundle->destination, &(tag.bdest));
	tag.bdest_len = strlen(tag.bdest);

	/* Step 2: Retrieve the rule for the current security operation */
	BpSecPolRule *rule = bslpol_rule_get_best_match(wm, tag);

	MRELEASE(tag.bsrc);
	MRELEASE(tag.bdest);
	MRELEASE(tag.ssrc);

	/* Step 3: Apply the rule to the bundle */
	if (rule != NULL)
	{
		/* Step 2.1: Retrieve the event set for the rule */
		PsmAddress esAddr = rule->eventSet;
		BpSecEventSet *esPtr = (BpSecEventSet *) psp(wm, esAddr);

		/* Step 2.2: Check if policy is configured for the current SOP event */
		if(esPtr->mask & sopEvent)
		{
			PsmAddress elt;
			PsmAddress curEventAddr = 0;
			BpSecEvent *curEventPtr = NULL;

			/* Step 2.3: Find the event with actions to be executed */
			for(elt = sm_list_first(wm, esPtr->events); elt; elt = sm_list_next(wm, elt))
			{
				curEventAddr = sm_list_data(wm, elt);
				curEventPtr = (BpSecEvent *) psp(wm, curEventAddr);
				if(curEventPtr->id == sopEvent)
				{
					/* Step 2.4: Execute the optional processing actions */
					if (curEventPtr->action_mask & BSLACT_REMOVE_SOP)
					{
						bsl_remove_sop_at_sender(bundle, sop);
					}

					if (curEventPtr->action_mask & BSLACT_REMOVE_SOP_TARGET)
					{
						bsl_remove_sop_target_at_sender(bundle, sop, tgtNum);
					}

					if (curEventPtr->action_mask & BSLACT_REMOVE_ALL_TARGET_SOPS)
					{
						bsl_remove_all_target_sops_at_sender(bundle, tgtNum);
					}

					if (curEventPtr->action_mask & BSLACT_DO_NOT_FORWARD)
					{
						bsl_do_not_forward_at_sender(bundle);
					}

					if (curEventPtr->action_mask & BSLACT_REPORT_REASON_CODE)
					{
						bsl_report_reason_code_at_sender(bundle,
								curEventPtr->action_parms[0].asReason.reasonCode);
					}
				}
			}
			return 1;
		}

		/*
		 * Step 2.6 : Policy is not configured for this SOP event. No optional
		 * processing actions to execute.
		 */
		else
		{
			return 1;
		}
	}

	else
	{
		return 1;
	}

	return -1;
}

/******************************************************************************
 *
 * \par Function Name: bslpol_handle_receiver_sop_event
 *
 * \par Purpose: This function is to be called each time a security operation
 *               event occurence is identified by a BPA taking on the role
 *               of Security Verifier or Security Acceptor. Bundle
 *               characteristics and identifying information from the security
 *               operation itself are used to determine the security policy rule
 *               which is applicable. If such a security policy rule is found,
 *               the function then determines if any optional processing events
 *               have been configured for the current security operation event
 *               and executes them.
 *
 * \param[in]  wk       Work area holding bundle information.
 * \param[in]  role      Security policy role.
 * \param[in]  sopEvent  The security operation event to respond to.
 * \param[in]  sop       The security block representing the security operation
 *                       in the current bundle.
 * \param[in]  tgt       The security target block.
 * \param[in]  tgtType   The block type of the securtity target block.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/29/21   S. Heiner      Initial Implementation
 *****************************************************************************/
int bsl_handle_receiver_sop_event(AcqWorkArea *wk, int role,
		BpSecEventId sopEvent, LystElt sop, LystElt tgt, unsigned char tgtNum)
{
	/* Step 0: Sanity checks */
	CHKERR(wk);
	CHKERR((role == BPRF_VER_ROLE) || (role == BPRF_ACC_ROLE));
	CHKERR(sopEvent >= 0);
	CHKERR(sop);

	Bundle *bundle = &(wk->bundle);
	PsmPartition wm = getIonwm();

	AcqExtBlock *secBlk = (AcqExtBlock *) lyst_data(sop);
	BpsecInboundBlock *asb = (BpsecInboundBlock *) (secBlk->object);

	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));

	/* Step 1: Populate the policy rule search tag */
	if (tgtNum == 0)
	{
		tag.type = PrimaryBlk;
	}
	else if (tgtNum == PayloadBlk)
	{
		tag.type = PayloadBlk;
	}
	else
	{
		ExtensionBlock tgt;
		Sdr	sdr = getIonsdr();
		Object tgtObj = getExtensionBlock(bundle, tgtNum);
		sdr_read(sdr, (char *) &tgt, tgtObj,sizeof(ExtensionBlock));
		tag.type = tgt.type;
	}

	tag.scid = asb->contextId;

	readEid(&asb->securitySource, &(tag.ssrc));
	tag.ssrc_len = strlen(tag.ssrc);

	readEid(&bundle->id.source, &(tag.bsrc));
	tag.bsrc_len = strlen(tag.bsrc);

	readEid(&bundle->destination, &(tag.bdest));
	tag.bdest_len = strlen(tag.bdest);

	/* Step 2: Retrieve the rule for the current security operation */
	BpSecPolRule *rule = bslpol_rule_get_best_match(wm, tag);

	MRELEASE(tag.bsrc);
	MRELEASE(tag.bdest);
	MRELEASE(tag.ssrc);

	/* Step 3: Apply the rule to the bundle */
	if (rule != NULL)
	{
		/* Step 2.1: Retrieve the event set for the rule */
		PsmAddress esAddr = rule->eventSet;
		BpSecEventSet *esPtr = (BpSecEventSet *) psp(wm, esAddr);

		/* Step 2.2: Check if policy is configured for the current SOP event */
		if(esPtr->mask & sopEvent)
		{
			PsmAddress elt;
			PsmAddress curEventAddr = 0;
			BpSecEvent *curEventPtr = NULL;

			/* Step 2.3: Find the event with actions to be executed */
			for(elt = sm_list_first(wm, esPtr->events); elt; elt = sm_list_next(wm, elt))
			{
				curEventAddr = sm_list_data(wm, elt);
				curEventPtr = (BpSecEvent *) psp(wm, curEventAddr);
				if(curEventPtr->id == sopEvent)
				{
					/* Step 2.4: Execute the optional processing actions */
					if (curEventPtr->action_mask & BSLACT_REMOVE_SOP)
					{
						bsl_remove_sop_at_receiver(wk, sop);
					}

					if (curEventPtr->action_mask & BSLACT_REMOVE_SOP_TARGET)
					{
						bsl_remove_sop_target_at_receiver(tgt, sop);
					}

					if (curEventPtr->action_mask & BSLACT_REMOVE_ALL_TARGET_SOPS)
					{
						bsl_remove_all_target_sops_at_receiver(wk, sop, tgtNum);
					}

					if (curEventPtr->action_mask & BSLACT_DO_NOT_FORWARD)
					{
						bsl_do_not_forward_at_receiver(wk);
					}

					if (curEventPtr->action_mask & BSLACT_REPORT_REASON_CODE)
					{
						bsl_report_reason_code_at_receiver(wk, curEventPtr->action_parms[0].asReason.reasonCode);
					}
				}
			}
			return 1;
		}

		/*
		 * Step 2.6 : Policy is not configured for this SOP event. No optional
		 * processing actions to execute.
		 */
		else
		{
			return 1;
		}
	}

	else
	{
		return 1;
	}

	return -1;
}
