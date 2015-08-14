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
 **  07/28/13  E. Birrane Initial Implementation
 *****************************************************************************/

#include "expr.h"
#include "shared/adm/adm.h"


value_t *expr_get_atomic(mid_t *mid)
{
	value_t *result = NULL;
	value_t tmp;
	adm_datadef_t *adm_def = NULL;

    DTNMP_DEBUG_ENTRY("expr_get_atomic","(0x%x)", (unsigned long) mid);

    /* Step 0: Sanity Checks. */
    if(mid == NULL)
    {
    	DTNMP_DEBUG_ERR("expr_get_atomic","Bad Args", NULL);
    	DTNMP_DEBUG_EXIT("expr_get_atomic","->NULL.", NULL);
    	return NULL;
    }

    /* Step 1: Find the atomic definition. */
    if((adm_def = adm_find_datadef(mid)) == NULL)
    {
      	DTNMP_DEBUG_INFO("expr_get_atomic","Can't find def.", NULL);
    	DTNMP_DEBUG_EXIT("expr_get_atomic","->NULL.", NULL);
    	return NULL;
    }

    /* Step 2: Collect the value. */
    tmp = adm_def->collect(mid->oid->params);

    if((result = val_as_ptr(tmp)) == NULL)
    {
      	DTNMP_DEBUG_INFO("expr_get_atomic","Def has bad value.", NULL);
    	DTNMP_DEBUG_EXIT("expr_get_atomic","->NULL.", NULL);
    	return NULL;
    }

	DTNMP_DEBUG_EXIT("expr_get_atomic", "->%#lx", (unsigned long) result);

	return result;
}



value_t *expr_get_computed(mid_t *mid)
{
	value_t *result = NULL;
	value_t tmp;
	def_gen_t *def = NULL;

    DTNMP_DEBUG_ENTRY("expr_get_computed","(0x%x)", (unsigned long) mid);

    /* Step 0: Sanity Checks. */
    if(mid == NULL)
    {
    	DTNMP_DEBUG_ERR("expr_get_computed","Bad Args", NULL);
    	DTNMP_DEBUG_EXIT("expr_get_computed","->NULL.", NULL);
    	return NULL;
    }

    /* Step 1: Find the atomic definition. */
    if((def = adm_find_computeddef(mid)) == NULL)
    {
      	DTNMP_DEBUG_INFO("expr_get_computed","Can't find def.", NULL);
    	DTNMP_DEBUG_EXIT("expr_get_computed","->NULL.", NULL);
    	return NULL;
    }

    /* Step 2: Collect the value. */
    /* \todo: We must prevent infinite recursion in definitions. */
    tmp = expr_eval(def->contents);

    if((result = val_as_ptr(tmp)) == NULL)
    {
      	DTNMP_DEBUG_INFO("expr_get_computed","Def has bad value.", NULL);
    	DTNMP_DEBUG_EXIT("expr_get_computed","->NULL.", NULL);
    	return NULL;
    }

	DTNMP_DEBUG_EXIT("expr_get_computed", "->%#lx", (unsigned long) result);

	return result;
}



value_t *expr_get_literal(mid_t *mid)
{
	value_t *result = NULL;
	value_t tmp;
	lit_t *lit = NULL;

	DTNMP_DEBUG_ENTRY("expr_get_literal","(%#llx)", (unsigned long) mid);

	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("expr_get_literal","Bad Parms.", NULL);
		DTNMP_DEBUG_EXIT("expr_get_literal","->NULL",NULL);
		return NULL;
	}

	if((lit = adm_find_lit(mid)) == NULL)
	{
		DTNMP_DEBUG_ERR("expr_get_literal","Can't find literal.", NULL);
		DTNMP_DEBUG_EXIT("expr_get_literal","->NULL",NULL);
		return NULL;
	}

	tmp = lit_get_value(lit);

	result = val_as_ptr(tmp);

	DTNMP_DEBUG_EXIT("expr_get_literal", "->%#lx", (unsigned long) result);

	return result;
}


/*
 * \todo Consider allowing value results from controls, macros.
 * \todo Check to exclude collections?
 */

value_t* expr_get_val(mid_t *mid)
{
	value_t* result = NULL;

	if(MID_GET_FLAG_TYPE(mid->flags) == MID_TYPE_DATA)
	{
		if(MID_GET_FLAG_CAT(mid->flags) == MID_CAT_ATOMIC)
		{
			result = expr_get_atomic(mid);
		}
		else if(MID_GET_FLAG_CAT(mid->flags) == MID_CAT_COMPUTED)
		{
			result = expr_get_computed(mid);
		}
	}
	else if(MID_GET_FLAG_TYPE(mid->flags) == MID_TYPE_LITERAL)
	{
		result = expr_get_literal(mid);
	}

	return result;
}


value_t* expr_apply_op(mid_t *op_mid, Lyst *stack)
{
	adm_op_t *op = NULL;
	value_t *result = NULL;
	value_t *cur = NULL;

	value_t tmp;
	int i;
	LystElt elt;

	DTNMP_DEBUG_ENTRY("expr_apply_op","(%#llx, %#llx)", (unsigned long) op_mid, (unsigned long) stack);

	if((op = adm_find_op(op_mid)) == NULL)
	{
		DTNMP_DEBUG_ERR("expr_apply_op","Can't find operator.", NULL);
		DTNMP_DEBUG_EXIT("expr_apply_op","->NULL",NULL);
		return NULL;
	}

	tmp = op->apply(*stack);

	if(tmp.type == DTNMP_TYPE_UNK)
	{
		DTNMP_DEBUG_ERR("expr_apply_op","Bad Op Value.", NULL);
		DTNMP_DEBUG_EXIT("expr_apply_op","->NULL",NULL);
		return NULL;
	}

	if((result = val_as_ptr(tmp)) == NULL)
	{
		DTNMP_DEBUG_ERR("expr_apply_op","Can't realloc result.", NULL);
		DTNMP_DEBUG_EXIT("expr_apply_op","->NULL",NULL);
		return NULL;
	}

	for(i = 0; i < op->num_parms; i++)
	{
		elt = lyst_first(*stack);
		cur = (value_t *) lyst_data(elt);
		val_release(cur);
		lyst_delete(elt);
	}

	DTNMP_DEBUG_EXIT("expr_apply_op","->%#llx",(unsigned long)result);

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

value_t expr_eval(Lyst expr)
{
	value_t result;
	value_t *tmp = NULL;
	Lyst stack = NULL;
	mid_t *cur_mid = NULL;
	LystElt elt;

	DTNMP_DEBUG_ENTRY("expr_eval","(%#llx)", (unsigned long) expr);

	val_init(&result);

	/* Step 0 - Sanity Checks. */
	if(expr == NULL)
	{
		DTNMP_DEBUG_ERR("expr_eval","Bad Parms.", NULL);
		DTNMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
		return result;
	}

	/* Step 1 - Create the stack. */
	if((stack = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("expr_eval","Can't create Stack Lyst.", NULL);
		DTNMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
		return result;
	}

	/* Step 2 - Walk the expression... */
	for(elt = lyst_first(expr); elt; elt = lyst_next(elt))
	{
		/* Step 2.1 - Grab the next item in the expression. */
		cur_mid = (mid_t *) lyst_data(elt);

		if(cur_mid == NULL)
		{
			valcol_destroy(&stack);

			DTNMP_DEBUG_ERR("expr_eval","Bad MID in expression.", NULL);
			DTNMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
			return result;
		}

		/* Step 2.2 - Figure out what kind of MID this is. */
		if(MID_GET_FLAG_TYPE(cur_mid->flags) == MID_TYPE_OPERATOR)
		{
			/* Step 2.2.1 - Calculate the new value and put it on the stack. */
			if((tmp = expr_apply_op(cur_mid, &stack)) == NULL)
			{
				char *msg = mid_to_string(cur_mid);

				if(msg != NULL)
				{
					DTNMP_DEBUG_ERR("expr_eval","No value for MID %s.", msg);
					MRELEASE(msg);
				}
				else
				{
					DTNMP_DEBUG_ERR("expr_eval","No value for MID ??.", NULL);
				}

				valcol_destroy(&stack);

				DTNMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
				return result;
			}

			/* Step 2.2.2 - Push the new result on th stack. */
			lyst_insert(stack, tmp);
		}
		else
		{
			if((tmp = expr_get_val(cur_mid)) == NULL)
			{
				char *msg = mid_to_string(cur_mid);

				if(msg != NULL)
				{
					DTNMP_DEBUG_ERR("expr_eval","No value for MID %s.", msg);
					MRELEASE(msg);
				}
				else
				{
					DTNMP_DEBUG_ERR("expr_eval","No value for MID ??.", NULL);
				}

				valcol_destroy(&stack);

				DTNMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
				return result;
			}

			lyst_insert(stack, tmp);
		}
	}

	/* Step 3 - Sanity check. We should have 1 result on the stack. */
	if(lyst_length(stack) > 1)
	{

		DTNMP_DEBUG_ERR("expr_eval","Stack has %d items?", lyst_length(stack));

		valcol_destroy(&stack);

		DTNMP_DEBUG_EXIT("expr_eval","->Unk", NULL);
		return result;
	}

	elt = lyst_first(stack);
	tmp = (value_t *) lyst_data(elt);

	result = *tmp;

	val_release(tmp);
	lyst_destroy(stack);

	return result;
}

