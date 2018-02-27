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
 **  2018-02-07  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_ION_LTP_ADMIN_IMPL_H_
#define ADM_ION_LTP_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"

/*   START typeENUM */
/*             TODO              */
/*   STOP typeENUM  */

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_ltp_admin_setup();
void adm_ion_ltp_admin_cleanup();

/* Metadata Functions */
value_t adm_ion_ltp_admin_meta_name(tdc_t params);
value_t adm_ion_ltp_admin_meta_namespace(tdc_t params);

value_t adm_ion_ltp_admin_meta_version(tdc_t params);

value_t adm_ion_ltp_admin_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_ion_ltp_admin_get_version(tdc_t params);


/* Control Functions */
tdc_t* adm_ion_ltp_admin_ctrl_init(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_manage_heap(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_manage_max_ber(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_manage_own_queue_time(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_manage_screening(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_span_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_span_change(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_span_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_watch_set(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_ION_LTP_ADMIN_IMPL_H_
