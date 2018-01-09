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
 **  2018-01-08  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_ION_ADMIN_IMPL_H_
#define ADM_ION_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"
/*   START typeENUM */
/*             TODO              */
/*   STOP typeENUM  */

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_admin_setup();
void adm_ion_admin_cleanup();

/* Metadata Functions */
value_t adm_ion_admin_meta_name(tdc_t params);
value_t adm_ion_admin_meta_namespace(tdc_t params);

value_t adm_ion_admin_meta_version(tdc_t params);

value_t adm_ion_admin_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_ion_admin_get_clock_error(tdc_t params);
value_t adm_ion_admin_get_clock_sync(tdc_t params);
value_t adm_ion_admin_get_congestion_alarm_control(tdc_t params);
value_t adm_ion_admin_get_congestion_end_time_forecasts(tdc_t params);
value_t adm_ion_admin_get_consumption_rate(tdc_t params);
value_t adm_ion_admin_get_inbound_file_system_occupancy_limit(tdc_t params);
value_t adm_ion_admin_get_inbound_heap_occupancy_limit(tdc_t params);
value_t adm_ion_admin_get_number(tdc_t params);
value_t adm_ion_admin_get_outbound_file_system_occupancy_limit(tdc_t params);
value_t adm_ion_admin_get_outbound_heap_occupancy_limit(tdc_t params);
value_t adm_ion_admin_get_production_rate(tdc_t params);
value_t adm_ion_admin_get_ref_time(tdc_t params);
value_t adm_ion_admin_get_utc_delta(tdc_t params);
value_t adm_ion_admin_get_version(tdc_t params);


/* Control Functions */
tdc_t* adm_ion_admin_ctrl_node_init(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_clock_error_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_clock_sync_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_congestion_alarm_control_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_congestion_end_time_forecasts_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_consumption_rate_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_contact_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_contact_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_inbound_heap_occupancy_limit_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_outbound_heap_occupancy_limit_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_production_rate_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_range_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_range_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_ref_time_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_node_utc_delta_set(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_list_contacts(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_list_usage(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_admin_ctrl_list_ranges(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_ION_ADMIN_IMPL_H_
