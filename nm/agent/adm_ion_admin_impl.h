/****************************************************************************
 **
 ** File Name: adm_ion_admin_impl.h
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

#ifndef ADM_ION_ADMIN_IMPL_H_
#define ADM_ION_ADMIN_IMPL_H_

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

void dtn_ion_ionadmin_setup();
void dtn_ion_ionadmin_cleanup();


/* Metadata Functions */
tnv_t *dtn_ion_ionadmin_meta_name(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_meta_namespace(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_meta_version(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_ion_ionadmin_get_clock_error(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_clock_sync(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_congestion_alarm_control(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_congestion_end_time_forecasts(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_consumption_rate(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_inbound_file_system_occupancy_limit(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_inbound_heap_occupancy_limit(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_number(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_outbound_file_system_occupancy_limit(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_outbound_heap_occupancy_limit(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_production_rate(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_ref_time(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_time_delta(tnvc_t *parms);
tnv_t *dtn_ion_ionadmin_get_version(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_ion_ionadmin_ctrl_node_init(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_clock_error_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_clock_sync_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_congestion_alarm_control_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_congestion_end_time_forecasts_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_consumption_rate_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_contact_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_contact_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_inbound_heap_occupancy_limit_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_outbound_heap_occupancy_limit_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_production_rate_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_range_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_range_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_ref_time_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ionadmin_ctrl_node_time_delta_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_ion_ionadmin_tblt_contacts(ari_t *id);
tbl_t *dtn_ion_ionadmin_tblt_ranges(ari_t *id);

#endif //ADM_ION_ADMIN_IMPL_H_
