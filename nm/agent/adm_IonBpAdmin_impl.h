/*****************************************************************************
 **
 ** File Name: ./agent/adm_IonBpAdmin_impl.h
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

#ifndef ADM_IONBPADMIN_IMPL_H_
#define ADM_IONBPADMIN_IMPL_H_

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

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/* Metadata Functions */
value_t adm_IonBpAdmin_meta_name(tdc_t params);
value_t adm_IonBpAdmin_meta_namespace(tdc_t params);

value_t adm_IonBpAdmin_meta_version(tdc_t params);

value_t adm_IonBpAdmin_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_IonBpAdmin_get_version(tdc_t params);


/* Table Functions */

table_t *adm_IonBpAdmin_table_endpoints();
table_t *adm_IonBpAdmin_table_inducts();
table_t *adm_IonBpAdmin_table_outducts();
table_t *adm_IonBpAdmin_table_protocols();
table_t *adm_IonBpAdmin_table_schemes();
table_t *adm_IonBpAdmin_table_egressPlans();

/* Control Functions */
tdc_t* adm_IonBpAdmin_ctrl_endpointAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_endpointChange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_endpointDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_inductAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_inductChange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_inductDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_inductStart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_inductStop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_manageHeapMax(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_outductAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_outductChange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_outductDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_outductStart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_outductStop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_protocolAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_protocolDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_protocolStart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_protocolStop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_schemeAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_schemeChange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_schemeDel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_schemeStart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_schemeStop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanAdd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanUpdate(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanDelete(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanStart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanStop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanBlock(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanUnblock(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanGateway(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanOutductAttach(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_IonBpAdmin_ctrl_egressPlanOutductDetatch(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_IONBPADMIN_IMPL_H_
