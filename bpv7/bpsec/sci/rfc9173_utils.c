/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2022 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: rfc9173_utils.h
 **
 ** Namespace:
 **    bpsec_rfc9173utl_  Utility functions
 **
 ** Description:
 **
 **     This file implements common utilities/functions used by the security
 **     contexts standardized by RFC9173.
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

#include "rfc9173_utils.h"

#include "bpsec_util.h"
#include "sc_value.h"
#include "cbor.h"



/******************************************************************************
 * @brief Constructs the in-memory portions of the RFC9173 auth data.
 *
 * @param[in]     state   The state of this security operation.
 * @param[in]     parm_id Parameter holding the scope flags.
 * @param[in]     tgtBlk  The number of the BIB's current security target block.
 * @param[in]     addData Whether to also add the block-type-specific data to
 *                        the auth data.
 * @param[in|out] bundle  The bundle to which the BIB is being added.
 * @param[in|out] wk      The acquisition area holding the BIB and targets.
 *
 * This function is used to construct authentication data for the security
 * contexts defined in RFC9173. For BIB-HMAC-SHA2 this data is called the
 * IPPT.  For BCB-AES-GCM this data is called the AAD.
 *
 * This function is called for both incoming blocks and
 * outgoing blocks. The case can be differentiated by whether the bundle or
 * the wk area are populated. One or the other must be !NULL.
 *
 * The memory-portions of the data include:
 *  - A Scope Flag
 *  - The Primary Block (if needed)
 *  - The Target Block Header Info (if needed, and target != Primary Block)
 *  - The Security Block Header Info (if needed and target != Primary Block)
 *
 *  The "header" of a block includes the block type, number, and processing
 *  control flags, in accordance with RFC9173.
 *
 * @note
 *   - If the data cannot be constructed, then the returned serialized data
 *     item will have 0 length.
 *   - The target block block-type-specific data is not included as that
 *     data may be very large and not fit into memory. When called by the
 *     BIB-HMAC-SHA2 function to construct the IPPT, this function only
 *     returns the IPPT "start" which is all extra information
 *     aside from the target's block-type-specific data.
 *   - It is assumed that the blocks in the bundle are in their final
 *     ready-to-be-serialized form when this function is called.
 *   - If the add_data flag is set to 1, then the block-type-specific data
 *     of the security target block is added. This can only be set when the
 *     target is an extension block - never for payload or primary blocks.
 *
 * @retval The serialized data value.
 *****************************************************************************/

BpsecSerializeData bpsec_rfc9173utl_authDataBuild(sc_state *state, int parm_id, int tgtBlk, int addData, Bundle *bundle, AcqWorkArea *wk)
{
    uint16_t flags = 0;
    uvast  uvtemp = 0;
    int tgtHdrLen = 0;
    int secHdrLen = 0;
    int priBlkLen = 0;
    int dataLen = 0;

    uint8_t *priBlkData = NULL;
    uint8_t *data = NULL;
    uint8_t tgtHdrData[BPSEC_RFC9173_UTIL_BLK_HDR_MAX_SIZE];
    uint8_t secHdrData[BPSEC_RFC9173_UTIL_BLK_HDR_MAX_SIZE];

    BpsecSerializeData result;
    unsigned char *cursor = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,%d,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                     (uaddr) state, parm_id, tgtBlk, (uaddr) bundle, (uaddr)wk);

    /* Step 0: Initialize and sanity checks. */
    memset(&result, 0, sizeof(BpsecSerializeData));
    if((bundle == NULL) && (wk==NULL))
    {
        BPSEC_DEBUG_ERR("Need bundle or work area.", NULL);
        return result;
    }
    if((addData && (tgtBlk == PayloadBlk)) ||
       (addData && (tgtBlk == PrimaryBlk)))
    {
        BPSEC_DEBUG_ERR("addData can only be set on extension block, not %d", tgtBlk);
        return result;
    }

    /* Step 1: Grab the IPPT Scope Flags from the state parameters. */
    flags = bpsec_rfc9173utl_intParmGet(state, parm_id, BPSEC_RFC9173_UTIL_SCOPE_DEFAULT);

    if((flags & ~BPSEC_RFC9173_UTIL_SCOPE_MASK) != 0)
    {
        BPSEC_DEBUG_ERR("Corrupt scope flags: 0x%x", flags);
        return result;
    }

    if((tgtBlk == PrimaryBlk) &&
    		((flags & BPSEC_RFC9173_UTIL_SCOPE_PRIMARY) ||
    		 (flags & BPSEC_RFC9173_UTIL_SCOPE_TGT_HDR)))
    {
    	BPSEC_DEBUG_ERR("Flag value 0x%x cannot be used with tgt %d.",
    			        flags, tgtBlk);
    	return result;
    }


    /* Step 2: Calculate individual IPPT components, as needed. */
    /* Step 2a: Serialize the primary block, if needed. */
    if(flags & BPSEC_RFC9173_UTIL_SCOPE_PRIMARY)
    {
        priBlkData = (bundle) ? bpsec_util_primaryBlkSerialize(bundle, &priBlkLen) :
                                bpsec_util_primaryBlkSerialize(&(wk->bundle), &priBlkLen);
    }

    /* Step 2b: Add the serialized target block header, if needed. */
    if(flags & BPSEC_RFC9173_UTIL_SCOPE_TGT_HDR)
    {
        tgtHdrLen = (bundle) ? bpsec_rfc9173utl_outBlkHdrSerialize(bundle, tgtBlk, tgtHdrData):
        		               bpsec_rfc9173utl_inBlkHdrSerialize(wk, tgtBlk, tgtHdrData);
    }

    /* Step 2c: Add the serialized security block header, if needed. */
    if(flags & BPSEC_RFC9173_UTIL_SCOPE_SEC_HDR)
    {
        secHdrLen = (bundle) ? bpsec_rfc9173utl_outBlkHdrSerialize(bundle, state->scSecBlkNum, secHdrData):
        		               bpsec_rfc9173utl_inBlkHdrSerialize(wk, state->scSecBlkNum, secHdrData);
    }

    /* Step 2d: Add the block data to the auth if requested. */
    if(addData)
    {
    	data = (bundle) ? bpsec_rfc9173utl_outExtBlkDataGet(bundle, tgtBlk, &dataLen) :
    			          bpsec_rfc9173utl_inExtBlkDataGet(wk, tgtBlk, &dataLen);
    }

    /*
     * Step 3: Allocate total buffer to hold the IPPT-related data. This will
     *         always be at least 1 byte because the IPPT flags are always
     *         included in the IPPT itself.
     */
    result.scSerializedLength = 1 + priBlkLen + tgtHdrLen + secHdrLen + dataLen;
    cursor = result.scSerializedText = MTAKE(result.scSerializedLength);

    if(result.scSerializedText == NULL)
    {
        BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", result.scSerializedLength);
        MRELEASE(priBlkData);
        MRELEASE(data);
        result.scSerializedLength = 0;
        return result;
    }


    /* Step 4: Copy in the parts of the IPPT. */
    uvtemp = flags;
    oK(cbor_encode_integer(uvtemp, &cursor));

    if(flags & BPSEC_RFC9173_UTIL_SCOPE_PRIMARY)
    {
        memcpy(cursor, priBlkData, priBlkLen);
        cursor += priBlkLen;
        MRELEASE(priBlkData);
    }

    if(flags & BPSEC_RFC9173_UTIL_SCOPE_TGT_HDR)
    {
        memcpy(cursor, tgtHdrData, tgtHdrLen);
        cursor += tgtHdrLen;
    }

    if(flags & BPSEC_RFC9173_UTIL_SCOPE_SEC_HDR)
    {
        memcpy(cursor, secHdrData, secHdrLen);
        cursor += secHdrLen;
    }

    if(data != NULL)
    {
    	memcpy(cursor, data, dataLen);
    	cursor += dataLen;
    	MRELEASE(data);
    }

    /*
     * For debugging
     *
     * char *tmp_str = bpsec_scutl_hexStrCvt(result.scSerializedText, result.scSerializedLength);
     * BPSEC_DEBUG_INFO("Len %d is 0x%s.", result.scSerializedLength, tmp_str);
     * MRELEASE(tmp_str);
     *
     */

    /* Step 6: Return constructed data. */
    BPSEC_DEBUG_PROC("--> Returning data of length %d", result.scSerializedLength);
    return result;
}




/******************************************************************************
 * @brief Constructs the in-memory representation of an outbound block's
 *        block-type-specific data.
 *
 * @param[in]   bundle  The bundle object holding the block.
 * @param[in]   blkNbr  The block whose data is being returned.
 * @param[out]  len     The length of the returned value.
 *
 * When security contexts are called for a bundle, the blocks in the bundle have
 * already been written to the SDR. This function extracts the block from the SDR,
 * calculates the block-type-specific data portion of the block, then then copies
 * it into memory for use by security contexts.
 *
 * @note
 *   - This function is only used for extension blocks - never the Primary Block or
 *     the Payload Block.
 * @note
 *   - TODO: Move this into bei.c, this is a generic extension block function.
 *
 * @retval  !NULL  The serialized data value.
 * @retval   NULL  The data value could not be serialized
 *****************************************************************************/

uint8_t *bpsec_rfc9173utl_outExtBlkDataGet(Bundle *bundle, uint8_t blkNbr, int *len)
{
    ExtensionBlock blk;
    Sdr    sdr = getIonsdr();
    Object blkObj = 0;
    uint8_t *result = NULL;
    uint8_t *data = NULL;


    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC")", (uaddr)bundle, blkNbr, (uaddr)len);

    /* Step 0 - Sanity Checks. */
    CHKNULL(bundle);
    CHKNULL(len);

    if((blkNbr == PayloadBlk) || (blkNbr == PrimaryBlk))
    {
        BPSEC_DEBUG_ERR("Cannot target %d.", blkNbr);
        return NULL;
    }

    /*
     * Step 1: Find the extension block from the bundle's
     *         list of extension blocks.
     *
     *         TODO: Can we just retrieve the ExtensionBlock
     *               object instead of getting Obj and doing
     *               the read?
     */
    if((blkObj = getExtensionBlock(bundle, blkNbr)) == 0)
    {
    	BPSEC_DEBUG_ERR("Cannot find block number %d.", blkNbr);
    	return NULL;
	}
    sdr_read(sdr, (char *) &blk, blkObj, sizeof(ExtensionBlock));


    /*
     * Step 2: The Extension Block structure holds the SDR location
     *         of the constructed block (including the block-type-specific
     *         data).  Allocate memorty for this and read it from the
     *         SDR.
     */
    if((data = MTAKE(blk.length)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", blk.length);
    	return NULL;
    }
	sdr_read(sdr, (char*)data, blk.bytes, blk.length);


	/*
	 * Step 3: Extract the block-type-specific payload from the
	 *         read-in bytes representing the extension block.
	 */
	if((result = MTAKE(blk.dataLength)) == NULL)
	{
		BPSEC_DEBUG_ERR("Cannot allocate %d bytes.", blk.dataLength);
		MRELEASE(data);
		return NULL;
	}
	memcpy(result, data + (blk.length - blk.dataLength), blk.dataLength);
	MRELEASE(data);

	/* Step 4: Cleanup. */
	*len = blk.dataLength;

	/* Step 5: Return the length of the buffer. */
    BPSEC_DEBUG_PROC("-->"ADDR_FIELDSPEC, (uaddr)result);
    return result;
}



/******************************************************************************
 * @brief Construct the RFC9173 block header for a given outbound block.
 *
 * @param[in]  bundle  The bundle holding the block.
 * @param[in]  blkNbr  The block whose header is being serialized.
 * @param[out] buffer  The serialized block header.
 *
 * The serialized portions of a block header (from RFC9173) include:
 *  - The Block Type (0-255)
 *  - The Block Number (up to 8 bytes)
 *  - The Block Processing Control Flags (up to 8 bytes)
 *
 * @note
 *   - It is assumed that the buffer is pre-allocated by the calling function
 *     and has a size of BPSEC_BHSSC_BLK_HDR_MAX_SIZE.
 *   - The block type and block number of the payload block are always 1.
 *   - This function cannot be called for the primary block.
 *
 * @retval >0 - The length of the serialized value
 * @retval -1 - Error
 *****************************************************************************/

int bpsec_rfc9173utl_outBlkHdrSerialize(Bundle *bundle, uint8_t blkNbr, uint8_t *buffer)
{
    uvast  uvtemp = 0;
    uint8_t *cursor = NULL;
    ExtensionBlock blk;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC")", (uaddr)bundle, blkNbr, (uaddr)buffer);

    /* Step 0 - Sanity Checks. */
    CHKERR(bundle);
    CHKERR(buffer);

    if(blkNbr == 0)
    {
        BPSEC_DEBUG_ERR("Cannot target Primary block.", NULL);
        return ERROR;
    }

    /* Step 1: Set up a cursor into the user-provided buffer. */
    cursor = buffer;

    /*
     * Step 2: Populate the header. ION treats the payload block
     *         differently than other blocks, so we must extract
     *         payload block information separately from how we
     *         would extract other extension block information.
     */
    if(blkNbr != PayloadBlk)
    {
        Sdr    sdr = getIonsdr();
        Object blkObj = 0;

        /*
         * Step 2.1: Canonicalize the extension block header.
         */
        if((blkObj = getExtensionBlockObj(bundle, blkNbr)) == 0)
        {
            BPSEC_DEBUG_ERR("Cannot find block type %d.", blkNbr);
            return ERROR;
        }
        sdr_read(sdr, (char *) &blk, blkObj, sizeof(ExtensionBlock));

        uvtemp = blk.type;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = blk.number;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = blk.blkProcFlags;
        oK(cbor_encode_integer(uvtemp, &cursor));
    }
    else
    {
    	/*
    	 * Step 2.2 - The payload block header information can be
    	 *            constructed with hard-coded values for type and
    	 *            number since they do not change.
    	 */
        uvtemp = PayloadBlk;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = PayloadBlk;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = bundle->payloadBlockProcFlags;
        oK(cbor_encode_integer(uvtemp, &cursor));
    }

    /* Step 3: Return the length of the buffer. */
    BPSEC_DEBUG_PROC("--> %d", cursor-buffer);
    return cursor - buffer;
}



/******************************************************************************
 * @brief Constructs the in-memory representation of an inbound block's
 *        block-type-specific data.
 *
 * @param[in]   wk      The work area holding the incoming extension block
 * @param[in]   blkNbr  The block whose data is being returned.
 * @param[out]  len     The length of the returned value.
 *
 * When security contexts are called for an incoming bundle, the blocks have not
 * yet been written to the SDR. This function extracts the block from the
 * memory working area and copies it into a memory area that can be used by
 * security contexts.
 *
 * @note
 *   - This function is only used for extension blocks - never the Primary Block or
 *     the Payload Block.
 * @note
 *   - TODO: Move this into bei.c, this is a generic extension block function.
 *
 * @retval  !NULL  The serialized data value.
 * @retval   NULL  The data value could not be serialized
 *****************************************************************************/

uint8_t *bpsec_rfc9173utl_inExtBlkDataGet(AcqWorkArea *wk, uint8_t blkNbr, int *len)
{
    uint8_t *result = NULL;
    AcqExtBlock *blk = NULL;
    LystElt elt = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC")", (uaddr)wk, blkNbr, (uaddr)len);

    /* Step 0 - Sanity Checks. */
    CHKNULL(wk);
    CHKNULL(len);

    if((blkNbr == PayloadBlk) || (blkNbr == PrimaryBlk))
    {
    	BPSEC_DEBUG_ERR("Cannot target %d.", blkNbr);
    	return NULL;
    }

    /*
     * Step 1: Retrieve the ELT of the extension block entry in
     *         the working area's list of extension blocks.
     */
    if((elt = getAcqExtensionBlock(wk, blkNbr)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot get block number %d", blkNbr);
    	return NULL;
    }

    /* Step 2: Grab the extension block from the ELT. */
    if((blk = (AcqExtBlock *) lyst_data(elt)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot get block number %d", blkNbr);
    	return NULL;
    }

    /* Step 3: Allocate space to put the block-type-specific data. */
    if((result = MTAKE(blk->dataLength)) == NULL)
    {
    	BPSEC_DEBUG_ERR("Cannot allocate %d bytes", blk->dataLength);
    	return NULL;
    }

    /* Step 4: Populate outbound data. */
    memcpy(result, blk->bytes + (blk->length-blk->dataLength), blk->dataLength);
    *len = blk->dataLength;


    /* Step 5: Return the length of the buffer. */
    BPSEC_DEBUG_PROC("-->"ADDR_FIELDSPEC,(uaddr)result);
    return result;
}



/******************************************************************************
 * @brief Construct the RFC9173 block header for a given inbound block.
 *
 * @param[in]  wk      The inbound acquisition area.
 * @param[in]  blkNbr  The block whose header is being serialized.
 * @param[out] buffer  The serialized block header.
 *
 * The serialized portions of a block header (from RFC9173) include:
 *  - The Block Type (0-255)
 *  - The Block Number (up to 8 bytes)
 *  - The Block Processing Control Flags (up to 8 bytes)
 *
 * @note
 *   - It is assumed that the buffer is pre-allocated by the calling function
 *     and has a size of BPSEC_BHSSC_BLK_HDR_MAX_SIZE.
 *   - The block type and block number of the payload block are always 1.
 *   - This function cannot be called for the primary block.
 *
 * @retval >0 - The length of the serialized value
 * @retval -1 - Error
 *****************************************************************************/

int bpsec_rfc9173utl_inBlkHdrSerialize(AcqWorkArea *wk, uint8_t blkNbr, uint8_t *buffer)
{
    uvast  uvtemp = 0;
    uint8_t *cursor = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC")", (uaddr)wk, blkNbr, (uaddr)buffer);

    /* Step 0 - Sanity Checks. */
    CHKERR(wk);
    CHKERR(buffer);

    if(blkNbr == 0)
    {
        BPSEC_DEBUG_ERR("Cannot target Primary block.", NULL);
        return ERROR;
    }

    /* Step 1: Set up a cursor into the user-provided buffer. */
    cursor = buffer;


    /*
     * Step 2: Populate the header. ION treats the payload block
     *         differently than other blocks, so we must extract
     *         payload block information separately from how we
     *         would extract other extension block information.
     */
    if(blkNbr != PayloadBlk)
    {
        AcqExtBlock *blk = NULL;
        LystElt elt = getAcqExtensionBlock(wk, blkNbr);

        if(elt == NULL)
        {
            BPSEC_DEBUG_ERR("Cannot get block number %d", blkNbr);
            return ERROR;
        }

        if((blk = (AcqExtBlock *) lyst_data(elt)) == NULL)
        {
            BPSEC_DEBUG_ERR("Cannot get block number %d", blkNbr);
            return ERROR;
        }

        uvtemp = blk->type;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = blk->number;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = blk->blkProcFlags;
        oK(cbor_encode_integer(uvtemp, &cursor));
    }
    else
    {
        uvtemp = PayloadBlk;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = PayloadBlk;
        oK(cbor_encode_integer(uvtemp, &cursor));
        uvtemp = wk->bundle.payloadBlockProcFlags;
        oK(cbor_encode_integer(uvtemp, &cursor));
    }

    /* Step 3: Return the length of the buffer. */
    BPSEC_DEBUG_PROC("--> %d", cursor-buffer);
    return cursor - buffer;
}



/******************************************************************************
 * @brief Generate a session key and a wrapped version of the session key
 *
 * @param[in]   state      The state of the security context
 * @param[in]   kek_id     The ID of the key encrypting key parameter
 * @param[in]   wrap_id    The ID of the wrapped key parameter
 * @param[in]   csi_suite  The CSI Suite needing a session key
 * @param[out]  sesKey     The session key generated to use with the csi_suite.
 * @param[out]  encSesKey  The wrapped sesKey to include in the security block
 *
 * This function will generate a session key for a given cipher suite and then
 * AES keywrap that key using an identified key encrypting key.
 *
 * The resultant wrapped key can be written back to a security block and will
 * be associated with the given parameter ID for wrapped keys in the security block.
 *
 * @retval >0 - The length of the serialized value
 * @retval -1 - Error
 *****************************************************************************/

int bpsec_rfc9173utl_sesKeyGet(sc_state *state, int kek_id, int wrap_id, int csi_suite, csi_val_t *sesKey, sc_value *encSesKey)
{
    sc_value kek;
    csi_val_t csi_kek;
    csi_val_t csi_encKey;


    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,"ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
                    (uaddr) state, csi_suite, (uaddr) sesKey, (uaddr)encSesKey);

    /* Step 0 - Sanity Checks. */
    CHKERR(state);
    CHKERR(sesKey);
    CHKERR(encSesKey);


    /* Step 1: Grab the key-encrypting key we will use for decrypting.*/
    if(bpsec_scutl_keyGet(state, kek_id,  &kek) == ERROR)
    {
        BPSEC_DEBUG_ERR("Unable to get key encrypting key.", NULL);
        return ERROR;
    }
    csi_kek.contents = kek.scRawValue.asPtr;
    csi_kek.len = kek.scValLength;


    /* Step 2: Generate a session key to use with this block. */
    *sesKey = csi_crypt_parm_get(csi_suite, CSI_PARM_BEK);
    if((sesKey->contents == NULL) || (sesKey->len == 0))
    {
        BPSEC_DEBUG_ERR("Can't get session key.", NULL);
        bpsec_scv_clear(0, &kek);
        return ERROR;
    }


    /*
     * Step 3: Use the KEK to encrypt the session key.
     *         AES keywrap/unwrap does not need cipher parameters, so these
     *         can be empty.
     */

    if ((csi_keywrap(1, csi_kek, *sesKey, &csi_encKey)) == ERROR)

    {
        BPSEC_DEBUG_ERR("Can't get encrypted session key.", NULL);
        bpsec_scv_clear(0, &kek);
        MRELEASE(sesKey->contents);
        return ERROR;
    }


    /* Step 4: Release the key-encrypting key, we don't need it anymore. */
    bpsec_scv_clear(0, &kek);


    /* STep 5: Build the wrapped key as an SC value. */
    *encSesKey = bpsec_scv_memCsiConvert(csi_encKey, SC_VAL_TYPE_PARM, wrap_id);

    BPSEC_DEBUG_PROC("--> 1", NULL);
    return 1;
}




/******************************************************************************
 * @brief Retrieves an integer security context parameter
 *
 * @param[in]   state   The state of the security context
 * @param[in]   id      The ID of the needed parameter
 * @param[in]   defVal  Default value if the parm was not found.
 *
 * This function assumes that the security context state has been initialized
 * with the set of available parameters. It will try and find the desired
 * parameters from this list.
 *
 * @retval The found value (or the default)
 *****************************************************************************/

uint16_t bpsec_rfc9173utl_intParmGet(sc_state *state, int id, uint16_t defVal)
{
    sc_value *tmp = NULL;

    BPSEC_DEBUG_PROC("("ADDR_FIELDSPEC",%d,%d)", (uaddr)state, id, defVal);

    /* Step 0: Sanity checks. */
    if(state == NULL)
    {
    	return defVal;
    }

    /* Step 1: If the parameter is found, return it. */
    if((tmp = bpsec_scv_lystFind(state->scStParms, id, SC_VAL_TYPE_PARM)) != NULL)
    {
        uint16_t *ptr = (uint16_t*) tmp->scRawValue.asPtr;
        return *ptr;
    }

    /* Step 2: If the parameter is not found, return the default. */
    BPSEC_DEBUG_WARN("Cannot find parm %d. Using Default.", id);
    return defVal;
}

