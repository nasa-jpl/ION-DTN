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


/**
 * \brief Populate the contents of a single report data entry.
 *
 * \author Ed Birrane
 *
 * \note We assume that the passed-in report is pre-allocated.
 * \note We do NOT fill in the report ID. This is because we call this function
 *       recursively on nest report definitions.
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * \param[in]  id   The ID of the generated report.
 * \param[out] rpt  The filled-in report.
 */
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

    if((adm_def = adm_find_datadef(id)) != NULL)
    {
    	DTNMP_DEBUG_INFO("ldc_fill_report_data","Filling pre-defined.", NULL);
    	result = ldc_fill_atomic(adm_def,id,entry);
    }
    else if((rpt_def = def_find_by_id(gAgentVDB.reports, &(gAgentVDB.reports_mutex), id)) != NULL)
    {
       	DTNMP_DEBUG_INFO("ldc_fill_report_data","Filling custom.", NULL);
       	result = ldc_fill_custom(rpt_def, entry);
       	entry->id = mid_copy(id);
    }
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



/**
 * \brief Populate a data entry from a custom definition.
 *
 * \author Ed Birrane
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * \param[in]  rpt_def  The custom definition
 * \param[out] rpt      The filled-in report.
 */

int ldc_fill_custom(def_gen_t *rpt_def, rpt_data_entry_t *rpt)
{
	Lyst entries = lyst_create();
	uint64_t total_size = 0;
	uint64_t idx = 0;
	LystElt elt;
	mid_t *cur_mid;
	rpt_data_entry_t *temp;
	int result = 0;

	DTNMP_DEBUG_ENTRY("ldc_fill_custom","(0x%x,0x%x)",
			          (unsigned long) rpt_def, (unsigned long) rpt);

    /* Step 1: For each MID in the definition...*/
    for (elt = lyst_first(rpt_def->contents); elt; elt = lyst_next(elt))
    {
        /* Step 1.1 Grab the mid */
        if((cur_mid = (mid_t*)lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't get mid from lyst!", NULL);
        	result = -1;
        	break;
        }
        else
        {
        	/* Step 1.1.1: Allocate an entry. */
        	temp = (rpt_data_entry_t*)MTAKE(sizeof(rpt_data_entry_t));
        	if(temp == NULL)
        	{
            	DTNMP_DEBUG_ERR("ldc_fill_custom","Can't get mid from lyst!", NULL);
            	result = -1;
            	break;
        	}
        	/* \todo We have no mechanism to catch infinite recursion. */
        	ldc_fill_report_data(cur_mid,temp);
        	lyst_insert_last(entries, temp);
        	total_size += temp->size;
        }
    }

    /* If we failed, free up the entries. */
    if(result == -1)
    {
    	/* Walk the list and delete the entries. */
        for (elt = lyst_first(entries); elt; elt = lyst_next(elt))
        {
        	temp = (rpt_data_entry_t *) lyst_data(elt);
        	rpt_release_data_entry(temp);
        }
        lyst_destroy(entries);
    	DTNMP_DEBUG_EXIT("ldc_fill_custom","->-1",NULL);
    	return -1;
    }

    /* Allocate space for the data entry. */
    rpt->size = total_size;
    rpt->contents = (uint8_t *) MTAKE(total_size);

    for (elt = lyst_first(entries); elt; elt = lyst_next(elt))
    {
    	temp = (rpt_data_entry_t *) lyst_data(elt);

    	/* Copy into the contents area. */
    	memcpy(&(rpt->contents[idx]), temp->contents, temp->size);
        idx += temp->size;
    	rpt_release_data_entry(temp);
    }

	lyst_destroy(entries);
	return 0;
}



/**
 * \brief Populate a data entry from a custom definition.
 *
 * \author Ed Birrane
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * \param[in]  adm_def  The atomic definition
 * \param[in]  id       Full ID (for OID parameters).
 * \param[out] rpt      The filled-in report.
 */
int ldc_fill_atomic(adm_datadef_t *adm_def, mid_t *id, rpt_data_entry_t *rpt)
{
    int i = 0;
    char *msg = NULL;
    mid_t *mid = NULL;
    uint32_t temp = 0;

    DTNMP_DEBUG_ENTRY("ldc_fill_atomic","(0x%x, 0x%x)",
    			      (unsigned long) adm_def, (unsigned long) rpt);

    /* Step 0: Sanity Checks. */
    if((adm_def == NULL) || (rpt == NULL))
    {
        DTNMP_DEBUG_ERR("ldc_fill_atomic","Bad Args", NULL);
        DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> -1.", NULL);
        return -1;
    }

    if((rpt->id = mid_copy(id)) == NULL)
    {
        DTNMP_DEBUG_ERR("ldc_fill_atomic","Unable to copy MID.", NULL);
        MRELEASE(rpt->id);
        DTNMP_DEBUG_EXIT("ldc_fill_atomic","-> -1.", NULL);
        return -1;
    }

    expr_result_t result = adm_def->collect(id->oid->params);
    rpt->contents = result.value;
    rpt->size = result.length;

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
