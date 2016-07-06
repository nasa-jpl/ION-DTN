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
 ** File Name: adm_agent.h
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

#include "shared/adm/adm.h"

#include "shared/primitives/value.h"
#include "adm_agent_impl.h"
#include "shared/primitives/report.h"
#include "rda.h"
#include "shared/primitives/ctrl.h"
#include "agent_db.h"
#include "instr.h"

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

value_t agent_md_name(tdc_t params)
{
	return val_from_string("DTNMP ADM");
}

value_t agent_md_ver(tdc_t params)
{
	return val_from_string("v0.2");
}

/* Retrieval Functions. */

value_t agent_get_num_rpt(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_rpt_defs;

	return result;
}

value_t agent_get_sent_rpt(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_sent_rpts;

	return result;
}

value_t agent_get_num_trl(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_time_rules;

	return result;
}

value_t agent_get_run_trl(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_time_rules_run;

	return result;
}

value_t agent_get_num_srl(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_prod_rules;

	return result;
}

value_t agent_get_run_srl(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_prod_rules_run;

	return result;
}

value_t agent_get_num_lit(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_consts;

	return result;
}

value_t agent_get_num_cust(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_data_defs;

	return result;
}

value_t agent_get_num_mac(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_macros;

	return result;
}

value_t agent_get_run_mac(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_macros_run;

	return result;
}

value_t agent_get_num_ctrl(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_ctrls;

	return result;
}

value_t agent_get_run_ctrl(tdc_t params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_ctrls_run;

	return result;
}

value_t agent_get_curtime(tdc_t params)
{
	value_t result;
	struct timeval cur_time;

	getCurrentTime(&cur_time);

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = (uint32_t) cur_time.tv_sec;

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
/* Return a data collection of strings. */

tdc_t* agent_ctl_adm_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *result = NULL;
	value_t cur_val;
	uint8_t *data = NULL;
	uint32_t data_size = 0;

	/* Step 1: Construct the values. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
    {
    	*status = CTRL_FAILURE;
    	return NULL;
    }

	cur_val.type = DTNMP_TYPE_STRING;
//	cur_val.length = strlen("DTNMP_AGENT");
	cur_val.value.as_ptr = adm_copy_string("DTNMP AGENT", NULL);
	data = val_serialize(cur_val, &data_size, 0);
	val_release(&cur_val, 0);
	tdc_insert(result, cur_val.type, data, data_size);
	SRELEASE(data);
	//tdc_insert(result, DTNMP_TYPE_STRING, (uint8_t*) "DTNMP AGENT", strlen("DTNMP AGENT"));

	cur_val.type = DTNMP_TYPE_STRING;
//	cur_val.length = strlen("BP");
	cur_val.value.as_ptr = adm_copy_string("BP", NULL);
	data = val_serialize(cur_val, &data_size, 0);
	val_release(&cur_val, 0);
	tdc_insert(result, DTNMP_TYPE_STRING, data, data_size);
	SRELEASE(data);

#ifdef _HAVE_LTP_ADM_
	cur_val.type = DTNMP_TYPE_STRING;
//	cur_val.length = strlen("LTP");
	cur_val.value.as_ptr = adm_copy_string("LTP", NULL);
	data = val_serialize(&cur_val, &data_size, 0);
	val_release(&cur_val, 0);
	tdc_insert(result, DTNMP_TYPE_STRING, data, data_size);
	SRELEASE(data);
#endif /* _HAVE_LTP_ADM_ */

#ifdef _HAVE_ION_ADM_
	cur_val.type = DTNMP_TYPE_STRING;
//	cur_val.length = strlen("ION");
	cur_val.value.as_ptr = adm_copy_string("ION", NULL);
	data = val_serialize(&cur_val, &data_size, 0);
	val_release(&cur_val, 0);
	tdc_insert(result, DTNMP_TYPE_STRING, data, data_size);
	SRELEASE(data);
#endif /* _Have_ION_ADM_ */

    *status = CTRL_SUCCESS;
    return result;
}


/*
 * This control defines a new computed data definition for the agent.
 *
 * THe control takes 2 parameters:
 * 1. The MID for the new computed data item.
 * 2. The type of the Computed Data Value
 * 3. The expression associated with the value.
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_cd_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	dtnmp_type_e type = DTNMP_TYPE_UNK;
	expr_t *expr = NULL;
	cd_t *cd = NULL;
	int8_t success = 0;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 2: Make sure this isn't a duplicate. */
	if(agent_vdb_compdata_find(mid) != NULL)
	{
		DTNMP_DEBUG_WARN("agent_ctl_cd_add","CD already defined for this MID. Ignoring.", NULL);
		*status = CTRL_FAILURE;
		mid_release(mid);
		return NULL;
	}

	/* Step 1.1: Verify this is a good MID for a CD. */
	if(MID_GET_FLAG_ID(mid->flags) != MID_COMPUTED)
	{
		DTNMP_DEBUG_ERR("agent_ctl_cd_add","MID flags do not match a CD definition.", NULL);
		*status = CTRL_FAILURE;
		mid_release(mid);
		return NULL;
	}

	/* Step 2: Grab the expression that defines the CD. */
	if((expr = adm_extract_expr(params, 1, &success)) == NULL)
	{
		*status = CTRL_FAILURE;
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the type that defines the CD. */
	type = adm_extract_sdnv(params, 2, &success);
	if(success != 1)
	{
		*status = CTRL_FAILURE;
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}


	/* Step 3: Add the new definition.
	 * NOTE: cd_create shallow-copies information. Do not release
	 * mid or val!
	 */
	value_t val;
	val_init(&val);
	if((cd = cd_create(mid, type, expr, val)) == NULL)
	{
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}

	expr_release(expr);

	agent_db_compdata_persist(cd);
	ADD_COMPDATA(cd);

	gAgentInstr.num_data_defs = agent_db_count(gAgentVDB.compdata, &(gAgentVDB.compdata_mutex)) +
			                    agent_db_count(gAdmData, NULL);

	*status = CTRL_SUCCESS;

	return NULL;
}

/*
 * This control defines a new computed data definition for the agent.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of CDs to remove.
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_cd_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		if(mid != NULL)
		{
			agent_db_compdata_forget(mid);
			agent_vdb_compdata_forget(mid);
		}
	}

	midcol_destroy(&mc);

	gAgentInstr.num_data_defs = agent_db_count(gAgentVDB.compdata, &(gAgentVDB.compdata_mutex)) +
			                    agent_db_count(gAdmData, NULL);


	*status = CTRL_SUCCESS;
	return NULL;
}


/*
 * This control defines a new computed data definition for the agent.
 *
 * THe control takes 0 parameters:
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_cd_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 0: Sanity Check. */

	/* Step 1: Build an MC of known compdata. */
	if((mc = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_cd_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	lockResource(&(gAgentVDB.compdata_mutex));
	for(elt = lyst_first(gAgentVDB.compdata); elt; elt = lyst_next(elt))
	{
		cd_t *cur = (cd_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.compdata_mutex));

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		DTNMP_DEBUG_ERR("agent_ctl_cd_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */

	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	if(tdc_insert(retval, DTNMP_TYPE_MC, data, data_len) != DTNMP_TYPE_MC)
	{
		tdc_destroy(&retval);
		SRELEASE(data);
		DTNMP_DEBUG_ERR("agent_ctl_cd_lst","Can't add MC to TDC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}


/*
 * This control defines a new computed data definition for the agent.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of CDs to describe
 *
 * The control returns a list of each compdata def.
 */

tdc_t *agent_ctl_cd_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	int8_t success = 0;


	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		DTNMP_DEBUG_ERR("agent_ctl_cd_dsc","Can't make tdc.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		cd_t *cur = agent_vdb_compdata_find(mid);

		if((data = cd_serialize(cur, &data_len)) != NULL)
		{
			tdc_insert(retval, DTNMP_TYPE_CD, data, data_len);
			SRELEASE(data);
		}
		else
		{
			DTNMP_DEBUG_ERR("agent_ctl_cd_desc","Cannot serialize CD.", NULL);
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}



/* REPORT-RELATED CONTROLS. */

/*
 * This control defines a new report definition for the agent.
 *
 * THe control takes 2 parameters:
 * 1. The MID of the new report.
 * 2. The MC describing the report.
 *
 * The control returns no DC.
 */

tdc_t *agent_ctl_rpt_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	int8_t success = 0;
	def_gen_t *result = NULL;
	mid_t *mid = NULL;
	Lyst expr = NULL;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		return NULL;
	}

	if(agent_vdb_report_find(mid) != NULL)
	{
		DTNMP_DEBUG_WARN("agent_ctl_rpt_add","Report for this MID already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 2: Grab the expression capturing the definition. */
	if((expr = adm_extract_mc(params, 1, &success)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Create the new definition. This is a shallow copy sp
	 * don't release the mis and expr.  */
	if((result = def_create_gen(mid, DTNMP_TYPE_RPT, expr)) == NULL)
	{
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

	agent_db_report_persist(result);
	ADD_REPORT(result);

	gAgentInstr.num_rpt_defs = agent_db_count(gAgentVDB.reports, &(gAgentVDB.reports_mutex)) +
			                    agent_db_count(gAdmRpts, NULL);

	*status = CTRL_SUCCESS;

	return NULL;
}


/*
 * This control deletes a report definition for the agent.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of rpts to remove.
 *
 * The control returns success/failure and does not generate a report.
 */


tdc_t *agent_ctl_rpt_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{

	Lyst mc = NULL;
	LystElt elt = NULL;

	int8_t success = 0;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params,0,&success)) == NULL)
	{
		*status = CTRL_FAILURE;
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

	gAgentInstr.num_rpt_defs = agent_db_count(gAgentVDB.reports, &(gAgentVDB.reports_mutex)) +
			                    agent_db_count(gAdmRpts, NULL);

	*status = CTRL_SUCCESS;
	return NULL;
}


/*
 * This control lists all reports known by the agent.
 *
 * THe control takes 0 parameters:
 *
 * The control returns a MC of known report MIDs.
 */

tdc_t *agent_ctl_rpt_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_rpt_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	lockResource(&(gAgentVDB.reports_mutex));
	for(elt = lyst_first(gAgentVDB.reports); elt; elt = lyst_next(elt))
	{
		def_gen_t *cur = (def_gen_t*) lyst_data(elt);
		lyst_insert_last(mc, mid_copy(cur->id));
	}
	unlockResource(&(gAgentVDB.reports_mutex));

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		DTNMP_DEBUG_ERR("agent_ctl_rpt_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, DTNMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}

/*
 * This control returns a description of each selected report MID.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of reports to describe
 *
 * The control returns a list of each report def.
 */

tdc_t *agent_ctl_rpt_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
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
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		midcol_destroy(&mc);
		DTNMP_DEBUG_ERR("agent_ctl_rpt_dsc","Can't make lyst.", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		def_gen_t *cur = agent_vdb_report_find(mid);

		data = def_serialize_gen(cur, &data_len);
		tdc_insert(retval, DTNMP_TYPE_RPT, data, data_len);
		SRELEASE(data);
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
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
 *  06/21/15  E. Birrane     Updated to new control return code structure.
 *****************************************************************************/
tdc_t *agent_ctl_rpt_gen(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	rpt_t *report = NULL;
	eid_t recipient;
	Lyst mc = NULL;
	Lyst rx = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;
	bzero(&recipient, sizeof(eid_t));

	/* Step 1: Get the MC from the params. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Get the list of mgrs to receive this report. */
	if((rx = adm_extract_dc(params,1, &success)) == NULL)
	{
		/* Step 2.1: If we don'thave, or can't get the rx list send report back to requester. */
		memcpy(&recipient, def_mgr, sizeof(eid_t));
	}
	else
	{
		blob_t *entry = NULL;

		if((entry = lyst_data(lyst_first(rx))) == NULL)
		{
			/* Step 2.1: If we don'thave, or can't get the rx list send report back to requester. */
			memcpy(&recipient, def_mgr, sizeof(eid_t));
		}
		else if(entry->length == 0)
		{
			/* Step 2.1: If we don'thave, or can't get the rx list send report back to requester. */
			memcpy(&recipient, def_mgr, sizeof(eid_t));
		}
		else if(entry->length < sizeof(recipient))
		{
			memcpy(&recipient, entry->value, entry->length);
		}
		else
		{
			DTNMP_DEBUG_ERR("agent_ctl_rpt_gen", "Rx length of %d > %d.", entry->length, sizeof(eid_t));
			dc_destroy(&rx);
			midcol_destroy(&mc);
			return NULL;
		}

		dc_destroy(&rx);
	}

	/* Step 2: Build the report.
	 *
	 * This function will grab an existing to-be-sent report or
	 * create a new report and add it to the "built-reports" section.
	 * Either way, it will be included in the next set of code to send
	 * out reports built in this time slice.
	 */
	if((report = rda_get_report(recipient)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_rpt_gen", "Cannot allocate report.", NULL);
		midcol_destroy(&mc);
		return NULL;
	}


	/* Step 3: For each MID in the report definition, construct the
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
			DTNMP_DEBUG_ERR("agent_ctl_rpt_gen","Can't build report for %s", midstr);
			SRELEASE(midstr);
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;

  return NULL;
}


/*
 * This control defines a new macro definition for the agent.
 *
 * The control takes 2 parameters:
 * 1. The MID of the new macro.
 * 2. The MC describing the macro.
 *
 * The control returns no DC.
 */

tdc_t *agent_ctl_mac_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	Lyst expr = NULL;
	def_gen_t *result = NULL;
	int8_t success = 0;
	char *name = NULL;

	*status = CTRL_FAILURE;

	if((name = adm_extract_string(params, 0, &success)) == NULL)
	{
		return NULL;
	}

	/* We ignore the name. */

	SRELEASE(name);

	/* Step 1: Grab the MID defining the new computed definition. */

	if((mid = adm_extract_mid(params, 1, &success)) == NULL)
	{
		return NULL;
	}

	if(agent_vdb_macro_find(mid) != NULL)
	{
		DTNMP_DEBUG_WARN("agent_ctl_mac_add","Macro for this MID already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}


	/* Step 2: Grab the expression capturing the definition. */
	if((expr = adm_extract_mc(params, 2, &success)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Add the new definition.
	 * This is a shllow-copy so don't release mid and expr.
	 */

	if((result = def_create_gen(mid, DTNMP_TYPE_MACRO, expr)) == NULL)
	{
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

	agent_db_macro_persist(result);
	ADD_MACRO(result);

	gAgentInstr.num_macros = agent_db_count(gAgentVDB.macros, &(gAgentVDB.macros_mutex)) +
			                    agent_db_count(gAdmMacros, NULL);

	*status = CTRL_SUCCESS;

	return NULL;
}


/*
 * This control deletes a macro definition for the agent.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of macros to remove.
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_mac_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
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

	gAgentInstr.num_macros = agent_db_count(gAgentVDB.macros, &(gAgentVDB.macros_mutex)) +
			                    agent_db_count(gAdmMacros, NULL);

	*status = CTRL_SUCCESS;
	return NULL;
}


/*
 * This control lists all macros known by the agent.
 *
 * THe control takes 0 parameters:
 *
 * The control returns a MC of known macro MIDs.
 */

tdc_t *agent_ctl_mac_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_mac_lst","Can't make MC.", NULL);
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
		DTNMP_DEBUG_ERR("agent_ctl_mac_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, DTNMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}

/*
 * This control returns a description of each selected macro MID.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of macros to describe
 *
 * The control returns a list of each macro.
 */

tdc_t *agent_ctl_mac_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
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
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_mac_dsc","Can't make lyst.", NULL);

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
		tdc_insert(retval, DTNMP_TYPE_RPT, data, data_len);
		SRELEASE(data);
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}


/*
 * This control adds a time-based rule to the agent.
 *
 * THe control takes 5 parameters:
 * 1. The MID of the new control.
 * 2. The time offset for starting the rule evaluation
 * 3. The relative time period for the rule.
 * 4. The number of times to execute the rule.
 * 5. The macro to run when applying the rule.
 *
 * The control returns no data.
 */

tdc_t *agent_ctl_trl_add(eid_t *def_mgr, tdc_t params, int8_t *status)
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
		return NULL;
	}

	if(agent_vdb_trl_find(mid) != NULL)
	{
		DTNMP_DEBUG_WARN("agent_ctl_trl_add","TRL for this MID already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}


	/* Step 2: Grab the offset for this rule. */
	offset = adm_extract_sdnv(params, 1, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the period for this rule. */
	period = adm_extract_sdnv(params, 2, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Grab the count for this rule. */
	count = adm_extract_sdnv(params, 3, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 5: Grab the macro action for this rule. */
	if((action =adm_extract_mc(params, 4, &success)) == NULL)
	{
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

	gAgentInstr.num_time_rules = agent_db_count(gAgentVDB.trls, &(gAgentVDB.trls_mutex));


	mid_release(mid);
	midcol_destroy(&action);
	return NULL;
}


/*
 * This control deletes a rule definition for the agent.
 *
 * THe control takes 1 parameter:
 * 1. The MC of rules to remove.
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_trl_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 0, &success)) == NULL)
	{
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

	gAgentInstr.num_time_rules = agent_db_count(gAgentVDB.trls, &(gAgentVDB.trls_mutex));

	*status = CTRL_SUCCESS;
	return NULL;
}



/*
 * This control lists all TRLs known by the agent.
 *
 * THe control takes 0 parameters:
 *
 * The control returns a MC of known TRL MIDs.
 */

tdc_t *agent_ctl_trl_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;


	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_trl_lst","Can't make MC.", NULL);
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
		DTNMP_DEBUG_ERR("agent_ctl_trl_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, DTNMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}


/*
 * This control returns a description of each selected TRL.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of TRLs to describe
 *
 * The control returns a MC comprising the MID of each TRL.
 */

tdc_t *agent_ctl_trl_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
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
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_trl_dsc","Can't make lyst.", NULL);

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
			tdc_insert(retval, DTNMP_TYPE_TRL, data, data_len);
			SRELEASE(data);
		}
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}


/*
 * This control adds a state-based rule to the agent.
 *
 * THe control takes 5 parameters:
 * 1. The MID of the new control.
 * 2. The time offset for starting the rule evaluation
 * 3. The expression triggering the rule.
 * 4. The number of times to execute the rule.
 * 5. The macro to run when applying the rule.
 *
 * The control returns no data.
 */


tdc_t *agent_ctl_srl_add(eid_t *def_mgr, tdc_t params, int8_t *status)
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
		return NULL;
	}

	if(agent_vdb_srl_find(mid) != NULL)
	{
		DTNMP_DEBUG_WARN("agent_ctl_srl_add","SRL for this MID already defined. Ignoring.", NULL);
		mid_release(mid);
		return NULL;
	}

	/* Step 2: Grab the offset for this rule. */
	offset = adm_extract_sdnv(params, 1, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the expression for this rule. */
	if((expr = adm_extract_expr(params, 2, &success)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Grab the count for this rule. */
	count = adm_extract_sdnv(params, 3, &success);
	if(success == 0)
	{
		mid_release(mid);
		expr_release(expr);
		return NULL;
	}

	/* Step 5: Grab the macro action for this rule. */
	if((action =adm_extract_mc(params, 4, &success)) == NULL)
	{
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

	gAgentInstr.num_prod_rules = agent_db_count(gAgentVDB.srls, &(gAgentVDB.srls_mutex));

	return NULL;
}



/*
 * This control deletes an SRL definition for the agent.
 *
 * THe control takes 1 parameter:
 * 1. The MC of SRLs to remove.
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_srl_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params,0,&success)) == NULL)
	{
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

	gAgentInstr.num_prod_rules = agent_db_count(gAgentVDB.srls, &(gAgentVDB.srls_mutex));

	*status = CTRL_SUCCESS;
	return NULL;
}


/*
 * This control lists all SRLs known by the agent.
 *
 * THe control takes 0 parameters:
 *
 * The control returns a MC of known SRL MIDs.
 */

tdc_t *agent_ctl_srl_lst(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t *retval = NULL;
	Lyst mc = NULL;
	LystElt elt;
	uint8_t *data = NULL;
	uint32_t data_len = 0;

	/* Step 1: Build an MC of known reports. */
	if((mc = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_srl_lst","Can't make MC.", NULL);
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
		DTNMP_DEBUG_ERR("agent_ctl_srl_lst","Can't make MC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 3: Populate the retval. */
	data = midcol_serialize(mc, &data_len);
	midcol_destroy(&mc);

	tdc_insert(retval, DTNMP_TYPE_MC, data, data_len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}


/*
 * This control returns a description of each selected SRL.
 *
 * THe control takes 1 parameter:
 * 1. THe MC of SRLs to describe
 *
 * The control returns a list of each SRL.
 */


tdc_t *agent_ctl_srl_dsc(eid_t *def_mgr, tdc_t params, int8_t *status)
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
		return NULL;
	}

	/* Step 2: Allocate the retval holding the descriptions. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_srl_dsc","Can't make lyst.", NULL);

		midcol_destroy(&mc);
		return NULL;
	}

	/* Step 2: For each MID in the MC... */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		mid_t *mid = lyst_data(elt);

		/* do not release cur, it is a direct ptr. */
		srl_t *srl = agent_vdb_srl_find(mid);

		data = srl_serialize(srl, &data_len);
		tdc_insert(retval, DTNMP_TYPE_SRL, data, data_len);
		SRELEASE(data);
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
}


/*
 * This control evaluates an EXPR and stores it in a CD.
 *
 * THe control takes 2 parameters:
 * Parm 1: CD to hold the expr value.
 * Parm 2: EXPR to evaluate.
 *
 * The control has no return report.
 */

tdc_t* agent_ctl_stor(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	mid_t *mid = NULL;
	cd_t *cd = NULL;
	expr_t *expr = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the CD MID to hold the result.
	 * We don't need to sanity check the MID or ensure it is a CD
	 * here because we will use it to look up the CD later and if the
	 * MID is bad, the lookup will fail.
	 * */
	if((mid = adm_extract_mid(params, 0, &success)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_ctl_stor","Can't get destination CD MID.", NULL);
		return NULL;
	}

	/* Step 2: Grab the EXPR to evaluate. */
	if((expr = adm_extract_expr(params, 1, &success)) == NULL)
	{
		mid_release(mid);
		DTNMP_DEBUG_ERR("agent_ctl_stor","Can't get expression.", NULL);
		return NULL;
	}

	/* Step 3: Make sure this isn't an ADM-defined MID. We can't re-assign
	 * them.
	 */
 	if((cd = cd_find_by_id(gAdmComputed, NULL, mid)) != NULL)
	{
 		DTNMP_DEBUG_ERR("agent_ctl_stor", "Cannot overwrite ADM-defined CD.", NULL);
	}

 	/* Step 4 - Find the CD definitions to update. */
 	else if((cd = cd_find_by_id(gAgentVDB.compdata, &(gAgentVDB.compdata_mutex), mid)) != NULL)
    {
		value_t tmp = expr_eval(expr);

		/* Step 4.1: Can the CD hold the expression result? */
		if(val_cvt_type(&tmp, cd->value.type) == 1)
		{
			// Free current CD value.
			val_release(&(cd->value), 0);

			// Can shallow copy this since tmp is a deep copy.
			cd->value = tmp;

			*status = CTRL_SUCCESS;
		}
		else
		{
			DTNMP_DEBUG_ERR("agent_ctl_stor", "Cannot convert from type %d to %d.", tmp.type, cd->value.type);
			val_release(&tmp, 0);
		}
    }
	else
	{
		DTNMP_DEBUG_ERR("agent_ctl_stor", "Cannot find CD.", NULL);
	}

 	mid_release(mid);
 	expr_release(expr);

 	return NULL;
}


tdc_t* agent_ctrl_reset_cnt(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	agent_instr_clear();
	*status = CTRL_SUCCESS;
	return NULL;
}



/*
 * OP FUnctions
 */

// Success = 1. Failure = 0
uint8_t adm_agent_op_prep(int num, Lyst stack, value_t *lval, value_t *rval)
{
    LystElt elt = 0;
    value_t *tmp = NULL;

	if((stack == NULL) || (num < 1) || (num > 2) ||
		(lval == NULL) ||
		((num == 2) && (rval == NULL)) || (lyst_length(stack) != num))
	{
        DTNMP_DEBUG_ERR("adm_agent_op_prep","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_prep","-> Unknown", NULL);
        return 0;
	}

	/* Step 2: Grab the L-value and R-value. */
	elt = lyst_first(stack);
	if((tmp = (value_t*) lyst_data(elt)) == NULL)
	{
        DTNMP_DEBUG_ERR("adm_agent_op_plus","NULL lval.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
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
			DTNMP_DEBUG_ERR("adm_agent_op_plus","NULL rval.",NULL);
			DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
			return 0;
		}
		else
		{
			*rval = *tmp;
		}
	}

	return 1;
}

/*
 * Good for: INT, UINT, VAST, UVAST, REAL32, REAL64
 */
value_t adm_agent_unary_num_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t val;
	uint32_t cvt_type = DTNMP_TYPE_UNK;
	uint8_t s = 0;

	/* Step 1 - Prep for the operation. */
	val_init(&result);

	if(adm_agent_op_prep(1, stack, &val, NULL) == 0)
	{
        DTNMP_DEBUG_ERR("adm_agent_unary_num_op","Can't get lval.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_unary_num_op","-> Unknown", NULL);
        return result;
	}

	cvt_type = result.type;

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_unary_num_op","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_unary_num_op","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */

    switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		switch(op)
		{
		case NEG:    result.value.as_int = 0 - val_cvt_int(val, &s); break;
		case BITNOT: result.value.as_int = ~val_cvt_int(val, &s); break;
		case ABS:    result.value.as_int = abs(val_cvt_int(val, &s)); break;
		default: s = 0; break;
		}
//		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		switch(op)
		{
		case BITNOT: result.value.as_uint = ~val_cvt_uint(val, &s); break;
		case ABS:    result.value.as_uint = val_cvt_uint(val, &s); break;
		default: s = 0; break;
		}
//		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		switch(op)
		{
		case NEG:    result.value.as_vast = 0 - val_cvt_vast(val, &s); break;
		case BITNOT: result.value.as_vast = ~val_cvt_vast(val, &s); break;
		case ABS:    result.value.as_vast = abs(val_cvt_vast(val, &s)); break;
		default: s = 0; break;
		}
//		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		switch(op)
		{
		case ABS:    result.value.as_uvast = val_cvt_uvast(val, &s); break;
		case BITNOT: result.value.as_uvast = ~val_cvt_uvast(val, &s); break;
		default: s = 0; break;
		}

//		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		switch(op)
		{
		case NEG:    result.value.as_real32 = 0 - val_cvt_real32(val, &s); break;
		case ABS:    result.value.as_real32 = fabs(val_cvt_real32(val, &s)); break;
		default: s = 0; break;
		}
//		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		switch(op)
		{
		case NEG:    result.value.as_real64 = 0 - val_cvt_real64(val, &s); break;
		case ABS:    result.value.as_real64 = fabs(val_cvt_real64(val, &s)); break;
		default: s = 0; break;
		}
//		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
		break;
	default:
		s = 0;
		break;
	}

	if(s == 0)
	{
        DTNMP_DEBUG_ERR("adm_agent_unary_num_op","Bad type combo. Can't get %d from %d.",cvt_type, val.type);
        DTNMP_DEBUG_EXIT("adm_agent_unary_num_op","-> Unknown", NULL);
        memset(&result, 0, sizeof(value_t));
	}

	return result;
}

value_t adm_agent_unary_log_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t val;
	uint32_t cvt_type = DTNMP_TYPE_UNK;
	uint8_t s = 0;

	/* Step 1 - Prep for the operation. */
    val_init(&result);
    if(adm_agent_op_prep(1, stack, &val, NULL) == 0)
	{
        DTNMP_DEBUG_ERR("adm_agent_unary_log_op","Can't get lval.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_unary_log_op","-> Unknown", NULL);
        return result;
	}


	cvt_type = result.type;

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_unary_log_op","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_unary_log_op","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */

    switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		switch(op)
		{
		case LOGNOT: result.value.as_uint = val_cvt_int(val, &s) ? 0 : 1; break;
		default: s = 0; break;
		}
//		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		switch(op)
		{
		case LOGNOT: result.value.as_uint = val_cvt_uint(val, &s) ? 0 : 1; break;
		default: s = 0; break;
		}
//		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		switch(op)
		{
		case LOGNOT: result.value.as_uint = val_cvt_vast(val, &s) ? 0 : 1; break;
		default: s = 0; break;
		}
//		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		switch(op)
		{
		case LOGNOT: result.value.as_uint = val_cvt_uvast(val, &s) ? 0 : 1; break;
		default: s = 0; break;
		}

//		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		switch(op)
		{
		default: s = 0; break;
		}
//		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		switch(op)
		{
		default: s = 0; break;
		}
//		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
		break;
	default:
		s = 0;
		break;
	}

	if(s == 0)
	{
        DTNMP_DEBUG_ERR("adm_agent_unary_log_op","Bad type combo. Can't get %d from %d.",cvt_type, val.type);
        DTNMP_DEBUG_EXIT("adm_agent_unary_log_op","-> Unknown", NULL);
        memset(&result, 0, sizeof(value_t));
	}

	return result;
}

value_t adm_agent_binary_log_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t lval, rval;
	uint32_t cvt_type = DTNMP_TYPE_UNK;
	uint8_t ls = 0;
	uint8_t rs = 0;

	/* Step 1 - Prep for the operation. */
	val_init(&result);
	if(adm_agent_op_prep(2, stack, &lval, &rval) == 0)
	{
        DTNMP_DEBUG_ERR("adm_agent_binary_log_op","Can't get lval.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_binary_log_op","-> Unknown", NULL);
        return result;
	}

	cvt_type = val_get_result_type(lval.type, rval.type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_binary_log_op","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_binary_log_op","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
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
//		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
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
//		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
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
//		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
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

//		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
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
//		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
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
//		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
		break;
	default:
		ls = rs = 0;
		break;
	}

	if((ls == 0) || (rs == 0))
	{
        DTNMP_DEBUG_ERR("adm_agent_binary_log_op","Bad type combo. Can't get %d from %d OP %d.",cvt_type, lval.type, rval.type);
        DTNMP_DEBUG_EXIT("adm_agent_binary_log_op","-> Unknown", NULL);
        memset(&result, 0, sizeof(value_t));
	}

	return result;

}

value_t adm_agent_binary_num_op(agent_adm_op_e op, Lyst stack)
{
	value_t result;
	value_t lval, rval;
	uint32_t cvt_type = DTNMP_TYPE_UNK;
	uint8_t ls = 0;
	uint8_t rs = 0;

	/* Step 1 - Prep for the operation. */
    val_init(&result);
    if(adm_agent_op_prep(2, stack, &lval, &rval) == 0)
	{
        DTNMP_DEBUG_ERR("adm_agent_binary_num_op","Can't get lval.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_binary_num_op","-> Unknown", NULL);
        return result;
	}

	cvt_type = val_get_result_type(lval.type, rval.type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_plus","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
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
//		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
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
//		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
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
//		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
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

//		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		switch(op)
		{
		case PLUS:   result.value.as_real32 = val_cvt_real32(lval, &ls) + val_cvt_real32(rval, &rs); break;
		case MINUS:  result.value.as_real32 = val_cvt_real32(lval, &ls) - val_cvt_real32(rval, &rs); break;
		case MULT:   result.value.as_real32 = val_cvt_real32(lval, &ls) * val_cvt_real32(rval, &rs); break;
		case DIV:    result.value.as_real32 = val_cvt_real32(lval, &ls) / val_cvt_real32(rval, &rs); break;
		case EXP:    result.value.as_real32 = (float) pow(val_cvt_real32(lval, &ls), val_cvt_real32(rval, &rs)); break;
		default: ls = rs = 0; break;
		}
//		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		switch(op)
		{
		case PLUS:   result.value.as_real64 = val_cvt_real64(lval, &ls) + val_cvt_real64(rval, &rs); break;
		case MINUS:  result.value.as_real64 = val_cvt_real64(lval, &ls) - val_cvt_real64(rval, &rs); break;
		case MULT:   result.value.as_real64 = val_cvt_real64(lval, &ls) * val_cvt_real64(rval, &rs); break;
		case DIV:    result.value.as_real64 = val_cvt_real64(lval, &ls) / val_cvt_real64(rval, &rs); break;
		case EXP:    result.value.as_real64 = (double) pow(val_cvt_real64(lval, &ls), val_cvt_real64(rval, &rs)); break;
		default: ls = rs = 0; break;
		}
//		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
		break;
	default:
		ls = rs = 0;
		break;
	}

	if((ls == 0) || (rs == 0))
	{
        DTNMP_DEBUG_ERR("adm_agent_op_plus","Bad type combo. Can'get get %d from %d OP %d.",cvt_type, lval.type, rval.type);
        DTNMP_DEBUG_EXIT("adm_agent_op_plus","-> Unknown", NULL);
        memset(&result, 0, sizeof(value_t));
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


value_t adm_agent_op_logxor(Lyst stack)
{
	value_t result;

	//result.length = 0;
	result.type = DTNMP_TYPE_UNK;
	result.value.as_ptr = NULL;

	return result;
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

value_t adm_agent_op_stor(Lyst stack)
{

	value_t result;
	value_t lval, rval;
    cd_t *cd_def = NULL;


	val_init(&result);
	/* Step 1 - Prep for the operation. */
    if(adm_agent_op_prep(2, stack, &lval, &rval) == 0)
    {
    	return result;
    }

    /* Step 2 - Check lval type. It needs to be a CD MID. */
    mid_t *mid = (mid_t *) lval.value.as_ptr;

    if((mid_sanity_check(mid) == 0) ||
       (MID_GET_FLAG_ID(mid->flags) != MID_COMPUTED))
    {
    	DTNMP_DEBUG_ERR("adm_agent_op_stor","lval not computed mid.", NULL);
    	return result;
    }

    /* Step 3 - Cannot store over top of an ADM-controlled CD. */
 	if((cd_def = cd_find_by_id(gAdmComputed, NULL, mid)) != NULL)
	{
 		DTNMP_DEBUG_ERR("adm_agent_op_stor", "Cannot overwrite ADM-defined CD.", NULL);
	}

 	/* Step 4 - Find the CD definitions to update. */
	if((cd_def = cd_find_by_id(gAgentVDB.compdata, &(gAgentVDB.compdata_mutex), mid)) != NULL)
    {
		value_t tmp = val_copy(rval);

		// Can we convert the rval to the lval type?
		if(val_cvt_type(&tmp, cd_def->value.type) != 1)
		{
			DTNMP_DEBUG_ERR("adm_agent_op_stor", "Cannot convert from type %d to %d.", tmp.type, cd_def->value.type);
			val_release(&tmp, 0);
			return result;
		}

		// Free current CD value.
		val_release(&(cd_def->value), 0);

		// Can shallow copy this since tmp is a deep copy of the rval.
		cd_def->value = tmp;

		// Make another deep copy to return on the stack.
		result = val_copy(tmp);
    }
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_op_stor", "Cannot find lval CD.", NULL);
	}
    return result;
}

