#ifdef AGENT_ROLE
/*****************************************************************************
 **
 ** File Name: adm_agent_impl.c
 **
 ** Description: This implements the private aspects of a DTNMP agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 *****************************************************************************/
#include <math.h>

#include "shared/adm/adm.h"

#include "shared/primitives/value.h"
#include "adm_agent_impl.h"
#include "shared/primitives/report.h"
#include "rda.h"

/******************************************************************************
 *
 * \par Function Name: agent_adm_init_agent
 *
 * \par Initializes the collect/run functions for agent ADM support. Both the
 *      manager and agent share functions for sizing and printing items. However,
 *      only the agent needs to implement the functions for collecting data and
 *      running controls.
 *
 * \par Notes:
 *
 *  - We build a string representation of the MID rather than storing one
 *    statically to save on static space.  Please see the DTNMP AGENT ADM
 *    for specifics on the information added here.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation.
 *****************************************************************************/

value_t agent_md_name(Lyst params)
{
	return val_from_string("DTNMP ADM");
}

value_t agent_md_ver(Lyst params)
{
	return val_from_string("v0.1");
}

/* Retrieval Functions. */

value_t agent_get_num_rpt(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_rpt_defs;
	result.length = sizeof(uint32_t);

	return result;
}


value_t agent_get_sent_rpt(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_sent_rpts;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_trl(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_time_rules;
	result.length = sizeof(uint32_t);

	return result;
}



value_t agent_get_run_trl(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_time_rules_run;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_srl(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_prod_rules;
	result.length = sizeof(uint32_t);

	return result;
}



value_t agent_get_run_srl(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_prod_rules_run;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_lit(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_consts;
	result.length = sizeof(uint32_t);

	return result;
}



value_t agent_get_num_cust(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_data_defs;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_mac(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_macros;
	result.length = sizeof(uint32_t);

	return result;
}



value_t agent_get_run_mac(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_macros_run;
	result.length = sizeof(uint32_t);

	return result;
}



value_t agent_get_num_ctrl(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_ctrls;
	result.length = sizeof(uint32_t);
	return result;
}



value_t agent_get_run_ctrl(Lyst params)
{
	value_t result;

	result.type = VAL_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_ctrls_run;
	result.length = sizeof(uint32_t);

	return result;
}



/* Control Functions */

/*
 * This control causes the agent to produce a report detailing the list
 * of all ADMs configured on the agent and available for use.
 * This control takes no parameters.
 * This control causes a report to be generated with the following format.
 * ListADMs Report Format
 * +--------+------------+     +------------+
 * | # ADMs | ADM 1 Name | ... | ADM N Name |
 * | [SDNV] |    [STR]   |     |    [STR]   |
 * +--------+------------+     +------------+
 * Figure 18
 * # ADMs - The number of ADMs known to the agent.
 * ADM [x] Name - For each ADM, it’s human readable name.
 */
uint32_t agent_ctl_adm_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_ad_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_ad_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_cd_add(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_cd_del(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_cd_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_cd_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_rpt_add(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_rpt_del(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_rpt_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_rpt_dsc(Lyst params)
{
  return 0;
}


/******************************************************************************
 *
 * \par Function Name: agent_ctl_rpt_gen
 *
 * \par Purpose: Causes a report to be generated by the agent and sent to
 *               appropriate managers.
 *
 * \return 0  - Failure.
 * 		   !0 - Success.
 *
 * \param[in]   params  The control parameters.
 *
 * \par Notes:
 *  - Parm 1 is a MC of items to be included in the report.
 *  - Parm 2 is a DC representing the manager to receive the report.
 *  - If there is no DC representing the recipient for the report, then
 *    the report should be sent to the originator of this request.
 *
 * \todo: Support multiple recipients
 * \todo: Support NULL recipient (controls should have a dossier about the
 *        control in question.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/17/15  E. Birrane     Initial implementation,
 *****************************************************************************/
uint32_t agent_ctl_rpt_gen(Lyst params)
{
  datacol_entry_t *dc = NULL; // Each parameter is a DC
  rpt_data_t *report = NULL;
  LystElt elt;
  eid_t recipient;
  Lyst mids = NULL;
  Lyst rx = NULL;
  uint32_t bytes = 0;

  /* Step 0: Make sure we have the correct parameters. */
  if((params == NULL) || (lyst_length(params) != 2))
  {
	  return 0;
  }

  /* Step 1: Get the MC from the params. */
  elt = lyst_first(params);
  dc = lyst_data(elt);
  mids = midcol_deserialize(dc->value, dc->length, &bytes);
  MRELEASE(dc->value);
  MRELEASE(dc);

  /* Step 2: Get the recipient for this message. */
  elt = lyst_next(elt);
  dc = lyst_data(elt);
  rx = dc_deserialize(dc->value, dc->length, &bytes);
  MRELEASE(dc->value);
  MRELEASE(dc);


  // \todo: Endian issues.
  // \todo: Handle more than 1 recipient.
  dc = lyst_data(lyst_first(rx));
  if(dc->length < sizeof(recipient))
  {
	  bzero(&recipient, sizeof(eid_t));
	  memcpy(&recipient, dc->value, dc->length);
	  dc_destroy(&rx);
  }
  else
  {
	  DTNMP_DEBUG_ERR("agent_ctl_rpt_gen", "Rx length of %d > %d.", dc->length, sizeof(eid_t));
	  dc_destroy(&rx);
	  midcol_destroy(&mids);
	  return 0;
  }

  /* Step 2: Build the report.
   *
   * This function will grab an existing to-be-sent report or
   * create a new report and add it to the "built-reports" section.
   * Either way, it will be included in the next set of code to send
   * out reports built in this time slice.
   */
  report = rda_get_report(recipient);


  /* Step 3: For each MID in the report definition, construct the
   * entry and add it to the report.
   */
  for(elt = lyst_first(mids); elt != NULL; elt = lyst_next(elt))
  {
	  mid_t *mid = lyst_data(elt);

	  rpt_data_entry_t *entry = rda_build_report_entry(mid);

	  if(entry != NULL)
	  {
		  lyst_insert_last(report->reports,entry);
	  }
	  else
	  {
// \todo: andle the error...
	  }
  }

  return 0;
}

uint32_t agent_ctl_op_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_op_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_ctl_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_ctl_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_mac_add(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_mac_del(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_mac_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_mac_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_trl_add(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_trl_del(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_trl_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_trl_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_srl_add(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_srl_del(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_srl_lst(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_srl_dsc(Lyst params)
{
  return 0;
}

uint32_t agent_ctl_lit_lst(Lyst params)
{
  return 0;
}


/*
 * OP FUnctions
 */

/*
 * \todo: Consider type promotion? when int * int = vast?
 */
value_t adm_agent_op_prep(int num, Lyst parms, value_t **lval, value_t **rval)
{
	value_t result;
    LystElt elt = 0;

	result.type = VAL_TYPE_UNK;
	result.value.as_int = 0;
	result.length = 0;

	if((parms == NULL) || (num < 1) || (num > 2) ||
		(lval == NULL) ||
		((num == 2) && (rval == NULL)) || (lyst_length(parms) != num))
	{
        DTNMP_DEBUG_ERR("adm_agent_op_prep","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_prep","-> Unknown", NULL);
        return result;
	}

	/* Step 2: Grab the L-value and R-value. */
	elt = lyst_first(parms);
	if((*lval = (value_t*) lyst_data(elt)) == NULL)
	{
        DTNMP_DEBUG_ERR("adm_agent_op_plus","NULL lval.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
        return result;
	}
	if(num == 2)
	{
		elt = lyst_next(elt);
		if((*rval = (value_t*) lyst_data(elt)) == NULL)
		{
			*lval = NULL;
			DTNMP_DEBUG_ERR("adm_agent_op_plus","NULL rval.",NULL);
			DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
			return result;
		}
	}

	return result;
}

/*
 * Good for: INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_plus(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_plus","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) + val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) + val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) + val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) + val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) + val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = VAL_TYPE_REAL32;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) + val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = VAL_TYPE_REAL64;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Good for: INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_minus(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_minus","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_minus","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) - val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) - val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) - val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) - val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) - val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = VAL_TYPE_REAL32;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) - val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = VAL_TYPE_REAL64;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Good For: INT, UINT, UVAST, VAST, REAL32, REAL64
 */
value_t adm_agent_op_mult(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_mult","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_mult","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) * val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) * val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) * val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) * val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) * val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = VAL_TYPE_REAL32;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) * val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = VAL_TYPE_REAL64;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Good for: INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_div(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_div","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_div","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) / val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) / val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) / val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) / val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) / val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = VAL_TYPE_REAL32;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) / val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = VAL_TYPE_REAL64;
		break;
	default:
		break;
	}

	return result;

}

/*
 * Good for INT, UINT, VAST, UVAST
 * Result type is always UINT or UVAST.
 */
value_t adm_agent_op_mod(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_mod","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_mod","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) % val_cvt_int(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) % val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uvast = val_cvt_vast(lval) % val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) % val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Good for INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_exp(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_exp","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_exp","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = (int32_t) pow(val_cvt_int(lval),val_cvt_int(rval));
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) pow(val_cvt_uint(lval), val_cvt_uint(rval));
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = (vast) pow(val_cvt_vast(lval), val_cvt_vast(rval));
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = (uvast) pow(val_cvt_uvast(lval),val_cvt_uvast(rval));
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_real32 = (float) pow(val_cvt_real32(lval), val_cvt_real32(rval));
		result.length = sizeof(float);
		result.type = VAL_TYPE_REAL32;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_real64 = (double) pow(val_cvt_real64(lval), val_cvt_real64(rval));
		result.length = sizeof(double);
		result.type = VAL_TYPE_REAL64;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Only for int, uint, vast, uvast
 */
value_t adm_agent_op_bitand(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_bitand","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_bitand","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) & val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) & val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) & val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) & val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	default:
		break;
	}

	return result;

}

/*
 * Only for int, uint, uvast, vast
 */
value_t adm_agent_op_bitor(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_bitor","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_bitor","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) | val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) | val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) | val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) | val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Only for int, uint, vast, uvast
 */
value_t adm_agent_op_bitxor(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_bitxor","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_bitxor","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) ^ val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) ^ val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) ^ val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) ^ val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	default:
		break;
	}

	return result;
}

/*
 * Only for int, uint, vast, uvast
 */
value_t adm_agent_op_bitnot(Lyst parms)
{
	value_t result;
	value_t *lval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(1, parms, &lval, NULL);
    cvt_type = lval->type;

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_plus","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_int = !val_cvt_int(lval);
		result.length = sizeof(int32_t);
		result.type = VAL_TYPE_INT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = !val_cvt_uint(lval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_vast = !val_cvt_vast(lval);
		result.length = sizeof(vast);
		result.type = VAL_TYPE_VAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = !val_cvt_uvast(lval);
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	default:
		break;
	}

	return result;
}


value_t adm_agent_op_logand(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_logand","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_logand","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
    /* We use type for the conversion, and then fix it since logical is
     * always a uint.
     */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) && val_cvt_int(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) && val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = val_cvt_vast(lval) && val_cvt_vast(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = val_cvt_uvast(lval) && val_cvt_uvast(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}


value_t adm_agent_op_logor(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_logor","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_logor","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
    /* We use type for the conversion, and then fix it since logical is
     * always a uint.
     */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) || val_cvt_int(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) || val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = val_cvt_vast(lval) || val_cvt_vast(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = val_cvt_uvast(lval) || val_cvt_uvast(rval);
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}


value_t adm_agent_op_logxor(Lyst parms)
{
	value_t result;

	result.length = 0;
	result.type = VAL_TYPE_UNK;
	result.value.as_ptr = NULL;

	return result;
}


value_t adm_agent_op_lognot(Lyst parms)
{
	value_t result;
	value_t *lval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(1, parms, &lval, NULL);
    cvt_type = lval->type;

	/* Step 3: Based on result type, convert and perform operations. */
    /* We use type for the conversion, and then fix it since logical is
     * always a uint.
     */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = val_cvt_vast(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = val_cvt_uvast(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}


/*
 * Works for INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_abs(Lyst parms)
{
	value_t result;
	value_t *lval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(1, parms, &lval, NULL);
	cvt_type = lval->type;

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_abs","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_abs","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (int32_t) abs(val_cvt_int(lval));
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) abs(val_cvt_uint(lval));
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uvast = (uvast) labs(val_cvt_vast(lval));
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uvast = (uvast) labs(val_cvt_uvast(lval));
		result.length = sizeof(uvast);
		result.type = VAL_TYPE_UVAST;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_real32 = (float) fabsf(val_cvt_real32(lval));
		result.length = sizeof(float);
		result.type = VAL_TYPE_REAL32;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_real64 = (double) fabs(val_cvt_real64(lval));
		result.length = sizeof(double);
		result.type = VAL_TYPE_REAL64;
		break;
	default:
		break;
	}

	return result;
}


value_t adm_agent_op_lt(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_lt","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_lt","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) < val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) < val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) < val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) < val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) < val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) < val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}

value_t adm_agent_op_gt(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_gt","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_gt","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) > val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) > val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) > val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) > val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) > val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) > val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}


value_t adm_agent_op_lte(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_lte","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_lte","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) <= val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) <= val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) <= val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) <= val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) <= val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) <= val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}

value_t adm_agent_op_gte(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_gte","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_gte","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) >= val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) >= val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) >= val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) >= val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) >= val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) >= val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}


value_t adm_agent_op_neq(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_neq","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_neq","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) != val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) != val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) != val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) != val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) != val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) != val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}

value_t adm_agent_op_eq(Lyst parms)
{
	value_t result;
	value_t *lval, *rval;
	uint32_t cvt_type = VAL_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == VAL_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_eq","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_eq","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case VAL_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) == val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) == val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) == val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) == val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) == val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	case VAL_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) == val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = VAL_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}

#endif
