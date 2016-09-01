/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 ** \file rda.c
 ** 
 ** File Name: rda.c
 **
 **
 ** Subsystem:
 **          Network Management Utilities: DTNMP Agent
 **
 ** Description: This file implements the DTNMP Agent's Remote Data
 **              Aggregator thread.  Periodically, this thread runs to evaluate
 **              what production rules are queued for execution, runs these
 **              rules, constructs the appropriate data reports, and queues
 **              them for transmission.
 **
 ** Notes:  
 **
 ** Assumptions:
 **      1. We assume that this code is not under such tight profiling
 **         constraints that sanity checks are too expensive.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/06/11  M. Reid        Initial Implementation (JHU/APL)
 **  10/21/11  E. Birrane     Code comments and functional updates. (JHU/APL)
 **  06/27/13  E. Birrane     Support persisted rules. (JHU/APL)
 **  08/30/15  E. Birrane     Updated support for SRL/TRL (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"
#include "lyst.h"

#include "../shared/utils/utils.h"
#include "../shared/primitives/mid.h"
#include "instr.h"
#include "../shared/primitives/expr.h"

#include "../shared/utils/db.h"

#include "nmagent.h"
#include "ldc.h"
#include "lcc.h"

#include <pthread.h>

Lyst g_rda_cur_rpts;
Lyst g_rda_trls_pend;
Lyst g_rda_srls_pend;

ResourceLock g_rda_cur_rpts_mutex;
ResourceLock g_rda_trls_pend_mutex;
ResourceLock g_rda_srls_pend_mutex;

/******************************************************************************
 *
 * \par Function Name: rda_cleanup
 *
 * \par Purpose: Cleans up any resources left over by the RDA when it exits.
 *
 * \retval void
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *  05/20/15  E. Birrane     Switched to global, mutex-protected lysts.
 *****************************************************************************/

void rda_cleanup()
{

	lockResource(&g_rda_trls_pend_mutex);
	lockResource(&g_rda_srls_pend_mutex);

	midcol_destroy(&g_rda_trls_pend);
	midcol_destroy(&g_rda_srls_pend);

    unlockResource(&g_rda_trls_pend_mutex);
    unlockResource(&g_rda_srls_pend_mutex);
    
    killResourceLock(&g_rda_trls_pend_mutex);
    killResourceLock(&g_rda_srls_pend_mutex);


	lockResource(&g_rda_cur_rpts_mutex);

	rpt_clear_lyst(&g_rda_cur_rpts, NULL, 0);
    lyst_destroy(g_rda_cur_rpts);

    unlockResource(&g_rda_cur_rpts_mutex);

    killResourceLock(&g_rda_cur_rpts_mutex);
}



/******************************************************************************
 *
 * \par Function Name: rda_get_report
 *
 * \par Purpose: Find the data report intended for a given recipient. The
 *               agent will, when possible, combine reports for a single
 *               recipient.
 *
 * \param[in]  recipient     - The recipient for which we are searching for
 *                             a report.
 *
 * \par Notes:
 *
 * \return !NULL - Report for this recipient.
 *         NULL  - Error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *  05/20/15  E. Birrane     Switched to using global lyst
 *  07/04/15  E. Birrane     Refactored report type and TDC support.
 *****************************************************************************/

rpt_t *rda_get_report(eid_t recipient)
{
    LystElt elt;
    rpt_t *cur_report = NULL;
    rpt_t *result = NULL;
    
    AMP_DEBUG_ENTRY("rda_get_report","(%s)", recipient.name);
    
    /* Step 0: Sanity check. */
    if(strlen(recipient.name) == 0)
    {
    	AMP_DEBUG_ERR("rda_get_report","Bad parms.",NULL);
    	return NULL;
    }

    lockResource(&g_rda_cur_rpts_mutex);

    /* Step 1: Search the list of reports identified so far. */
    for (elt = lyst_first(g_rda_cur_rpts); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the current report */
        cur_report = (rpt_t*) lyst_data(elt);
        
        /* Step 1.2: Check if this report is destined for our recipient. */
        if(strcmp(cur_report->recipient.name, recipient.name) == 0)
        {
            AMP_DEBUG_INFO("rda_get_report",
            		         "Found existing report for recipient %s", recipient);

            /* Step 1.2.1: Remeber report if it is a match. */
            result = cur_report;

            /* Step 1.2.3: Currently, we only match on 1 recipient, so, break.*/
            break;
        }
    }

    /* 
     * Step 2: If there is no matching report, we will need to create a new
     *         one for this recipient.
     */
    if(result == NULL)
    {
    	eid_t rx;
    	strcpy(rx.name,recipient.name);
    	result = rpt_create((uint32_t)getUTCTime(), lyst_create(), rx);

    	AMP_DEBUG_INFO("rda_get_report","New report for recipient %s", recipient);

        lyst_insert_first(g_rda_cur_rpts, result);
    }

    unlockResource(&g_rda_cur_rpts_mutex);

    AMP_DEBUG_EXIT("rda_get_report","->0x%x", (unsigned long) result);
    
    return result;
}



/******************************************************************************
 *
 * \par Function Name: rda_scan_rules
 *
 * \par Purpose: Walks the list of rules defined by this agent, determines
 *               which rules are to be executed, and updates housekeeping
 *               information for each rule.
 *
 * \retval int -  0 : Success
 *               -1 : Failure
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *  05/20/15  E. Birrane     Moved to global, mutex-protected lyst
 *  08/14/15  E. Birrane     Added SRL processing.
 *****************************************************************************/

int rda_scan_rules()
{
    LystElt elt;
    trl_t *trl = NULL;
    srl_t *srl = NULL;
    
    AMP_DEBUG_ENTRY("rda_scan_rules","()", NULL);

    
    /* Step 0: Start with a fresh list for pending rules */
    lockResource(&g_rda_trls_pend_mutex);
    lockResource(&g_rda_srls_pend_mutex);
    midcol_clear(g_rda_trls_pend);
/***
    if(lyst_length(g_rda_srls_pend) > 0)
    {
    	LystElt elt;
    	int i = 1;
    	for(elt = lyst_first(g_rda_srls_pend);elt;elt = lyst_next(elt))
    	{
    		mid_t *mid = lyst_data(elt);
    		char *midstr = mid_to_string(mid);
    		SRELEASE(midstr);
    		i++;
    	}
    }
***/
    midcol_clear(g_rda_srls_pend);

    unlockResource(&g_rda_trls_pend_mutex);
    unlockResource(&g_rda_srls_pend_mutex);
    
    /* 
     * Step 1: Walk through each defined time rule and see if it should be
     *         included in the current evaluation scan.
     */    
    for (elt = lyst_first(gAgentVDB.trls); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next rule...*/
        if((trl = (trl_t *) lyst_data(elt)) == NULL)
        {
            AMP_DEBUG_ERR("rda_scan_rules","Found NULL TRL. Exiting", NULL);
            AMP_DEBUG_EXIT("rda_scan_rules","->-1.", NULL);
            return -1;
        }
                
        /* Step 1.2: Determine if this rule is ready for possible evaluation. */
        if(trl->countdown_ticks == 0)
        {
            /*
             * Step 1.2.1: Determine if this rule has been evaluated more than
             *             its maximum number of evaluations
             */
            if((trl->desc.num_evals > 0) ||
               (trl->desc.num_evals == DTNMP_RULE_EXEC_ALWAYS))
            {
            	/* Step 1.2.2: If ready, add rule to list of rules pending
            	 *             evaluation in this current tick.
            	 */
            	mid_t *tmp = NULL;

                lockResource(&g_rda_trls_pend_mutex);
                if((tmp = mid_copy(trl->mid)) == NULL)
                {
                	AMP_DEBUG_ERR("rda_scan_rules","Can't copy TRL MID. Skipping from next eval.", NULL);
                }
                else
                {
                	lyst_insert_first(g_rda_trls_pend, tmp);
                	AMP_DEBUG_INFO("rda_scan_rules","Added rule to evaluate list.",
                    		         NULL);
                }
                unlockResource(&g_rda_trls_pend_mutex);

            }
        }
        else
        {
        	/* Step 1.2.2: If not ready, note that another tick has elapsed. */
            trl->countdown_ticks--;
        }                
    }
    

    /*
     * Step 2: Walk through each defined state rule and see if it should be
     *         included in the current evaluation scan.
     */
    lockResource(&(gAgentVDB.srls_mutex));

    for (elt = lyst_first(gAgentVDB.srls); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next rule...*/
        if((srl = (srl_t *) lyst_data(elt)) == NULL)
        {
        	unlockResource(&(gAgentVDB.srls_mutex));
            AMP_DEBUG_ERR("rda_scan_rules","Found NULL SRL. Exiting", NULL);
            AMP_DEBUG_EXIT("rda_scan_rules","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Determine if this rule is ready for possible evaluation. */
        if(srl->countdown_ticks == 0)
        {
            /*
             * Step 1.2.1: Determine if this rule has been evaluated more than
             *             its maximum number of evaluations
             */
            if((srl->desc.num_evals > 0) ||
               (srl->desc.num_evals == DTNMP_RULE_EXEC_ALWAYS))
            {
            	/* Step 1.2.2: If ready, add rule to list of rules pending
            	 *             evaluation in this current tick.
            	 */
            	value_t val = expr_eval(srl->expr);
            	uint8_t success;
            	uint32_t result;
            	result = val_cvt_uint(val, &success);
            	if(success == 0)
            	{
            		srl->desc.num_evals = 0;
            		AMP_DEBUG_ERR("rda_scan_rules", "Can't convert to uint. Stopping eval of SRL.", NULL);
            	}
            	// If the expression is true, we add the SRL to list of SRLs whose action is now pending to run.
            	else if(result != 0)
            	{
                	mid_t *tmp = NULL;

            		lockResource(&g_rda_srls_pend_mutex);

                    if((tmp = mid_copy(srl->mid)) == NULL)
                    {
                    	AMP_DEBUG_ERR("rda_scan_rules","Can't copy SRL MID. Skipping from next eval.", NULL);
                    }
                    else
                    {

                    	lyst_insert_last(g_rda_srls_pend, tmp);

                    	AMP_DEBUG_INFO("rda_scan_rules","Added rule to evaluate list.",
                    					NULL);
                    }

            		unlockResource(&g_rda_srls_pend_mutex);
            	}
            }
        }
        else
        {
        	/* Step 1.2.2: If not ready, note that another tick has elapsed. */
            srl->countdown_ticks--;
        }
    }

    unlockResource(&(gAgentVDB.srls_mutex));

    AMP_DEBUG_EXIT("rda_scan_rules","->0.", NULL);
    return 0;
}
 


/******************************************************************************
 *
 * \par Function Name: rda_scan_ctrls
 *
 * \par Purpose: Walks the list of ctrls defined by this agent, determines
 *               which ctrls are to be executed, and updates housekeeping
 *               information for each ctrl.
 *
 * \retval int -  0 : Success
 *               -1 : Failure
 *
 * \param[in,out]  exec_defs - List of ctrls that should be executed during
 *                             this execution period.
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/
int rda_scan_ctrls(Lyst exec_defs)
{
    LystElt elt;
    ctrl_exec_t *ctrl_p = NULL;

    AMP_DEBUG_ENTRY("rda_scan_ctrls","(0x%#llx)", (unsigned long)exec_defs);

    /*
     * Step 1: Walk through each defined ctrl and see if it should be included
     *         in the current evaluation scan.
     */
    for (elt = lyst_first(exec_defs); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next ctrl...*/
        if((ctrl_p = (ctrl_exec_t *) lyst_data(elt)) == NULL)
        {
            AMP_DEBUG_ERR("rda_scan_ctrls","Found NULL ctrl. Exiting", NULL);
            AMP_DEBUG_EXIT("rda_scan_ctrls","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Determine if this rule is ready for possible evaluation. */
        if(ctrl_p->status == CONTROL_ACTIVE)
        {
        	/* Step 1.3: If the control is ready to execute, run it. */
        	if(ctrl_p->countdown_ticks <= 0)
        	{
        		lcc_run_ctrl(ctrl_p);

        		/* Step 1.3.1: controls disable after they fire.*/
        		ctrl_p->status = CONTROL_INACTIVE;
        	}
        	/* Step 1.4: If the control is not ready, note a tick elapsed. */
        	else
        	{
        		ctrl_p->countdown_ticks--;
        	}
        }
    }

    AMP_DEBUG_EXIT("rda_scan_ctrls","->0.", NULL);
    return 0;
}


/******************************************************************************
 *
 * \par Function Name: rda_clean_ctrls
 *
 * \par Purpose: Walks the list of ctrls defined by this agent, determines
 *               which ctrls are to be removed based on active status, and
 *               forgets them from the database.
 *
 * \retval int -  0 : Success
 *               -1 : Failure
 *
 * \param[in,out]  exec_defs - List of ctrls
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/22/13  E. Birrane     Initial implementation,
 *****************************************************************************/
int rda_clean_ctrls(Lyst exec_defs)
{
    LystElt elt;
    LystElt del_elt;
    ctrl_exec_t *ctrl_p = NULL;

    AMP_DEBUG_ENTRY("rda_clean_ctrls","(0x%#llx)", (unsigned long)exec_defs);

    /*
     * Step 1: Walk through each defined ctrl and see if it should be included
     *         in the current evaluation scan.
     */
    for (elt = lyst_first(exec_defs); elt; )
    {
        /* Step 1.1: Grab the next ctrl...*/
        if((ctrl_p = (ctrl_exec_t *) lyst_data(elt)) == NULL)
        {
            AMP_DEBUG_ERR("rda_clean_ctrls","Found NULL ctrl. Exiting", NULL);
            AMP_DEBUG_EXIT("rda_clean_ctrls","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Determine if this rule should be removed. */
        if(ctrl_p->status != CONTROL_ACTIVE)
        {
        	/* Step 1.2.1: Remove control from the memory list. */
        	del_elt = elt;
        	elt = lyst_prev(elt);
        	lyst_delete(del_elt);

        	/* Step 1.2.2: Remove control from the persistent store. */
        	db_forget(&(ctrl_p->desc.itemObj),
        			  &(ctrl_p->desc.descObj),
        			  gAgentDB.ctrls);

        	/* Step 1.2.3: Release the control object. */
        	ctrl_release(ctrl_p);
        }

        if(elt != NULL)
        {
        	elt = lyst_next(elt);
        }
    }

    AMP_DEBUG_EXIT("rda_clean_ctrls","->0.", NULL);
    return 0;
}


/******************************************************************************
 *
 * \par Function Name: rda_build_report_entry
 *
 * \par Purpose: Create a report entry containing data for a given report MID
 *               and filled from the agent Local Data Collector.
 *
 * \return  NULL - Error
 *         !NULL - Constructed report.
 *
 * \param[in]  mid  - The MID identifying the report to populate.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/

rpt_entry_t *rda_build_report_entry(mid_t *mid)
{
	rpt_entry_t *entry = NULL;
    
    AMP_DEBUG_ENTRY("rda_build_report_entry","(0x%x)", (unsigned long) mid);
    
    /* Step 1: Make sure we have an entry structure to populate. */
    if((entry = rpt_entry_create(mid)) == NULL)
    {
        AMP_DEBUG_ERR("rda_build_report_entry","Can't create new entry.", NULL);
        AMP_DEBUG_EXIT("rda_build_report_entry","->NULL.", NULL);
        return NULL;
    }

    /* Step 2: Fill the report with data. */
    if(ldc_fill_report_entry(entry) == -1)
    {
        AMP_DEBUG_ERR("rda_build_report_entry","Can't fill report.",NULL);
        rpt_entry_release(entry);

        AMP_DEBUG_EXIT("rda_build_report_entry","->NULL.", NULL);
        return NULL;
    }

    AMP_DEBUG_EXIT("rda_build_report_entry","->0x%x", (unsigned long) entry);
    return entry;
}

/******************************************************************************
 *
 * \par Function Name: rda_eval_pending_rules
 *
 * \par Purpose: Walks the list of rules flagged for evaluation and evaluates
 *               them, taking the appropriate action for each rule.
 *
 * \retval int -  0 : Success
 *               -1 : Failure
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *  05/20/15  E. Birrane     Updated to use global, mutex-protected lysts
 *****************************************************************************/

int rda_eval_pending_rules()
{
    
    LystElt pending_elt;
	mid_t *mid = NULL;
    trl_t *trl = NULL;
    srl_t *srl = NULL;
    
    AMP_DEBUG_ENTRY("rda_eval_pending_rules","()", NULL);

    lockResource(&g_rda_trls_pend_mutex);

    AMP_DEBUG_INFO("rda_eval_pending_rules","Preparing to eval%d rules.",
    		         lyst_length(g_rda_trls_pend));

    for (pending_elt = lyst_first(g_rda_trls_pend); pending_elt; pending_elt = lyst_next(pending_elt))
    {
        /* Grab the next rule mid*/
    	if((mid = (mid_t *) lyst_data(pending_elt)) == NULL)
        {
            AMP_DEBUG_ERR("rda_eval_pending_rules",
            		        "Cannot find pending rule.", NULL);

            unlockResource(&g_rda_trls_pend_mutex);

            AMP_DEBUG_EXIT("rda_eval_pending_rules","-> -1.", NULL);
            return -1;
        }
        
    	/*
    	 * Grab the rule associated with the TRL. Not finding
    	 * a rule isn't necessarily an error - a control may
    	 * have removed the rule since its MID was added to the
    	 * pending list.
    	 */
    	if((trl = agent_vdb_trl_find(mid)) != NULL)
    	{
    		Lyst action;
    		/* Evaluate the rule */

    		gAgentInstr.num_trls_run++;

    		/* First, copy the macro, in case the TRL deletes itself as its action. */
    		action = midcol_copy(trl->action);
    		lcc_run_macro(action);
    		midcol_destroy(&action);

    		/* Note that the rule has been evaluated */
    		if(trl->desc.num_evals > 0)
    		{
    			AMP_DEBUG_INFO("rda_eval_pending_rules",
    					         "Decrementing rule eval count from %d.",
    					         trl->desc.num_evals);
    			trl->desc.num_evals--;
    		}
    	}
    }

    unlockResource(&g_rda_trls_pend_mutex);



    /* SRLS */
    lockResource(&g_rda_srls_pend_mutex);

    AMP_DEBUG_INFO("rda_eval_pending_rules","Preparing to eval %d SRLs.",
    		         lyst_length(g_rda_srls_pend));

    for (pending_elt = lyst_first(g_rda_srls_pend); pending_elt; pending_elt = lyst_next(pending_elt))
    {
        /* Grab the next rule mid*/
    	if((mid = (mid_t *) lyst_data(pending_elt)) == NULL)
        {
            AMP_DEBUG_ERR("rda_eval_pending_rules",
            		        "Cannot find pending rule.", NULL);

            unlockResource(&g_rda_srls_pend_mutex);

            AMP_DEBUG_EXIT("rda_eval_pending_rules","-> -1.", NULL);
            return -1;
        }

    	/*
    	 * Grab the rule associated with the TRL. Not finding
    	 * a rule isn't necessarily an error - a control may
    	 * have removed the rule since its MID was added to the
    	 * pending list.
    	 */
    	if((srl = agent_vdb_srl_find(mid)) != NULL)
        {
    		Lyst action;
    		/* Evaluate the rule */

    		gAgentInstr.num_srls_run++;

    		/* First, copy the macro, in case the TRL deletes itself as its action. */
    		action = midcol_copy(srl->action);
    		lcc_run_macro(action);
    		midcol_destroy(&action);

    		/* Note that the rule has been evaluated */
    		if(srl->desc.num_evals > 0)
    		{
    			AMP_DEBUG_INFO("rda_eval_pending_rules",
        		   	             "Decrementing rule eval count from %d.",
        		   	             srl->desc.num_evals);
    			srl->desc.num_evals--;
    		}

    	//	srl_release(srl);
        }
    }

    unlockResource(&g_rda_srls_pend_mutex);


    AMP_DEBUG_EXIT("rda_eval_pending_rules","-> 0", NULL);
    return 0;
}



/******************************************************************************
 *
 * \par Function Name: rda_send_reports
 *
 * \par Purpose: For each report constructed during this evaluation period,
 *               create a message and send it.
 *
 * \retval int -  0 : Success
 *               -1 : Failure
 *
 * \par Notes:
 *		- When we construct the reports, we build one compound report
 *		  per recipient. By the time we get to this function, we should have
 *		  one report per recipient, so making one message per report should
 *		  not result in multiple messages to the same recipient.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *  05/20/15  E. Birrane     Updated to use global, mutex-protected lyst
 *****************************************************************************/

int rda_send_reports()
{
    LystElt report_elt;
    rpt_t *report = NULL;
    uint8_t *raw_report = NULL;
    uint32_t raw_report_len = 0;
    
    AMP_DEBUG_ENTRY("rda_send_reports","()", NULL);

    lockResource(&g_rda_cur_rpts_mutex);
    
    AMP_DEBUG_INFO("rda_send_reports","Preparing to send %d reports.",
    		         lyst_length(g_rda_cur_rpts));
    

    /* Step 1: For each report that has been built... */
    for (report_elt = lyst_first(g_rda_cur_rpts); report_elt; report_elt = lyst_next(report_elt))
    {
        /*
         * Step 1.1: Grab the report. Bail if the report is bad. It is better to
         * send no reports than to send potentially garbled reports. If the
         * report here is NULL, something has clearly gone wrong on the system.
         */
        if((report = (rpt_t*) lyst_data(report_elt)) == NULL)
        {
            AMP_DEBUG_ERR("rda_send_reports","Can't find report from elt %x.",
            		        report_elt);

            unlockResource(&g_rda_cur_rpts_mutex);

            AMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Construct a message for the report. */
       	pdu_msg_t *pdu_msg = NULL;
       	pdu_group_t *pdu_group = NULL;

        /* Step 1.3: Serialize the payload. */
        if((raw_report = rpt_serialize(report, &raw_report_len)) == NULL)
        {
        	AMP_DEBUG_ERR("rda_send_reports","Can't serialize report.",NULL);

            unlockResource(&g_rda_cur_rpts_mutex);

        	AMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        /* Step 1.4: Construct the containers. */
        if((pdu_msg = pdu_create_msg(MSG_TYPE_RPT_DATA_RPT,
        		                 raw_report, raw_report_len, NULL)) == NULL)
        {
        	AMP_DEBUG_ERR("rda_send_reports","Can't serialize report.",NULL);
        	SRELEASE(raw_report);

            unlockResource(&g_rda_cur_rpts_mutex);

        	AMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        if((pdu_group = pdu_create_group(pdu_msg)) == NULL)
        {
        	AMP_DEBUG_ERR("rda_send_reports","Can't serialize report.",NULL);

        	/* This will also release the associated raw_report which was
        	 * shallow-copied into the message. */
        	pdu_release_msg(pdu_msg);

            unlockResource(&g_rda_cur_rpts_mutex);

        	AMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        /* Step 1.5: Send the message. */
        iif_send(&ion_ptr, pdu_group, report->recipient.name);

        /*
         * Step 1.6: This will also release the raw_report, which is
         *           shallow-copied by the call to pdu_create_msg.
         */
        pdu_release_group(pdu_group);

        /* Step 1.7: Update statistics. */
        gAgentInstr.num_sent_rpts++;
    }

    unlockResource(&g_rda_cur_rpts_mutex);

    AMP_DEBUG_EXIT("rda_send_reports","->0", NULL);
    return 0;    
}



/******************************************************************************
 *
 * \par Function Name: rda_eval_cleanup
 *
 * \par Purpose: Clean up lists and associated structures after an
 *               evaluation period.
 *
 * \retval int -  0 : Success
 *               -1 : Failure
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *  05/20/15  E. Birrane     Update to use global, mutex-protected lyst
 *****************************************************************************/

int rda_eval_cleanup()
{
    LystElt pending_elt;
    mid_t *mid = NULL;

    trl_t *trl = NULL;
    srl_t *srl = NULL;
    
    AMP_DEBUG_ENTRY("rda_eval_cleanup","()", NULL);
        
    lockResource(&g_rda_trls_pend_mutex);

    /* Step 1: For each pending rule...*/
    for (pending_elt = lyst_first(g_rda_trls_pend); pending_elt; pending_elt = lyst_next(pending_elt))
    {
        /*
         * Step 1.1: Grab the next rule. If it is NULL, we stop processing
         *           as the system is in a potentially unknown state.
         */

    	if((mid = (mid_t *) lyst_data(pending_elt)) == NULL)
    	{
    		AMP_DEBUG_ERR("rda_eval_cleanup","Can't grab MID for pending TRL",NULL);

            unlockResource(&g_rda_trls_pend_mutex);

            AMP_DEBUG_EXIT("rda_eval_cleanup","-> -1", NULL);
            return -1;
    	}

        if((trl = agent_vdb_trl_find(mid)) != NULL)
        {
        	/* Step 1.2: If the rule should no longer execute, delete it */
        	if(trl->desc.num_evals == 0)
        	{
        		AMP_DEBUG_INFO("rda_eval_cleanup","Removing expired rule.", NULL);

        		/* Step 1.2.1: Find and remove the rule in the memory list. */
        		agent_db_trl_forget(trl->mid);
        		agent_vdb_trl_forget(trl->mid);

        		/* Step 1.2.3: Release the rule. */
        		//trl_release(trl);
        		//trl = NULL;

        		/* Step 1.2.4: Correct counters. */
        		gAgentInstr.num_trls--;
        	}
        
        	/* Step 1.3: If the rule should run again, reset its timer. */
        	else
        	{
        		trl->countdown_ticks = trl->desc.interval_ticks;

        		/* Step 1.3.1: Re-persist the rule to update its status in the SDR. */
        		agent_db_trl_persist(trl);
        	}
        }
    }

    unlockResource(&g_rda_trls_pend_mutex);


    /* SRLs */

    lockResource(&g_rda_srls_pend_mutex);

    /* Step 1: For each pending rule...*/
    for (pending_elt = lyst_first(g_rda_srls_pend); pending_elt; pending_elt = lyst_next(pending_elt))
    {
    	if((mid = (mid_t *) lyst_data(pending_elt)) == NULL)
    	{
    		AMP_DEBUG_ERR("rda_eval_cleanup","Can't grab MID for pending TRL",NULL);

            unlockResource(&g_rda_srls_pend_mutex);

            AMP_DEBUG_EXIT("rda_eval_cleanup","-> -1", NULL);
            return -1;
    	}

        /*
         * Step 1.1: Grab the next rule. If it is NULL, we stop processing
         *           as the system is in a potentially unknown state.
         */
        if((srl = agent_vdb_srl_find(mid)) != NULL)
        {

        	/* Step 1.2: If the rule should no longer execute, delete it */
        	if(srl->desc.num_evals == 0)
        	{
        		AMP_DEBUG_INFO("rda_eval_cleanup","Removing expired rule.", NULL);

        		/* Step 1.2.1: Find and remove the rule in the memory list. */
        		agent_db_srl_forget(srl->mid);
        		agent_vdb_srl_forget(srl->mid);

        		/* Step 1.2.3: Release the rule. */
        		//srl_release(srl);
        		srl = NULL;

        		/* Step 1.2.4: Correct counters. */
        		gAgentInstr.num_srls--;
        	}

        	/* Step 1.3: If the rule should run again, reset its timer. */
        	else
        	{
        		srl->countdown_ticks = srl->desc.interval_ticks;

        		/* Step 1.3.1: Re-persist the rule to update its status in the SDR. */
        		agent_db_srl_persist(srl);
//        		agent_vdb_srl_forget(srl->mid);
//        		ADD_SRL(srl);
        	}
        }
    }

    unlockResource(&g_rda_srls_pend_mutex);



    AMP_DEBUG_EXIT("rda_eval_cleanup","->0", NULL);
    return 0;    
}




/******************************************************************************
 *
 * \par Function Name: rda_thread
 *
 * \par Purpose: "Main" function for the remote data aggregator.  This thread
 *               performs the following functions:
 *               1) Collect set of rules that are to be processed
 *               2) Process the rules (data collection, cmd execution)
 *               3) Update statistics and capture outgoing reports
 *               4) Perform rule housekeeping/cleanup.
 *
 * \retval void * - pthread_exit(NULL).
 *
 * \param[in,out]  threadId The thread id for the RDA thread.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  09/06/11  M. Reid        Initial Implementation
 *  10/21/11  E. Birrane     Code comments and functional updates.
 *****************************************************************************/

void* rda_thread(int* running)
{
    struct timeval start_time;
    vast delta = 0;
#ifndef mingw
    AMP_DEBUG_ENTRY("rda_thread","(0x%X)", (unsigned long) pthread_self()); //threadId);
#endif
	if((g_rda_cur_rpts = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("rda_thread","Can't allocate Rpts Lyst!", NULL);
		AMP_DEBUG_EXIT("rda_thread","->-1.",NULL);
		return NULL;
	}

	if((g_rda_trls_pend = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("rda_thread","Can't allocate Pending TRLs Lyst!", NULL);
		AMP_DEBUG_EXIT("rda_thread","->-1.",NULL);
		return NULL;
	}


	if((g_rda_srls_pend = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("rda_thread","Can't allocate Pending SRLs Lyst!", NULL);
		AMP_DEBUG_EXIT("rda_thread","->-1.",NULL);
		return NULL;
	}

	if(initResourceLock(&g_rda_cur_rpts_mutex))
	{
        AMP_DEBUG_ERR("rda_thread","Unable to initialize mutex, errno = %s",
        		        strerror(errno));
        AMP_DEBUG_EXIT("rda_thread","->-1.",NULL);
        return NULL;
	}

	if(initResourceLock(&g_rda_trls_pend_mutex))
	{
        AMP_DEBUG_ERR("rda_thread","Unable to initialize mutex, errno = %s",
        		        strerror(errno));
        AMP_DEBUG_EXIT("rda_thread","->-1.",NULL);
        return NULL;
	}


	if(initResourceLock(&g_rda_srls_pend_mutex))
	{
        AMP_DEBUG_ERR("rda_thread","Unable to initialize mutex, errno = %s",
        		        strerror(errno));
        AMP_DEBUG_EXIT("rda_thread","->-1.",NULL);
        return NULL;
	}

    AMP_DEBUG_INFO("rda_thread","Running Remote Data Aggregator Thread.", NULL);
   
    /* While the DTNMP Agent is running...*/
    while(*running)
    {
    	getCurrentTime(&start_time);



        AMP_DEBUG_INFO("rda_thread","Processing %u ctrls.",
        		        (unsigned long) lyst_length(gAgentVDB.trls));

        lockResource(&(gAgentVDB.ctrls_mutex));
        if(rda_scan_ctrls(gAgentVDB.ctrls) == -1)
        {
            AMP_DEBUG_ERR("rda_thread","Problem scanning ctrls.", NULL);
            //pthread_exit(NULL);
            continue;
        }

        /* For now, remove/forget inactive controls. */
        /* \todo: Update this later to keep them in storage for re-enable. */
        rda_clean_ctrls(gAgentVDB.ctrls);
        unlockResource(&(gAgentVDB.ctrls_mutex));



        AMP_DEBUG_INFO("rda_thread","Processing %u rules.",
        		        (unsigned long) lyst_length(gAgentVDB.trls));
                
        /* Lock the rule list while we are scanning and processinf rules */
        lockResource(&(gAgentVDB.trls_mutex));
        lockResource(&(gAgentVDB.srls_mutex));

        /* Step 1: Collect set of rules to be processed */
        if(rda_scan_rules() == -1)
        {
            AMP_DEBUG_ERR("rda_thread","Problem scanning rule list. Exiting.", NULL);
            //rda_cleanup();
            //pthread_exit(NULL);
            continue;
        }
        
        /* Step 2: Evaluate each rule. */
        if(rda_eval_pending_rules() == -1)
        {
            AMP_DEBUG_ERR("rda_thread","Problem evaluating rules. Exiting.", NULL);
            //rda_cleanup();
            //pthread_exit(NULL);
            continue;
        }
        
        /* Step 3: Send out any built reports. */
        if(rda_send_reports() == -1)
        {
            AMP_DEBUG_ERR("rda_thread","Problem sending built reports. Exiting.", NULL);
            //rda_cleanup();
            //pthread_exit(NULL);
            continue;
        }
        
        
        /* Step 4: Perform housekeeping for all evaluated rules. */
        if(rda_eval_cleanup() == -1)
        {
            AMP_DEBUG_ERR("rda_thread","Problem cleaning up after rules eval. Exiting.", NULL);
            //rda_cleanup();
            //pthread_exit(NULL);
            continue;
        }

        lockResource(&g_rda_cur_rpts_mutex);

        rpt_clear_lyst(&g_rda_cur_rpts, NULL, 0);

        unlockResource(&g_rda_cur_rpts_mutex);

        unlockResource(&(gAgentVDB.srls_mutex));
        unlockResource(&(gAgentVDB.trls_mutex));






        delta = utils_time_cur_delta(&start_time);

        // Sleep for 1 second (10^6 microsec) subtracting the processing time.
        if((delta < 1000000) && (delta > 0))
        {
        	microsnooze((unsigned int)(1000000 - delta));
        }
        
    } // end while
    
    rda_cleanup();

    AMP_DEBUG_ALWAYS("rda_thread","Shutting Down Agent Data Aggregator Thread.",NULL);

    AMP_DEBUG_EXIT("rda_thread","->NULL.", NULL);
    
    pthread_exit(NULL);
    return NULL; /* Defensive */
}
