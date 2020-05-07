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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_ION_IPN_ADMIN_IMPL_H_
#define ADM_ION_IPN_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
//#include "../shared/adm/adm_bp.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */
/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ipnadmin_setup();
void dtn_ion_ipnadmin_cleanup();


/* Metadata Functions */
tnv_t *dtn_ion_ipnadmin_meta_name(tnvc_t *parms);
tnv_t *dtn_ion_ipnadmin_meta_namespace(tnvc_t *parms);
tnv_t *dtn_ion_ipnadmin_meta_version(tnvc_t *parms);
tnv_t *dtn_ion_ipnadmin_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_ion_ipnadmin_get_ion_version(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_ion_ipnadmin_ctrl_exit_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ipnadmin_ctrl_exit_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ipnadmin_ctrl_exit_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ipnadmin_ctrl_plan_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ipnadmin_ctrl_plan_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ipnadmin_ctrl_plan_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_ion_ipnadmin_tblt_exits(ari_t *id);
tbl_t *dtn_ion_ipnadmin_tblt_plans(ari_t *id);

#endif //ADM_ION_IPN_ADMIN_IMPL_H_
