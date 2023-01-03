/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
// TODO Update documentation
/*****************************************************************************
 **
 ** File Name: bcb.c
 **
 ** Description: Definitions supporting generic processing of BCB blocks.
 **              This includes both the BCB Interface to the ION bpsec
 **              API as well as a default implementation of the BCB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BCB bpsec Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              bpsec_encrypt
 **              bcbSerialize
 **              bcbRelease
 **              bcbCopy
 **                                                  bpsec_decrypt
 **                                                  bcbAcquire
 **                                                  bcbRecord
 **                                                  bcbClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    ENCRYPT SIDE                     DECRYPT SIDE
 **
 **              bcbDefaultEncrypt
 **
 **                                              bcbDefaultDecrypt
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/05/10  E. Birrane     Implementation as extsbspbcb.c (JHU/APL)
 **            S. Burleigh    Implementation as sbspbcb.c for Sbsp
 **  11/07/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 **  09/02/19  S. Burleigh    Rename everything for bpsec
 **  10/14/20  S. Burleigh    Restructure for target multiplicity
 **  
 *****************************************************************************/

#include "zco.h"
#include "csi.h"
#include "bpsec_util.h"
#include "bcb.h"
#include "bpsec_instr.h"
#include "bpsec_policy.h"
#include "bpsec_policy_rule.h"

#if (BCB_DEBUGGING == 1)
extern char        gMsg[];        /*    Debug message buffer.    */
#endif


/*****************************************************************************
 *                      BCB BLOCK MANAGEMENT FUNCTIONS                       *
 *****************************************************************************/



/*****************************************************************************
 *                      BPSEC ENCRYPTION FUNCTIONS                           *
 *****************************************************************************/

// TODO document function
int    bpsec_encrypt(Bundle *bundle)
{
    Sdr                    sdr = getIonsdr();
    Object                elt;
    Object                blockObj;
    ExtensionBlock        block;
// TODO Figure this out...   size_t                xmitRate = 125000;


    /*    NOTE: need to reinstate a processOnDequeue method
     *    for BCB extension blocks: extracts xmitRate from
     *    DequeueContext and stashes it in the Bundle so that
     *    bpsec_encrypt can retrieve it.  Or else libbpP.c
     *    could pass that value directly to bpsec_encrypt
     *    as an API parameter.                    */

    /**** Apply all applicable security policy rules ****/

    BpSecPolRule *curRule = NULL;

    /* If there is a policy rule for the payload block, apply it */
    if((curRule = bslpol_get_sender_rule(bundle, BlockConfidentialityBlk, PayloadBlk)) != NULL)
    {
        BPSEC_DEBUG_INFO("Found rule id %d.", curRule->user_id);

        if (bslpol_proc_applySenderPolRule(bundle, BlockConfidentialityBlk, curRule, 1) < 0)
        {
            BPSEC_DEBUG_ERR("Failed applying rule %d (payload blk).", curRule->user_id);
            /* Handle sop_misconf_at_src event */
            bsl_handle_sender_sop_event(bundle, sop_misconf_at_src, NULL, NULL, 1);
            BCB_TEST_POINT("sop_misconf_at_src", bundle, PayloadBlk);

            return -1;
        }
        BCB_TEST_POINT("sop_added_at_src", bundle, PayloadBlk);
    }


    /*
     * We don't serialize the BCB, because we might be adding more security results to BIBs
     * as we loop through all of this.
     */
    for (elt = sdr_list_first(sdr, bundle->extensions); elt; elt = sdr_list_next(sdr, elt))
    {
        if((blockObj = sdr_list_data(sdr, elt)) == 0)
        {
            continue;
        }
        sdr_read(sdr, (char*) &block, blockObj, sizeof(ExtensionBlock));

        if((curRule = bslpol_get_sender_rule(bundle, BlockConfidentialityBlk, block.type)) != NULL)
        {
            if (bslpol_proc_applySenderPolRule(bundle, BlockConfidentialityBlk, curRule, block.number) < 0)
            {
                BPSEC_DEBUG_ERR("Failed applying rule %d (ext blk type %d).", curRule->user_id, block.type);
                bsl_handle_sender_sop_event(bundle, sop_misconf_at_src, NULL, NULL, block.number);
                BCB_TEST_POINT("sop_misconf_at_src", bundle, block.type);
                return -1;
            }
            BCB_TEST_POINT("sop_added_at_src", bundle, block.type);
            curRule = NULL;
        }
    }


    /* Now, for each BCB block in the bundle, serialize each BCB and
     * add it to the bundle structure.
     */

    /*    Now attach all new BCBs, signing all targets. */
    if (bpsec_util_attachSecurityBlocks(bundle, BlockConfidentialityBlk, SC_ACT_ENCRYPT) < 0)
    {
        BCB_DEBUG_ERR("Unable to attach all BCB blocks.", NULL);
        return -1;
    }

    return 0;
}



/*****************************************************************************
 *                            BCB DECRYPTION FUNCTIONS                         *
 *****************************************************************************/

// TODO document function
void bcb_handle_rx_error(AcqWorkArea *work, LystElt bcbBlkElt, LystElt tgtBlkElt,
                         AcqExtBlock *tgtBlk, int tgtId, int result, size_t tgtBlkOrigLen)
{

    switch(result)
    {

        /* TODO: add checks for tgtId validity in test point statements */

        case 0:  /* Corrupt target */
        case -1: /* System error handled as corrupt block. */

            work->malformed = 1;

            /* Handle sop_corrupt_at_acceptor event */
            bsl_handle_receiver_sop_event(work, BPRF_ACC_ROLE, sop_corrupt_at_acceptor, bcbBlkElt, tgtBlkElt, tgtId);
            BCB_TEST_POINT("sop_corrupt_at_acceptor", (&(work->bundle)), (tgtBlk) ? tgtBlk->type : -1);
            break;

        case -2: /* Misconfiguration of BCB. */
        default: /* Anything else is treat as a misconfiguration. */
            
            /* Handle sop_misconf_at_acceptor event */
            bsl_handle_receiver_sop_event(work, BPRF_ACC_ROLE, sop_misconf_at_acceptor, bcbBlkElt, tgtBlkElt, tgtId);
            BCB_TEST_POINT("sop_misconf_at_acceptor", (&(work->bundle)), (tgtBlk) ? tgtBlk->type : -1);
            break;
    }

    /* TODO: Make sure the policy actions don't preempt any of the below processing. */

    /* If target block was an extension block and it was discarded during decryption */
    /* TODO: What if the tgtBlk was NULL? Should that ever happen? */
    if ((tgtBlk != NULL) && (tgtBlk->length == 0))
    {
        deleteAcqExtBlock(tgtBlkElt);
        work->bundle.extensionsLength -= tgtBlkOrigLen;
    }

    /* If target block was the payload block and it was discarded during decryption */
    if ((tgtId == PayloadBlk) && (work->bundle.payload.length == 0))
    {
        /* A bundle without a payload is malformed */
        work->malformed = 1;
    }
}

// TODO document function
// TODO we must become default security acceptor if we are bundle destination.
int    bpsec_decrypt(AcqWorkArea *work)
{
    Bundle                *bundle = &(work->bundle);
    LystElt                bcbBlkElt;
    AcqExtBlock            *bcbBlk;
    BpsecInboundASB    *asb;
    BpsecInboundTargetResult    *tgtResult;
    LystElt             tgtResultElt;
    LystElt             tgtBlkElt;
    size_t tgtBlkOrigLen = 0;
    sc_Def def;
    int result = 0;
    int secBlkMisconf = 0;
    char fromEid[MAX_EID_LEN];
    AcqExtBlock *tgtBlk = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC")", (uaddr) work);

    /**** Apply all applicable security policy rules ****/

    BpSecPolRule *polRule = NULL;

    /* For each BCB in the bundle */
    for (bcbBlkElt = lyst_first(work->extBlocks); bcbBlkElt; bcbBlkElt = lyst_next(bcbBlkElt))
    {
        /* Grab the block and see if it is a BCB. */
        bcbBlk = (AcqExtBlock *) lyst_data(bcbBlkElt);
        if ((bcbBlk != NULL) && (bcbBlk->type == BlockConfidentialityBlk))
        {
            char *tmp = NULL;
            asb = (BpsecInboundASB *) (bcbBlk->object);

            readEid(&(asb->scSource), &tmp); /* Wish readEid did not allocate buffer if one were provided...*/
            istrcpy(fromEid, tmp, MAX_EID_LEN);
            MRELEASE(tmp);

            secBlkMisconf = (bpsec_sci_defFind(asb->scId, &def) != 1) ? 1 : 0;

            /* Check each target block for applicable rule */
            for (tgtResultElt = lyst_first(asb->scResults); tgtResultElt; tgtResultElt = lyst_next(tgtResultElt))
            {
                tgtResult = (BpsecInboundTargetResult *) lyst_data(tgtResultElt);
                polRule = bslpol_get_receiver_rule(work, tgtResult->scTargetId, asb->scId);

                if (polRule != NULL)
                {

                    /*
                     * If the security block is corrupted (meaning we cannot process items
                     * due to issues with security context or parameters, then we need to
                     * individually handle events associated with each policy rule.
                     */
                    result = (secBlkMisconf) ?
                             -2 :
                             bslpol_proc_applyReceiverPolRule(work, polRule, SC_ACT_DECRYPT, bcbBlk, asb, tgtResult, &def, &tgtBlkElt, &tgtBlkOrigLen);


                    tgtBlk = (tgtBlkElt != NULL) ? (AcqExtBlock*) lyst_data(tgtBlkElt) : NULL;

                    if (result < 1)
                    {
                        BCB_DEBUG_ERR("Rule %d failed to process block %d. Error: %d", polRule->user_id, tgtResult->scTargetId, result);
                        ADD_BCB_RX_FAIL(fromEid, 1, tgtBlkOrigLen);

                        bcb_handle_rx_error(work, bcbBlkElt, tgtBlkElt, tgtBlk, tgtResult->scTargetId, result, tgtBlkOrigLen);

                        // TODO do we remove block from bundle here?
                        if(result == -1)
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        BCB_DEBUG_INFO("Processing of block %d by rule %d successful.", tgtResult->scTargetId, polRule->user_id);

                        /* Handle sop_processed event */
                        bsl_handle_receiver_sop_event(work, BPRF_ACC_ROLE, sop_processed, bcbBlkElt, tgtResultElt, tgtResult->scTargetId);
                        
                        /* If the block was an extension block. */
                        if (tgtBlk != NULL)
                        {
                            BCB_TEST_POINT("sop_processed", bundle, tgtBlk->type);
                        }
                        /* Else if the block was the payload or primary block, it cannot be represented
                         * as an AcqExtBlock, making the TgtBlk pointer NULL and requiring use of the 
                         * security context target ID as the block identifier. */
                        else if (tgtResult->scTargetId >= 0)
                        {
                            BCB_TEST_POINT("sop_processed", bundle, tgtResult->scTargetId);
                        }
                        
                        if (bpsec_util_destIsLocalCheck(&(work->bundle)))
                        {
                            ADD_BCB_RX_PASS(fromEid, 1, 0);
                        }
                        else
                        {
                            ADD_BCB_FWD(fromEid, 1, 0);
                        }

                        /*
                         * If we accepted the BCB (not just verified) then we must deal with the
                         * fact that the target block was decrypted and also remove the SOP from the
                         * BCB.
                         */
                        if(BPSEC_RULE_ROLE_IDX(polRule) == BPRF_ACC_ROLE)
                        {
                        	/*
                        	 * If target block is an extension block and its length has changed.
                        	 *
                        	 * If the target is the payload, we believe that length is recorded
                        	 * during the decryption.  This only updated the bundle extensionsLength
                        	 * field, which is only affected by changes to the extension block length.
                        	 */
                        	if ((tgtResult->scTargetId != PayloadBlk) &&
                        			(tgtBlk != NULL) &&
									(tgtBlk->length != tgtBlkOrigLen))
                        	{
                        		bundle->extensionsLength -= tgtBlkOrigLen;
                        		bundle->extensionsLength += tgtBlk->length;
                        	}

                            /* Remove this target result from the security block.... */
                            bpsec_asb_inboundTargetResultRemove(tgtResultElt, bcbBlkElt);

                            /*
                             * TODO: At this point, we need to see if the target block was protected by a BIB, such that the
                             *       BIB also needs to be decrypted.
                             */
                        }
                    }

                    polRule = NULL;
                }
            }
        }
    }
    return 0;
}
