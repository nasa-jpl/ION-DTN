/*****************************************************************************
 **
 ** \file nm_mgr_print.h
 **
 **
 ** Description: Helper file holding functions for printing DTNMP data to
 **              the screen.  These functions differ from the "to string"
 **              functions for individual DTNMP data types as these functions
 **              "pretty print" directly to stdout and are nly appropriate for
 **              ground-related applications. We do not anticipate these functions
 **              being appropriate for use in embedded systems.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/10/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
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
      ui_print_reports(agent);
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
         ui_print_reports(agent);
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

/*
 * We need to find out a description for the entry so we can print it out.
 * So, if entry is <RPT MID> <int d1><int d2><int d3> we need to match the items
 * to elements of the report definition.
 *
 */
void ui_print_entry(tnvc_t *entry, uvast *mid_sizes, uvast *data_sizes)
{
//	def_gen_t *cur_def = NULL;
//	uint8_t del_def = 0;
    vecit_t it;

	if((entry == NULL) || (mid_sizes == NULL) || (data_sizes == NULL))
	{
		AMP_DEBUG_ERR("ui_print_entry","Bad Args.", NULL);
		return;
	}

	/* Step 1: Calculate sizes...*/
    // TODO: This is num entries, not size as originally output
    *mid_sizes = *mid_sizes + vec_num_entries(entry->values); 

    for(it = vecit_first(&(entry->values)); vecit_valid(it); it = vecit_next(it))
    {
       blob_t *cur = (blob_t*)vecit_data(it);
        *data_sizes = *data_sizes + cur->length;
    }

#if 0 // TODO
    *data_sizes = *data_sizes + entry->contents->hdr.length;

	/* Step 1: Print the MID associated with the Entry. */
    ui_printf(" (");
    ui_print_mid(entry->id);
	ui_printf(") has %d values.", entry->contents->hdr.length);

    /*
     * Step 2: Try and find the metadata associated with each
     *         value in the TDC. Since the TDC is already typed, the
     *         needed meta-data information is simply the
     *         "name" of the data.
     *
     *         i Only computed data definitions, reports, and macros
     *         need names. Literals, controls, and atomic data do
     *         not (currently) define extra meta-data for their
     *         definitions.
     *
     *         \todo: Consider printing names for each return
     *         value from a control.
     */

    cur_def = NULL;

	if((MID_GET_FLAG_ID(entry->id->flags) == MID_ATOMIC) ||
	   (MID_GET_FLAG_ID(entry->id->flags) == MID_LITERAL))
	{
		adm_datadef_t *ad_entry = adm_find_datadef(entry->id);

		/* Fake a def_gen_t for now. */
		if(ad_entry != NULL)
		{
	    	Lyst tmp = lyst_create();
	    	lyst_insert(tmp,mid_copy(ad_entry->mid));
	    	cur_def = def_create_gen(mid_copy(ad_entry->mid), ad_entry->type, tmp);
	    	del_def = 1;
		}
	}
	else if(MID_GET_FLAG_ID(entry->id->flags) == MID_COMPUTED)
	{
		var_t *cd = NULL;
	    if(MID_GET_FLAG_ISS(entry->id->flags) == 0)
	    {
	    	cd = var_find_by_id(gAdmComputed, NULL, entry->id);
	    }
	    else
	    {
	    	cd = var_find_by_id(gMgrVDB.compdata, &(gMgrVDB.compdata_mutex), entry->id);
	    }
	    // Fake a def_gen just for this CD item.
	    if(cd != NULL)
	    {
	    	Lyst tmp = lyst_create();
	    	lyst_insert(tmp,mid_copy(cd->id));
	    	cur_def = def_create_gen(mid_copy(cd->id), cd->value.type, tmp);
	    	del_def = 1;
	    }
	}
	else if(MID_GET_FLAG_ID(entry->id->flags) == MID_REPORT)
	{
		rpttpl_t *cur_tpl = NULL;

	    if(MID_GET_FLAG_ISS(entry->id->flags) == 0)
	    {
	    	cur_tpl = rpttpl_find_by_id(gAdmRptTpls, NULL, entry->id);
	    }
	    else
	    {
	    	cur_tpl= rpttpl_find_by_id(gMgrVDB.reports, &(gMgrVDB.reports_mutex), entry->id);
	    }

	    // Fake a def_gen just for this RPT item.
	    if(cur_tpl != NULL)
	    {
	    	Lyst tmp = lyst_create();
	    	uint32_t i = 0;

	    	for(elt = lyst_first(cur_tpl->contents); elt; elt = lyst_next(elt))
	    	{
	    		rpttpl_item_t *item = lyst_data(elt);

	    		if(item != NULL)
	    		{
	    			mid_t *cur_mid = mid_copy(item->mid);
	    			amp_type_e tpl_type = AMP_TYPE_UNK;

	    			for(i = 0; i < item->num_map; i++)
	    			{
	    				blob_t *src_parm = mid_get_param(entry->id, RPT_MAP_GET_SRC_IDX(item->parm_map[i])-1, &tpl_type);
	    				mid_update_parm(cur_mid, RPT_MAP_GET_DEST_IDX(item->parm_map[i]), 1, tpl_type, src_parm);
	    			}
	    			lyst_insert_last(tmp, cur_mid);
	    		}
	    	}

	    	cur_def = def_create_gen(mid_copy(cur_tpl->id), AMP_TYPE_RPT, tmp);
	    	del_def = 1;
	    }

	}
	else if(MID_GET_FLAG_ID(entry->id->flags) == MID_MACRO)
	{
	    if(MID_GET_FLAG_ISS(entry->id->flags) == 0)
	    {
	    	cur_def = def_find_by_id(gAdmMacros, NULL, entry->id);
	    }
	    else
	    {
	    	cur_def = def_find_by_id(gMgrVDB.macros, &(gMgrVDB.macros_mutex), entry->id);
	    }

	}

	/* Step 3: Print the TDC holding data for the entry. */
    if (cur_def != NULL)
    {
       ui_print_tdc(entry->contents, cur_def);

       if(del_def)
       {
          def_release_gen(cur_def);
       }
    }
#endif
    return;
}

#if 0
void ui_print_expr(expr_t *expr)
{
	char *str;

	if(expr == NULL)
	{
		printf("NULL");
	}

	str = expr_to_string(expr);
	printf("%s", str);
	SRELEASE(str);

}

void ui_print_mc(Lyst mc)
{
	LystElt elt = NULL;
	int i = 0;
	mid_t *mid = NULL;

	printf("{ ");

	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		if(i > 0)
		{
			printf(", ");
		}
		i++;
		mid = lyst_data(elt);
		ui_print_mid(mid);
	}

	printf(" }");

}

void ui_print_mid(mid_t *mid)
{
	char *result = NULL;

	if(mid == NULL)
	{
		printf("NULL");
		return;
	}

	result = names_get_name(mid);
	printf("%s", result);
	SRELEASE(result);

	if(mid_get_num_parms(mid) > 0)
	{
		char *parms = tdc_to_str(&(mid->oid.params));
		printf("(%s)", parms);
		SRELEASE(parms);
	}
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
 *  07/10/15  E. Birrane     Updated to new report structure.
 *****************************************************************************

void ui_print_predefined_rpt(mid_t *mid, uint8_t *data, uint64_t data_size, uint64_t *data_used, adm_datadef_t *adu)
{
	uint64_t len;
	char *mid_str = NULL;
	//char *mid_name = NULL;
	char *mid_val = NULL;
	char *name = NULL;
	uint32_t bytes = 0;

	value_t val = val_deserialize(data, data_size, &bytes);  ;

	uint32_t str_size = 0;

	mid_str = mid_to_string(mid);
	//mid_name = names_get_name(mid);

	*data_used = bytes;
	mid_val = val_to_string(val);
	name = names_get_name(mid);

	printf("Data Name: %s\n", name);
	printf("MID      : %s\n", mid_str);
	printf("Value    : %s\n", mid_val);
	SRELEASE(name);
	SRELEASE(mid_val);
	SRELEASE(mid_str);
	val_release(&val, 0);
}
*/
#endif

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
   CHKVOID(agent);
   rpt_t *cur_report = NULL;
   tnvc_t *cur_entry = NULL;
   vecit_t rpt_it, entry_it;
   int num_reports = vec_num_entries(agent->rpts);
   
   char title[40];
   sprintf(title, "Agent Reports for %s", agent->eid.name);
   ui_display_init(title);
   
   if (num_reports == 0)
   {
      ui_printf("No reports received from this agent");
      AMP_DEBUG_ALWAYS("ui_print_reports","[No reports received from this agent.]", NULL);
      ui_display_exec();
      AMP_DEBUG_EXIT("ui_print_reports", "->.", NULL);
      return;
   }

   /* Iterate through all reports for this agent. */
   for(rpt_it = vecit_first(&(agent->rpts)); vecit_valid(rpt_it); rpt_it = vecit_next(rpt_it))
   {
      /* Grab the current report */
      cur_report = (rpt_t*)vecit_data(rpt_it);
      
      uvast mid_sizes = 0;
      uvast data_sizes = 0;
      int num_entries = vec_num_entries(cur_report->entries->values);
      int i = 0;

      /* Print the Report Header */
      ui_printf("\n----------------------------------------");
      ui_printf("\n            DTNMP DATA REPORT           ");
      ui_printf("\n----------------------------------------");
      ui_printf("\nSent to   : %s", cur_report->recipient.name);
      ui_printf("\nTimestamp : %s", ctime(&(cur_report->time)));
      ui_printf("\n# Entries : %d", num_entries);
      ui_printf("\n----------------------------------------");
      
      /* For each MID in this report, print it. */
      for(entry_it = vecit_first(&(cur_report->entries->values)); vecit_valid(entry_it); entry_it = vecit_next(entry_it))
      {
         ui_printf("\nEntry %d ", i);
         cur_entry = (tnvc_t*)vecit_data(rpt_it);
         ui_print_entry(cur_entry, &mid_sizes, &data_sizes);
         i++;
      }
      
      ui_printf("\n----------------------------------------");
      ui_printf("\nSTATISTICS:");
      ui_printf("\nMIDs total "UVAST_FIELDSPEC" bytes", mid_sizes);
      ui_printf("\nData total: "UVAST_FIELDSPEC" bytes", data_sizes);
      if (mid_sizes > 0 && data_sizes > 0)
      {
         ui_printf("\nEfficiency: %.2f%%", (double)(((double)data_sizes)/((double)mid_sizes + data_sizes)) * (double)100.0);
      }
      
      ui_printf("\n----------------------------------------\n\n\n");
   }
   ui_display_exec();
}

#if 0
void ui_print_srl(srl_t *srl)
{
	char *mid_str = NULL;

	if(srl == NULL)
	{
		printf("NULL");
		return;
	}

	mid_str = mid_to_string(srl->mid);

	printf("SRL %s: T:%d E:", mid_str, (uint32_t) srl->time);
	ui_print_expr(srl->expr);
	printf(" C:"UVAST_FIELDSPEC" A:", srl->count);
	ui_print_mc(srl->action);
	SRELEASE(mid_str);

}

// THis is a DC of values? Generally, a typed data collection is a DC of values.
void ui_print_tdc(tdc_t *tdc, def_gen_t *cur_def)
{
	LystElt elt = NULL;
	LystElt def_elt = NULL;
	uint32_t i = 0;
	amp_type_e cur_type;
	blob_t *cur_entry = NULL;
	value_t *cur_val = NULL;

	if(tdc == NULL)
	{
		AMP_DEBUG_ERR("ui_print_tdc","Bad Args.", NULL);
		return;
	}

	if(cur_def != NULL)
	{
		if(lyst_length(cur_def->contents) != tdc->hdr.length)
		{
			AMP_DEBUG_WARN("ui_print_tdc","def and TDC length mismatch: %d != %d. Ignoring.",
					        lyst_length(cur_def->contents), tdc->hdr.length);
			cur_def = NULL;
		}
	}


	elt = lyst_first(tdc->datacol);
	if(cur_def != NULL)
	{
		def_elt = lyst_first(cur_def->contents);
	}

	for(i = 0; ((i < tdc->hdr.length) && elt); i++)
	{
		cur_type = (amp_type_e) tdc->hdr.data[i];

		printf("\n\t");
		if(cur_def != NULL)
		{
			printf("Value %d (", i);
			ui_print_mid((mid_t *) lyst_data(def_elt));
			printf(") ");
		}

		// \todo: Check return values.
		if((cur_entry = lyst_data(elt)) == NULL)
		{
			printf("NULL\n");
		}
		else
		{
			ui_print_val(cur_type, cur_entry->value, cur_entry->length);
		}


		elt = lyst_next(elt);

		if(cur_def != NULL)
		{
			def_elt = lyst_next(def_elt);
		}
	}

}


/******************************************************************************
 *
 * \par Function Name: ui_print_table
 *
 * \par Purpose: Prints a table to stdout.
 *
 * \par
 * COL HDR: Name (Type), Name (Type), Name (Type)...
 * ROW   0: Value1, Value2, Value3...
 * ROW   1: Value1, Value2, Value3...
 *
 * \param[in]  table  The table to be printed
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/8/16  E. Birrane     Initial implementation,
 *****************************************************************************/
void ui_print_table(table_t *table)
{
	int32_t i = 0;
	uint32_t row_num = 1;
	int8_t first = 0;

	LystElt elt;
	LystElt elt2;

	char *temp = NULL;
	blob_t *cur_blob = NULL;
	table_row_t *cur_row = NULL;

	if(table == NULL)
	{
		printf("NULL");
		return;
	}

	printf("COL HDR: ");
	for(elt = lyst_first(table->hdr.names); elt; elt = lyst_next(elt))
	{
		cur_blob = (blob_t*) lyst_data(elt);

		if(first == 0)
		{
			first = 1;
		}
		else
		{
			printf(", ");
		}
		printf(" %s (%s)", (char *)cur_blob->value, type_to_str(table->hdr.types.value[i]));
		i++;
	}
	printf("\n");

	for(elt = lyst_first(table->rows); elt; elt = lyst_next(elt))
	{
		cur_row = (table_row_t*) lyst_data(elt);

		printf("ROW %3d: ", row_num++);

		if(cur_row == NULL)
		{
			printf("NULL");
		}
		else
		{
			first = 0;
			i = 0;

			for(elt2 = lyst_first(cur_row->cels); elt2; elt2 = lyst_next(elt2))
			{
				cur_blob = (blob_t *) lyst_data(elt2);
				value_t  val = val_from_blob(cur_blob, table->hdr.types.value[i]);

				if(first == 0)
				{
					first = 1;
				}
				else
				{
					printf(", ");
				}

				temp = val_to_string(val);
				printf("%s", temp);
				SRELEASE(temp);
				val_release(&val, 0);
				i++;
			}
			printf("\n");
		}
	}

}

char *ari_to_str(ari_t *ari);

void ui_print_trl(trl_t *trl)
{
	char *mid_str = NULL;

	if(trl == NULL)
	{
		printf("NULL");
		return;
	}

	mid_str = mid_to_string(trl->mid);

	printf("TRL %s: T:%d P:"UVAST_FIELDSPEC" C:"UVAST_FIELDSPEC" A:", mid_str, (uint32_t) trl->time, trl->period, trl->count);
	ui_print_mc(trl->action);
	SRELEASE(mid_str);
}

void ui_print_val(uint8_t type, uint8_t *data, uint32_t length)
{
	uint32_t bytes = 0;

	if(data == NULL)
	{
		printf("NULL");
		return;
	}



	switch(type)
	{
		case AMP_TYPE_VAR:
		{
			var_t *cd = var_deserialize(data, length, &bytes);
			char *str = var_to_string(cd);
			printf("%s", str);
			SRELEASE(str);
			var_release(cd);
		}
			break;

		case AMP_TYPE_INT:
			printf("%d", utils_deserialize_int(data, length, &bytes));
			break;

		case AMP_TYPE_TS:
		case AMP_TYPE_UINT:
			printf("%d", utils_deserialize_uint(data, length, &bytes));
			break;

		case AMP_TYPE_VAST:
			printf(VAST_FIELDSPEC, utils_deserialize_vast(data, length, &bytes));
			break;

		case AMP_TYPE_SDNV:
		case AMP_TYPE_UVAST:
			printf(UVAST_FIELDSPEC, utils_deserialize_uvast(data, length, &bytes));
			break;

		case AMP_TYPE_REAL32:
			printf("%f", utils_deserialize_real32(data, length, &bytes));
			break;

		case AMP_TYPE_REAL64:
			printf("%f", utils_deserialize_real64(data, length, &bytes));
			break;

		case AMP_TYPE_STRING:
			{
				char* tmp = NULL;
				tmp = utils_deserialize_string(data, length, &bytes);
				printf("%s", tmp);
				SRELEASE(tmp);
			}
			break;

		case AMP_TYPE_BLOB:
			{
				blob_t *blob = blob_deserialize(data, length, &bytes);
				char *str = blob_to_str(blob);
				printf("%s", str);
				SRELEASE(str);
				SRELEASE(blob);
			}
			break;

		case AMP_TYPE_DC:
			{
				uint32_t bytes = 0;
				Lyst dc = dc_deserialize(data, length, &bytes);
				ui_print_dc(dc);
				dc_destroy(&dc);
			}
			break;

		case AMP_TYPE_ARI:
			{
				char * s =ari_to_str(ari_t *ari);
				[print...]
				 SRELEASE(s);

				uint32_t bytes = 0;
				mid_t *mid = mid_deserialize(data, length, &bytes);
				ui_print_mid(mid);
				mid_release(mid);
			}
			break;

		case AMP_TYPE_MC:
			{
				uint32_t bytes = 0;
				Lyst mc = midcol_deserialize(data, length, &bytes);
				ui_print_mc(mc);
				midcol_destroy(&mc);
			}
			break;
			// \todo: Expression has no priority. Need to re-think priority.

		case AMP_TYPE_EXPR:
			{
				uint32_t bytes = 0;
				expr_t *expr = expr_deserialize(data, length, &bytes);
				ui_print_expr(expr);
				expr_release(expr);
			}
			break;

/*		case DTNMP_TYPE_DEF:
			{
				uint32_t bytes = 0;
				def_gen_t *def = def_deserialize_gen(data, length, &bytes);
				ui_print_def(def);
				def_release_gen(def);
			}
			break;
*/
		case AMP_TYPE_TRL:
			{
				uint32_t bytes = 0;
				trl_t *trl = trl_deserialize(data, length, &bytes);
				ui_print_trl(trl);
				trl_release(trl);
			}
			break;

		case AMP_TYPE_TABLE:
			{
				uint32_t bytes = 0;
				table_t *table = table_deserialize(data, length, &bytes);
				ui_print_table(table);
				table_destroy(table, 1);
			}
			break;

		case AMP_TYPE_SRL:
			{
				uint32_t bytes = 0;
				srl_t *srl = srl_deserialize(data, length, &bytes);
				ui_print_srl(srl);
				srl_release(srl);
			}
			break;

		default:
			printf("Unknown.");
	}
}
#endif
