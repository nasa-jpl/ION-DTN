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
 *****************************************************************************/

#ifndef BLOB_H_
#define BLOB_H_

#include "stdint.h"

#include "lyst.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * The BLOB is a self-delineating structure that captured arbitrary user data.
 */
typedef struct {
	uint8_t *value;   /**> The data associated with the entry. */
	uint32_t length;  /**> The length of the data in bytes. */
} blob_t;

/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int8_t   blob_append(blob_t *blob, uint8_t *buffer, uint32_t length);
blob_t * blob_create(uint8_t *value, uint32_t length);
blob_t * blob_copy(blob_t *blob);
blob_t * blob_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t *bytes_used);
void blob_destroy(blob_t *blob, uint8_t destroy);
uint32_t blob_get_serialize_size(blob_t *blob);
uint8_t* blob_serialize(blob_t *blob, uint32_t *size);
char*    blob_to_str(blob_t *blob);
int8_t   blob_trim(blob_t *blob, uint32_t length);
void     blobcol_clear(Lyst *blobs);


#endif // BLOB_H
