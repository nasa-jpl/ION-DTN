/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin_impl.h
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
 **  2018-02-07  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_ION_BP_ADMIN_IMPL_H_
#define ADM_ION_BP_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"

/*   START typeENUM */
/*   STOP typeENUM  */

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_bp_admin_setup();
void adm_ion_bp_admin_cleanup();

/* Metadata Functions */
value_t adm_ion_bp_admin_meta_name(tdc_t params);
value_t adm_ion_bp_admin_meta_namespace(tdc_t params);

value_t adm_ion_bp_admin_meta_version(tdc_t params);

value_t adm_ion_bp_admin_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_ion_bp_admin_get_version(tdc_t params);


/* Control Functions */
tdc_t* adm_ion_bp_admin_ctrl_endpoint_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_endpoint_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_endpoint_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_induct_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_induct_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_induct_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_induct_start(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_induct_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_init(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_manage_heap_max(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outduct_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outduct_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outduct_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outduct_start(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_egress_plan_block(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_egress_plan_unblock(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outduct_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocol_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocol_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocol_start(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocol_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_scheme_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_scheme_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_scheme_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_scheme_start(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_scheme_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_start(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_watch(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_ION_BP_ADMIN_IMPL_H_
