/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

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
 
 

/**
 - Used in tnv.c, tnv_deserialize_value_by_type
 - 
 */
char *cut_get_cbor_str(QCBORDecodeContext *it, int *success)
{
	char *result = NULL;
	QCBORItem value;
	QCBORError err;
	
	CHKNULL(it);

	err = QCBORDecode_GetNext(it, &value);
	if (err != QCBOR_SUCCESS || value.uDataType != QCBOR_TYPE_TEXT_STRING)
	{
		AMP_DEBUG_ERR("cut_get_cbor_str", "Not a valid string: %d", err);
		*success = AMP_FAIL;
        return NULL;
	}
	
	if(value.uDataType == QCBOR_TYPE_TEXT_STRING)
	{
		result = STAKE(value.val.string.len+1);
		CHKNULL(result);
		strncpy(result, value.val.string.ptr, value.val.string.len);
		result[value.val.string.len] = 0; // Guarantee result is NULL-terminated
        *success = AMP_OK;
	}

	return result;
}
int cut_get_cbor_str_ptr(QCBORDecodeContext *it, char *dst, size_t length)
{
	QCBORItem item;
	QCBORError err;
	
	CHKUSR(it, AMP_FAIL);
	CHKUSR(dst, AMP_FAIL);

	err = QCBORDecode_GetNext(it, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_TEXT_STRING)
	{
		AMP_DEBUG_ERR("cut_get_cbor_str_ptr", "Not a valid string: %d", err);
		return AMP_FAIL;
	}
	else if (item.val.string.len > length)
	{
		AMP_DEBUG_WARN("cut_get_cbor_str_ptr", "Encoded string (%d) is larger than buffer (%d); truncating",
					   item.val.string.len, length);
	}
	else if (item.val.string.len < length)
	{
		length = item.val.string.len;
	}
	memcpy(dst, item.val.string.ptr, length);
	return AMP_OK;
}


/** This encodes a raw byte.  This is not strictly compliant with CBOR protocol and uses internal APIs to achieve this effect.
 * This borrows from internal functions in QCBOR, including Nesting_Increment()
 */
int cut_enc_bytes(QCBOREncodeContext *encoder, uint8_t *buf, size_t len)
{
   CHKUSR(encoder,AMP_FAIL);
   if(len >= QCBOR_MAX_ITEMS_IN_ARRAY - encoder->nesting.pCurrentNesting->uCount) {
      return AMP_FAIL; // QCBOR_ERR_ARRAY_TOO_LONG;
   }
   
   UsefulOutBuf_InsertData(&(encoder->OutBuf),
                           buf,
                           len,
                           UsefulOutBuf_GetEndPosition(&(encoder->OutBuf))
      );
   encoder->uError = Nesting_Increment(&(encoder->nesting));
   if (encoder->uError != QCBOR_SUCCESS)
   {
      return AMP_FAIL;
   }
   else
   {
      return AMP_OK;
   }
}

/** This function allows for decoding a series of raw bytes from a CBOR string.  As with cut_enc_byte, this is not
 *    a feature strictly compliant with CBOR.  The bytes decoded in this method do NOT include CBOR types or headers.
 */
int cut_dec_bytes(QCBORDecodeContext *it, uint8_t *buf, size_t len)
{
   // This isn't directly supported, so we get a reference to underlying data buf
   UsefulInputBuf *inbuf = &(it->InBuf);
   const void *tmp;

   // Check that there is space left in the decoder buffer
   if (UsefulInputBuf_BytesUnconsumed(inbuf) < len)
   {
      AMP_DEBUG_ERR("cut_dec_bytes", "Can't read byte(s) past end of buffer", NULL);
      return AMP_FAIL;
   }

   // Retrieve bytes & advance it
   tmp = UsefulInputBuf_GetBytes(inbuf, len);
   if (tmp == NULL)
   {
      return AMP_FAIL;
   }
   memcpy(buf, tmp, len);

   // Decrement the nesting level
   DecodeNesting_DecrementCount(&(it->nesting)); // VERIFY

   // And check for errors
   if (UsefulInputBuf_GetError(inbuf) == 0) {
      return AMP_OK;
   } else {
      AMP_DEBUG_ERR("cut_cbor_numeric","Error retrieving byte", NULL);
      return AMP_FAIL;
   }

   return AMP_OK;

}

/** Encode a UVAST into CBOR-encoding and return as a blob
 */
int cut_enc_uvast(uvast num, blob_t *result)
{
	QCBOREncodeContext encoder;

	if(result == NULL)
	{
		return AMP_SYSERR;
	}

	if(blob_init(result, NULL, 0, 16) != AMP_OK)
	{
		AMP_DEBUG_ERR("cut_enc_uvast","Unable to create space for value.", NULL);
		return AMP_FAIL;
	}

	QCBOREncode_Init(&encoder, UsefulBuf_FROM_BYTE_ARRAY(result->value));

	QCBOREncode_AddUInt64(&encoder, num);

	UsefulBufC Encoded;
	if(QCBOREncode_Finish(&encoder, &Encoded)) {
		AMP_DEBUG_ERR("cut_enc_uvast", "Encoding failed", NULL);
		return AMP_FAIL;
	}

	result->length = Encoded.len;
	return AMP_OK;
}

int cut_get_cbor_numeric(QCBORDecodeContext *it, amp_type_e type, void *val)
{
	QCBORItem item;
	QCBORError status;
	int errorFlag = 0;
	CHKERR(it);
	CHKERR(val);

	/*
	 * CBOR doesn't let you encode just BYTEs.
	 * TinyCbor doesn't have a "advance single byte" function.
	 * So we invent a "get next byte and advance" capability here.
	 *
	 * LibCbor does not utilize an iterator, so we can simply retrieve the data and return
	 */
	if(type == AMP_TYPE_BYTE)
	{
		// This isn't directly supported, so we get a reference to underlying data buf
		UsefulInputBuf *buf = &(it->InBuf);

		// Check that there is space left in the buffer
		if (UsefulInputBuf_BytesUnconsumed(buf) == 0)
		{
			AMP_DEBUG_ERR("cut_cbor_numeric", "Can't read byte past end of buffer", NULL);
			return AMP_FAIL;
		}

		// Retrieve a byte & advance it
		*((uint8_t*)val) = UsefulInputBuf_GetByte(buf);

        // Decrement the nesting level
        DecodeNesting_DecrementCount(&(it->nesting));

		// And check for errors
		if (UsefulInputBuf_GetError(buf) == 0) {
			return AMP_OK;
		} else {
			AMP_DEBUG_ERR("cut_cbor_numeric","Error retrieving byte", NULL);
			return AMP_FAIL;
		}

		return AMP_OK;
	}

	// Get Next Item
	if( (status = QCBORDecode_GetNext(it, &item)) != QCBOR_SUCCESS) {
       AMP_DEBUG_ERR("cut_cbor_numeric", "QCBOR Error", status);
       return AMP_FAIL;
    }

	switch(type)
	{
		case AMP_TYPE_BOOL:
			if(item.uDataType == QCBOR_TYPE_TRUE) 
			{
               *(int*)val = 1;
			}
			else if (item.uDataType == QCBOR_TYPE_FALSE)
			{
               *(int*)val = 0;
			}
			else
			{
				errorFlag = 1;
			}
			break;

		case AMP_TYPE_UINT:
           // QCBOR may return type INT64 if value can fit in an unsigned integer
           if(item.uDataType == QCBOR_TYPE_UINT64 || (item.uDataType == QCBOR_TYPE_INT64 && item.val.int64 >= 0))
			{
				uint64_t tmp = item.val.uint64;
				*(uint32_t*)val = (uint32_t) tmp;
			}
			else
			{
				errorFlag = 1;
			}
			break;
		case AMP_TYPE_INT:
			if(item.uDataType == QCBOR_TYPE_INT64 ) 
			{
				*(uint32_t*)val = item.val.int64;
			}
			else
			{
				errorFlag = 1;
			}
			break;
		case AMP_TYPE_VAST:
		case AMP_TYPE_TS:
		case AMP_TYPE_TV:
		case AMP_TYPE_UVAST:
			if(item.uDataType == QCBOR_TYPE_UINT64)
			{
               *(uint64_t*)val = item.val.uint64;
			}
			else if(item.uDataType == QCBOR_TYPE_INT64 ) 
			{
               *(uint64_t*)val = item.val.int64;
			}
			else
			{
				errorFlag = 1;
			}
			break;

		case AMP_TYPE_REAL32:
			if(item.uDataType == QCBOR_TYPE_FLOAT)
			{
				*(float *)val = (float)item.val.dfnum;
			}
			else
			{
				errorFlag = 1;
			}
			break;
		case AMP_TYPE_REAL64:
			if(item.uDataType == QCBOR_TYPE_DOUBLE)
			{
				*(double *)val = (double)item.val.dfnum;
			}
			else
			{
				errorFlag = 1;
			}
			break;
		default:
			break;
	}

	if(errorFlag > 0)
	{
       AMP_DEBUG_ERR("cut_get_cbor_numeric","Bad CBOR Data Type %d for AMP Type %d", item.uDataType, type);
		return AMP_FAIL;
	}
	return AMP_OK;
}

/** cut_serialize_wrapper() 
 * Wrapper function to serialize an object into a new CBOR-encoded blob_t
 * @param[in] size    Expected size of item.  Not currently used by QCBOR library.
 * @param[in] item    Pointer to an item to encode, which will be passed to callback function.
 * @param     encode  Function Pointer to an encoding function
 * @returns Reference to CBOR-encoded object, or NULL on failure.
 */
blob_t* cut_serialize_wrapper(size_t size, void *item, cut_enc_fn encode)
{
	blob_t *result = NULL;
	QCBOREncodeContext encoder;
	QCBORError err;

	if(item == NULL)
	{
		return NULL;
	}

	// Calculate Required Size; QCBOR Requires Exact Length to Encode. We can optimize by calculating size-only
	QCBOREncode_Init(&encoder, (UsefulBuf){NULL,size});

	// Encode: Calculate Size Only
	encode(&encoder, item);
	err = QCBOREncode_FinishGetSize(&encoder, &size);
	if (err != QCBOR_SUCCESS && err != QCBOR_ERR_BUFFER_TOO_LARGE && err != QCBOR_ERR_BUFFER_TOO_SMALL)
	{
		AMP_DEBUG_ERR("cut_serialize_wrapper", "Error in wrapped encoder: %d", err);
		return NULL;
	}
	
	// Initialize Blob to specified length
	if((result = blob_create(NULL, 0, size)) == NULL)
	{
		AMP_DEBUG_ERR("cut_serialize_wrapper","Can't alloc encoding space",NULL);
		return NULL;
	}

	// Encode to blob_t buffer
	QCBOREncode_Init(&encoder, (UsefulBuf){result->value,result->alloc});
	if( encode(&encoder, item) != AMP_OK) {
       AMP_DEBUG_ERR("cut_serialize_wrapper", "Encoding Error", NULL);
		blob_release(result,1);
		return NULL;
	}

    UsefulBufC Encoded;
    err = QCBOREncode_Finish(&encoder, &Encoded);
    if (err != QCBOR_SUCCESS) {
		AMP_DEBUG_ERR("cut_serialize_wrapper", "Encoding Error %d", err);
		blob_release(result,1);
		return NULL;
    }
    result->length = Encoded.len;
	return result;
}

int cut_deserialize_vector(vector_t *vec, QCBORDecodeContext *it, vec_des_fn des_fn)
{
	QCBORError err = QCBOR_SUCCESS;
	QCBORItem item;
	size_t length;
	size_t i;

	AMP_DEBUG_ENTRY("cut_deserialize_vector","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)des_fn);

	// Sanity checks
	if((vec == NULL) || (it == NULL))
	{
		return AMP_FAIL;
	}

	// Open the Array
	if( (err = QCBORDecode_GetNext(it, &item)) != QCBOR_SUCCESS) {
       AMP_DEBUG_ERR("cut_deserialize_vector", "QCBOR Error %d", err);
       return AMP_FAIL;
    }
	else if (item.uDataType != QCBOR_TYPE_ARRAY)
	{
		AMP_DEBUG_ERR("cut_deserialize_vector","Not a container. Type is %d", item.uDataType);
		return AMP_FAIL;
	}

	length = item.val.uCount;

	for(i = 0; i < length; i++)
	{
       int success = AMP_FAIL;
		
		// Decode Item Contents
		void *cur_item = des_fn(it, &success);

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
		}
	}
	return AMP_OK;
}

/** cut_serialize_vector()
 * Serialize the given vector and append it to given CBOR encoding in progress.
 *
 * @param[in,out] encoder  A QCBOR encoder to append serialized vector to
 * @param[in] vec          The vector to serialize
 * @param enc_fn           An encoding function suitable to serialize items in the given vector.
 */
int cut_serialize_vector(QCBOREncodeContext *encoder, vector_t *vec, cut_enc_fn enc_fn)
{
	vecit_t it;
    int err;
	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(vec, AMP_FAIL);

	// Open Array
	QCBOREncode_OpenArray(encoder);

	for(it = vecit_first(vec); vecit_valid(it); it = vecit_next(it))
	{
		err = enc_fn(encoder, vecit_data(it));
		if (err != AMP_OK)
		{
			AMP_DEBUG_ERR("cut_serialize_vector","Can't serialize item #%d. Err is %d.",vecit_idx(it), err);
            QCBOREncode_CloseArray(encoder);
			return err;
		}
	}

	QCBOREncode_CloseArray(encoder);
	return AMP_OK;
}


/** cut_char_deserialize()
 * TODO: This wrapper can be removed
 */
void *cut_char_deserialize(QCBORDecodeContext *it, int *success)
{
	return cut_get_cbor_str(it, success);
}

int cut_char_serialize(QCBOREncodeContext *encoder, void *item)
{
	QCBOREncode_AddSZString(encoder, (char *) item);
	return AMP_OK;
}




