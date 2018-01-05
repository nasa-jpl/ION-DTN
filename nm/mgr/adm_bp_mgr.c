/****************************************************************************
 **
 ** File Name: adm_bp_mgr.c
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
 **  2018-01-04  AUTO             Auto-generated c file 
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
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"

#define _HAVE_BP_ADM_
#ifdef _HAVE_BP_ADM_

void adm_bp_init()
{
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
	adm_add_edd(ADM_BP_EDD_BP_NODE_ID_MID, AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("BP_NODE_ID", "The node administration endpoint", ADM_BP, ADM_BP_EDD_BP_NODE_ID_MID);

	adm_add_edd(ADM_BP_EDD_BP_NODE_VERSION_MID, AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("BP_NODE_VERSION", "The latest version of the BP supported by this node", ADM_BP, ADM_BP_EDD_BP_NODE_VERSION_MID);

	adm_add_edd(ADM_BP_EDD_AVAILABLE_STORAGE_MID, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("AVAILABLE_STORAGE", "Bytes available for bundle storage", ADM_BP, ADM_BP_EDD_AVAILABLE_STORAGE_MID);

	adm_add_edd(ADM_BP_EDD_LAST_RESET_TIME_MID, AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("LAST_RESET_TIME", "The last time that BP counters were reset, either due to execution of a reset control or a restart of the node itself", ADM_BP, ADM_BP_EDD_LAST_RESET_TIME_MID);

	adm_add_edd(ADM_BP_EDD_NUM_REGISTRATIONS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_REGISTRATIONS", "number of registrations", ADM_BP, ADM_BP_EDD_NUM_REGISTRATIONS_MID);

	adm_add_edd(ADM_BP_EDD_NUM_PEND_FWD_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_PEND_FWD", "number of bundles pending forwarding", ADM_BP, ADM_BP_EDD_NUM_PEND_FWD_MID);

	adm_add_edd(ADM_BP_EDD_NUM_PEND_DIS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_PEND_DIS", "number of bundles awaiting dispatch", ADM_BP, ADM_BP_EDD_NUM_PEND_DIS_MID);

	adm_add_edd(ADM_BP_EDD_NUM_IN_CUST_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_IN_CUST", "number of bundles", ADM_BP, ADM_BP_EDD_NUM_IN_CUST_MID);

	adm_add_edd(ADM_BP_EDD_NUM_PEND_REASSEMBLY_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_PEND_REASSEMBLY", "number of bundles pending reassembly", ADM_BP, ADM_BP_EDD_NUM_PEND_REASSEMBLY_MID);

	adm_add_edd(ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("BUNDLES_BY_PRIORITY", "number of bundles for the given priority. Priority is given as a priority mask where Bulk=0x1, normal=0x2, express=0x4. Any bundles matching any of the masked priorities will be included in the returned count", ADM_BP, ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID);

	adm_add_edd(ADM_BP_EDD_BYTES_BY_PRIORITY_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("BYTES_BY_PRIORITY", "number of bytes of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0x2, express=0x4. Any bundles matching any of the masked priorities will be included in the returned count.", ADM_BP, ADM_BP_EDD_BYTES_BY_PRIORITY_MID);

	adm_add_edd(ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SRC_BUNDLES_BY_PRIORITY", "number of bundles sourced by this node of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0x2, express=0x4. Any bundles sourced by this node and matching any of the masked priorities will be included in the returned count.", ADM_BP, ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID);

	adm_add_edd(ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SRC_BYTES_BY_PRIORITY", "number of bytes sourced by this node of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0x2, express=0x4. Any bundles sourced by this node and matching any of the masked priorities will be included in the returned count", ADM_BP, ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID);

	adm_add_edd(ADM_BP_EDD_NUM_FRAGMENTED_BUNDLES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FRAGMENTED_BUNDLES", "number of fragmented bundles", ADM_BP, ADM_BP_EDD_NUM_FRAGMENTED_BUNDLES_MID);

	adm_add_edd(ADM_BP_EDD_NUM_FRAGMENTS_PRODUCED_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FRAGMENTS_PRODUCED", "number of bundles with fragmentary payloads produced by this node", ADM_BP, ADM_BP_EDD_NUM_FRAGMENTS_PRODUCED_MID);

	adm_add_edd(ADM_BP_EDD_NUM_FAILED_BY_REASON_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_FAILED_BY_REASON", "number of bundles failed for any of the given reasons. (noInfo=0x1, Expired=0x2, UniFwd=0x4, Cancelled=0x8, NoStorage=0x10, BadEID=0x20, NoRoute=0x40, NoContact=0x80, BadBlock=0x100)", ADM_BP, ADM_BP_EDD_NUM_FAILED_BY_REASON_MID);

	adm_add_edd(ADM_BP_EDD_NUM_BUNDLES_DELETED_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUM_BUNDLES_DELETED", "number of bundles deleted by this node", ADM_BP, ADM_BP_EDD_NUM_BUNDLES_DELETED_MID);

	adm_add_edd(ADM_BP_EDD_FAILED_CUSTODY_BUNDLES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("FAILED_CUSTODY_BUNDLES", "number of bundle fails at this node", ADM_BP, ADM_BP_EDD_FAILED_CUSTODY_BUNDLES_MID);

	adm_add_edd(ADM_BP_EDD_FAILED_CUSTODY_BYTES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("FAILED_CUSTODY_BYTES", "number bytes of fails at this node", ADM_BP, ADM_BP_EDD_FAILED_CUSTODY_BYTES_MID);

	adm_add_edd(ADM_BP_EDD_FAILED_FORWARD_BUNDLES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("FAILED_FORWARD_BUNDLES", "number bundles not forwarded by this node", ADM_BP, ADM_BP_EDD_FAILED_FORWARD_BUNDLES_MID);

	adm_add_edd(ADM_BP_EDD_FAILED_FORWARD_BYTES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("FAILED_FORWARD_BYTES", "number of bytes not forwaded by this node", ADM_BP, ADM_BP_EDD_FAILED_FORWARD_BYTES_MID);

	adm_add_edd(ADM_BP_EDD_ABANDONED_BUNDLES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ABANDONED_BUNDLES", "number of bundles abandoned by this node", ADM_BP, ADM_BP_EDD_ABANDONED_BUNDLES_MID);

	adm_add_edd(ADM_BP_EDD_ABANDONED_BYTES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ABANDONED_BYTES", "number of bytes abandoned by this node", ADM_BP, ADM_BP_EDD_ABANDONED_BYTES_MID);

	adm_add_edd(ADM_BP_EDD_DISCARDED_BUNDLES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("DISCARDED_BUNDLES", "number of bundles discarded by this node", ADM_BP, ADM_BP_EDD_DISCARDED_BUNDLES_MID);

	adm_add_edd(ADM_BP_EDD_DISCARDED_BYTES_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("DISCARDED_BYTES", "number of bytes discarded by this node", ADM_BP, ADM_BP_EDD_DISCARDED_BYTES_MID);

	adm_add_edd(ADM_BP_EDD_ENDPOINT_NAMES_MID, AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("ENDPOINT_NAMES", "CSV list of endpoint names for this node", ADM_BP, ADM_BP_EDD_ENDPOINT_NAMES_MID);

	adm_add_edd(ADM_BP_EDD_ENDPOINT_ACTIVE_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ENDPOINT_ACTIVE", "is the given endpoint active? (0=no)", ADM_BP, ADM_BP_EDD_ENDPOINT_ACTIVE_MID);

	adm_add_edd(ADM_BP_EDD_ENDPOINT_SINGLETON_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ENDPOINT_SINGLETON", "is the given endpoint singleton? (0=no)", ADM_BP, ADM_BP_EDD_ENDPOINT_SINGLETON_MID);

	adm_add_edd(ADM_BP_EDD_ENDPOINT_POLICY_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ENDPOINT_POLICY", "Does the endpoint abandon on fail (0=no)", ADM_BP, ADM_BP_EDD_ENDPOINT_POLICY_MID);

}


void adm_bp_init_variables()
{
}


void adm_bp_init_controls()
{
	adm_add_ctrl(ADM_BP_CTRL_RESET_ALL_COUNTS_MID, NULL);
	names_add_name("RESET_ALL_COUNTS", "This control causes the Agent to reset all counts associated with bundle or byte statistics and to set the last reset time of the BP primitive data to the time when the control was run", ADM_BP, ADM_BP_CTRL_RESET_ALL_COUNTS_MID);

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
	adm_add_edd(ADM_BP_META_NAME_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_BP, ADM_BP_META_NAME_MID);
	adm_add_edd(ADM_BP_META_NAMESPACE_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM.", ADM_BP, ADM_BP_META_NAMESPACE_MID);
	adm_add_edd(ADM_BP_META_VERSION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM", ADM_BP, ADM_BP_META_VERSION_MID);
	adm_add_edd(ADM_BP_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM.", ADM_BP, ADM_BP_META_ORGANIZATION_MID);
}


void adm_bp_init_ops()
{
}


void adm_bp_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = NULL;
	rpt = lyst_create();
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_META.LABEL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_META.VERSION_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_BP_NODE_ID_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_BP_NODE_VERSION_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_AVAILABLE_STORAGE_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_LAST_RESET_TIME_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_REGISTRATIONS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_PEND_FWD_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_PEND_DIS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_IN_CUST_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_PEND_REASSEMBLY_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_BUNDLES_BY_PRIORITY_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_BYTES_BY_PRIORITY_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_SRC_BUNDLES_BY_PRIORITY_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_SRC_BYTES_BY_PRIORITY_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_FRAGMENTED_BUNDLES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_FRAGMENTS_PRODUCED_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_FAILED_BY_REASON_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_NUM_BUNDLES_DELETED_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_FAILED_CUSTODY_BUNDLES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_FAILED_CUSTODY_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_FAILED_FORWARD_BUNDLES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_FAILED_FORWARD_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_ABANDONED_BUNDLES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_DISCARDED_BUNDLES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_DISCARDED_BYTES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_ENDPOINT_NAMES_MID, ADM_MID_ALLOC, &used));
	adm_add_rpt(ADM_BP_RPT_FULL_REPORT_MID, rpt);
	midcol_destroy(&rpt);

	rpt = lyst_create();
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_ENDPOINT_ACTIVE_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_ENDPOINT_SINGLETON_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_BP_ENDPOINT_POLICY_MID, ADM_MID_ALLOC, &used));
	adm_add_rpt(ADM_BP_RPT_ENDPOINT_REPORT_MID, rpt);
	midcol_destroy(&rpt);

}

#endif // _HAVE_BP_ADM_
