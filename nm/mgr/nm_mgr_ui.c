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
 ** \file nm_mgr_ui.c
 **
 **
 ** Description: A text-based DTNMP Manager.
 **
 ** Notes:
 **		1. Currently we do not support ACLs.
 **		2. Currently we do not support multiple DTNMP Agents.
 **		3. When defining new MID, add to list so it can be IDX selected.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/18/13  E. Birrane     Code comments and cleanup
 **  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag.
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

#include "ctype.h"

#include "platform.h"

#include "shared/utils/utils.h"
#include "shared/adm/adm.h"
#include "shared/primitives/rules.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/oid.h"
#include "shared/msg/pdu.h"

#include "nm_mgr_ui.h"

/*
#define UI_MAIN_MENU 0
#define UI_DEF_MENU  1
#define UI_CTRL_MENU 2
#define UI_RPT_MENU  3
*/

int gContext;


/******************************************************************************
 *
 * \par Function Name: ui_build_mid
 *
 * \par Builds a MID object from user input.
 *
 * \par Notes:
 *
 * \retval NULL  - Error
 * 		   !NULL - The constructed MID.
 *
 * \param[in]  mid_str  The user input to define the MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

mid_t *ui_build_mid(char *mid_str)
{
	mid_t *result = NULL;
	uint8_t *tmp = NULL;
	uint32_t len = 0;
	uint32_t bytes = 0;
	adm_datadef_t adu;

	DTNMP_DEBUG_ENTRY("ui_build_mid","(0x%x)", mid_str);

	/* Step 0: Sanity check. */
	if(mid_str == NULL)
	{
		DTNMP_DEBUG_ERR("ui_build_mid","Bad args.", NULL);
		DTNMP_DEBUG_EXIT("ui_build_mid","->NULL", NULL);
		return NULL;
	}

	/* Step 1: Convert the string into a binary buffer. */
    if((tmp = utils_string_to_hex((unsigned char*)mid_str, &len)) == NULL)
    {
    	DTNMP_DEBUG_ERR("ui_build_mid","Can't Parse MID ID of %s.", mid_str);
		DTNMP_DEBUG_EXIT("ui_build_mid","->NULL", NULL);
		return NULL;
    }

    /* Step 2: Build an ADU from the buffer. */
 //   memcpy(adu.mid, tmp, len);
 //   adu.mid_len = len;
//    MRELEASE(tmp);

    /* Step 3: Build a mid by "deserializing" the ADU into a MID. */
//	result = mid_deserialize((unsigned char*)&(adu.mid),ADM_MID_ALLOC,&bytes);
    result = mid_deserialize(tmp, len, &bytes);

	DTNMP_DEBUG_EXIT("ui_build_mid","->0x%x", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: ui_select_agent
 *
 * \par Prompts the user to select a known agent from a list.
 *
 * \par Notes:
 *
 * \par Returns the selected agent's EID, or NULL if cancelled (or an error occurs).
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/18/13  V.Ramachandran Initial implementation
 *  06/17/13  E. Birrane     Working implementation
 *****************************************************************************/
agent_t* ui_select_agent()
{
	char line[10];
	int idx = -1;
	int total;
	agent_t *agent = NULL;
	eid_t *agent_eid;
	LystElt elt;

	printf("Select an Agent:");
	total = ui_print_agents();

	if(ui_get_user_input("Agent (#), or 'x' to cancel:",
			(char **) &line, 10) == 0)
	{
		DTNMP_DEBUG_ERR("ui_select_agent","Unable to read user input.", NULL);
		DTNMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
		return NULL;
	}
	else if(strcmp(line, "x") == 0)
	{
		DTNMP_DEBUG_EXIT("ui_select_agent","->[cancelled]", NULL);
		return NULL;
	}

	sscanf(line, "%d", &idx);
	if(idx < 0 || idx >= total)
	{
		printf("Invalid option.\n");
		DTNMP_DEBUG_ALWAYS("ui_select_agent", "User selected invalid option (%d).", idx);
		DTNMP_DEBUG_EXIT("ui_select_agent", "->NULL", NULL);
		return NULL;
	}

	if(idx == 0)
	{
		DTNMP_DEBUG_ALWAYS("ui_select_agent", "User opted to cancel.", NULL);
		DTNMP_DEBUG_EXIT("ui_select_agent", "->NULL", NULL);
		return NULL;
	}

	idx--; // Switch from 1-index to 0-index.

	elt = lyst_first(known_agents);
	if(elt == NULL)
	{
		DTNMP_DEBUG_ERR("ui_select_agent","Empty known_agents lyst.", NULL);
		DTNMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
		return NULL;
	}

	while(idx != 0)
	{
		idx--;
		elt = lyst_next(elt);
		if(elt == NULL)
		{
			DTNMP_DEBUG_ERR("ui_select_agent","Out-of-bounds index in known_agents lyst (%d).", idx);
			DTNMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
			return NULL;
		}
	}

	if((agent = (agent_t *) lyst_data(elt)) == NULL)
	{
		DTNMP_DEBUG_ERR("ui_select_agent","Null EID in known_agents lyst.", NULL);
		DTNMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("ui_select_agent","->%s", agent->agent_eid.name);

	return agent;
}

/******************************************************************************
 *
 * \par Function Name: ui_clear_reports
 *
 * \par Clears the list of received data reports from an agent.
 *
 * \par Notes:
 *	\todo - Add ability to clear reports from a particular agent, or of a
 *	\todo   particular type, or for a particular timeframe.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation,
 *  01/18/13  E. Birrane     Debug updates.
 *  04/18/13  V.Ramachandran Multiple-agent support (added param)
 *****************************************************************************/
void ui_clear_reports(agent_t* agent)
{
    if(agent == NULL)
    {
    	DTNMP_DEBUG_ENTRY("ui_clear_reports","(NULL)", NULL);
    	DTNMP_DEBUG_ERR("ui_clear_reports", "No agent specified.", NULL);
        DTNMP_DEBUG_EXIT("ui_clear_reports","->.",NULL);
        return;
    }
    DTNMP_DEBUG_ENTRY("ui_clear_reports","(%s)",agent->agent_eid.name);

	int num = lyst_length(agent->reports);
	rpt_clear_lyst(&(agent->reports), NULL, 0);
	g_reports_total -= num;

	DTNMP_DEBUG_ALWAYS("ui_clear_reports","Cleared %d reports.", num);
    DTNMP_DEBUG_EXIT("ui_clear_reports","->.",NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_construct_ctrl_by_idx
 *
 * \par Constructs a "execute control" message by selecting the control from
 *      a list of controls. Puts the control in a PDU and sends to agent.
 *
 * \par Notes:
 *	\todo Add ability to select which agent, and to apply ACL.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  04/18/13  V.Ramachandran Multiple-agent support (added param)
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

void ui_construct_ctrl_by_idx(agent_t* agent)
{
	char line[256];
	uint32_t offset;
	char mid_str[256];
	Lyst mids = lyst_create();
	uint32_t size = 0;

	if(agent == NULL)
	{
		DTNMP_DEBUG_ENTRY("ui_construct_ctrl_by_idx","(NULL)", NULL);
		DTNMP_DEBUG_ERR("ui_construct_ctrl_by_idx", "No agent specified.", NULL);
		DTNMP_DEBUG_EXIT("ui_construct_ctrl_by_idx","->.",NULL);
		return;
	}
	DTNMP_DEBUG_ENTRY("ui_construct_ctrl_by_idx","(%s)", agent->agent_eid.name);

	/* Step 0: Read the user input. */
	if(ui_get_user_input("Enter ctrl as follows: Offset <Ctrl Idx>",
			             (char **)&line, 256) == 0)
	{
		DTNMP_DEBUG_ERR("ui_construct_ctrl_by_idx","Unable to read user input.", NULL);
		DTNMP_DEBUG_EXIT("ui_construct_ctrl_by_idx","->.", NULL);
		return;
	}

	/* Step 1: Parse the user input. */
	sscanf(line,"%d %s", &offset, mid_str);
	mids = ui_parse_mid_str(mid_str, lyst_length(gAdmCtrls)-1, MID_TYPE_CONTROL);

	/* Step 2: Construct the control primitive. */
	ctrl_exec_t *entry = ctrl_create_exec(offset, mids);

	/* Step 3: Construct a PDU to hold the primitive. */
	uint8_t *data = ctrl_serialize_exec(entry, &size);
	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 4: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 5: Release remaining resources. */
	pdu_release_group(pdu_group);
	ctrl_release_exec(entry);

	DTNMP_DEBUG_EXIT("ui_construct_ctrl_by_idx","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_construct_time_rule_by_idx
 *
 * \par Constructs a "time production report" control by selecting data from
 *      a list of MIDs. Puts the control in a PDU and sends to agent.
 *
 * \par Notes:
 *	\todo Add ability to select which agent, and to apply ACL.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

void ui_construct_time_rule_by_idx(agent_t* agent)
{
	char line[256];
	time_t offset = 0;
	uint32_t period = 0;
	uint32_t evals = 0;
	char mid_str[256];
	Lyst mids = lyst_create();
	uint32_t size = 0;

	if(agent == NULL)
	{
		DTNMP_DEBUG_ENTRY("ui_construct_time_rule_by_idx","(NULL)", NULL);
		DTNMP_DEBUG_ERR("ui_construct_time_rule_by_idx", "Null EID", NULL);
		DTNMP_DEBUG_EXIT("ui_construct_time_rule_by_idx","->.", NULL);
		return;
	}
	DTNMP_DEBUG_ENTRY("ui_construct_time_rule_by_idx","(%s)", agent->agent_eid.name);

	/* Step 1: Read and parse the rule. */
	if(ui_get_user_input("Enter rule as follows: Offset Period #Evals MID1,MID2,MID3,...,MIDn",
			             (char **) &line, 256) == 0)
	{
		DTNMP_DEBUG_ERR("ui_construct_time_rule_by_idx","Unable to read user rule input.", NULL);
		DTNMP_DEBUG_EXIT("ui_construct_time_rule_by_idx","->.", NULL);
		return;
	}

	sscanf(line,"%ld %d %d %s", &offset, &period, &evals, mid_str);
	mids = ui_parse_mid_str(mid_str, lyst_length(gAdmData)-1, MID_TYPE_DATA);

	/* Step 2: Construct the control primitive. */
	rule_time_prod_t *entry = rule_create_time_prod_entry(offset, evals, period, mids);

	/* Step 3: Construct a PDU to hold the primitive. */
	uint8_t *data = ctrl_serialize_time_prod_entry(entry, &size);
	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_PERIOD_PROD, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 4: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 5: Release remaining resources. */
	pdu_release_group(pdu_group);
	rule_release_time_prod_entry(entry);

	DTNMP_DEBUG_EXIT("ui_construct_time_rule_by_idx","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_construct_time_rule_by_mid
 *
 * \par Constructs a "time production report" control by building a MID. Put
 *      the control in a PDU and sends to agent.
 *
 * \par Notes:
 *	\todo Add ability to select which agent, and to apply ACL.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/
void ui_construct_time_rule_by_mid(agent_t* agent)
{
	char line[256];
	time_t offset = 0;
	uint32_t period = 0;
	uint32_t evals = 0;
	char mid_str[256];
	Lyst mids = lyst_create();
	mid_t *midp = NULL;
	uint32_t size = 0;

	if(agent == NULL)
	{
		DTNMP_DEBUG_ENTRY("ui_construct_time_rule_by_mid","(NULL)", NULL);
		DTNMP_DEBUG_ERR("ui_construct_time_rule_by_mid", "Null EID", NULL);
		DTNMP_DEBUG_EXIT("ui_construct_time_rule_by_mid","->.", NULL);
		return;
	}
	DTNMP_DEBUG_ENTRY("ui_construct_time_rule_by_mid","(%s)", agent->agent_eid.name);

	/* Step 0: Read the user input. */
	if(ui_get_user_input("Enter rule as follows: Offset Period #Evals MID",
			             (char **) &line, 256) == 0)
	{
		DTNMP_DEBUG_ERR("ui_construct_time_rule_by_mid","Unable to read user input.", NULL);
		DTNMP_DEBUG_EXIT("ui_construct_time_rule_by_mid","->.", NULL);
		return;
	}

	/* Step 1: Parse the user input. */
	sscanf(line,"%ld %d %d %s", &offset, &period, &evals, mid_str);
	midp = ui_build_mid(mid_str);

	char *str = mid_to_string(midp);
	printf("MID IS %s\n", str);
	MRELEASE(str);

	lyst_insert_last(mids,midp);

	/* Step 2: Construct the control primitive. */
	rule_time_prod_t *entry = rule_create_time_prod_entry(offset, evals, period, mids);

	/* Step 3: Construct a PDU to hold the primitive. */
	uint8_t *data = ctrl_serialize_time_prod_entry(entry, &size);
	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_PERIOD_PROD, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 4: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 5: Release remaining resources. */
	pdu_release_group(pdu_group);
	rule_release_time_prod_entry(entry);

	DTNMP_DEBUG_EXIT("ui_construct_time_rule_by_mid","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_define_report
 *
 * \par Define a custom report and send the definition to the agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

void ui_define_report(agent_t* agent)
{
	mid_t *new_id = NULL;
	char line[256];
	char mid_str[256];
	Lyst mids;
	def_gen_t *rpt_def = NULL;
	uint32_t size = 0;

	if(agent == NULL)
	{
		DTNMP_DEBUG_ENTRY("ui_define_report","(NULL)", NULL);
		DTNMP_DEBUG_ERR("ui_define_report", "Null EID", NULL);
		DTNMP_DEBUG_EXIT("ui_define_report","->.", NULL);
		return;
	}
	DTNMP_DEBUG_ENTRY("ui_define_report","(%s)", agent->agent_eid.name);

	/* Step 0: Grab the identifier for the new report. */
	new_id = ui_input_mid();

	/* Step 1:Grab the MID list defining the report. */
	if(ui_get_user_input("MIDS comprising this report: IDX1,IDX2,...,IDX_N",
			             (char **)&line, 256) == 0)
	{
		DTNMP_DEBUG_ERR("ui_define_report","Unable to read user input.", NULL);
		DTNMP_DEBUG_EXIT("ui_define_report","->.", NULL);
		return;
	}

	/* Step 1: Parse the user input. */
	sscanf(line,"%s", mid_str);
	mids = ui_parse_mid_str(mid_str, lyst_length(gAdmData)-1, MID_TYPE_DATA);

	/* Step 2: Construct the control primitive. */
	rpt_def = def_create_gen(new_id, mids);

	/* Step 3a: Record this definition in the agent def lyst. */
	lockResource(&(agent->mutex));
	lyst_insert_last(agent->custom_defs, rpt_def);
	unlockResource(&(agent->mutex));

	/* Step 4: Construct a PDU to hold the primitive. */
	uint8_t *data = def_serialize_gen(rpt_def, &size);
	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_DEF_CUST_RPT, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 5: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 6: Release remaining resources. */
	pdu_release_group(pdu_group);

	/*
	 * Remember, we do not free the report definition because we added it
	 * to the list of local custom definitions for this agent.
	 */

	DTNMP_DEBUG_EXIT("ui_define_report","->.", NULL);
}


/******************************************************************************
 *
 * \par Function Name: ui_define_macro
 *
 * \par Define a custom macro and send the definition to the agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

void ui_define_macro(agent_t* agent)
{
	mid_t *new_id = NULL;
	char line[256];
	char mid_str[256];
	Lyst mids;
	def_gen_t *macro_def = NULL;
	uint32_t size = 0;

	/* Step 0: Grab the identifier for the new macro. */
	new_id = ui_input_mid();

	/* Step 1:Grab the MID list defining the macro. */
	if(ui_get_user_input("Controls comprising this macro: IDX1,IDX2,...,IDX_N",
			             (char **)&line, 256) == 0)
	{
		DTNMP_DEBUG_ERR("ui_define_macro","Unable to read user input.", NULL);
		DTNMP_DEBUG_EXIT("ui_define_macro","->.", NULL);
		return;
	}

	/* Step 1: Parse the user input. */
	sscanf(line,"%s", mid_str);
	mids = ui_parse_mid_str(mid_str, lyst_length(gAdmCtrls)-1, MID_TYPE_DATA);

	/* Step 2: Construct the control primitive. */
	macro_def = def_create_gen(new_id, mids);

	/* Step 3: Remember this definition for future use. */
	lockResource(&macro_defs_mutex);
	lyst_insert_last(macro_defs, macro_def);
	unlockResource(&macro_defs_mutex);

	/* Step 4: Construct a PDU to hold the primitive. */
	uint8_t *data = def_serialize_gen(macro_def, &size);
	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_DEF_MACRO, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 5: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 6: Release remaining resources. */
	pdu_release_group(pdu_group);

	DTNMP_DEBUG_EXIT("ui_define_report","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_define_mid_params
 *
 * \par Allows user to input MID parameters.
 *
 * \par Notes:
 * \todo Find a way to name each parameter.
 *
 * \param[in]  name       The name of the MID needing parameters.
 * \param[out] num_parms  The number of parameters needed.
 * \param[out] mid        The augmented MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_define_mid_params(char *name, int num_parms, mid_t *mid)
{
	char mid_str[256];
	char line[256];
	int cmdFile = fileno(stdin);
	int len = 0;
	int i = 0;
	uint32_t size = 0;

	DTNMP_DEBUG_ENTRY("ui_define_mid_params", "(0x%x, %d, 0x%x)",
			          (unsigned long) name, num_parms, (unsigned long) mid);

	if((name == NULL) || (mid == NULL))
	{
		DTNMP_DEBUG_ERR("ui_define_mid_params", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("ui_define_mid_params","->.", NULL);
		return;
	}

	printf("MID %s needs %d parameters.\n", name, num_parms);

	for(i = 0; i < num_parms; i++)
	{
		printf("Enter Parm %d:\n",i);
	    if (igets(cmdFile, (char *)line, (int) sizeof(line), &len) == NULL)
	    {
	    	if (len != 0)
	    	{
	    		DTNMP_DEBUG_ERR("ui_define_mid_params","igets failed.", NULL);
	    		DTNMP_DEBUG_EXIT("ui_define_mid_params","->.", NULL);
	    		return;
	    	}
	    }

    	sscanf(line,"%s", mid_str);

    	//result = utils_string_to_hex((unsigned char*) mid_str, &size);
    	size = strlen(mid_str);

    	mid_add_param(mid, (unsigned char*)mid_str, size);
	}

	DTNMP_DEBUG_EXIT("ui_define_mid_params","->.", NULL);
}

/******************************************************************************
 *
 * \par Function Name: ui_register_agent
 *
 * \par Register a new agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/18/13  V.Ramachandran Initial Implementation
 *****************************************************************************/
void ui_register_agent()
{
	char line[MAX_EID_LEN];
	eid_t agent_eid;
	agent_t *agent;

	DTNMP_DEBUG_ENTRY("register_agent", "()", NULL);

	/* Grab the new agent's EID. */
	if(ui_get_user_input("Enter EID of new agent:",
						 (char **)&line, MAX_EID_LEN) == 0)
	{
		DTNMP_DEBUG_ERR("register_agent","Unable to read user input.", NULL);
		DTNMP_DEBUG_EXIT("register_agent","->.", NULL);
		return;
	}
	else
		DTNMP_DEBUG_INFO("register_agent", "User entered agent EID name %s", line);


	/* Check if the agent is already known. */
	sscanf(line, "%s", agent_eid.name);
	mgr_agent_add(agent_eid);

	DTNMP_DEBUG_EXIT("register_agent", "->.", NULL);
}

/******************************************************************************
 *
 * \par Function Name: ui_deregister_agent
 *
 * \par Remove and deallocate an agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/23/13  V.Ramachandran Initial Implementation
 *****************************************************************************/
void ui_deregister_agent(agent_t* agent)
{
	char line[MAX_EID_LEN];

	DTNMP_DEBUG_ENTRY("ui_deregister_agent","(%llu)", (unsigned long)agent);

	if(agent == NULL)
	{
		DTNMP_DEBUG_ERR("ui_deregister_agent", "No agent specified.", NULL);
		DTNMP_DEBUG_EXIT("ui_deregister_agent","->.",NULL);
		return;
	}
	DTNMP_DEBUG_ENTRY("ui_deregister_agent","(%s)",agent->agent_eid.name);

	lockResource(&agents_mutex);

	if(mgr_agent_remove(&(agent->agent_eid)) != 0)
	{
		DTNMP_DEBUG_WARN("ui_deregister_agent","No agent by that name is currently registered.\n", NULL);
	}
	else
	{
		DTNMP_DEBUG_ALWAYS("ui_deregister_agent","Successfully deregistered agent.\n", NULL);
	}

	unlockResource(&agents_mutex);
}

/******************************************************************************
 *
 * \par Function Name: ui_eventLoop
 *
 * \par Main event loop for the UI thread.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/
void ui_eventLoop()
{
	int cmdFile = fileno(stdin);
	char choice[3];
	int len;

	/*
	#define UI_MAIN_MENU 0
	#define UI_DEF_MENU  1
	#define UI_CTRL_MENU 2
	#define UI_RPT_MENU  3
	*/

	int gContext = UI_MAIN_MENU;


	while(g_running)
	{
		switch(gContext)
		{
			case UI_MAIN_MENU:  ui_print_menu_main();  break;
			case UI_ADMIN_MENU: ui_print_menu_admin(); break;
			case UI_DEF_MENU:   ui_print_menu_def();   break;
			case UI_CTRL_MENU:  ui_print_menu_ctrl();  break;
			case UI_RPT_MENU:   ui_print_menu_rpt();   break;
			default: printf("Error. Unknown menu context.\n"); break;
		}

		if ((igets(cmdFile, (char *)choice, (int) sizeof(choice), &len) != NULL) && (len > 0))
		{
			char cmd = toupper(choice[0]);

			switch(gContext)
			{
				case UI_MAIN_MENU:
					switch(cmd)
					{
						case '1' : gContext = UI_ADMIN_MENU; break;
						case '2' : gContext = UI_DEF_MENU; break;
						case '3' : gContext = UI_RPT_MENU; break;
						case '4' : gContext = UI_CTRL_MENU; break;
						case '5' : exit(1); break;
						default: printf("Unknown command.\n");break;
					}
					break;

				case UI_ADMIN_MENU:
					switch(cmd)
					{
						case '0' : gContext = UI_MAIN_MENU; break;
						case '1' : ui_register_agent(); break;
						case '2' : ui_print_agents(); break;
						case '3' : ui_deregister_agent(ui_select_agent()); break;
						default: printf("Unknown command.\n"); break;
					}
					break;

				case UI_DEF_MENU:
					switch(cmd)
					{
						// Custom Reports
						//case '1' : ui_print_custom_rpt_def();  break; // List Mgr Custom Rpt Def.
						case '2' : ui_define_report(ui_select_agent());         break; // Define Custom Report
						//case '3' : ui_print_cust_rpt_by_idx(); break; // Identify custom report by idx.
						//case '4' : ui_clear_cust_rpt_by_idx(); break; // Remove custom rept using index.

						// Computed Data
						//case '5' : ui_print_comp_data_def();    break; // List Computed Data Def
						//case '6' : ui_define_comp_data();       break; // Define Computed Data
						//case '7' : ui_print_comp_data_by_idx(); break; // Identify computed data by idx.
						//case '8' : ui_clear_comp_data_by_idx(); break; // Remove computed data using index.

						// Macro
						//case '9' : ui_print_macro_def();    break; // List Macro Def
						case 'A' : ui_define_macro(ui_select_agent());       break; // Define Macro
						//case 'B' : ui_print_macro_by_idx(); break; // Identify macro by idx.
						//case 'C' : ui_clear_macro_by_idx(); break; // Remove macro using index.

						case 'D' : gContext = UI_MAIN_MENU; break;
						default: printf("Unknown command.\n"); break;
					}
					break;

				case UI_CTRL_MENU:
					switch(cmd)
					{
						//case '1' : ui_print_adms();      break; // List supported ADMs
						case '2' : ui_print_mids();      break; // List Data MIDS by Index
						case '3' : ui_print_ctrls();     break; // List Control MIDs by Index
						//case '4' : ui_print_literals();  break; // List Literal MIDs by Index
						//case '5' : ui_print_operators(); break; // List Operator MIDs by Index

						// time based production
						case '6' : ui_construct_time_rule_by_idx(ui_select_agent()); break; // Construct from indices
						case '7' : ui_construct_time_rule_by_mid(ui_select_agent()); break; // Construct from user-input MID
						//case '8' : ui_cancel_time_rule_by_idx();    break; // Cancel sent production

						// Content-Based Production
						//case '9' : ui_construct_pred_rule_by_idx(); break; // Construct from indices
						//case 'A' : ui_construct_pred_rule_by_mid(); break; // Construct from user-input MID
						//case 'B' : ui_cancel_pred_rule_by_idx();    break; // Cancel sent production

						// Perform Control
						case 'C' : ui_construct_ctrl_by_idx(ui_select_agent()); break; // Construct from indices
						//case 'D' : ui_construct_ctrl_by_mid(); break; // Construct user-input MID
						//case 'E' : ui_print_sent_ctrls();      break; // List Sent Controls
						//case 'F' : ui_cancel_ctrl_by_idx();    break; // Cancel Sent Control by Idx

						case 'G' : gContext = UI_MAIN_MENU; break;
						default: printf("Unknown command.\n"); break;
					}
					break;

				case UI_RPT_MENU:
					switch(cmd)
					{
						// Data List
						//case '1' : ui_print_agent_comp_data_def(); break; // LIst agent computed data defs

						// Definitions List
						//case '2' : ui_print_agent_cust_rpt_defs(); break; // List agent custom report defs
						//case '3' : ui_print_agent_macro_defs();    break; // LIst agent macro defs.

						// Report List
						case '4' : ui_print_reports(ui_select_agent());   break; // Print received reports.
						case '5' : ui_clear_reports(ui_select_agent());	break; // Clear received reports.

						// Production Schedules.
						//case '6' : ui_print_agent_prod_rules();    break; // List agent production rules.
						case '7' : gContext = UI_MAIN_MENU;				break;
						default: printf("Unknown command.\n");			break;
					}
					break;

				default: printf("Error. Unknown menu context.\n"); break;
			}
		}
	}
}


/******************************************************************************
 *
 * \par Function Name: ui_get_user_input
 *
 * \par Read a line of input from the user.
 *
 * \par Notes:
 *
 * \retval 0 - Could not get user input.
 * 		   1 - Got user input.
 *
 * \param[in]  prompt   The prompt to the user.
 * \param[out] line     The line of text read from the user.
 * \param [in] max_len  The maximum size of the line.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

int ui_get_user_input(char *prompt, char **line, int max_len)
{
	int len = 0;

	DTNMP_DEBUG_ENTRY("ui_get_user_input","(%s,0x%x,%d)",prompt, *line, max_len);

	while(len == 0)
	{
		printf("Note: Only the first %d characters will be read.\n%s\n",
				max_len, prompt);

		if (igets(fileno(stdin), (char *)line, max_len, &len) == NULL)
		{
			if (len != 0)
			{
				DTNMP_DEBUG_ERR("ui_get_user_input","igets failed.", NULL);
				DTNMP_DEBUG_EXIT("ui_get_user_input","->0.",NULL);
				return 0;
			}
		}
	}

	DTNMP_DEBUG_INFO("ui_get_user_input","Read user input.", NULL);

	DTNMP_DEBUG_EXIT("ui_get_user_input","->1.",NULL);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: ui_input_mid
 *
 * \par Construct a MID object completely from user input.
 *
 * \par Notes:
 *
 * \retval NULL  - Problem building the MID.
 * 		   !NULL - The constructed MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag.
 *****************************************************************************/

mid_t *ui_input_mid()
{
	uint8_t flag;
	char line[256];
	uint32_t size = 0;
	uint8_t *data = NULL;
	mid_t *result = NULL;
	uint32_t bytes = 0;

	/* Step 0: Allocate the resultant MID. */
	if((result = (mid_t*)MTAKE(sizeof(mid_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("ui_input_mid","Can't alloc %d bytes.", sizeof(mid_t));
		DTNMP_DEBUG_EXIT("ui_input_mid", "->NULL.", NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(mid_t));
	}

	/* Step 1: Get the MID flag. */
	ui_input_mid_flag(&(result->flags));
	result->type = MID_GET_FLAG_TYPE(result->flags);
	result->category = MID_GET_FLAG_CAT(result->flags);

	/* Step 2: Grab Issuer, if necessary. */
	if(MID_GET_FLAG_ISS(result->flags))
	{
		ui_get_user_input("Issuer (up to 18 hex): 0x", (char**)&line, 256);
		data = utils_string_to_hex((unsigned char*)line, &size);
		memcpy(&(result->issuer), data, 4);
		MRELEASE(data);

		if(size > 4)
		{
			DTNMP_DEBUG_ERR("ui_input_mid", "Issuer too big: %d.", size);
			DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
			mid_release(result);
			return NULL;
		}
	}

	/* Step 3: Grab the OID. */
	ui_get_user_input("OID: 0x", (char**)&line, 256);
	data = utils_string_to_hex((unsigned char *)line, &size);
	result->oid = NULL;

	switch(MID_GET_FLAG_OID(result->flags))
	{
		case OID_TYPE_FULL:
			result->oid = oid_deserialize_full(data, size, &bytes);
			printf("OID value size is %d\n", result->oid->value_size);
			break;
		case OID_TYPE_PARAM:
			result->oid = oid_deserialize_param(data, size, &bytes);
			break;
		case OID_TYPE_COMP_FULL:
			result->oid = oid_deserialize_comp(data, size, &bytes);
			break;
		case OID_TYPE_COMP_PARAM:
			result->oid = oid_deserialize_comp_param(data, size, &bytes);
			break;
		default:
			DTNMP_DEBUG_ERR("ui_input_mid","Unknown OID Type %d",
						    MID_GET_FLAG_OID(result->flags));
			break;
	}
	MRELEASE(data);

	if((result->oid == NULL) || (bytes != size))
	{
		DTNMP_DEBUG_ERR("ui_input_mid", "Bad OID. Size %d. Bytes %d.",
				        size, bytes);
		mid_release(result);

		DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
		return NULL;
	}

	/* Step 4: Grab a tag, if one exists. */
	if(MID_GET_FLAG_TAG(result->flags))
	{
		ui_get_user_input("Tag (up to 18 hex): 0x", (char**)&line, 256);
		data = utils_string_to_hex((unsigned char*)line, &size);
		memcpy(&(result->tag), data, 4);
		MRELEASE(data);

		if(size > 4)
		{
			DTNMP_DEBUG_ERR("ui_input_mid", "Tag too big: %d.", size);
			DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
			mid_release(result);
			return NULL;
		}
	}

	mid_internal_serialize(result);

	/* Step 5: Sanity check this mid. */
	if(mid_sanity_check(result) == 0)
	{
		DTNMP_DEBUG_ERR("ui_input_mid", "Sanity check failed.", size);
		DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
		mid_release(result);
		return NULL;
	}

	char *mid_str = mid_to_string(result);
	DTNMP_DEBUG_ALWAYS("ui_input_mid", "Defined MID: %s", mid_str);
	MRELEASE(mid_str);

	DTNMP_DEBUG_EXIT("ui_input_mid", "->0x%x", (unsigned long) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: ui_input_mid_flag
 *
 * \par Construct a MID flag byte completely from user input.
 *
 * \par Notes:
 *
 * \retval 0 - Can't construct flag byte
 * 		   1 - Flag byte constructed
 *
 * \param[out]  flag   The resulting flags
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field.Add ISS Flag.
 *****************************************************************************/

int ui_input_mid_flag(uint8_t *flag)
{
	int result = 0;
	char line[256];
	int tmp;

	*flag = 0;

	ui_get_user_input("Type: Data (0), Ctrl (1), Literal (2), Op (3):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag = (tmp & 0x3);

	ui_get_user_input("Cat: Atomic (0), Computed (1), Collection (2):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x3) << 2;

	ui_get_user_input("Issuer Field Present? Yes (1)  No (0):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x1) << 4;

	ui_get_user_input("Tag Field Present? Yes (1)  No (0):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x1) << 5;

	ui_get_user_input("OID Type: Full (0), Parm (1), Comp (2), Parm+Comp(3):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x3) << 6;

	printf("Constructed Flag Byte: 0x%x\n", *flag);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: ui_parse_mid_str
 *
 * \par Convert a series of MID indices into a lyst of actual MID objects.
 *
 * \par Notes:
 *
 * \retval NULL  - Problem parsing MIDs.
 * 		   !NULL - The list of MIDs
 *
 * \param[in]  mid_str  String of MID indices.
 * \param[in]  max_idx  The largest valid MID idx.
 * \param[in]  type     Type of MID being indexed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

Lyst ui_parse_mid_str(char *mid_str, int max_idx, int type)
{
	char *cur_mid = NULL;
	char *tmp = NULL;
	int cur_mid_idx = 0;
	uint32_t mid_size = 0;
	Lyst mids = NULL;

	DTNMP_DEBUG_ENTRY("ui_parse_mid_str","(0x%x)",mid_str);

	/* Step 0: Sanity Check. */
	if(mid_str == NULL)
	{
		DTNMP_DEBUG_ERR("ui_parse_mid_str","Bad args.", NULL);
		DTNMP_DEBUG_EXIT("ui_parse_mid_str","->NULL.", NULL);
		return NULL;
	}

	mids = lyst_create();

	/* Step 1: Walk through each MID */
	for(cur_mid = strtok_r(mid_str, ", ", &tmp);
	    cur_mid != NULL;
	    cur_mid = strtok_r(NULL, ", ", &tmp))
	{
		DTNMP_DEBUG_INFO("ui_parse_mid_str","Read MID index of %s", cur_mid);

		/* Step 1a. Convert and check MID index. */
		if((cur_mid_idx = atoi(cur_mid)) <= max_idx)
		{
			uint32_t bytes = 0;
			mid_t *midp = NULL;
			char *name = NULL;
			int num_parms = 0;
			int mid_len = 0;

			/* Step 1b: Grab MID and put it into the list. */

			switch(type)
			{
			case MID_TYPE_DATA:
				{
					adm_datadef_t *cur = adm_find_datadef_by_idx(cur_mid_idx);
					midp = mid_copy(cur->mid);
					num_parms = cur->num_parms;
					/***
					name = adus[cur_mid_idx].name;
					num_parms = adus[cur_mid_idx].num_parms;
					mid_len = adus[cur_mid_idx].mid_len;
					midp = mid_deserialize((unsigned char*)&(adus[cur_mid_idx].mid),
						                adus[cur_mid_idx].mid_len,&bytes);
					 */
				}
				break;
			case MID_TYPE_CONTROL:
				{
					adm_ctrl_t *cur = adm_find_ctrl_by_idx(cur_mid_idx);
					midp = mid_copy(cur->mid);
					num_parms = cur->num_parms;
					/**
					name = ctrls[cur_mid_idx].name;
					num_parms = ctrls[cur_mid_idx].num_parms;
					mid_len = ctrls[cur_mid_idx].mid_len;
					midp = mid_deserialize((unsigned char*)&(ctrls[cur_mid_idx].mid),
						               ctrls[cur_mid_idx].mid_len,&bytes);
					 **/
				}
				break;
			case MID_TYPE_LITERAL:
				/* \todo: Write this. */
			case MID_TYPE_OPERATOR:
				/* \todo: Write this. */
			default:
				DTNMP_DEBUG_ERR("ui_parse_mid_str","Unknown type %d", type);
				DTNMP_DEBUG_EXIT("ui_parse_mid_str","->NULL.",NULL);
				lyst_destroy(mids);
				return NULL;
			}

			/* If this MID has parameters, get them from the user. */
			if(num_parms > 0)
			{
				ui_define_mid_params(name, num_parms, midp);
				mid_internal_serialize(midp);
			}

			mid_size += mid_len;
			lyst_insert_last(mids, midp);
		}
		else
		{
			DTNMP_DEBUG_ERR("ui_parse_mid_str",
					        "Bad MID index: %d. Max is %d. Skipping.",
					        cur_mid_idx, max_idx);
		}


	}


	DTNMP_DEBUG_EXIT("ui_parse_mid_str","->0x%x.", mids);

	return mids;
}

/******************************************************************************
 *
 * \par Function Name: ui_print_agents
 *
 * \par Prints list of known agents
 *
 * \par Returns number of agents
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/18/13  V.Ramachandran Initial Implementation
 *****************************************************************************/

int ui_print_agents()
{
  int i = 1;
  LystElt element;

  DTNMP_DEBUG_ENTRY("ui_print_agents","()",NULL);

  printf("\n------------- Known Agents --------------\n");

  element = lyst_first(known_agents);
  if(element == NULL)
  {
	  printf("[None]\n");
  }
  while(element != NULL)
  {
	  printf("%d) %s\n", i++, (char *) lyst_data(element));
	  element = lyst_next(element);
  }

  printf("\n------------- ************ --------------\n");
  printf("\n");

  DTNMP_DEBUG_EXIT("ui_print_agents","->%d", (i-1));
  return i;
}

/******************************************************************************
 *
 * \par Function Name: ui_print_ctrls
 *
 * \par Prints list of configured controls and their associated index
 *
 * \par Notes:
 * 	1. Assuming 80 column display, 2 column are printed of length 40 each.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_print_ctrls()
{
  int i = 0;
  int num_full_rows = 0;
  int num_rows = 0;
  LystElt elt = 0;
  adm_ctrl_t *cur = NULL;

  DTNMP_DEBUG_ENTRY("ui_print_ctrls","()",NULL);

  num_full_rows = (int) (lyst_length(gAdmCtrls) / 2);

  for(elt = lyst_first(gAdmCtrls); elt; elt = lyst_next(elt))
  {
	  cur = (adm_ctrl_t*) lyst_data(elt);
	  printf("%3d) %-35s ", i, cur->name);
	  i++;

	  if(num_rows < num_full_rows)
	  {
		  elt = lyst_next(elt);
		  cur = (adm_ctrl_t*) lyst_data(elt);
		  printf("%3d) %-35s\n", i, cur->name);
		  i++;
	  }
	  else
	  {
		  printf("\n\n\n");
	  }
	  num_rows++;
  }

  DTNMP_DEBUG_EXIT("ui_print_ctrls","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_print_custom_rpt
 *
 * \par Prints a custom report received by a DTNMP Agent.
 *
 * \par Notes:
 *
 * \param[in]  rpt_entry  The entry containing the report data to print.
 * \param[in]  rpt_def    The static definition of the report.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_print_custom_rpt(rpt_data_entry_t *rpt_entry, def_gen_t *rpt_def)
{
	LystElt elt;
	uint64_t idx = 0;
	mid_t *cur_mid = NULL;
	adm_datadef_t *adu = NULL;
	uint64_t data_used;

	for(elt = lyst_first(rpt_def->contents); elt; elt = lyst_next(elt))
	{
		char *mid_str;
		cur_mid = (mid_t*)lyst_data(elt);
		mid_str = mid_to_string(cur_mid);
		if((adu = adm_find_datadef(cur_mid)) != NULL)
		{
			DTNMP_DEBUG_INFO("ui_print_custom_rpt","Printing MID %s", mid_str);
			ui_print_predefined_rpt(cur_mid, (uint8_t*)&(rpt_entry->contents[idx]),
					             rpt_entry->size - idx, &data_used, adu);
			idx += data_used;
		}
		else
		{
			DTNMP_DEBUG_ERR("ui_print_custom_rpt","Unable to find MID %s", mid_str);
		}

		MRELEASE(mid_str);
	}
}



void ui_print_menu_admin()
{
	printf("============ Administration Menu =============\n");

	printf("\n------------ Agent Registration ------------\n");
	printf("1) Register Agent.\n");
	printf("2) List Registered Agent.\n");
	printf("3) De-Register Agent.\n");

	printf("\n------------ Reporting Policies ------------\n");


	printf("\n-------------- Status Message --------------\n");
	printf("X) Not Implemented.\n");

	printf("\n--------------------------------------------\n");
	printf("0) Return to Main Menu.\n");

}

void ui_print_menu_ctrl()
{
	printf("=============== Controls Menu ================\n");

	printf("\n------------- ADM Information --------------\n");
	printf("1) List supported ADMs.\n");
	printf("2) List Data MIDs by Index.     (%ld MIDs)\n", lyst_length(gAdmData));
	printf("3) List Control MIDs by Index.  (%ld MIDs)\n", lyst_length(gAdmCtrls));
	printf("4) List Literal MIDs by Index.  (%ld MIDs)\n", lyst_length(gAdmLiterals));
	printf("5) List Operator MIDs by Index. (%ld MIDs)\n", lyst_length(gAdmOps));

	printf("\n---------- Time-Based Production -----------\n");
	printf("6) Construct from indices.\n");
	printf("7) Construct from user-input MID.\n");
	printf("8) Cancel Sent Production.\n");

	printf("\n--------- Content-Based Production ---------\n");
	printf("X) Construct from indices.\n");
	printf("X) Construct from user-input MID.\n");
	printf("X) Cancel Sent Production.\n");

	printf("\n-------------- Perform Control -------------\n");
	printf("C) Construct from indices.\n");
	printf("D) Construct from user-input MID.\n");
	printf("E) List Sent Controls.\n");
	printf("F) Cancel Sent Control by index.\n");

	printf("\n--------------------------------------------\n");
	printf("G) Return to Main Menu.\n");
}

void ui_print_menu_def()
{
	printf("============== Definitions Menu ==============\n");

	printf("\n-------------- Custom Reports --------------\n");
	printf("1) List Manager Custom Report Definitions.\n");
	printf("2) Define Custom Report.\n");
	printf("3) Identify Custom Report Using Index.\n");
	printf("4) Remove Custom Report Using Index.\n");

	printf("\n--------------- Computed Data --------------\n");
	printf("5) List Manager Computed Data Definitions.\n");
	printf("6) Define Computed Data.\n");
	printf("7) Identify Computed Data Using Index.\n");
	printf("8) Remove Computed Data Using Index.\n");

	printf("\n------------------- Macros -----------------\n");
	printf("9) List Manager Macro Definitions.\n");
	printf("A) Define Macro.\n");
	printf("B) Identify Macro Using Index.\n");
	printf("C) Remove Macro Using Index.\n");

	printf("\n--------------------------------------------\n");
	printf("D) Return to Main Menu.\n");
}


/******************************************************************************
 *
 * \par Function Name: ui_print_menu
 *
 * \par Prints the user menu.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_print_menu_main()
{
	printf("================== Main Menu =================\n");
	printf("1) Administrative Menu.\n");
	printf("2) Definitions Menu.\n");
	printf("3) Reporting Menu.\n");
	printf("4) Control Menu. \n");
	printf("5) Exit.\n");

//	printf("2) Define Custom MID. (We have %ld defined.)\n", lyst_length(custom_defs));
//	printf("3) Construct and Send Production Rule Using Index.\n");
//	printf("4) Construct and Send Production Rule Using MID.\n");
//	printf("5) Go to Reports Menu.\n");
//	printf("6) Go to Controls Menu.\n");
//	printf("9) Exit.\n");
//	printf("10) Run Tests.\n");

}

void ui_print_menu_rpt()
{

	printf("========================= Reporting Menu =========================\n");

	printf("\n--------------------------- Data List --------------------------\n");
	printf("1) List Agent Computed Data Definitions.\n");

	printf("\n------------------------ Definitions List ----------------------\n");
	printf("2) List Agent Custom Report Definition.\n");
	printf("3) List Agent Macro Definitions.\n");

	printf("\n-------------------------- Report List -------------------------\n");
	printf("4) Print Reports Received from an Agent (We have %d reports).\n", g_reports_total);
	printf("5) Clear Reports Received from an Agent.\n");

	printf("\n---------------------- Production Schedules --------------------\n");
	printf("6) List Agent Production Rules.\n");

	printf("------------------------------------------------------------------\n");
	printf("7) Return to Main Menu.\n");
}



/******************************************************************************
 *
 * \par Function Name: ui_print_mids
 *
 * \par Prints list of configured data items and their associated index
 *
 * \par Notes:
 * 	1. Assuming 80 column display, 2 column are printed of length 40 each.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_print_mids()
{
	int i = 0;
	int num_full_rows = 0;
	int num_rows = 0;
	LystElt elt = 0;
	adm_datadef_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("ui_print_mids","()",NULL);


	num_full_rows = (int) (lyst_length(gAdmData) / 2);

	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		cur = (adm_datadef_t*) lyst_data(elt);
		printf("%3d) %-35s ", i, cur->name);
		i++;

		if(num_rows < num_full_rows)
		{
			elt = lyst_next(elt);
			cur = (adm_datadef_t*) lyst_data(elt);
			printf("%3d) %-35s\n", i, cur->name);
			i++;
		}
		else
		{
			printf("\n\n\n");
		}
		num_rows++;
	}

	DTNMP_DEBUG_EXIT("ui_print_mids","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: ui_print_predefined_rpt
 *
 * \par Prints a pre-defined report received by a DTNMP Agent.
 *
 * \par Notes:
 *
 * \param[in]  mid        The identifier of the data item being printed.
 * \param[in]  data       The contents of the data item.
 * \param[in]  data_size  The size of the data to be printed.
 * \param[out] data_used  The bytes of the data consumed by printing.
 * \param[in]  adu        The static definition of the report.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_print_predefined_rpt(mid_t *mid, uint8_t *data, uint64_t data_size, uint64_t *data_used, adm_datadef_t *adu)
{
	uint64_t len;
	char *mid_str = NULL;
	char *mid_val = NULL;
	uint32_t val_size = adu->get_size(data, data_size);
	uint32_t str_size = 0;

	mid_str = mid_to_string(mid);

	if((mid_val = adu->to_string(data, data_size, val_size, &str_size)) == NULL)
	{
		DTNMP_DEBUG_ERR("ui_print_predefined_rpt","Can't print data value for %s.",
				        mid_str);
		MRELEASE(mid_str);
		return;
	}

	*data_used = str_size;
	printf("Data Name: %s\n", adu->name);
	printf("MID      : %s\n", mid_str);
	printf("Value    : %s\n", mid_val);
	MRELEASE(mid_val);
	MRELEASE(mid_str);
}



/******************************************************************************
 *
 * \par Function Name: ui_print_reports
 *
 * \par Print all reports in the received reports queue.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

void ui_print_reports(agent_t* agent)
{
	 LystElt report_elt;
	 LystElt entry_elt;
	 rpt_data_t *cur_report = NULL;
	 rpt_data_entry_t *cur_entry = NULL;

	 if(agent == NULL)
	 {
		 DTNMP_DEBUG_ENTRY("ui_print_reports","(NULL)", NULL);
		 DTNMP_DEBUG_ERR("ui_print_reports", "No agent specified", NULL);
		 DTNMP_DEBUG_EXIT("ui_print_reports", "->.", NULL);
		 return;

	 }
	 DTNMP_DEBUG_ENTRY("ui_print_reports","(%s)", agent->agent_eid.name);

	 if(lyst_length(agent->reports) == 0)
	 {
		 DTNMP_DEBUG_ALWAYS("ui_print_reports","[No reports received from this agent.]", NULL);
		 DTNMP_DEBUG_EXIT("ui_print_reports", "->.", NULL);
		 return;
	 }

	 /* Free any reports left in the reports list. */
	 for (report_elt = lyst_first(agent->reports); report_elt; report_elt = lyst_next(report_elt))
	 {
		 /* Grab the current report */
	     if((cur_report = (rpt_data_t*)lyst_data(report_elt)) == NULL)
	     {
	        DTNMP_DEBUG_ERR("ui_print_reports","Unable to get report from lyst!", NULL);
	     }
	     else
	     {
	    	 unsigned long mid_sizes = 0;
	    	 unsigned long data_sizes = 0;
	    	 adm_datadef_t *adu = NULL;
	    	 def_gen_t *report = NULL;

	    	 /* Print the Report Header */
	    	 printf("\n-----------------\nDTNMP DATA REPORT\n-----------------\n");
	    	 printf("Sent to  : %s\n", cur_report->recipient.name);
	    	 printf("Rpt. Size: %d\n", cur_report->size);
	    	 printf("Timestamp: %ld\n", cur_report->time);
	    	 printf("Num Mids : %ld\n", lyst_length(cur_report->reports));
	    	 printf("Value(s)\n---------------------------------\n");


	    	 /* For each MID in this report, print it. */
	    	 for(entry_elt = lyst_first(cur_report->reports); entry_elt; entry_elt = lyst_next(entry_elt))
	    	 {
	    		 cur_entry = (rpt_data_entry_t*)lyst_data(entry_elt);

	    		 mid_sizes += cur_entry->id->raw_size;
	    		 data_sizes += cur_entry->size;

	    		 /* See if this is a pre-defined report, or a custom report. */
	    		 /* Find ADM associated with this entry. */
	    		 if((adu = adm_find_datadef(cur_entry->id)) != NULL)
	    		 {
	    			 uint64_t used;
	    			 ui_print_predefined_rpt(cur_entry->id, cur_entry->contents, cur_entry->size, &used, adu);
	    		 }
	    		 else if((report = def_find_by_id(agent->custom_defs, &(agent->mutex), cur_entry->id)) != NULL)
	    		 {
	    			 ui_print_custom_rpt(cur_entry, report);
	    		 }
	    		 else
	    		 {
	    			 char *mid_str = mid_to_string(cur_entry->id);
	    			 DTNMP_DEBUG_ERR("ui_print_reports","Could not print MID %s", mid_str);
	    			 MRELEASE(mid_str);
	    		 }
	    	 }
	    	 printf("=================\n");
	    	 printf("STATISTICS:\n");
	    	 printf("MIDs total %ld bytes\n", mid_sizes);
	    	 printf("Data total: %ld bytes\n", data_sizes);
	    	 printf("Efficiency: %.2f%%\n", (double)(((double)data_sizes)/((double)cur_report->size)) * (double)100.0);
	    	 printf("-----------------\n\n\n");
	     }
	 }
}



/******************************************************************************
 *
 * \par Function Name: ui_run_tests
 *
 * \par Run local manager tests to test out libraries.
 *
 * \par Notes:
 * \todo Move this to a test file.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field.
 *****************************************************************************/

void ui_run_tests()
{
	char *str;
	unsigned char *msg;

	/* Test 1: Construct an OID and serialize/deserialize it. */
	// # bytes (SDNV), followe dby the bytes.
	fprintf(stderr,"OID TEST 1\n----------------------------------------\n");
	unsigned char tmp_oid[8] = {0x07,0x01,0x02,0x03,0x04,0x05,0x06,0x00};
	uint32_t bytes = 0;

	fprintf(stderr,"Initial is ");
	utils_print_hex(tmp_oid,8);

	oid_t *oid = oid_deserialize_full(tmp_oid, 8, &bytes);

	fprintf(stderr,"Deserialized %d bytes into:\n", bytes);
	str = oid_pretty_print(oid);
	fprintf(stderr,"%s",str);
	MRELEASE(str);

	msg = oid_serialize(oid,&bytes);
	fprintf(stderr,"Serialized %d bytes into ", bytes);
	utils_print_hex(msg,bytes);
	MRELEASE(msg);
	fprintf(stderr,"\n----------------------------------------\n");


	/* Test 2: Construct a MID and serialize/deserialize it. */
	fprintf(stderr,"MID TEST 1\n");
	uvast issuer = 0, tag = 0;

	mid_t *mid = mid_construct(0,0, NULL, NULL, oid);
	msg = (unsigned char*)mid_to_string(mid);
	fprintf(stderr,"Constructed mid: %s\n", msg);
	MRELEASE(msg);

	msg = mid_serialize(mid, &bytes);
	fprintf(stderr,"Serialized %d bytes into ", bytes);
	utils_print_hex(msg, bytes);

	uint32_t b2;
	mid_t *mid2 = mid_deserialize(msg, bytes, &b2);
	MRELEASE(msg);
	msg = (unsigned char *)mid_to_string(mid2);

	fprintf(stderr,"Deserialized %d bytes into MID %s\n", b2, msg);
	MRELEASE(msg);
	mid_release(mid2);
	mid_release(mid);
}



void *ui_thread(void * threadId)
{
	DTNMP_DEBUG_ENTRY("ui_thread","(0x%x)", (unsigned long) threadId);

	ui_eventLoop();

	DTNMP_DEBUG_EXIT("ui_thread","->.", NULL);
	pthread_exit(NULL);
}
