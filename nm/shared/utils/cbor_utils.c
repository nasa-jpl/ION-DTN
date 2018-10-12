/*****************************************************************************
 **
 ** File Name: cbor_utils.c
 **
 ** Subsystem:
 **          Shared utilities
 **
 ** Description: This file provides CBOR encoding/decoding functions for 
 **              AMP structures.                
 **
 ** Notes:
 **
 ** Assumptions:
 **              
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/31/18  E. Birrane     Initial Implementation (JHU/APL)
 *****************************************************************************/
 
#include "platform.h"
#include "cbor_utils.h"
#include "utils.h"



 uint8_t gEncBuf[CUT_ENC_BUFSIZE];
 
 


int cut_get_array_len(CborValue *it, size_t *result)
{
	CborError err;

	CHKERR(it);
	CHKERR(result);

	if(!cbor_value_is_array(it))
	{
		AMP_DEBUG_ERR("cut_get_array_len","Value not array.", NULL);
		return AMP_FAIL;
	}

	if((err = cbor_value_get_array_length(it, result)) != CborNoError)
	{
		AMP_DEBUG_ERR("cut_get_array_len","Cbor Error %d.", err);
		return AMP_FAIL;
	}

	return AMP_OK;
}


int cut_advance_it(CborValue *value)
{
	CborError err;

	CHKERR(value);

	if(cbor_value_at_end(value))
	{
		return AMP_OK;
	}

	if((err = cbor_value_advance(value)) != CborNoError)
	{
		AMP_DEBUG_ERR("cut_advance_it","Cbor error advancing value %d", err);
		return AMP_FAIL;
	}

	return AMP_OK;
}



char *cut_get_cbor_str(CborValue *value, int *success)
{
	char *result = NULL;
	CborError err;

	if(cbor_value_is_text_string(value))
	{
		size_t len = 0;
		err = cbor_value_calculate_string_length(value, &len);
		CHKNULL(err == CborNoError);
		result = STAKE(len);
		CHKNULL(result);
		err = cbor_value_copy_text_string(value, result, &len, NULL);
		if(err != CborNoError)
		{
			AMP_DEBUG_ERR("cut_get_cbor_str","Cbor error copying string %d", err);
			SRELEASE(result);
			*success = AMP_FAIL;
			result = NULL;
		}
	}

	*success = cut_advance_it(value);

	return result;
}



CborError cut_enc_byte(CborEncoder *encoder, uint8_t byte)
{
	return append_byte_to_buffer(encoder, byte);
}

int cut_enter_array(CborValue *it, size_t min, size_t max, size_t *num, CborValue *array_it)
{
	CborValue result;


	CHKUSR(it, AMP_FAIL);
	CHKUSR(array_it, AMP_FAIL);
	CHKUSR(cbor_value_is_array(it), AMP_FAIL);

	if(cbor_value_get_array_length(it, num) != CborNoError)
	{
		return AMP_FAIL;
	}

	if(min != 0)
	{
		if (*num < min)
		{
			return AMP_FAIL;
		}
	}

	if(max != 0)
	{
		if(*num > max)
		{
			return AMP_FAIL;
		}
	}

	if(cbor_value_enter_container(it, array_it) != CborNoError)
	{
		return AMP_FAIL;
	}

	return AMP_OK;
}


int cut_get_cbor_numeric(CborValue *value, amp_type_e type, void *val)
{
	CborError err = CborErrorUnknownType;

	CHKERR(value);
	CHKERR(val);

	/*
	 * CBOR doesn't let you encode just BYTEs.
	 * TinyCbor doesn't have a "advance single byte" function.
	 * So we invent a "get next byte and advance" capability here.
	 */
	if(type == AMP_TYPE_BYTE)
	{
		*((uint8_t*)val) = *((uint8_t*) value->ptr);
		value->ptr++;
		preparse_value(value);
		return AMP_OK;
	}

	switch(type)
	{
		case AMP_TYPE_BOOL:
			if(cbor_value_is_simple_type(value))
			{
				err = cbor_value_get_simple_type(value, (uint8_t *)val);
				if(*(uint8_t*)val > 1)
				{
					AMP_DEBUG_ERR("cut_cbor_numeric","Bad boolean value %d", *((uint8_t*)val));
					return AMP_FAIL;
				}
			}
			break;

		case AMP_TYPE_UINT:
			if(cbor_value_is_unsigned_integer(value))
			{
				uvast tmp;
				err = cbor_value_get_uint64(value, &tmp);
				*(uint32_t*)val = (uint32_t) tmp;
			}
			break;
		case AMP_TYPE_INT:
			if(cbor_value_is_integer(value))
			{
				err = cbor_value_get_int(value, (int32_t *)val);
			}
			break;

		case AMP_TYPE_VAST:
			if(cbor_value_get_type(value) == CborIntegerType )
			{
				err = cbor_value_get_int64(value, (vast *)val);
			}
			break;

		case AMP_TYPE_TS:
		case AMP_TYPE_TV:
		case AMP_TYPE_UVAST:
			if(cbor_value_get_type(value) == CborIntegerType )
			{
				err = cbor_value_get_uint64(value, (uvast *)val);
			}
			break;

		case AMP_TYPE_REAL32:
			if(cbor_value_is_float(value))
			{
				err = cbor_value_get_float(value, (float *)val);
			}
			break;
		case AMP_TYPE_REAL64:
			if(cbor_value_is_double(value))
			{
				err = cbor_value_get_double(value, (double *)val);
			}
			break;
		default:
			break;
	}

	if(err != CborNoError)
	{
		AMP_DEBUG_ERR("cut_get_cbor_numeric","Cbor error %d", err);
		return AMP_FAIL;
	}

	return cut_advance_it(value);
}


void cut_init_enc(CborEncoder *encoder, uint8_t *val, uint32_t size)
{

	if(val == NULL)
	{
		cbor_encoder_init(encoder, gEncBuf, CUT_ENC_BUFSIZE, 0);
	}
	else
	{
		cbor_encoder_init(encoder, val, size, 0);
	}
}


blob_t* cut_serialize_wrapper(size_t size, void *item, cut_enc_fn encode)
{
	blob_t *result = NULL;
	CborEncoder encoder;
	CborError err;

	CHKNULL(item);

	if((result = blob_create(NULL, 0, size)) == NULL)
	{
		AMP_DEBUG_ERR("cut_serialize_wrapper","Can't alloc encoding space",NULL);
		return NULL;
	}

	cbor_encoder_init(&encoder, result->value, result->alloc, 0);
	err = encode(&encoder, item);

	/* If we didn't have enough memory, calculate how much we need and try again.*/
	if(err == CborErrorOutOfMemory)
	{
		size_t extra = cbor_encoder_get_extra_bytes_needed(&encoder);

		if(blob_grow(result, extra) != AMP_OK)
		{
			AMP_DEBUG_ERR("cut_serialize_wrapper", "Can't grow by %d", extra);
			blob_release(result, 1);
			return NULL;
		}

		cbor_encoder_init(&encoder, result->value, result->alloc, 0);
		err = encode(&encoder, item);
	}

	/* If we failed to serialize, free resources. */
	if(err != CborNoError)
	{
		AMP_DEBUG_ERR("cut_serialize_wrapper","Cbor Error: %d", err);
		blob_release(result, 1);
		return NULL;
	}

	/* Note how many bytes of the blob we used in the encoding. */
	result->length = cbor_encoder_get_buffer_size(&encoder, result->value);
	return result;
}


int cut_deserialize_vector(vector_t *vec, CborValue *it, vec_des_fn des_fn)
{
	CborError err = CborNoError;
	CborValue array_it;
	size_t length;
	size_t i;
	int success;

	/* Step 1: are we at an array? */

	AMP_DEBUG_ENTRY("cut_deserialize_vector","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

	CHKUSR(vec, AMP_FAIL);
	CHKUSR(it, AMP_FAIL);

	success = AMP_FAIL;

	if((!cbor_value_is_container(it)) ||
	   ((err = cbor_value_enter_container(it, &array_it)) != CborNoError))
	{
		AMP_DEBUG_ERR("cut_deserialize_vector","Not a container. Error is %d", err);
		return success;
	}

	if((err = cbor_value_get_array_length(it, &length)) != CborNoError)
	{
		AMP_DEBUG_ERR("cut_deserialize_vector","Can't get array length. Err is %d", err);
		return success;
	}

	for(i = 0; i < length; i++)
	{
		void *cur_item = des_fn(&array_it, &success);

		if((cur_item == NULL) || (success != AMP_OK))
		{
			AMP_DEBUG_ERR("cut_deserialize_vector","Can't get item %d",i);
			return AMP_FAIL;
		}

		if(vec_insert(vec, cur_item, NULL) != VEC_OK)
		{
			if(vec->delete_fn != NULL)
			{
				vec->delete_fn(cur_item);
			}
			success = AMP_FAIL;
		}

	}

	if((err = cbor_value_leave_container(it, &array_it)) != CborNoError)
	{
		AMP_DEBUG_ERR("cut_deserialize_vector","Can't leave container. Err is %d.", err);
		return AMP_FAIL;
	}

	return AMP_OK;
}


CborError cut_serialize_vector(CborEncoder *encoder, vector_t *vec, cut_enc_fn enc_fn)
{
	CborError err;
	CborEncoder array_enc;
	vecit_t it;
	vec_idx_t max;
	int success;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(vec, CborErrorIO);

	max = vec_num_entries(*vec);
	err = cbor_encoder_create_array(encoder, &array_enc, max);
	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		return err;
	}

	for(it = vecit_first(vec); vecit_valid(it); it = vecit_next(it))
	{
		blob_t *result = cut_serialize_wrapper(CUT_ENC_BUFSIZE, vecit_data(it), enc_fn);

		err = CborErrorIO;
		if(result != NULL)
		{
			err = cbor_encode_byte_string(encoder, result->value, result->length);
			blob_release(result, 1);
		}

		if((err != CborNoError) && (err != CborErrorOutOfMemory))
		{
			AMP_DEBUG_ERR("cut_serialize_vector","Can't serialize item #%d. Err is %d.",vecit_idx(it), err);
			cbor_encoder_close_container(encoder, &array_enc);
			return err;
		}
	}

	return cbor_encoder_close_container(encoder, &array_enc);
}



void *cut_char_deserialize(CborValue *it, int *success)
{
	char *new_item;
	size_t length;

	*success = AMP_FAIL;
	CHKNULL(it);
	CHKNULL(cbor_value_is_text_string(it));



	if(cbor_value_get_string_length(it,&length) != CborNoError)
	{
		return NULL;
	}

	if((new_item = (char *) STAKE(length + 1)) == NULL)
	{
		return NULL;
	}

	if(cbor_value_copy_text_string(it, new_item, &length, it) != CborNoError)
	{
		SRELEASE(new_item);
		new_item = NULL;
	}


	return new_item;
}

CborError cut_char_serialize(CborEncoder *encoder, void *item)
{
	return cbor_encode_text_stringz(encoder, (char *) item);
}




