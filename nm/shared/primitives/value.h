/*****************************************************************************
 **
 ** File Name: value.h
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

#ifndef VALUE_H_
#define VALUE_H_

//#include "ion_if.h"
//#include "nm_types.h"

#include "shared/primitives/mid.h"

#define VAL_TYPE_UNK    0  /* unknown          */
#define VAL_TYPE_INT    1  /* Fits an int32_t. */
#define VAL_TYPE_UINT   2  /* Fits a uint32_t. */
#define VAL_TYPE_VAST   3  /* Fits an int64_t. */
#define VAL_TYPE_UVAST  4  /* Fits a uin64_t.  */
#define VAL_TYPE_REAL32 5  /* Fits float.      */
#define VAL_TYPE_REAL64 6  /* Fits double.   . */
#define VAL_TYPE_STRING 7  /* Always ptr.    */
#define VAL_TYPE_BLOB   8  /* ALways ptr.    */

#define VAL_OPTYPE_ARITHMETIC 0
#define VAL_OPTYPE_LOGICAL    1


/*
 * Arithmetic casting matrix.
 *
 * 			 UNK	 INT	 UINT	  VAST	  UVAST	   REAL32	REAL64	 STRING	  BLOB
 *         +-----+--------+--------+--------+--------+--------+--------+--------+------+
 *  UNK	   | UNK | UNK    | UNK    | UNK    | UNK    | UNK    | UNK    | UNK    | UNK  |
 *  INT	   | UNK | INT    | INT	   | VAST   | UNK    | REAL32 | REAL64 | UNK	| UNK  |
 *  UINT   | UNK | INT    | UINT   | VAST   | UVAST  | REAL32 | REAL64 | UNK	| UNK  |
 *  VAST   | UNK | VAST   | VAST   | VAST   | VAST   | REAL32 | REAL64 | UNK	| UNK  |
 *  UVAST  | UNK | UNK    | UVAST  | VAST   | UVAST  | REAL32 | REAL64 | UNK	| UNK  |
 *  REAL32 | UNK | REAL32 |	REAL32 | REAL32 | REAL32 | REAL32 | REAL64 | UNK	| UNK  |
 *  REAL64 | UNK | REAL64 |	REAL64 | REAL64 | REAL64 | REAL64 | REAL64 | UNK	| UNK  |
 *  STRING | UNK | UNK    | UNK	   | UNK    | UNK    | UNK    | UNK	   | UNK	| UNK  |
 *  BLOB   | UNK | UNK    | UNK	   | UNK    | UNK    | UNK    | UNK	   | UNK	| UNK  |
 *         +-----+--------+--------+--------+--------+--------+--------+--------+------+
 *
 */
extern int gValResultType[9][9];

/**
 * Describes a strongly typed value in the ION DTNMP.
 *
 * Values are stored in one of three formats by using
 * a union, as follows.
 *
 * 1. Primitive data types are represented by their
 *    associated types.
 * 2. The as_ptr is used for strings and blobs and
 *    indicates the first byte of the data value.
 *
 * Note: The length field is NOT simply the sizeof
 *       the encapsulating primitive type. For
 *       example, a uint8_t may be captured with
 *       a type of VAL_TYPE_UINT as stored uing the
 *       as_uint struct in the value union. However,
 *       it's length is still 1 byte, not 4. This is
 *       important when serializing and deserializing
 *       the data.
 *
 * Note: It is assumed that no value length with be
 *       greater than 4GB and therefore it is stored
 *       as a uint32_t.
 *
 * Note: The serialization strategy is:
 *       <type><length><value> as <byte><dc>
 */
typedef struct
{
	uint8_t type;

	union {
		uint8_t *as_ptr;
		uint32_t as_uint;
		int32_t  as_int;
		vast     as_vast;
		uvast    as_uvast;
		float    as_real32;
		double   as_real64;
	} value;

	uint32_t length;
} value_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

value_t* val_as_ptr(value_t val);

int32_t  val_cvt_int(value_t *val);

uint32_t val_cvt_uint(value_t *val);

vast 	 val_cvt_vast(value_t *val);

uvast 	 val_cvt_uvast(value_t *val);

float    val_cvt_real32(value_t *val);

double	 val_cvt_real64(value_t *val);

value_t* val_deserialize(unsigned char *buffer,
		                 uint32_t buffer_size,
		                 uint32_t *bytes_used);

value_t* val_deserialize_one(unsigned char *buffer,
		                     uint32_t buffer_size);

value_t  val_from_string(char *str);

int      val_get_result_type(int ltype, int rtype, int optype);

void     val_init(value_t *val);

int      val_is_int(value_t *val);

int      val_is_real(value_t *val);

void     val_release(value_t *val);

uint8_t* val_serialize(value_t *val, uint32_t *size);

char*    val_to_string(value_t *val);

value_t  val_int_bin_op(uvast lval, uvast rval, int ltype, int rtype, int op);

void     valcol_destroy(Lyst *valcol);


#endif /* VALUE_H_ */
