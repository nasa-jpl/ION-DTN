/*****************************************************************************
 **
 ** File Name: csi.c
 **
 ** Description: This file defines NULL implementations of the ION crypto
 **              interface.
 **
 **              A NULL interfaces provides NO SECURITY SERVICE and NO
 **              CIPHERSUITE implementation.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/05/15  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 **  02/27/16  E. Birrane     Update to CSI [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 **  03/02/22  L. Babun       Update to check for system errors
 **            C. Dunbar      Update to add automatic logical fail condition
 *****************************************************************************/

#include "platform.h"
#include "csi.h"
#include "csi_debug.h"

// Condition set to fail the NULL CS with logical failure on all function calls
//# define NULL_FAIL



#define NCS_BLOCKSIZE   65000
#define NCS_ICV_LEN     18
#define NCS_LEN_VAL	    20

/************************************************************************
 *                         NULL Crypto Functions                        *
 ************************************************************************/
char *crypto_suite_name = "NULL_SUITES";
char gCsiMsg[GMSG_BUFLEN];      /*  Debug message buffer.  */

/************************************************************************
 *                        NULL Utility Functions                        *
 * *********************************************************************/


int csi_keywrap(int wrap, csi_val_t kek, csi_val_t input, csi_val_t *output)
{
	output->contents = MTAKE(NCS_LEN_VAL);
	output->len = NCS_LEN_VAL;
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: csi_memAlloc
 *
 * \par Allocates memory, assigns it to a pointer based on the length passed 
 *      in.  After assignment, the memory is zeroized.
 *
 * \param[in]     mem   Pointer to the pointer that holds the new memory
 *                      allocation
 * 
 * \param[in]     len   Length in bytes of the new memory to be allocated
 *
 * \par Notes: mem is passed as a double pointer so we can cleanly update 
 *             the original pointer with the new memory location.
 *
 * \return 1 on success, 0 or ERROR on failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/02/22  C. Dunbar      Initial Implementation 
 *****************************************************************************/

static int8_t csi_memAlloc(uint8_t **mem, int32_t *len)
{
    if((*mem = (uint8_t *) MTAKE(*len)) == NULL)
    {
        CSI_DEBUG_ERR("x csi_memAlloc: Cannot allocate result of size %d",
        *len);
        *len = 0;
        return ERROR;
    }

    memset(*mem, 0, *len);

    return 1;
}


/************************************************************************
 *                   BP-version-independent functions                   *
 ************************************************************************/

int csi_init()
{
    return 1;
}

void csi_teardown()
{
    return;
}

/******************************************************************************
 *
 * \par Function Name: csi_blocksize
 *
 * \par Retrieves the block size associated with applying a cipher. This is
 *      necessary in cases where an entire dataset cannot be processed in a single
 *      operation. Generally speaking, if a data volume is less than this size,
 *      then the cipher operation will be performed in a single operation. If
 *      the data volume is larger than this size, then the data volume will
 *      be chunked to this size, with each chunk processed with a ciphersuite
 *      context.
 *
 * \param[in]     suite    The ciphersuite being used.
 *
 * \par Notes:
 *
 * \return Blocksize on success or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_blocksize(csi_csid_t suite)
{
    return NCS_BLOCKSIZE;
}

/******************************************************************************
 *
 * \par Function Name: csi_ctx_len
 *
 * \par Return the maximum length of a context for this given ciphersuite.
 *
 * \param[in]     suite    The ciphersuite context length being queried.
 *
 * \par Notes:
 *  - NULL ciphersuites will not use a context. However, to be able to
 *    evaluate whether calls to this and associated context functions correctly
 *    handle memory, a contrived context length will be returned.
 *
 * \return Context Length on success or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_ctx_len(csi_csid_t suite)
{
    return NCS_LEN_VAL;
}

/******************************************************************************
 *
 * \par Function Name: csi_ctx_free
 *
 * \par Release a ciphersuite context.
 *
 * \param[in]     suite    The ciphersuite whose context is being finished.
 * \param[in/out] context  The context being freed.
 *
 * \par Notes:
 *  - The context MUST NOT be accessed after a call to this function.
 *
 * \return 1 on success or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t csi_ctx_free(csi_csid_t suite, void *context)
{
    CHKZERO(context);
    MRELEASE(context);    
    return 1;
}

/******************************************************************************
 *
 * \par Function Name: csi_sign_res_len
 *
 * \par Return the length of the raw ciphersuite result field.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  context  Cryptographic context
 *
 * \return The length of the security result on success or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_sign_res_len(csi_csid_t suite, void *context)
{
    CHKZERO(context);
    return NCS_LEN_VAL;  
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_parm_get_len
 *
 * \par Report a ciphersuite parameter length.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  parmid   The ciphersuite parameter whose length is queried
 *
 * \par Notes:
 *
 * \return The parameter length on success or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_crypt_parm_get_len(csi_csid_t suite, csi_parmid_t parmid)
{
    return NCS_LEN_VAL;
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_res_len
 *
 * \par Return the length of the raw ciphersuite result field.
 *
 * \param[in]  suite     The ciphersuite being used.
 * \param[in]  context   Cryptographic context
 * \param[in]  blocksize Size information for the cryptofunction
 * \param[in]  svc       Cryptographic service to perform (encrypt or decrypt)
 *
 * \return The length of the security result on success or 0 on failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_crypt_res_len(csi_csid_t suite, void *context, csi_blocksize_t blocksize, csi_svcid_t svc)
{
    CHKZERO(context);
    CHKZERO(blocksize.plaintextLen);

    return blocksize.plaintextLen;
}

/*****************************************************************************
 *                  Functions supporting BPv6 crypto                         *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: csi_build_parms
 *
 * \par Purpose: This utility function builds a set of parameters from an
 *               input parameters buffer. This is, effectively, a deserialization
 *               from an input stream into a paramater-holding structure.
 *
 * \retval The built parameters structure on success or empty parameters on failure.
 *
 * \param[in] buf      The serialized parameters
 * \param[in] len      The length of the serialized parameters
 *
 * \par Notes:
 *      1. If a parameter in the structure is not present in the paramater
 *         stream, the parameter is represented as having length 0.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_cipherparms_t csi_build_parms(unsigned char *buf, uint32_t len)
{
    csi_cipherparms_t result;

    memset(&result, 0, sizeof(csi_cipherparms_t));

    if((buf == NULL) || (len == 0))
    {
        CSI_DEBUG_ERR("x csi_build_parms - Bad buf.", NULL);
        CSI_DEBUG_ERR("x csi_build_parms - Bad len.", len);
        return result;
    }

    CSI_DEBUG_PROC("+ csi_build_parms(0x"ADDR_FIELDSPEC",%d", (uaddr)buf,
            len);

    result.iv = csi_extract_tlv(CSI_PARM_IV, buf, len);
    result.intsig = csi_extract_tlv(CSI_PARM_INTSIG, buf, len);
    result.salt = csi_extract_tlv(CSI_PARM_SALT, buf, len);
    result.icv = csi_extract_tlv(CSI_PARM_ICV, buf, len);
    result.keyinfo = csi_extract_tlv(CSI_PARM_KEYINFO, buf, len);

    CSI_DEBUG_PROC("- csi_build_parms -> parms", NULL);

    return result;
}

/******************************************************************************
 *
 * \par Function Name: csi_extract_tlv
 *
 * \par Purpose: This function searches within a buffer (a ciphersuite
 *               parameters field or a security results field) of an
 *               inbound sbsp block for an information item of specified type.
 *
 * \retval The LV requested.  Len = 0 indicates not found.
 *
 * \param[in] itemNeeded The code number of the type of item to search
 *                       for.  Valid item type codes are defined in
 *                       sbsp.h as SBSP_CSPARM_xxx macros.
 * \param[in] buf        The serialized parameters
 * \param[in] len        The length of the serialized parameters
 *
 * \par Notes:
 *      1. If a parameter in the structure is not present in the parameter
 *         stream, the parameter is represented as having length 0.
 *      2. Each paramater is represented as a type-len-value (TLV) field
 *         where TYPE is a byte, LEN is an SDNV, and VALUE is a blob.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t csi_extract_tlv(uint8_t itemNeeded, uint8_t *buf, uint32_t bufLen)
{
    csi_val_t result;
    uint8_t   *cursor = buf;
    uint8_t   itemType;
    uvast     sdnvLength;
    uvast     longNumber;
    uint32_t  itemLength;

    CSI_DEBUG_PROC("+ csi_extract_tlv(%d, 0x"ADDR_FIELDSPEC",%d)",
                   itemNeeded, (uaddr)buf, bufLen);

    memset(&result,0, sizeof(csi_val_t));

    /* Step 0 - Sanity Check. */
    if((buf == NULL) || (bufLen == 0))
    {
        CSI_DEBUG_ERR("x csi_extract_tlv - Bad Parms.", NULL);
        CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
                result.len);
        return result;
    }

    /**
     *  Step 1 - Walk through all items in the buffer searching for an
     *           item of the indicated type.
     */

    while (bufLen > 0)
    {
        /* Step 1a - Grab the type, which should be the first byte. */
        itemType = *cursor;

        cursor++;
        bufLen--;

        if (bufLen == 0)
        {
            CSI_DEBUG_ERR("x csi_extract_tlv: Read type %d and \
ran out of space.", itemType);
            CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
                    result.len);

            return result;
        }

        /* Step 1b - Grab the length, which is an SDNV. */
        sdnvLength = decodeSdnv(&longNumber, cursor);

        itemLength = longNumber;
        cursor += sdnvLength;
        bufLen -= sdnvLength;

        if (sdnvLength == 0 || sdnvLength > bufLen)
        {
            CSI_DEBUG_ERR("x csi_extract_tlv: Bad Len of %d \
with %d buffer remaining.", sdnvLength, bufLen);
            CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
                    result.len);

            return result;
        }

        /**
         * Step 1c - Evaluate this item. If the item is empty
         * or not a match, skip over it. Otherwise, copy it out
         * and return.                                       */

        if (itemLength == 0)     /*Empty item.      */
        {
            continue;
        }

        if (itemType == itemNeeded)
        {
            if((result.contents = MTAKE(itemLength)) == NULL)
            {
                CSI_DEBUG_ERR("x csi_extract_tlv: Cannot \
allocate size of %d.", itemLength);
                CSI_DEBUG_PROC("- csi_extract_tlv -> result \
(len=%d)", result.len);

                return result;
            }

            memcpy(result.contents, cursor, itemLength);
            result.len = itemLength;

            CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
                    result.len);
            return result;
        }

        /*    Look at next item in buffer.*/

        cursor += itemLength;
        bufLen -= itemLength;
    }

    CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);
    return result;
}

/******************************************************************************
 *
 * \par Function Name: csi_build_tlv
 *
 * \par Purpose: This utility function builds a TLV from individual fields.
 *               A TLV (type-length-value) structure uses one byte for the
 *               type, the length is an SDNV encoded integer, and the
 *               value is a BLOB of length given by the length field.
 *
 * \par Date Written:  2/27/2016
 *
 * \retval The serialized TLV. Length 0 indicates failure.
 *
 * \param[in] id       The type of data being written.
 * \param[in] len      The length of the value field.
 * \param[in] contents The value field.
 *
 * \par Notes:
 *      1. The TLV structure is allocated and must be released.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t csi_build_tlv(uint8_t id, uint32_t len, uint8_t *contents)
{
    csi_val_t result;
    Sdnv      lenSdnv;

    CSI_DEBUG_PROC("+ csi_build_tlv(%d, %d, 0x"ADDR_FIELDSPEC")", id, len,
            (uaddr)contents);

    memset(&result, 0, sizeof(result));

    /* Step 0 - Sanity checks. */

    if((len == 0) || (contents == NULL))
    {
        CSI_DEBUG_ERR("x csi_build_tlv: Bad parms.", NULL);
        CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)",
                result.len);
        return result;
    }

    /* Step 1 - Encode the length of the parameter. */
    encodeSdnv(&lenSdnv, len);

    /* Step 2 - Allocate space for the parameter. */
    result.len = 1 + lenSdnv.length + len;
    if((result.contents = MTAKE(result.len)) == NULL)
    {
        CSI_DEBUG_ERR("x csi_build_tlv: Can't allocate result of length %d.", result.len);
        result.len = 0;
        CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)",
                result.len);
        return result;
    }

    /* Step 3 - Populate parameter. */
    result.contents[0] = id;
    memcpy(&(result.contents[1]), lenSdnv.text, lenSdnv.length);
    memcpy(&(result.contents[1+lenSdnv.length]), contents, len);

    CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", result.len);
    return result;
}

csi_val_t csi_serialize_parms(csi_cipherparms_t parms)
{
    csi_val_t result;
    uint32_t offset = 0;
    csi_val_t iv;
    csi_val_t aad;
    csi_val_t keyinfo;
    csi_val_t salt;
    csi_val_t icv;
    csi_val_t intsig;

    memset(&result, 0, sizeof(csi_val_t));

    if(parms.aad.len <= 0 ||
       parms.icv.len <= 0 ||
       parms.intsig.len <= 0 ||
       parms.iv.len <= 0 || 
       parms.keyinfo.len <= 0 ||
       parms.salt.len <= 0)
    {
        CSI_DEBUG_ERR("x csi_build_tlv: Parms len=%d.", result.len);
        return result;
    }

    /* Step 1 - Initialize the individual TLV fields. */
    memset(&iv, 0, sizeof(csi_val_t));
    memset(&aad, 0, sizeof(csi_val_t));
    memset(&salt, 0, sizeof(csi_val_t));
    memset(&icv, 0, sizeof(csi_val_t));
    memset(&keyinfo, 0, sizeof(csi_val_t));
    memset(&intsig, 0, sizeof(csi_val_t));

    /* Step 2 - Populate TLV fields */
    if(parms.intsig.len > 0)
    {
        intsig = csi_build_tlv(CSI_PARM_INTSIG, parms.intsig.len,
                   parms.intsig.contents);
        result.len += intsig.len;
    }

    if(parms.icv.len > 0)
    {
        icv = csi_build_tlv(CSI_PARM_ICV, parms.icv.len,
                parms.icv.contents);
        result.len += icv.len;
    }

    if(parms.iv.len > 0)
    {
        iv = csi_build_tlv(CSI_PARM_IV, parms.iv.len,
                parms.iv.contents);
        result.len += iv.len;
    }

    if(parms.salt.len > 0)
    {
        salt = csi_build_tlv(CSI_PARM_SALT, parms.salt.len,
                parms.salt.contents);
        result.len += salt.len;
    }

    if(parms.keyinfo.len > 0)
    {
        keyinfo = csi_build_tlv(CSI_PARM_KEYINFO, parms.keyinfo.len,
                parms.keyinfo.contents);
        result.len += keyinfo.len;
    }


    /* Step 3 - Allocate the SDR space. */
    if((result.contents = MTAKE(result.len)) == 0)
    {
        CSI_DEBUG_ERR("csi_serialize_parms: Can't allocate result of length %d.", result.len);
        result.len = 0;
        MRELEASE(intsig.contents);
        MRELEASE(icv.contents);
        MRELEASE(iv.contents);
        MRELEASE(salt.contents);
        MRELEASE(keyinfo.contents);
        return result;
    }

    if(parms.aad.len > 0)
    {
        memcpy(result.contents+offset, (char *) intsig.contents,
                intsig.len);
        offset += intsig.len;
        MRELEASE(intsig.contents);
    }

    if(parms.icv.len > 0)
    {
        memcpy(result.contents+offset, (char *) icv.contents, icv.len);
        offset += icv.len;
        MRELEASE(icv.contents);
    }

    if(parms.iv.len > 0)
    {
        memcpy(result.contents+offset, (char *) iv.contents, iv.len);
        offset += iv.len;
        MRELEASE(iv.contents);
    }

    if(parms.salt.len > 0)
    {
        memcpy(result.contents+offset, (char *) salt.contents,
                salt.len);
        offset += salt.len;
        MRELEASE(salt.contents);
    }

    if(parms.keyinfo.len > 0)
    {
        memcpy(result.contents+offset, (char *) keyinfo.contents,
                keyinfo.len);
        offset += keyinfo.len;
        MRELEASE(keyinfo.contents);
    }

    return result;
}

int8_t csi_crypt_key(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, 
                     csi_val_t longtermkey, csi_val_t input, csi_val_t *output)
{
    int8_t retval;

    output->len = NCS_LEN_VAL;
    retval = csi_memAlloc(&output->contents, &output->len);
    CSI_DEBUG_PROC("- csi_crypt_key ->%d", retval);
    return retval;
}

//2/21
void csi_cipherparms_free(csi_cipherparms_t parms)
{
        
    MRELEASE(parms.aad.contents);
    MRELEASE(parms.icv.contents);
    MRELEASE(parms.intsig.contents);
    MRELEASE(parms.iv.contents);
    MRELEASE(parms.keyinfo.contents);
    MRELEASE(parms.salt.contents);

    return;
}

// 2/21
csi_val_t csi_rand(csi_csid_t suite, uint32_t len)
{
    csi_val_t result;

    memset(&result, 0, sizeof(result)); 

    if (len == 0)
    {
        CSI_DEBUG_ERR("x csi_rand: Can't allocate result of length %d.", len);
        return result;
    }
    
    result.len = len;
    
    csi_memAlloc(&result.contents, &result.len);

    return result;
}

/******************************************************************************
 *
 * \par Function Name: crypt_init
 *
 * \par Initialize a ciphersuite context.
 *
 * \param[in]   suite     The ciphersuite whose context is being initialized.
 * \param[in]   key_info  Key information related to the ciphersuite action.
 * \param[in]   svc       The service being performed.
 *
 * \par Notes:
 *  - This function allocates the context in the ION memory pool.
 *
 * \return NULL or created/initialized context.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t *csi_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc)
{
    uint8_t *context = NULL;
    int32_t temp_len = csi_ctx_len(suite);

    if(csi_memAlloc(&context, &temp_len) <= 0)
    {
        return NULL;
    }

    if(key_info.len > 0)
    {
        memcpy(context, key_info.contents, MIN(temp_len, key_info.len));
    }

    return context;
}

/******************************************************************************
 *
 * \par Function Name: csi_sign_start
 *
 * \par Start a ciphersuite context.
 *
 * \param[in] suite     The ciphersuite whose context is being initialized.
 * \param[in] context   The GCM context being started.
 *
 * \par Notes:
 *  - Starting the context is taken care of in the init function.
 *
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_sign_start(csi_csid_t suite, void *context)
{
    CHKERR(context);

    return 1;
}

/******************************************************************************
 *
 * \par Function Name: csi_sign_update
 *
 * \par Incrementally apply a ciphersuite to a new chunk of input data.
 *
 * \param[in]     suite    The ciphersuite being used.
 * \param[in\out] context  The context being reset
 * \param[in]     data     Current chunk of data.
 * \param[in]     svc      Service being performed (sign or verify)
 *
 * \par Notes:
 *
 * \return 1 on success or ERROR or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc)
{
    CHKERR(context);

    if(data.contents == NULL || data.len <= 0)
    {
        return ERROR;
    }

#ifdef NULL_FAIL
    return 0;
#endif
    
    return 1;
}

/******************************************************************************
 *
 * \par Function Name: csi_sign_finish
 *
 * \par Finish a streaming operation from a ciphersuite.
 *
 * \param[in]     suite    The ciphersuite whose context is being finished.
 * \param[in/out] context  The context being finished.
 * \param[out]    result   The security result.
 * \param[in]     svc      Service being performed (sign or verify)
 *
 * \par Notes:
 *  - It is required that the result be pre-allocated.
 *
 * \return 1 on success or ERROR or 0 on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_sign_finish(csi_csid_t suite, void *context, csi_val_t *result, csi_svcid_t svc)
{
    CHKERR(context);

#ifdef NULL_FAIL
    return 0;
#endif

    result->len = NCS_LEN_VAL;

    return csi_memAlloc(&result->contents, &result->len);
}

/******************************************************************************
 *
 * \par Function Name: csi_sign_full
 *
 * \par Apply a ciphersuite to a given set of input data.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  input    The input to the ciphersuite
 * \param[in]  key      The key to use for the ciphersuite service.
 * \param[out] result   THe result of the ciphersuite operation
 * \param[in]  svc      Cryptographic service to perform (sign or verify)
 *
 * \par Notes:
 *      - The returned output structure MUST be correctly released by the
 *        calling function.
 *
 * \return 1      - Success
 *         0      - Config error
 *         4      - Logical issue (such as verification failure)
 *         ERROR  - System error

 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *result, csi_svcid_t svc)
{
    CHKERR(result);

#ifdef NULL_FAIL
    return 4;
#endif

    memset(result,0,sizeof(csi_val_t));

    result->len = NCS_LEN_VAL;
    return csi_memAlloc(&result->contents, &result->len);
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_finish
 *
 * \par Finish a streaming operation from a ciphersuite.
 *
 * \param[in]     suite    The ciphersuite whose context is being finished.
 * \param[in/out] context  The context being finished.
 * \param[in]     svc      Cryptographic service (encrypt or decrypt)
 * \param[in/out] parms    Ciphersuite parameters.
 *
 * \par Notes:
 *  - The ciphertext is generated incrementally by calls to csi_crypt_update.
 *    When the csi_crypt_finish method is called, only leftover data is
 *    calculated (such as ICV) and added to the parms field.
 *
 * \return 1 on success or 0 or ERROR on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_crypt_finish(csi_csid_t suite, void *context, csi_svcid_t svc, csi_cipherparms_t *parms)
{
    CHKERR(context);
    CHKERR(parms);
    
#ifdef NULL_FAIL
    return 0;
#endif

    parms->icv.len = NCS_ICV_LEN;
    return csi_memAlloc(&parms->icv.contents, &parms->icv.len);;
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_full
 *
 * \par Apply a ciphersuite to a given set of input data.
 *
 * \param[in]      suite  The ciphersuite being used.
 * \param[in]      svc    Cryptographic service to perform (encrypt or decrypt)
 * \param[in|out]  parms  Cryptographic parameters.
 * \param[in]      key    Key to use for this operation
 * \param[in]      input  Data to operate on.
 * \param[out]     output Result of cryptographic operation.
 *
 * \par Notes:
 *      - The returned output structure MUST be correctly released by the
 *        calling function.
 *      - the output structure MUST NOT have any allocated data in it.
 *
 * \return 1 on success or, 0 or ERROR, on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_crypt_full(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, 
                      csi_val_t key, csi_val_t input, csi_val_t *output)
{
    int8_t retval = ERROR;

	CSI_DEBUG_PROC("+ csi_crypt_full(%d, %d, key (len=%d), input(len=%d),"ADDR_FIELDSPEC")",
			       suite, svc, (uaddr)parms, key.len, input.len, (uaddr)output);

	CHKERR(parms);
	CHKERR(output);

#ifdef NULL_FAIL
    return 0;
#endif

    switch(suite)
	{

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		 output->len = input.len;
    
        if(csi_memAlloc(&output->contents, &output->len) == ERROR)
        {
            return ERROR;
        }

        /* TODO: Check return value. */
        memcpy(output->contents, input.contents, MIN(output->len, input.len));
        retval = 1;
		break;

	default:
		CSI_DEBUG_ERR("x csi_crypt_full: Unsupported suite %d.", suite);
		break;
	}

    /* TODO: Check return value. */
    parms->icv.len = NCS_ICV_LEN;
    csi_memAlloc(&parms->icv.contents, &parms->icv.len);

	CSI_DEBUG_PROC("- csi_crypt_full ->%d", retval);
	return retval;

}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_parm_get
 *
 * \par Generate a ciphersuite parameter.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  parmid   The ciphersuite parmeter to generate
 *
 * \par Notes:
 *
 * \return The parameter. Length 0 indicates an failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t csi_crypt_parm_get(csi_csid_t suite, csi_parmid_t parmid)
{
    csi_val_t result;

    result.len = csi_crypt_parm_get_len(suite,  parmid);
    result.contents = NULL;

    csi_memAlloc(&result.contents, &result.len);

    return result;
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_start
 *
 * \par Initialize a ciphersuite context.
 *
 * \param[in] suite     The ciphersuite whose context is being initialized.
 * \param[in] context   The GCM context being started.
 * \param[in] parms     Parameters for this ciphersuite.
 *
 * \par Notes:
 *
 * \return 1 on success or ERROR on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_crypt_start(csi_csid_t suite, void *context, csi_cipherparms_t parms)
{
    CHKERR(context);

    return 1;
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_update
 *
 * \par Incrementally apply a ciphersuite to a new chunk of input data.
 *
 * \param[in]     suite    The ciphersuite being used.
 * \param[in\out] context  The context being reset
 * \param[in]     svc      The servie being applied (encrypt or decrypt)
 * \param[in]     data     Current chunk of data to apply service to.
 *
 * \par Notes:
 *  - This is only used for applying the current ciphertext. Extra data
 *    such as signing is applied when we call gcm_crypt_finish.
 *
 * \return ciphersuite output. Length 0 indicates failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t csi_crypt_update(csi_csid_t suite, void *context, csi_svcid_t svc, csi_val_t data)
{
    csi_val_t result;

    memset(&result, 0, sizeof(result)); 

    if(context == NULL || (data.contents == NULL && data.len <= 0))
    {
        CSI_DEBUG_ERR("x csi_crypt_update: Bad argument.", NULL);
        return result;
    }

#ifdef NULL_FAIL
    return result;
#endif

    result.len = data.len;
    if(csi_memAlloc(&result.contents, &result.len) <= 0)
    {
        return result;
    }
    memcpy(result.contents, data.contents, result.len);
    return result;
}

