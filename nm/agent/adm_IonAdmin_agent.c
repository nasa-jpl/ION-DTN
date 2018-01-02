/****************************************************************************
 **
 ** File Name: ./agent/adm_IonAdmin_agent.c
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
#include "adm_IonAdmin_impl.h"
#include "rda.h"

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
	adm_add_edd(ADM_IONADMIN_EDD_CLOCKERROR_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_clockError, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_CLOCKSYNC_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_clockSync, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_CONGESTIONALARMCONTROL_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_congestionAlarmControl, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_CONGESTIONENDTIMEFORECASTS_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_congestionEndTimeForecasts, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_CONSUMPTIONRATE_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_consumptionRate, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_INBOUNDFILESYSTEMOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_inboundFileSystemOccupancyLimit, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_INBOUNDHEAPOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_inboundHeapOccupancyLimit, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_NUMBER_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_number, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_OUTBOUNDFILESYSTEMOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_outboundFileSystemOccupancyLimit, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_OUTBOUNDHEAPOCCUPANCYLIMIT_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_outboundHeapOccupancyLimit, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_PRODUCTIONRATE_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_productionRate, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_REFTIME_MID, AMP_TYPE_TS, 0, adm_IonAdmin_get_refTime, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_UTCDELTA_MID, AMP_TYPE_UINT, 0, adm_IonAdmin_get_utcDelta, NULL, NULL);
	adm_add_edd(ADM_IONADMIN_EDD_VERSION_MID, AMP_TYPE_STR, 0, adm_IonAdmin_get_version, NULL, NULL);
}

void adm_IonAdmin_init_variables()
{
}

void adm_IonAdmin_init_controls()
{
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEINIT_MID, adm_IonAdmin_ctrl_nodeInit);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECLOCKERRORSET_MID, adm_IonAdmin_ctrl_nodeClockErrorSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECLOCKSYNCSET_MID, adm_IonAdmin_ctrl_nodeClockSyncSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONGESTIONALARMCONTROLSET_MID, adm_IonAdmin_ctrl_nodeCongestionAlarmControlSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONGESTIONENDTIMEFORECASTSSET_MID, adm_IonAdmin_ctrl_nodeCongestionEndTimeForecastsSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONSUMPTIONRATESET_MID, adm_IonAdmin_ctrl_nodeConsumptionRateSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONTACTADD_MID, adm_IonAdmin_ctrl_nodeContactAdd);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODECONTACTDEL_MID, adm_IonAdmin_ctrl_nodeContactDel);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEINBOUNDHEAPOCCUPANCYLIMITSET_MID, adm_IonAdmin_ctrl_nodeInboundHeapOccupancyLimitSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEOUTBOUNDHEAPOCCUPANCYLIMITSET_MID, adm_IonAdmin_ctrl_nodeOutboundHeapOccupancyLimitSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEPRODUCTIONRATESET_MID, adm_IonAdmin_ctrl_nodeProductionRateSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODERANGEADD_MID, adm_IonAdmin_ctrl_nodeRangeAdd);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODERANGEDEL_MID, adm_IonAdmin_ctrl_nodeRangeDel);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEREFTIMESET_MID, adm_IonAdmin_ctrl_nodeRefTimeSet);
	adm_add_ctrl(ADM_IONADMIN_CTRL_NODEUTCDELTASET_MID, adm_IonAdmin_ctrl_nodeUTCDeltaSet);
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
	oid_nn_add_parm(IONADMIN_ADM_ROOT_NN_IDX, IONADMIN_ADM_ROOT_NN_STR, "IONADMIN", "2017-08-17");
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_IONADMIN_META_NAME_MID, AMP_TYPE_STR, 0, adm_IonAdmin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(ADM_IONADMIN_META_NAMESPACE_MID, AMP_TYPE_STR, 0, adm_IonAdmin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(ADM_IONADMIN_META_VERSION_MID, AMP_TYPE_STR, 0, adm_IonAdmin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(ADM_IONADMIN_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, adm_IonAdmin_meta_organization, adm_print_string, adm_size_string);
}

void adm_IonAdmin_init_ops()
{
}

void adm_IonAdmin_init_reports()
{
	uint32_t used= 0;

}

#endif // _HAVE_IONADMIN_ADM_
