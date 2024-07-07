/*****************************************************************************
 **
 ** File Name: csi_hsha.c
 **
 ** Description: This file defines implementations of the ION Cryptographic
 **              interface as related to HMAC-SHA(1, 256, 384) suites.
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
 **  02/10/16  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/


#include "platform.h"
#include "csi.h"
#include "csi_hsha.h"


#include "mbedtls/hmac_drbg.h"



mbedtls_hmac_drbg_context g_csi_hmac_drbg_256_ctx;
mbedtls_hmac_drbg_context g_csi_hmac_drbg_384_ctx;
mbedtls_hmac_drbg_context g_csi_hmac_drbg_512_ctx;



#if (CSI_DEBUGGING == 1)
extern char	gMsg[];		/*	Debug message buffer.	*/
#endif


int hsha_init(mbedtls_entropy_context *entropy)
{
	int ret = 0;

	mbedtls_hmac_drbg_init( &g_csi_hmac_drbg_256_ctx );
	mbedtls_hmac_drbg_init( &g_csi_hmac_drbg_384_ctx );
	mbedtls_hmac_drbg_init( &g_csi_hmac_drbg_512_ctx );



	ret = mbedtls_hmac_drbg_seed(&g_csi_hmac_drbg_256_ctx,
								mbedtls_md_info_from_type(MBEDTLS_MD_SHA256 ),
			                    mbedtls_entropy_func,
								entropy,
								(const unsigned char *) "CSI SHA256",
								16);
	mbedtls_hmac_drbg_set_prediction_resistance( &g_csi_hmac_drbg_256_ctx, MBEDTLS_HMAC_DRBG_PR_OFF );

	if(ret != 0)
	{
		CSI_DEBUG_ERR("x hsha_init: Cannot initialize 256 random num gen. Error %d", ret);
		return -1;
	}

	ret = mbedtls_hmac_drbg_seed(&g_csi_hmac_drbg_384_ctx,
								mbedtls_md_info_from_type(MBEDTLS_MD_SHA384 ),
			                    mbedtls_entropy_func,
								entropy,
								(const unsigned char *) "CSI SHA384",
								16);
	mbedtls_hmac_drbg_set_prediction_resistance( &g_csi_hmac_drbg_384_ctx, MBEDTLS_HMAC_DRBG_PR_OFF );

	if(ret != 0)
	{
		CSI_DEBUG_ERR("x hsha_init: Cannot initialize 384 random num gen. Error %d", ret);
		return -1;
	}

	ret = mbedtls_hmac_drbg_seed(&g_csi_hmac_drbg_512_ctx,
								mbedtls_md_info_from_type(MBEDTLS_MD_SHA512 ),
			                    mbedtls_entropy_func,
								entropy,
								(const unsigned char *) "CSI SHA512",
								16);
	mbedtls_hmac_drbg_set_prediction_resistance( &g_csi_hmac_drbg_512_ctx, MBEDTLS_HMAC_DRBG_PR_OFF );

	if(ret != 0)
	{
		CSI_DEBUG_ERR("x hsha_init: Cannot initialize 512 random num gen. Error %d", ret);
		return -1;
	}

	return 1;
}

/**
extern int hsha_key_gen(csi_csid_t suite, csi_val_t *result)
{
	CHKERR(result);

	result = csi_rand(suite, gcm_crypt_parm_get_len(suite, parmid));

	switch(suite)
	{
	case CSTYPE_HMAC_SHA256:
		retVal = mbedtls_hmac_drbg_random(&g_csi_hmac_drbg_256_ctx, result.contents, result.len);
		break;
	case CSTYPE_HMAC_SHA384:
		retVal = mbedtls_hmac_drbg_random(&g_csi_hmac_drbg_384_ctx, result.contents, result.len);
		break;
	case CSTYPE_HMAC_SHA512:
		retVal = mbedtls_hmac_drbg_random(&g_csi_hmac_drbg_512_ctx, result.contents, result.len);
		break;
	default:
		retVal = -1;
		CSI_DEBUG_ERR("x hsha_rand: Unsupported suite: %d.", suite);
	}

}
**/


uint32_t hsha_parm_get_len(csi_csid_t suite, csi_parmid_t parmid)
{
	uint32_t result = 0;

	switch(parmid)
	{
		case CSI_PARM_BEK:
		{
			switch(suite)
			{
				case CSTYPE_HMAC_SHA256: result = 32; break;
				case CSTYPE_HMAC_SHA384: result = 48; break;
				case CSTYPE_HMAC_SHA512: result = 64; break;
				default:
					CSI_DEBUG_ERR("x hsha_parm_get_len: Unknown suite %d", suite);
					break;
			}
		}
		break;

		default:
			CSI_DEBUG_ERR("x gcm_crypt_csi_crypt_parm_get_lenget_parm: Unknown parm id %d", parmid);
	}

	return result;
}


extern csi_val_t hsha_rand(csi_csid_t suite, uint32_t len)
{
	csi_val_t result;

	int retVal = 0;

	memset(&result, 0, sizeof(csi_val_t));
	if ((result.contents = MTAKE(len)) == NULL)
	{
		CSI_DEBUG_ERR("x hsha_rand: Cannot allocate result of size %d", len);
		return result;
	}

	result.len = len;
	switch (suite)
	{
        case CSTYPE_HMAC_SHA256:
        	retVal = mbedtls_hmac_drbg_random(&g_csi_hmac_drbg_256_ctx, result.contents, result.len);
        	break;
        case CSTYPE_HMAC_SHA384:
        	retVal = mbedtls_hmac_drbg_random(&g_csi_hmac_drbg_384_ctx, result.contents, result.len);
        	break;
        case CSTYPE_HMAC_SHA512:
        	retVal = mbedtls_hmac_drbg_random(&g_csi_hmac_drbg_512_ctx, result.contents, result.len);
        	break;
        default:
        	retVal = -1;
	    	CSI_DEBUG_ERR("x hsha_rand: Unsupported suite: %d.", suite);
	}

	if (retVal != 0)
	{
		CSI_DEBUG_ERR("x hsha_rand: Cannot generate number of len %d. \
Error %d.", len, retVal);
		MRELEASE(result.contents);
		result.contents = NULL;
		result.len = 0;
	}

	return result;
}

void hsha_teardown()
{
    mbedtls_hmac_drbg_free( &g_csi_hmac_drbg_256_ctx );
    mbedtls_hmac_drbg_free( &g_csi_hmac_drbg_384_ctx );
    mbedtls_hmac_drbg_free( &g_csi_hmac_drbg_512_ctx );
}



/******************************************************************************
 *
 * \par Function Name: hsha_blocksize
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
 * \return Bocksize or 0 on error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/12/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t hsha_blocksize(csi_csid_t suite)
{
	return 65000;
}


/******************************************************************************
 *
 * \par Function Name: hsha_ctx_len
 *
 * \par Return the maximum length of a context for this given ciphersuite.
 *
 * \param[in]     suite    The ciphersuite context length being queried.
 *
 * \par Notes:
 *
 * \return Context Length.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/11/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t hsha_ctx_len(csi_csid_t suite)
{
	return sizeof( mbedtls_md_context_t );
}



/******************************************************************************
 *
 * \par Function Name: hsha_ctx_init
 *
 * \par Initialize a ciphersuite context.
 *
 * \param[in]     suite    The ciphersuite whose context is being initialized.
 * \param[in/out] key_info Key information related to the ciphersuite action.
 * \param[in/out] svc      The service being performed (sign or verify).
 *
 * \par Notes:
 *  - The context is allocated by MTAKE and must be freed my MRELEASE.
 *  - The md_info structure, while a pointer, points to a common structure and
 *    must NOT be freed.
 *
 * \return NULL or created/initialized context.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/12/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

mbedtls_md_context_t *hsha_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc)
{
	mbedtls_md_context_t *ctx;
	uint32_t size = hsha_ctx_len(suite);
	mbedtls_md_info_t *md_info;
	mbedtls_md_type_t md_type = MBEDTLS_MD_NONE;
	uint32_t result = 0;

	/* Step 1: Allocate the new context. */
	if((ctx = MTAKE(size)) == NULL)
	{
		CSI_DEBUG_ERR("x hsha_ctx_init: Can't allocate context of size %d",size);
		return NULL;
	}

	/* Step 2: Initialize the context. */
	mbedtls_md_init(ctx);

	/* Step 3: Set up the context. */
	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
		md_type = MBEDTLS_MD_SHA1;
		break;
	case CSTYPE_HMAC_SHA256:
		md_type = MBEDTLS_MD_SHA256;
		break;
	case CSTYPE_HMAC_SHA384:
		md_type = MBEDTLS_MD_SHA384;
		break;
	case CSTYPE_HMAC_SHA512:
		md_type = MBEDTLS_MD_SHA512;
	default:
		break;
	}

	if(md_type == MBEDTLS_MD_NONE)
	{
		CSI_DEBUG_ERR("x hsha_ctx_init: Unsupported suite: %d", suite);
		hsha_ctx_free(suite, ctx);
		return NULL;
	}

	if((md_info = (mbedtls_md_info_t *)mbedtls_md_info_from_type(md_type)) == NULL)
	{
		CSI_DEBUG_ERR("x hsha_ctx_init: Can't get MD info for suite %d", suite);
		hsha_ctx_free(suite, ctx);
		return NULL;
	}

	if((result = mbedtls_md_setup(ctx, md_info, 1)) != 0)
	{
		CSI_DEBUG_ERR("x hsha_ctx_init: Error setting up context: %d", result);
		hsha_ctx_free(suite, ctx);
		return NULL;
	}

	if((result = mbedtls_md_hmac_starts(ctx, key_info.contents, key_info.len)) != 0)
	{
		CSI_DEBUG_ERR("x hsha_ctx_init: Error starting signing: %d", result);
		hsha_ctx_free(suite, ctx);
		return NULL;
	}

	return ctx;
}



/******************************************************************************
 *
 * \par Function Name: hsha_ctx_free
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
 *  02/12/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t  hsha_ctx_free(csi_csid_t suite, void *context)
{
	mbedtls_md_context_t *ctx = (mbedtls_md_context_t *) context;

	if(ctx != NULL)
	{
		mbedtls_md_free(ctx);
		MRELEASE(ctx);
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: hsha_sign_res_len
 *
 * \par Return the length of the raw ciphersuite result field.
 *
 * \param[in]  suite      The ciphersuite being used.
 * \param[in]  context    Cryptographic context
 *
 * \return The length of the security result.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/12/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t hsha_sign_res_len(csi_csid_t suite, void *context)
{
	mbedtls_md_context_t *ctx = (mbedtls_md_context_t *) context;
	uint32_t result = 0;

	if(ctx == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_res_len: NULL context provided.", NULL);
		return 0;
	}

	if(ctx->md_info == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_res_len: No MD Info in context.", NULL);
		return 0;
	}

	if((result = (uint32_t) mbedtls_md_get_size(ctx->md_info)) == 0)
	{
		CSI_DEBUG_ERR("x hsha_sign_res_len: Could not get size for result.", NULL);
		return 0;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: hsha_sign_start
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

int8_t  hsha_sign_start(csi_csid_t suite, void *context)
{
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: hsha_sign_update
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
 *  02/12/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t  hsha_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc)
{
	mbedtls_md_context_t *ctx = (mbedtls_md_context_t *) context;
	uint32_t result = 0;

	if(ctx == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_update: NULL context provided.", NULL);
		return ERROR;
	}

	if(ctx->md_info == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_update: No MD Info in context.", NULL);
		return ERROR;
	}

	if((result = mbedtls_md_hmac_update(ctx, data.contents, data.len)) != 0)
	{
		CSI_DEBUG_ERR("x hsha_sign_update: Error updating: %d.", result);
		return ERROR;
	}

	return 1;
}






/******************************************************************************
 *
 * \par Function Name: hsha_sign_finish
 *
 * \par Finish a streaming operation from a ciphersuite.
 *
 * \param[in]     suite    The ciphersuite whose context is being finished.
 * \param[in/out] context  The context being finished.
 * \param[out]    digest   The security result.
 * \param[in]     svc      Service being performed (sign or verify)
 *
 * \par Notes:
 *
 * \return 1      - Success
 *         0      - Config error
 *         4      - Logical issue (such as verification failure)
 *         ERROR  - System error
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/12/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t  hsha_sign_finish(csi_csid_t suite, void *context, csi_val_t *digest, csi_svcid_t svc)
{
	mbedtls_md_context_t *ctx = (mbedtls_md_context_t *) context;
	uint32_t success = 0;
	int8_t result = 1;

	if(ctx == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_finish: NULL context.", NULL);
		return ERROR;
	}

	if(digest == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_finish: NULL result.", NULL);
		return ERROR;
	}

	if(svc == CSI_SVC_SIGN)
	{
		digest->len = hsha_sign_res_len(suite, context);
		if((digest->contents = MTAKE(digest->len)) == NULL)
		{
			CSI_DEBUG_ERR("x hsha_sign_finish: Can't allocate result of size %d.", digest->len);
			digest->len = 0;
			return ERROR;
		}

		if((success = mbedtls_md_hmac_finish(ctx, digest->contents)) != 0)
		{
			CSI_DEBUG_ERR("x hsha_sign_finish: Unable to finish signing: %d", success);
			MRELEASE(digest->contents);
			digest->len = 0;
			return ERROR;
		}
	}
	else if(svc == CSI_SVC_VERIFY)
	{
		csi_val_t loc_digest;

		/* First, make sure the passed-in digest is the right length. */
		if(digest->len != csi_sign_res_len(suite, context))
		{
			CSI_DEBUG_ERR("x hsha_sign_finish: Wrong length digest in \
	BIB: %d != %d.", digest->len, csi_sign_res_len(suite, context));
			return 0;
		}

		/* Second, Calculate a local digest. */
		loc_digest.len = hsha_sign_res_len(suite, context);
		if((loc_digest.contents = MTAKE(loc_digest.len)) == NULL)
		{
			CSI_DEBUG_ERR("x hsha_sign_finish: Can't allocate result of size %d.", loc_digest.len);
			return ERROR;
		}

		if((success = mbedtls_md_hmac_finish(ctx, loc_digest.contents)) != 0)
		{
			CSI_DEBUG_ERR("x hsha_sign_finish: Unable to finish signing: %d", success);
			MRELEASE(loc_digest.contents);
			return ERROR;
		}


		/* COmpare the local digest with the passed-in digest. */
		if (memcmp(digest->contents, loc_digest.contents, digest->len) == 0)
		{
			result = 1;	/*	Target block not altered.	*/
		}
		else
		{
			CSI_DEBUG_WARN("x hsha_sign_finish: digests don't match.",
					NULL);
			result = 4;	/*	Target block was altered.	*/
		}

		MRELEASE(loc_digest.contents);
	}
	else
	{
		CSI_DEBUG_ERR("x hsha_sign_finish: Bad service: %x.", svc);
		result = ERROR;
	}

	return result;
}





/******************************************************************************
 *
 * \par Function Name: hsha_sign_full
 *
 * \par Apply a ciphersuite to a given set of input data.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  input    The input to the ciphersuite
 * \param[in]  key      The key to use for the ciphersuite service.
 * \param[in|out] digest The message digest (created on SIGN, checked on VERIFY)
 * \param[in]  svc      Cryptographic service to perform (sign or verify)
 *
 * \par Notes:
 *	    - The return result structure will have its contents area allocated
 *	      and that contents area MUST be freed when the result is no longer
 *	      necessary.
 *
 * \return 1      - Success
 *         0      - Config error
 *         4      - Logical issue (such as verification failure)
 *         ERROR  - System error
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/12/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *  03/12/16  E. Birrane     Update to include verify service. [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/
int8_t hsha_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *digest, csi_svcid_t svc)
{
	mbedtls_md_info_t *md_info;
	mbedtls_md_type_t md_type = MBEDTLS_MD_NONE;
	uint8_t retval = ERROR;
	uint32_t digest_len = 0;

	CHKERR(digest);


	/* Step 1: Grab the MD Info. */
	switch(suite)
	{
	case CSTYPE_HMAC_SHA1:
		md_type = MBEDTLS_MD_SHA1;
		break;
	case CSTYPE_HMAC_SHA256:
		md_type = MBEDTLS_MD_SHA256;
		break;
	case CSTYPE_HMAC_SHA384:
		md_type = MBEDTLS_MD_SHA384;
		break;
	default:
		break;
	}

	if(md_type == MBEDTLS_MD_NONE)
	{
		CSI_DEBUG_ERR("x hsha_sign_full: Unsupported suite: %d", suite);
		return ERROR;
	}

	if((md_info = (mbedtls_md_info_t *)mbedtls_md_info_from_type(md_type)) == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_full: Can't get MD info for suite %d", suite);
		return ERROR;
	}

	if((digest_len = (uint32_t) mbedtls_md_get_size(md_info)) == 0)
	{
		CSI_DEBUG_ERR("x hsha_sign_full: Could not get size for result.", NULL);
		return ERROR;
	}

	/* If we are signing, allocate the digest, populate it, and return it. */
	if(svc == CSI_SVC_SIGN)
	{
		digest->len = digest_len;

		if((digest->contents = MTAKE(digest->len)) == NULL)
		{
			CSI_DEBUG_ERR("x hsha_sign_full: Can't allocate result of size %d.", digest->len);
			digest->len = 0;
			return ERROR;
		}

		if((retval = mbedtls_md_hmac(md_info, key.contents, key.len, input.contents, input.len, digest->contents)) != 0)
		{
			CSI_DEBUG_ERR("x hsha_sign_full: Error signing: %d.", retval);
			MRELEASE(digest->contents);
			return ERROR;
		}
	}

	/* If we are verifying, calculate a local digest and compare it to the
	 *
	 */
	else if(svc == CSI_SVC_VERIFY)
	{
		csi_val_t loc_digest;

		/* First, make sure the passed-in digest is the right length. */
		if(digest->len != digest_len)
		{
			CSI_DEBUG_ERR("x hsha_sign_full: Bad digest len. %d != %d.", digest->len, digest_len);
			return ERROR;
		}

		/* Second, Calculate a local digest. */
		loc_digest.len = digest_len;
		if((loc_digest.contents = MTAKE(loc_digest.len)) == NULL)
		{
			CSI_DEBUG_ERR("x hsha_sign_full: Can't allocate result of size %d.", loc_digest.len);
			return ERROR;
		}

		if((retval = mbedtls_md_hmac(md_info, key.contents, key.len, input.contents, input.len, loc_digest.contents)) != 0)
		{
			CSI_DEBUG_ERR("x hsha_sign_full: Unable to finish signing: %d", retval);
			MRELEASE(loc_digest.contents);
			return ERROR;
		}

		/* COmpare the local digest with the passed-in digest. */
		if (memcmp(digest->contents, loc_digest.contents, digest->len) == 0)
		{
			retval = 1;	/*	Target block not altered.	*/
		}
		else
		{
			CSI_DEBUG_WARN("x hsha_sign_full: digests don't match.",
					NULL);
			retval = 4;	/*	Target block was altered.	*/
		}

		MRELEASE(loc_digest.contents);
	}
	else
	{
		CSI_DEBUG_ERR("x hsha_sign_finish: Bad service: %x.", svc);
		retval = ERROR;
	}

	return retval;
}

