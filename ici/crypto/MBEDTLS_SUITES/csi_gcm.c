/*****************************************************************************
 **
 ** File Name: csi_gcm.c
 **
 ** Description: This file defines implementations of the ION Cryptographic
 **              interface as related to AES in Galois Counter Mode.
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
 **  02/16/16  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/


#include "platform.h"
#include "csi.h"
#include "csi_gcm.h"
#include "mbedtls/gcm.h"
#include "mbedtls/ctr_drbg.h"



mbedtls_ctr_drbg_context g_csi_ctr_drbg_ctx;


#if (CSI_DEBUGGING == 1)
extern char	gMsg[];		/*	Debug message buffer.	*/
#endif


int gcm_init(mbedtls_entropy_context *entropy)
{
	int ret = 0;

	mbedtls_ctr_drbg_init( &g_csi_ctr_drbg_ctx );
	ret = mbedtls_ctr_drbg_seed(&g_csi_ctr_drbg_ctx,
			                    mbedtls_entropy_func,
								entropy,
								(const unsigned char *) "CSI CIPHERSUITES",
								16);
	mbedtls_ctr_drbg_set_prediction_resistance( &g_csi_ctr_drbg_ctx, MBEDTLS_CTR_DRBG_PR_OFF );

	if(ret != 0)
	{
		CSI_DEBUG_ERR("x csi_init: Cannot initialize random num gen. Error %d", ret);
		return -1;
	}

	return 1;
}

void gcm_teardown()
{
    mbedtls_ctr_drbg_free( &g_csi_ctr_drbg_ctx );
}

/******************************************************************************
 *
 * \par Function Name: gcm_blocksize
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
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t gcm_blocksize(csi_csid_t suite)
{
	return 65000;
}



/******************************************************************************
 *
 * \par Function Name: gcm_ctx_len
 *
 * \par Return the maximum length of a context for this given ciphersuite.
 *
 * \param[in]     suite    The ciphersuite context length being queried.
 * \param[in/out] scratch  Optional ciphersuite-specific information.
 *
 * \par Notes:
 *
 * \return Context Length.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/18/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t gcm_ctx_len(csi_csid_t suite)
{
	return sizeof(csi_gcm_context_t);
}



/******************************************************************************
 *
 * \par Function Name: gcm_ctx_init
 *
 * \par Initialize a ciphersuite context.
 *
 * \param[in] suite     The ciphersuite whose context is being initialized.
 * \param[in] key_info  Key information related to the ciphersuite action.
 * \param[in] svc       THe ciphersuite service for the context (encrypt or decrypt).
 *
 * \par Notes:
 *  - The context is allocated by MTAKE and must be freed my MRELEASE.
 *  - The key must be 128 bits for CSTYPE_SHA256_AES128
 *  - The key must be 256 bits for CSTYPE_SHA384_AES256
 *
 * \return NULL or created/initialized context.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/18/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
csi_gcm_context_t *gcm_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc)
{
	csi_gcm_context_t *result = NULL;
	uint32_t size = gcm_ctx_len(suite);
	uint32_t retval = 0;

	/* Step 0: Sanity checks. */

	/* Step 1: Allocate and initialize the main context. */
	if((result = MTAKE(size)) == NULL)
	{
		CSI_DEBUG_ERR("x gcm_ctx_init: Can't allocate CSI context of size %d",size);
		return NULL;
	}
	memset(result, 0, size);

	/* Step 2: Initialize the GCM context. This basically zeros th structure. */
	mbedtls_gcm_init(&(result->gcm_ctx));

	/*
	 * Step 3: Verify the provided key size and set the key portion of the
	 *         context.
	 */
	if((suite == CSTYPE_SHA256_AES128) || (suite == CSTYPE_AES128_GCM))
	{
		if(key_info.len != 16)
		{
			CSI_DEBUG_ERR("x gcm_ctx_init: Expected key size 16 not %d for suite %d.",
					         key_info.len, suite);
			gcm_ctx_free(suite, result);
			return NULL;
		}

		retval = mbedtls_gcm_setkey(&(result->gcm_ctx), MBEDTLS_CIPHER_ID_AES, key_info.contents, 128);
	}
	else if((suite == CSTYPE_SHA384_AES256) || (suite == CSTYPE_AES256_GCM))
	{
		if(key_info.len != 32)
		{
			CSI_DEBUG_ERR("x gcm_ctx_init: Expected key size 32 not %d for suite %d.",
					         key_info.len, suite);
			gcm_ctx_free(suite, result);
			return NULL;
		}

		retval = mbedtls_gcm_setkey(&(result->gcm_ctx), MBEDTLS_CIPHER_ID_AES, key_info.contents, 256);
	}
	else
	{
		CSI_DEBUG_ERR("x gcm_ctx_init: Unsupported suite %d", suite);
		gcm_ctx_free(suite, result);
		return NULL;
	}

	if(retval != 0)
	{
		CSI_DEBUG_ERR("x gcm_ctx_init: Can't set gcm key. Error %d", retval);
		gcm_ctx_free(suite, result);
		return NULL;
	}

	/* Step 4: Start the GCM process. */

	if(svc == CSI_SVC_ENCRYPT)
	{
		result->mode = MBEDTLS_GCM_ENCRYPT;
	}
	else if(svc == CSI_SVC_DECRYPT)
	{
		result->mode = MBEDTLS_GCM_DECRYPT;
	}
	else
	{
		CSI_DEBUG_ERR("x gcm_ctx_init: Bad function: %d", svc);
		gcm_ctx_free(suite, result);
		return NULL;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: gcm_ctx_free
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
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t gcm_ctx_free(csi_csid_t suite, void *context)
{
	csi_gcm_context_t *csi_gcm_ctx = (csi_gcm_context_t *) context;

	/* Step 1: If NULL, nothing to free. */
	if(csi_gcm_ctx != NULL)
	{
		if(csi_gcm_ctx != NULL)
		{
			mbedtls_gcm_free(&(csi_gcm_ctx->gcm_ctx));
		}

		MRELEASE(csi_gcm_ctx);
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: gcm_crypt_finish
 *
 * \par Finish a streaming operation from a ciphersuite.
 *
 * \param[in]     suite    The ciphersuite whose context is being finished.
 * \param[in/out] context  The context being finished.
 * \param[in]     svc      The Cryptographic service being finished (encrypt or decrypt)
 * \param[out]    parms    Ciphersuite parameters (to update)
 *
 * \par Notes:
 *  - Will update the ICV if a non-NULL ICV is given.
 *
 * \return 1 or ERROR
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int8_t gcm_crypt_finish(csi_csid_t suite, void *context, csi_svcid_t svc, csi_cipherparms_t *parms)
{
	csi_gcm_context_t *csi_gcm_ctx = (csi_gcm_context_t *) context;
	int retval = 0;

	if(context == NULL)
	{
		CSI_DEBUG_ERR("x gcm_crypt_finish: NULL context.", NULL);
		return ERROR;
	}

	if(parms->icv.contents == NULL)
	{
		parms->icv.len = 16;
		if((parms->icv.contents = MTAKE(parms->icv.len)) == NULL)
		{
			CSI_DEBUG_ERR("x gcm_crypt_start: Can't allocate ICV of length %d", parms->icv.len);
			return ERROR;
		}
	}
	else if(parms->icv.len != 16)
	{
		CSI_DEBUG_ERR("x gcm_crypt_start: ICV length must be 16 not %d", parms->icv.len);
		return ERROR;
	}

	retval = mbedtls_gcm_finish(&(csi_gcm_ctx->gcm_ctx), parms->icv.contents, parms->icv.len);

	if(retval != 0)
	{
		CSI_DEBUG_ERR("x gcm_crypt_finish: Failed finishing context. Error %d.", retval);
		return ERROR;
	}

	return 1;
}







/******************************************************************************
 *
 * \par Function Name: gcm_crypt_full
 *
 * \par Apply a ciphersuite to a given set of input data.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  svc      The Cryptographic service to perform (encrypt or decrypt)
 * \param[in]  parms    Ciphersuite parameters to use for this service.
 * \param[in]  key      The key to use for this service.
 * \param[in]  input    The input to the ciphersuite (plaintext or ciphertext)
 * \param[out] output   The output of the ciphersuite (plaintext or ciphertext)
 *
 * \par Notes:
 *	    - The returned output structure MUST be correctly released by the
 *	      calling function.
 *	    - If given a NULL icv, this function will allocate one.
 *	    - The ICV is the AES-GCM Authentication Tag. Only a 16 octet value is required
 *	       to be supported. 8 and 12 octet values are optional in the spec and
 *	       not supported here.
 *
 * \return 1 on success (and authentication) else ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int8_t  gcm_crypt_full(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms,
		               csi_val_t key, csi_val_t input, csi_val_t *output)
{
	uint32_t retval = 0;
	mbedtls_gcm_context gcm_ctx;
	int alloc = 0;

	/* Step 0: Sanity check. */
	CHKERR(parms);
	CHKERR(output);

	/* Step 1: Create the GCM context and setup the key. */
	mbedtls_gcm_init(&gcm_ctx);

	if((suite == CSTYPE_SHA256_AES128) || (suite == CSTYPE_AES128_GCM))
	{
		if(key.len != 16)
		{
			CSI_DEBUG_ERR("x gcm_crypt_full: Expected key size 16 not %d for suite %d.",
					key.len, suite);
			mbedtls_gcm_free(&gcm_ctx);
			return ERROR;
		}

		retval = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key.contents, 128);
	}
	else if((suite == CSTYPE_SHA384_AES256) || (suite == CSTYPE_AES256_GCM))
	{
		if(key.len != 32)
		{
			CSI_DEBUG_ERR("x gcm_crypt_full: Expected key size 32 not %d for suite %d.",
					key.len, suite);
			mbedtls_gcm_free(&gcm_ctx);
			return ERROR;
		}

		retval = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key.contents, 256);
	}
	else
	{
		CSI_DEBUG_ERR("x gcm_crypt_full: Unsupported suite %d", suite);
		mbedtls_gcm_free(&gcm_ctx);
		return ERROR;
	}

	if(retval != 0)
	{
		CSI_DEBUG_ERR("x gcm_crypt_full: Can't set gcm key. Error %d", retval);
		mbedtls_gcm_free(&gcm_ctx);
		return ERROR;
	}

	if(parms->icv.contents == NULL)
	{
		parms->icv.len = 16;
		if((parms->icv.contents = MTAKE(parms->icv.len)) == NULL)
		{
			CSI_DEBUG_ERR("x gcm_crypt_full: Can't allocate ICV of length %d", parms->icv.len);
			mbedtls_gcm_free(&gcm_ctx);
			return ERROR;
		}
		alloc = 1;
	}
	else if(parms->icv.len != 16)
	{
		CSI_DEBUG_ERR("x gcm_crypt_full: ICV length must be 16 not %d", parms->icv.len);
		mbedtls_gcm_free(&gcm_ctx);
		return ERROR;
	}

	if((output->contents = MTAKE(input.len)) == NULL)
	{
		CSI_DEBUG_ERR("x gcm_crypt_full: Can't allocate output buffer of len %d", input.len);
		output->len = 0;
		mbedtls_gcm_free(&gcm_ctx);
		return ERROR;
	}
	output->len = input.len;


	/* Step 2: Figure out the GCM service. */
	if(svc == CSI_SVC_ENCRYPT)
	{

		retval = mbedtls_gcm_crypt_and_tag(&gcm_ctx,
		                       	   	       MBEDTLS_GCM_ENCRYPT,
										   input.len,
										   parms->iv.contents,
										   parms->iv.len,
										   parms->aad.contents,
										   parms->aad.len,
										   input.contents,
										   output->contents,
										   parms->icv.len,
										   parms->icv.contents);
		if(retval != 0)
		{
			CSI_DEBUG_ERR("x gcm_crypt_full: Failed to encrypt. input %d, iv %d, aad %d, icv %d. "
					"Error %d", input.len, parms->iv.len, parms->aad.len, parms->icv.len, retval);
			mbedtls_gcm_free(&gcm_ctx);

			if(alloc == 1)
			{
				MRELEASE(parms->icv.contents);
				parms->icv.contents = NULL;
				parms->icv.len = 0;
			}
			return ERROR;
		}

	}
	else if(svc == CSI_SVC_DECRYPT)
	{

		retval = mbedtls_gcm_auth_decrypt(&gcm_ctx,
				                          input.len,
										  parms->iv.contents,
										  parms->iv.len,
										  parms->aad.contents,
										  parms->aad.len,
										  parms->icv.contents,
										  parms->icv.len,
										  input.contents,
										  output->contents);

		if(retval != 0)
		{
			CSI_DEBUG_ERR("x gcm_crypt_full: Failed to decrypt. Error %d", retval);
			mbedtls_gcm_free(&gcm_ctx);

			if(alloc == 1)
			{
				MRELEASE(parms->icv.contents);
				parms->icv.contents = NULL;
				parms->icv.len = 0;
			}

			return ERROR;
		}
	}
	else
	{
		CSI_DEBUG_ERR("x gcm_crypt_full: Bad function: %d", svc);
		mbedtls_gcm_free(&gcm_ctx);
		if(alloc == 1)
		{
			MRELEASE(parms->icv.contents);
			parms->icv.contents = NULL;
			parms->icv.len = 0;
		}

		return ERROR;
	}

	mbedtls_gcm_free(&gcm_ctx);

	return 1;
}

/*
 * Key Info is a tuple TLV of <key icv> <encrypted key>
 * \todo  check MTAKE results.
 */
int8_t gcm_crypt_key(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, csi_val_t longtermkey,
		             csi_val_t input, csi_val_t *output)
{
	int8_t result = ERROR;
	csi_cipherparms_t keyparms;

	CHKERR(parms);
	CHKERR(output);

	memset(&keyparms, 0, sizeof(csi_cipherparms_t));

	switch(suite)
	{
		case CSTYPE_SHA256_AES128:
		case CSTYPE_AES128_GCM:
			output->len = 16;
			break;
		case CSTYPE_SHA384_AES256:
		case CSTYPE_AES256_GCM:
			output->len = 32;
			break;
		default:
			return ERROR;
			break;
	}

	if((output->contents = MTAKE(output->len)) == NULL)
	{
		CSI_DEBUG_ERR("gcm_crypt_key: Could not allocate %d bytes. %d", output->len);
		return ERROR;
	}

	if(svc == CSI_SVC_ENCRYPT)
	{
		keyparms.iv = parms->iv;
		keyparms.salt = parms->salt;

		if((result = gcm_crypt_full(suite, svc, &keyparms, longtermkey, input, output)) == ERROR)
		{
			MRELEASE(output->contents);
			return ERROR;
		}

		parms->keyinfo = csi_build_tlv(CSI_PARM_BEKICV, keyparms.icv.len, keyparms.icv.contents);
	}
	else if(svc == CSI_SVC_DECRYPT)
	{

		keyparms.iv = parms->iv;
		keyparms.salt = parms->salt;

		keyparms.icv = csi_extract_tlv(CSI_PARM_BEKICV, parms->keyinfo.contents, parms->keyinfo.len);

		result = gcm_crypt_full(suite, svc, &keyparms, longtermkey, input, output);

		MRELEASE(keyparms.icv.contents);

		if(result == ERROR)
		{
		    CSI_DEBUG_ERR("gcm_crypt_key: Could not decrypt key.", NULL);
			MRELEASE(output->contents);
			return ERROR;
		}

	}
	else
	{
		MRELEASE(output->contents);
	}

	return result;
}

csi_val_t gcm_rand(csi_csid_t suite, uint32_t len)
{
	csi_val_t result;
	int retVal = 0;

	result.len = len;
	result.contents = NULL;

	if((result.contents = MTAKE(len)) == NULL)
	{
		CSI_DEBUG_ERR("x gcm_csi_rand: Cannot allocate %d bytes.", len);
		return result;
	}

	if((retVal = mbedtls_ctr_drbg_random(&g_csi_ctr_drbg_ctx, result.contents, result.len)) != 0)
	{
		CSI_DEBUG_ERR("x gcm_csi_rand: Cannot generate number of len %d. Error %d.", len, retVal);
		MRELEASE(result.contents);
		result.len = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: csi_crypt_parm_gen
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
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t gcm_crypt_parm_gen(csi_csid_t suite, csi_parmid_t parmid)
{
	csi_val_t result;

	memset(&result, 0, sizeof(csi_val_t));

	switch(parmid)
	{
		case CSI_PARM_IV:
		case CSI_PARM_SALT:
		case CSI_PARM_BEK:
			result = csi_rand(suite, gcm_crypt_parm_get_len(suite, parmid));
			break;

		default:
			CSI_DEBUG_ERR("x csi_crypt_parm_get: Unknown parm id %d", parmid);
	}

	return result;
}








/******************************************************************************
 *
 * \par Function Name: gcm_crypt_parm_get_len
 *
 * \par Report a ciphersuite parameter length.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  parmid   The ciphersuite parameter whose length is queried
 * \param[in]  parms    Current set of cipher parms for reference.
 *
 * \par Notes:
 *
 * \return The parameter length. 0 indicates err
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t gcm_crypt_parm_get_len(csi_csid_t suite, csi_parmid_t parmid)
{
	uint32_t result = 0;

	switch(parmid)
	{
		case CSI_PARM_IV:
			result = 12;
			break;

		case CSI_PARM_SALT:
			result = 4;
			break;

		case CSI_PARM_BEK:
		{
			switch(suite)
			{
				case CSTYPE_AES128_GCM:
				case CSTYPE_SHA256_AES128:
					result = 16;
					break;

				case CSTYPE_AES256_GCM:
				case CSTYPE_SHA384_AES256:
					result = 32;
					break;

				default:
					CSI_DEBUG_ERR("x csi_crypt_parm_get_len: Unknown suite %d", suite);
					break;
			}
		}
		break;

		case CSI_PARM_ICV:
			result = 16;
			break;

		default:
			CSI_DEBUG_ERR("x gcm_crypt_csi_crypt_parm_get_lenget_parm: Unknown parm id %d", parmid);
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: gcm_crypt_res_len
 *
 * \par Return the length of the output of the ciphersuite.
 *
 * \param[in]  suite      The ciphersuite being used.
 * \param[in]  context    Cryptographic context
 * \param[in]  blocksize  Size information for the cryptofunction
 * \param[in]  svc        Cryptographic service to perform
 *
 * \par Notes:
 *   - In GCM mode, the security result is the same size as the
 *     input text.
 *
 * \return The length of the security result.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
uint32_t gcm_crypt_res_len(csi_csid_t suite, void *context, csi_blocksize_t blocksize, csi_svcid_t svc)
{
	return (uint32_t) blocksize.plaintextLen;
}


/******************************************************************************
 *
 * \par Function Name: gcm_crypt_start
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
int8_t gcm_crypt_start(csi_csid_t suite, void *context, csi_cipherparms_t parms)
{
	csi_gcm_context_t *csi_gcm_ctx = NULL;
	uint32_t retval = 0;

	CHKERR(context);

	csi_gcm_ctx = (csi_gcm_context_t *) context;


	retval = mbedtls_gcm_starts(&(csi_gcm_ctx->gcm_ctx),
			                    csi_gcm_ctx->mode,
								parms.iv.contents,
								parms.iv.len,
								parms.aad.contents,
								parms.aad.len);

	if(retval != 0)
	{
		CSI_DEBUG_ERR("x gcm_crypt_start: Can't start GCM operation. Error %d", retval);
		return ERROR;
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: gcm_crypt_update
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
 *  02/21/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
csi_val_t  gcm_crypt_update(csi_csid_t suite, void *context, csi_svcid_t svc, csi_val_t data)
{

	csi_gcm_context_t *csi_gcm_ctx = (csi_gcm_context_t *) context;
	csi_val_t result;
	int retval = 0;

	memset(&result, 0, sizeof(result));

	if(context == NULL)
	{
		CSI_DEBUG_ERR("x gcm_crypt_update: NULL context provided.", NULL);
		return result;
	}

	result.len = data.len;
	if((result.contents = MTAKE(result.len)) == NULL)
	{
		CSI_DEBUG_ERR("x gcm_crypt_update: Can't allocate result of size %d.", result.len);
		result.len = 0;
		return result;
	}

	retval = mbedtls_gcm_update(&(csi_gcm_ctx->gcm_ctx), data.len, data.contents, result.contents);

	if(retval != 0)
	{
		CSI_DEBUG_ERR("x gcm_crypt_update: Can't update GCM context. Error %d.", retval);
		MRELEASE(result.contents);
		result.contents = NULL;
		result.len = 0;
		return result;
	}

	return result;
}
