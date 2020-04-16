/****************************************************************************
 **
 ** File Name: adm_bp_agent_mgr.c
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
#include "metadata.h"
#include "nm_mgr_ui.h"




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

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "version", "The version of the ADM");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bp_agent_idx[ADM_META_IDX], DTN_BP_AGENT_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_bp_agent_init_cnst()
{

}

void dtn_bp_agent_init_edd()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BP_NODE_ID);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "bp_node_id", "The node administration endpoint");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BP_NODE_VERSION);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "bp_node_version", "The latest version of the BP supported by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_AVAILABLE_STORAGE);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_BP_AGENT, "available_storage", "Bytes available for bundle storage");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_LAST_RESET_TIME);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_BP_AGENT, "last_reset_time", "The last time that BP counters were reset, either due to execution of a reset control or a restart of the node itself");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_REGISTRATIONS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_registrations", "number of registrations");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_FWD);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_pend_fwd", "number of bundles pending forwarding");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_DIS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_pend_dis", "number of bundles awaiting dispatch");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_IN_CUST);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_in_cust", "number of bundles");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_PEND_REASSEMBLY);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_pend_reassembly", "number of bundles pending reassembly");

	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BUNDLES_BY_PRIORITY);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "bundles_by_priority", "number of bundles for the given priority. Priority is given as a priority mask where Bulk=0x1, normal=0x2, express=0x4. Any bundles matching any of the masked priorities will be included in the returned count");

	meta_add_parm(meta, "mask", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_BYTES_BY_PRIORITY);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "bytes_by_priority", "number of bytes of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0x2, express=0x4. Any bundles matching any of the masked priorities will be included in the returned count.");

	meta_add_parm(meta, "mask", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BUNDLES_BY_PRIORITY);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "src_bundles_by_priority", "number of bundles sourced by this node of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0x2, express=0x4. Any bundles sourced by this node and matching any of the masked priorities will be included in the returned count.");

	meta_add_parm(meta, "mask", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_SRC_BYTES_BY_PRIORITY);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "src_bytes_by_priority", "number of bytes sourced by this node of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0x2, express=0x4. Any bundles sourced by this node and matching any of the masked priorities will be included in the returned count");

	meta_add_parm(meta, "mask", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FRAGMENTED_BUNDLES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_fragmented_bundles", "number of fragmented bundles");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FRAGMENTS_PRODUCED);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_fragments_produced", "number of bundles with fragmentary payloads produced by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_failed_by_reason", "number of bundles failed for any of the given reasons. (noInfo=0x1, Expired=0x2, UniFwd=0x4, Cancelled=0x8, NoStorage=0x10, BadEID=0x20, NoRoute=0x40, NoContact=0x80, BadBlock=0x100)");

	meta_add_parm(meta, "mask", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_NUM_BUNDLES_DELETED);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "num_bundles_deleted", "number of bundles deleted by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_CUSTODY_BUNDLES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "failed_custody_bundles", "number of bundle fails at this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_CUSTODY_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "failed_custody_bytes", "number bytes of fails at this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_FORWARD_BUNDLES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "failed_forward_bundles", "number bundles not forwarded by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_FAILED_FORWARD_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "failed_forward_bytes", "number of bytes not forwaded by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ABANDONED_BUNDLES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "abandoned_bundles", "number of bundles abandoned by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ABANDONED_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "abandoned_bytes", "number of bytes abandoned by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_DISCARDED_BUNDLES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "discarded_bundles", "number of bundles discarded by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_DISCARDED_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "discarded_bytes", "number of bytes discarded by this node");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_NAMES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_BP_AGENT, "endpoint_names", "CSV list of endpoint names for this node");

	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_ACTIVE);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "endpoint_active", "is the given endpoint active? (0=no)");

	meta_add_parm(meta, "endpoint_name", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_SINGLETON);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "endpoint_singleton", "is the given endpoint singleton? (0=no)");

	meta_add_parm(meta, "endpoint_name", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_POLICY);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BP_AGENT, "endpoint_policy", "Does the endpoint abandon on fail (0=no)");

	meta_add_parm(meta, "endpoint_name", AMP_TYPE_STR);
}

void dtn_bp_agent_init_op()
{

}

void dtn_bp_agent_init_var()
{

}

void dtn_bp_agent_init_ctrl()
{

	ari_t *id = NULL;


	/* RESET_ALL_COUNTS */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_dtn_bp_agent_idx[ADM_CTRL_IDX], DTN_BP_AGENT_CTRL_RESET_ALL_COUNTS);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_DTN_BP_AGENT, "reset_all_counts", "This control causes the Agent to reset all counts associated with bundle or byte statistics and to set the last reset time of the BP primitive data to the time when the control was run");

}

void dtn_bp_agent_init_mac()
{

}

void dtn_bp_agent_init_rpttpl()
{

	metadata_t *meta = NULL;

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
	meta_add_rpttpl(def->id, ADM_ENUM_DTN_BP_AGENT, "full_report", "This is all known meta-data, EDD, and VAR values known by the agent.");
	/* ENDPOINT_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 1, g_dtn_bp_agent_idx[ADM_RPTT_IDX], DTN_BP_AGENT_RPTTPL_ENDPOINT_REPORT));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_ACTIVE, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_SINGLETON, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bp_agent_idx[ADM_EDD_IDX], DTN_BP_AGENT_EDD_ENDPOINT_POLICY, tnv_from_map(AMP_TYPE_STR, 0)));
	adm_add_rpttpl(def);
	meta = meta_add_rpttpl(def->id, ADM_ENUM_DTN_BP_AGENT, "endpoint_report", "This is all known endpoint information");
	meta_add_parm(meta, "endpoint_id", AMP_TYPE_STR);
}

void dtn_bp_agent_init_tblt()
{

}

#endif // _HAVE_DTN_BP_AGENT_ADM_
