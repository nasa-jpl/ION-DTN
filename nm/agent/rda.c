/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 ** \file rda.cpp
 ** 
 ** File Name: rda.cpp
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
 **  09/06/11  M. Reid        Initial Implementation
 **  10/21/11  E. Birrane     Code comments and functional updates.
 **  06/27/13  E. Birrane     Support persisted rules.
 *****************************************************************************/

#include "platform.h"
#include "lyst.h"

#include "shared/utils/utils.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/instr.h"

#include "shared/msg/msg_reports.h"
#include "shared/utils/db.h"

#include "nmagent.h"
#include "ldc.h"
#include "lcc.h"



/******************************************************************************
 *
 * \par Function Name: rda_cleanup
 *
 * \par Purpose: Cleans up any resources left over by the RDA when it exits.
 *
 * \retval void
 *
 * \param[in,out]  rules_pending - List of rules RDA has been evaluating.
 * \param[in,out]  built_reports - List of reports built during an RDA run.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/

void rda_cleanup(Lyst rules_pending, Lyst built_reports)
{
    /* rules_pending only holds pointers. Nothing to free. */
    lyst_destroy(rules_pending);
    
    rpt_clear_lyst(&built_reports, NULL, 0);
    lyst_destroy(built_reports);    
}



/******************************************************************************
 *
 * \par Function Name: rda_find_report
 *
 * \par Purpose: Find the data report intended for a given recipient. The
 *               agent will, when possible, combine reports for a single
 *               recipient.
 *
 * \param[in]  built_reports - List of reports built during an RDA run.
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
 *****************************************************************************/

rpt_data_t *rda_find_report(Lyst built_reports, char *recipient)
{
    LystElt elt;
    rpt_data_t *cur_report = NULL;
    rpt_data_t *result = NULL;
    
    DTNMP_DEBUG_ENTRY("rda_find_report","(0x%#llx, %s)",
    		         (unsigned long) built_reports, recipient);
    
    /* Step 0: Sanity check. */
    if(recipient == NULL)
    {
    	DTNMP_DEBUG_ERR("rda_find_report","Bad parms.",NULL);
    	return NULL;
    }

    /* Step 1: Search the list of reports identified so far. */
    for (elt = lyst_first(built_reports); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the current report */
        cur_report = (rpt_data_t*) lyst_data(elt);
        
        /* Step 1.2: Check if this report is destined for our recipient. */
        if(strcmp(cur_report->recipient.name, recipient) == 0)
        {
            DTNMP_DEBUG_INFO("rda_find_report",
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
    	strcpy(rx.name,recipient);
    	result = rpt_create_data((uint32_t)getUTCTime(), lyst_create(), rx);

    	DTNMP_DEBUG_INFO("rda_find_report","New report for recipient %s", recipient);
        
        lyst_insert_first(built_reports, result);
    }        
    
    DTNMP_DEBUG_EXIT("rda_find_report","->0x%x", (unsigned long) result);
    
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
 * \param[in,out]  rules_pending - List of rules that should be executed during
 *                                 this execution period.
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/

int rda_scan_rules(Lyst rules_pending)
{
    LystElt elt;
    rule_time_prod_t *rule_p = NULL;
    
    DTNMP_DEBUG_ENTRY("rda_scan_rules","(0x%#llx)", (unsigned long)rules_pending);

    
    /* Step 0: Start with a fresh list for pending rules */
    lyst_clear(rules_pending);
    
    
    /* 
     * Step 1: Walk through each defined rule and see if it should be
     *         included in the current evaluation scan.
     */    
    for (elt = lyst_first(gAgentVDB.rules); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next rule...*/
        if((rule_p = (rule_time_prod_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_scan_rules","Found NULL rule. Exiting", NULL);
            DTNMP_DEBUG_EXIT("rda_scan_rules","->-1.", NULL);
            return -1;
        }
                
        /* Step 1.2: Determine if this rule is ready for possible evaluation. */
        if(rule_p->countdown_ticks == 0)
        {
            /*
             * Step 1.2.1: Determine if this rule has been evaluated more than
             *             its maximum number of evaluations
             */
            if((rule_p->desc.num_evals > 0) ||
               (rule_p->desc.num_evals == DTNMP_RULE_EXEC_ALWAYS))
            {
            	/* Step 1.2.2: If ready, add rule to list of rules pending
            	 *             evaluation in this current tick.
            	 */
                lyst_insert_first(rules_pending, rule_p);
                DTNMP_DEBUG_INFO("rda_scan_rules","Added rule to evaluate list.",
                		         NULL);
            }
        }
        else
        {
        	/* Step 1.2.2: If not ready, note that another tick has elapsed. */
            rule_p->countdown_ticks--;
        }                
    }
    
    DTNMP_DEBUG_EXIT("rda_scan_rules","->0.", NULL);
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

    DTNMP_DEBUG_ENTRY("rda_scan_ctrls","(0x%#llx)", (unsigned long)exec_defs);

    /*
     * Step 1: Walk through each defined ctrl and see if it should be included
     *         in the current evaluation scan.
     */
    for (elt = lyst_first(exec_defs); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next ctrl...*/
        if((ctrl_p = (ctrl_exec_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_scan_ctrls","Found NULL ctrl. Exiting", NULL);
            DTNMP_DEBUG_EXIT("rda_scan_ctrls","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Determine if this rule is ready for possible evaluation. */
        if(ctrl_p->desc.state == CONTROL_ACTIVE)
        {
        	/* Step 1.3: If the control is ready to execute, run it. */
        	if(ctrl_p->countdown_ticks <= 0)
        	{
        		lcc_run_ctrl(ctrl_p);

        		/* Step 1.3.1: controls disable after they fire.*/
        		ctrl_p->desc.state = CONTROL_INACTIVE;
        	}
        	/* Step 1.4: If the control is not ready, note a tick elapsed. */
        	else
        	{
        		ctrl_p->countdown_ticks--;
        	}
        }
    }

    DTNMP_DEBUG_EXIT("rda_scan_ctrls","->0.", NULL);
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

    DTNMP_DEBUG_ENTRY("rda_clean_ctrls","(0x%#llx)", (unsigned long)exec_defs);

    /*
     * Step 1: Walk through each defined ctrl and see if it should be included
     *         in the current evaluation scan.
     */
    for (elt = lyst_first(exec_defs); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next ctrl...*/
        if((ctrl_p = (ctrl_exec_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_clean_ctrls","Found NULL ctrl. Exiting", NULL);
            DTNMP_DEBUG_EXIT("rda_clean_ctrls","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Determine if this rule should be removed. */
        if(ctrl_p->desc.state != CONTROL_ACTIVE)
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
        	ctrl_release_exec(ctrl_p);
        }
    }

    DTNMP_DEBUG_EXIT("rda_clean_ctrls","->0.", NULL);
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

rpt_data_entry_t *rda_build_report_entry(mid_t *mid)
{
	rpt_data_entry_t *entry = NULL;
    int result = 0;
    
    DTNMP_DEBUG_ENTRY("rda_build_report_entry","(0x%x)", (unsigned long) mid);
    
    /* Step 1: Make sure we have an entry structure to populate. */
    if((entry = (rpt_data_entry_t *) MTAKE(sizeof(rpt_data_entry_t))) == NULL)
    {
        DTNMP_DEBUG_ERR("rda_build_report_entry","Can't allocate %d bytes.",
        		        sizeof(rpt_data_entry_t));
        DTNMP_DEBUG_EXIT("rda_build_report_entry","->NULL.", NULL);
        return NULL;        
    }

    
    /* Step 2: Fill the report with data. */
    if(ldc_fill_report_data(mid,entry) == -1)
    {
        DTNMP_DEBUG_ERR("rda_build_report_entry","Can't fill report.",NULL);
        rpt_release_data_entry(entry);

        DTNMP_DEBUG_EXIT("rda_build_report_entry","->NULL.", NULL);
        return NULL;
    }

    DTNMP_DEBUG_EXIT("rda_build_report_entry","->0x%x", (unsigned long) entry);
    return entry;
}

/*
 * Returns # rules processed.
 */

/******************************************************************************
 *
 * \par Function Name: rda_eval_rule
 *
 * \par Purpose: Generate a data report by evaluating a time-based production
 *               rule.
 *
 * \return -1   - Error
 *         >= 0 - # rules processed.
 *
 * \param[in]  rule_p   - The MID identifying the report to populate.
 * \param[out] report_p - The constructed report.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/

int rda_eval_rule(rule_time_prod_t *rule_p, rpt_data_t *report_p)
{
	rpt_data_entry_t *entry = NULL;
    LystElt elt;
    mid_t *cur_mid = NULL;
    int result = 0;
    Sdnv tmp;
    
    DTNMP_DEBUG_ENTRY("rda_eval_rule","(0x%#llx 0x%#llx)",
    		           (unsigned long) rule_p, (unsigned long) report_p);
    
    /* Step 0: Sanity check.*/
    if((rule_p == NULL) || (report_p == NULL))
    {
    	DTNMP_DEBUG_ERR("rda_eval_rule","Bad parms.", NULL);
    	return -1;
    }

    /* Step 1: Update statistics. */
	gAgentInstr.num_time_rules_run++;

    /* Step 2: For each MID listed in the evaluating report...*/
    for (elt = lyst_first(rule_p->mids); elt; elt = lyst_next(elt))
    {

        if((cur_mid = (mid_t*) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_rule","Bad Rule MID.", NULL);
            DTNMP_DEBUG_EXIT("rda_eval_rule","->-1", NULL);
            return -1;
        }
    
        /* Step 2.1: Construct The data entry for this MID. */
        if((entry = rda_build_report_entry(cur_mid)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_rule","Can't build report entry.", NULL);
            DTNMP_DEBUG_EXIT("rda_eval_rule","->-1", NULL);
            return -1;            
        }
        
        /* Step 2.2: Add new entry to the report. */
        lyst_insert_last(report_p->reports,entry);
        
        result++;
    }
    
    DTNMP_DEBUG_EXIT("rda_eval_rule","-> %d", result);
    return result;
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
 * \param[in,out]  rules_pending - List of rules that should be executed during
 *                                 this execution period.
 *
 * \param[in,out]  built_reports - List of reports being generted by the rda.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/

int rda_eval_pending_rules(Lyst rules_pending, Lyst built_reports)
{
    
    LystElt pending_elt;
    rule_time_prod_t *rule_p = NULL;
    
    
    DTNMP_DEBUG_ENTRY("rda_eval_pending_rules","(0x%x,0x%x)",
    		          (unsigned long) rules_pending, (unsigned long) built_reports);
    
    DTNMP_DEBUG_INFO("rda_eval_pending_rules","Preparing to eval%d rules.",
    		         lyst_length(rules_pending));
    
    for (pending_elt = lyst_first(rules_pending); pending_elt; pending_elt = lyst_next(pending_elt))
    {
        /* Grab the next rule...*/
        if((rule_p = (rule_time_prod_t*) lyst_data(pending_elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_pending_rules",
            		        "Cannot find pending rule from elt 0x%x.", pending_elt);
            DTNMP_DEBUG_EXIT("rda_eval_pending_rules","-> -1.", NULL);
            return -1;
        }
        
        /* Evaluate the rule */
        rpt_data_t *rpt = rda_find_report(built_reports, rule_p->desc.sender.name);

        rda_eval_rule(rule_p, rpt);
        
        /* Note that the rule has been evaluated */        
        if(rule_p->desc.num_evals > 0)
        {
        	DTNMP_DEBUG_INFO("rda_eval_pending_rules",
        			         "Decrementing rule eval count from %d.",
        			         rule_p->desc.num_evals);
            rule_p->desc.num_evals--;
        }
    }
    
    DTNMP_DEBUG_EXIT("rda_eval_pending_rules","-> 0", NULL);
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
 * \param[in,out]  built_reports - List of reports generated by the rda.
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
 *****************************************************************************/

int rda_send_reports(Lyst built_reports)
{
    LystElt report_elt;
    rpt_data_t *report = NULL;
    uint8_t *raw_report = NULL;
    uint32_t raw_report_len = 0;
    
    DTNMP_DEBUG_ENTRY("rda_send_reports","(0x%#llx)",
    		         (unsigned long) built_reports);
    
    DTNMP_DEBUG_INFO("rda_send_reports","Preparing to send %d reports.",
    		         lyst_length(built_reports));
    

    /* Step 1: For each report that has been built... */
    for (report_elt = lyst_first(built_reports); report_elt; report_elt = lyst_next(report_elt))
    {
        /*
         * Step 1.1: Grab the report. Bail if the report is bad. It is better to
         * send no reports than to send potentially garbled reports. If the
         * report here is NULL, something has clearly gone wrong on the system.
         */
        if((report = (rpt_data_t*) lyst_data(report_elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_send_reports","Can't find report from elt %x.",
            		        report_elt);
            DTNMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        /* Step 1.2: Construct a message for the report. */
       	pdu_msg_t *pdu_msg = NULL;
       	pdu_group_t *pdu_group = NULL;

        /* Step 1.3: Serialize the payload. */
        if((raw_report = rpt_serialize_data(report, &raw_report_len)) == NULL)
        {
        	DTNMP_DEBUG_ERR("rda_send_reports","Can't serialize report.",NULL);
            DTNMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        /* Step 1.4: Construct the containers. */
        if((pdu_msg = pdu_create_msg(MSG_TYPE_RPT_DATA_RPT,
        		                 raw_report, raw_report_len, NULL)) == NULL)
        {
        	DTNMP_DEBUG_ERR("rda_send_reports","Can't serialize report.",NULL);
        	MRELEASE(raw_report);
            DTNMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }

        if((pdu_group = pdu_create_group(pdu_msg)) == NULL)
        {
        	DTNMP_DEBUG_ERR("rda_send_reports","Can't serialize report.",NULL);

        	/* This will also release the associated raw_report which was
        	 * shallow-copied into the message. */
        	pdu_release_msg(pdu_msg);
            DTNMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
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
    
    DTNMP_DEBUG_EXIT("rda_send_reports","->0", NULL);
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
 * \param[in,out]  rules_pending - The list of rules that were pending for
 *                                 evaluation during this period.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/21/11  E. Birrane     Initial implementation,
 *****************************************************************************/

int rda_eval_cleanup(Lyst rules_pending)
{
    LystElt pending_elt;
    rule_time_prod_t *rule_p = NULL;
    
    DTNMP_DEBUG_ENTRY("rda_eval_cleanup","(0x%#llx)", rules_pending);
        
    /* Step 1: For each pending rule...*/
    for (pending_elt = lyst_first(rules_pending); pending_elt; pending_elt = lyst_next(pending_elt))
    {
        /*
         * Step 1.1: Grab the next rule. If it is NULL, we stop processing
         *           as the system is in a potentially unknown state.
         */
        if((rule_p = (rule_time_prod_t*) lyst_data(pending_elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_cleanup","Can't find pending rule from elt %x.", pending_elt);
            DTNMP_DEBUG_EXIT("rda_eval_cleanup","-> -1", NULL);
            return -1;
        }
        
        /* Step 1.2: If the rule should no longer execute, delete it */
        if(rule_p->desc.num_evals == 0)
        {
            DTNMP_DEBUG_INFO("rda_eval_cleanup","Removing expired rule.", NULL);

        	/* Step 1.2.1: Find and remove the rule in the memory list. */
            LystElt tmp_elt;
            LystElt del_elt;
            for(tmp_elt = lyst_first(gAgentVDB.rules); tmp_elt; tmp_elt = lyst_next(tmp_elt))
            {
            	rule_time_prod_t *tmp_rule = (rule_time_prod_t*) lyst_data(tmp_elt);
            	if(tmp_rule == rule_p)
            	{
            		del_elt = tmp_elt;
            		tmp_elt = lyst_prev(tmp_elt);
            		lyst_delete(del_elt);
            	}
            }

            /* Step 1.2.2: Remove the rule from the SDR storage.. */
            db_forget(&(rule_p->desc.itemObj),
         	          &(rule_p->desc.descObj),
                      gAgentDB.rules);

            /* Step 1.2.3: Release the rule. */
            rule_release_time_prod_entry(rule_p);
            rule_p = NULL;

            /* Step 1.2.4: Correct counters. */
        	gAgentInstr.num_time_rules--;
        }
        
        /* Step 1.3: If the rule should run again, reset its timer. */
        else
        {
            rule_p->countdown_ticks = rule_p->desc.interval_ticks;

            /* Step 1.3.1: Re-persist the rule to update its status in the SDR. */
            agent_db_rule_persist(rule_p);
        }
    }

    DTNMP_DEBUG_EXIT("rda_eval_cleanup","->0", NULL);
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

void* rda_thread(void* threadId)
{
    struct timeval start_time;
    vast delta = 0;

    Lyst rules_pending;
    Lyst built_reports;
    
    DTNMP_DEBUG_ENTRY("rda_thread","(0x%x)", threadId);
    
    rules_pending = lyst_create();
    built_reports = lyst_create();
    
    DTNMP_DEBUG_INFO("rda_thread","Running Remote Data Aggregator Thread.", NULL);
   
    /* While the DTNMP Agent is running...*/
    while(g_running)
    {
    	getCurrentTime(&start_time);

        DTNMP_DEBUG_INFO("rda_thread","Processing %u ctrls.",
        		        (unsigned long) lyst_length(gAgentVDB.rules));

        lockResource(&(gAgentVDB.ctrls_mutex));
        if(rda_scan_ctrls(gAgentVDB.ctrls) == -1)
        {
            DTNMP_DEBUG_ERR("rda_thread","Problem scanning ctrls.", NULL);
            pthread_exit(NULL);
        }

        /* For now, remove/forget inactive controls. */
        /* \todo: Update this later to keep them in storage for re-enable. */
        rda_clean_ctrls(gAgentVDB.ctrls);
        unlockResource(&(gAgentVDB.ctrls_mutex));


        DTNMP_DEBUG_INFO("rda_thread","Processing %u rules.",
        		        (unsigned long) lyst_length(gAgentVDB.rules));
                
        /* Lock the rule list while we are scanning and processinf rules */
        lockResource(&(gAgentVDB.rules_mutex));

        /* Step 1: Collect set of rules to be processed */
        if(rda_scan_rules(rules_pending) == -1)
        {
            DTNMP_DEBUG_ERR("rda_thread","Problem scanning rule list. Exiting.", NULL);
            rda_cleanup(rules_pending, built_reports);
            pthread_exit(NULL);
        }
        
        /* Step 2: Evaluate each rule. */
        if(rda_eval_pending_rules(rules_pending, built_reports) == -1)
        {
            DTNMP_DEBUG_ERR("rda_thread","Problem evaluating rules. Exiting.", NULL);
            rda_cleanup(rules_pending, built_reports);
            pthread_exit(NULL);            
        }
        
        /* Step 3: Send out any built reports. */
        if(rda_send_reports(built_reports) == -1)
        {
            DTNMP_DEBUG_ERR("rda_thread","Problem sending built reports. Exiting.", NULL);
            rda_cleanup(rules_pending, built_reports);
            pthread_exit(NULL);
        }
        
        
        /* Step 4: Perform housekeeping for all evaluated rules. */
        if(rda_eval_cleanup(rules_pending) == -1)
        {
            DTNMP_DEBUG_ERR("rda_thread","Problem cleaning up after rules eval. Exiting.", NULL);
            rda_cleanup(rules_pending, built_reports);
            pthread_exit(NULL);            
        }

        rpt_clear_lyst(&built_reports, NULL, 0);

        unlockResource(&(gAgentVDB.rules_mutex));
                
        delta = utils_time_cur_delta(&start_time);

        // Sleep for 1 second (10^6 microsec) subtracting the processing time.
        if((delta < 1000000) && (delta > 0))
        {
        	microsnooze((unsigned int)(1000000 - delta));
        }
        
    } // end while
    
    rda_cleanup(rules_pending, built_reports);

    DTNMP_DEBUG_ALWAYS("rda_thread","Shutting Down Agent Data Aggregator Thread.",NULL);

    DTNMP_DEBUG_EXIT("rda_thread","->NULL.", NULL);
    
    pthread_exit(NULL);
}
