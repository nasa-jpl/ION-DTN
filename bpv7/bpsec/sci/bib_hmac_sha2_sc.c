/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2022 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bib_hmac_sha2.h
 **
 ** Namespace:
 **    bpsec_bhssci_   SCI INterface functions
 **    bpsec_bhsscutl  General utilities
 **
 ** Description:
 **
 **     This file implements The BIB-HMAC-SHA2 security context standardized
 **     by RFC9173.
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  03/07/22  E. Birrane     Initial implementation
 *****************************************************************************/

#include "bib_hmac_sha2_sc.h"

#include "bpsec_util.h"
#include "sc_value.h"
#include "cbor.h"
#include "rfc9173_utils.h"


static sc_value_map gBHS_ParmMap[] = {
    {"key_name",    BPSEC_BHSSC_PARM_LTK_NAME,    SC_VAL_TYPE_PARM,   bpsec_scvm_strStrDecode, bpsec_scv_clear, bpsec_scvm_strStrEncode, NULL, NULL},
    {"sha_variant", BPSEC_BHSSC_PARM_SHA_VAR_ID,  SC_VAL_TYPE_PARM,   bpsec_scvm_intStrDecode, bpsec_scv_clear, bpsec_scvm_intStrEncode, bpsec_scvm_intCborEncode, bpsec_scvm_intCborDecode},
    {"wrapped_key", BPSEC_BHSSC_PARM_WRAPPED_KEY, SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {"scope_flags", BPSEC_BHSSC_PARM_SCOPE_FLAGS, SC_VAL_TYPE_PARM,   bpsec_scvm_intStrDecode, bpsec_scv_clear, bpsec_scvm_intStrEncode, bpsec_scvm_intCborEncode, bpsec_scvm_intCborDecode},
    {"ehmac",       BPSEC_BHSSC_RESULT_EHMAC,     SC_VAL_TYPE_RESULT, bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {NULL,-1, SC_VAL_TYPE_UNKNOWN, NULL, NULL, NULL, NULL, NULL}
};



/******************************************************************************
 * @brief Process a security operation in an incoming security block.
 *
 * @param[in]  state     - The state of the security block processing
 * @param[in]  wk        - The acquisition work area
 * @param[in]  asb       - The deserialized abstract security block
 * @param[in]  tgtBlkElt - The ELT of the target block in the wk area block list
 * @param[out] tgtResult - Security results generated for this target.
 *
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/

int bpsec_bhssci_procInBlk(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb,
		                  LystElt tgtBlkElt, BpsecInboundTargetResult *tgtResult)
{
    int result = 0;
    sc_value *assertedDigest = NULL;
    csi_val_t key_value;
    uint8_t *csi_ctx = NULL;
    csi_val_t csi_digest;
    BpsecSerializeData ippt_preamble;
    Object ipptZco = 0;
    int ipptZcoLen = 0;
    int addData = 0;
    uint16_t sha_variant = 0;

    /* Step 0 - Sanity Checks. */
    CHKERR(state);
    CHKERR(state->scStAction == SC_ACT_VERIFY);


    /* Step 1: Get and validate the SHA Variant to use to verify. */
    sha_variant = bpsec_rfc9173utl_intParmGet(state, BPSEC_BHSSC_PARM_SHA_VAR_ID, BPSEC_BHSSC_SV_DEFAULT);

    switch(sha_variant)
    {
        case CSTYPE_HMAC_SHA256:
        case CSTYPE_HMAC_SHA384:
        case CSTYPE_HMAC_SHA512:

            break;
        default:
            BPSEC_DEBUG_WARN("Incorrect value for parm %s, defaulting to %d.",  bpsec_scvm_byIdNameFind(gBHS_ParmMap, BPSEC_BHSSC_PARM_SHA_VAR_ID, SC_VAL_TYPE_PARM), BPSEC_BHSSC_SV_DEFAULT);
            sha_variant = BPSEC_BHSSC_SV_DEFAULT;
            break;
    }


    /*
     * Step 2: Get the verifying key. If we are NOT the first security operation in
     *         this security block, then the state will have the key used
     *         for this operation.
     *
     *         If we are the first security operation in this block, then we need to
     *         unwrap the key for us (and subsequent security operations).
     */

    /*
     * Step 2.1 - If there is no session key associated with this state, we are
     *            the first operation for this block, so unwrap the key.
     */
    if(state->scRawKey.scValLength == 0)
    {
        csi_cipherparms_t parms;
        int result = 0;

        /*
         * Step 2.1.1: Get they key to use for signing. AES keywrap does not need any
         *         parameters, so we can pass in empty parms here.
         */
        memset(&parms, 0, sizeof(csi_cipherparms_t));
        result = bpsec_scutl_keyUnwrap(state, BPSEC_BHSSC_PARM_LTK_NAME, &key_value, BPSEC_BHSSC_PARM_WRAPPED_KEY, CSTYPE_AES_KW, &parms);

        if(result == ERROR)
        {
            BPSEC_DEBUG_ERR("Cannot find a key for verification.", NULL);
            return ERROR;
        }

        /* Step 2.1.2: Store this key for use by other operations in the block. */
        state->scRawKey = bpsec_scv_memCsiConvert(key_value, SC_VAL_STORE_MEM, CSI_PARM_BEK);
    }

    /* Step 2.2 - If the state already has a session key, we can use that instead. */
    else
    {
        key_value.contents = state->scRawKey.scRawValue.asPtr;
        key_value.len = state->scRawKey.scValLength;
    }


    /* Step 3 - Retrieve the asserted digest from the security block. */
    if((assertedDigest = bpsec_scv_lystFind(tgtResult->scIndTargetResults, BPSEC_BHSSC_RESULT_EHMAC, SC_VAL_TYPE_RESULT)) == NULL)
    {
        BPSEC_DEBUG_ERR("No digest found for target %d", tgtResult->scTargetId);
        return ERROR;
    }
    csi_digest.len = assertedDigest->scValLength;
    csi_digest.contents = assertedDigest->scRawValue.asPtr;


    /*
     * Step 3: Generate a canonical version of the IPPT to be signed. This is built in
     *         in two steps:
     *         1. Generate the in-memory IPPT preamble which consists of the scope
     *            flag and other (small-so-as-to-fit-in-memory) data.
     *         2. The block-type-specific data of the target block, which might be
     *            very large and this accessed only through a ZCO.
     */


    if((tgtResult->scTargetId == PayloadBlk) || (tgtResult->scTargetId == PrimaryBlk))
    {
    	/* Step 3.2 - Grab the IPPT ZCO. */
    	if((ipptZcoLen = bpsec_util_canonicalizeIn(wk, tgtResult->scTargetId, &ipptZco)) <= 0)
    	{
    		BPSEC_DEBUG_ERR("Cannot canonicalize block-type specific data of %d.", tgtResult->scTargetId);
    		return ERROR;
    	}
    	addData = 0;
    }
    else
    {
    	ipptZcoLen = 0;
    	ipptZco = 0;
    	addData = 1;
    }


    /* Step 3.1 - Grab the IPPT preamble. */
    ippt_preamble = bpsec_rfc9173utl_authDataBuild(state, BPSEC_BHSSC_PARM_SCOPE_FLAGS, tgtResult->scTargetId, addData, NULL, wk);

    if((ippt_preamble.scSerializedLength <= 0) || (ippt_preamble.scSerializedText == NULL))
    {
        BPSEC_DEBUG_ERR("Cannot build IPPT Data.",NULL);
    	zco_destroy(state->sdr, ipptZco);
        return ERROR;
    }

    if(ipptZco != 0)
    {
    	/* Step 4: Compute the signature over the entire IPPT value. */
    	csi_ctx = bpsec_bhsscutl_computeSignature(ippt_preamble, ipptZco, ipptZcoLen, sha_variant, key_value, CSI_SVC_VERIFY);
    	zco_destroy(state->sdr, ipptZco);

        result = csi_sign_finish(sha_variant, csi_ctx, &csi_digest, CSI_SVC_VERIFY);
        csi_ctx_free(sha_variant, csi_ctx);
    }
    else
    {
    	csi_val_t input;
    	input.len = ippt_preamble.scSerializedLength;
    	input.contents = ippt_preamble.scSerializedText;
    	result = csi_sign_full(sha_variant, input, key_value, &csi_digest, CSI_SVC_VERIFY);
    }
	MRELEASE(ippt_preamble.scSerializedText);


    /*
     * Step 5: Finish the context. When this is called for signing, the
     *         computed signature is copied out.
     */

    return (result == 1);
}




/******************************************************************************
 * @brief Process a security operation in an outgoing security block.
 *
 * @param[in]  state      - The state of the security block processing
 * @param[out] extraParms - Parameters to add to the outgoing security block
 * @param[in]  bundle     - The outgoing bundle
 * @param[in]  asb        - The deserialized abstract security block
 * @param[out] tgtResult  - Security results generated for this target.
 *
 * The extraParms list allows a security operation to generate a parameter that
 * will both be made available for future processing of operations in the same
 * block as well as serialized and included in the security block as it is
 * written to the bundle itself.
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/
int bpsec_bhssci_procOutBlk(sc_state *state, Lyst extraParms, Bundle *bundle,
		                    BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult)
{
    csi_val_t key;
    BpsecSerializeData ippt_preamble;
    Object ipptZco = 0;
    int ipptZcoLen = 0;
    uint8_t *csi_ctx = NULL;
    sc_value *digest = NULL;
    csi_val_t csi_result;
    int result = 0;
    int addData = 0;
    uint16_t sha_variant = 0;

    /* Step 0 - Sanity Checks. */
    CHKERR(state);
    CHKERR(state->scStAction == SC_ACT_SIGN);

    /* Step 1: Get and validate the SHA Variant to use to sign. */
    sha_variant = bpsec_rfc9173utl_intParmGet(state, BPSEC_BHSSC_PARM_SHA_VAR_ID, BPSEC_BHSSC_SV_DEFAULT);

    switch(sha_variant)
    {
        case CSTYPE_HMAC_SHA256:
        case CSTYPE_HMAC_SHA384:
        case CSTYPE_HMAC_SHA512:

            break;
        default:
            BPSEC_DEBUG_WARN("Incorrect value for parm %s, defaulting to %d.", bpsec_scvm_byIdNameFind(gBHS_ParmMap, BPSEC_BHSSC_PARM_SHA_VAR_ID, SC_VAL_TYPE_PARM), BPSEC_BHSSC_SV_DEFAULT);
            sha_variant = BPSEC_BHSSC_SV_DEFAULT;
            break;
    }


    /*
     * Step 2: Get the signing key. If we are NOT the first security operation being
     *         added to this security block, then the state will have the key used
     *         for this operation.
     *
     *         If we are the first security operation in this block, then we need to
     *         define keys and store them in the state.
     *
     *         NOTE: Once we add the wrapped key as a parameter for this state, we
     *               don't need it for any processing, it just needs to be send in
     *               the security block to a downstream verifier/acceptor.
     */

    if(state->scRawKey.scValLength == 0)
    {
        sc_value *wrappedKey = MTAKE(sizeof(sc_value));
        if(bpsec_rfc9173utl_sesKeyGet(state, BPSEC_BHSSC_PARM_LTK_NAME, BPSEC_BHSSC_PARM_WRAPPED_KEY, sha_variant, &key, wrappedKey) == ERROR)
        {
            BPSEC_DEBUG_ERR("Cannot get signing key for variant %d", sha_variant);
            return ERROR;
        }

        state->scRawKey = bpsec_scv_memCsiConvert(key, SC_VAL_TYPE_PARM, CSI_PARM_BEK);

        if((lyst_insert_last(extraParms, wrappedKey)) == NULL)
        {
            BPSEC_DEBUG_ERR("Unable to store wrapped key.", NULL);
            bpsec_scv_clear(0, &(state->scRawKey));
            bpsec_scv_clear(0, wrappedKey);
            MRELEASE(wrappedKey);
            return ERROR;
        }
    }
    else
    {
        key.contents = state->scRawKey.scRawValue.asPtr;
        key.len = state->scRawKey.scValLength;
    }


    /*
    * Step 3: Generate a canonical version of the IPPT to be signed. This is built in
    *         in two steps:
    *         1. Generate the in-memory IPPT preamble which consists of the scope
    *            flag and other (small-so-as-to-fit-in-memory) data.
    *         2. The block-type-specific data of the target block, which might be
    *            very large and this accessed only through a ZCO.
    */
    if((tgtResult->scTargetId == PayloadBlk) || (tgtResult->scTargetId == PrimaryBlk))
      {
    	/* Step 3.1 - Grab the IPPT ZCO. */
    	if((ipptZcoLen = bpsec_util_canonicalizeOut(bundle, tgtResult->scTargetId, &ipptZco)) <= 0)
    	{
    		BPSEC_DEBUG_ERR("Cannot canonicalize block-type specific data of %d.", tgtResult->scTargetId);
    		return ERROR;
    	}
      }
      else
      {
      	ipptZcoLen = 0;
      	ipptZco = 0;
      	addData = 1;
      }

    /* Step 3.2 - Grab the IPPT preamble. */
    ippt_preamble = bpsec_rfc9173utl_authDataBuild(state, BPSEC_BHSSC_PARM_SCOPE_FLAGS, tgtResult->scTargetId, addData, bundle, NULL);

    if((ippt_preamble.scSerializedLength <= 0) || (ippt_preamble.scSerializedText == NULL))
    {
        BPSEC_DEBUG_ERR("Cannot build IPPT Data.",NULL);
        zco_destroy(state->sdr, ipptZco);
        return ERROR;

    }

    /*
     * Step 4 - If we have a ZCO, this means that the block-type-specific data of the
     *          security target block is accessed through the ZCO. In this case, we use
     *          a helper function to do fixed-sized reads of data as we build the
     *          digest.
     *
     *          If there is no ZCO, then the entire IPPT exists in memory and we can
     *          try and do the signing in one single call.
     *
     *          Either way, once we have the digest, we can release the in-memory part
     *          of the IPPT.
     */
    if(ipptZco != 0)
    {
    	/* Step 4.1.1: Compute the signature over the entire IPPT value. */
    	csi_ctx = bpsec_bhsscutl_computeSignature(ippt_preamble, ipptZco, ipptZcoLen, sha_variant, key, CSI_SVC_SIGN);
    	zco_destroy(state->sdr, ipptZco);

        /*
        * Step 4.1.2: Finish the context. When this is called for signing, the
        *         computed signature is copied out.
        */
        result = csi_sign_finish(sha_variant, csi_ctx, &csi_result, CSI_SVC_SIGN);
        csi_ctx_free(sha_variant, csi_ctx);
    }
    else
    {
       	csi_val_t input;

       	/* Step 4.2.1: Convert the ippt to a csi value. */
       	input.len = ippt_preamble.scSerializedLength;
       	input.contents = ippt_preamble.scSerializedText;

       	/* Step 4.2.2: Calculate the digest. */
       	result = csi_sign_full(sha_variant, input, key, &csi_result, CSI_SVC_SIGN);
    }
   	MRELEASE(ippt_preamble.scSerializedText);


    /* Step 5: Handle any errors. */
    if(result == ERROR)
    {
    	BPSEC_DEBUG_ERR("Processing error. Returning %d.", ERROR);
        return ERROR;
    }

    /*
     * Step 6: The resultant signature is stored as a security context result
     *         which must be converted from a CSI value to a SC value.
     *
     *         So, convert the value and add it to the target result list
     *         for this security operation.
     */
    if((digest = MTAKE(sizeof(sc_value))) == NULL)
    {
        BPSEC_DEBUG_ERR("Unable to allocate digest.", NULL);
        return ERROR;
    }
    *digest = bpsec_scv_memCsiConvert(csi_result, SC_VAL_TYPE_RESULT, BPSEC_BHSSC_RESULT_EHMAC);

    if((lyst_insert_last(state->scStResults, digest)) == NULL)
    {
        BPSEC_DEBUG_ERR("Unable to append new result.", NULL);
        bpsec_scv_clear(0, digest);
        MRELEASE(digest);
        return ERROR;
    }

    return 1;
}



/******************************************************************************
 * @brief Retrieves the sc value map for this SC
 *
 * @retval !NULL - The value map for this SC
 * @retval  NULL - There was an error.
 *****************************************************************************/
sc_value_map* bpsec_bhssci_valMapGet()
{
    return gBHS_ParmMap;
}



/******************************************************************************
 * @brief Calculate a digest for a given set of IPPT.
 *
 * @param[in]  preamble   The parts of the IPPT that are not in a ZCO.
 * @param[in]  zcoObj     ZCO to the target block block-type-specific data
 * @param[in]  zcoLen     The length of the ZCO.
 * @param[in]  csi_suite  Which digest to use.
 * @param[in]  csi_key    The key to use for signing.
 * @param[in]  svc        Whether we are signing or verifying.
 *
 * This function is used when part of the IPPT data exists in a ZCO and, thus,
 * might be very large requiring an iterative approach to calculating a digest.
 *
 * The digest is not returned by this function. Rather, the function returns
 * the CSI context for the digest creation. This allows the caller to either
 *
 *   1. Use the context to add more data to the digest if needed later
 *   2. Finalize the context to compute the digest result as-is.
 *
 * @retval !NULL - The CSI context for the digest.
 * @retval NULL  - Error
 *****************************************************************************/

uint8_t *bpsec_bhsscutl_computeSignature(BpsecSerializeData preamble, Object zcoObj, int zcoLen, int csi_suite, csi_val_t csi_key, csi_svcid_t svc)
{
    Sdr sdr = getIonsdr();
    ZcoReader dataReader;
    unsigned int zcoRemaining = zcoLen;
    unsigned int zcoRead = 0;
    unsigned int preambleRemaining = 0;
    uint32_t chunkSize = csi_blocksize(csi_suite);
    csi_val_t chunkData;
    uint8_t *csi_ctx = NULL;
    void *cursor = NULL;
    int success = 1;


    /* Step 0 - Sanity check. */
    CHKNULL(csi_key.contents);


    /* Step 1 - Allocate a working buffer. */
    if((chunkData.contents = MTAKE(chunkSize)) == NULL)
    {
        BPSEC_DEBUG_ERR("Failure to allocate chunk size of %d", chunkSize);
        return NULL;
    }
    chunkData.len = chunkSize;


    /* Step 2 - Create the CSI context for the signing. */
    csi_ctx = csi_ctx_init(csi_suite, csi_key, CSI_SVC_SIGN);
    if(csi_sign_start(csi_suite, csi_ctx) == ERROR)
    {
        BPSEC_DEBUG_ERR("Can't start context.", NULL);
        MRELEASE(chunkData.contents);
        csi_ctx_free(csi_suite, csi_ctx);
        return NULL;
    }


    /* Step 3 - Set up a ZCO reader and an associated transaction. */
    zco_start_transmitting(zcoObj, &dataReader);

    if ((sdr_begin_xn(sdr)) == 0)
    {
        BPSEC_DEBUG_ERR("Can't start txn.", NULL);
        MRELEASE(chunkData.contents);
        csi_ctx_free(csi_suite, csi_ctx);
        return NULL;
    }


    /*
     * Step 4 - Start calculating signature with preamble data until
     *          there is less than a whole chunk worth of such data
     *          left.
     */
    preambleRemaining = preamble.scSerializedLength;
    cursor = preamble.scSerializedText;

    while((success) && (preambleRemaining >= chunkSize))
    {
        memcpy(chunkData.contents, cursor, chunkSize);
        cursor += chunkSize;

        /* Add the data to the context.        */
        if(csi_sign_update(csi_suite, csi_ctx, chunkData, svc) == ERROR)
        {
            BPSEC_DEBUG_ERR("Error updating signature.", NULL);

            /* Setting success to 0 skips remaining processing. */
            success = 0;
        }

        preambleRemaining -= chunkSize;
    }


    /*
     * Step 5 - Add remaining preamble + start of ZCO to the digest.
     *          Account for the case where the remaining preamble and
     *          entire ZCO fit into a single chunk.
     */
    if((success) && (preambleRemaining > 0))
    {
        int delta = chunkSize - preambleRemaining;

        memcpy(chunkData.contents, cursor, preambleRemaining);

        cursor = chunkData.contents + preambleRemaining;

        if(delta >= zcoRemaining)
        {
            delta = zcoRemaining;
            chunkData.len = preambleRemaining + zcoRemaining;
        }

        zcoRead = zco_transmit(sdr, &dataReader, delta, cursor);
        if(zcoRead != delta)
        {
            BPSEC_DEBUG_ERR("Read %d bytes, but expected %d.", zcoRead, delta);

            /* Setting success to 0 skips remaining processing. */
            success = 0;
        }
        else if(csi_sign_update(csi_suite, csi_ctx, chunkData, svc) == ERROR)
        {
            BPSEC_DEBUG_ERR("Error updating signature.", NULL);

            /* Setting success to 0 skips remaining processing. */
            success = 0;
        }

        zcoRemaining -= delta;
    }


    /* Step 6 - add any remaining ZCO data to the digest. */
    while((success) && (zcoRemaining > 0))
    {
        if(zcoRemaining < chunkSize)
        {
            chunkSize = zcoRemaining;
            chunkData.len = chunkSize;
        }

        zcoRead = zco_transmit(sdr, &dataReader, chunkSize, (char *) chunkData.contents);
        if(zcoRead != chunkSize)
        {
            BPSEC_DEBUG_ERR("Read %d bytes, but expected %d.", zcoRead, chunkSize);
            success = 0;
        }
        else if(csi_sign_update(csi_suite, csi_ctx, chunkData, svc) == ERROR)
        {
            BPSEC_DEBUG_ERR("Error updating signature.", NULL);
            success = 0;
        }

        zcoRemaining -= chunkSize;
    }

    /* Step 7 - Cleanup, to include handling error. */
    sdr_exit_xn(sdr);
    MRELEASE(chunkData.contents);

    if(!success)
    {
        csi_ctx_free(csi_suite, csi_ctx);
        csi_ctx = NULL;
    }

    BPSEC_DEBUG_PROC("-->"ADDR_FIELDSPEC,(uaddr)csi_ctx);
    return csi_ctx;
}
