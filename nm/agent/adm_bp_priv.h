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
 ** File Name: adm_bp_priv.h
 **
 ** Description: This implements the private aspects of a BP ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/16/13  E. Birrane     Initial Implementation
 *****************************************************************************/
#ifndef ADM_BP_PRIV_H_
#define ADM_BP_PRIV_H_


#include "shared/adm/adm_bp.h"
#include "shared/utils/expr.h"


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

void agent_adm_init_bp();

/* Retrieval Functions */

/* BP NODE */

expr_result_t bp_node_get_all(Lyst params);
expr_result_t bp_node_get_node_id(Lyst params);
expr_result_t bp_node_get_version(Lyst params);
expr_result_t bp_node_get_storage(Lyst params);
expr_result_t bp_node_get_last_restart(Lyst params);
expr_result_t bp_node_get_num_reg(Lyst params);

expr_result_t bp_node_get_fwd_pend(Lyst params);
expr_result_t bp_node_get_dispatch_pend(Lyst params);
expr_result_t bp_node_get_in_cust(Lyst params);
expr_result_t bp_node_get_reassembly_pend(Lyst params);

expr_result_t bp_node_get_blk_src_cnt(Lyst params);
expr_result_t bp_node_get_norm_src_cnt(Lyst params);
expr_result_t bp_node_get_exp_src_cnt(Lyst params);
expr_result_t bp_node_get_blk_src_bytes(Lyst params);
expr_result_t bp_node_get_norm_src_bytes(Lyst params);
expr_result_t bp_node_get_exp_src_bytes(Lyst params);
expr_result_t bp_node_get_blk_res_cnt(Lyst params);
expr_result_t bp_node_get_norm_res_cnt(Lyst params);
expr_result_t bp_node_get_exp_res_cnt(Lyst params);
expr_result_t bp_node_get_blk_res_bytes(Lyst params);
expr_result_t bp_node_get_norm_res_bytes(Lyst params);
expr_result_t bp_node_get_exp_res_bytes(Lyst params);

expr_result_t bp_node_get_bundles_frag(Lyst params);
expr_result_t bp_node_get_frag_produced(Lyst params);

expr_result_t bp_node_get_del_none(Lyst params);
expr_result_t bp_node_get_del_expired(Lyst params);
expr_result_t bp_node_get_del_fwd_uni(Lyst params);
expr_result_t bp_node_get_del_cancel(Lyst params);
expr_result_t bp_node_get_del_deplete(Lyst params);
expr_result_t bp_node_get_del_bad_eid(Lyst params);
expr_result_t bp_node_get_del_no_route(Lyst params);
expr_result_t bp_node_get_del_no_contact(Lyst params);
expr_result_t bp_node_get_del_bad_blk(Lyst params);
expr_result_t bp_node_get_del_bytes(Lyst params);

expr_result_t bp_node_get_fail_cust_cnt(Lyst params);
expr_result_t bp_node_get_fail_cust_bytes(Lyst params);
expr_result_t bp_node_get_fail_fwd_cnt(Lyst params);
expr_result_t bp_node_get_fail_fwd_bytes(Lyst params);
expr_result_t bp_node_get_fail_abandon_cnt(Lyst params);
expr_result_t bp_node_get_fail_abandon_bytes(Lyst params);
expr_result_t bp_node_get_fail_discard_cnt(Lyst params);
expr_result_t bp_node_get_fail_discard_bytes(Lyst params);

/* BP ENDPOINT */
expr_result_t bp_endpoint_get_names(Lyst params);
expr_result_t bp_endpoint_get_all(Lyst params);
expr_result_t bp_endpoint_get_name(Lyst params);
expr_result_t bp_endpoint_get_active(Lyst params);
expr_result_t bp_endpoint_get_singleton(Lyst params);
expr_result_t bp_endpoint_get_abandon(Lyst params);


/******************************************************************************
 *                              Control Functions                             *
 ******************************************************************************/

uint32_t bp_ctrl_reset(Lyst params);

#endif //#ifndef ADM_BP_PRIV_H_


