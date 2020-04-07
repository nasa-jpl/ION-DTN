/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file var.c
 **
 **
 ** Description: Structures that capture variable definitions.
 **
 ** Notes:
 **   These functions were originally part of the def.c package.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/05/16  E. Birrane     Initial implementation (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane     Renamed CD to VAR. (Secure DTN - NASA: NNX14CS58P)
 **  09/21/18  E. Birrane     Updated to AMP v5. Added EDDs. (JHU/APL)
 *****************************************************************************/

#include "platform.h"

#include "edd_var.h"
#include "../utils/utils.h"
#include "../utils/nm_types.h"


#include "../utils/db.h"

int       edd_cb_comp_fn(void *i1, void *i2)
{
	edd_t *e1 = (edd_t*) i1;
	edd_t *e2 = (edd_t*) i2;
	int success;

	CHKUSR(e1, -1);
	CHKUSR(e2, -1);

	/* Compare the base IDs. */

	if((success = ari_cb_comp_fn(e1->def.id, e2->def.id)) == 0)
	{
		/* Compare the parameters. */
		success = tnvc_compare(e1->parms, e2->parms);
	}

	return success;
}

void      edd_cb_del_fn(void *item)
{
	edd_release((edd_t*)item, 1);
}

void edd_cb_ht_del(rh_elt_t *elt)
{
	edd_release((edd_t*)elt->value, 1);
}

rh_idx_t  edd_cb_hash(void *table, void *key)
{
	edd_t *edd = (edd_t*)key;
	ari_t *tmp;
	rhht_t *ht = (rhht_t*) table;
	rh_idx_t result = ht->num_bkts;

	CHKUSR(edd, ht->num_bkts);

	tmp = ari_copy_ptr(edd->def.id);
	ari_replace_parms(tmp, edd->parms);
	result = ari_cb_hash(ht, tmp);
	ari_release(tmp, 1);

	return result;
}

edd_t* edd_create(ari_t *id, tnvc_t *parms, edd_collect_fn collect)
{
	edd_t *result = NULL;

	if((result = STAKE(sizeof(edd_t))) == NULL)
	{
		return NULL;
	}

	result->def.collect = collect;
	result->def.id = id;
	result->parms = parms;

	return result;
}

void edd_release(edd_t *edd, int destroy)
{
	CHKVOID(edd);

	ari_release(edd->def.id, 1);
	tnvc_release(edd->parms, 1);

	if(destroy)
	{
		SRELEASE(edd);
	}
}


int var_cb_comp_fn(void *i1, void *i2)
{
	var_t *v1 = (var_t*)i1;
	var_t *v2 = (var_t*)i2;
	int result;

	CHKUSR(v1, -1);
	CHKUSR(v2, -1);

	if((result = ari_cb_comp_fn(v1->id, v2->id)) == 0)
	{
		result = tnv_cb_comp(v1->value, v2->value);
	}

	return result;
}

void var_cb_del_fn(void *item)
{
	var_release((var_t*)item, 1);
}


rh_idx_t  var_cb_hash(void *table, void *key)
{
	var_t *var = (var_t*)key;
	rhht_t *ht = (rhht_t*) table;
	CHKUSR(var, ht->num_bkts);

	return ari_cb_hash(ht, var->id);
}

void      var_cb_ht_del_fn(rh_elt_t *elt)
{
	CHKVOID(elt);
	var_cb_del_fn(elt->value);
}

var_t*    var_copy_ptr(var_t *src)
{
	var_t *result = NULL;

	CHKNULL(src);

	result = (var_t *) STAKE(sizeof(var_t));
	CHKNULL(result);

	if((result->id = ari_copy_ptr(src->id)) == NULL)
	{
		SRELEASE(result);
		return NULL;
	}

	if((result->value = tnv_copy_ptr(src->value)) == NULL)
	{
		ari_release(result->id, 1);
		SRELEASE(result);
		return NULL;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: var_create_expr
 *
 * \par Create a variable definition from an initializing expression.
 *
 * \retval NULL Failure
 *        !NULL The new variable definition
 *
 * \param[in] ari   The identifier for the new VAR.
 * \param[in] type  The data type of the VAR value.
 * \param[in] expr  The initializing expression
 *
 * \par Notes:
 *		1. The ID is SHALLOW COPIED into this object and
 *		   MUST NOT be released by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *  09/30/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

var_t* var_create(ari_t *id, amp_type_e type, expr_t *expr)
{
	var_t *result = NULL;
	tnv_t val;

	if((id == NULL) || (expr == NULL))
	{
		return NULL;
	}

	result = STAKE(sizeof(var_t));
	CHKNULL(result);

	result->id = id;

	/* If the variable is an expression, store the expression. */
	if(type == AMP_TYPE_EXPR)
	{
		if((result->value = tnv_create()) == NULL)
		{
			expr_release(expr, 1);
			ari_release(id, 1);
			SRELEASE(result);
			result = NULL;
		}
		else
		{
			result->value->type = type;
			TNV_SET_ALLOC(result->value->flags);
			result->value->value.as_ptr = expr;
		}
	}
	/* If the var/expr types agree, calculate value. */
	else if(type == expr->type)
	{
		result->value = expr_eval(expr);
		expr_release(expr, 1);
	}
	/* If var/expr types differ, calc value and then cast. */
	else
	{
		tnv_t *tmp = expr_eval(expr);
		expr_release(expr, 1);

		result->value = NULL;
		if(tmp != NULL)
		{
			if(tmp->type != type){
				result->value = tnv_cast(tmp, type);
				tnv_release(tmp, 1);
			}
			else
			{
				result->value = tmp;
			}
		}

		if(result->value == NULL)
		{
			ari_release(id, 1);
			SRELEASE(result);
			result = NULL;
		}
	}

	return result;
}


var_t* var_create_from_def(var_def_t def)
{
	return var_create(def.id, def.type, def.expr);
}

var_t* var_create_from_tnv(ari_t *id, tnv_t val)
{
	var_t *result = NULL;

	CHKNULL(id);
	result = STAKE(sizeof(var_t));
	CHKNULL(result);

	result->id = id;
	result->value = tnv_copy_ptr(&val);

	return result;


}

/******************************************************************************
 *
 * \par Function Name: var_deserialize
 *
 * \par Construct a VAR from a serialized byte stream.
 *
 * \retval NULL Failure
 *        !NULL The new variable definition
 *
 * \param[in] cursor       The start of the byte stream
 * \param[in] size         The length of the byte stream in bytes
 * \param[out] bytes_used  Number of bytes read from the stream.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *  09/25/18  E. Birrane     Updated to CBOR and AMp v5. (JHU/APL)
 *****************************************************************************/

var_t *var_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	var_t *result = NULL;

    AMP_DEBUG_ENTRY("var_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

    CHKNULL(success);
    *success = AMP_FAIL;

    CHKNULL(it);

    result = STAKE(sizeof(var_t));
    CHKNULL(result);


    /* Grab the var ARI. */
#if AMP_VERSION < 7
    blob_t *tmp = blob_deserialize_ptr(it, success);
    result->id = ari_deserialize_raw(tmp, success);
    blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(it);
	result->id = ari_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif
    if((result->id == NULL) || (*success != AMP_OK))
    {
    	SRELEASE(result);
    	return NULL;
    }
	
    /* Grab the TNV. */
#if AMP_VERSION < 7
    tmp = blob_deserialize_ptr(it, success);
    result->value = tnv_deserialize_raw(tmp, success);
    blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(it);
	result->value = tnv_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif

    if((result->value == NULL) || (*success != AMP_OK))
    {
    	*success = AMP_FAIL;
    	var_release(result, 1);
    	return NULL;
    }

    return result;
}

var_t* var_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;

	if((data == NULL) || (success == NULL))
	{
		return NULL;
	}
	*success = AMP_FAIL;

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	var_t *tmp = var_deserialize_ptr(&it, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}

/******************************************************************************
 *
 * \par Function Name: var_release
 *
 * \par Releases memory associated with a VAR Object
 *
 *
 * \param[in|out]    var      The VAR object being freed.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *  09/25/18  E. Birrane     Updated to CBOR and AMp v5. (JHU/APL)
 *****************************************************************************/

void var_release(var_t *var, int destroy)
{
	CHKVOID(var);

	ari_release(var->id, 1);
	tnv_release(var->value, 1);

	if(destroy)
	{
		SRELEASE(var);
	}
}



/******************************************************************************
 *
 * \par Function Name: var_serialize
 *
 * \par Serializes a VAR object to a byte stream.
 *
 * \retval NULL Failure
 *        !NULL The serialized VAR object as a byte stream
 *
 * \param[in]  var   The VAR object to represent.
 * \param[out] len  The length of the byte stream
 *
 * \par Notes:
 *   1. The byte stream is allocated on the heap and MUST be released by the
 *      calling function.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation
 *  09/25/18  E. Birrane     Updated to CBOR and AMp v5. (JHU/APL)
 *****************************************************************************/

int var_serialize(QCBOREncodeContext *encoder, void *item)
{
	int err;
#if AMP_VERSION < 7
	blob_t *result;
#endif
	var_t *var = (var_t*) item;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(var, AMP_FAIL);

	/* Step 1: Encode the ARI. */
#if AMP_VERSION < 7
	result = ari_serialize_wrapper(var->id);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = ari_serialize(encoder, var->id);
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("var_serialize","CBOR Error: %d", err);
		return err;
	}

	/* Step 2: Encode the value. */
#if AMP_VERSION < 7
	result = tnv_serialize_wrapper(var->value);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = tnv_serialize(encoder, var->value);
	QCBOREncode_CloseArrayOctet(encoder);
#endif

	return err;
}

blob_t*   var_serialize_wrapper(var_t *var)
{
	return cut_serialize_wrapper(VAR_DEFAULT_ENC_SIZE, var, (cut_enc_fn)var_serialize);
}



var_def_t  vardef_deserialize(QCBORDecodeContext *it, int *success)
{
	var_def_t result;

	AMP_DEBUG_ENTRY("vardef_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);
	memset(&result,0,sizeof(var_def_t));
	result.type = AMP_TYPE_UNK;

	CHKUSR(success, result);
	*success = AMP_FAIL;

    CHKUSR(it, result);

    /* Grab the Id. */
    result.id = ari_deserialize_ptr(it, success);
    if((result.id == NULL) || (*success != AMP_OK))
    {
    	return result;
    }

    /* Grab the expression. */
    result.expr = expr_deserialize_ptr(it, success);
    if((result.expr == NULL) || (*success != AMP_OK))
    {
    	*success = AMP_FAIL;
    	ari_release(result.id, 1);
    	result.type = AMP_TYPE_UNK;
    	return result;
    }

    *success = AMP_OK;

    return result;
}



void vardef_release(var_def_t *def, int destroy)
{
	CHKVOID(def);
	ari_release(def->id, 1);
	expr_release(def->expr, 1);

	if(destroy)
	{
		SRELEASE(def);
	}
}



int  vardef_serialize(QCBOREncodeContext *encoder, void *item)
{
	blob_t *result;
	int success, err;;
	var_def_t *def = (var_def_t*)item;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(def, AMP_FAIL);

	/* Step 1: Encode the ARI. */
	result = ari_serialize_wrapper(def->id);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);

	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("vardef_serialize","CBOR Error: %d", err);
		return err;
	}

	/* Step 2: Encode the type. */
	err = cut_enc_byte(encoder, def->type);
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("vardef_serialize","CBOR Error: %d", err);
		return err;
	}

	/* Step 3: Encode the expression. */
	result = expr_serialize_wrapper(def->expr);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);

	return err;
}


blob_t*    vardef_serialize_wrapper(var_def_t *def)
{
	return cut_serialize_wrapper(VARDEF_DEFAULT_ENC_SIZE, def, (cut_enc_fn)vardef_serialize);
}

