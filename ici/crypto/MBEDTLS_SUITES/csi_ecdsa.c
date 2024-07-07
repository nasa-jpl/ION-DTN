/*****************************************************************************
 **
 ** File Name: csi_ecdsa.c
 **
 ** Description: This file defines implementations of the ION Cryptographic
 **              interface as related to elliptic curve DSA suites.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **   - This implementation assumes that ECDSA is signing a SHA hash.
 **   - This implementation assumes that the Q values are provided by the
 **     calling function. There is no key management or exchange built into
 **     this implementation.
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
#include "csi_debug.h"
#include "csi_ecdsa.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"

#if (CSI_DEBUGGING == 1)
extern char	gMsg[];		/*	Debug message buffer.	*/
#endif


/******************************************************************************
 *
 * \par Function Name: ecdsa_blocksize
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
 * \return Blocksize,or 0 on error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t  ecdsa_blocksize(csi_csid_t suite)
{
	return 65000;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_ctx_build
 *
 * \par Builds an ECDSA context.
 *
 * \param[in] suite    The ciphersuite being used for this context.
 * \param[in] key_info The key to use for this context.
 *
 * \par Notes:
 *
 * \return The built context, or NULL on error
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

mbedtls_ecdsa_context *ecdsa_ctx_build(csi_csid_t suite, csi_val_t *key_info)
{
	mbedtls_ecdsa_context *result = NULL;
//	mbedtls_ctr_drbg_context ctr_drbg;
	uint32_t size = sizeof(mbedtls_ecdsa_context);
	int retval = 0;

	/* Step 1: Allocate the context. */
	if((result = MTAKE(size)) == NULL)
	{
		CSI_DEBUG_ERR("x ecdsa_loc_build_ctx: Can't allocate context of size %d", size);
		return NULL;
	}

	/* Step 2: Initialize. This just zeros the memory. */
	mbedtls_ecdsa_init(result);

	/* Step 3: Pick the curve based on the suite. */
	if(suite == CSTYPE_ECDSA_SHA256)
	{
		mbedtls_ecp_group_load(&(result->grp), MBEDTLS_ECP_DP_SECP256R1);
	}
	else if(suite == CSTYPE_ECDSA_SHA384)
	{
		mbedtls_ecp_group_load(&(result->grp), MBEDTLS_ECP_DP_SECP384R1);
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_loc_build_ctx: Unknown suite %d", suite);
		mbedtls_ecdsa_free(result);
		MRELEASE(result);
		return NULL;
	}

	if((key_info->contents != NULL) && (key_info->len > 0))
	{
		/* Step 4: Extract the public and private key, encoded as a TLV, in
		 *	 	       the passed-in key.
		 */

		csi_val_t Q = csi_extract_tlv(0, key_info->contents, key_info->len);
		csi_val_t d = csi_extract_tlv(1, key_info->contents, key_info->len);

#ifdef CSI_DEBUGGING
		char *str;
		if((str = csi_val_print(Q, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i ecdsa_ctx_build: Read Q value of %s...",str);
			MRELEASE(str);
		}

		if((str = csi_val_print(d, 20)) != NULL)
		{
			CSI_DEBUG_INFO("i ecdsa_ctx_build: Read d value of %s...",str);
			MRELEASE(str);
		}
#endif

		if((Q.len == 0) || (Q.contents == NULL))
		{
			CSI_DEBUG_ERR("x ecdsa_loc_build_ctx, Can't read Q from passed-in key of length %d.", key_info->len);
			mbedtls_ecdsa_free(result);
			MRELEASE(result);
			return NULL;
		}

		if((d.len == 0) || (d.contents == NULL))
		{
			CSI_DEBUG_ERR("x ecdsa_loc_build_ctx, Can't read d from passed-in key of length %d.", key_info->len);
			MRELEASE(Q.contents);
			mbedtls_ecdsa_free(result);
			MRELEASE(result);
			return NULL;
		}

		if((mbedtls_mpi_read_binary(&(result->d), d.contents, d.len)) != 0)
		{
			CSI_DEBUG_ERR("x ecdsa_loc_build_ctx: Can't read d from  buffer of size %d.", d.len);
			mbedtls_ecdsa_free(result);
			MRELEASE(Q.contents);
			MRELEASE(d.contents);
			MRELEASE(result);
			return NULL;
		}

		if((retval = mbedtls_ecp_point_read_binary(&(result->grp), &(result->Q), Q.contents, Q.len)) != 0)
		{
			CSI_DEBUG_ERR("x ecdsa_loc_build_ctx: Can't read Q from  buffer of size %d. Error 0x%x.", Q.len, retval);
			mbedtls_ecdsa_free(result);
			MRELEASE(Q.contents);
			MRELEASE(d.contents);
			MRELEASE(result);
			return NULL;
		}

		MRELEASE(Q.contents);
		MRELEASE(d.contents);
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_ctx_len
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
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t  ecdsa_ctx_len(csi_csid_t suite)
{
	return sizeof(csi_ecdsa_ctx_t);
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_ctx_free
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
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t ecdsa_ctx_free(csi_csid_t suite, void *context)
{
	csi_ecdsa_ctx_t *csi_ecdsa_ctx = (csi_ecdsa_ctx_t *) context;

	/* Step 1: If NULL, nothing to free. */
	if(csi_ecdsa_ctx == NULL)
	{
		return 1;
	}

	/* Step 2: Make sure the caller is freeing the correct kind of context. */
	if(suite != csi_ecdsa_ctx->type)
	{
		CSI_DEBUG_ERR("x ecdsa_ctx_free: Type mismatch. %d != %d", suite, csi_ecdsa_ctx->type);
		return ERROR;
	}

	/* Step 3: Free the ECDSA context, if it exists. */
	if(csi_ecdsa_ctx->ecdsa_ctx != NULL)
	{
		mbedtls_ecdsa_free(csi_ecdsa_ctx->ecdsa_ctx);
		MRELEASE(csi_ecdsa_ctx->ecdsa_ctx);
	}

	/* Step 4: Free the SHA context, based on its type. */
	if(csi_ecdsa_ctx->type == CSTYPE_ECDSA_SHA256)
	{
		 mbedtls_sha256_context *ctx = ( mbedtls_sha256_context *) csi_ecdsa_ctx->sha_ctx;
		 mbedtls_sha256_free(ctx);
		 MRELEASE(ctx);
	}
	else if(csi_ecdsa_ctx->type == CSTYPE_ECDSA_SHA384)
	{
		 mbedtls_sha512_context *ctx = ( mbedtls_sha512_context *) csi_ecdsa_ctx->sha_ctx;
		 mbedtls_sha512_free(ctx);
		 MRELEASE(ctx);
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_ctx_free: Bad type %d. Not freeing sha context. Possible leak.",
				csi_ecdsa_ctx->type);
	}

	/* Step 5: Release the main context memory. */
	MRELEASE(csi_ecdsa_ctx);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_ctx_init
 *
 * \par Initialize a ciphersuite context.
 *
 * \param[in]     suite    The ciphersuite whose context is being initialized.
 * \param[in/out] key_info Key information related to the ciphersuite action.
 * \param[in/out] svc      The ciphersuite service being performed (sign or verify).
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
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_ecdsa_ctx_t  *ecdsa_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc)
{
	csi_ecdsa_ctx_t *csi_ecdsa_ctx;
	uint32_t size = ecdsa_ctx_len(suite);

	/* Step 1: Allocate and initialize the main context. */
	if((csi_ecdsa_ctx = MTAKE(size)) == NULL)
	{
		CSI_DEBUG_ERR("x ecdsa_ctx_init: Can't allocate ICI context of size %d",size);
		return NULL;
	}
	memset(csi_ecdsa_ctx,0,size);

	csi_ecdsa_ctx->type = suite;

	/* Step 2: Allocate and initialize the SHA context. */
	if(csi_ecdsa_ctx->type == CSTYPE_ECDSA_SHA256)
	{
		size = sizeof(mbedtls_sha256_context);
		if((csi_ecdsa_ctx->sha_ctx = MTAKE(size)) == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_ctx_init: Can't allocate SHA256 context of size %d",
					         size);
			ecdsa_ctx_free(suite, csi_ecdsa_ctx);
			return NULL;
		}

		mbedtls_sha256_init((mbedtls_sha256_context *) csi_ecdsa_ctx->sha_ctx);
		mbedtls_sha256_starts_ret((mbedtls_sha256_context *) csi_ecdsa_ctx->sha_ctx, 0);
	}
	else if(csi_ecdsa_ctx->type == CSTYPE_ECDSA_SHA384)
	{
		size = sizeof(mbedtls_sha512_context);
		if((csi_ecdsa_ctx->sha_ctx = MTAKE(size)) == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_ctx_init: Can't allocate SHA512 context of size %d",
					         size);
			ecdsa_ctx_free(suite, csi_ecdsa_ctx);
			return NULL;
		}

		mbedtls_sha512_init((mbedtls_sha512_context *) csi_ecdsa_ctx->sha_ctx);
		mbedtls_sha512_starts_ret((mbedtls_sha512_context *) csi_ecdsa_ctx->sha_ctx, 1);
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_ctx_init: Unknown suite %d", csi_ecdsa_ctx->type);
		ecdsa_ctx_free(suite, csi_ecdsa_ctx);
		return NULL;
	}

	/* Step 3: Create the ECDSA context. */
	if((csi_ecdsa_ctx->ecdsa_ctx = ecdsa_ctx_build(suite, &key_info)) == NULL)
	{
		CSI_DEBUG_ERR("x ecdsa_ctx_init: Can't create EDSA context.", NULL);
		ecdsa_ctx_free(suite, csi_ecdsa_ctx);
		return NULL;
	}

	return csi_ecdsa_ctx;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_sign_finish
 *
 * \par Finish a streaming operation from a ciphersuite.
 *
 * \param[in]     suite    The ciphersuite whose context is being finished.
 * \param[in/out] context  The context being finished.
 * \param[out]    result   The security result.
 * \param[in]     svc      Cryptographic service to perform (sign or verify)
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
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t ecdsa_sign_finish(csi_csid_t suite, void *context, csi_val_t *digest, csi_svcid_t svc)
{
	csi_ecdsa_ctx_t *ici_ecdsa_ctx = (csi_ecdsa_ctx_t *) context;
	csi_val_t hashval;
	uint32_t retval = 0;
	mbedtls_md_type_t hashalg = MBEDTLS_MD_NONE;
	mbedtls_ctr_drbg_context ctr_drbg;

	/* Step 1: If NULL, nothing to free. */
	if(ici_ecdsa_ctx == NULL)
	{
		return 1;
	}

	/* Step 2: Make sure the caller is finishing the correct kind of context. */
	if(suite != ici_ecdsa_ctx->type)
	{
		CSI_DEBUG_ERR("x ecdsa_sign_finish: Type mismatch. %d != %d", suite, ici_ecdsa_ctx->type);
		return ERROR;
	}

	if(digest == NULL)
	{
		CSI_DEBUG_ERR("x hsha_sign_finish: NULL result.", NULL);
		return ERROR;
	}

	/* Step 3: Finish the hashing based on the hash type. */

	if(ici_ecdsa_ctx->type == CSTYPE_ECDSA_SHA256)
	{
		hashval.len = 32;
		hashalg = MBEDTLS_MD_SHA256;

		if((hashval.contents = MTAKE(hashval.len)) == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_finish: Can't allocate hash value of size %d", hashval.len);
			return ERROR;
		}

		mbedtls_sha256_context *ctx = ( mbedtls_sha256_context *) ici_ecdsa_ctx->sha_ctx;
		mbedtls_sha256_finish_ret(ctx, hashval.contents);
	}
	else if(ici_ecdsa_ctx->type == CSTYPE_ECDSA_SHA384)
	{
		hashval.len = 64;
		hashalg = MBEDTLS_MD_SHA384;

		if((hashval.contents = MTAKE(hashval.len)) == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_finish: Can't allocate hash value of size %d", hashval.len);
			return ERROR;
		}
		mbedtls_sha512_context *ctx = ( mbedtls_sha512_context *) ici_ecdsa_ctx->sha_ctx;
		mbedtls_sha512_finish_ret(ctx, hashval.contents);
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_sign_finish: Unknown type. %d", ici_ecdsa_ctx->type);
		return ERROR;
	}

	/* Step 4: Sign the hash with ECDSA. */
	if(svc == CSI_SVC_SIGN)
	{
		digest->len = ecdsa_sign_res_len(suite, context);
		if((digest->contents = MTAKE(digest->len)) == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_finish: Can't allocate result of size %d.", digest->len);
			digest->len = 0;
			return ERROR;
		}

		retval = mbedtls_ecdsa_write_signature(ici_ecdsa_ctx->ecdsa_ctx, // ECDSA context
				                               hashalg,                  // Algorithm used to hash the message.
											   hashval.contents,        // Message hash
											   hashval.len,             // Length of hash
											   digest->contents,         // Buffer that will hold the signature
											   (size_t *) &(digest->len),           // Length of the signature written
											   mbedtls_ctr_drbg_random,
											   (void *) &ctr_drbg);
		if(retval != 0)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_finish: Unable to write signature. Error %x.", retval);
			retval = ERROR;
		}
		else
		{
			retval = 1;
		}

	}
	else if(svc == CSI_SVC_VERIFY)
	{
		retval = mbedtls_ecdsa_read_signature(ici_ecdsa_ctx->ecdsa_ctx, // ECDSA context
 		                          	  	      hashval.contents,         // Message Hash
											  hashval.len,              // Length of Hash
		                                      digest->contents,
											  digest->len);
		if(retval != 0)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_finish: Unable to verify signature. Error %x.", retval);
			retval = 4;
		}
		else
		{
			retval = 1;
		}

	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_sign_finish: Bad service: %x.", svc);
		retval = ERROR;
	}

	MRELEASE(hashval.contents);

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_sign_full
 *
 * \par Apply a ciphersuite to a given set of input data.
 *
 * \param[in]  suite    The ciphersuite being used.
 * \param[in]  input    The input to the ciphersuite
 * \param[in]  key      The key to use for the sign operation.
 * \param[out] result   The result, if any, of the service.
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
int8_t ecdsa_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *result, csi_svcid_t svc)
{
	csi_val_t hashval;
	mbedtls_ecdsa_context *ecdsa_ctx = NULL;
	uint32_t hashalg = 0;
	mbedtls_ctr_drbg_context ctr_drbg;
	//mbedtls_md_type_t md_type = MBEDTLS_MD_NONE;
	uint32_t retval = 0;

	CHKERR(result);

	/* Step 1: Create the ECDSA context. */
	if((ecdsa_ctx = ecdsa_ctx_build(suite, &key)) == NULL)
	{
		CSI_DEBUG_ERR("x ecdsa_sign_full: Can't create EDSA context.", NULL);
		ecdsa_ctx_free(suite, ecdsa_ctx);
		return ERROR;
	}

	/* Step 2: Hash the input data. */
	if(suite == CSTYPE_ECDSA_SHA256)
	{
		hashval.len = 32;
		hashalg = MBEDTLS_MD_SHA256;

		if((hashval.contents = MTAKE(hashval.len)) == 0)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_full: Can't allocate hash result of size %d", hashval.len);
			ecdsa_ctx_free(suite, ecdsa_ctx);
			return ERROR;
		}

		mbedtls_sha256_ret(input.contents, input.len, hashval.contents, 0);
	}
	else if(suite == CSTYPE_ECDSA_SHA384)
	{
		hashval.len = 32;
		hashalg = MBEDTLS_MD_SHA384;

		if((hashval.contents = MTAKE(hashval.len)) == 0)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_full: Can't allocate hash result of size %d", hashval.len);
			ecdsa_ctx_free(suite, ecdsa_ctx);
			return ERROR;
		}

		mbedtls_sha256_ret(input.contents, input.len, hashval.contents, 0);
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_sign_full: Unknown suite %d.", suite);
		ecdsa_ctx_free(suite, ecdsa_ctx);
		return ERROR;
	}

	/* Step 4: Sign the hash with ECDSA. */
	if(svc == CSI_SVC_SIGN)
	{

		/* Step 3: Allocate space for the result. */
		result->len = ecdsa_sign_res_len(suite, ecdsa_ctx);
		if((result->contents = MTAKE(result->len)) == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_full: Cannot allocate result length of %d.", result->len);
			result->len = 0;
			ecdsa_ctx_free(suite, ecdsa_ctx);
			MRELEASE(hashval.contents);
			return ERROR;
		}

		/* Step 4: Sign the result. */
		retval = mbedtls_ecdsa_write_signature(ecdsa_ctx, // ECDSA context
				                                   hashalg,                  // Algorithm used to hash the message.
											       hashval.contents,        // Message hash
											       hashval.len,             // Length of hash
											       result->contents,         // Buffer that will hold the signature
											       (size_t*)&(result->len),           // Length of the signature written
												   mbedtls_ctr_drbg_random,
												   (void *)&ctr_drbg);

		if(retval != 0)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_full: Unable to write signature. Error %x.", retval);
			MRELEASE(result->contents);
			ecdsa_ctx_free(suite, ecdsa_ctx);
			MRELEASE(hashval.contents);
			return ERROR;
		}
	}
	else if(svc == CSI_SVC_VERIFY)
	{
		if(result->contents == NULL)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_full: Unable to write signature. Error %x.", retval);
			ecdsa_ctx_free(suite, ecdsa_ctx);
			MRELEASE(hashval.contents);
			return ERROR;
		}

		retval = mbedtls_ecdsa_read_signature(ecdsa_ctx, // ECDSA context
 		                          	  	      hashval.contents,         // Message Hash
											  hashval.len,              // Length of Hash
		                                      result->contents,
											  result->len);
		if(retval != 0)
		{
			CSI_DEBUG_ERR("x ecdsa_sign_finish: Unable to verify signature. Error %x.", retval);
			retval = 4;
		}
		else
		{
			retval = 1;
		}
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_sign_finish: Bad service: %x.", svc);
		retval = ERROR;
	}

	MRELEASE(hashval.contents);
	ecdsa_ctx_free(suite, ecdsa_ctx);

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_sign_res_len
 *
 * \par Return the length of the raw ciphersuite result field. Raw results length
 *      is the result iteself, absent any higher-level encoding such as for
 *      inclusion in BSP blocks.
 *
 * \param[in]  suite     The ciphersuite being used.
 * \param[in]  context   Cryptographic context
 * \param[in]  blocksize Size information for the cryptofunction
 * \param[in]  svc       Cryptographic service being performed (sign or verify)
 *
 * \return The length of the security result.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t ecdsa_sign_res_len(csi_csid_t suite, void *context)
{
	return (uint32_t) MBEDTLS_ECDSA_MAX_LEN;
}




/******************************************************************************
 *
 * \par Function Name: ecdsa_sign_start
 *
 * \par Start a ciphersuite context.
 *
 * \param[in] suite     The ciphersuite whose context is being initialized.
 * \param[in] context   The ECDSA context being started.
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

int8_t   ecdsa_sign_start(csi_csid_t suite, void *context)
{
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: ecdsa_sign_update
 *
 * \par Incrementally apply a ciphersuite to a new chunk of input data.
 *
 * \param[in]     suite    The ciphersuite being used.
 * \param[in\out] context  The context being reset
 * \param[in]     data     Current chunk of data.
 * \param[in]     svc      Cryptographic Service to perform (sign or verify)
 *
 * \par Notes:
 *  - This is only used for hashing in the SHA part. ECDSA signing happens
 *    when we call finish.
 *
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/14/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t ecdsa_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc)
{

	csi_ecdsa_ctx_t *csi_ecdsa_ctx = (csi_ecdsa_ctx_t *) context;
	//uint32_t result = 0;

	if(csi_ecdsa_ctx == NULL)
	{
		CSI_DEBUG_ERR("x ecdsa_sign_update: NULL context provided.", NULL);
		return ERROR;
	}

	/* Step 2: Make sure the caller is freeing the correct kind of context. */
	if(suite != csi_ecdsa_ctx->type)
	{
		CSI_DEBUG_ERR("x ecdsa_sign_update: Type mismatch. %d != %d", suite, csi_ecdsa_ctx->type);
		return ERROR;
	}

	/* Step 3: Update the SHA context, based on its type. */
	if(csi_ecdsa_ctx->type == CSTYPE_ECDSA_SHA256)
	{
		mbedtls_sha256_context *ctx = ( mbedtls_sha256_context *) csi_ecdsa_ctx->sha_ctx;
		mbedtls_sha256_update_ret(ctx, data.contents, data.len);
	}
	else if(csi_ecdsa_ctx->type == CSTYPE_ECDSA_SHA384)
	{
		mbedtls_sha512_context *ctx = ( mbedtls_sha512_context *) csi_ecdsa_ctx->sha_ctx;
		mbedtls_sha512_update_ret(ctx, data.contents, data.len);
	}
	else
	{
		CSI_DEBUG_ERR("x ecdsa_sign_update: Bad type %d.", csi_ecdsa_ctx->type);
		return ERROR;
	}

	return 1;
}
