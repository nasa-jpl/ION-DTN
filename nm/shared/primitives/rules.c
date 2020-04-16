/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file rules.c
 **
 **
 ** Description:
 **
 **\todo Add more checks for overbounds reads on deserialization.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/04/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  07/05/16  E. Birrane     Fixed time offsets when creating TRL & SRL (Secure DTN - NASA: NNX14CS58P)
 **  09/29/18  E. Birrane     Updated to AMPv0.5 (JHU/APL)
 *****************************************************************************/
#include "platform.h"

#include "rules.h"

#include "../utils/utils.h"


int rule_cb_comp_fn(void *i1, void *i2)
{
	rule_t *r1 = (rule_t*)i1;
	rule_t *r2 = (rule_t*)i2;

	CHKUSR(r1, -1);
	CHKUSR(r2, -1);

	return ari_cb_comp_fn(&(r1->id), &(r2->id));
}

void rule_cb_del_fn(void *item)
{
	rule_release((rule_t*)item, 1);
}

void rule_cb_ht_del_fn(rh_elt_t *elt)
{
	CHKVOID(elt);
	rule_cb_del_fn(elt->value);
}

/* 9/29/2018 */
rule_t*   rule_copy_ptr(rule_t *src)
{
	rule_t *result = NULL;
	int success;

	CHKNULL(src);

	if((result = (rule_t *) STAKE(sizeof(rule_t))) == NULL)
	{
		AMP_DEBUG_ERR("rule_copy_ptr","Can't Alloc %d bytes.", sizeof(rule_t));
		return NULL;
	}

	/* Deep copy the hard things, checking as we go. */
	result->id = ari_copy(src->id, &success);
	if(success != AMP_OK)
	{
		SRELEASE(result);
		return NULL;
	}
	// TODO: Check return value for these copies.
	result->action = ac_copy(&(src->action));

	if(result->id.type == AMP_TYPE_SBR)
	{
		//TODO: Check return value for these copies.
		result->def.as_sbr.expr = expr_copy(src->def.as_sbr.expr);
		result->def.as_sbr.max_fire = src->def.as_sbr.max_fire;
		result->def.as_sbr.max_eval = src->def.as_sbr.max_eval;
	}
	else
	{
		result->def.as_tbr.period = src->def.as_tbr.period;
		result->def.as_tbr.max_fire = src->def.as_tbr.max_fire;
	}

	/* Shallow copy the easy things. */
	result->ticks_left = src->ticks_left;
	result->flags = src->flags;
	result->num_eval = src->num_eval;
	result->num_fire = src->num_fire;
	result->start = src->start;

	return result;
}


rule_t*  rule_create_sbr(ari_t id, uvast start, sbr_def_t def, ac_t action)
{
	rule_t *result = NULL;
	int success;

	/* Step 1: Allocate the message. */
	if((result = (rule_t*) STAKE(sizeof(rule_t))) == NULL)
	{
		AMP_DEBUG_ERR("rule_create_sbr","Can't alloc %d bytes.", sizeof(rule_t));
		AMP_DEBUG_EXIT("rule_create_sbr","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->id = ari_copy(id, &success);
	result->start = start;
	result->action = action;

	result->def.as_sbr = def;

	RULE_SET_ACTIVE(result->flags);

	if(start < AMP_RELATIVE_TIME_EPOCH)
	{
		result->ticks_left = start;
	}
	else
	{
		result->ticks_left = (start - getCtime());
	}


	AMP_DEBUG_EXIT("rule_create_sbr", ADDR_FIELDSPEC, (uaddr) result);

	return result;
}

rule_t*  rule_create_tbr(ari_t id, uvast start, tbr_def_t def, ac_t action)
{
	rule_t *result = NULL;
	int success;

	/* Step 1: Allocate the message. */
	if((result = (rule_t*) STAKE(sizeof(rule_t))) == NULL)
	{
		AMP_DEBUG_ERR("rule_create_tbr","Can't alloc %d bytes.", sizeof(rule_t));
		AMP_DEBUG_EXIT("rule_create_tbr","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->id = ari_copy(id, &success);
	if(success != AMP_OK)
	{
		AMP_DEBUG_ERR("rule_create_tbr", "Can't copy ID.", NULL);
		SRELEASE(result);
		return NULL;
	}

	result->start = start;
	result->action = action;

	result->def.as_tbr = def;

	RULE_SET_ACTIVE(result->flags);

	if(start < AMP_RELATIVE_TIME_EPOCH)
	{
		result->ticks_left = start + def.period;
	}
	else
	{
		result->ticks_left = (start - getCtime()) + def.period;
	}


	AMP_DEBUG_EXIT("rule_create_tbr", ADDR_FIELDSPEC, (uaddr) result);

	return result;
}



rule_t*  rule_deserialize_helper(QCBORDecodeContext *array_it, int *success)
{
	rule_t *result = NULL;
	ari_t *id;
	uvast start;
	ac_t action;
	sbr_def_t as_sbr;
	tbr_def_t as_tbr;

#if AMP_VERSION < 7
	blob_t *tmp = blob_deserialize_ptr(array_it, success);
	id = ari_deserialize_raw(tmp, success);
	blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(array_it);
	id = ari_deserialize_ptr(array_it, success);
	QCBORDecode_EndOctets(array_it);
#endif
	
	if(*success != AMP_OK)
	{
		return result;
	}

	if((*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &start)) != AMP_OK)
	{
		ari_release(id, 1);
		return result;
	}

	if(id->type == AMP_TYPE_SBR)
	{
		as_sbr = sbrdef_deserialize(array_it, success);
	}
	else
	{
		as_tbr = tbrdef_deserialize(array_it, success);
	}

#if AMP_VERSION < 7
	tmp = blob_deserialize_ptr(array_it, success);
	action = ac_deserialize_raw(tmp, success);
	blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(array_it);
	action = ac_deserialize(array_it, success);
	QCBORDecode_EndOctets(array_it);
#endif
	
	if(*success != AMP_OK)
	{
		ari_release(id, 1);
		return result;
	}

	if(id->type == AMP_TYPE_SBR)
	{
		result = rule_create_sbr(*id, start, as_sbr, action);
	}
	else
	{
		result = rule_create_tbr(*id, start, as_tbr, action);
	}

	if(result == NULL)
	{
		if(id->type == AMP_TYPE_SBR)
		{
			expr_release(&(as_sbr.expr), 0);
		}

		ari_release(id, 1);
		ac_release(&action, 0);
		SRELEASE(result);
		*success = AMP_FAIL;
		return NULL;
	}
	else
	{
		/* Release ari container only */
		ari_release(id, 1);
	}

	*success = AMP_OK;
	return result;
}


rule_t*  rule_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	QCBORItem item;
	rule_t *result = NULL;
	QCBORError err;

	AMP_DEBUG_ENTRY("rule_deserialize_ptr","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
						(uaddr)it, (uaddr)success);

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);

	/*
	 * Make sure we are in a container. All rules are captured as
	 * arrays.
	 */
#if AMP_VERSION < 7
	err = QCBORDecode_GetNext(it, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
		return NULL;
	}
	else if (item.val.uCount < 5 || item.val.uCount > 6)
	{
		/*
		 * See how many items are in the container. Should be 5 for a TBR
		 * and 6 for an SBR.
		 */
		AMP_DEBUG_ERR("rule_deserialize_ptr","Bad array length. Len %d",
						   item.val.uCount);
		return NULL;
	}
#else // virtual array (octets)
	QCBORDecode_StartOctets(it);
#endif
	
	result = rule_deserialize_helper(it, success);

#if AMP_VERSION >= 7
	QCBORDecode_EndOctets(it);
#endif
	
	return result;
}



rule_t* rule_deserialize_raw(blob_t *data, int *success)
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

	rule_t *tmp = rule_deserialize_ptr(&it, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}




rule_t*  rule_db_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	QCBORItem item;
	rule_t *result = NULL;
	QCBORError err;

	AMP_DEBUG_ENTRY("rule_db_deserialize_ptr","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					(uaddr)it, (uaddr)success);

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);

#if AMP_VERSION < 7
	/*
	 * Make sure we are in a container. All rules are captured as
	 * arrays.
	 */
	err = QCBORDecode_GetNext(it, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
	   return NULL;
	}
	else if (item.val.uCount < 7 || item.val.uCount > 8)
	{
		/*
		 * See how many items are in the container. Should be 7 for a TBR
		 * and 8 for an SBR.
		 */
		AMP_DEBUG_ERR("rule_db_deserialize_ptr","Bad array length. Len %d",
						   item.val.uCount);
		return NULL;
	}
#else // virtual array (octets)
	QCBORDecode_StartOctets(it);
#endif

	result = rule_deserialize_helper(it, success);

#if AMP_VERSION >= 7
	QCBORDecode_EndOctets(it);
#endif

	if((*success != AMP_OK) || (result == NULL))
	{
		return NULL;
	}

	*success = cut_get_cbor_numeric(it, AMP_TYPE_UINT, &(result->ticks_left));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		return NULL;
	}

	*success = cut_get_cbor_numeric(it, AMP_TYPE_UVAST, &(result->num_eval));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		return NULL;
	}

	*success = cut_get_cbor_numeric(it, AMP_TYPE_UVAST, &(result->num_fire));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		return NULL;
	}

	*success = cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &(result->flags));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		return NULL;
	}

	return result;
}

rule_t*  rule_db_deserialize_raw(blob_t *data, int *success)
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

	rule_t *tmp = rule_db_deserialize_ptr(&it, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}


int rule_db_serialize(QCBOREncodeContext *encoder, void *item)
{
	int err;
	rule_t *rule = (rule_t *) item;

	if((encoder == NULL) || (item == NULL))
	{
		return AMP_FAIL;
	}

	/* Start a container. */
	//length = (rule->id.type == AMP_TYPE_SBR) ? 8 : 7;
	QCBOREncode_OpenArray(encoder);
	
	/* Step 1: Encode the rule definition. */
	err = rule_serialize_helper(encoder, rule);
	if(err != AMP_OK)
	{
#if AMP_VERSION < 7
	   QCBOREncode_CloseArray(encoder);
#else
	   QCBOREncode_CloseArrayOctet(encoder);
#endif
	   return err;
	}

	/* Step 2: Encode the ticks left. */
	QCBOREncode_AddUInt64(encoder, rule->ticks_left);
	QCBOREncode_AddUInt64(encoder, rule->num_eval);
	QCBOREncode_AddUInt64(encoder, rule->num_fire);

	/* Step 3:Encode the flag byte. */
	err = cut_enc_byte(encoder, rule->flags);
#if AMP_VERSION < 7
	QCBOREncode_CloseArray(encoder);
#else
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	return err;
}



blob_t*   rule_db_serialize_wrapper(rule_t *rule)
{
	return cut_serialize_wrapper(RULE_DEFAULT_ENC_SIZE, rule, (cut_enc_fn)rule_db_serialize);
}


void rule_release(rule_t *rule, int destroy)
{
	CHKVOID(rule);

	if(rule->id.type == AMP_TYPE_SBR)
	{
		expr_release(&(rule->def.as_sbr.expr), 0);
	}

	ari_release(&(rule->id), 0);
	ac_release(&(rule->action), 0);

	if(destroy)
	{
		SRELEASE(rule);
	}
}

int rule_serialize(QCBOREncodeContext *encoder, void *item)
{
	rule_t *rule = (rule_t *) item;
	int err;
	
	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(rule, AMP_FAIL);

	/* Start a container. */
	//length = (rule->id.type == AMP_TYPE_SBR) ? 6 : 5;
	QCBOREncode_OpenArray(encoder);
	
	err = rule_serialize_helper(encoder, rule);

#if AMP_VERSION < 7
	QCBOREncode_CloseArray(encoder);
#else
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	return err;
}

int rule_serialize_helper(QCBOREncodeContext *array_enc, rule_t *rule)
{
	int err;
	blob_t *result;

	/* Step 1: Encode the ARI. */
#if AMP_VERSION < 7
	result = ari_serialize_wrapper(&(rule->id));
	err = blob_serialize(array_enc, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(array_enc);
	err = ari_serialize(array_enc, &(rule->id));
	QCBOREncode_CloseArrayOctet(array_enc);
#endif
	
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("rule_serialize_helper","CBOR Error: %d", err);
		return err;
	}

	/* Step 2: the start time. */
	QCBOREncode_AddUInt64(array_enc, rule->start);

	/* Step 3: Encode def. */
	if(rule->id.type == AMP_TYPE_SBR)
	{
		err = sbrdef_serialize(array_enc, &(rule->def.as_sbr));
	}
	else
	{
		err = tbrdef_serialize(array_enc, &(rule->def.as_tbr));
	}

	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("rule_serialize_helper","CBOR Error: %d", err);
		return err;
	}

	/* Step 4: Encode the action. */
#if AMP_VERSION < 7
	result = ac_serialize_wrapper(&(rule->action));
	err = blob_serialize(array_enc, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(array_enc);
	err = ac_serialize(array_enc, &(rule->action));
	QCBOREncode_CloseArrayOctet(array_enc);
#endif
	return err;
}

blob_t* rule_serialize_wrapper(rule_t *rule)
{
	return cut_serialize_wrapper(RULE_DEFAULT_ENC_SIZE, rule, (cut_enc_fn)rule_serialize);
}

// 0 means no. 1 means yes.
int sbr_should_fire(rule_t *rule)
{
	int result = 0;
	int success = AMP_FAIL;
	tnv_t *eval_result;

	CHKZERO(rule);
	CHKZERO(rule->id.type == AMP_TYPE_SBR);

	eval_result = expr_eval(&(rule->def.as_sbr.expr));
	CHKZERO(eval_result);

	if(type_is_numeric(eval_result->type))
	{
	   	result = tnv_to_int(*eval_result, &success);
	   	result = (success != AMP_OK) ? 0 : result;
	}

   	tnv_release(eval_result, 1);

   	return result;
}

sbr_def_t sbrdef_deserialize(QCBORDecodeContext *array_it, int *success)
{
	sbr_def_t result;

#if AMP_VERSION < 7
	blob_t *tmp = blob_deserialize_ptr(array_it, success);
	expr_t *e = expr_deserialize_raw(tmp, success);
	blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(array_it);
	expr_t *e = expr_deserialize_ptr(array_it, success);
	QCBORDecode_EndOctets(array_it);
#endif

	result.expr = *e;
	SRELEASE(e); // Just release container since we shallow-copied contents.

	if(*success != AMP_OK)
	{
		return result;
	}
	
	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.max_eval));
	if(*success != AMP_OK)
	{
		expr_release(&(result.expr), 0);
		return result;
	}

	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.max_fire));
	if(*success != AMP_OK)
	{
		expr_release(&(result.expr), 0);
	}

	return result;
}

int sbrdef_serialize(QCBOREncodeContext *array_enc, sbr_def_t *def)
{
	int err;
	blob_t *result;

	/* Step 1: Encode the Expr. */
#if AMP_VERSION < 7
	result = expr_serialize_wrapper(&(def->expr));
	err = blob_serialize(array_enc, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(array_enc);
	err = expr_serialize(array_enc, &(def->expr));
	QCBOREncode_CloseArrayOctet(array_enc);
#endif
	
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("sbrdef_serialize","CBOR Error: %d", err);
		return err;
	}

	/* Step 2: Encode max num evals. */
	QCBOREncode_AddUInt64(array_enc, def->max_eval);

	/* Step 3: Encode max num fires. */
	QCBOREncode_AddUInt64(array_enc, def->max_fire);

	return AMP_OK;
}



tbr_def_t tbrdef_deserialize(QCBORDecodeContext *array_it, int *success)
{
	tbr_def_t result;
	memset(&result, 0, sizeof(tbr_def_t));
	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.period));
	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("tbredef_deserialize","Failed to get period. %d", *success);
		return result;
	}

	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.max_fire));

	return result;
}


int tbrdef_serialize(QCBOREncodeContext *array_enc, tbr_def_t *def)
{
	/* Step 1: Encode period. */
	QCBOREncode_AddUInt64(array_enc, def->period);

	/* Step 2: Encode max num fires. */
	QCBOREncode_AddUInt64(array_enc, def->max_fire);

	return AMP_OK;
}
