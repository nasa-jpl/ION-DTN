/*****************************************************************************
 **
 ** File Name: value.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary to represent typed values within the
 **              the DTNMP.
 **
 **              A value is a strongly-typed outcome of an operation, the result of
 **              an expression, the association of data with a literal, or any other
 **              case where typed data comparisons are necessary.
 **
 ** Notes:
 **
 **  1. Data types are typed based on their data type (integer, real, etc...) as
 **     well as their precision. Precision is segmented into "up-to-32-bit" and
 **     "greater-than-32-bit" representations.
 **
 ** Assumptions:
 **		1. ION DTNMP assumes operators are binary operators.
 **		2. ION DTNMP does not support integer or real values > 64 bits.
 **
 *****************************************************************************/

#include "ion.h"
#include "platform.h"

#include "shared/primitives/value.h"

int gValResultType[9][9] = {
{DTNMP_TYPE_UNK,DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_INT,   DTNMP_TYPE_INT,   DTNMP_TYPE_VAST,  DTNMP_TYPE_UNK,   DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL64,DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_INT,   DTNMP_TYPE_UINT,  DTNMP_TYPE_VAST,  DTNMP_TYPE_UVAST, DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL64,DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_VAST,  DTNMP_TYPE_VAST,  DTNMP_TYPE_VAST,  DTNMP_TYPE_VAST,  DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL64,DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_UNK,   DTNMP_TYPE_UVAST, DTNMP_TYPE_VAST,  DTNMP_TYPE_UVAST, DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL64,DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL32,DTNMP_TYPE_REAL64,DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_REAL64,DTNMP_TYPE_REAL64,DTNMP_TYPE_REAL64,DTNMP_TYPE_REAL64,DTNMP_TYPE_REAL64,DTNMP_TYPE_REAL64,DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK, DTNMP_TYPE_UNK},
{DTNMP_TYPE_UNK,DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK,   DTNMP_TYPE_UNK, DTNMP_TYPE_UNK}
};


value_t* val_as_ptr(value_t val)
{
	value_t *result = NULL;

	if((result = (value_t *) MTAKE(sizeof(value_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("val_as_ptr","Unable to allocate val of size %d", sizeof(value_t));
		DTNMP_DEBUG_EXIT("val_as_ptr","->NULL",NULL);
		return NULL;
	}

	result->length = val.length;
	result->type = val.type;

	switch(result->type)
	{
		case DTNMP_TYPE_INT:
			result->value.as_int = val.value.as_int;
			break;
		case DTNMP_TYPE_TS:
		case DTNMP_TYPE_UINT:
			result->value.as_uint = val.value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result->value.as_vast = val.value.as_vast;
			break;
		case DTNMP_TYPE_SDNV:
		case DTNMP_TYPE_UVAST:
			result->value.as_uvast = val.value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result->value.as_real32 = val.value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result->value.as_real64 = val.value.as_real64;
			break;
		case DTNMP_TYPE_STRING:
		case DTNMP_TYPE_BLOB:
		case DTNMP_TYPE_DC:
		case DTNMP_TYPE_MID:
		case DTNMP_TYPE_MC:
		case DTNMP_TYPE_EXPR:
		case DTNMP_TYPE_DEF:
		case DTNMP_TYPE_TRL:
		case DTNMP_TYPE_SRL:
			if((result->value.as_ptr = (uint8_t*) MTAKE(result->length)) == NULL)
			{
				DTNMP_DEBUG_ERR("val_as_ptr","Unable to allocate val of size %d", sizeof(result->length));

				MRELEASE(result);
				DTNMP_DEBUG_EXIT("val_as_ptr","->NULL",NULL);
				return NULL;
			}

			memcpy(result->value.as_ptr, val.value.as_ptr, result->length);
		default:
			result->value.as_uvast = val.value.as_uvast;
		break;
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: val_cvt_int
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 *
 *****************************************************************************/

int32_t  val_cvt_int(value_t *val)
{
	int32_t result = 0;

	if(val == NULL)
	{
		return result;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
			result = val->value.as_int;
			break;
		case DTNMP_TYPE_UINT:
			result = (int32_t) val->value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result = (int32_t) val->value.as_vast;
			break;
		case DTNMP_TYPE_UVAST:
			result = (int32_t) val->value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result = (int32_t) val->value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result = (int32_t) val->value.as_real64;
			break;
		default:
			break;
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name: val_cvt_uint
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 *
 *****************************************************************************/

uint32_t  val_cvt_uint(value_t *val)
{
	uint32_t result = 0;

	if(val == NULL)
	{
		return result;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
			result = (uint32_t) val->value.as_int;
			break;
		case DTNMP_TYPE_UINT:
			result = val->value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result = (uint32_t) val->value.as_vast;
			break;
		case DTNMP_TYPE_UVAST:
			result = (uint32_t) val->value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result = (uint32_t) val->value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result = (uint32_t) val->value.as_real64;
			break;
		default:
			break;
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name: val_cvt_vast
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 *
 *****************************************************************************/

vast  val_cvt_vast(value_t *val)
{
	vast result = 0;

	if(val == NULL)
	{
		return result;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
			result = (vast) val->value.as_int;
			break;
		case DTNMP_TYPE_UINT:
			result = (vast) val->value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result = val->value.as_vast;
			break;
		case DTNMP_TYPE_UVAST:
			result = (vast) val->value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result = (vast) val->value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result = (vast) val->value.as_real64;
			break;
		default:
			break;
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name: val_cvt_uvast
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 *
 *****************************************************************************/

uvast  val_cvt_uvast(value_t *val)
{
	uvast result = 0;

	if(val == NULL)
	{
		return result;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
			result = (uvast) val->value.as_int;
			break;
		case DTNMP_TYPE_UINT:
			result = (uvast) val->value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result = (uvast) val->value.as_vast;
			break;
		case DTNMP_TYPE_UVAST:
			result =  val->value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result = (uvast) val->value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result = (uvast) val->value.as_real64;
			break;
		default:
			break;
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name: val_cvt_real32
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 *
 *****************************************************************************/

float  val_cvt_real32(value_t *val)
{
	float result = 0;

	if(val == NULL)
	{
		return result;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
			result = (float) val->value.as_int;
			break;
		case DTNMP_TYPE_UINT:
			result = (float) val->value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result = (float) val->value.as_vast;
			break;
		case DTNMP_TYPE_UVAST:
			result = (float) val->value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result = (float) val->value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result = (float) val->value.as_real64;
			break;
		default:
			break;
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name: val_cvt_real64
 *
 * \par Conversion function, relying on language type casting
 *      rather than trying to be clever based on internal
 *      representation.
 *
 * \retval The converted value of the value type.
 *
 * \param[in]  val 	The value type to be converted.
 *
 *****************************************************************************/

double  val_cvt_real64(value_t *val)
{
	double result = 0;

	if(val == NULL)
	{
		return result;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
			result = (double) val->value.as_int;
			break;
		case DTNMP_TYPE_UINT:
			result = (double) val->value.as_uint;
			break;
		case DTNMP_TYPE_VAST:
			result = (double) val->value.as_vast;
			break;
		case DTNMP_TYPE_UVAST:
			result = (double) val->value.as_uvast;
			break;
		case DTNMP_TYPE_REAL32:
			result = (double) val->value.as_real32;
			break;
		case DTNMP_TYPE_REAL64:
			result = (double) val->value.as_real64;
			break;
		default:
			break;
	}

	return result;
}


void val_cvt_type(value_t *val, dtnmp_type_e type)
{
	if((val == NULL) || (type == DTNMP_TYPE_UNK))
	{
		return;
	}

	if(val->type == type)
	{
		return;
	}

	if((type_is_numeric(type) == 0) ||
	   (type_is_numeric(val->type) == 0))
	{
		DTNMP_DEBUG_ERR("val_cvt_type","Can't cvt from %d to %d.", val->type, type);
		return;
	}

	val->type = type;
	switch(val->type)
	{
	case DTNMP_TYPE_INT:
		val->value.as_int = val_cvt_int(val);
		break;
	case DTNMP_TYPE_UINT:
		val->value.as_uint = val_cvt_uint(val);
		break;
	case DTNMP_TYPE_VAST:
		val->value.as_vast = val_cvt_vast(val);
		break;
	case DTNMP_TYPE_UVAST:
		val->value.as_uvast = val_cvt_uvast(val);
		break;
	case DTNMP_TYPE_REAL32:
		val->value.as_real32 = val_cvt_real32(val);
		break;
	case DTNMP_TYPE_REAL64:
		val->value.as_real64 = val_cvt_real64(val);
		break;
	default:
		DTNMP_DEBUG_ERR("val_cvt_type","Can't convert to non-numeric type.", NULL);
		break;

	}
}


/******************************************************************************
 *
 * \par Function Name: val_deserialize
 *
 * \par Extracts a value from a byte buffer.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized value.
 *
 * \param[in]  buffer         The byte buffer holding the value data
 * \param[in]  bytes_left     The # bytes available in the buffer
 * \param[in/out] bytes_used  The # of bytes consumed in the deserialization.
 *
 * 7/10/15 E. Birrane  Updated to support all DTNMP types.
 *****************************************************************************/

value_t*  val_deserialize(unsigned char *buffer,
		                 uint32_t bytes_left,
		                 uint32_t *bytes_used)
{
    value_t *result = NULL;
    unsigned char *cursor = NULL;
    uint32_t cur_bytes = 0;
    uint32_t bytes=0;
    uint32_t len = 0;

    DTNMP_DEBUG_ENTRY("val_deserialize","(%lx, %lu, %lx)",
                     (unsigned long) buffer,
                     (unsigned long) bytes_left,
                     (unsigned long) bytes_used);

    /* Step 1: Sanity checks. */
    if((buffer == NULL) || (bytes_left == 0) || (bytes_used == NULL))
    {
        DTNMP_DEBUG_ERR("val_deserialize","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	*bytes_used = 0;
    	cursor = buffer;
    }

    /* Step 2: Allocate/Zero the value. */
    if((result = (value_t *) MTAKE(sizeof(value_t))) == NULL)
    {
        DTNMP_DEBUG_ERR("val_deserialize","Cannot allocate value of size %d",
        		        sizeof(value_t));
        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
        memset(result,0,sizeof(value_t));
    }

    /* Step 3: Grab the value type */
    if(utils_grab_byte(cursor, bytes_left, (uint8_t*)&(result->type)) != 1)
    {
    	DTNMP_DEBUG_ERR("val_deserialize","Can't grab type byte.", NULL);
    	MRELEASE(result);
    	*bytes_used = 0;

        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	cursor += 1;
    	bytes_left -= 1;
    	*bytes_used += 1;
    }

    /* Step 4: Grab the length */
    uvast slen = 0;
	if((cur_bytes = utils_grab_sdnv(cursor, bytes_left, &(slen))) == 0)
    {
    	DTNMP_DEBUG_ERR("val_deserialize","Can't grab issuer.", NULL);
    	MRELEASE(result);
    	*bytes_used = 0;

        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	result->length = (uint32_t) slen;
    	if((uvast)result->length != slen)
    	{
        	DTNMP_DEBUG_ERR("val_deserialize","Precision error.", NULL);
        	MRELEASE(result);
        	*bytes_used = 0;

            DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
            return NULL;
    	}

    	cursor += cur_bytes;
    	bytes_left -= cur_bytes;
    	*bytes_used += cur_bytes;
    }

	/* Step 5: Make sure we have enough space to grab the value. */
	if(result->length > bytes_left)
	{
    	DTNMP_DEBUG_ERR("val_deserialize","Bytes left %d, value length %d!", bytes_left, result->length);

    	MRELEASE(result);
    	*bytes_used = 0;

        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
	}

	/* Step 6: Based on the type, grab the value. */

	/* Step 3c: Copy in the val. */
	switch(result->type)
	{
		case DTNMP_TYPE_BYTE:
			result->value.as_int = (uint32_t) cursor[0];
			cur_bytes = 1;
			break;

		case DTNMP_TYPE_INT:
			result->value.as_int = val_deserialize_int(cursor, bytes_left, &cur_bytes);
			break;

			// \todo: TS should be a UINT not a SDNV...
		case DTNMP_TYPE_TS:
		case DTNMP_TYPE_UINT:
			result->value.as_uint = val_deserialize_uint(cursor, bytes_left, &cur_bytes);
			break;

		case DTNMP_TYPE_VAST:
			result->value.as_vast = val_deserialize_vast(cursor, bytes_left, &cur_bytes);
			break;

		case DTNMP_TYPE_SDNV:
		case DTNMP_TYPE_UVAST:
			result->value.as_uvast = val_deserialize_uvast(cursor, bytes_left, &cur_bytes);
			break;

		case DTNMP_TYPE_REAL32:
			result->value.as_real32 = val_deserialize_real32(cursor, bytes_left, &cur_bytes);
			break;

		case DTNMP_TYPE_REAL64:
			result->value.as_real64 = val_deserialize_real64(cursor, bytes_left, &cur_bytes);
			break;

		case DTNMP_TYPE_STRING:
			result->value.as_ptr = (uint8_t *) val_deserialize_string(cursor, bytes_left, &cur_bytes);
			result->length = strlen((char *) result->value.as_ptr);
			break;

		case DTNMP_TYPE_BLOB:
		case DTNMP_TYPE_DC:
		case DTNMP_TYPE_MID:
		case DTNMP_TYPE_MC:
		case DTNMP_TYPE_EXPR:
		case DTNMP_TYPE_DEF:
		case DTNMP_TYPE_TRL:
		case DTNMP_TYPE_SRL:
			result->value.as_ptr = val_deserialize_blob(cursor, bytes_left, &cur_bytes, &(result->length));
			break;

		case DTNMP_TYPE_UNK:
		default:
			cur_bytes = 0;
	}

	if(cur_bytes == 0)
	{
	   	DTNMP_DEBUG_ERR("val_deserialize","Can't deserialize item of type %d.", result->type);
	   	MRELEASE(result);
    	*bytes_used = 0;
	   	return NULL;
	}

	cursor += cur_bytes;
	*bytes_used = *bytes_used + cur_bytes;


	/* Step 7: Return the value. */
    DTNMP_DEBUG_EXIT("val_deserialize","-> 0x%lx", (unsigned long) result);
    return result;
}


/*
 * Blob is an SDNV length then the bytes of data...
 */
uint8_t* val_deserialize_blob(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used, uint32_t *data_len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = buffer;
	uint32_t cur_bytes = 0;
	uvast slen = 0;

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (bytes_used == NULL) || (data_len == NULL))
	{
		DTNMP_DEBUG_ERR("val_deserialize_blob","Bad Args", NULL);

		return NULL;
	}

	/* Step 1: Grab the length in bytes as an sdnv. */
	if((cur_bytes = utils_grab_sdnv(cursor, bytes_left, &(slen))) == 0)
    {
    	DTNMP_DEBUG_ERR("val_deserialize_blob","Can't grab # items.", NULL);
    	*bytes_used = 0;
        return NULL;
    }
	*data_len = (uint32_t) slen;

	cursor += cur_bytes;
	bytes_left -= cur_bytes;
	*bytes_used += cur_bytes;

	/* Step 2: Allocate the data. */
	if((result = (uint8_t *) MTAKE(*data_len)) == NULL)
	{
    	DTNMP_DEBUG_ERR("val_deserialize_blob","Can't allocate %d bytes.", *data_len);
    	*bytes_used = 0;
        return NULL;
	}

	/* Step 3; Copy the data. */
	memcpy(result, cursor, *data_len);

	return result;
}



int32_t  val_deserialize_int(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	return (int32_t) val_deserialize_uint(buffer, bytes_left, bytes_used);
}


/******************************************************************************
 *
 * \par Function Name: val_deserialize_one
 *
 * \par Extracts a value from a byte buffer holding this one value.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized value.
 *
 * \param[in]  buffer         The byte buffer holding the value data
 * \param[in]  bytes_left     The # bytes available in the buffer
 *****************************************************************************/

value_t*  val_deserialize_one(unsigned char *buffer, uint32_t bytes_left)
{
	uint32_t bytes_used = 0;
	return val_deserialize(buffer, bytes_left, &bytes_used);
}



float    val_deserialize_real32(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	/* \todo Implement. */
	DTNMP_DEBUG_ERR("val_deserialize_real32","Not implemented.",NULL);

	*bytes_used = 0;
	return 0;
}

double   val_deserialize_real64(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	/* \todo Implement. */
	DTNMP_DEBUG_ERR("val_deserialize_real64","Not implemented.",NULL);

	*bytes_used = 0;
	return 0;
}

char*    val_deserialize_string(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	uint32_t blob_len = 0;
	uint8_t *blob = NULL;
	char *result = NULL;

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("val_deserialize_string","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Grab the value as a blob. */
	if((blob = val_deserialize_blob(buffer, bytes_left, bytes_used, &blob_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_deserialize_string","Can't get blob.", NULL);
		*bytes_used = 0;
		return NULL;
	}

	/* Step 2: Allocate string which is 1 extra character (null terminator). */
	if((result = (char *) MTAKE(blob_len + 1)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_deserialize_string","Can't allocate %d bytes.", blob_len + 1);
		MRELEASE(blob);
		*bytes_used = 0;
		return NULL;
	}

	memcpy(result, blob, blob_len);
	result[blob_len] = '\0';

	MRELEASE(blob);
	return result;
}

uint32_t val_deserialize_uint(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	uint32_t result = 0;

	/* Step 0: Sanity check. */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("val_deserialize_uint","Bad Args.", NULL);
		if(bytes_used != NULL)
		{
			*bytes_used = 0;
		}
		return 0;
	}

	// + 1 for length byte.
	if(bytes_left < sizeof(uint32_t))
	{
		DTNMP_DEBUG_ERR("val_deserialize_uint","Buffer size %d too small for %d.", bytes_left, sizeof(uint32_t));
		*bytes_used = 0;
		return 0;
	}

	/* Step 1: Populate the uint32_t. */

	result |= buffer[0] << 24;
	result |= buffer[1] << 16;
	result |= buffer[2] << 8;
	result |= buffer[3];

	*bytes_used = 4;

	/* Step 2: Check byte order. */
	result = ntohl(result);

	return result;
}

uvast    val_deserialize_uvast(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{

	uvast result = 0;

	/* Step 0: Sanity check. */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("val_deserialize_uvast","Bad Args.", NULL);
		if(bytes_used != NULL)
		{
			*bytes_used = 0;
		}
		return 0;
	}

	if(bytes_left < sizeof(uvast))
	{
		DTNMP_DEBUG_ERR("val_deserialize_uvast","Buffer size %d too small for %d.", bytes_left, sizeof(uvast));
		*bytes_used = 0;
		return 0;
	}

	/* Step 1: Populate the uvast. */
	uvast tmp;

	tmp = buffer[0]; tmp <<= 56;
	result |= tmp;

	tmp = buffer[1]; tmp <<= 48;
	result |= tmp;

	tmp = buffer[2]; tmp <<= 40;
	result |= tmp;

	tmp = buffer[3]; tmp <<= 32;
	result |= tmp;

	tmp = buffer[4]; tmp <<= 24;
	result |= tmp;

	tmp = buffer[5]; tmp <<= 16;
	result |= tmp;

	tmp = buffer[6]; tmp <<= 8;
	result |= tmp;

	result |= buffer[7];

	*bytes_used = 8;

	/* Step 2: Check byte order. */
	result = ntohv(result);

	return result;
}

vast     val_deserialize_vast(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used)
{
	return (vast) val_deserialize_uvast(buffer, bytes_left, bytes_used);
}


/*
 * Must release result...
 */
value_t  val_from_string(char *str)
{
	value_t result;
	uint32_t len;

	DTNMP_DEBUG_ENTRY("val_from_string","(0x%lx)", (unsigned long) str);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_ptr = NULL;

	/* Step 1 - Sanity Check. */
	if(str == NULL)
	{
        DTNMP_DEBUG_ERR("val_from_string","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("val_from_string","-> Unk", NULL);
        return result;
	}

	/* Step 2 - Find out length of string. */
	len = strlen(str) + 1;

	/* Step 3 - Allocate storage for the string. */
	if((result.value.as_ptr = MTAKE(len)) == NULL)
	{
        DTNMP_DEBUG_ERR("val_from_string","Can't allocate %d bytes.",len);
        DTNMP_DEBUG_EXIT("val_from_string","-> Unk", NULL);
        return result;
	}

	/* Step 4 - Populate the return value. */
	result.length = strlen(str) + 1;
	result.type = DTNMP_TYPE_STRING;
	memcpy(result.value.as_ptr,str,result.length);

	DTNMP_DEBUG_EXIT("val_from_string","-> %s", str);
	return result;
}

/*
 * optype: 0 - Arithmetic
 *         1 - Logic
 */
int val_get_result_type(int ltype, int rtype, int optype)
{

	if(optype == 1)
	{
		return DTNMP_TYPE_UINT;
	}
	else if ((ltype > 8) || (rtype > 8))
	{
		return DTNMP_TYPE_UNK;
	}

	return gValResultType[ltype][rtype];
}


void     val_init(value_t *val)
{
	if(val != NULL)
	{
		val->type = DTNMP_TYPE_UNK;
		val->length = 0;
		val->value.as_uvast = 0;
	}
}


/******************************************************************************
 *
 * \par Function Name: val_is_int
 *
 * \par Returns whether value is a type of integer or not.
 *
 * \retval 0  - The value type is not an integer
 *         !0 - The value is a type of integer
 *
 * \param[in/out] val  The structure whose value is being evaluated.
 *
 *****************************************************************************/

int val_is_int(value_t *val)
{
	int result = 0;

	DTNMP_DEBUG_ENTRY("val_is_int","(0x%lx)", (unsigned long) val);

	DTNMP_DEBUG_INFO("val_is_int","Type: %d", val->type);

	switch(val->type)
	{
		case DTNMP_TYPE_INT:
		case DTNMP_TYPE_UINT:
		case DTNMP_TYPE_VAST:
		case DTNMP_TYPE_UVAST:
			result = 1;
			break;
		default:
	        result = 0;
	        break;
	}

	DTNMP_DEBUG_EXIT("val_is_int","-> %d", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: val_is_real
 *
 * \par Returns whether value is a type of real or not.
 *
 * \retval 0  - The value type is not a real
 *         !0 - The value is a type of real
 *
 * \param[in/out] val  The structure whose value is being evaluated.
 *
 *****************************************************************************/

int val_is_real(value_t *val)
{
	int result = 0;

	DTNMP_DEBUG_ENTRY("val_is_real","(0x%lx)", (unsigned long) val);

	DTNMP_DEBUG_INFO("val_is_real","Type: %d", val->type);

	switch(val->type)
	{
		case DTNMP_TYPE_REAL32:
		case DTNMP_TYPE_REAL64:
			result = 1;
			break;
		default:
	        result = 0;
	        break;
	}

	DTNMP_DEBUG_EXIT("val_is_real","-> %d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: val_release
 *
 * \par Frees a value structure.
 *
 * \param[in/out] val  The structure to be released.
 *
 *****************************************************************************/

void val_release(value_t *val)
{
	DTNMP_DEBUG_ENTRY("val_release","(0x%lx)", (unsigned long) val);

	if(val != NULL)
	{
		if((val->type == DTNMP_TYPE_BLOB)   ||
		   (val->type == DTNMP_TYPE_STRING) ||
		   (val->type == DTNMP_TYPE_DC)     ||
		   (val->type == DTNMP_TYPE_MID)    ||
		   (val->type == DTNMP_TYPE_MC)     ||
		   (val->type == DTNMP_TYPE_EXPR)   ||
		   (val->type == DTNMP_TYPE_DEF)    ||
		   (val->type == DTNMP_TYPE_TRL)    ||
		   (val->type == DTNMP_TYPE_SRL))
		{
			if(val->value.as_ptr != NULL)
			{
				MRELEASE(val->value.as_ptr);
			}
		}
		MRELEASE(val);
	}

	DTNMP_DEBUG_EXIT("val_release",".", NULL);
}



/******************************************************************************
 *
 * \par Function Name: val_serialize
 *
 * \par Purpose: Returns a serialized version of a value
 *
 * \retval  NULL - Failure
 *         !NULL - The serialized value.
 *
 * \param[in]   val   The value being serialized
 * \param[out]  size  The # bytes of the serialized value.
 *
 * \par Notes:
 *		1. The serialized version of the value exists on the memory pool and must be
 *         released when finished.
 *
 *8/6/2015 E. Birrane Added use_type
 *****************************************************************************/

uint8_t* val_serialize(value_t *val, uint32_t *size, int use_type)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	uint8_t *data = NULL;
	uint32_t data_size = 0;
	Sdnv length;
	uint8_t no_free = 0;
	uint8_t byte_data = 0;

	DTNMP_DEBUG_ENTRY("val_serialize","(0x%lx, 0x%lx)",
			          (unsigned long) val, (unsigned long) size);

	/* Step 0: Sanity Check. */
	if((val == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("val_serialize","Bad args.", NULL);
		DTNMP_DEBUG_EXIT("val_serialize","->NULL", NULL);
		return NULL;
	}

	/* Step 1: Encode the length as an SDNV. */
	encodeSdnv(&length, val->length);

	/* Step 3c: Copy in the val. */
	switch(val->type)
	{
		case DTNMP_TYPE_BYTE:
			byte_data = (uint8_t) val->value.as_uint;
			data = (uint8_t*) &(byte_data);
			data_size = 1;
			no_free = 1; // Note that data here should not be freed.
			break;
		case DTNMP_TYPE_INT:
			data = val_serialize_int(val->value.as_int, &data_size);
			break;
// \todo: TS should be a UINT not a SDNV...
		case DTNMP_TYPE_TS:
		case DTNMP_TYPE_UINT:
			data = val_serialize_uint(val->value.as_uint, &data_size);
			break;

		case DTNMP_TYPE_VAST:
			data = val_serialize_vast(val->value.as_vast, &data_size);
			break;

		case DTNMP_TYPE_SDNV:
		case DTNMP_TYPE_UVAST:
			data = val_serialize_uvast(val->value.as_uvast, &data_size);
			break;

		case DTNMP_TYPE_REAL32:
			data = val_serialize_real32(val->value.as_real32, &data_size);
			break;

		case DTNMP_TYPE_REAL64:
			data = val_serialize_real64(val->value.as_real64, &data_size);
			break;

    	/* In these cases, we assume the ptr already contains the serialized object. */

		case DTNMP_TYPE_STRING:
		case DTNMP_TYPE_BLOB:
		case DTNMP_TYPE_DC:
		case DTNMP_TYPE_MID:
		case DTNMP_TYPE_MC:
		case DTNMP_TYPE_EXPR:
		case DTNMP_TYPE_DEF:
		case DTNMP_TYPE_TRL:
		case DTNMP_TYPE_SRL:
			data = val->value.as_ptr;
			data_size = val->length;
			no_free = 1; // Note that data here should not be freed.

			break;

		default:
			DTNMP_DEBUG_ERR("val_serialize","Can't serialize value of this type.", *size);
			*size = 0;
			MRELEASE(result);
			DTNMP_DEBUG_EXIT("val_serialize","->NULL", NULL);
			return NULL;

	}

	if((data == NULL) || (data_size == 0))
	{
		DTNMP_DEBUG_ERR("val_serialize","Can't get serialized data.", NULL);
		*size = 0;
		return NULL;
	}

	/* Step 2: Calculate the overall length of the serialized value. */
	*size = 1 + length.length + data_size;

	/* Don't count type is we are not using it. */
	if(use_type == 0)
	{
		*size = *size - 1;
	}

	/* Step 3: Allocate result. */
	if((result = (uint8_t*) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize","Can't alloc %d bytes.", *size);
		*size = 0;
		DTNMP_DEBUG_EXIT("val_serialize","->NULL", NULL);
		return NULL;
	}

	cursor = result;

	/* Step 3a: Copy in the value type. */
	if(use_type != 0)
	{
		memcpy(cursor, &(val->type), 1);
		cursor += 1;
	}

	/* Step 3b: Copy in the value length as an SDNV. */
	memcpy(cursor, length.text, length.length);
	cursor += length.length;

	memcpy(cursor, data, data_size);
	cursor += data_size;

	if(no_free == 0)
	{
		MRELEASE(data);
	}

	DTNMP_DEBUG_EXIT("val_serialize","->0x%lx", (unsigned long)result);
	return result;
}


uint8_t *val_serialize_blob(uint8_t *value, uint32_t value_size, uint32_t *size)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	Sdnv length;

	/* Step 0: Sanity Check. */
	if((value == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("val_serialize_blob", "Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Encode the size as an SDNV. */
	encodeSdnv(&length, value_size);

	/* Step 2: Calculate the overall length of the serialized value. */
	*size = length.length + value_size;

	/* Step 3: Allocate the new buffer. */
	if((result = (uint8_t *) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize_blob", "Can't allocate %d bytes.", *size);
		*size = 0;
		return NULL;
	}
	cursor = result;

	/* Step 4: Populate the result. */
	memcpy(cursor, length.text, length.length);
	cursor += length.length;

	memcpy(cursor, value, value_size);
	cursor += value_size;

	return result;
}

uint8_t *val_serialize_byte(uint8_t value, uint32_t *size)
{
	uint8_t *result = NULL;

	*size = 1;
	if((result = (uint8_t *) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize_byte","Cannot grab memory.", NULL);
		return NULL;
	}

	result[0] = value;

	return result;
}

uint8_t *val_serialize_int(int32_t value, uint32_t *size)
{
	return val_serialize_uint((uint32_t) value, size);
}


uint8_t* val_serialize_raw(value_t *val, uint32_t *size)
{
	uint8_t *data = NULL;

	if((val == NULL) || (size == NULL))
	{
		return NULL;
	}

	switch(val->type)
	{
		case DTNMP_TYPE_BYTE:
			data = val_serialize_byte(val->value.as_int, size);
			break;

		case DTNMP_TYPE_INT:
			data = val_serialize_int(val->value.as_int, size);
			break;

		case DTNMP_TYPE_TS:
		case DTNMP_TYPE_UINT:
			data = val_serialize_uint(val->value.as_uint, size);
			break;

		case DTNMP_TYPE_VAST:
			data = val_serialize_vast(val->value.as_vast, size);
			break;

		case DTNMP_TYPE_UVAST:
			data = val_serialize_uvast(val->value.as_uvast, size);
			break;

		case DTNMP_TYPE_REAL32:
			data = val_serialize_real32(val->value.as_real32, size);
			break;

		case DTNMP_TYPE_REAL64:
			data = val_serialize_real64(val->value.as_real64, size);
			break;

		case DTNMP_TYPE_STRING:
			data = val_serialize_string(val->value.as_ptr, size);
			break;

		case DTNMP_TYPE_SDNV:
		case DTNMP_TYPE_DC:
		case DTNMP_TYPE_MID:
		case DTNMP_TYPE_MC:
		case DTNMP_TYPE_EXPR:
		case DTNMP_TYPE_BLOB:
			data = val_serialize_blob(val->value.as_ptr, val->length, size);
			break;
	}

	return data;
}

uint8_t *val_serialize_real32(float value, uint32_t *size)
{
	/* \todo Implement. */
	DTNMP_DEBUG_ERR("val_serialize_real32","Not implemented.",NULL);

	return NULL;
}

uint8_t *val_serialize_real64(double value, uint32_t *size)
{
	/* \todo Implement. */
	DTNMP_DEBUG_ERR("val_serialize_real64","Not implemented.",NULL);

	return NULL;
}

uint8_t *val_serialize_string(char *value, uint32_t *size)
{
	if((value == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("val_serialize_string","Bad Args", NULL);
		return NULL;
	}

	return val_serialize_blob((uint8_t *)value, (uint32_t) strlen(value), size);
}

uint8_t *val_serialize_uint(uint32_t value, uint32_t *size)
{
	uint8_t *result = NULL;
	uint32_t tmp;

	if(size == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize_uint", "Bad Args.", NULL);
		return NULL;
	}

	if((result = (uint8_t *) MTAKE(sizeof(uint32_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize_uint", "Can't allocate %d bytes", sizeof(uint32_t));
		return NULL;
	}

	tmp = htonl(value);
	result[0] = (tmp >> 24) & (0xFF);
	result[1] = (tmp >> 16) & (0xFF);
	result[2] = (tmp >> 8) & (0xFF);
	result[3] = tmp & (0xFF);

	*size = 4;

	return result;
}


uint8_t *val_serialize_uvast(uvast value, uint32_t *size)
{
	uint8_t *result = NULL;
	uvast tmp;

	if(size == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize_uvast", "Bad Args.", NULL);
		return NULL;
	}

	if(sizeof(uvast) != 8)
	{
		DTNMP_DEBUG_ERR("val_serialize_uvast","uvast isn't size 8?", NULL);
		return NULL;
	}

	if((result = (uint8_t *) MTAKE(sizeof(uvast))) == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize_uvast", "Can't allocate %d bytes", sizeof(uvast));
		return NULL;
	}

	tmp = htonv(value);

	result[0] = (tmp >> 56) & (0xFF);
	result[1] = (tmp >> 48) & (0xFF);
	result[2] = (tmp >> 40) & (0xFF);
	result[3] = (tmp >> 32) & (0xFF);
	result[4] = (tmp >> 24) & (0xFF);
	result[5] = (tmp >> 16) & (0xFF);
	result[6] = (tmp >> 8) & (0xFF);
	result[7] = tmp & (0xFF);

	*size = 8;

	return result;
}


uint8_t *val_serialize_vast(vast value, uint32_t *size)
{
	return val_serialize_uvast((uvast) value, size);
}





/******************************************************************************
 *
 * \par Function Name: val_to_string
 *
 * \par Generate a string representation of a value in the system..
 *
 * \retval NULL Failure
 *        !NULL The string representation.
 *
 * \param[in] val   The value being converted to string.
 *
 * \par Notes:
 *		1. The string version of a value is "(type) data [size]" where data
 *		   is the hex value of the value as a uvast. This is the actual value
 *		   for most values and simply the pointer for strings/blobs.
 *		2. The returned string must be released by the caller
 *		3. The maximum size of this string is given as follows:
 *		   "(unknown) "          - 10 bytes
 *         "0xFFFFFFFFFFFFFFFF " - 19 bytes
 *         "[XXXXXXXXXX]"        - 12 bytes
 *         \0                    - 1 byte null terminator
 *                               = 42 bytes
 *
 *****************************************************************************/

char *val_to_string(value_t *val)
{
	char *result = NULL;
	const char *type_str = NULL;
	uint32_t size = 42;
	char *cursor = NULL;


	DTNMP_DEBUG_ENTRY("val_to_string","(0x%lx)", (unsigned long) val);

	/* Step 0: Sanity check. */
	if(val == NULL)
	{
		DTNMP_DEBUG_ERR("val_to_string", "Bas Args.", NULL);
		DTNMP_DEBUG_EXIT("val_to_string", "->NULL", NULL);
		return NULL;
	}

	/* Step 1: Allocate the string. */
	if((result = (char*)MTAKE(size)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_to_string", "Can't alloc %d bytes.", size);
		DTNMP_DEBUG_EXIT("val_to_string","->NULL",NULL);
		return NULL;
	}

	if((type_str = type_to_str(val->type)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_to_string","Can't get type string.", NULL);
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("val_to_string","-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Populate the allocated string. Keep an eye on the size. */
	cursor = result;

	sprintf(cursor,"%s ", type_str);

	cursor += sprintf(cursor,UVAST_FIELDSPEC, val->value.as_uvast);
	cursor += sprintf(cursor,"[%d]", val->length);

	/* Step 3: Sanity check. */
	if((cursor - result) > size)
	{
		DTNMP_DEBUG_ERR("val_to_string", "OVERWROTE! Alloc %d, wrote %llu.",
				        size, (cursor-result));
		MRELEASE(result);
		DTNMP_DEBUG_EXIT("val_to_string","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_INFO("val_to_string","Wrote %llu into %d string.",
			         (cursor -result), size);

	DTNMP_DEBUG_EXIT("val_to_string","->0x%lx",(unsigned long) result);
	return result;
}

void valcol_destroy(Lyst *valcol)
{
	LystElt elt;
	value_t *tmp;

	if(valcol == NULL)
	{
		return;
	}

	for(elt = lyst_first(*valcol); elt; elt = lyst_next(elt))
	{
		tmp = (value_t *) lyst_data(elt);
		val_release(tmp);
	}

	lyst_destroy(*valcol);
	*valcol = NULL;
}
