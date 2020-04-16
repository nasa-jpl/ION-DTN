/****************************************************************************
 **
 ** File Name: adm_bp_agent_agent.c
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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_bp_agent.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_bp_agent_impl.h"
#include "agent/rda.h"



#define _HAVE_DTN_BP_AGENT_ADM_
#ifdef _HAVE_DTN_BP_AGENT_ADM_

static vec_idx_t g_dtn_bp_agent_idx[11];

void dtn_bp_agent_init()
{
	adm_add_adm_info("dtn_bp_agent", ADM_ENUM_DTN_BP_AGENT);

	VDB_ADD_NN(((ADM_ENUM_DTN_BP_AGENT * 20) + ADM_META_IDX), &(g_dtn_bp_agent_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BP_AGENT * 20) + ADM_RPTT_IDX), &(g_dtn_bp_agent_idx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BP_AGENT * 20) + ADM_EDD_IDX), &(g_dtn_bp_agent_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BP_AGENT * 20) + ADM_CTRL_IDX), &(g_dtn_bp_agent_idx[ADM_CTRL_IDX]));


	dtn_bp_agent_setup();
	dtn_bp_agent_init_meta();
	dtn_bp_agent_init_cnst();
	dtn_bp_agent_init_edd();
	dtn_bp_agent_init_op();
	dtn_bp_agent_init_var();
	dtn_bp_agent_init_ctrl();
	dtn_bp_agent_init_mac();
	dtn_bp_agent_init_rpttpl();
	dtn_bp_agent_init_tblt();
}

void dtn_bp_agent_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_NAME), dtn_bp_agent_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_NAMESPACE), dtn_bp_agent_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_VERSION), dtn_bp_agent_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_ORGANIZATION), dtn_bp_agent_meta_organization);
}

void dtn_bp_agent_init_cnst()
{

}

void dtn_bp_agent_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BP_NODE_ID), dtn_bp_agent_get_bp_node_id);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BP_NODE_VERSION), dtn_bp_agent_get_bp_node_version);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_AVAILABLE_STORAGE), dtn_bp_agent_get_available_storage);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_LAST_RESET_TIME), dtn_bp_agent_get_last_reset_time);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_REGISTRATIONS), dtn_bp_agent_get_num_registrations);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_FWD), dtn_bp_agent_get_num_pend_fwd);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_DIS), dtn_bp_agent_get_num_pend_dis);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_IN_CUST), dtn_bp_agent_get_num_in_cust);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_REASSEMBLY), dtn_bp_agent_get_num_pend_reassembly);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BUNDLES_BY_PRIORITY), dtn_bp_agent_get_bundles_by_priority);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BYTES_BY_PRIORITY), dtn_bp_agent_get_bytes_by_priority);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BUNDLES_BY_PRIORITY), dtn_bp_agent_get_src_bundles_by_priority);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BYTES_BY_PRIORITY), dtn_bp_agent_get_src_bytes_by_priority);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FRAGMENTED_BUNDLES), dtn_bp_agent_get_num_fragmented_bundles);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FRAGMENTS_PRODUCED), dtn_bp_agent_get_num_fragments_produced);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON), dtn_bp_agent_get_num_failed_by_reason);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_BUNDLES_DELETED), dtn_bp_agent_get_num_bundles_deleted);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_CUSTODY_BUNDLES), dtn_bp_agent_get_failed_custody_bundles);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_CUSTODY_BYTES), dtn_bp_agent_get_failed_custody_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_FORWARD_BUNDLES), dtn_bp_agent_get_failed_forward_bundles);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_FORWARD_BYTES), dtn_bp_agent_get_failed_forward_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ABANDONED_BUNDLES), dtn_bp_agent_get_abandoned_bundles);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ABANDONED_BYTES), dtn_bp_agent_get_abandoned_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_DISCARDED_BUNDLES), dtn_bp_agent_get_discarded_bundles);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_DISCARDED_BYTES), dtn_bp_agent_get_discarded_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_NAMES), dtn_bp_agent_get_endpoint_names);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_ACTIVE), dtn_bp_agent_get_endpoint_active);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_SINGLETON), dtn_bp_agent_get_endpoint_singleton);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_POLICY), dtn_bp_agent_get_endpoint_policy);
}

void dtn_bp_agent_init_op()
{

}

void dtn_bp_agent_init_var()
{

}

void dtn_bp_agent_init_ctrl()
{

	adm_add_ctrldef(g_dtn_bp_agent_idx[ADM_CTRL_IDX], DTN_BP_AGENT_CTRL_RESET_ALL_COUNTS, 0, dtn_bp_agent_ctrl_reset_all_counts);
}

void dtn_bp_agent_init_mac()
{

}

void dtn_bp_agent_init_rpttpl()
{

	rpttpl_t *def = NULL;

	/* FULL_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 0, g_dtn_bp_agent_idx[ADM_RPTT_IDX], DTN_BP_AGENT_RPTTPL_FULL_REPORT));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_NAME));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_VERSION));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BP_NODE_ID));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BP_NODE_VERSION));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_AVAILABLE_STORAGE));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_LAST_RESET_TIME));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_REGISTRATIONS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_FWD));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_DIS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_IN_CUST));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_REASSEMBLY));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BUNDLES_BY_PRIORITY, tnv_from_uint(1)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BUNDLES_BY_PRIORITY, tnv_from_uint(2)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BUNDLES_BY_PRIORITY, tnv_from_uint(4)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BYTES_BY_PRIORITY, tnv_from_uint(1)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BYTES_BY_PRIORITY, tnv_from_uint(2)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BYTES_BY_PRIORITY, tnv_from_uint(4)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BUNDLES_BY_PRIORITY, tnv_from_uint(1)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BUNDLES_BY_PRIORITY, tnv_from_uint(2)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BUNDLES_BY_PRIORITY, tnv_from_uint(4)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BYTES_BY_PRIORITY, tnv_from_uint(1)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BYTES_BY_PRIORITY, tnv_from_uint(2)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BYTES_BY_PRIORITY, tnv_from_uint(4)));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FRAGMENTED_BUNDLES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FRAGMENTS_PRODUCED));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(1)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(2)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(4)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(8)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(16)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(32)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(64)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(128)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON, tnv_from_uint(256)));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_BUNDLES_DELETED));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_CUSTODY_BUNDLES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_CUSTODY_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_FORWARD_BUNDLES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_FORWARD_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ABANDONED_BUNDLES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_DISCARDED_BUNDLES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_DISCARDED_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_NAMES));
	adm_add_rpttpl(def);
	/* ENDPOINT_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 1, g_dtn_bp_agent_idx[ADM_RPTT_IDX], DTN_BP_AGENT_RPTTPL_ENDPOINT_REPORT));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_ACTIVE, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_SINGLETON, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_POLICY, tnv_from_map(AMP_TYPE_STR, 0)));
	adm_add_rpttpl(def);
}

void dtn_bp_agent_init_tblt()
{

}

#endif // _HAVE_DTN_BP_AGENT_ADM_
