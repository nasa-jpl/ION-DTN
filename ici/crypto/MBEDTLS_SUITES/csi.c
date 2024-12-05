/*****************************************************************************
 **
 ** File Name: csi.c
 **
 ** Description: This file defines the ION crypto interface.
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
 **  02/05/16  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/


#ifdef NULL_CRYPTO_SUITES
#error "Cannot compile MBEDTLS ciphersuites with NULL ciphersuites"
#endif

#include "platform.h"
#include "csi.h"

#include "mbedtls/entropy.h"
#include "mbedtls/nist_kw.h"

#include "csi_hsha.h"
#include "csi_ecdsa.h"
#include "csi_gcm.h"

/*****************************************************************************
 *                         NULL Crypto Functions                             *
 *****************************************************************************/
char *crypto_suite_name = "MBEDTLS_SUITES";


mbedtls_entropy_context g_csi_entropy;

static uint8_t g_csi_init = 0;

#if (CSI_DEBUGGING == 1)
char	gCsiMsg[GMSG_BUFLEN];		/*	Debug message buffer.	*/
#endif


#define CSI_CHK if(g_csi_init == 0) csi_init();

/**
extern int csi_key_gen(csi_csid_t suite, csi_val_t *result)
{
	CHKERR(result);

	CSI_CHK

	memset(result, 0, sizeof(result));

	switch(suite)
	{
        case CSTYPE_HMAC_SHA1:
	    case CSTYPE_HMAC_SHA256:
	    case CSTYPE_HMAC_SHA384:
	    case CSTYPE_HMAC_SHA512:
		    return hsha_key_gen(suite, result);

	    case CSTYPE_SHA256_AES128:
	    case CSTYPE_SHA384_AES256:
	    case CSTYPE_AES128_GCM:
	    case CSTYPE_AES256_GCM:
	    	return gcm_key_gen(suite, result);

	    default:
	    	CSI_DEBUG_ERR("x csi_key_gen: Unsupported suite: %d.", suite);
	    	return -1;
	    	break;
	}

	return 1;
}
**/



int csi_keywrap(int wrap, csi_val_t kek, csi_val_t input, csi_val_t *output)
{
	int result = 0;
	size_t result_len;

	mbedtls_nist_kw_context kw_ctx;

	mbedtls_nist_kw_init(&kw_ctx);

	output->len = 2 * input.len;
	if((output->contents = MTAKE(output->len)) == NULL)
	{
		CSI_DEBUG_ERR("x csi_keyWrap: Cannot allocate %d.", output->len);
		return -1;
	}

	result = mbedtls_nist_kw_setkey(&kw_ctx, MBEDTLS_CIPHER_ID_AES, kek.contents, kek.len*8, wrap);
	if(result != 0)
	{
		CSI_DEBUG_ERR("x csi_keyWrap: Failed to set key. Return is %d. kek bits len is %d", result, kek.len*8);
		MRELEASE(output->contents);
		output->contents = NULL;
		output->len = 0;
		mbedtls_nist_kw_free(&kw_ctx);
		return -1;
	}

	if(wrap)
	{
		result = mbedtls_nist_kw_wrap(&kw_ctx, MBEDTLS_KW_MODE_KW, input.contents, input.len,
				                      output->contents, &result_len, output->len);
	}
	else
	{
		result = mbedtls_nist_kw_unwrap(&kw_ctx, MBEDTLS_KW_MODE_KW, input.contents, input.len,
				                      output->contents, &result_len, output->len);
	}

	mbedtls_nist_kw_free(&kw_ctx);

	if(result != 0)
	{
		CSI_DEBUG_ERR("x csi_keyWrap: Failed to (un)wrap key. Result is %d. Input len %d. Output len is %d. Result len is %ld.",
				result, input.len, output->len, result_len);
		MRELEASE(output->contents);
		output->contents = NULL;
		output->len = 0;
		result = -1;
	}
	else
	{
		output->len = result_len;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: csi_build_parms
 *
 * \par Purpose: This utility function builds a set of parameters from an
 *               input parameters buffer. This is, effectively, a deserialization
 *               from an input stream into a paramater-holding structure.
 *
 * \retval The built parameters structure.
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

	CSI_DEBUG_PROC("+ csi_build_parms(0x"ADDR_FIELDSPEC",%d", (uaddr)buf, len);

	memset(&result, 0, sizeof(csi_cipherparms_t));

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
 *               inbound bpsec block for an information item of specified type.
 *
 * \retval The LV requested.  Len = 0 indicates not found.
 *
 * \param[in] itemNeeded The code number of the type of item to search
 *                       for.  Valid item type codes are defined in
 *                       bpsec_util.h as BPSEC_CSPARM_xxx macros.
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
	uint8_t	  *cursor = buf;
	uint8_t	  itemType;
	uvast	  sdnvLength;
	uvast	  longNumber;
	uint32_t  itemLength;

	CSI_DEBUG_PROC("+ csi_extract_tlv(%d, "ADDR_FIELDSPEC",%d)",
			       itemNeeded, (uaddr)buf, bufLen);

	memset(&result,0, sizeof(csi_val_t));

	/* Step 0 - Sanity Check. */
	if((buf == NULL) || (bufLen == 0))
	{
		CSI_DEBUG_ERR("x csi_extract_tlv - Bad Parms.", NULL);
		CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);
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
			CSI_DEBUG_ERR("x csi_extract_tlv: Read type %d and ran out of space.", itemType);
			CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);

			return result;
		}

		/* Step 1b - Grab the length, which is an SDNV. */
		sdnvLength = decodeSdnv(&longNumber, cursor);

		itemLength = longNumber;
		cursor += sdnvLength;
		bufLen -= sdnvLength;

		if (sdnvLength == 0 || sdnvLength > bufLen)
		{
			CSI_DEBUG_ERR("x csi_extract_tlv: Bad Len of %d with %d buffer remaining.", sdnvLength, bufLen);
			CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);

			return result;
		}

		/**
		 * Step 1c - Evaluate this item. If the item is empty or not a match,
		 *           skip over it. Otherwise, copy it out and return.
		 */

		if (itemLength == 0)	/*	Empty item.		*/
		{
			continue;
		}

		if (itemType == itemNeeded)
		{
			if((result.contents = MTAKE(itemLength)) == NULL)
			{
				CSI_DEBUG_ERR("x csi_extract_tlv: Cannot allocate size of %d.", itemLength);
				CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);

				return result;
			}
			memcpy(result.contents, cursor, itemLength);
			result.len = itemLength;

			CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);
			return result;
		}

		/*	Look at next item in buffer.			*/

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
 * \retval The serialized TLV. Length 0 indicates error.
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

	CSI_DEBUG_PROC("+ csi_build_tlv(%d, %d, "ADDR_FIELDSPEC")", id, len, (uaddr)contents);

	memset(&result, 0, sizeof(result));

	/* Step 0 - Sanity checks. */

	if((len == 0) || (contents == NULL))
	{
		CSI_DEBUG_ERR("x csi_build_tlv: Bad parms.", NULL);
		CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", result.len);

		return result;
	}

	/* Step 1 - Encode the length of the parameter. */
	encodeSdnv(&lenSdnv, len);

	/* Step 2 - Allocate space for the parameter. */
	result.len = 1 + lenSdnv.length + len;
	if((result.contents = MTAKE(result.len)) == NULL)
	{
		CSI_DEBUG_ERR("x csi_build_tlv: Can't allocate result of length %d.",
				result.len);
		result.len = 0;

		CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", result.len);

		return result;
	}

	/* Step 3 - Populate parameter. */
	result.contents[0] = id;
	memcpy(&(result.contents[1]), lenSdnv.text, lenSdnv.length);
	memcpy(&(result.contents[1+lenSdnv.length]), contents, len);

	CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", result.len);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: csi_cipherParms_free
 *
 * \par Purpose: Releases memory associated with cipher parameters.
 *
 * \retval None.
 *
 * \param[in/out] parms  The cipherparms to be freed.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/21/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

void csi_cipherparms_free(csi_cipherparms_t parms)
{
	MRELEASE(parms.iv.contents);
	MRELEASE(parms.salt.contents);
	MRELEASE(parms.icv.contents);
	MRELEASE(parms.aad.contents);
}


int csi_entropy_poll( void *data,
                      unsigned char *output, size_t len, size_t *olen )
{
	size_t i = 0;

	for(i = 0; i < len; i++)
	{
		output[i] = rand();
	}
	*olen = len;

	return (0);
}

// 2/21
int csi_init()
{
	CSI_DEBUG_PROC("+ csi_init()", NULL);

	g_csi_init = 1;

	mbedtls_entropy_init(&g_csi_entropy );

	mbedtls_entropy_add_source(&g_csi_entropy,
							   csi_entropy_poll, NULL, 0, MBEDTLS_ENTROPY_SOURCE_STRONG);


	if(gcm_init(&g_csi_entropy) != 1)
	{
		CSI_DEBUG_ERR("x csi_int: Error initializing gcm.", NULL);
		return -1;
	}

	if(hsha_init(&g_csi_entropy) != 1)
	{
		CSI_DEBUG_ERR("x csi_int: Error initializing hsha.", NULL);
		return -1;
	}

	CSI_DEBUG_PROC("- csi_init -> 1.", NULL);
	return 1;
}

// 2/27 impl
// 3/14 added max.

char *csi_val_print(csi_val_t val, uint32_t maxLen)
{
	char *result = NULL;
	uint32_t char_size = 0;

	char temp[3];
	int i = 0;
	int r = 0;


	if(maxLen <= 4)
	{
		maxLen = 4;
	}

	if(val.len < maxLen)
	{
		maxLen = val.len;
	}

	/* Each byte requires 2 characters to represent in HEX. Also, require
	 * three additional bytes to capture '0x' and NULL terminator.
	 */
	char_size = (2 * maxLen) + 3;
	result = (char *) MTAKE(char_size);

	if(result == NULL)
	{
		CSI_DEBUG_ERR("utils_hex_to_string", "Cannot allocate %d bytes.",
				char_size);
		CSI_DEBUG_ERR("utils_hex_to_string", "-> NULL.", NULL);
		return NULL;
	}

	memset(result, 0, char_size);


	result[0] = '0';
	result[1] = 'x';


	if(val.contents == NULL)
	{
		result[2] = '0';
		result[3] = '\0';
		return result;
	}

	r = 2;

	for(i = 0; i < maxLen; i++)
	{
		sprintf(temp, "%.2x", (unsigned int)val.contents[i]);
		result[r++] = temp[0];
		result[r++] = temp[1];
	}

	result[r] = '\0';

	return result;
}


// 2/21
csi_val_t csi_rand(csi_csid_t suite, uint32_t len)
{
	csi_val_t result;

	CSI_CHK

	memset(&result, 0, sizeof(result));

	switch(suite)
	{
        case CSTYPE_HMAC_SHA1:
	    case CSTYPE_HMAC_SHA256:
	    case CSTYPE_HMAC_SHA384:
	    case CSTYPE_HMAC_SHA512:
		    return hsha_rand(suite, len);

	    case CSTYPE_SHA256_AES128:
	    case CSTYPE_SHA384_AES256:
	    case CSTYPE_AES128_GCM:
	    case CSTYPE_AES256_GCM:
	    	return gcm_rand(suite, len);

	    default:
	    	CSI_DEBUG_ERR("Unsupported suite: %d.", suite);
	    	break;
	}

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
		intsig = csi_build_tlv(CSI_PARM_INTSIG, parms.intsig.len, parms.intsig.contents);
		result.len += intsig.len;
	}

	if(parms.icv.len > 0)
	{
		icv = csi_build_tlv(CSI_PARM_ICV, parms.icv.len, parms.icv.contents);
		result.len += icv.len;
	}

	if(parms.iv.len > 0)
	{
		iv = csi_build_tlv(CSI_PARM_IV, parms.iv.len, parms.iv.contents);
		result.len += iv.len;
	}

	if(parms.salt.len > 0)
	{
		salt = csi_build_tlv(CSI_PARM_SALT, parms.salt.len, parms.salt.contents);
		result.len += salt.len;
	}

	if(parms.keyinfo.len > 0)
	{
		keyinfo = csi_build_tlv(CSI_PARM_KEYINFO, parms.keyinfo.len, parms.keyinfo.contents);
		result.len += keyinfo.len;
	}


	/* Step 3 - Allocate the SDR space. */
	if((result.contents = MTAKE(result.len)) == 0)
	{
		CSI_DEBUG_ERR("bpsec_build_sdr_parm: Can't allocate result of length %d.",
				result.len);
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
		memcpy(result.contents+offset, (char *) intsig.contents, intsig.len);
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
		memcpy(result.contents+offset, (char *) salt.contents, salt.len);
		offset += salt.len;
		MRELEASE(salt.contents);
	}

	if(parms.keyinfo.len > 0)
	{
		memcpy(result.contents+offset, (char *) keyinfo.contents, keyinfo.len);
		offset += keyinfo.len;
		MRELEASE(keyinfo.contents);
	}

	return result;
}

void      csi_teardown()
{
	gcm_teardown();
    hsha_teardown();
    mbedtls_entropy_free(&g_csi_entropy );
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_blocksize(csi_csid_t suite)
{
	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return hsha_blocksize(suite);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return ecdsa_blocksize(suite);

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		return gcm_blocksize(suite);
		break;

	default:
		break;
	}

	CSI_DEBUG_ERR("x crypt_get_blocksize: Unsupported suite %d.", suite);

	return 0;
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
 * \return Context Length.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/10/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_ctx_len(csi_csid_t suite)
{
	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return hsha_ctx_len(suite);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return ecdsa_ctx_len(suite);

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		return gcm_ctx_len(suite);

	default:
		break;
	}

	CSI_DEBUG_ERR("x csi_ctx_len: Unsupported suite %d.", suite);
	return 0;
}



/******************************************************************************
 *
 * \par Function Name: csi_ctx_init
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
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t *csi_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc)
{
	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return (uint8_t *) hsha_ctx_init(suite, key_info, svc);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return (uint8_t *) ecdsa_ctx_init(suite, key_info, svc);

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		return (uint8_t *) gcm_ctx_init(suite, key_info, svc);

	default:
		break;
	}

	CSI_DEBUG_ERR("x csi_ctx_init: Unsupported suite %d.", suite);
	return NULL;
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t  csi_ctx_free(csi_csid_t suite, void *context)
{
	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return hsha_ctx_free(suite, context);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return ecdsa_ctx_free(suite, context);

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:

		return gcm_ctx_free(suite, context);

	default:
		break;
	}

	CSI_DEBUG_ERR("x csi_ctx_free: Unsupported suite %d.", suite);
	return ERROR;
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
 * \return The length of the security result.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
uint32_t csi_sign_res_len(csi_csid_t suite, void *context)
{

	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return hsha_sign_res_len(suite, context);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return ecdsa_sign_res_len(suite, context);

	default:
		break;
	}

	CSI_DEBUG_ERR("x csi_sign_res_len: Unsupported suite %d.", suite);

	return ERROR;
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
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_sign_start(csi_csid_t suite, void *context)
{

	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return hsha_sign_start(suite, context);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return ecdsa_sign_start(suite, context);

	default:
		break;
	}

	CSI_DEBUG_ERR("x csi_sign_start: Unsupported suite %d.", suite);

	return ERROR;
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t  csi_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc)
{
	CSI_CHK

	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
	case CSTYPE_HMAC_SHA256:
	case CSTYPE_HMAC_SHA384:
		return hsha_sign_update(suite, context, data, svc);

	case CSTYPE_ECDSA_SHA256:
	case CSTYPE_ECDSA_SHA384:
		return ecdsa_sign_update(suite, context, data, svc);

	default:
		break;
	}

	CSI_DEBUG_ERR("x csi_sign_update: Unsupported suite %d.", suite);

	return ERROR;
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
 *\todo: Update return values and description based on SVC.
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t  csi_sign_finish(csi_csid_t suite, void *context, csi_val_t *result, csi_svcid_t svc)
{
	int8_t retval = ERROR;

	CSI_DEBUG_PROC("+ csi_sign_finish(%d, "ADDR_FIELDSPEC","ADDR_FIELDSPEC",%d)",
				    suite, (uaddr)context, (uaddr)result, svc);

	CSI_CHK

	switch(suite)
		{
		case CSTYPE_HMAC_SHA1:
		case CSTYPE_HMAC_SHA256:
		case CSTYPE_HMAC_SHA384:
			retval = hsha_sign_finish(suite, context, result, svc);
			break;

		case CSTYPE_ECDSA_SHA256:
		case CSTYPE_ECDSA_SHA384:
			retval = ecdsa_sign_finish(suite, context, result, svc);
			break;

		default:
			CSI_DEBUG_ERR("x csi_sign_finish: Unsupported suite %d.", suite);
			break;
		}

#ifdef CSI_DEBUGGING
	if(retval != ERROR)
	{
		CSI_DEBUG_INFO("i csi_sign_finish: Suite: %d. Svc: %d. Length %d", suite, svc, result->len);
	}
#endif

	CSI_DEBUG_PROC("- csi_sign_finish -> %d", retval);
	return retval;
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
 * \param[in]  svc      Cryptographic service to perform (sign or verify)
 *
 * \par Notes:
 *	    - The returned output structure MUST be correctly released by the
 *	      calling function.
 *
 * \return The result of the ciphersuite operation
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  03/12/16  E. Birrane     Updated args to support verify. [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int8_t csi_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *result, csi_svcid_t svc)
{
	int8_t retval = ERROR;

	CSI_DEBUG_PROC("+csi_sign_full(%d, input (len=%d), key (len=%d),"ADDR_FIELDSPEC",%d)",
			       suite, input.len, key.len, (uaddr) result, svc);

	CSI_CHK

	CHKERR(result);

	switch(suite)
		{
		case CSTYPE_HMAC_SHA1:
		case CSTYPE_HMAC_SHA256:
		case CSTYPE_HMAC_SHA384:
			retval = hsha_sign_full(suite, input, key, result, svc);
			break;

		case CSTYPE_ECDSA_SHA256:
		case CSTYPE_ECDSA_SHA384:
			retval = ecdsa_sign_full(suite, input, key, result, svc);
			break;

		default:
			CSI_DEBUG_ERR("x csi_sign_full: Unsupported suite %d.", suite);
			break;
		}

#ifdef CSI_DEBUGGING
	if(retval != ERROR)
	{
		char tmp[21];
		memset(tmp,0,21);

		CSI_DEBUG_INFO("i csi_sign_full: Suite: %d. Svc: %d.", suite, svc);

		memcpy(tmp,input.contents, 20);
		CSI_DEBUG_INFO("i csi_sign_full: Input - Len:%d  Val:%s...", input.len, tmp);

		memcpy(tmp,key.contents, 20);
		CSI_DEBUG_INFO("i csi_sign_full: Key - Len:%d  Val:%s...", key.len, tmp);

		memcpy(tmp,result->contents, 20);
		CSI_DEBUG_INFO("i csi_sign_full: Result - Len:%d  Val:%s...", result->len, tmp);
	}
#endif

	CSI_DEBUG_PROC("- csi_sign_full -> %d", retval);

	return retval;
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/20/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t csi_crypt_finish(csi_csid_t suite, void *context, csi_svcid_t svc, csi_cipherparms_t *parms)
{
	int8_t retval = ERROR;

	CSI_CHK

	switch(suite)
	{
	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		retval = gcm_crypt_finish(suite, context, svc, parms);
		break;

	default:
		CSI_DEBUG_ERR("x csi_crypt_finish: Unsupported suite %d.", suite);
		break;
	}

#ifdef CSI_DEBUGGING
	if(retval != ERROR)
	{
		char *tmp = NULL;

		CSI_DEBUG_INFO("i csi_crypt_finish: Suite: %d. Svc: %d.", suite, svc);

		if((tmp = csi_val_print(parms->iv, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_finish: IV - Len:%d  Val:%s...", parms->iv.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->salt, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_finish: SALT - Len:%d  Val:%s...", parms->salt.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->icv, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_finish: ICV - Len:%d  Val:%s...", parms->icv.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->intsig, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_finish: INTSIG - Len:%d  Val:%s...", parms->intsig.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->aad, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_finish: ADD - Len:%d  Val:%s...", parms->aad.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->keyinfo, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_finish: KEYINFO - Len:%d  Val:%s...", parms->keyinfo.len, tmp);
			MRELEASE(tmp);
		}
	}
#endif

	CSI_DEBUG_PROC("- csi_crypt_finish ->%d", retval);
	return retval;
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
 *	    - The returned output structure MUST be correctly released by the
 *	      calling function.
 *	    - the output structure MUST NOT have any allocated data in it.
 *
 * \return 1 or ERROR.
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

	CSI_CHK

	CHKERR(parms);
	CHKERR(output);

	switch(suite)
	{

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		retval = gcm_crypt_full(suite, svc, parms, key, input, output);
		break;

	default:
		CSI_DEBUG_ERR("x csi_crypt_full: Unsupported suite %d.", suite);
		break;
	}


#ifdef CSI_DEBUGGING
	if(retval != ERROR)
	{
		char *tmp = NULL;

		CSI_DEBUG_INFO("i csi_crypt_full: Suite: %d. Svc: %d.", suite, svc);

		if((tmp = csi_val_print(input, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: Input - Len:%d  Val:%s...", input.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(key, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: Key - Len:%d  Val:%s...", key.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(*output, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: Key - Len:%d  Val:%s...", output->len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->iv, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: IV - Len:%d  Val:%s...", parms->iv.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->salt, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: SALT - Len:%d  Val:%s...", parms->salt.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->icv, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: ICV - Len:%d  Val:%s...", parms->icv.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->intsig, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: INTSIG - Len:%d  Val:%s...", parms->intsig.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->aad, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: ADD - Len:%d  Val:%s...", parms->aad.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->keyinfo, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_full: KEYINFO - Len:%d  Val:%s...", parms->keyinfo.len, tmp);
			MRELEASE(tmp);
		}
	}
#endif

	CSI_DEBUG_PROC("- csi_crypt_full ->%d", retval);
	return retval;
}


int8_t csi_crypt_key(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, csi_val_t longtermkey, csi_val_t input, csi_val_t *output)
{
	int8_t retval = ERROR;

	CSI_DEBUG_PROC("+ csi_crypt_key(%d, %d, "ADDR_FIELDSPEC", longtermkey (len=%d), input(len=%d), "ADDR_FIELDSPEC")",
			       suite, svc, (uaddr) parms, longtermkey.len, input.len, (uaddr)output);

	CSI_CHK

	CHKERR(parms);
	CHKERR(output);

	switch(suite)
	{

	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		retval = gcm_crypt_key(suite, svc, parms, longtermkey, input, output);
		break;

	default:
		CSI_DEBUG_ERR("x csi_crypt_key: Unsupported suite %d.", suite);
		break;
	}

#ifdef CSI_DEBUGGING
	if(retval != ERROR)
	{
		char *tmp = NULL;

		CSI_DEBUG_INFO("i csi_crypt_key: Suite: %d. Svc: %d.", suite, svc);


		if((tmp = csi_val_print(longtermkey, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: longtermkey - Len:%d  Val:%s...", longtermkey.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(input, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: input - Len:%d  Val:%s...", input.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(*output, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: output - Len:%d  Val:%s...", output->len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->iv, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: IV - Len:%d  Val:%s...", parms->iv.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->salt, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: SALT - Len:%d  Val:%s...", parms->salt.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->icv, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: ICV - Len:%d  Val:%s...", parms->icv.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->intsig, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: INTSIG - Len:%d  Val:%s...", parms->intsig.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->aad, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: ADD - Len:%d  Val:%s...", parms->aad.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(parms->keyinfo, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_key: KEYINFO - Len:%d  Val:%s...", parms->keyinfo.len, tmp);
			MRELEASE(tmp);
		}
	}
#endif

	CSI_DEBUG_PROC("- csi_crypt_key ->%d", retval);
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
 * \return The parameter. Length 0 indicates an error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int csi_parm_len_get(csi_csid_t suite, csi_parmid_t parmid)
{
  int len = 0;

  switch(suite)
  	{
  		case CSTYPE_HMAC_SHA256:
  		case CSTYPE_HMAC_SHA384:
  		case CSTYPE_HMAC_SHA512:
  			len = hsha_parm_get_len(suite, parmid);
  			break;

  		case CSTYPE_SHA256_AES128:
  		case CSTYPE_SHA384_AES256:
  		case CSTYPE_AES128_GCM:
  		case CSTYPE_AES256_GCM:
  			len = gcm_crypt_parm_get_len(suite, parmid);
  			break;
  		default:
  			CSI_DEBUG_ERR("x csi_crypt_parm_get: Unsupported suite %d.", suite);
  			break;
  	}

  return len;
}

csi_val_t csi_crypt_parm_get(csi_csid_t suite, csi_parmid_t parmid)
{
	csi_val_t result;
	int len = 0;

	CSI_CHK

	memset(&result, 0, sizeof(csi_val_t));

	len = csi_parm_len_get(suite, parmid);

	if(len > 0)
	{
		result = csi_rand(suite, len);
	}
	else
	{
  	   CSI_DEBUG_ERR("x csi_crypt_parm_get: Bad length - suite %d Parm %d length %d.", suite, parmid, len);
	}

	return result;
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
 * \return The parameter length. 0 indicates error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t  csi_crypt_parm_get_len(csi_csid_t suite, csi_parmid_t parmid)
{
	CSI_CHK

	switch(suite)
	{
		case CSTYPE_SHA256_AES128:
		case CSTYPE_SHA384_AES256:
		case CSTYPE_AES128_GCM:
  		case CSTYPE_AES256_GCM:
			return gcm_crypt_parm_get_len(suite, parmid);

		default:
			break;
	}

	CSI_DEBUG_ERR("x csi_crypt_parm_get_len: Unsupported suite %d.", suite);
	return ERROR;
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
 * \return The length of the security result. 0 indicates error
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_crypt_res_len(csi_csid_t suite, void *context, csi_blocksize_t blocksize, csi_svcid_t svc)
{

	CSI_CHK

	switch(suite)
	{
	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		return gcm_crypt_res_len(suite, context, blocksize, svc);
		break;

	default:
		break;

	}

	CSI_DEBUG_ERR("x csi_crypt_res_len: Unsupported suite %d.", suite);

	return 0;
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int8_t csi_crypt_start(csi_csid_t suite, void *context, csi_cipherparms_t parms)
{

	CSI_CHK

	switch(suite)
	{
	case CSTYPE_SHA256_AES128:
	case CSTYPE_SHA384_AES256:
	case CSTYPE_AES128_GCM:
	case CSTYPE_AES256_GCM:
		return gcm_crypt_start(suite, context, parms);
		break;

	default:
		break;

	}

	CSI_DEBUG_ERR("x csi_crypt_res_len: Unsupported suite %d.", suite);

	return ERROR;
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
 * \return ciphersuite output. Length 0 indicates error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t  csi_crypt_update(csi_csid_t suite, void *context, csi_svcid_t svc, csi_val_t data)
{
	csi_val_t result;

	CSI_DEBUG_PROC("+ csi_crypt_update(%d, "ADDR_FIELDSPEC",%d,data (len=%d)",
			       suite, (uaddr) context, svc, data.len);

	CSI_CHK

	memset(&result, 0, sizeof(csi_val_t));

	switch(suite)
	{

		case CSTYPE_SHA256_AES128:
		case CSTYPE_SHA384_AES256:
		case CSTYPE_AES128_GCM:
		case CSTYPE_AES256_GCM:
			result =  gcm_crypt_update(suite, context, svc, data);
			break;

		default:
			CSI_DEBUG_ERR("x csi_crypt_update: Unsupported suite %d.", suite);
			break;
	}



#ifdef CSI_DEBUGGING
	if(result.len > 0)
	{
		char *tmp;

		CSI_DEBUG_INFO("i csi_crypt_update: Suite: %d. Svc: %d.", suite, svc);

		if((tmp = csi_val_print(data, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_update: Data   - Len: %d  Val: %s...", data.len, tmp);
			MRELEASE(tmp);
		}

		if((tmp = csi_val_print(result, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i csi_crypt_update: Result - Len: %d  Val: %s...", result.len, tmp);
			MRELEASE(tmp);
		}
	}
#endif


	CSI_DEBUG_PROC("- csi_crypt_update -> result (len = %d)", result.len);
	return result;
}






















