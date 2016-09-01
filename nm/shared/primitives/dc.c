/*****************************************************************************
 **
 ** File Name: dc.c
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Data Collections (DCs).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/28/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  04/14/16  E. Birrane     Migrated to BLOB type (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#include "platform.h"
#include "../adm/adm.h"

#include "../primitives/dc.h"
#include "../utils/nm_types.h"

/*
 * Value is deep-copied.
 */
int dc_add(Lyst dc, uint8_t *value, uint32_t length)
{
	blob_t *entry = NULL;
	uint8_t *val = NULL;

	if((dc == NULL) || (value == NULL))
	{
		return 0;
	}

	if((val = (uint8_t*) STAKE(length)) == NULL)
	{
		return 0;
	}

	memcpy(val, value, length);

	entry = blob_create(val, length);
	lyst_insert_last(dc, entry);
	return 1;
}

/*
 * Value is deep copied. 9/9/2015
 */
int dc_add_first(Lyst dc, uint8_t *value, uint32_t length)
{
	blob_t *entry = NULL;
	uint8_t *val = NULL;

	if((dc == NULL) || (value == NULL))
	{
		return 0;
	}

	if((val = (uint8_t*) STAKE(length)) == NULL)
	{
		return 0;
	}

	memcpy(val, value, length);

	entry = blob_create(val, length);
	lyst_insert_first(dc, entry);
	return 1;
}


int dc_append(Lyst dest, Lyst src)
{
	LystElt elt;
	blob_t *cur_entry = NULL;
	blob_t *new_entry = NULL;

	/* Step 0: Sanity Check. */
	if((dest == NULL) || (src == NULL))
	{
		return 0;
	}

	/* Step 1: For each item in the source DC... */
	for(elt = lyst_first(src); elt; elt = lyst_next(elt))
	{
		cur_entry = (blob_t *) lyst_data(elt);

		/* Step 1.1; Copy the item...*/
		if((new_entry = blob_copy(cur_entry)) == NULL)
		{
			AMP_DEBUG_ERR("dc_append","Can't copy entry.", NULL);
			// \todo: Consider removing previously added items.
			return 0;
		}

		/* Step 1.2: Insert at the end of the dest. */
		lyst_insert_last(dest, new_entry);
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: dc_compare
 *
 * \par Determines equivalence of two Data Collections.
 *
 * \retval -1 : Error
 * 			0 : col1 == col2
 * 			1 : col1 != col2
 *
 * \param[in] col1      First Data Collection being compared.
 * \param[in] col2      Second Data Collection being compared.
 *
 * \par Notes:
 *		1. This function should only check for equivalence (== 0), not order
 *         since we do not differentiate between col1 < col2 and error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *****************************************************************************/

int dc_compare(Lyst col1, Lyst col2)
{
	LystElt elt1;
	LystElt elt2;
	blob_t *entry1 = NULL;
	blob_t *entry2 = NULL;

	AMP_DEBUG_ENTRY("dc_compare","(%#llx, %#llx)",col1, col2);

	/* Step 0: Sanity check. */
	if((col1 == NULL) || (col2 == NULL))
	{
		AMP_DEBUG_ERR("dc_compare", "Bad Args.", NULL);

		AMP_DEBUG_EXIT("dc_compare","->-1.", NULL);
		return -1;
	}

	/* Step 1: Easy checks: Same magnitude? */
	if(lyst_length(col1) == lyst_length(col2))
	{
		elt1 = lyst_first(col1);
		elt2 = lyst_first(col2);
		while(elt1 && elt2)
		{
			entry1 = (blob_t *) lyst_data(elt1);
			entry2 = (blob_t *) lyst_data(elt2);

			if(entry1->length != entry2->length)
			{
				AMP_DEBUG_EXIT("dc_compare","->1.", NULL);
				return 1;
			}
			else if(memcmp(entry1->value,entry2->value,entry1->length) != 0)
			{
				AMP_DEBUG_EXIT("dc_compare","->1.", NULL);
				return 1;
			}

			elt1 = lyst_next(elt1);
			elt2 = lyst_next(elt2);
		}
	}
	else
	{
		AMP_DEBUG_EXIT("dc_compare","->1.", NULL);
		return 1;
	}

	AMP_DEBUG_EXIT("dc_compare","->0.", NULL);
	return 0;
}



/******************************************************************************
 *
 * \par Function Name: dc_copy
 *
 * \par Duplicates a Data Collection object.
 *
 * \retval NULL - Failure
 *         !NULL - The copied MID
 *
 * \param[in] col  The Data Collection being copied.
 *
 * \par Notes:
 *		1. The returned Data Collection is allocated and must be freed when
 *		   no longer needed.  This is a deep copy.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/22/12  E. Birrane     Initial implementation,
 *****************************************************************************/

Lyst dc_copy(Lyst col)
{
	Lyst result = NULL;
	LystElt elt;
	blob_t *cur_entry = NULL;
	blob_t *new_entry = NULL;

	AMP_DEBUG_ENTRY("dc_copy","(%#llx)",(unsigned long) col);

	/* Step 0: Sanity Check. */
	if(col == NULL)
	{
		AMP_DEBUG_ERR("dc_copy","Bad Args.",NULL);
		AMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
		return NULL;
	}

	/* Build the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("dc_copy","Unable to create lyst.",NULL);
		AMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
		return NULL;
	}

	/* Walking copy. */
	for(elt = lyst_first(col); elt; elt = lyst_next(elt))
	{
		cur_entry = (blob_t *) lyst_data(elt);

		if((new_entry = blob_copy(cur_entry)) == NULL)
		{
			AMP_DEBUG_ERR("dc_copy","Failed to alloc %d bytes.",
					        sizeof(blob_t));
			dc_destroy(&result);

			AMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
			return NULL;
		}
		lyst_insert_last(result,new_entry);
	}

	AMP_DEBUG_EXIT("dc_copy","->%#llx.",(unsigned long) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: dc_deserialize
 *
 * \par Extracts a Data Collection from a byte buffer. When serialized, a
 *      Data Collection is an SDNV # of items, followed by a series of
 *      entries.  Each entry is an SDNV size followed by a blob of data.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized Data Collection.
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
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/17/13  E. Birrane     Updated to ION 3.1.3, moved to uvast data type.
 *****************************************************************************/

Lyst dc_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t *bytes_used)
{
	unsigned char *cursor = NULL;
	Lyst result = NULL;
	uint32_t bytes = 0;
	uvast num = 0;
	uint32_t i = 0;

	AMP_DEBUG_ENTRY("dc_deserialize","(%#llx,%d,%#llx)",
			          (unsigned long) buffer, buffer_size,
			          (unsigned long) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("dc_deserialize","Bad Args", NULL);
		AMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;

	/* Step 1: Create the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("dc_deserialize","Can't create lyst.", NULL);
		AMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab # entries in the collection. */
	if((bytes = utils_grab_sdnv(cursor, buffer_size, &num)) == 0)
	{
		AMP_DEBUG_ERR("dc_deserialize","Can't parse SDNV.", NULL);
		lyst_destroy(result);

		AMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Grab entries. */
	for(i = 0; i < num; i++)
	{
		blob_t *entry = NULL;

		entry = blob_deserialize(cursor, buffer_size, &bytes);
		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;

		/* Drop it in the lyst in order. */
		lyst_insert_last(result, entry);
	}

	AMP_DEBUG_EXIT("dc_deserialize","->%#llx",(unsigned long)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: dc_destroy
 *
 * \par Releases all memory allocated in the Data Collection. Clear all Data
 *      Collection information.
 *
 * \param[in,out]  datacol      The data collection to be destroyed.
 *
 * \par Notes:
 *      - We pass in a pointer so we can destroy the lyst. The lyst must not
 *        be accessed after this call.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/22/12  E. Birrane     Initial implementation,
 *****************************************************************************/

void dc_destroy(Lyst *datacol)
{
	LystElt elt;
	LystElt del_elt;
	blob_t *entry = NULL;

	AMP_DEBUG_ENTRY("dc_destroy","(%#llx)", (unsigned long) datacol);

	/*
	 * Step 0: Make sure we even have a lyst. Not an error if not, since we
	 * are destroying anyway.
	 */
	if((datacol == NULL) || (*datacol == NULL))
	{
		AMP_DEBUG_ERR("dc_destroy","Bad Args.",NULL);
		AMP_DEBUG_EXIT("dc_destroy","->.", NULL);
		return;
	}

	/* Step 1: Walk through the MIDs releasing as you go. */
    for(elt = lyst_first(*datacol); elt;)
    {
    	entry = (blob_t *) lyst_data(elt);
    	del_elt = elt;
    	elt = lyst_next(elt);
    	blob_destroy(entry, 1);
    	lyst_delete(del_elt);
    }

    /* Step 2: Destroy and zero out the lyst. */
    lyst_clear(*datacol);
    lyst_destroy(*datacol);
    *datacol = NULL;

    AMP_DEBUG_EXIT("dc_destroy","->.", NULL);
}




/******************************************************************************
 *
 * \par Function Name: dc_get_entry
 *
 * \par Purpose: Retrive the ith blob in a DC
 *
 * \retval NULL - Error
 * 		   !NULL - The ith entry.
 *
 * \param[in]  datacol    The Data Collection whose entry is being queried.
 * \param[in]  idx        The entry being queried.
 *
 * \par Notes:
 *		1. The result is a direct pointer into the data collection and must
 *		   not be released.
 *		2. The index is 1 based.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/19/15  E. Birrane     Initial implementation,
 *  04/14/16  E. Birrane     Updated to use blob_t
 *****************************************************************************/

blob_t* dc_get_entry(Lyst datacol, uint32_t idx)
{
	LystElt elt = NULL;
	blob_t *entry = NULL;
	uint32_t cur = 1;

	if(datacol == NULL)
	{
		AMP_DEBUG_ERR("dc_get_entry","Bad args",NULL);
		return NULL;
	}

	if(idx > lyst_length(datacol))
	{
		AMP_DEBUG_ERR("dc_get_entry","Can't as for %d in list of length %d", idx, lyst_length(datacol));
		return NULL;
	}

	for(elt = lyst_first(datacol); elt; elt = lyst_next(elt))
	{
		if(cur == idx)
		{
			entry = (blob_t *) lyst_data(elt);
			return entry;
		}
		cur++;
	}

	return NULL;
}


/*
 * 9/9/15
 * Return 1 on success.
 *
 */
int dc_remove_first(Lyst dc, int del)
{
	LystElt elt;
	blob_t *entry = NULL;

	if(dc == NULL)
	{
		return 1;
	}

	if((elt = lyst_first(dc)) == NULL)
	{
		return 1;
	}

	if((entry = (blob_t *) lyst_data(elt)) == NULL)
	{
		return 1;
	}

	if(del != 0)
	{
    	blob_destroy(entry, 1);
	}

	lyst_delete(elt);

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: dc_serialize
 *
 * \par Purpose: Generate full, serialized version of a Data Collection. A
 *      serialized Data Collection is of the form:
 * \par +-------+--------+--------+    +--------+
 *      |   #   | Item 1 | Item 2 |    | Item N |
 *      | Items |        |        |... |        |
 *      | [SDNV]| [BLOB] |[BLOB]  |    | [BLOB] |
 *      +-------+--------+--------+    +--------+
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized collection.
 *
 * \param[in]  datacol    The Data Collection to be serialized.
 * \param[out] size       The size of the resulting serialized Collection.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

uint8_t *dc_serialize(Lyst datacol, uint32_t *size)
{
	uint8_t *result = NULL;
	Sdnv num_sdnv;
	LystElt elt;

	AMP_DEBUG_ENTRY("dc_serialize","(%#llx, %#llx)",
			          (unsigned long) datacol, (unsigned long) size);

	/* Step 0: Sanity Check */
	if((datacol == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("dc_serialize","Bad args.", NULL);
		AMP_DEBUG_EXIT("dc_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Calculate the size. */

	/* Consider the size of the SDNV holding # data entries.*/
	encodeSdnv(&num_sdnv, lyst_length(datacol));
	*size = num_sdnv.length;

	/* Walk through each datacol and look at size. */
    for(elt = lyst_first(datacol); elt; elt = lyst_next(elt))
    {
    	blob_t *entry = (blob_t *) lyst_data(elt);
    	*size += blob_get_serialize_size(entry);
    }

    /* Step 3: Allocate the space for the serialized list. */
    if((result = (uint8_t*) STAKE(*size)) == NULL)
    {
		AMP_DEBUG_ERR("dc_serialize","Can't alloc %d bytes", *size);
		*size = 0;

		AMP_DEBUG_EXIT("dc_serialize","->NULL",NULL);
		return NULL;
    }

    /* Step 4: Walk through list again copying as we go. */
    uint8_t *cursor = result;

    /* Copy over the number of data entries in the collection. */
    memcpy(cursor, num_sdnv.text, num_sdnv.length);
    cursor += num_sdnv.length;

    for(elt = lyst_first(datacol); elt; elt = lyst_next(elt))
    {
    	blob_t *entry = (blob_t *) lyst_data(elt);
    	uint32_t len = 0;
    	if(entry != NULL)
    	{
        	uint8_t *val = blob_serialize(entry, &len);
        	memcpy(cursor, val, len);
        	cursor += len;
        	SRELEASE(val);
    	}
    	else
    	{
    		AMP_DEBUG_WARN("dc_serialize","Found NULL MID?", NULL);
    	}
    }

    /* Step 5: Final sanity check. */
    if((cursor - result) != *size)
    {
		AMP_DEBUG_ERR("dc_serialize","Wrote %d bytes not %d bytes",
				        (cursor - result), *size);
		*size = 0;
		SRELEASE(result);
		AMP_DEBUG_EXIT("dc_serialize","->NULL",NULL);
		return NULL;
    }

	AMP_DEBUG_EXIT("dc_serialize","->%#llx",(unsigned long) result);
	return result;
}
