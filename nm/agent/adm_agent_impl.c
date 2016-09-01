/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/


/*****************************************************************************
 **
 ** File Name: adm_agent_impl.c
 **
 **
 ** Description: This file implements the private (agent-specific) aspects of
 **              an AMP Agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation (JHU/APL)
 **  07/03/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/


#include <math.h>

#include "../shared/adm/adm.h"

#include "../shared/primitives/value.h"
#include "adm_agent_impl.h"
#include "../shared/primitives/report.h"
#include "rda.h"
#include "../shared/primitives/ctrl.h"
#include "agent_db.h"
#include "instr.h"




/******************************************************************************
 *
 * \par Function Name: adm_agent_md_name
 *
 * \par Returns the name of this ADM.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Updated Comments.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_md_name(tdc_t params)
{
	return val_from_string("DTNMP ADM");
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_md_ver
 *
 * \par Returns the version of this ADM.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Updated Comments.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_md_ver(tdc_t params)
{
	return val_from_string("v0.2");
}



/* Retrieval Functions. */


/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_rpt
 *
 * \par Returns The number of reports defined for this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_rpt(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_rptt_defs);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_sent_rpt
 *
 * \par Returns The number of reports sent by this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_sent_rpt(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_sent_rpts);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_trl
 *
 * \par Returns The number of TRLs defined for this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_trl(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_trls);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_run_trl
 *
 * \par Returns The number of TRLs evaluations on this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_run_trl(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_trls_run);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_srl
 *
 * \par Returns The number of SRLs defined on this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_srl(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_srls);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_run_srl
 *
 * \par Returns The number of SRL evaluations by this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_run_srl(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_srls_run);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_lit
 *
 * \par Returns The number of Literals known by this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_lit(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_lits);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_var
 *
 * \par Returns The number of Variables known to this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_var(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_vars);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_mac
 *
 * \par Returns The number of Macros known to this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_mac(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_macros);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_run_mac
 *
 * \par Returns The number of Macros tun by this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_run_mac(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_macros_run);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_num_ctrl
 *
 * \par Returns The number of Controls known to this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_num_ctrl(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_ctrls);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_run_ctrl
 *
 * \par Returns The number of Controls run by this agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_run_ctrl(tdc_t params)
{
	return val_from_uint(gAgentInstr.num_ctrls_run);
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_get_curtime
 *
 * \par Returns The current UTC time on the agent.
 *
 * \param[in] params  Unused. This control takes 0 parameters.
 *
 * \return The value_t holding the result, or type UNK on error.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Use val_from.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_get_curtime(tdc_t params)
{
	struct timeval cur_time;

	getCurrentTime(&cur_time);

	return val_from_uint((uint32_t) cur_time.tv_sec);
}


/* Control Functions */

/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_adm_lst
 *
 * \par Causes the agent to produce a report entry detailing the list of all
 *      ADMs supported by the agent and available for use.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   Unused. This control takes 0 parameters.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return A TDC of strings representing each ADM known to this agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t* adm_agent_ctl_adm_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t data_size = 0;
	amp_type_e type = AMP_TYPE_UNK;

	*status = CTRL_FAILURE;

	/* Step 1: Construct the values. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
    {
    	*status = CTRL_FAILURE;
    	return NULL;
    }

	data = utils_serialize_string("AGENT", &data_size);
	type = tdc_insert(result, AMP_TYPE_STRING, data, data_size);
	SRELEASE(data);

	if(type != AMP_TYPE_STRING)
	{
		tdc_destroy(&result);
		return NULL;
	}

	data = utils_serialize_string("BP", &data_size);
	type = tdc_insert(result, AMP_TYPE_STRING, data, data_size);
	SRELEASE(data);
	if(type != AMP_TYPE_STRING)
	{
		tdc_destroy(&result);
		return NULL;
	}

#ifdef _HAVE_LTP_ADM_
	data = utils_serialize_string("LTP", &data_size);
	type = tdc_insert(result, AMP_TYPE_STRING, data, data_size);
	SRELEASE(data);
	if(type != AMP_TYPE_STRING)
	{
		tdc_destroy(&result);
		return NULL;
	}
#endif /* _HAVE_LTP_ADM_ */

#ifdef _HAVE_ION_ADM_
	data = utils_serialize_string("ION", &data_size);
	type = tdc_insert(result, AMP_TYPE_STRING, data, data_size);
	SRELEASE(data);
	if(type != AMP_TYPE_STRING)
	{
		tdc_destroy(&result);
		return NULL;
	}
#endif /* _Have_ION_ADM_ */

    *status = CTRL_SUCCESS;
    return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_var_add
 *
 * \par Defines a new variable on the agent. The added VAR can be interpetted
 *      as having the declaration:
 *
 *      Type Id = EVAL(Initializer);
 *
 *      Unless the type of the variable is EXPR, in which case the variable should
 *      be interpretted as having the following definition:
 *
 *      Type Id = Initializer;
 *
 *      The parameters for this control are as follows.
 *
 *       +--------+-------------+--------+
 *       |   Id   | Initializer |  Type  |
 *       |  [MID] |    [EXPR]   | [BYTE] |
 *       +--------+-------------+--------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for this control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.  This control does not generate an RPTE.
 *
 * \par Notes:
 *  - This control will fail if the variable ID is already defined.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_var_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	uint32_t tmp = 0;
	amp_type_e type = AMP_TYPE_UNK;
	expr_t *expr = NULL;
	var_t *var = NULL;
	value_t val;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 1.1: Make sure this isn't a duplicate. */
	if(agent_vdb_var_find(mid) != NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Dup. Id. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 1.2: Verify this is a good MID for a VAR. */
	if(MID_GET_FLAG_ID(mid->flags) != MID_COMPUTED)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_var_add","Bad MID.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 2: Grab the expression that defines the VAR. */
	if((expr = adm_extract_expr(params, 1, &success)) == NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Parm 1 (EXPR) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the type that defines the VAR. */
	tmp = (uint32_t) adm_extract_sdnv(params, 2, &success);
	if(success != 1)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Parm 2 (SDNV) Err.", NULL);
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}

	/* Step 3.1: Verify the type is valid. */
	type = type_from_uint(tmp);
	if(type == AMP_TYPE_UNK)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Bad type %d.", type);
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}

	/* Step 3.2: Verify if type != EXPR, that expr value = type value. */
	if((type != AMP_TYPE_EXPR) && (type != expr->type))
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Expr has wrong type %d not %d.", expr->type, type);
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}


	/* Step 4 : Construct the variable. Note, this function shallow-copies the
	 *          MID so do not release it. Also, passing an empty val to the
	 *          create function will cause the expression to be evaluated to
	 *          generate a value.
	 */
	val_init(&val);
	if((var = var_create(mid, type, expr, val)) == NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_var_add","Can't make VAR.", NULL);
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}


	/* Step 5: Release the expr, it is not used after the VAR create. */
	expr_release(expr);

	/* Step 6: Update ADM Agent statistics. */
	agent_db_var_persist(var);
	ADD_VAR(var);

	gAgentInstr.num_vars = agent_db_count(gAgentVDB.vars, &(gAgentVDB.var_mutex)) +
			                    agent_db_count(gAdmComputed, NULL);

	*status = CTRL_SUCCESS;

	return NULL;
}




/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_var_del
 *
 * \par Removes a variable definition from the agent.
 *
 *      The parameters for this control are as follows.
 *
 *       +---------------+
 *       | Ids To Remove |
 *       |      [MC]     |
 *       +---------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for this control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.  This control does not generate an RPTE.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_var_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_var_del","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		if(mid != NULL)
		{
			agent_db_var_forget(mid);
			agent_vdb_var_forget(mid);
		}
	}

	/* Step 3: Cleanup. */
	midcol_destroy(&mc);

	gAgentInstr.num_vars = agent_db_count(gAgentVDB.vars, &(gAgentVDB.var_mutex)) +
			                    agent_db_count(gAdmComputed, NULL);


	*status = CTRL_SUCCESS;
	return NULL;
}




/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_var_lst
 *
 * \par Lists variable definitions known to the agent.
 *
 *      This control does not take any parameters.
 *
 *      Generates a report entry of the following format:
 *
 *      +--------------------+
 *      | Known Variable IDs |
 *      |         [MC]       |
 *      +--------------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   This control does not take parameters.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return The list of known variable names as a MC.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_var_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 0: Sanity Check. */

	/* Step 1: Build an MC of known variables. */
	if((mc = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_var_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	lockResource(&(gAgentVDB.var_mutex));
	for(elt = lyst_first(gAgentVDB.vars); elt; elt = lyst_next(elt))
	{
		var_t *cur = (var_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.var_mutex));

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("adm_agent_ctl_var_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */

	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	if(tdc_insert(retval, AMP_TYPE_MC, data, data_len) != AMP_TYPE_MC)
	{
		tdc_destroy(&retval);
		SRELEASE(data);
		AMP_DEBUG_ERR("adm_agent_ctl_var_lst","Can't add MC to TDC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_var_dsc
 *
 * \par Described certain variables known to the agent.
 *
 *      This control takes 1 parameter:.
 *
 *      +-----------------+
 *      | Ids To Describe |
 *      |       [MC]      |
 *      +-----------------+
 *
 *      Generates a report entry of the following format:
 *
 *      +--------+--------+--------+    +--------+
 *      | # VARS | Types  | VAR 1  |... | VAR N  |
 *      | [SDNV] | [BLOB] | [BLOB] |    | [BLOB] |
 *      +--------+--------+--------+    +--------+
 *                          ||
 *                          ||
 *             ____________/  \__________
 *            /                          \
 *            +-------+--------+--------+
 *            |  ID   |  TYPE  |  Value |
 *            | [MID] | [BYTE] | [BLOB] |
 *            +-------+--------+--------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   This control does not take parameters.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return Description of variables.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_var_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_var_dsc","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("adm_agent_ctl_var_dsc","Can't make tdc.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		var_t *cur = agent_vdb_var_find(mid);

		if((data = var_serialize(cur, &data_len)) != NULL)
		{
			tdc_insert(retval, AMP_TYPE_VAR, data, data_len);
			SRELEASE(data);
		}
		else
		{
			AMP_DEBUG_ERR("adm_agent_ctl_var_dsc","Cannot serialize VAR.", NULL);
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}



/* REPORT-RELATED CONTROLS. */


/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_rptt_add
 *
 * \par Defines a new report template on the agent.
 *
 *      This control takes 2 parameter:.
 *
 *      +--------+------------+
 *      | Rpt Id | Definition |
 *      |  [MID] |    [MC]    |
 *      +--------+------------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  07/31/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_rptt_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	int8_t success = 0;
	def_gen_t *result = NULL;
	mid_t *mid = NULL;
	Lyst expr = NULL;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_add","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 2: Make sure this is not a duplicate definition. */
	if(agent_vdb_report_find(mid) != NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_rptt_add","Report already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the expression capturing the definition. */
	if((expr = adm_extract_mc(params, 1, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_add","Parm 1 (MC) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Create the new definition. This is a shallow copy so
	 * don't release the mid and expr.  */
	if((result = def_create_gen(mid, AMP_TYPE_RPT, expr)) == NULL)
	{
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

	/* Step 5: Add the report. */
	agent_db_report_persist(result);
	ADD_REPORT(result);

	gAgentInstr.num_rptt_defs = agent_db_count(gAgentVDB.reports, &(gAgentVDB.reports_mutex)) +
			                    agent_db_count(gAdmRpts, NULL);

	*status = CTRL_SUCCESS;

	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_rptt_del
 *
 * \par Removes a report template definition from the agent.
 *
 *      This control takes 1 parameter:
 *
 *      +---------------+
 *      | Ids To Remove |
 *      |      [MC]     |
 *      +---------------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_rptt_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;

	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params,0,&success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_del","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		if(mid != NULL)
		{
			agent_db_report_forget(mid);
			agent_vdb_report_forget(mid);
		}
	}

	midcol_destroy(&mc);

	/* Step 3: Update metrics. */
	gAgentInstr.num_rptt_defs = agent_db_count(gAgentVDB.reports, &(gAgentVDB.reports_mutex)) +
			                    agent_db_count(gAdmRpts, NULL);

	*status = CTRL_SUCCESS;
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_rptt_lst
 *
 * \par Lists all report template definition on the agent.
 *
 *      This control takes no parameters
 *
 *      This control generates the following report entry.
 *
 *      +---------------+
 *      | Known Reports |
 *      |      [MC]     |
 *      +---------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_rptt_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_lst","Can't make MC.", NULL);
		return NULL;
	}

	/* Step 2: Build list of report MIDs. */
	lockResource(&(gAgentVDB.reports_mutex));
	for(elt = lyst_first(gAgentVDB.reports); elt; elt = lyst_next(elt))
	{
		def_gen_t *cur = (def_gen_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.reports_mutex));

	/* Step 3: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_lst","Can't make MC.", NULL);
		return NULL;
	}

	/* Step 4: Populate the return RPTE. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, AMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_rptt_dsc
 *
 * \par Produce a description of the given report templates.
 *
 *      This control takes 1 parameter
 *
 *      +-----------------+
 *      | Ids To Describe |
 *      |       [MC]     |
 *      +-----------------+
 *
 *      This control generates the following report entry.
 *
 *      +--------+--------+--------+    +--------+
 *      | # Tpls |  Types |  Tpl 1 |... |  Tpl N |
 *      | [SDNV] | [BLOB] | [BLOB] |    | [BLOB] |
 *      +--------+--------+--------+    +--------+
 *                           ||
 *                           ||
 *                 _________/  \_________
 *                /                      \
 *                 +-------+------------+
 *                 |  ID   | Definition |
 *                 | [MID] |    [MC]    |
 *                 +-------+------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_rptt_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_dsc","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("adm_agent_ctl_rptt_dsc","Can't make lyst.", NULL);
		return NULL;
	}

	/* Step 3: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		def_gen_t *cur = agent_vdb_report_find(mid);

		data = def_serialize_gen(cur, &data_len);
		tdc_insert(retval, AMP_TYPE_RPT, data, data_len);
		SRELEASE(data);
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}


/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_rpt_gen
 *
 * \par Purpose: Causes a report to be generated by the agent and sent to
 *               appropriate managers.
 *
 *      This control takes 2 parameters
 *
 *      +---------------------+---------------------+
 *      | Reports to Generate | Managers to Receive |
 *      |         [MC]        |         [DC]        |
 *      +---------------------+---------------------+
 *
 *      This control does not, itself, return a report entry. It adds a report
 *      entry manually to the next outgoing report to the given managers. The
 *      report entries (RPTEs) added are formatted as follows.
 *
 *                           +---------+     +---------+
 *                           | Entry 1 |     | Entry N |
 *                           |  [RPTE] | ... |  [RPTE] |
 *                           +---------+     +---------+
 *                                ||
 *                                ||
 *                        _______/  \_______
 *                       /                  \
 *                        +-------+--------+
 *                        |   ID  | Values |
 *                        | [MID] | [TDC]  |
 *                        +-------+--------+
 *                                   ||
 *                                   ||
 *       ___________________________/  \_______________________________
 *      /                                                              \
 *       +----------+-------------+---------+---------+     +---------+
 *       | # Values | Value Types | Value 1 | Value 2 |     | Value N |
 *       |  [SDNV]  |    [BLOB]   | [BLOB]  | [BLOB]  | ... |  [BLOB] |
 *       +----------+-------------+---------+---------+     +---------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
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
 *  05/17/15  E. Birrane     Initial implementation, (Secure DTN - NASA: NNX14CS58P)
 *  06/21/15  E. Birrane     Updated to new control return code structure. (Secure DTN - NASA: NNX14CS58P)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_rpt_gen(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	rpt_t *report = NULL;
	eid_t recipient;
	Lyst mc = NULL;
	Lyst rx = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;
	memset(&recipient, 0, sizeof(eid_t));

	/* Step 1: Get the MC from the params. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rpt_gen","Parm 0 (MC) Err.", NULL);
		return NULL;
	}

	/* Step 2: Get the list of mgrs to receive this report. */
	if((rx = adm_extract_dc(params,1, &success)) == NULL)
	{
		/* Step 2.1: If we don't have, or can't get the rx list send report back to requester. */
		memcpy(&recipient, def_mgr, sizeof(eid_t));
	}
	else
	{
		blob_t *entry = NULL;

		if((entry = lyst_data(lyst_first(rx))) == NULL)
		{
			/* Step 2.1: If we don't have, or can't get the rx list send report back to requester. */
			memcpy(&recipient, def_mgr, sizeof(eid_t));
		}
		else if(entry->length == 0)
		{
			/* Step 2.1: If we don't have, or can't get the rx list send report back to requester. */
			memcpy(&recipient, def_mgr, sizeof(eid_t));
		}
		else if(entry->length < sizeof(recipient))
		{
			memcpy(&recipient, entry->value, entry->length);
		}
		else
		{
			AMP_DEBUG_ERR("adm_agent_ctl_rpt_gen", "Rx length of %d > %d.", entry->length, sizeof(eid_t));
			dc_destroy(&rx);
			midcol_destroy(&mc);
			return NULL;
		}

		dc_destroy(&rx);
	}

	/* Step 3: Build the report.
	 *
	 * This function will grab an existing to-be-sent report or
	 * create a new report and add it to the "built-reports" section.
	 * Either way, it will be included in the next set of code to send
	 * out reports built in this time slice.
	 */
	if((report = rda_get_report(recipient)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_rpt_gen", "Cannot allocate report.", NULL);
		midcol_destroy(&mc);
		return NULL;
	}


	/* Step 4: For each MID in the report definition, construct the
	 * entry and add it to the report.
	 */
	for(elt = lyst_first(mc); elt != NULL; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		rpt_entry_t *entry = rda_build_report_entry(mid);

		if(entry != NULL)
		{
			lyst_insert_last(report->entries, entry);
		}
		else
		{
			char *midstr = mid_to_string(mid);
			AMP_DEBUG_ERR("adm_agent_ctl_rpt_gen","Can't build report for %s", midstr);
			SRELEASE(midstr);
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;

    return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_mac_add
 *
 * \par Defines a new macro on the agent.
 *
 *      This control takes 3 parameter:.
 *
 *      +------------+----------+----------+
 *      | Macro Name | Macro Id | Controls |
 *      |    [STR]   |   [MID]  |   [MC]   |
 *      +------------+----------+----------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_mac_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	Lyst expr = NULL;
	def_gen_t *result = NULL;
	int8_t success = 0;
	char *name = NULL;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the first parameter, the Macro name. */
	if((name = adm_extract_string(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_mac_add","Parm 0 (STR) Err.", NULL);
		return NULL;
	}

	/* Step 2: We ignore the name. */

	SRELEASE(name);

	/* Step 3: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 1, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_mac_add","Parm 1 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 4: Make sure the macro is not already defined. */
	if(agent_vdb_macro_find(mid) != NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_mac_add","Macro ID already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 5: Grab the expression capturing the definition. */
	if((expr = adm_extract_mc(params, 2, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_mac_add","Parm 2 (MC) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 6: Add the new definition.
	 * This is a shallow-copy so don't release mid and expr.
	 */

	if((result = def_create_gen(mid, AMP_TYPE_MACRO, expr)) == NULL)
	{
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

	/* Step 7: Persist the Macro definition. */
	agent_db_macro_persist(result);
	ADD_MACRO(result);

	gAgentInstr.num_macros = agent_db_count(gAgentVDB.macros, &(gAgentVDB.macros_mutex)) +
			                    agent_db_count(gAdmMacros, NULL);

	*status = CTRL_SUCCESS;

	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_mac_del
 *
 * \par Removes a macro definition from the agent.
 *
 *      This control takes 1 parameter:.
 *
 *      +---------------+
 *      | Ids To Remove |
 *      |      [MC]     |
 *      +---------------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_mac_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_mac_del","Parm 0 (MC) err.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		if(mid != NULL)
		{
			agent_db_macro_forget(mid);
			agent_vdb_macro_forget(mid);
		}
	}

	midcol_destroy(&mc);

	/* Step 3: Update metrics. */
	gAgentInstr.num_macros = agent_db_count(gAgentVDB.macros, &(gAgentVDB.macros_mutex)) +
			                    agent_db_count(gAdmMacros, NULL);

	*status = CTRL_SUCCESS;
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_mac_lst
 *
 * \par Lists all macro IDs defined on the agent.
 *
 *      This control takes no parameters
 *
 *      This control generates the following report entry.
 *
 *      +--------------+
 *      | Known Macros |
 *      |     [MC]     |
 *      +--------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_mac_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("agent_ctl_mac_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	lockResource(&(gAgentVDB.macros_mutex));
	for(elt = lyst_first(gAgentVDB.macros); elt; elt = lyst_next(elt))
	{
		def_gen_t *cur = (def_gen_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.macros_mutex));

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("agent_ctl_mac_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, AMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_mac_dsc
 *
 * \par Produce a description of the given macros.
 *
 *      This control takes 1 parameter
 *
 *      +-----------------+
 *      | Ids To Describe |
 *      |       [MC]      |
 *      +-----------------+
 *
 *      This control generates the following report entry.
 *
 *      +--------+--------+------------+   +------------+
 *      | # Defs | Types  | MacroDef 1 |...| MacroDef N |
 *      | [SDNV] | [BLOB] |   [BLOB]   |   |   [BLOB]   |
 *      +--------+--------+------------+   +------------+
 *                             ||
 *                             ||
 *                    ________/  \__________
 *                   /                      \
 *                    +-------+------------+
 *                    |  ID   | Definition |
 *                    | [MID] |    [MC]    |
 *                    +-------+------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_mac_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_mac_dsc","Parm 0 (MC) Err.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_mac_dsc","Can't make lyst.", NULL);
		midcol_destroy(&mc);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		def_gen_t *cur = agent_vdb_macro_find(mid);

		data = def_serialize_gen(cur, &data_len);
		tdc_insert(retval, AMP_TYPE_RPT, data, data_len);
		SRELEASE(data);
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_trl_add
 *
 * \par Defines a new TRL on the agent.
 *
 *      This control takes 5 parameters.
 *
 *      +-------+-------+------------+--------+--------+
 *      |  Id   | Start | Period (s) | Count  | Action |
 *      | [MID] |  [TS] |   [SDNV]   | [SDNV] |  [MC]  |
 *      +-------+-------+------------+--------+--------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_trl_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	Lyst action = NULL;
	uint32_t offset = 0;
	uvast count = 0;
	uvast period = 0;
	trl_t *rule = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new rule. */
	if((mid = adm_extract_mid(params,0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_add","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	if(agent_vdb_trl_find(mid) != NULL)
	{
		AMP_DEBUG_WARN("adm_agent_ctl_trl_add","TRL already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}


	/* Step 2: Grab the offset for this rule. */
	offset = adm_extract_sdnv(params, 1, &success);
	if(success == 0)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_add","Parm 1 (SDNV) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the period for this rule. */
	period = adm_extract_sdnv(params, 2, &success);
	if(success == 0)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_add","Parm 2 (SDNV) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Grab the count for this rule. */
	count = adm_extract_sdnv(params, 3, &success);
	if(success == 0)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_add","Parm 3 (SDNV) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 5: Grab the macro action for this rule. */
	if((action =adm_extract_mc(params, 4, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_add","Parm 4 (MC) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 6: Create the new rule. */
	if((rule = trl_create(mid, (time_t) offset, period, count, action)) != NULL)
	{
		*status = CTRL_SUCCESS;

		rule->desc.sender = *def_mgr;
		agent_db_trl_persist(rule);
		ADD_TRL(rule);
	}

	gAgentInstr.num_trls = agent_db_count(gAgentVDB.trls, &(gAgentVDB.trls_mutex));


	mid_release(mid);
	midcol_destroy(&action);
	return NULL;
}


/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_trl_del
 *
 * \par Removes a TRL definition from the agent.
 *
 *      This control takes 1 parameter:.
 *
 *      +---------------+
 *      | Ids To Remove |
 *      |      [MC]     |
 *      +---------------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_trl_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_del","Parm 0 (MC) Err.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		if(mid != NULL)
		{
			agent_db_trl_forget(mid);
			agent_vdb_trl_forget(mid);
		}
	}

	midcol_destroy(&mc);

	gAgentInstr.num_trls = agent_db_count(gAgentVDB.trls, &(gAgentVDB.trls_mutex));

	*status = CTRL_SUCCESS;
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_trl_lst
 *
 * \par Lists all TRL IDs defined on the agent.
 *
 *      This control takes no parameters
 *
 *      This control generates the following report entry.
 *
 *      +--------------+
 *      |  Known TRLs  |
 *      |     [MC]     |
 *      +--------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_trl_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;


	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	lockResource(&(gAgentVDB.trls_mutex));
	for(elt = lyst_first(gAgentVDB.trls); elt; elt = lyst_next(elt))
	{
		def_gen_t *cur = (def_gen_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.trls_mutex));

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("adm_agent_ctl_trl_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, AMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_trl_dsc
 *
 * \par Produce a description of the given TRLs.
 *
 *      This control takes 1 parameter
 *
 *      +-----------------+
 *      | Ids To Describe |
 *      |       [MC]      |
 *      +-----------------+
 *
 *      This control generates the following report entry.
 *
 *         +--------+--------+--------+    +--------+
 *         | # TRLs | Types  | TRL 1  |... | TRL N  |
 *         | [SDNV] | [BLOB] | [BLOB] |    | [BLOB] |
 *         +--------+--------+--------+    +--------+
 *                              ||
 *                              ||
 *          ___________________/  \_____________________
 *         /                                            \
 *          +-------+-------+--------+--------+--------+
 *          |  ID   | Start | Period | Count  | Action |
 *          | [MID] | [TS]  | [SDNV] | [SDNV] |  [MC]  |
 *          +-------+-------+--------+--------+--------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/03/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_trl_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_dsc","Parm 0 (MC) Err.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_trl_dsc","Can't make lyst.", NULL);

		midcol_destroy(&mc);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		trl_t *trl = agent_vdb_trl_find(mid);

		if(trl != NULL)
		{
			data = trl_serialize(trl, &data_len);
			tdc_insert(retval, AMP_TYPE_TRL, data, data_len);
			SRELEASE(data);
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_srl_add
 *
 * \par Defines a new SRL on the agent.
 *
 *      This control takes 5 parameters.
 *
 *       +-------+-------+--------+--------+--------+
 *       |  ID   | START |  COND  | COUNT  | ACTION |
 *       | [MID] | [TS]  | [PRED] | [SDNV] |  [MC]  |
 *       +-------+-------+--------+--------+--------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_srl_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	Lyst action = NULL;
	expr_t *expr = NULL;
	uvast offset = 0;
	uvast count = 0;
	srl_t *rule = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new rule. */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_add","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	if(agent_vdb_srl_find(mid) != NULL)
	{
		AMP_DEBUG_WARN("agent_ctl_srl_add","MID already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 2: Grab the offset for this rule. */
	offset = adm_extract_sdnv(params, 1, &success);
	if(success == 0)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_add","Parm 1 (SDNV) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the expression for this rule. */
	if((expr = adm_extract_expr(params, 2, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_add","Parm 2 (SDNV) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Grab the count for this rule. */
	count = adm_extract_sdnv(params, 3, &success);
	if(success == 0)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_add","Parm 3 (SDNV) Err.", NULL);
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}

	/* Step 5: Grab the macro action for this rule. */
	if((action =adm_extract_mc(params, 4, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_add","Parm 4 (MC) Err.", NULL);
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}

	/* Step 6: Create the new rule. */
	if((rule = srl_create(mid, offset, expr, count, action)) != NULL)
	{
		*status = CTRL_SUCCESS;
		agent_db_srl_persist(rule);
		ADD_SRL(rule);
	}

	mid_release(mid);
	expr_release(expr);
	midcol_destroy(&action);

	gAgentInstr.num_srls = agent_db_count(gAgentVDB.srls, &(gAgentVDB.srls_mutex));

	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_srl_del
 *
 * \par Removes a SRL definition from the agent.
 *
 *      This control takes 1 parameter:.
 *
 *      +---------------+
 *      | Ids To Remove |
 *      |      [MC]     |
 *      +---------------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_srl_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params,0,&success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_del", "Parm 0 (MC) Err.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		if(mid != NULL)
		{
			agent_db_srl_forget(mid);
			agent_vdb_srl_forget(mid);
		}
	}

	midcol_destroy(&mc);

	gAgentInstr.num_srls = agent_db_count(gAgentVDB.srls, &(gAgentVDB.srls_mutex));

	*status = CTRL_SUCCESS;
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_srl_lst
 *
 * \par Lists all SRLs defined on the agent.
 *
 *      This control takes no parameters
 *
 *      This control generates the following report entry.
 *
 *      +--------------+
 *      |  Known SRLs  |
 *      |     [MC]     |
 *      +--------------+
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_srl_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_lst","Can't make Lyst.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	lockResource(&(gAgentVDB.srls_mutex));
	for(elt = lyst_first(gAgentVDB.srls); elt; elt = lyst_next(elt))
	{
		def_gen_t *cur = (def_gen_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.srls_mutex));

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		AMP_DEBUG_ERR("adm_agent_ctl_srl_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, AMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_srl_dsc
 *
 * \par Produce a description of the given SRLs.
 *
 *      This control takes 1 parameter
 *
 *      +-----------------+
 *      | Ids To Describe |
 *      |       [MC]      |
 *      +-----------------+
 *
 *      This control generates the following report entry.
 *
 *         +--------+--------+--------+    +--------+
 *         | # SRLs | Types  | SRL 1  |... | SRL N  |
 *         | [SDNV] | [BLOB] | [BLOB] |    | [BLOB] |
 *         +--------+--------+--------+    +--------+
 *                              ||
 *                              ||
 *          ___________________/  \_____________________
 *         /                                            \
 *          +-------+-------+--------+--------+-------+
 *          |  ID   | START |  COND  | COUNT  |   ACT |
 *          | [MID] | [TS]  | [PRED] | [SDNV] |  [MC] |
 *          +-------+-------+--------+--------+-------+
 *
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation. (JHU/APL)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t *adm_agent_ctl_srl_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_dsc","Parm 0 (MC) Err.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_srl_dsc","Can't make lyst.", NULL);

		midcol_destroy(&mc);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		srl_t *srl = agent_vdb_srl_find(mid);

		if(srl != NULL)
		{
			if((data = srl_serialize(srl, &data_len)) != NULL)
			{
				tdc_insert(retval, AMP_TYPE_SRL, data, data_len);
				SRELEASE(data);
			}
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_stor
 *
 * \par Store a value into a variable.
 *
 *      This control takes 2 parameter
 *
 *      +--------+--------+
 *      | Target |  Expr. |
 *      | [VAR]  | [EXPR] |
 *      +--------+--------+
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t* adm_agent_ctl_stor(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	var_t *var = NULL;
	expr_t *expr = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the VAR MID to hold the result.
	 * We don't need to sanity check the MID or ensure it is a VAR
	 * here because we will use it to look up the VAR later and if the
	 * MID is bad, the lookup will fail.
	 * */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_stor","Parm 0 (MID) Err.", NULL);
		return NULL;
	}

	/* Step 2: Grab the EXPR to evaluate. */
	if((expr = adm_extract_expr(params, 1, &success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_agent_ctl_stor","Parm 1 (EXPR) Err.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Make sure this isn't an ADM-defined MID. We can't re-assign
	 * them.
	 */
 	if((var = var_find_by_id(gAdmComputed, NULL, mid)) != NULL)
	{
 		AMP_DEBUG_ERR("adm_agent_ctl_stor", "Cannot target ADM-defined VAR.", NULL);
	}

 	/* Step 4 - Find the VAR definitions to update. */
 	else if((var = var_find_by_id(gAgentVDB.vars, &(gAgentVDB.var_mutex), mid)) != NULL)
    {
		value_t tmp = expr_eval(expr);

		/* Step 4.1: Can the VAR hold the expression result? */
		if(val_cvt_type(&tmp, var->value.type) == 1)
		{
			// Free current VAR value.
			val_release(&(var->value), 0);

			// Can shallow copy this since tmp is a deep copy.
			var->value = tmp;

			*status = CTRL_SUCCESS;
		}
		else
		{
			AMP_DEBUG_ERR("adm_agent_ctl_stor", "Cannot convert from type %d to %d.", tmp.type, var->value.type);
			val_release(&tmp, 0);
		}
    }
	else
	{
		AMP_DEBUG_ERR("adm_agent_ctl_stor", "Cannot find VAR.", NULL);
	}

 	mid_release(mid);
 	expr_release(expr);

 	return NULL;
}


/******************************************************************************
 *
 * \par Function Name: adm_agent_ctl_reset_cnt
 *
 * \par Reset Agent ADM metric counters
 *
 *      This control takes no parameters
 *
 *      This control generates no report entries.
 *
 * \param[in]  def_mgr  The default manager receiving the report entry.
 * \param[in]  params   The parameters for the control.
 * \param[out] status   Whether the control passed or failed.
 *
 * \return NULL.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

tdc_t* adm_agent_ctl_reset_cnt(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	agent_instr_clear();
	*status = CTRL_SUCCESS;
	return NULL;
}



/*
 * OP FUnctions
 */

/******************************************************************************
 *
 * \par Function Name: adm_agent_op_prep
 *
 * \par Fetch operator parameters from an operand stack.
 *
 * \param[in]  num    # operands to pop (1 or 2)
 * \param[in]  stack  The operand stack
 * \param[out] lval   The first (or only) operand popped
 * \param[out] rval   The second operand popped
 *
 * \return 1 (success) or 0 (failure).
 *
 * \par Notes:
 *    - The lval and rval are shallow-copied
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

uint8_t adm_agent_op_prep(int num, Lyst stack, value_t *lval, value_t *rval)
{
    LystElt elt = 0;
    value_t *tmp = NULL;

	if((stack == NULL) || (num < 1) || (num > 2) ||
		(lval == NULL) ||
		((num == 2) && (rval == NULL)) || (lyst_length(stack) != num))
	{
        AMP_DEBUG_ERR("adm_agent_op_prep","Bad params.",NULL);
        return 0;
	}

	/* Step 2: Grab the L-value and R-value. */
	elt = lyst_first(stack);
	if((tmp = (value_t*) lyst_data(elt)) == NULL)
	{
        AMP_DEBUG_ERR("adm_agent_op_prep","NULL lval.",NULL);
        return 0;
	}
	else
	{
		*lval = *tmp;
	}
	if(num == 2)
	{
		elt = lyst_next(elt);
		if((tmp = (value_t*) lyst_data(elt)) == NULL)
		{
			AMP_DEBUG_ERR("adm_agent_op_prep","NULL rval.",NULL);
			return 0;
		}
		else
		{
			*rval = *tmp;
		}
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_unary_num_op
 *
 * \par Implement a unary numerical operator.
 *
 * \param[in]  op    The OP to apply.
 * \param[in]  stack  The operand stack
 *
 * \return The value of the operation. An invalid value_t indicates failure.
 *
 * \par Notes:
 *    - This implements an operator for INTs, UINTs, VASTs, UVASTS, REAL32s, and
 *      REAL64s.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_unary_num_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t val;
	uint8_t s = 0;

	/* Step 1 - Prep for the operation. */
	val_init(&result);

	if(adm_agent_op_prep(1, stack, &val, NULL) == 0)
	{
        AMP_DEBUG_ERR("adm_agent_unary_num_op","Can't get lval.",NULL);
        return result;
	}

	result.type = val.type;

    /* Step 2 - Sanity check the prep. */
    if(result.type == AMP_TYPE_UNK)
    {
        AMP_DEBUG_ERR("adm_agent_unary_num_op","Bad params.",NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */

    switch(result.type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
			case NEG:    result.value.as_int = 0 - val_cvt_int(val, &s); break;
			case BITNOT: result.value.as_int = ~val_cvt_int(val, &s); break;
			case ABS:    result.value.as_int = abs(val_cvt_int(val, &s)); break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
			case BITNOT: result.value.as_uint = ~val_cvt_uint(val, &s); break;
			case ABS:    result.value.as_uint = val_cvt_uint(val, &s); break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
			case NEG:    result.value.as_vast = 0 - val_cvt_vast(val, &s); break;
			case BITNOT: result.value.as_vast = ~val_cvt_vast(val, &s); break;
			case ABS:    result.value.as_vast = labs(val_cvt_vast(val, &s)); break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
			case ABS:    result.value.as_uvast = val_cvt_uvast(val, &s); break;
			case BITNOT: result.value.as_uvast = ~val_cvt_uvast(val, &s); break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
			case NEG:    result.value.as_real32 = 0 - val_cvt_real32(val, &s); break;
			case ABS:    result.value.as_real32 = fabs(val_cvt_real32(val, &s)); break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
			case NEG:    result.value.as_real64 = 0 - val_cvt_real64(val, &s); break;
			case ABS:    result.value.as_real64 = fabs(val_cvt_real64(val, &s)); break;
			default: s = 0; break;
		}
		break;
	default:
		s = 0;
		break;
	}

	if(s == 0)
	{
        AMP_DEBUG_ERR("adm_agent_unary_num_op","Bad op (%d) or type (%d).",op, result.type);
    	val_init(&result);
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_unary_log_op
 *
 * \par Implement a unary logical operator.
 *
 * \param[in]  op    The OP to apply.
 * \param[in]  stack  The operand stack
 *
 * \return The value of the operation. An invalid value_t indicates failure.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_unary_log_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t val;
	uint8_t s = 0;

	/* Step 1 - Prep for the operation. */
    val_init(&result);
    if(adm_agent_op_prep(1, stack, &val, NULL) == 0)
	{
        AMP_DEBUG_ERR("adm_agent_unary_log_op","Can't get lval.",NULL);
        return result;
	}

    /* Step 2 - Sanity check the prep. */
    if(val.type == AMP_TYPE_UNK)
    {
        AMP_DEBUG_ERR("adm_agent_unary_log_op","Bad params.",NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
    result.type = val.type;
    switch(val.type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
			case LOGNOT: result.value.as_uint = val_cvt_int(val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
			case LOGNOT: result.value.as_uint = val_cvt_uint(val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
			case LOGNOT: result.value.as_uint = val_cvt_vast(val, &s) ? 0 : 1; break;
			default: s = 0; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
			case LOGNOT: result.value.as_uint = val_cvt_uvast(val, &s) ? 0 : 1; break;
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
        AMP_DEBUG_ERR("adm_agent_unary_log_op","Bad op (%d) or type (%d).",op, result.type);
        val_init(&result);
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_agent_binary_log_op
 *
 * \par Implement a binary logical operator.
 *
 * \param[in]  op    The OP to apply.
 * \param[in]  stack  The operand stack
 *
 * \return The value of the operation. An invalid value_t indicates failure.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_binary_log_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t lval, rval;
	uint32_t cvt_type = AMP_TYPE_UNK;
	uint8_t ls = 0;
	uint8_t rs = 0;

	/* Step 1 - Prep for the operation. */
	val_init(&result);
	if(adm_agent_op_prep(2, stack, &lval, &rval) == 0)
	{
        AMP_DEBUG_ERR("adm_agent_binary_log_op","Can't get lval.",NULL);
        return result;
	}

	cvt_type = val_get_result_type(lval.type, rval.type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == AMP_TYPE_UNK)
    {
        AMP_DEBUG_ERR("adm_agent_binary_log_op","Bad params.",NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
    result.type = cvt_type;
    switch(cvt_type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
		case LOGAND: result.value.as_uint = val_cvt_int(lval, &ls) && val_cvt_int(rval, &rs); break;
		case LOGOR:  result.value.as_uint = val_cvt_int(lval, &ls) || val_cvt_int(rval, &rs); break;
		case LT:     result.value.as_uint = val_cvt_int(lval, &ls) < val_cvt_int(rval, &rs); break;
		case GT:     result.value.as_uint = val_cvt_int(lval, &ls) > val_cvt_int(rval, &rs); break;
		case LTE:    result.value.as_uint = val_cvt_int(lval, &ls) <= val_cvt_int(rval, &rs); break;
		case GTE:    result.value.as_uint = val_cvt_int(lval, &ls) >= val_cvt_int(rval, &rs); break;
		case EQ:     result.value.as_uint = val_cvt_int(lval, &ls) == val_cvt_int(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
		case LOGAND: result.value.as_uint = val_cvt_uint(lval, &ls) && val_cvt_uint(rval, &rs); break;
		case LOGOR:  result.value.as_uint = val_cvt_uint(lval, &ls) || val_cvt_uint(rval, &rs); break;
		case LT:     result.value.as_uint = val_cvt_uint(lval, &ls) < val_cvt_uint(rval, &rs); break;
		case GT:     result.value.as_uint = val_cvt_uint(lval, &ls) > val_cvt_uint(rval, &rs); break;
		case LTE:    result.value.as_uint = val_cvt_uint(lval, &ls) <= val_cvt_uint(rval, &rs); break;
		case GTE:    result.value.as_uint = val_cvt_uint(lval, &ls) >= val_cvt_uint(rval, &rs); break;
		case EQ:     result.value.as_uint = val_cvt_uint(lval, &ls) == val_cvt_uint(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
		case LOGAND: result.value.as_uint = val_cvt_vast(lval, &ls) && val_cvt_vast(rval, &rs); break;
		case LOGOR:  result.value.as_uint = val_cvt_vast(lval, &ls) || val_cvt_vast(rval, &rs); break;
		case LT:     result.value.as_uint = val_cvt_vast(lval, &ls) < val_cvt_vast(rval, &rs); break;
		case GT:     result.value.as_uint = val_cvt_vast(lval, &ls) > val_cvt_vast(rval, &rs); break;
		case LTE:    result.value.as_uint = val_cvt_vast(lval, &ls) <= val_cvt_vast(rval, &rs); break;
		case GTE:    result.value.as_uint = val_cvt_vast(lval, &ls) >= val_cvt_vast(rval, &rs); break;
		case EQ:     result.value.as_uint = val_cvt_vast(lval, &ls) == val_cvt_vast(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
		case LOGAND: result.value.as_uint = val_cvt_uvast(lval, &ls) && val_cvt_uvast(rval, &rs); break;
		case LOGOR:  result.value.as_uint = val_cvt_uvast(lval, &ls) || val_cvt_uvast(rval, &rs); break;
		case LT:     result.value.as_uint = val_cvt_uvast(lval, &ls) < val_cvt_uvast(rval, &rs); break;
		case GT:     result.value.as_uint = val_cvt_uvast(lval, &ls) > val_cvt_uvast(rval, &rs); break;
		case LTE:    result.value.as_uint = val_cvt_uvast(lval, &ls) <= val_cvt_uvast(rval, &rs); break;
		case GTE:    result.value.as_uint = val_cvt_uvast(lval, &ls) >= val_cvt_uvast(rval, &rs); break;
		case EQ:     result.value.as_uint = val_cvt_uvast(lval, &ls) == val_cvt_uvast(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
		case LOGAND: result.value.as_uint = val_cvt_real32(lval, &ls) && val_cvt_real32(rval, &rs); break;
		case LOGOR:  result.value.as_uint = val_cvt_real32(lval, &ls) || val_cvt_real32(rval, &rs); break;
		case LT:     result.value.as_uint = val_cvt_real32(lval, &ls) < val_cvt_real32(rval, &rs); break;
		case GT:     result.value.as_uint = val_cvt_real32(lval, &ls) > val_cvt_real32(rval, &rs); break;
		case LTE:    result.value.as_uint = val_cvt_real32(lval, &ls) <= val_cvt_real32(rval, &rs); break;
		case GTE:    result.value.as_uint = val_cvt_real32(lval, &ls) >= val_cvt_real32(rval, &rs); break;
		case EQ:     result.value.as_uint = val_cvt_real32(lval, &ls) == val_cvt_real32(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
		case LOGAND: result.value.as_uint = val_cvt_real64(lval, &ls) && val_cvt_real64(rval, &rs); break;
		case LOGOR:  result.value.as_uint = val_cvt_real64(lval, &ls) || val_cvt_real64(rval, &rs); break;
		case LT:     result.value.as_uint = val_cvt_real64(lval, &ls) < val_cvt_real64(rval, &rs); break;
		case GT:     result.value.as_uint = val_cvt_real64(lval, &ls) > val_cvt_real64(rval, &rs); break;
		case LTE:    result.value.as_uint = val_cvt_real64(lval, &ls) <= val_cvt_real64(rval, &rs); break;
		case GTE:    result.value.as_uint = val_cvt_real64(lval, &ls) >= val_cvt_real64(rval, &rs); break;
		case EQ:     result.value.as_uint = val_cvt_real64(lval, &ls) == val_cvt_real64(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	default:
		ls = rs = 0;
		break;
	}

	if((ls == 0) || (rs == 0))
	{
        AMP_DEBUG_ERR("adm_agent_binary_log_op","Bad op (%d) or type (%d -> %d).",op, lval.type, rval.type);
        val_init(&result);
	}

	return result;

}



/******************************************************************************
 *
 * \par Function Name: adm_agent_binary_num_op
 *
 * \par Implement a binary numeric operator.
 *
 * \param[in]  op    The OP to apply.
 * \param[in]  stack  The operand stack
 *
 * \return The value of the operation. An invalid value_t indicates failure.
 *
 * \par Notes:
 *    - This implements an operator for INTs, UINTs, VASTs, UVASTS, REAL32s, and
 *      REAL64s.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_binary_num_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t lval, rval;
	uint32_t cvt_type = AMP_TYPE_UNK;
	uint8_t ls = 0;
	uint8_t rs = 0;

	/* Step 1 - Prep for the operation. */
    val_init(&result);
    if(adm_agent_op_prep(2, stack, &lval, &rval) == 0)
	{
        AMP_DEBUG_ERR("adm_agent_binary_num_op","Can't get lval.",NULL);
        return result;
	}

	cvt_type = val_get_result_type(lval.type, rval.type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == AMP_TYPE_UNK)
    {
        AMP_DEBUG_ERR("adm_agent_op_plus","Bad params.",NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
    result.type = cvt_type;
    switch(cvt_type)
	{
	case AMP_TYPE_INT:
		switch(op)
		{
		case PLUS:   result.value.as_int = val_cvt_int(lval, &ls) + val_cvt_int(rval, &rs); break;
		case MINUS:  result.value.as_int = val_cvt_int(lval, &ls) - val_cvt_int(rval, &rs); break;
		case MULT:   result.value.as_int = val_cvt_int(lval, &ls) * val_cvt_int(rval, &rs); break;
		case DIV:    result.value.as_int = val_cvt_int(lval, &ls) / val_cvt_int(rval, &rs); break;
		case MOD:    result.value.as_int = val_cvt_int(lval, &ls) % val_cvt_int(rval, &rs); break;
		case EXP:    result.value.as_int = (int32_t) pow(val_cvt_int(lval, &ls), val_cvt_int(rval, &rs)); break;
		case BITAND: result.value.as_int = val_cvt_int(lval, &ls) & val_cvt_int(rval, &rs); break;
		case BITOR:  result.value.as_int = val_cvt_int(lval, &ls) | val_cvt_int(rval, &rs); break;
		case BITXOR: result.value.as_int = val_cvt_int(lval, &ls) ^ val_cvt_int(rval, &rs); break;
		case BITLSHFT: result.value.as_int = val_cvt_int(lval, &ls) << val_cvt_uint(rval, &rs); break;
		case BITRSHFT: result.value.as_int = val_cvt_int(lval, &ls) >> val_cvt_uint(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_UINT:
		switch(op)
		{
		case PLUS:   result.value.as_uint = val_cvt_uint(lval, &ls) + val_cvt_uint(rval, &rs); break;
		case MINUS:  result.value.as_uint = val_cvt_uint(lval, &ls) - val_cvt_uint(rval, &rs); break;
		case MULT:   result.value.as_uint = val_cvt_uint(lval, &ls) * val_cvt_uint(rval, &rs); break;
		case DIV:    result.value.as_uint = val_cvt_uint(lval, &ls) / val_cvt_uint(rval, &rs); break;
		case MOD:    result.value.as_uint = val_cvt_uint(lval, &ls) % val_cvt_uint(rval, &rs); break;
		case EXP:    result.value.as_uint = (uint32_t) pow(val_cvt_uint(lval, &ls), val_cvt_uint(rval, &rs)); break;
		case BITAND: result.value.as_uint = val_cvt_uint(lval, &ls) & val_cvt_uint(rval, &rs); break;
		case BITOR:  result.value.as_uint = val_cvt_uint(lval, &ls) | val_cvt_uint(rval, &rs); break;
		case BITXOR: result.value.as_uint = val_cvt_uint(lval, &ls) ^ val_cvt_uint(rval, &rs); break;
		case BITLSHFT: result.value.as_uint = val_cvt_uint(lval, &ls) << val_cvt_uint(rval, &rs); break;
		case BITRSHFT: result.value.as_uint = val_cvt_uint(lval, &ls) >> val_cvt_uint(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_VAST:
		switch(op)
		{
		case PLUS:   result.value.as_vast = val_cvt_vast(lval, &ls) + val_cvt_vast(rval, &rs); break;
		case MINUS:  result.value.as_vast = val_cvt_vast(lval, &ls) - val_cvt_vast(rval, &rs); break;
		case MULT:   result.value.as_vast = val_cvt_vast(lval, &ls) * val_cvt_vast(rval, &rs); break;
		case DIV:    result.value.as_vast = val_cvt_vast(lval, &ls) / val_cvt_vast(rval, &rs); break;
		case MOD:    result.value.as_vast = val_cvt_vast(lval, &ls) % val_cvt_vast(rval, &rs); break;
		case EXP:    result.value.as_vast = (vast) pow(val_cvt_vast(lval, &ls), val_cvt_vast(rval, &rs)); break;
		case BITAND: result.value.as_vast = val_cvt_vast(lval, &ls) & val_cvt_vast(rval, &rs); break;
		case BITOR:  result.value.as_vast = val_cvt_vast(lval, &ls) | val_cvt_vast(rval, &rs); break;
		case BITXOR: result.value.as_vast = val_cvt_vast(lval, &ls) ^ val_cvt_vast(rval, &rs); break;
		case BITLSHFT: result.value.as_vast = val_cvt_vast(lval, &ls) << val_cvt_uint(rval, &rs); break;
		case BITRSHFT: result.value.as_vast = val_cvt_vast(lval, &ls) >> val_cvt_uint(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_UVAST:
		switch(op)
		{
		case PLUS:   result.value.as_uvast = val_cvt_uvast(lval, &ls) + val_cvt_uvast(rval, &rs); break;
		case MINUS:  result.value.as_uvast = val_cvt_uvast(lval, &ls) - val_cvt_uvast(rval, &rs); break;
		case MULT:   result.value.as_uvast = val_cvt_uvast(lval, &ls) * val_cvt_uvast(rval, &rs); break;
		case DIV:    result.value.as_uvast = val_cvt_uvast(lval, &ls) / val_cvt_uvast(rval, &rs); break;
		case MOD:    result.value.as_uvast = val_cvt_uvast(lval, &ls) % val_cvt_uvast(rval, &rs); break;
		case EXP:    result.value.as_uvast = (uvast) pow(val_cvt_uvast(lval, &ls), val_cvt_uvast(rval, &rs)); break;
		case BITAND: result.value.as_uvast = val_cvt_uvast(lval, &ls) & val_cvt_uvast(rval, &rs); break;
		case BITOR:  result.value.as_uvast = val_cvt_uvast(lval, &ls) | val_cvt_uvast(rval, &rs); break;
		case BITXOR: result.value.as_uvast = val_cvt_uvast(lval, &ls) ^ val_cvt_uvast(rval, &rs); break;
		case BITLSHFT: result.value.as_uvast = val_cvt_uvast(lval, &ls) << val_cvt_uint(rval, &rs); break;
		case BITRSHFT: result.value.as_uvast = val_cvt_uvast(lval, &ls) >> val_cvt_uint(rval, &rs); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_REAL32:
		switch(op)
		{
		case PLUS:   result.value.as_real32 = val_cvt_real32(lval, &ls) + val_cvt_real32(rval, &rs); break;
		case MINUS:  result.value.as_real32 = val_cvt_real32(lval, &ls) - val_cvt_real32(rval, &rs); break;
		case MULT:   result.value.as_real32 = val_cvt_real32(lval, &ls) * val_cvt_real32(rval, &rs); break;
		case DIV:    result.value.as_real32 = val_cvt_real32(lval, &ls) / val_cvt_real32(rval, &rs); break;
		case EXP:    result.value.as_real32 = (float) pow(val_cvt_real32(lval, &ls), val_cvt_real32(rval, &rs)); break;
		default: ls = rs = 0; break;
		}
		break;
	case AMP_TYPE_REAL64:
		switch(op)
		{
		case PLUS:   result.value.as_real64 = val_cvt_real64(lval, &ls) + val_cvt_real64(rval, &rs); break;
		case MINUS:  result.value.as_real64 = val_cvt_real64(lval, &ls) - val_cvt_real64(rval, &rs); break;
		case MULT:   result.value.as_real64 = val_cvt_real64(lval, &ls) * val_cvt_real64(rval, &rs); break;
		case DIV:    result.value.as_real64 = val_cvt_real64(lval, &ls) / val_cvt_real64(rval, &rs); break;
		case EXP:    result.value.as_real64 = (double) pow(val_cvt_real64(lval, &ls), val_cvt_real64(rval, &rs)); break;
		default: ls = rs = 0; break;
		}
		break;
	default:
		ls = rs = 0;
		break;
	}

	if((ls == 0) || (rs == 0))
	{
        AMP_DEBUG_ERR("adm_agent_binary_num_op","Bad op (%d) or type (%d -> %d).",op, lval.type, rval.type);
        val_init(&result);
	}

	return result;
}


value_t adm_agent_op_plus(Lyst stack)
{
	return adm_agent_binary_num_op(PLUS, stack);
}

/*
 * Good for: INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_minus(Lyst stack)
{
	return adm_agent_binary_num_op(MINUS, stack);
}

/*
 * Good For: INT, UINT, UVAST, VAST, REAL32, REAL64
 */
value_t adm_agent_op_mult(Lyst stack)
{
   return adm_agent_binary_num_op(MULT, stack);
}

/*
 * Good for: INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_div(Lyst stack)
{
   return adm_agent_binary_num_op(DIV, stack);
}

/*
 * Good for INT, UINT, VAST, UVAST
 * Result type is always UINT or UVAST.
 */
value_t adm_agent_op_mod(Lyst stack)
{
   return adm_agent_binary_num_op(MOD, stack);
}

/*
 * Good for INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_op_exp(Lyst stack)
{
   return adm_agent_binary_num_op(EXP, stack);
}

/*
 * Only for int, uint, vast, uvast
 */
value_t adm_agent_op_bitand(Lyst stack)
{
   return adm_agent_binary_num_op(BITAND, stack);
}

/*
 * Only for int, uint, uvast, vast
 */
value_t adm_agent_op_bitor(Lyst stack)
{
   return adm_agent_binary_num_op(BITOR, stack);
}

/*
 * Only for int, uint, vast, uvast
 */
value_t adm_agent_op_bitxor(Lyst stack)
{
   return adm_agent_binary_num_op(BITXOR, stack);
}

/*
 * Only for int, uint, vast, uvast
 */
value_t adm_agent_op_bitnot(Lyst stack)
{
	return adm_agent_unary_num_op(BITNOT, stack);
}


value_t adm_agent_op_logand(Lyst stack)
{
	return adm_agent_binary_log_op(LOGAND, stack);
}


value_t adm_agent_op_logor(Lyst stack)
{
	return adm_agent_binary_log_op(LOGOR, stack);
}


value_t adm_agent_op_lognot(Lyst stack)
{
	return adm_agent_unary_log_op(LOGNOT, stack);
}


value_t adm_agent_op_abs(Lyst stack)
{
	return adm_agent_unary_num_op(ABS, stack);
}


value_t adm_agent_op_lt(Lyst stack)
{
	return adm_agent_binary_log_op(LT, stack);
}

value_t adm_agent_op_gt(Lyst stack)
{
	return adm_agent_binary_log_op(GT, stack);
}


value_t adm_agent_op_lte(Lyst stack)
{
	return adm_agent_binary_log_op(LTE, stack);
}

value_t adm_agent_op_gte(Lyst stack)
{
	return adm_agent_binary_log_op(GTE, stack);
}


value_t adm_agent_op_neq(Lyst stack)
{
	return adm_agent_unary_log_op(NEQ, stack);
}

value_t adm_agent_op_neg(Lyst stack)
{
	return adm_agent_unary_num_op(NEG, stack);
}


value_t adm_agent_op_eq(Lyst stack)
{
	return adm_agent_binary_log_op(EQ, stack);
}

value_t adm_agent_op_lshft(Lyst stack)
{
	return adm_agent_binary_num_op(BITLSHFT, stack);
}

value_t adm_agent_op_rshft(Lyst stack)
{
	return adm_agent_binary_num_op(BITRSHFT, stack);
}


/******************************************************************************
 *
 * \par Function Name: adm_agent_op_stor
 *
 * \par Implements the storage operator: STOR(VAR, VAL) which assigns
 *      VAL to a VAR.
 *
 * \param[in]  stack  The operand stack
 *
 * \return The value of the operation. An invalid value_t indicates failure.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *            E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  08/13/16  E. Birrane     Cleanup. Error checking. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

value_t adm_agent_op_stor(Lyst stack)
{
	value_t result;
	value_t lval, rval;
    var_t *var_def = NULL;

	val_init(&result);

	/* Step 1 - Prep for the operation. */
    if(adm_agent_op_prep(2, stack, &lval, &rval) == 0)
    {
    	AMP_DEBUG_ERR("adm_agent_op_stor","Bad parms.", NULL);
    	return result;
    }

    /* Step 2 - Check lval type. It needs to be a VAR MID. */
    mid_t *mid = (mid_t *) lval.value.as_ptr;

    if((mid_sanity_check(mid) == 0) ||
       (MID_GET_FLAG_ID(mid->flags) != MID_COMPUTED))
    {
    	AMP_DEBUG_ERR("adm_agent_op_stor","lval not computed mid.", NULL);
    	return result;
    }

    /* Step 3 - Cannot store over top of an ADM-controlled VAR. */
 	if((var_def = var_find_by_id(gAdmComputed, NULL, mid)) != NULL)
	{
 		AMP_DEBUG_ERR("adm_agent_op_stor", "Cannot overwrite ADM-defined VAR.", NULL);
	}

 	/* Step 4 - Find the VAR definitions to update. */
	if((var_def = var_find_by_id(gAgentVDB.vars, &(gAgentVDB.var_mutex), mid)) != NULL)
    {
		value_t tmp = val_copy(rval);

		// Can we convert the rval to the lval type?
		if(val_cvt_type(&tmp, var_def->value.type) != 1)
		{
			AMP_DEBUG_ERR("adm_agent_op_stor", "Cannot convert from type %d to %d.", tmp.type, var_def->value.type);
			val_release(&tmp, 0);
			return result;
		}

		// Free current VAR value.
		val_release(&(var_def->value), 0);

		// Can shallow copy this since tmp is a deep copy of the rval.
		var_def->value = tmp;

		// Make another deep copy to return on the stack.
		result = val_copy(tmp);
    }
	else
	{
		AMP_DEBUG_ERR("adm_agent_op_stor", "Cannot find lval VAR.", NULL);
	}
    return result;
}

