/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
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

	if(tblt_check_row(VDB_FINDKEY_TBLT(tbl->id), row) != AMP_OK)
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

	if((result = (tbl_t*) STAKE(sizeof(tbl_t))) == NULL)
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


void* tbl_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	tbl_t *result = NULL;
	size_t i;
	size_t len;
	ari_t *tpl_id;
	QCBORError err;
	QCBORItem item;

	AMP_DEBUG_ENTRY("tbl_deserialize_ptr",
					"("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					(uaddr)it, (uaddr)success);

	/* Sanity Checks. */
	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);

	/*
	 * Step 1: Determine how many elements are in the array.
	 * First element is an ARI. Rest are rows.
	 */
	err = QCBORDecode_GetNext(it, &item);
	if ( err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
		return NULL;
	}
	len = item.val.uCount;

	/* Step 2: grab the Id. */
#if AMP_VERSION < 7
	blob_t *blob = blob_deserialize_ptr(it, success);
	tpl_id = ari_deserialize_raw(blob, success);
	blob_release(blob, 1);
#else
	QCBORDecode_StartOctets(it);
	tpl_id = ari_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif
	
	if((tpl_id == NULL) || (*success != AMP_OK))
	{
		return NULL;
	}

	/* Step 3: Create the table. */
	result = tbl_create(tpl_id);
	ari_release(tpl_id, 1);
	tpl_id = NULL;

	if(result == NULL)
	{
		return NULL;
	}

	/*
	 * Step 4: Grab the rows. If any. Start at 1 for counting
	 * since we already grabbed the ARI from the array.
	 */
	for(i = 1; i < len; i++)
	{
		tnvc_t *cur_row = NULL;
#if AMP_VERSION < 7
		blob_t bytestr = blob_deserialize(it, success);

		if(*success != AMP_OK)
		{
			AMP_DEBUG_ERR("tbl_deserialize_ptr","Can't get bytestr.", NULL);
			break;
		}

		cur_row = tnvc_deserialize_ptr_raw(&bytestr, success);
		blob_release(&bytestr, 0);
#else
		QCBORDecode_StartOctets(it);
		cur_row = tnvc_deserialize_ptr(it, success);
		QCBORDecode_EndOctets(it);
#endif
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
	QCBORDecodeContext it;

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(data);

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	tbl_t *rtv = tbl_deserialize_ptr(&it, success);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return rtv;
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

int tbl_serialize(QCBOREncodeContext *encoder, void *item)
{
	blob_t *result;
	int err = AMP_OK;
	size_t num;
	size_t i;
	tbl_t *tbl = (tbl_t *) item;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(tbl, AMP_FAIL);

	num = tbl_num_rows(tbl) + 1;

	/* Start a container. */
	QCBOREncode_OpenArray(encoder);

	/* Step 1: Encode the ARI. */
#if AMP_VERSION < 7
	result = ari_serialize_wrapper(tbl->id);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = ari_serialize(encoder, tbl->id);
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	if(err != AMP_OK)
	{
		QCBOREncode_CloseArray(encoder);
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
#if AMP_VERSION < 7
		result = tnvc_serialize_wrapper(cur_row);
		err = blob_serialize(encoder, result);
		blob_release(result, 1);
#else
		QCBOREncode_OpenArray(encoder);
		err = tnvc_serialize(encoder, cur_row);
		QCBOREncode_CloseArrayOctet(encoder);
#endif
		if(err != AMP_OK)
		{
			QCBOREncode_CloseArray(encoder);
			return err;
		}
	}

	QCBOREncode_CloseArray(encoder);
	return err;
}

blob_t*   tbl_serialize_wrapper(tbl_t *tbl)
{
	return cut_serialize_wrapper(TBL_DEFAULT_ENC_SIZE, tbl, (cut_enc_fn)tbl_serialize);
}

void tbl_cb_del_fn(void *item)
{
	tbl_release((tbl_t*) item, 1);
}
int tbl_cb_comp_fn(void *i1, void *i2)
{
	tbl_t* t1 = (tbl_t*)i1;
	tbl_t* t2 = (tbl_t*)i2;

	CHKUSR(t1, -1);
	CHKUSR(t2, -1);

	return ari_compare(t1->id, t2->id, 0);
}
void* tbl_cb_copy_fn(void *item)
{
	return tbl_copy_ptr((tbl_t*)item);
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

	return ari_compare(t1->id, t2->id, 0);
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
	}
	return new_col;
}

