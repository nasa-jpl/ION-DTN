/*****************************************************************************
 **
 ** File Name: csi.c
 **
 ** Description: This file defines NULL implementations of the ION crypto
 **		 interface.
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
 *****************************************************************************/

#include "platform.h"
#include "csi.h"
#include "csi_debug.h"


/*****************************************************************************
 *                         NULL Crypto Functions                             *
 *****************************************************************************/
char	*crypto_suite_name = "NULL_SUITES";
char	gCsiMsg[GMSG_BUFLEN];		/*	Debug message buffer.	*/


/******************************************************************************
 *
 * \par Function Name: csi_extract_tlv
 *
 * \par Purpose: This function searches within a Lyst (a ciphersuite
 *               parameters field or a security results field) of an
 *               inbound sbsp block for an information item of specified type.
 *
 * \retval The LV requested.  Len = 0 indicates not found.
 *
 * \param[in] itemNeeded The code number of the type of item to search
 *                       for.  Valid item type codes are defined in
 *                       sbsp.h as SBSP_CSPARM_xxx macros.
 * \param[in] items      The items to search through
 *
 * \par Notes:
 *      1. If the required items is not present in the list,
 *         the parameter is represented as having length 0.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t csi_extract_tlv(uint8_t itemNeeded, Lyst items)
{
	csi_val_t 	result;
	LystElt	  	elt;
	BsecInboundTv	*tv;

	CSI_DEBUG_PROC("+ csi_extract_tlv(%d, 0x"ADDR_FIELDSPEC")",
			       itemNeeded, (uaddr) items);

	memset(&result,0, sizeof(csi_val_t));	/*	Default.	*/

	/* Step 0 - Sanity Check. */
	if ((items == NULL))
	{
		CSI_DEBUG_ERR("x csi_extract_tlv - Bad Items.", NULL);
		CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
			result.len);
		return result;
	}

	/**
	 *  Step 1 - Walk through all items in the list searching for an
	 *           item of the indicated type.
	 */

	for (elt = lyst_first(items); elt; elt = lyst_next(elt))
	{
		tv = (BpsecInboundTv *) lyst_data(elt);
		if (tv->id != itemNeeded || tv->length == 0)
		{
			continue;
		}

		if ((result.contents = MTAKE(tv->length)) == NULL)
		{
			CSI_DEBUG_ERR("x csi_extract_tlv: Cannot allocate size \
of %d.", tv->length);
			CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
					result.len);
			return result;
		}

		memcpy(result.contents, (char *) (tv->value), tv->length);
		result.len = tv->length;

		CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)",
				result.len);
		return result;
	}

	CSI_DEBUG_PROC("- csi_extract_tlv -> result (len=%d)", result.len);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: csi_build_parms
 *
 * \par Purpose: This utility function builds a parameter-set structure
 *		 from an input parameters lyst.
 *
 * \retval The built parameters structure.
 *
 * \param[in] items    The list of deserialized parameters
 *
 * \par Notes:
 *      1. If a parameter in the structure is not present in the list,
 *         the parameter is represented as having length 0.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  02/27/16  E. Birrane    Initial Implementation [Secure DTN
 *                          implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_cipherparms_t csi_build_parms(Lyst items)
{
	csi_cipherparms_t result;

	CSI_DEBUG_PROC("+ csi_build_parms(0x" ADDR_FIELDSPEC, (uaddr) items);

	memset(&result, 0, sizeof(csi_cipherparms_t));

	result.iv = csi_extract_tlv(CSI_PARM_IV, items);
	result.intsig = csi_extract_tlv(CSI_PARM_INTSIG, items);
	result.salt = csi_extract_tlv(CSI_PARM_SALT, items);
	result.icv = csi_extract_tlv(CSI_PARM_ICV, items);
	result.keyinfo = csi_extract_tlv(CSI_PARM_KEYINFO, items);

	CSI_DEBUG_PROC("- csi_build_parms -> parms", NULL);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: csi_build_tlv
 *
 * \par Purpose: This utility function builds a TLV from individual fields.
 *
 * \par Date Written:  2/27/2016
 *
 * \retval The resulting TLV structure. Length 0 indicates error.
 *
 * \param[in] id       The type of data being written.
 * \param[in] len      The length of the value field.
 * \param[in] contents The value field.
 *
 * \par Notes:
 *      1. The TV structure is allocated and must be released.
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

	CSI_DEBUG_PROC("+ csi_build_tlv(%d, %d, 0x"ADDR_FIELDSPEC")", id, len, (uaddr)contents);

	memset(&result, 0, sizeof(result));

	/* Step 0 - Sanity checks. */

	if((len == 0) || (contents == NULL))
	{
		CSI_DEBUG_ERR("x csi_build_tlv: Bad parms.", NULL);
		CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", result.len);

		return result;
	}

	/* Step 1 - Encode the length of the parameter. */

	result.len = len;
	result.id = id;

	/* Step 2 - Allocate space for the parameter. */
	result.contents = MTAKE(len);
	if (result.contents == NULL)
	{
		CSI_DEBUG_ERR("x csi_build_tlv: Can't allocate result of \
length %d.", len);

		CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", len);

		return result;
	}

	/* Step 3 - Populate parameter. */

	memcpy(result.contents, contents, len);

	CSI_DEBUG_PROC("- csi_build_tlv -> result (len=%d)", len);
	return result;
}


int8_t	csi_crypt_key(csi_csid_t suite, csi_svcid_t svc,
		csi_cipherparms_t *parms, csi_val_t longtermkey,
		csi_val_t input, csi_val_t *output)
{
	int8_t retval = ERROR;

	output->len = 20;
	if ((output->contents = MTAKE(output->len)) == 0)
	{
		output->len = 0;
		return ERROR;
	}

	memset(output->contents, 0, output->len);
	CSI_DEBUG_PROC("- csi_crypt_key ->%d", retval);
	return 0;
}


//2/21
void csi_cipherparms_free(csi_cipherparms_t parms)
{
	return;
}

// 2/21
void csi_init()
{
	return;
}

// 2/21
csi_val_t csi_rand(uint32_t len)
{
	csi_val_t result;

	memset(&result, 0, sizeof(result));

	if((result.contents = MTAKE(len)) == NULL)
	{
		CSI_DEBUG_ERR("x csi_rand: Cannot allocate result of size %d",
				len);
		return result;
	}

	result.len = len;

	return result;
}


void      csi_teardown()
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_blocksize(csi_csid_t suite)
{
	return 65000;
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
 *  02/27/15  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_ctx_len(csi_csid_t suite)
{
	return 20;
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

	if((context = MTAKE(20)) == NULL)
	{
		return NULL;
	}

	memset(context, 0, 20);

	if(key_info.len > 0)
	{
		memcpy(context, key_info.contents, MIN(20, key_info.len));
	}

	return context;
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
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint8_t  csi_ctx_free(csi_csid_t suite, void *context)
{
	if (context != NULL)
	{
		MRELEASE(context);
	}

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
 * \return The length of the security result.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

uint32_t csi_sign_res_len(csi_csid_t suite, void *context)
{
	return 20;
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

int8_t  csi_sign_start(csi_csid_t suite, void *context)
{
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
 * \return 1 or ERROR.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t  csi_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc)
{
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
	CHKERR(context);

	result->len = 20;
	result->contents = MTAKE(result->len);
	memset(result->contents, 0, 20);

	return 1;
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
 *	    - The returned output structure MUST be correctly released by the
 *	      calling function.
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

int8_t	csi_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key,
		csi_val_t *result, csi_svcid_t svc)
{
	CHKERR(result);

	memset(result,0,sizeof(csi_val_t));

	result->len = 20;
	if ((result->contents = (uint8_t *) MTAKE(result->len)) == NULL)
	{
		result->len = 0;
		return ERROR;
	}

	memset(result->contents, 0, result->len);
	return 1;
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
 *  02/27/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

int8_t	csi_crypt_finish(csi_csid_t suite, void *context, csi_svcid_t svc,
		csi_cipherparms_t *parms)
{
	CHKERR(context);

	parms->icv.len = 20;
	if ((parms->icv.contents = MTAKE(parms->icv.len)) == NULL)
	{
		parms->icv.len = 0;
		return ERROR;
	}

	memset(parms->icv.contents, 0, parms->icv.len);

	return 1;
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

int8_t	csi_crypt_full(csi_csid_t suite, csi_svcid_t svc,
		csi_cipherparms_t *parms, csi_val_t key, csi_val_t input,
		csi_val_t *output)
{

	output->len = 20;
	if ((output->contents = (uint8_t *) MTAKE(output->len)) == NULL)
	{
		return ERROR;
	}

	memset(output->contents, 0, output->len);
	memcpy(output->contents, input.contents, MIN(output->len, input.len));

	return 1;
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

csi_val_t csi_crypt_parm_get(csi_csid_t suite, csi_parmid_t parmid)
{
	csi_val_t result;

	if((result.contents = (uint8_t*) MTAKE(20)) == NULL)
	{
		result.len = 0;
		return result;
	}

	result.len = 20;

	memset(result.contents, 0, 20);

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
	return 20;
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

uint32_t	csi_crypt_res_len(csi_csid_t suite, void *context,
			csi_blocksize_t blocksize, csi_svcid_t svc)
{
	return blocksize.plaintextLen;
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

int8_t	csi_crypt_start(csi_csid_t suite, void *context,
		csi_cipherparms_t parms)
{
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
 * \return ciphersuite output. Length 0 indicates error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  02/26/16  E. Birrane     Initial Implementation [Secure DTN
 *                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

csi_val_t	csi_crypt_update(csi_csid_t suite, void *context,
			csi_svcid_t svc, csi_val_t data)
{

	csi_val_t result;

	result.len = data.len;
	result.contents = MTAKE(result.len);
	memcpy(result.contents, data.contents, result.len);
	return result;
}
