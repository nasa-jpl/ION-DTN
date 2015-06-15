/*****************************************************************************
 **
 ** File Name: adm_bp_impl.h
 **
 ** Description: This implements the private aspects of a BP ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 *****************************************************************************/
#ifndef ADM_BP_IMPL_H_
#define ADM_BP_IMPL_H_


#include "shared/adm/adm_bp.h"


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/* Retrieval Functions */

/* Metadata */
value_t bp_md_name(Lyst params);
value_t bp_md_ver(Lyst params);


/* BP NODE */

value_t bp_node_get_node_id(Lyst params);
value_t bp_node_get_version(Lyst params);
value_t bp_node_get_storage(Lyst params);
value_t bp_node_get_last_restart(Lyst params);
value_t bp_node_get_num_reg(Lyst params);

value_t bp_node_get_fwd_pend(Lyst params);
value_t bp_node_get_dispatch_pend(Lyst params);
value_t bp_node_get_in_cust(Lyst params);
value_t bp_node_get_reassembly_pend(Lyst params);

value_t bp_node_get_blk_src_cnt(Lyst params);
value_t bp_node_get_norm_src_cnt(Lyst params);
value_t bp_node_get_exp_src_cnt(Lyst params);
value_t bp_node_get_blk_src_bytes(Lyst params);
value_t bp_node_get_norm_src_bytes(Lyst params);
value_t bp_node_get_exp_src_bytes(Lyst params);
value_t bp_node_get_blk_res_cnt(Lyst params);
value_t bp_node_get_norm_res_cnt(Lyst params);
value_t bp_node_get_exp_res_cnt(Lyst params);
value_t bp_node_get_blk_res_bytes(Lyst params);
value_t bp_node_get_norm_res_bytes(Lyst params);
value_t bp_node_get_exp_res_bytes(Lyst params);

value_t bp_node_get_bundles_frag(Lyst params);
value_t bp_node_get_frag_produced(Lyst params);

value_t bp_node_get_del_none(Lyst params);
value_t bp_node_get_del_expired(Lyst params);
value_t bp_node_get_del_fwd_uni(Lyst params);
value_t bp_node_get_del_cancel(Lyst params);
value_t bp_node_get_del_deplete(Lyst params);
value_t bp_node_get_del_bad_eid(Lyst params);
value_t bp_node_get_del_no_route(Lyst params);
value_t bp_node_get_del_no_contact(Lyst params);
value_t bp_node_get_del_bad_blk(Lyst params);
value_t bp_node_get_del_bytes(Lyst params);

value_t bp_node_get_fail_cust_cnt(Lyst params);
value_t bp_node_get_fail_cust_bytes(Lyst params);
value_t bp_node_get_fail_fwd_cnt(Lyst params);
value_t bp_node_get_fail_fwd_bytes(Lyst params);
value_t bp_node_get_fail_abandon_cnt(Lyst params);
value_t bp_node_get_fail_abandon_bytes(Lyst params);
value_t bp_node_get_fail_discard_cnt(Lyst params);
value_t bp_node_get_fail_discard_bytes(Lyst params);

/* BP ENDPOINT */
value_t bp_endpoint_get_names(Lyst params);
value_t bp_endpoint_get_name(Lyst params);
value_t bp_endpoint_get_active(Lyst params);
value_t bp_endpoint_get_singleton(Lyst params);
value_t bp_endpoint_get_abandon(Lyst params);


/******************************************************************************
 *                              Control Functions                             *
 ******************************************************************************/

uint32_t bp_ctrl_reset(Lyst params);

#endif //#ifndef ADM_BP_IMPL_H_


