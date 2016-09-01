/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm_adm_bp_impl.h
 **
 ** Description: This implements the private aspects of a BP ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (JHU/APL)
 *****************************************************************************/

#ifndef ADM_BP_IMPL_H_
#define ADM_BP_IMPL_H_


#include "../shared/adm/adm_bp.h"


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/* Retrieval Functions */

/* Metadata */
value_t adm_bp_md_name(tdc_t params);
value_t adm_bp_md_ver(tdc_t params);


/* BP NODE */

value_t adm_bp_node_get_node_id(tdc_t params);
value_t adm_bp_node_get_version(tdc_t params);
value_t adm_bp_node_get_storage(tdc_t params);
value_t adm_bp_node_get_last_restart(tdc_t params);
value_t adm_bp_node_get_num_reg(tdc_t params);

value_t adm_bp_node_get_fwd_pend(tdc_t params);
value_t adm_bp_node_get_dispatch_pend(tdc_t params);
value_t adm_bp_node_get_in_cust(tdc_t params);
value_t adm_bp_node_get_reassembly_pend(tdc_t params);

value_t adm_bp_node_get_blk_src_cnt(tdc_t params);
value_t adm_bp_node_get_norm_src_cnt(tdc_t params);
value_t adm_bp_node_get_exp_src_cnt(tdc_t params);
value_t adm_bp_node_get_blk_src_bytes(tdc_t params);
value_t adm_bp_node_get_norm_src_bytes(tdc_t params);
value_t adm_bp_node_get_exp_src_bytes(tdc_t params);
value_t adm_bp_node_get_blk_res_cnt(tdc_t params);
value_t adm_bp_node_get_norm_res_cnt(tdc_t params);
value_t adm_bp_node_get_exp_res_cnt(tdc_t params);
value_t adm_bp_node_get_blk_res_bytes(tdc_t params);
value_t adm_bp_node_get_norm_res_bytes(tdc_t params);
value_t adm_bp_node_get_exp_res_bytes(tdc_t params);

value_t adm_bp_node_get_bundles_frag(tdc_t params);
value_t adm_bp_node_get_frag_produced(tdc_t params);

value_t adm_bp_node_get_del_none(tdc_t params);
value_t adm_bp_node_get_del_expired(tdc_t params);
value_t adm_bp_node_get_del_fwd_uni(tdc_t params);
value_t adm_bp_node_get_del_cancel(tdc_t params);
value_t adm_bp_node_get_del_deplete(tdc_t params);
value_t adm_bp_node_get_del_bad_eid(tdc_t params);
value_t adm_bp_node_get_del_no_route(tdc_t params);
value_t adm_bp_node_get_del_no_contact(tdc_t params);
value_t adm_bp_node_get_del_bad_blk(tdc_t params);
value_t adm_bp_node_get_del_bytes(tdc_t params);

value_t adm_bp_node_get_fail_cust_cnt(tdc_t params);
value_t adm_bp_node_get_fail_cust_bytes(tdc_t params);
value_t adm_bp_node_get_fail_fwd_cnt(tdc_t params);
value_t adm_bp_node_get_fail_fwd_bytes(tdc_t params);
value_t adm_bp_node_get_fail_abandon_cnt(tdc_t params);
value_t adm_bp_node_get_fail_abandon_bytes(tdc_t params);
value_t adm_bp_node_get_fail_discard_cnt(tdc_t params);
value_t adm_bp_node_get_fail_discard_bytes(tdc_t params);

/* BP ENDPOINT */
value_t adm_bp_endpoint_get_names(tdc_t params);
value_t adm_bp_endpoint_get_name(tdc_t params);
value_t adm_bp_endpoint_get_active(tdc_t params);
value_t adm_bp_endpoint_get_singleton(tdc_t params);
value_t adm_bp_endpoint_get_abandon(tdc_t params);


/******************************************************************************
 *                              Control Functions                             *
 ******************************************************************************/

tdc_t* adm_bp_ctrl_reset(eid_t *def_mgr, tdc_t params, int8_t *status);

#endif //#ifndef ADM_BP_IMPL_H_


