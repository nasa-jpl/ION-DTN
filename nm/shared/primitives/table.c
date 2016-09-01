/*****************************************************************************
 **
 ** File Name: table.c
 **
 ** Description: This implements a typed table.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/15/16  E. Birrane    Initial Implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "lyst.h"
#include "ion.h"
#include "tdc.h"
#include "../utils/utils.h"

#include "../primitives/table.h"

#include "../primitives/value.h"


/******************************************************************************
 *
 * \par Function Name: table_add_col
 *
 * \par Purpose: Adds a col to an existing, EMPTY table structure.
 *
 * \return -1 System Error, col not added
 *          0 User Error, col not added. Perhaps table not empty.
 *          1 Success, col added
 *
 * \param[in|out]   table    The table having a column added
 * \param[in]       name     The column name being added
 * \param[in]       type     The data type of the column
 *
 * \par Notes:
 *  - This function DEEP-COPIES the col name into the table.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/06/16  E. Birrane     Initial implementation
 *****************************************************************************/

int8_t table_add_col(table_t *table, char *name, amp_type_e type)
{
	blob_t *new_name = NULL;
	uint8_t *data = NULL;

	/* Step 0 - Sanity checks. */
	CHKERR(table);
	CHKERR(name);

	if(lyst_length(table->rows) != 0)
	{
		AMP_DEBUG_ERR("table_add_col","Table has %d rows.", lyst_length(table->rows));
		return ERROR;
	}

	/* Step 1 - Add the extra type to the blob. */
	if((blob_append(&(table->hdr.types), (uint8_t*)&type, 1)) == ERROR)
	{
		AMP_DEBUG_ERR("table_add_col", "Unable to add new type.", NULL);
		return ERROR;
	}

	/* Step 2 - Deep copy name. */
	if((data = STAKE(strlen(name) + 1)) == NULL)
	{
		blob_trim(&(table->hdr.types), 1);
		AMP_DEBUG_ERR("table_add_col", "Unable to allocate %d bytes.", strlen(name));
		return ERROR;
	}

	/* Step 3 - Copy name in (plus NULL terminator). */
	memcpy(data, name, strlen(name) + 1);

	/* Step 4 - Copy the name and add it to the table name list.
	 * This function shallow-copies data, so do not release data on success.
	 */
	if((new_name = blob_create((uint8_t*)data, strlen(name) + 1)) == NULL)
	{
		SRELEASE(data);
		blob_trim(&(table->hdr.types), 1);
		AMP_DEBUG_ERR("table_add_col", "Unable to allocate %d bytes.", strlen(name));
		return ERROR;
	}

	lyst_insert_last(table->hdr.names, new_name);

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: table_add_row
 *
 * \par Purpose: Adds a row to an existing table structure.
 *
 * \return -1 System Error, row not added
 *          0 User Error, row not added
 *          1 Success, row added
 *
 * \param[in|out]   table    The table having a row added
 * \param[in]       new_cels  The row being added
 *
 * \par Notes:
 *  - This function SHALLOW-COPIES the row into the table. The calling function
 *     MUST NOT release this memory.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

int8_t table_add_row(table_t *table, Lyst new_cels)
{
	table_row_t *new_row = NULL;

	if((table == NULL) || (new_cels == NULL))
	{
		AMP_DEBUG_ERR("table_add_row","Bad Args.", NULL);
		return 0;
	}

	if((new_row = STAKE(sizeof(table_row_t))) == NULL)
	{
		AMP_DEBUG_ERR("table_add_row","Can't allocate %d bytes.", sizeof(table_row_t));
		return -1;
	}

	new_row->cels = new_cels;
	lyst_insert_last(table->rows, new_row);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: table_clear
 *
 * \par Purpose: Removes all rows from a table,
 *
 * \param[in|out]   table    The table being cleared.
 *
 * \par Notes:
 *  - This function SHALLOW-COPIES the row into the table. The calling function
 *     MUST NOT release this memory.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

void table_clear(table_t *table)
{
	LystElt row_elt = NULL;
	LystElt cel_elt = NULL;
	blob_t* cur_blob = NULL;
	table_row_t *cur_row = NULL;

	for(row_elt = lyst_first(table->rows); row_elt; row_elt = lyst_next(row_elt))
	{
		if((cur_row = (table_row_t*) lyst_data(row_elt)) != NULL)
		{
			for(cel_elt = lyst_first(cur_row->cels); cel_elt; cel_elt = lyst_next(cel_elt))
			{
				if((cur_blob = (blob_t *) lyst_data(cel_elt)))
				{
					blob_destroy(cur_blob, 1);
				}
			}
			lyst_destroy(cur_row->cels);
		}
		SRELEASE(cur_row);
	}
	lyst_clear(table->rows);
}



/******************************************************************************
 *
 * \par Function Name: table_create
 *
 * \par Purpose: Creates a table structure.
 *
 * \return NULL - Error
 *         !NULL - The created table.
 *
 * \param[in]  col_desc   Description of the columns for the table.
 * \param[in]  names      Name for each column
 *
 * \par Notes:
 *
 *  - This function SHALLOW-COPIES the column names into the table. The calling
 *    function MUST NOT release this memory.
 *  - If this function returns NULL, the calling function MUST release the passed
 *    in memory, since the table was never created.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

table_t*  table_create(blob_t *col_desc, Lyst names)
{
	table_t *result = NULL;

	if((col_desc != NULL) && (names != NULL))
	{
		if(col_desc->length != lyst_length(names))
		{
			AMP_DEBUG_ERR("table_create","%d columns but %d names.", col_desc->length, lyst_length(names));
			return NULL;
		}
	}

	if((result = (table_t *) STAKE(sizeof(table_t))) == NULL)
	{
		AMP_DEBUG_ERR("table_create","Can't allocate %d bytes.", sizeof(table_t));
		return NULL;
	}

	memset(result, 0, sizeof(table_t));


	if(names == NULL)
	{
		result->hdr.names = lyst_create();
	}
	else
	{
		result->hdr.names = names;
	}

	if(col_desc != NULL)
	{
		blob_t *tmp;

		/* copy the bloc to deep copy the underlying data structures, then
		 * shallow copy that to the table, then release the tmp pointer.
		 */
		if((tmp = blob_copy(col_desc)) == NULL)
		{
			AMP_DEBUG_ERR("table_create","Can't copy blob.", NULL);
			SRELEASE(result);
			return NULL;
		}
		result->hdr.types = *tmp;
		SRELEASE(tmp);
	}

	if((result->rows = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("table_create", "Can't make lyst.", NULL);
		blob_destroy(&(result->hdr.types), 0);
		SRELEASE(result);
		return NULL;
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: table_copy
 *
 * \par Purpose: Deep copies a data structure.
 *
 * \return NULL - Error
 *         !NULL - The copied table.
 *
 * \param[in]  table  THe table to be copied.
 *
 * \par Notes:
 *  - This is A QUICK and INEFFICIENT copy...
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

table_t*  table_copy(table_t *table)
{
	table_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;

	if((data = table_serialize(table, &size)) == NULL)
	{
		return NULL;
	}

	result = table_deserialize(data, size, &bytes);

	SRELEASE(data);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: table_deserialize
 *
 * \par Purpose: Creates a table structure from a byte array
 *
 * \return NULL - Error
 *         !NULL - The deserialized table.
 *
 * \param[in]   buffer		the byte array
 * \param[in]   buffer_size	The length of buffer
 * \param[out]  bytes_used	The length of buffer which was deserialized.
 *
 * \par Notes:
 *
 * A table on the wire looks like:
 *
 *  +-----------+-----------+--------+-------+     +-------+
 *  | Col Names | Col Types | # Rows | Row 1 |     | Row N |
 *  |   [DC]    |  [BLOB]   | [SDNV] | [DC]  | ... |  [DC] |
 *  +-----------+-----------+--------+-------+     +-------+
 *
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

table_t*  table_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t* bytes_used)
{
	table_t *result = NULL;
	blob_t *types = NULL;
	Lyst names = NULL;
	Lyst cur_cels = NULL;

	uint8_t *cursor = NULL;
	uvast num_rows = 0;
	uvast cur_row = 0;

	uint32_t size = 0;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("table_deserialize","(" ADDR_FIELDSPEC ",%d," ADDR_FIELDSPEC ")",
			          (uaddr) buffer, buffer_size, (uaddr) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("table_deserialize","Bad Args", NULL);
		AMP_DEBUG_EXIT("table_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;
	size = buffer_size;

	/* Step 1 - Deserialize the column names. */
	if((names = dc_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("table_deserialize","Can't get names.", NULL);
		*bytes_used = 0;
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 2: Deserialize the column descriptions.	 */
	if((types = blob_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("table_deserialize","Can't get types", NULL);
		dc_destroy(&names);
		*bytes_used = 0;
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Deserialize the # rows. */
	if((bytes = utils_grab_sdnv(cursor, buffer_size, &num_rows)) == 0)
	{
		AMP_DEBUG_ERR("table_deserialize","Can't parse SDNV.", NULL);
		blob_destroy(types, 1);
		dc_destroy(&names);
		*bytes_used = 0;
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 4 - Create the empty table. */
	if((result = table_create(types, names)) == NULL)
	{
		AMP_DEBUG_ERR("table_deserialize","Can't parse SDNV.", NULL);
		blob_destroy(types, 1);
		dc_destroy(&names);
		*bytes_used = 0;
		return NULL;
	}

	/* Step 5 - Deserialize and add each row. */
	for(cur_row = 0; cur_row < num_rows; cur_row++)
	{
		if((cur_cels = dc_deserialize(cursor, size, &bytes)) == NULL)
		{
			AMP_DEBUG_ERR("table_deserialize","Can't get %dth row.", cur_row);
			table_destroy(result, 1);
			blob_destroy(types, 1);
			*bytes_used = 0;
			return NULL;
		}
		else
		{
			cursor += bytes;
			size -= bytes;
			*bytes_used += bytes;

			table_add_row(result, cur_cels);
		}
	}

	blob_destroy(types, 1);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: table_destroy
 *
 * \par Purpose: Frees all memory associated with a table.
 *
 *
 * \param[in|out] table    The table being destroyed.
 * \param[in]     destroy  Whether to free the memory holding the table.
 *
 * \par Notes:
 *
 * - After a call to this function, the table MUST NOT be accessed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

void table_destroy(table_t *table, uint8_t destroy)
{
	LystElt name_elt = NULL;

	CHKVOID(table);

	/* Remove all rows from the table. */
	table_clear(table);
	lyst_destroy(table->rows);

	/* Remove column information from the table. */
	blob_destroy(&(table->hdr.types), 0);

	for(name_elt = lyst_first(table->hdr.names); name_elt; name_elt = lyst_next(name_elt))
	{
		blob_t *name = (blob_t *)lyst_data(name_elt);
		blob_destroy(name, 1);
	}

	lyst_destroy(table->hdr.names);

	if(destroy)
	{
		SRELEASE(table);
	}
}



/******************************************************************************
 *
 * \par Function Name: table_extract_col
 *
 * \par Purpose: Creates a typed data collection of entries in the column.
 *
 * \retval NULL - Error
 *         !NULL - a TDC representing values from the column.
 *
 * \param[in] table    The table being queried.
 * \param[in] col_idx  THe column being queried.
 *
 * \par Notes:
 *
 * - The TDC is a DEEP COPY of the data and must be freed by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

tdc_t *table_extract_col(table_t *table, uint32_t col_idx)
{
	tdc_t *result = NULL;
	LystElt row_elt = NULL;
	LystElt cel_elt = NULL;
	blob_t* cur_blob = NULL;
	table_row_t *cur_row = NULL;
	uint32_t cur_col_idx = 0;
	amp_type_e type = AMP_TYPE_UNK;

	/* Step 0 - Sanity check. */
	if((table == NULL) || (col_idx > table->hdr.types.length))
	{
		AMP_DEBUG_ERR("table_extract_col","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1 - Figure out the column type. The Nth column
	 * type is the Nth byte in the column header type blob.
	 */
	type = table->hdr.types.value[col_idx];

	/* Step 2 - Create the resultant TDC. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
	{
		AMP_DEBUG_ERR("table_Extract_col", "Can't create result TDC.", NULL);
		return NULL;
	}

	/* Step 3 - For each row... */
	for(row_elt = lyst_first(table->rows); row_elt; row_elt = lyst_next(row_elt))
	{
		if((cur_row = (table_row_t*) lyst_data(row_elt)) != NULL)
		{
			cur_col_idx = 0;

			/* Step 3.1 - For each cell in the row... */
			for(cel_elt = lyst_first(cur_row->cels); cel_elt; cel_elt = lyst_next(cel_elt))
			{
				if(cur_col_idx == col_idx)
				{
					if((cur_blob = (blob_t *) lyst_data(cel_elt)))
					{
						tdc_insert(result, type, cur_blob->value, cur_blob->length);
					}
					break;
				}
				cur_col_idx++;
			}
		}
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: table_extract_row
 *
 * \par Purpose: Creates a typed data collection of entries in a row.
 *
 * \retval NULL - Error
 *         !NULL - a TDC representing values from the row.
 *
 * \param[in] table    The table being queried.
 * \param[in] row_idx  The row being queried.
 *
 * \par Notes:
 *
 * - The TDC is a DEEP COPY of the data and must be freed by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

tdc_t *table_extract_row(table_t *table, uint32_t row_idx)
{
	tdc_t *result = NULL;
	LystElt row_elt = NULL;
	LystElt cel_elt = NULL;
	blob_t* cur_blob = NULL;
	table_row_t *cur_row = NULL;
	uint32_t cur_row_idx = 0;
	uint32_t cel_idx = 0;

	/* Step 0 - Sanity check. */
	if((table == NULL) || (row_idx > table_get_num_rows(table)))
	{
		AMP_DEBUG_ERR("table_extract_row","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1 - Create the resultant TDC. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
	{
		AMP_DEBUG_ERR("table_Extract_row", "Can't create result TDC.", NULL);
		return NULL;
	}

	/* Step 3 - For each row... */
	for(row_elt = lyst_first(table->rows); row_elt; row_elt = lyst_next(row_elt))
	{
		/* Step 3.1 - If this is the row being queried... */
		if(cur_row_idx == row_idx)
		{
			if((cur_row = (table_row_t*) lyst_data(row_elt)) != NULL)
			{
				cel_idx = 0;

				/* Step 3.2 - For each cel in the row, add it to the TDC. */
				for(cel_elt = lyst_first(cur_row->cels); cel_elt; cel_elt = lyst_next(cel_elt))
				{
					if((cur_blob = (blob_t *) lyst_data(cel_elt)))
					{
						tdc_insert(result, table->hdr.types.value[cel_idx], cur_blob->value, cur_blob->length);
					}
					cel_idx++;
				}
			}
		}
		cur_row_idx++;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: table_extract_cel
 *
 * \par Purpose: Retrieve a blob from a table.
 *
 * \retval NULL - Error
 *         !NULL - The cel contents.
 *
 * \param[in] table    The table being queried.
 * \param[in] col_idx  The column being queried.
 * \param[in] row_idx  The row being queried.
 *
 * \par Notes:
 *
 * - The BLOB is a DEEP COPY of the data and must be freed by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

blob_t* table_extract_cel(table_t *table, uint32_t col_idx, uint32_t row_idx)
{
	blob_t *result = NULL;
	LystElt row_elt = NULL;
	LystElt cel_elt = NULL;
	blob_t* cur_blob = NULL;
	table_row_t *cur_row = NULL;
	uint32_t cur_row_idx = 0;
	uint32_t cel_idx = 0;

	/* Step 0 - Sanity check. */
	if((table == NULL) ||
	   (col_idx > table_get_num_cols(table)) ||
	   (row_idx > table_get_num_rows(table)))
	{
		AMP_DEBUG_ERR("table_extract_cel","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1 - For each row... */
	for(row_elt = lyst_first(table->rows); row_elt; row_elt = lyst_next(row_elt))
	{
		/* Step 3.1 - If this is the row being queried... */
		if(cur_row_idx == row_idx)
		{
			if((cur_row = (table_row_t*) lyst_data(row_elt)) != NULL)
			{
				cel_idx = 0;

				/* Step 3.2 - For each cel in the row, add it to the TDC. */
				for(cel_elt = lyst_first(cur_row->cels); cel_elt; cel_elt = lyst_next(cel_elt))
				{
					if(cel_idx == col_idx)
					{
						if((cur_blob = (blob_t *) lyst_data(cel_elt)))
						{
							result = blob_copy(cur_blob);
							return result;
						}
					}
					cel_idx++;
				}
			}
		}
		cur_row_idx++;
	}

	return NULL;

}



/******************************************************************************
 *
 * \par Function Name: table_get_col_idx
 *
 * \par Purpose: Retrieve the column index for a given column ID.
 *
 * \retval < 0 - Error
 *         >= 0 - The column index.
 *
 * \param[in] table     The table whose column index is being found.
 * \param[in] col_name  The column name.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

int32_t table_get_col_idx(table_t *table, blob_t *col_name)
{
	int32_t result = -1;
	LystElt elt = NULL;
	blob_t *cur_blob = NULL;

	CHKERR(table);
	CHKERR(col_name);

	result = 0;
	for(elt = lyst_first(table->hdr.names); elt; elt = lyst_next(elt))
	{
		if((cur_blob = (blob_t *) lyst_data(elt)) != NULL)
		{
			if((cur_blob->length == col_name->length) &&
			   (memcmp(cur_blob->value, col_name->value, col_name->length) == 0))
			{
				return result;
			}
		}

		result++;
	}

	return -1;
}



/******************************************************************************
 *
 * \par Function Name: table_get_num_cols
 *
 * \par Purpose: Retrieve the number of columns in the table.
 *
 * \retval < 0 - Error
 *         >= 0 - The # columns.
 *
 * \param[in] table     The table whose column count is being queried.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

int8_t table_get_num_cols(table_t *table)
{
	CHKERR(table);
	return table->hdr.types.length;
}



/******************************************************************************
 *
 * \par Function Name: table_get_num_rows
 *
 * \par Purpose: Retrieve the number of rows in the table.
 *
 * \retval < 0 - Error
 *         >= 0 - The # rows.
 *
 * \param[in] table     The table whose row count is being queried.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

int8_t   table_get_num_rows(table_t *table)
{
	CHKERR(table);
	CHKERR(table->rows);
	return lyst_length(table->rows);
}



/******************************************************************************
 *
 * \par Function Name: table_make_subtable
 *
 * \par Purpose: Create a sub-table with a reduced number of columns.
 *
 * \retval NULL - Error
 *         !NULL - The subtable.
 *
 * \param[in] table     The master table
 * \param[in] col_names The names of the columns to take for the new table.
 *
 * \par Notes:
 *
 *  - The col_names is DEEP-COPIED into the new table.
 *  - The master table data is also DEEP-COPIED into the new table.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

table_t*  table_make_subtable(table_t *table, Lyst col_names)
{
	AMP_DEBUG_ERR("table_make_subtable","Not implemented.", NULL);
	return NULL;
}


/******************************************************************************
 *
 * \par Function Name: table_remove_row
 *
 * \par Purpose: Create a sub-table with a reduced number of columns.
 *
 * \retval -1 - System Error
 *         1 - Success.
 *
 * \param[in|out] table    The table
 * \param[in]     row_idx  The index of the row to remove.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *****************************************************************************/

int8_t    table_remove_row(table_t *table, uint32_t row_idx)
{
	AMP_DEBUG_ERR("table_make_subtable","Not implemented.", NULL);
	return -1;
}



/******************************************************************************
 *
 * \par Function Name: table_serialize
 *
 * \par Purpose: Generate full, serialized version of a Table.
 *               A serialized table is of the form:
 *
 * \par
 *  +-----------+-----------+--------+-------+     +-------+
 *  | Col Names | Col Types | # Rows | Row 1 |     | Row N |
 *  |   [DC]    |  [BLOB]   | [SDNV] | [DC]  | ... |  [DC] |
 *  +-----------+-----------+--------+-------+     +-------+
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized collection.
 *
 * \param[in]  table  The table to be serialized.
 * \param[out] size   The size of the resulting serialized table.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/15  E. Birrane     Initial implementation,
 *****************************************************************************/

uint8_t*  table_serialize(table_t *table, uint32_t *size)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	uint8_t *names_data = NULL;
	uint32_t names_len = 0;
	uint8_t *types_data = NULL;
	uint32_t types_len = 0;
	uint8_t **rows_data = NULL;
	uint32_t *rows_len = 0;
	int32_t num_rows = 0;
	uint32_t cur_row = 0;
	uint32_t tot_size = 0;
	Sdnv num_rows_sdnv;
	uint32_t i = 0;

	/* Step 0: Sanity Check. */
	if((table == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("table_serialize","Bad Args.", NULL);
		return NULL;
	}

	*size = 0;

	/* Step 1: Serialize column names. */
	if((names_data = dc_serialize(table->hdr.names, &names_len)) == NULL)
	{
		AMP_DEBUG_ERR("table_serialize","Can't serialize names.", NULL);
		return NULL;
	}
	tot_size += names_len;

	/* Step 2: Serialize column types. */
	if((types_data = blob_serialize(&(table->hdr.types), &types_len)) == NULL)
	{
		SRELEASE(names_data);
		AMP_DEBUG_ERR("table_serialize","Can't serialize types.", NULL);
		return NULL;
	}
	tot_size += types_len;

	/* Step 3 : Get number of rows. */
	if((num_rows = table_get_num_rows(table)) < 0)
	{
		SRELEASE(names_data);
		SRELEASE(types_data);
		AMP_DEBUG_ERR("table_serialize","Can't get num rows.", NULL);
		return NULL;
	}
	encodeSdnv(&num_rows_sdnv, num_rows);
	tot_size += num_rows_sdnv.length;

	/* Step 4: generate serialized version of each row, if any. */
	if(num_rows > 0)
	{
		LystElt elt = NULL;
		table_row_t *cur_row;
		uint32_t cur_idx = 0;

		/* Step 4.1: Allocate serialized space for each row. */
		if((rows_data = (uint8_t **) STAKE(num_rows * sizeof(uint8_t *))) == NULL)
		{
			SRELEASE(names_data);
			SRELEASE(types_data);
			AMP_DEBUG_ERR("table_serialize","Can't allocate space for %d rows.", num_rows);
			return NULL;
		}
		if((rows_len = (uint32_t *) STAKE(num_rows * sizeof(uint32_t))) == NULL)
		{
			SRELEASE(names_data);
			SRELEASE(types_data);
			SRELEASE(rows_data);
			AMP_DEBUG_ERR("table_serialize","Can't allocate space for %d rows.", num_rows);
			return NULL;

		}

		/* Step 4.2: Serialize each row. */
		for(elt = lyst_first(table->rows); elt; elt = lyst_next(elt))
		{
			if((cur_row = (table_row_t*) lyst_data(elt)) != NULL)
			{
				if((rows_data[cur_idx] = dc_serialize(cur_row->cels, &(rows_len[cur_idx]))) == NULL)
				{
					uint32_t i = 0;
					for(i = 0; i < cur_idx-1; i++)
					{
						SRELEASE(rows_data[i]);
					}
					SRELEASE(rows_data);
					SRELEASE(rows_len);
					SRELEASE(names_data);
					SRELEASE(types_data);
					AMP_DEBUG_ERR("table_serialize","Can't Serialize row %d.", cur_idx);
					return NULL;
				}
				tot_size += rows_len[cur_idx];
			}
			cur_idx++;
		}
	}


	/* Step 5 - Allocate buffer to hold serialized data. */
	if((result = (uint8_t *) STAKE(tot_size)) == NULL)
	{
		for(i = 0; i < num_rows; i++)
		{
			SRELEASE(rows_data[i]);
		}
		SRELEASE(rows_data);
		SRELEASE(rows_len);
		SRELEASE(names_data);
		SRELEASE(types_data);
		AMP_DEBUG_ERR("table_serialize","Can't allocate %d bytes.", tot_size);
		return NULL;
	}

	/* Step 6 - Populate result. */
	cursor = result;

	memcpy(cursor, names_data, names_len);
	SRELEASE(names_data);
	cursor += names_len;

	memcpy(cursor, types_data, types_len);
	SRELEASE(types_data);
	cursor += types_len;

	memcpy(cursor, num_rows_sdnv.text, num_rows_sdnv.length);
	cursor += num_rows_sdnv.length;

	for(i = 0; i < num_rows; i++)
	{
		memcpy(cursor, rows_data[i], rows_len[i]);
		SRELEASE(rows_data[i]);
		cursor += rows_len[i];
	}
	SRELEASE(rows_data);
	SRELEASE(rows_len);

	/* Step 7 - Sanity check result. */
	if((cursor - result) != tot_size)
	{
		AMP_DEBUG_ERR("table_serialize", "Sanity check failed. %d != %d.", (cursor-result), tot_size);
		SRELEASE(result);
		result = NULL;
	}

	*size = tot_size;

	/* Step 8: Return the serialized TDC. */
	return result;
}



