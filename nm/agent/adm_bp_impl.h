/****************************************************************************
 **
 ** File Name: adm_bp_impl.h
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
 **  2018-01-04  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_BP_IMPL_H_
#define ADM_BP_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/adm/adm_bp.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"
/******************
 * TODO: typeENUM *
 *****************/

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_bp_setup();
void adm_bp_cleanup();

/* Metadata Functions */
value_t adm_bp_meta_name(tdc_t params);
value_t adm_bp_meta_namespace(tdc_t params);

value_t adm_bp_meta_version(tdc_t params);

value_t adm_bp_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_bp_get_bp_node_id(tdc_t params);
value_t adm_bp_get_bp_node_version(tdc_t params);
value_t adm_bp_get_available_storage(tdc_t params);
value_t adm_bp_get_last_reset_time(tdc_t params);
value_t adm_bp_get_num_registrations(tdc_t params);
value_t adm_bp_get_num_pend_fwd(tdc_t params);
value_t adm_bp_get_num_pend_dis(tdc_t params);
value_t adm_bp_get_num_in_cust(tdc_t params);
value_t adm_bp_get_num_pend_reassembly(tdc_t params);
value_t adm_bp_get_bundles_by_priority(tdc_t params);
value_t adm_bp_get_bytes_by_priority(tdc_t params);
value_t adm_bp_get_src_bundles_by_priority(tdc_t params);
value_t adm_bp_get_src_bytes_by_priority(tdc_t params);
value_t adm_bp_get_num_fragmented_bundles(tdc_t params);
value_t adm_bp_get_num_fragments_produced(tdc_t params);
value_t adm_bp_get_num_failed_by_reason(tdc_t params);
value_t adm_bp_get_num_bundles_deleted(tdc_t params);
value_t adm_bp_get_failed_custody_bundles(tdc_t params);
value_t adm_bp_get_failed_custody_bytes(tdc_t params);
value_t adm_bp_get_failed_forward_bundles(tdc_t params);
value_t adm_bp_get_failed_forward_bytes(tdc_t params);
value_t adm_bp_get_abandoned_bundles(tdc_t params);
value_t adm_bp_get_abandoned_bytes(tdc_t params);
value_t adm_bp_get_discarded_bundles(tdc_t params);
value_t adm_bp_get_discarded_bytes(tdc_t params);
value_t adm_bp_get_endpoint_names(tdc_t params);
value_t adm_bp_get_endpoint_active(tdc_t params);
value_t adm_bp_get_endpoint_singleton(tdc_t params);
value_t adm_bp_get_endpoint_policy(tdc_t params);


/* Control Functions */
tdc_t* adm_bp_ctrl_reset_all_counts(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_BP_IMPL_H_
