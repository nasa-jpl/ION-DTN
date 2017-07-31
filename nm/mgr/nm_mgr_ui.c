/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **		2. \todo When defining new MID, add to list so it can be IDX selected.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/18/13  E. Birrane     Code comments and cleanup (JHU/APL)
 **  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  07/26/17  E. Birrane     Added batch test file capabilities (JHU/APL)
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "ctype.h"

#include "platform.h"

#include "../shared/utils/utils.h"
#include "../shared/adm/adm.h"
#include "../shared/adm/adm_agent.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/rules.h"
#include "../shared/primitives/mid.h"
#include "../shared/primitives/oid.h"
#include "../shared/msg/pdu.h"
#include "../shared/msg/msg_ctrl.h"
#include "mgr/nm_mgr_names.h"

#include "nm_mgr_ui.h"
#include "mgr/ui_input.h"
#include "nm_mgr_print.h"
#include "mgr_db.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif

int gContext;

Lyst gParmSpec;


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
 *  07/04/16  E. Birrane     Auto-select if only 1 agent known.
 *****************************************************************************/
agent_t* ui_select_agent()
{
	char line[10];
	int idx = -1;
	int total;
	agent_t *agent = NULL;
	LystElt elt;

	printf("Select an Agent:");
	total = ui_print_agents();

	if(total == 0)
	{
		printf("No agents registered. Aborting.\n");
		return NULL;
	}

	if(total == 1)
	{
		if((agent = (agent_t *) lyst_data(lyst_first(known_agents))) == NULL)
		{
			AMP_DEBUG_ERR("ui_select_agent","Null EID in known_agents lyst.", NULL);
			AMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
			return NULL;
		}

		printf("Autoselecting sole known agent: %s.\n", agent->agent_eid.name);
		return agent;
	}

	if(ui_input_get_line("Agent (#), or 'x' to cancel:",
			(char **) &line, 10) == 0)
	{
		AMP_DEBUG_ERR("ui_select_agent","Unable to read user input.", NULL);
		AMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
		return NULL;
	}
	else if(strcmp(line, "x") == 0)
	{
		AMP_DEBUG_EXIT("ui_select_agent","->[cancelled]", NULL);
		return NULL;
	}

	sscanf(line, "%d", &idx);
	if(idx < 0 || idx > total)
	{
		printf("Invalid option.\n");
		AMP_DEBUG_ALWAYS("ui_select_agent", "User selected invalid option (%d).", idx);
		AMP_DEBUG_EXIT("ui_select_agent", "->NULL", NULL);
		return NULL;
	}

	if(idx == 0)
	{
		AMP_DEBUG_ALWAYS("ui_select_agent", "User opted to cancel.", NULL);
		AMP_DEBUG_EXIT("ui_select_agent", "->NULL", NULL);
		return NULL;
	}

	idx--; // Switch from 1-index to 0-index.

	elt = lyst_first(known_agents);
	if(elt == NULL)
	{
		AMP_DEBUG_ERR("ui_select_agent","Empty known_agents lyst.", NULL);
		AMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
		return NULL;
	}

	while(idx != 0)
	{
		idx--;
		elt = lyst_next(elt);
		if(elt == NULL)
		{
			AMP_DEBUG_ERR("ui_select_agent","Out-of-bounds index in known_agents lyst (%d).", idx);
			AMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
			return NULL;
		}
	}

	if((agent = (agent_t *) lyst_data(elt)) == NULL)
	{
		AMP_DEBUG_ERR("ui_select_agent","Null EID in known_agents lyst.", NULL);
		AMP_DEBUG_EXIT("ui_select_agent","->.", NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("ui_select_agent","->%s", agent->agent_eid.name);

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
    	AMP_DEBUG_ENTRY("ui_clear_reports","(NULL)", NULL);
    	AMP_DEBUG_ERR("ui_clear_reports", "No agent specified.", NULL);
        AMP_DEBUG_EXIT("ui_clear_reports","->.",NULL);
        return;
    }
    AMP_DEBUG_ENTRY("ui_clear_reports","(%s)",agent->agent_eid.name);

	int num = lyst_length(agent->reports);
	rpt_clear_lyst(&(agent->reports), NULL, 0);
	g_reports_total -= num;

	AMP_DEBUG_ALWAYS("ui_clear_reports","Cleared %d reports.", num);
    AMP_DEBUG_EXIT("ui_clear_reports","->.",NULL);
}


/******************************************************************************
 *
 * \par Function Name: ui_postprocess_ctrl
 *
 * \par Evaluates a control prior to sending it to agents to see if the manager
 *      needs to perform any postprocessing of the control.
 *
 * \par Notes:
 * 1. Post-processing includes the following activities
 * 		- Verifying that definitions are unique.
 * 		- Remembering the definitions of any custom-defined data
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/18/15  E. Birrane      Initial implementation,
 *****************************************************************************/

void ui_postprocess_ctrl(mid_t *mid)
{

	if(mid == NULL)
	{
		AMP_DEBUG_ERR("ui_postprocess_ctrl","Bad Args.", NULL);
		return;
	}


	/* If this is a computed data definition...*/
	if(ui_test_mid(mid, ADM_AGENT_CTL_ADDCD_MID) == 0)
	{

	}
	/* If this is removing a computed data definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELCD_MID) == 0)
	{

	}
	/* If this is adding a report definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_ADDRPT_MID) == 0)
	{
		def_gen_t *def = def_create_from_rpt_parms(mid->oid.params);
		if(def != NULL)
		{
			mgr_db_report_persist(def);
			ADD_REPORT(def);
		}
		else
		{
			AMP_DEBUG_ERR("ui_postprocess_ctrl", "Adding report definition.",NULL);
		}
	}
	/* If this is removing a report definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELRPT_MID) == 0)
	{
		int8_t success = 0;
		Lyst mc = adm_extract_mc(mid->oid.params, 0, &success);

		if(mc != NULL)
		{
			LystElt elt = NULL;
			for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
			{
				mid_t *tmpmid = (mid_t *)lyst_data(elt);
				mgr_db_report_forget(tmpmid);
				mgr_vdb_report_forget(tmpmid);
			}
			midcol_destroy(&mc);
		}
		else
		{
			AMP_DEBUG_ERR("ui_postprocess_ctrl","Can't get entry.", NULL);
		}
	}
	/* If this is adding a macro definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_ADDMAC_MID) == 0)
	{
		int8_t success = 0;
		mid_t *tmp_mid = NULL;
		Lyst mc = NULL;

		tmp_mid = adm_extract_mid(mid->oid.params, 1, &success);
		if((tmp_mid != NULL) && (success != 0))
		{
			mc = adm_extract_mc(mid->oid.params, 2, &success);
			if((mc == NULL) || (success == 0))
			{
				mid_release(tmp_mid);
			}
			else
			{
				def_gen_t *def = def_create_gen(tmp_mid, AMP_TYPE_MACRO, mc);
				if(def != NULL)
				{
					mgr_db_macro_persist(def);
					ADD_MACRO(def);
				}
			}
		}
	}
	/* If this is removing a macro definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELMAC_MID) == 0)
	{
		int8_t success = 0;
		Lyst mc = adm_extract_mc(mid->oid.params, 0, &success);

		if(mc != NULL)
		{
			LystElt elt = NULL;
			for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
			{
				mid_t *tmp_mid = (mid_t *)lyst_data(elt);
				mgr_db_macro_forget(tmp_mid);
				mgr_vdb_macro_forget(tmp_mid);
			}
			midcol_destroy(&mc);
		}
		else
		{
			AMP_DEBUG_ERR("ui_postprocess_ctrl","DEL Macro: Can't get entry.", NULL);
		}
	}
	/* If this is adding a TRL definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_ADDTRL_MID) == 0)
	{

	}
	/* If this is removing a TRL definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELTRL_MID) == 0)
	{

	}
	/* If this is adding an SRL definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_ADDSRL_MID) == 0)
	{

	}
	/* If this is removing an SRL definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELSRL_MID) == 0)
	{

	}


}

int ui_test_mid(mid_t *mid, const char *mid_str)
{
	int result = 0;
	mid_t*  m2 =  NULL;

	if((mid == NULL) || (mid_str == NULL))
	{
		AMP_DEBUG_ERR("ui_test_mid","Bad args.", NULL);
		return 0;
	}

	m2 = mid_from_string((char *)mid_str);

	result = mid_compare(mid, m2,0);
	mid_release(m2);
	return result;
}



void ui_build_control(agent_t* agent)
{
	mid_t *mid = NULL;
	uint32_t offset = 0;
	uint32_t size = 0;
	time_t ts = 0;

	if(agent == NULL)
	{
		AMP_DEBUG_ENTRY("ui_build_control","(NULL)", NULL);
		AMP_DEBUG_ERR("ui_build_control", "No agent specified.", NULL);
		AMP_DEBUG_EXIT("ui_build_control","->.",NULL);
		return;
	}
	AMP_DEBUG_ENTRY("ui_build_control","(%s)", agent->agent_eid.name);

	ts = ui_input_uint("Control Timestamp");
	mid = ui_input_mid("Control MID:", ADM_ALL, MID_CONTROL);

	if(mid == NULL)
	{
		AMP_DEBUG_ERR("ui_build_control","Can't get control MID.",NULL);
		return;
	}

	ui_postprocess_ctrl(mid);

	Lyst mc = lyst_create();
	lyst_insert_first(mc, mid);

	msg_perf_ctrl_t *ctrl = msg_create_perf_ctrl(ts, mc);

	/* Step 2: Construct a PDU to hold the primitive. */
	uint8_t *data = msg_serialize_perf_ctrl(ctrl, &size);

	char *str = utils_hex_to_string(data, size);
	printf("Data is %s\n", str);
	SRELEASE(str);

	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 4: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 5: Release remaining resources. */
	pdu_release_group(pdu_group);
	msg_destroy_perf_ctrl(ctrl);
	midcol_destroy(&mc); // Also destroys mid.

	AMP_DEBUG_EXIT("ui_build_control","->.", NULL);
}


void ui_send_file(agent_t* agent, uint8_t enter_ts)
{
	mid_t *cur_mid = NULL;
	uint32_t offset = 0;
	uint32_t size = 0;
	time_t ts = 0;
	blob_t *contents = NULL;
	char *cursor = NULL;
	char *saveptr = NULL;
	uint32_t bytes = 0;
	uint8_t *value = NULL;
	uint32_t len = 0;

	if(agent == NULL)
	{
		AMP_DEBUG_ENTRY("ui_send_file","(NULL)", NULL);
		AMP_DEBUG_ERR("ui_send_file", "No agent specified.", NULL);
		AMP_DEBUG_EXIT("ui_send_file","->.",NULL);
		return;
	}
	AMP_DEBUG_ENTRY("ui_send_file","(%s)", agent->agent_eid.name);

	if(enter_ts != 0)
	{
		ts = ui_input_uint("Control Timestamp");
	}
	else
	{
		ts = 0;
	}


	if((contents = ui_input_file_contents("Enter file name containing commands:")) == NULL)
	{
		AMP_DEBUG_ERR("ui_send_file", "Can't read file contents.", NULL);
		AMP_DEBUG_EXIT("ui_send_file","->.",NULL);
		return;
	}

	cursor = strtok_r((char *) contents->value,"\n",&saveptr);

	while(cursor != NULL)
	{
		if(strlen(cursor) <= 0)
		{
//			fprintf(stderr,"Ignoring blank line.\n");
			cursor = strtok_r(NULL, "\n", &saveptr);

			continue;
		}
//		fprintf(stderr,"Read line %s from file.\n", cursor);

		if((cursor[0] == '#') || (cursor[0] == ' '))
		{
//			fprintf(stderr,"Ignoring comment or blank line.\n");
			cursor = strtok_r(NULL, "\n", &saveptr);

			continue;
		}

		if(strncmp(cursor,"WAIT",4) == 0)
		{
			int delay = 0;
			sscanf(cursor,"WAIT %d", &delay);
			fprintf(stderr,"Sleeping for %ds: ", delay);
			int sum = 0;

			int dur = 5;

			if(delay < dur)
			{
				sleep(delay);
			}
			else
			{
				while(delay > 0)
				{
					if(delay >= dur)
					{
						sleep(dur);
						delay -= dur;
						sum += dur;
						fprintf(stderr,"%d...", sum);
					}
					else
					{
						sleep(delay);
						delay = 0;
						sum += delay;
						fprintf(stderr,"%d...", sum);
					}
				}
			}
			fprintf(stderr,"Done waiting!\n");
			cursor = strtok_r(NULL, "\n", &saveptr);
			continue;
		}

		if((value = utils_string_to_hex(cursor, &len)) == NULL)
		{
			AMP_DEBUG_ERR("ui_send_file", "Can't make value from %s", cursor);
			blob_destroy(contents, 1);
			return;
		}

		if((cur_mid = mid_deserialize(value, len, &bytes)) == NULL)
		{
			AMP_DEBUG_ERR("ui_send_file", "Can't make mid from %s", cursor);
			SRELEASE(value);
			blob_destroy(contents, 1);
			return;
		}
		SRELEASE(value);


		ui_postprocess_ctrl(cur_mid);

		Lyst mc = lyst_create();
		lyst_insert_first(mc, cur_mid);

		/* This is a deep copy into ctrl. */
		msg_perf_ctrl_t *ctrl = msg_create_perf_ctrl(ts, mc);
		midcol_destroy(&mc); // Also destroys mid.

		/* Step 2: Construct a PDU to hold the primitive. */
		uint8_t *data = msg_serialize_perf_ctrl(ctrl, &size);
		msg_destroy_perf_ctrl(ctrl);

		char *str = utils_hex_to_string(data, size);
		printf("Data is %s\n", str);
		SRELEASE(str);

		// Shallow copy data into pdu msg.
		pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size, NULL);

		// Shallow copy into group.
		pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

		/* Step 4: Send the PDU. */
		iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

		/* Step 5: Release remaining resources. */
		pdu_release_group(pdu_group);


		cursor = strtok_r(NULL, "\n", &saveptr);
	}


	blob_destroy(contents, 1);

	AMP_DEBUG_EXIT("ui_send_file","->.", NULL);
}


void ui_send_raw(agent_t* agent, uint8_t enter_ts)
{
	mid_t *mid = NULL;
	uint32_t offset = 0;
	uint32_t size = 0;
	time_t ts = 0;

	if(agent == NULL)
	{
		AMP_DEBUG_ENTRY("ui_send_raw","(NULL)", NULL);
		AMP_DEBUG_ERR("ui_send_raw", "No agent specified.", NULL);
		AMP_DEBUG_EXIT("ui_send_raw","->.",NULL);
		return;
	}
	AMP_DEBUG_ENTRY("ui_send_raw","(%s)", agent->agent_eid.name);

	if(enter_ts != 0)
	{
		ts = ui_input_uint("Control Timestamp");
	}
	else
	{
		ts = 0;
	}

	printf("Enter raw MID to send.\n");
	mid = ui_input_mid_raw(1);

	if(mid == NULL)
	{
		AMP_DEBUG_ERR("ui_send_raw","Can't get control MID.",NULL);
		return;
	}

	ui_postprocess_ctrl(mid);

	Lyst mc = lyst_create();
	lyst_insert_first(mc, mid);

	/* This is a deep copy into ctrl. */
	msg_perf_ctrl_t *ctrl = msg_create_perf_ctrl(ts, mc);
	midcol_destroy(&mc); // Also destroys mid.

	/* Step 2: Construct a PDU to hold the primitive. */
	uint8_t *data = msg_serialize_perf_ctrl(ctrl, &size);
	msg_destroy_perf_ctrl(ctrl);

	char *str = utils_hex_to_string(data, size);
	printf("Data is %s\n", str);
	SRELEASE(str);

	// Shallow copy data into pdu msg.
	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size, NULL);

	// Shallow copy into group.
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 4: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 5: Release remaining resources. */
	pdu_release_group(pdu_group);

	AMP_DEBUG_EXIT("ui_send_raw","->.", NULL);
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
 *  06/11/16  E. Birrane     Updated to use parmspec.
 *****************************************************************************/

void ui_define_mid_params(char *name, ui_parm_spec_t* parmspec, mid_t *mid)
{
	char mid_str[256];
	char line[256];
	int cmdFile = fileno(stdin);
	int len = 0;
	int i = 0;
	uint32_t size = 0;

	AMP_DEBUG_ENTRY("ui_define_mid_params", "("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC"))", (uaddr) name, (uaddr)parmspec, (uaddr) mid);

	if((name == NULL) || (parmspec == NULL) || (mid == NULL))
	{
		AMP_DEBUG_ERR("ui_define_mid_params", "Bad Args.", NULL);
		AMP_DEBUG_EXIT("ui_define_mid_params","->.", NULL);
		return;
	}

	printf("MID %s needs %d parameters.\n", name, parmspec->num_parms);

	for(i = 0; i < parmspec->num_parms; i++)
	{
		const char *parm_type = type_to_str(parmspec->parm_type[i]);
		printf("Enter Parm %d (%s):\n",i,parm_type);
	    if (igets(cmdFile, (char *)line, (int) sizeof(line), &len) == NULL)
	    {
	    	if (len != 0)
	    	{
	    		AMP_DEBUG_ERR("ui_define_mid_params","igets failed.", NULL);
	    		AMP_DEBUG_EXIT("ui_define_mid_params","->.", NULL);
	    		return;
	    	}
	    }

    	sscanf(line,"%s", mid_str);

    	size = strlen(mid_str);
    	blob_t b;
    	b.length = size;
    	b.value = (uint8_t*)mid_str;
    	mid_add_param(mid, parmspec->parm_type[i], &b);
	}

	AMP_DEBUG_EXIT("ui_define_mid_params","->.", NULL);
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
	char line[AMP_MAX_EID_LEN];
	eid_t agent_eid;

	AMP_DEBUG_ENTRY("register_agent", "()", NULL);

	/* Grab the new agent's EID. */
	if(ui_input_get_line("Enter EID of new agent:",
						 (char **)&line, AMP_MAX_EID_LEN) == 0)
	{
		AMP_DEBUG_ERR("register_agent","Unable to read user input.", NULL);
		AMP_DEBUG_EXIT("register_agent","->.", NULL);
		return;
	}
	else
		AMP_DEBUG_INFO("register_agent", "User entered agent EID name %s", line);


	/* Check if the agent is already known. */
	sscanf(line, "%s", agent_eid.name);
	mgr_agent_add(agent_eid);

	AMP_DEBUG_EXIT("register_agent", "->.", NULL);
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
	AMP_DEBUG_ENTRY("ui_deregister_agent","(%llu)", (unsigned long)agent);

	if(agent == NULL)
	{
		AMP_DEBUG_ERR("ui_deregister_agent", "No agent specified.", NULL);
		AMP_DEBUG_EXIT("ui_deregister_agent","->.",NULL);
		return;
	}
	AMP_DEBUG_ENTRY("ui_deregister_agent","(%s)",agent->agent_eid.name);

	lockResource(&agents_mutex);

	if(mgr_agent_remove(&(agent->agent_eid)) != 0)
	{
		AMP_DEBUG_WARN("ui_deregister_agent","No agent by that name is currently registered.\n", NULL);
	}
	else
	{
		AMP_DEBUG_ALWAYS("ui_deregister_agent","Successfully deregistered agent.\n", NULL);
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
 *  04/24/16  E. Birrane     Updated to accept global running flag
 *****************************************************************************/
void ui_eventLoop(int *running)
{
	int cmdFile = fileno(stdin);
	char choice[3];
	int len;

	int gContext = UI_MAIN_MENU;


	while(*running)
	{
		switch(gContext)
		{
			case UI_MAIN_MENU:  ui_print_menu_main();  break;
			case UI_ADMIN_MENU: ui_print_menu_admin(); break;
			case UI_CTRL_MENU:  ui_print_menu_ctrl();  break;
			case UI_RPT_MENU:   ui_print_menu_rpt();   break;

#ifdef HAVE_MYSQL
			case UI_DB_MENU:    ui_print_menu_db();    break;
#endif
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
						case '2' : gContext = UI_RPT_MENU; break;
						case '3' : gContext = UI_CTRL_MENU; break;
#ifdef HAVE_MYSQL
						case '4' : gContext = UI_DB_MENU; break;
#endif
						case 'Z' : *running = 0; return; break;
						default: printf("Unknown command.\n");break;
					}
					break;

				case UI_ADMIN_MENU:
					switch(cmd)
					{
						case 'Z' : gContext = UI_MAIN_MENU; break;
						case '1' : ui_register_agent(); break;
						case '2' : ui_print_agents(); break;
						case '3' : ui_deregister_agent(ui_select_agent()); break;
						default: printf("Unknown command.\n"); break;
					}
					break;

				case UI_CTRL_MENU:
					switch(cmd)
					{

						// List Definitions (User or Static)
						case '1' : ui_list_adms();      break; // List supported ADMs
						case '2' : ui_list_atomic();    break; // List Data MIDS by Index
						case '3' : ui_list_compdef();   break; // List Computed Data Items
						case '4' : ui_list_ctrls();     break; // List Control MIDs by Index
						case '5' : ui_list_literals();  break; // List Literal MIDs by Index
						case '6' : ui_list_macros();    break; // List MACRO Definitions by Index
						case '7' : ui_list_ops();       break; // List Operator MIDs by Index
						case '8' : ui_list_rpts();      break; // List Reports by Index.

						case '9' : ui_build_control(ui_select_agent()); break;
						case 'A' : ui_send_raw(ui_select_agent(),0); break;
						case 'B' : ui_send_file(ui_select_agent(),0); break;

						case 'Z' : gContext = UI_MAIN_MENU; break;
						default: printf("Unknown command.\n"); break;
					}
					break;

				case UI_RPT_MENU:
					switch(cmd)
					{
					  // Definitions List
					  case '1' : ui_print_nop(); break; //ui_print_agent_comp_data_def(); break; // LIst agent computed data defs
					  case '2' : ui_print_nop(); break; //ui_print_agent_cust_rpt_defs(); break; // List agent custom report defs
					  case '3' : ui_print_nop(); break; //ui_print_agent_macro_defs();    break; // LIst agent macro defs.

					  // Report List
					  case '4' : ui_print_reports(ui_select_agent());   break; // Print received reports.
					  case '5' : ui_clear_reports(ui_select_agent());	break; // Clear received reports.

					  // Production Schedules.
					  case '6' : ui_print_nop(); break; //ui_print_agent_prod_rules();    break; // List agent production rules.

					  case 'Z' : gContext = UI_MAIN_MENU;				break;

					  default: printf("Unknown command.\n");			break;
					}
					break;

#ifdef HAVE_MYSQL
					case UI_DB_MENU:
						switch(cmd)
						{
						  // Definitions List
						  case '1' : ui_db_set_parms(); break; // New Connection Parameters
						  case '2' : ui_db_print_parms(); break;
						  case '3' : ui_db_reset(); break; // Reset Tables
						  case '4' : ui_db_clear_rpt(); break; // Clear Received Reports
						  case '5' : ui_db_disconn(); break; // Disconnect from DB
						  case '6' : ui_db_conn(); break; // Connect to DB
						  case '7' : ui_db_write(); break; // Write DB info to file.
						  case '8' : ui_db_read(); break; // Read DB infor from file.

						  case 'Z' : gContext = UI_MAIN_MENU;				break;

						  default: printf("Unknown command.\n");			break;
						}
						break;

#endif

				default: printf("Error. Unknown menu context.\n"); break;
			}
		}
	}
}








void ui_list_adms()
{

}

void ui_list_atomic()
{
	ui_list_gen(ADM_ALL, MID_ATOMIC);
}

void ui_list_compdef()
{
	ui_list_gen(ADM_ALL, MID_COMPUTED);
}

void ui_list_ctrls()
{
	ui_list_gen(ADM_ALL, MID_CONTROL);
}

mid_t * ui_get_mid(int adm_type, int mid_id, uint32_t opt)
{
	mid_t *result = NULL;

	int i = 0;
	LystElt elt = 0;
	mgr_name_t *cur = NULL;

	AMP_DEBUG_ENTRY("ui_print","(%d, %d)",adm_type, mid_id);

	Lyst names = names_retrieve(adm_type, mid_id);

	for(elt = lyst_first(names); elt; elt = lyst_next(elt))
	{
		if(i == opt)
		{
			cur = (mgr_name_t *) lyst_data(elt);
			result = mid_copy(cur->mid);
			break;
		}
		i++;
	}

	lyst_destroy(names);
	AMP_DEBUG_EXIT("ui_print","->.", NULL);

	return result;
}


void ui_list_gen(int adm_type, int mid_id)
{
	  int i = 0;
	  LystElt elt = 0;
	  mgr_name_t *cur = NULL;

	  AMP_DEBUG_ENTRY("ui_print","(%d, %d)",adm_type, mid_id);

	  Lyst result = names_retrieve(adm_type, mid_id);

	  for(elt = lyst_first(result); elt; elt = lyst_next(elt))
	  {
		  cur = (mgr_name_t *) lyst_data(elt);
		  printf("%3d) %-50s - %-25s\n", i, cur->name, cur->descr);
		  i++;
	  }

	  lyst_destroy(result);
	  AMP_DEBUG_EXIT("ui_print","->.", NULL);
}

void ui_list_literals()
{
	ui_list_gen(ADM_ALL, MID_LITERAL);
}

void ui_list_macros()
{
	ui_list_gen(ADM_ALL, MID_MACRO);
}

void ui_list_ops()
{
	ui_list_gen(ADM_ALL, MID_OPERATOR);
}

void ui_list_rpts()
{
	ui_list_gen(ADM_ALL, MID_REPORT);
}







void ui_print_menu_admin()
{
	printf("============ Administration Menu =============\n");

	printf("\n------------ Agent Registration ------------\n");
	printf("1) Register Agent.\n");
	printf("2) List Registered Agent.\n");
	printf("3) De-Register Agent.\n");

	printf("\n--------------------------------------------\n");
	printf("Z) Return to Main Menu.\n");

}

void ui_print_menu_ctrl()
{
	printf("=============== Controls Menu ================\n");

	printf("\n------------- ADM Information --------------\n");
	printf("1) List supported ADMs.\n");
	printf("2) List Atomic Data MIDs by Index.   (%lu Known)\n",
			(unsigned long) lyst_length(gAdmData));
	printf("3) List Computed Data MIDs by Index. (%lu Known)\n",
		       (unsigned long) 	lyst_length(gAdmComputed));
	printf("4) List Control MIDs by Index.       (%lu Known)\n",
		       (unsigned long) 	lyst_length(gAdmCtrls));
	printf("5) List Literal MIDs by Index.       (%lu Known)\n",
		       (unsigned long) 	lyst_length(gAdmLiterals));
	printf("6) List Macro MIDs by Index.         (%lu Known)\n",
		       (unsigned long) 	lyst_length(gAdmMacros));
	printf("7) List Operator MIDs by Index.      (%lu Known)\n",
		       (unsigned long) 	lyst_length(gAdmOps));
	printf("8) List Reports MIDs by Index.       (%lu Known)\n",
		       (unsigned long) 	lyst_length(gAdmRpts));

	printf("\n-------------- Perform Control -------------\n");
	printf("9) Build Arbitrary Control.\n");
	printf("A) Specify Raw Control.\n");
	printf("B) Specify Control File.\n");

	printf("\n--------------------------------------------\n");
	printf("Z) Return to Main Menu.\n");
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
	printf("2) Reporting Menu.\n");
	printf("3) Control Menu. \n");

#ifdef HAVE_MYSQL
	printf("4) Database Menu. \n");
#endif

	printf("Z) Exit.\n");

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
	printf("Z) Return to Main Menu.\n");
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

	oid_t oid = oid_deserialize_full(tmp_oid, 8, &bytes);

	fprintf(stderr,"Deserialized %d bytes into:\n", bytes);
	str = oid_pretty_print(oid);
	fprintf(stderr,"%s",str);
	SRELEASE(str);

	msg = oid_serialize(oid,&bytes);
	fprintf(stderr,"Serialized %d bytes into ", bytes);
	utils_print_hex(msg,bytes);
	SRELEASE(msg);
	fprintf(stderr,"\n----------------------------------------\n");


	/* Test 2: Construct a MID and serialize/deserialize it. */
	fprintf(stderr,"MID TEST 1\n");
	uvast issuer = 0, tag = 0;

	mid_t *mid = mid_construct(0,NULL, NULL, oid);
	msg = (unsigned char*)mid_to_string(mid);
	fprintf(stderr,"Constructed mid: %s\n", msg);
	SRELEASE(msg);

	msg = mid_serialize(mid, &bytes);
	fprintf(stderr,"Serialized %d bytes into ", bytes);
	utils_print_hex(msg, bytes);

	uint32_t b2;
	mid_t *mid2 = mid_deserialize(msg, bytes, &b2);
	SRELEASE(msg);
	msg = (unsigned char *)mid_to_string(mid2);

	fprintf(stderr,"Deserialized %d bytes into MID %s\n", b2, msg);
	SRELEASE(msg);
	mid_release(mid2);
	mid_release(mid);
}


/*
 * No double-checking, assumes code is correct...
 */
void ui_add_parmspec(char *mid_str,
						       uint8_t num,
		                       char *n1, uint8_t p1,
		                       char *n2, uint8_t p2,
		                       char *n3, uint8_t p3,
		                       char *n4, uint8_t p4,
		                       char *n5, uint8_t p5)
{
	ui_parm_spec_t *spec = STAKE(sizeof(ui_parm_spec_t));
	CHKVOID(spec);

	memset(spec, 0, sizeof(ui_parm_spec_t));

	spec->mid = mid_from_string(mid_str);
	spec->num_parms = num;

	if(n1 != NULL) istrcpy(spec->parm_name[0], n1, MAX_PARM_NAME);
	spec->parm_type[0] = p1;

	if(n2 != NULL) istrcpy(spec->parm_name[1], n2, MAX_PARM_NAME);
	spec->parm_type[1] = p2;

	if(n3 != NULL) istrcpy(spec->parm_name[2], n3, MAX_PARM_NAME);
	spec->parm_type[2] = p3;

	if(n4 != NULL) istrcpy(spec->parm_name[3], n4, MAX_PARM_NAME);
	spec->parm_type[3] = p4;

	if(n5 != NULL) istrcpy(spec->parm_name[4], n5, MAX_PARM_NAME);
	spec->parm_type[4] = p5;

	lyst_insert_last(gParmSpec, spec);
}

ui_parm_spec_t* ui_get_parmspec(mid_t *mid)
{
	ui_parm_spec_t *result = NULL;

	LystElt elt;

	for(elt = lyst_first(gParmSpec); elt; elt = lyst_next(elt))
	{
		result = lyst_data(elt);

		if(mid_compare(mid, result->mid, 0) == 0)
		{
			return result;
		}
	}

	return NULL;
}

void ui_print_nop()
{
	printf("This command is currently not implemented in this development version.\n\n");
}


void *ui_thread(int *running)
{
	AMP_DEBUG_ENTRY("ui_thread","(0x%x)", (unsigned long) running);

	ui_eventLoop(running);

	AMP_DEBUG_ALWAYS("ui_thread","Exiting.", NULL);

	AMP_DEBUG_EXIT("ui_thread","->.", NULL);

#ifdef HAVE_MYSQL
	db_mgt_close();
#endif


	pthread_exit(NULL);

	return NULL;
}




#ifdef HAVE_MYSQL

void ui_print_menu_db()
{

	printf("========================= Database Menu ==========================\n");
	printf("Database Status: ");

	if(db_mgt_connected() == 0)
	{
		printf("[ACTIVE]\n");
	}
	else
	{
		printf("[NOT CONNECTED]\n");
	}

	printf("1) Set Database Connection Information.\n");
	printf("2) Print Database Connection Information.\n");
	printf("3) Reset Database to ADMs.\n");
	printf("4) Clear Received Reports.\n");
	printf("5) Disconnect From DB.\n");
	printf("6) Connect to DB.\n");
	printf("7) Write DB Info to File\n");
	printf("8) Read DB Info from file\n");

	printf("------------------------------------------------------------------\n");
	printf("Z) Return to Main Menu.\n");

}


void ui_db_conn()
{
	ui_db_t parms;

	ui_db_disconn();

	memset(&parms, 0, sizeof(ui_db_t));

	lockResource(&(gMgrVDB.sqldb_mutex));

	memcpy(&parms, &(gMgrVDB.sqldb), sizeof(ui_db_t));

	unlockResource(&(gMgrVDB.sqldb_mutex));

	db_mgt_init(parms, 0, 1);
}

void ui_db_disconn()
{
    db_mgt_close();
}


void ui_db_write()
{
  FILE *fp = 0;
  char *tmp = NULL;
  
  tmp = ui_input_string("Enter file name.");

  if((fp = fopen(tmp, "w+")) == NULL)
  {
    printf("Can't open or create %s.\n", tmp);
    SRELEASE(tmp);
    return; 
  }


 lockResource(&(gMgrVDB.sqldb_mutex));

 fwrite(&(gMgrVDB.sqldb.server), UI_SQL_SERVERLEN-1, 1, fp);
 fwrite(&(gMgrVDB.sqldb.database), UI_SQL_DBLEN-1, 1, fp);
 fwrite(&(gMgrVDB.sqldb.username), UI_SQL_ACCTLEN-1,1, fp);
 fwrite(&(gMgrVDB.sqldb.password), UI_SQL_ACCTLEN-1,1, fp);

 unlockResource(&(gMgrVDB.sqldb_mutex));

fclose(fp);
  printf("Database infor written to %s.\n", tmp);
 
 SRELEASE(tmp);
}

void ui_db_read()
{
  FILE *fp = NULL;
  char *tmp = NULL;

  tmp = ui_input_string("Enter file name.");

  if(tmp == NULL)
  {
    printf("Error reading string.\n");
    return;  
  }
  if ((fp = fopen(tmp, "r")) == NULL)
  {
    printf("Can't open file %s.\n", tmp);
    SRELEASE(tmp);
    return;
  }

  lockResource(&(gMgrVDB.sqldb_mutex));

  if(fread(&(gMgrVDB.sqldb.server), UI_SQL_SERVERLEN-1, 1, fp) <= 0)
    printf("Error reading server.\n");

  if(fread(&(gMgrVDB.sqldb.database), UI_SQL_DBLEN-1, 1, fp) <= 0)
    printf("Error reading database.\n");

  if(fread(&(gMgrVDB.sqldb.username), UI_SQL_ACCTLEN-1,1, fp) <= 0)
    printf("Error reading username.\n");

  if(fread(&(gMgrVDB.sqldb.password), UI_SQL_ACCTLEN-1,1, fp) <= 0)
    printf("Error reading password.r\n");
 
  mgr_db_sql_persist(&gMgrVDB.sqldb);

  unlockResource(&(gMgrVDB.sqldb_mutex));
  fclose(fp);

  printf("Read from %s.\n", tmp);
  SRELEASE(tmp);
  return;
}


void ui_db_set_parms()
{
	ui_db_t parms;

	char *tmp = NULL;
	char prompt[80];

	memset(&parms, 0, sizeof(ui_db_t));

	printf("Enter SQL Database Connection Information:\n");

	sprintf(prompt,"Enter Database Server (up to %d characters", UI_SQL_SERVERLEN-1);
	tmp = ui_input_string(prompt);
	strncpy(parms.server, tmp, UI_SQL_SERVERLEN-1);
	SRELEASE(tmp);

	sprintf(prompt,"Enter Database Name (up to %d characters", UI_SQL_DBLEN-1);
	tmp = ui_input_string(prompt);
	strncpy(parms.database, tmp, UI_SQL_DBLEN-1);
	SRELEASE(tmp);

	sprintf(prompt,"Enter Database Username (up to %d characters", UI_SQL_ACCTLEN-1);
	tmp = ui_input_string(prompt);
	strncpy(parms.username, tmp, UI_SQL_ACCTLEN-1);
	SRELEASE(tmp);

	sprintf(prompt,"Enter Database Password (up to %d characters", UI_SQL_ACCTLEN-1);
	tmp = ui_input_string(prompt);
	strncpy(parms.password, tmp, UI_SQL_ACCTLEN-1);
	SRELEASE(tmp);

	mgr_db_sql_persist(&parms);

	lockResource(&(gMgrVDB.sqldb_mutex));

	memcpy(&(gMgrVDB.sqldb), &parms, sizeof(ui_db_t));

	unlockResource(&(gMgrVDB.sqldb_mutex));

}

void ui_db_print_parms()
{
	printf("\n\n");
	printf("Server: %s\nDatabase: %s\nUsername: %s\nPassword: %s\n",
		gMgrVDB.sqldb.server, gMgrVDB.sqldb.database, gMgrVDB.sqldb.username, gMgrVDB.sqldb.password);
	printf("\n\n");
}

void ui_db_reset()
{
	printf("Clearing non-ADM tables in the Database....\n");
	db_mgt_clear();
	printf("Done!\n\n");
}

void ui_db_clear_rpt()
{
	printf("Not implemented yet.\n");
}

#endif


