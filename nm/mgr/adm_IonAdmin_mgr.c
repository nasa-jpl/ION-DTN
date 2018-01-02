/****************************************************************************
 **
 ** File Name: ./mgr/adm_IonAdmin_mgr.c
 **
 ** Description: 
 **
 ** Notes: 
 **
 ** Assumptions: 
 **
 ** Modification History: 
 **  MM/DD/YY  AUTHOR           DESCRIPTION
 **  --------  --------------   ------------------------------------------------
 **  2017-11-11  AUTO           Auto generated c file 
 **
****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_IonAdmin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"
#define _HAVE_IONADMIN_ADM_
#ifdef _HAVE_IONADMIN_ADM_
void adm_IonAdmin_init()
{
	adm_IonAdmin_init_edd();
	adm_IonAdmin_init_variables();
	adm_IonAdmin_init_controls();
	adm_IonAdmin_init_constants();
	adm_IonAdmin_init_macros();
	adm_IonAdmin_init_metadata();
	adm_IonAdmin_init_ops();
	adm_IonAdmin_init_reports();
}

void adm_IonAdmin_init_edd()
{
	adm_add_edd(ADM_IONADMIN_EDD_CLOCKERROR_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_CLOCKERROR_MID", "This is how accurate the ION Agent's clock is.", ADM_IONADMIN, ADM_IONADMIN_EDD_CLOCKERROR_MID);

	adm_add_edd(ADM_IONADMIN_EDD_CLOCKSYNC_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_CLOCKSYNC_MID", "This is whether or not the the computer on which the local ION node is running has a synchronized clock.", ADM_IONADMIN, ADM_IONADMIN_EDD_CLOCKSYNC_MID);

	adm_add_edd(ADM_IONADMIN_EDD_CONGESTIONALARMCONTROL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_CONGESTIONALARMCONTROL_MID", "This is whether or not the node has a control that will set off alarm if it will become congested at some future time.", ADM_IONADMIN, ADM_IONADMIN_EDD_CONGESTIONALARMCONTROL_MID);

	adm_add_edd(ADM_IONADMIN_EDD_CONGESTIONENDTIMEFORECASTS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_CONGESTIONENDTIMEFORECASTS_MID", "This is when the node will be predicted to be no longer congested.", ADM_IONADMIN, ADM_IONADMIN_EDD_CONGESTIONENDTIMEFORECASTS_MID);

	adm_add_edd(ADM_IONADMIN_EDD_CONSUMPTIONRATE_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_CONSUMPTIONRATE_MID", "This is the mean rate of continuous data delivery to local BP applications.", ADM_IONADMIN, ADM_IONADMIN_EDD_CONSUMPTIONRATE_MID);

	adm_add_edd(ADM_IONADMIN_EDD_INBOUNDFILESYSTEMOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_INBOUNDFILESYSTEMOCCUPANCYLIMIT_MID", "This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of inbound zero-copy objects. The default heap limit is 1 Terabyte.", ADM_IONADMIN, ADM_IONADMIN_EDD_INBOUNDFILESYSTEMOCCUPANCYLIMIT_MID);

	adm_add_edd(ADM_IONADMIN_EDD_INBOUNDHEAPOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_INBOUNDHEAPOCCUPANCYLIMIT_MID", "This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. The default heap limit is 30% of the SDR data space's total heap size.", ADM_IONADMIN, ADM_IONADMIN_EDD_INBOUNDHEAPOCCUPANCYLIMIT_MID);

	adm_add_edd(ADM_IONADMIN_EDD_NUMBER_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_NUMBER_MID", "This is a CBHE node number which uniquely identifies the node in the delay-tolerant network.", ADM_IONADMIN, ADM_IONADMIN_EDD_NUMBER_MID);

	adm_add_edd(ADM_IONADMIN_EDD_OUTBOUNDFILESYSTEMOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_OUTBOUNDFILESYSTEMOCCUPANCYLIMIT_MID", "This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of outbound zero-copy objects. The default heap limit is 1 Terabyte.", ADM_IONADMIN, ADM_IONADMIN_EDD_OUTBOUNDFILESYSTEMOCCUPANCYLIMIT_MID);

	adm_add_edd(ADM_IONADMIN_EDD_OUTBOUNDHEAPOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_OUTBOUNDHEAPOCCUPANCYLIMIT_MID", "This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects. The default heap limit is 30% of the SDR data space's total heap size.", ADM_IONADMIN, ADM_IONADMIN_EDD_OUTBOUNDHEAPOCCUPANCYLIMIT_MID);

	adm_add_edd(ADM_IONADMIN_EDD_PRODUCTIONRATE_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_PRODUCTIONRATE_MID", "This is the rate of local data production.", ADM_IONADMIN, ADM_IONADMIN_EDD_PRODUCTIONRATE_MID);

	adm_add_edd(ADM_IONADMIN_EDD_REFTIME_MID, AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_REFTIME_MID", "This is the reference time that will be used for interpreting relative time values from now until the next revision of reference time.", ADM_IONADMIN, ADM_IONADMIN_EDD_REFTIME_MID);

	adm_add_edd(ADM_IONADMIN_EDD_UTCDELTA_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_UTCDELTA_MID", "The UTC delta is used to compensate for error (drift) in clocks, particularly spacecraft clocks. The hardware clock on a spacecraft might gain or lose a few seconds every month, to the point at which its understanding of the current time - as reported out by the operating system - might differ significantly from the actual value of UTC as reported by authoritative clocks on Earth. To compensate for this difference without correcting the clock itself (which can be difficult and dangerous), ION simply adds the UTC delta to the UTC reported by the operating system.", ADM_IONADMIN, ADM_IONADMIN_EDD_UTCDELTA_MID);

	adm_add_edd(ADM_IONADMIN_EDD_VERSION_MID, AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONADMIN_EDD_VERSION_MID", "This is the version of ION that is currently installed.", ADM_IONADMIN, ADM_IONADMIN_EDD_VERSION_MID);

}

void adm_IonAdmin_init_variables()
{
}

void adm_IonAdmin_init_controls()
{
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEINIT_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODEINIT_MID", "Until this control is executed, the local ION node does not exist and most ionadmin controls will fail. The control configures the local node to be identified by nodeNumber, a CBHE node number which uniquely identifies the node in the delay-tolerant network.  It also configures ION's data space (SDR) and shared working-memory region.  For this purpose it uses a set of default settings if no argument follows nodeNumber or if the argument following nodeNumber is ''; otherwise it uses the configuration settings found in a configuration file.  If configuration file name '.' is provided, then the configuration file's name is implicitly 'hostname.ionconfig'; otherwise, ionConfigFileName is taken to be the explicit configuration file name.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODEINIT_MID);
	UI_ADD_PARMSPEC_2(ADM_IONADMIN_CTRL_NODEINIT_MID, "nodeNbr", AMP_TYPE_UINT, "configFile", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECLOCKERRORSET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECLOCKERRORSET_MID", "This management control sets ION's understanding of the accuracy of the scheduled start and stop times of planned contacts, in seconds.  The default value is 1.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECLOCKERRORSET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODECLOCKERRORSET_MID, "knownMaximumClockError", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECLOCKSYNCSET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECLOCKSYNCSET_MID", "This management control reports whether or not the computer on which the local ION node is running has a synchronized clock.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECLOCKSYNCSET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODECLOCKSYNCSET_MID, "newState", AMP_TYPE_BOOL);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONGESTIONALARMCONTROLSET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECONGESTIONALARMCONTROLSET_MID", "This management control establishes a control which will automatically be executed whenever ionadmin predicts that the node will become congested at some future time.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECONGESTIONALARMCONTROLSET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODECONGESTIONALARMCONTROLSET_MID, "congestionAlarmControl", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONGESTIONENDTIMEFORECASTSSET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECONGESTIONENDTIMEFORECASTSSET_MID", "This management control sets the end time for computed congestion forecasts. Setting congestion forecast horizon to zero sets the congestion forecast end time to infinite time in the future: if there is any predicted net growth in bundle storage space occupancy at all, following the end of the last scheduled contact, then eventual congestion will be predicted. The default value is zero, i.e., no end time.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECONGESTIONENDTIMEFORECASTSSET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODECONGESTIONENDTIMEFORECASTSSET_MID, "endTimeForCongestionForcasts", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONSUMPTIONRATESET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECONSUMPTIONRATESET_MID", "This management control sets ION's expectation of the mean rate of continuous data delivery to local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data consumption is unknown; in that case local data consumption is not considered in the computation of congestion forecasts.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECONSUMPTIONRATESET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODECONSUMPTIONRATESET_MID, "plannedDataConsumptionRate", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONTACTADD_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECONTACTADD_MID", "This control schedules a period of data transmission from sourceNode to destNode. The period of transmission will begin at startTime and end at stopTime, and the rate of data transmission will be xmitDataRate bytes/second. Our confidence in the contact defaults to 1.0, indicating that the contact is scheduled - not that non-occurrence of the contact is impossible, just that occurrence of the contact is planned and scheduled rather than merely imputed from past node behavior. In the latter case, confidence indicates our estimation of the likelihood of this potential contact.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECONTACTADD_MID);
	UI_ADD_PARMSPEC_6(ADM_IONADMIN_CTRL_NODECONTACTADD_MID, "start", AMP_TYPE_TS, "stop", AMP_TYPE_TS, "nodeId", AMP_TYPE_UINT, "dest", AMP_TYPE_STR, "dataRate", AMP_TYPE_FLOAT32, "prob", AMP_TYPE_FLOAT32);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONTACTDEL_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODECONTACTDEL_MID", "This control deletes the scheduled period of data transmission from sourceNode to destNode starting at startTime. To delete all contacts between some pair of nodes, use '*' as startTime.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODECONTACTDEL_MID);
	UI_ADD_PARMSPEC_3(ADM_IONADMIN_CTRL_NODECONTACTDEL_MID, "start", AMP_TYPE_TS, "nodeId", AMP_TYPE_UINT, "dest", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEINBOUNDHEAPOCCUPANCYLIMITSET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODEINBOUNDHEAPOCCUPANCYLIMITSET_MID", "This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODEINBOUNDHEAPOCCUPANCYLIMITSET_MID);
	UI_ADD_PARMSPEC_2(ADM_IONADMIN_CTRL_NODEINBOUNDHEAPOCCUPANCYLIMITSET_MID, "heapOccupancyLimit", AMP_TYPE_UINT, "fileSystemOccupancyLimit", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEOUTBOUNDHEAPOCCUPANCYLIMITSET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODEOUTBOUNDHEAPOCCUPANCYLIMITSET_MID", "This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects.  A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODEOUTBOUNDHEAPOCCUPANCYLIMITSET_MID);
	UI_ADD_PARMSPEC_2(ADM_IONADMIN_CTRL_NODEOUTBOUNDHEAPOCCUPANCYLIMITSET_MID, "heapOccupancyLimit", AMP_TYPE_UINT, "fileSystemOccupancyLimit", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEPRODUCTIONRATESET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODEPRODUCTIONRATESET_MID", "This management control sets ION's expectation of the mean rate of continuous data origination by local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data production is unknown; in that case local data production is not considered in the computation of congestion forecasts.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODEPRODUCTIONRATESET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODEPRODUCTIONRATESET_MID, "plannedDataProductionRate", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODERANGEADD_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODERANGEADD_MID", "This control predicts a period of time during which the distance from node to otherNode will be constant to within one light second. The period will begin at startTime and end at stopTime, and the distance between the nodes during that time will be distance light seconds.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODERANGEADD_MID);
	UI_ADD_PARMSPEC_5(ADM_IONADMIN_CTRL_NODERANGEADD_MID, "start", AMP_TYPE_TS, "stop", AMP_TYPE_TS, "node", AMP_TYPE_UINT, "otherNode", AMP_TYPE_UINT, "distance", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODERANGEDEL_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODERANGEDEL_MID", "This control deletes the predicted period of constant distance between node and otherNode starting at startTime. To delete all ranges between some pair of nodes, use '*' as startTime.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODERANGEDEL_MID);
	UI_ADD_PARMSPEC_3(ADM_IONADMIN_CTRL_NODERANGEDEL_MID, "start", AMP_TYPE_TS, "node", AMP_TYPE_UINT, "otherNode", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEREFTIMESET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODEREFTIMESET_MID", "This is used to set the reference time that will be used for interpreting relative time values from now until the next revision of reference time. sNote that the new reference time can be a relative time, i.e., an offset beyond the current reference time.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODEREFTIMESET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODEREFTIMESET_MID, "time", AMP_TYPE_TS);

	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEUTCDELTASET_MID, NULL);
	names_add_name("ADM_IONADMIN_CTRL_NODEUTCDELTASET_MID", "This management control sets ION's understanding of the current difference between correct UTC time and the time values reported by the clock for the local ION node's computer. This delta is automatically applied to locally obtained time values whenever ION needs to know the current time.", ADM_IONADMIN, ADM_IONADMIN_CTRL_NODEUTCDELTASET_MID);
	UI_ADD_PARMSPEC_1(ADM_IONADMIN_CTRL_NODEUTCDELTASET_MID, "localTimeSecAfterUTC", AMP_TYPE_UINT);


}

void adm_IonAdmin_init_constants()
{

}

void adm_IonAdmin_init_macros()
{

}

void adm_IonAdmin_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(IONADMIN_ADM_META_NN_IDX, IONADMIN_ADM_META_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_EDD_NN_IDX, IONADMIN_ADM_EDD_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_VAR_NN_IDX, IONADMIN_ADM_VAR_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_RPT_NN_IDX, IONADMIN_ADM_RPT_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_CTRL_NN_IDX, IONADMIN_ADM_CTRL_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_CONST_NN_IDX, IONADMIN_ADM_CONST_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_MACRO_NN_IDX, IONADMIN_ADM_MACRO_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_OP_NN_IDX, IONADMIN_ADM_OP_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_TBL_NN_IDX, IONADMIN_ADM_TBL_NN_STR, "IONADMIN", "2017-08-17");
	oid_nn_add_parm(IONADMIN_ADM_ROOT_NN_IDX, IONADMIN_ADM_ROOT_NN_STR, "IONADMIN", "2017-08-17");
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_IONADMIN_META_NAME_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONADMIN_META_NAME_MID", "The human-readable name of the ADM.", ADM_IONADMIN, ADM_IONADMIN_META_NAME_MID);
	adm_add_edd(ADM_IONADMIN_META_NAMESPACE_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONADMIN_META_NAMESPACE_MID", "The namespace of the ADM.", ADM_IONADMIN, ADM_IONADMIN_META_NAMESPACE_MID);
	adm_add_edd(ADM_IONADMIN_META_VERSION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONADMIN_META_VERSION_MID", "The version of the ADM.", ADM_IONADMIN, ADM_IONADMIN_META_VERSION_MID);
	adm_add_edd(ADM_IONADMIN_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONADMIN_META_ORGANIZATION_MID", "The name of the issuing organization of the ADM.", ADM_IONADMIN, ADM_IONADMIN_META_ORGANIZATION_MID);

}

void adm_IonAdmin_init_ops()
{

}

void adm_IonAdmin_init_reports()
{
	uint32_t used= 0;
}

#endif // _HAVE_IONADMIN_ADM_
