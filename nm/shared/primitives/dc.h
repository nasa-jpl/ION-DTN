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
 **  05/28/15  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef DC_H_
#define DC_H_

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
 * The Data Collection is similar to a MID collection: an SDNV count followed
 * by one or more of a homogeneous structure.  In the case of a Data Collection,
 * the structure is given by the datacol_entry_t.  This is simply a buffer with
 * associated byte length.
 */
typedef struct {
	uint8_t *value;   /**> The data associated with the entry. */
	uint32_t length;  /**> The length of the data in bytes. */
} datacol_entry_t;

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

int              dc_add(Lyst dc, uint8_t *value, uint32_t length);
int              dc_append(Lyst dest, Lyst src);
int              dc_compare(Lyst col1, Lyst col2);
Lyst             dc_copy(Lyst col);
datacol_entry_t* dc_copy_entry(datacol_entry_t *entry);
Lyst             dc_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t *bytes_used);
void             dc_destroy(Lyst *datacol);
datacol_entry_t* dc_get_entry(Lyst datacol, uint32_t idx);
void             dc_release_entry(datacol_entry_t *entry);
uint8_t*         dc_serialize(Lyst datacol, uint32_t *size);
char*            dc_entry_to_str(datacol_entry_t *entry);


#endif // DC_H
