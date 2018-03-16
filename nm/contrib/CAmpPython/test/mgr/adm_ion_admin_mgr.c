/****************************************************************************
 **
 ** File Name: adm_ion_admin_mgr.c
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
 **  2018-03-16  AUTO             Auto-generated c file 
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
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"


#define _HAVE_ION_ADMIN_ADM_
#ifdef _HAVE_ION_ADMIN_ADM_
void adm_ion_admin_init()
{

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
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CLOCK_ERROR_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("CLOCK_ERROR", "This is how accurate the ION Agent's clock is.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_CLOCK_ERROR_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CLOCK_SYNC_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("CLOCK_SYNC", "This is whether or not the the computer on which the local ION node is running has a synchronized clock.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_CLOCK_SYNC_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CONGESTION_ALARM_CONTROL_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("CONGESTION_ALARM_CONTROL", "This is whether or not the node has a control that will set off alarm if it will become congested at some future time.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_CONGESTION_ALARM_CONTROL_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CONGESTION_END_TIME_FORECASTS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("CONGESTION_END_TIME_FORECASTS", "This is when the node will be predicted to be no longer congested.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_CONGESTION_END_TIME_FORECASTS_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_CONSUMPTION_RATE_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("CONSUMPTION_RATE", "This is the mean rate of continuous data delivery to local BP applications.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_CONSUMPTION_RATE_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT", "This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of inbound zero-copy objects. The default heap limit is 1 Terabyte.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("INBOUND_HEAP_OCCUPANCY_LIMIT", "This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. The default heap limit is 30% of the SDR data space's total heap size.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_NUMBER_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("NUMBER", "This is a CBHE node number which uniquely identifies the node in the delay-tolerant network.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_NUMBER_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT", "This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of outbound zero-copy objects. The default heap limit is 1 Terabyte.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("OUTBOUND_HEAP_OCCUPANCY_LIMIT", "This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects. The default heap limit is 30% of the SDR data space's total heap size.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_PRODUCTION_RATE_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("PRODUCTION_RATE", "This is the rate of local data production.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_PRODUCTION_RATE_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_REF_TIME_MID), AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("REF_TIME", "This is the reference time that will be used for interpreting relative time values from now until the next revision of reference time.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_REF_TIME_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_UTC_DELTA_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("UTC_DELTA", "The UTC delta is used to compensate for error (drift) in clocks, particularly spacecraft clocks. The hardware clock on a spacecraft might gain or lose a few seconds every month, to the point at which its understanding of the current time - as reported out by the operating system - might differ significantly from the actual value of UTC as reported by authoritative clocks on Earth. To compensate for this difference without correcting the clock itself (which can be difficult and dangerous), ION simply adds the UTC delta to the UTC reported by the operating system.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_UTC_DELTA_MID);

	adm_add_edd(mid_from_value(ADM_ION_ADMIN_EDD_VERSION_MID), AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("VERSION", "This is the version of ION that is currently installed.", ADM_ION_ADMIN, ADM_ION_ADMIN_EDD_VERSION_MID);


}

void adm_ion_admin_init_variables()
{

}

void adm_ion_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_INIT_MID), NULL);
	names_add_name("NODE_INIT", "Until this control is executed, the local ION node does not exist and most ionadmin controls will fail. The control configures the local node to be identified by node_number, a CBHE node number which uniquely identifies the node in the delay-tolerant network.  It also configures ION's data space (SDR) and shared working-memory region.  For this purpose it uses a set of default settings if no argument follows node_number or if the argument following node_number is ''; otherwise it uses the configuration settings found in a configuration file.  If configuration file name is provided, then the configuration file's name is implicitly 'hostname.ionconfig'; otherwise, ion_config_filename is taken to be the explicit configuration file name.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_INIT_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_ADMIN_CTRL_NODE_INIT_MID, "node_nbr", AMP_TYPE_UINT, "config_file", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CLOCK_ERROR_SET_MID), NULL);
	names_add_name("NODE_CLOCK_ERROR_SET", "This management control sets ION's understanding of the accuracy of the scheduled start and stop times of planned contacts, in seconds.  The default value is 1.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CLOCK_ERROR_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CLOCK_ERROR_SET_MID, "known_maximum_clock_error", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CLOCK_SYNC_SET_MID), NULL);
	names_add_name("NODE_CLOCK_SYNC_SET", "This management control reports whether or not the computer on which the local ION node is running has a synchronized clock.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CLOCK_SYNC_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CLOCK_SYNC_SET_MID, "new_state", AMP_TYPE_BOOL);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET_MID), NULL);
	names_add_name("NODE_CONGESTION_ALARM_CONTROL_SET", "This management control establishes a control which will automatically be executed whenever ionadmin predicts that the node will become congested at some future time.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET_MID, "congestion_alarm_control", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET_MID), NULL);
	names_add_name("NODE_CONGESTION_END_TIME_FORECASTS_SET", "This management control sets the end time for computed congestion forecasts. Setting congestion forecast horizon to zero sets the congestion forecast end time to infinite time in the future: if there is any predicted net growth in bundle storage space occupancy at all, following the end of the last scheduled contact, then eventual congestion will be predicted. The default value is zero, i.e., no end time.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET_MID, "end_time_for_congestion_forcasts", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONSUMPTION_RATE_SET_MID), NULL);
	names_add_name("NODE_CONSUMPTION_RATE_SET", "This management control sets ION's expectation of the mean rate of continuous data delivery to local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data consumption is unknown; in that case local data consumption is not considered in the computation of congestion forecasts.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CONSUMPTION_RATE_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_CONSUMPTION_RATE_SET_MID, "planned_data_consumption_rate", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONTACT_ADD_MID), NULL);
	names_add_name("NODE_CONTACT_ADD", "This control schedules a period of data transmission from source_node to dest_node. The period of transmission will begin at start_time and end at stop_time, and the rate of data transmission will be xmit_data_rate bytes/second. Our confidence in the contact defaults to 1.0, indicating that the contact is scheduled - not that non-occurrence of the contact is impossible, just that occurrence of the contact is planned and scheduled rather than merely imputed from past node behavior. In the latter case, confidence indicates our estimation of the likelihood of this potential contact.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CONTACT_ADD_MID);
	UI_ADD_PARMSPEC_6(ADM_ION_ADMIN_CTRL_NODE_CONTACT_ADD_MID, "start", AMP_TYPE_TS, "stop", AMP_TYPE_TS, "node_id", AMP_TYPE_UINT, "dest", AMP_TYPE_STR, "data_rate", AMP_TYPE_FLOAT32, "prob", AMP_TYPE_FLOAT32);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_CONTACT_DEL_MID), NULL);
	names_add_name("NODE_CONTACT_DEL", "This control deletes the scheduled period of data transmission from source_node to dest_node starting at start_time. To delete all contacts between some pair of nodes, use '*' as start_time.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_CONTACT_DEL_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_ADMIN_CTRL_NODE_CONTACT_DEL_MID, "start", AMP_TYPE_TS, "node_id", AMP_TYPE_UINT, "dest", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID), NULL);
	names_add_name("NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET", "This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_ADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID, "heap_occupancy_limit", AMP_TYPE_UINT, "file_system_occupancy_limit", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID), NULL);
	names_add_name("NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET", "This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects.  A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_ADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID, "heap_occupancy_limit", AMP_TYPE_UINT, "file_system_occupancy_limit", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_PRODUCTION_RATE_SET_MID), NULL);
	names_add_name("NODE_PRODUCTION_RATE_SET", "This management control sets ION's expectation of the mean rate of continuous data origination by local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data production is unknown; in that case local data production is not considered in the computation of congestion forecasts.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_PRODUCTION_RATE_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_PRODUCTION_RATE_SET_MID, "planned_data_production_rate", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_RANGE_ADD_MID), NULL);
	names_add_name("NODE_RANGE_ADD", "This control predicts a period of time during which the distance from node to other_node will be constant to within one light second. The period will begin at start_time and end at stop_time, and the distance between the nodes during that time will be distance light seconds.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_RANGE_ADD_MID);
	UI_ADD_PARMSPEC_5(ADM_ION_ADMIN_CTRL_NODE_RANGE_ADD_MID, "start", AMP_TYPE_TS, "stop", AMP_TYPE_TS, "node", AMP_TYPE_UINT, "other_node", AMP_TYPE_UINT, "distance", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_RANGE_DEL_MID), NULL);
	names_add_name("NODE_RANGE_DEL", "This control deletes the predicted period of constant distance between node and other_node starting at start_time. To delete all ranges between some pair of nodes, use '*' as start_time.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_RANGE_DEL_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_ADMIN_CTRL_NODE_RANGE_DEL_MID, "start", AMP_TYPE_TS, "node", AMP_TYPE_UINT, "other_node", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_REF_TIME_SET_MID), NULL);
	names_add_name("NODE_REF_TIME_SET", "This is used to set the reference time that will be used for interpreting relative time values from now until the next revision of reference time. Note that the new reference time can be a relative time, i.e., an offset beyond the current reference time.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_REF_TIME_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_REF_TIME_SET_MID, "time", AMP_TYPE_TS);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_NODE_UTC_DELTA_SET_MID), NULL);
	names_add_name("NODE_UTC_DELTA_SET", "This management control sets ION's understanding of the current difference between correct UTC time and the time values reported by the clock for the local ION node's computer. This delta is automatically applied to locally obtained time values whenever ION needs to know the current time.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_NODE_UTC_DELTA_SET_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_ADMIN_CTRL_NODE_UTC_DELTA_SET_MID, "local_time_sec_after_utc", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_LIST_CONTACTS_MID), NULL);
	names_add_name("LIST_CONTACTS", "Lists all schedule periods of data transmission.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_LIST_CONTACTS_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_LIST_USAGE_MID), NULL);
	names_add_name("LIST_USAGE", "Lists ION's current data space occupancy.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_LIST_USAGE_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_ADMIN_CTRL_LIST_RANGES_MID), NULL);
	names_add_name("LIST_RANGES", "Lists all predefined periods of constant distance between nodes.", ADM_ION_ADMIN, ADM_ION_ADMIN_CTRL_LIST_RANGES_MID);


}

void adm_ion_admin_init_constants()
{

}

void adm_ion_admin_init_macros()
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
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_ION_ADMIN, ADM_ION_ADMIN_META_NAME_MID);
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM.", ADM_ION_ADMIN, ADM_ION_ADMIN_META_NAMESPACE_MID);
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM.", ADM_ION_ADMIN, ADM_ION_ADMIN_META_VERSION_MID);
	adm_add_edd(mid_from_value(ADM_ION_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM.", ADM_ION_ADMIN, ADM_ION_ADMIN_META_ORGANIZATION_MID);

}

void adm_ion_admin_init_ops()
{

}

void adm_ion_admin_init_reports()
{

	Lyst rpt                = NULL;
	mid_t *cur_mid          = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used           = 0;
	
	
}

#endif // _HAVE_ION_ADMIN_ADM_
