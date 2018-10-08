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

	result->action = macdef_copy(&(src->action), &success);
	if(success != AMP_OK)
	{
		ari_release(&(result->id), 0);
		SRELEASE(result);
		return NULL;
	}

	if(result->id.type == AMP_TYPE_SBR)
	{

		result->def.as_sbr.expr = expr_copy(src->def.as_sbr.expr);
		if(success != AMP_OK)
		{
			ari_release(&(result->id), 0);
			macdef_release(&(result->action), 0);
			SRELEASE(result);
			return NULL;
		}
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

	return result;
}


rule_t*  rule_create_sbr(ari_t id, uvast start, sbr_def_t def, macdef_t action)
{
	rule_t *result = NULL;

	/* Step 1: Allocate the message. */
	if((result = (rule_t*) STAKE(sizeof(rule_t))) == NULL)
	{
		AMP_DEBUG_ERR("rule_create_sbr","Can't alloc %d bytes.", sizeof(rule_t));
		AMP_DEBUG_EXIT("rule_create_sbr","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->id = id;
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
		result->ticks_left = (start - getUTCTime());
	}


	AMP_DEBUG_EXIT("rule_create_sbr", ADDR_FIELDSPEC, (uaddr) result);

	return result;
}

rule_t*  rule_create_tbr(ari_t id, uvast start, tbr_def_t def, macdef_t action)
{
	rule_t *result = NULL;

	/* Step 1: Allocate the message. */
	if((result = (rule_t*) STAKE(sizeof(rule_t))) == NULL)
	{
		AMP_DEBUG_ERR("rule_create_tbr","Can't alloc %d bytes.", sizeof(rule_t));
		AMP_DEBUG_EXIT("rule_create_tbr","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->id = id;
	result->start = start;
	result->action = action;

	result->def.as_tbr = def;

	RULE_SET_ACTIVE(result->flags);

	if(start < AMP_RELATIVE_TIME_EPOCH)
	{
		result->ticks_left = start;
	}
	else
	{
		result->ticks_left = (start - getUTCTime());
	}


	AMP_DEBUG_EXIT("rule_create_tbr", ADDR_FIELDSPEC, (uaddr) result);

	return result;
}



rule_t*  rule_deserialize_helper(CborValue *array_it, int *success)
{
	rule_t *result = NULL;
	ari_t id;
	uvast start;
	macdef_t action;
	sbr_def_t as_sbr;
	tbr_def_t as_tbr;

	id = ari_deserialize(array_it, success);
	if(*success != AMP_OK)
	{
		ari_release(&id, 0);
		return result;
	}

	if((*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &start)) != AMP_OK)
	{
		ari_release(&id, 0);
		return result;
	}

	action = macdef_deserialize(array_it, success);
	if(*success != AMP_OK)
	{
		ari_release(&id, 0);
		return result;
	}

	if(id.type == AMP_TYPE_SBR)
	{
		as_sbr = sbrdef_deserialize(array_it, success);
		result = rule_create_sbr(id, start, as_sbr, action);
	}
	else
	{
		as_tbr = tbrdef_deserialize(array_it, success);
		result = rule_create_tbr(id, start, as_tbr, action);
	}

	if((*success != AMP_OK) || (result == NULL))
	{
		if(id.type == AMP_TYPE_SBR)
		{
			expr_release(&(as_sbr.expr), 0);
		}

		ari_release(&id, 0);
		macdef_release(&action, 0);
		SRELEASE(result);
		*success = AMP_FAIL;
		return NULL;
	}

	*success = AMP_OK;
	return result;
}


rule_t*  rule_deserialize_ptr(CborValue *it, int *success)
{
	CborError err = CborNoError;
	CborValue array_it;
	size_t length = 0;
	rule_t *result = NULL;


	AMP_DEBUG_ENTRY("rule_deserialize_ptr","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
				        (uaddr)it, (uaddr)success);

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);

	/*
	 * Make sure we are in a container. All rules are captured as
	 * arrays.
	 */
	if((!cbor_value_is_container(it)) ||
	   ((err = cbor_value_enter_container(it, &array_it)) != CborNoError))
	{
		AMP_DEBUG_ERR("rule_deserialize_ptr","Not a container. Error is %d", err);
		return NULL;
	}

	/*
	 * See how many items are in the container. Should be 5 for a TBR
	 * and 6 for an SBR.
	 */

	err = cbor_value_get_array_length(&array_it, &length);
	if((err != CborNoError) || (length < 5) || (length > 6))
	{
		AMP_DEBUG_ERR("rule_deserialize_ptr","Bad array length. Err: %d. Len %d",
					       err, length);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	result = rule_deserialize_helper(&array_it, success);
	cbor_value_leave_container(it, &array_it);
	return result;
}



rule_t* rule_deserialize_raw(blob_t *data, int *success)
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

	return rule_deserialize_ptr(&it, success);
}




rule_t*  rule_db_deserialize_ptr(CborValue *it, int *success)
{
	CborError err = CborNoError;
	CborValue array_it;
	size_t length = 0;
	rule_t *result = NULL;


	AMP_DEBUG_ENTRY("rule_db_deserialize_ptr","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
			        (uaddr)it, (uaddr)success);

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);

	/*
	 * Make sure we are in a container. All rules are captured as
	 * arrays.
	 */
	if((!cbor_value_is_container(it)) ||
		((err = cbor_value_enter_container(it, &array_it)) != CborNoError))
	{
		AMP_DEBUG_ERR("rule_db_deserialize_ptr","Not a container. Error is %d", err);
		return NULL;
	}

	/*
	 * See how many items are in the container. Should be 7 for a TBR
	 * and 8 for an SBR to the database.
	 */

	err = cbor_value_get_array_length(&array_it, &length);
	if((err != CborNoError) || (length < 7) || (length > 8))
	{
		AMP_DEBUG_ERR("rule_db_deserialize_ptr","Bad array length. Err: %d. Len %d",
				       err, length);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	result = rule_deserialize_helper(&array_it, success);

	if((*success != AMP_OK) || (result == NULL))
	{
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	*success = cut_get_cbor_numeric(&array_it, AMP_TYPE_UINT, &(result->ticks_left));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	*success = cut_get_cbor_numeric(&array_it, AMP_TYPE_UVAST, &(result->num_eval));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	*success = cut_get_cbor_numeric(&array_it, AMP_TYPE_UVAST, &(result->num_fire));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	*success = cut_get_cbor_numeric(&array_it, AMP_TYPE_BYTE, &(result->flags));
	if(*success != AMP_OK)
	{
		rule_release(result, 1);
		cbor_value_leave_container(it, &array_it);
		return NULL;
	}

	return result;
}

rule_t*  rule_db_deserialize_raw(blob_t *data, int *success)
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

	return rule_db_deserialize_ptr(&it, success);
}


CborError rule_db_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	CborEncoder array_enc;
	size_t length;
	rule_t *rule = (rule_t *) item;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(rule, CborErrorIO);


	/* Start a container. */
	length = (rule->id.type == AMP_TYPE_SBR) ? 8 : 7;
	err = cbor_encoder_create_array(encoder, &array_enc, length);

	CHKUSR(((err != CborNoError) && (err != CborErrorOutOfMemory)), err);

	/* Step 1: Encode the rule definition. */
	err = rule_serialize_helper(&array_enc, rule);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}

	/* Step 2: Encode the ticks left. */
	err = cbor_encode_uint(&array_enc, rule->ticks_left);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}

	err = cbor_encode_uint(&array_enc, rule->num_eval);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}

	err = cbor_encode_uint(&array_enc, rule->num_fire);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		cbor_encoder_close_container(encoder, &array_enc);
		return err;
	}

	/* Step 3:Encode the flag byte. */
	err = cut_enc_byte(&array_enc, rule->flags);

	cbor_encoder_close_container(encoder, &array_enc);
	return err;
}



blob_t*   rule_db_serialize_wrapper(rule_t *rule)
{
	return cut_serialize_wrapper(RULE_DEFAULT_ENC_SIZE, rule, rule_db_serialize);
}


void rule_release(rule_t *rule, int destroy)
{
	CHKVOID(rule);

	if(rule->id.type == AMP_TYPE_SBR)
	{
		expr_release(&(rule->def.as_sbr.expr), 0);
	}

	ari_release(&(rule->id), 0);
	macdef_release(&(rule->action), 0);

	if(destroy)
	{
		SRELEASE(rule);
	}
}

CborError rule_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	CborEncoder array_enc;
	size_t length;
	rule_t *rule = (rule_t *) item;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(rule, CborErrorIO);


	/* Start a container. */
	length = (rule->id.type == AMP_TYPE_SBR) ? 6 : 5;
	err = cbor_encoder_create_array(encoder, &array_enc, length);

	CHKUSR(((err != CborNoError) && (err != CborErrorOutOfMemory)), err);

	err = rule_serialize_helper(&array_enc, rule);

	cbor_encoder_close_container(encoder, &array_enc);
	return err;
}

CborError rule_serialize_helper(CborEncoder *array_enc, rule_t *rule)
{
	CborError err;
	blob_t *result;

	/* Step 1: Encode the ARI. */
	result = ari_serialize_wrapper(&(rule->id));
	err = blob_serialize(array_enc, result);
	blob_release(result, 1);

	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	/* Step 2: the start time. */
	err = cbor_encode_uint(array_enc, rule->start);
	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	/* Step 3: Encode def. */
	if(rule->id.type == AMP_TYPE_SBR)
	{
		err = sbrdef_serialize(array_enc, &(rule->def.as_sbr));
	}
	else
	{
		err = tbrdef_serialize(array_enc, &(rule->def.as_tbr));
	}

	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	/* Step 4: Encode the action. */
	result = macdef_serialize_wrapper(&(rule->action));
	err = blob_serialize(array_enc, result);
	blob_release(result, 1);

	return err;
}

blob_t* rule_serialize_wrapper(rule_t *rule)
{
	return cut_serialize_wrapper(RULE_DEFAULT_ENC_SIZE, rule, rule_serialize);
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

sbr_def_t sbrdef_deserialize(CborValue *array_it, int *success)
{
	sbr_def_t result;

	result.expr = expr_deserialize(array_it, success);
	CHKUSR(*success == AMP_OK, result);

	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.max_eval));
	CHKUSR(*success == AMP_OK, result);

	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.max_fire));

	return result;
}

CborError sbrdef_serialize(CborEncoder *array_enc, sbr_def_t *def)
{
	CborError err;
	blob_t *result;

	/* Step 1: Encode the Expr. */
	result = expr_serialize_wrapper(&(def->expr));
	err = blob_serialize(array_enc, result);
	blob_release(result, 1);

	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	/* Step 2: Encode max num evals. */
	err = cbor_encode_uint(array_enc, def->max_eval);
	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	/* Step 3: Encode max num fires. */
	err = cbor_encode_uint(array_enc, def->max_fire);
	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	return err;
}



tbr_def_t tbrdef_deserialize(CborValue *array_it, int *success)
{
	tbr_def_t result;

	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.period));
	CHKUSR(*success == AMP_OK, result);

	*success = cut_get_cbor_numeric(array_it, AMP_TYPE_UVAST, &(result.max_fire));

	return result;
}


CborError tbrdef_serialize(CborEncoder *array_enc, tbr_def_t *def)
{
	CborError err;

	/* Step 1: Encode period. */
	err = cbor_encode_uint(array_enc, def->period);
	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	/* Step 2: Encode max num fires. */
	err = cbor_encode_uint(array_enc, def->max_fire);
	CHKUSR((err != CborNoError) && (err != CborErrorOutOfMemory), err);

	return err;
}
