/****************************************************************************
 **
 ** File Name: adm_bp_agent_impl.h
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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_BP_AGENT_IMPL_H_
#define ADM_BP_AGENT_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */
/*             TODO              */
/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_bp_agent_setup();
void dtn_bp_agent_cleanup();


/* Metadata Functions */
tnv_t *dtn_bp_agent_meta_name(tnvc_t *parms);
tnv_t *dtn_bp_agent_meta_namespace(tnvc_t *parms);
tnv_t *dtn_bp_agent_meta_version(tnvc_t *parms);
tnv_t *dtn_bp_agent_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_bp_agent_get_bp_node_id(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_bp_node_version(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_available_storage(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_last_reset_time(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_registrations(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_pend_fwd(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_pend_dis(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_in_cust(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_pend_reassembly(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_bundles_by_priority(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_bytes_by_priority(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_src_bundles_by_priority(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_src_bytes_by_priority(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_fragmented_bundles(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_fragments_produced(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_failed_by_reason(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_num_bundles_deleted(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_failed_custody_bundles(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_failed_custody_bytes(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_failed_forward_bundles(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_failed_forward_bytes(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_abandoned_bundles(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_abandoned_bytes(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_discarded_bundles(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_discarded_bytes(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_endpoint_names(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_endpoint_active(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_endpoint_singleton(tnvc_t *parms);
tnv_t *dtn_bp_agent_get_endpoint_policy(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_bp_agent_ctrl_reset_all_counts(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */

#endif //ADM_BP_AGENT_IMPL_H_
