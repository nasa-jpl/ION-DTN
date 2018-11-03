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
 **  2018-10-16  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/primitives/expr.h"
#include "../shared/primitives/tnv.h"
#include "instr.h"
#include "../shared/adm/adm.h"
#include "../shared/msg/msg.h"
#include "rda.h"
#include "ldc.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "adm_amp_agent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */


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
		*lval = vec_pop(stack, &success);
	}
	if((success == VEC_OK) && (num > 1))
	{
		*rval = vec_pop(stack, &success);
	}

	return success;
}


int amp_agent_binary_num_op(tnv_t *result, amp_agent_op_e op, vector_t *stack, amp_type_e result_type)
{
	int ls = 0;
	int rs = 0;
	tnv_t *lval = NULL;
	tnv_t *rval = NULL;

	if((stack == NULL) || (result == NULL))
	{
		return AMP_FAIL;
	}

	if(adm_agent_op_prep(2, &lval, &rval, stack) != AMP_OK)
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return AMP_FAIL;
	}

	// TODO more boundary checks.
	result->type = (result_type == AMP_TYPE_UNK) ? gValNumCvtResult[lval->type - AMP_TYPE_INT][rval->type - AMP_TYPE_INT] : result_type;

	if(result->type == AMP_TYPE_UNK)
	{
		tnv_release(lval, 1);
		tnv_release(rval, 1);
		return AMP_FAIL;
	}

    switch(result->type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
		case PLUS:   result->value.as_int = tnv_to_int(*lval, &ls) + tnv_to_int(*rval, &rs); break;
		case MINUS:  result->value.as_int = tnv_to_int(*lval, &ls) - tnv_to_int(*rval, &rs); break;
		case MULT:   result->value.as_int = tnv_to_int(*lval, &ls) * tnv_to_int(*rval, &rs); break;
		case DIV:    result->value.as_int = tnv_to_int(*lval, &ls) / tnv_to_int(*rval, &rs); break;
		case MOD:    result->value.as_int = tnv_to_int(*lval, &ls) % tnv_to_int(*rval, &rs); break;
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
		case DIV:    result->value.as_uint = tnv_to_uint(*lval, &ls) / tnv_to_uint(*rval, &rs); break;
		case MOD:    result->value.as_uint = tnv_to_uint(*lval, &ls) % tnv_to_uint(*rval, &rs); break;
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
		case DIV:    result->value.as_vast = tnv_to_vast(*lval, &ls) / tnv_to_vast(*rval, &rs); break;
		case MOD:    result->value.as_vast = tnv_to_vast(*lval, &ls) % tnv_to_vast(*rval, &rs); break;
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
		case DIV:    result->value.as_uvast = tnv_to_uvast(*lval, &ls) / tnv_to_uvast(*rval, &rs); break;
		case MOD:    result->value.as_uvast = tnv_to_uvast(*lval, &ls) % tnv_to_uvast(*rval, &rs); break;
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
		case DIV:    result->value.as_real32 = tnv_to_real32(*lval, &ls) / tnv_to_real32(*rval, &rs); break;
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
		case DIV:    result->value.as_real64 = tnv_to_real64(*lval, &ls) / tnv_to_real64(*rval, &rs); break;
		case EXP:    result->value.as_real64 = (double) pow(tnv_to_real64(*lval, &ls), tnv_to_real64(*rval, &rs)); break;
		default: ls = rs = 0; break;
		}
		break;
		default:
			ls = rs = AMP_FAIL; break;
	}

	tnv_release(lval, 1);
	tnv_release(rval, 1);

	if((ls != AMP_OK) || (rs != AMP_OK))
	{
        AMP_DEBUG_ERR("adm_agent_binary_num_op","Bad op (%d) or type (%d -> %d).",op, lval->type, rval->type);
        return AMP_FAIL;
	}

	return AMP_OK;
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
	return tnv_from_str("adm_amp_agent");
}


tnv_t *amp_agent_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("Amp/Agent");
}


tnv_t *amp_agent_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.5");
}


tnv_t *amp_agent_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


tnv_t* amp_agent_get_amp_epoch(tnvc_t *parms)
{
	return tnv_from_uvast(1504915200);
}


/* Table Functions */


/*
 * This table lists all the adms that are supported by the agent.
 */
tbl_t* adm_amp_agent_tbl_adms(ari_t *id)
{
	tbl_t *table = NULL;

	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_adms BODY
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
	 * |STOP CUSTOM FUNCTION tbl_adms BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every variable that is known to the agent.
 */
tbl_t* adm_amp_agent_tbl_variables(ari_t *id)
{
	tbl_t *table = NULL;

	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_variables BODY
	 * +-------------------------------------------------------------------------+
	 */
	if(amp_agent_build_ari_table(table, &(gVDB.vars)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_variables BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every report template that is known to the agent.
 */
tbl_t* adm_amp_agent_tbl_rptt(ari_t *id)
{
	tbl_t *table = NULL;

	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	if(amp_agent_build_ari_table(table, &(gVDB.rpttpls)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every macro that is known to the agent.
 */
tbl_t* adm_amp_agent_tbl_macro(ari_t *id)
{
	tbl_t *table = NULL;

	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_macro BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_build_ari_table(table, &(gVDB.macdefs)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists the ARI for every tbr that is known to the agent.
 */
tbl_t* adm_amp_agent_tbl_rule(ari_t *id)
{
	tbl_t *table = NULL;

	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_build_ari_table(table, &(gVDB.rules)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}



/*
 * This table lists the ARI for every table template that is known to the agent.
 */
tbl_t* adm_amp_agent_tbl_tblt(ari_t *id)
{
	tbl_t *table = NULL;

	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_tblt BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_build_ari_table(table, &(gVDB.adm_tblts)) != AMP_OK)
	{
		tbl_release(table, 1);
		table = NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_tblt BODY
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

tnv_t* amp_agent_ctrl_add_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_var BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	expr_t *expr = adm_get_parm_obj(parms, 1, AMP_TYPE_EXPR);
	amp_type_e type = adm_get_parm_int(parms, 2, &success);

	if((id == NULL) || (expr == NULL) || (type = AMP_TYPE_UNK))
	{
		AMP_DEBUG_ERR("ADD_VAR", "Bad parameters for control", NULL);
		return result;
	}

	if(adm_add_var_from_expr(ari_copy_ptr(id), type, expr_copy_ptr(expr)) != AMP_OK)
	{
		AMP_DEBUG_ERR("ADD_VAR", "Error adding new variable.", NULL);
		return result;
	}

	var_t *var = VDB_FINDKEY_VAR(id);
	if(var == NULL)
	{
		AMP_DEBUG_ERR("ADD_VAR", "Issue adding var to RAM DB.", NULL);
		return result;
	}

	if(db_persist_var(var) != AMP_OK)
	{
		AMP_DEBUG_ERR("ADD_VAR", "Unable to persist new var.", NULL);
		return result;
	}

	*status = CTRL_SUCCESS;

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
tnv_t* amp_agent_ctrl_del_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
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
tnv_t* amp_agent_ctrl_add_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
	ari_t *id = adm_get_parm_obj(parms, 0, AMP_TYPE_ARI);
	ac_t *template = adm_get_parm_obj(parms, 0, AMP_TYPE_AC);
	rpttpl_t *def = NULL;

	if((id == NULL) || (template == NULL))
	{
		AMP_DEBUG_ERR("ADD_RPTT", "Bad Parameters.", NULL);
		return result;
	}

	def = rpttpl_create(ari_copy_ptr(id), ac_copy(template));

	if(adm_add_rpttpl(def) != AMP_OK)
	{
		AMP_DEBUG_ERR("ADD_RPTT", "Failure adding template.", NULL);
		return result;
	}

	if((def = VDB_FINDKEY_RPTT(id)) == NULL)
	{
		AMP_DEBUG_ERR("ADD_RPTT", "Issue adding template to RAM DB.", NULL);
		return result;
	}

	if(db_persist_rpttpl(def) != AMP_OK)
	{
		AMP_DEBUG_ERR("ADD_VAR", "Unable to persist new template.", NULL);
		return result;
	}

	*status = CTRL_SUCCESS;

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
tnv_t* amp_agent_ctrl_del_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
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
tnv_t* amp_agent_ctrl_desc_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_rptt BODY
	 * +-------------------------------------------------------------------------+
	 */
// todo: Remove from the JSON. This is replaced by a table.
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
tnv_t* amp_agent_ctrl_gen_rpts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
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

		strncpy(mgr_eid.name, cur_mgr->value.as_ptr, sizeof(mgr_eid.name));
		msg_rpt = rda_get_msg_rpt(mgr_eid);

		/* For each report being sent. */
		for(ac_it = vecit_first(&(ids->values)); vecit_valid(ac_it); ac_it = vecit_next(ac_it))
		{
			ari_t *cur_id = vecit_data(ac_it);
			rpttpl_t *def = VDB_FINDKEY_RPTT(cur_id);
			rpt_t *rpt = rpt_create(ari_copy_ptr(cur_id), getUTCTime(), NULL);

			ldc_fill_rpt(def, rpt);
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
 * This control generates tables for each identified table template.
 */
tnv_t* amp_agent_ctrl_gen_tbls(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
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

		strncpy(mgr_eid.name, cur_mgr->value.as_ptr, sizeof(mgr_eid.name));
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
				((rpt = rpt_create(ari_copy_ptr(cur_id), getUTCTime(), NULL)) == NULL) ||
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
tnv_t* amp_agent_ctrl_add_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
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
tnv_t* amp_agent_ctrl_del_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
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
tnv_t* amp_agent_ctrl_desc_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_macro BODY
	 * +-------------------------------------------------------------------------+
	 */
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
tnv_t* amp_agent_ctrl_add_tbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes one or more TBR definitions from the Agent.
 */
tnv_t* amp_agent_ctrl_del_tbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control produces a detailed description of one or more TRL identifier(ARI)s known to the Agent.
 */
tnv_t* amp_agent_ctrl_desc_tbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_desc_tbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures a new state-based rule(SBR) definition on the Agent.
 */
tnv_t* amp_agent_ctrl_add_sbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes one or more SBR definitions from the Agent.
 */
tnv_t* amp_agent_ctrl_del_sbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control produces a detailed description of one or more SBR identifier(ARI)s known to the Agent.
 */
tnv_t* amp_agent_ctrl_desc_sbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_desc_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_desc_sbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control stores variables.
 */
tnv_t* amp_agent_ctrl_store_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_store_var BODY
	 * +-------------------------------------------------------------------------+
	 */
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
tnv_t* amp_agent_ctrl_reset_counts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_reset_counts BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusint BODY
	 * +-------------------------------------------------------------------------+
	 */

	if(amp_agent_binary_num_op(result, PLUS, stack, AMP_TYPE_INT) != AMP_OK)
	{
		tnv_release(result, 1);
		result = NULL;
	}

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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusuint BODY
	 * +-------------------------------------------------------------------------+
	 */
	if(amp_agent_binary_num_op(result, PLUS, stack, AMP_TYPE_UINT) != AMP_OK)
	{
		tnv_release(result, 1);
		result = NULL;
	}
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_plusreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusuint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_minusreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multuint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_multreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divuint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_divreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_moduint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_moduvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_modreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expuint BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expuvast BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expreal32 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_expreal64 BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitand BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitor BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitxor BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitnot BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_logand BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_logor BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_lognot BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_abs BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_abs BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Less than
 */
tnv_t *amp_agent_op_lessthan(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_lessthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_lessthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Greater than
 */
tnv_t *amp_agent_op_greaterthan(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_greaterthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_greaterthan BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Less than or equal to
 */
tnv_t *amp_agent_op_lessequal(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_lessequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_lessequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Greater than or equal to
 */
tnv_t *amp_agent_op_greaterequal(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_greaterequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_greaterequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Not equal
 */
tnv_t *amp_agent_op_notequal(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_notequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_notequal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Equal to
 */
tnv_t *amp_agent_op_equal(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_equal BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_equal BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bitwise left shift
 */
tnv_t *amp_agent_op_bitshiftleft(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitshiftleft BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_bitshiftleft BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bitwise right shift
 */
tnv_t *amp_agent_op_bitshiftright(vector_t *stack)
{
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_bitshiftright BODY
	 * +-------------------------------------------------------------------------+
	 */
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
	tnv_t *result = tnv_create();
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION op_stor BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION op_stor BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

