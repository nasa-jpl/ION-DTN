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
 ** File Name: expr.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to construct and
 **              evaluate expressions.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/28/13  E. Birrane Initial Implementation
 *****************************************************************************/

#include "expr.h"

expr_result_t *expr_get_value(mid_t *mid)
{
	switch(MID_GET_FLAG_TYPE(mid->flags))
	{
		case MID_TYPE_DATA:
			/* Use the associated collect function */

			break;

		case MID_TYPE_LITERAL:
			break;
		case MID_TYPE_OPERATOR:
			break;
		default:
			break;
	}

}


expr_result_t *expr_apply_binary_op(mid_t *op, expr_result_t *expr1, expr_result_t *expr2)
{

}


expr_result_t *expr_eval(Lyst expr)
{

}

void expr_release(expr_result_t result)
{
	MRELEASE(result.value);
}
