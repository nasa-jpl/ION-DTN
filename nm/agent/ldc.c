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
 ** File Name: ldc.c
 **
 ** Description: This implements the NM Agent Local Data Collector (LDC).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Code comments and functional updates.
 **  01/10/13  E. Birrane     Update to latest version of DTNMP. Cleanup.
 *****************************************************************************/

#include "shared/adm/adm.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/value.h"
#include "shared/primitives/report.h"
#include "shared/primitives/expr.h"

#include "nmagent.h"
#include "ldc.h"

/******************************************************************************
 *
 * \par Function Name: ldc_fill_report_entry
 *
 * \par Populate the contents of a single report entry.
 *
 * \param[out] entry  The Report Entry to fill.
 *
 * \par Notes:
 *		- We assume that the passed-in report is pre-allocated.
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/11  E. Birrane     Initial implementation.
 *  08/18/13  E. Birrane     Added nesting levels to limit recursion.
 *  07/04/15  E. Birrane     Updated to new reporting structure.
 *****************************************************************************/

int ldc_fill_report_entry(rpt_entry_t *entry)
{
    int result = -1;
    char *msg = NULL;

    DTNMP_DEBUG_ENTRY("ldc_fill_report_entry","(0x%x)",
    		          (unsigned long) entry);

    /* Step 0: Sanity Check */
    if(entry == NULL)
    {
    	DTNMP_DEBUG_ERR("ldc_fill_report_entry","Bad Args.",NULL);
    	DTNMP_DEBUG_EXIT("ldc_fill_report_entry","-> -1",NULL);
    	return -1;
    }

    msg = mid_to_string(entry->id);
    DTNMP_DEBUG_INFO("ldc_fill_report_entry","Gathering report data for MID: %s",
    		         msg);

    /* Step 1: Search for this MID...
     *
     * Reports can contain information from:
     *
     * 1. Atomic data definitions (from ADMs)
     * 2. Computed data definitions (from ADMs or user-defined)
     * 3. Report definitions (from ADMs or user-defined)
     *
     */

    switch(MID_GET_FLAG_ID(entry->id->flags))
    {

        /* Step 1.1: If this is an atomic data definition...*/
    	case MID_ATOMIC:
    	{
    	    adm_datadef_t *adm_def = NULL;

    	    if((adm_def = adm_find_datadef(entry->id)) != NULL)
    	    {
    	    	DTNMP_DEBUG_INFO("ldc_fill_report_entry","Filling pre-defined.", NULL);
    	    	result = ldc_fill_atomic(adm_def, entry);
    	    }
    	}
    	break;

   		/* Step 1.2: If this is a computed definition... */
    	case MID_COMPUTED:
    	{
    	    cd_t *cd_def = NULL;

    		/* Step 1.3.1: Check if this is an ADM-defined CD. */
    		if((cd_def = cd_find_by_id(gAdmComputed, NULL, entry->id)) != NULL)
    		{
    	       	DTNMP_DEBUG_INFO("ldc_fill_report_entry","Filling ADM Custom Definition.", NULL);
    	       	result = ldc_fill_computed(cd_def, entry);
    		}
    		/* Step 1.3.2: Check if this is a user-defined CD. */
    		else if((cd_def = cd_find_by_id(gAgentVDB.compdata, &(gAgentVDB.compdata_mutex), entry->id)) != NULL)
    	    {
    	       	DTNMP_DEBUG_INFO("ldc_fill_report_entry","Filling User Computed Definition.", NULL);
    	       	result = ldc_fill_computed(cd_def, entry);
    	    }
     	}
    		break;

   	    /* Step 1.2: If this is a data report...*/
    	case MID_REPORT:
    	{
    	    def_gen_t *rpt_def = NULL;

    		/* Step 1.3.1: Check if this is an ADM-defined report. */
    		if((rpt_def = def_find_by_id(gAdmRpts, NULL, entry->id)) != NULL)
    		{
    	       	DTNMP_DEBUG_INFO("ldc_fill_report_entry","Filling ADM Report.", NULL);
    	       	result = ldc_fill_custom(rpt_def, entry);
    		}
    		/* Step 1.3.2: Check if this is a user-defined report. */
    		else if((rpt_def = def_find_by_id(gAgentVDB.reports, &(gAgentVDB.reports_mutex), entry->id)) != NULL)
    	    {
    	       	DTNMP_DEBUG_INFO("ldc_fill_report_entry","Filling User Report.", NULL);
    	       	result = ldc_fill_custom(rpt_def, entry);
    	    }
    	}
    	break;
    }

    if(result == -1)
    {
    	DTNMP_DEBUG_ERR("ldc_fill_report_entry","Could not find def for MID %s",
    			        msg);
    }

    SRELEASE(msg);
    DTNMP_DEBUG_EXIT("ldc_fill_report_entry","-> %d", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: ldc_fill_custom
 *
 * \par Populate a report entry from a custom definition. This is somewhat
 *      tricky because a custom definition may, itself, contain other
 *      custom definitions.
 *
 * \param[in]  def    The custom definition
 * \param[out] entry  The entry accepting the custom definition value.
 *
 * \par Notes:
 *		- We impose a maximum nesting level of 5. A custom definition may
 *		  contain no more than 5 other custom definitions.
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/11  E. Birrane     Initial implementation.
 *  08/18/13  E. Birrane     Added nesting levels to limit recursion.
 *  07/04/15  E. Birrane     Updated to new reporting structure and TDCs
 *****************************************************************************/

int ldc_fill_custom(def_gen_t *def, rpt_entry_t *entry)
{
	LystElt elt;
	mid_t *cur_mid = NULL;
	Lyst tmp_entries = NULL;
	rpt_entry_t *tmp_entry = NULL;
	int result = 0;

	static int nesting = 0;

	DTNMP_DEBUG_ENTRY("ldc_fill_custom","("UVAST_FIELDSPEC","UVAST_FIELDSPEC")",
			          (uvast) def, (uvast) entry);

	nesting++;

	/* Step 0: Sanity Checks. */
	if((def == NULL) || (entry == NULL))
	{
		DTNMP_DEBUG_ERR("ldc_fill_custom","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("ldc_fill_custom","-->-1", NULL);
		nesting--;
		return -1;
	}

	/* Step 1: Check for too much recursion. */
	if(nesting > 5)
	{
		DTNMP_DEBUG_ERR("ldc_fill_custom","Too many nesting levels %d.", nesting);
		DTNMP_DEBUG_EXIT("ldc_fill_custom","-->-1", NULL);
		nesting--;
		return -1;
	}

	/* Step 2: Create a list to hold temporary entries. */
	if((tmp_entries = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("ldc_fill_custom","Can't create lyst.", NULL);
		nesting--;
		return -1;
	}


    /*
     * Step 3: For each MID in the definition build the data for the
     *         entry and store the result.
     */
    for (elt = lyst_first(def->contents); elt; elt = lyst_next(elt))
    {
    	uint8_t clear_parms = 0;

        /* Step 3.1 Grab the mid */
        if((cur_mid = (mid_t*)lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't get mid from lyst!", NULL);
        	result = -1;
        	break;
        }

        /* Step 3.2 If this is a parameterized MID, and the MID does not have
         * parameters, then take the parameters from the entry MID and copy them
         * into this MID.  This handles the case of a parameterized report ID.
         */
        if( ((MID_GET_FLAG_OID(cur_mid->flags) == OID_TYPE_PARAM) ||
             (MID_GET_FLAG_OID(cur_mid->flags) == OID_TYPE_COMP_PARAM)) &&

            (mid_get_num_parms(cur_mid) == 0)
		  )
		{
        	mid_copy_parms(cur_mid, entry->id);
        	clear_parms = 1;
		}


        /* Step 3.2 Create a temporary entry for this item.
         * This deep-copies cur_mid. */
        tmp_entry = rpt_entry_create(cur_mid);

        /*
         * If you copied parameters into the MID template, we have
         * now generated the entry and can clear out the parms.
         */
        if(clear_parms)
        {
        	mid_clear_parms(cur_mid);
        }

        if(tmp_entry == NULL)
        {
        	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't allocate entry.", NULL);
        	result = -1;
        	break;
        }

        /* Step 3.3 Fill in the temporary entry. */
      	if(ldc_fill_report_entry(tmp_entry) != 0)
      	{
      		DTNMP_DEBUG_ERR("ldc_fill_custom","Can't populate entry!", NULL);
      		result = -1;
      		break;
      	}

      	/* Step 3.4 Remember this entry. */
      	lyst_insert_last(tmp_entries, tmp_entry);
    }


    /* Step 4: If there was an error, roll back. */
    if(result == -1)
    {
    	/* Step 4.1: Walk the list and delete the entries. */
        for (elt = lyst_first(tmp_entries); elt; elt = lyst_next(elt))
        {
        	tmp_entry = (rpt_entry_t *) lyst_data(elt);
        	rpt_entry_release(tmp_entry);
        }
        lyst_destroy(tmp_entries);

    	DTNMP_DEBUG_EXIT("ldc_fill_custom","->-1",NULL);

    	nesting--;
    	return -1;
    }


    /*
     * Step 5: In order, add the results of all the temporary entries back to
     *         the original entry. We don't store the temporary MIDs, we are
     *         just copying back into the original entry's typed data collection.
     */

    for (elt = lyst_first(tmp_entries); elt; elt = lyst_next(elt))
    {
    	tmp_entry = (rpt_entry_t *) lyst_data(elt);

    	/* Copy into the contents area. */
        tdc_append(entry->contents, tmp_entry->contents);

    	rpt_entry_release(tmp_entry);
    }

    /* Step 6: Destroy the entries list. We destroyed list contents above. */
	lyst_destroy(tmp_entries);

	nesting--;
	return 0;
}


/******************************************************************************
 *
 * \par Function Name: ldc_fill_atomic
 *
 * \par Populate a data entry from a custom definition.
 *
 * \param[in]  adm_def  The atomic definition
 * \param[in]  id       Full ID (for OID parameters).
 * \param[out] rpt      The filled-in report.
 *
 * \par Notes:
 *		- We impose a maximum nesting level of 5. A custom definition may
 *		  contain no more than 5 other custom definitions.
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/11  E. Birrane     Initial implementation.
 *  07/04/15  E. Birrane     Updated to new reporting structure and TDCs
 *****************************************************************************/

int ldc_fill_atomic(adm_datadef_t *adm_def, rpt_entry_t *entry)
{
    uint8_t *val_data = NULL;
    uint32_t val_len = 0;

    DTNMP_DEBUG_ENTRY("ldc_fill_atomic","("UVAST_FIELDSPEC","UVAST_FIELDSPEC")",
    			      (uvast) adm_def, (uvast) entry);

    /* Step 0: Sanity Checks. */
    if((adm_def == NULL) || (entry == NULL))
    {
        DTNMP_DEBUG_ERR("ldc_fill_atomic","Bad Args", NULL);
        DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> -1.", NULL);
        return -1;
    }

    /* Step 1: Collect the information for this datum. */
    value_t result = adm_def->collect(entry->id->oid.params);

    if(result.type == DTNMP_TYPE_UNK)
    {
    	char *midstr = mid_to_string(entry->id);
    	if(midstr == NULL)
    	{
    		DTNMP_DEBUG_INFO("ldc_fill_atomic","Unable to collect NULL MID", NULL);
    	}
    	else
    	{
    		DTNMP_DEBUG_INFO("ldc_fill_atomic","Unable to collect %s", midstr);
        	SRELEASE(midstr);
    	}
    }
    else
    {
    	val_data = val_serialize(result, &val_len, 0);
    	tdc_insert(entry->contents, adm_def->type, val_data, val_len);
    	SRELEASE(val_data);
    	val_release(&result, 0);
    }

    DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> 0", NULL);
    return 0;
}


/******************************************************************************
 *
 * \par Function Name: ldc_fill_computed
 *
 * \par Populate a data entry from a computed definition.
 *
 * \param[in]  cd_def  The computed definition
 * \param[out] entry   The report entry being filled in.
 *
 * \par Notes:
 *		- We impose a maximum nesting level of 5. A custom definition may
 *		  contain no more than 5 other custom definitions.
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/31/15  E. Birrane     Initial implementation.
 *****************************************************************************/

int ldc_fill_computed(cd_t *cd, rpt_entry_t *entry)
{
    uint8_t *val_data = NULL;
    uint32_t val_len = 0;
    value_t result;

    DTNMP_DEBUG_ENTRY("ldc_fill_computed","(0x"UHF",0x"UHF")",
    			      (uvast) cd, (uvast) entry);

    /* Step 0: Sanity Checks. */
    if((cd == NULL) || (entry == NULL))
    {
        DTNMP_DEBUG_ERR("ldc_fill_computed","Bad Args", NULL);
        DTNMP_DEBUG_EXIT("ldc_fill_computed","-> -1.", NULL);
        return -1;
    }

    val_init(&result);

    /* Step 1: Collect the information for this datum. */

    if(cd->value.type == DTNMP_TYPE_EXPR)
    {
    	result = expr_eval((expr_t*)cd->value.value.as_ptr);
    }
    else
    {
    	result = val_copy(cd->value);
    }

    if(result.type == DTNMP_TYPE_UNK)
    {
    	DTNMP_DEBUG_ERR("ldc_fill_computed","Can't get value.", NULL);
    	return -1;
    }


    val_data = val_serialize(result, &val_len, 0);

    tdc_insert(entry->contents, result.type, val_data, val_len);

    SRELEASE(val_data);

    DTNMP_DEBUG_EXIT("ldc_fill_computed","-> 0", NULL);
    return 0;
}
