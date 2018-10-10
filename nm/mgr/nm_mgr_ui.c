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
 ** Description: A text-based AMP Manager.
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
 **  10/07/18  E. Birrane     Update to AMP v0.5. (JHU/APL)
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "ctype.h"

#include "platform.h"

#include "../shared/utils/utils.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/rules.h"
#include "../shared/msg/msg.h"

#include "nm_mgr_ui.h"
#include "ui_input.h"
#include "nm_mgr_print.h"
#include "metadata.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif

int gContext;


void ui_build_control(agent_t* agent)
{
	ari_t *id = NULL;
	uvast ts;
	msg_ctrl_t *msg;

	AMP_DEBUG_ENTRY("ui_build_control","("ADDR_FIELDSPEC")", (uaddr)agent);

	CHKVOID(agent);

	ts = ui_input_uint("Control Timestamp");
	if((id = ui_input_ari("Control MID:", ADM_ENUM_ALL, AMP_TYPE_CTRL)) == NULL)
	{
		AMP_DEBUG_ERR("ui_build_control","Can't get control.",NULL);
		return;
	}

	ui_postprocess_ctrl(id);

	if((msg = msg_ctrl_create_ari(id)) != NULL)
	{
		msg->start = ts;
		iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);
		msg_ctrl_release(msg, 1);
	}
	else
	{
		ari_release(id, 1);
	}
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
 *  10/06/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/
void ui_clear_reports(agent_t* agent)
{
	CHKVOID(agent);

	gMgrDB.tot_rpts -= vec_num_entries(agent->rpts);

	vec_clear(&(agent->rpts));
}


/******************************************************************************
 *
 * \par Function Name: ui_create_rpttpl_from_rpt_parms
 *
 * \par Release resources associated with a report template.
 *
 * \param[in|out]  rpttpl  The template to be released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/09/18  E. Birrane     Initial implementation.
 *  10/06/18  E. Birrane     Update for AMP v0.5 (JHU/APL)
 *****************************************************************************/
rpttpl_t *ui_create_rpttpl_from_parms(tnvc_t parms)
{
	rpttpl_t *result = NULL;

	ari_t *ari = (ari_t *) adm_get_parm_obj(&parms, 0, AMP_TYPE_ARI);
	ac_t *ac = (ac_t *) adm_get_parm_obj(&parms, 1, AMP_TYPE_AC);

	CHKNULL(ari);
	CHKNULL(ac);

	ari_t *a1 = ari_copy_ptr(*ari);
	ac_t ac2 = ac_copy(ac);

	if((result = rpttpl_create(a1, ac2)) == NULL)
	{
		ari_release(a1, 1);
		ac_release(&ac2, 0);
		result = NULL;
	}

	return result;
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
	CHKVOID(agent);
	AMP_DEBUG_ENTRY("ui_deregister_agent","(%s)",agent->eid.name);
	vec_del(&(gMgrDB.agents), agent->idx);
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
	char choice;

	int context = UI_MAIN_MENU;


	while(*running)
	{
		switch(context)
		{
			case UI_MAIN_MENU:  ui_print_menu_main();  break;
			case UI_ADMIN_MENU: ui_menu_admin_show(); break;
			case UI_CTRL_MENU:  ui_menu_ctrl_show();  break;
			case UI_RPT_MENU:   ui_menu_rpt_show();   break;
#ifdef HAVE_MYSQL
			case UI_DB_MENU:    ui_menu_sql_show();    break;
#endif
			default: printf("Error. Unknown menu context.\n"); break;
		}

		choice = ui_input_byte(">");
		choice = toupper(choice);

		switch(context)
		{
			case UI_MAIN_MENU:
				switch(choice)
				{
					case '1' : context = UI_ADMIN_MENU; break;
					case '2' : context = UI_RPT_MENU; break;
					case '3' : context = UI_CTRL_MENU; break;
#ifdef HAVE_MYSQL
					case '4' : context = UI_DB_MENU; break;
#endif
					case 'Z' : *running = 0; return; break;
					default: printf("Unknown command.\n");break;
				}
				break;

			case UI_ADMIN_MENU:
				context = ui_menu_admin_do(choice);
				break;

			case UI_CTRL_MENU:
				context = ui_menu_ctrl_do(choice);
				break;

			case UI_RPT_MENU:
				context = ui_menu_rpt_do(choice);
				break;

#ifdef HAVE_MYSQL
			case UI_DB_MENU:
				context = ui_menu_sql_do(choice);
				break;
#endif

			default: printf("Error. Unknown menu context: %d.\n", context); break;
		}
	}
}



void ui_list_objs()
{
	uint8_t adm_id = 0;
	uint8_t type = 0;
	int i = 0;

	meta_col_t *col = NULL;
	metadata_t *meta = NULL;
	vecit_t it;

	adm_id = ui_input_adm_id("");
	printf("Enter the AMP Object Type:\n");

	type = ui_input_ari_type();
	col =  meta_filter(adm_id, type);

	for(it = vecit_first(&(col->results)); vecit_valid(it); it = vecit_next(it))
	{
		meta = vecit_data(it);

		printf("%d) %s\t%s\n", i++, meta->name, meta->descr);
	}

	metacol_release(col, 1);
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

void ui_postprocess_ctrl(ari_t *id)
{
	metadata_t *meta;

	CHKVOID(id);
	CHKVOID(id->type == AMP_TYPE_CTRL);

	// TODO: Put locks around these retrieve calls.
	meta = rhht_retrieve_key(&(gMgrDB.metadata), id);

	CHKVOID(meta);

	if(strcmp(meta->name, AGENT_ADD_VAR_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_DEL_VAR_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_ADD_RPTT_STR) == 0)
	{
		rpttpl_t *def = ui_create_rpttpl_from_parms(id->as_reg.parms);
		if(def != NULL)
		{
			VDB_ADD_RPTT(def->id, def);
			db_persist_rpttpl(def);
		}
	}
	else if(strcmp(meta->name, AGENT_DEL_RPTT_STR) == 0)
	{
		rpttpl_t *def = VDB_FINDKEY_RPTT(id);
		if(def != NULL)
		{
			db_forget(&(def->desc), gDB.rpttpls);
			VDB_DELKEY_RPTT(id);
		}
	}
	else if(strcmp(meta->name, AGENT_ADD_MAC_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_DEL_MAC_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_ADD_SBR_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_DEL_SBR_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_ADD_TBR_STR) == 0)
	{

	}
	else if(strcmp(meta->name, AGENT_DEL_TBR_STR) == 0)
	{

	}

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
	agent_add(agent_eid);

	AMP_DEBUG_EXIT("register_agent", "->.", NULL);
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
 *  07/04/16  E. Birrane     Auto-select if only 1 agent known.
 *  10/07/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/
agent_t* ui_select_agent()
{
	char line[10];
	int idx = -1;
	int total;
	agent_t *agent = NULL;

	printf("Select an Agent:");
	total = ui_print_agents();

	if(total == 0)
	{
		AMP_DEBUG_ERR("ui_select_agent", "No agents registered.\n", NULL);
		return NULL;
	}

	if(total == 1)
	{
		idx = 0;
		printf("Auto-selecting sole known agent.");
	}
	else if((idx = ui_input_int("Agent (#), or 0 to cancel:")) == 0)
	{
		AMP_DEBUG_ERR("ui_select_agent","No agent selected.", NULL);
		return NULL;
	}

	if((agent = vec_at(gMgrDB.agents, idx-1)) == NULL)
	{
		AMP_DEBUG_ERR("ui_select_agent","Error selecting agent #%d", idx);
		return NULL;
	}

	return agent;
}



void ui_send_file(agent_t* agent, uint8_t enter_ts)
{
	ari_t *cur_id = NULL;
	uint32_t offset = 0;
	time_t ts = 0;
	blob_t *contents = NULL;
	char *cursor = NULL;
	char *saveptr = NULL;
	uint32_t bytes = 0;
	blob_t *value = NULL;
	int success;

	CHKVOID(agent);

	ts = (enter_ts) ? ui_input_uint("Control Timestamp") : 0;


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

		if((value = utils_string_to_hex(cursor)) == NULL)
		{
			AMP_DEBUG_ERR("ui_send_file", "Can't make value from %s", cursor);
			blob_release(contents, 1);
			return;
		}

		cur_id = ari_deserialize_raw(value, &success);
		blob_release(value, 1);
		if(cur_id == NULL)
		{
			AMP_DEBUG_ERR("ui_send_file", "Can't make mid from %s", cursor);
			blob_release(contents, 1);
			return;
		}

		//TODO: Make next several lines a helper function.
		ui_postprocess_ctrl(cur_id);
		msg_ctrl_t *msg;
		if((msg = msg_ctrl_create(cur_id)) == NULL)
		{
			ari_release(cur_id, 1);
			return;
		}

		msg->start = ts;
		iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);

		msg_ctrl_release(msg, 1);
		cursor = strtok_r(NULL, "\n", &saveptr);
	}

	blob_release(contents, 1);

	AMP_DEBUG_EXIT("ui_send_file","->.", NULL);
}




void ui_send_raw(agent_t* agent, uint8_t enter_ts)
{
	ari_t *id = NULL;
	time_t ts = 0;
	msg_ctrl_t *msg = NULL;

	CHKVOID(agent);


	ts = (enter_ts) ? ui_input_uint("Control Timestamp") : 0;

	printf("Enter raw MID to send.\n");
	id = ui_input_ari_raw(1);

	if(id == NULL)
	{
		AMP_DEBUG_ERR("ui_send_raw","Can't get control MID.",NULL);
		return;
	}

	ui_postprocess_ctrl(id);

	if((msg = msg_ctrl_create(id)) == NULL)
	{
		ari_release(id, 1);
		return;
	}
	msg->start = ts;
	iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);

	msg_ctrl_release(msg, 1);
}













void ui_menu_admin_show()
{
	printf("============ Administration Menu =============\n");

	printf("\n------------ Agent Registration ------------\n");
	printf("1) Register Agent.\n");
	printf("2) List Registered Agent.\n");
	printf("3) De-Register Agent.\n");

	printf("\n--------------------------------------------\n");
	printf("Z) Return to Main Menu.\n");

}

int ui_menu_admin_do(uint8_t choice)
{
	int context = UI_ADMIN_MENU;
	switch(choice)
	{
		case 'Z' : context = UI_MAIN_MENU; break;
		case '1' : ui_register_agent(); break;
		case '2' : ui_print_agents(); break;
		case '3' : ui_deregister_agent(ui_select_agent()); break;
		default: printf("Unknown command: %c.\n", choice); break;
	}
	return context;
}



void ui_menu_ctrl_show()
{
	printf("=============== Controls Menu ================\n");

	printf("\n------------- AMM Object Information --------------\n");
	printf("0) List supported ADMs.\n");
	printf("1) List Atomics (EDD, CNST, LIT) (%d Known)\n", gVDB.adm_atomics.num_elts);
	printf("2) List Control Definitions      (%d Known)\n", gVDB.adm_ctrl_defs.num_elts);
	printf("3) List Macro Definitions        (%d Known)\n", gVDB.macdefs.num_elts);
	printf("4) List Operator Definitions     (%d Known)\n", gVDB.adm_ops.num_elts);
	printf("5) List Rpt Template Definitions (%d Known)\n", gVDB.rpttpls.num_elts);
	printf("6) List Rules                    (%d Known)\n", gVDB.rules.num_elts);
	printf("7) List Tbl Template Definitions (%d Known)\n", gVDB.adm_tblts.num_elts);
	printf("8) List Variables                (%d Known)\n", gVDB.vars.num_elts);

	printf("\n-------------- Perform Control -------------\n");
	printf("9) Build Arbitrary Control.\n");
	printf("A) Specify Raw Control.\n");
	printf("B) Run Control File.\n");

	printf("\n--------------------------------------------\n");
	printf("Z) Return to Main Menu.\n");
}


int ui_menu_ctrl_do(uint8_t choice)
{
	int context = UI_CTRL_MENU;
	switch(choice)
	{
		case '0' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_UNK);
				   break;

		case '1' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_EDD);
				   ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_CNST);
				   ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_LIT);
				   break;

		case '2' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_CTRL);
				   break;

		case '3' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_MAC);
				   break;

		case '4' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_OPER);
				   break;

		case '5' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_RPTTPL);
				   break;

		case '6' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_SBR);
				   ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_TBR);
				   break;

		case '7' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_TBLT);
				   break;

		case '8' : ui_list_objs(ADM_ENUM_ALL, AMP_TYPE_VAR);
				   break;

		case '9' : ui_build_control(ui_select_agent()); break;
		case 'A' : ui_send_raw(ui_select_agent(),0); break;
		case 'B' : ui_send_file(ui_select_agent(),0); break;

		case 'Z' : context = UI_MAIN_MENU; break;
		default: printf("Unknown command %c.\n", choice); break;
	}
	return context;
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

void ui_menu_rpt_show()
{

	printf("========================= Reporting Menu =========================\n");

	printf("\n--------------------------- Data List --------------------------\n");
	printf("1) List Agent Computed Data Definitions.\n");

	printf("\n------------------------ Definitions List ----------------------\n");
	printf("2) List Agent Custom Report Definition.\n");
	printf("3) List Agent Macro Definitions.\n");

	printf("\n-------------------------- Report List -------------------------\n");
	printf("4) Print Reports Received from an Agent (We have %ld reports).\n", gMgrDB.tot_rpts);
	printf("5) Clear Reports Received from an Agent.\n");

	printf("\n---------------------- Production Schedules --------------------\n");
	printf("6) List Agent Production Rules.\n");

	printf("------------------------------------------------------------------\n");
	printf("Z) Return to Main Menu.\n");
}

int ui_menu_rpt_do(uint8_t choice)
{
	int context = UI_RPT_MENU;
	switch(choice)
	{
		// Definitions List
		case '1' : ui_print_nop(); break; //ui_print_agent_comp_data_def(); break; // LIst agent computed data defs
		case '2' : ui_print_nop(); break; //ui_print_agent_cust_rpt_defs(); break; // List agent custom report defs
		case '3' : ui_print_nop(); break; //ui_print_agent_macro_defs();    break; // LIst agent macro defs.

		// Report List
		case '4' : ui_print_nop(); break; //ui_print_reports(ui_select_agent());   break; // Print received reports.
		case '5' : ui_print_nop(); break; //ui_clear_reports(ui_select_agent());	break; // Clear received reports.

		// Production Schedules.
		case '6' : ui_print_nop(); break; //ui_print_agent_prod_rules();    break; // List agent production rules.

		case 'Z' : context = UI_MAIN_MENU;				break;

		default: printf("Unknown command.\n");			break;
	}
	return context;
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

int ui_menu_sql_do(uint8_t choice)
{
	int context = UI_DB_MENU;
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
	  case 'Z' : context = UI_MAIN_MENU;				break;

	  default: printf("Unknown command %d.\n", choice);			break;
	}
	return context;
}

void ui_menu_sql_show()
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


