/****************************************************************************
 **
 ** File Name: adm_bp_agent.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2018-01-08  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_bp.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_bp_impl.h"
#include "rda.h"

#define _HAVE_BP_ADM_
#ifdef _HAVE_BP_ADM_

void adm_bp_init()
{
	adm_bp_setup();
	adm_bp_init_edd();
	adm_bp_init_variables();
	adm_bp_init_controls();
	adm_bp_init_constants();
	adm_bp_init_macros();
	adm_bp_init_metadata();
	adm_bp_init_ops();
	adm_bp_init_reports();
}

void adm_bp_init_edd()
{
	adm_add_edd(mid_from_value(ADM_BP_EDD_BP_NODE_ID_MID), AMP_TYPE_STR, 0, adm_bp_get_bp_node_id, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_BP_NODE_VERSION_MID), AMP_TYPE_STR, 0, adm_bp_get_bp_node_version, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_AVAILABLE_STORAGE_MID), AMP_TYPE_UVAST, 0, adm_bp_get_available_storage, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_LAST_RESET_TIME_MID), AMP_TYPE_UVAST, 0, adm_bp_get_last_reset_time, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_REGISTRATIONS_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_registrations, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_PEND_FWD_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_pend_fwd, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_PEND_DIS_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_pend_dis, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_IN_CUST_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_in_cust, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_PEND_REASSEMBLY_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_pend_reassembly, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID), AMP_TYPE_UINT, 0, adm_bp_get_bundles_by_priority, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_BYTES_BY_PRIORITY_MID), AMP_TYPE_UINT, 0, adm_bp_get_bytes_by_priority, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID), AMP_TYPE_UINT, 0, adm_bp_get_src_bundles_by_priority, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID), AMP_TYPE_UINT, 0, adm_bp_get_src_bytes_by_priority, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_FRAGMENTED_BUNDLES_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_fragmented_bundles, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_FRAGMENTS_PRODUCED_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_fragments_produced, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_failed_by_reason, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_NUM_BUNDLES_DELETED_MID), AMP_TYPE_UINT, 0, adm_bp_get_num_bundles_deleted, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_FAILED_CUSTODY_BUNDLES_MID), AMP_TYPE_UINT, 0, adm_bp_get_failed_custody_bundles, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_FAILED_CUSTODY_BYTES_MID), AMP_TYPE_UINT, 0, adm_bp_get_failed_custody_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_FAILED_FORWARD_BUNDLES_MID), AMP_TYPE_UINT, 0, adm_bp_get_failed_forward_bundles, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_FAILED_FORWARD_BYTES_MID), AMP_TYPE_UINT, 0, adm_bp_get_failed_forward_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_ABANDONED_BUNDLES_MID), AMP_TYPE_UINT, 0, adm_bp_get_abandoned_bundles, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_ABANDONED_BYTES_MID), AMP_TYPE_UINT, 0, adm_bp_get_abandoned_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_DISCARDED_BUNDLES_MID), AMP_TYPE_UINT, 0, adm_bp_get_discarded_bundles, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_DISCARDED_BYTES_MID), AMP_TYPE_UINT, 0, adm_bp_get_discarded_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_ENDPOINT_NAMES_MID), AMP_TYPE_STR, 0, adm_bp_get_endpoint_names, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_ENDPOINT_ACTIVE_MID), AMP_TYPE_UINT, 0, adm_bp_get_endpoint_active, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_ENDPOINT_SINGLETON_MID), AMP_TYPE_UINT, 0, adm_bp_get_endpoint_singleton, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BP_EDD_ENDPOINT_POLICY_MID), AMP_TYPE_UINT, 0, adm_bp_get_endpoint_policy, NULL, NULL);

}

void adm_bp_init_variables()
{

}

void adm_bp_init_controls()
{


	adm_add_ctrl(mid_from_value(ADM_BP_CTRL_RESET_ALL_COUNTS_MID),adm_bp_ctrl_reset_all_counts);
}

void adm_bp_init_constants()
{
}

void adm_bp_init_macros()
{
}

void adm_bp_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(BP_ADM_META_NN_IDX, BP_ADM_META_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_EDD_NN_IDX, BP_ADM_EDD_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_VAR_NN_IDX, BP_ADM_VAR_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_RPT_NN_IDX, BP_ADM_RPT_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_CTRL_NN_IDX, BP_ADM_CTRL_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_CONST_NN_IDX, BP_ADM_CONST_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_MACRO_NN_IDX, BP_ADM_MACRO_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_OP_NN_IDX, BP_ADM_OP_NN_STR, "BP", "2017-08-17");
	oid_nn_add_parm(BP_ADM_ROOT_NN_IDX, BP_ADM_ROOT_NN_STR, "BP", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_BP_META_NAME_MID), AMP_TYPE_STR, 0, adm_bp_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_BP_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_bp_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_BP_META_VERSION_MID), AMP_TYPE_STR, 0, adm_bp_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_BP_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_bp_meta_organization, adm_print_string, adm_size_string);
}

void adm_bp_init_ops()
{
}

void adm_bp_init_reports()
{
	Lyst rpt = NULL;
	uint32_t used= 0;
	mid_t *cur_mid = NULL;
	uint32_t val = 0;

	rpt = lyst_create();
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_META_NAME_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_META_VERSION_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_BP_NODE_ID_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_BP_NODE_VERSION_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_AVAILABLE_STORAGE_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_LAST_RESET_TIME_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_REGISTRATIONS_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_PEND_FWD_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_PEND_DIS_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_IN_CUST_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_PEND_REASSEMBLY_MID),0));

	cur_mid = mid_from_value(ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(1));
	lyst_insert_last(rpt, rpttpl_item_create(cur_mid,0));

	cur_mid = mid_from_value(ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(2));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid,0));

	cur_mid = mid_from_value(ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(4));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid,0));

	cur_mid = mid_from_value(ADM_BP_EDD_BYTES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(1));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_BYTES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(2));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_BYTES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(4));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(1));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(2));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(4));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(1));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(2));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(4));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_FRAGMENTED_BUNDLES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_FRAGMENTS_PRODUCED_MID),0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x1));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x2));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x4));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x8));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x10));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x20));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x40));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x80));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	cur_mid = mid_from_value(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);
	mid_add_param_from_value(cur_mid, val_from_uint(0x100));
	lyst_insert_last(rpt,rpttpl_item_create(cur_mid, 0));

	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_NUM_BUNDLES_DELETED_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_FAILED_CUSTODY_BUNDLES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_FAILED_CUSTODY_BYTES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_FAILED_FORWARD_BUNDLES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_FAILED_FORWARD_BYTES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_ABANDONED_BUNDLES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_DISCARDED_BUNDLES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_DISCARDED_BYTES_MID),0));
	lyst_insert_last(rpt,rpttpl_item_create(mid_from_value(ADM_BP_EDD_ENDPOINT_NAMES_MID),0));


	adm_add_rpttpl(mid_from_value(ADM_BP_RPT_FULL_REPORT_MID), rpt);


	rpt = lyst_create();

	rpttpl_item_t *cur_item = NULL;

	cur_mid = mid_from_value(ADM_BP_EDD_ENDPOINT_ACTIVE_MID);
	cur_item = rpttpl_item_create(cur_mid, 1);
	mid_add_param_from_value(cur_mid, val_from_string(""));
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_mid = mid_from_value(ADM_BP_EDD_ENDPOINT_SINGLETON_MID);
	cur_item = rpttpl_item_create(cur_mid, 1);
	mid_add_param_from_value(cur_mid, val_from_string(""));
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	cur_mid = mid_from_value(ADM_BP_EDD_ENDPOINT_POLICY_MID);
	cur_item = rpttpl_item_create(cur_mid, 1);
	mid_add_param_from_value(cur_mid, val_from_string(""));
	rpttpl_item_add_parm_map(cur_item, 1, 1);
	lyst_insert_last(rpt,cur_item);

	adm_add_rpttpl(mid_from_value(ADM_BP_RPT_ENDPOINT_REPORT_MID), rpt);
}

#endif // _HAVE_BP_ADM_
