/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: adm_bp.h
 **
 ** Description: This file contains the definitions of the Bundle Protocol
 **              ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **      1. We current use a non-official OID root tree for DTN Bundle Protocol
 **         identifiers.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "../adm/adm_bp.h"
#include "../utils/utils.h"
#include "../primitives/def.h"
#include "../primitives/nn.h"

#ifdef AGENT_ROLE
#include "../../agent/adm_bp_impl.h"
#else
#include "../../mgr/nm_mgr_names.h"
#include "../../mgr/nm_mgr_ui.h"
#endif


void adm_bp_init()
{
	adm_bp_init_atomic();
	adm_bp_init_computed();
	adm_bp_init_controls();
	adm_bp_init_literals();
	adm_bp_init_macros();
	adm_bp_init_metadata();
	adm_bp_init_ops();
	adm_bp_init_reports();
}



void adm_bp_init_atomic()
{

	#ifdef AGENT_ROLE

	adm_add_datadef(ADM_BP_AD_NODE_ID_MID,     AMP_TYPE_STRING, 0, adm_bp_node_get_node_id,  adm_print_string, adm_size_string);
	adm_add_datadef(ADM_BP_AD_NODE_VER_MID,    AMP_TYPE_STRING, 0, adm_bp_node_get_version,  adm_print_string, adm_size_string);
	adm_add_datadef(ADM_BP_AD_AVAIL_STOR_MID,  AMP_TYPE_UVAST,  0, adm_bp_node_get_storage,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RESET_TIME_MID,  AMP_TYPE_UVAST,  0, adm_bp_node_get_last_restart,  NULL, bp_size_node_restart_time);
	adm_add_datadef(ADM_BP_AD_NUM_REG_MID,     AMP_TYPE_UVAST,  0, adm_bp_node_get_num_reg,  NULL, bp_size_node_num_reg);
	adm_add_datadef(ADM_BP_AD_ENDPT_NAMES_MID, AMP_TYPE_STRING, 0, adm_bp_endpoint_get_names,  adm_print_string,   adm_size_string);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_FWD_PEND_CNT_MID,      AMP_TYPE_UVAST, 0, adm_bp_node_get_fwd_pend,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_DISPATCH_PEND_CNT_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_dispatch_pend,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_IN_CUSTODY_CNT_MID,    AMP_TYPE_UVAST, 0, adm_bp_node_get_in_cust,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_REASSMBL_PEND_CNT_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_reassembly_pend,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_BULK_RES_CNT_MID,      AMP_TYPE_UVAST, 0, adm_bp_node_get_blk_src_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_NORM_RES_CNT_MID,      AMP_TYPE_UVAST, 0, adm_bp_node_get_norm_src_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_EXP_RES_CNT_MID,       AMP_TYPE_UVAST, 0, adm_bp_node_get_exp_src_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_BULK_RES_BYTES_MID,    AMP_TYPE_UVAST, 0, adm_bp_node_get_blk_src_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_NORM_BYTES_MID,        AMP_TYPE_UVAST, 0, adm_bp_node_get_norm_src_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_CUR_EXP_BYTES_MID,         AMP_TYPE_UVAST, 0, adm_bp_node_get_exp_src_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_BULK_SRC_CNT_MID,          AMP_TYPE_UVAST, 0, adm_bp_node_get_blk_res_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_NORM_SRC_CNT_MID,          AMP_TYPE_UVAST, 0, adm_bp_node_get_norm_res_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_EXP_SRC_CNT_MID,           AMP_TYPE_UVAST, 0, adm_bp_node_get_exp_res_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_BULK_SRC_BYTES_MID,        AMP_TYPE_UVAST, 0, adm_bp_node_get_blk_res_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_NORM_SRC_BYTES_MID,        AMP_TYPE_UVAST, 0, adm_bp_node_get_norm_res_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_EXP_SRC_BYTES_MID,         AMP_TYPE_UVAST, 0, adm_bp_node_get_exp_res_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_FRAGMENTED_CNT_MID,        AMP_TYPE_UVAST, 0, adm_bp_node_get_bundles_frag,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_BNDL_FRAG_PRODUCED_MID,         AMP_TYPE_UVAST, 0, adm_bp_node_get_frag_produced,  NULL, NULL);

	adm_add_datadef(ADM_BP_AD_RPT_NOINFO_DEL_CNT_MID,  AMP_TYPE_UVAST, 0, adm_bp_node_get_del_none,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_EXPIRED_DEL_CNT_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_del_expired,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_UNI_FWD_DEL_CNT_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_del_fwd_uni,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_CANCEL_DEL_CNT_MID,  AMP_TYPE_UVAST, 0, adm_bp_node_get_del_cancel,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_NO_STRG_DEL_CNT_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_del_deplete,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_BAD_EID_DEL_CNT_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_del_bad_eid,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_NO_ROUTE_DEL_CNT_MID,AMP_TYPE_UVAST, 0, adm_bp_node_get_del_no_route,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_NO_CONTACT_DEL_CNT_MID,   AMP_TYPE_UVAST, 0, adm_bp_node_get_del_no_contact,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_BAD_BLOCK_DEL_CNT_MID,    AMP_TYPE_UVAST, 0, adm_bp_node_get_del_bad_blk,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_BUNDLES_DEL_CNT_MID,      AMP_TYPE_UVAST, 0, adm_bp_node_get_del_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_FAIL_CUST_XFER_CNT_MID,   AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_cust_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_FAIL_CUST_XFER_BYTES_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_cust_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_FAIL_FWD_CNT_MID,    AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_fwd_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_FAIL_FWD_BYTES_MID,  AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_fwd_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_ABANDONED_CNT_MID,   AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_abandon_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_ABANDONED_BYTES_MID, AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_abandon_bytes,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_DISCARD_CNT_MID,     AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_discard_cnt,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_RPT_DISCARD_BYTES_MID,   AMP_TYPE_UVAST, 0, adm_bp_node_get_fail_discard_bytes,  NULL, NULL);

	adm_add_datadef(ADM_BP_AD_ENDPT_NAME_MID,                AMP_TYPE_STRING, 1, adm_bp_endpoint_get_name,  adm_print_string, bp_size_endpoint_name);
	adm_add_datadef(ADM_BP_AD_ENDPT_ACTIVE_MID,              AMP_TYPE_UINT, 1, adm_bp_endpoint_get_active,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_ENDPT_SINGLETON_MID,           AMP_TYPE_UINT, 1, adm_bp_endpoint_get_singleton,  NULL, NULL);
	adm_add_datadef(ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID, AMP_TYPE_UINT, 1, adm_bp_endpoint_get_abandon,  NULL, NULL);

#else
	adm_add_datadef(ADM_BP_AD_NODE_ID_MID,     AMP_TYPE_STRING, 0, NULL,  adm_print_string, adm_size_string);
	names_add_name("NODE ID", "Node Identifier", ADM_BP, ADM_BP_AD_NODE_ID_MID);

	adm_add_datadef(ADM_BP_AD_NODE_VER_MID,    AMP_TYPE_STRING, 0, NULL,  adm_print_string, adm_size_string);
	names_add_name("NODE VER", "Node Version", ADM_BP, ADM_BP_AD_NODE_VER_MID);

	adm_add_datadef(ADM_BP_AD_AVAIL_STOR_MID,  AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("AVAIL STOR", "Available Storage in Bytes", ADM_BP, ADM_BP_AD_AVAIL_STOR_MID);

	adm_add_datadef(ADM_BP_AD_RESET_TIME_MID,  AMP_TYPE_UVAST, 0, NULL,  NULL, bp_size_node_restart_time);
	names_add_name("RESET TIME", "UTC of Last Reset", ADM_BP, ADM_BP_AD_RESET_TIME_MID);

	adm_add_datadef(ADM_BP_AD_NUM_REG_MID,     AMP_TYPE_UVAST, 0, NULL,  NULL, bp_size_node_num_reg);
	names_add_name("NUM REG", "# Registered Nodes", ADM_BP, ADM_BP_AD_NUM_REG_MID);

	adm_add_datadef(ADM_BP_AD_ENDPT_NAMES_MID, AMP_TYPE_STRING, 0, NULL,  adm_print_string,   adm_size_string);
	names_add_name("ENDPT NAMES", "Endpoint Names", ADM_BP, ADM_BP_AD_ENDPT_NAMES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_FWD_PEND_CNT_MID, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("CUR FWD PEND CNT", "# Bundles Pending Forwarding", ADM_BP, ADM_BP_AD_BNDL_CUR_FWD_PEND_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_DISPATCH_PEND_CNT_MID, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("CUR DISPATCH PEND CNT", "# Bundles Pending Dispatch", ADM_BP, ADM_BP_AD_BNDL_CUR_DISPATCH_PEND_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_IN_CUSTODY_CNT_MID,    AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("CUR IN CUSTODY CNT", "# Bundles In Custody", ADM_BP, ADM_BP_AD_BNDL_CUR_IN_CUSTODY_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_REASSMBL_PEND_CNT_MID, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("CUR REASSMBL PEND CNT", "# Bundles Pending Reassembly", ADM_BP, ADM_BP_AD_BNDL_CUR_REASSMBL_PEND_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_BULK_RES_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("CUR BULK RES CNT", "# Bundles Pending Reassembly", ADM_BP, ADM_BP_AD_BNDL_CUR_BULK_RES_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_NORM_RES_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_CUR_NORM_RES_CNT_MID", "Cur Norm Res Cnt", ADM_BP, ADM_BP_AD_BNDL_CUR_NORM_RES_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_EXP_RES_CNT_MID,       AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_CUR_EXP_RES_CNT_MID", "Cur Exp Res Cnt", ADM_BP, ADM_BP_AD_BNDL_CUR_EXP_RES_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_BULK_RES_BYTES_MID,    AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_CUR_BULK_RES_BYTES_MID", "Cur Bulk Res Bytes", ADM_BP, ADM_BP_AD_BNDL_CUR_BULK_RES_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_NORM_BYTES_MID,        AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_CUR_NORM_BYTES_MID", "Cur Norm Bytes", ADM_BP, ADM_BP_AD_BNDL_CUR_NORM_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_CUR_EXP_BYTES_MID,         AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_CUR_EXP_BYTES_MID", "Cur Exp Bytes", ADM_BP, ADM_BP_AD_BNDL_CUR_EXP_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_BULK_SRC_CNT_MID,          AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_BULK_SRC_CNT_MID", "Bulk Src Cnt", ADM_BP, ADM_BP_AD_BNDL_BULK_SRC_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_NORM_SRC_CNT_MID,          AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_NORM_SRC_CNT_MID", "Norm Src Cnt", ADM_BP, ADM_BP_AD_BNDL_NORM_SRC_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_EXP_SRC_CNT_MID,           AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_EXP_SRC_CNT_MID", "Exp Src Cnt", ADM_BP, ADM_BP_AD_BNDL_EXP_SRC_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_BULK_SRC_BYTES_MID,        AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_BULK_SRC_BYTES_MID", "Bulk Src Bytes", ADM_BP, ADM_BP_AD_BNDL_BULK_SRC_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_NORM_SRC_BYTES_MID,        AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_NORM_SRC_BYTES_MID", "Norm Src Bytes", ADM_BP, ADM_BP_AD_BNDL_NORM_SRC_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_EXP_SRC_BYTES_MID,         AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_EXP_SRC_BYTES_MID", "Exp Src Bytes", ADM_BP, ADM_BP_AD_BNDL_EXP_SRC_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_FRAGMENTED_CNT_MID,        AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_FRAGMENTED_CNT_MID", "Bundles Fragmented Count", ADM_BP, ADM_BP_AD_BNDL_FRAGMENTED_CNT_MID);

	adm_add_datadef(ADM_BP_AD_BNDL_FRAG_PRODUCED_MID,         AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_BNDL_FRAG_PRODUCED_MID", "Bundles Fragments Produced", ADM_BP, ADM_BP_AD_BNDL_FRAG_PRODUCED_MID);





	adm_add_datadef(ADM_BP_AD_RPT_NOINFO_DEL_CNT_MID,       AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_NOINFO_DEL_CNT_MID", "No-Info Del Cnt", ADM_BP, ADM_BP_AD_RPT_NOINFO_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_EXPIRED_DEL_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_EXPIRED_DEL_CNT_MID", "Expired Del Cnt", ADM_BP, ADM_BP_AD_RPT_EXPIRED_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_UNI_FWD_DEL_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_UNI_FWD_DEL_CNT_MID", "Uni Fwd Del Cnt", ADM_BP, ADM_BP_AD_RPT_UNI_FWD_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_CANCEL_DEL_CNT_MID,       AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_CANCEL_DEL_CNT_MID", "Cancel Del Cnt", ADM_BP, ADM_BP_AD_RPT_CANCEL_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_NO_STRG_DEL_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_NO_STRG_DEL_CNT_MID", "No Storage Del Cnt", ADM_BP, ADM_BP_AD_RPT_NO_STRG_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_BAD_EID_DEL_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_BAD_EID_DEL_CNT_MID", "Bad EID Del Cnt", ADM_BP, ADM_BP_AD_RPT_BAD_EID_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_NO_ROUTE_DEL_CNT_MID,     AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_NO_ROUTE_DEL_CNT_MID", "No Route Del Cnt", ADM_BP, ADM_BP_AD_RPT_NO_ROUTE_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_NO_CONTACT_DEL_CNT_MID,   AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_NO_CONTACT_DEL_CNT_MID", "No Contact Del Cnt", ADM_BP, ADM_BP_AD_RPT_NO_CONTACT_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_BAD_BLOCK_DEL_CNT_MID,    AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_BAD_BLOCK_DEL_CNT_MID", "Bad Block Del Cnt", ADM_BP, ADM_BP_AD_RPT_BAD_BLOCK_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_BUNDLES_DEL_CNT_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_BUNDLES_DEL_CNT_MID", "Bundles Del Cnt", ADM_BP, ADM_BP_AD_RPT_BUNDLES_DEL_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_FAIL_CUST_XFER_CNT_MID,   AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_FAIL_CUST_XFER_CNT_MID", "Cust Fail Del Cnt", ADM_BP, ADM_BP_AD_RPT_FAIL_CUST_XFER_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_FAIL_CUST_XFER_BYTES_MID, AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_FAIL_CUST_XFER_BYTES_MID", "Cust Fail Bytes", ADM_BP, ADM_BP_AD_RPT_FAIL_CUST_XFER_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_RPT_FAIL_FWD_CNT_MID,         AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_FAIL_FWD_CNT_MID", "Fail Fwd Cnt", ADM_BP, ADM_BP_AD_RPT_FAIL_FWD_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_FAIL_FWD_BYTES_MID,       AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_FAIL_FWD_BYTES_MID", "Fail Fwd Bytes", ADM_BP, ADM_BP_AD_RPT_FAIL_FWD_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_RPT_ABANDONED_CNT_MID,        AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_ABANDONED_CNT_MID", "Abandoned Cnt", ADM_BP, ADM_BP_AD_RPT_ABANDONED_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_ABANDONED_BYTES_MID,      AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_ABANDONED_BYTES_MID", "Abandoned Bytes", ADM_BP, ADM_BP_AD_RPT_ABANDONED_BYTES_MID);

	adm_add_datadef(ADM_BP_AD_RPT_DISCARD_CNT_MID,          AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_DISCARD_CNT_MID", "Discard Count", ADM_BP, ADM_BP_AD_RPT_DISCARD_CNT_MID);

	adm_add_datadef(ADM_BP_AD_RPT_DISCARD_BYTES_MID,        AMP_TYPE_UVAST, 0, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_RPT_DISCARD_BYTES_MID", "Discard Bytes", ADM_BP, ADM_BP_AD_RPT_DISCARD_BYTES_MID);



	adm_add_datadef(ADM_BP_AD_ENDPT_NAME_MID,                AMP_TYPE_STRING, 1, NULL,  adm_print_string, bp_size_endpoint_name);
	names_add_name("ADM_BP_AD_ENDPT_NAME_MID", "Endpoint Names", ADM_BP, ADM_BP_AD_ENDPT_NAME_MID);
	ui_add_parmspec(ADM_BP_AD_ENDPT_NAME_MID, 1, "Endpt Name", AMP_TYPE_STRING, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	adm_add_datadef(ADM_BP_AD_ENDPT_ACTIVE_MID,              AMP_TYPE_UINT, 1, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_ENDPT_ACTIVE_MID", "Active Endpoint", ADM_BP, ADM_BP_AD_ENDPT_ACTIVE_MID);
	ui_add_parmspec(ADM_BP_AD_ENDPT_ACTIVE_MID, 1, "Endpt Name", AMP_TYPE_STRING, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	adm_add_datadef(ADM_BP_AD_ENDPT_SINGLETON_MID,           AMP_TYPE_UINT, 1, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_ENDPT_SINGLETON_MID", "Singleton Endpoint", ADM_BP, ADM_BP_AD_ENDPT_SINGLETON_MID);
	ui_add_parmspec(ADM_BP_AD_ENDPT_SINGLETON_MID, 1, "Endpt Name", AMP_TYPE_STRING, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	adm_add_datadef(ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID, AMP_TYPE_UINT, 1, NULL,  NULL, NULL);
	names_add_name("ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID", "Abandon On Del Fail", ADM_BP, ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID);
	ui_add_parmspec(ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID, 1,"Endpt Name", AMP_TYPE_STRING, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

#endif

}



void adm_bp_init_computed()
{

}


void adm_bp_init_controls()
{

#ifdef AGENT_ROLE
	adm_add_ctrl(ADM_BP_CTL_RESET_BP_COUNTS, adm_bp_ctrl_reset);

#else
	adm_add_ctrl(ADM_BP_CTL_RESET_BP_COUNTS, NULL);
	names_add_name("ADM_BP_CTL_RESET_BP_COUNTS", "Reset BP Counts", ADM_BP, ADM_BP_CTL_RESET_BP_COUNTS);
#endif

}


void adm_bp_init_literals()
{

}


void adm_bp_init_macros()
{

}



void adm_bp_init_metadata()
{
	/* Step 1: Register Nicknames */

	oid_nn_add_parm(BP_ADM_MD_NN_IDX,   BP_ADM_MD_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_AD_NN_IDX,   BP_ADM_AD_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_CD_NN_IDX,   BP_ADM_CD_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_RPT_NN_IDX,  BP_ADM_RPT_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_CTRL_NN_IDX, BP_ADM_CTRL_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_LTRL_NN_IDX, BP_ADM_LTRL_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_MAC_NN_IDX,  BP_ADM_MAC_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_OP_NN_IDX,   BP_ADM_OP_NN_STR, "BP", "6");
	oid_nn_add_parm(BP_ADM_ROOT_NN_IDX, BP_ADM_ROOT_NN_STR, "BP", "6");

	/* Step 2: Register Metadata Information. */
#ifdef AGENT_ROLE
	adm_add_datadef(ADM_BP_MD_NAME_MID, AMP_TYPE_STRING, 0, adm_bp_md_name, adm_print_string, adm_size_string);
	adm_add_datadef(ADM_BP_MD_VER_MID,  AMP_TYPE_STRING, 0, adm_bp_md_ver,  adm_print_string, adm_size_string);
#else
	adm_add_datadef(ADM_BP_MD_NAME_MID, AMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_BP_MD_NAME_MID", "BP Name", ADM_BP, ADM_BP_MD_NAME_MID);

	adm_add_datadef(ADM_BP_MD_VER_MID,  AMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_BP_MD_VER_MID", "BP Version", ADM_BP, ADM_BP_MD_VER_MID);
#endif

}

void adm_bp_init_names()
{
}


void adm_bp_init_ops()
{
}

void adm_bp_init_reports()
{
	uint32_t used = 0;
	Lyst rpt = lyst_create();

	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_NODE_ID_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_NODE_VER_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_AVAIL_STOR_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RESET_TIME_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_NUM_REG_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_FWD_PEND_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_DISPATCH_PEND_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_IN_CUSTODY_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_REASSMBL_PEND_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_BULK_RES_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_NORM_RES_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_EXP_RES_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_BULK_RES_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_NORM_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_CUR_EXP_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_BULK_SRC_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_NORM_SRC_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_EXP_SRC_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_BULK_SRC_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_NORM_SRC_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_EXP_SRC_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_FRAGMENTED_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_BNDL_FRAG_PRODUCED_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_NOINFO_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_EXPIRED_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_UNI_FWD_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_CANCEL_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_NO_STRG_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_BAD_EID_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_NO_ROUTE_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_NO_CONTACT_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_BAD_BLOCK_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_BUNDLES_DEL_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_FAIL_CUST_XFER_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_FAIL_CUST_XFER_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_FAIL_FWD_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_FAIL_FWD_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_ABANDONED_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_ABANDONED_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_DISCARD_CNT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_RPT_DISCARD_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_ENDPT_NAMES_MID, ADM_MID_ALLOC, &used));

	adm_add_rpt(ADM_BP_RPT_FULL_MID, rpt);

	midcol_destroy(&rpt);

	rpt = lyst_create();
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_ENDPT_NAME_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_ENDPT_ACTIVE_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_ENDPT_SINGLETON_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID, ADM_MID_ALLOC, &used));

	adm_add_rpt(ADM_BP_ENDPT_FULL_MID, rpt);

	midcol_destroy(&rpt);

#ifndef AGENT_ROLE
	names_add_name("ADM_BP_RPT_FULL_MID", "Full Report", ADM_BP, ADM_BP_RPT_FULL_MID);
	names_add_name("ADM_BP_ENDPT_FULL_MID", "Endpoint Full Report", ADM_BP, ADM_BP_ENDPT_FULL_MID);
	ui_add_parmspec(ADM_BP_ENDPT_FULL_MID, 1,"Endpt Name", AMP_TYPE_STRING, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

#endif

}

/* SIZE */



uint32_t bp_size_node_id(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.nodeID);
}

uint32_t bp_size_node_version(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.bpVersionNbr);
}

uint32_t bp_size_node_restart_time(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.lastRestartTime);
}

uint32_t bp_size_node_num_reg(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.nbrOfRegistrations);
}


uint32_t bp_size_endpoint_name(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpEndpoint endpoint;
	return sizeof(endpoint.eid);
}



