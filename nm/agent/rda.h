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
 ** File Name: rda.h
 **
 ** Description: This implements the Remote Data Aggregator (RDA)
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef RDA_H_
#define RDA_H_

void              rda_cleanup(Lyst rules_pending, Lyst built_reports);
rpt_data_t*       rda_find_report(Lyst built_reports, char *recipient);
int               rda_scan_rules(Lyst rules_pending);
int               rda_scan_ctrls(Lyst exec_defs);
rpt_data_entry_t* rda_build_report_entry(mid_t *mid);
int               rda_eval_rule(rule_time_prod_t *rule_p, rpt_data_t *report_p);
int               rda_eval_pending_rules(Lyst rules_pending, Lyst built_reports);
int               rda_send_reports(Lyst built_reports);
int               rda_eval_cleanup(Lyst rules_pending);

void*             rda_thread(void* threadId);




#endif /* RDA_H_ */
