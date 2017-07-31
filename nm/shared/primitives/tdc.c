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
 **  06/27/15  E. Birrane     Migrate from datalist to TDC. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "lyst.h"
#include "ion.h"
#include "tdc.h"
#include "../utils/utils.h"

#include "../primitives/value.h"


int8_t tdc_append(tdc_t *dst, tdc_t *src)
{
	int i = 0;
	LystElt elt = NULL;
	blob_t *cur_entry = NULL;


	/* Step 0: Sanity Check. */
	if((dst == NULL) || (src == NULL))
	{
		AMP_DEBUG_ERR("tdc_append","Bad Args.", NULL);
		return ERROR;
	}

	/* Step 1: If the TDC has no entries, we are done... */
	if(src->datacol == NULL)
	{
		return 1;
	}

	/* Step 1: For each item in the source...*/
	for(elt = lyst_first(src->datacol); elt; elt = lyst_next(elt))
	{
		cur_entry = (blob_t *) lyst_data(elt);

		/* Step 1.1: Insert the item into the dest TDC. */
		tdc_insert(dst, tdc_get_type(src, i), cur_entry->value, cur_entry->length);

		/* Step 1.2: Note the next index for the type array. */
		i++;
	}

	return 1;
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
			SRELEASE(tdc->hdr.data);
		}

		memset(tdc, 0, sizeof(tdc_t));
	}
}



/******************************************************************************
 *
 * \par Function Name: tdc_compare
 *
 * \par Purpose: Compares two TDCs. This is a logical comparison of values,
 *               not a comparison of addresses.
 *
 * \return -1 System Error
 *          0 TDCs equal
 *          1 TDCs do not match
 *
 * \param[in]   t1	The first TDC to compare
 * \param[in]   t2  The second TDC to compare.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/11/16  E. Birrane     Initial implementation,
 *****************************************************************************/

int8_t tdc_compare(tdc_t *t1, tdc_t *t2)
{
	CHKERR(t1);
	CHKERR(t2);

	/* Step 1 - Number of items must match. */
	if(t1->hdr.length != t2->hdr.length)
	{
		return 1;
	}

	/* Step 2 - If comparing 2 empty TDCs, they match! */
	if(t1->hdr.length == 0)
	{
		return 0;
	}

	/* Step 2 - Types must match. */
	if(memcmp(t1->hdr.data, t2->hdr.data, t1->hdr.length) != 0)
	{
		return 1;
	}

	/* Step 3 - Values must match. */
	return dc_compare(t1->datacol, t2->datacol);
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
	if((result = (tdc_t *) STAKE(sizeof(tdc_t))) == NULL)
	{
		AMP_DEBUG_ERR("tdc_create","Cannot allocate new TDC.", NULL);
		return NULL;
	}

	memset(result,0,sizeof(tdc_t));

	/* Step 1: Copy in an existing DC, if provided. */
	if((dc != NULL) && (types != NULL))
	{
		if(type_cnt != lyst_length(*dc))
		{
			AMP_DEBUG_ERR("tdc_create", "DC entry (%d) to type count (%d) mismatch.", lyst_length(*dc), type_cnt);
			SRELEASE(result);
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
		AMP_DEBUG_ERR("tdc_copy","Bad Args.",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new tdc structure. */
	if((result = (tdc_t *) STAKE(sizeof(tdc_t))) == NULL)
	{
		AMP_DEBUG_ERR("tdc_copy","Can't allocate new TDC.",NULL);
		return NULL;
	}

	/* Step 2: Deep copy contents. */
	result->hdr = tdc->hdr;

	result->hdr.index = tdc->hdr.index;
	result->hdr.length = tdc->hdr.length;

	if(result->hdr.length > 0)
	{

		if((result->hdr.data = (uint8_t *) STAKE(result->hdr.length)) == NULL)
		{
			AMP_DEBUG_ERR("tdc_copy","Can't Copy header type data.",NULL);
			SRELEASE(result);
			return NULL;
		}
		memcpy(result->hdr.data, tdc->hdr.data, result->hdr.length);
	}
	else
	{
		result->hdr.data = NULL;
	}

	if((result->datacol = dc_copy(tdc->datacol)) == NULL)
	{
		AMP_DEBUG_ERR("tdc_copy","Can't Copy DC.",NULL);
		SRELEASE(result->hdr.data);
		SRELEASE(result);
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
 *  09/09/15  E. Birrane     Updated TDC to match AMP-01 spec
 *  04/15/16  E. Birrane     Updated to blob_t
 *****************************************************************************/

tdc_t *tdc_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t* bytes_used)
{
	blob_t *entry = NULL;
	tdc_t *result = NULL;
    Lyst dc = NULL;

	AMP_DEBUG_ENTRY("tdc_deserialize","(" ADDR_FIELDSPEC ",%d," ADDR_FIELDSPEC ")",
			          (uaddr) buffer, buffer_size, (uaddr) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("tdc_deserialize","Bad Args", NULL);
		AMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;

	/* Step 1: Deserialize as a DC. */
	if((dc = dc_deserialize(buffer, buffer_size, bytes_used)) == NULL)
	{
		AMP_DEBUG_ERR("tdc_deserialize","Can't deserialize DC.", NULL);
		AMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
		return NULL;
	}

	/* If we deserialized an empty data collection... */
	if(lyst_length(dc) == 0)
	{
		if((result = tdc_create(NULL, NULL, 0)) == NULL)
		{
			AMP_DEBUG_ERR("tdc_deserialize","Can't create TDC.", NULL);
			dc_destroy(&dc);
			AMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
			return NULL;
		}
	}
	else
	{
		/* Step 2: Grab the first item and deserialize it. */
		entry = dc_get_entry(dc, 1);

		/* Step 3: Remove the types from the original dc. Don't
		 *         delete the entry, because we have a handle to
		 *         it from the above line.
		 */
		dc_remove_first(dc, 0);

		/* Step 4: Create the TDC. This is a shallow copy. */
		if((result = tdc_create(&dc, entry->value, lyst_length(dc))) == NULL)
		{
			AMP_DEBUG_ERR("tdc_deserialize","Can't create TDC.", NULL);

			blob_destroy(entry, 1);
			dc_destroy(&dc);

			AMP_DEBUG_EXIT("tdc_deserialize","->NULL",NULL);
			return NULL;
		}

		/* Remove entry BLOB container, but do not destroy value, which was
		 * copied into the TDC above.
		 */
		SRELEASE(entry);
	}

	AMP_DEBUG_EXIT("tdc_deserialize", "->" ADDR_FIELDSPEC, (uaddr)result);
	return result;
}


void tdc_destroy(tdc_t **tdc)
{
	if(tdc == NULL)
	{
		return;
	}

	tdc_clear(*tdc);
	SRELEASE(*tdc);
	*tdc = NULL;
}

/******************************************************************************
 *
 * \par Function Name: tdc_get
 *
 * \par Purpose: Grabs an item from the specified index, checks the type given is
 *				equal to the type of the item, then copies it to the given buffer.
 *
 * \return The type, or AMP_TYPE_UNK if something failed.
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
 *  04/15/16  E. Birrane     Updated to blob_t
 *****************************************************************************/


amp_type_e tdc_get(tdc_t* tdc, uint8_t index, amp_type_e type, uint8_t** optr, size_t* outsize)
{
	uint8_t cur_type = 0;
	blob_t *entry = NULL;

	/* Step 0: Sanity Checks. */
	if((tdc == NULL) || (optr == NULL) || (outsize == NULL))
	{
		AMP_DEBUG_ERR("tdc_get","Bad Args.", NULL);
		return AMP_TYPE_UNK;
	}

	if(tdc->hdr.length <= index)
	{
		AMP_DEBUG_ERR("tdc_get","Cannot get index %d with only %d items.", index, tdc->hdr.length);
		return AMP_TYPE_UNK;
	}

	/* Step 1: Check the type. */
	cur_type = tdc_get_type(tdc, index);
	if(cur_type != type)
	{
		AMP_DEBUG_ERR("tdc_get","Item %d has type %d not %d.", index, cur_type, type);
		return AMP_TYPE_UNK;
	}

	/* Step 2: Grab the entry. */
	if((entry = tdc_get_colentry(tdc, index)) == NULL)
	{
		AMP_DEBUG_ERR("tdc_get", "UNable to retrieve index %d from TDC.", index);
		return AMP_TYPE_UNK;
	}

	/* Step 3: Copy over the pointers. */
	if((*optr = (uint8_t *) STAKE(entry->length)) == NULL)
	{
		AMP_DEBUG_ERR("tdc_get","Can't allocate entry.", NULL);
		return AMP_TYPE_UNK;
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
 *  04/15/16  E. Birrane     Updated to blob_t
 *****************************************************************************/

blob_t* tdc_get_colentry(tdc_t* tdc, uint8_t index)
{
	blob_t *result = NULL;
	uint8_t idx = 0;
	LystElt elt = NULL;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("tdc_get_colentry","Bad args.", NULL);
		return NULL;
	}

	if(tdc->hdr.length < index)
	{
		AMP_DEBUG_ERR("tdc_get_colentry","Bad index %d out of %d.", index, tdc->hdr.length);
		return NULL;
	}

	/* Step 1: Find correct DC. */
//	for(elt = lyst_first(tdc->datacol); idx != index + 1; elt = lyst_next(elt), idx++);

	for(elt = lyst_first(tdc->datacol); idx != index; elt = lyst_next(elt), idx++);

	/* Step 2: Copy it. */
	if(elt)
	{
		result = (blob_t*) lyst_data(elt);
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: tdc_get_count
 *
 * \par Purpose: Returns the number of items in a typed data collection
 *
 * \return <0 System Error
 *          >= 0 The number of elements
 *
 * \param[in]   tdc		The typed data collection
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation,
 *  06/27/15  E. Birrane     Ported from datalist to TDC.
 *****************************************************************************/

int8_t tdc_get_count(tdc_t* tdc)
{
	uint8_t result = 0;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("tdc_get_count","Bad Args.", NULL);
		return ERROR;
	}
	else if(tdc->datacol == NULL)
	{
		return 0;
	}

	/* Step 1: Grab the number of elements in the lyst. */
	result = lyst_length(tdc->datacol);

	/* Step 2: Check against type information. */
	if(tdc->hdr.length != result)
	{
		AMP_DEBUG_ERR("tdc_get_count","Count Mismatch %d entries and %d types.", result, tdc->hdr.length);
		return ERROR;
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
 *  04/15/16  E. Birrane     Updated to blob_t
 *****************************************************************************/

uint32_t tdc_get_entry_size(tdc_t* tdc, uint8_t index)
{
	blob_t *entry = NULL;

	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("tdc_get_entry_size","Bad Args.", NULL);
		return 0;
	}

	if(tdc->hdr.length <= index)
	{
		AMP_DEBUG_ERR("tdc_get_entry_size","Bad index %d <= %d.", tdc->hdr.length, index);
		return 0;
	}

	if((entry = tdc_get_colentry(tdc, index)) == NULL)
	{
		AMP_DEBUG_ERR("tdc_get_colentry","Can't get entry.", NULL);
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
 * \return the requested type, or AMP_TYPE_UNK.
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

amp_type_e tdc_get_type(tdc_t* tdc, uint8_t index)
{
	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("tdc_get_type","Bad Args.", NULL);
		return AMP_TYPE_UNK;
	}

	if(tdc->hdr.length <= index)
	{
		AMP_DEBUG_ERR("tdc_get_type","Bad index %d <= %d.", tdc->hdr.length, index);
		return AMP_TYPE_UNK;
	}

	return (amp_type_e) tdc->hdr.data[index];
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
 *  07/27/17  E. Birrane     Allow 0 length data
 *****************************************************************************/

uint8_t tdc_hdr_allocate(tdc_hdr_t* header, uint8_t dataSize)
{
	if(header == NULL)
	{
		AMP_DEBUG_ERR("tdc_hdr_allocate","Header == null",NULL);
		return 0;
	}


	if(header->data != NULL)
	{
		AMP_DEBUG_ERR("tdc_hdr_allocate","Header data != null",NULL);
		return 0;
	}


	if(dataSize > 0)
	{
		header->data = (uint8_t*) STAKE(dataSize);
	}
	else
	{
		header->data = NULL;
	}

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
 *  06/11/16  E. Birrane     Updated to handle reallocating an empty header.
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
	if((tmp = (uint8_t *) STAKE(newSize)) == NULL)
	{
		AMP_DEBUG_ERR("datalist_header_reallocate","Unable to allocate of size %d", newSize);
		return header->length;
	}

	/* Step 3: Copy over. */
	if(header->data != NULL)
	{
		memcpy(tmp, header->data, header->length);
		SRELEASE(header->data);
	}

	header->data = tmp;
	header->length = newSize;

	return newSize;
}


void tdc_init(tdc_t *tdc)
{
	if(tdc != NULL)
	{
		memset(tdc, 0, sizeof(tdc_t));
	}
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
 * \return       The type inserted, or AMP_TYPE_UNK if something failed.
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
 *  06/11/16  E. Birrane     Update to handle inserting to empty TDC.
 *****************************************************************************/

amp_type_e tdc_insert(tdc_t* tdc, amp_type_e type, uint8_t* data, uint32_t size)
{
	uint8_t len = 0;

	/* Step 0: Sanity Checks. */
	if((tdc == NULL) || (data == NULL))
	{
		AMP_DEBUG_ERR("tdc_insert","Bad Args.",NULL);
		return AMP_TYPE_UNK;
	}

	/* Step 1: Calculate the size of the new data. */
	if(size == 0)
	{
		if((size = type_get_size(type)) == 0)
		{
			AMP_DEBUG_ERR("tdc_insert","Can't get size for type %d", type);
			return AMP_TYPE_UNK;
		}
	}

	/* Step 2: Reallocate the type array. */
	if(tdc->datacol == NULL)
	{
		if((tdc->datacol = lyst_create()) == NULL)
		{
			AMP_DEBUG_ERR("tdc_insert","Can't allocate lyst.", NULL);
			return AMP_TYPE_UNK;
		}
		len = 1;
	}
	else
	{
		len = lyst_length(tdc->datacol) + 1;
	}

	if(tdc_hdr_reallocate(&(tdc->hdr), len) != len)
	{
		AMP_DEBUG_ERR("tdc_insert", "Can't expand type array", NULL);
		return AMP_TYPE_UNK;
	}

	/* Step 3: Add the data to the data collection. */
	if(dc_add(tdc->datacol, data, size) == 0)
	{
		// rollback.
		tdc_hdr_reallocate(&(tdc->hdr), len-1);
		AMP_DEBUG_ERR("tdc_insert","Unable to add to DC.", NULL);
		return AMP_TYPE_UNK;
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
 * \par +---------+-----------+-------------+     +-------------+
 *      | # BLOBs | TYPE BLOB | DATA BLOB 1 | ... | DATA BLOB N |
 *      |  [SDNV] |   [BLOB]  |   [BLOB]    |     |    [BLOB]   |
 *      +---------+-----------+-------------+     +-------------+
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
 *  09/09/15  E. Birrane     Updated to latest AMP spec.
 *****************************************************************************/

uint8_t* tdc_serialize(tdc_t *tdc, uint32_t *size)
{
	uint8_t *result = NULL;

	/* Step 0: Sanity Check. */
	if((tdc == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("tdc_serialize","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: If the TDC is empty, just write 0 as the # blobs. */
	if(tdc->hdr.length == 0)
	{
		if((result = (uint8_t *) STAKE(1)) == NULL)
		{
			AMP_DEBUG_ERR("tdc_serialize","Can't allocate 1 byte.", NULL);
			return NULL;
		}
		*result = 0;
		*size = 1;
		return result;
	}

	/* Step 2: Temporarily add the type blob to the TDC data collection. */
	dc_add_first(tdc->datacol, tdc->hdr.data, tdc->hdr.length);

	/* Step 3: Serialize the underlying DC with type blob */
	if((result = dc_serialize(tdc->datacol, size)) == NULL)
	{
		AMP_DEBUG_ERR("tdc_serialize","Can't serialize DC.", NULL);
		return NULL;
	}

	/* Step 4: Remove the temporarily added type blob. */
	dc_remove_first(tdc->datacol, 1);

	/* Step 5: Return the serialized TDC. */
	return result;
}



/******************************************************************************
 *
 * \par Function Name: tdc_set_type
 *
 * \par Purpose: Sets the type for a given index, allocating additional header
 *				space as needed-
 *				space
 * \return The type, or AMP_TYPE_UNK if something failed.
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

amp_type_e tdc_set_type(tdc_t* tdc, uint8_t index, amp_type_e type)
{
	/* Step 0: Sanity Check. */
	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("tdc_set_type","Bad Args.", NULL);
		return 0;
	}

	if(tdc->hdr.length <= index)
	{
		AMP_DEBUG_ERR("tdc_set_type","Bad index %d <= %d.", tdc->hdr.length, index);
		return 0;
	}

	tdc->hdr.data[index]=type;

	return type;
}



/*
 * Extremely inefficient. Only call on the ground.
 *
 * 07/27/2017: Allow 0 length header in TDC.
 */

char* tdc_to_str(tdc_t *tdc)
{
	char **entries = NULL;
	uint32_t *lens = NULL;
	LystElt elt = NULL;
	blob_t *cur_entry = NULL;
	amp_type_e cur_type;
	int i = 0;
	uint32_t tot_size = 0;
	char *result = NULL;
	char *cursor = NULL;

	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("tdc_to_str","Bad Args", NULL);
		return NULL;
	}

	if(tdc->hdr.length <= 0)
	{
		if((result = (char *) STAKE(11)) == NULL)
		{
			AMP_DEBUG_ERR("tdc_to_str","Can't allocate storage.",NULL);
			return NULL;
		}

		isprintf(result,11,"Empty TDC.", NULL);
		return result;
	}

	entries = (char **) STAKE(tdc->hdr.length * (sizeof(char *)));
	lens = (uint32_t *) STAKE(tdc->hdr.length * (sizeof(uint32_t)));

	if((entries == NULL) || (lens == NULL))
	{
		AMP_DEBUG_ERR("tdc_to_str","Can't allocate storage.",NULL);
		SRELEASE(entries);
		SRELEASE(lens);
		return NULL;
	}

	for(elt = lyst_first(tdc->datacol); elt; elt = lyst_next(elt))
	{
		cur_type = tdc->hdr.data[i];
		cur_entry = (blob_t *) lyst_data(elt);
		char *data_str = blob_to_str(cur_entry);

		lens[i] = 20 + strlen(data_str);
		entries[i] = (char *) STAKE(lens[i]);

		if(entries[i] != NULL)
		{
			sprintf(entries[i],"(%s) %s\n",  type_to_str(cur_type), data_str);
			tot_size += lens[i];
		}

		SRELEASE(data_str);

		i++;
	}

	result = (char *) STAKE(tot_size);
	cursor = result;

	for(i = 0; i < lyst_length(tdc->datacol); i++)
	{
		if(entries[i] != NULL)
		{
			if(cursor != NULL)
			{
				memcpy(cursor, entries[i], lens[i]);
				cursor += lens[i];
			}
			SRELEASE(entries[i]);
		}
	}

	SRELEASE(entries);
	SRELEASE(lens);

	return result;
}


