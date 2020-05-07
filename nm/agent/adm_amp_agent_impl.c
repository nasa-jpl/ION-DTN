/****************************************************************************
 **
 ** File Name: adm_amp_agent_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2020-04-16  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/primitives/expr.h"
#include "../shared/primitives/tnv.h"
#include "instr.h"
#include "../shared/msg/msg.h"
#include "rda.h"
#include "ldc.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_amp_agent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
#define AMP_SAFE_MOD(a,b) ((b == 0) ? 0 : (a%b))
#define AMP_SAFE_DIV(a,b) ((b == 0) ? 0 : (a/b))
void amp_agent_collect_ari_keys(rh_elt_t *elt, void *tag)
{
	vector_t *vec = (vector_t *) tag;

	if((elt == NULL) || (vec == NULL))
	{
		return;
	}

	vec_push(vec, elt->key);
}

int amp_agent_build_ari_table(tbl_t *table, rhht_t *ht)
{
	int success;
	vector_t vec = vec_create(4, NULL, NULL, NULL, 0, &success);
	tnvc_t *cur_row;
	vecit_t it;

	if(success != VEC_OK)
	{
		return AMP_FAIL;
	}

	rhht_foreach(ht, amp_agent_collect_ari_keys, &vec);

	success = AMP_OK;
	for(it = vecit_first(&vec); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *ari = (ari_t*) vecit_data(it);
		cur_row = tnvc_create(1);
		tnvc_insert(cur_row, tnv_from_obj(AMP_TYPE_ARI, ari_copy_ptr(ari)));
		if(tbl_add_row(table, cur_row) != AMP_OK)
		{
			success = AMP_FAIL;
			break;
		}
	}
	vec_release(&vec, 0);
	return success;
}

int adm_agent_op_prep(uint8_t num, tnv_t **lval, tnv_t **rval, vector_t *stack)
{
	int success = AMP_OK;

	if(num > 0)
	{
		*rval = vec_pop(stack, &success);
	}
	else
	{
		*lval = vec_pop(stack, &success);
	}

	if((success == VEC_OK) && (num > 1))
	{
		*lval = vec_pop(stack, &success);
	}

	return success;
}


tnv_t *amp_agent_binary_num_op(amp_agent_op_e op, vector_t *stack, amp_type_e result_type)
{
	int ls = 0;
	int rs = 0;
	tnv_t *lval = NULL;
	tnv_t *rval = NULL;
	tnv_t *result = NULL;
	int success;

	if(stack == NULL)
	{
		return result;
	}

	success = adm_agent_op_prep(2, &lval, &rval, stack);
	if( (success != AMP_OK) || (lval == NULL) || (rval == NULL))
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return NULL;
	}

	// TODO more boundary checks.
	if((result = tnv_create()) == NULL)
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return NULL;
	}

	result->type = (result_type == AMP_TYPE_UNK) ? gValNumCvtResult[lval->type - AMP_TYPE_INT][rval->type - AMP_TYPE_INT] : result_type;

	if(result->type == AMP_TYPE_UNK)
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return NULL;
	}

    switch(result->type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
		case PLUS:   result->value.as_int = tnv_to_int(*lval, &ls) + tnv_to_int(*rval, &rs); break;
		case MINUS:  result->value.as_int = tnv_to_int(*lval, &ls) - tnv_to_int(*rval, &rs); break;
		case MULT:   result->value.as_int = tnv_to_int(*lval, &ls) * tnv_to_int(*rval, &rs); break;
		case DIV:    result->value.as_int = AMP_SAFE_DIV(tnv_to_int(*lval, &ls), tnv_to_int(*rval, &rs)); break;
		case MOD:    result->value.as_int = AMP_SAFE_MOD(tnv_to_int(*lval, &ls), tnv_to_int(*rval, &rs)); break;
		case EXP:    result->value.as_int = (int32_t) pow(tnv_to_int(*lval, &ls), tnv_to_int(*rval, &rs)); break;
		case BITAND: result->value.as_int = tnv_to_int(*lval, &ls) & tnv_to_int(*rval, &rs); break;
		case BITOR:  result->value.as_int = tnv_to_int(*lval, &ls) | tnv_to_int(*rval, &rs); break;
		case BITXOR: result->value.as_int = tnv_to_int(*lval, &ls) ^ tnv_to_int(*rval, &rs); break;
		case BITLSHFT: result->value.as_int = tnv_to_int(*lval, &ls) << tnv_to_uint(*rval, &rs); break;
		case BITRSHFT: result->value.as_int = tnv_to_int(*lval, &ls) >> tnv_to_uint(*rval, &rs); break;
		default:
			ls = rs = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
		case PLUS:   result->value.as_uint = tnv_to_uint(*lval, &ls) + tnv_to_uint(*rval, &rs); break;
		case MINUS:  result->value.as_uint = tnv_to_uint(*lval, &ls) - tnv_to_uint(*rval, &rs); break;
		case MULT:   result->value.as_uint = tnv_to_uint(*lval, &ls) * tnv_to_uint(*rval, &rs); break;
		case DIV:    result->value.as_uint = AMP_SAFE_DIV(tnv_to_uint(*lval, &ls), tnv_to_uint(*rval, &rs)); break;
		case MOD:    result->value.as_uint = AMP_SAFE_MOD(tnv_to_uint(*lval, &ls), tnv_to_uint(*rval, &rs)); break;
		case EXP:    result->value.as_uint = (uint32_t) pow(tnv_to_uint(*lval, &ls), tnv_to_uint(*rval, &rs)); break;
		case BITAND: result->value.as_uint = tnv_to_uint(*lval, &ls) & tnv_to_uint(*rval, &rs); break;
		case BITOR:  result->value.as_uint = tnv_to_uint(*lval, &ls) | tnv_to_uint(*rval, &rs); break;
		case BITXOR: result->value.as_uint = tnv_to_uint(*lval, &ls) ^ tnv_to_uint(*rval, &rs); break;
		case BITLSHFT: result->value.as_uint = tnv_to_uint(*lval, &ls) << tnv_to_uint(*rval, &rs); break;
		case BITRSHFT: result->value.as_uint = tnv_to_uint(*lval, &ls) >> tnv_to_uint(*rval, &rs); break;
		default:
			ls = rs = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
		case PLUS:   result->value.as_vast = tnv_to_vast(*lval, &ls) + tnv_to_vast(*rval, &rs); break;
		case MINUS:  result->value.as_vast = tnv_to_vast(*lval, &ls) - tnv_to_vast(*rval, &rs); break;
		case MULT:   result->value.as_vast = tnv_to_vast(*lval, &ls) * tnv_to_vast(*rval, &rs); break;
		case DIV:    result->value.as_vast = AMP_SAFE_DIV(tnv_to_vast(*lval, &ls), tnv_to_vast(*rval, &rs)); break;
		case MOD:    result->value.as_vast = AMP_SAFE_MOD(tnv_to_vast(*lval, &ls), tnv_to_vast(*rval, &rs)); break;
		case EXP:    result->value.as_vast = (vast) pow(tnv_to_vast(*lval, &ls), tnv_to_vast(*rval, &rs)); break;
		case BITAND: result->value.as_vast = tnv_to_vast(*lval, &ls) & tnv_to_vast(*rval, &rs); break;
		case BITOR:  result->value.as_vast = tnv_to_vast(*lval, &ls) | tnv_to_vast(*rval, &rs); break;
		case BITXOR: result->value.as_vast = tnv_to_vast(*lval, &ls) ^ tnv_to_vast(*rval, &rs); break;
		case BITLSHFT: result->value.as_vast = tnv_to_vast(*lval, &ls) << tnv_to_uint(*rval, &rs); break;
		case BITRSHFT: result->value.as_vast = tnv_to_vast(*lval, &ls) >> tnv_to_uint(*rval, &rs); break;
		default:
			ls = rs = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
		case PLUS:   result->value.as_uvast = tnv_to_uvast(*lval, &ls) + tnv_to_uvast(*rval, &rs); break;
		case MINUS:  result->value.as_uvast = tnv_to_uvast(*lval, &ls) - tnv_to_uvast(*rval, &rs); break;
		case MULT:   result->value.as_uvast = tnv_to_uvast(*lval, &ls) * tnv_to_uvast(*rval, &rs); break;
		case DIV:    result->value.as_uvast = AMP_SAFE_DIV(tnv_to_uvast(*lval, &ls), tnv_to_uvast(*rval, &rs)); break;
		case MOD:    result->value.as_uvast = AMP_SAFE_MOD(tnv_to_uvast(*lval, &ls), tnv_to_uvast(*rval, &rs)); break;
		case EXP:    result->value.as_uvast = (uvast) pow(tnv_to_uvast(*lval, &ls), tnv_to_uvast(*rval, &rs)); break;
		case BITAND: result->value.as_uvast = tnv_to_uvast(*lval, &ls) & tnv_to_uvast(*rval, &rs); break;
		case BITOR:  result->value.as_uvast = tnv_to_uvast(*lval, &ls) | tnv_to_uvast(*rval, &rs); break;
		case BITXOR: result->value.as_uvast = tnv_to_uvast(*lval, &ls) ^ tnv_to_uvast(*rval, &rs); break;
		case BITLSHFT: result->value.as_uvast = tnv_to_uvast(*lval, &ls) << tnv_to_uint(*rval, &rs); break;
		case BITRSHFT: result->value.as_uvast = tnv_to_uvast(*lval, &ls) >> tnv_to_uint(*rval, &rs); break;
		default:
			ls = rs = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
		case PLUS:   result->value.as_real32 = tnv_to_real32(*lval, &ls) + tnv_to_real32(*rval, &rs); break;
		case MINUS:  result->value.as_real32 = tnv_to_real32(*lval, &ls) - tnv_to_real32(*rval, &rs); break;
		case MULT:   result->value.as_real32 = tnv_to_real32(*lval, &ls) * tnv_to_real32(*rval, &rs); break;
		case DIV:    result->value.as_real32 = AMP_SAFE_DIV(tnv_to_real32(*lval, &ls), tnv_to_real32(*rval, &rs)); break;
		case EXP:    result->value.as_real32 = (float) pow(tnv_to_real32(*lval, &ls), tnv_to_real32(*rval, &rs)); break;
		default:
			ls = rs = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
		case PLUS:   result->value.as_real64 = tnv_to_real64(*lval, &ls) + tnv_to_real64(*rval, &rs); break;
		case MINUS:  result->value.as_real64 = tnv_to_real64(*lval, &ls) - tnv_to_real64(*rval, &rs); break;
		case MULT:   result->value.as_real64 = tnv_to_real64(*lval, &ls) * tnv_to_real64(*rval, &rs); break;
		case DIV:    result->value.as_real64 = AMP_SAFE_DIV(tnv_to_real64(*lval, &ls), tnv_to_real64(*rval, &rs)); break;
		case EXP:    result->value.as_real64 = (double) pow(tnv_to_real64(*lval, &ls), tnv_to_real64(*rval, &rs)); break;
		default: ls = rs = 0; break;
		}
		break;
		default:
			ls = rs = AMP_FAIL; break;
	}

	if((ls != AMP_OK) || (rs != AMP_OK))
	{
        AMP_DEBUG_ERR("adm_agent_binary_num_op","Bad op (%d) or type (%d -> %d).",op, lval->type, rval->type);
        tnv_release(result, 1);
        result = NULL;
	}

	tnv_release(lval, 1);
	tnv_release(rval, 1);

	return result;
}

tnv_t *adm_agent_unary_num_op(amp_agent_op_e op, vector_t *stack, amp_type_e result_type)
{
	int ls = 0;
	tnv_t *lval = NULL;
	tnv_t *result = NULL;
	int success;

	if(stack == NULL)
	{
		return result;
	}

	success = adm_agent_op_prep(1, &lval, NULL, stack);
	if( (success != AMP_OK) || (lval == NULL))
	{
		tnv_release(lval, 1);
		return NULL;
	}

	// TODO more boundary checks.
	if((result = tnv_create()) == NULL)
	{
		tnv_release(lval, 1);
		return NULL;
	}

	result->type = (result_type == AMP_TYPE_UNK) ? gValNumCvtResult[lval->type - AMP_TYPE_INT][lval->type - AMP_TYPE_INT] : result_type;

	if(result->type == AMP_TYPE_UNK)
	{
		tnv_release(lval, 1);
		return NULL;
	}

    switch(result->type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
		case BITNOT: result->value.as_int = ~(tnv_to_int(*lval, &ls)); break;
		case ABS:    result->value.as_int = abs(tnv_to_int(*lval, &ls)); break;
		case NEG:    result->value.as_int = -1 * (tnv_to_int(*lval, &ls)); break;
		default:
			ls = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
		case BITNOT: result->value.as_uint = ~(tnv_to_uint(*lval, &ls)); break;
		case ABS:    result->value.as_uint = abs(tnv_to_int(*lval, &ls)); break;
		case NEG:    result->value.as_uint = -1 * (tnv_to_uint(*lval, &ls)); break;
		default:
			ls = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
		case BITNOT: result->value.as_vast = ~(tnv_to_vast(*lval, &ls)); break;
		case ABS:    result->value.as_vast = abs((int)tnv_to_vast(*lval, &ls)); break;
		case NEG:    result->value.as_vast = -1 * (tnv_to_vast(*lval, &ls)); break;
		default:
			ls = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
		case BITNOT: result->value.as_uvast = ~(tnv_to_uvast(*lval, &ls)); break;
		case ABS:    result->value.as_uvast = abs((int)tnv_to_vast(*lval, &ls)); break;
		case NEG:    result->value.as_uvast = -1 * (tnv_to_uvast(*lval, &ls)); break;
		default:
			ls = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
		case ABS:    result->value.as_real32 = fabs(tnv_to_real32(*lval, &ls)); break;
		case NEG:    result->value.as_real32 = -1 * (tnv_to_real32(*lval, &ls)); break;
		default:
			ls = AMP_FAIL; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
		case ABS:    result->value.as_real64 = fabs(tnv_to_real64(*lval, &ls)); break;
		case NEG:    result->value.as_real64 = -1 * (tnv_to_real64(*lval, &ls)); break;
		default: ls = AMP_FAIL; break;
		}
		break;
		default:
			ls = AMP_FAIL; break;
	}

	if(ls != AMP_OK)
	{
        AMP_DEBUG_ERR("adm_agent_binary_num_op","Bad op (%d) or type (%d -> %d).",op, lval->type);
        tnv_release(result, 1);
        result = NULL;
	}

	tnv_release(lval, 1);

	return result;
}

tnv_t *adm_agent_unary_log_op(amp_agent_op_e op, vector_t *stack)
{
	tnv_t *result = NULL;
	tnv_t *val = NULL;
	int s = 0;
	int success = AMP_FAIL;

	if(stack == NULL)
	{
		return result;
	}

	success = adm_agent_op_prep(1, &val, NULL, stack);
	if((success != AMP_OK) || (val == NULL))
	{
		tnv_release(result, 1);
		return NULL;
	}

	// TODO more boundary checks.
	if((result = tnv_create()) == NULL)
	{
		tnv_release(val, 1);
		return NULL;
	}

	result->type = AMP_TYPE_BOOL;

	switch(val->type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
			case LOGNOT: result->value.as_uint = tnv_to_int(*val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
			case LOGNOT: result->value.as_uint = tnv_to_uint(*val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
			case LOGNOT: result->value.as_uint = tnv_to_vast(*val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
			case LOGNOT: result->value.as_uint = tnv_to_uvast(*val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
			default: s = 0; break;
		}
		break;
	default:
		s = 0;
		break;
	}

    if(s == 0)
	{
        AMP_DEBUG_ERR("adm_agent_unary_log_op","Bad op (%d) or type (%d).",op, val->type);
        tnv_release(result, 1);
        result = NULL;
	}

	tnv_release(val, 1);

	return result;
}



tnv_t *adm_agent_binary_log_op(amp_agent_op_e op, vector_t *stack)
{
	int ls = 0;
	int rs = 0;
	tnv_t *lval = NULL;
	tnv_t *rval = NULL;
	tnv_t *result = NULL;
	int success;


	if(stack == NULL)
	{
		return result;
	}

	success = adm_agent_op_prep(2, &lval, &rval, stack);
	if( (success != AMP_OK) || (lval == NULL) || (rval == NULL))
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return NULL;
	}

	// TODO more boundary checks.
	if((result = tnv_create()) == NULL)
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return NULL;
	}

	result->type = AMP_TYPE_BOOL;

	/* Step 3: Based on result type, convert and perform operations. */
    switch(lval->type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
		case LOGAND: result->value.as_uint = tnv_to_int(*lval, &ls) && tnv_to_int(*rval, &rs); break;
		case LOGOR:  result->value.as_uint = tnv_to_int(*lval, &ls) || tnv_to_int(*rval, &rs); break;
		case LT:     result->value.as_uint = tnv_to_int(*lval, &ls) < tnv_to_int(*rval, &rs); break;
		case GT:     result->value.as_uint = tnv_to_int(*lval, &ls) > tnv_to_int(*rval, &rs); break;
		case LTE:    result->value.as_uint = tnv_to_int(*lval, &ls) <= tnv_to_int(*rval, &rs); break;
		case GTE:    result->value.as_uint = tnv_to_int(*lval, &ls) >= tnv_to_int(*rval, &rs); break;
		case EQ:     result->value.as_uint = tnv_to_int(*lval, &ls) == tnv_to_int(*rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
		case LOGAND: result->value.as_uint = tnv_to_uint(*lval, &ls) && tnv_to_uint(*rval, &rs); break;
		case LOGOR:  result->value.as_uint = tnv_to_uint(*lval, &ls) || tnv_to_uint(*rval, &rs); break;
		case LT:     result->value.as_uint = tnv_to_uint(*lval, &ls) < tnv_to_uint(*rval, &rs); break;
		case GT:     result->value.as_uint = tnv_to_uint(*lval, &ls) > tnv_to_uint(*rval, &rs); break;
		case LTE:    result->value.as_uint = tnv_to_uint(*lval, &ls) <= tnv_to_uint(*rval, &rs); break;
		case GTE:    result->value.as_uint = tnv_to_uint(*lval, &ls) >= tnv_to_uint(*rval, &rs); break;
		case EQ:     result->value.as_uint = tnv_to_uint(*lval, &ls) == tnv_to_uint(*rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
		case LOGAND: result->value.as_uint = tnv_to_vast(*lval, &ls) && tnv_to_vast(*rval, &rs); break;
		case LOGOR:  result->value.as_uint = tnv_to_vast(*lval, &ls) || tnv_to_vast(*rval, &rs); break;
		case LT:     result->value.as_uint = tnv_to_vast(*lval, &ls) < tnv_to_vast(*rval, &rs); break;
		case GT:     result->value.as_uint = tnv_to_vast(*lval, &ls) > tnv_to_vast(*rval, &rs); break;
		case LTE:    result->value.as_uint = tnv_to_vast(*lval, &ls) <= tnv_to_vast(*rval, &rs); break;
		case GTE:    result->value.as_uint = tnv_to_vast(*lval, &ls) >= tnv_to_vast(*rval, &rs); break;
		case EQ:     result->value.as_uint = tnv_to_vast(*lval, &ls) == tnv_to_vast(*rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
		case LOGAND: result->value.as_uint = tnv_to_uvast(*lval, &ls) && tnv_to_uvast(*rval, &rs); break;
		case LOGOR:  result->value.as_uint = tnv_to_uvast(*lval, &ls) || tnv_to_uvast(*rval, &rs); break;
		case LT:     result->value.as_uint = tnv_to_uvast(*lval, &ls) < tnv_to_uvast(*rval, &rs); break;
		case GT:     result->value.as_uint = tnv_to_uvast(*lval, &ls) > tnv_to_uvast(*rval, &rs); break;
		case LTE:    result->value.as_uint = tnv_to_uvast(*lval, &ls) <= tnv_to_uvast(*rval, &rs); break;
		case GTE:    result->value.as_uint = tnv_to_uvast(*lval, &ls) >= tnv_to_uvast(*rval, &rs); break;
		case EQ:     result->value.as_uint = tnv_to_uvast(*lval, &ls) == tnv_to_uvast(*rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
		case LOGAND: result->value.as_uint = tnv_to_real32(*lval, &ls) && tnv_to_real32(*rval, &rs); break;
		case LOGOR:  result->value.as_uint = tnv_to_real32(*lval, &ls) || tnv_to_real32(*rval, &rs); break;
		case LT:     result->value.as_uint = tnv_to_real32(*lval, &ls) < tnv_to_real32(*rval, &rs); break;
		case GT:     result->value.as_uint = tnv_to_real32(*lval, &ls) > tnv_to_real32(*rval, &rs); break;
		case LTE:    result->value.as_uint = tnv_to_real32(*lval, &ls) <= tnv_to_real32(*rval, &rs); break;
		case GTE:    result->value.as_uint = tnv_to_real32(*lval, &ls) >= tnv_to_real32(*rval, &rs); break;
		case EQ:     result->value.as_uint = tnv_to_real32(*lval, &ls) == tnv_to_real32(*rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
		case LOGAND: result->value.as_uint = tnv_to_real64(*lval, &ls) && tnv_to_real64(*rval, &rs); break;
		case LOGOR:  result->value.as_uint = tnv_to_real64(*lval, &ls) || tnv_to_real64(*rval, &rs); break;
		case LT:     result->value.as_uint = tnv_to_real64(*lval, &ls) < tnv_to_real64(*rval, &rs); break;
		case GT:     result->value.as_uint = tnv_to_real64(*lval, &ls) > tnv_to_real64(*rval, &rs); break;
		case LTE:    result->value.as_uint = tnv_to_real64(*lval, &ls) <= tnv_to_real64(*rval, &rs); break;
		case GTE:    result->value.as_uint = tnv_to_real64(*lval, &ls) >= tnv_to_real64(*rval, &rs); break;
		case EQ:     result->value.as_uint = tnv_to_real64(*lval, &ls) == tnv_to_real64(*rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	default:
		ls = rs = 0;
		break;
	}

    if((ls == 0) || (rs == 0))
	{
        AMP_DEBUG_ERR("adm_agent_binary_log_op","Bad op (%d) or type (%d -> %d).",op, lval->type, rval->type);
        tnv_release(result, 1);
        result = NULL;
	}

    tnv_release(lval, 1);
	tnv_release(rval, 1);

	return result;
}



/*   STOP CUSTOM FUNCTIONS HERE  */

void amp_agent_setup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void amp_agent_cleanup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


tnv_t *amp_agent_meta_name(tnvc_t *parms)
{
	return tnv_from_str("amp_agent");
}


tnv_t *amp_agent_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("Amp/Agent");
}


tnv_t *amp_agent_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v3.1");
}


tnv_t *amp_agent_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
tnv_t *amp_agent_get_amp_epoch(tnvc_t *parms)
{
	return tnv_from_uvast(1504915200);
}

/* Table Functions */


/*
 * This table lists all the adms that are supported by the agent.
 */
tbl_t *amp_agent_tblt_adms(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_adms BODY
	 * +-------------------------------------------------------------------------+
	 */
	tnvc_t *cur_row = NULL;

	cur_row = tnvc_create(1);
	tnvc_insert(cur_row, tnv_from_str("AMP AGENT"));
	if(tbl_add_row(table, cur_row) != AMP_OK)
	{
		tbl_release(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_adms BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every variable that is known to the agent.
 */
tbl_t *amp_agent_tblt_variables(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_variables BODY
	 * +-------------------------------------------------------------------------+
	 */
	if(amp_agent_build_ari_table(table, &(gVDB.vars)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_variables BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every report template that is known to the agent.
 */
tbl_t *amp_agent_tblt_rptts(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_rptts BODY
	 * +-------------------------------------------------------------------------+
	 */
	if(amp_agent_build_ari_table(table, &(gVDB.rpttpls)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_rptts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every macro that is known to the agent.
 */
tbl_t *amp_agent_tblt_macros(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_macros BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_build_ari_table(table, &(gVDB.macdefs)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_macros BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every rule that is known to the agent.
 */
tbl_t *amp_agent_tblt_rules(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_rules BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_build_ari_table(table, &(gVDB.rules)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every table template that is known to the agent.
 */
tbl_t *amp_agent_tblt_tblts(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_tblts BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_build_ari_table(table, &(gVDB.adm_tblts)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_tblts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is the number of report templates known to the Agent.
 */
tnv_t *amp_agent_get_num_rpt_tpls(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_rpt_tpls BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_from_uint(gVDB.rpttpls.num_elts);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_rpt_tpls BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of table templates known to the Agent.
 */
tnv_t *amp_agent_get_num_tbl_tpls(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_tbl_tpls BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gVDB.adm_tblts.num_elts);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_tbl_tpls BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of reports sent by the agent.
 */
tnv_t *amp_agent_get_sent_reports(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_sent_reports BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gAgentInstr.num_sent_rpts);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_sent_reports BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of time-based rules running on the agent.
 */
tnv_t *amp_agent_get_num_tbr(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gAgentInstr.num_tbrs);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of time-based rules run by the agent since the last reset.
 */
tnv_t *amp_agent_get_run_tbr(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_run_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gAgentInstr.num_tbrs_run);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_run_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of state-based rules running on the agent.
 */
tnv_t *amp_agent_get_num_sbr(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gAgentInstr.num_sbrs);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of state-based rules run by the agent since the last reset.
 */
tnv_t *amp_agent_get_run_sbr(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_run_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gAgentInstr.num_sbrs_run);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_run_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of constants known by the agent.
 */
tnv_t *amp_agent_get_num_const(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_const BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gVDB.adm_atomics.num_elts);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_const BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of variables known by the agent.
 */
tnv_t *amp_agent_get_num_var(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_var BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = tnv_from_uint(gVDB.vars.num_elts);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_var BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of macros known by the agent.
 */
tnv_t *amp_agent_get_num_macros(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_macros BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_from_uint(gVDB.macdefs.num_elts);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_macros BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of macros run by the agent since the last reset.
 */
tnv_t *amp_agent_get_run_macros(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_run_macros BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_from_uint(gAgentInstr.num_macros_run);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_run_macros BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of controls known by the agent.
 */
tnv_t *amp_agent_get_num_controls(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_controls BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_from_uint(gVDB.ctrls.total_slots - gVDB.ctrls.num_free);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_controls BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the number of controls run by the agent since the last reset.
 */
tnv_t *amp_agent_get_run_controls(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_run_controls BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_from_uint(gAgentInstr.num_ctrls_run);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_run_controls BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This is the current system time.
 */
tnv_t *amp_agent_get_cur_time(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_cur_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	struct timeval cur_time;

	getCurrentTime(&cur_time);

	result = tnv_from_uvast(cur_time.tv_sec);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_cur_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * This control configures a new variable definition on the Agent.
 */
tnv_t *amp_agent_ctrl_add_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_var BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	expr_t *expr = adm_get_parm_obj(parms, 1, AMP_TYPE_EXPR);
	amp_type_e type = adm_get_parm_uint(parms, 2, &success);

	if((id == NULL) || (expr == NULL) || (type == AMP_TYPE_UNK))
	{
		AMP_DEBUG_ERR("ADD_VAR", "Bad parameters for control", NULL);
		return result;
	}

	var_t *new_var = var_create(ari_copy_ptr(id), type, expr_copy_ptr(expr));

	if(new_var == NULL)
	{
		AMP_DEBUG_ERR("ADD_VAR","Unable to make new var.", NULL);
		return result;
	}

	if(VDB_FINDKEY_VAR(new_var->id) == NULL)
	{
		int rh_code = VDB_ADD_VAR(new_var->id, new_var);

		if(rh_code != RH_OK)
		{
			var_release(new_var, 1);
		}
		else
		{
			*status = CTRL_SUCCESS;
			db_persist_var(new_var);
		}
	}
	else
	{
		*status = CTRL_SUCCESS;
		AMP_DEBUG_WARN("ADD_VAR","Ignoring duplicate item.", NULL);
		var_release(new_var, 1);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_var BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes one or more variable definitions from the Agent.
 */
tnv_t *amp_agent_ctrl_del_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_var BODY
	 * +-------------------------------------------------------------------------+
	 */
	vecit_t it;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DEL_VAR", "Bad parameters.", NULL);
		return result;
	}

	for(it = vecit_first(&(ids->values)); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *cur_id = vecit_data(it);
		var_t *var = VDB_FINDKEY_VAR(cur_id);

		if(var == NULL)
		{
			AMP_DEBUG_WARN("DEL_VAR", "Cannot find var to be deleted.", NULL);
		}
		else
		{
			db_forget(&(var->desc), gDB.vars);
			VDB_DELKEY_VAR(cur_id);
		}
	}

	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_var BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures a new report template definition on the Agent.
 */
tnv_t *amp_agent_ctrl_add_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	ac_t *template = adm_get_parm_obj(parms, 1, AMP_TYPE_AC);
	rpttpl_t *def = NULL;

	if((id == NULL) || (template == NULL))
	{
		AMP_DEBUG_ERR("ADD_RPTT", "Bad Parameters.", NULL);
		return result;
	}

	def = rpttpl_create(ari_copy_ptr(id), ac_copy(template));


	if(VDB_FINDKEY_RPTT(def->id) == NULL)
	{
		int rh_code = VDB_ADD_RPTT(def->id, def);

		if(rh_code != RH_OK)
		{
			rpttpl_release(def, 1);
		}
		else
		{
			*status = CTRL_SUCCESS;
			db_persist_rpttpl(def);
		}
	}
	else
	{
		*status = CTRL_SUCCESS;
		AMP_DEBUG_WARN("ADD_RPTT","Ignoring duplicate item.", NULL);
		rpttpl_release(def, 1);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes one or more report template definitions from the Agent.
 */
tnv_t *amp_agent_ctrl_del_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t it;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DEL_RPTT", "Bad parameters.", NULL);
		return result;
	}

	for(it = vecit_first(&(ids->values)); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *cur_id = vecit_data(it);
		rpttpl_t *def = VDB_FINDKEY_RPTT(cur_id);

		if(def == NULL)
		{
			AMP_DEBUG_WARN("DEL_RPTT", "Cannot find template to be deleted.", NULL);
		}
		else
		{
			db_forget(&(def->desc), gDB.rpttpls);
			VDB_DELKEY_RPTT(cur_id);
		}
	}

	*status = CTRL_SUCCESS;


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control produces a detailed description of one or more report template  identifier(ARI) known t
 * o the Agent.
 */
tnv_t *amp_agent_ctrl_desc_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t ac_it;
	int i = 0;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DESC_RPTT", "Bad parameters.", NULL);
		return result;
	}

	tnvc_t *tnvc = tnvc_create(vec_num_entries(ids->values));

	/* For each rptt being described. */
	for(ac_it = vecit_first(&(ids->values)); vecit_valid(ac_it); ac_it = vecit_next(ac_it))
	{
		ari_t *cur_id = vecit_data(ac_it);
		rpttpl_t *def = VDB_FINDKEY_RPTT(cur_id);

		if(def == NULL)
		{
			AMP_DEBUG_WARN("DESC_RPTT","Cannot find RPTT for item %d.", i);
		}
		else
		{
			tnv_t *val = tnv_from_obj(AMP_TYPE_RPTTPL, rpttpl_copy_ptr(def));
			tnvc_insert(tnvc, val);
		}
	}

	result = tnv_from_obj(AMP_TYPE_TNVC, tnvc);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_desc_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control causes the Agent to produce a report entry for each identified report templates and sen
 * d them to one or more identified managers(ARIs).
 */
tnv_t *amp_agent_ctrl_gen_rpts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_gen_rpts BODY
	 * +-------------------------------------------------------------------------+
	 */


	vecit_t ac_it;
	vecit_t mgr_it;

	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);
	tnvc_t *mgrs = adm_get_parm_obj(parms, 1, AMP_TYPE_TNVC);

	if((ids == NULL) || (mgrs == NULL))
	{
		AMP_DEBUG_ERR("GEN_RPTT", "Bad parameters.", NULL);
		return result;
	}

	if(tnvc_get_count(mgrs) == 0)
	{
		if((tnvc_insert(mgrs, tnv_from_str(def_mgr->name))) != AMP_OK)
		{
			AMP_DEBUG_ERR("GEN_RPTT","Empty TNVC and can't add default mgr.", NULL);
			return result;
		}
	}

	/* For each manager receiving a report. */
	for(mgr_it = vecit_first(&(mgrs->values)); vecit_valid(mgr_it); mgr_it = vecit_next(mgr_it))
	{
		tnv_t *cur_mgr = (tnv_t*)vecit_data(mgr_it);
		eid_t mgr_eid;
		msg_rpt_t* msg_rpt;

		if((cur_mgr == NULL) || (cur_mgr->type != AMP_TYPE_STR))
		{
			AMP_DEBUG_ERR("GEN_RPTT","Cannot parse MGR EID to send to.", NULL);
			return result;
		}

		strncpy(mgr_eid.name, cur_mgr->value.as_ptr, AMP_MAX_EID_LEN-1);
		msg_rpt = rda_get_msg_rpt(mgr_eid);

		/* For each report being sent. */
		for(ac_it = vecit_first(&(ids->values)); vecit_valid(ac_it); ac_it = vecit_next(ac_it))
		{
			ari_t *cur_id = vecit_data(ac_it);
			rpt_t *rpt = rpt_create(ari_copy_ptr(cur_id), getCtime(), NULL);

			if(cur_id->type == AMP_TYPE_RPTTPL)
			{
				rpttpl_t *def = VDB_FINDKEY_RPTT(cur_id);
				ldc_fill_rpt(def, rpt);
			}
			else
			{
				tnv_t *cur_val = ldc_collect(cur_id, &(cur_id->as_reg.parms));
				rpt_add_entry(rpt, cur_val);
			}

			msg_rpt_add_rpt(msg_rpt, rpt);
		}
	}


	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_gen_rpts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control causes the Agent to produce a table for each identified table templates and send them t
 * o one or more identified managers(ARIs).
 */
tnv_t *amp_agent_ctrl_gen_tbls(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_gen_tbls BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t ac_it;
	vecit_t mgr_it;

	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);
	tnvc_t *mgrs = adm_get_parm_obj(parms, 1, AMP_TYPE_TNVC);

	if((ids == NULL) || (mgrs == NULL))
	{
		AMP_DEBUG_ERR("GEN_TBLT", "Bad parameters.", NULL);
		return result;
	}

	if(tnvc_get_count(mgrs) == 0)
	{
		if((tnvc_insert(mgrs, tnv_from_str(def_mgr->name))) != AMP_OK)
		{
			AMP_DEBUG_ERR("GEN_TBLT","Empty TNVC and can't add default mgr.", NULL);
			return result;
		}
	}

	/* For each manager receiving a report. */
	for(mgr_it = vecit_first(&(mgrs->values)); vecit_valid(mgr_it); mgr_it = vecit_next(mgr_it))
	{
		tnv_t *cur_mgr = (tnv_t*)vecit_data(mgr_it);
		eid_t mgr_eid;
		msg_rpt_t* msg_rpt;

		if((cur_mgr == NULL) || (cur_mgr->type != AMP_TYPE_STR))
		{
			AMP_DEBUG_ERR("GEN_TBLT","Cannot parse MGR EID to send to.", NULL);
			return result;
		}

		strncpy(mgr_eid.name, cur_mgr->value.as_ptr, AMP_MAX_EID_LEN-1);
		msg_rpt = rda_get_msg_rpt(mgr_eid);

		/* For each report being sent. */
		for(ac_it = vecit_first(&(ids->values)); vecit_valid(ac_it); ac_it = vecit_next(ac_it))
		{
			ari_t *cur_id = vecit_data(ac_it);
			tblt_t *def = VDB_FINDKEY_TBLT(cur_id);
			rpt_t *rpt = NULL;
			tbl_t *tbl = NULL;
			tnv_t *val = NULL;

			if( (def == NULL) ||
				((rpt = rpt_create(ari_copy_ptr(cur_id), getCtime(), NULL)) == NULL) ||
				((tbl = def->build(cur_id)) == NULL) ||
				((val = tnv_from_obj(AMP_TYPE_TBL, tbl)) == NULL) ||
				(rpt_add_entry(rpt, val) != AMP_OK))
			{
				rpt_release(rpt, 1);
				tbl_release(tbl, 1);

				AMP_DEBUG_ERR("GEN_TBLT","Cannot build table.", NULL);
				continue;
			}

			msg_rpt_add_rpt(msg_rpt, rpt);
		}
	}

	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_gen_tbls BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures a new macro definition on the Agent.
 */
tnv_t *amp_agent_ctrl_add_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_macro BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	int i, num;
	ari_t *id = adm_get_parm_obj(parms, 1, AMP_TYPE_ARI);
	ac_t *def = adm_get_parm_obj(parms, 2, AMP_TYPE_AC);

	if((id == NULL) || (def == NULL))
	{
		AMP_DEBUG_ERR("ADD_MACRO", "Bad parameters for control", NULL);
		return result;
	}

	num = ac_get_count(def);
	macdef_t *macro = macdef_create(num, ari_copy_ptr(id));

	for(i = 0; i < num; i++)
	{
		ctrl_t *cur_ctrl = ctrl_create(ac_get(def, i));
		macdef_append(macro, cur_ctrl);
	}

	if(VDB_FINDKEY_MACDEF(macro->ari) == NULL)
	{
		int rh_code = VDB_ADD_MACDEF(macro->ari, macro);

		if(rh_code != RH_OK)
		{
			macdef_release(macro, 1);
		}
		else
		{
			*status = CTRL_SUCCESS;
			db_persist_macdef(macro);
		}
	}
	else
	{
		*status = CTRL_SUCCESS;
		AMP_DEBUG_WARN("ADD_MACRO","Ignoring duplicate item.", NULL);
		macdef_release(macro, 1);
	}


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes one or more macro definitions from the Agent.
 */
tnv_t *amp_agent_ctrl_del_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_macro BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t it;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DEL_MACRO", "Bad parameters.", NULL);
		return result;
	}

	for(it = vecit_first(&(ids->values)); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *cur_id = vecit_data(it);
		macdef_t *def = VDB_FINDKEY_MACDEF(cur_id);

		if(def == NULL)
		{
			AMP_DEBUG_WARN("DEL_MACRO", "Cannot find template to be deleted.", NULL);
		}
		else
		{
			db_forget(&(def->desc), gDB.macdefs);
			VDB_DELKEY_MACDEF(cur_id);
		}
	}

	*status = CTRL_SUCCESS;


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control produces a detailed description of one or more macro identifier(ARI) known to the Agent
 * .
 */
tnv_t *amp_agent_ctrl_desc_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_macro BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t ac_it;
	int i = 0;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DESC_MACRO", "Bad parameters.", NULL);
		return result;
	}

	tnvc_t *tnvc = tnvc_create(vec_num_entries(ids->values));

	/* For each macro being described. */
	for(ac_it = vecit_first(&(ids->values)); vecit_valid(ac_it); ac_it = vecit_next(ac_it))
	{
		ari_t *cur_id = vecit_data(ac_it);
		macdef_t *def = VDB_FINDKEY_MACDEF(cur_id);

		if(def == NULL)
		{
			AMP_DEBUG_WARN("DESC_MACRO","Cannot find MACRO for item %d.", i);
		}
		else
		{
			tnv_t *val = tnv_from_obj(AMP_TYPE_MAC, macdef_copy_ptr(def));
			tnvc_insert(tnvc, val);
		}
	}

	result = tnv_from_obj(AMP_TYPE_TNVC, tnvc);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_desc_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures a new time-based rule(TBR) definition on the Agent.
 */
tnv_t *amp_agent_ctrl_add_tbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	int rh_code;
	tbr_def_t def;
	macdef_t mac;
	rule_t *tbr = NULL;

	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	uvast start = adm_get_parm_uvast(parms, 1, &success);
	def.period = adm_get_parm_uvast(parms, 2, &success);
	def.max_fire = adm_get_parm_uvast(parms, 3, &success);
	ac_t action = ac_copy(adm_get_parm_obj(parms, 4, AMP_TYPE_AC));

	if(id == NULL)
	{
		AMP_DEBUG_ERR("ADD_TBR", "Bad parameters for control", NULL);
		return result;
	}

	if((tbr = rule_create_tbr(*id, start, def, action)) == NULL)
	{
		AMP_DEBUG_ERR("ADD_TBR", "Unable to create TBR structure.", NULL);
		return result;
	}


	if(VDB_FINDKEY_RULE(&(tbr->id)) == NULL)
	{
		int rh_code = VDB_ADD_RULE(&(tbr->id), tbr);

		if(rh_code != RH_OK)
		{
			rule_release(tbr, 1);
		}
		else
		{
			gAgentInstr.num_tbrs++;
			*status = CTRL_SUCCESS;
			db_persist_rule(tbr);
		}
	}
	else
	{
		*status = CTRL_SUCCESS;
		AMP_DEBUG_WARN("ADD_TBR","Ignoring duplicate item.", NULL);
		rule_release(tbr, 1);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures a new state-based rule(SBR) definition on the Agent.
 */
tnv_t *amp_agent_ctrl_add_sbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	sbr_def_t def;
	rule_t *sbr = NULL;
	int success;
	int rh_code;

	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	uvast start = adm_get_parm_uvast(parms, 1, &success);
	expr_t *state = adm_get_parm_obj(parms, 2, AMP_TYPE_EXPR);
	def.expr = expr_copy(*state);
	def.max_eval = adm_get_parm_uvast(parms, 3, &success);
	def.max_fire = adm_get_parm_uvast(parms, 4, &success);
	ac_t action = ac_copy(adm_get_parm_obj(parms, 5, AMP_TYPE_AC));

	if(id == NULL)
	{
		AMP_DEBUG_ERR("ADD_SBR", "Bad parameters for control", NULL);
		return result;
	}

	if((sbr = rule_create_sbr(*id, start, def, action)) == NULL)
	{
		AMP_DEBUG_ERR("ADD_SBR", "Unable to create SBR structure.", NULL);
		return result;
	}

	if(VDB_FINDKEY_RULE(&(sbr->id)) == NULL)
	{
		int rh_code = VDB_ADD_RULE(&(sbr->id), sbr);

		if(rh_code != RH_OK)
		{
			rule_release(sbr, 1);
		}
		else
		{
			gAgentInstr.num_sbrs++;
			*status = CTRL_SUCCESS;
			db_persist_rule(sbr);
		}
	}
	else
	{
		*status = CTRL_SUCCESS;
		AMP_DEBUG_WARN("ADD_SBR","Ignoring duplicate item.", NULL);
		rule_release(sbr, 1);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes one or more rule definitions from the Agent.
 */
tnv_t *amp_agent_ctrl_del_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_rule BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t it;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DEL_RULE", "Bad parameters.", NULL);
		return result;
	}

	for(it = vecit_first(&(ids->values)); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *cur_id = vecit_data(it);
		rule_t *rule = VDB_FINDKEY_RULE(cur_id);

		if(rule == NULL)
		{
			AMP_DEBUG_WARN("DEL_RULE", "Cannot find RULE to be deleted.", NULL);
		}
		else
		{
			db_forget(&(rule->desc), gDB.rules);
			VDB_DELKEY_RULE(cur_id);
		}
	}

	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control produces a detailed description of one or more rules known to the Agent.
 */
tnv_t *amp_agent_ctrl_desc_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_rule BODY
	 * +-------------------------------------------------------------------------+
	 */

	vecit_t ac_it;
	int i = 0;
	ac_t *ids = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);

	if(ids == NULL)
	{
		AMP_DEBUG_ERR("DESC_RULE", "Bad parameters.", NULL);
		return result;
	}

	tnvc_t *tnvc = tnvc_create(vec_num_entries(ids->values));

	/* For each rule being described. */
	for(ac_it = vecit_first(&(ids->values)); vecit_valid(ac_it); ac_it = vecit_next(ac_it))
	{
		ari_t *cur_id = vecit_data(ac_it);
		rule_t *rule = VDB_FINDKEY_RULE(cur_id);

		if(rule == NULL)
		{
			AMP_DEBUG_WARN("DESC_RULE","Cannot find RULE for item %d.", i);
		}
		else
		{
			tnv_t *val = tnv_from_obj(rule->id.type, rule_copy_ptr(rule));
			tnvc_insert(tnvc, val);
		}
	}

	result = tnv_from_obj(AMP_TYPE_TNVC, tnvc);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_desc_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control stores variables.
 */
tnv_t *amp_agent_ctrl_store_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_store_var BODY
	 * +-------------------------------------------------------------------------+
	 */

	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	expr_t *expr = adm_get_parm_obj(parms, 1, AMP_TYPE_EXPR);

	var_t *var = VDB_FINDKEY_VAR(id);

	if(var == NULL)
	{
		AMP_DEBUG_ERR("stor_var","Cannot find variable.", NULL);
		return result;
	}

	tnv_t *tmp = expr_eval(expr);
	if(tmp != NULL)
	{
		tnv_release(var->value, 1);
		var->value = tmp;
		*status = CTRL_SUCCESS;
	}
	else
	{
		AMP_DEBUG_ERR("stor_var","unable to assign new value.", NULL);
	}


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_store_var BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control resets all Agent ADM statistics reported in the Agent ADM report.
 */
tnv_t *amp_agent_ctrl_reset_counts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_reset_counts BODY
	 * +-------------------------------------------------------------------------+
	 */
	agent_instr_clear();
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_reset_counts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */

/*
 * Int32 addition
 */
tnv_t *amp_agent_op_plusint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusint BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = amp_agent_binary_num_op(PLUS, stack, AMP_TYPE_INT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_plusint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int32 addition
 */
tnv_t *amp_agent_op_plusuint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(PLUS, stack, AMP_TYPE_UINT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_plusuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int64 addition
 */
tnv_t *amp_agent_op_plusvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(PLUS, stack, AMP_TYPE_VAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_plusvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int64 addition
 */
tnv_t *amp_agent_op_plusuvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(PLUS, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_plusuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real32 addition
 */
tnv_t *amp_agent_op_plusreal32(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = amp_agent_binary_num_op(PLUS, stack, AMP_TYPE_REAL32);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_plusreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real64 addition
 */
tnv_t *amp_agent_op_plusreal64(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(PLUS, stack, AMP_TYPE_REAL64);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_plusreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int32 subtraction
 */
tnv_t *amp_agent_op_minusint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MINUS, stack, AMP_TYPE_INT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_minusint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int32 subtraction
 */
tnv_t *amp_agent_op_minusuint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MINUS, stack, AMP_TYPE_UINT);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_minusuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int64 subtraction
 */
tnv_t *amp_agent_op_minusvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MINUS, stack, AMP_TYPE_VAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_minusvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int64 subtraction
 */
tnv_t *amp_agent_op_minusuvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MINUS, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_minusuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real32 subtraction
 */
tnv_t *amp_agent_op_minusreal32(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MINUS, stack, AMP_TYPE_REAL32);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_minusreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real64 subtraction
 */
tnv_t *amp_agent_op_minusreal64(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MINUS, stack, AMP_TYPE_REAL64);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_minusreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int32 multiplication
 */
tnv_t *amp_agent_op_multint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MULT, stack, AMP_TYPE_INT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_multint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int32 multiplication
 */
tnv_t *amp_agent_op_multuint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MULT, stack, AMP_TYPE_UINT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_multuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int64 multiplication
 */
tnv_t *amp_agent_op_multvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MULT, stack, AMP_TYPE_VAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_multvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int64 multiplication
 */
tnv_t *amp_agent_op_multuvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MULT, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_multuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real32 multiplication
 */
tnv_t *amp_agent_op_multreal32(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MULT, stack, AMP_TYPE_REAL32);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_multreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real64 multiplication
 */
tnv_t *amp_agent_op_multreal64(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MULT, stack, AMP_TYPE_REAL64);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_multreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int32 division
 */
tnv_t *amp_agent_op_divint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(DIV, stack, AMP_TYPE_INT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_divint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int32 division
 */
tnv_t *amp_agent_op_divuint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(DIV, stack, AMP_TYPE_UINT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_divuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int64 division
 */
tnv_t *amp_agent_op_divvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(DIV, stack, AMP_TYPE_VAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_divvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int64 division
 */
tnv_t *amp_agent_op_divuvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(DIV, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_divuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real32 division
 */
tnv_t *amp_agent_op_divreal32(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(DIV, stack, AMP_TYPE_REAL32);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_divreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real64 division
 */
tnv_t *amp_agent_op_divreal64(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(DIV, stack, AMP_TYPE_REAL64);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_divreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int32 modulus division
 */
tnv_t *amp_agent_op_modint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MOD, stack, AMP_TYPE_INT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_modint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int32 modulus division
 */
tnv_t *amp_agent_op_moduint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_moduint BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = amp_agent_binary_num_op(MOD, stack, AMP_TYPE_UINT);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_moduint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int64 modulus division
 */
tnv_t *amp_agent_op_modvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MOD, stack, AMP_TYPE_VAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_modvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int64 modulus division
 */
tnv_t *amp_agent_op_moduvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_moduvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MOD, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_moduvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real32 modulus division
 */
tnv_t *amp_agent_op_modreal32(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = amp_agent_binary_num_op(MOD, stack, AMP_TYPE_REAL32);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_modreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real64 modulus division
 */
tnv_t *amp_agent_op_modreal64(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(MOD, stack, AMP_TYPE_REAL64);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_modreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int32 exponentiation
 */
tnv_t *amp_agent_op_expint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(EXP, stack, AMP_TYPE_INT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_expint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned int32 exponentiation
 */
tnv_t *amp_agent_op_expuint(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(EXP, stack, AMP_TYPE_UINT);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_expuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Int64 exponentiation
 */
tnv_t *amp_agent_op_expvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expvast BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = amp_agent_binary_num_op(EXP, stack, AMP_TYPE_VAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_expvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Unsigned Int64 exponentiation
 */
tnv_t *amp_agent_op_expuvast(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(EXP, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_expuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real32 exponentiation
 */
tnv_t *amp_agent_op_expreal32(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(EXP, stack, AMP_TYPE_REAL32);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_expreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Real64 exponentiation
 */
tnv_t *amp_agent_op_expreal64(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(EXP, stack, AMP_TYPE_REAL64);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_expreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bitwise and
 */
tnv_t *amp_agent_op_bitand(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitand BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(BITAND, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitand BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bitwise or
 */
tnv_t *amp_agent_op_bitor(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitor BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(BITOR, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitor BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bitwise xor
 */
tnv_t *amp_agent_op_bitxor(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitxor BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(BITXOR, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitxor BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bitwise not
 */
tnv_t *amp_agent_op_bitnot(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitnot BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_unary_num_op(BITNOT, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitnot BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Logical and
 */
tnv_t *amp_agent_op_logand(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_logand BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = adm_agent_binary_log_op(LOGAND, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_logand BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Logical or
 */
tnv_t *amp_agent_op_logor(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_logor BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(LOGOR, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_logor BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Logical not
 */
tnv_t *amp_agent_op_lognot(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_lognot BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(LOGNOT, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_lognot BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * absolute value
 */
tnv_t *amp_agent_op_abs(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_abs BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_unary_num_op(ABS, stack, AMP_TYPE_UVAST);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_abs BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * <
 */
tnv_t *amp_agent_op_lessthan(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_lessthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(LT, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_lessthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * >
 */
tnv_t *amp_agent_op_greaterthan(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_greaterthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(GT, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_greaterthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * <=
 */
tnv_t *amp_agent_op_lessequal(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_lessequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(LTE, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_lessequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * >=
 */
tnv_t *amp_agent_op_greaterequal(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_greaterequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(GTE, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_greaterequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * !=
 */
tnv_t *amp_agent_op_notequal(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_notequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(NEQ, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_notequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * ==
 */
tnv_t *amp_agent_op_equal(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_equal BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_agent_binary_log_op(EQ, stack);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_equal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * <<
 */
tnv_t *amp_agent_op_bitshiftleft(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitshiftleft BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = amp_agent_binary_num_op(BITLSHFT, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitshiftleft BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * >>
 */
tnv_t *amp_agent_op_bitshiftright(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitshiftright BODY
	 * +-------------------------------------------------------------------------+
	 */

	result = amp_agent_binary_num_op(BITRSHFT, stack, AMP_TYPE_UVAST);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitshiftright BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Store value of parm 2 in parm 1
 */
tnv_t *amp_agent_op_stor(vector_t *stack)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_stor BODY
	 * +-------------------------------------------------------------------------+
	 */

	AMP_DEBUG_ERR("stor","Not Implemented.", NULL);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_stor BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

