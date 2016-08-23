/*****************************************************************************
 **
 ** \file var.h
 **
 **
 ** Description: Structures that capture AMP Variable definitions.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/05/16  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane     Renamed CD to VAR. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef _VAR_H_
#define _VAR_H_

#include "mid.h"
#include "value.h"
#include "expr.h"

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


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * This structure captures the location of this Variable object
 * in the applicable, persistent SDR.
 */
typedef struct
{
	Object itemObj;   /**> Location of the VAR object in SDR. */
	uint32_t size;    /**> Size of the VAR object in the SDR */

	Object descObj;   /**> Location of this descr in SDR. */

} var_desc_t;

// \todo make a single db desc object since these contain no data-type specific units.

/**
 * This structure captures the contents of a Variable object. This
 * includes the definition of the object itself and, on the Agent side,
 * the last computed value of the definition.
 */
typedef struct
{
    mid_t *id;     /**> The identifier (name) of the CD. */
    value_t value; /**> The value of the CD. */

    /* Below items are not serialized with this structure. */
    var_desc_t desc; /**> Descriptor of def in the SDR. */
} var_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


var_t *var_create(mid_t *id,
				amp_type_e type,
				expr_t *init,
				value_t val);

var_t *var_create_from_parms(Lyst parms);

var_t *var_deserialize(uint8_t *cursor,
		             uint32_t size,
		             uint32_t *bytes_used);

var_t *var_duplicate(var_t *orig);

var_t *var_find_by_id(Lyst defs, ResourceLock *mutex, mid_t *id);

void var_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy);

void var_release(var_t *cd);

uint8_t *var_serialize(var_t *cd, uint32_t *len);

char *var_to_string(var_t *cd);

#endif /* _VAR_H_ */
