/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin_impl.h
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

#ifndef ADM_ION_IPN_ADMIN_IMPL_H_
#define ADM_ION_IPN_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/adm/adm_bp.h"
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

void adm_ion_ipn_admin_setup();
void adm_ion_ipn_admin_cleanup();

/* Metadata Functions */
value_t adm_ion_ipn_admin_meta_name(tdc_t params);
value_t adm_ion_ipn_admin_meta_namespace(tdc_t params);

value_t adm_ion_ipn_admin_meta_version(tdc_t params);

value_t adm_ion_ipn_admin_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_ion_ipn_admin_get_ion_version(tdc_t params);


/* Control Functions */
tdc_t* adm_ion_ipn_admin_ctrl_exit_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_exit_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_exit_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_exit_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_exit_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_exit_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_plan_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_plan_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_plan_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_plan_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_plan_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ipn_admin_ctrl_plan_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_ION_IPN_ADMIN_IMPL_H_
