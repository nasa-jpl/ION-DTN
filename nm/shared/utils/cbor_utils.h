/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: cbor_utils.h
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

#include "nm_types.h"
#include "vector.h"

#include "contrib/tinycbor/src/cbor.h"


#define CUT_ENC_BUFSIZE 4096

typedef CborError (*cut_enc_fn)(CborEncoder *encoder, void *item);
typedef void* (*vec_des_fn)(CborValue *it, int *success);

int       cut_advance_it(CborValue *value);

CborError cut_enc_byte(CborEncoder *encoder, uint8_t byte);
int cut_enc_uvast(uvast num, blob_t *result);


CborError cut_enc_refresh(CborValue *it);
void cut_enc_expect_more(CborValue*it, int num);

int cut_enter_array(CborValue *it, size_t min, size_t max, size_t *num, CborValue *array_it);

int       cut_get_array_len(CborValue *it, size_t *result);
int       cut_get_cbor_numeric(CborValue *value, amp_type_e type, void *val);
char *    cut_get_cbor_str(CborValue *value, int *success);

void      cut_init_enc(CborEncoder *encoder, uint8_t *val, uint32_t size);


blob_t*   cut_serialize_wrapper(size_t size, void *item, cut_enc_fn encode);


CborError cut_deserialize_vector(vector_t *vec, CborValue *it, vec_des_fn des_fn);
CborError cut_serialize_vector(CborEncoder *encoder, vector_t *vec, cut_enc_fn enc_fn);

void *cut_char_deserialize(CborValue *it, int *success);
CborError cut_char_serialize(CborEncoder *encoder, void *item);

