/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: expr.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to construct and
 **              evaluate expressions.
 **
 ** Notes:
 **
 ** Assumptions:
 **		1. For the time being, we only support unsigned long as a data type.
 **		2. For the time being, we assume operators are binary operators.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/28/13  E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  09/25/18  E. Birrane     Update to AMP v05 (JHU/APL)
 *****************************************************************************/
#ifndef EXPR_H_
#define EXPR_H_

#include "../utils/nm_types.h"

#include "ari.h"
#include "tnv.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */
#define VAL_OPTYPE_ARITHMETIC 0
#define VAL_OPTYPE_LOGICAL    1

#define EXPR_DEFAULT_ENC_SIZE 1024



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

/*
 * Arithmetic casting matrix.
 *
 * 			  INT     UINT	   VAST    UVAST     REAL32   REAL64
 *         +--------+--------+--------+--------+--------+--------+
 *  INT	   | INT    | INT	 | VAST   | UNK    | REAL32 | REAL64 |
 *  UINT   | INT    | UINT   | VAST   | UVAST  | REAL32 | REAL64 |
 *  VAST   | VAST   | VAST   | VAST   | VAST   | REAL32 | REAL64 |
 *  UVAST  | UNK    | UVAST  | VAST   | UVAST  | REAL32 | REAL64 |
 *  REAL32 | REAL32 | REAL32 | REAL32 | REAL32 | REAL32 | REAL64 |
 *  REAL64 | REAL64 | REAL64 | REAL64 | REAL64 | REAL64 | REAL64 |
 *         +--------+--------+--------+--------+--------+--------+
 *
 */
extern int gValNumCvtResult[6][6];

typedef tnv_t* (*op_fn)(vector_t *stack);


typedef struct
{
	ari_t *id;		    /**> The ARI identifying this def.        */
	uint8_t num_parms;  /**> # params needed to complete this MID.*/
	op_fn apply;        /**> Configured operator apply function. */
} op_t;


typedef struct
{
	amp_type_e type;
	ac_t rpn;
} expr_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int       expr_add_item(expr_t *expr, ari_t *item);

tnv_t*    expr_apply_op(ari_t *id, vector_t *stack);

int       expr_calc_result_type(int ltype, int rtype, int optype);

expr_t*   expr_create(amp_type_e type);

expr_t    expr_copy(expr_t expr);

expr_t*   expr_copy_ptr(expr_t *expr);

expr_t    expr_deserialize(QCBORDecodeContext *it, int *success);
expr_t*   expr_deserialize_ptr(QCBORDecodeContext *it, int *success);
expr_t*   expr_deserialize_raw(blob_t *data, int *success);

tnv_t*    expr_eval(expr_t *expr);

tnv_t*    expr_get_atomic(ari_t *ari);

tnv_t*    expr_get_val(ari_t *ari);
tnv_t*    expr_get_var(ari_t *ari);

void      expr_release(expr_t *expr, int destroy);

int       expr_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*   expr_serialize_wrapper(expr_t *expr);

void      op_cb_del_fn(void *item);
int       op_cb_comp_fn(void *i1, void *i2);

int       op_cb_ht_comp_fn(void *key, void *value);
void      op_cb_ht_del_fn(rh_elt_t *elt);

op_t*     op_copy_ptr(op_t *src);
op_t*     op_create(ari_t *ari, uint8_t num, op_fn apply);


void      op_release(op_t *op, int destroy);


#endif /* EXPR_H_ */
