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

#include "shared/primitives/rules.h"
#include "shared/primitives/report.h"

extern Lyst g_rda_cur_rpts; // Reports being built in the current tao.
extern Lyst g_rda_trls_pend;
extern Lyst g_rda_srls_pend;

extern ResourceLock g_rda_cur_rpts_mutex;
extern ResourceLock g_rda_trls_pend_mutex;
extern ResourceLock g_rda_srls_pend_mutex;

void         rda_cleanup();
rpt_t*       rda_get_report(eid_t recipient);
int          rda_scan_rules();
int          rda_scan_ctrls(Lyst exec_defs);
rpt_entry_t* rda_build_report_entry(mid_t *mid);

int          rda_eval_pending_rules();
int          rda_send_reports();
int          rda_eval_cleanup();

void*        rda_thread(void* threadId);

#endif /* RDA_H_ */
