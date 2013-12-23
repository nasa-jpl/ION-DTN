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
 ** File Name: adm_agent_priv.h
 **
 ** Description: This implements the private aspects of a DTNMP agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef ADM_AGENT_PRIV_H_
#define ADM_AGENT_PRIV_H_

#include "shared/adm/adm_agent.h"
#include "shared/adm/adm_bp.h"
#include "shared/adm/adm_ion.h"
#include "shared/adm/adm_ltp.h"
#include "shared/primitives/instr.h"
#include "shared/utils/expr.h"

void agent_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/


/* Retrieval Functions */

/* DTNMP AGENT */


/* Collect Functions */
expr_result_t agent_get_all(Lyst params);
expr_result_t agent_get_num_rpt_defs(Lyst params);
expr_result_t agent_get_num_sent_rpts(Lyst params);
expr_result_t agent_get_num_time_rules(Lyst params);
expr_result_t agent_get_num_time_rules_run(Lyst params);
expr_result_t agent_get_num_prod_rules(Lyst params);
expr_result_t agent_get_num_prod_rules_run(Lyst params);
expr_result_t agent_get_num_consts(Lyst params);
expr_result_t agent_get_num_data_defs(Lyst params);
expr_result_t agent_get_num_macros(Lyst params);
expr_result_t agent_get_num_macros_run(Lyst params);
expr_result_t agent_get_num_ctrls(Lyst params);
expr_result_t agent_get_num_ctrls_run(Lyst params);

/* Control Functions */
uint32_t agent_list_rpt_defs(Lyst params);
uint32_t agent_list_time_rules(Lyst params);
uint32_t agent_list_prod_rules(Lyst params);
uint32_t agent_list_consts(Lyst params);
uint32_t agent_list_data_defs(Lyst params);
uint32_t agent_list_macros(Lyst params);
uint32_t agent_list_ctrls(Lyst params);
uint32_t agent_remove_item(Lyst params);

#endif // ADM_AGENT_PRIV_H_



