/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: blob.h
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Binary Large Objects (BLOBs).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/14/16  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  11/18/18  E. Birrane     Updates for new AMP. (JHU/APL).
 *****************************************************************************/

#ifndef BLOB_H_
#define BLOB_H_

#include "stdint.h"
#include "qcbor.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

#define BLOB_DEFAULT_ENC_SIZE (SMALL_SIZES * WORD_SIZE)

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * The BLOB is a self-delineating structure that captured arbitrary user data.
 */
typedef struct {
	uint8_t *value; /**> The data associated with the entry. */
	size_t length;  /**> The length of the data in bytes. */
	size_t alloc;   /**> The allocated size of the entry. */
} blob_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int       blob_append(blob_t *blob, uint8_t *buffer, uint32_t length);
blob_t*   blob_create(uint8_t *value, size_t length, size_t alloc);
int       blob_compare(blob_t* v1, blob_t *v2);
int       blob_copy(blob_t src, blob_t *dest);
blob_t*   blob_copy_ptr(blob_t *src);

blob_t    blob_deserialize(QCBORDecodeContext *it, int *success);
blob_t    blob_deserialize_as_bytes(QCBORDecodeContext *it, int *success, size_t len);
blob_t*   blob_deserialize_as_bytes_ptr(QCBORDecodeContext *it, int *success, size_t len);
blob_t*   blob_deserialize_ptr(QCBORDecodeContext *it, int *success);
int       blob_serialize(QCBOREncodeContext *encoder, blob_t *item);
int       blob_serialize_as_bytes(QCBOREncodeContext *it, blob_t *blob);

int       blob_grow(blob_t *blob, uint32_t length);
int       blob_init(blob_t *blob, uint8_t *value, size_t length, size_t alloc);
void      blob_release(blob_t *blob, int destroy);

blob_t*   blob_serialize_wrapper(blob_t *blob);
int8_t    blob_trim(blob_t *blob, uint32_t length);

#endif // BLOB_H
