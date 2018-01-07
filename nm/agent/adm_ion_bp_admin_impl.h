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
 **  2018-01-06  AUTO             Auto-generated header file 
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
tdc_t* adm_ion_bp_admin_ctrl_endpointadd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_endpointchange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_endpointdel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_inductadd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_inductchange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_inductdel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_inductstart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_inductstop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_init(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_manageheapmax(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductadd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductchange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductdel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductstart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductblock(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductunblock(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_outductstop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocoladd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocoldel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocolstart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_protocolstop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_schemeadd(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_schemechange(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_schemedel(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_schemestart(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_schemestop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_start(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_stop(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ion_bp_admin_ctrl_watch(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_ION_BP_ADMIN_IMPL_H_
