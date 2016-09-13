/*****************************************************************************
 **
 ** File Name: dc.h
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Data Collections (DCs).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/28/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  04/14/16  E. Birrane     Migrated to BLOB type (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef DC_H_
#define DC_H_

#include "stdint.h"
#include "../primitives/blob.h"
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

int      dc_add(Lyst dc, uint8_t *value, uint32_t length);
int      dc_add_first(Lyst dc, uint8_t *value, uint32_t length);
int      dc_append(Lyst dest, Lyst src);
int      dc_compare(Lyst col1, Lyst col2);
Lyst     dc_copy(Lyst col);
Lyst     dc_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t *bytes_used);
void     dc_destroy(Lyst *datacol);
blob_t*  dc_get_entry(Lyst datacol, uint32_t idx);
int      dc_remove_first(Lyst dc, int del);
uint8_t* dc_serialize(Lyst datacol, uint32_t *size);


#endif // DC_H
