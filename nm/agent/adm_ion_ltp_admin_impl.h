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
 **  2018-01-06  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_ION_LTP_ADMIN_IMPL_H_
#define ADM_ION_LTP_ADMIN_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
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
/*             TODO              */
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
tdc_t* adm_ion_ltp_admin_ctrl_manageheap(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_managemaxber(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_manageownqueuetime(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_managescreening(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_spanadd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_spanchange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_spandel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_ltp_admin_ctrl_watchset(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_ION_LTP_ADMIN_IMPL_H_
