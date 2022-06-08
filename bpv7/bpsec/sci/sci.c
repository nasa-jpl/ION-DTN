/******************************************************************************
#include <bib_hmac_sha2_sc.h>
#include <bpsec/sci/sci_valmap.h>
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/


/*****************************************************************************
 **
 ** File Name: sci.c
 **
 ** Namespace: bpsec_sci_
 **
 ** Description:
 **
 **     The BPSec security protocol used to secure BPv7 bundles defines
 **     the concept of a "security context" used to generate and consume
 **     cryptographic material associated with BPSec security blocks.
 **
 **     The ION Security Context Interface (SCI) provides methods for finding
 **     and using security contexts to perform security block processing.
 **
 ** Notes:
 **
 **     1. This interface does not, itself, implement cryptographic functions.
 **
 **     2. The security context "state" represents the state of a security
 **        context in the process of applying a security service at a node (such
 **        as when encrypting a target block). This state is used under the
 **        auspices of the single thread within the BPA tasked with the
 **        encryption. As such, this state is stored in regular heap memory and
 **        not in shared memory.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/03/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#include "sci.h"
#include "sc_value.h"
#include "bpsec_util.h"



/*****************************************************************************
 *                         Security Context Includes                         *
 *****************************************************************************/

#include "ion_test_sc.h"
#include "bib_hmac_sha2_sc.h"
#include "bcb_aes_gcm_sc.h"


/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/


static sc_Def gScDefs[] =
{
   { BPSEC_ITSC_SC_NAME,             /** Name                      **/
     BPSEC_ITSC_SC_ID,               /** Context Id                **/
     SC_SVC_BIBINT | SC_SVC_BCBCONF, /** Services Offered.         **/

     NULL,                           /** bpsec_sc_init             **/
     bpsec_itsci_initAsbFn,          /** bpsec_sc_initOutboundASB  **/
     NULL,                           /** bpsec_sc_teardown         **/

     bpsec_sci_stateInit,            /** bpsec_sc_stateInit        **/
     bpsec_sci_stateIncr,            /** bpsec_sc_stateIncr        **/
     bpsec_sci_stateClear,           /** bpsec_sc_stateClear       **/

     bpsec_itsci_procOutBlk,         /** bpsec_sc_procOutboundBlk  **/
     bpsec_itsci_procInBlk,          /** bpsec_sc_procInboundBlk   **/
	 bpsec_itsci_valMapGet           /** bpsec_sc_valMapGet        **/
   },

   {
    "BIB_HMAC_SHA2",                 /** Name                      **/
    BPSEC_BIB_HMAC_SHA2_SC_ID,       /** Context Id                **/
     SC_SVC_BIBINT,                  /** Services Offered.         **/

     NULL,                           /** bpsec_sc_init             **/
	 bpsec_sci_initAsbFn,            /** bpsec_sc_initOutboundASB  **/
     NULL,                           /** bpsec_sc_teardown         **/

     bpsec_sci_stateInit,            /** bpsec_sc_stateInit        **/
     bpsec_sci_stateIncr,            /** bpsec_sc_stateIncr        **/
     bpsec_sci_stateClear,           /** bpsec_sc_stateClear       **/

     bpsec_bhssci_procOutBlk,        /** bpsec_sc_procOutboundBlk  **/
     bpsec_bhssci_procInBlk,         /** bpsec_sc_procInboundBlk   **/
	 bpsec_bhssci_valMapGet          /** bpsec_sc_valMapGet        **/
   },

   {
     "BCB_AES_GCM",                  /** Name                      **/
     BPSEC_BCB_AES_GCM_SC_ID,        /** Context Id                **/
	 SC_SVC_BCBCONF,                 /** Services Offered.         **/

     NULL,                           /** bpsec_sc_init             **/
	 bpsec_sci_initAsbFn,            /** bpsec_sc_initOutboundASB  **/
     NULL,                           /** bpsec_sc_teardown         **/

     bpsec_sci_stateInit,            /** bpsec_sc_stateInit        **/
     bpsec_sci_stateIncr,            /** bpsec_sc_stateIncr        **/
     bpsec_sci_stateClear,           /** bpsec_sc_stateClear       **/

     bpsec_bagsci_procOutBlk,        /** bpsec_sc_procOutboundBlk  **/
     bpsec_bagsci_procInBlk,         /** bpsec_sc_procInboundBlk   **/
	 bpsec_bagsci_valMapGet          /** bpsec_sc_valMapGet        **/
   }
};

static int gNumScDefs = sizeof(gScDefs) / sizeof(sc_Def);

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/



/******************************************************************************
 * @brief Return the number of security contexts registered in the system.
 *
 * @note
 * We inspect the structure holding security contexts. This was taken from the
 * pattern used for extension block callbacks in bpv7/library/ext.
 *
 * @retval - # structures registered
 *****************************************************************************/

int bpsec_sci_defCount()
{
    return gNumScDefs;
}



/******************************************************************************
 * @brief Find the security context definition for a given SC Identifier
 *
 * @param[in]  sc_id - The SC whose context definition is being queried.
 * @param[out] def   - The definition for this SC.
 *
 * @note
 * - If the def passed in is NULL, this function just returns whether the SC ID
 *   is known.
 *
 * @retval 1 - The BPA implements the requested security context, and the def
 *             pointer, if provided, points to that definition.
 * @retval 0 - The BPA does not implement the requested security context
 *****************************************************************************/

int bpsec_sci_defFind(int sci_id, sc_Def *def)
{
    int i = 0;

    for (i = 0; i < gNumScDefs; i++)
    {
        if (gScDefs[i].scId == sci_id)
        {
            if(def != NULL)
            {
                *def = gScDefs[i];
            }
            return 1;
        }
    }

    return 0;
}


/******************************************************************************
 * @brief Find the security context ID for a given security context name
 *
 * @param[in] sc_name The security context name to use when searching for the
 *                    corresponding security context ID.
 * @param[out] sc_id  The security context identifier corresponding to the
 *                    provided name IF it is supported by the BPA.
 *
 * @retval 1 - The BPA implements the requested security context, and the sc_id
 *             pointer, if provided, points to that definition.
 * @retval 0 - The BPA does not implement the requested security context and/or
 *             the provided security context name is invalid.
 *****************************************************************************/

int bpsec_sci_idFind(char *sc_name, int *sc_id)
{
    int count = bpsec_sci_defCount();
    int i = 0;

    for (i = 0; i < count; i++)
    {
        /* If a security context is found matching the provided name, return success and
         * populate the SC ID */
        if ((strcmp(gScDefs[i].scName, sc_name)) == 0)
        {
            *sc_id = gScDefs[i].scId;
            return 1;
        }
    }

    /* Return 0 if a security context by the provided name is not found */
    return 0;
}




/******************************************************************************
 * @brief Initialize security contexts on startup
 *
 * @note
 *  1) ION doesn't have a specific, single "startup" function, but this can be
 *     called upon initialization of the BPSec security system.
 *  2) This function stops when a security context fails to initialize.
 *
 * @retval -1 - System Error.
 * @retval  0 - Logical error.
 * @retval  1 - All security contexts initialized successfully.
  *****************************************************************************/

int bpsec_sci_execInit()
{
    int i = 0;
    int result = 1;

    for (i = 0; i < gNumScDefs; i++)
    {
        if(gScDefs[i].scInit)
        {
            if((result = gScDefs[i].scInit()) <= 0)
            {
                BPSEC_DEBUG_ERR("Failed to initialize context %s (id %d).", gScDefs[i].scName, gScDefs[i].scId);
                return result;
            }
        }
    }

    return 1;
}



/******************************************************************************
 * @brief teardown security contexts on shutdown
 *
 * @note
 * 1) ION doesn't have a specific, single "teardown" function, but this can be
 *    called upon termination of the BPSec security system.
 ******************************************************************************/

void bpsec_sci_execTeardown()
{
    int i = 0;

    for (i = 0; i < gNumScDefs; i++)
    {
        if(gScDefs[i].scTeardown)
        {
            gScDefs[i].scTeardown();
        }
    }
}



/******************************************************************************
 * @brief Creates a policy parameter from string input and adds it to a list.
 *
 * @param[in]     wm     The ION shared memory partition.
 * @param[in/out] parms  The list of policy parameters holding the new parm.
 * @param[in]     def    The security context that can interpret/create the
 *                       parameter.
 * @param[in]     key    The string holding the parameter identifier.
 * @param[in]     value  The string holding the data value of the parameter.
 *
 * @note
 *  - It is assumed that the parameter list has been created by the calling
 *    function.
 *
 * @retval 0  - The parameter was added to the list.
 * @retval !0 - The parameter was not added to the list. Something went wrong.
 *****************************************************************************/

int bpsec_sci_polParmAdd(PsmPartition wm, PsmAddress parms, sc_Def *def, char *key, char *value)
{
    PsmAddress addr = 0;
    sc_value_map *map = NULL;
    int idx = 0;

    /* Step 0: Sanity Check. */
    CHKERR(wm);
    CHKERR(parms);
    CHKERR(def);

    /*
     * Step 1: Grab the value map for this SC and check that the requested key
     *         exists and is of the right type.
     */
    map = def->scValMapGet();
    idx = bpsec_scvm_byNameIdxFind(map, key);


    /*
     * Step 2: If the index was not found, or was found by was not a parameter
     *         then we cannot add the policy parameter.
     */
    if((idx < 0) ||
       (map[idx].scValType != SC_VAL_TYPE_PARM))
    {
        BPSEC_DEBUG_ERR("Parameter %s unrecognized by context %s (id %d).",
                        (key == NULL) ? "null" : key,
                        def->scName, def->scId);

        return -1;
    }


    /* Step 3: Allocate space for the parameter in shared memory. */
    if((addr = bpsec_scv_smCreate(wm, SC_VAL_TYPE_PARM)) != 0)
    {
        /*
         * Step 3.1: Use the security context to decode the strings into an
         *           appropriately-structured parameter.
         */
        sc_value *val = psp(wm, addr);
        val->scValId = map[idx].scValId;
        val->scValType = map[idx].scValType;

        if(map[idx].scValFromStr(wm, val, strlen(value), value) != 0)
        {
            BPSEC_DEBUG_ERR("Value %s could not be decoded by context %s (id %d).",
                                    (value == NULL) ? "null" : value,
                                    def->scName, def->scId);

            bpsec_scv_clear(wm, val);
            psm_free(wm, addr);

            return -1;
        }

        /* Step 1.2: Add the parameter to this list. */
        sm_list_insert_last(wm, parms, addr);
        return 0;
    }

    /* Step 2: If we were not able to add the parm, return error. */

    return -1;
}



/******************************************************************************
 * @brief Generate a string representation of a series of policy parameters
 *
 * @param[in]  wm     The memory partition.
 * @param[in]  sc_def The security context definition interpreting the parms.
 * @param[in]  parms  The policy parms list being printed.
 *
 * @todo
 *  1. More error checking.
 *
 * @note
 *  1. The resultant string is a comma-separated list of parameters
 *  2. The resultant string must be released by the caller.
 *
 * @retval !NULL - The string representation of the parameters.
 * @retval NULL  - Error or no parameters.
 *****************************************************************************/

char* bpsec_sci_polParmPrint(PsmPartition wm, sc_Def *sc_def, PsmAddress pol_parms)
{
    char **tmp_array;
    int num_items = 0;
    int i = 0;
    int size = 0;
    PsmAddress eltAddr = 0;
    sc_value_map *map = NULL;
    int idx = 0;


    CHKNULL(wm);
    CHKNULL(sc_def);
    CHKNULL(pol_parms);

    /*
     * Step 1: Grab the value map for this SC and check that the requested key
     *         exists and is of the right type.
     */
    map = sc_def->scValMapGet();
    num_items = sm_list_length(wm, pol_parms);

    if((tmp_array = (char **) MTAKE(num_items * sizeof(char*))) == NULL)
    {
        return NULL;
    }

    /* Step 1: Walk through the list... */
    for(eltAddr = sm_list_first(wm, pol_parms); eltAddr; eltAddr = sm_list_next(wm, eltAddr))
    {
        sc_value *val = psp(wm, sm_list_data(wm, eltAddr));

        if((idx = bpsec_scvm_byIdIdxFind(map, val->scValId, val->scValType)) >= 0)
        {
            tmp_array[i++] = map[idx].scValToStr(wm, val);
            size += strlen(tmp_array[i++]);
        }
        else
        {
            BPSEC_DEBUG_ERR("Cannot convert value of type %d to string.", val->scValId);
        }
    }

    return bpsec_scutl_strFromStrsCreate(tmp_array, size, num_items);
}


/******************************************************************************
 * @brief Deconflict multiple definitions of parameters when generating a state
 *
 * @param[out] state      The state object holding the deconflicted parms.
 * @param[in]  def        The security context used to build parameters from raw
 *                        data (such as TLVs)
 * @param[in]  wm         The memory partition holding policy parameters.
 * @param[in]  pol_parms  Shared memory List of parameters defined by local node policy.
 * @param[in]  blk_parms  Lyst of parameters defined in a received security block.
 *
 * @todo
 *  1. Error Checking
 *
 * @note
 *  1. It is assumed that the state has been allocated, but that the parameter
 *     structures have not been populated.
 *
 *  2. This default behavior is to prioritize information in a security block
 *     over information stored in a policy parameter. This default merge
 *     function will not allow duplicate IDs between local node and blk parms.
 *     It will allow duplicate IDs within a set of node parms or a set of
 *     blk parms.
 *
 *  3. Policy parameters are SHALLOW copied whereas non-policy parameters are
 *     deep copied.
 *
 * @retval 0  - Success
 * @retval <0 - Error.
 *****************************************************************************/

int bpsec_sci_parmFilter(sc_state *state, sc_Def *def, PsmPartition wm, PsmAddress pol_parms, Lyst blk_parms)
{
    LystElt elt = NULL;
    PsmAddress eltAddr = 0;
    int found = 0;
    sc_value *val = NULL;
    sc_value *polval = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC",wm,%d,"ADDR_FIELDSPEC")",
                    (uaddr)state, (uaddr)def, pol_parms, (uaddr)blk_parms);

    /* Step 0: Sanity checks on inputs */
    CHKERR(wm);
    CHKERR(state);
    CHKERR(state->scStParms);
    CHKERR(def);

    /*
     * Step 2: For each block-defined parameter, create an sci_parm for this
     *         state. This involved de-serializing the parm from its CBOR
     *         encoding.
     */

    BPSEC_DEBUG_INFO("There are %d blk parms.", lyst_length(blk_parms));


    for(elt = lyst_first(blk_parms); elt; elt = lyst_next(elt))
    {
        if((val = bpsec_scv_memCopy((sc_value *) lyst_data(elt))) != NULL)
        {
            lyst_insert_last(state->scStParms, val);
        }
    }

    /* Step 3: Copy in any non-redundant policy parameters. */
    if(pol_parms != 0)
    {
        for(eltAddr = sm_list_first(wm, pol_parms); eltAddr; eltAddr = sm_list_next(wm, eltAddr))
        {
            PsmAddress polAddr = sm_list_data(wm, eltAddr);
            found = 0;

            if((polval = psp(wm, polAddr)) == NULL)
            {
                continue;
            }

            for(elt = lyst_first(state->scStParms); elt; elt = lyst_next(elt))
            {
                val = (sc_value*) lyst_data(elt);

                if((val != NULL) && (val->scValLength == polval->scValId))
                {
                    found = 1;
                    break;
                }
            }

            if(found == 0)
            {
                lyst_insert_last(state->scStParms, polval);
            }
        }
    }

    BPSEC_DEBUG_PROC("-->0", NULL);
    return 0;
}




/******************************************************************************
 * @brief Initializes an outbound ASB to be used for adding security operations.
 *
 * @param[in]     def     The SC definition that will initialize this ASB
 * @param[in|out] bundle  The bundle holding the security block.
 * @param[in|out] asb     The ASB being initialized.
 * @param[in]     sdr     The SDR where the bundle is stored.
 * @param[in]     wm      The shared memory area holding SM parms.
 * @param[in]     parms   The address of the parms list in shared memory.
 *
 * Initialize an outbound ASB when creating a new security block at a security
 * source. This function populated a security block with shared information
 * such as the security context ID and the security context parameters that
 * should be kept in the block.
 *
 * @retval 1  - The ASB has been initialized.
 * @retval 0  - There was a logic error initializing the ASB.
 * @retval -1 - System error
 *****************************************************************************/

int   bpsec_sci_initAsbFn(void *def, Bundle *bundle, BpsecOutboundASB *asb, Sdr sdr, PsmPartition wm, PsmAddress parms)
{
	sc_Def *ctx_def = (sc_Def*) def;

	BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC",sdr,wm,%d",
	                 (uaddr)def, (uaddr)bundle, (uaddr)asb, parms);

    /* Step 0: Sanity Checks. */
    CHKERR(def);
    CHKERR(asb);

    /* Step 1 - Allocate SDR space for ASB elements. */
    if((asb->scResults = sdr_list_create(sdr)) == 0)
    {
        BPSEC_DEBUG_ERR("Cannot allocate sdr list.", NULL);
        return -1;
    }

    /*
     * Step 2 - If there are security context parameters, record them with the
     *          outbound ASB.
     */

    if(sm_list_length(wm, parms) > 0)
    {
        if((asb->scParms = bpsec_scv_smListRecord(sdr, 0, wm, parms)) == 0)
        {
            BPSEC_DEBUG_ERR("Cannot record parms list to SDR.", NULL);
            sdr_list_destroy(sdr, asb->scResults, NULL, NULL);
            return 0;
        }
    }

    /* Step 3: Populate portions of the ABS. */
    bpsec_asb_outboundSecuritySourceInsert(bundle, asb);
    asb->scId = ctx_def->scId;

    return 1;
}




/******************************************************************************
 * @brief release resources of a state object
 *
 * @param[out] state The SC state being cleared.
 *
 * Clearing an SC state involves removing any resources used by the state.
 *
 * @note
 *  1. The state object will need to be re-initialized before re-use by the
 *     calling function.
 *****************************************************************************/

void bpsec_sci_stateClear(sc_state *state)
{
    CHKVOID(state);

    /* Destroy the parameters - there is a delete callback on this Lyst. */
  // TODO Do not destroy parms.   lyst_destroy(state->scStParms);

    /* Delete the results list and DO destroy its deep-copy results. */
    lyst_destroy(state->scStResults);

    bpsec_scv_clear(0, &(state->scRawKey));

    memset(state, 0, sizeof(sc_state));
}



/******************************************************************************
 * @brief Prepares a security context state for processing the next security
 *        operation in the security block.
 *
 * @param[in/out] state The SC state being incremented
 *
 * Security blocks contain some shared information (such as parameters) but
 * otherwise encapsulate multiple security operations. When processing a security
 * block, the security context state represents all shared information and is
 * otherwise updated (incremented) as each security operation in the block
 * is completed.
 *
 * Incrementing a state requires updating the current security target from the
 * block's target list (one operation per target) and removing any results
 * specific to the old target.
 *
 * @retval 1  - Success
 * @retval <0 - Error.
 *
 *****************************************************************************/

int  bpsec_sci_stateIncr(sc_state *state)
{
    CHKERR(state);

    state->scStCurTgt++;
    lyst_clear(state->scStResults);

    return 1;
}



/******************************************************************************
 * @brief Initialize a state instance to handle a security service for the
 *        given context.
 *
 * @param[in]  wm          The memory partition where parameters live
 * @param[out] state       The state being initialized.
 * @param[in]  def         The security context used to process parms
 * @param[in]  svc         The security service being performed
 * @param[in]  src         The security source of the service.
 * @param[in]  pol_parms   Security parameters from local node policy.
 * @param[in]  blk_parms   Security parameters from the security block.
 * @param[in]  num_tgts    The number of targets in the security block that
 *                         will be processed by this state.
 *
 * @todo
 *  1. Logging
 *
 * @note
 *  - The security context is passed in as a void * to avoid the circular
 *    reference of a callback inside of a structure taking that structure
 *    as a parameter.
 *
 *
 * @retval 0  - Success
 * @retval <0 - Error
 *****************************************************************************/

int bpsec_sci_stateInit(PsmPartition wm, sc_state *state, unsigned char secBlkNum, void *def, sc_role role, sc_action action,
                      EndpointId src, PsmAddress pol_parms, Lyst blk_parms, int num_tgts)
{
    sc_Def *sc_def = (sc_Def *) def;

    BPSEC_DEBUG_PROC("(wm,"ADDR_FIELDSPEC","ADDR_FIELDSPEC",%d, src, %d,"ADDR_FIELDSPEC",%d)",
                    (uaddr)state, (uaddr)def, action, pol_parms, (uaddr)blk_parms, num_tgts);

    /* Step 0: Sanity checks. */
    CHKERR(wm);
    CHKERR(state);
    CHKERR(sc_def);

    if((state->scStResults = lyst_create()) == NULL)
    {
        BPSEC_DEBUG_ERR("Unable to create lyst.", NULL);
        bpsec_sci_stateClear(state);
        return -1;
    }

    lyst_delete_set(state->scStResults, bpsec_scv_lystCbDel, NULL);


    if((state->scStParms = lyst_create()) == NULL)
    {
        BPSEC_DEBUG_ERR("Unable to create lyst.", NULL);
        bpsec_sci_stateClear(state);
        return -1;
    }

    lyst_delete_set(state->scStParms, bpsec_scv_lystCbDel, NULL);

    memset(&(state->scRawKey), 0, sizeof(sc_value));

    state->scStWm = wm;
    state->scSecBlkNum = secBlkNum;
    state->scStId = sc_def->scId;
    state->scRole = role;
    state->scStAction = action;
    state->scStSize = 0;
    state->scStStatus = SC_STAT_OK;
    state->scStCurTgt = 0;
    state->scStTotTgts = num_tgts;
    state->scStSource = src;
    state->sdr = getIonsdr();

    if(bpsec_sci_parmFilter(state, def, wm, pol_parms, blk_parms) != 0)
    {
        BPSEC_DEBUG_ERR("Unable to filter parameters.", NULL);

        bpsec_sci_stateClear(state);
        return -1;
    }

    return 0;
}




/******************************************************************************
 * @brief Determine if the given security block can hold a new security operation.
 *
 * @param[in] sdr   The SDR holding the security block parameters
 * @param[in] asb   The existing security block
 * @param[in] def   The security context of the new SOP
 * @param[in] wm    The memory partition where SOP parameters live
 * @param[in] parms The SOP parameters.
 *
 * A security block holds multiple security operations. All security operations
 * within a block must share the same security context, security source, and
 * security parameters.
 *
 * To successfully pass the multiplicity check, three things must happen:
 * 1. The security block must be using the same context as the SOP
 * 2. The security block must have the same security source as the SOP
 * 3. The security block must have the same security context parameters as the SOP
 *
 * @note The order of checks is not by importance, but expediency. Doing the fast
 *       checks first increases the likelihood of identifying a mismatch faster.
 *
 * @note This compares parameters from policy. If a security context adds parameters
 *       later as part of processing (such as some RFC9173 contexts that treat the
 *       wrapped key as part of parameters and not results) that will not cause
 *       this function to accidentally say a SOP is not allowed in the security block.
 *
 * @retval 1  - The SOP can be added to this block
 * @retval 0  - The SOP cannot be added to this block
 * @retval -1 - Processing error
 *****************************************************************************/
int  bpsec_sci_multCheck(Sdr sdr, BpsecOutboundASB *asb, sc_Def *def, PsmPartition wm, PsmAddress parms)
{
    Lyst blk_parms = NULL;
    PsmAddress sop_elt;
    LystElt blk_elt;
    sc_value *blk_val = NULL;
    sc_value *sop_val = NULL;
    int matched = 0;

    CHKERR(asb);
    CHKERR(def);


    /*
     * Step 1: Make sure the security contexts are compatible. The security
     *         context used by the block must be the security context that
     *         will be used by the SOP.
     */
    if(asb->scId != def->scId)
    {
        return 0;
    }

    /* Step 2: Make sure the number of parameters are the same.  */
    if(sdr_list_length(sdr, asb->scParms) != sm_list_length(wm, parms))
    {
        return 0;
    }

    /*
     * Step 3: Make sure this security block is created locally. We can only
     *         add SOPs to local security blocks.
     */
    if(bpsec_util_eidIsLocalCheck(asb->scSource) <= 0)
    {
        return 0;
    }

    /*
     * Step 4: Make sure every parameter in the security block is the
     *         same as every parameter in the SOP.
     *
     *         Start by pulling SDR parms into memory.
     *
     *         TODO: This might be slower than reading them in 1 by 1
     *               from SDR...
     */
    blk_parms = bpsec_scv_sdrListRead(sdr, asb->scParms);

    for(blk_elt = lyst_first(blk_parms); blk_elt; blk_elt = lyst_next(blk_elt))
    {
        /* Step 4.1: Grab block param and assume there is no match for it. */
        blk_val = (sc_value *) lyst_data(blk_elt);
        matched = 0;

        for(sop_elt = sm_list_first(wm, parms); sop_elt; sop_elt = sm_list_next(wm, sop_elt))
        {
           sop_val = (sc_value *) psp(wm, sm_list_data(wm, sop_elt));
           if(sop_val->scValId == (blk_val->scValId))
           {
               if((sop_val->scValId == blk_val->scValId) &&
                  (sop_val->scValLength == blk_val->scValLength))
               {
                   if(memcmp(psp(wm,sop_val->scRawValue.asAddr), blk_val->scRawValue.asPtr, sop_val->scValLength) == 0)
                   {
                       /* Step 4.2: If we find a match, note it and move on. */
                       matched = 1;
                       break;
                   }
               }
           }
        }

        /*
         * Step 4.3: If there is no match for the block param, the SOP cannot go into the
         *           security block, because the block parameters and SOP parameters are
         *           different.
         */
        if(matched == 0)
        {
            break;
        }
    }
    lyst_destroy(blk_parms);

    /* Step 5: If all parameters matched, then the SOP can go in the security block. */
    return matched;
}


