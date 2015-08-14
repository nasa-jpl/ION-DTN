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
 **  05/28/15  E. Birrane     Initial Implementation
 *****************************************************************************/
#include "platform.h"
#include "shared/adm/adm.h"

#include "shared/primitives/dc.h"
#include "shared/utils/nm_types.h"

/*
 * Value is deep-copied.
 */
int dc_add(Lyst dc, uint8_t *value, uint32_t length)
{
	datacol_entry_t *entry = NULL;

	if((dc == NULL) || (value == NULL))
	{
		return 0;
	}

	if((entry = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t))) == NULL)
	{
		return 0;
	}

	entry->length = length;
	if((entry->value = (uint8_t*) MTAKE(entry->length)) == NULL)
	{
		MRELEASE(entry);
		return 0;
	}

	memcpy(entry->value, value, entry->length);

	lyst_insert_last(dc, entry);
	return 1;
}


int dc_append(Lyst dest, Lyst src)
{
	LystElt elt;
	datacol_entry_t *cur_entry = NULL;
	datacol_entry_t *new_entry = NULL;

	/* Step 0: Sanity Check. */
	if((dest == NULL) || (src == NULL))
	{
		return 0;
	}

	/* Step 1: For each item in the source DC... */
	for(elt = lyst_first(src); elt; elt = lyst_next(elt))
	{
		cur_entry = (datacol_entry_t *) lyst_data(elt);

		/* Step 1.1; Copy the item...*/
		if((new_entry = dc_copy_entry(cur_entry)) == NULL)
		{
			DTNMP_DEBUG_ERR("dc_append","Can't copy entry.", NULL);
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
	datacol_entry_t *entry1 = NULL;
	datacol_entry_t *entry2 = NULL;

	DTNMP_DEBUG_ENTRY("dc_compare","(%#llx, %#llx)",col1, col2);

	/* Step 0: Sanity check. */
	if((col1 == NULL) || (col2 == NULL))
	{
		DTNMP_DEBUG_ERR("dc_compare", "Bad Args.", NULL);

		DTNMP_DEBUG_EXIT("dc_compare","->-1.", NULL);
		return -1;
	}

	/* Step 1: Easy checks: Same magnitude? */
	if(lyst_length(col1) == lyst_length(col2))
	{
		elt1 = lyst_first(col1);
		elt2 = lyst_first(col2);
		while(elt1 && elt2)
		{
			entry1 = (datacol_entry_t *) lyst_data(elt1);
			entry2 = (datacol_entry_t *) lyst_data(elt2);

			if(entry1->length != entry2->length)
			{
				DTNMP_DEBUG_EXIT("dc_compare","->1.", NULL);
				return 1;
			}
			else if(memcmp(entry1->value,entry2->value,entry1->length) != 0)
			{
				DTNMP_DEBUG_EXIT("dc_compare","->1.", NULL);
				return 1;
			}

			elt1 = lyst_next(elt1);
			elt2 = lyst_next(elt2);
		}
	}
	else
	{
		DTNMP_DEBUG_EXIT("dc_compare","->1.", NULL);
		return 1;
	}

	DTNMP_DEBUG_EXIT("dc_compare","->0.", NULL);
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
	datacol_entry_t *cur_entry = NULL;
	datacol_entry_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("dc_copy","(%#llx)",(unsigned long) col);

	/* Step 0: Sanity Check. */
	if(col == NULL)
	{
		DTNMP_DEBUG_ERR("dc_copy","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
		return NULL;
	}

	/* Build the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("dc_copy","Unable to create lyst.",NULL);
		DTNMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
		return NULL;
	}

	/* Walking copy. */
	int success = 1;
	for(elt = lyst_first(col); elt; elt = lyst_next(elt))
	{
		cur_entry = (datacol_entry_t *) lyst_data(elt);

		if((new_entry = (datacol_entry_t*)MTAKE(sizeof(datacol_entry_t))) == NULL)
		{
			DTNMP_DEBUG_ERR("dc_copy","Failed to alloc %d bytes.",
					        sizeof(datacol_entry_t));
			dc_destroy(&result);

			DTNMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
			return NULL;
		}

		new_entry->length = cur_entry->length;

		if((new_entry->value = (uint8_t*)MTAKE(new_entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("dc_copy","Failed to alloc %d bytes.",
					        new_entry->length);
			MRELEASE(new_entry);
			dc_destroy(&result);

			DTNMP_DEBUG_EXIT("dc_copy","->NULL.",NULL);
			return NULL;
		}

		memcpy(new_entry->value, cur_entry->value, new_entry->length);

		lyst_insert_last(result,new_entry);
	}

	DTNMP_DEBUG_EXIT("dc_copy","->%#llx.",(unsigned long) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: dc_copy_entry
 *
 * \par Duplicates a Data Collection entry object.
 *
 * \retval NULL - Failure
 *         !NULL - The copied entry
 *
 * \param[in] entry  The Data Collection Entry being copied.
 *
 * \par Notes:
 *		1. The returned Data Collection Entry is allocated and must be freed when
 *		   no longer needed.  This is a deep copy.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/27/15  E. Birrane     Initial implementation,
 *****************************************************************************/

datacol_entry_t* dc_copy_entry(datacol_entry_t *entry)
{
	datacol_entry_t *result = NULL;

	/* Step 0: Sanity Checks. */
	if(entry == NULL)
	{
		DTNMP_DEBUG_ERR("dc_copy_entry","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Allocate the new entry.*/
	if((result = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("dc_copy_entry","Can't allocate new entry.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the value. */
	if((result->value = (uint8_t *) MTAKE(entry->length)) == NULL)
	{
		DTNMP_DEBUG_ERR("dc_copy_entry","Can't allocate entry value.", NULL);
		MRELEASE(result);
		return NULL;
	}

	memcpy(&(result->value), &(entry->value), entry->length);
	result->length = entry->length;

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
	uvast len = 0;
	uint32_t i = 0;

	DTNMP_DEBUG_ENTRY("dc_deserialize","(%#llx,%d,%#llx)",
			          (unsigned long) buffer, buffer_size,
			          (unsigned long) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("dc_deserialize","Bad Args", NULL);
		DTNMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;

	/* Step 1: Create the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("dc_deserialize","Can't create lyst.", NULL);
		DTNMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab # entries in the collection. */
	if((bytes = utils_grab_sdnv(cursor, buffer_size, &num)) == 0)
	{
		DTNMP_DEBUG_ERR("dc_deserialize","Can't parse SDNV.", NULL);
		lyst_destroy(result);

		DTNMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
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
		datacol_entry_t *entry = NULL;

		/* Make new entry. */
		if((entry = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t))) == NULL)
		{
			DTNMP_DEBUG_ERR("dc_deserialize","Can't grab MID #%d.", i);
			dc_destroy(&result);

			DTNMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
			return NULL;
		}

		/* Grab data item length. */
		if((bytes = utils_grab_sdnv(cursor, buffer_size, &len)) == 0)
		{
			DTNMP_DEBUG_ERR("dc_deserialize","Can't parse SDNV.", NULL);
			MRELEASE(entry);
			dc_destroy(&result);

			DTNMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
			return NULL;
		}

		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;

		entry->length = len;

		if((entry->value = (uint8_t*)MTAKE(entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("dc_deserialize","Can't parse SDNV.", NULL);
			MRELEASE(entry);
			dc_destroy(&result);

			DTNMP_DEBUG_EXIT("dc_deserialize","->NULL",NULL);
			return NULL;
		}

		memcpy(entry->value, cursor, entry->length);
		cursor += entry->length;
		buffer_size -= entry->length;
		*bytes_used += entry->length;

		/* Drop it in the lyst in order. */
		lyst_insert_last(result, entry);
	}

	DTNMP_DEBUG_EXIT("dc_deserialize","->%#llx",(unsigned long)result);
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
	datacol_entry_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("dc_destroy","(%#llx)", (unsigned long) datacol);

	/*
	 * Step 0: Make sure we even have a lyst. Not an error if not, since we
	 * are destroying anyway.
	 */
	if((datacol == NULL) || (*datacol == NULL))
	{
		DTNMP_DEBUG_ERR("dc_destroy","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("dc_destroy","->.", NULL);
		return;
	}

	/* Step 1: Walk through the MIDs releasing as you go. */
    for(elt = lyst_first(*datacol); elt; elt = lyst_next(elt))
    {
    	entry = (datacol_entry_t *) lyst_data(elt);

    	dc_release_entry(entry);
    }

    /* Step 2: Destroy and zero out the lyst. */
    lyst_destroy(*datacol);
    *datacol = NULL;

    DTNMP_DEBUG_EXIT("dc_destroy","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: dc_release_entry
 *
 * \par Purpose: Release a data column entry.
 *
 * \param[in|out] entry  The entry to be released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/20/15  E. Birrane     Initial implementation,
 *****************************************************************************/

void  dc_release_entry(datacol_entry_t *entry)
{
	if(entry != NULL)
	{
		MRELEASE(entry->value);
		MRELEASE(entry);
	}
}

/******************************************************************************
 *
 * \par Function Name: dc_serialize
 *
 * \par Purpose: Generate full, serialized version of a Data Collection. A
 *      serialized Data Collection is of the form:
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
 *****************************************************************************/

datacol_entry_t* dc_get_entry(Lyst datacol, uint32_t idx)
{
	LystElt elt = NULL;
	datacol_entry_t *entry = NULL;
	uint32_t cur = 1;

	if(datacol == NULL)
	{
		DTNMP_DEBUG_ERR("dc_get_entry","Bad args",NULL);
		return NULL;
	}

	if(idx > lyst_length(datacol))
	{
		DTNMP_DEBUG_ERR("dc_get_entry","Can't as for %d in list of length %d", idx, lyst_length(datacol));
		return NULL;
	}

	for(elt = lyst_first(datacol); elt; elt = lyst_next(elt))
	{
		if(cur == idx)
		{
			entry = (datacol_entry_t *) lyst_data(elt);
			return entry;
		}
		cur++;
	}

	return NULL;
}

/******************************************************************************
 *
 * \par Function Name: dc_serialize
 *
 * \par Purpose: Generate full, serialized version of a Data Collection. A
 *      serialized Data Collection is of the form:
 * \par +-------+--------+--------+    +--------+--------+
 *      |   #   | Item 1 | Item 1 |    | Item N | Item N |
 *      | Items |  Size  |  Data  |... |  Size  |  Data  |
 *      | [SDNV]| [SDNV] |[BYTES] |    | [SDNV] |[BYTES] |
 *      +-------+--------+--------+    +--------+--------+
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
	Sdnv tmp;
	LystElt elt;

	DTNMP_DEBUG_ENTRY("dc_serialize","(%#llx, %#llx)",
			          (unsigned long) datacol, (unsigned long) size);

	/* Step 0: Sanity Check */
	if((datacol == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("dc_serialize","Bad args.", NULL);
		DTNMP_DEBUG_EXIT("dc_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Calculate the size. */

	/* Consider the size of the SDNV holding # data entries.*/
	encodeSdnv(&num_sdnv, lyst_length(datacol));
	*size = num_sdnv.length;

	/* Walk through each datacol and look at size. */
    for(elt = lyst_first(datacol); elt; elt = lyst_next(elt))
    {
    	datacol_entry_t *entry = (datacol_entry_t *) lyst_data(elt);

    	if(entry != NULL)
    	{
    		encodeSdnv(&tmp, entry->length);
    		*size += (entry->length + tmp.length);
    	}
    	else
    	{
    		DTNMP_DEBUG_WARN("dc_serialize","Found NULL Entry?", NULL);
    	}
    }

    /* Step 3: Allocate the space for the serialized list. */
    if((result = (uint8_t*) MTAKE(*size)) == NULL)
    {
		DTNMP_DEBUG_ERR("dc_serialize","Can't alloc %d bytes", *size);
		*size = 0;

		DTNMP_DEBUG_EXIT("dc_serialize","->NULL",NULL);
		return NULL;
    }

    /* Step 4: Walk through list again copying as we go. */
    uint8_t *cursor = result;

    /* Copy over the number of data entries in the collection. */
    memcpy(cursor, num_sdnv.text, num_sdnv.length);
    cursor += num_sdnv.length;

    for(elt = lyst_first(datacol); elt; elt = lyst_next(elt))
    {
    	datacol_entry_t *entry = (datacol_entry_t *) lyst_data(elt);

    	if(entry != NULL)
    	{
    		encodeSdnv(&tmp, entry->length);
    		memcpy(cursor, tmp.text, tmp.length);
    		cursor += tmp.length;

    		memcpy(cursor,entry->value, entry->length);
    		cursor += entry->length;
    	}
    	else
    	{
    		DTNMP_DEBUG_WARN("dc_serialize","Found NULL MID?", NULL);
    	}
    }

    /* Step 5: Final sanity check. */
    if((cursor - result) != *size)
    {
		DTNMP_DEBUG_ERR("dc_serialize","Wrote %d bytes not %d bytes",
				        (cursor - result), *size);
		*size = 0;
		MRELEASE(result);
		DTNMP_DEBUG_EXIT("dc_serialize","->NULL",NULL);
		return NULL;
    }

	DTNMP_DEBUG_EXIT("dc_serialize","->%#llx",(unsigned long) result);
	return result;
}



char *dc_entry_to_str(datacol_entry_t *entry)
{
	char *result = NULL;

	if(entry == NULL)
	{
		return NULL;
	}

	return utils_hex_to_string(entry->value, entry->length);
}
