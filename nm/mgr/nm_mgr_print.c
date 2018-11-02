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

#include "../shared/utils/utils.h"
#include "../shared/primitives/blob.h"
#include "../shared/primitives/table.h"

#ifdef USE_NCURSES
#include <menu.h>
#endif

ui_menu_list_t agent_submenu_list[] = {
   {"(D)e-register agent", NULL, NULL},
   {"(B)uild Control", NULL, NULL},
   {"Send (R)aw Command", NULL, NULL},
   {"Send Command (F)ile", NULL, NULL},
   {"(P)rint Agent Reports", NULL, NULL},
   {"(W)rite Agent Reports to file", NULL, NULL},
   {"(C)lear Agent Reports", NULL, NULL},
};




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
int ui_print_agents_cb_fn(int idx, int keypress, void* data, char* status_msg) {
   int choice;
   agent_t *agent = (agent_t*)data;
   char *subtitle = "";
   char *tmp;

#ifdef USE_NCURSES
   if (keypress == KEY_ENTER || keypress == 10)
   {
#endif
      // Switch below treats values corresponding to menu presses as ints instead of char

      keypress = ui_menu_listing(agent->eid.name,
                                 agent_submenu_list, ARRAY_SIZE(agent_submenu_list),
                                 NULL, 0,
#ifdef USE_NCURSES
                                 "F1 or 'e' to cancel. Arrow keys to navigate and enter to select. (x) indicates key that can be pressed directly from agent listing (parent) menu to perform this action.",
#else
                                 NULL,
#endif
                                 NULL,
                                 UI_OPT_AUTO_LABEL | UI_OPT_ENTER_SEL);
#ifdef USE_NCURSES
   }
#endif
   
   switch(keypress)
   {
   case 'e':
   case 'E':
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
   case 'w':
   case 'W':
   case 5:
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
   case 6:
      // clear agent reports
      choice = ui_prompt("Clear reports for this agent?", "Yes", "No", NULL);
      if (choice == 0)
      {
         ui_clear_reports(agent);
         return UI_CB_RTV_ERR; // Return from parent to force update of report counts in display
      }
   }
   // Default behavior is to return 0, indicating parent menu should continue on.
   return UI_CB_RTV_CONTINUE;
}

int ui_print_agents()
{
  vecit_t it;
  int i = 0, tmp;
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
      if (tmp > 0)
      {
         list[i].description = malloc(32);
         sprintf(list[i].description, "%d reports available", tmp);
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


void ui_print_report(rpt_t *rpt)
{
	int num_entries = 0;
	tnv_t *val = NULL;
	char name[32];
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

    ui_printf("\n----------------------------------------");
    ui_printf("\n             AMP DATA REPORT            ");
    ui_printf("\n----------------------------------------");
    ui_printf("\nSent to   : %s", rpt->recipient.name);
    ui_printf("\nRpt Name  : %s", (rpt_info == NULL) ? "Unknown" : rpt_info->name);
    ui_printf("\nTimestamp : %s", ctime(&(rpt->time)));
    ui_printf("\n# Entries : %d", num_entries);
    ui_printf("\n----------------------------------------\n");


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
			val = tnvc_get(rpt->entries, i);
			ari_t *entry_id = ac_get(&(tpl->contents), i);
			entry_info = (tpl == NULL) ? NULL : rhht_retrieve_key(&(gMgrDB.metadata), entry_id);

			if(entry_info == NULL)
			{
				sprintf(name, "Entry %d", i);
			}
			else
			{
				sprintf(name, "%30s", entry_info->name);
			}

			ui_print_report_entry(name, val);
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
			sprintf(name, "%30s", rpt_info->name);
		}

		ui_print_report_entry(name, val);
	}


	/* Step 3: Print report trailer. */
    ui_printf("\n----------------------------------------\n\n");
}


void ui_print_report_entry(char *name, tnv_t *val)
{
	if(val == NULL)
	{
		ui_printf("%s: null", name);
		return;
	}

	char *str = ui_str_from_tnv(val);
	ui_printf("%s : %s\n", name, (str == NULL) ? "null" : str);
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


char *ui_str_from_ac(ac_t *ac)
{
	char *str = STAKE(1024);
	vecit_t it;

	for(it = vecit_first(&(ac->values)); vecit_valid(it); it = vecit_next(it))
	{
		ari_t *id = (ari_t*) vecit_data(it);
		char *alt_str = ui_str_from_ari(id, NULL, 0);
		strcat(str, alt_str);
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
			sprintf(str,"%s(%s)\n", meta->name, parm_str);
			SRELEASE(parm_str);
		}
		else
		{
			sprintf(str,"%s\n", meta->name);
		}

		if(desc)
		{
			strcat(str,"\t: ");
			strcat(str,meta->descr);
		}
		strcat(str,"\n");
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
	// TODO
	return NULL;
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

char *ui_str_from_rpttpl(rpt_t *rpt)
{
// TODO
	return NULL;
}

char *ui_str_from_sbr(rule_t *rule)
{
	// TODO
	return NULL;
}

char *ui_str_from_tbl(tbl_t *tbl)
{
	// TODO
	return NULL;
}

char *ui_str_from_tblt(tblt_t *tbltt)
{
	// TODO
	return NULL;
}

char *ui_str_from_tbr(rule_t *tbr)
{
	// TODO

	return NULL;
}


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
		case AMP_TYPE_VAST:  sprintf(str,"%ld", tnv->value.as_vast);   break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST: sprintf(str,"%ld", tnv->value.as_uvast);  break;
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


char *ui_str_from_tnvc(tnvc_t *tnvc)
{
	char *str = STAKE(1024);
	int i;
	int max;
	CHKNULL(str);

	max = tnvc_get_count(tnvc);
	for(i = 0; i < max; i++)
	{
		char *val_str = ui_str_from_tnv(tnvc_get(tnvc,i));
		strcat(str, val_str);
		if(i != 0)
		{
			strcat(str, ", ");
		}
		SRELEASE(val_str);
	}

	return str;
}



char *ui_str_from_var(var_t *var)
{
	return ui_str_from_tnv(var->value);
}





