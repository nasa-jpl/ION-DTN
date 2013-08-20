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
#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_def.h"

#include "nmagent.h"
#include "ldc.h"


/******************************************************************************
 *
 * \par Function Name: ldc_fill_report_data
 *
 * \par Populate the contents of a single report data entry.
 *
 * \param[in]  id     The ID of the generated report.
 * \param[out] entry  The filled-in report.
 *
 * \par Notes:
 *		- We assume that the passed-in report is pre-allocated.
 * 		- We do NOT fill in the report ID. This is because we call this function
 *        recursively on nested report definitions.
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/11  E. Birrane     Initial implementation.
 *  08/18/13  E. Birrane     Added nesting levels to limit recursion.
 *****************************************************************************/

int ldc_fill_report_data(mid_t *id, rpt_data_entry_t *entry)
{
    int result = 0;
    adm_datadef_t *adm_def = NULL;
    def_gen_t *rpt_def = NULL;
    char *msg = NULL;

    DTNMP_DEBUG_ENTRY("ldc_fill_report_data","(0x%x,0x%x)",
    		          (unsigned long) id, (unsigned long) entry);

    /* Step 0: Sanity Check */
    if((id == NULL) || (entry == NULL))
    {
    	DTNMP_DEBUG_ERR("ldc_fill_report_data","Bad Args.",NULL);
    	DTNMP_DEBUG_EXIT("ldc_fill_report_data","-> -1",NULL);
    	return -1;
    }

    msg = mid_to_string(id);
    DTNMP_DEBUG_INFO("ldc_fill_report_data","Gathering report data for MID: %s",
    		         msg);

    /* Step 1: Search for this MID... */

    /* Step 1.1: If this is an atomic data definition...*/
    if((adm_def = adm_find_datadef(id)) != NULL)
    {
    	DTNMP_DEBUG_INFO("ldc_fill_report_data","Filling pre-defined.", NULL);
    	result = ldc_fill_atomic(adm_def,id,entry);
    }

    /* Step 1.2: If this is a data report...*/
    else if((rpt_def = def_find_by_id(gAgentVDB.reports, &(gAgentVDB.reports_mutex), id)) != NULL)
    {
       	DTNMP_DEBUG_INFO("ldc_fill_report_data","Filling custom.", NULL);
       	result = ldc_fill_custom(rpt_def, entry);

       	/* \todo: Do we need this? */
       	entry->id = mid_copy(id);
    }

    /* Step 1.3: If this is an unknown data MID. */
    else
    {
    	DTNMP_DEBUG_ERR("ldc_fill_report_data","Could not find def for MID %s",
    			        msg);
    	result = -1;
    }

    MRELEASE(msg);
    DTNMP_DEBUG_EXIT("ldc_fill_report_data","-> %d", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: ldc_fill_custom
 *
 * \par Populate a data entry from a custom definition. This is somewhat
 *      tricky because a custom definition may, itself, contain other
 *      custom definitions.
 *
 * \param[in]  rpt_def  The custom definition
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
 *  08/18/13  E. Birrane     Added nesting levels to limit recursion.
 *****************************************************************************/

int ldc_fill_custom(def_gen_t *rpt_def, rpt_data_entry_t *rpt)
{
	Lyst entries;
	uint64_t total_size = 0;
	uint64_t idx = 0;
	LystElt elt;
	mid_t *cur_mid = NULL;
	rpt_data_entry_t *temp = NULL;
	int result = 0;
	static int nesting = 0;

	DTNMP_DEBUG_ENTRY("ldc_fill_custom","(0x%x,0x%x)",
			          (unsigned long) rpt_def, (unsigned long) rpt);

	nesting++;

	/* Step 0: Sanity Checks. */
	if((rpt_def == NULL) || (rpt == NULL))
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

	/*
	 * Step 2: Allocate a lyst to hold individual reports from each MID
	 *         in this custom definition.
	 */
	if((entries = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("ldc_fill_custom","Can't allocate lyst.", NULL);
		DTNMP_DEBUG_EXIT("ldc_fill_custom","-->-1", NULL);
		nesting--;
		return -1;
	}

    /*
     * Step 3: For each MID in the definition build the data for the
     *         report and store the result in a temporary area.
     */
    for (elt = lyst_first(rpt_def->contents); elt; elt = lyst_next(elt))
    {
        /* Step 3.1 Grab the mid */
        if((cur_mid = (mid_t*)lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't get mid from lyst!", NULL);
        	result = -1;
        	break;
        }
    	/* Step 3.2: Grab the report for this MID. */
        else
        {
        	/* Step 3.2.1: Allocate space for this report. */
        	if((temp = (rpt_data_entry_t*)MTAKE(sizeof(rpt_data_entry_t))) == NULL)
        	{
            	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't allocate %d bytes!",
            			        sizeof(rpt_data_entry_t));
            	result = -1;
            	break;
        	}

        	/* Step 3.2.2: Populate the report. */
        	if(ldc_fill_report_data(cur_mid,temp) != 0)
        	{
            	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't get mid from lyst!", NULL);
            	rpt_release_data_entry(temp);
            	result = -1;
            	break;
        	}

        	/* Step 3.2.3: Add this report to the report list. */
        	lyst_insert_last(entries, temp);

        	/* Step 3.2.4: Remember the total size of this report. */
        	total_size += temp->size;
        }
    }

    /* Step 4: Allocate total space for the resultant report. */
    rpt->size = total_size;

    if((rpt->contents = (uint8_t *) MTAKE(rpt->size)) == NULL)
    {
    	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't allocate %d bytes!", rpt->size);
    	rpt->size = 0;
    	result = -1;
    }

    /*
     * Step 5: If any of the report generation failed, or the allocation of the
     *         consolidated report from step 4 failed, clean up.
     */
    if(result == -1)
    {
    	/* Step 4.1: Walk the list and delete the entries. */
        for (elt = lyst_first(entries); elt; elt = lyst_next(elt))
        {
        	temp = (rpt_data_entry_t *) lyst_data(elt);
        	rpt_release_data_entry(temp);
        }
        lyst_destroy(entries);

    	DTNMP_DEBUG_EXIT("ldc_fill_custom","->-1",NULL);

    	nesting--;
    	return -1;
    }

    /*
     * Step 6: Copy all the comprising reports into the single, consolidated
     *         report. Note that we destroy the comprising reports as we go.
     */
    for (elt = lyst_first(entries); elt; elt = lyst_next(elt))
    {
    	temp = (rpt_data_entry_t *) lyst_data(elt);

    	/* Copy into the contents area. */
    	memcpy(&(rpt->contents[idx]), temp->contents, temp->size);
        idx += temp->size;
    	rpt_release_data_entry(temp);
    }

    /* Step 7: Destroy the entries list. We destroyed list contents above. */
	lyst_destroy(entries);

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
 *****************************************************************************/

int ldc_fill_atomic(adm_datadef_t *adm_def, mid_t *id, rpt_data_entry_t *rpt)
{
    int i = 0;
    char *msg = NULL;
    mid_t *mid = NULL;
    uint32_t temp = 0;

    DTNMP_DEBUG_ENTRY("ldc_fill_atomic","(0x%x, 0x%x)",
    			      (unsigned long) adm_def, (unsigned long) rpt);

    /* Step 0: Sanity Checks. */
    if((adm_def == NULL) || (id == NULL) || (rpt == NULL))
    {
        DTNMP_DEBUG_ERR("ldc_fill_atomic","Bad Args", NULL);
        DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> -1.", NULL);
        return -1;
    }

    /* Step 1: Populate the report MID. */
    if((rpt->id = mid_copy(id)) == NULL)
    {
        DTNMP_DEBUG_ERR("ldc_fill_atomic","Unable to copy MID.", NULL);
        DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> -1.", NULL);
        return -1;
    }

    /* Step 2: Collect the information for this datum. */
    expr_result_t result = adm_def->collect(id->oid->params);
    rpt->contents = result.value;
    rpt->size = result.length;

    /* Step 3: If there was a problem collecting information, bail. */
    if((rpt->size == 0) || (rpt->contents == NULL))
    {
        DTNMP_DEBUG_ERR("ldc_fill_atomic","Unable to collect data.", NULL);
        MRELEASE(rpt->id);
        MRELEASE(rpt->contents);
        DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> -1.", NULL);
        return -1;
    }

    DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> 0", NULL);
    return 0;
}


/*
 * \todo: Add fill computed data.
 */
