/*****************************************************************************
 **
 ** File Name: blob.c
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Binry Large Objects (BLOBs)
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/14/16  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
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
 *****************************************************************************/

int8_t blob_append(blob_t *blob, uint8_t *buffer, uint32_t length)
{
	uint32_t new_len = 0;
	uint8_t *new_data = NULL;

	if((blob == NULL) || (buffer == NULL) || (length == 0))
	{
		AMP_DEBUG_ERR("blob_append","Bad Args.", NULL);
		return ERROR;
	}

	new_len = blob->length + length;

	if((new_data = STAKE(new_len)) == NULL)
	{
		AMP_DEBUG_ERR("blob_append","Can't allocate %d bytes.", new_len);
		return ERROR;
	}

	if(blob->length > 0)
	{
		memcpy(new_data, blob->value, blob->length);
	}
	memcpy(new_data + blob->length, buffer, length);

	if(blob->value != NULL)
	{
		SRELEASE(blob->value);
	}

	blob->value = new_data;
	blob->length = new_len;

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: blob_create
 *
 * \par Creates a BLOB object.
 *
 * \retval NULL - Failure
 *         !NULL - The created BLOB
 *
 * \param[in] value  The BLOB value.
 * \param[in] length The length of the value in bytes
 *
 * \par Notes:
 *		1. The returned Blob is shallow copied and MUST NOT be freed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
blob_t * blob_create(uint8_t *value, uint32_t length)
{
	blob_t *result = NULL;

	if((length != 0) && (value == NULL))
	{
		AMP_DEBUG_ERR("blob_create","Bad args.", NULL);
		return NULL;
	}

	if((result = (blob_t*)STAKE(sizeof(blob_t))) == NULL)
	{
		AMP_DEBUG_ERR("blob_create","Can't allocate %d bytes.", sizeof(blob_t));
		return NULL;
	}

	result->value = value;
	result->length = length;

	return result;
}



/******************************************************************************
 *
 * \par Function Name: blob_copy
 *
 * \par Duplicates a BLOB object.
 *
 * \retval NULL - Failure
 *         !NULL - The copied BLOB
 *
 * \param[in] blob  The BLOB being copied.
 *
 * \par Notes:
 *		1. The returned BLOB is allocated and must be freed when
 *		   no longer needed.  This is a deep copy.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

blob_t* blob_copy(blob_t *blob)
{
	blob_t *result = NULL;

	/* Step 0: Sanity Checks. */
	if(blob == NULL)
	{
		AMP_DEBUG_ERR("blob_copy","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Allocate the new entry.*/
	if((result = (blob_t*) STAKE(sizeof(blob_t))) == NULL)
	{
		AMP_DEBUG_ERR("blob_copy","Can't allocate new entry.", NULL);
		return NULL;
	}

	if(blob->length == 0)
	{
		result->length = 0;
		result->value = NULL;
		return result;
	}

	/* Step 2: Allocate the value. */
	if((result->value = (uint8_t *) STAKE(blob->length)) == NULL)
	{
		AMP_DEBUG_ERR("blob_copy","Can't allocate blob value.", NULL);
		SRELEASE(result);
		return NULL;
	}

	memcpy(result->value, blob->value, blob->length);
	result->length = blob->length;

	return result;
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
 * \param[in]  buffer       The byte buffer holding the data
 * \param[in]  buffer_size  The # bytes available in the buffer
 * \param[out] bytes_used   The # of bytes consumed in the deserialization.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/14/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

blob_t *blob_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t *bytes_used)
{
	unsigned char *cursor = NULL;
	blob_t *result = NULL;
	uint32_t bytes = 0;
	uvast len = 0;
	uint8_t *value = NULL;

	AMP_DEBUG_ENTRY("blob_deserialize","(0x"ADDR_FIELDSPEC",%d,0x"ADDR_FIELDSPEC")",
			          (uaddr) buffer, buffer_size, (uaddr) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("blob_deserialize","Bad Args", NULL);
		AMP_DEBUG_EXIT("blob_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;

	/* Grab data item length. */
	if((bytes = utils_grab_sdnv(cursor, buffer_size, &len)) == 0)
	{
		AMP_DEBUG_ERR("blob_deserialize","Can't parse SDNV.", NULL);
		return NULL;
	}

	cursor += bytes;
	buffer_size -= bytes;
	*bytes_used += bytes;

	if(len != 0)
	{
		/* Grab BLOB data. */
		if((value = (uint8_t*)STAKE(len)) == NULL)
		{
			AMP_DEBUG_ERR("blob_deserialize","Can't allocate %d bytes.", len);
			return NULL;
		}

		memcpy(value, cursor, len);
		cursor += len;
		buffer_size -= len;
		*bytes_used += len;
	}

	if((result = blob_create(value, len)) == NULL)
	{
		AMP_DEBUG_ERR("blob_deserialize","Can't create blob of length %d", len);
		SRELEASE(value);
		return NULL;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: blob_destroy
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
 *****************************************************************************/

void blob_destroy(blob_t *blob, uint8_t destroy)
{

	if(blob != NULL)
	{
		if(blob->value != NULL)
		{
			SRELEASE(blob->value);
		}

		if(destroy != 0)
		{
			SRELEASE(blob);
		}
	}
}



/******************************************************************************
 *
 * \par Function Name: blob_get_serialize_size
 *
 * \par Determine the size of the blob when it is serialized.
 *
 * \param[in]  blob   The BLOB whose serialized size is being queried.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/--/16  E. Birrane     Initial implementation,(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

uint32_t blob_get_serialize_size(blob_t *blob)
{
	Sdnv tmp;

	if(blob == NULL)
	{
		return 0;
	}

	encodeSdnv(&tmp, blob->length);
	return (blob->length + tmp.length);
}



/******************************************************************************
 *
 * \par Function Name: blob_serialize
 *
 * \par Purpose: Generate full, serialized version of a BLOB. A
 *      serialized Data Collection is of the form:
 *
 *              +---------+--------+--------+     +--------+
 *              | # Bytes | BYTE 1 | BYTE 2 | ... | BYTE N |
 *              |  [SDNV] | [BYTE] | [BYTE] |     | [BYTE] |
 *              +---------+--------+--------+     +--------+
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
 *****************************************************************************/

uint8_t* blob_serialize(blob_t *blob, uint32_t *size)
{
	uint8_t *result = NULL;
	Sdnv num_sdnv;

	AMP_DEBUG_ENTRY("blob_serialize","(0x"ADDR_FIELDSPEC")", (uaddr) blob);

	/* Step 0: Sanity Check */
	if(blob == NULL)
	{
		AMP_DEBUG_ERR("blob_serialize","Bad args.", NULL);
		AMP_DEBUG_EXIT("blob_serialize","->NULL",NULL);
		return NULL;
	}


	/* Step 1: Calculate the size. */

	/* Consider the size of the SDNV holding # data entries.*/
	encodeSdnv(&num_sdnv, blob->length);

	*size = num_sdnv.length + blob->length;

    /* Step 2: Allocate the space for the serialized BLOB. */
    if((result = (uint8_t*) STAKE(*size)) == NULL)
    {
		AMP_DEBUG_ERR("blob_serialize","Can't alloc %d bytes", *size);
		*size = 0;
		return NULL;
    }

    /* Step 4: Walk through list again copying as we go. */
    uint8_t *cursor = result;

    /* Copy over the number of data entries in the collection. */
    memcpy(cursor, num_sdnv.text, num_sdnv.length);
    cursor += num_sdnv.length;

    if(blob->length != 0)
    {
    	memcpy(cursor,blob->value, blob->length);
    	cursor += blob->length;
    }

    /* Step 5: Final sanity check. */
    if((cursor - result) != *size)
    {
		AMP_DEBUG_ERR("blob_serialize","Wrote %d bytes not %d bytes",
				        (cursor - result), *size);
		*size = 0;
		SRELEASE(result);
		return NULL;
    }

	return result;
}


char* blob_to_str(blob_t *blob)
{
	if(blob == NULL)
	{
		return NULL;
	}

	return utils_hex_to_string(blob->value, blob->length);
}

/*
 * For now, just adjust the length, no need to reallocate if shrinking.
 */
int8_t blob_trim(blob_t *blob, uint32_t length)
{
	CHKERR(blob);

	if(blob->length <= length)
	{
		return ERROR;
	}

	blob->length -= length;

	return 1;
}

void blobcol_clear(Lyst *blobs)
{
	LystElt elt = NULL;
	blob_t *blob = NULL;

	CHKVOID(blobs);
	CHKVOID(*blobs);

	for(elt = lyst_first(*blobs); elt; elt = lyst_next(elt))
	{
		blob = (blob_t *) lyst_data(elt);
		blob_destroy(blob, 1);
	}
	lyst_clear(*blobs);
}
