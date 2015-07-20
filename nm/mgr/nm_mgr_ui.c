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
 **  01/18/13  E. Birrane     Code comments and cleanup
 **  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag.
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

#include "ctype.h"

#include "platform.h"

#include "shared/utils/utils.h"
#include "shared/adm/adm.h"
#include "shared/adm/adm_agent.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/rules.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/oid.h"
#include "shared/msg/pdu.h"
#include "shared/msg/msg_ctrl.h"
#include "mgr/nm_mgr_names.h"

#include "nm_mgr_ui.h"
#include "mgr/ui_input.h"
#include "nm_mgr_print.h"
#include "mgr_db.h"

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

	if(ui_input_get_line("Agent (#), or 'x' to cancel:",
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
		DTNMP_DEBUG_ERR("ui_postprocess_ctrl","Bad Args.", NULL);
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
		def_gen_t *def = def_create_from_rpt_parms(mid->oid->params);
		if(def != NULL)
		{
			mgr_db_report_persist(def);
			ADD_REPORT(def);
		}
		else
		{
			DTNMP_DEBUG_ERR("ui_postprocess_ctrl", "Adding report definition.",NULL);
		}
	}
	/* If this is removing a report definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELRPT_MID) == 0)
	{
		datacol_entry_t *entry = dc_get_entry(mid->oid->params, 1);

		if(entry != NULL)
		{
			uint32_t bytes = 0;
			LystElt elt = NULL;
			Lyst mc = midcol_deserialize(entry->value, entry->length, &bytes);
			for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
			{
				mid_t *mid = (mid_t *)lyst_data(elt);
				mgr_db_report_forget(mid);
				mgr_vdb_report_forget(mid);
			}
			midcol_destroy(&mc);
		}
		else
		{
			DTNMP_DEBUG_ERR("ui_postprocess_ctrl","Can't get entry.", NULL);
		}
	}
	/* If this is adding a macro definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_ADDMAC_MID) == 0)
	{

	}
	/* If this is removing a macro definition. */
	else if (ui_test_mid(mid, ADM_AGENT_CTL_DELMAC_MID) == 0)
	{

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
		DTNMP_DEBUG_ERR("ui_test_mid","Bad args.", NULL);
		return 0;
	}

	m2 = mid_from_string((char *)mid_str);

	result = mid_compare(mid, m2,0);
	mid_release(m2);
	return result;
}

void ui_send_control(agent_t* agent)
{
	mid_t *mid = NULL;
	uint32_t offset = 0;
	uint32_t size = 0;
	time_t ts = 0;

	if(agent == NULL)
	{
		DTNMP_DEBUG_ENTRY("ui_send_control","(NULL)", NULL);
		DTNMP_DEBUG_ERR("ui_send_control", "No agent specified.", NULL);
		DTNMP_DEBUG_EXIT("ui_send_control","->.",NULL);
		return;
	}
	DTNMP_DEBUG_ENTRY("ui_construct_ctrl_by_idx","(%s)", agent->agent_eid.name);

	ts = ui_input_uint("Control Timestamp");
	mid = ui_input_mid("Control MID:", ADM_ALL, MID_TYPE_CONTROL, MID_CAT_ATOMIC);

	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("ui_send_control","Can't get control MID.",NULL);
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
	MRELEASE(str);

	pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size, NULL);
	pdu_group_t *pdu_group = pdu_create_group(pdu_msg);

	/* Step 4: Send the PDU. */
	iif_send(&ion_ptr, pdu_group, agent->agent_eid.name);

	/* Step 5: Release remaining resources. */
	pdu_release_group(pdu_group);
	msg_destroy_perf_ctrl(ctrl);
	midcol_destroy(&mc); // Also destroys mid.

	DTNMP_DEBUG_EXIT("ui_construct_ctrl_by_idx","->.", NULL);
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
	if(ui_input_get_line("Enter EID of new agent:",
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
	#define UI_CTRL_MENU 1
	#define UI_RPT_MENU  2
	*/

	int gContext = UI_MAIN_MENU;


	while(g_running)
	{
		switch(gContext)
		{
			case UI_MAIN_MENU:  ui_print_menu_main();  break;
			case UI_ADMIN_MENU: ui_print_menu_admin(); break;
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
						case '2' : gContext = UI_RPT_MENU; break;
						case '3' : gContext = UI_CTRL_MENU; break;
						case 'Z' : exit(1); break;
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

						case '9' : ui_send_control(ui_select_agent()); break;

						case 'Z' : gContext = UI_MAIN_MENU; break;
						default: printf("Unknown command.\n"); break;
					}
					break;

				case UI_RPT_MENU:
					switch(cmd)
					{
					  // Definitions List
					  case '1' : ui_print_nop(); //ui_print_agent_comp_data_def(); break; // LIst agent computed data defs
					  case '2' : ui_print_nop(); //ui_print_agent_cust_rpt_defs(); break; // List agent custom report defs
					  case '3' : ui_print_nop(); //ui_print_agent_macro_defs();    break; // LIst agent macro defs.

					  // Report List
					  case '4' : ui_print_reports(ui_select_agent());   break; // Print received reports.
					  case '5' : ui_clear_reports(ui_select_agent());	break; // Clear received reports.

					  // Production Schedules.
					  case '6' : ui_print_nop(); //ui_print_agent_prod_rules();    break; // List agent production rules.

					  case 'Z' : gContext = UI_MAIN_MENU;				break;

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

void ui_list_adms()
{

}

void ui_list_atomic()
{
	ui_list_gen(ADM_ALL, MID_TYPE_DATA, MID_CAT_ATOMIC);
}

void ui_list_compdef()
{
	ui_list_gen(ADM_ALL, MID_TYPE_DATA, MID_CAT_COMPUTED);
}

void ui_list_ctrls()
{
	ui_list_gen(ADM_ALL, MID_TYPE_CONTROL, MID_CAT_ATOMIC);
}

mid_t * ui_get_mid(int adm_type, int mid_type, int mid_cat, uint32_t opt)
{
	mid_t *result = NULL;

	int i = 0;
	LystElt elt = 0;
	mgr_name_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("ui_print","(%d, %d, %d)",adm_type, mid_type, mid_cat);

	Lyst names = names_retrieve(adm_type, mid_type, mid_cat);

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
	DTNMP_DEBUG_EXIT("ui_print","->.", NULL);

	return result;
}


void ui_list_gen(int adm_type, int mid_type, int mid_cat)
{
	  int i = 0;
	  LystElt elt = 0;
	  mgr_name_t *cur = NULL;

	  DTNMP_DEBUG_ENTRY("ui_print","(%d, %d, %d)",adm_type, mid_type, mid_cat);

	  Lyst result = names_retrieve(adm_type, mid_type, mid_cat);

	  for(elt = lyst_first(result); elt; elt = lyst_next(elt))
	  {
		  cur = (mgr_name_t *) lyst_data(elt);
		  printf("%3d) %-50s - %-25s\n", i, cur->name, cur->descr);
		  i++;
	  }

	  lyst_destroy(result);
	  DTNMP_DEBUG_EXIT("ui_print","->.", NULL);
}

void ui_list_literals()
{
	ui_list_gen(ADM_ALL, MID_TYPE_LITERAL, MID_CAT_ATOMIC);
}

void ui_list_macros()
{
	ui_list_gen(ADM_ALL, MID_TYPE_CONTROL, MID_CAT_COLLECTION);
}

void ui_list_ops()
{
	ui_list_gen(ADM_ALL, MID_TYPE_OPERATOR, MID_CAT_ATOMIC);
}

void ui_list_rpts()
{
	ui_list_gen(ADM_ALL, MID_TYPE_DATA, MID_CAT_COLLECTION);
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
	printf("2) List Atomic Data MIDs by Index.   (%ld Known)\n", lyst_length(gAdmData));
	printf("3) List Computed Data MIDs by Index. (%ld Known)\n", lyst_length(gAdmComputed));
	printf("4) List Control MIDs by Index.       (%ld Known)\n", lyst_length(gAdmCtrls));
	printf("5) List Literal MIDs by Index.       (%ld Known)\n", lyst_length(gAdmLiterals));
	printf("6) List Macro MIDs by Index.         (%ld Known)\n", lyst_length(gAdmMacros));
	printf("7) List Operator MIDs by Index.      (%ld Known)\n", lyst_length(gAdmOps));
	printf("8) List Reports MIDs by Index.       (%ld Known)\n", lyst_length(gAdmRpts));

	printf("\n-------------- Perform Control -------------\n");
	printf("9) Execute Arbitrary Control.\n");

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


/*
 * No double-checking, assumes code is correct...
 */
void ui_add_parmspec(char *mid_str, uint8_t num, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5)
{
	ui_parm_spec_t *spec = MTAKE(sizeof(ui_parm_spec_t));

	spec->mid = mid_from_string(mid_str);
	spec->num_parms = num;
	spec->parm_type[0] = p1;
	spec->parm_type[1] = p2;
	spec->parm_type[2] = p3;
	spec->parm_type[3] = p4;
	spec->parm_type[4] = p5;

	lyst_insert_first(gParmSpec, spec);
}

ui_parm_spec_t* ui_get_parmspec(mid_t *mid)
{
	ui_parm_spec_t *result = NULL;

	LystElt elt;

	for(elt = lyst_first(gParmSpec); elt; elt = lyst_next(elt))
	{
		char *mid_str2;
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


void *ui_thread(void * threadId)
{
	DTNMP_DEBUG_ENTRY("ui_thread","(0x%x)", (unsigned long) threadId);

	ui_eventLoop();

	DTNMP_DEBUG_EXIT("ui_thread","->.", NULL);
	pthread_exit(NULL);

	return NULL;
}
