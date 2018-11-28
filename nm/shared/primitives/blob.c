/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: blob.c
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Binary Large Objects (BLOBs)
 **
 ** Notes:
 **
 ** Assumptions:
 **              BLOBs will be capped at 65KB in size, allowing for a 2 byte
                 length. 
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/14/16  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  08/30/18  E. Birrane     CBOR Updates and Structure Optimization (JHU/APL)
 *****************************************************************************/
#include "platform.h"

#include "../adm/adm.h"

#include "../primitives/blob.h"



/******************************************************************************
 *
 * \par Function Name: blob_append
 *
 * \par Adds data to the end of a blob object.
 *
 * \retval ERROR - The blob was not updated.
 *         1 - The blob was updated.
 *
 * \param[in|out]  blob    The blob being extended
 * \param[in]      buffer  The new data to append.
 * \param[in]      length  The length of the new data
 *
 * \par Notes:
 *		1. The BLOB data is reallocated.
 *		2. The buffer is DEEP-Copied and must be released by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/06/16  E. Birrane     Initial implementation (Secure DTN - NASA: NNX14CS58P)
 *  08/30/18  E. Birrane     Support pre-allocated BLOBs. (JHU/APL)
 *****************************************************************************/

int blob_append(blob_t *blob, uint8_t *buffer, uint32_t length)
{
	uint32_t new_len = 0;
	int success = AMP_OK;

	if((blob == NULL) || (buffer == NULL) || (length == 0))
	{
		AMP_DEBUG_ERR("blob_append","Bad Args.", NULL);
		return AMP_FAIL;
	}

	if((success = blob_grow(blob, length)) != AMP_OK)
	{
		AMP_DEBUG_ERR("blob_append","Cannot grow blob by %d bytes.", length);
		return success;
	}

	memcpy(blob->value + blob->length, buffer, length);

	blob->length += length;

	return AMP_OK;
}



/******************************************************************************
 *
 * \par Function Name: blob_create
 *
 * \par Dynamically allocate a BLOB.
 *
 * \retval NULL - The blob was not created.
 *         !NULL - The created blob.
 *
 * \param[in]  value   The BLOB value. Or NULL if the BLOB is to be empty.
 * \param[in]  length  The length of the initial data.
 * \param[in]  alloc   The size of allocate. Must be at least length in size.
 *
 * \par Notes:
 *		2. The buffer is DEEP-Copied and must be released by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/30/18  E. Birrane     Initial Implementation. (JHU/APL)
 *****************************************************************************/

blob_t * blob_create(uint8_t *value, size_t length, size_t alloc)
{

	blob_t *result = NULL;

	result = (blob_t *) STAKE(sizeof(blob_t));

	if((blob_init(result, value, length, alloc) != AMP_OK))
	{
		SRELEASE(result);
		result = NULL;
	}

	return result;

}


int blob_compare(blob_t* v1, blob_t *v2)
{
	CHKERR(v1);
	CHKERR(v2);

	if(v1->length != v2->length)
	{
		return 1;
	}

	return memcmp(v1->value, v2->value, v1->length);
}


/******************************************************************************
 *
 * \par Function Name: blob_copy
 *
 * \par Duplicates a BLOB object.
 *
 * \retval AMP_SYSERR - System Error.
 *         AMP_FAIL   - Logic Error
 *         AMP_OK     - BLOB Initialized.
 *
 * \param[in]  src  The BLOB being copied.
 * \param[out] dest The deep-copy BLOB
 *
 * \par Notes:
 *		1. The dest BLOB is allocated and must be freed when no longer needed.
 *		   This is a deep copy.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *  08/30/18  E. Birrane     Support pre-allocated BLOBs. (JHU/APL)
 *****************************************************************************/

int blob_copy(blob_t src, blob_t *dest)
{
	/* Step 0: Sanity Checks. */
	if (dest == NULL)
	{
		AMP_DEBUG_ERR("blob_copy","Bad Args.", NULL);
		return AMP_FAIL;
	}

	/* Step 1: If the source blob is empty, create an empty dest blob. */
	dest->length = src.length;
	dest->alloc = src.alloc;

	if(src.alloc == 0)
	{
		dest->value = NULL;
	}
	else
	{
		if((dest->value = (uint8_t *) STAKE(dest->alloc)) == NULL)
		{
			AMP_DEBUG_ERR("blob_copy","Can't allocate blob of size %d.", dest->alloc);
			return AMP_SYSERR;
		}
		memcpy(dest->value, src.value, dest->length);
	}

	return AMP_OK;
}


blob_t* blob_copy_ptr(blob_t *src)
{
	CHKNULL(src);

	return blob_create(src->value, src->length, src->alloc);

}

/******************************************************************************
 *
 * \par Function Name: blob_deserialize
 *
 * \par Extracts a BLOB from a byte buffer. When serialized, a
 *      BLOB is an SDNV # of bytes, followed by a byte stream.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized BLOB.
 *
 * \param[in]  cborval  The CBOR value to be deserialized
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *  08/31/18  E. Birrane     Update to CBOR. (JHU/APL)
 *****************************************************************************/

blob_t blob_deserialize(CborValue *it, int *success)
{
	blob_t result;
	CborError err;
	size_t len = 0;

	AMP_DEBUG_ENTRY("blob_deserialize","(0x"ADDR_FIELDSPEC",0x"ADDR_FIELDSPEC")", (uaddr) it, (uaddr) success);

	memset(&result, 0, sizeof(blob_t));
	*success = AMP_FAIL;

	if(!cbor_value_is_byte_string(it))
	{
		AMP_DEBUG_ERR("blob_deserialize", "Bad CBOR encoding.", NULL);
		return result;
	}

	if((err = cbor_value_get_string_length(it, &len)) != CborNoError)
	{
		AMP_DEBUG_ERR("blob_deserialize", "Cbor Error %d.", err);
		return result;
	}

	/* Create an empty-but-allocated blob. */
	result.length = len;
	result.alloc = len;
	if((result.value = STAKE(len)) == NULL)
	{
		AMP_DEBUG_ERR("blob_deserialize", "Can't make new blob.", NULL);
		return result;
	}

	/* Copy bytestring value into the BLOB */
	err = cbor_value_copy_byte_string(it, result.value, &(result.length), it);
	if((err != CborNoError) && (err != CborErrorUnexpectedEOF))
	{
		AMP_DEBUG_ERR("blob_deserialize", "Cbor Error %d.", err);
		blob_release(&result, 0);
		memset(&result, 0, sizeof(blob_t));
		return result;
	}

	*success = AMP_OK;
	return result;
}

blob_t *blob_deserialize_ptr(CborValue *it, int *success)
{
	blob_t *result = NULL;

	if((result = (blob_t*)STAKE(sizeof(blob_t))) == NULL)
	{
		AMP_DEBUG_ERR("blob_deserialize_ptr","Can't allocate new struct.", NULL);
		*success = AMP_FAIL;
	}

	*result = blob_deserialize(it, success);
	if(*success != AMP_OK)
	{
		SRELEASE(result);
		result = NULL;
	}

	return result;
}



int blob_grow(blob_t *blob, uint32_t length)
{
	uint32_t new_len= 0;
	CHKUSR(blob, AMP_FAIL);

	new_len = blob->length + length;

	/* Reallocate the BLOB if necessary. */
	if(new_len > blob->alloc)
	{
		uint8_t *new_data = NULL;

		if((new_data = STAKE(new_len)) == NULL)
		{
			AMP_DEBUG_ERR("blob_append","Can't allocate %d bytes.", new_len);
			return AMP_SYSERR;
		}

		if(blob->length > 0)
		{
			memcpy(new_data, blob->value, blob->length);
		}

		if(blob->value != NULL)
		{
			SRELEASE(blob->value);
		}

		blob->value = new_data;
		blob->alloc = new_len;
	}

	return AMP_OK;
}

/******************************************************************************
 *
 * \par Function Name: blob_init
 *
 * \par Initializes a BLOB object.
 *
 * \retval AMP_SYSERR - System Error.
 *         AMP_FAIL   - Logic Error
 *         AMP_OK     - BLOB Initialized.
 *
 * \param[in|out] blob   The BLOB being initialized.
 * \param[in]     value  The BLOB value.
 * \param[in]     length The length of the value in bytes
 * \param[in]     alloc  The size to allocate for the blob.
 *
 * \par Notes:
 *   - This was previously blob_create
 *   - This is a deep copy.
 *   - value can be NULL. In which case we will allocate zero'd space.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *  08/30/18  E. Birrane     Support pre-allocated BLOBs. (JHU/APL)
 *****************************************************************************/

int blob_init(blob_t *blob, uint8_t *value, size_t length, size_t alloc)
{
	if((blob == NULL) || (alloc == 0))
	{
		AMP_DEBUG_ERR("blob_create","Bad args.", NULL);
		return AMP_FAIL;
	}

	if((blob->value = (uint8_t*)STAKE(alloc)) == NULL)
	{
		AMP_DEBUG_ERR("blob_create","Can't allocate %d bytes.", alloc);
		return AMP_SYSERR;
	}

	memset(blob->value, 0, alloc);

	blob->length = length;
	blob->alloc = alloc;

	if(value != NULL)
	{
		memcpy(blob->value, value, length);
	}

	return AMP_OK;
}


/******************************************************************************
 *
 * \par Function Name: blob_release
 *
 * \par Releases all memory allocated in the BLOB.
 *
 * \param[in,out] blob     The BLOB whose contents are to be destroyed.
 * \param[in]     destroy  Whether to destroy (!0) the blob container or not (0)
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *  09/02/18  E. Birrane     Update to latest spec. (JHU/APL)
 *****************************************************************************/

void blob_release(blob_t *blob, int destroy)
{
	if(blob == NULL)
	{
		return;
	}

	if(blob->value != NULL)
	{
		SRELEASE(blob->value);
	}

	if(destroy != 0)
	{
		SRELEASE(blob);
	}
}



/******************************************************************************
 *
 * \par Function Name: blob_serialize
 *
 * \par Purpose: Generate full, serialized version of a BLOB as a CBOR
 *               bytestring.
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized blob.
 *
 * \param[in]  blob    The BLOB to be serialized.
 * \param[out] size    The size of the serialized BLOB.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation, (Secure DTN - NASA: NNX14CS58P)
 *  08/31/18  E. Birrane     Update to CBOR. (JHU/APL)
 *****************************************************************************/

blob_t* blob_serialize_wrapper(blob_t *blob)
{
	return cut_serialize_wrapper(BLOB_DEFAULT_ENC_SIZE, blob, blob_serialize);
}


CborError blob_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	blob_t *blob = (blob_t *) item;

	if(blob == NULL)
	{
		return CborErrorIO;
	}

	return cbor_encode_byte_string(encoder, blob->value, blob->length);
}

/*
 * For now, just adjust the length, no need to reallocate if shrinking.
 */
/******************************************************************************
 *
 * \par Function Name: blob_trim
 *
 * \par Purpose: Shorten a blob by X bytes.
 *
 * \retval Whether the blob was trimmed or not.
 *
 * \param[in|out] blob    The BLOB to be trimmed.
 * \param[in]     size    The # bytes to trim.
 *
 * \par Notes:
 *		1. This does not reduce the allocated size of the blob.
 *		2. Trying to trim more blob than exists is an error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation, (Secure DTN - NASA: NNX14CS58P)
 *  08/31/18  E. Birrane     Update to CBOR. (JHU/APL)
 *****************************************************************************/
int8_t blob_trim(blob_t *blob, uint32_t length)
{
	CHKERR(blob);

	if(blob->length <= length)
	{
		return AMP_FAIL;
	}

	blob->length -= length;

	return AMP_OK;
}
