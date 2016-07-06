/*****************************************************************************
 **
 ** \file cd.h
 **
 **
 ** Description: Structures that capture protocol computed data definitions.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/05/16  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef _CD_H_
#define _CD_H_

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
 * This structure captures the location of this Computed Data object
 * in the applicable, persistent SDR.
 */
typedef struct
{
	Object itemObj;   /**> Location of the CD object in SDR. */
	uint32_t size;    /**> Size of the CD object in the SDR */

	Object descObj;   /**> Location of this descr in SDR. */

} cd_desc_t;

// \todo make a single db desc object since these contain no data-typespecific units.

/**
 * This structure captures the contents of a Computed Data object. This
 * includes the definition of the object itself and, on the Agent side,
 * the last computed value of the definition.
 */
typedef struct
{
    mid_t *id;         /**> The identifier (name) of the CD. */
    value_t value; /**> The value of the CD. */

    /* Below items are not serialized with this structure. */
    cd_desc_t desc; /**> Descriptor of def in the SDR. */
} cd_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */



/* Create functions. */
cd_t *cd_create(mid_t *id,
				dtnmp_type_e type,
				expr_t *init,
				value_t val);

cd_t *cd_create_from_parms(Lyst parms);

cd_t *cd_deserialize(uint8_t *cursor,
		             uint32_t size,
		             uint32_t *bytes_used);

cd_t *cd_duplicate(cd_t *orig);

cd_t *cd_find_by_id(Lyst defs, ResourceLock *mutex, mid_t *id);

void cd_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy);

void cd_release(cd_t *cd);

uint8_t *cd_serialize(cd_t *cd, uint32_t *len);

char *cd_to_string(cd_t *cd);

#endif /* _CD_H_ */
