/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bpsec_policy.c
 **
 ** Description: This file contains general utilities for initializing the
 **              BPSec policy engine and applying policy actions to blocks.
 **
 ** Notes:
 **  BPSec policy is implemented as a distributed set of utilities that
 **  coordinate information through ION shared memory and the ION SDR. As
 **  such, initialization routines need to be made idempotent. This is
 **  done by checking to see if a prior initialization was completed before
 **  running any initialization routine.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/22/21  S. Heiner &    Initial implementation
 **            E. Birrane
 **
 ****************************************************************************/

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
 * @param[in,out] partition - The shared memory partition
 *
 * @note
 *
 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int bsl_all_init(PsmPartition partition)
{
	int success = 0;
	int result = 0;
	CHKERR(partition);

	if((result = bsl_vdb_init(partition)) == 1)
	{
		success = bsl_sdr_bootstrap(partition);
	}
	else
	{
		success = (result > 0);
	}


	return success;
}



/******************************************************************************
 * @briefReads Helper function to read a value from a buffer
 *
 * BPSec policy serialize/deserialize routines use an iterator pattern to
 * read chunks of data from a buffer. This function helps deserialize routines
 * by reading a fixed length value from a serialized string.
 *
 * @param[out] value      - The item holding the extracted value.
 * @param[in]  cursor     - A pointer into a serialized buffer holding the value
 * @param[in]  length     - Byte length of value being read.
 * @param[out] bytes_left - The number of bytes remaining in the buffer pointed
 *                          to by the cursor.
 *
 * @note
 * The value is copied out of the serialized buffer into the value.
 * \par
 * The caller is expected to have pre-allocated the value and ensure it is of
 * the size indicated by "length"
 *
 * @retval  >0 - The number of bytes to advance the cursor address
 * @retval   0 - Failure
 *****************************************************************************/

Address bsl_bufread(void *value, char *cursor, int length, int *bytes_left)
{
	CHKZERO(*bytes_left >= length);
	memcpy(value, cursor, length);
	*bytes_left -= length;
	return length;
}



/******************************************************************************
 * @briefReads Helper function to write a value into a buffer
 *
 * BPSec policy serialize/deserialize routines use an iterator pattern to
 * write chunks of data into a buffer. This function helps serialize routines
 * by writing a fixed length value into a serialized string.
 *
 * @param[out]    cursor     - A pointer into a serialized buffer being updated
 * @param[in]     value      - The value being written into the buffer @ cursor.
 * @param[in]     length     - Byte length of value being written.
 * @param[in,out] bytes_left - The number of bytes remaining in the buffer
 *                             pointed to by the cursor.
 *
 * @note
 * The caller is expected to have pre-allocated the buffer pointed into by
 * cursor.
 *
 * @retval  >0 - The number of bytes to advance the cursor address
 * @retval   0 - Failure
 *****************************************************************************/

Address bsl_bufwrite(char *cursor, void *value, int length, int *bytes_left)
{
	CHKZERO(*bytes_left >= length);

	memcpy(cursor, value, length);
	*bytes_left -= length;
	return length;
}



/******************************************************************************
 * @brief Generates a pointer into the EID Dictionary for a given EID.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     eid       - The EID whose dictionary reference is being queried.
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
	SecVdb *secvdb = getSecVdb();

	/* Step 0: Sanity Checks. */
	CHKZERO(partition);
	CHKZERO(eid);
	if (secvdb == NULL) return 0;

	/* Step 1: See if we have this EID in the dictionary. */
	userDataAddr = radix_find(partition, secvdb->bpsecEidDictionary, eid, 0);

	/* Step 2: If we have no ref, make a ref. */
	if(userDataAddr == 0)
	{
		/* Step 2.1: Allocate and init new EID. */
		int len = istrlen(eid, MAX_EID_LEN);
		char *dataPtr = NULL;

		userDataAddr = psm_zalloc(partition, len+1);
		CHKZERO(userDataAddr);
		dataPtr = (char *) psp(partition, userDataAddr);
		memset(dataPtr, 0, len+1);
		istrcpy(dataPtr, eid, len+1);

		/* Step 2.2: Insert it into the EID dictionary. */
		if(radix_insert(partition, secvdb->bpsecEidDictionary, eid, userDataAddr, NULL, bsl_cb_ed_delete) != 1)
		{
			psm_free(partition, userDataAddr);
			userDataAddr = 0;
		}
	}

	/* Step 3: Return either the found or newly created ref. */
	return userDataAddr;
}



/******************************************************************************
 * @briefReads BPSec objects from the SDR and builds them in shared memory
 *
 * @param[in,out] wm - The shared memory partition
 *
 * @note
 * This SDR bootstrapping must always be done once to avoid duplicate entries
 * being placed in shared memory. The way this is serialized is to see if
 * there are currently any objects in shared memory. If any such object exists,
 * then SDR bootstrapping must not occur because the system had already come
 * up previously.
 *
 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int bsl_sdr_bootstrap(PsmPartition wm)
{

	Sdr ionsdr = getIonsdr();
	SecDB *secdb = getSecConstants();
	SecVdb *secvdb = getSecVdb();
	Object sdrElt = 0;
	BpSecPolicyDbEntry entry;

	if (secdb == NULL) return -1;
	if (secvdb == NULL) return -1;

	// If we don't have any bpsec eventsets, see if any exist in the SDR.
	if(sm_rbt_length(wm, secvdb->bpsecEventSet) == 0)
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

	// If we don't have any bpsec policyrules, see if any exist in the SDR.
	if(sm_list_length(wm, secvdb->bpsecPolicyRules) == 0)
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

	return 1;
}



/******************************************************************************
 * @briefReads Helper function to insert a bpsec object into the SDR.
 *
 * The SDR entry object is a (Length,Value-Address) tuple. Inserting an
 * entry into the SDR involves placing the value into the SDR and then
 * writing the length and address of that value into the list of bpsec
 * policy objects.
 *
 * @param[in,out] ionsdr - The SDR (BPSec objects exist in the ION SDR).
 * @param[in]     buffer - The serialized bpsec object
 * @param[in]     entry  - The entry for the bpsec object
 * @param[in]     list   - Which SDR list to hold the entry
 *
 * @note

 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int bsl_sdr_insert(Sdr ionsdr, char *buffer, BpSecPolicyDbEntry entry, Object list)
{

	Object itemObj = 0;

	CHKERR(sdr_begin_xn(ionsdr));

	// Step 1 - Allocate space for a bpsec entry in the SDR.
	if((itemObj = sdr_malloc(ionsdr, sizeof(BpSecPolicyDbEntry))) == 0)
	{
		sdr_cancel_xn(ionsdr);
		putErrmsg("Cannot persist policy entry.", NULL);
		return -1;
	}

	// Step 2 - Allocate space for the serialized entry.
	if((entry.entryObj = sdr_malloc(ionsdr, entry.size)) == 0)
	{
		sdr_free(ionsdr, itemObj);
		sdr_cancel_xn(ionsdr);
		putErrmsg("Cannot persist serialized item.", NULL);
		return -1;
	}

	/*
	  Step 3 - Write it all to the SDR.
	           - Write the serialized entry value to the SDR.
	           - Write the entry information to the SDR
	           - Store the entry information location in the SDR list.
	 */
	sdr_write(ionsdr, entry.entryObj, buffer, entry.size);
	sdr_write(ionsdr, itemObj, (char *) &entry, sizeof(BpSecPolicyDbEntry));
	sdr_list_insert_last(ionsdr, list, itemObj);

	sdr_end_xn(ionsdr);

	return 1;
}



/******************************************************************************
 * @briefReads Initialize shared memory structures to hold BPSec policy objects
 *
 * @param[in,out] wm - The shared memory partition
 *
 * @retval  2 - System was already initialized.
 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int bsl_vdb_init(PsmPartition partition)
{

	SecVdb *secvdb = getSecVdb();

	/*
	 * Step 1: Make sure we have't already initialized the BPSec VDB. If we have
	 *         any secvdb Bpsec structure initialized, then we can presume that this
	 *         call to initialize the VDB has been called either by this calling thread
	 *         or by some other utility.
	 */

	if (secvdb == NULL) return -1;

	if(secvdb->bpsecPolicyRules != 0)
	{
		return 2;
	}

	// Step 2 - Create data structures used to store policy information

	// Step 2.1: Policy rules are stored in a shared memory linked list.

	secvdb->bpsecPolicyRules = sm_list_create(partition);

	// Step 2.2: Policy rules are indexed by EID for fast lookups.
	secvdb->bpsecRuleIdxBySrc  = radix_create(partition);
	secvdb->bpsecRuleIdxByDest = radix_create(partition);
	secvdb->bpsecRuleIdxBySSrc = radix_create(partition);

	// Step 2.3: An EID dictionary is used to reduce the size impact of EIDs.
	secvdb->bpsecEidDictionary = radix_create(partition);

	// Step 2.4: Make red-black tree to store named eventsets.
	secvdb->bpsecEventSet = sm_rbt_create(partition);

	return 1;
}



/******************************************************************************
 * @brief Cleans up the BPSec Policy Engine resources.
 *
 * @note
 * TODO: This is not currently called as there is not a single place where
 *       ION does a graceful shutdown. Stopping the ION node will clear shared
 *       memory and the ION memory pool, so this teardown may be unnecessary.
 *****************************************************************************/

void bsl_vdb_teardown(PsmPartition partition)
{
	/*
	SecVdb *secvdb = getSecVdb();

	if (secvdb == NULL) return;

	radix_destroy(partition, secvdb->bpsecRuleIdxBySrc, NULL);
	radix_destroy(partition, secvdb->bpsecRuleIdxByDest, NULL);
	radix_destroy(partition, secvdb->bpsecRuleIdxBySSrc, NULL);

	sm_list_destroy(partition, secvdb->bpsecPolicyRules, bslpol_cb_smlist_delete, NULL);

	radix_destroy(partition, secvdb->bpsecEidDictionary, bsl_cb_ed_delete);
	//TODO: Delete RBT?
	 */
}



/******************************************************************************
 * @brief Callback to free EID dictionary reference on deletion
 *
 * @param[in,out] wm        - The shared memory partition
 * @param[in,out] user_data - The EID stored in shared memory.
 *
 * @note
 *  The EID dictionary stores EIDs as a key->value pair where key is a partial
 *  key in a RADIX tree and value is the full EID. When a node in the dictionary
 *  is removed, the key is cleaned up automatically and this function is called
 *  to delete the value (full EID).
 *****************************************************************************/

void bsl_cb_ed_delete(PsmPartition partition, PsmAddress user_data)
{
	CHKVOID(partition);
	CHKVOID(user_data);
	psm_free(partition, user_data);
}




/*
 * +--------------------------------------------------------------------------+
 * |	      	     OPTIONAL PROCESSING ACTION UTILITIES  	     			  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |	      	     OPTIONAL PROCESSING ACTION CALLBACKS  	     			  +
 * +--------------------------------------------------------------------------+
 */

/******************************************************************************
 *
 * \par Function Name: bsl_remove_sop_at_sender
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

	if (sopBlk == NULL)
	{
		writeMemo("[i] Cannot remove security block. No security block provided.");
		return;
	}

	/* Step 1: Find security block representing the security operation */
	Object sop = getExtensionBlock(bundle, sopBlk->number);

	if (sop)
	{
		/* Step 2: If found, remove the security block from the bundle */
		deleteExtensionBlock(sop, &(bundle->extensionsLength));
		return;
	}
	else
	{
		writeMemo("[i] Cannot remove security block. No security block found.");
	}
}

/******************************************************************************
 *
 * \par Function Name: bsl_remove_sop_at_receiver
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

	if (sopElt == NULL)
	{
		writeMemo("[i] Cannot remove security block. No security block provided.");
		return;
	}

	AcqExtBlock *secBlk = (AcqExtBlock *) lyst_data(sopElt);

	/* Step 1: Find security block representing the security operation */
	LystElt	blkElt = getAcqExtensionBlock(wk, secBlk->number);

	if(blkElt != NULL)
	{
		/* Step 2: If found, remove the security block from the bundle */
		deleteAcqExtBlock(blkElt);
	}
	else
	{
		writeMemo("[i] Cannot remove security block. No security block found.");
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name: bsl_remove_sop_target_at_sender
 *
 * \par Purpose: This function removes the security target of the security
 *               block provided.
 *
 * \param[in]  bundle       Current, working bundle.
 * \param[in]  sopBlk       Security block representing the security operation.
 * \param[in]  asb          Abstract Security Block (outbound).
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
		BpsecOutboundASB *asb, unsigned char tgtNum)
{
	/* Step 0: Sanity checks. */
	CHKVOID(bundle);
	CHKVOID(tgtNum);

	if (sopBlk == NULL)
	{
		writeMemo("[i] Cannot remove security target. No security block provided.");
		return;
	}
	if (asb == NULL)
	{
		writeMemo("[i] Cannot remove security target. Abstract security "
				"block not provided.");
		return;
	}

	Sdr sdr = getIonsdr();

	/* Step 1: Search for the security target block */
	Object tgt = bspsec_util_findOutboundBpsecTargetBlock(bundle, tgtNum, sopBlk->type);

	if (tgt)
	{
		/* Step 2: Remove the target block from the security block */
		sdr_list_delete(sdr, tgt, NULL, NULL);

		/* Step 3: Remove the target block from the bundle */

		/*
		 * Step 3.1: If target is the primary or payload block, we must
		 * abandon the bundle (cannot exist without one of these blocks).
		 */
		if (tgtNum == PrimaryBlk || tgtNum == PayloadBlk)
		{
			bundle->corrupt = 1;
			return;
		}

		/* Step 3.2: If target is an extension block, remove from the bundle */
		else
		{
			deleteExtensionBlock(tgt, &(bundle->extensionsLength));
			return;
		}
	}
	else
	{
		writeMemo("[i] Cannot remove security target. No security target found.");
	}
}

/******************************************************************************
 *
 * \par Function Name: bsl_remove_sop_target_at_receiver
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
	if (sopElt == NULL)
	{
		writeMemo("[i] Cannot remove security target. No security block provided.");
		return;
	}
	if (tgtElt == NULL)
	{
		writeMemo("[i] Cannot remove security target. Security target not provided.");
		return;
	}

	/* Step 1: Discard target block */
	bpsec_asb_inboundTargetResultRemove(tgtElt, sopElt);

	return;
}

/******************************************************************************
 *
 * \par Function Name: bsl_remove_all_target_sops_at_sender
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

	Sdr	               sdr = getIonsdr();
	Object	           sopElt;
	Object	           sopAddr;
	Object             tgtElt;
	unsigned char      sopTgtNum;
	BpsecOutboundASB   asb;
	OBJ_POINTER(ExtensionBlock, sopBlk);

	/* Find all security blocks in the bundle */
	for (sopElt = sdr_list_first(sdr, bundle->extensions); sopElt;
			sopElt = sdr_list_next(sdr, sopElt))
	{
		sopAddr = sdr_list_data(sdr, sopElt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, sopBlk, sopAddr);

		/* When a security block is found */
		if (sopBlk->type == BlockIntegrityBlk || (sopBlk->type == BlockConfidentialityBlk))
		{
			sdr_read(sdr, (char *) &asb, sopBlk->object, sizeof(BpsecOutboundASB));

			/* If that security block has a target whose block number is the same
			 * as the input target number, the security operation must be removed.
			 * This function removes up to two security operations: bib-integrity and
			 * bcb-confidentiality.
			 * TODO: Remove bcb-confidentiality before bib-integrity. */
			for (tgtElt = sdr_list_first(sdr, asb.scResults); tgtElt;
					tgtElt = sdr_list_next(sdr, tgtElt))
			{
				sopTgtNum = (unsigned char) sdr_list_data(sdr, tgtElt);
				if (sopTgtNum == tgtNum)
				{
					bsl_remove_sop_at_sender(bundle, sopBlk);
					return;
				}
			}
		}
	}
}

/******************************************************************************
 *
 * \par Function Name: bsl_remove_all_target_sops_at_receiver
 *
 * \par Purpose: This function removes all security operations targeting a
 *               specified block.
 *
 * \param[in]  wk         Work area holding bundle information.
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
void bsl_remove_all_target_sops_at_receiver(AcqWorkArea *wk, unsigned char tgtNum)
{
	/* Step 0: Sanity checks. */
	CHKVOID(wk);
	CHKVOID(tgtNum);

	AcqExtBlock	       *extBlock;
	LystElt            tgtElt;
	LystElt            sopElt;
	BpsecInboundTargetResult *sopTgt;

	/* Find all security blocks in the bundle */
	for (sopElt = lyst_first(wk->extBlocks); sopElt; sopElt = lyst_next(sopElt))
	{
		extBlock = (AcqExtBlock *) lyst_data(sopElt);

		/* When a security block is found */
		if ((extBlock->type == BlockIntegrityBlk) || (extBlock->type == BlockConfidentialityBlk))
		{
			BpsecInboundASB *asb = (BpsecInboundASB *) (extBlock->object);

			for (tgtElt = lyst_first(asb->scResults); tgtElt; tgtElt = lyst_next(tgtElt))
			{
				sopTgt = (BpsecInboundTargetResult *) lyst_data(tgtElt);
				if (sopTgt->scTargetId == tgtNum)
				{
					/* If that security block has a target whose block number is the same
					 * as the input target number, the security operation must be removed.
					 * This function removes up to two security operations: bib-integrity and
					 * bcb-confidentiality.
					 * TODO: Remove bcb-confidentiality before bib-integrity. */
					bsl_remove_sop_at_receiver(wk, sopElt);
					return;
				}
			}
		}
	}
}

/******************************************************************************
 *
 * \par Function Name: bsl_do_not_forward_at_sender
 *
 * \par Purpose: This function results in the end of bundle transmission at
 *               the current node by marking the current bundle as corrupt,
 *               which will result in the bundle being abandoned.
 *
 * \param[in]  bundle     Current, working bundle.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 * 02/03/21   S. Heiner      Initial Implementation
 *****************************************************************************/
void bsl_do_not_forward_at_sender(Bundle *bundle)
{
	CHKVOID(bundle);

	/* Abandon bundle */
	bundle->corrupt = 1;
}

/******************************************************************************
 *
 * \par Function Name: bsl_do_not_forward_at_receiver
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
 * \par Function Name: bsl_request_storage
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
void bsl_request_storage(Bundle *bundle)
{
	BPSEC_DEBUG_WARN("Function not implemented.", NULL);
	CHKVOID(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bsl_report_reason_code_at_sender
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
 * \par Function Name: bsl_report_reason_code_at_receiver
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
 * \par Function Name: bsl_override_target_bpcf
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
void bsl_override_target_bpcf(Bundle *bundle)
{
	BPSEC_DEBUG_WARN("Function not implemented.", NULL);
	CHKVOID(bundle);
}

/******************************************************************************
 *
 * \par Function Name: bsl_override_sop_bpcf
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
void bsl_override_sop_bpcf(Bundle *bundle)
{
	BPSEC_DEBUG_WARN("Function not implemented.", NULL);

	CHKVOID(bundle);
}

/*
 * +--------------------------------------------------------------------------+
 * |	      	     SECURITY OPERATION EVENT HANDLING  	     			  +
 * +--------------------------------------------------------------------------+
 */

/******************************************************************************
 *
 * \par Function Name: bsl_handle_sender_sop_event
 *
 * \par Purpose: This function is to be called each time a security operation
 *               event occurrence is identified by a BPA taking on the role
 *               of Security Source. Bundle characteristics and identifying
 *               information from the security operation itself are used to
 *               determine the security policy rule which is applicable.
 *               If such a security policy rule is found, the function then
 *               determines if any optional processing events have been
 *               configured for the current security operation event and
 *               executes them.
 *
 *TODO note return values
 *
 * \param[in]  bundle        Current, working bundle.
 * \param[in]  sopEvent      The security operation event to respond to.
 * \param[in]  sop           Security block representing the security operation.
 * \param[in]  asb           Abstract security block for the security operation.
 * \param[in]  tgtNum        Block number of the security operation's target.
 *
 *****************************************************************************/
int bsl_handle_sender_sop_event(Bundle *bundle, BpSecEventId sopEvent,
		ExtensionBlock *sop, BpsecOutboundASB *asb, unsigned char tgtNum)
{
	/* Step 0: Sanity checks */
	CHKERR(bundle);
	CHKERR(sopEvent >= 0);
	CHKERR(tgtNum >=0);

	BpSecPolRuleSearchTag tag;
	memset(&tag,0,sizeof(tag));
	PsmPartition wm = getIonwm();

	/* Step 1: Populate the policy rule search tag */
	if (tgtNum == PrimaryBlk)
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

	if (asb != NULL)
	{
		tag.scid = asb->scId;
		readEid(&asb->scSource, &(tag.ssrc));
		if (tag.ssrc != NULL)
		{
			tag.ssrc_len = strlen(tag.ssrc);
		}
	}

	readEid(&bundle->id.source, &(tag.bsrc));
	if (tag.bsrc != NULL)
	{
		tag.bsrc_len = strlen(tag.bsrc);
	}

	readEid(&bundle->destination, &(tag.bdest));
	if (tag.bdest != NULL)
	{
		tag.bdest_len = strlen(tag.bdest);
	}

	/* Step 2: Retrieve the rule for the current security operation */
	BpSecPolRule *rule = bslpol_rule_get_best_match(wm, tag);

	if (tag.ssrc_len > 0)
	{
		MRELEASE(tag.ssrc);
	}
	if (tag.bsrc_len > 0)
	{
		MRELEASE(tag.bsrc);
	}
	if (tag.bdest_len > 0)
	{
		MRELEASE(tag.bdest);
	}

	/* Step 3: Apply the rule to the bundle */
	if (rule != NULL)
	{
		/* Step 2.1: Retrieve the event set for the rule */
		PsmAddress esAddr = rule->eventSet;
		BpSecEventSet *esPtr = (BpSecEventSet *) psp(wm, esAddr);

		/* Step 2.2: Check if policy is configured for the current SOP event */
		if(esPtr != NULL)
		{
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
						/* Step 2.4: Execute the optional processing action(s) */
						if (curEventPtr->action_mask & BSLACT_REMOVE_SOP)
						{
							bsl_remove_sop_at_sender(bundle, sop);
						}

						if (curEventPtr->action_mask & BSLACT_REMOVE_SOP_TARGET)
						{
							bsl_remove_sop_target_at_sender(bundle, sop, asb, tgtNum);
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
 * \par Function Name: bsl_handle_receiver_sop_event
 *
 * \par Purpose: This function is to be called each time a security operation
 *               event occurrence is identified by a BPA taking on the role
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
 * \param[in]  tgtNum    The block number of the security target block.
 *
 * \todo Can this function, and bsl_handle_sender_sop_event be consolidated?
 * \todo To improve performance, can the security policy rule be passed to this
 *       function rather than the lookup performed with bslpol_rule_get_best_match?
 *       Why is bslpol_rule_get_best_match not used in the bpsec processing code
 *       for functions like bpsec_verify or bpsec_encrypt?
 *****************************************************************************/
int bsl_handle_receiver_sop_event(AcqWorkArea *wk, int role,
		BpSecEventId sopEvent, LystElt sop, LystElt tgt, unsigned char tgtNum)
{
	/* Step 0: Sanity checks */
	CHKERR(wk);
	CHKERR((role == BPRF_VER_ROLE) || (role == BPRF_ACC_ROLE));
	CHKERR(sopEvent >= 0);

	Bundle *bundle = &(wk->bundle);
	PsmPartition wm = getIonwm();

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
		LystElt	elt = getAcqExtensionBlock(wk, tgtNum);

		if(elt != NULL)
		{
			AcqExtBlock	*blk = lyst_data(elt);
			tag.type = blk->type;
		}
		else
		{
			BPSEC_DEBUG_INFO("Target %d does not exist in bundle.", tgtNum);
			return ERROR;
		}
	}

	if (sop != NULL)
	{
		AcqExtBlock *secBlk = (AcqExtBlock *) lyst_data(sop);
		BpsecInboundASB *asb = (BpsecInboundASB *) (secBlk->object);

		if (asb != NULL)
		{
			tag.scid = asb->scId;
			readEid(&asb->scSource, &(tag.ssrc));
			if (tag.ssrc != NULL)
			{
				tag.ssrc_len = strlen(tag.ssrc);
			}
		}
	}

	readEid(&bundle->id.source, &(tag.bsrc));
	if (tag.bsrc != NULL)
	{
		tag.bsrc_len = strlen(tag.bsrc);
	}

	readEid(&bundle->destination, &(tag.bdest));
	if (tag.bdest != NULL)
	{
		tag.bdest_len = strlen(tag.bdest);
	}

	/* Step 2: Retrieve the rule for the current security operation */
	BpSecPolRule *rule = bslpol_rule_get_best_match(wm, tag);

	if (tag.ssrc_len > 0)
	{
		MRELEASE(tag.ssrc);
	}
	if (tag.bsrc_len > 0)
	{
		MRELEASE(tag.bsrc);
	}
	if (tag.bdest_len > 0)
	{
		MRELEASE(tag.bdest);
	}

	/* Step 3: Apply the rule to the bundle */
	if (rule != NULL)
	{
		/* Step 2.1: Retrieve the event set for the rule */
		PsmAddress esAddr = rule->eventSet;
		BpSecEventSet *esPtr = (BpSecEventSet *) psp(wm, esAddr);

		/* Step 2.2: Check if policy is configured for the current SOP event */
		if(esPtr != NULL)
		{
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
							bsl_remove_all_target_sops_at_receiver(wk, tgtNum);
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
