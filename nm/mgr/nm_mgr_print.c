/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file nm_mgr_print.h
 **
 **
 ** Description: Helper file holding functions for printing menus and AMP data
 **              to the screen.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/10/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  11/01/18  E. Birrane     Update to latest AMP. (JHU/APL)
 *****************************************************************************/

#include "nm_mgr_ui.h"
#include "nm_mgr_print.h"
#include "agents.h"
#include "ui_input.h"

#ifdef USE_JSON
#include "cJSON.h"

// JSON Prototypes (TODO: Move to header and/or discrete file)
cJSON* ui_json_from_tnvc(tnvc_t *tnvc);
cJSON *ui_json_from_tnv(tnv_t *tnv);
#endif

#include "../shared/utils/utils.h"
#include "../shared/primitives/blob.h"
#include "../shared/primitives/table.h"

#ifdef USE_NCURSES
#include <menu.h>
#endif

ui_menu_list_t agent_submenu_list[] = {
   {"De-register agent", NULL, NULL},
   {"Build Control", NULL, NULL},
   {"Send Raw Command", NULL, NULL},
   {"Send Command File", NULL, NULL},
   {"Print Agent Reports", NULL, NULL},
   {"Print Agent Tables", NULL, NULL},
   {"Write Agent Reports to file", NULL, NULL},
   {"Clear Agent Reports", NULL, NULL},
   {"Clear Agent Tables", NULL, NULL},
};

static int ui_print_agents_cb_parse(int idx, int keypress, void* data, char* status_msg);
static void ui_print_report_entry(ui_print_cfg_t *fd, char *name, tnv_t *val);

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
 *  07/04/16  E. Birrane     Correct return value and agent casting.
 *  10/07/18  E. Birrane     Update top AMP v0.5 (JHU/APL)
 *****************************************************************************/
ui_cb_return_values_t ui_print_agents_cb_fn(int idx, int keypress, void* data, char* status_msg) {
   ui_cb_return_values_t rtv = UI_CB_RTV_CONTINUE;
   agent_t *agent = (agent_t*)data;

#ifdef USE_NCURSES
   if (keypress == KEY_ENTER || keypress == 10)
   {
      // User selected an agent. Let's loop on the sub-menu
      while(rtv == UI_CB_RTV_CONTINUE )
      {
         keypress = ui_menu_listing(agent->eid.name,
                                    agent_submenu_list, ARRAY_SIZE(agent_submenu_list),
                                    NULL, 0,
                                    "F1 or 'e' to cancel. Arrow keys to navigate and enter to select. (x) indicates key that can be pressed directly from agent listing (parent) menu to perform this action.",
                                    NULL,
                                    UI_OPT_AUTO_LABEL | UI_OPT_ENTER_SEL);
         rtv = ui_print_agents_cb_parse(idx, keypress, data, status_msg);
      }
   }
   else
   {
      return ui_print_agents_cb_parse(idx, keypress, data, status_msg);
   }
#else
   while(rtv == UI_CB_RTV_CONTINUE)
   {
         keypress = ui_menu_listing(agent->eid.name,
                                    agent_submenu_list, ARRAY_SIZE(agent_submenu_list),
                                    NULL, 0,
                                    NULL,
                                    NULL,
                                    UI_OPT_AUTO_LABEL | UI_OPT_ENTER_SEL);
      rtv = ui_print_agents_cb_parse(idx, keypress, data, status_msg);
   }
#endif
   return rtv;
}

static int ui_print_agents_cb_parse(int idx, int keypress, void* data, char* status_msg) {
   int choice;
   int rtv = UI_CB_RTV_CONTINUE;
   agent_t *agent = (agent_t*)data;
   char *subtitle = "";
   char *tmp;
   
   switch(keypress)
   {
   case 'e':
   case 'E':
   case -1:
      return UI_CB_RTV_ERR; // Exit Listing
   case 'd':
   case 'D':
   case 0:
      choice = ui_prompt("Delete selected agent?", "Yes", "No", NULL);
      if (choice == 0)
      {
         ui_deregister_agent(agent);
         return UI_CB_RTV_ERR; // Exit from parent menu to force agent listing to be refreshed.
      }
      break;
   case 'b':
   case 'B':
   case 1:
      choice = ui_build_control(agent);
      if (status_msg != NULL)
      {
         if (choice == 1)
         {
            sprintf(status_msg, "Succcessfully built & sent control to '%s'", agent->eid.name);
         }
         else
         {
            sprintf(status_msg, "Build Control aborted or transmission to '%s' failed", agent->eid.name);
         }
         return UI_CB_RTV_STATUS;
      }
      break;
   case 'r':
   case 'R':
   case 2:
      ui_send_raw(agent,0);
      break;
   case 'f':
   case 'F':
   case 3:
      ui_send_file(agent,0);
      break;
   case 'p':
   case 'P':
   case 4:
      ui_print_report_set(agent);
      break;
   case 't':
   case 5:
      ui_print_table_set(agent);
      break;
   case 'w':
   case 'W':
   case 6:
      tmp = ui_input_string("Enter file name");
      if (tmp)
      {
         // Redirect the next display page to a file
         ui_display_to_file(tmp);
         SRELEASE(tmp);

         // Print reports. ui_display_exec() will automatically close the file
         ui_print_report_set(agent);
      }
      break;
   case 'c':
   case 'C':
   case 7:
      // clear agent reports
      choice = ui_prompt("Clear reports for this agent?", "Yes", "No", NULL);
      if (choice == 0)
      {
         ui_clear_reports(agent);
         return UI_CB_RTV_ERR; // Return from parent to force update of report counts in display
      }
      break;
   case 'T':
   case 8:
      // clear agent tables
      choice = ui_prompt("Clear tables for this agent?", "Yes", "No", NULL);
      if (choice == 0)
      {
         ui_clear_tables(agent);
         return UI_CB_RTV_ERR; // Return from parent to force update of report counts in display
      }
      break;
   }

   // Default behavior is to return 0, indicating parent menu should continue on.
   return UI_CB_RTV_CONTINUE;
}

int ui_print_agents()
{
  vecit_t it;
  int i = 0, tmp, tmp2;
  int num_agents = vec_num_entries(gMgrDB.agents);
  agent_t * agent = NULL;
  ui_menu_list_t* list;
  char status_msg[80] = "";

  if(num_agents == 0)
  {
	  printf("[None]\n");
	  return 0;
  }

  list = calloc(num_agents, sizeof(ui_menu_list_t) );

  for(it = vecit_first(&(gMgrDB.agents)); vecit_valid(it); it = vecit_next(it))
  {
	  agent = (agent_t *) vecit_data(it);
      list[i].name = agent->eid.name;

      tmp = vec_num_entries(agent->rpts);
      tmp2 = vec_num_entries(agent->tbls);
      if (tmp > 0)
      {
         list[i].description = malloc(32);
         snprintf(list[i].description, 32, "%d reports & %d tables", tmp, tmp2);
      }
      else
      {
         // Nothing to describe except number of reports
         list[i].description = NULL;
      }

      
      list[i].data = (void*)agent;
      i++;
  }

  ui_menu_listing("Known Agents", list, num_agents, status_msg, 0,
#ifdef USE_NCURSES
                  "F1 to exit, Enter for Agent actions menu & additional usage informationo",
#else
                  NULL,
#endif
                  ui_print_agents_cb_fn,
                  UI_OPT_AUTO_LABEL);
  
  for(i = 0; i < num_agents; i++)
  {
     if (list[i].description != NULL)
     {
        free(list[i].description);
     }
  }
  
  free(list);
  return num_agents;
}

#ifdef USE_JSON
/* Convert a TBL into cJSON */
cJSON* ui_json_table(tbl_t *tbl)
{
   size_t num_rows;
   tblt_t *tblt;
   vecit_t it;
   cJSON *out_tbl;
   
   CHKNULL(tbl);
   
   tblt = VDB_FINDKEY_TBLT(tbl->id);
   CHKNULL(tblt);

   out_tbl = cJSON_CreateObject();
   cJSON *cols = cJSON_AddArrayToObject(out_tbl, "cols");
   cJSON *rows = cJSON_AddArrayToObject(out_tbl, "rows");

   // Build Columns Description
   for(it = vecit_first(&tblt->cols); vecit_valid(it); it = vecit_next(it))
   {
      tblt_col_t *col = (tblt_col_t*) vecit_data(it);
      cJSON *jcol = cJSON_CreateObject();
      
      if (col != NULL) {
         cJSON_AddStringToObject(jcol, "name", col->name);
         cJSON_AddStringToObject(jcol, "type", type_to_str(col->type) );
      } // Else we add an empty object
      cJSON_AddItemToArray(cols, jcol);
   }

   // For each row (if there are no rows, we will return an empty array)
   num_rows = tbl_num_rows(tbl);
   for(int i = 0; i < num_rows; i++)
   {
      	tnvc_t *cur_row = tbl_get_row(tbl, i);
        cJSON *obj = cJSON_CreateArray();
        vecit_t it;
        int j;

        // For each column; Simultaneously iterate on data col and title columns
        for(j = 0; j < tnvc_get_count(cur_row); j++)
        {
           tnv_t *val = tnvc_get(cur_row, j);

           cJSON_AddItemToArray(obj,
                                 ui_json_from_tnv(val)
              );
        }

        cJSON_AddItemToArray(rows, obj);
   }
   
   
   return out_tbl;
}

void ui_fprint_json_table(ui_print_cfg_t *fd, tbl_t *table)
{
   cJSON *json = ui_json_table(table);
   if (json == NULL)
   {
      return;
   }
   else
   {
      char *str = cJSON_Print(json);
      ui_fprintf(fd, str);
      cJSON_free(str);
      cJSON_Delete(json);
   }
}

#endif

void ui_fprint_table(ui_print_cfg_t *fd, tbl_t *tbl)
{
   char *tmp = ui_str_from_tbl(tbl);
   if (tmp != NULL) {
      ui_fprintf(fd,"%s\n", tmp);
      SRELEASE(tmp);
   }
   else
   {
      ui_fprintf(fd, "***ERROR: Unable to prepare Table for output\n");
   }
}

void ui_fprint_report(ui_print_cfg_t *fd, rpt_t *rpt)
{
	int num_entries = 0;
	tnv_t *val = NULL;
	char name[META_NAME_MAX+3];
	metadata_t *rpt_info = NULL;
	metadata_t *entry_info = NULL;

	if((rpt == NULL) || (rpt->id == NULL))
	{
		return;
	}

	/* Step 1: Print the report banner. This will include the report
	 *         name.
	 */
	rpt_info = rhht_retrieve_key(&(gMgrDB.metadata), rpt->id);
    num_entries = tnvc_get_count(rpt->entries);

    ui_fprintf(fd,"\n----------------------------------------");
    ui_fprintf(fd,"\n             AMP DATA REPORT            ");
    ui_fprintf(fd,"\n----------------------------------------");
    ui_fprintf(fd,"\nSent to   : %s", rpt->recipient.name);

    if(rpt_info == NULL)
    {
    	char *rpt_str = ui_str_from_ari(rpt->id, NULL, 0);
        ui_fprintf(fd,"\nRpt Name  : %s", (rpt_str == NULL) ? "Unknown" : rpt_str);
        SRELEASE(rpt_str);
    }
    else
    {
    	if(vec_num_entries(rpt->id->as_reg.parms.values) > 0)
    	{
    		char *parm_str = ui_str_from_tnvc(&(rpt->id->as_reg.parms));
    		ui_fprintf(fd,"\nRpt Name  : %s(%s)", rpt_info->name, (parm_str == NULL) ? "" : parm_str);
    		SRELEASE(parm_str);
    	}
    	else
    	{
    		ui_fprintf(fd,"\nRpt Name  : %s", rpt_info->name);
    	}
    }
    ui_fprintf(fd,"\nTimestamp : %s", ctime(&(rpt->time)));
    ui_fprintf(fd,"\n# Entries : %d", num_entries);
    ui_fprintf(fd,"\n----------------------------------------\n");


    /* Step 2: Print individual entries, based on type. */

	if(rpt->id->type == AMP_TYPE_RPTTPL)
	{
		int i = 0;
		rpttpl_t *tpl = VDB_FINDKEY_RPTT(rpt->id);

		if((tpl != NULL) && (ac_get_count(&(tpl->contents)) != num_entries))
		{
			AMP_DEBUG_ERR("ui_print_report",
					      "Template mismatch. Expected %d entries but have %d. Not using template.",
						  ac_get_count(&(tpl->contents)),
						  num_entries);
			tpl = NULL;
		}

		for(i = 0; i < num_entries; i++)
		{
			ari_t *entry_id = (tpl == NULL) ? NULL : ac_get(&(tpl->contents), i);
			entry_info = (entry_id == NULL) ? NULL : rhht_retrieve_key(&(gMgrDB.metadata), entry_id);
			val = tnvc_get(rpt->entries, i);

			if(entry_info == NULL)
			{
				sprintf(name, "Entry %d", i);
			}
			else
			{
				tnvc_t *parms = ari_resolve_parms(&(entry_id->as_reg.parms), &(rpt->id->as_reg.parms));
				char *parm_str = NULL;

				if(parms != NULL)
				{
					if(vec_num_entries(parms->values) > 0)
					{
						parm_str = ui_str_from_tnvc(parms);
					}
					tnvc_release(parms, 1);
				}

				if(parm_str != NULL)
				{
                   if(snprintf(name, sizeof(name),"%s(%s)", entry_info->name, parm_str) < 0) {
                      snprintf(name, sizeof(name), "%s(.)", entry_info->name);
                   }
					SRELEASE(parm_str);
				}
				else
				{
                   strncpy(name, entry_info->name, sizeof(name));
				}
			}

			ui_print_report_entry(fd, name, val);
			ui_fprintf(fd,"\n");
		}
	}
	else if(rpt->id->type == AMP_TYPE_TBLT)
	{
		int i = 0;
		vecit_t it;
		tbl_t *tbl = NULL;

		for(i = 0; i < num_entries; i++)
		{
			val = tnvc_get(rpt->entries, i);
			if(val->type == AMP_TYPE_TBL)
			{
				tbl = (tbl_t *) val->value.as_ptr;
                ui_fprint_table(fd, tbl);
			}
		}
	}
	else
	{
		if(rpt_info == NULL)
		{
			sprintf(name, "Entry 1");
		}
		else
		{
			sprintf(name, "%.30s", rpt_info->name);
		}

		val = tnvc_get(rpt->entries, 0);

		ui_print_report_entry(fd, name, val);
		ui_fprintf(fd,"\n");
	}


	/* Step 3: Print report trailer. */
    ui_fprintf(fd,"\n----------------------------------------\n\n");
}

#ifdef USE_JSON
/** Output a report in JSON format
 * @returns A cJSON object representing the given report.  
 *   Caller is responsible for freeing with cJSON_Delete
 */
cJSON* ui_json_report(rpt_t *rpt)
{
	int num_entries = 0;
	tnv_t *val = NULL;
	char name[META_NAME_MAX+3];
	metadata_t *rpt_info = NULL;
	metadata_t *entry_info = NULL;
    cJSON *rtv = cJSON_CreateObject();

	if((rtv == NULL || rpt == NULL) || (rpt->id == NULL))
	{
		return NULL;
	}

	/* Step 1: Print the report banner. This will include the report
	 *         name.
	 */
	rpt_info = rhht_retrieve_key(&(gMgrDB.metadata), rpt->id);
    num_entries = tnvc_get_count(rpt->entries);

    cJSON_AddNumberToObject(rtv, "num_entries", num_entries);
    cJSON_AddStringToObject(rtv, "recipient_name", rpt->recipient.name);

    if(rpt_info == NULL)
    {
    	char *rpt_str = ui_str_from_ari(rpt->id, NULL, 0);
        cJSON_AddStringToObject(rtv, "name", (rpt_str == NULL) ? "Unknown" : rpt_str);
        SRELEASE(rpt_str);
    }
    else
    {
       cJSON_AddStringToObject(rtv, "name", rpt_info->name);                                       

       if(vec_num_entries(rpt->id->as_reg.parms.values) > 0)
       {
          cJSON* obj = ui_json_from_tnvc(&(rpt->id->as_reg.parms));
          if (obj != NULL) {
             cJSON_AddItemToObject(rtv,
                                "arguments",
                                   obj);
          }
       }
    }
    cJSON_AddStringToObject(rtv, "timestamp", ctime(&(rpt->time)));

    /* Step 2: Print individual entries, based on type. */
	if(rpt->id->type == AMP_TYPE_RPTTPL)
	{
		int i = 0;
		rpttpl_t *tpl = VDB_FINDKEY_RPTT(rpt->id);
        cJSON *entries = cJSON_AddObjectToObject(rtv, "entries");
        cJSON_AddStringToObject(rtv, "type", "rpttpl");

		if((tpl != NULL) && (ac_get_count(&(tpl->contents)) != num_entries))
		{
			AMP_DEBUG_ERR("ui_print_report",
					      "Template mismatch. Expected %d entries but have %d. Not using template.",
						  ac_get_count(&(tpl->contents)),
						  num_entries);
			tpl = NULL;
		}


		for(i = 0; i < num_entries; i++)
		{
			ari_t *entry_id = (tpl == NULL) ? NULL : ac_get(&(tpl->contents), i);
			entry_info = (entry_id == NULL) ? NULL : rhht_retrieve_key(&(gMgrDB.metadata), entry_id);
			val = tnvc_get(rpt->entries, i);
            
			if(entry_info == NULL)
			{
				sprintf(name, "Entry %d", i);
			}
			else
			{
				tnvc_t *parms = ari_resolve_parms(&(entry_id->as_reg.parms), &(rpt->id->as_reg.parms));
				char *parm_str = NULL;

				if(parms != NULL)
				{
					if(vec_num_entries(parms->values) > 0)
					{
						parm_str = ui_str_from_tnvc(parms);
					}
					tnvc_release(parms, 1);
				}

				if(parm_str != NULL)
				{
                   if(snprintf(name, sizeof(name),"%s(%s)", entry_info->name, parm_str) < 0) {
                      snprintf(name, sizeof(name), "%s(.)", entry_info->name);
                   }
					SRELEASE(parm_str);
				}
				else
				{
                   strncpy(name, entry_info->name, sizeof(name));
				}
			}

            cJSON_AddItemToObject(entries, name, ui_json_from_tnv(val));
		}
        
	}
	else if(rpt->id->type == AMP_TYPE_TBLT)
	{
		int i = 0;
		vecit_t it;
		tbl_t *tbl = NULL;
        cJSON *entries = cJSON_AddArrayToObject(rtv, "entries");
                cJSON_AddStringToObject(rtv, "type", "tblt");
		for(i = 0; i < num_entries; i++)
		{
			val = tnvc_get(rpt->entries, i);
			if(val->type == AMP_TYPE_TBL)
			{
				tbl = (tbl_t *) val->value.as_ptr;
                cJSON_AddItemToObject(rtv, "table", ui_json_table(tbl));                
			}
		}
	}
	else
	{
       cJSON *entries = cJSON_AddArrayToObject(rtv, "entries");
       cJSON_AddStringToObject(rtv, "type", "unknown");
       
		if(rpt_info == NULL)
		{
           cJSON_AddItemToObject(entries,
                                 "(Entry 1)",
                                 ui_json_from_tnvc(rpt->entries)
           );
		}
		else
		{
           cJSON_AddItemToObject(entries,
                                 rpt_info->name,
                                 ui_json_from_tnvc(rpt->entries)
           );
		}
	}


	/* Step 3: Add report to object */
    return rtv;
}
void ui_fprint_json_report(ui_print_cfg_t *fd, rpt_t *rpt)
{
   cJSON *json = ui_json_report(rpt);
   if (json == NULL)
   {
      return;
   }
   else
   {
      char *str = cJSON_Print(json);
      ui_fprintf(fd, str);
      ui_fprintf(fd, "\n");
      cJSON_free(str);
      cJSON_Delete(json);
   }
}
#endif


static void ui_print_report_entry(ui_print_cfg_t *fd, char *name, tnv_t *val)
{
	if(val == NULL)
	{
		ui_fprintf(fd,"%s: null", name);
		return;
	}

	char *str = ui_str_from_tnv(val);
	if(name)
	{
		ui_fprintf(fd,"%s : %s", name, (str == NULL) ? "null" : str);
	}
	else
	{
		ui_fprintf(fd,"%s", (str == NULL) ? "null" : str);
	}

	SRELEASE(str);
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
 *  11/01/18  E. Birrane     Update for AMP v0.5 (JHU/APL)
 *****************************************************************************/
void ui_print_report_set(agent_t* agent)
{
   rpt_t *cur_report = NULL;
   vecit_t rpt_it;
   int num_rpts;
   char title[40];

   if(agent == NULL)
   {
	   return;
   }

   num_rpts = vec_num_entries(agent->rpts);

   snprintf(title, 39, "Agent Reports for %s", agent->eid.name);
   ui_display_init(title);

   if (num_rpts == 0)
   {
      ui_printf("No reports received from this agent");
      AMP_DEBUG_ALWAYS("ui_print_reports","[No reports received from this agent.]", NULL);
      ui_display_exec();
      return;
   }

   /* Iterate through all reports for this agent. */
   for(rpt_it = vecit_first(&(agent->rpts)); vecit_valid(rpt_it); rpt_it = vecit_next(rpt_it))
   {
      ui_print_report((rpt_t*)vecit_data(rpt_it));
   }
   ui_display_exec();
}

void ui_print_table_set(agent_t* agent)
{
   tbl_t *cur_report = NULL;
   vecit_t tbl_it;
   int num_tbls;
   char title[40];

   if(agent == NULL)
   {
	   return;
   }

   num_tbls = vec_num_entries(agent->tbls);

   snprintf(title, 39, "Agent Tables for %s", agent->eid.name);
   ui_display_init(title);

   if (num_tbls == 0)
   {
      ui_printf("No tables received from this agent");
      AMP_DEBUG_ALWAYS("ui_print_tables","[No tables received from this agent.]", NULL);
      ui_display_exec();
      return;
   }

   /* Iterate through all reports for this agent. */
   for(tbl_it = vecit_first(&(agent->tbls)); vecit_valid(tbl_it); tbl_it = vecit_next(tbl_it))
   {
      ui_print_table((tbl_t*)vecit_data(tbl_it));
   }
   ui_display_exec();

}


char *ui_str_from_ac(ac_t *ac)
{
	char *str = STAKE(1024);
	vecit_t it;

	for(it = vecit_first(&(ac->values)); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *id = (ari_t*) vecit_data(it);
		char *alt_str = ui_str_from_ari(id, NULL, 0);
		strcat(str, (alt_str==NULL) ? "null" : alt_str);
		strcat(str, " ");
		SRELEASE(alt_str);
	}

	return str;
}

char *ui_str_from_ari(ari_t *id, tnvc_t *ap, int desc)
{
	metadata_t *meta = NULL;
	ari_t *print_id = NULL;
	char *str = STAKE(1024);

	CHKNULL(str);

	if(id == NULL)
	{
		strcat(str,"null");
		return str;
	}

	if(id->type == AMP_TYPE_LIT)
	{
		SRELEASE(str);
		return ui_str_from_tnv(&(id->as_lit));
	}

	meta = rhht_retrieve_key(&(gMgrDB.metadata), id);

	if(ap != NULL)
	{
		print_id = ari_copy_ptr(id);
		ari_replace_parms(print_id, ap);
	}
	else
	{
		print_id = id;
	}

	if(meta == NULL)
	{
		blob_t *blob = ari_serialize_wrapper(print_id);

		if(blob != NULL)
		{
			char *nm_str = utils_hex_to_string(blob->value, blob->length);
			blob_release(blob, 1);
			sprintf(str, "Anonymous ARI: %s", nm_str);
			SRELEASE(nm_str);
		}
		else
		{
			sprintf(str, "NULL ARI");
		}
	}
	else
	{
		char *parm_str = NULL;

		/* If we have actual parameters, print those. */
		if(ap != NULL)
		{
			parm_str = ui_str_from_tnvc(ap);
		}
		else if (vec_num_entries(meta->parmspec) > 0)
		{
			parm_str = ui_str_from_fp(meta);
		}

		if(parm_str != NULL)
		{
			sprintf(str,"%s(%s)", meta->name, parm_str);
			SRELEASE(parm_str);
		}
		else
		{
			sprintf(str,"%s", meta->name);
		}

		if(desc)
		{
			strcat(str,"\t: ");
			strcat(str,meta->descr);
		}
	}

	if(print_id != id)
	{
		ari_release(print_id, 1);
	}

	return str;
}


char *ui_str_from_blob(blob_t *blob)
{
	CHKNULL(blob);
	return utils_hex_to_string(blob->value, blob->length);
}


char *ui_str_from_ctrl(ctrl_t *ctrl)
{
	return ui_str_from_ari(ctrl->def.as_ctrl->ari, ctrl->parms, 0);
}


char *ui_str_from_edd(edd_t *edd)
{
	return ui_str_from_ari(edd->def.id, edd->parms, 0);
}

char *ui_str_from_expr(expr_t *expr)
{
	char *str = STAKE(1024);
	CHKNULL(expr);
	CHKNULL(str);

	char *alt_str = ui_str_from_ac(&(expr->rpn));
	snprintf(str, 1023, "EXPR: (%s) %s", type_to_str(expr->type), (alt_str == NULL) ? "null" : alt_str);
	SRELEASE(alt_str);
	return str;
}


char *ui_str_from_fp(metadata_t *meta)
{
	char *str = STAKE(1024);
	CHKNULL(str);

    vecit_t itp;
    int j = 0;

    strcat(str, "(");

    for(j=0, itp = vecit_first(&(meta->parmspec)); vecit_valid(itp); itp = vecit_next(itp), j++)
    {
    	meta_fp_t *parm = (meta_fp_t *) vecit_data(itp);
    	if(j != 0)
    	{
    		strcat(str, ",");
    	}
    	if(parm == NULL)
    	{
    		strcat(str, "? ?");
    	}
    	else
    	{
    		strcat(str, type_to_str(parm->type));
    		strcat(str, " ");
    		strcat(str, parm->name);
    	}
    }

    strcat(str, ")");

	return str;
}

char *ui_str_from_mac(macdef_t *mac)
{
	char *result = STAKE(4096);
	char fmt[1024];
	char *tmp = NULL;
	int i = 0;
	vecit_t it;

	tmp = ui_str_from_ari(mac->ari, NULL, 0);
	sprintf(fmt,"%s = [", (tmp==NULL) ? "null" : tmp);
	SRELEASE(tmp);

	strcat(result, fmt);

	for(it = vecit_first(&(mac->ctrls)); vecit_valid(it); it = vecit_next(it))
	{
		ctrl_t *cur_ctrl = (ctrl_t*) vecit_data(it);
		tmp = ui_str_from_ctrl(cur_ctrl);
		if(i != 0)
		{
			strcat(result,", ");
		}
		strcat(result, tmp);
		SRELEASE(tmp);
		i++;
	}

	strcat(result, "]");

	return result;
}

char *ui_str_from_op(op_t *op)
{
	return ui_str_from_ari(op->id, NULL, 0);
}

char *ui_str_from_rpt(rpt_t *rpt)
{
// TODO
	return NULL;
}

char *ui_str_from_rpttpl(rpttpl_t *rpttpl)
{
	char *result = STAKE(4096);
	char fmt[1024];
	char *tmp = NULL;
	int num = 0;
	int i = 0;

	tmp = ui_str_from_ari(rpttpl->id, NULL, 0);
	sprintf(fmt,"%s = [", (tmp==NULL) ? "null" : tmp);
	SRELEASE(tmp);

	strcat(result, fmt);

	num = ac_get_count(&(rpttpl->contents));
	for(i = 0; i < num; i++)
	{
		ari_t *cur_ari = ac_get(&(rpttpl->contents), i);
		tmp = ui_str_from_ari(cur_ari, NULL, 0);
		if(i != 0)
		{
			strcat(result,", ");
		}
		strcat(result, tmp);
		SRELEASE(tmp);
	}
	strcat(result, "]");

	return result;
}

char *ui_str_from_sbr(rule_t *sbr)
{
	char *str = STAKE(1024);

	if(str != NULL) {
		char *id_str = ui_str_from_ari(&(sbr->id), NULL, 0);
		char *ac_str = ui_str_from_ac(&(sbr->action));
		char *expr_str = ui_str_from_expr(&(sbr->def.as_sbr.expr));

		snprintf(str,
				1023,
				"SBR: ID=%s, S=0x"ADDR_FIELDSPEC", E=%s, M=0x"ADDR_FIELDSPEC", C=0x"ADDR_FIELDSPEC", A=%s\n",
				(id_str == NULL) ? "null" :id_str,
				(uaddr)sbr->start,
				(expr_str == NULL) ? "null" : expr_str,
				(uaddr)sbr->def.as_sbr.max_eval,
				(uaddr)sbr->def.as_sbr.max_fire,
				(ac_str == NULL) ? "null" : ac_str);

		SRELEASE(id_str);
		SRELEASE(ac_str);
		SRELEASE(expr_str);
	}
	return str;
}

char *ui_str_from_tbl(tbl_t *tbl)
{
	char *result = STAKE(4096); // todo dynamically size this.
	char fmt[100];
	vecit_t it;
	int i, j;
	size_t num_rows = 0;
	tnvc_t *cur_row = NULL;
	tblt_t *tblt = VDB_FINDKEY_TBLT(tbl->id);
	char *tmp = NULL;

	CHKNULL(result);

	/* Print table headers, if we have a table template. */
	if((tmp = ui_str_from_tblt(tblt)) == NULL)
	{
		SRELEASE(result);
		return NULL;
	}
	strcat(result, tmp);
	SRELEASE(tmp);

	num_rows = tbl_num_rows(tbl);

	strcat(result, "----------------------------------------------------------------------\n");

	/* For each row */
	for(i = 0; i < num_rows; i++)
	{
		cur_row = tbl_get_row(tbl, i);

		for(j = 0; j < tnvc_get_count(cur_row); j++)
		{
			tnv_t *val = tnvc_get(cur_row, j);
			if(j == 0)
			{
				strcat(result, "|");
			}
			char *tmp = ui_str_from_tnv(val);
			snprintf(fmt,100,"   %23s   ", tmp);
			SRELEASE(tmp);
			strcat(result, fmt);
			strcat(result, "|");
		}
		strcat(result, "\n");
	}
	strcat(result, "----------------------------------------------------------------------\n");

	return result;
}

// TODO
#if 0 //def USE_JSON

void ui_json_from_tbl(cJSON *obj, tbl_t *tbl)
{
	char fmt[100];
	vecit_t it;
	int i, j;
	size_t num_rows = 0;
	tnvc_t *cur_row = NULL;
	tblt_t *tblt = VDB_FINDKEY_TBLT(tbl->id);
	char *tmp = NULL;

	CHKVOID(obj);
    CHKVOID(tbl);

	/* Print table headers, if we have a table template. */
	if((tmp = ui_str_from_tblt(tblt)) == NULL)
	{
		return;
	}
	strcat(result, tmp);
	SRELEASE(tmp);

	num_rows = tbl_num_rows(tbl);

	strcat(result, "----------------------------------------------------------------------\n");

	/* For each row */
	for(i = 0; i < num_rows; i++)
	{
		cur_row = tbl_get_row(tbl, i);

		for(j = 0; j < tnvc_get_count(cur_row); j++)
		{
			tnv_t *val = tnvc_get(cur_row, j);
			if(j == 0)
			{
				strcat(result, "|");
			}
			char *tmp = ui_str_from_tnv(val);
			snprintf(fmt,100,"   %23s   ", tmp);
			SRELEASE(tmp);
			strcat(result, fmt);
			strcat(result, "|");
		}
		strcat(result, "\n");
	}
	strcat(result, "----------------------------------------------------------------------\n");

	return result;
}
#endif

char *ui_str_from_tblt(tblt_t *tblt)
{
	char *result = STAKE(1024);
	int i = 0;
	vecit_t it;

	CHKNULL(result);

	if(tblt != NULL)
	{
		i = 0;
		strcat(result, "----------------------------------------------------------------------\n");
		for(it = vecit_first(&(tblt->cols)); vecit_valid(it); it = vecit_next(it))
		{
			tblt_col_t *col = (tblt_col_t*) vecit_data(it);
			char tmp[64];

			if(i == 0)
			{
				strcat(result, "|");
			}
			sprintf(tmp, "   %7s %15s   |",
					(col) ? type_to_str(col->type) : "null",
					(col) ? col->name : "null");
			strcat(result, tmp);
			i = 1;
		}
		strcat(result, "\n");
	}

	return result;
}

char *ui_str_from_tbr(rule_t *tbr)
{
	char *str = STAKE(1024);
	if(str != NULL)
	{
		char *id_str = ui_str_from_ari(&(tbr->id), NULL, 0);
		char *ac_str = ui_str_from_ac(&(tbr->action));
		snprintf(str,
				1024,
				"TBR: ID=%s, S=" \
				UVAST_FIELDSPEC ", P=" \
				UVAST_FIELDSPEC ", C=" \
				UVAST_FIELDSPEC ", A=%s\n",
				(id_str == NULL) ? "null" :id_str,
				tbr->start,
			    tbr->def.as_tbr.period,
			    tbr->def.as_tbr.max_fire,
		        (ac_str == NULL) ? "null" : ac_str);

		SRELEASE(id_str);
		SRELEASE(ac_str);
	}

	return str;
}

#ifdef USE_JSON
cJSON *ui_json_from_tnv(tnv_t *tnv)
{
   char *str;
   CHKNULL(tnv);

   // TODO: Additional data types from ui_str_from_tnv() can be converted to json
   switch(tnv->type)
   {
      /* Primitive Types */
   case AMP_TYPE_BOOL:   return cJSON_CreateBool(tnv->value.as_byte);
   case AMP_TYPE_INT:    return cJSON_CreateNumber(tnv->value.as_int);
   case AMP_TYPE_UINT:   return cJSON_CreateNumber(tnv->value.as_uint);
   case AMP_TYPE_VAST:   return cJSON_CreateNumber(tnv->value.as_vast);
   case AMP_TYPE_TV:
   case AMP_TYPE_TS:
   case AMP_TYPE_UVAST:  return cJSON_CreateNumber(tnv->value.as_uvast);
   case AMP_TYPE_REAL32: return cJSON_CreateNumber(tnv->value.as_real32);
   case AMP_TYPE_REAL64: return cJSON_CreateNumber(tnv->value.as_real64);
      
      /* Compound Objects */
   case AMP_TYPE_TNV:    return ui_json_from_tnv(tnv->value.as_ptr);
   case AMP_TYPE_TNVC:   return ui_json_from_tnvc(tnv->value.as_ptr);
      
   default: // Fallback to STR Parsing
      str = ui_str_from_tnv(tnv);
      if (str != NULL)
      {
         cJSON *rtv = cJSON_CreateString(str);
         SRELEASE(str);
         return rtv;
      }
      else
      {
         return NULL;
      }
   }
}
#endif

char *ui_str_from_tnv(tnv_t *tnv)
{
	char *str = STAKE(1024);
	char *alt_str = NULL;
	CHKNULL(str);

	if(tnv == NULL)
	{
		strcat(str, "null");
		return str;
	}

	switch(tnv->type)
	{
		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:   alt_str = ui_str_from_edd(tnv->value.as_ptr);    break;
		case AMP_TYPE_CTRL:  alt_str = ui_str_from_ctrl(tnv->value.as_ptr);   break;
		case AMP_TYPE_LIT:
		case AMP_TYPE_ARI:   alt_str = ui_str_from_ari(tnv->value.as_ptr, NULL, 0);    break;
		case AMP_TYPE_MAC:   alt_str = ui_str_from_mac(tnv->value.as_ptr);    break;
		case AMP_TYPE_OPER:  alt_str = ui_str_from_op(tnv->value.as_ptr);     break;
		case AMP_TYPE_RPT:   alt_str = ui_str_from_rpt(tnv->value.as_ptr);    break;
		case AMP_TYPE_RPTTPL:alt_str = ui_str_from_rpttpl(tnv->value.as_ptr); break;
		case AMP_TYPE_SBR:   alt_str = ui_str_from_sbr(tnv->value.as_ptr);    break;
		case AMP_TYPE_TBL:   alt_str = ui_str_from_tbl(tnv->value.as_ptr);    break;
		case AMP_TYPE_TBLT:  alt_str = ui_str_from_tblt(tnv->value.as_ptr);   break;
		case AMP_TYPE_TBR:   alt_str = ui_str_from_tbr(tnv->value.as_ptr);    break;
		case AMP_TYPE_VAR:   alt_str = ui_str_from_var(tnv->value.as_ptr);    break;

		/* Primitive Types */
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:  sprintf(str,"%d", tnv->value.as_byte);    break;
		case AMP_TYPE_STR:   snprintf(str, 1023, "%s", (char*) tnv->value.as_ptr); break;
		case AMP_TYPE_INT:   sprintf(str,"%d", tnv->value.as_int);     break;
		case AMP_TYPE_UINT:  sprintf(str,"%d", tnv->value.as_uint);    break;
		case AMP_TYPE_VAST:  sprintf(str, VAST_FIELDSPEC , tnv->value.as_vast);   break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST: sprintf(str, UVAST_FIELDSPEC , tnv->value.as_uvast);  break;
		case AMP_TYPE_REAL32:sprintf(str,"%f", tnv->value.as_real32);  break;
		case AMP_TYPE_REAL64:sprintf(str,"%lf", tnv->value.as_real64); break;

		/* Compound Objects */
		case AMP_TYPE_TNV:    alt_str = ui_str_from_tnv(tnv->value.as_ptr);  break;
		case AMP_TYPE_TNVC:   alt_str = ui_str_from_tnvc(tnv->value.as_ptr); break;
		case AMP_TYPE_AC:     alt_str = ui_str_from_ac(tnv->value.as_ptr);   break;
		case AMP_TYPE_EXPR:   alt_str = ui_str_from_expr(tnv->value.as_ptr); break;
		case AMP_TYPE_BYTESTR:alt_str = ui_str_from_blob(tnv->value.as_ptr); break;

		default:
			strcat(str, "UNK");
			break;
	}

	if(alt_str != NULL)
	{
		strncat(str, alt_str, 1023);
		SRELEASE(alt_str);
	}

	return str;
}

#ifdef USE_JSON
/* Convert a TNVC into a JSON Array and insert into specified object */
cJSON* ui_json_from_tnvc(tnvc_t *tnvc)
{
	int i;
	int max;
    CHKNULL(tnvc);

	max = tnvc_get_count(tnvc);
	if(max == 0)
	{
		return NULL;
	}
    else if (max == 1)
    {
       return ui_json_from_tnv(tnvc_get(tnvc,0));
    }

    cJSON *obj = cJSON_CreateArray();
    
	for(i = 0; i < max; i++)
	{
       cJSON_AddItemToArray(obj, ui_json_from_tnv(tnvc_get(tnvc,i)));
	}

	return obj;
}
#endif

char *ui_str_from_tnvc(tnvc_t *tnvc)
{
	char *str = STAKE(1024);
	int i;
	int max;
	CHKNULL(str);

	max = tnvc_get_count(tnvc);
	if(max == 0)
	{
		strcat(str,"null");
		return str;
	}

	for(i = 0; i < max; i++)
	{
		char *val_str = ui_str_from_tnv(tnvc_get(tnvc,i));
		if(i != 0)
		{
			strcat(str, ", ");
		}
		strcat(str, val_str);
		SRELEASE(val_str);
	}

	return str;
}



char *ui_str_from_var(var_t *var)
{
	return ui_str_from_tnv(var->value);
}





