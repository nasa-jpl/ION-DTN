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
{VAL_TYPE_UNK,VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_INT,   VAL_TYPE_INT,   VAL_TYPE_VAST,  VAL_TYPE_UNK,   VAL_TYPE_REAL32,VAL_TYPE_REAL64,VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_INT,   VAL_TYPE_UINT,  VAL_TYPE_VAST,  VAL_TYPE_UVAST, VAL_TYPE_REAL32,VAL_TYPE_REAL64,VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_VAST,  VAL_TYPE_VAST,  VAL_TYPE_VAST,  VAL_TYPE_VAST,  VAL_TYPE_REAL32,VAL_TYPE_REAL64,VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_UNK,   VAL_TYPE_UVAST, VAL_TYPE_VAST,  VAL_TYPE_UVAST, VAL_TYPE_REAL32,VAL_TYPE_REAL64,VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_REAL32,VAL_TYPE_REAL32,VAL_TYPE_REAL32,VAL_TYPE_REAL32,VAL_TYPE_REAL32,VAL_TYPE_REAL64,VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_REAL64,VAL_TYPE_REAL64,VAL_TYPE_REAL64,VAL_TYPE_REAL64,VAL_TYPE_REAL64,VAL_TYPE_REAL64,VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK, VAL_TYPE_UNK},
{VAL_TYPE_UNK,VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK,   VAL_TYPE_UNK, VAL_TYPE_UNK}
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
		case VAL_TYPE_INT:
			result->value.as_int = val.value.as_int;
			break;
		case VAL_TYPE_UINT:
			result->value.as_uint = val.value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result->value.as_vast = val.value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result->value.as_uvast = val.value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result->value.as_real32 = val.value.as_real32;
			break;
		case VAL_TYPE_REAL64:
			result->value.as_real64 = val.value.as_real64;
			break;
		case VAL_TYPE_STRING:
		case VAL_TYPE_BLOB:
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
		case VAL_TYPE_INT:
			result = val->value.as_int;
			break;
		case VAL_TYPE_UINT:
			result = (int32_t) val->value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result = (int32_t) val->value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result = (int32_t) val->value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result = (int32_t) val->value.as_real32;
			break;
		case VAL_TYPE_REAL64:
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
		case VAL_TYPE_INT:
			result = (uint32_t) val->value.as_int;
			break;
		case VAL_TYPE_UINT:
			result = val->value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result = (uint32_t) val->value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result = (uint32_t) val->value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result = (uint32_t) val->value.as_real32;
			break;
		case VAL_TYPE_REAL64:
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
		case VAL_TYPE_INT:
			result = (vast) val->value.as_int;
			break;
		case VAL_TYPE_UINT:
			result = (vast) val->value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result = val->value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result = (vast) val->value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result = (vast) val->value.as_real32;
			break;
		case VAL_TYPE_REAL64:
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
		case VAL_TYPE_INT:
			result = (uvast) val->value.as_int;
			break;
		case VAL_TYPE_UINT:
			result = (uvast) val->value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result = (uvast) val->value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result =  val->value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result = (uvast) val->value.as_real32;
			break;
		case VAL_TYPE_REAL64:
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
		case VAL_TYPE_INT:
			result = (float) val->value.as_int;
			break;
		case VAL_TYPE_UINT:
			result = (float) val->value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result = (float) val->value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result = (float) val->value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result = (float) val->value.as_real32;
			break;
		case VAL_TYPE_REAL64:
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
		case VAL_TYPE_INT:
			result = (double) val->value.as_int;
			break;
		case VAL_TYPE_UINT:
			result = (double) val->value.as_uint;
			break;
		case VAL_TYPE_VAST:
			result = (double) val->value.as_vast;
			break;
		case VAL_TYPE_UVAST:
			result = (double) val->value.as_uvast;
			break;
		case VAL_TYPE_REAL32:
			result = (double) val->value.as_real32;
			break;
		case VAL_TYPE_REAL64:
			result = (double) val->value.as_real64;
			break;
		default:
			break;
	}

	return result;
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
 * \param[in]  buffer_size    The # bytes available in the buffer
 * \param[in/out] bytes_used  The # of bytes consumed in the deserialization.
 *
 *****************************************************************************/

value_t*  val_deserialize(unsigned char *buffer,
		                 uint32_t buffer_size,
		                 uint32_t *bytes_used)
{
    value_t *result = NULL;
    unsigned char *cursor = NULL;
    uint32_t cur_bytes = 0;
    uint32_t bytes_left=0;
    uint32_t len = 0;

    DTNMP_DEBUG_ENTRY("val_deserialize","(%lx, %lu, %lx)",
                     (unsigned long) buffer,
                     (unsigned long) buffer_size,
                     (unsigned long) bytes_used);

    /* Step 1: Sanity checks. */
    if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
    {
        DTNMP_DEBUG_ERR("val_deserialize","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	*bytes_used = 0;
    	cursor = buffer;
    	bytes_left = buffer_size;
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
    if(utils_grab_byte(cursor, bytes_left, &(result->type)) != 1)
    {
    	DTNMP_DEBUG_ERR("val_deserialize","Can't grab type byte.", NULL);
    	MRELEASE(result);

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

        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
        return NULL;
	}

	/* Step 6: Based on the type, grab the value. */
	/* \todo: Watch network byte order here. */
	switch(result->type)
	{
		case VAL_TYPE_INT:
			result->value.as_int = 0;
			len = MIN(result->length, sizeof(result->value.as_int));
			memcpy(&(result->value.as_int), cursor, len);
			*bytes_used += len;
			break;
		case VAL_TYPE_UINT:
			result->value.as_uint = 0;
			len = MIN(result->length, sizeof(result->value.as_uint));
			memcpy(&(result->value.as_uint), cursor, len);
			*bytes_used += len;
			break;
		case VAL_TYPE_VAST:
			result->value.as_vast = 0;
			len = MIN(result->length, sizeof(result->value.as_vast));
			memcpy(&(result->value.as_vast), cursor, len);
			*bytes_used += len;
			break;
		case VAL_TYPE_UVAST:
			result->value.as_uvast = 0;
			len = MIN(result->length, sizeof(result->value.as_uvast));
			memcpy(&(result->value.as_uvast), cursor, len);
			*bytes_used += len;
			break;
		case VAL_TYPE_REAL32:
			result->value.as_real32 = 0;
			len = MIN(result->length, sizeof(result->value.as_real32));
			memcpy(&(result->value.as_real32), cursor, len);
			*bytes_used += len;
			break;
		case VAL_TYPE_REAL64:
			result->value.as_real64 = 0;
			len = MIN(result->length, sizeof(result->value.as_real64));
			memcpy(&(result->value.as_real64), cursor, len);
			*bytes_used += len;
			break;
		case VAL_TYPE_STRING:
		case VAL_TYPE_BLOB:

			if((result->value.as_ptr = MTAKE(result->length)) == NULL)
			{
		    	DTNMP_DEBUG_ERR("val_deserialize","Can't allocate %d bytes for type %d.", result->length, result->type);
		    	MRELEASE(result);

		        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
		        return NULL;
			}
			else
			{
				memcpy(&(result->value.as_ptr), cursor, result->length);
			}
			*bytes_used += result->length;
			break;
		case VAL_TYPE_UNK:
		default:
	    	DTNMP_DEBUG_ERR("val_deserialize","Unknown type %d.", result->type);
	    	MRELEASE(result);

	        DTNMP_DEBUG_EXIT("val_deserialize","-> NULL", NULL);
	        return NULL;
	}

	/* Step 7: Return the value. */
    DTNMP_DEBUG_EXIT("val_deserialize","-> 0x%lx", (unsigned long) result);
    return result;
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
 * \param[in]  buffer_size    The # bytes available in the buffer
 *****************************************************************************/

value_t*  val_deserialize_one(unsigned char *buffer,
		                     uint32_t buffer_size)
{
	uint32_t bytes_used = 0;
	return val_deserialize(buffer, buffer_size, &bytes_used);
}


/*
 * Must release result...
 */
value_t  val_from_string(char *str)
{
	value_t result;
	uint32_t len;

	DTNMP_DEBUG_ENTRY("val_from_string","(0x%lx)", (unsigned long) str);

	result.type = VAL_TYPE_UNK;
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
	len = strlen(str);

	/* Step 3 - Allocate storage for the string. */
	if((result.value.as_ptr = MTAKE(len)) == NULL)
	{
        DTNMP_DEBUG_ERR("val_from_string","Can't allocate %d bytes.",len);
        DTNMP_DEBUG_EXIT("val_from_string","-> Unk", NULL);
        return result;
	}

	/* Step 4 - Populate the return value. */
	result.length = strlen(str);
	result.type = VAL_TYPE_STRING;
	memcpy(&(result.value.as_ptr),str,result.length);

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
		return VAL_TYPE_UINT;
	}
	else if ((ltype > 8) || (rtype > 8))
	{
		return VAL_TYPE_UNK;
	}

	return gValResultType[ltype][rtype];
}

void     val_init(value_t *val)
{
	if(val != NULL)
	{
		val->type = VAL_TYPE_UNK;
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
		case VAL_TYPE_INT:
		case VAL_TYPE_UINT:
		case VAL_TYPE_VAST:
		case VAL_TYPE_UVAST:
			result = 1;
			break;
		case VAL_TYPE_REAL32:
		case VAL_TYPE_REAL64:
		case VAL_TYPE_STRING:
		case VAL_TYPE_BLOB:
		case VAL_TYPE_UNK:
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
		case VAL_TYPE_INT:
		case VAL_TYPE_UINT:
		case VAL_TYPE_VAST:
		case VAL_TYPE_UVAST:
			result = 0;
			break;
		case VAL_TYPE_REAL32:
		case VAL_TYPE_REAL64:
			result = 1;
			break;
		case VAL_TYPE_STRING:
		case VAL_TYPE_BLOB:
		case VAL_TYPE_UNK:
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
		if((val->type == VAL_TYPE_BLOB) || (val->type == VAL_TYPE_STRING))
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
 *****************************************************************************/

uint8_t* val_serialize(value_t *val, uint32_t *size)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	Sdnv length;

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

	/* Step 2: Calculate the overall length of the serialized value. */
	*size = 1 + length.length + val->length;

	/* Step 3: Allocate result. */
	if((result = (uint8_t*) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("val_serialize","Can't alloc %d bytes.", *size);
		*size = 0;
		DTNMP_DEBUG_EXIT("val_serialize","->NULL", NULL);
		return NULL;
	}

	/* Step 3a: Copy in the value type. */
	cursor = result;
	memcpy(cursor, &(val->type), 1);
	cursor += 1;

	/* Step 3b: Copy in the value length as an SDNV. */
	memcpy(cursor, length.text, length.length);
	cursor += length.length;

	/* Step 3c: Copy in the val. */
	switch(val->type)
	{
		case VAL_TYPE_INT:
			memcpy(cursor, &(val->value.as_int), val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_UINT:
			memcpy(cursor, &(val->value.as_uint), val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_VAST:
			memcpy(cursor, &(val->value.as_vast), val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_UVAST:
			memcpy(cursor, &(val->value.as_uvast), val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_REAL32:
			memcpy(cursor, &(val->value.as_real32), val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_REAL64:
			memcpy(cursor, &(val->value.as_real64), val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_STRING:
			memcpy(cursor, val->value.as_ptr, val->length);
			cursor += val->length;
			break;

		case VAL_TYPE_BLOB:
			memcpy(cursor, val->value.as_ptr, val->length);
			cursor += val->length;
			break;

	}

	DTNMP_DEBUG_EXIT("val_serialize","->0x%lx", (unsigned long)result);
	return result;
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

	/* Step 2: Populate the allocated string. Keep an eye on the size. */
	cursor = result;

	switch(val->type)
	{
	case VAL_TYPE_INT:    cursor += sprintf(cursor,"(int) "); break;
	case VAL_TYPE_UINT:   cursor += sprintf(cursor,"(uint) "); break;
	case VAL_TYPE_VAST:   cursor += sprintf(cursor,"(vast) "); break;
	case VAL_TYPE_UVAST:  cursor += sprintf(cursor,"(uvast) "); break;
	case VAL_TYPE_REAL32: cursor += sprintf(cursor,"(real32) "); break;
	case VAL_TYPE_REAL64: cursor += sprintf(cursor,"(real64) "); break;
	case VAL_TYPE_STRING: cursor += sprintf(cursor,"(string) "); break;
	case VAL_TYPE_BLOB:   cursor += sprintf(cursor,"(blob) "); break;
	default:              cursor += sprintf(cursor,"(Unknown) "); break;
	}

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
