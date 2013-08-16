/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file def.h
 **
 **
 ** Description: Structures that capture protocol custom definitions.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/17/13  E. Birrane     Redesign of messaging architecture.
 *****************************************************************************/

#ifndef _DEF_H_
#define _DEF_H_

#include "mid.h"

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



typedef struct
{
	Object itemObj;     /**> Location of definition in SDR. */
	uint32_t size; /**> Size of definition in the SDR */

	Object descObj;    /**> Location of this descr in SDR. */

} def_gen_desc_t;

/*
 * Associated Message Type(s): MSG_TYPE_DEF_CUST_RPT
 *                             MSG_TYPE_DEF_COMP_DATA
 *                             MSG_TYPE_DEF_MACRO
 *
 * Purpose: Define custom item on an agent. Since many definitions in the
 *          standard have the same format, we use one data structure to
 *          represent them.
 *
 * +-------+------------+
 * |  ID   |  Contents  |
 * | (MID) |    (MC)    |
 * +-------+------------+
 */
typedef struct {
    mid_t *id;             /**> The identifier (name) of the report. */
    Lyst contents;         /**> The ordered MIDs comprising the definition. */

    /* Below items are not serialized with this structure. */

    def_gen_desc_t desc; /**> Descriptor of def in the SDR. */
} def_gen_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */



/* Create functions. */
def_gen_t *def_create_gen(mid_t *id,
					      Lyst contents);

def_gen_t *def_find_by_id(Lyst defs, ResourceLock *mutex, mid_t *id);

/* Release functions.*/
void def_release_gen(def_gen_t *def);


void def_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy);

void def_print_gen(def_gen_t *def);




#endif /* _DEF_H_ */
