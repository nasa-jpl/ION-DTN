/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: tnv.h
 **
 ** Description: This implements various, compact, binary encodings of the
 **              AMM TNVC objects. This was originally based on the
 **              typed-data-collection (TDC) structure.
 **
 ** Notes:
 **
 **
 ** Assumptions:
 ** 1. TNV collections will have less than 256 elements.
 ** 2. The only TNVC supported by ION AMP is the TVC. Names are never exchanged
 **    with ION agents. Values are always typed.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/24/15  J. P. Mayer    Initial Implementation.
 **  06/27/15  E. Birrane     Migrate from datalist to TDC. (Secure DTN - NASA: NNX14CS58P)
 **  01/11/18  E. Birrane     Add update ability. (JHU/APL)
 **  09/01/18  E. Birrane     Migrate from TDC to TNV. (JHU/APL)
 *****************************************************************************/
#ifndef TNV_H_
#define TNV_H_

#include "stdint.h"


#include "../utils/nm_types.h"
#include "../utils/cbor_utils.h"

#include "../utils/vector.h"



/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/*
 * TNVs support 2 flags:
 *  ALLOC: Whether the as_ptr value, if populated, must be freed.
 *  MAP: Whether the parameter is a mapped parameter. In this case, the type
 *       of the value is correct, but the value itself will always be UINT
 *       and specify the index of the actual parameter in some external
 *       parameter list.
 */
#define TNV_FLAG_ALLOC (0x1)
#define TNV_FLAG_MAP   (0x2)

#define TNVC_HAS_T (0x4)
#define TNVC_HAS_N (0x2)
#define TNVC_HAS_V (0x1)

typedef enum
{
	                /* TYPE    NAME   VALUE */
	TNVC_NIL  = 0,  /*  N       N       N   */
	TNVC_VC   = 1,  /*  N       N       Y   */
	TNVC_NC   = 2,  /*  N       Y       N   */
	TNVC_NVC  = 3,  /*  N       Y       Y   */
	TNVC_TC   = 4,  /*  Y       N       N   */
	TNVC_TVC  = 5,  /*  Y       N       Y   */
	TNVC_TNC  = 6,  /*  Y       Y       N   */
	TNVC_TNVC = 7,  /*  Y       Y       Y   */
	TNVC_UNK  = 8
} tnv_enc_e;


#define TNV_DEFAULT_ENC_SIZE (SMALL_SIZES * WORD_SIZE)
#define TNVC_DEFAULT_ENC_SIZE (SMALL_SIZES * WORD_SIZE)

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */
#define TNV_IS_ALLOC(flags) (flags & TNV_FLAG_ALLOC)
#define TNV_IS_MAP(flags)   (flags & TNV_FLAG_MAP)

#define TNV_SET_ALLOC(flags) (flags |= TNV_FLAG_ALLOC)
#define TNV_SET_MAP(flags) (flags |= TNV_FLAG_MAP)

#define TNV_CLEAR_ALLOC(flags) (flags &= ~TNV_FLAG_ALLOC)
#define TNV_CLEAR_MAP(flags) (flags &= ~TNV_FLAG_MAP)

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Describes a strongly typed value in AMP.
 *
 * Values are stored in one of two formats by using a union, as follows.
 *
 * 1. Primitive data types are represented by their associated types.
 *
 * 2. AMM Objects and Compound types are stored as a pointer to the object.
 *
 * For portability, bool is modeled as a byte.
 */
typedef struct
{
	amp_type_e type;        /**> The type of the information in this TNV. */

	union {
		void*    as_ptr;    /**> Value of the TNV. */
		uint8_t  as_byte;
		uint32_t as_uint;
		int32_t  as_int;
		vast     as_vast;
		uvast    as_uvast;
		float    as_real32;
		double   as_real64;
	} value;

	uint8_t flags;         /**> Flags for this TNV.*/

} tnv_t;



/**
 * A Type-Name-Value Collection (TNVC)...
 */
typedef struct
{
	vector_t values; /* Typed as tnv_t pointers. */
} tnvc_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */



/*** TNV Functions ***/

tnv_t*    tnv_cast(tnv_t *tnv, amp_type_e type);
int       tnv_compare(tnv_t *v1, tnv_t *v2);
void      tnv_cb_del(void *item);
int       tnv_cb_comp(void *i1, void *i2);
void*     tnv_cb_copy(void *item);
tnv_t     tnv_copy(tnv_t val, int *success);
tnv_t*    tnv_copy_ptr(tnv_t *val);
tnv_t*    tnv_create();
tnv_t     tnv_deserialize(QCBORDecodeContext *it, int *success);
tnv_t*    tnv_deserialize_ptr(QCBORDecodeContext *it, int *success);
tnv_t*    tnv_deserialize_raw(blob_t *data, int *success);
int       tnv_deserialize_val_by_type(QCBORDecodeContext *it, tnv_t *result);
int       tnv_deserialize_val_raw(blob_t *data, tnv_t *result);
tnv_t*    tnv_from_bool(uint8_t val);
tnv_t*    tnv_from_blob(blob_t *val);
tnv_t*    tnv_from_byte(uint8_t val);
tnv_t*    tnv_from_int(int32_t val);
tnv_t*    tnv_from_map(amp_type_e type, uint8_t map_idx);
tnv_t*    tnv_from_obj(amp_type_e type, void *item);
tnv_t*    tnv_from_real32(float val);
tnv_t*    tnv_from_real64(double val);
tnv_t*    tnv_from_str(char *str);
tnv_t*    tnv_from_uint(uint32_t val);
tnv_t*    tnv_from_uvast(uvast val);
tnv_t*    tnv_from_tv(time_t val);
tnv_t*    tnv_from_vast(vast val);
void      tnv_init(tnv_t *val, amp_type_e type);
int       tnv_serialize(QCBOREncodeContext *encoder, void *item);
int       tnv_serialize_value(QCBOREncodeContext *encoder, void *item);
blob_t*   tnv_serialize_value_wrapper(tnv_t *tnv);
blob_t*   tnv_serialize_wrapper(tnv_t *tnv);
int       tnv_set_map(tnv_t *tnv, uint32_t map);
void      tnv_release(tnv_t *val, int destroy);
int32_t   tnv_to_int(tnv_t val, int *success);
float     tnv_to_real32(tnv_t val, int *success);
double	  tnv_to_real64(tnv_t val, int *success);
uint32_t  tnv_to_uint(tnv_t val, int *success);
uvast 	  tnv_to_uvast(tnv_t val, int *success);
vast 	  tnv_to_vast(tnv_t val, int *success);


/*** TNVC Functions ***/


int      tnvc_append(tnvc_t *dst, tnvc_t *src);

void     tnvc_cb_del(void *item);
int      tnvc_cb_comp(void *i1, void *i2);
void*    tnvc_cb_copy_fn(void *item);



void     tnvc_clear(tnvc_t *tnvc);
int      tnvc_compare(tnvc_t *t1, tnvc_t *t2);
tnvc_t*  tnvc_create(uint8_t num);
tnvc_t*  tnvc_copy(tnvc_t *src);

tnvc_t   tnvc_deserialize(QCBORDecodeContext *it, int *success);
tnvc_t*  tnvc_deserialize_ptr(QCBORDecodeContext *it, int *success);
tnvc_t*  tnvc_deserialize_ptr_raw(blob_t *data, int *success);
tnvc_t   tnvc_deserialize_raw(blob_t *data, int *success);

tnv_t*     tnvc_get(tnvc_t* tnvc, uint8_t index);
uint8_t    tnvc_get_count(tnvc_t* tnvc);
tnv_enc_e  tnvc_get_encode_type(tnvc_t *tnvc);
amp_type_e tnvc_get_type(tnvc_t *tnvc, uint8_t index);
blob_t     tnvc_get_types(tnvc_t *tnvc, int *success);

int      tnvc_init(tnvc_t *tnvc, size_t num);
int      tnvc_insert(tnvc_t* tnvc, tnv_t *tnv);
void     tnvc_release(tnvc_t *tnvc, int destroy);

int      tnvc_serialize(QCBOREncodeContext *encoder, void *item);
blob_t*  tnvc_serialize_wrapper(tnvc_t *tnvc);

int      tnvc_size(tnvc_t *tnvc);

int      tnvc_update(tnvc_t *tnvc, uint8_t idx, tnv_t *src_tnv);


#endif // TNV_H_INCLUDED
