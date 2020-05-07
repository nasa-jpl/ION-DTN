/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file pdu.c
 **
 **
 ** Description:
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/24/12  E. Birrane     Initial Implementation (JHU/APL)
 **  11/01/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  06/26/13  E. Birrane     Added group timestamp (JHU/APL)
 **  10/01/18  E. Birrane     Updated to AMP v0.5. Migrate from pdu.h. (JHU/APL)
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "../adm/adm.h"
#include "../msg/msg.h"
#include "../utils/utils.h"
#include "../utils/cbor_utils.h"
#include "../utils/vector.h"

msg_hdr_t msg_hdr_deserialize(QCBORDecodeContext *it, int *success)
{
	msg_hdr_t hdr;

	*success = cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &(hdr.flags));

	return hdr;
}

int msg_hdr_serialize(QCBOREncodeContext *encoder, msg_hdr_t hdr)
{
	return cut_enc_byte(encoder, hdr.flags);
}

msg_agent_t* msg_agent_create()
{
	msg_agent_t *result = STAKE(sizeof(msg_agent_t));
	CHKNULL(result);

	MSG_HDR_SET_OPCODE(result->hdr.flags,MSG_TYPE_REG_AGENT);

	return result;
}


msg_agent_t *msg_agent_deserialize(blob_t *data, int *success)
{
	msg_agent_t *result = msg_agent_create();
	QCBORError err;
	QCBORDecodeContext decoder;
	QCBORItem item;
	eid_t tmp;
	int len;

	*success = AMP_FAIL;
	CHKNULL(data);
	CHKNULL(result);

	QCBORDecode_Init(&decoder,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);
	
	/* Step 1: Grab the header. */
	result->hdr = msg_hdr_deserialize(&decoder, success);
	if(*success != AMP_OK)
	{
		msg_agent_release(result, 1);
		return NULL;
	}

	//len = AMP_MAX_EID_LEN;

	err = QCBORDecode_GetNext(&decoder, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_TEXT_STRING)
	{
		AMP_DEBUG_ERR("msg_agent_deserialize", "can't retrieve string: err %d, type %d", err, item.uDataType);
		*success = AMP_FAIL;
		msg_agent_release(result, 1);
		return NULL;
	}
		
	// Verify Decoding Completed Successfully
	cut_decode_finish(&decoder);

	// Copy String and ensure it is NULL-terminated
	len = item.val.string.len;

	if (len >= AMP_MAX_EID_LEN)
	{
	   AMP_DEBUG_WARN("msg_agent_deserialize", "String length (%d) greater than AMP_MAX_EID_LEN, truncating", len);
	   len = AMP_MAX_EID_LEN - 1;
	}
	memcpy(tmp.name, item.val.string.ptr, len);
	tmp.name[len] = 0; // Ensure it's NULL-terminated

	msg_agent_set_agent(result, tmp);	 
	
	return result;
}



void msg_agent_release(msg_agent_t *msg, int destroy)
{
	CHKVOID(msg);

	if(destroy)
	{
		SRELEASE(msg);
	}
}



int msg_agent_serialize(QCBOREncodeContext *encoder, void *item)
{
	msg_agent_t *msg = (msg_agent_t *) item;

	if( msg_hdr_serialize(encoder, msg->hdr) != AMP_OK)
	{
		return AMP_FAIL;
	}

	QCBOREncode_AddSZString(encoder, msg->agent_id.name);

	return AMP_OK;
}



blob_t*   msg_agent_serialize_wrapper(msg_agent_t *msg)
{
	return cut_serialize_wrapper(MSG_DEFAULT_ENC_SIZE, msg, (cut_enc_fn)msg_agent_serialize);
}


void msg_agent_set_agent(msg_agent_t *msg, eid_t agent)
{
	CHKVOID(msg);
	msg->agent_id = agent;
}

msg_ctrl_t* msg_ctrl_create()
{
	msg_ctrl_t *result = STAKE(sizeof(msg_ctrl_t));
	CHKNULL(result);

	MSG_HDR_SET_OPCODE(result->hdr.flags,MSG_TYPE_PERF_CTRL);

	return result;
}

msg_ctrl_t* msg_ctrl_create_ari(ari_t *id)
{
	msg_ctrl_t *msg = NULL;

	CHKNULL(id);
	msg = msg_ctrl_create();
	CHKNULL(msg);
	if((msg->ac = ac_create()) == NULL)
	{
		msg_ctrl_release(msg, 1);
		return NULL;
	}

	if(vec_push(&(msg->ac->values), id) != VEC_OK)
	{
		msg_ctrl_release(msg, 1);
		return NULL;
	}

	return msg;
}

msg_ctrl_t* msg_ctrl_deserialize(blob_t *data, int *success)
{
	msg_ctrl_t *result = msg_ctrl_create();
	QCBORDecodeContext decoder;

	*success = AMP_FAIL;
	CHKNULL(data);
	CHKNULL(result);

	QCBORDecode_Init(&decoder,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	/* Step 1: Grab the header. */
	result->hdr = msg_hdr_deserialize(&decoder, success);
	if(*success != AMP_OK)
	{
		msg_ctrl_release(result, 1);
		return NULL;
	}

	/* Step 2: Grab the start time. */
	*success = cut_get_cbor_numeric(&decoder, AMP_TYPE_UVAST, &(result->start));
	if(*success != AMP_OK)
	{
		msg_ctrl_release(result, 1);
		return NULL;
	}

	/* Step 3: Grab the macro. */
	result->ac = ac_deserialize_ptr(&decoder, success);
	if(*success != AMP_OK)
	{
		msg_ctrl_release(result, 1);
		return NULL;
	}

	// Verify Decoding Completed Successfully
	cut_decode_finish(&decoder);

	return result;
}


void msg_ctrl_release(msg_ctrl_t *msg, int destroy)
{
	CHKVOID(msg);
	ac_release(msg->ac, 1);

	if(destroy)
	{
		SRELEASE(msg);
	}
}


int msg_ctrl_serialize(QCBOREncodeContext *encoder, void *item)
{
	msg_ctrl_t *msg = (msg_ctrl_t*) item;

 	if( msg_hdr_serialize(encoder, msg->hdr) != AMP_OK)
	{
		return AMP_FAIL;
	}


	QCBOREncode_AddUInt64(encoder, msg->start);

	return ac_serialize(encoder, msg->ac);
}



blob_t* msg_ctrl_serialize_wrapper(msg_ctrl_t *msg)
{
	return cut_serialize_wrapper(MSG_DEFAULT_ENC_SIZE, msg, (cut_enc_fn)msg_ctrl_serialize);
}



// Shallow copy.
int msg_rpt_add_rpt(msg_rpt_t *msg, rpt_t *rpt)
{
	CHKUSR(msg, AMP_FAIL);
	CHKUSR(rpt, AMP_FAIL);

	if(vec_push(&(msg->rpts), rpt) != VEC_OK)
	{
		return AMP_FAIL;
	}

	return AMP_OK;
}

void msg_rpt_cb_del_fn(void *item)
{
	msg_rpt_release((msg_rpt_t*)item, 1);
}


// TODO: name?
msg_rpt_t* msg_rpt_create(char *rx_name)
{
	int success;
	msg_rpt_t *result = STAKE(sizeof(msg_rpt_t));
	char *name_copy;
	int len;
	CHKNULL(result);

	MSG_HDR_SET_OPCODE(result->hdr.flags,MSG_TYPE_RPT_SET);

	if(vec_str_init(&(result->rx), 0) != VEC_OK)
	{
		SRELEASE(result);
		return NULL;
	}

	if(rx_name != NULL)
	{
		len = strlen(rx_name);
		if((name_copy = STAKE(len + 1)) == NULL)
		{
			vec_release(&(result->rx), 0);
			SRELEASE(result);
			return NULL;
		}
		memcpy(name_copy, rx_name, len );
		name_copy[len] = 0; // Ensure null-termination
		
		if(vec_push(&(result->rx), name_copy) != VEC_OK)
		{
			vec_release(&(result->rx), 0);
			SRELEASE(result);
			SRELEASE(name_copy);
			return NULL;
		}
	}

	result->rpts = vec_create(0, rpt_cb_del_fn, rpt_cb_comp_fn, NULL, VEC_FLAG_AS_STACK, &success);
	if(success != VEC_OK)
	{
		vec_release(&(result->rx), 0);
		SRELEASE(result);
		return NULL;
	}

	return result;
}



msg_rpt_t *msg_rpt_deserialize(blob_t *data, int *success)
{
	msg_rpt_t *result;
	QCBORDecodeContext it;

	*success = AMP_FAIL;
	CHKNULL(data);

	result = msg_rpt_create(NULL);
	CHKNULL(result);

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	/* Step 1: Grab the header. */
	result->hdr = msg_hdr_deserialize(&it, success);
	if(*success != AMP_OK)
	{
		msg_rpt_release(result, 1);
		return NULL;
	}

	/* Step 2: Grab the array of recipients. */
	if((cut_deserialize_vector(&(result->rx), &it, cut_char_deserialize)) != AMP_OK)
	{
		*success = AMP_FAIL;
		msg_rpt_release(result, 1);
		return NULL;
	}

	/* Step 3: Grab the array of report entries. */
	if((cut_deserialize_vector(&(result->rpts), &it, rpt_deserialize_ptr)) != AMP_OK)
	{
		*success = AMP_FAIL;
		msg_rpt_release(result, 1);
		return NULL;
	}

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);
	
	return result;
}


void msg_rpt_release(msg_rpt_t *msg, int destroy)
{
	CHKVOID(msg);
	vec_release(&(msg->rx), 0);
	vec_release(&(msg->rpts), 0);
	if(destroy)
	{
		SRELEASE(msg);
	}
}


/*
 * First item: Header
 * Second item: Vector of strings.
 * Third item:Vector of reports.
 *
 */
int msg_rpt_serialize(QCBOREncodeContext *encoder, void *item)
{
	msg_rpt_t *msg = (msg_rpt_t *)item;

	if (msg_hdr_serialize(encoder, msg->hdr) != AMP_OK)
	{
		return AMP_FAIL;
	}


	if (cut_serialize_vector(encoder, &(msg->rx), (cut_enc_fn)cut_char_serialize) != AMP_OK)
	{
		return AMP_FAIL;
	}


	return cut_serialize_vector(encoder, &(msg->rpts), (cut_enc_fn)rpt_serialize);

}


blob_t* msg_rpt_serialize_wrapper(msg_rpt_t *msg)
{
	return cut_serialize_wrapper(MSG_DEFAULT_ENC_SIZE, msg, (cut_enc_fn)msg_rpt_serialize);
}


int msg_tbl_add_tbl(msg_tbl_t *msg, tbl_t *tbl)
{
	CHKUSR(msg, AMP_FAIL);
	CHKUSR(tbl, AMP_FAIL);

	if(vec_push(&(msg->tbls), tbl) != VEC_OK)
	{
		return AMP_FAIL;
	}

	return AMP_OK;	
}

void msg_tbl_cb_del_fn(void *item)
{
	msg_tbl_release((msg_tbl_t*)item, 1);
}

msg_tbl_t* msg_tbl_create(char *rx_name)
{
	int success;
	msg_tbl_t *result = STAKE(sizeof(msg_tbl_t));
	char *name_copy;
	int len;
	CHKNULL(result);

	MSG_HDR_SET_OPCODE(result->hdr.flags,MSG_TYPE_TBL_SET);

	if(vec_str_init(&(result->rx), 0) != VEC_OK)
	{
		SRELEASE(result);
		return NULL;
	}

	if(rx_name != NULL)
	{
		len = strlen(rx_name);
		if((name_copy = STAKE(len + 1)) == NULL)
		{
			vec_release(&(result->rx), 0);
			SRELEASE(result);
			return NULL;
		}
		memcpy(name_copy, rx_name, len);
		name_copy[len] = 0; // Ensure enull-termination

		if(vec_push(&(result->rx), name_copy) != VEC_OK)
		{
			vec_release(&(result->rx), 0);
			SRELEASE(result);
			SRELEASE(name_copy);
			return NULL;
		}
	}

	result->tbls = vec_create(0, tbl_cb_del_fn, tbl_cb_comp_fn, NULL, VEC_FLAG_AS_STACK, &success);
	if(success != VEC_OK)
	{
		vec_release(&(result->rx), 0);
		SRELEASE(result);
		return NULL;
	}

	return result;
}

msg_tbl_t *msg_tbl_deserialize(blob_t *data, int *success)
{
	msg_tbl_t *result;
	QCBORDecodeContext it;

	*success = AMP_FAIL;
	CHKNULL(data);

	result = msg_tbl_create(NULL);
	CHKNULL(result);

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	/* Step 1: Grab the header. */
	result->hdr = msg_hdr_deserialize(&it, success);
	if(*success != AMP_OK)
	{
		msg_tbl_release(result, 1);
		return NULL;
	}

	/* Step 2: Grab the array of recipients. */
	if((cut_deserialize_vector(&(result->rx), &it, cut_char_deserialize)) != AMP_OK)
	{
		*success = AMP_FAIL;
		msg_tbl_release(result, 1);
		return NULL;
	}

	/* Step 3: Grab the array of table entries. */
	if((cut_deserialize_vector(&(result->tbls), &it, tbl_deserialize_ptr)) != AMP_OK)
	{
		*success = AMP_FAIL;
		msg_tbl_release(result, 1);
		return NULL;
	}

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);
	
	return result;
}


void msg_tbl_release(msg_tbl_t *msg, int destroy)
{
	CHKVOID(msg);
	vec_release(&(msg->rx), 0);
	vec_release(&(msg->tbls), 0);
	if(destroy)
	{
		SRELEASE(msg);
	}
}


int msg_tbl_serialize(QCBOREncodeContext *encoder, void *item)
{
	msg_tbl_t *msg = (msg_tbl_t *)item;

	if (msg_hdr_serialize(encoder, msg->hdr) != AMP_OK)
	{
		return AMP_FAIL;
	}


	if (cut_serialize_vector(encoder, &(msg->rx), (cut_enc_fn)cut_char_serialize) != AMP_OK)
	{
		return AMP_FAIL;
	}


	return cut_serialize_vector(encoder, &(msg->tbls), (cut_enc_fn)tbl_serialize);

}


blob_t* msg_tbl_serialize_wrapper(msg_tbl_t *msg)
{
	return cut_serialize_wrapper(MSG_DEFAULT_ENC_SIZE, msg, (cut_enc_fn)msg_tbl_serialize);
}

#if 0
void msg_release(void *msg, int msg_type, int destroy)
{
	switch(type) {
	case MSG_TYPE_PERF_CTRL:
		msg_ctrl_release(msg, destroy);
		break;
	case MSG_TYPE_RPT_SET:
		msg_rpt_release(msg, destroy);
		break;
	case MSG_TYPE_REG_AGENT:
		msg_agent_release(msg, destroy);
		break;
		// TODO: case MSG_TYPE_TABLE_SET
	}
}
#endif
int msg_grp_add_msg(msg_grp_t *grp, blob_t *msg, uint8_t type)
{
	CHKUSR(grp, AMP_FAIL);
	CHKUSR(msg, AMP_FAIL);

	if( (blob_append(&(grp->types), &type, 1) != AMP_OK) ||
        (vec_push(&(grp->msgs), msg) == VEC_OK))
	{
		return AMP_OK;
	}
	return AMP_FAIL;
}


int msg_grp_add_msg_agent(msg_grp_t *grp, msg_agent_t *msg)
{
	blob_t *result = NULL;
	int success;

	CHKUSR(grp, AMP_FAIL);
	CHKUSR(msg, AMP_FAIL);

	result = msg_agent_serialize_wrapper(msg);
	CHKUSR(result, AMP_FAIL);

	if((success = msg_grp_add_msg(grp, result, MSG_TYPE_REG_AGENT)) != AMP_OK)
	{
		blob_release(result, 1);
	}

	return success;
}

int msg_grp_add_msg_ctrl(msg_grp_t *grp, msg_ctrl_t *msg)
{
	blob_t *result = NULL;
	int success;

	CHKUSR(grp, AMP_FAIL);
	CHKUSR(msg, AMP_FAIL);

	result = msg_ctrl_serialize_wrapper(msg);
	CHKUSR(result, AMP_FAIL);

	if((success = msg_grp_add_msg(grp, result, MSG_TYPE_PERF_CTRL)) != AMP_OK)
	{
		blob_release(result, 1);
	}

	return success;
}

int msg_grp_add_msg_rpt(msg_grp_t *grp, msg_rpt_t *msg)
{
	blob_t *result = NULL;
	int success;

	CHKUSR(grp, AMP_FAIL);
	CHKUSR(msg, AMP_FAIL);

	result = msg_rpt_serialize_wrapper(msg);
	CHKUSR(result, AMP_FAIL);

	if((success = msg_grp_add_msg(grp, result, MSG_TYPE_RPT_SET)) != AMP_OK)
	{
		blob_release(result, 1);
	}

	return success;
}

int msg_grp_add_msg_tbl(msg_grp_t *grp, msg_tbl_t *msg)
{
	blob_t *result = NULL;
	int success;

	CHKUSR(grp, AMP_FAIL);
	CHKUSR(msg, AMP_FAIL);

	result = msg_tbl_serialize_wrapper(msg);
	CHKUSR(result, AMP_FAIL);

	if((success = msg_grp_add_msg(grp, result, MSG_TYPE_TBL_SET)) != AMP_OK)
	{
		blob_release(result, 1);
	}

	return success;
}


msg_grp_t  *msg_grp_create(uint8_t length)
{
	msg_grp_t *result = STAKE(sizeof(msg_grp_t));
	CHKNULL(result);
	if(vec_blob_init(&(result->msgs), length) != VEC_OK)
	{
		msg_grp_release(result, 1);
		return NULL;
	}

	blob_init(&(result->types), NULL, 0, length);

	return result;
}

msg_grp_t* msg_grp_deserialize(blob_t *data, int *success)
{
	QCBORDecodeContext decoder;
	QCBORItem item;
	QCBORError err;
	size_t length;
	size_t i;
	msg_grp_t *result = NULL;

	*success = AMP_FAIL;
	CHKNULL(data);

	QCBORDecode_Init(&decoder,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	/* Step 1: are we at an array? */
	err = QCBORDecode_GetNext(&decoder, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount == 0)
	{
	   AMP_DEBUG_ERR("msg_grp_deserialize",
					 "First item not a valid array: err %d type %d cnt %d",
					 err, item.uDataType, item.val.uCount);
		return NULL;
	}

	length = item.val.uCount;
	   
	// first element of the array is the timestamp.
	result = msg_grp_create(length-1);
	CHKNULL(result);

	if((*success = cut_get_cbor_numeric(&decoder, AMP_TYPE_TS, &(result->time))) != AMP_OK)
	{
		msg_grp_release(result, 1);
		return NULL;
	}

	for(i = 1; i < length; i++)
	{
		blob_t *cur_item = blob_deserialize_ptr(&decoder, success);
		int msg_type;

		/* Get the type of the message.*/
		msg_type = MSG_HDR_GET_OPCODE(cur_item->value[0]);

		if((*success = msg_grp_add_msg(result, cur_item, msg_type)) != AMP_OK)
		{
			blob_release(cur_item, 1);
			msg_grp_release(result, 1);
			return NULL;
		}
	}

	// Verify Decoding Completed Successfully
	cut_decode_finish(&decoder);

	return result;
}


int msg_grp_get_type(msg_grp_t *grp, int idx)
{
	CHKUSR(grp, MSG_TYPE_UNK);
	CHKUSR(idx < grp->types.length, MSG_TYPE_UNK);

	return (int) grp->types.value[idx];
}

void msg_grp_release(msg_grp_t *group, int destroy)
{
	CHKVOID(group);
	vec_release(&(group->msgs), 0);
	blob_release(&(group->types), 0);
	if(destroy)
	{
		SRELEASE(group);
	}
}

int msg_grp_serialize(QCBOREncodeContext *encoder, void *item)
{
	vec_idx_t i;
	vec_idx_t max;
	msg_grp_t *msg_grp = (msg_grp_t*) item;


	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(item, AMP_FAIL);

	// Open Array
	QCBOREncode_OpenArray(encoder);

	max = vec_num_entries(msg_grp->msgs);

	QCBOREncode_AddUInt64(encoder, msg_grp->time);

	for(i = 0; i < max; i++)
	{
		blob_t *tmp = vec_at(&msg_grp->msgs, i);

		if(tmp == NULL)
		{
			QCBOREncode_CloseArray(encoder); // Not necessary if encoding is fully aborted.
			return AMP_FAIL;
		}
		else
		{
			QCBOREncode_AddBytes(encoder, (UsefulBufC){tmp->value,tmp->length} );
		}

	}

	QCBOREncode_CloseArray(encoder);
	return AMP_OK;
}


blob_t *msg_grp_serialize_wrapper(msg_grp_t *msg_grp)
{
	return cut_serialize_wrapper(MSG_DEFAULT_ENC_SIZE, msg_grp, (cut_enc_fn)msg_grp_serialize);
}


