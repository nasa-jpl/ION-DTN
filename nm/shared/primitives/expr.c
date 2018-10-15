/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: expr.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP agents to construct and
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
 **  07/28/13  E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  09/21/18  E. Birrane     Update to AMP v03 (JHU/APL).
 *****************************************************************************/

#include "expr.h"
#include "../adm/adm.h"

#include "../utils/db.h"



int gValNumCvtResult[6][6] = {
{AMP_TYPE_INT,   AMP_TYPE_INT,   AMP_TYPE_VAST,  AMP_TYPE_UNK,   AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_INT,   AMP_TYPE_UINT,  AMP_TYPE_VAST,  AMP_TYPE_UVAST, AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_VAST,  AMP_TYPE_VAST,  AMP_TYPE_VAST,  AMP_TYPE_VAST,  AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_UNK,   AMP_TYPE_UVAST, AMP_TYPE_VAST,  AMP_TYPE_UVAST, AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL32,AMP_TYPE_REAL64},
{AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64,AMP_TYPE_REAL64}
};


// UNK means failure.
tnv_t *expr_apply_op(ari_t *id, vector_t *stack)
{
	op_t *op = NULL;
	tnv_t *result = tnv_create();

	AMP_DEBUG_ENTRY("expr_apply_op","("ADDR_FIELDSPEC", "ADDR_FIELDSPEC")",
			        (uaddr) id, (uaddr) stack);

	CHKNULL(result);

	/* Grab the operator instance. */
	if((op = VDB_FINDKEY_OP(id)) == NULL)
	{
		AMP_DEBUG_ERR("expr_apply_op","Can't find operator.", NULL);
		return result;
	}

	/* Calculate the result. */
	result = op->apply(stack);

	if(result->type == AMP_TYPE_UNK)
	{
		AMP_DEBUG_ERR("expr_apply_op","Can't apply operator.", NULL);
	}

	AMP_DEBUG_EXIT("expr_apply_op","->%d", result->type);

	return result;
}





/*
 * optype: 0 - Arithmetic
 *         1 - Logic
 */
int expr_calc_result_type(int ltype, int rtype, int optype)
{

	if(optype == 1)
	{
		return AMP_TYPE_UINT;
	}
	else if(ltype == rtype)
	{
		return ltype;
	}
	else if(type_is_numeric(ltype) && type_is_numeric(rtype))
	{
		/*
		 * \todo: Reconsider this design. Right now we normalize int to have
		 * DTNMP_TYPE_INT = 0 to do a lookup.
		 */
		return gValNumCvtResult[ltype-(int)AMP_TYPE_INT][rtype-(int)AMP_TYPE_INT];
	}

	return AMP_TYPE_UNK;
}


// Shallow copy.
expr_t *expr_create(amp_type_e type, ac_t rpn)
{
	expr_t *result = NULL;

	if((result = (expr_t*) STAKE(sizeof(expr_t))) == NULL)
	{
		return NULL;
	}

	result->type = type;
	result->rpn = rpn;

	return result;
}

expr_t expr_copy(expr_t expr)
{
	expr_t result;

	result.type = expr.type;
	result.rpn = ac_copy(&(expr.rpn));

	return result;
}

expr_t *expr_copy_ptr(expr_t *expr)
{
	ac_t rpn;
	expr_t *result = NULL;
	CHKNULL(expr);

	rpn = ac_copy(&(expr->rpn));

	if((result = (expr_create(expr->type, rpn))) == NULL)
	{
		ac_release(&rpn, 0);
	}

	return result;
}


expr_t expr_deserialize(CborValue *it, int *success)
{
	expr_t result;
	uint8_t *byte;

	AMP_DEBUG_ENTRY("expr_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

	result.type = AMP_TYPE_UNK;
	CHKUSR(success, result);
	*success = AMP_FAIL;

	CHKUSR(it, result);


	/* Get the expression type. */
	byte = (uint8_t *) cbor_value_get_next_byte(it);
	CHKUSR(byte, result);
	result.type = *byte;
    if(type_is_known(result.type) == 0)
    {
    	*success = AMP_FAIL;
    	return result;
    }

    /* Deserialize the AC list holding the RPN expression. */
    result.rpn = ac_deserialize(it, success);
    if(*success != AMP_OK)
    {
    	result.type = AMP_TYPE_UNK;
    	ac_release(&(result.rpn), 0);
    	return result;
    }

    *success = AMP_OK;

    return result;
}

expr_t* expr_deserialize_ptr(CborValue *it, int *success)
{
	expr_t *result = NULL;

	CHKNULL(success);
	*success = AMP_FAIL;

	result = STAKE(sizeof(expr_t));
	CHKNULL(result);

	*result = expr_deserialize(it, success);
	if(*success != AMP_OK)
	{
		expr_release(result, 1);
		result = NULL;
	}

	return result;
}



/*
 * TODO: Migrate some of this to top of file comments...?
 *
 * POSTFIX evaluation works as follows:
 * - An item is pulled from the expression. This item is either an
 *   operator (op) or an operand (data, literal).
 * - If the item is an operand, it is pushed on a temporary stack.
 * - If the item is an operator, the operator function is called
 *   passing in the stack. The parameters for the operator are
 *   removed from the stack, and the result of the operator is
 *   pushed back onto the stack.
 * - This process continues until there are no more items.
 *
 * Example: (3 + 4) / 5 becomes 3 4 + 7 /
 *
 * - We read 3 and 4 as operators and put them on the stack: S={3,4}
 * - We read +, which is a binary operator so we take top two
 *   parameters off the stack and add them, and push the result
 *   back on the stack, so the stack is now: S = {7}
 * - We read 5 and add it to the stack: S = {7,7}
 * - We read / which is a binary operator, so we take top two
 *   parameters off the stack anddivide them, and push the result
 *   back on the stack, so the stack is now S = {1}
 * - We are at the end of the process, so we verify we have just one
 *   value left on the stack, which we do, so the answer is 1.
 *
 *   The stack is a stack of value_t.
 */

tnv_t *expr_eval(expr_t *expr)
{
	tnv_t *result;

	vector_t stack;
	vec_idx_t max;
	vecit_t it;
	int success;
	ari_t *cur_ari = NULL;

	AMP_DEBUG_ENTRY("expr_eval","(0x"ADDR_FIELDSPEC")", (uaddr) expr);

	/* Sanity Checks. */

	CHKNULL(expr);
	result = tnv_create();
	CHKNULL(result);


	if((max = vec_num_entries(expr->rpn.values)) == 0)
	{
		return NULL;
	}

	/*
	 * Build the stack used to hold the results. This can't be larger than
	 * the RPN used to build it.
	 */
	stack = vec_create(max, tnv_cb_del, tnv_cb_comp, tnv_cb_copy, VEC_FLAG_AS_STACK, &success);
	if(success != AMP_OK)
	{
		return NULL;
	}

	for(it = vecit_first(&(expr->rpn.values)); vecit_valid(it); it = vecit_next(it))
	{
		if((cur_ari = (ari_t *) vecit_data(it)) == NULL)
		{
			AMP_DEBUG_ERR("expr_eval","Bad ARI in expression at %d.", vecit_idx(it));
			vec_release(&stack, 0);
			return NULL;
		}

		/* Step 2.2: Based on what kind of object this is,
		 *           calculate the new value to push on the stack.
		 */

		if(cur_ari->type == AMP_TYPE_OPER)
		{
			result = expr_apply_op(cur_ari, &stack);
		}
		else
		{
			result = expr_get_val(cur_ari);
		}

		if(result == NULL)
		{
			AMP_DEBUG_ERR("expr_eval","Can't apply Op at %d", vecit_idx(it));
			vec_release(&stack, 0);
			return NULL;
		}

		if(vec_push(&stack, result) != AMP_OK)
		{
			AMP_DEBUG_ERR("expr_eval","Can't push new values to stack at %d", vecit_idx(it));
			vec_release(&stack, 0);
			return NULL;
		}
	}

	/* Step 3 - Sanity check. We should have 1 result on the stack. */
	if(vec_num_entries(stack) > 1)
	{
		AMP_DEBUG_ERR("expr_eval","Stack has %d items?", vec_num_entries(stack));
		vec_release(&stack, 0);
		return NULL;
	}

	/* Step 4 - Get the last value and return it. */
	result = vec_pop(&stack, &success);
	if(success != AMP_OK)
	{
		AMP_DEBUG_ERR("expr_eval", "Cannot convert from type %d to %d.", result->type, expr->type);
		tnv_release(result, 1);
		return NULL;
	}
	vec_release(&stack, 0);

	/* Step 5 - Convert the type. */
	tnv_t *tmp = tnv_cast(result, expr->type);
	tnv_release(result, 1);
	result = tmp;
	if(result == NULL)
	{
		AMP_DEBUG_ERR("expr_eval", "Cannot convert from type %d to %d.", result->type, expr->type);
		return NULL;
	}

	return result;
}




tnv_t *expr_get_atomic(ari_t *ari)
{
	tnv_t *result = NULL;

    AMP_DEBUG_ENTRY("expr_get_edd","("ADDR_FIELDSPEC")", (uaddr) ari);

	CHKNULL(ari);

    /* Step 1: Handle special case of literal. */
	if(ari->type == AMP_TYPE_LIT)
	{
		result = tnv_copy_ptr(ari->as_lit);
	}
	else
	{
		edd_t *edd = NULL;

		if(ari->type == AMP_TYPE_EDD)
		{
			edd = VDB_FINDKEY_EDD(ari);
		}
		else
		{
			edd = VDB_FINDKEY_CONST(ari);
		}

		if(edd == NULL)
	    {
	      	AMP_DEBUG_INFO("expr_get_edd","Can't find def.", NULL);
	    	return NULL;
	    }

	    /* Step 2: Collect the value. */
	    result = edd->def.collect(&(ari->as_reg.parms));
	}

	AMP_DEBUG_EXIT("expr_get_edd", "("ADDR_FIELDSPEC")", (uaddr) result);

	return result;
}



/*
 * \todo Consider allowing value results from controls, macros.
 * \todo Check to exclude collections?
 */

tnv_t *expr_get_val(ari_t *ari)
{
	tnv_t *result = NULL;

	switch(ari->type)
	{
		case AMP_TYPE_LIT:
		case AMP_TYPE_EDD:
		case AMP_TYPE_CNST:
			result = expr_get_atomic(ari);
			break;
		case AMP_TYPE_VAR:
			result = expr_get_var(ari);
			break;
		default:
			break;
	}

	return result;
}


tnv_t *expr_get_var(ari_t *ari)
{
	tnv_t *result = NULL;
	var_t *var = NULL;

    AMP_DEBUG_ENTRY("expr_get_var","("ADDR_FIELDSPEC")", (uaddr) ari);

    CHKNULL(ari);

    if((var = VDB_FINDKEY_VAR(ari)) == NULL)
    {
    	AMP_DEBUG_ERR("expr_get_computed","Can't find var.", NULL);
    	return result;
	}

    /* Step 2: create ephermeral value to use in this evaluation. */
    if(var->value->type == AMP_TYPE_EXPR)
    {
    	/* \todo: limit recursion. */
        result = expr_eval((expr_t*)var->value->value.as_ptr);
    }
    else
    {
        result = tnv_copy_ptr(*(var->value));
    }

	return result;
}



void expr_release(expr_t *expr, int destroy)
{
	CHKVOID(expr);

	ac_release(&(expr->rpn), 1);

	if(destroy)
	{
		SRELEASE(expr);
	}
}



CborError expr_serialize(CborEncoder *encoder, void *item)
{
	CborError err;
	blob_t *result;
	expr_t *expr = (expr_t*) item;

	CHKUSR(encoder, CborErrorIO);
	CHKUSR(expr, CborErrorIO);

	/* Step 1: Encode the byte. */
	err = cut_enc_byte(encoder, expr->type);

	if((err != CborNoError) && (err != CborErrorOutOfMemory))
	{
		AMP_DEBUG_ERR("expr_serialize","CBOR Error: %d", err);
		return err;
	}

	result = ac_serialize_wrapper(&(expr->rpn));
	err = blob_serialize(encoder, result);
	blob_release(result, 1);

	return err;
}


blob_t*   expr_serialize_wrapper(expr_t *expr)
{
	return cut_serialize_wrapper(EXPR_DEFAULT_ENC_SIZE, expr, expr_serialize);
}



void    op_cb_del_fn(void *item)
{
	op_release((op_t*)item, 1);
}

int     op_cb_comp_fn(void *i1, void *i2)
{
	op_t *o1 = (op_t*) i1;
	op_t *o2 = (op_t*) i2;
	CHKUSR(o1, -1);
	CHKUSR(o2, -1);
	return ari_cb_comp_fn(o1->id, o2->id);
}

op_t*     op_copy_ptr(op_t *src)
{
	ari_t *new_ari = NULL;

	CHKNULL(src);
	new_ari = ari_copy_ptr(*(src->id));
	return op_create(new_ari, src->num_parms, src->apply);
}

void op_cb_ht_del_fn(rh_elt_t *elt)
{
	CHKVOID(elt);
	op_cb_del_fn(elt->value);
}

// shallow copy.
op_t* op_create(ari_t *ari, uint8_t num, op_fn apply)
{
	op_t *result = NULL;

	if((result = STAKE(sizeof(op_t))) == NULL)
	{
		return result;
	}
	result->id = ari;
	result->apply = apply;
	result->num_parms = num;

	return result;
}



void op_release(op_t *op, int destroy)
{
	CHKVOID(op);

	ari_release(op->id, 1);
	if(destroy)
	{
		SRELEASE(op);
	}
}




