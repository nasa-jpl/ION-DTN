/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file var.h
 **
 **
 ** Description: Structures that capture AMP EDD and Variable definitions.
 **
 ** Notes:
 **
 **  - Handles both EDD and VAR
 **  - A var_def is different than a var.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/05/16  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane     Renamed CD to VAR. (Secure DTN - NASA: NNX14CS58P)
 **  09/21/18  E. Birrane     Update to new AMP. Add EDDs. (JHU/APL)
 *****************************************************************************/

#ifndef _VAR_H_
#define _VAR_H_

#include "../utils/db.h"
#include "ari.h"
#include "tnv.h"
#include "expr.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define VAR_DEFAULT_ENC_SIZE (SMALL_SIZES * WORD_SIZE)
#define VARDEF_DEFAULT_ENC_SIZE (SMALL_SIZES * WORD_SIZE)

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

typedef tnv_t* (*edd_collect_fn)(tnvc_t *parms);



/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

typedef struct
{
	ari_t *id;
	edd_collect_fn collect;
} edd_def_t;


typedef struct
{
	edd_def_t def;
	tnvc_t *parms;
} edd_t;


typedef struct
{
	ari_t *id;
	amp_type_e type;
	expr_t *expr;
} var_def_t;

/**
 * This structure captures the contents of a Variable object. This
 * includes the definition of the object itself and, on the Agent side,
 * the last computed value of the definition.
 */
typedef struct
{
	ari_t *id;

    tnv_t *value; /**> The value of the variable. */

    /* Below items are not serialized with this structure. */
    db_desc_t desc; /**> Descriptor of def in the SDR. */
} var_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int       edd_cb_comp_fn(void *i1, void *i2);

void      edd_cb_del_fn(void *item);

void      edd_cb_ht_del(rh_elt_t *elt);

rh_idx_t  edd_cb_hash(void *table, void *key);

edd_t*    edd_create(ari_t *id, tnvc_t *parms, edd_collect_fn collect);

void      edd_release(edd_t *edd, int destroy);

int       var_cb_comp_fn(void *i1, void *i2);

void      var_cb_del_fn(void *item);

rh_idx_t  var_cb_hash(void *table, void *key);

void      var_cb_ht_del_fn(rh_elt_t *elt);

var_t*    var_copy_ptr(var_t *src);

var_t*    var_create(ari_t *id, amp_type_e type, expr_t *expr);

var_t*    var_create_from_def(var_def_t def);

var_t*    var_create_from_tnv(ari_t *id, tnv_t val);

var_t*    var_deserialize_ptr(QCBORDecodeContext *it, int *success);

var_t*    var_deserialize_raw(blob_t *data, int *success);

void      var_release(var_t *var, int destroy);

int       var_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*   var_serialize_wrapper(var_t *var);



var_def_t  vardef_deserialize(QCBORDecodeContext *it, int *success);

void       vardef_release(var_def_t *def, int destroy);

int        vardef_serialize(QCBOREncodeContext *encoder, void *item);
blob_t*    vardef_serialize_wrapper(var_def_t *def);



#endif /* _VAR_H_ */
