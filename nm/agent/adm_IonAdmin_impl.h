/*****************************************************************************
 **
 ** File Name: ./agent/adm_IonAdmin_impl.h
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  2017-11-11  AUTO           Auto generated header file 
 *****************************************************************************/

#ifndef ADM_IONADMIN_IMPL_H_
#define ADM_IONADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  *
 *           TODO                *
 *           ----                *
 *   STOP CUSTOM INCLUDES HERE   */

#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"


/******************
 * TODO: typeENUM *
 *****************/

void Name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/* Metadata Functions */
value_t adm_IonAdmin_meta_name(tdc_t params);
value_t adm_IonAdmin_meta_namespace(tdc_t params);

value_t adm_IonAdmin_meta_version(tdc_t params);

value_t adm_IonAdmin_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_IonAdmin_get_clockError(tdc_t params);
value_t adm_IonAdmin_get_clockSync(tdc_t params);
value_t adm_IonAdmin_get_congestionAlarmControl(tdc_t params);
value_t adm_IonAdmin_get_congestionEndTimeForecasts(tdc_t params);
value_t adm_IonAdmin_get_consumptionRate(tdc_t params);
value_t adm_IonAdmin_get_inboundFileSystemOccupancyLimit(tdc_t params);
value_t adm_IonAdmin_get_inboundHeapOccupancyLimit(tdc_t params);
value_t adm_IonAdmin_get_number(tdc_t params);
value_t adm_IonAdmin_get_outboundFileSystemOccupancyLimit(tdc_t params);
value_t adm_IonAdmin_get_outboundHeapOccupancyLimit(tdc_t params);
value_t adm_IonAdmin_get_productionRate(tdc_t params);
value_t adm_IonAdmin_get_refTime(tdc_t params);
value_t adm_IonAdmin_get_utcDelta(tdc_t params);
value_t adm_IonAdmin_get_version(tdc_t params);

/* Table Functions */

table_t *adm_IonAdmin_table_contacts();
table_t *adm_IonAdmin_table_usage();
table_t *adm_IonAdmin_table_ranges();

/* Control Functions */
tdc_t* adm_IonAdmin_ctrl_nodeInit(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeClockErrorSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeClockSyncSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeCongestionAlarmControlSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeCongestionEndTimeForecastsSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeConsumptionRateSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeContactAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeContactDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeInboundHeapOccupancyLimitSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeOutboundHeapOccupancyLimitSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeProductionRateSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeRangeAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeRangeDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeRefTimeSet(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonAdmin_ctrl_nodeUTCDeltaSet(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_IONADMIN_IMPL_H_
