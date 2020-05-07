/****************************************************************************
 **
 ** File Name: adm_ion_ltp_admin_impl.h
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

#ifndef ADM_ION_LTP_ADMIN_IMPL_H_
#define ADM_ION_LTP_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */
/*             TODO              */
/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ltpadmin_setup();
void dtn_ion_ltpadmin_cleanup();


/* Metadata Functions */
tnv_t *dtn_ion_ltpadmin_meta_name(tnvc_t *parms);
tnv_t *dtn_ion_ltpadmin_meta_namespace(tnvc_t *parms);
tnv_t *dtn_ion_ltpadmin_meta_version(tnvc_t *parms);
tnv_t *dtn_ion_ltpadmin_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_ion_ltpadmin_get_ion_version(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_ion_ltpadmin_ctrl_manage_heap(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_manage_max_ber(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_manage_own_queue_time(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_manage_screening(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_span_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_span_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_span_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_ion_ltpadmin_ctrl_watch_set(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_ion_ltpadmin_tblt_spans(ari_t *id);

#endif //ADM_ION_LTP_ADMIN_IMPL_H_
