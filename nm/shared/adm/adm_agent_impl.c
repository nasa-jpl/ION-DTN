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
#include "shared/primitives/ctrl.h"
#include "agent_db.h"
#include "shared/primitives/instr.h"

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

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_rpt_defs;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_sent_rpt(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_sent_rpts;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_trl(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_time_rules;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_run_trl(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_time_rules_run;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_srl(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_prod_rules;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_run_srl(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_prod_rules_run;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_lit(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_consts;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_cust(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_data_defs;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_mac(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_macros;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_run_mac(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_macros_run;
	result.length = sizeof(uint32_t);

	return result;
}

value_t agent_get_num_ctrl(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
	result.value.as_uint = gAgentInstr.num_ctrls;
	result.length = sizeof(uint32_t);
	return result;
}

value_t agent_get_run_ctrl(Lyst params)
{
	value_t result;

	result.type = DTNMP_TYPE_UINT;
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
/* Return a data collection of strings. */

tdc_t* agent_ctl_adm_lst(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	tdc_t *result = NULL;
	value_t cur_val;
	uint8_t *data = NULL;
	uint32_t data_size = 0;
	Lyst dc = NULL;

	/* Step 1: Construct the values. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
    {
    	*status = CTRL_FAILURE;
    	return NULL;
    }

	cur_val.type = DTNMP_TYPE_STRING;
	cur_val.length = strlen("DTNMP_AGENT");
	cur_val.value.as_ptr = (uint8_t*) "DTNMP AGENT";
	data = val_serialize(&cur_val, &data_size, 0);
	tdc_insert(result, cur_val.type, data, data_size);
	MRELEASE(data);
	//tdc_insert(result, DTNMP_TYPE_STRING, (uint8_t*) "DTNMP AGENT", strlen("DTNMP AGENT"));

	cur_val.type = DTNMP_TYPE_STRING;
	cur_val.length = strlen("BP");
	cur_val.value.as_ptr = (uint8_t*) "BP";
	data = val_serialize(&cur_val, &data_size, 0);
	tdc_insert(result, DTNMP_TYPE_STRING, data, data_size);
	MRELEASE(data);

#ifdef _HAVE_LTP_ADM_
	cur_val.type = DTNMP_TYPE_STRING;
	cur_val.length = strlen("LTP");
	cur_val.value.as_ptr = (uint8_t*) "LTP";
	data = val_serialize(&cur_val, &data_size);
	tdc_insert(result, DTNMP_TYPE_STRING, data, data_size);
	MRELEASE(data);
#endif /* _HAVE_LTP_ADM_ */

#ifdef _HAVE_ION_ADM_
	cur_val.type = DTNMP_TYPE_STRING;
	cur_val.length = strlen("ION");
	cur_val.value.as_ptr = (uint8_t*) "ION";
	data = val_serialize(&cur_val, &data_size);
	tdc_insert(result, DTNMP_TYPE_STRING, data, data_size);
	MRELEASE(data);
#endif /* _Have_ION_ADM_ */

    *status = CTRL_SUCCESS;
    return result;
}


/*
 * This control defines a new computed data definition for the agent.
 *
 * THe control takes 3 parameters:
 * 1. The MID for the new computed data item.
 * 2. The EXPR that describes then new computed data item.
 * 3. The TYPE of the computed data item.
 *
 * The control returns success/failure and does not generate a report.
 */

tdc_t *agent_ctl_cd_add(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	mid_t *mid = NULL;
	Lyst expr = NULL;
	uvast def_type = 0;
	def_gen_t *cd;
	uint8_t success = 0;

	// \todo: CHange OID params to datalist type.
	// \todo: Consider passing in full mid instead of just mid parms?

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 1, &success)) == NULL)
	{
		*status = CTRL_FAILURE;
		return NULL;
	}


	/* Step 1.1: Verify this is a good MID for a CD. */
	if(MID_GET_FLAG_TYPECAT(mid->flags) != MID_COMPUTED)
	{
		DTNMP_DEBUG_ERR("agent_ctl_cd_add","MID flags do not match a CD definition.", NULL);
		*status = CTRL_FAILURE;
		mid_release(mid);
		return NULL;
	}

	/* Step 2: Grab the expression that defines the MID. */
	if((expr = adm_extract_mc(params, 2, &success)) == NULL)
	{
		*status = CTRL_FAILURE;
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the type of the definition. */
	def_type = adm_extract_sdnv(params, 3, &success);
	if(success == 0)
	{
		*status = CTRL_FAILURE;
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

	/* Step 4: Add the new definition.
	 * NOTE: def_create_gen shallow-copies information. Do not release
	 * mid or expr!
	 */

	if((cd = def_create_gen(mid, def_type, expr)) == NULL)
	{
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

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

tdc_t *agent_ctl_cd_del(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	uint8_t success = 0;

	/* Step 0: Sanity Check. */
	if(lyst_length(params) != 1)
	{
		DTNMP_DEBUG_ERR("agent_ctl_cd_del","Bad # params. Need 1, received %d", lyst_length(params));
		*status = CTRL_FAILURE;
		return NULL;
	}

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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

tdc_t *agent_ctl_cd_lst(eid_t *def_mgr, Lyst params, uint8_t *status)
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
		def_gen_t *cur = (def_gen_t*) lyst_data(elt);
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
		MRELEASE(data);
		DTNMP_DEBUG_ERR("agent_ctl_cd_lst","Can't add MC to TDC.", NULL);
		*status = CTRL_FAILURE;
		return NULL;
	}

	MRELEASE(data);

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

tdc_t *agent_ctl_cd_dsc(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	uint8_t success = 0;


	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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
		def_gen_t *cur = agent_vdb_compdata_find(mid);

		// \todo: Check for bad return values.
		data = def_serialize_gen(cur, &data_len);
		tdc_insert(retval, DTNMP_TYPE_DEF, data, data_len);
		MRELEASE(data);
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

tdc_t *agent_ctl_rpt_add(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	uint8_t success = 0;
	def_gen_t *result = NULL;
	mid_t *mid = NULL;
	Lyst expr = NULL;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 1, &success)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Grab the expression capturing the definition. */
	if((expr = adm_extract_mc(params, 2, &success)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Create the new definition. This is a shallow copy sp
	 * don't release the mis and expr.  */
	if((result = def_create_gen(mid, DTNMP_TYPE_DEF, expr)) == NULL)
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


tdc_t *agent_ctl_rpt_del(eid_t *def_mgr, Lyst params, uint8_t *status)
{

	Lyst mc = NULL;
	LystElt elt = NULL;

	uint8_t success = 0;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params,1,&success)) == NULL)
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

tdc_t *agent_ctl_rpt_lst(eid_t *def_mgr, Lyst params, uint8_t *status)
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
	MRELEASE(data);

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

tdc_t *agent_ctl_rpt_dsc(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	uint8_t success = 0;
	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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
		tdc_insert(retval, DTNMP_TYPE_DEF, data, data_len);
		MRELEASE(data);
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
tdc_t *agent_ctl_rpt_gen(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	rpt_t *report = NULL;
	eid_t recipient;
	Lyst mc = NULL;
	Lyst rx = NULL;
	LystElt elt = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;
	bzero(&recipient, sizeof(eid_t));

	/* Step 1: Get the MC from the params. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Get the list of mgrs to receive this report. */
	if((rx = adm_extract_dc(params,2, &success)) == NULL)
	{
		/* Step 2.1: If we don'thave, or can't get the rx list send report back to requester. */
		memcpy(&recipient, def_mgr, sizeof(eid_t));
	}
	else
	{
		datacol_entry_t *entry = NULL;

		// \todo: Handle more than 1 recipient.
		if((entry = lyst_data(lyst_first(rx))) == NULL)
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
	report = rda_get_report(recipient);


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
			// \todo: andle the error...
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

tdc_t *agent_ctl_mac_add(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	mid_t *mid = NULL;
	Lyst expr = NULL;
	def_gen_t *result = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;


	/* Step 1: Grab the MID defining the new computed definition. */
	if((mid = adm_extract_mid(params, 1, &success)) == NULL)
	{
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

	if((result = def_create_gen(mid, DTNMP_TYPE_DEF, expr)) == NULL)
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

tdc_t *agent_ctl_mac_del(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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

tdc_t *agent_ctl_mac_lst(eid_t *def_mgr, Lyst params, uint8_t *status)
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
	MRELEASE(data);

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

tdc_t *agent_ctl_mac_dsc(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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
		tdc_insert(retval, DTNMP_TYPE_DEF, data, data_len);
		MRELEASE(data);
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

tdc_t *agent_ctl_trl_add(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	mid_t *mid = NULL;
	Lyst action = NULL;
	uint32_t offset = 0;
	uvast count = 0;
	uvast period = 0;
	trl_t *rule = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new rule. */
	if((mid = adm_extract_mid(params,1, &success)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Grab the offset for this rule. */
	offset = adm_extract_uint(params, 2, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the period for this rule. */
	period = adm_extract_sdnv(params, 3, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Grab the count for this rule. */
	count = adm_extract_sdnv(params, 4, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 5: Grab the macro action for this rule. */
	if((action =adm_extract_mc(params, 5, &success)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 6: Create the new rule. */
	if((rule = trl_create(mid, (time_t) offset, count, period, action)) != NULL)
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

tdc_t *agent_ctl_trl_del(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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

tdc_t *agent_ctl_trl_lst(eid_t *def_mgr, Lyst params, uint8_t *status)
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
	MRELEASE(data);

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

tdc_t *agent_ctl_trl_dsc(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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

		data = trl_serialize(trl, &data_len);
		tdc_insert(retval, DTNMP_TYPE_TRL, data, data_len);
		MRELEASE(data);
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


tdc_t *agent_ctl_srl_add(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	mid_t *mid = NULL;
	Lyst action = NULL;
	Lyst expr = NULL;
	uvast offset = 0;
	uvast count = 0;
	srl_t *rule = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new rule. */
	if((mid = adm_extract_mid(params, 1, &success)) == NULL)
	{
		return NULL;
	}


	/* Step 2: Grab the offset for this rule. */
	offset = adm_extract_uint(params, 2, &success);
	if(success == 0)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Grab the expression for this rule. */
	if((expr = adm_extract_mc(params, 3, &success)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 4: Grab the count for this rule. */
	count = adm_extract_sdnv(params, 4, &success);
	if(success == 0)
	{
		mid_release(mid);
		midcol_destroy(&expr);
		return NULL;
	}

	/* Step 5: Grab the macro action for this rule. */
	if((action =adm_extract_mc(params, 5, &success)) == NULL)
	{
		mid_release(mid);
		midcol_destroy(&expr);
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
	midcol_destroy(&expr);
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

tdc_t *agent_ctl_srl_del(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((mc = adm_extract_mc(params,1,&success)) == NULL)
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

tdc_t *agent_ctl_srl_lst(eid_t *def_mgr, Lyst params, uint8_t *status)
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
	MRELEASE(data);

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


tdc_t *agent_ctl_srl_dsc(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	Lyst mc = NULL;
	LystElt elt = NULL;
	tdc_t *retval = NULL;
	uint8_t *data = NULL;
	uint32_t data_len = 0;
	uint8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the list of MIDs to describe. */
	if((mc = adm_extract_mc(params, 1, &success)) == NULL)
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
		MRELEASE(data);
	}

	midcol_destroy(&mc);

	*status = CTRL_SUCCESS;
	return retval;
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

	result.type = DTNMP_TYPE_UNK;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

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
		result.value.as_int = val_cvt_int(lval) + val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) + val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) + val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) + val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) + val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) + val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_minus","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_minus","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) - val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) - val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) - val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) - val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) - val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) - val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_mult","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_mult","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) * val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) * val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) * val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) * val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) * val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) * val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_div","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_div","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) / val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) / val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) / val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) / val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_real32 = val_cvt_real32(lval) / val_cvt_real32(rval);
		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_real64 = val_cvt_real64(lval) / val_cvt_real64(rval);
		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_mod","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_mod","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) % val_cvt_int(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) % val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uvast = val_cvt_vast(lval) % val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) % val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_exp","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_exp","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = (int32_t) pow(val_cvt_int(lval),val_cvt_int(rval));
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) pow(val_cvt_uint(lval), val_cvt_uint(rval));
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = (vast) pow(val_cvt_vast(lval), val_cvt_vast(rval));
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = (uvast) pow(val_cvt_uvast(lval),val_cvt_uvast(rval));
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_real32 = (float) pow(val_cvt_real32(lval), val_cvt_real32(rval));
		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_real64 = (double) pow(val_cvt_real64(lval), val_cvt_real64(rval));
		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_bitand","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_bitand","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) & val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) & val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) & val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) & val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_bitor","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_bitor","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) | val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) | val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) | val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) | val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_bitxor","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_bitxor","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_int = val_cvt_int(lval) ^ val_cvt_int(rval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) ^ val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = val_cvt_vast(lval) ^ val_cvt_vast(rval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = val_cvt_uvast(lval) ^ val_cvt_uvast(rval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(1, parms, &lval, NULL);
    cvt_type = lval->type;

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
		result.value.as_int = !val_cvt_int(lval);
		result.length = sizeof(int32_t);
		result.type = DTNMP_TYPE_INT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = !val_cvt_uint(lval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_vast = !val_cvt_vast(lval);
		result.length = sizeof(vast);
		result.type = DTNMP_TYPE_VAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = !val_cvt_uvast(lval);
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
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
	case DTNMP_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) && val_cvt_int(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) && val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = val_cvt_vast(lval) && val_cvt_vast(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = val_cvt_uvast(lval) && val_cvt_uvast(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
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
	case DTNMP_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) || val_cvt_int(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) || val_cvt_uint(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = val_cvt_vast(lval) || val_cvt_vast(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = val_cvt_uvast(lval) || val_cvt_uvast(rval);
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	result.type = DTNMP_TYPE_UNK;
	result.value.as_ptr = NULL;

	return result;
}


value_t adm_agent_op_lognot(Lyst parms)
{
	value_t result;
	value_t *lval;
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(1, parms, &lval, NULL);
    cvt_type = lval->type;

	/* Step 3: Based on result type, convert and perform operations. */
    /* We use type for the conversion, and then fix it since logical is
     * always a uint.
     */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = val_cvt_int(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = val_cvt_uint(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = val_cvt_vast(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = val_cvt_uvast(lval) ? 0 : 1;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(1, parms, &lval, NULL);
	cvt_type = lval->type;

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_abs","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_abs","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (int32_t) abs(val_cvt_int(lval));
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) abs(val_cvt_uint(lval));
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uvast = (uvast) labs(val_cvt_vast(lval));
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uvast = (uvast) labs(val_cvt_uvast(lval));
		result.length = sizeof(uvast);
		result.type = DTNMP_TYPE_UVAST;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_real32 = (float) fabsf(val_cvt_real32(lval));
		result.length = sizeof(float);
		result.type = DTNMP_TYPE_REAL32;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_real64 = (double) fabs(val_cvt_real64(lval));
		result.length = sizeof(double);
		result.type = DTNMP_TYPE_REAL64;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_lt","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_lt","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) < val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) < val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) < val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) < val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) < val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) < val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_gt","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_gt","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) > val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) > val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) > val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) > val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) > val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) > val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_lte","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_lte","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) <= val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) <= val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) <= val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) <= val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) <= val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) <= val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_gte","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_gte","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) >= val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) >= val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) >= val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) >= val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) >= val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) >= val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_neq","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_neq","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) != val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) != val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) != val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) != val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) != val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) != val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
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
	uint32_t cvt_type = DTNMP_TYPE_UNK;

	/* Step 1 - Prep for the operation. */
    result = adm_agent_op_prep(2, parms, &lval, &rval);
	cvt_type = val_get_result_type(lval->type, rval->type, VAL_OPTYPE_ARITHMETIC);

    /* Step 2 - Sanity check the prep. */
    if(cvt_type == DTNMP_TYPE_UNK)
    {
        DTNMP_DEBUG_ERR("adm_agent_op_eq","Bad params.",NULL);
        DTNMP_DEBUG_EXIT("adm_agent_op_eq","-> Unknown", NULL);
        return result;
    }

	/* Step 3: Based on result type, convert and perform operations. */
	switch(cvt_type)
	{
	case DTNMP_TYPE_INT:
		result.value.as_uint = (uint32_t) (val_cvt_int(lval) == val_cvt_int(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UINT:
		result.value.as_uint = (uint32_t) (val_cvt_uint(lval) == val_cvt_uint(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_VAST:
		result.value.as_uint = (uint32_t) (val_cvt_vast(lval) == val_cvt_vast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_UVAST:
		result.value.as_uint = (uint32_t) (val_cvt_uvast(lval) == val_cvt_uvast(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL32:
		result.value.as_uint = (uint32_t) (val_cvt_real32(lval) == val_cvt_real32(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	case DTNMP_TYPE_REAL64:
		result.value.as_uint = (uint32_t) (val_cvt_real64(lval) == val_cvt_real64(rval)) ? 1 : 0;
		result.length = sizeof(uint32_t);
		result.type = DTNMP_TYPE_UINT;
		break;
	default:
		break;
	}

	return result;
}

#endif
