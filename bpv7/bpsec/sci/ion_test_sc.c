/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ion_test_sc.c
 **
 ** Namespace: bpsec_itsc_
 **
 ** Description:
 **
 **     This file implements a testing-only security context suitable for basic
 **     testing of ION security block packaging.
 **
 ** Notes:
 **
 **     1. This context is not standardized, and uses the identifier -1.
 **        SC Ids < 0 are reserved by RFC9172 for local/site-specific uses.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/07/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#include "ion_test_sc.h"
#include "bpsec_util.h"
#include "sc_value.h"

sc_value_map gITSC_ParmMap[] = {
    {"key_name", CSI_PARM_KEYINFO, SC_VAL_TYPE_PARM,   bpsec_scvm_strStrDecode, bpsec_scv_clear, bpsec_scvm_strStrEncode, NULL, NULL},
    {"iv",       CSI_PARM_IV,      SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {"salt",     CSI_PARM_SALT,    SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, NULL, NULL},
    {"icv",      CSI_PARM_ICV,     SC_VAL_TYPE_RESULT, bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, NULL, NULL},
    {"intsig",   CSI_PARM_INTSIG,  SC_VAL_TYPE_RESULT, bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {"bek",      CSI_PARM_BEK,     SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
    {"bekicv",   CSI_PARM_BEKICV,  SC_VAL_TYPE_RESULT, bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, NULL, NULL},
    {NULL,-1, SC_VAL_TYPE_UNKNOWN, NULL, NULL, NULL, NULL, NULL}
};



/******************************************************************************
 * @brief Decrypt a target block
 *
 * @param[in]     state     The SC state used for this decryption.
 * @param[in|out] wk        The work area acquiring the encrypted block.
 * @param[in|out] asb       The inbound ASB.
 * @param[in|out] tgtResult The inbound security results associated with the
 *                          decryption.
 * @note
 *   - returning 1 when at a verifier means the BCB was verified.
 *   - returning 1 when at a source means the target was decrypted.
 *
 * @todo
 *   - Consider storing the state parameters as csi_parms to avoid always
 *     converting them.
 *
 * @retval 1  - The target block was processed successfully.
 * @retval 0  - The SOP cannot be processed
 * @retval -1 - System error
 * @retval -2 - Unable to grab session key
 *****************************************************************************/

int bpsec_itscbcb_decrypt(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, BpsecInboundTargetResult *tgtResult)
{
    csi_val_t sessionKeyClear;
    csi_cipherparms_t csi_parms;
    int result = 1;

    BPSEC_DEBUG_PROC("(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC "," ADDR_FIELDSPEC "," ADDR_FIELDSPEC ")",
                     (uaddr) state, (uaddr) wk, (uaddr) asb, (uaddr) tgtResult);


    if(state->scRole != SC_ROLE_ACCEPTOR)
    {
    	BPSEC_DEBUG_INFO("ITSC default passes at verifier.", NULL);
    	return 1;
    }

    /* Step 1 - Convert parameters from the security context state to cipher parms. */
    bpsec_scutl_parmsExtract(state, &csi_parms);

    /* Step 2: Grab the wrapped key if it exists. */
    if(bpsec_scutl_keyUnwrap(state, CSI_PARM_KEYINFO, &sessionKeyClear, CSI_PARM_BEK, BPSEC_ITSC_BCB_SUITE, &csi_parms) == ERROR)
    {
        BPSEC_DEBUG_ERR("Could not decrypt session key", NULL);
        BPSEC_DEBUG_PROC("--> 0", NULL);

        return -2; /* TODO: We should define return values in sci.h? */
    }

    /*
     * Step 3: Decrypt the payload block. The ION test security context only
     *         applies BCBs to the payload.
     */
    switch (tgtResult->scTargetId)
    {
        case 1:        /*    Target block is the payload block.    */

            if (bpsec_itscbcb_compute(&(wk->bundle.payload.content), csi_blocksize(BPSEC_ITSC_BCB_SUITE), BPSEC_ITSC_BCB_SUITE, sessionKeyClear,
                  &csi_parms, CSI_SVC_DECRYPT) == ERROR)
            {
                BPSEC_DEBUG_ERR("Can't decrypt payload.", NULL);
                result = 0;
            }
          break;

        default:    /*    Target block is an extension block.    */
          BPSEC_DEBUG_ERR("The ION Test SC only decrypts payloads.", NULL);
          result = 0;
    }

    /* Step 4: Clean up the session key and return. */
    MRELEASE(sessionKeyClear.contents);
    BPSEC_DEBUG_PROC("--> %d", result);
    return result;
}



/******************************************************************************
 * @brief Extract and/or generate parameters needed to encrypt a target block
 *
 * @param[in]     state     The SC state holding security block parms
 * @param[out]    parms     The CSI parms extracted
 * @param[out]    sesKey    The cleartext session key to use for encryption
 * @param[out]    encSesKey The wrapped session key to go in the security block
 *
 * This function will extract parameters from the security context state that
 * are needed to encrypt a target block.
 *
 * This function will, separately, generate a session key and a wrapped version
 * of the session key.
 *
 * @retval 1  - Parameters were generated/extracted
 * @retval -1 - System error
 *****************************************************************************/

int bpsec_itscbcb_parmsGet(sc_state *state,    csi_cipherparms_t *parms,
                           csi_val_t *sesKey,  csi_val_t *encSesKey)
{
    sc_value kek;
    csi_val_t csi_kek;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                     (uaddr) state, (uaddr) parms, (uaddr) sesKey, (uaddr) encSesKey);

    /* Step 0 - Sanity checks. */
    CHKERR(state);
    CHKERR(parms);
    CHKERR(sesKey);
    CHKERR(encSesKey);


    /* Step 1: Grab the key-encrypting key. */
    if(bpsec_scutl_keyGet(state, CSI_PARM_KEYINFO, &kek) == ERROR)
    {
        BPSEC_DEBUG_ERR("Unable to get key encrypting key.", NULL);
        return ERROR;
    }
    csi_kek.contents = kek.scRawValue.asPtr;
    csi_kek.len = kek.scValLength;


    /* Step 2: Generate a session key to use with this block. */
    *sesKey = csi_crypt_parm_get(BPSEC_ITSC_BCB_SUITE, CSI_PARM_BEK);
    if((sesKey->contents == NULL) || (sesKey->len == 0))
    {
        BPSEC_DEBUG_ERR("Can't get session key.", NULL);
        bpsec_scv_clear(0, &kek);

        BPSEC_DEBUG_PROC("---> %d", ERROR);
        return ERROR;
    }

    /* Step 3: Generate other encryption parameters. */
    bpsec_scutl_parmsExtract(state, parms);


    /*    Now use the long-term key to encrypt the session key.
     *    We assume session key sizes fit into memory and do
     *    not need to be chunked. We want to make sure we can
     *    encrypt all the keys before doing surgery on the
     *    target block itself.                    */

    if ((csi_crypt_key(BPSEC_ITSC_BCB_SUITE, CSI_SVC_ENCRYPT, parms, csi_kek, *sesKey, encSesKey)) == ERROR)
    {
        BPSEC_DEBUG_ERR("Can't get encrypted session key.", NULL);
        bpsec_scv_clear(0, &kek);
        MRELEASE(sesKey->contents);
        BPSEC_DEBUG_PROC("--> %d", ERROR);
        return ERROR;
    }

    // TODO Verify that we should be freeing the KEK here.
    bpsec_scv_clear(0, &kek);
    return 1;
}



/******************************************************************************
 * @brief Extract and/or generate parameters needed to encrypt a target block
 *
 * @param[in]  dataObj    - The serialized data to hash, a ZCO.
 * @param[in]  chunkSize  - The chunking size for going through the bundle
 * @param[in]  suite      - Which ciphersuite to use to caluclate the value.
 * @param[in]  sesKey     - The key to use.
 * @param[in]  parms      - Other ciphersuite parameters
 * @param[in]  function   - Encrypt or Decrypt

 * This function encrypts or decrypts a target block (depending on what function
 * is provided).
 *
 * @todo
 *   - Ingest xmit rate instead of hard-coding it.
 *   - Rethink Scott's original design of limiting SDR tries by xmit rate?
 *
 * @retval 1  - Success
 * @retval -1 - Failure
 *****************************************************************************/

int bpsec_itscbcb_compute(Object *dataObj, uint32_t chunkSize,       uint32_t suite,
                         csi_val_t sesKey, csi_cipherparms_t *parms, uint8_t function)
{
    Sdr sdr = getIonsdr();
    csi_blocksize_t blocksize;
    uint32_t  cipherBufLen = 0;
    uint32_t  bytesRemaining = 0;
    ZcoReader dataReader;
    uint8_t   *context = NULL;
//    Object    cipherBuffer = 0;
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
    blocksize.chunkSize = chunkSize;
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


    BPSEC_DEBUG_INFO("Encrypting target of size %d to cipher buffer of size %d.", bytesRemaining, cipherBufLen);

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
    if (cipherBufLen < BPSEC_ITSC_MIN_FILE_BUFFER)
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
        if (cipherBufLen < BPSEC_ITSC_MIN_FILE_BUFFER)
        {
            /*    Slow down to avoid over-stressing the file system. */
            siestaBytes = BPSEC_ITSC_MIN_FILE_BUFFER - cipherBufLen;
            siestaUsec = (1000000.0 * siestaBytes) / BPSEC_ITSC_XMIT_RATE;
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
        	result = bpsec_util_fileBlkConvert(suite, context, &blocksize,
        			&dataReader,
        			                           cipherBufLen, &cipherZco, BPSEC_ITSC_BCB_FILENAME, function);

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
 * @brief Encrypt a target block
 *
 * @param[in]     state      The SC state used for this encryption.
 * @param[out]    extraParms Any parms this SC adds to the block.
 * @param[in|out] bundle     The bundle holding the block to be encrypted.
 * @param[in|out] asb        The outbound ASB.
 * @param[in|out] tgtResult  The outbound security results associated with the
 *                           encryption.
 *
 * @todo
 *   - Consider storing the state parameters as csi_parms to avoid always
 *     converting them.
 *
 * @retval 1  - The target block was encrypted.
 * @retval 0  - The SOP cannot be processed
 * @retval -1 - System error
 * @retval -2 - Unable to grab session key
 *****************************************************************************/

int bpsec_itscbcb_encrypt(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult)
{
    csi_val_t sessionKey;
    csi_val_t encryptedSessionKey;
    csi_cipherparms_t parms;
    sc_value *sci_encrypted_key;

    /* Step 1 - Retrieve parameters to use for encryption from the SC state. */
    if(bpsec_itscbcb_parmsGet(state, &parms, &sessionKey, &encryptedSessionKey) == ERROR)
    {
        return ERROR;
    }

    /* Step 2 - The ION Test SC only encrypts the payload. */
    if(tgtResult->scTargetId != 1)
    {
    	BPSEC_DEBUG_ERR("The ION Test SC only encrypts payloads.", NULL);
        MRELEASE(sessionKey.contents);
        MRELEASE(encryptedSessionKey.contents);
        return ERROR;
    }

    /* Step 3 - Encrypt the payload block. */
    if(bpsec_itscbcb_compute(&(bundle->payload.content), csi_blocksize(BPSEC_ITSC_BCB_SUITE), BPSEC_ITSC_BCB_SUITE, sessionKey, &parms,
                            CSI_SVC_ENCRYPT) == ERROR)
    {
        BPSEC_DEBUG_ERR("encryption of extension block (%d) is not yet implemented.", tgtResult->scTargetId);
        MRELEASE(sessionKey.contents);
        MRELEASE(encryptedSessionKey.contents);
        return ERROR;
    }

    /* Step 4 - Free the clear session key, we don't need it after encrypting. */
    MRELEASE(sessionKey.contents);

    /* Step 5 - Store the encrypted key as a SC parameter. */
    sci_encrypted_key = MTAKE(sizeof(sc_value));
    sci_encrypted_key->scValId = CSI_PARM_BEK;
    sci_encrypted_key->scValLoc = SC_VAL_STORE_MEM;
    sci_encrypted_key->scValType = SC_VAL_TYPE_PARM;
    sci_encrypted_key->scValLength = encryptedSessionKey.len;
    sci_encrypted_key->scRawValue.asPtr = encryptedSessionKey.contents;

    lyst_insert_last(extraParms, sci_encrypted_key); // TODO check return

    return 1;
}



/******************************************************************************
 * @brief Calculate a digest for a target block.
 *
 * @param[in] dataObj   The target block whose digest is being calculated.
 * @param[in] key_Value The key to use for signing the digest
 * @param[in] svc         - Service being performed: SIGN or VERIFY
 *
 * This function calculates a digest over a given extension block and
 * returns the CSI context associated with that digest creation.
 *
 * @retval !NULL - The CSI context
 * @retval NULL  - There was a processing error.
 *****************************************************************************/

uint8_t *bpsec_itscbib_compute(Object dataObj, sc_value *key_value, csi_svcid_t svc)
{
    Sdr sdr = getIonsdr();
    char *dataBuffer = NULL;
    ZcoReader dataReader;
    unsigned int bytesRemaining = 0;
    unsigned int bytesRetrieved = 0;
    uint32_t chunkSize = csi_blocksize(BPSEC_ITSC_BIB_SUITE);
    csi_val_t chunkData;
    uint8_t *csi_ctx = NULL;
    csi_val_t csi_key;
    int success = 1;

    /* Step 0 - Sanity check. */
    CHKNULL(key_value);

    /* Step 1 - Wrap the key in a CSI structure. */
    csi_key.contents = key_value->scRawValue.asPtr;
    csi_key.len = key_value->scValLength;

    /* Step 2 - Allocate a working buffer and wrap it in a CSI structure. */
    if ((dataBuffer = MTAKE(chunkSize)) == NULL)
    {
        BPSEC_DEBUG_ERR("Can't allocate buffer of size %d.", chunkSize);
        return NULL;
    }
    chunkData.contents = (uint8_t *) dataBuffer;
    chunkData.len = chunkSize;

    /* Step 3 - Create the CSI context for the signing. */
    csi_ctx = csi_ctx_init(BPSEC_ITSC_BIB_SUITE, csi_key, CSI_SVC_SIGN);
    if(csi_sign_start(BPSEC_ITSC_BIB_SUITE, csi_ctx) == ERROR)
    {
        BPSEC_DEBUG_ERR("Can't start context.", NULL);
        csi_ctx_free(BPSEC_ITSC_BIB_SUITE, csi_ctx);
        MRELEASE(dataBuffer);
        return NULL;
    }

    /* Step 4 - Setup playback of data from the data object. */
    if ((bytesRemaining = zco_length(sdr, dataObj)) <= 0)
    {
        BPSEC_DEBUG_ERR("Data object has no length.",  NULL);
        csi_ctx_free(BPSEC_ITSC_BIB_SUITE, csi_ctx);
        MRELEASE(dataBuffer);
        return NULL;
    }

    if ((sdr_begin_xn(sdr)) == 0)
    {
        BPSEC_DEBUG_ERR("Can't start txn.", NULL);
        csi_ctx_free(BPSEC_ITSC_BIB_SUITE, csi_ctx);
        MRELEASE(dataBuffer);
        return NULL;
    }

    zco_start_transmitting(dataObj, &dataReader);

    BPSEC_DEBUG_INFO("Signing %d bytes.", bytesRemaining);

    /* Step 5 - Loop through the data in chunks, updating the context. */
    while (bytesRemaining > 0)
    {
    	/*
    	 * Step 5.1 If this is the last bits, it might not be a
    	 * full chunk size.
    	 */
        if (bytesRemaining < chunkSize)
        {
            chunkSize = bytesRemaining;
            chunkData.len = chunkSize;
        }

        /* Step 5.2: Retrieve the next chunk of data from the ZCO. */
        bytesRetrieved = zco_transmit(sdr, &dataReader, chunkSize, dataBuffer);
        if (bytesRetrieved != chunkSize)
        {
            BPSEC_DEBUG_ERR("Read %d bytes, but expected %d.", bytesRetrieved, chunkSize);
            success = 0;
            break;
        }

        /* Step 5.3: Add the data to the context.        */
		if(csi_sign_update(BPSEC_ITSC_BIB_SUITE, csi_ctx, chunkData, svc) == ERROR)
		{
            BPSEC_DEBUG_ERR("Read %d bytes, but expected %d.", bytesRetrieved, chunkSize);
            success = 0;
            break;
		}

		bytesRemaining -= bytesRetrieved;
		BPSEC_DEBUG_INFO("Read %d and %d remaining.", bytesRetrieved, bytesRemaining);
    }

    /* Step 6: Cleanup */
    sdr_exit_xn(sdr);
    MRELEASE(dataBuffer);

    if(success == 0)
    {
        csi_ctx_free(BPSEC_ITSC_BIB_SUITE, csi_ctx);
        csi_ctx = NULL;
    }

    BPSEC_DEBUG_PROC("-->"ADDR_FIELDSPEC,(uaddr)csi_ctx);

    return csi_ctx;
}



/******************************************************************************
 * @brief Sign a target block
 *
 * @param[in]     state      The SC state used for this signing.
 * @param[out]    extraParms Any parms this SC adds to the block.
 * @param[in|out] bundle     The bundle holding the block to be signed.
 * @param[in|out] asb        The outbound ASB.
 * @param[in|out] tgtResult  The outbound security results associated with the
 *                           encryption.
 *
 * @todo
 *   - Consider storing the state parameters as csi_parms to avoid always
 *     converting them.
 *
 * @retval 1  - The target block was signed.
 * @retval 0  - The SOP cannot be processed
 * @retval -1 - System error
 * @retval -2 - Unable to grab session key
 *****************************************************************************/

int bpsec_itscbib_sign(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult)
{
    Object targetZco = 0;
    int length = 0;
    uint8_t *csi_ctx = NULL;
    sc_value key_value;
    sc_value *digest = NULL;
   	csi_val_t csi_result;
   	int result = 0;

    /*
     * Step 1: Get they key to use for signing.
     */
    if(bpsec_scutl_keyGet(state, CSI_PARM_KEYINFO, &key_value) == ERROR)
    {
    	return ERROR;
    }

    if((tgtResult->scTargetId != PayloadBlk) && (tgtResult->scTargetId != PrimaryBlk))
    {
    	BPSEC_DEBUG_ERR("ION Test SC only signs primary and payload blocks.", NULL);
    	return ERROR;
    }

    /*
     * Step 2: Generate a canonical version of the block being signed. This is places in a
     *         zero-copy object, as the outgoing data is already either in the SDR or on the
     *         file system.
     */
    length = bpsec_util_canonicalizeOut(bundle, tgtResult->scTargetId, &targetZco);
    if (length < 1)
    {
    	bpsec_scv_clear(0, &key_value);
    	if(targetZco != 0)
    	{
    		zco_destroy(state->sdr, targetZco);
    	}
        return ERROR;
    }

    /*
     * Step 3: Compute the signature. The value of this is kept in
     *         the CSI context.
     */
    csi_ctx = bpsec_itscbib_compute(targetZco, &key_value, CSI_SVC_SIGN);
    zco_destroy(state->sdr, targetZco);
    bpsec_scv_clear(0, &key_value);

    /*
     * Step 4: Finish the context. When this is called for signing, the
     *         computed signature is copied out.
     */
	result = csi_sign_finish(BPSEC_ITSC_BIB_SUITE, csi_ctx, &csi_result, CSI_SVC_SIGN);
	csi_ctx_free(BPSEC_ITSC_BIB_SUITE, csi_ctx);

	if(result == ERROR)
	{
		return ERROR;
	}

	/*
	 * Step 5: Allocate a security result SCI value, populate it, and
	 *         add it to the target result lists for this security operation.
	 */
	if((digest = MTAKE(sizeof(sc_value))) == NULL)
	{
		BPSEC_DEBUG_ERR("Unable to allocate digest.", NULL);
		return ERROR;
	}

	digest->scValId = CSI_PARM_INTSIG;
	digest->scValLoc = SC_VAL_STORE_MEM;
	digest->scValType = SC_VAL_TYPE_RESULT;
	digest->scValLength = csi_result.len;
	digest->scRawValue.asPtr = csi_result.contents;

	BPSEC_DEBUG_INFO("Generated digest with length %d.", digest->scValLength);

    /* Step 4:  Insert the security result.            */
    if((lyst_insert_last(state->scStResults, digest)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Unable to append new result.", NULL);
    	bpsec_scv_clear(0, digest);
        MRELEASE(digest);
        return ERROR;
    }

    BPSEC_DEBUG_PROC("-->1", NULL);
    return 1;
}



/******************************************************************************
 * @brief Verify a signed target block
 *
 * @param[in]     state     The SC state used for this verification.
 * @param[in|out] wk        The acquisition area holding the signed target block
 * @param[in|out] asb       The inbound ASB.
 * @param[in|out] tgtResult The inbound security results associated with the
 *                          encryption.
 *
 * @todo
 *   - Consider storing the state parameters as csi_parms to avoid always
 *     converting them.
 *
 * @retval 1  - The digest verified.
 * @retval 0  - The digest did not verify.
 * @retval -1 - System error
 *****************************************************************************/

int bpsec_itscbib_verify(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, BpsecInboundTargetResult *tgtResult)
{
    Object targetZco = 0;
    int length = 0;
    int result = 0;
	sc_value *assertedDigest;
	sc_value key_value;
    uint8_t *csi_ctx = NULL;
    csi_val_t csi_digest;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",wk,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                    (uaddr) state, (uaddr) asb, (uaddr) tgtResult);

    /*
     * Step 1: Get they key to use for signing.
     */
    if(bpsec_scutl_keyGet(state, CSI_PARM_KEYINFO, &key_value) == ERROR)
    {
        BPSEC_DEBUG_ERR("Error retrieving key value.", NULL);
    	return ERROR;
    }

    /* Step 2 - Retrieve the asserted digest from the security block. */
	if((assertedDigest = bpsec_scv_lystFind(tgtResult->scIndTargetResults, CSI_PARM_INTSIG, SC_VAL_TYPE_RESULT)) == NULL)
	{
		BPSEC_DEBUG_ERR("No digest found for target %d", tgtResult->scTargetId);
		return ERROR;
	}

    /*
     * Step 3 - Generate a canonical version of the block being signed. This is placed in a
     *          zero-copy object.
     */
    length = bpsec_util_canonicalizeIn(wk, tgtResult->scTargetId, &targetZco);
    if (length < 1)
    {
    	bpsec_scv_clear(0, &key_value);
        zco_destroy(state->sdr, targetZco);
        result = -1;
    }

    /* Step 4 - Calculate a signature over the target block and verify it. */
    csi_ctx = bpsec_itscbib_compute(targetZco, &key_value, CSI_SVC_VERIFY);

    zco_destroy(state->sdr, targetZco);
    bpsec_scv_clear(0, &key_value);
    /*
     * Step 5: Finish the context. When this is called for signing, the
     *         computed signature is copied out.
     */
    csi_digest.len = assertedDigest->scValLength;
    csi_digest.contents = assertedDigest->scRawValue.asPtr;
	result = csi_sign_finish(BPSEC_ITSC_BIB_SUITE, csi_ctx, &csi_digest, CSI_SVC_VERIFY);
	csi_ctx_free(BPSEC_ITSC_BIB_SUITE, csi_ctx);

	BPSEC_DEBUG_PROC("Returning %d.", (result == 1))
	return (result == 1);
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

int   bpsec_itsci_initAsbFn(void *def, Bundle *bundle, BpsecOutboundASB *asb, Sdr sdr, PsmPartition wm, PsmAddress parms)
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
 * @brief Process a security operation on an inbound security block
 *
 * @param[in]     state     The SC state for security block processing.
 * @param[in|out] wk        Acquisition area for the inbound block
 * @param[in|out] asb       The ASB for the security block.
 * @param[in]     tgtBlkElt The ELT for the target block.
 * @param[out]    tgtResult Any generated security results.
 *
 * This function takes in an sc_state that has been previously initialized
 * with a call to bpsec_sc_stateInit. This function is called by a
 * security verifier or security acceptor as part of processing an existing
 * security operation in a received security block.
 *
 * @retval 1  - The security block was processed
 * @retval 0  - There was a logic error
 * @retval -1 - System error
 *****************************************************************************/

int bpsec_itsci_procInBlk(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, LystElt tgtBlkElt, BpsecInboundTargetResult *tgtResult)
{
    CHKERR(state);

    switch(state->scStAction)
    {
        case SC_ACT_VERIFY:
        	return bpsec_itscbib_verify(state, wk, asb, tgtResult);
        case SC_ACT_DECRYPT:
            return bpsec_itscbcb_decrypt(state, wk, asb, tgtResult);
        default:
            return -1;
    }
}




/******************************************************************************
 * @brief Process a security operation on an outbound security block
 *
 * @param[in]     state      The SC state for security block processing.
 * @param[out]    extraParms Extra parms that this SC adds to the block.
 * @param[in|out] bundle     The bundle holding the outbound block
 * @param[in|out] asb        The ASB for the security block.
 * @param[out]    tgtResult  Any generated security results.
 *
 * This function takes in an sc_state that has been previously initialized with
 * a call to bpsec_sc_stateInit. This function is called by a security source
 * as part of adding a security operation to a block.
 *
 * @retval 1  - The security block was processed
 * @retval 0  - There was a logic error
 * @retval -1 - System error
 *****************************************************************************/

int bpsec_itsci_procOutBlk(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult)
{
    CHKERR(state);

    switch(state->scStAction)
    {
        case SC_ACT_SIGN:
            return bpsec_itscbib_sign(state, extraParms, bundle, asb, tgtResult);
        case SC_ACT_ENCRYPT:
        	return bpsec_itscbcb_encrypt(state, extraParms, bundle, asb, tgtResult);
        default:
            return -1;
    }
}



/******************************************************************************
 * @brief Retrieves the sc value map for this SC
 *
 * @retval !NULL - The value map for this SC
 * @retval  NULL - There was an error.
 *****************************************************************************/
sc_value_map* bpsec_itsci_valMapGet()
{
    return gITSC_ParmMap;
}


















