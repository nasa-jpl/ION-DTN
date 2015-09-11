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
 **  07/28/13  E. Birrane     Initial Implementation
 *****************************************************************************/
#ifndef EXPR_H_
#define EXPR_H_

#include "shared/utils/ion_if.h"
#include "shared/utils/nm_types.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/value.h"

value_t* expr_get_atomic(mid_t *mid);
value_t* expr_get_computed(mid_t *mid);
value_t* expr_get_literal(mid_t *mid);
value_t* expr_get_val(mid_t *mid);
value_t* expr_apply_op(mid_t *op, Lyst *stack);

value_t expr_eval(Lyst expr);


#endif /* EXPR_H_ */
