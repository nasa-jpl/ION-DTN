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
 *****************************************************************************/

#include "expr.h"
#include "../adm/adm.h"

#ifdef AGENT_ROLE
#include "../../agent/agent_db.h"
#else
#include "../../mgr/mgr_db.h"
#endif


// Shallow copy.
expr_t *expr_create(amp_type_e type, Lyst contents)
{
	expr_t *result = NULL;

	if((result = (expr_t*) STAKE(sizeof(expr_t))) == NULL)
	{
		return NULL;
	}

	result->type = type;
	result->contents = contents;

	return result;
}

expr_t *expr_copy(expr_t *expr)
{
	CHKNULL(expr);
	return (expr_create(expr->type, midcol_copy(expr->contents)));
}


expr_t *expr_deserialize(uint8_t *cursor,
		             uint32_t size,
		             uint32_t *bytes_used)
{
	expr_t *result = NULL;
	uint32_t bytes = 0;

	amp_type_e type;
	Lyst contents;

	type = *cursor;
	cursor += 1;

	if((contents = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		*bytes_used = 0;
		return NULL;
	}

	if((result = expr_create(type, contents)) == NULL)
	{
		*bytes_used = 0;
		return NULL;
	}

	*bytes_used = bytes + 1;

	return result;
}

void expr_release(expr_t *expr)
{
	if(expr != NULL)
	{
		midcol_destroy(&(expr->contents));
		SRELEASE(expr);
	}
}

uint8_t *expr_serialize(expr_t *expr, uint32_t *len)
{
	uint8_t *result;
	uint8_t *content;
	uint32_t content_len;

	CHKNULL(expr);
	CHKNULL(len);

	content = midcol_serialize(expr->contents, &content_len);
	if((result = (uint8_t*) STAKE(content_len + 1)) == NULL)
	{
		SRELEASE(content);
		*len = 0;
		return NULL;
	}
	*len = content_len + 1;
	result[0] = expr->type;
	memcpy(&(result[1]), content, content_len);

	SRELEASE(content);
	return result;
}

char *expr_to_string(expr_t *expr)
{
	char *result = NULL;
	char *contents = NULL;
	uint32_t size = 0;

	CHKNULL(expr);
	if((contents = midcol_to_string(expr->contents)) == NULL)
	{
		AMP_DEBUG_ERR("expr_to_string","Can't to_str midcol.", NULL);
		return NULL;
	}

	size = strlen(contents) + 13;
	if((result = (char *) STAKE(size)) == NULL)
	{
		AMP_DEBUG_ERR("expr_to_string","Can't allocate %d bytes.", size);

		SRELEASE(contents);
		return NULL;
	}

	sprintf(result, "(%s) %s", type_to_str(expr->type), contents);
	SRELEASE(contents);

	return result;
}


value_t expr_get_atomic(mid_t *mid)
{
	value_t result;
	adm_datadef_t *adm_def = NULL;

    AMP_DEBUG_ENTRY("expr_get_atomic","(0x%x)", (unsigned long) mid);

    val_init(&result);

    /* Step 0: Sanity Checks. */
    if(mid == NULL)
    {
    	AMP_DEBUG_ERR("expr_get_atomic","Bad Args", NULL);
    	return result;
    }

    /* Step 1: Find the atomic definition. */
    if((adm_def = adm_find_datadef(mid)) == NULL)
    {
      	AMP_DEBUG_INFO("expr_get_atomic","Can't find def.", NULL);
    	return result;
    }

    /* Step 2: Collect the value. */
    result = adm_def->collect(mid->oid.params);

	AMP_DEBUG_EXIT("expr_get_atomic", "->%d", result.type);

	return result;
}



value_t expr_get_computed(mid_t *mid)
{
	value_t result;
	var_t *cd = NULL;

    AMP_DEBUG_ENTRY("expr_get_computed","(0x%x)", (unsigned long) mid);

    val_init(&result);

    /* Step 0: Sanity Checks. */
    if(mid == NULL)
    {
    	AMP_DEBUG_ERR("expr_get_computed","Bad Args", NULL);
    	return result;
    }

    /* Step 1: Find the computed definition. */
    if((cd = adm_find_computeddef(mid)) == NULL)
    {

#ifdef AGENT_ROLE
    	if((cd = var_find_by_id(gAgentVDB.vars, &(gAgentVDB.var_mutex), mid)) == NULL)
		{
	      	AMP_DEBUG_ERR("expr_get_computed","Can't find var.", NULL);
	    	return result;
		}
#else
		if((cd = var_find_by_id(gMgrVDB.compdata, &(gMgrVDB.compdata_mutex), mid)) == NULL)
		{
		  	AMP_DEBUG_ERR("expr_get_computed","Can't find var.", NULL);
		   	return result;
		}
#endif
    }

    /* Step 2: create ephermeral value to use in this evaluation. */
    if(cd->value.type == AMP_TYPE_EXPR)
    {
    	/* \todo: limit recursion. */
        result = expr_eval((expr_t*)cd->value.value.as_ptr);
    }
    else
    {
        result = val_copy(cd->value);
    }

	AMP_DEBUG_EXIT("expr_get_computed", "->%d", result.type);

	return result;
}



value_t expr_get_literal(mid_t *mid)
{
	value_t result;
	lit_t *lit = NULL;
	mid_t *tmp_mid = NULL;

	AMP_DEBUG_ENTRY("expr_get_literal","(%#llx)", (unsigned long) mid);

	val_init(&result);

	if(mid == NULL)
	{
		AMP_DEBUG_ERR("expr_get_literal","Bad Parms.", NULL);
    	return result;
	}

	if((lit = adm_find_lit(mid)) == NULL)
	{
		AMP_DEBUG_ERR("expr_get_literal","Can't find literal.", NULL);
    	return result;
	}

	/* Swap in MID with the actual parameters. */
	tmp_mid = lit->id;
	lit->id = mid;
	result = lit_get_value(lit);
	lit->id = tmp_mid;

	AMP_DEBUG_EXIT("expr_get_literal", "->%d", result.type);

	return result;
}


/*
 * \todo Consider allowing value results from controls, macros.
 * \todo Check to exclude collections?
 */

value_t expr_get_val(mid_t *mid)
{
	value_t result;

	val_init(&result);

	switch(MID_GET_FLAG_ID(mid->flags))
	{
	case MID_ATOMIC:
		result = expr_get_atomic(mid);
		break;
	case MID_COMPUTED:
		result = expr_get_computed(mid);
		break;
	case MID_LITERAL:
		result = expr_get_literal(mid);
		break;
	default:
		break;
	}

	return result;
}


value_t expr_apply_op(mid_t *op_mid, Lyst *stack)
{
	adm_op_t *op = NULL;
	value_t result;
	value_t *cur = NULL;

	int i;
	LystElt elt;

	AMP_DEBUG_ENTRY("expr_apply_op","(%#llx, %#llx)", (unsigned long) op_mid, (unsigned long) stack);

	val_init(&result);

	if((op = adm_find_op(op_mid)) == NULL)
	{
		AMP_DEBUG_ERR("expr_apply_op","Can't find operator.", NULL);
    	return result;
	}

	result = op->apply(*stack);

	for(i = 0; i < op->num_parms; i++)
	{
		elt = lyst_first(*stack);
		cur = (value_t *) lyst_data(elt);
		val_release(cur, 1);
		lyst_delete(elt);
	}

	AMP_DEBUG_EXIT("expr_apply_op","->%d", result.type);

	return result;
}


/*
 * Lyst is a midcol representing the expression in postix.
 *
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

value_t expr_eval(expr_t *expr)
{
	value_t result;
	value_t tmp;
	Lyst stack = NULL;
	mid_t *cur_mid = NULL;
	LystElt elt;

	AMP_DEBUG_ENTRY("expr_eval","(0x"ADDR_FIELDSPEC")", (uaddr) expr);

	val_init(&result);

	/* Step 0 - Sanity Checks. */
	if(expr == NULL)
	{
		AMP_DEBUG_ERR("expr_eval","Bad Parms.", NULL);
    	return result;
	}

	val_init(&result);
	val_init(&tmp);

	/* Step 2 - Create the stack. */
	if((stack = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("expr_eval","Can't create Stack Lyst.", NULL);
		return result;
	}

	/* Step 3 - Walk the expression... */
	for(elt = lyst_first(expr->contents); elt; elt = lyst_next(elt))
	{
		/* Step 2.1 - Grab the next item in the expression. */
		cur_mid = (mid_t *) lyst_data(elt);

		if(cur_mid == NULL)
		{
			valcol_destroy(&stack);

			AMP_DEBUG_ERR("expr_eval","Bad MID in expression.", NULL);
	    	return result;
		}

		/* Step 2.2 - Figure out what kind of MID this is. */
		if(MID_GET_FLAG_ID(cur_mid->flags) == MID_OPERATOR)
		{
			/* Step 2.2.1 - Calculate the new value and put it on the stack. */
			tmp = expr_apply_op(cur_mid, &stack);
			if(tmp.type == AMP_TYPE_UNK)
			{
				char *msg = mid_to_string(cur_mid);

				if(msg != NULL)
				{
					AMP_DEBUG_ERR("expr_eval","No value for MID %s.", msg);
					SRELEASE(msg);
				}
				else
				{
					AMP_DEBUG_ERR("expr_eval","No value for MID ??.", NULL);
				}

				valcol_destroy(&stack);
				AMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
		    	return result;
			}

			/* Step 2.2.2 - Push the new result on th stack. */
			lyst_insert(stack, val_ptr(tmp));
		}
		else
		{
			tmp = expr_get_val(cur_mid);
			if(tmp.type == AMP_TYPE_UNK)
			{
				char *msg = mid_to_string(cur_mid);

				if(msg != NULL)
				{
					AMP_DEBUG_ERR("expr_eval","No value for MID %s.", msg);
					SRELEASE(msg);
				}
				else
				{
					AMP_DEBUG_ERR("expr_eval","No value for MID ??.", NULL);
				}

				valcol_destroy(&stack);
				AMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
				return result;
			}

			lyst_insert(stack, val_ptr(tmp));
		}
	}

	/* Step 3 - Sanity check. We should have 1 result on the stack. */
	if(lyst_length(stack) > 1)
	{
		AMP_DEBUG_ERR("expr_eval","Stack has %d items?", lyst_length(stack));

		valcol_destroy(&stack);
		AMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
    	return result;
	}

	elt = lyst_first(stack);
	value_t *v_ptr = (value_t *) lyst_data(elt);

	if(val_cvt_type(v_ptr, expr->type) != 1)
	{
		AMP_DEBUG_ERR("expr_eval", "Cannot convert from type %d to %d.", v_ptr->type, expr->type);
		val_release(v_ptr, 1);
		return result;
	}

	result = val_copy(*v_ptr);
	val_release(v_ptr, 1);
	lyst_destroy(stack);

	return result;
}

