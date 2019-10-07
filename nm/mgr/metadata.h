/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file metadata.h
 **
 **
 ** Description: Helper file holding meta-data associated with AMM objects such
 **              as their human-readable description and parameter specs. This
 **              is used by the NM manager to automate input for a variety of
 **              AMM objects.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/06/18  E. Birrane     Inital Implementation  (JHU/APL)
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
#define META_NAME_MAX 64
#define META_DESCR_MAX 1024
#define META_PARM_NAME 32


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
 * Formal Parameter specification.
 *
 * Captured the formal parameters for a parameterized AMM Object.
 */
typedef struct
{
	amp_type_e type;			/** The parameter type */
	char name[META_PARM_NAME];  /** The parameter name */
} meta_fp_t;


/**
 * Collection of Metadata.
 *
 * A metatdata collection is a subset of all known metadata in the system
 * that is collected by filtering the set of metatdata information
 * according to type and ADM id.
 *
 * An ADM id of ADM_ANY indicates that metatadata in the collection may be
 * from any ADM.
 *
 * A type of AMP_TYPE_UNK indicates that metatdata in the collection may
 * be of any AMM object type.
 *
 * NOTE: The results vector is populated with references directly into the
 * master metadata hashtable and, as such, the contents of the results
 * vector MUST NOT be released by anything working on this collection.
 *
 */
typedef struct
{
	vector_t results;  /** Contents of type metadata_t        */
	uint32_t adm_id;   /** Shared ADM ID of items in results. */
	amp_type_e type;   /** Shared type of items in results.   */
} meta_col_t;


/**
 * Metadata information
 *
 * This structure captures a set of metatadata associated with an AMM object
 * that informs the manager about how to populate, use, or match objects
 * as part of providing a user interface.
 *
 * The ID associated with the metadata, for efficiency, is a reference into
 * the AMM object instance held in the master list of objects of that type.
 * As such, this metadata structure MUST treat the ID as const and not try to
 * release it.
 *
 * NOTE: The type of a metadata object is dependent on the type of AMM object.
 *       For CNST, EDD, LIT, and VAR objects this is the type of their value.
 *       For OPER objects this is the resultant type of the operation.
 *       For all other objects this is AMP_TYPE_UNK.
 */
typedef struct
{
	ari_t *id;                  /**> Parameterless ID of the object.       */
	uint32_t adm_id;            /**> The ADM that defines this AMM object. */

	amp_type_e type;            /**> Base type of this AMM Object.         */

    char name[META_NAME_MAX];   /**> The string name of the item.          */
    char descr[META_DESCR_MAX]; /**> The string description of the item.   */

    vector_t parmspec;          /**> Contains items of type (meta_fp_t*)   */

} metadata_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


metadata_t* meta_add_edd(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_cnst(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_op(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_var(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_ctrl(ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_macro(ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_rpttpl(ari_t *id, uint8_t adm_id, char *name, char *descr);

metadata_t* meta_add_tblt(ari_t *id, uint8_t adm_id, char *name, char *descr);



int         meta_add_parm(metadata_t *meta, char *name, amp_type_e type);
void        meta_cb_del(rh_elt_t *elt);
void        meta_cb_filter(rh_elt_t *elt, void *tag);
metadata_t* meta_create(amp_type_e type, ari_t *id, uint32_t adm_id, char *name, char *desc);
meta_col_t* meta_filter(uint32_t adm_id, amp_type_e type);
meta_fp_t*  meta_get_parm(metadata_t *meta, uint8_t idx);
void        meta_release(metadata_t *meta, int destroy);

meta_col_t* metacol_create();
void        metacol_release(meta_col_t*col, int destroy);


#endif // METADATA_H_
