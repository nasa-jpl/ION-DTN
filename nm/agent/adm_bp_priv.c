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
 ** File Name: adm_bp_priv.c
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
#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "shared/adm/adm_bp.h"
#include "shared/utils/utils.h"

#include "adm_bp_priv.h"

void agent_adm_init_bp()
{

	/* Node-specific Information. */
	uint8_t mid_str[ADM_MID_ALLOC];

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_all);

	/* Node State Information */
	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_node_id);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_version);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_storage);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_last_restart);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 5, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_num_reg);


	/* Bundle State Information */

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 6, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fwd_pend);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 7, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_dispatch_pend);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 8, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_in_cust);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 9, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_reassembly_pend);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 10, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_blk_src_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 11, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_norm_src_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 12, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_exp_src_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 13, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_blk_src_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 14, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_norm_src_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 15, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_exp_src_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 16, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_blk_res_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 17, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_norm_res_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 18, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_exp_res_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 19, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_blk_res_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 20, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_norm_res_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 21, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_exp_res_bytes);


	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 22, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_bundles_frag);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 23, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_frag_produced);


	/* Error and Reporting Information */
	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 24, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_none);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 25, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_expired);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 26, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_fwd_uni);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 27, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_cancel);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 28, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_deplete);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 29, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_bad_eid);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 30, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_no_route);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 31, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_no_contact);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 32, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_bad_blk);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 33, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_del_bytes);


	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 34, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_cust_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 35, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_cust_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 36, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_fwd_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 37, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_fwd_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 38, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_abandon_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 39, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_abandon_bytes);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 40, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_discard_cnt);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 41, mid_str);
	adm_add_datadef_collect(mid_str, bp_node_get_fail_discard_bytes);


	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 42, mid_str);
	adm_add_datadef_collect(mid_str, bp_endpoint_get_names);


	/* Endpoint-Specific Information */

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str, bp_endpoint_get_all);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str, bp_endpoint_get_name);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str, bp_endpoint_get_active);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str, bp_endpoint_get_singleton);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str, bp_endpoint_get_abandon);


	/* Controls */
	adm_build_mid_str(0x01, BP_ADM_DATA_CTRL_NN, BP_ADM_DATA_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl_run(mid_str, bp_ctrl_reset);

}


expr_result_t bp_node_get_all(Lyst params)
{
	NmbpNode node_state;
	NmbpDisposition state;
	expr_result_t result;

	bpnm_node_get(&node_state);
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_BLOB;
	result.value = (uint8_t*) MTAKE(sizeof(node_state) + sizeof(state));
	memcpy(result.value, &node_state, sizeof(node_state));
	memcpy(result.value + sizeof(node_state), &state, sizeof(state));
	result.length = sizeof(node_state) + sizeof(state);

	return result;
}

expr_result_t bp_node_get_node_id(Lyst params)
{
	expr_result_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = EXPR_TYPE_STRING;
	result.value = adm_copy_string((char *) node_state.nodeID, &(result.length));
	return result;
}

expr_result_t bp_node_get_version(Lyst params)
{
	expr_result_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = EXPR_TYPE_STRING;
	result.value = adm_copy_string((char *) node_state.bpVersionNbr, &(result.length));

	return result;
}

expr_result_t bp_node_get_storage(Lyst params)
{
	expr_result_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(node_state.avblStorage),
								    sizeof(node_state.avblStorage),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_last_restart(Lyst params)
{
	expr_result_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(node_state.lastRestartTime),
								    sizeof(node_state.lastRestartTime),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_num_reg(Lyst params)
{
	expr_result_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(node_state.nbrOfRegistrations),
								    sizeof(node_state.nbrOfRegistrations),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_fwd_pend(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentForwardPending),
								    sizeof(state.currentForwardPending),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_dispatch_pend(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentDispatchPending),
								    sizeof(state.currentDispatchPending),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_in_cust(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentInCustody),
								    sizeof(state.currentInCustody),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_reassembly_pend(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentReassemblyPending),
								    sizeof(state.currentReassemblyPending),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_blk_src_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleSourceCount[0]),
								    sizeof(state.bundleSourceCount[0]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_norm_src_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleSourceCount[1]),
								    sizeof(state.bundleSourceCount[1]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_exp_src_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleSourceCount[2]),
								    sizeof(state.bundleSourceCount[2]),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_blk_src_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleSourceBytes[0]),
								    sizeof(state.bundleSourceBytes[0]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_norm_src_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleSourceBytes[1]),
								    sizeof(state.bundleSourceBytes[1]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_exp_src_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleSourceBytes[2]),
								    sizeof(state.bundleSourceBytes[2]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_blk_res_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentResidentCount[0]),
								    sizeof(state.currentResidentCount[0]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_norm_res_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentResidentCount[1]),
								    sizeof(state.currentResidentCount[1]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_exp_res_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentResidentCount[2]),
								    sizeof(state.currentResidentCount[2]),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_blk_res_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentResidentBytes[0]),
								    sizeof(state.currentResidentBytes[0]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_norm_res_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentResidentBytes[1]),
								    sizeof(state.currentResidentBytes[1]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_exp_res_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.currentResidentBytes[2]),
								    sizeof(state.currentResidentBytes[2]),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_bundles_frag(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundlesFragmented),
								    sizeof(state.bundlesFragmented),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_frag_produced(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.fragmentsProduced),
								    sizeof(state.fragmentsProduced),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_del_none(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delNoneCount),
								    sizeof(state.delNoneCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_expired(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delExpiredCount),
								    sizeof(state.delExpiredCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_fwd_uni(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delFwdUnidirCount),
								    sizeof(state.delFwdUnidirCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_cancel(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delCanceledCount),
								    sizeof(state.delCanceledCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_deplete(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delDepletionCount),
								    sizeof(state.delDepletionCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_bad_eid(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delEidMalformedCount),
								    sizeof(state.delEidMalformedCount),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_del_no_route(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delNoRouteCount),
								    sizeof(state.delNoRouteCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_no_contact(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delNoContactCount),
								    sizeof(state.delNoContactCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_bad_blk(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.delBlkMalformedCount),
								    sizeof(state.delBlkMalformedCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_del_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bytesDeletedToDate),
								    sizeof(state.bytesDeletedToDate),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_fail_cust_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.custodyRefusedCount),
								    sizeof(state.custodyRefusedCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_fail_cust_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.custodyRefusedBytes),
								    sizeof(state.custodyRefusedBytes),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_fail_fwd_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleFwdFailedCount),
								    sizeof(state.bundleFwdFailedCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_fail_fwd_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleFwdFailedBytes),
								    sizeof(state.bundleFwdFailedBytes),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_fail_abandon_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleAbandonCount),
								    sizeof(state.bundleAbandonCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_fail_abandon_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleAbandonBytes),
								    sizeof(state.bundleAbandonBytes),
								    &(result.length));
	return result;
}


expr_result_t bp_node_get_fail_discard_cnt(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleDiscardCount),
								    sizeof(state.bundleDiscardCount),
								    &(result.length));
	return result;
}

expr_result_t bp_node_get_fail_discard_bytes(Lyst params)
{
	expr_result_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = EXPR_TYPE_UVAST;
	result.value = adm_copy_integer((uint8_t*)&(state.bundleDiscardBytes),
								    sizeof(state.bundleDiscardBytes),
								    &(result.length));
	return result;
}











expr_result_t bp_endpoint_get_names(Lyst params)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	uint8_t *cursor = NULL;


	bpnm_endpointNames_get((char *) names, 2048, ptrs, &num);


	extern void	bpnm_endpointNames_get(char * nameBuffer, int bufLen,
				char * nameArray [], int * numStrings);


	encodeSdnv(&nm_sdnv, num);

	result.length = nm_sdnv.length +         /* NUM as SDNV length */
			        strlen(ptrs[num-1]) +    /* length of last string */
			        (ptrs[num-1] - names) +  /* # bytes to get to last string */
			        1;                       /* Final NULL terminator. */
	result.value = (uint8_t *) MTAKE(result.length);

	cursor = result.value;

	memcpy(cursor,nm_sdnv.text, nm_sdnv.length);
	cursor += nm_sdnv.length;

	memcpy(cursor, names, result.length - nm_sdnv.length);

	return result;
}

expr_result_t bp_endpoint_get_all(Lyst params)
{
	expr_result_t result;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];

	/* \todo: Check for NULL entry here. */
	NmbpEndpoint endpoint;
	int success = 0;

	result.type = EXPR_TYPE_BLOB;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value = (uint8_t*) MTAKE(sizeof(endpoint));
		memcpy(result.value, &endpoint, sizeof(endpoint));
		result.length = sizeof(endpoint);
	}

	return result;
}


expr_result_t bp_endpoint_get_name(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_STRING;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.length = (uint64_t) strlen(endpoint.eid) + 1;
		result.value = (uint8_t*) MTAKE(result.length);
		memset(result.value,0,result.length);
		memcpy(result.value, endpoint.eid, result.length);
	}

	return result;
}

expr_result_t bp_endpoint_get_active(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(endpoint.active),
									    sizeof(endpoint.active),
									    &(result.length));
	}

	return result;
}

expr_result_t bp_endpoint_get_singleton(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(endpoint.singleton),
									    sizeof(endpoint.singleton),
									    &(result.length));
	}

	return result;
}


expr_result_t bp_endpoint_get_abandon(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(endpoint.abandonOnDelivFailure),
									    sizeof(endpoint.abandonOnDelivFailure),
									    &(result.length));
	}

	return result;
}



/* Controls */
uint32_t bp_ctrl_reset(Lyst params)
{
	bpnm_disposition_reset();
	return 0;
}


