/*****************************************************************************
 **
 ** File Name: value.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary to represent typed values within the
 **              the AMP.
 **
 **              A value is a strongly-typed datum, the result of an initializing
 **              expression, association with a literal, or as set at runtime
 **              as part of an assignment operation.
 **
 ** Notes:
 **
 **  1. Values can represent basic numeric types (UINT, INT, etc...)
 **  2. Values can represent non-numeric basic data types (string, blob)
 **  3. Values can represent AMP structures (rules, MID, MID Collection)
 **
 ** Assumptions:
 **		1. ION AMP does not support integer or real values > 64 bits.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef VALUE_H_
#define VALUE_H_

#include "../utils/nm_types.h"
#include "../primitives/mid.h"

// \todo: Make enum
#define VAL_OPTYPE_ARITHMETIC 0
#define VAL_OPTYPE_LOGICAL    1


/*
 * Arithmetic casting matrix.
 *
 * 			  INT     UINT	   VAST    UVAST     REAL32   REAL64
 *         +--------+--------+--------+--------+--------+--------+
 *  INT	   | INT    | INT	 | VAST   | UNK    | REAL32 | REAL64 |
 *  UINT   | INT    | UINT   | VAST   | UVAST  | REAL32 | REAL64 |
 *  VAST   | VAST   | VAST   | VAST   | VAST   | REAL32 | REAL64 |
 *  UVAST  | UNK    | UVAST  | VAST   | UVAST  | REAL32 | REAL64 |
 *  REAL32 | REAL32 | REAL32 | REAL32 | REAL32 | REAL32 | REAL64 |
 *  REAL64 | REAL64 | REAL64 | REAL64 | REAL64 | REAL64 | REAL64 |
 *         +--------+--------+--------+--------+--------+--------+
 *
 */
extern int gValNumCvtResult[6][6];

/**
 * Describes a strongly typed value in the ION DTNMP.
 *
 * Values are stored in one of three formats by using a union, as follows.
 *
 * 1. Primitive data types are represented by their associated types.
 *
 * 2. The as_ptr is used for strings and blobs and indicates the first byte
 *    of the data value.
 *
 * Note: The length field is NOT simply the sizeof the encapsulating
 *       primitive type. For example, a uint8_t may be captured with
 *       a type of VAL_TYPE_UINT as stored uing the as_uint struct in
 *       the value union. However, it's length is still 1 byte, not 4. This
 *       is important when serializing and deserializing the data.
 *
 * Note: It is assumed that no value length with be greater than 4GB and
 *       therefore it is stored as a uint32_t.
 *
 */
typedef struct
{
	amp_type_e type;      /* The strong type of the value. */

	union {
		uint8_t *as_ptr;    /* When used, this is serialized? */
		uint32_t as_uint;
		int32_t  as_int;
		vast     as_vast;
		uvast    as_uvast;
		float    as_real32;
		double   as_real64;
	} value;

} value_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

value_t  val_copy(value_t val);

int32_t  val_cvt_int(value_t val, uint8_t *success);

uint32_t val_cvt_uint(value_t val, uint8_t *success);

vast 	 val_cvt_vast(value_t val, uint8_t *success);

uvast 	 val_cvt_uvast(value_t val, uint8_t *success);

float    val_cvt_real32(value_t val, uint8_t *success);

double	 val_cvt_real64(value_t val, uint8_t *success);

uint8_t	 val_cvt_type(value_t *val, amp_type_e type);

value_t  val_deserialize(unsigned char *buffer, uint32_t bytes_left, uint32_t *bytes_used);


value_t  val_from_blob(blob_t *blob, amp_type_e type);
value_t  val_from_int(int32_t val);
value_t  val_from_real32(float val);
value_t  val_from_real64(double val);
value_t  val_from_string(char *str);
value_t  val_from_uint(uint32_t val);
value_t  val_from_uvast(uvast val);
value_t  val_from_vast(vast val);





int      val_get_result_type(int ltype, int rtype, int optype);



void     val_init(value_t *val);

int      val_is_int(value_t val);

int      val_is_real(value_t val);

// \todo add val_is_numeric?

value_t *val_ptr(value_t val);

void     val_release(value_t *val, uint8_t destroy);

uint8_t* val_serialize(value_t val, uint32_t *size, int use_type);



char*    val_to_string(value_t val);

value_t  val_int_bin_op(uvast lval, uvast rval, int ltype, int rtype, int op);

void     valcol_destroy(Lyst *valcol);


#endif /* VALUE_H_ */
