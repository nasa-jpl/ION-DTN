/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
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
#include "../nm.h"
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



int expr_add_item(expr_t *expr, ari_t *item)
{
	if(expr == NULL)
	{
		return AMP_FAIL;
	}

	return ac_insert(&(expr->rpn), item);
}

// UNK means failure.
tnv_t *expr_apply_op(ari_t *id, vector_t *stack)
{
	op_t *op = NULL;
	tnv_t *result = NULL;

	AMP_DEBUG_ENTRY("expr_apply_op","("ADDR_FIELDSPEC", "ADDR_FIELDSPEC")",
			        (uaddr) id, (uaddr) stack);

	if((id == NULL) || (stack == NULL))
	{
		AMP_DEBUG_ERR("expr_apply_op","Bad parms", NULL);
		return NULL;
	}

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
expr_t *expr_create(amp_type_e type)
{
	expr_t *result = NULL;

	if((result = (expr_t*) STAKE(sizeof(expr_t))) == NULL)
	{
		return NULL;
	}

	result->type = type;
	ac_init(&(result->rpn));

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
	expr_t *result = NULL;

	if(expr == NULL)
	{
		return NULL;
	}

	if((result = expr_create(expr->type)) == NULL)
	{
		return NULL;
	}

	if(ac_append(&(result->rpn), &(expr->rpn)) != AMP_OK)
	{
		expr_release(result, 1);
		return NULL;
	}

	return result;
}


expr_t expr_deserialize(QCBORDecodeContext *it, int *success)
{
	expr_t result;
#if AMP_VERSION < 7
	blob_t *data = NULL;
#endif

	AMP_DEBUG_ENTRY("expr_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

	result.type = AMP_TYPE_UNK;
	CHKUSR(success, result);
	*success = AMP_FAIL;

	CHKUSR(it, result);

#if AMP_VERSION < 7
	/* Unpack the byte string holding the expression. */
	data = blob_deserialize_ptr(it, success);

	/* Grab and verify the expression type. */
	result.type = data->value[0];
#else
	cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &result.type);
#endif
    if(type_is_known(result.type) == 0)
    {
    	AMP_DEBUG_ERR("expr_deserialize","Unknown expression type %d", result.type);
    	*success = AMP_FAIL;
		
#if AMP_VERSION < 7
    	blob_release(data, 1);
#endif
		
    	return result;
    }

#if AMP_VERSION < 7
    /* Create fake blob to point after first byte. */
    blob_t tmp;
    tmp.value = data->value + sizeof(uint8_t);
    tmp.length = data->length - 1;
    tmp.alloc = data->alloc-1;
#endif

    /* Deserialize the AC list holding the RPN expression. */
#if AMP_VERSION < 7
    result.rpn = ac_deserialize_raw(&tmp, success);
    blob_release(data, 1);
#else
	result.rpn = ac_deserialize(it, success);
#endif
	
    if(*success != AMP_OK)
    {
    	result.type = AMP_TYPE_UNK;
    	ac_release(&(result.rpn), 0);
    	return result;
    }

    *success = AMP_OK;

    return result;
}

expr_t* expr_deserialize_ptr(QCBORDecodeContext *it, int *success)
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

expr_t* expr_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;

	if((data == NULL) || (success == NULL))
	{
		return NULL;
	}
	*success = AMP_FAIL;

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	expr_t *tmp = expr_deserialize_ptr(&it, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
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
	tnv_t *result = NULL;

	vector_t stack;
	vec_idx_t max;
	vecit_t it;
	int success;

	AMP_DEBUG_ENTRY("expr_eval","(0x"ADDR_FIELDSPEC")", (uaddr) expr);

	/* Sanity Checks. */
	if((expr == NULL) || ((max = vec_num_entries(expr->rpn.values)) == 0))
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
		ari_t *cur_ari = NULL;

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
			vec_release(&stack, 0);
			return NULL;
		}

		if(vec_push(&stack, result) != AMP_OK)
		{
			AMP_DEBUG_ERR("expr_eval","Can't push new values to stack at %d", vecit_idx(it));
			tnv_release(result, 1);
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
	vec_release(&stack, 0);

	if((success != AMP_OK) || (result == NULL))
	{
		AMP_DEBUG_ERR("expr_eval", "Cannot convert type.", NULL);
		tnv_release(result, 1);
		return NULL;
	}

	/* Step 5 - Convert the type. */
	if(expr->type != result->type)
	{
		tnv_t *tmp = tnv_cast(result, expr->type);
		if(tmp == NULL)
		{
			AMP_DEBUG_ERR("expr_eval", "Cannot convert from type %d to %d.", result->type, expr->type);
		}
		tnv_release(result, 1);
		result = tmp;
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
		result = tnv_copy_ptr(&(ari->as_lit));
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
		else if(edd->def.collect == NULL)
		{
			AMP_DEBUG_INFO("expr_get_edd","No collect function defined.", NULL);
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
        result = tnv_copy_ptr(var->value);
    }

	return result;
}



void expr_release(expr_t *expr, int destroy)
{
	CHKVOID(expr);

	ac_release(&(expr->rpn), 0);

	if(destroy)
	{
		SRELEASE(expr);
	}
}


int expr_serialize(QCBOREncodeContext *encoder, void *item)
{
#if AMP_VERSION < 7
	blob_t *result = NULL;
	blob_t *ac_data = NULL;
#endif
	expr_t *expr = (expr_t*) item;
	int err;

	if((encoder == NULL) || (item == NULL))
	{
		return AMP_FAIL;
	}
#if AMP_VERSION < 7
	result = blob_create((uint8_t*)&(expr->type), 1, 1);

	if((ac_data = ac_serialize_wrapper(&(expr->rpn))) == NULL)
	{
		AMP_DEBUG_ERR("expr_serialize","Can't serialize AC.", NULL);
		blob_release(result, 1);
		return AMP_FAIL;
	}

	blob_append(result, ac_data->value, ac_data->length);
	blob_release(ac_data, 1);

	err = blob_serialize(encoder, result);
	blob_release(result, 1);

#else
	err = cut_enc_byte(encoder, expr->type);
	if (err != AMP_OK)
	{
		AMP_DEBUG_ERR("expr_serialize", "Can't serialize type", NULL);
		return AMP_FAIL;
	}
	ac_serialize(encoder, &(expr->rpn));

#endif
	
	return err;
}


blob_t*   expr_serialize_wrapper(expr_t *expr)
{
	return cut_serialize_wrapper(EXPR_DEFAULT_ENC_SIZE, expr, (cut_enc_fn)expr_serialize);
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

int     op_cb_ht_comp_fn(void *key, void *value)
{
	op_t *op = (op_t*) value;
	CHKUSR(key, -1);
	CHKUSR(op, -1);
	return ari_cb_comp_fn(key, op->id);
}

op_t*     op_copy_ptr(op_t *src)
{
	ari_t *new_ari = NULL;

	CHKNULL(src);
	new_ari = ari_copy_ptr(src->id);
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

