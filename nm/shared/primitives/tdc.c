/*****************************************************************************
 **
 ** File Name: tdc.c
 **
 ** Description: This implements a strongly-typed data collection, based on the
 **              original datalist data type proposed by Jeremy Mayer.
 **
 ** Notes:
 **
 ** Assumptions:
 ** 1. The typed data collection has less than 256 elements.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/24/15  J. P. Mayer    Initial Implementation.
 **  06/27/15  E. Birrane     Migrate from datalist to TDC.
 *****************************************************************************/

#include "platform.h"

#include "lyst.h"
#include "ion.h"
#include "tdc.h"
#include "shared/utils/utils.h"



void tdc_append(tdc_t *dst, tdc_t *src)
{
	int i = 0;
	LystElt elt = NULL;
	datacol_entry_t *cur_entry = NULL;


	/* Step 0: Sanity Check. */
	if((dst == NULL) || (src == NULL))
	{
		DTNMP_DEBUG_ERR("tdc_append","Bad Args.", NULL);
		return;
	}

	/* Step 1: For each item in the source...*/
	for(elt = lyst_first(src->datacol); elt; elt = lyst_next(elt))
	{
		cur_entry = (datacol_entry_t *) lyst_data(elt);

		/* Step 1.1: Insert the item into the dest TDC. */
		tdc_insert(dst, tdc_get_type(src, i), cur_entry->value, cur_entry->length);

		/* Step 1.2: Note the next index for the type array. */
		i++;
	}
}

/******************************************************************************
 *
 * \par Function Name: tdc_clear
 *
 * \par Purpose: Clears the TDC, including all contents within.
 *
 * \return N/A
 *
 * \param[in]   tdc  A pointer to the TDC
 *
 * \par Notes:
 *  - This function does not destroy the tdc itself, it merely clears
 *    out the contents of a TDC. THerefore, it is suitable to clear both
 *    dynamically and statically scoped TDCs.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

void tdc_clear(tdc_t* tdc)
{
	if(tdc != NULL)
	{
		// Free datacol
		if(tdc->datacol != NULL)
		{
			dc_destroy(&(tdc->datacol));
		}

		// Free types header.
		if(tdc->hdr.data != NULL)
		{
			MRELEASE(tdc->hdr.data);
		}

		memset(tdc, 0, sizeof(tdc_t));
	}
}



/******************************************************************************
 *
 * \par Function Name: tdc_create
 *
 * \par Purpose: Creates a typed data collection.
 *
 * \return The allocated TDC.
 *
 * \param[in]   dc	      The datacol to deserialize from (optional)
 * \param[in]   types     Array of dc entry types.
 * \param[in]   type_cnt  # types.
 *
 * \par Notes:
 *
 *  - It is possible to create a typed data collection from an existing
 *    data collection, if the types of each datum comprising the collection
 *    are provided. When constructing a TDC from a DC, the original DC and
 *    type array are preserved in the resulting TDC and MUST NOT be deallocated
 *    prior to deallocating the encapsulating TDC (after a tdc_clear()).
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Migrated from datalist to TDC.
 *****************************************************************************/

tdc_t *tdc_create(Lyst *dc, uint8_t *types, uint8_t type_cnt)
{
	tdc_t *result = NULL;

	/* Step 0: Allocate the new TDC. */
	if((result = (tdc_t *) MTAKE(sizeof(tdc_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_create","Cannot allocate new TDC.", NULL);
		return NULL;
	}

	memset(result,0,sizeof(tdc_t));

	/* Step 1: Copy in an existing DC, if provided. */
	if((dc != NULL) && (types != NULL))
	{
		if(type_cnt != lyst_length(*dc))
		{
			DTNMP_DEBUG_ERR("tdc_create", "DC entry (%d) to type count (%d) mismatch.", lyst_length(*dc), type_cnt);
			MRELEASE(result);
			return NULL;
		}

		result->datacol = *dc;
		result->hdr.data = types;
		result->hdr.length = type_cnt;
		result->hdr.index = 0;
	}
	else
	{
		result->datacol = lyst_create();
	}

	/* Step 2: Return the created TDC. */
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tdc_copy
 *
 * \par Purpose: Deep-copies an existing TDC.
 *
 * \return The TDC, or NULL if something failed.
 *
 * \param[in]   tdc		The TDC being copied.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/15  E. Birrane     Initial Implementation.
 *****************************************************************************/

tdc_t* tdc_copy(tdc_t *tdc)
{
	tdc_t *result = NULL;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_copy","Bad Args.",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new tdc structure. */
	if((result = (tdc_t *) MTAKE(sizeof(tdc_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_copy","Can't allocate new TDC.",NULL);
		return NULL;
	}

	/* Step 2: Deep copy contents. */
	result->hdr = tdc->hdr;

	result->hdr.index = tdc->hdr.index;
	result->hdr.length = tdc->hdr.length;
	if((result->hdr.data = (uint8_t *) MTAKE(result->hdr.length)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_copy","Can't Copy header type data.",NULL);
		MRELEASE(result);
		return NULL;
	}
	memcpy(result->hdr.data, tdc->hdr.data, result->hdr.length);

	if((result->datacol = dc_copy(tdc->datacol)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_copy","Can't Copy DC.",NULL);
		MRELEASE(result->hdr.data);
		MRELEASE(result);
		return NULL;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: tdc_deserialize
 *
 * \par Purpose: Deserializes a tdc from a byte array
 *
 * \return The datalist, or NULL if something failed.
 *
 * \param[in]   buffer		the byte array
 * \param[in]   buffer_size	The length of buffer
 * \param[out]  bytes_used	The length of buffer which was deserialized.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

tdc_t *tdc_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t* bytes_used)
{
	unsigned char *cursor = NULL;
	tdc_t *result = NULL;
	uint32_t bytes = 0;
    uint8_t *types = NULL;
    Lyst dc = NULL;

	DTNMP_DEBUG_ENTRY("tdc_deserialize","(" UVAST_FIELDSPEC ",%d," UVAST_FIELDSPEC ")",
			          (uvast) buffer, buffer_size, (uvast) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("tdc_deserialize","Bad Args", NULL);
		DTNMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;

	/* Step 1: Grab the encapsulated data collection. */
	if((dc = dc_deserialize(cursor, buffer_size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_deserialize","Can't deserialize DC.", NULL);

		MRELEASE(types);

		DTNMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 2: Grab the type information. */
	bytes = lyst_length(dc);
	if((types = (uint8_t *) MTAKE(bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_deserialize","Can't allocate type array of size %d.", bytes);
		DTNMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		memcpy(types, cursor, bytes);
		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Create the TDC. */
	if((result = tdc_create(&dc, types, lyst_length(dc))) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_deserialize","Can't create TDC.", NULL);

		MRELEASE(types);
		dc_destroy(&dc);

		DTNMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("tdc_deserialize", "->" UVAST_FIELDSPEC, (uvast)result);
	return result;

}


void tdc_destroy(tdc_t **tdc)
{
	if(tdc == NULL)
	{
		return;
	}

	tdc_clear(*tdc);
	MRELEASE(*tdc);
	*tdc = NULL;
}

/******************************************************************************
 *
 * \par Function Name: tdc_get
 *
 * \par Purpose: Grabs an item from the specified index, checks the type given is
 *				equal to the type of the item, then copies it to the given buffer.
 *
 * \return The type, or DTNMP_TYPE_UNK if something failed.
 *
 *
 * \param[in]   tdc	        The typed data collection being queried
 * \param[in]   index		The index of the requested item
 * \param[in]   type        The expected type of the requested item
 * \param[out]  optr		The buffer holding the response
 * \param[out]  outSize     The size of the buffer
 *
 * \par Notes:
 * - The item is copied to the given memory, so you must make sure that optr has enough space
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/


dtnmp_type_e tdc_get(tdc_t* tdc, uint8_t index, dtnmp_type_e type, uint8_t** optr, size_t* outsize)
{
	uint8_t cur_type = 0;
	datacol_entry_t *entry = NULL;

	/* Step 0: Sanity Checks. */
	if((tdc == NULL) || (optr == NULL) || (outsize == NULL))
	{
		DTNMP_DEBUG_ERR("tdc_get","Bad Args.", NULL);
		return DTNMP_TYPE_UNK;
	}

	if(tdc->hdr.length <= index)
	{
		DTNMP_DEBUG_ERR("tdc_get","Cannot get index %d with only %d items.", index, tdc->hdr.length);
		return DTNMP_TYPE_UNK;
	}

	/* Step 1: Check the type. */
	cur_type = tdc_get_type(tdc, index);
	if(cur_type != type)
	{
		DTNMP_DEBUG_ERR("tdc_get","Item %d has type %d not %d.", index, cur_type, type);
		return DTNMP_TYPE_UNK;
	}

	/* Step 2: Grab the entry. */
	if((entry = tdc_get_colentry(tdc, index)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get", "UNable to retrieve index %d from TDC.", index);
		return DTNMP_TYPE_UNK;
	}

	/* Step 3: Copy over the pointers. */
	if((*optr = (uint8_t *) MTAKE(entry->length)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get","Can't allocate entry.", NULL);
		return DTNMP_TYPE_UNK;
	}

	memcpy(*optr, entry, entry->length);
	*outsize = entry->length;

	/* Step 4: Return the entry type. */
	return cur_type;
}



/******************************************************************************
 *
 * \par Function Name: tdc_get_colentry
 *
 * \par Purpose: Returns the data collection entry for a given index
 *
 * \return The datacol_entry_t, or NULL if something failed.
 *
 *
 * \param[in]   tdc		The typed data collection
 * \param[in]   index	The zero-based index index
 *
 * \par Notes:
 *  - The returned entry is NOT allocated and must NOT be released by the caller.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

datacol_entry_t* tdc_get_colentry(tdc_t* tdc, uint8_t index)
{
	datacol_entry_t *result = NULL;
	uint8_t idx = 0;
	LystElt elt = NULL;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get_colentry","Bad args.", NULL);
		return NULL;
	}

	if(tdc->hdr.length < index)
	{
		DTNMP_DEBUG_ERR("tdc_get_colentry","Bad index %d out of %d.", index, tdc->hdr.length);
		return NULL;
	}

	/* Step 1: Find correct DC. */
	for(elt = lyst_first(tdc->datacol); idx != index + 1; elt = lyst_next(elt), idx++);

	/* Step 2: Copy it. */
	if(elt)
	{
		result = (datacol_entry_t*) lyst_data(elt);
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: tdc_get_count
 *
 * \par Purpose: Returns the number of items in a typed data collection
 *
 * \return The number of elements
 *
 * \param[in]   tdc		The typed data collection
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

uint8_t tdc_get_count(tdc_t* tdc)
{
	uint8_t result = 0;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get_count","Bad Args.", NULL);
		return 0;
	}

	/* Step 1: Grab the number of elements in the lyst. */
	result = lyst_length(tdc->datacol);

	/* Step 2: Check against type information. */
	if(tdc->hdr.length != result)
	{
		DTNMP_DEBUG_ERR("tdc_get_count","Count Mismatch %d entries and %d types.", result, tdc->hdr.length);
		return 0;
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: tdc_get_entry_size
 *
 * \par Purpose: Gets the size of a single item in the TDC.
 *
 * \return the size, or 0 is something failed.
 *
 * \param[in]   tdc      The typed data collection
 * \param[in]   index	 The 0-based index of the item.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

uint32_t tdc_get_entry_size(tdc_t* tdc, uint8_t index)
{
	datacol_entry_t *entry = NULL;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get_entry_size","Bad Args.", NULL);
		return 0;
	}

	if(tdc->hdr.length <= index)
	{
		DTNMP_DEBUG_ERR("tdc_get_entry_size","Bad index %d <= %d.", tdc->hdr.length, index);
		return 0;
	}

	if((entry = tdc_get_colentry(tdc, index)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get_colentry","Can't get entry.", NULL);
		return 0;
	}

	return entry->length;
}


/******************************************************************************
 *
 * \par Function Name: tdc_get_type
 *
 * \par Purpose: returns the type of a given entry.
 *
 * \return the requested type, or DTNMP_TYPE_UNK.
 *
 *
 * \param[in]   tdc			The typed data collection
 * \param[in]   index		The zero-based index.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

dtnmp_type_e tdc_get_type(tdc_t* tdc, uint8_t index)
{
	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_get_type","Bad Args.", NULL);
		return DTNMP_TYPE_UNK;
	}

	if(tdc->hdr.length <= index)
	{
		DTNMP_DEBUG_ERR("tdc_get_type","Bad index %d <= %d.", tdc->hdr.length, index);
		return DTNMP_TYPE_UNK;
	}

	return (dtnmp_type_e) tdc->hdr.data[index];
}





/******************************************************************************
 *
 * \par Function Name: tdc_header_allocate
 *
 * \par Purpose: Allocates the items of a datalist_header_t. The header must already exist,
 *				This function just sets it up.
 *
 * \return The number of items allocated
 *
 *
 * \param[in]   header	The header structure
 * \param[in]   dataSize The desired initial size.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

uint8_t tdc_hdr_allocate(tdc_hdr_t* header, uint8_t dataSize)
{
	if(header == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_hdr_allocate","Header == null",NULL);
		return 0;
	}


	if(header->data != NULL)
	{
		DTNMP_DEBUG_ERR("tdc_hdr_allocate","Header data != null",NULL);
		return 0;
	}


	header->data = (uint8_t*) MTAKE(dataSize);
	header->length = dataSize;
	header->index=0;

	return dataSize;
}




/******************************************************************************
 *
 * \par Function Name: datalist_header_reallocate
 *
 * \par Purpose: Reallocates the items of a header_t, but only if the newSize is
 *		larger then the current size.
 *
 * \return The number of items allocated or 0 on error.
 *
 *
 * \param[in]   header	The header structure
 * \param[in]   newSize The new number of items in the TDC.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

uint8_t tdc_hdr_reallocate(tdc_hdr_t* header, uint8_t newSize)
{
	uint8_t *tmp = NULL;

	/* Step 0: Sanity checks. */
	if(header == NULL)
	{
		return 0;
	}

	/* Step 1: If the new size is smaller, keep the larger allocation. */
	if(newSize <= header->length)
	{
		return header->length;
	}

	/* Step 2: Allocate a larger data set and copy it over. */
	if((tmp = (uint8_t *) MTAKE(newSize)) == NULL)
	{
		DTNMP_DEBUG_ERR("datalist_header_reallocate","Unable to allocate of size %d", newSize);
		return header->length;
	}

	/* Step 3: Copy over. */
	memcpy(tmp, header->data, header->length);
	MRELEASE(header->data);
	header->data = tmp;
	header->length = newSize;

	return newSize;
}



/******************************************************************************
 *
 * \par Function Name: tdc_insert
 *
 * \par Purpose: Inserts an item to the end of the TDC, taking into account
 *               its type. If the type is variable length (blob, string),
 *               the size field must be filled out... memory is also
 *				 allocated and the parameter copied, so remember to free
 *				 your list!
 *
 * \return       The type inserted, or DTNMP_TYPE_UNK if something failed.
 *
 *
 * \param[in]   tdc     The typed data collection.
 * \param[in]   type  	The type field
 * \param[in]   data	A pointer to the data which shall be inserted.
 * \param[in]   size	The size of the variable pointed to by data,
 *                      required for variable length stuff and 0 otherwise
 *
 * \par Notes:
 *  - The data entry is deep-copied into the TDC, so the data parameter
 *    MUST be de-allocated by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

dtnmp_type_e tdc_insert(tdc_t* tdc, dtnmp_type_e type, uint8_t* data, uint32_t size)
{
	uint8_t len = 0;

	/* Step 0: Sanity Checks. */
	if((tdc == NULL) || (data == NULL))
	{
		DTNMP_DEBUG_ERR("tdc_insert","Bad Args.",NULL);
		return DTNMP_TYPE_UNK;
	}

	/* Step 1: Calculate the size of the new data. */
	if(size == 0)
	{
		if((size = type_get_size(type)) == 0)
		{
			DTNMP_DEBUG_ERR("tdc_insert","Can't get size for type %d", type);
			return DTNMP_TYPE_UNK;
		}
	}

	/* Step 2: Reallocate the type array. */
	len = lyst_length(tdc->datacol) + 1;
	if(tdc_hdr_reallocate(&(tdc->hdr), len) != len)
	{
		DTNMP_DEBUG_ERR("tdc_insert", "Can't expand type array", NULL);
		return DTNMP_TYPE_UNK;
	}

	/* Step 3: Add the data to the data collection. */
	if(dc_add(tdc->datacol, data, size) == 0)
	{
		// rollback.
		tdc_hdr_reallocate(&(tdc->hdr), len-1);
		DTNMP_DEBUG_ERR("tdc_insert","Unable to add to DC.", NULL);
		return DTNMP_TYPE_UNK;
	}

	/* Step 3: Update type information and index. */
	tdc_set_type(tdc,len-1,type);
	tdc->hdr.index++;

	return type;
}



/******************************************************************************
 *
 * \par Function Name: tdc_serialize
 *
 * \par Purpose: Generate full, serialized version of a Typed Data Collection.
 *               A serialized Typed Data Collection is of the form:
 *
 * \par +-------+--------+--------+     +--------+--------+---------+
 *      |   #   | Item 1 | Item 1 |     | Item N | Item N |         |
 *      | Items | Size   | Data   | ... |  Size  |  Data  |  Types  |
 *      |[SDNV] | [SDNV] |[BYTES] |     | [SDNV] |[BYTES] | [BYTES] |
 *      +-------+--------+--------+     +--------+--------+---------+
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized collection.
 *
 * \param[in]  tdc    The Typed Data Collection to be serialized.
 * \param[out] size   The size of the resulting serialized Collection.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

uint8_t* tdc_serialize(tdc_t *tdc, uint32_t *size)
{
	uint8_t *cursor = NULL;
	uint8_t *result = NULL;
	uint8_t *dc_data = NULL;
	uint32_t dc_size = 0;

	/* Step 0: Sanity Check. */
	if((tdc == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("tdc_serialize","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Serialize the underlying DC. */
	if((dc_data = dc_serialize(tdc->datacol, &dc_size)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_serialize","Can't serialize DC.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the serialized buffer. */
	*size = dc_size + tdc->hdr.length;
	if((result = (uint8_t *) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_serialize","Can't allocate %d bytes.", *size);
		MRELEASE(dc_data);
		return NULL;
	}

	/* Step 3: Copy the serialized DC into the serialized TDC. */
	cursor = result;
	memcpy(cursor, dc_data, dc_size);
	cursor += dc_size;

	/* Step 4: Copy the extra type data at the end. */
	memcpy(cursor, tdc->hdr.data, tdc->hdr.length);

	/* Step 5: Free temp resources. */
	MRELEASE(dc_data);

	/* Step 6: Return the serialized TDC. */
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tdc_set_type
 *
 * \par Purpose: Sets the type for a given index, allocating additional header
 *				space as needed-
 *				space
 * \return The type, or DTNMP_TYPE_UNK if something failed.
 *
 *
 * \param[in]   tdc		The Typed Data Collection
 * \param[in]   index	The zero-based index index
 * \param[in]   type	The requested type
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

dtnmp_type_e tdc_set_type(tdc_t* tdc, uint8_t index, dtnmp_type_e type)
{
	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_set_type","Bad Args.", NULL);
		return 0;
	}

	if(tdc->hdr.length <= index)
	{
		DTNMP_DEBUG_ERR("tdc_set_type","Bad index %d <= %d.", tdc->hdr.length, index);
		return 0;
	}

	tdc->hdr.data[index]=type;

	return type;
}



/*
 * Extremely inefficient. Only call on the ground.
 */

char* tdc_to_str(tdc_t *tdc)
{
	char **entries = NULL;
	uint32_t *lens = NULL;
	LystElt elt = NULL;
	datacol_entry_t *cur_entry = NULL;
	dtnmp_type_e cur_type;
	int i = 0;
	uint32_t tot_size = 0;
	char *result = NULL;
	char *cursor = NULL;

	if(tdc == NULL)
	{
		DTNMP_DEBUG_ERR("tdc_to_str","Bad Args", NULL);
		return NULL;
	}

	entries = (char **) MTAKE(tdc->hdr.length * (sizeof(char *)));
	lens = (uint32_t *) MTAKE(tdc->hdr.length * (sizeof(uint32_t)));

	if((entries == NULL) || (lens == NULL))
	{
		DTNMP_DEBUG_ERR("tdc_to_str","Can't allocate storage.",NULL);
		MRELEASE(entries);
		MRELEASE(lens);
		return NULL;
	}

	for(elt = lyst_first(tdc->datacol); elt; elt = lyst_next(elt))
	{
		cur_type = tdc->hdr.data[i];
		cur_entry = (datacol_entry_t *) lyst_data(elt);
		char *data_str = dc_entry_to_str(cur_entry);

		// \todo:Handle failure here.
		lens[i] = 20 +
		     	  strlen(data_str);
		entries[i] = (char *) MTAKE(lens[i]);

		sprintf(entries[i],"(%s) %s\n",  type_to_str(cur_type), data_str);
		tot_size += lens[i];

		MRELEASE(data_str);

		i++;
	}

	result = (char *) MTAKE(tot_size);
	cursor = result;

	for(i = 0; i < lyst_length(tdc->datacol); i++)
	{
		memcpy(cursor, entries[i], lens[i]);
		MRELEASE(entries[i]);
	}

	MRELEASE(entries);
	MRELEASE(lens);

	return result;
}


