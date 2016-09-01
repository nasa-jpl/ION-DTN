/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
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
 *****************************************************************************/
#ifndef EXPR_H_
#define EXPR_H_

#include "../utils/ion_if.h"
#include "../utils/nm_types.h"

#include "../primitives/mid.h"
#include "../primitives/value.h"

typedef struct
{
	amp_type_e type;
	Lyst contents; /* MidCol */
} expr_t;


expr_t *expr_create(amp_type_e type, Lyst contents);

expr_t *expr_copy(expr_t *expr);

expr_t *expr_deserialize(uint8_t *cursor,
		             uint32_t size,
		             uint32_t *bytes_used);


void expr_release(expr_t *expr);

uint8_t *expr_serialize(expr_t *expr, uint32_t *len);

char *expr_to_string(expr_t *expr);



value_t expr_get_atomic(mid_t *mid);
value_t expr_get_computed(mid_t *mid);
value_t expr_get_literal(mid_t *mid);
value_t expr_get_val(mid_t *mid);
value_t expr_apply_op(mid_t *op, Lyst *stack);

value_t expr_eval(expr_t *expr);


#endif /* EXPR_H_ */
