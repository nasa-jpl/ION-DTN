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
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "ion.h"
#include "platform.h"

#include "../adm/adm.h"

#include "../primitives/value.h"
#include "../primitives/expr.h"
#include "../primitives/blob.h"
#include "../primitives/expr.h"
#include "../primitives/mid.h"
#include "../primitives/rules.h"
#include "../primitives/def.h"
#include "../primitives/tdc.h"
#include "../primitives/dc.h"
#include "../primitives/table.h"
#include "var.h"


int gValNumCvtResult[6][6] = {
{AMP_TYPE_INT,   AMP_TYPE_INT,   AMP_TYPE_VAST,  AMP_TYPE_UNK,   AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_INT,   AMP_TYPE_UINT,  AMP_TYPE_VAST,  AMP_TYPE_UVAST, AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_VAST,  AMP_TYPE_VAST,  AMP_TYPE_VAST,  AMP_TYPE_VAST,  AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_UNK,   AMP_TYPE_UVAST, AMP_TYPE_VAST,  AMP_TYPE_UVAST, AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64}
};



value_t  val_copy(value_t val)
{
	value_t result;

	result = val;

	switch(val.type)
	{
	case AMP_TYPE_VAR:    result.value.as_ptr = (uint8_t*) var_duplicate((var_t*)val.value.as_ptr); break;
	case AMP_TYPE_RPT:   result.value.as_ptr = (uint8_t*) def_duplicate((def_gen_t *)val.value.as_ptr); break;
	case AMP_TYPE_DC:    result.value.as_ptr = (uint8_t*) dc_copy((Lyst) val.value.as_ptr); break;
	case AMP_TYPE_TDC:   result.value.as_ptr = (uint8_t*) tdc_copy((tdc_t*)val.value.as_ptr); break;
	case AMP_TYPE_BLOB:  result.value.as_ptr = (uint8_t*) blob_copy((blob_t*)val.value.as_ptr); break;
	case AMP_TYPE_MACRO: result.value.as_ptr = (uint8_t*) midcol_copy((Lyst) val.value.as_ptr); break;
	case AMP_TYPE_MC:    result.value.as_ptr = (uint8_t*) midcol_copy((Lyst) val.value.as_ptr); break;
	case AMP_TYPE_MID:   result.value.as_ptr = (uint8_t*) mid_copy((mid_t*)val.value.as_ptr); break;
	case AMP_TYPE_STRING:result.value.as_ptr = adm_copy_string((char *)val.value.as_ptr, NULL); break;
//	case DTNMP_TYPE_TRL:   result.value.as_ptr = (uint8_t*) trl_copy((trl_t*)val.value.as_ptr); break;
	case AMP_TYPE_SRL:   result.value.as_ptr = (uint8_t*) srl_copy((srl_t*)val.value.as_ptr); break;
	case AMP_TYPE_EXPR:  result.value.as_ptr = (uint8_t *) expr_copy((expr_t*)val.value.as_ptr); break;
//	case DTNMP_TYPE_TABLE: result.value.as_ptr = (uint8_t *)table_copy((table_t*)val.value.as_ptr); break;
	default: break;
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
 * \param[out] success If the conversion succeeded (1) or not (0)
 *****************************************************************************/

int32_t  val_cvt_int(value_t val, uint8_t *success)
{
	int32_t result = 0;

	*success = 1;

	switch(val.type)
	{
	case AMP_TYPE_INT:    result = val.value.as_int; break;
	case AMP_TYPE_UINT:   result = (int32_t) val.value.as_uint; break;
	case AMP_TYPE_VAST:   result = (int32_t) val.value.as_vast; break;
	case AMP_TYPE_UVAST:  result = (int32_t) val.value.as_uvast; break;
	case AMP_TYPE_REAL32: result = (int32_t) val.value.as_real32; break;
	case AMP_TYPE_REAL64: result = (int32_t) val.value.as_real64; break;
	default: *success = 0; break;
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
 * \param[out] success If the conversion succeeded (1) or not (0)
 *****************************************************************************/

uint32_t  val_cvt_uint(value_t val, uint8_t *success)
{
	uint32_t result = 0;

	*success = 1;

	switch(val.type)
	{
	case AMP_TYPE_INT:    result = (uint32_t) val.value.as_int; break;
	case AMP_TYPE_UINT:   result = val.value.as_uint; break;
	case AMP_TYPE_VAST:   result = (uint32_t) val.value.as_vast; break;
	case AMP_TYPE_UVAST:	result = (uint32_t) val.value.as_uvast; break;
	case AMP_TYPE_REAL32:	result = (uint32_t) val.value.as_real32; break;
	case AMP_TYPE_REAL64:	result = (uint32_t) val.value.as_real64; break;
	default: *success = 0; break;
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
 * \param[out] success If the conversion succeeded (1) or not (0)
 *****************************************************************************/

vast  val_cvt_vast(value_t val, uint8_t *success)
{
	vast result = 0;

	*success = 1;
	switch(val.type)
	{
	case AMP_TYPE_INT:   result = (vast) val.value.as_int; break;
	case AMP_TYPE_UINT:   result = (vast) val.value.as_uint; break;
	case AMP_TYPE_VAST:   result = val.value.as_vast; break;
	case AMP_TYPE_UVAST:  result = (vast) val.value.as_uvast; break;
	case AMP_TYPE_REAL32: result = (vast) val.value.as_real32; break;
	case AMP_TYPE_REAL64: result = (vast) val.value.as_real64; break;
	default: *success = 0; break;
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
 * \param[out] success If the conversion succeeded (1) or not (0)
 *
 *****************************************************************************/

uvast  val_cvt_uvast(value_t val, uint8_t *success)
{
	uvast result = 0;

	*success = 1;

	switch(val.type)
	{
	case AMP_TYPE_INT:    result = (uvast) val.value.as_int; break;
	case AMP_TYPE_UINT:   result = (uvast) val.value.as_uint; break;
	case AMP_TYPE_VAST:   result = (uvast) val.value.as_vast; break;
	case AMP_TYPE_UVAST:  result =  val.value.as_uvast; break;
	case AMP_TYPE_REAL32: result = (uvast) val.value.as_real32; break;
	case AMP_TYPE_REAL64: result = (uvast) val.value.as_real64; break;
	default: *success = 0; break;
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
 * \param[out] success If the conversion succeeded (1) or not (0)
 *
 *****************************************************************************/

float  val_cvt_real32(value_t val, uint8_t *success)
{
	float result = 0;

	*success = 1;

	switch(val.type)
	{
	case AMP_TYPE_INT:   result = (float) val.value.as_int; break;
	case AMP_TYPE_UINT:  result = (float) val.value.as_uint; break;
	case AMP_TYPE_VAST:  result = (float) val.value.as_vast; break;
	case AMP_TYPE_UVAST: result = (float) val.value.as_uvast; break;
	case AMP_TYPE_REAL32:result = (float) val.value.as_real32; break;
	case AMP_TYPE_REAL64:result = (float) val.value.as_real64; break;
	default: *success = 0; break;
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

double  val_cvt_real64(value_t val, uint8_t *success)
{
	double result = 0;

	*success = 1;

	switch(val.type)
	{
	case AMP_TYPE_INT:   result = (double) val.value.as_int; break;
	case AMP_TYPE_UINT:  result = (double) val.value.as_uint; break;
	case AMP_TYPE_VAST:  result = (double) val.value.as_vast; break;
	case AMP_TYPE_UVAST: result = (double) val.value.as_uvast; break;
	case AMP_TYPE_REAL32:result = (double) val.value.as_real32; break;
	case AMP_TYPE_REAL64:result = (double) val.value.as_real64; break;
	default: *success = 0; break;
	}

	return result;
}

// 0 ERROR
// 1 OK
uint8_t val_cvt_type(value_t *val, amp_type_e type)
{
	uint8_t success = 0;

	if(((val == NULL) || type == AMP_TYPE_UNK))
	{
		return 0;
	}

	if(val->type == type)
	{
		return 1;
	}

	if((type_is_numeric(type) == 0) ||
	   (type_is_numeric(val->type) == 0))
	{
		AMP_DEBUG_ERR("val_cvt_type","Can't cvt from %d to %d.", val->type, type);
		return 0;
	}

	success = 1;
	val->type = type;
	switch(val->type)
	{
	case AMP_TYPE_INT:    val->value.as_int = val_cvt_int(*val, &success); break;
	case AMP_TYPE_UINT:   val->value.as_uint = val_cvt_uint(*val, &success); break;
	case AMP_TYPE_VAST:	val->value.as_vast = val_cvt_vast(*val, &success); break;
	case AMP_TYPE_UVAST:	val->value.as_uvast = val_cvt_uvast(*val, &success); break;
	case AMP_TYPE_REAL32:	val->value.as_real32 = val_cvt_real32(*val, &success); break;
	case AMP_TYPE_REAL64:	val->value.as_real64 = val_cvt_real64(*val, &success); break;
	default: success = 0; break;
	}

	return success;
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

value_t  val_deserialize(unsigned char *buffer,
		                 uint32_t bytes_left,
		                 uint32_t *bytes_used)
{
    value_t result;
    unsigned char *cursor = NULL;
    uint32_t cur_bytes = 0;
    uvast len;


    AMP_DEBUG_ENTRY("val_deserialize","(%lx, %lu, %lx)",
                     (unsigned long) buffer,
                     (unsigned long) bytes_left,
                     (unsigned long) bytes_used);

    val_init(&result);

    /* Step 1: Sanity checks. */
    if((buffer == NULL) || (bytes_left == 0) || (bytes_used == NULL))
    {
        AMP_DEBUG_ERR("val_deserialize","Bad params.",NULL);
        return result;
    }
    else
    {
    	*bytes_used = 0;
    	cursor = buffer;
    }

    /* Step 3: Grab the value type */
    if(utils_grab_byte(cursor, bytes_left, (uint8_t*)&(result.type)) != 1)
    {
    	AMP_DEBUG_ERR("val_deserialize","Can't grab type byte.", NULL);
    	*bytes_used = 0;

        return result;
    }
    else
    {
    	cursor += 1;
    	bytes_left -= 1;
    	*bytes_used += 1;
    }

    if((cur_bytes = utils_grab_sdnv(cursor, bytes_left, &len)) == 0)
    {
    	AMP_DEBUG_ERR("val_deserialize","Can't parse SDNV.", NULL);
    	*bytes_used = 0;
    	return result;
    }
    else
    {
    	cursor += cur_bytes;
    	bytes_left -= cur_bytes;
    	*bytes_used += cur_bytes;
    }


	/* Step 4: deserialize value based on type. */
	switch(result.type)
	{
	case AMP_TYPE_VAR:     result.value.as_ptr = (uint8_t*) var_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_RPT:    result.value.as_ptr = (uint8_t*) def_deserialize_gen(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_SRL:    result.value.as_ptr = (uint8_t *)srl_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TRL:    result.value.as_ptr = (uint8_t *)trl_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_MC:     result.value.as_ptr = (uint8_t *)midcol_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_MACRO:  result.value.as_ptr = (uint8_t *)midcol_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_BYTE:   result.value.as_int = (uint32_t) cursor[0]; cur_bytes = 1; break;
	case AMP_TYPE_INT:    result.value.as_int = utils_deserialize_int(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TS:     result.value.as_uint = utils_deserialize_uint(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_UINT:   result.value.as_uint = utils_deserialize_uint(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_VAST:   result.value.as_vast = utils_deserialize_vast(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_UVAST:  result.value.as_uvast = utils_deserialize_uvast(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_REAL32:	result.value.as_real32 = utils_deserialize_real32(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_REAL64:	result.value.as_real64 = utils_deserialize_real64(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_STRING:	result.value.as_ptr = (uint8_t *) utils_deserialize_string(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_BLOB:   result.value.as_ptr = (uint8_t*) blob_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_MID:    result.value.as_ptr = (uint8_t *)mid_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_EXPR:   result.value.as_ptr = (uint8_t *)expr_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_DC:     result.value.as_ptr = (uint8_t *) dc_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TDC:    result.value.as_ptr = (uint8_t *) tdc_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TABLE:  result.value.as_ptr = (uint8_t *) table_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_EDD:
	case AMP_TYPE_CTRL:
	case AMP_TYPE_LIT:
	case AMP_TYPE_OP:
	case AMP_TYPE_SDNV:
	default: cur_bytes = 0; break;
	}

	if((cur_bytes == 0) || (cur_bytes != len))
	{
	   	AMP_DEBUG_ERR("val_deserialize","Can't deserialize item of type %d.", result.type);
    	*bytes_used = 0;
    	val_init(&result);
        return result;
	}

	cursor += cur_bytes;
	*bytes_used = *bytes_used + cur_bytes;


	/* Step 5: Return the value. */
    AMP_DEBUG_EXIT("val_deserialize","-> %d", result.type);
    return result;
}

/* Must release result */
value_t  val_from_blob(blob_t *blob, amp_type_e type)
{
	value_t result;
	uint32_t bytes_left = 0;
	uint32_t cur_bytes = 0;
	uint8_t *cursor = NULL;
	result.type = AMP_TYPE_UNK;

	if(blob == NULL)
	{
		return result;
	}

	result.type = type;
	bytes_left = blob->length;
	cursor = blob->value;

	switch(result.type)
	{
	case AMP_TYPE_VAR:     result.value.as_ptr = (uint8_t*) var_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_RPT:    result.value.as_ptr = (uint8_t*) def_deserialize_gen(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_SRL:    result.value.as_ptr = (uint8_t *)srl_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TRL:    result.value.as_ptr = (uint8_t *)trl_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_MC:     result.value.as_ptr = (uint8_t *)midcol_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_MACRO:  result.value.as_ptr = (uint8_t *)midcol_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_BYTE:   result.value.as_int = (uint32_t) cursor[0]; cur_bytes = 1; break;
	case AMP_TYPE_INT:    result.value.as_int = utils_deserialize_int(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TS:     result.value.as_uint = utils_deserialize_uint(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_UINT:   result.value.as_uint = utils_deserialize_uint(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_VAST:   result.value.as_vast = utils_deserialize_vast(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_UVAST:  result.value.as_uvast = utils_deserialize_uvast(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_REAL32:	result.value.as_real32 = utils_deserialize_real32(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_REAL64:	result.value.as_real64 = utils_deserialize_real64(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_STRING:	result.value.as_ptr = (uint8_t *) utils_deserialize_string(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_BLOB:   result.value.as_ptr = (uint8_t*) blob_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_MID:    result.value.as_ptr = (uint8_t *)mid_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_EXPR:   result.value.as_ptr = (uint8_t *)expr_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_DC:     result.value.as_ptr = (uint8_t *) dc_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TDC:    result.value.as_ptr = (uint8_t *) tdc_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_TABLE:  result.value.as_ptr = (uint8_t *) table_deserialize(cursor, bytes_left, &cur_bytes); break;
	case AMP_TYPE_EDD:
	case AMP_TYPE_CTRL:
	case AMP_TYPE_LIT:
	case AMP_TYPE_OP:
	case AMP_TYPE_SDNV:
	default: result.type = AMP_TYPE_UNK; break;
	}

	return result;
}


value_t  val_from_int(int32_t val)
{
	value_t result;
	result.type = AMP_TYPE_INT;
	result.value.as_int = val;
	return result;
}

value_t  val_from_real32(float val)
{
	value_t result;
	result.type = AMP_TYPE_REAL32;
	result.value.as_real32 = val;
	return result;
}

value_t  val_from_real64(double val)
{
	value_t result;
	result.type = AMP_TYPE_REAL64;
	result.value.as_real64 = val;
	return result;
}



/*
 * Must release result...
 */
value_t  val_from_string(char *str)
{
	value_t result;
	uint32_t len;

	AMP_DEBUG_ENTRY("val_from_string","(0x%lx)", (unsigned long) str);

	result.type = AMP_TYPE_UNK;
	result.value.as_ptr = NULL;

	/* Step 1 - Sanity Check. */
	if(str == NULL)
	{
        AMP_DEBUG_ERR("val_from_string","Bad params.",NULL);
        AMP_DEBUG_EXIT("val_from_string","-> Unk", NULL);
        return result;
	}

	/* Step 2 - Find out length of string. */
	len = strlen(str) + 1;

	/* Step 3 - Allocate storage for the string. */
	if((result.value.as_ptr = STAKE(len)) == NULL)
	{
        AMP_DEBUG_ERR("val_from_string","Can't allocate %d bytes.",len);
        AMP_DEBUG_EXIT("val_from_string","-> Unk", NULL);
        return result;
	}

	/* Step 4 - Populate the return value. */
	result.type = AMP_TYPE_STRING;
	memcpy(result.value.as_ptr,str, len);

	AMP_DEBUG_EXIT("val_from_string","-> %s", str);
	return result;
}

value_t  val_from_uint(uint32_t val)
{
	value_t result;
	result.type = AMP_TYPE_UINT;
	result.value.as_uint = val;
	return result;
}

value_t  val_from_uvast(uvast val)
{
	value_t result;
	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = val;
	return result;
}

value_t  val_from_vast(vast val)
{
	value_t result;
	result.type = AMP_TYPE_VAST;
	result.value.as_vast = val;
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
		return AMP_TYPE_UINT;
	}
	else if(ltype == rtype)
	{
		return ltype;
	}
	else if(type_is_numeric(ltype) && type_is_numeric(rtype))
	{
		/*
		 * \todo: Reconsider this design. Right now we normalize int to have
		 * DTNMP_TYPE_INT = 0 to do a lookup.
		 */
		return gValNumCvtResult[ltype-(int)AMP_TYPE_INT][rtype-(int)AMP_TYPE_INT];
	}

	return AMP_TYPE_UNK;
}


void     val_init(value_t *val)
{
	if(val != NULL)
	{
		val->type = AMP_TYPE_UNK;
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

int val_is_int(value_t val)
{
	int result = 0;

	AMP_DEBUG_ENTRY("val_is_int","(val type %d)", val.type);

	AMP_DEBUG_INFO("val_is_int","Type: %d", val.type);

	switch(val.type)
	{
		case AMP_TYPE_INT:
		case AMP_TYPE_UINT:
		case AMP_TYPE_VAST:
		case AMP_TYPE_UVAST:
			result = 1;
			break;
		default:
	        result = 0;
	        break;
	}

	AMP_DEBUG_EXIT("val_is_int","-> %d", result);

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

int val_is_real(value_t val)
{
	int result = 0;

	AMP_DEBUG_ENTRY("val_is_real","(val type %d)", val.type);

	AMP_DEBUG_INFO("val_is_real","Type: %d", val.type);

	switch(val.type)
	{
		case AMP_TYPE_REAL32:
		case AMP_TYPE_REAL64:
			result = 1;
			break;
		default:
	        result = 0;
	        break;
	}

	AMP_DEBUG_EXIT("val_is_real","-> %d", result);

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

void val_release(value_t *val, uint8_t destroy)
{
	AMP_DEBUG_ENTRY("val_release","(0x%lx)", (unsigned long) val);

	if(val != NULL)
	{
		switch(val->type)
		{
		case AMP_TYPE_VAR:     var_release((var_t*)val->value.as_ptr); break;
		case AMP_TYPE_RPT:    def_release_gen((def_gen_t *)val->value.as_ptr); break;
		case AMP_TYPE_DC:     dc_destroy((Lyst*) &(val->value.as_ptr)); break;
		case AMP_TYPE_TDC:    tdc_destroy((tdc_t**)&(val->value.as_ptr)); break;
		case AMP_TYPE_BLOB:   blob_destroy((blob_t*)val->value.as_ptr, 1); break;
		case AMP_TYPE_MACRO:  midcol_destroy((Lyst*) &(val->value.as_ptr)); break;
		case AMP_TYPE_MC:     midcol_destroy((Lyst*) &(val->value.as_ptr)); break;
		case AMP_TYPE_MID:    mid_release((mid_t*)val->value.as_ptr); break;
		case AMP_TYPE_STRING: SRELEASE(val->value.as_ptr); break;
		case AMP_TYPE_TRL:    trl_release((trl_t*)val->value.as_ptr); break;
		case AMP_TYPE_SRL:    srl_release((srl_t*)val->value.as_ptr); break;
		case AMP_TYPE_EXPR:   expr_release((expr_t*)val->value.as_ptr); break;
		case AMP_TYPE_TABLE:  table_destroy((table_t*)val->value.as_ptr, 1); break;
		default:
			break;
		}

		if(destroy != 0)
		{
			SRELEASE(val);
		}
	}

	AMP_DEBUG_EXIT("val_release",".", NULL);
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

uint8_t* val_serialize(value_t val, uint32_t *size, int use_type)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *data = NULL;
	uint32_t data_size;
	Sdnv tmp;

	/* Step 0: Sanity check. */
	if(size == NULL)
	{
		return NULL;
	}


	/* Step 1: Serialize the val data. */
	switch(val.type)
	{
	case AMP_TYPE_VAR:     data = var_serialize((var_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_RPT:    data = def_serialize_gen((def_gen_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_SRL:    data = srl_serialize((srl_t*) val.value.as_ptr, &data_size); break;
	case AMP_TYPE_TRL:    data = trl_serialize((trl_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_MC:     data = midcol_serialize((Lyst)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_MACRO:  data = midcol_serialize((Lyst)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_BYTE:   data = utils_serialize_byte(val.value.as_int, &data_size); break;
	case AMP_TYPE_INT:    data = utils_serialize_int(val.value.as_int, &data_size); break;
	case AMP_TYPE_TS:     data = utils_serialize_uint(val.value.as_uint, &data_size); break;
	case AMP_TYPE_UINT:   data = utils_serialize_uint(val.value.as_uint, &data_size); break;
	case AMP_TYPE_VAST:   data = utils_serialize_vast(val.value.as_vast, &data_size); break;
	case AMP_TYPE_UVAST:  data = utils_serialize_uvast(val.value.as_uvast, &data_size); break;
	case AMP_TYPE_REAL32:	data = utils_serialize_real32(val.value.as_real32, &data_size); break;
	case AMP_TYPE_REAL64:	data = utils_serialize_real64(val.value.as_real64, &data_size); break;
	case AMP_TYPE_STRING:	data = utils_serialize_string((char *) val.value.as_ptr, &data_size); break;

	case AMP_TYPE_BLOB:   data = blob_serialize((blob_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_MID:    data = mid_serialize((mid_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_EXPR:   data = expr_serialize((expr_t*)val.value.as_ptr, &data_size); break;

	case AMP_TYPE_DC:     data = dc_serialize((Lyst)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_TDC:    data = tdc_serialize((tdc_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_TABLE:  data = table_serialize((table_t*)val.value.as_ptr, &data_size); break;
	case AMP_TYPE_EDD:
	case AMP_TYPE_CTRL:
	case AMP_TYPE_LIT:
	case AMP_TYPE_OP:
	case AMP_TYPE_SDNV:
	default:
		AMP_DEBUG_ERR("val_serialize","Unknown type", NULL);
		break;
	}

	if(data == NULL)
	{
		AMP_DEBUG_ERR("val_serialize","Can't serialize val of type %d", val.type);
		*size = 0;
		return NULL;
	}

	if(use_type == 0)
	{
		*size = data_size;
		return data;
	}

	encodeSdnv(&tmp, data_size);

	*size = data_size + 1 + tmp.length;
	if((result = STAKE(*size)) == NULL)
	{
		AMP_DEBUG_ERR("val_serialize", "Can't allocate %d bytes.", *size);
		*size = 0;
		SRELEASE(data);
		return NULL;
	}

	cursor = result;
	result[0] = val.type;
	cursor++;

	memcpy(cursor, tmp.text, tmp.length);
	cursor += tmp.length;

	memcpy(cursor, data, data_size);
	SRELEASE(data);
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
 *		1. The string version of a value is "(type) data"
 *****************************************************************************/

char *val_to_string(value_t val)
{
	char *result = NULL;
	const char *type_str = NULL;
	char *val_str = NULL;
	char alt_str[22];
	uint8_t use_alt = 0;
	uint8_t free_val = 1;
	uint32_t size = 0;

	AMP_DEBUG_ENTRY("val_to_string","(val type %d)", val.type);

	/* Step 1: Grab the type string. */
	if((type_str = type_to_str(val.type)) == NULL)
	{
		AMP_DEBUG_ERR("val_to_string","Can't get type string.", NULL);
		return NULL;
	}

	memset(alt_str,0,22);
	/* Step 2: Grab the value string. */
	switch(val.type)
	{
	case AMP_TYPE_VAR:     val_str = var_to_string((var_t*)val.value.as_ptr); break;
	//case DTNMP_TYPE_RPT:    val_str = def_to_str((def_gen_t*)val.value.as_ptr); break;
	//case DTNMP_TYPE_DEF:    val_str = def_to_str((def_gen_t*)val.value.as_ptr); break;
//	case DTNMP_TYPE_SRL:    val_str = srl_to_str((srl_t*) val.value.as_ptr); break;
//	case DTNMP_TYPE_TRL:    val_str = trl_to_str((trl_t*)val.value.as_ptr); break;
	case AMP_TYPE_MC:     val_str = midcol_to_string((Lyst)val.value.as_ptr); break;
	case AMP_TYPE_MACRO:  val_str = midcol_to_string((Lyst)val.value.as_ptr); break;
	case AMP_TYPE_BYTE:   use_alt = 1; sprintf(alt_str,"%x", (unsigned int) val.value.as_int); break;
	case AMP_TYPE_INT:    use_alt = 1; sprintf(alt_str,"%d", (int) val.value.as_int); break;
	case AMP_TYPE_TS:     use_alt = 1; sprintf(alt_str,"%d", (int) val.value.as_uint); break;
	case AMP_TYPE_UINT:   use_alt = 1; sprintf(alt_str,"%u", (unsigned int) val.value.as_uint); break;
	case AMP_TYPE_VAST:   use_alt = 1; sprintf(alt_str,VAST_FIELDSPEC, val.value.as_vast); break;
	case AMP_TYPE_UVAST:  use_alt = 1; sprintf(alt_str,UVAST_FIELDSPEC, val.value.as_uvast); break;
	case AMP_TYPE_REAL32:	use_alt = 1; sprintf(alt_str,"%f", val.value.as_real32); break;
	case AMP_TYPE_REAL64:	use_alt = 1; sprintf(alt_str,"%f", val.value.as_real64); break;
	case AMP_TYPE_STRING:	free_val = 0; val_str = (char*)val.value.as_ptr; break;

	case AMP_TYPE_BLOB:   val_str = blob_to_str((blob_t*)val.value.as_ptr); break;
	case AMP_TYPE_MID:    val_str = mid_to_string((mid_t*)val.value.as_ptr); break;
	case AMP_TYPE_EXPR:   val_str = expr_to_string((expr_t*)val.value.as_ptr); break;

//	case DTNMP_TYPE_DC:     val_str = dc_to_str((Lyst)val.value.as_ptr); break;
	case AMP_TYPE_TDC:    val_str = tdc_to_str((tdc_t*)val.value.as_ptr); break;
//	case DTNMP_TYPE_TABLE:  val_str = table_to_str((table_t*) val.value.as_ptr); break;

	case AMP_TYPE_EDD:
	case AMP_TYPE_CTRL:
	case AMP_TYPE_LIT:
	case AMP_TYPE_OP:
	case AMP_TYPE_SDNV:
	default:
		AMP_DEBUG_ERR("val_to_str","Unsupported type %d", val.type); break;
	}

	if((val_str == NULL) && (use_alt == 0))
	{
		AMP_DEBUG_ERR("val_to_str","Error converting value to string.", NULL);
		return NULL;
	}

	size = strlen(type_str) + 3; // "(TYPE) "
	size += (use_alt == 1) ? strlen(alt_str) + 2 : strlen(val_str) + 2;

	if((result = STAKE(size)) == NULL)
	{
		AMP_DEBUG_ERR("val_to_str","an't allocate %d bytes.", size);
		if(free_val)
		{
			SRELEASE(val_str);
		}
		return NULL;
	}

	if(use_alt == 1)
	{
		sprintf(result, "(%s) %s", type_str, alt_str);
	}
	else
	{
		sprintf(result, "(%s) %s", type_str, val_str);
		if(free_val)
		{
			SRELEASE(val_str);
		}
	}

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
		val_release(tmp, 1);
	}

	lyst_destroy(*valcol);
	*valcol = NULL;
}

value_t *val_ptr(value_t val)
{
	value_t *result = NULL;

	if((result = STAKE(sizeof(value_t))) == NULL)
	{
		return NULL;
	}

	*result = val_copy(val);

	return result;
}

