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

#include "ion_if.h"
#include "nm_types.h"

#include "shared/primitives/mid.h"

/* 3 bits */
#define EXPR_TYPE_INT32   0
#define EXPR_TYPE_UINT32  1
#define EXPR_TYPE_VAST    2
#define EXPR_TYPE_UVAST   3
#define EXPR_TYPE_REAL32  4
#define EXPR_TYPE_REAL64  5
#define EXPR_TYPE_BLOB    6
#define EXPR_TYPE_STRING  7



/*
Literal... I need to know
: Type
: Value (pre-defined or in the OID structure.)
OID reserved for LITERAL. Parm 1 is type. Parm 2 is value.


Operator I need to know: This can be hard-coded to MID/OID.
- Operands (1, 2, 3)
- Result type
*/


typedef struct
{
	int type;
	uint8_t *value;
	uint64_t length;
} expr_result_t;


expr_result_t *expr_get_value(mid_t *mid);
expr_result_t *expr_apply_binary_op(mid_t *op, expr_result_t *expr1, expr_result_t *expr2);

expr_result_t *expr_eval(Lyst expr);

void expr_release(expr_result_t result);


#endif /* EXPR_H_ */
