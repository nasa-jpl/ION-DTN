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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_ION_BP_ADMIN_IMPL_H_
#define ADM_ION_BP_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
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

void dtn_ion_bpadmin_setup();
void dtn_ion_bpadmin_cleanup();


/* Metadata Functions */
tnv_t *dtn_ion_bpadmin_meta_name(tnvc_t *parms);
tnv_t *dtn_ion_bpadmin_meta_namespace(tnvc_t *parms);
tnv_t *dtn_ion_bpadmin_meta_version(tnvc_t *parms);
tnv_t *dtn_ion_bpadmin_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_ion_bpadmin_get_bp_version(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_ion_bpadmin_ctrl_endpoint_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_endpoint_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_endpoint_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_induct_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_induct_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_induct_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_induct_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_induct_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_manage_heap_max(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_outduct_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_outduct_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_outduct_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_outduct_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_egress_plan_block(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_egress_plan_unblock(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_outduct_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_protocol_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_protocol_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_protocol_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_protocol_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_scheme_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_scheme_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_scheme_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_scheme_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_scheme_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_bpadmin_ctrl_watch(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_ion_bpadmin_tblt_endpoints(ari_t *id);
tbl_t *dtn_ion_bpadmin_tblt_inducts(ari_t *id);
tbl_t *dtn_ion_bpadmin_tblt_outducts(ari_t *id);
tbl_t *dtn_ion_bpadmin_tblt_protocols(ari_t *id);
tbl_t *dtn_ion_bpadmin_tblt_schemes(ari_t *id);
tbl_t *dtn_ion_bpadmin_tblt_egress_plans(ari_t *id);

#endif //ADM_ION_BP_ADMIN_IMPL_H_
