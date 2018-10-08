/*****************************************************************************
 **
 ** \file metadata.h
 **
 **
 ** Description: Helper file holding meta-data associated with AMM objects such
 **              as their human-readable description and parameter specs.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/26/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  10/06/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef METADATA_H_
#define METADATA_H_

#include "nm_mgr.h"

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/ari.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */
#define META_NAME_MAX 32
#define META_DESCR_MAX 100
#define META_PARM_NAME 16


#define MAX_PARMS 8
#define MAX_PARM_NAME 16

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



typedef struct
{
	amp_type_e type;
	char name[META_PARM_NAME];
} parm_t;

typedef struct
{
	vector_t results; /* metadata_t */
	uint32_t adm_id;
	amp_type_e type;
} meta_col_t;


typedef struct
{
	ari_t *id;       /**> Parameterless ID of the object.       */
	uint32_t adm_id; /**> The ADM that defines this AMM object. */

    char name[META_NAME_MAX];   /**> The string name of the item.        */
    char descr[META_DESCR_MAX];  /**> The string description of the item. */

    vector_t parmspec; /* Parameter information for this item. (parm_t *) */

} metadata_t;


int         meta_add_parm(metadata_t *meta, char *name, amp_type_e type);
void        meta_cb_del(rh_elt_t *elt);
void        meta_cb_filter(void *value, void *tag);
metadata_t* meta_create(ari_t *id, uint32_t adm_id, char *name, char *desc);
meta_col_t* meta_filter(uint32_t adm_id, amp_type_e type);
parm_t*     meta_get_parm(metadata_t *meta, uint8_t idx);
void        meta_release(metadata_t *meta, int destroy);


meta_col_t* metacol_create();
void        metacol_release(meta_col_t*col, int destroy);


#endif // METADATA_H_
