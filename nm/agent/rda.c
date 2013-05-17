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
 *****************************************************************************/

#include "platform.h"
#include "lyst.h"

#include "shared/utils/utils.h"
#include "shared/primitives/mid.h"
#include "shared/msg/msg_reports.h"

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
    
    rpt_clear_lyst(built_reports);
    lyst_destroy(built_reports);    
}


rpt_data_t *rda_find_report(Lyst built_reports, char *recipient)
{
    LystElt elt;
    rpt_data_t *cur_report = NULL;
    rpt_data_t *result = NULL;
    
    DTNMP_DEBUG_ENTRY("rda_find_report","(0x%x, %s)",
    		         (unsigned long) built_reports, recipient);
    
    /* Search the list of reports identified so far. */
    for (elt = lyst_first(built_reports); elt; elt = lyst_next(elt))
    {
        /* Grab the current report */
        cur_report = (rpt_data_t*) lyst_data(elt);
        
        /* Currently, just match on single recipient. */

        if(strcmp(cur_report->recipient.name, recipient) == 0)
        {
            DTNMP_DEBUG_INFO("rda_find_report",
            		         "Found existing report for recipient %s", recipient);
            result = cur_report;
            break;
        }
    }
    
    /* 
     * If there is no report, we will need to create one for this
     * recipient.
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
 * \par Function Name: rda_scanRules
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
 * \param[in,out]  pending_bufsize - Estimated # bytes taken by all rule MIDs. 
 *
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
    
    DTNMP_DEBUG_ENTRY("rda_scan_rules","(0x%x)", (unsigned long)rules_pending);

    
    /* Start with a fresh list for pending rules */
    lyst_clear(rules_pending);
    
    
    /* 
     * Walk through each defined rule and see if it should be included in
     * the current evaluation scan.
     */    
    for (elt = lyst_first(rules_active); elt; elt = lyst_next(elt))
    {
        /* Grab the next rule...*/
        if((rule_p = (rule_time_prod_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_scan_rules","Found NULL rule. Exiting", NULL);
            DTNMP_DEBUG_EXIT("rda_scan_rules","->-1.", NULL);
            return -1;
        }
                
        /* Determine if this rule is ready for possible evaluation. */
        if(rule_p->countdown_ticks == 0)
        {
            /* Determine if this rule has been evaluated more than its
             * maximum number of evaluations */
            if((rule_p->num_evals > 0) ||
               (rule_p->num_evals == DTNMP_RULE_EXEC_ALWAYS))
            {
                lyst_insert_first(rules_pending, rule_p);
                DTNMP_DEBUG_INFO("rda_scan_rules","Added rule to evaluate list.",
                		         NULL);
            }
        }
        else
        {
            rule_p->countdown_ticks--;
        }                
    }
    
    DTNMP_DEBUG_EXIT("rda_scan_rules","->0.", NULL);
    return 0;
}
 


int rda_scan_ctrls(Lyst exec_defs)
{
    LystElt elt;
    ctrl_exec_t *ctrl_p = NULL;

    DTNMP_DEBUG_ENTRY("rda_scan_ctrls","(0x%x)", (unsigned long)exec_defs);

    /*
     * Walk through each defined ctrl and see if it should be included in
     * the current evaluation scan.
     */
    for (elt = lyst_first(exec_defs); elt; elt = lyst_next(elt))
    {
        /* Grab the next ctrl...*/
        if((ctrl_p = (ctrl_exec_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_scan_ctrls","Found NULL ctrl. Exiting", NULL);
            DTNMP_DEBUG_EXIT("rda_scan_ctrls","->-1.", NULL);
            return -1;
        }

        /* Determine if this rule is ready for possible evaluation. */
        if(ctrl_p->state == CONTROL_ACTIVE)
        {
        	if(ctrl_p->countdown_ticks <= 0)
        	{
        		lcc_run_ctrl(ctrl_p);
        		/* controls disable after they fire.*/
        		ctrl_p->state = CONTROL_INACTIVE;
        	}
        	else
        	{
        		ctrl_p->countdown_ticks--;
        	}
        }
    }

    DTNMP_DEBUG_EXIT("rda_scan_ctrls","->0.", NULL);
    return 0;
}


/* \todo: Does this really need to be a function? */
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
int rda_eval_rule(rule_time_prod_t *rule_p, rpt_data_t *report_p)
{
	rpt_data_entry_t *entry = NULL;
    LystElt elt;
    mid_t *cur_mid = NULL;
    int result = 0;
    Sdnv tmp;
    
    DTNMP_DEBUG_ENTRY("rda_eval_rule","(0x%x 0x%x)",
    		           (unsigned long) rule_p, (unsigned long) report_p);
    
    
    /* For each MID listed in the evaluating report...*/
    for (elt = lyst_first(rule_p->mids); elt; elt = lyst_next(elt))
    {

        if((cur_mid = (mid_t*) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_rule","Bad Rule MID.", NULL);
            DTNMP_DEBUG_EXIT("rda_eval_rule","->-1", NULL);
            return -1;
        }
    
        /* Step 2: Construct The data entry for this MID. */
        if((entry = rda_build_report_entry(cur_mid)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_rule","Can't build report entry.", NULL);
            DTNMP_DEBUG_EXIT("rda_eval_rule","->-1", NULL);
            return -1;            
        }
        
        /* Step 3: Add new entry to the report. */
        lyst_insert_last(report_p->reports,entry);
        
        result++;
    }
    
    DTNMP_DEBUG_EXIT("rda_eval_rule","-> %d", result);
    return result;
}    
    


/******************************************************************************
 *
 * \par Function Name: rda_evalPendingRules
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
        rpt_data_t *rpt = rda_find_report(built_reports, rule_p->sender.name);

        rda_eval_rule(rule_p, rpt);
        
        /* Note that the rule has been evaluated */        
        if(rule_p->num_evals > 0)
        {
        	DTNMP_DEBUG_INFO("rda_eval_pending_rules",
        			         "Decrementing rule eval count from %d.",
        			         rule_p->num_evals);
            rule_p->num_evals--;
        }
    }
    
    
    DTNMP_DEBUG_EXIT("rda_eval_pending_rules","-> 0", NULL);
    return 0;
}


int rda_send_reports(Lyst built_reports)
{
    LystElt report_elt;
    rpt_data_t *report = NULL;
    uint8_t *raw_report = NULL;
    uint32_t raw_report_len = 0;
    
    DTNMP_DEBUG_ENTRY("rda_send_reports","(0x%x)",
    		          (unsigned long) built_reports);
    
    DTNMP_DEBUG_INFO("rda_send_reports","Preparing to send %d reports.",
    		         lyst_length(built_reports));
    
    for (report_elt = lyst_first(built_reports); report_elt; report_elt = lyst_next(report_elt))
    {
        /* Grab the report */
        if((report = (rpt_data_t*) lyst_data(report_elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_send_reports","Can't find report from elt %x.",
            		        report_elt);
            DTNMP_DEBUG_EXIT("rda_send_reports","->-1.", NULL);
            return -1;
        }




        /* Send the report to the report recipient.*/
       	pdu_msg_t *pdu_msg = NULL;
       	pdu_bundle_t *pdu_bundle = NULL;

        /* Serialize the payload. */
        raw_report = rpt_serialize_data(report, &raw_report_len);

        pdu_msg = pdu_create_msg(MSG_TYPE_RPT_DATA_RPT, raw_report, raw_report_len, NULL);
        pdu_bundle = pdu_create_bundle(pdu_msg);

        unsigned char *msg;
        uint32_t msg_len;
        iif_send(&ion_ptr, pdu_bundle, report->recipient.name);
        pdu_release_bundle(pdu_bundle);
    }
    
    DTNMP_DEBUG_EXIT("rda_send_reports","->0", NULL);
    return 0;    
}


int rda_eval_cleanup(Lyst rules_pending)
{
    LystElt pending_elt;
    rule_time_prod_t *rule_p = NULL;
    
    DTNMP_DEBUG_ENTRY("rda_eval_cleanup","(0x%x)", rules_pending);
        
    for (pending_elt = lyst_first(rules_pending); pending_elt; pending_elt = lyst_next(pending_elt))
    {
        /* Grab the next rule...*/
        if((rule_p = (rule_time_prod_t*) lyst_data(pending_elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rda_eval_cleanup","Can't find pending rule from elt %x.", pending_elt);
            DTNMP_DEBUG_EXIT("rda_eval_cleanup","-> -1", NULL);
            return -1;
        }
        
        /* Perform post-evaluation cleanup on the rule */
        
        /* If the rule should no longer execute, expire it */
        if(rule_p->num_evals == 0)
        {
            // Remove this rule from the active list and place it in the
            // expired list.
            DTNMP_DEBUG_INFO("rda_eval_cleanup","Removing expired rule.", NULL);
            
            // \todo maybe put active ELT in the pending list?
            LystElt tmp_elt;
            for(tmp_elt = lyst_first(rules_active); tmp_elt; tmp_elt = lyst_next(tmp_elt))
            {
            	rule_time_prod_t *tmp_rule = (rule_time_prod_t*) lyst_data(tmp_elt);
            	if(tmp_rule == rule_p)
            	{
            		lyst_delete(tmp_elt);
            	}
            }

            lyst_insert_first(rules_expired, rule_p);            
        }
        
        /* Otherwise, reset its countdown timer. */
        else
        {
            rule_p->countdown_ticks = rule_p->interval_ticks;   
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
 *               performs the fllowing functions:
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
    time_t  start_time = 0;

    Lyst rules_pending;
    Lyst built_reports;
    
    
    
    DTNMP_DEBUG_ENTRY("rda_thread","(0x%x)", threadId);
    
    rules_pending = lyst_create();
    built_reports = lyst_create();
    
    
    DTNMP_DEBUG_INFO("rda_thread","Running Remote Data Aggregator Thread.", NULL);
   
    /* While the DTNMP Agent is running...*/
    while(g_running)
    {
        start_time = getUTCTime();

        DTNMP_DEBUG_INFO("rda_thread","Processing %u ctrls.",
        		        (unsigned long) lyst_length(rules_active));

        lockResource(&exec_defs_mutex);
        if(rda_scan_ctrls(exec_defs) == -1)
        {
            DTNMP_DEBUG_ERR("rda_thread","Problem scanning ctrls.", NULL);
            pthread_exit(NULL);
        }
        unlockResource(&exec_defs_mutex);


        DTNMP_DEBUG_INFO("rda_thread","Processing %u rules.",
        		        (unsigned long) lyst_length(rules_active));
                
        /* Lock the rule list while we are scanning and processinf rules */
        lockResource(&rules_active_mutex);

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
        rpt_clear_lyst(built_reports);

        unlockResource(&rules_active_mutex);
                
        // Sleep for 1 second (10^6 microsec) subtracting the processing time.
        microsnooze((unsigned int)(1000000 - (getUTCTime() - start_time)));
        
    } // end while
    
    rda_cleanup(rules_pending, built_reports);

    DTNMP_DEBUG_EXIT("rda_thread","->NULL.", NULL);
    
    pthread_exit(NULL);
}
