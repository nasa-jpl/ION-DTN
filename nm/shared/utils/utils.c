/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file utils.c
 **
 ** Subsystem:
 **          Network Management Utilities
 **
 ** Description:
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/29/12  E. Birrane     Initial Implementation.
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "shared/utils/debug.h"
#include "shared/utils/utils.h"



/******************************************************************************
 *
 * \par Function Name: atox
 *
 * \par Initializes an unsigned long with a value from a string. For example,
 * 		return an unsigned long with the value 0x01020304 from the string
 * 		"01020304".
 *
 * \param[in]  s        The string to be converted to a hex array.
 * \param[out] success  Whether the conversion was a success.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *  12/16/12  E. Birrane     Added success return, error checks, logging
 *****************************************************************************/

unsigned long utils_atox(char *s, int *success)
{
	unsigned long result = 0;
	int i = 0;
	int mult = 0;
	int j = 0;
	int temp = 0;

	DTNMP_DEBUG_ENTRY("utils_atox","(%#llx, %#llx)", s, success);

	/* Step 0 - Sanity Check. */
	if((s == NULL) || (success == NULL))
	{
		DTNMP_DEBUG_ERR("utils_atox","Bad Args.",NULL);
		*success = 0;
		DTNMP_DEBUG_ENTRY("utils_atox","->0.",NULL);
		return 0;
	}

	*success = 1;

	/* Step 1 - Make sure string isn't too long. Since every character in the
	 *          string represents a nibble, 2 characters are a byte, making
	 *          the longest valid length sizeof(unsigned long) * 2.
	 */
	if(strlen(s) > (sizeof(unsigned long) * 2))
	{
		DTNMP_DEBUG_ERR("utils_atox","x UI: String %s too long to convert to hex unsigned long.", s);
		*success = 0;
		DTNMP_DEBUG_ENTRY("utils_atox","->0.",NULL);
		return 0;
	}

	/* Step 2 - Walk through the string building the result. */
	for(i = strlen(s)-1; i >= 0; i--)
	{
		mult = j * 16;
		if(mult == 0)
		{
			mult = 1;
		}

		switch(s[i])
		{
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9':
				  temp = s[i] - '0';
				  result += temp * mult;
				  break;
		case 'A': case 'a': result += 10 * mult; break;
		case 'B': case 'b': result += 11 * mult; break;
		case 'C': case 'c': result += 12 * mult; break;
		case 'D': case 'd': result += 13 * mult; break;
		case 'E': case 'e': result += 14 * mult; break;
		case 'F': case 'f': result += 15 * mult; break;
		default:
			DTNMP_DEBUG_ERR("utils_atox","x Non-hex character: %c", s[i]);
			*success = 0;
			j--;
			break;
		}
		j++;
	}

	DTNMP_DEBUG_INFO("utils_atox","i UI: Turned string %s to number %x.", s, result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: utils_datacol_compare
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

int utils_datacol_compare(Lyst col1, Lyst col2)
{
	LystElt elt1;
	LystElt elt2;
	datacol_entry_t *entry1 = NULL;
	datacol_entry_t *entry2 = NULL;

	DTNMP_DEBUG_ENTRY("utils_datacol_compare","(%#llx, %#llx)",col1, col2);

	/* Step 0: Sanity check. */
	if((col1 == NULL) || (col2 == NULL))
	{
		DTNMP_DEBUG_ERR("utils_datacol_compare", "Bad Args.", NULL);

		DTNMP_DEBUG_EXIT("utils_datacol_compare","->-1.", NULL);
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
				DTNMP_DEBUG_EXIT("utils_datacol_compare","->1.", NULL);
				return 1;
			}
			else if(memcmp(entry1->value,entry2->value,entry1->length) != 0)
			{
				DTNMP_DEBUG_EXIT("utils_datacol_compare","->1.", NULL);
				return 1;
			}

			elt1 = lyst_next(elt1);
			elt2 = lyst_next(elt2);
		}
	}
	else
	{
		DTNMP_DEBUG_EXIT("utils_datacol_compare","->1.", NULL);
		return 1;
	}

	DTNMP_DEBUG_EXIT("utils_datacol_compare","->0.", NULL);
	return 0;
}



/******************************************************************************
 *
 * \par Function Name: utils_datacol_copy
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

Lyst utils_datacol_copy(Lyst col)
{
	Lyst result = NULL;
	LystElt elt;
	datacol_entry_t *cur_entry = NULL;
	datacol_entry_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("utils_datacol_copy","(%#llx)",(unsigned long) col);

	/* Step 0: Sanity Check. */
	if(col == NULL)
	{
		DTNMP_DEBUG_ERR("utils_datacol_copy","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("utils_datacol_copy","->NULL.",NULL);
		return NULL;
	}

	/* Build the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("utils_datacol_copy","Unable to create lyst.",NULL);
		DTNMP_DEBUG_EXIT("utils_datacol_copy","->NULL.",NULL);
		return NULL;
	}

	/* Walking copy. */
	int success = 1;
	for(elt = lyst_first(col); elt; elt = lyst_next(elt))
	{
		cur_entry = (datacol_entry_t *) lyst_data(elt);

		if((new_entry = (datacol_entry_t*)MTAKE(sizeof(datacol_entry_t))) == NULL)
		{
			DTNMP_DEBUG_ERR("utils_datacol_copy","Failed to alloc %d bytes.",
					        sizeof(datacol_entry_t));
			utils_datacol_destroy(&result);

			DTNMP_DEBUG_EXIT("utils_datacol_copy","->NULL.",NULL);
			return NULL;
		}

		new_entry->length = cur_entry->length;

		if((new_entry->value = (uint8_t*)MTAKE(new_entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("utils_datacol_copy","Failed to alloc %d bytes.",
					        new_entry->length);
			MRELEASE(new_entry);
			utils_datacol_destroy(&result);

			DTNMP_DEBUG_EXIT("utils_datacol_copy","->NULL.",NULL);
			return NULL;
		}

		memcpy(new_entry->value, cur_entry->value, new_entry->length);

		lyst_insert_last(result,new_entry);
	}

	DTNMP_DEBUG_EXIT("utils_datacol_copy","->%#llx.",(unsigned long) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: utils_datacol_deserialize
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

Lyst utils_datacol_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t *bytes_used)
{
	unsigned char *cursor = NULL;
	Lyst result = NULL;
	uint32_t bytes = 0;
	uvast num = 0;
	uvast len = 0;
	uint32_t i = 0;

	DTNMP_DEBUG_ENTRY("utils_datacol_deserialize","(%#llx,%d,%#llx)",
			          (unsigned long) buffer, buffer_size,
			          (unsigned long) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("utils_datacol_deserialize","Bad Args", NULL);
		DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;

	/* Step 1: Create the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("utils_datacol_deserialize","Can't create lyst.", NULL);
		DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab # entries in the collection. */
	if((bytes = utils_grab_sdnv(cursor, buffer_size, &num)) == 0)
	{
		DTNMP_DEBUG_ERR("utils_datacol_deserialize","Can't parse SDNV.", NULL);
		lyst_destroy(result);

		DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->NULL",NULL);
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
			DTNMP_DEBUG_ERR("utils_datacol_deserialize","Can't grab MID #%d.", i);
			utils_datacol_destroy(&result);

			DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->NULL",NULL);
			return NULL;
		}

		/* Grab data item length. */
		if((bytes = utils_grab_sdnv(cursor, buffer_size, &len)) == 0)
		{
			DTNMP_DEBUG_ERR("utils_datacol_deserialize","Can't parse SDNV.", NULL);
			MRELEASE(entry);
			utils_datacol_destroy(&result);

			DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->NULL",NULL);
			return NULL;
		}

		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;

		entry->length = len;

		if((entry->value = (uint8_t*)MTAKE(entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("utils_datacol_deserialize","Can't parse SDNV.", NULL);
			MRELEASE(entry);
			utils_datacol_destroy(&result);

			DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->NULL",NULL);
			return NULL;
		}

		memcpy(entry->value, cursor, entry->length);
		cursor += entry->length;
		buffer_size -= entry->length;
		*bytes_used += entry->length;

		/* Drop it in the lyst in order. */
		lyst_insert_last(result, entry);
	}

	DTNMP_DEBUG_EXIT("utils_datacol_deserialize","->%#llx",(unsigned long)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: utils_datacol_destroy
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

void utils_datacol_destroy(Lyst *datacol)
{
	LystElt elt;
	datacol_entry_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("utils_datacol_destroy","(%#llx)", (unsigned long) datacol);

	/*
	 * Step 0: Make sure we even have a lyst. Not an error if not, since we
	 * are destroying anyway.
	 */
	if((datacol == NULL) || (*datacol == NULL))
	{
		DTNMP_DEBUG_ERR("utils_datacol_destroy","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("utils_datacol_destroy","->.", NULL);
		return;
	}

	/* Step 1: Walk through the MIDs releasing as you go. */
    for(elt = lyst_first(*datacol); elt; elt = lyst_next(elt))
    {
    	entry = (datacol_entry_t *) lyst_data(elt);

    	if(entry != NULL)
    	{
    		MRELEASE(entry->value);
    		MRELEASE(entry);
    	}
    }

    /* Step 2: Destroy and zero out the lyst. */
    lyst_destroy(*datacol);
    *datacol = NULL;

    DTNMP_DEBUG_EXIT("utils_datacol_destroy","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: utils_datacol_serialize
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

uint8_t *utils_datacol_serialize(Lyst datacol, uint32_t *size)
{
	uint8_t *result = NULL;
	Sdnv num_sdnv;
	Sdnv tmp;
	LystElt elt;

	DTNMP_DEBUG_ENTRY("utils_datacol_serialize","(%#llx, %#llx)",
			          (unsigned long) datacol, (unsigned long) size);

	/* Step 0: Sanity Check */
	if((datacol == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("utils_datacol_serialize","Bad args.", NULL);
		DTNMP_DEBUG_EXIT("utils_datacol_serialize","->NULL",NULL);
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
    		DTNMP_DEBUG_WARN("utils_datacol_serialize","Found NULL Entry?", NULL);
    	}
    }

    /* Step 3: Allocate the space for the serialized list. */
    if((result = (uint8_t*) MTAKE(*size)) == NULL)
    {
		DTNMP_DEBUG_ERR("utils_datacol_serialize","Can't alloc %d bytes", *size);
		*size = 0;

		DTNMP_DEBUG_EXIT("utils_datacol_serialize","->NULL",NULL);
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
    		DTNMP_DEBUG_WARN("utils_datacol_serialize","Found NULL MID?", NULL);
    	}
    }

    /* Step 5: Final sanity check. */
    if((cursor - result) != *size)
    {
		DTNMP_DEBUG_ERR("utils_datacol_serialize","Wrote %d bytes not %d bytes",
				        (cursor - result), *size);
		*size = 0;
		MRELEASE(result);
		DTNMP_DEBUG_EXIT("utils_datacol_serialize","->NULL",NULL);
		return NULL;
    }

	DTNMP_DEBUG_EXIT("utils_datacol_serialize","->%#llx",(unsigned long) result);
	return result;
}




/******************************************************************************
 *
 * \par Function Name: utils_grab_byte
 *
 * \par Purpose: extracts a byte from a sized buffer.
 *
 * \return 0 - Failure.
 * 		   >0 - # bytes consumed from the buffer.
 *
 * \param[in,out]  cursor      Pointer into current buffer.
 * \param[in]      size        Remaining size of the buffer.
 * \param[out]     result      The extracted byte.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

int8_t utils_grab_byte(unsigned char *cursor,
		  		       uint32_t size,
				       uint8_t *result)
{
	DTNMP_DEBUG_ENTRY("utils_grab_byte","(%x,%d,%x)",
			          (unsigned long) cursor, size,
			          (unsigned long) result);

	/* Do we have a byte to grab? */
	if(size < 1)
	{
        DTNMP_DEBUG_ERR("utils_grab_byte","Bounds overrun. Size %d Used %d.",
        				size, 1);
        DTNMP_DEBUG_EXIT("utils_grab_byte","-> 0", NULL);
        return 0;
	}

    *result = *cursor;

    DTNMP_DEBUG_EXIT("utils_grab_byte","-> 1", NULL);

    return 1;
}



/******************************************************************************
 *
 * \par Function Name: utils_grab_sdnv
 *
 * \par Purpose: extracts an SDNV value from a sized buffer.
 *
 * \return 0 - Failure.
 * 		   >0 - # bytes consumed from the buffer.
 *
 * \param[in,out]  cursor      Pointer into current buffer.
 * \param[in]      size        Remaining size of the buffer.
 * \param[out]     result      The extracted value.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *  06/17/13  E. Birrane     Updated to ION 3.1.3, added uvast type.
 *****************************************************************************/

uint32_t utils_grab_sdnv(unsigned char *cursor,
		                 uint32_t size,
		                 uvast *result)
{
	int result_len = 0;

	DTNMP_DEBUG_ENTRY("utils_grab_sdnv","(%x,%d,%x)",
			          (unsigned long) cursor,
			          (unsigned long) size,
			          (unsigned long) result);

    if((result_len = decodeSdnv(result, cursor)) == 0)
    {
        DTNMP_DEBUG_ERR("utils_grab_sdnv","Bad SDNV extract.", NULL);
		DTNMP_DEBUG_EXIT("utils_grab_sdnv","-> 0", NULL);
        return 0;
    }

    /* Did we go too far? */
	if (result_len > size)
	{
		DTNMP_DEBUG_ERR("utils_grab_sdnv","Bounds overrun. Size %d Used %d.",
						size, result_len);

		DTNMP_DEBUG_EXIT("utils_grab_sdnv","-> 0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("utils_grab_sdnv","-> %d", result_len);
	return result_len;
}



/******************************************************************************
 *
 * \par Function Name: utils_hex_to_string
 *
 * \par Purpose: Constructs a character string of values from a buffer.
 *
 * \return NULL - Failure.
 * 		   !NULL - Desired string
 *
 * \param[in]  buffer  The buffer whose string representation is desired.
 * \param[in]  size    Size of the buffer, in bytes,
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

char *utils_hex_to_string(uint8_t *buffer, uint32_t size)
{
    char *result = NULL;
    uint32_t char_size = 0;

    char temp[3];
    int i = 0;
    int r = 0;

    DTNMP_DEBUG_ENTRY("utils_hex_to_string","(%x,%d)",
    		          (unsigned long) buffer, size);

    /* Each byte requires 2 characters to represent in HEX. Also, require
     * three additional bytes to capture '0x' and NULL terminator.
     */
    char_size = (2 * size) + 3;
    result = (char *) MTAKE(char_size);

    if(result == NULL)
    {
        DTNMP_DEBUG_ERR("utils_hex_to_string", "Cannot allocate %d bytes.",
        		        char_size);
        DTNMP_DEBUG_EXIT("utils_hex_to_string", "-> NULL.", NULL);
        return NULL;
    }

    result[0] = '0';
    result[1] = 'x';
    r = 2;

    for(i = 0; i < size; i++)
    {
        sprintf(temp, "%.2x", (unsigned int)buffer[i]);
        result[r++] = temp[0];
        result[r++] = temp[1];
    }

    result[r] = '\0';

    DTNMP_DEBUG_EXIT("mid_to_string","->%s.", result);

    return result;
}



/******************************************************************************
 *
 * \par Function Name: utils_print_hex
 *
 * \par Purpose: Prints a string as a series of hex characters.
 *
 * \param[in]  s     The string to be printed.
 * \param[in]  len   The length of the string.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

void utils_print_hex(unsigned char *s, uint32_t len)
{
	int i;

	printf("0x");
	for(i = 0; i < len; i++)
	{
		printf("%.2x", s[i]);
	}
	printf("\n");
}



/******************************************************************************
 *
 * \par Function Name: utils_string_to_hex
 *
 * \par Purpose: Converts an ASCII string representing hex values to a byte
 *               array of those hex values.
 *
 * \return NULL - Failure.
 * 		   !NULL - The desired byte array.
 *
 * \param[in]   value  The string to be converted to hex.
 * \param[out]  size   The length of the converted byte array.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

uint8_t *utils_string_to_hex(unsigned char *value, uint32_t *size)
{
	uint8_t *result = NULL;
	char tmp_s[3];
	int len = 0;
	int success = 0;
	int pad = 0; 

	DTNMP_DEBUG_ENTRY("utils_string_to_hex","(%#llx, %#llx)", value, size);

	/* Step 0 - Sanity Checks. */
	if((value == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("utils_string_to_hex", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("utils_string_to_hex", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Figure out the size of the byte array. Since each ASCII
	 *          character represents a nibble, the size of the byte array is
	 *          half the size of the string (accounting for odd values).
	 */
	len = strlen((char*)value);

	if((len%2) == 0)
	{
	  *size = len/2;
	}
	else
	{
  	*size = (len/2) + 1;
       pad = 1;
	}

	if((result = (uint8_t *) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("utils_string_to_hex","Can't Alloc %d bytes.", *size);
		*size = 0;

		DTNMP_DEBUG_EXIT("utils_string_to_hex", "->NULL.", NULL);
		return NULL;
	}

	/* Step 2 - For each byte, copy in the nibbles. */
	tmp_s[2] = '\0';
	int incr = 1;
    int base = 0;
    int i = 0;

	for(i = 0; i < len;)
	{
		if(pad == 1)
		{
			tmp_s[0] = '0';
			tmp_s[1] = value[i];
			pad = 0;
			incr = 1;
			base = 1;
		}
		else
		{
			memcpy(tmp_s, &(value[i]), 2);
			incr = 2;
		}

		result[(i+base)/2] = utils_atox(tmp_s, &success);
	
		i += incr;
		if(success == 0)
		{
			DTNMP_DEBUG_ERR("utils_string_to_hex","Can't AtoX %s.", tmp_s);
			MRELEASE(result);
			*size = 0;

			DTNMP_DEBUG_EXIT("utils_string_to_hex", "->NULL.", NULL);
			return NULL;
		}
	}

	DTNMP_DEBUG_EXIT("utils_string_to_hex", "->%#llx.", result);
	return result;
}




/*
 * THis software adapted from:
 * http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 *
 * performs: result = t1 - t2.
 */
int utils_time_delta(struct timeval *result, struct timeval *t1, struct timeval *t2)
{
	/* Perform the carry for the later subtraction by updating t2. */
	if (t1->tv_usec < t2->tv_usec) {
		int nsec = (t2->tv_usec - t1->tv_usec) / 1000000 + 1;
		t2->tv_usec -= 1000000 * nsec;
		t2->tv_sec += nsec;
	}
	if (t1->tv_usec - t2->tv_usec > 1000000) {
		int nsec = (t1->tv_usec - t2->tv_usec) / 1000000;
		t2->tv_usec += 1000000 * nsec;
		t2->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	          tv_usec is certainly positive. */
	result->tv_sec = t1->tv_sec - t2->tv_sec;
	result->tv_usec = t1->tv_usec - t2->tv_usec;

	/* Return 1 if result is negative. */
	return t1->tv_sec < t2->tv_sec;
}

/* Return number of micro-seconds that have elapsed since the passed-in time.*/
vast    utils_time_cur_delta(struct timeval *t1)
{
	vast result = 0;

	struct timeval cur;
	struct timeval delta;
	int neg = 0;

	getCurrentTime(&cur);
	neg = utils_time_delta(&delta, &cur, t1);

	result = delta.tv_usec;
	result += delta.tv_sec * 1000000;

	if(neg)
	{
		result *= -1;
	}

	return result;
}

