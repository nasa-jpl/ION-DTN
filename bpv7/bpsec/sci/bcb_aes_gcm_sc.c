/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bcb_aes_gcm_sc.c
 **
 ** Namespace:
 **    bpsec_bagsci_   SCI Interface functions
 **    bpsec_bagscutl  General utilities
 **
 **
 ** Description:
 **
 **     This file implements The BCB-AES-GCM security context standardized
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
 **  08/07/21  E. Birrane     Initial implementation
 *****************************************************************************/

#include "bcb_aes_gcm_sc.h"

#include "bpsec_util.h"
#include "sc_value.h"
#include "cbor.h"
#include "rfc9173_utils.h"

static sc_value_map gBAG_ParmMap[] = {
    {"key_name",    BPSEC_BAGSC_PARM_LTK_NAME,    SC_VAL_TYPE_PARM,   bpsec_scvm_strStrDecode, bpsec_scv_clear, bpsec_scvm_strStrEncode, NULL, NULL},
    {"iv",          BPSEC_BAGSC_PARM_IV,          SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {"aes_variant", BPSEC_BAGSC_PARM_AES_VAR_ID,  SC_VAL_TYPE_PARM,   bpsec_scvm_intStrDecode, bpsec_scv_clear, bpsec_scvm_intStrEncode, bpsec_scvm_intCborEncode, bpsec_scvm_intCborDecode},
    {"wrapped_key", BPSEC_BAGSC_PARM_WRAPPED_KEY, SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {"aad_scope",   BPSEC_BAGSC_PARM_AAD_SCOPE,   SC_VAL_TYPE_PARM,   bpsec_scvm_intStrDecode, bpsec_scv_clear, bpsec_scvm_intStrEncode, bpsec_scvm_intCborEncode, bpsec_scvm_intCborDecode},
    {"tag",         BPSEC_BAGSC_RESULT_TAG,       SC_VAL_TYPE_RESULT, bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {NULL,-1, SC_VAL_TYPE_UNKNOWN, NULL, NULL, NULL, NULL, NULL}
};


sc_value_map* bpsec_bagsci_valMapGet()
{
	return gBAG_ParmMap;
}



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
int bpsec_bagsci_procInBlk(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, LystElt tgtBlkElt, BpsecInboundTargetResult *tgtResult)
{
    int result = 0;
    csi_val_t key_value;
    uint16_t aes_variant = 0;
    csi_cipherparms_t csi_parms;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
    		         (uaddr)state, (uaddr)wk, (uaddr)asb,(uaddr)tgtBlkElt,(uaddr)tgtResult);


    /* Step 0: Sanity Checks. */
    CHKERR(state);

    if(state->scRole != SC_ROLE_ACCEPTOR)
    {
        BPSEC_DEBUG_INFO("BCB-AES-GCM default passes at verifier.", NULL);
        return 1;
    }

    CHKERR(state->scStAction == SC_ACT_DECRYPT);


    /* Step 1: Get and validate the AES Variant to use to verify. */
    aes_variant = bpsec_rfc9173utl_intParmGet(state, BPSEC_BAGSC_PARM_AES_VAR_ID, BPSEC_BAGSC_AV_DEFAULT);

    switch(aes_variant)
    {
        case BPSEC_BAGSC_AV_128:
        	aes_variant = CSTYPE_AES128_GCM;
        	break;
        case BPSEC_BAGSC_AV_256:
        	aes_variant = CSTYPE_AES256_GCM;
            break;
        default:
            BPSEC_DEBUG_WARN("Incorrect value for parm %s, defaulting to %d.",  bpsec_scvm_byIdNameFind(gBAG_ParmMap, BPSEC_BAGSC_PARM_AES_VAR_ID, SC_VAL_TYPE_PARM), BPSEC_BAGSC_AV_DEFAULT);
            aes_variant = (BPSEC_BAGSC_AV_DEFAULT == BPSEC_BAGSC_AV_128) ? CSTYPE_AES128_GCM : CSTYPE_AES256_GCM;
            break;
    }

    /*
     * Step 2: Get the decrypting key. If we are NOT the first security operation in
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

        result = bpsec_scutl_keyUnwrap(state, BPSEC_BAGSC_PARM_LTK_NAME, &key_value, BPSEC_BAGSC_PARM_WRAPPED_KEY, CSTYPE_AES_KW, &parms);

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

    /* Step 3 - Grab parameters needed for decryption. */
    if(bpsec_bagscu_inParmsGet(state, wk, tgtResult, &csi_parms) <= 0)
    {
        BPSEC_DEBUG_ERR("Cannot construct parms for decryption.", NULL);
        return ERROR;
    }

    /*
     * Step 4: If the security target block is the payload, then we need
     *         to work with a ZCO to access the payload data.
     *
     *         Otherwise, we assume that the extension block is small enough
     *         to fit in memory and we can process it all at once.
     */
    if(tgtResult->scTargetId == PayloadBlk)
    {
    	/* Step 4.1.1: Decrypt the payload via its ZCO. */
    	if(bpsec_bagscu_zcoCompute(aes_variant, &(wk->bundle.payload.content), key_value, &csi_parms, CSI_SVC_DECRYPT) == ERROR)
    	{
            BPSEC_DEBUG_ERR("Cannot get data for payload block.", NULL);
            return ERROR;
    	}
    	result = 1;
    }
    else
    {
        AcqExtBlock *tgtBlk = (AcqExtBlock *) lyst_data(tgtBlkElt);
        csi_val_t input;
        csi_val_t output;
        int offset = 0;

        /* Step 4.2.1: Make sure we have the security target extension block. */
        if(tgtBlk == NULL)
        {
        	BPSEC_DEBUG_ERR("Cannot find target block.", NULL);
        	return ERROR;
        }

        /*
         * Step 4.2.2: Wrap the target block's block-type-specific data field in a
         *             CSI value.
         *
         *             The block-type-specific data is just part of the
         *             overall bytes array, so we need to skip the block header
         *             information (offset) to get to it.
         *
         *             The length is the dataLength, as any trailing CRC information
         *             is not considered part of the block-type-specific data field.
         *
         *             NOTE: It appears that tgtBlk->length does not include the length
         *                   of any trailing CRC value, so the offset calculation should
         *                   be safe regardless of whether the extension block has a CRC
         *                   or not.
         */
        offset = tgtBlk->length - tgtBlk->dataLength;
        input.len = tgtBlk->dataLength;
		input.contents = tgtBlk->bytes + offset;

		/* Step 4.2.3: Perform the decryption. and check results.*/
    	if(csi_crypt_full(aes_variant, CSI_SVC_DECRYPT, &csi_parms, key_value, input, &output) == ERROR)
    	{
            BPSEC_DEBUG_ERR("Cannot compute data for block %d.", tgtBlk->number);
            return ERROR;
    	}

    	if(output.len != input.len)
    	{
    		BPSEC_DEBUG_ERR("Cannot handle resizing extension blocks at this time.", NULL);
    		/*
    		 * TODO: We currently expect that ION uses cipher suites that generate a tag
    		 *       value distinguished from the cipher text and, thus, carried in a
    		 *       security context result. Therefore, the decrypted extension block
    		 *       (with AES in GCM) should have the same size.
    		 *
    		 *       This code may need to be updated when using cipher suites that
    		 *       include the tag with ciphertext, resulting in size changes with
    		 *       encryption and decrption.
    		 */
    		return ERROR;
    	}
    	else
    	{
    		memcpy(tgtBlk->bytes + offset, output.contents, tgtBlk->dataLength);
    		result = 1;
    		MRELEASE(output.contents);
    	}
    }

    return result;
}



/******************************************************************************
 * @brief Compute cipher suite results and replace contents of a given block.
 *
 * @param[in]  suite    - Which cipher suite to use to calculate the value.
 * @param[in]  dataObj  - The block-type-specific data to process (in a ZCO)
 * @param[in]  sesKey   - The key to use.
 * @param[in]  parms    - Other ciphersuite parameters
 * @param[in]  function - Encrypt or Decrypt

 * This function encrypts or decrypts a target block (depending on what function
 * is provided).
 *
 * @todo
 *   - Ingest xmit rate instead of hard-coding it.
 *   - Rethink Scott's original design of limiting SDR tries by xmit rate?
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/

int bpsec_bagscu_zcoCompute(uint32_t suite, Object *dataObj, csi_val_t sesKey, csi_cipherparms_t *parms, uint8_t function)
{
    Sdr sdr = getIonsdr();
    csi_blocksize_t blocksize;
    uint32_t  cipherBufLen = 0;
    uint32_t  bytesRemaining = 0;
    ZcoReader dataReader;
    uint8_t   *context = NULL;
    int32_t   result = 0;
    Object    cipherZco = 0;

    /* Step 0 - Sanity checks. */
    CHKERR(dataObj);


    /* Step 1 - Start a transaction. */
    if ((sdr_begin_xn(sdr)) == 0)
    {
        BPSEC_DEBUG_ERR("Can't start txn.", NULL);
        return -1;
    }


    /*
     * Step 2 - Setup playback of data from the data object. The data object
     *          is the target block.
     */

    if ((bytesRemaining = zco_length(sdr, *dataObj)) <= 0)
    {
        BPSEC_DEBUG_ERR("Data object has no data.", NULL);
        sdr_cancel_xn(sdr);
        return -1;
    }
    zco_start_transmitting(*dataObj, &dataReader);


    /* Step 3 - Grab and initialize a crypto context. */
    if ((context = csi_ctx_init(suite, sesKey, function)) == NULL)
    {
        BPSEC_DEBUG_ERR("Can't get context.", NULL);
        sdr_cancel_xn(sdr);
        BPSEC_DEBUG_PROC("--> NULL", NULL);
        return -1;
    }


    /* Step 4: Calculate the maximum size of the ciphertext. */
    blocksize.chunkSize = csi_blocksize(suite);
    blocksize.keySize = sesKey.len;
    blocksize.plaintextLen = bytesRemaining;

    if ((cipherBufLen = csi_crypt_res_len(suite, context, blocksize, function)) <= 0)
    {
        BPSEC_DEBUG_ERR("Predicted bad ciphertext length: %d", cipherBufLen);
        csi_ctx_free(suite, context);
        sdr_cancel_xn(sdr);

        BPSEC_DEBUG_PROC("--> %d", -1);
        return -1;
    }


    /**
     * Step 5: Replace plaintext with ciphertext.
     *
     *         If we are encrypting, we have to make a decision on how we
     *         generate the ciphertext; if we are decrypting, we similarly
     *         have to decide how to generate the plaintext.
     *
     *         So we have two choices for housing the ciphertext:
     *         1. Built a ZCO out of SDR dataspace (fast, but space limited)
     *         2. Built a ZCO to a temp file (slow, but accommodates large
     *             data)
     *
     *         If we think the cipher text is small enough, we will try the
     *         SDR approach first. If that fails, we will try the file system
     *         approach second.
     *
     *         Small here is defined as a function of an average file system
     *         transmit rate and the number of time files per second.
     */
    result = 0;

    /* Step 5.1 - Attempt to process in the SDR if the cipher text is small enough. */
    if (cipherBufLen < BPSEC_BAGSC_MIN_FILE_BUFFER)
    {
        if (csi_crypt_start(suite, context, *parms) == ERROR)
        {
            BPSEC_DEBUG_ERR("Can't start context", NULL);
            result = ERROR;
        }
        else
        {
        	result = bpsec_util_sdrBlkConvert(suite, context, &blocksize, &dataReader,
        								      cipherBufLen, &cipherZco, function);

        	if(csi_crypt_finish(suite, context, function, parms) == ERROR)
        	{
            	BPSEC_DEBUG_ERR("Could not finish context.", NULL);
            	result = ERROR;
        	}
        }
    }

    /* Step 5.2 - If cipher text is large or the SDR processing failed...*/
    if (result <= 0)
    {
        double    siestaBytes = 0;
        double    siestaUsec = 0;

        /* Step 5.2.1 - If we just failed with the SDR, let file system recover. */
        if (cipherBufLen < BPSEC_BAGSC_MIN_FILE_BUFFER)
        {
            /*    Slow down to avoid over-stressing the file system. */
            siestaBytes = BPSEC_BAGSC_MIN_FILE_BUFFER - cipherBufLen;
            siestaUsec = (1000000.0 * siestaBytes) / BPSEC_BAGSC_XMIT_RATE;
            microsnooze((unsigned int) siestaUsec);
        }

        if (csi_crypt_start(suite, context, *parms) == ERROR)
        {
            BPSEC_DEBUG_ERR("Can't start context", NULL);
            result = ERROR;
        }
        else
        {

        	/* Step 5.2.2 - Try processing using a tmp file. */
        	result = bpsec_util_fileBlkConvert(suite, context, &blocksize, &dataReader,
        			                           cipherBufLen, &cipherZco, BPSEC_BAGSC_FILENAME, function);

        	if(csi_crypt_finish(suite, context, function, parms) == ERROR)
        	{
        		BPSEC_DEBUG_ERR("Could not finish context.", NULL);
        		result = ERROR;
        	}
        }
    }


    /* Step 6 - Free resources. */
    zco_destroy(sdr, *dataObj);
    csi_ctx_free(suite, context);

    /* Step 7 - If we could not process, signal error. */
    if (result <= 0)
    {
        BPSEC_DEBUG_ERR("Cannot process ciphertext of size " UVAST_FIELDSPEC, cipherBufLen);
        sdr_cancel_xn(sdr);
        return -1;
    }

    /* Step 8 - Copy out cipher ZCO and vlose transaction. */
    if (sdr_end_xn(sdr) < 0)
    {
        BPSEC_DEBUG_ERR("Can't end encrypt txn.", NULL);
        return -1;
    }

    *dataObj = cipherZco;
    return 0;
}



/******************************************************************************
 * @brief Generate needed parameters to process an inbound security block.
 *
 * @param[in]  state     - The state of the security operation processing.
 * @param[in]  wk        - The acquisition work area
 * @param[in]  tgtResult - Security results for the security operation.
 * @param[out] parms     - Populated CSI parameters.
 *
 * The output CSI parameters are deep copies of the results stored in the
 * tgtResult, as the CSI interface may modify and will expect to free
 * CSI parameters.
 *
 * @note
 *   - This implementation assumes that the authentication tag generated by the
 *     AES-GCM suite is carried as a security result, and not otherwise
 *     attached to the cipher text.
 *
 * The parameters extracted are:
 * 1. The Initialization Vector (IV)
 * 2. The Authentication Tag
 * 3. The calculated AAD
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/

int bpsec_bagscu_inParmsGet(sc_state *state, AcqWorkArea *wk, BpsecInboundTargetResult *tgtResult, csi_cipherparms_t *parms)
{
    BpsecSerializeData aad;
    sc_value *tmp_val = NULL;

    /* Step 0: Sanity Checks and initialization. */
    CHKERR(tgtResult);
    CHKERR(parms);

    memset(parms, 0, sizeof(csi_cipherparms_t));


    /* Step 1 - Retrieve the IV from the security block and wrap it in a CSI value. */
    if((tmp_val = bpsec_scv_lystFind(state->scStParms, BPSEC_BAGSC_PARM_IV, SC_VAL_TYPE_PARM)) == NULL)
    {
        BPSEC_DEBUG_ERR("No IV present for target %d", tgtResult->scTargetId);
        return ERROR;
    }
    parms->iv.len = tmp_val->scValLength;
    if((parms->iv.contents = MTAKE(parms->iv.len)) == NULL)
    {
        BPSEC_DEBUG_ERR("Unable to allocate an IV of length %d.", parms->iv.len);
        return ERROR;
    }
    memcpy(parms->iv.contents, tmp_val->scRawValue.asPtr, parms->iv.len);


    /* Step 2 - Retrieve the authentication tag and wrap it in a CSI value. */
    if((tmp_val = bpsec_scv_lystFind(tgtResult->scIndTargetResults, BPSEC_BAGSC_RESULT_TAG, SC_VAL_TYPE_RESULT)) == NULL)
    {
        BPSEC_DEBUG_ERR("No authentication tag found for target %d", tgtResult->scTargetId);
        csi_cipherparms_free(*parms);
        return ERROR;
    }
    parms->icv.len = tmp_val->scValLength;
    if((parms->icv.contents = MTAKE(parms->icv.len)) == NULL)
    {
        BPSEC_DEBUG_ERR("Unable to allocate an ICV of length %d.", parms->icv.len);
        csi_cipherparms_free(*parms);
        return ERROR;
    }
    memcpy(parms->icv.contents, tmp_val->scRawValue.asPtr, parms->icv.len);


    /* Step 3: Generate the AAD. */
    aad = bpsec_rfc9173utl_authDataBuild(state, BPSEC_BAGSC_PARM_AAD_SCOPE, tgtResult->scTargetId, 0, NULL, wk);

    if((aad.scSerializedLength <= 0) || (aad.scSerializedText == NULL))
    {
        BPSEC_DEBUG_ERR("Cannot build AAD.",NULL);
        csi_cipherparms_free(*parms);
        return ERROR;
    }
    parms->aad.contents = aad.scSerializedText;
    parms->aad.len = aad.scSerializedLength;

    BPSEC_DEBUG_PROC("Returning 1.", NULL);
    return 1;
}



/******************************************************************************
 * @brief Generate needed parameters to process an outbound security block.
 *
 * @param[in]  state     - The state of the security operation processing.
 * @param[in]  suite     - The AES variant being used.
 * @param[out] extraParms -
 * @param[in|out]  bundle    - The outbound bundle.
 * @param[in] tgtResult - Security results for the security operation
 * @param[out] parms    - Populated CSI parameters.
 *
 * The output CSI parameters are deep copies of the results stored in the
 * tgtResult, as the CSI interface may modify and will expect to free
 * CSI parameters.
 *
 * @note
 *   - RFC9173 keeps the IV as a single security parameter for the security
 *     block, which requires that the security block only use a single
 *     security operation. Trying to place multiple security operations in
 *     a security block using this security context would result in using
 *     the IV twice, which is not allowed by RFC9173.
 *
 * The parameters extracted are:
 * 1. The Initialization Vector (IV)
 * 2. The calculated AAD
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/
int bpsec_bagscu_outParmsGet(sc_state *state, int suite, Lyst extraParms, Bundle *bundle, BpsecOutboundTargetResult *tgtResult, csi_cipherparms_t *parms)
{
    BpsecSerializeData aad;
    sc_value *tmp_val = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
    		         (uaddr) state, suite, (uaddr) extraParms, (uaddr) bundle, (uaddr) tgtResult, (uaddr) parms);

    /* Step 0: Sanity checks and initialization. */
    CHKERR(state);
    CHKERR(extraParms);
    CHKERR(bundle);
    CHKERR(tgtResult);
    CHKERR(parms);

    memset(parms, 0, sizeof(csi_cipherparms_t));


    /*
     * Step 1 - Generate the Initialization Vector (IV).  If there is already an IV associated with
     *          this security block, it means that we have already used the security block IV for
     *          an encryption and we cannot use it again.
     *
     *          NOTE: This is a limitation of RFC9173 - since the IV is a parameter associated with the
     *                security block itself, a security block using this security context can only
     *                hold a single security operation, representing a single use of the IV parameter.
     */
    if((tmp_val = bpsec_scv_lystFind(state->scStParms, BPSEC_BAGSC_PARM_IV, SC_VAL_TYPE_PARM)) == NULL)
    {
    	csi_val_t csi_tmp;

        csi_tmp = csi_crypt_parm_get(suite, (csi_parmid_t) BPSEC_BAGSC_PARM_IV);
        if(csi_tmp.len == 0)
        {
            BPSEC_DEBUG_ERR("Cannot get IV using suite %d.", suite);
            return -1;
        }

    	if((tmp_val = MTAKE(sizeof(sc_value))) == NULL)
    	{
    		BPSEC_DEBUG_ERR("Cannot allocate sc value.", NULL);
    		return -1;
    	}

    	*tmp_val = bpsec_scv_memCsiConvert(csi_tmp, SC_VAL_TYPE_PARM, BPSEC_BAGSC_PARM_IV);

    	/* Make available to other SOPs in this block. */
    	lyst_insert_last(state->scStParms, tmp_val);

    	/* Make available in outgoing security block. */
    	lyst_insert_last(extraParms, tmp_val);
    }
    else
    {
        BPSEC_DEBUG_ERR("MISCONFIGURATION: IV already used for this security block. We cannot use the same IV.", NULL);
        /* TODO: How do we signal failure due to misconfiguration here? */
        return ERROR;
    }

    parms->iv.len = tmp_val->scValLength;
    if((parms->iv.contents = MTAKE(parms->iv.len)) == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot allocate IV of length %d.", parms->iv.len);
        return ERROR;
    }
    memcpy(parms->iv.contents, tmp_val->scRawValue.asPtr, parms->iv.len);


    /*
     * Step 2: Generate the AAD.
     */
    aad = bpsec_rfc9173utl_authDataBuild(state, BPSEC_BAGSC_PARM_AAD_SCOPE, tgtResult->scTargetId, 0, bundle, NULL);

    if((aad.scSerializedLength <= 0) || (aad.scSerializedText == NULL))
    {
        BPSEC_DEBUG_ERR("Cannot build AAD.",NULL);
        csi_cipherparms_free(*parms);
        return ERROR;
    }

    parms->aad.contents = aad.scSerializedText;
    parms->aad.len = aad.scSerializedLength;

    return 1;

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

int bpsec_bagsci_procOutBlk(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult)
{
    int result = 1;
    csi_val_t key_value;
    uint16_t aes_variant = 0;
    csi_cipherparms_t csi_parms;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
    		         (uaddr) state, (uaddr) extraParms, (uaddr) bundle, (uaddr) asb, (uaddr) tgtResult);

    /* Step 0 - Sanity checks. */
    CHKERR(state);
    CHKERR(bundle);

    if(state->scRole != SC_ROLE_SOURCE)
    {
        BPSEC_DEBUG_INFO("BCB-AES-GCM only adds block at security source.", NULL);
        return ERROR;
    }

    CHKERR(state->scStAction == SC_ACT_ENCRYPT);


    /* Step 1: Get and validate the AES Variant to use to verify. */
    aes_variant = bpsec_rfc9173utl_intParmGet(state, BPSEC_BAGSC_PARM_AES_VAR_ID, BPSEC_BAGSC_AV_DEFAULT);

    switch(aes_variant)
    {
    	case BPSEC_BAGSC_AV_128:
           	aes_variant = CSTYPE_AES128_GCM;
           	break;
        case BPSEC_BAGSC_AV_256:
          	aes_variant = CSTYPE_AES256_GCM;
            break;
        default:
            BPSEC_DEBUG_WARN("Incorrect value for parm %s, defaulting to %d.",  bpsec_scvm_byIdNameFind(gBAG_ParmMap, BPSEC_BAGSC_PARM_AES_VAR_ID, SC_VAL_TYPE_PARM), BPSEC_BAGSC_AV_DEFAULT);
            aes_variant = (BPSEC_BAGSC_AV_DEFAULT == BPSEC_BAGSC_AV_128) ? CSTYPE_AES128_GCM : CSTYPE_AES256_GCM;
            break;
    }


    /*
     * Step 2: Get the encrypting key. If we are NOT the first security operation in
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
        sc_value *wrappedKey = MTAKE(sizeof(sc_value));
        if(bpsec_rfc9173utl_sesKeyGet(state, BPSEC_BAGSC_PARM_LTK_NAME, BPSEC_BAGSC_PARM_WRAPPED_KEY, aes_variant, &key_value, wrappedKey) == ERROR)
        {
            BPSEC_DEBUG_ERR("Cannot get signing key for variant %d", aes_variant);
            return ERROR;
        }

        state->scRawKey = bpsec_scv_memCsiConvert(key_value, SC_VAL_TYPE_PARM, CSI_PARM_BEK);

        if((lyst_insert_last(extraParms, wrappedKey)) == NULL)
        {
            BPSEC_DEBUG_ERR("Unable to store wrapped key.", NULL);
            bpsec_scv_clear(0, &(state->scRawKey));
            bpsec_scv_clear(0, wrappedKey);
            MRELEASE(wrappedKey);
            return ERROR;
        }
    }

    /* Step 2.2 - If the state already has a session key, we can use that instead. */
    else
    {
        key_value.contents = state->scRawKey.scRawValue.asPtr;
        key_value.len = state->scRawKey.scValLength;
    }

    /* Step 3 - Grab parameters needed for decryption. */
    if(bpsec_bagscu_outParmsGet(state, aes_variant, extraParms, bundle, tgtResult, &csi_parms) <= 0)
    {
        BPSEC_DEBUG_ERR("Cannot construct parms for decryption.", NULL);
        return ERROR;
    }

    /*
     * Step 5: Grab the ZCO to the target block's block-type-specific data.
     */
    if(tgtResult->scTargetId == PayloadBlk)
    {

    	if(bpsec_bagscu_zcoCompute(aes_variant, &(bundle->payload.content), key_value, &csi_parms, CSI_SVC_ENCRYPT) == ERROR)
    	{
            BPSEC_DEBUG_ERR("Cannot get data for payload block.", NULL);
            result = ERROR;
    	}
    }
    else
    {
    	Object blkObj = getExtensionBlock(bundle, tgtResult->scTargetId);
    	OBJ_POINTER(ExtensionBlock, blk);
    	unsigned char *data = NULL;
    	csi_val_t input;
    	csi_val_t output;

    	GET_OBJ_POINTER(state->sdr, ExtensionBlock, blk, sdr_list_data(state->sdr, blkObj));

    	if((data = MTAKE(blk->length)) == NULL)
    	{
    		BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", blk->length);
    		return ERROR;
    	}
    	sdr_read(state->sdr, (char*)data, blk->bytes, blk->length);

    	input.len = blk->dataLength;
    	input.contents = data + (blk->length - blk->dataLength);


    	if(csi_crypt_full(aes_variant, CSI_SVC_ENCRYPT, &csi_parms, key_value, input, &output) == ERROR)
    	{
    		BPSEC_DEBUG_ERR("Cannot compute data for block %d.", tgtResult->scTargetId);
    		result = ERROR;
    	}
    	else if(output.len != input.len)
    	{
    		// TODO - This is complex, as we need to resize the entire AcqBLock...
    		// Put a function in BEI maybe?

    		BPSEC_DEBUG_ERR("Cannot handle resizing extension blocks at this time.", NULL);
    		result = ERROR;
    	}
    	else
    	{
    		result = 1;
        	memcpy(input.contents, output.contents, output.len);
        	sdr_write(state->sdr, blk->bytes, (char*)data, blk->length);
    	}

    	MRELEASE(output.contents);
    	MRELEASE(data);
    }


    if((result != ERROR) && (csi_parms.icv.len != 0))
    {
    	sc_value *tag = MTAKE(sizeof(sc_value));

    	if(tag == NULL)
    	{
    		BPSEC_DEBUG_ERR("Unable to allocate tag sc_value.", NULL);
    		result = ERROR;
    	}
    	else
    	{
    	    *tag = bpsec_scv_memCsiConvert(csi_parms.icv, SC_VAL_TYPE_RESULT, BPSEC_BAGSC_RESULT_TAG);

    	    /* Step 8:  Insert the security result.            */
    	    if((lyst_insert_last(state->scStResults, tag)) == NULL)
    	    {
    	        BPSEC_DEBUG_ERR("Unable to append new result.", NULL);
    	        bpsec_scv_clear(0, tag);
    	        MRELEASE(tag);
    	        result = ERROR;
    	    }
    	}
    }
    else if(csi_parms.icv.len == 0)
    {
    	BPSEC_DEBUG_WARN("No integrity check value (authentication tag) produced.", NULL);
    }


    csi_cipherparms_free(csi_parms);


    return result;
}


