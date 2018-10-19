/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
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
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  10/04/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef RDA_H_
#define RDA_H_

#include "../shared/primitives/rules.h"
#include "../shared/primitives/report.h"
#include "../shared/msg/msg.h"


#define RDA_DEF_NUM_RPTS 8
#define RDA_DEF_NUM_TBRS 8
#define RDA_DEF_NUM_SBRS 8


/*
 * TODO: Sort these vectors by time to execute.
 */
typedef struct
{
	vector_t rpt_msgs; /* of type (msg_rpt_t *)  */
	vector_t tbrs;    /* of type (rule_t *) */
	vector_t sbrs;    /* of type (rule_t *) */
} agent_db_t;

extern agent_db_t gAgentDb;

int rda_init();

void         rda_cleanup();
msg_rpt_t*   rda_get_msg_rpt(eid_t recipient);

int          rda_process_ctrls();

void rda_scan_tbrs_cb(rh_elt_t *elt, void *tag);
void rda_scan_sbrs_cb(rh_elt_t *elt, void *tag);

int          rda_process_rules();


int          rda_send_reports();

void*        rda_thread(int* running);

#endif /* RDA_H_ */
