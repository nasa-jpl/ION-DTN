/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file ctrl.h
 **
 ** Description: This module contains the functions, structures, and other
 **              information used to capture both controls and macros.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  05/17/15  E. Birrane     Redesign around DTNMP v0.1 (Secure DTN - NASA: NNX14CS58P)
 **  11/18/18  E. Birrane     Update to latest AMP version. (JHU/APL)
 *****************************************************************************/
#ifndef _CTRL_H
#define _CTRL_H

#include "../utils/utils.h"
#include "../utils/nm_types.h"
#include "../utils/db.h"
#include "tnv.h"
#include "ari.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */



#define CTRL_SUCCESS (AMP_OK)
#define CTRL_FAILURE (AMP_FAIL)


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



typedef tnv_t* (*ctrldef_run_fn)(eid_t *def_mgr, tnvc_t *params, int8_t *status);

/**
 * Describes an ADM Control in the system.
 *
 * This structure captures general information about a control, including
 * its name an associated MID.
 */
typedef struct
{
    ari_t *ari;			  /**> The ID identifying this def.        */

    uint8_t num_parms;    /**> # params needed to complete this MID.*/

    ctrldef_run_fn run;  /**> Function implementing the control.   */

    db_desc_t desc;
} ctrldef_t;


typedef struct
{

	ari_t *ari;     /* The name of the macro. */
	vector_t ctrls; /* Of type ctrl_t*/

	db_desc_t desc;
} macdef_t;


typedef struct
{
	time_t start;   /**> ALways kept as an absolute time once rx.*/
	eid_t caller;   /**> EID of entity that created the control. */
	tnvc_t *parms;
	amp_type_e type;

	union {
		macdef_t *as_mac;   /**> Shallow pointers. Never free. */
		ctrldef_t *as_ctrl; /**> Shallow pointers. Never free. */
	} def;

	db_desc_t desc;
} ctrl_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

void ctrl_cb_del_fn(void *item);
int  ctrl_cb_comp_fn(void *i1, void *i2);
void *ctrl_cb_copy_fn(void *item);
rh_idx_t ctrl_cb_hash(void *table, void *key);



ctrl_t *ctrl_copy_ptr(ctrl_t *src);
ctrl_t *ctrl_create(ari_t *ari);


ctrl_t *ctrl_db_deserialize(blob_t *data);
blob_t *ctrl_db_serialize(ctrl_t *ctrl);

void*   ctrl_deserialize_ptr(QCBORDecodeContext *it, int *success);
ctrl_t* ctrl_deserialize_raw(blob_t *data, int *success);
ari_t*  ctrl_get_id(ctrl_t *ctrl);

void    ctrl_release(ctrl_t *ctrl, int destroy);
int     ctrl_serialize(QCBOREncodeContext *encoder, void *item);
blob_t* ctrl_serialize_wrapper(ctrl_t *ctrl);
void    ctrl_set_exec(ctrl_t *ctrl, time_t start, eid_t caller);

ctrldef_t *ctrldef_create(ari_t *ari, uint8_t num, ctrldef_run_fn run);
void       ctrldef_del_fn(rh_elt_t *elt);
void       ctrldef_release(ctrldef_t *def, int destroy);




int     macdef_append(macdef_t *mac, ctrl_t *ctrl);
int     macdef_append_ac(macdef_t *mac, ac_t *ac);

void    macdef_cb_del_fn(void *item);
int     macdef_cb_comp_fn(void *i1, void *i2);
void    macdef_cb_ht_del_fn(rh_elt_t *elt);

void    macdef_clear(macdef_t *mac);

macdef_t   macdef_copy(macdef_t *src, int *success);
macdef_t*  macdef_copy_ptr(macdef_t *src);

macdef_t*  macdef_create(size_t num, ari_t *ari);

macdef_t   macdef_deserialize(QCBORDecodeContext *it, int *success);
macdef_t*  macdef_deserialize_ptr(QCBORDecodeContext *it, int *success);
macdef_t   macdef_deserialize_raw(blob_t *data, int *success);

ctrl_t* macdef_get(macdef_t* mac, uint8_t index);
uint8_t macdef_get_count(macdef_t* mac);

int     macdef_init(macdef_t *mac, size_t num, ari_t *ari);
void    macdef_release(macdef_t *mac, int destroy);

int       macdef_serialize(QCBOREncodeContext *encoder, void *item);
blob_t*   macdef_serialize_wrapper(macdef_t *mac);



#endif // _CTRL_H
