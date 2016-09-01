/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: lcc.c
 **
 ** Description: This implements the NM Agent Local Command and Control (LCC).
 **              This applies controls and macros.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/22/13  E. Birrane     Update to latest version of DTNMP. Cleanup. (JHU/APL)
 **  05/17/15  E. Birrane     Add Macro support, updated to DTNMP v0.1 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "../shared/adm/adm.h"
#include "../shared/primitives/mid.h"
#include "../shared/primitives/rules.h"
#include "instr.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/report.h"

#include "nmagent.h"
#include "lcc.h"
#include "rda.h"



/******************************************************************************
 *
 * \par Function Name: lcc_run_ctrl_mid
 *
 * \par Run a control given the MID associated with that control.
 *
 * \param[in]  id   The MID identifying the control
 *
 * \par Notes:
 *
 * \return -1 - Error
 *         !(-1) - Value returned from the run control.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int lcc_run_ctrl_mid(mid_t *id)
{
	int result = 0;
    int8_t status = 0;
    adm_ctrl_t *adm_ctrl = NULL;
    def_gen_t *macro_def = NULL;
    static int nesting = 0;
    char *msg = NULL;

    nesting++;

    AMP_DEBUG_ENTRY("lcc_run_ctrl_mid","(0x%#llx)", (unsigned long) id);


    /* Step 0: Sanity Check */
    if((id == NULL) || (id->oid.type == OID_TYPE_UNK))
    {
    	AMP_DEBUG_ERR("lcc_run_ctrl_mid","Bad Args.",NULL);
    	AMP_DEBUG_EXIT("lcc_run_ctrl_mid","-> -1",NULL);
    	nesting--;
    	return -1;
    }

    if(nesting > 5)
    {
    	AMP_DEBUG_ERR("lcc_run_ctrl_mid","Too many nesting levels (%d).",nesting);
    	AMP_DEBUG_EXIT("lcc_run_ctrl_mid","-> -1",NULL);
    	nesting--;
    	return -1;
    }

    msg = mid_to_string(id);
    AMP_DEBUG_INFO("lcc_run_ctrl_mid","Running control: %s", msg);

    /* Step 1: See if this identifies an atomic control. */
    if((adm_ctrl = adm_find_ctrl(id)) != NULL)
    {
        tdc_t *retval = NULL;

    	AMP_DEBUG_INFO("lcc_run_ctrl_mid","Found control.", NULL);

    	retval = adm_ctrl->run(&manager_eid, id->oid.params, &status);

    	if(status != CTRL_SUCCESS)
    	{
    		AMP_DEBUG_WARN("lcc_run_ctrl_mid","Error running control.", NULL);
    	}
    	else if(retval != NULL)
    	{
    		lcc_send_retval(&manager_eid, retval, id);
    		tdc_destroy(&retval);
    	}

    	gAgentInstr.num_ctrls_run++;
    }
    else
    {
    	if((macro_def = def_find_by_id(gAgentVDB.macros, &(gAgentVDB.macros_mutex), id)) == NULL)
    	{
    		macro_def = def_find_by_id(gAdmMacros, NULL, id);
    	}

    	if(macro_def != NULL)
    	{
    		if(lcc_run_macro(macro_def->contents) != 0)
    		{
    			AMP_DEBUG_ERR("lcc_run_ctrl_mid","Error running macro %s.", msg);
    		}
    	}
        /* Step 3: Otherwise, give up. */
        else
        {
        	AMP_DEBUG_ERR("lcc_run_ctrl_mid","Could not find control for MID %s",
        			        msg);
        	result = -1;
        }
    }

    SRELEASE(msg);

    nesting--;
    AMP_DEBUG_EXIT("lcc_run_ctrl_mid","-> %d", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: lcc_run_ctrl
 *
 * \par Run a control given a control execution structure.
 *
 * \param[in]  ctrl_p  The control execution structure.
 *
 * \par Notes:
 *
 * \return -1 - Error
 *         !(-1) - Value returned from the run control.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int lcc_run_ctrl(ctrl_exec_t *ctrl_p)
{
	int8_t status = CTRL_FAILURE;
    tdc_t* retval = NULL;

	AMP_DEBUG_ENTRY("lcc_run_ctrl","(0x%x)", (unsigned long) ctrl_p);

	/* Step 0: Sanity Check */
	if((ctrl_p == NULL) || (ctrl_p->adm_ctrl == NULL) || (ctrl_p->adm_ctrl->run == NULL))
	{
		AMP_DEBUG_ERR("lcc_run_ctrl","Bad Args.",NULL);
		AMP_DEBUG_EXIT("lcc_run_ctrl","-> -1",NULL);
		return -1;
	}

	retval = ctrl_p->adm_ctrl->run(&(ctrl_p->sender), ctrl_p->mid->oid.params, &status);

	gAgentInstr.num_ctrls_run++;

	if(status != CTRL_SUCCESS)
	{
		AMP_DEBUG_WARN("lcc_run_ctrl","Error running control.", NULL);
	}
	else if(retval != NULL)
	{
		eid_t *recipient = &(ctrl_p->sender);

		if(recipient == NULL)
		{
			recipient = &manager_eid;
		}

		lcc_send_retval(recipient, retval, ctrl_p->mid);
		tdc_destroy(&retval);
	}

	AMP_DEBUG_EXIT("lcc_run_ctrl","-> %d", status);
	return status;
}


int lcc_run_macro(Lyst macro)
{
	LystElt elt = NULL;
	mid_t *cur_mid = NULL;

	if(macro == NULL)
	{
		AMP_DEBUG_ERR("lcc_run_macro","Bad Args", NULL);
		return -1;
	}

	for(elt = lyst_first(macro); elt; elt = lyst_next(elt))
	{
		cur_mid = (mid_t *) lyst_data(elt);

		/* If one control is in error, we continue with the macro. */
		lcc_run_ctrl_mid(cur_mid);
	}

	return 0;
}


/******************************************************************************
 *
 * \par Function Name: lcc_send_retval
 *
 * \par Sends back to a manager the return value of a function call. The
 *      return value is captured as a Data Collection (DC).
 *
 * \todo Make the rx a list of managers.
 *
 * \param[in]  rx		The Manager to receive this result.
 * \param[in]  retval	The DC to send back, or NULL.
 * \param[in]  mid		The control MID generating this retval.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/21/15  E. Birrane     Initial implementation.
 *  06/28/15  E. Birrane     Implemented TDCs
 *****************************************************************************/

void lcc_send_retval(eid_t *rx, tdc_t *retval, mid_t *mid)
{
	rpt_t *report = NULL;
	rpt_entry_t *entry = NULL;

	/* Step 0: Sanity Checks. */
	if((rx == NULL) || (mid == NULL))
	{
		AMP_DEBUG_ERR("lcc_send_retval","Bad Args.", NULL);
		return;
	}


	/*
	 * This function will grab an existing to-be-sent report or
	 * create a new report and add it to the "built-reports" section.
	 * Either way, it will be included in the next set of code to send
	 * out reports built in this time slice.
	 */
	if((report = rda_get_report(*rx)) != NULL)
	{
		if((entry = (rpt_entry_t*) STAKE(sizeof(rpt_entry_t))) != NULL)
		{
			entry->id = mid_copy(mid);
			entry->contents = tdc_copy(retval);
			lyst_insert_last(report->entries, entry);
		}
	}

}

