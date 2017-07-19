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
#include "mgr_db.h"

#include "../shared/utils/utils.h"
#include "../shared/primitives/blob.h"
#include "../shared/primitives/table.h"

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
 *****************************************************************************/

int ui_print_agents()
{
  int i = 0;
  LystElt element = NULL;
  agent_t * agent = NULL;

  AMP_DEBUG_ENTRY("ui_print_agents","()",NULL);

  printf("\n------------- Known Agents --------------\n");

  element = lyst_first(known_agents);
  if(element == NULL)
  {
	  printf("[None]\n");
  }
  while(element != NULL)
  {
	  i++;
	  if((agent = lyst_data(element)) == NULL)
	  {
		  printf("%d) NULL?\n", i);
	  }
	  else
	  {
		  printf("%d) %s\n", i, (char *) agent->agent_eid.name);
	  }
	  element = lyst_next(element);
  }

  printf("\n------------- ************ --------------\n");
  printf("\n");

  AMP_DEBUG_EXIT("ui_print_agents","->%d", (i));
  return i;
}


/******************************************************************************
 *
 * \par Function Name: ui_print_custom_rpt_entry
 *
 * \par Prints the Typed Data Collection contained within a report entry.
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
 *  07/10/15  E. Birrane     Updated to new reporting structure.
 *****************************************************************************

void ui_print_custom_rpt_entry(rpt_entry_t *rpt_entry, def_gen_t *rpt_def)
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
			ui_print_predefined_rpt_entry(cur_mid, (uint8_t*)&(rpt_entry->contents[idx]),
					                rpt_entry->size - idx, &data_used, adu);
			idx += data_used;
		}
		else
		{
			DTNMP_DEBUG_ERR("ui_print_custom_rpt","Unable to find MID %s", mid_str);
		}

		SRELEASE(mid_str);
	}
}
*/

void ui_print_dc(Lyst dc)
{
	printf("ui_print_dc not implements.");

}

void ui_print_def(def_gen_t *def)
{
	if(def == NULL)
	{
		printf("NULL");
	}
	else
	{
		ui_print_mid(def->id);
//		printf(",%s,", type_to_str(def->type));
		ui_print_mc(def->contents);
	}
}

/*
 * We need to find out a description for the entry so we can print it out.
 * So, if entry is <RPT MID> <int d1><int d2><int d3> we need to match the items
 * to elements of the report definition.
 *
 */
void ui_print_entry(rpt_entry_t *entry, uvast *mid_sizes, uvast *data_sizes)
{
	LystElt elt = NULL;
	def_gen_t *cur_def = NULL;
	uint8_t del_def = 0;

	if((entry == NULL) || (mid_sizes == NULL) || (data_sizes == NULL))
	{
		AMP_DEBUG_ERR("ui_print_entry","Bad Args.", NULL);
		return;
	}

	/* Step 1: Calculate sizes...*/
    *mid_sizes = *mid_sizes + entry->id->raw_size;

    for(elt = lyst_first(entry->contents->datacol); elt; elt = lyst_next(elt))
    {
    	blob_t *cur = lyst_data(elt);
        *data_sizes = *data_sizes + cur->length;
    }
    *data_sizes = *data_sizes + entry->contents->hdr.length;

	/* Step 1: Print the MID associated with the Entry. */
    printf(" (");
    ui_print_mid(entry->id);
	printf(") has %d values.", entry->contents->hdr.length);


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

	if(MID_GET_FLAG_ID(entry->id->flags) == MID_ATOMIC)
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
	    if(MID_GET_FLAG_ISS(entry->id->flags) == 0)
	    {
	    	cur_def = def_find_by_id(gAdmRpts, NULL, entry->id);
	    }
	    else
	    {
	    	cur_def = def_find_by_id(gMgrVDB.reports, &(gMgrVDB.reports_mutex), entry->id);
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
    ui_print_tdc(entry->contents, cur_def);

    if(del_def)
    {
    	def_release_gen(cur_def);
    }
    return;
}

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
	 rpt_t *cur_report = NULL;
	 rpt_entry_t *cur_entry = NULL;

	 if(agent == NULL)
	 {
		 AMP_DEBUG_ENTRY("ui_print_reports","(NULL)", NULL);
		 AMP_DEBUG_ERR("ui_print_reports", "No agent specified", NULL);
		 AMP_DEBUG_EXIT("ui_print_reports", "->.", NULL);
		 return;

	 }
	 AMP_DEBUG_ENTRY("ui_print_reports","(%s)", agent->agent_eid.name);

	 if(lyst_length(agent->reports) == 0)
	 {
		 AMP_DEBUG_ALWAYS("ui_print_reports","[No reports received from this agent.]", NULL);
		 AMP_DEBUG_EXIT("ui_print_reports", "->.", NULL);
		 return;
	 }

	 /* Free any reports left in the reports list. */
	 for (report_elt = lyst_first(agent->reports); report_elt; report_elt = lyst_next(report_elt))
	 {
		 /* Grab the current report */
	     if((cur_report = (rpt_t*)lyst_data(report_elt)) == NULL)
	     {
	        AMP_DEBUG_ERR("ui_print_reports","Unable to get report from lyst!", NULL);
	     }
	     else
	     {
	    	 uvast mid_sizes = 0;
	    	 uvast data_sizes = 0;
	    	 int i = 1;

	    	 /* Print the Report Header */
	    	 printf("\n----------------------------------------");
	    	 printf("\n            DTNMP DATA REPORT           ");
	    	 printf("\n----------------------------------------");
	    	 printf("\nSent to   : %s", cur_report->recipient.name);
	    	 printf("\nTimestamp : %s", ctime(&(cur_report->time)));
	    	 printf("\n# Entries : %lu",
			 (unsigned long) lyst_length(cur_report->entries));
	    	 printf("\n----------------------------------------");

 	    	 /* For each MID in this report, print it. */
	    	 for(entry_elt = lyst_first(cur_report->entries); entry_elt; entry_elt = lyst_next(entry_elt))
	    	 {
	    		 printf("\nEntry %d ", i);
	    		 cur_entry = (rpt_entry_t*)lyst_data(entry_elt);
	    		 ui_print_entry(cur_entry, &mid_sizes, &data_sizes);
	    		 i++;
	    	 }

	    	 printf("\n----------------------------------------");
	    	 printf("\nSTATISTICS:");
	    	 printf("\nMIDs total "UVAST_FIELDSPEC" bytes", mid_sizes);
	    	 printf("\nData total: "UVAST_FIELDSPEC" bytes", data_sizes);
		 if ((mid_sizes + data_sizes) > 0)
		 {
	    	 	printf("\nEfficiency: %.2f%%", (double)(((double)data_sizes)/((double)mid_sizes + data_sizes)) * (double)100.0);
		 }

	    	 printf("\n----------------------------------------\n\n\n");
	     }
	 }
}


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

		case AMP_TYPE_MID:
			{
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
