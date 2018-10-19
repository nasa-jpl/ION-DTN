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
 **  09/30/18  E. Birrane    Updated to AMP v0.5. (JHU/APL)
 *****************************************************************************/

#include "platform.h"

#include "ion.h"
#include "../utils/utils.h"

#include "../primitives/table.h"
#include "tnv.h"


/******************************************************************************
 *
 * \par Function Name: tbl_add_row
 *
 * \par Purpose: Adds a row to an existing table.
 *
 * \return AMP Status Code
 *
 * \param[in|out] tbl  The table having a row added
 * \param[in]     row  The row being added
 *
 * \par Notes:
 *  - This function SHALLOW-COPIES the row into the table. The calling function
 *     MUST NOT release this memory.
 *  - Each entry in the row MUST have type information. Tables are strongly
 *    typed in this implementation, even if type information isn't in the
 *    serialized rows in the AMP protocol.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/15/16  E. Birrane     Initial implementation
 *  09/30/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int tbl_add_row(tbl_t *tbl, tnvc_t *row)
{
	CHKUSR(tbl, AMP_FAIL);
	CHKUSR(row, AMP_FAIL);

	if(tblt_check_row(VDB_FINDKEY_TBLT(&(tbl->id)), row) != AMP_OK)
	{
		AMP_DEBUG_ERR("tbl_add_row", "row doesn't match template.", NULL);
		return AMP_FAIL;
	}

	return vec_push(&(tbl->rows), row);
}

void     tbl_clear(tbl_t *tbl)
{
	CHKVOID(tbl);
	vec_clear(&(tbl->rows));
}


tbl_t* tbl_copy_ptr(tbl_t *tbl)
{
	tbl_t *result = NULL;
	int success;

	CHKNULL(tbl);

	if((result = (tbl_t*) STAKE(sizeof(tbl_t))) == NULL)
	{
		return NULL;
	}

	if((result->id = ari_copy_ptr(tbl->id)) == NULL)
	{
		SRELEASE(result);
		return NULL;
	}

	result->rows = vec_copy(&(tbl->rows), &success);
	if(success != AMP_OK)
	{
		ari_release(result->id,1);
		SRELEASE(result);
		result = NULL;
	}

	return result;
}

tbl_t*   tbl_create(ari_t *id)
{
	tbl_t *result = NULL;
	int success;

	CHKNULL(id);

	if((result = (tbl_t*) STAKE(sizeof(tbl_t*))) == NULL)
	{
		return NULL;
	}

	if((result->id = ari_copy_ptr(id)) == NULL)
	{
		SRELEASE(result);
		return NULL;
	}

	result->rows = vec_create(0, tnvc_cb_del, tnvc_cb_comp, tnvc_cb_copy_fn, 0, &success);

	if(success != AMP_OK)
	{
		tbl_release(result, 1);
		result = NULL;
	}

	return result;
}


tbl_t* tbl_deserialize_ptr(CborValue *it, int *success)
{
	tbl_t *result = NULL;
	size_t i;
	size_t len;
	ari_t *tpl_id;
	tnvc_t entries;
	CborError err;
	CborValue array_it;

	AMP_DEBUG_ENTRY("tbl_deserialize_ptr",
			        "("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					(uaddr)it, (uaddr)success);

	/* Sanity Checks. */
	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);
	CHKNULL(cbor_value_is_array(it));

	/*
	 * Step 1: Determine how many elements are in the array.
	 * First element is an ARI. Rest are rows.
	 */
	if(cbor_value_get_array_length(it, &len) != CborNoError)
	{
		return NULL;
	}

	if(cbor_value_enter_container(it, &array_it) != CborNoError)
	{
		return NULL;
	}

	/* Step 2: grab the Id. */
	tpl_id = ari_deserialize_ptr(&array_it, success);

	if((tpl_id == NULL) || (*success != AMP_OK))
	{
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	/*
	 * Step 3: Grab the rows. If any. Start at 1 for counting
	 * since we already grabbed the ARI from the array.
	 */
	for(i = 1; i < len; i++)
	{
		blob_t bytestr = blob_deserialize(&array_it, success);
		tnvc_t *cur_row = NULL;

		if(*success != AMP_OK)
		{
			AMP_DEBUG_ERR("tbl_deserialize_ptr","Can't get bytestr.", NULL);
			break;
		}

		cur_row = tnvc_deserialize_ptr_raw(&bytestr, success);
		blob_release(&bytestr, 0);

		if((cur_row == NULL) || (*success != AMP_OK))
		{
			AMP_DEBUG_ERR("tbl_deserialize_ptr","Can't get row.", NULL);
			break;
		}

		if((*success = tbl_add_row(result, cur_row)) != AMP_OK)
		{
			AMP_DEBUG_ERR("tbl_deserialize_ptr","Can't add row.", NULL);

			tnvc_release(cur_row,1);
			break;
		}
	}

	if(*success != AMP_OK)
	{
		tbl_release(result, 1);
		result = NULL;
	}

	return result;
}


tbl_t* tbl_deserialize_raw(blob_t *data, int *success)
{
	CborParser parser;
	CborValue it;

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(data);

	if(cbor_parser_init(data->value, data->length, 0, &parser, &it) != CborNoError)
	{
		return NULL;
	}

	return tbl_deserialize_ptr(&it, success);
}



tnvc_t*  tbl_get_row(tbl_t *tbl, int row_idx)
{
	tnvc_t *result = NULL;
	CHKNULL(tbl);
	result = (tnvc_t*) vec_at(&tbl->rows, row_idx);
	return result;
}


void tbl_release(tbl_t *tbl, int destroy)
{
	CHKVOID(tbl);
	ari_release(tbl->id, 1);
	vec_release(&(tbl->rows), 0);

	if(destroy)
	{
		SRELEASE(tbl);
	}
}

int tbl_num_rows(tbl_t *tbl)
{
	CHKZERO(tbl);
	return vec_num_entries(tbl->rows);
}

CborError tbl_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	CborEncoder array_enc;
	blob_t *result;
	int success;
	size_t num;
	size_t i;
	tbl_t *tbl = (tbl_t *) item;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(tbl, CborErrorIO);

	num = tbl_num_rows(tbl) + 1;

	/* Start a container. */
	err = cbor_encoder_create_array(encoder, &array_enc, num);

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tbl_serialize","CBOR Error: %d", err);
		return err;
	}

	/* Step 1: Encode the ARI. */
	result = ari_serialize_wrapper(tbl->id);
	err = blob_serialize(&array_enc, result);
	blob_release(result, 1);

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}

	/*
	 * For each row. We start counting at 1 because we already
	 * accounted for the ARI ID in the array.
	 *
	 * We don't need to do lots of NULL checks here because the
	 * functions called check for NULL themselves.
	 */
	for(i = 1; i < num; i++)
	{
		tnvc_t *cur_row = tbl_get_row(tbl, i-1);
		result = tnvc_serialize_wrapper(cur_row);
		err = blob_serialize(&array_enc, result);
		blob_release(result, 1);

		if((err != CborNoError) && (err != CborErrorOutOfMemory))
		{
			cbor_encoder_close_container(encoder, &array_enc);
			return err;
		}
	}

	cbor_encoder_close_container(encoder, &array_enc);
	return err;
}

blob_t*   tbl_serialize_wrapper(tbl_t *tbl)
{
	return cut_serialize_wrapper(TBL_DEFAULT_ENC_SIZE, tbl, tbl_serialize);
}


/* TABLE TEMPLATE FUNCTIONS */



/******************************************************************************
 *
 * \par Function Name: tblt_add_col
 *
 * \par Purpose: Adds a col to a table template.
 *
 * \return AMP Status Code
 *
 * \param[in|out]   table    The table having a column added
 * \param[in]       name     The column name being added
 * \param[in]       type     The data type of the column
 *
 * \par Notes:
 *  - This function DEEP-COPIES the col name into the table.
 *  - Columns are appended to end of list of columns.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/06/16  E. Birrane     Initial implementation
 *  09/30/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int tblt_add_col(tblt_t *tblt, amp_type_e type, char *name)
{
	tblt_col_t *new_col = NULL;

	CHKUSR(tblt, AMP_FAIL);
	CHKUSR(type_is_known(type), AMP_FAIL);

	if((new_col = STAKE(sizeof(tblt_col_t))) == NULL)
	{
		return AMP_SYSERR;
	}

	new_col->type = type;
	if(name != NULL)
	{
		if((new_col->name = (char*) STAKE(strlen(name)+2)) == NULL)
		{
			SRELEASE(new_col);
			return AMP_SYSERR;
		}
		memcpy(new_col->name, name, strlen(name) + 1);
	}
	else
	{
		new_col->name = NULL;
	}

	if(vec_push(&(tblt->cols), new_col) != VEC_OK)
	{
		SRELEASE(new_col->name);
		SRELEASE(new_col);
		return AMP_FAIL;
	}

	return AMP_OK;
}


void tblt_cb_del_fn(void *item)
{
	tblt_release((tblt_t*) item, 1);
}

int tblt_cb_comp_fn(void *i1, void *i2)
{
	tblt_t* t1 = (tblt_t*)i1;
	tblt_t* t2 = (tblt_t*)i2;

	CHKUSR(t1, -1);
	CHKUSR(t2, -1);

	return ari_compare(t1->id, t2->id);
}

void* tblt_cb_copy_fn(void *item)
{
	return tblt_copy_ptr((tblt_t*)item);
}

void tblt_cb_ht_del_fn(rh_elt_t *elt)
{
	if(elt == NULL)
	{
		return;
	}
	tblt_cb_del_fn(elt->value);
}

int tblt_check_row(tblt_t *tblt, tnvc_t *row)
{
	size_t i;
	size_t max;
	CHKZERO(tblt);
	CHKZERO(row);

	max = vec_num_entries(tblt->cols);
	if(tnvc_size(row) != max)
	{
		return 0;
	}

	for(i = 0; i < max; i++)
	{
		tnv_t *tmp = tnvc_get(row, i);
		if(tmp->type != tblt_get_type(tblt, i))
		{
			return 0;
		}
	}

	return 1;
}

tblt_t* tblt_copy_ptr(tblt_t *tblt)
{
	tblt_t *result = NULL;
	int success;

	CHKNULL(tblt);

	if((result = (tblt_t*) STAKE(sizeof(tblt_t))) == NULL)
	{
		return NULL;
	}

	if((result->id = ari_copy_ptr(tblt->id)) == NULL)
	{
		SRELEASE(result);
		return NULL;
	}

	result->cols = vec_copy(&(tblt->cols), &success);
	if(success != AMP_OK)
	{
		ari_release(result->id,1);
		SRELEASE(result);
		result = NULL;
	}

	return result;
}


// Shallow copy.
tblt_t* tblt_create(ari_t *id, 	tblt_build_fn build)
{
	tblt_t *result = NULL;
	int success;

	if(id == NULL)
	{
		return NULL;
	}

	if((result = (tblt_t*) STAKE(sizeof(tblt_t))) == NULL)
	{
		return NULL;
	}

	result->id = id;
	result->cols = vec_create(0, tblt_col_cb_del_fn, NULL, tblt_col_cb_copy_fn, 0, &success);
	result->build = build;

	if(success != VEC_OK)
	{
		tblt_release(result, 1);
		result = NULL;
	}

	return result;
}

amp_type_e tblt_get_type(tblt_t *tblt, int idx)
{
	tblt_col_t *col = NULL;

	CHKUSR(tblt, AMP_TYPE_UNK);

	col = vec_at(&tblt->cols, idx);
	if(col == NULL)
	{
		return AMP_TYPE_UNK;
	}

	return col->type;
}

/*
vector_t tblt_deserialize_names(CborValue *it, size_t num, int *success)
{
	CborValue names_it;
	CborError err;
	vector_t result;
	size_t num_names;
	size_t i;

	if((*success = cut_enter_array(it, num, num, &num_names, &names_it)) != AMP_OK)
	{
		return result;
	}

	result = vec_create(num_names, NULL, NULL, NULL, 0, success);
	if(*success != AMP_OK)
	{
		cbor_value_leave_container(it, &names_it);
		return result;
	}

	for(i = 0; i < num_names; i++)
	{
		char *cur_name = NULL;
		size_t cur_len;

		*success = AMP_FAIL;
		if((err = cbor_value_calculate_string_length(&names_it, &cur_len)) != CborNoError)
		{
			break;
		}

		if((cur_name = STAKE(cur_len + 1)) == NULL)
		{
			break;
		}

		if((err = cbor_value_copy_text_string(&names_it, cur_name, &cur_len, &names_it)) != CborNoError)
		{
			break;
		}

		if(vec_push(&result, cur_name) != VEC_OK)
		{
			SRELEASE(cur_name);
			break;
		}
		*success = AMP_OK;
	}

	cbor_value_leave_container(it, &names_it);

	if(*success != AMP_OK)
	{
		vec_release(&result, 0);
	}

	return result;
}

tblt_t* tblt_deserialize_ptr(CborValue *it, int *success)
{
	tblt_t *result = NULL;
	size_t i;
	size_t len;
	size_t num_col;
	ari_t *id;
	blob_t *types;

	CborError err;
	CborValue array_it;

	AMP_DEBUG_ENTRY("tblt_deserialize_ptr",
			        "("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					(uaddr)it, (uaddr)success);
	*success = AMP_FAIL;

	/ * Step 1: Make sure array is correct and enter array. * /
	if((*success = cut_enter_array(it, 2, 3, &len, &array_it)) != AMP_OK)
	{
		AMP_DEBUG_ERR("tblt_deserialize_ptr", "Bad Array.", NULL);
		return NULL;
	}


	/ * Step 2: grab the template Id. * /
	id = ari_deserialize_ptr(&array_it, success);
	if((id == NULL) || (*success != AMP_OK))
	{
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	/ * Step 3: Create the table template object. * /
	result = tblt_create(id);

	if(result == NULL)
	{
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	/ * Step 4: Grab the types for the columns. * /
	types = blob_deserialize_ptr(&array_it, success);
	if((id == NULL) || (*success != AMP_OK))
	{
		tblt_release(result, 1);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}
	num_col = types->length;

	/ * Step 5: Create columns and populate with types.  * /
	for(i = 0; i < num_col; i++)
	{
		if(tblt_add_col(result, types->value[i], NULL) != AMP_OK)
		{
			blob_release(types, 1);
			tblt_release(result, 1);
			cbor_value_leave_container(it, &array_it);
			return NULL;
		}
	}
	blob_release(types, 1);

	/ * Step 6: If we have names, too, we need to add them. * /
	if(len == 3)
	{
		vector_t names = tblt_deserialize_names(&array_it, num_col, success);
		if(*success != AMP_OK)
		{
			tblt_release(result, 1);
			cbor_value_leave_container(it, &array_it);
			return NULL;
		}

		for(i = 0; i < num_col; i++)
		{
			tblt_col_t *cur_col = vec_at(&result->cols, i);
			if(cur_col != NULL)
			{
				cur_col->name = (char*) vec_at(&names, i);
			}
		}

		/ *
		 * There is no delete function on thi vector because we copied ptrs
		 * over to the tblt columns.
		 * /
		vec_release(&names, 0);
	}

	return result;
}



tblt_t* tblt_deserialize_raw(blob_t *data, int *success)
{
	CborParser parser;
	CborValue it;

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(data);

	if(cbor_parser_init(data->value, data->length, 0, &parser, &it) != CborNoError)
	{
		return NULL;
	}

	return tblt_deserialize_ptr(&it, success);
}
*/

int tblt_num_cols(tblt_t *tblt)
{
	CHKZERO(tblt);
	return vec_num_entries(tblt->cols);
}

void tblt_release(tblt_t *tblt, int destroy)
{
	if(tblt == NULL)
	{
		return;
	}

	ari_release(tblt->id, 1);
	vec_release(&(tblt->cols), 0);

	if(destroy)
	{
		SRELEASE(tblt);
	}
}


/*
/ *
 * We don'y serialize names in this implementation.
 * /
CborError tblt_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	CborEncoder array_enc;
	blob_t *result;
	int success;
	size_t num;
	tblt_t *tblt = (tblt_t *) item;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(tblt, CborErrorIO);

	/ * Start a container. * /
	err = cbor_encoder_create_array(encoder, &array_enc, 2);

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("tblt_serialize","CBOR Error: %d", err);
		return err;
	}

	/ * Step 1: Encode the ARI. * /
	result = ari_serialize_wrapper(tblt->id);
	err = blob_serialize(&array_enc, result);
	blob_release(result, 1);

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}


	/ * Step 2: Encode the column types. * /
	result = tblt_serialize_types(tblt);
	err = blob_serialize(&array_enc, result);
	blob_release(result, 1);

	cbor_encoder_close_container(encoder, &array_enc);
	return err;
}


blob_t* tblt_serialize_types(tblt_t* tblt)
{
	blob_t *result = NULL;
	int i;

	result = blob_create(NULL, 0, tblt_num_cols(tblt));
	CHKNULL(result);

	for(i = 0; i < tblt_num_cols(tblt); i++)
	{
		result->value[i] = tblt_get_type(tblt, i);
	}

	return result;
}

blob_t*   tblt_serialize_wrapper(tblt_t *tblt)
{
	return cut_serialize_wrapper(TBLT_DEFAULT_ENC_SIZE, tblt, tblt_serialize);
}
*/

void tblt_col_cb_del_fn(void *item)
{
	tblt_col_t *col = (tblt_col_t*)item;

	if(col == NULL)
	{
		return;
	}

	SRELEASE(col->name);
	SRELEASE(col);
}


void*  tblt_col_cb_copy_fn(void *item)
{
	tblt_col_t *col = (tblt_col_t*)item;
	tblt_col_t *new_col = NULL;

	if((new_col = STAKE(sizeof(tblt_col_t))) != NULL)
	{
		new_col->type = col->type;
		new_col->name = NULL;
		if(col->name != NULL)
		{
			if((col->name = STAKE(strlen(col->name)+1)) == NULL)
			{
				SRELEASE(new_col);
				return NULL;
			}
			memcpy(col->name, col->name, strlen(col->name));
		}
	}
	return new_col;
}




