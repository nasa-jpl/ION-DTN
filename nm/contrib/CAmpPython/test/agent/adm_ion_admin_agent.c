/****************************************************************************
 **
 ** File Name: adm_ion_admin_agent.c
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
 **  2018-02-07  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ion_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_ion_admin_impl.h"
#include "rda.h"

#define _HAVE_ION_ADMIN_ADM_
#ifdef _HAVE_ION_ADMIN_ADM_

void adm_ion_admin_init()
{

	adm_ion_admin_setup();
	adm_ion_admin_init_edd();
	adm_ion_admin_init_variables();
	adm_ion_admin_init_controls();
	adm_ion_admin_init_constants();
	adm_ion_admin_init_macros();
	adm_ion_admin_init_metadata();
	adm_ion_admin_init_ops();
	adm_ion_admin_init_reports();

}

void adm_ion_admin_init_edd()
{
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CLOCK_ERROR_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_clock_error, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CLOCK_SYNC_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_clock_sync, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CONGESTION_ALARM_CONTROL_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_congestion_alarm_control, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CONGESTION_END_TIME_FORECASTS_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_congestion_end_time_forecasts, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CONSUMPTION_RATE_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_consumption_rate, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_inbound_file_system_occupancy_limit, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_inbound_heap_occupancy_limit, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_NUMBER_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_number, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_outbound_file_system_occupancy_limit, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_outbound_heap_occupancy_limit, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_PRODUCTION_RATE_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_production_rate, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_REF_TIME_MID), AMP_TYPE_TS, 0, adm_ion_admin_get_ref_time, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_UTC_DELTA_MID), AMP_TYPE_UINT, 0, adm_ion_admin_get_utc_delta, NULL, NULL);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_admin_get_version, NULL, NULL);


}

void adm_ion_admin_init_variables()
{

	uint32_t used  = 0;
	mid_t *cur_mid = NULL;
	expr_t *expr   = NULL;
	Lyst def       = NULL;


}

void adm_ion_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_INIT_MID), adm_ion_admin_ctrl_node_init);
	UI_ADD_PARMSPEC_2(ADM_ION_ADMIN_CTRL_NODE_INIT_MID, "node_nbr", AMP_TYPE_UINT, "config_file", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CLOCK_ERROR_SET_MID), adm_ion_admin_ctrl_node_clock_error_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CLOCK_ERROR_SET_MID, "known_maximum_clock_error", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CLOCK_SYNC_SET_MID), adm_ion_admin_ctrl_node_clock_sync_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CLOCK_SYNC_SET_MID, "new_state", AMP_TYPE_BOOL);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET_MID), adm_ion_admin_ctrl_node_congestion_alarm_control_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET_MID, "congestion_alarm_control", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET_MID), adm_ion_admin_ctrl_node_congestion_end_time_forecasts_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET_MID, "end_time_for_congestion_forcasts", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONSUMPTION_RATE_SET_MID), adm_ion_admin_ctrl_node_consumption_rate_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CONSUMPTION_RATE_SET_MID, "planned_data_consumption_rate", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONTACT_ADD_MID), adm_ion_admin_ctrl_node_contact_add);
	UI_ADD_PARMSPEC_6(ADM_ION_ADMIN_CTRL_NODE_CONTACT_ADD_MID, "start", AMP_TYPE_TS, "stop", AMP_TYPE_TS, "node_id", AMP_TYPE_UINT, "dest", AMP_TYPE_STR, "data_rate", AMP_TYPE_FLOAT32, "prob", AMP_TYPE_FLOAT32);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONTACT_DEL_MID), adm_ion_admin_ctrl_node_contact_del);
	UI_ADD_PARMSPEC_3(ADM_ION_ADMIN_CTRL_NODE_CONTACT_DEL_MID, "start", AMP_TYPE_TS, "node_id", AMP_TYPE_UINT, "dest", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID), adm_ion_admin_ctrl_node_inbound_heap_occupancy_limit_set);
	UI_ADD_PARMSPEC_2(ADM_ION_ADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID, "heap_occupancy_limit", AMP_TYPE_UINT, "file_system_occupancy_limit", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID), adm_ion_admin_ctrl_node_outbound_heap_occupancy_limit_set);
	UI_ADD_PARMSPEC_2(ADM_ION_ADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID, "heap_occupancy_limit", AMP_TYPE_UINT, "file_system_occupancy_limit", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_PRODUCTION_RATE_SET_MID), adm_ion_admin_ctrl_node_production_rate_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_PRODUCTION_RATE_SET_MID, "planned_data_production_rate", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_RANGE_ADD_MID), adm_ion_admin_ctrl_node_range_add);
	UI_ADD_PARMSPEC_5(ADM_ION_ADMIN_CTRL_NODE_RANGE_ADD_MID, "start", AMP_TYPE_TS, "stop", AMP_TYPE_TS, "node", AMP_TYPE_UINT, "other_node", AMP_TYPE_UINT, "distance", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_RANGE_DEL_MID), adm_ion_admin_ctrl_node_range_del);
	UI_ADD_PARMSPEC_3(ADM_ION_ADMIN_CTRL_NODE_RANGE_DEL_MID, "start", AMP_TYPE_TS, "node", AMP_TYPE_UINT, "other_node", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_REF_TIME_SET_MID), adm_ion_admin_ctrl_node_ref_time_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_REF_TIME_SET_MID, "time", AMP_TYPE_TS);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_UTC_DELTA_SET_MID), adm_ion_admin_ctrl_node_utc_delta_set);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_UTC_DELTA_SET_MID, "local_time_sec_after_utc", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_LIST_CONTACTS_MID), adm_ion_admin_ctrl_list_contacts);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_LIST_USAGE_MID), adm_ion_admin_ctrl_list_usage);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_LIST_RANGES_MID), adm_ion_admin_ctrl_list_ranges);


}

void adm_ion_admin_init_constants()
{

}

void adm_ion_admin_init_macros()
{

}

void adm_ion_admin_init_ops()
{

}

void adm_ion_admin_init_metadata()
{

	/* Step 1: Register Nicknames */
	oid_nn_add_parm(ION_ADMIN_ADM_META_NN_IDX, ION_ADMIN_ADM_META_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_EDD_NN_IDX, ION_ADMIN_ADM_EDD_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_VAR_NN_IDX, ION_ADMIN_ADM_VAR_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_RPT_NN_IDX, ION_ADMIN_ADM_RPT_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_CTRL_NN_IDX, ION_ADMIN_ADM_CTRL_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_CONST_NN_IDX, ION_ADMIN_ADM_CONST_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_MACRO_NN_IDX, ION_ADMIN_ADM_MACRO_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_OP_NN_IDX, ION_ADMIN_ADM_OP_NN_STR, "ION_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_ADMIN_ADM_ROOT_NN_IDX, ION_ADMIN_ADM_ROOT_NN_STR, "ION_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, adm_ion_admin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_ion_admin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_admin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_ion_admin_meta_organization, adm_print_string, adm_size_string);

}

void adm_ion_admin_init_reports()
{

	Lyst rpt                = NULL;
	mid_t *cur_mid          = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used           = 0;
	
	
}

#endif // _HAVE_ION_ADMIN_ADM_
