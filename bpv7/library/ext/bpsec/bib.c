/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
// TODO: Update description
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
 **                 bpsec_sign
 **                 bibRelease
 **                 bibCopy
 **                                                  bpsec_verify
 **                                                  bibParse
 **                                                  bibRecord
 **                                                  bibClear
 **
 ** Notes:
 **    BIBs are automatically serialized at the time
 **    they are attached to a bundle, and they are not
 **    subject to canonicalization (a BIB cannot be the
 **    target of another BIB).  Nothing to do here.
 **
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
 **  02/05/21  S. Heiner      Initial implementation of bpsec policy:
 **                           identification of security operation events and
 **                           security policy rule handling.
 *****************************************************************************/

#include "zco.h"
#include "csi.h"
#include "bpsec_util.h"
#include "bib.h"
#include "bpsec_instr.h"
#include "bei.h"

#if (BIB_DEBUGGING == 1)
extern char gMsg[]; /*    Debug message buffer.    */
#endif

/*****************************************************************************
 *                       BIB COMPUTATION FUNCTIONS                           *
 *****************************************************************************/


// TODO document function
/*
 * TODO - Consider having apply sender rule return a list of blocks so we don't
 *        need to go looking for them later?
 */
int bpsec_sign(Bundle *bundle)
{
    Sdr sdr = getIonsdr();
    Object elt;
    Object blockObj;
    BpSecPolRule *curRule = NULL;
    ExtensionBlock block;

    /**** Apply all applicable security policy rules ****/

    /* If there is a policy rule for the primary block, apply it */
    if((curRule = bslpol_get_sender_rule(bundle, BlockIntegrityBlk, PrimaryBlk)) != NULL)
    {
        if (bslpol_proc_applySenderPolRule(bundle, BlockIntegrityBlk, curRule, 0) < 0)
        {
            BIB_DEBUG_ERR("Failed applying rule %d (primary blk).", curRule->user_id);

            /* Handle sop_misconf_at_src event */
            bsl_handle_sender_sop_event(bundle, sop_misconf_at_src, NULL, NULL, PrimaryBlk);
            BIB_TEST_POINT("sop_misconf_at_src", bundle, PrimaryBlk);
	    bundle->insecure = 1;
	    return 0;
        }
        BIB_TEST_POINT("sop_added_at_src", bundle, PrimaryBlk);
    }

    /* If there is a policy rule for the payload block, apply it */
    if((curRule = bslpol_get_sender_rule(bundle, BlockIntegrityBlk, PayloadBlk)) != NULL)
    {
        if (bslpol_proc_applySenderPolRule(bundle, BlockIntegrityBlk, curRule, 1) < 0)
        {
            BIB_DEBUG_ERR("Failed applying rule %d (payload blk).", curRule->user_id);
            /* Handle sop_misconf_at_src event */
            bsl_handle_sender_sop_event(bundle, sop_misconf_at_src, NULL, NULL, PayloadBlk);
            BIB_TEST_POINT("sop_misconf_at_src", bundle, PayloadBlk);
	    bundle->insecure = 1;
	    return 0;
        }
        BIB_TEST_POINT("sop_added_at_src", bundle, PayloadBlk);
    }

    /*
     * TODO: Think through this... after this loop we immediate go into
     *       serialize mode. Must be a way to optimize this.
     */
    /* Iterate over extension blocks as potential targets of a bib-integrity
     * security operation */

    /*
     * We don't serialize the BIB, because we might be adding more security results to BIBs
     * as we loop through all of this.
     */
    for (elt = sdr_list_first(sdr, bundle->extensions); elt; elt = sdr_list_next(sdr, elt))
    {
        if((blockObj = sdr_list_data(sdr, elt)) == 0)
        {
            continue;
        }
        sdr_read(sdr, (char*) &block, blockObj, sizeof(ExtensionBlock));

        if((curRule = bslpol_get_sender_rule(bundle, BlockIntegrityBlk, block.type)) != NULL)
        {
            if (bslpol_proc_applySenderPolRule(bundle, BlockIntegrityBlk, curRule, block.number) < 0)
            {
                BIB_DEBUG_ERR("Failed applying rule %d (ext blk type %d).", curRule->user_id, block.type);
                bsl_handle_sender_sop_event(bundle, sop_misconf_at_src, NULL, NULL, block.number);
                BIB_TEST_POINT("sop_misconf_at_src", bundle, block.type);
		bundle->insecure = 1;
		return 0;
            }
            BIB_TEST_POINT("sop_added_at_src", bundle, block.type);
            curRule = NULL;
        }
    }

    /* Now, for each BIB block in the bundle, serialize each BIB and
     * add it to the bundle structure. It is assumed that the security results
     * are calculated up in bslpol_get_sender_rule.
     */

    /*    Now attach all new BIBs, signing all targets. */
    if (bpsec_util_attachSecurityBlocks(bundle, BlockIntegrityBlk, SC_ACT_SIGN) < 0)
    {
        BIB_DEBUG_ERR("Unable to attach all BIB blocks.", NULL);
	bundle->insecure = 1;
    }

    return 0;
}



/*****************************************************************************
 *                      BPSEC VERIFICATION FUNCTIONS                         *
 *****************************************************************************/
// TODO document function
void bib_handle_rx_error(AcqWorkArea *work, Bundle *bundle, BpSecPolRule *polRule,
                      LystElt bibElt, LystElt tgtResultElt, int tgtId, int result,
                      uint8_t tgtBlkType)
{

    if(result == -1)
    {
        if((tgtId != PrimaryBlk) && (tgtId != PayloadBlk))
        {
            bundle->altered = 1;
        }
        else
        {
            bundle->corrupt = 1;
        }

        if (polRule->filter.flags & BPRF_VER_ROLE)
        {
            bsl_handle_receiver_sop_event(work, BPRF_VER_ROLE, sop_corrupt_at_verifier, bibElt, tgtResultElt, tgtId);
            BIB_TEST_POINT("sop_corrupt_at_verifier", bundle, tgtBlkType);
        }
        else
        {
            bsl_handle_receiver_sop_event(work, BPRF_ACC_ROLE, sop_corrupt_at_acceptor, bibElt, tgtResultElt, tgtId);
            BIB_TEST_POINT("sop_corrupt_at_acceptor", bundle, tgtBlkType);
        }
    }

    else if(result == 0)
    {
        if (polRule->filter.flags & BPRF_VER_ROLE)
        {
            bsl_handle_receiver_sop_event(work, BPRF_VER_ROLE, sop_misconf_at_verifier, bibElt, tgtResultElt, tgtId);
            BIB_TEST_POINT("sop_misconf_at_verifier", bundle, tgtBlkType);
        }
        else
        {
            bsl_handle_receiver_sop_event(work, BPRF_ACC_ROLE, sop_misconf_at_acceptor, bibElt, tgtResultElt, tgtId);
            BIB_TEST_POINT("sop_misconf_at_acceptor", bundle, tgtBlkType);
        }

        if(tgtId == PrimaryBlk)
        {
            work->authentic = 0;
        }
        else if(tgtId == PayloadBlk)
        {
            bundle->altered = 1;
        }
    }
}

// TODO document function
int bpsec_verify(AcqWorkArea *work)
{
    Bundle *bundle = &(work->bundle);
    LystElt blkElt;
    LystElt nextBlkElt;
    AcqExtBlock *block;
    BpsecInboundASB *asb;
    LystElt tgtElt;
    LystElt nextTgtElt;
    BpsecInboundTargetResult *target;
    sc_Def def;
    int secBlkMisconf = 0;
    int result = 0;
    char *fromEid = NULL;
    size_t length = 0;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC")", (uaddr) work);

    CHKERR(bundle);

    /****    Apply all applicable security policy rules         ****/

    BpSecPolRule *polRule = NULL;

    /*    First check all BIBS that are present in the bundle.    */
    for (blkElt = lyst_first(work->extBlocks); blkElt; blkElt = nextBlkElt)
    {
	nextBlkElt = lyst_next(blkElt);
        if((block = (AcqExtBlock*) lyst_data(blkElt)) == NULL)
        {
            continue; // Weird...
        }

        if (block->type == BlockIntegrityBlk)
        {
            if((asb = (BpsecInboundASB*) (block->object)) == NULL)
            {
                continue; // Weird...
            }

            readEid(&(asb->scSource), &fromEid);
            secBlkMisconf = (bpsec_sci_defFind(asb->scId, &def) != 1) ? 1 : 0;

            /* Check each target block for applicable rule */
            for (tgtElt = lyst_first(asb->scResults); tgtElt; tgtElt = nextTgtElt)
            {
		nextTgtElt = lyst_next(tgtElt);
                target = (BpsecInboundTargetResult*) lyst_data(tgtElt);

                polRule = bslpol_get_receiver_rule(work, target->scTargetId, asb->scId);

                if (polRule == NULL)	/*	No BIB rule for target.	*/
		{
			/*	SB 07/26/2024				*/
			if (bundle->deliverable == 0)
			{
				/*	Missing rule is not a problem
				 *	at a waypoint node.		*/

				continue;
			}

			/*	SB 05/27/2024				*/
			/*	If BIB target is the primary block,
			 *	then BPSec authentication is required.
			 *	If there is no policy rule for primary
			 *	block verification, then authentication
			 *	by BPSec is not possible.  So the
			 *	bundle is tagged as inauthentic.
			 *
			 *	If BIB target is NOT the primary
			 *	block, then the security source has
			 *	asserted that the integrity of this
			 *	target block must be verified.  If
			 *	there is no policy rule for verifying
			 *	the integrity of this block, then
			 *	that verification is impossible.
			 *	So the bundle is tagged as altered.
			 *
			 *	Note that these determinations may
			 *	need to be revisited: a hostile
			 *	forwarding node might attach a BIB
			 *	for the target of which the security
			 *	acceptor is known not to have any
			 *	rule.  This would cause bundles to
			 *	be discarded incorrectly, a serious
			 *	denial-of-service attack.
			 *
			 *	SB 5/26/2024				*/

			if (target->scTargetId == PrimaryBlk)
			{
				work->authentic = 0;
			}
			else	/*	Not the primary block.		*/
			{
				 bundle->altered = 1;
			}
		}
		else	/*	Applicable policy rule was found.	*/
                {
                    /*
                     * If the security block is corrupted (meaning we cannot process items
                     * due to issues with security context or parameters, then we need to
                     * individually handle events associated with each policy rule.
                     */
                    result = (secBlkMisconf) ?
                               0 :
                               bslpol_proc_applyReceiverPolRule(work, polRule, SC_ACT_VERIFY, block, asb, target, &def, NULL, &length);

                    if(result < 1)
                    {
                        ADD_BIB_RX_FAIL(fromEid, 1, length);
                        BIB_DEBUG_WARN("Failed receipt rule for %d", polRule->user_id);
                        
                        bib_handle_rx_error(work, bundle, polRule, blkElt, tgtElt, target->scTargetId, result, polRule->filter.blk_type);
                        if(result == -1)
                        {
                            MRELEASE(fromEid);
			    bundle->insecure = 1;
                            return 0;
                        }
                    }

                    /* Otherwise, the target signature was verified. */
                    else
                    {
                        if ((work->authentic == -1) && (target->scTargetId == PrimaryBlk))
                        {
                            work->authentic = 1;
                        }
                        if (bpsec_util_destIsLocalCheck(bundle))
                        {
                            ADD_BIB_RX_PASS(fromEid, 1, length);

                            /* Handle sop_processed event */
                            bsl_handle_receiver_sop_event(work, BPRF_ACC_ROLE, sop_processed, blkElt, tgtElt, target->scTargetId);
                            BIB_TEST_POINT("sop_processed", bundle, polRule->filter.blk_type);
                            bpsec_asb_inboundTargetResultRemove(tgtElt, blkElt);
                        }
                        else
                        {
                            ADD_BIB_FWD(fromEid, 1, length);
                            /* Handle sop_verified event */
                            bsl_handle_receiver_sop_event(work, BPRF_VER_ROLE, sop_verified, blkElt, tgtElt, target->scTargetId);
                            BIB_TEST_POINT("sop_verified", bundle, polRule->filter.blk_type);
                        }
                    }

                    polRule = NULL;
                }
            }

	    MRELEASE(fromEid);		/*	SB 12/20/2023		*/
        }
    }

    /*    Then check for BIBs that are required but not present.    */
    // TODO - This needs to be implemented.
    // TODO - This probably also means being able to query by wildcard SCID.
    // TODO - Maybe we keep a list of processed rule IDs

    /*	Note: if no BIB targeting the primary block is present in
     *	the bundle (one of the potential required-but-not-present
     *	BIBs alluded to above), then it is assumed that BPSec
     *	authentication is not required; the bundle is assumed to be
     *	authentic unless it had been tagged as inauthentic by some
     *	other means.  Removal of such a BIB (or BIB target) by a
     *	hostile forwarding node would constitute a security attack.
     *
     *	SB 5/26/2024							*/

    bundle->clDossier.authentic = (work->authentic == 0 ? 0 : 1);
    BPSEC_DEBUG_PROC("Returning 0", NULL);
    return 0;
}
