/*****************************************************************************
 **
 ** File Name: ./agent/adm_AmpAgent_impl.h
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
 **  2017-11-21  AUTO           Auto generated header file 
 *****************************************************************************/

#ifndef ADM_AMPAGENT_IMPL_H_
#define ADM_AMPAGENT_IMPL_H_

/*   START CUSTOM INCLUDES HERE  *
 *           TODO                *
 *           ----                *
 *   STOP CUSTOM INCLUDES HERE   */

#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"

/******************
 * TODO: typeENUM *
 *****************/

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE  *
 *           TODO                *
 *           ----                *
 *   STOP CUSTOM FUNCTIONS HERE   */

/* Metadata Functions */
value_t adm_AmpAgent_meta_name(tdc_t params);
value_t adm_AmpAgent_meta_namespace(tdc_t params);

value_t adm_AmpAgent_meta_version(tdc_t params);

value_t adm_AmpAgent_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_AmpAgent_get_numReports(tdc_t params);
value_t adm_AmpAgent_get_sentReports(tdc_t params);
value_t adm_AmpAgent_get_numTrl(tdc_t params);
value_t adm_AmpAgent_get_runTrl(tdc_t params);
value_t adm_AmpAgent_get_numSrl(tdc_t params);
value_t adm_AmpAgent_get_runSrl(tdc_t params);
value_t adm_AmpAgent_get_numConst(tdc_t params);
value_t adm_AmpAgent_get_numVar(tdc_t params);
value_t adm_AmpAgent_get_numMacros(tdc_t params);
value_t adm_AmpAgent_get_runMacros(tdc_t params);
value_t adm_AmpAgent_get_numCTRL(tdc_t params);
value_t adm_AmpAgent_get_runCTRL(tdc_t params);
value_t adm_AmpAgent_get_curTime(tdc_t params);


/* Control Functions */
tdc_t* adm_AmpAgent_ctrl_listADMs(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_addVar(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_delVar(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_listVar(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_descVar(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_addRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_delRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_listRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_descRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_genRpts(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_addMacro(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_delMacro(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_listMacro(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_descMacro(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_addTrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_delTrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_listTrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_desCTRL(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_addSrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_delSrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_listSrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_descSrl(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_storeVar(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_AmpAgent_ctrl_resetCounts(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */
value_t* adm_AmpAgent_op_+INT(Lyst stack);
value_t* adm_AmpAgent_op_+UINT(Lyst stack);
value_t* adm_AmpAgent_op_+VAST(Lyst stack);
value_t* adm_AmpAgent_op_+UVAST(Lyst stack);
value_t* adm_AmpAgent_op_+REAL32(Lyst stack);
value_t* adm_AmpAgent_op_+REAL64(Lyst stack);
value_t* adm_AmpAgent_op_-INT(Lyst stack);
value_t* adm_AmpAgent_op_-UINT(Lyst stack);
value_t* adm_AmpAgent_op_-VAST(Lyst stack);
value_t* adm_AmpAgent_op_-UVAST(Lyst stack);
value_t* adm_AmpAgent_op_-REAL32(Lyst stack);
value_t* adm_AmpAgent_op_-REAL64(Lyst stack);
value_t* adm_AmpAgent_op_*INT(Lyst stack);
value_t* adm_AmpAgent_op_*UINT(Lyst stack);
value_t* adm_AmpAgent_op_*VAST(Lyst stack);
value_t* adm_AmpAgent_op_*UVAST(Lyst stack);
value_t* adm_AmpAgent_op_*REAL32(Lyst stack);
value_t* adm_AmpAgent_op_*REAL64(Lyst stack);
value_t* adm_AmpAgent_op_/INT(Lyst stack);
value_t* adm_AmpAgent_op_/UINT(Lyst stack);
value_t* adm_AmpAgent_op_/VAST(Lyst stack);
value_t* adm_AmpAgent_op_/UVAST(Lyst stack);
value_t* adm_AmpAgent_op_/REAL32(Lyst stack);
value_t* adm_AmpAgent_op_/REAL64(Lyst stack);
value_t* adm_AmpAgent_op_MODINT(Lyst stack);
value_t* adm_AmpAgent_op_MODUINT(Lyst stack);
value_t* adm_AmpAgent_op_MODVAST(Lyst stack);
value_t* adm_AmpAgent_op_MODUVAST(Lyst stack);
value_t* adm_AmpAgent_op_MODREAL32(Lyst stack);
value_t* adm_AmpAgent_op_MODREAL64(Lyst stack);
value_t* adm_AmpAgent_op_^INT(Lyst stack);
value_t* adm_AmpAgent_op_^UINT(Lyst stack);
value_t* adm_AmpAgent_op_^VAST(Lyst stack);
value_t* adm_AmpAgent_op_^UVAST(Lyst stack);
value_t* adm_AmpAgent_op_^REAL32(Lyst stack);
value_t* adm_AmpAgent_op_^REAL64(Lyst stack);
value_t* adm_AmpAgent_op_&(Lyst stack);
value_t* adm_AmpAgent_op_|(Lyst stack);
value_t* adm_AmpAgent_op_#(Lyst stack);
value_t* adm_AmpAgent_op_~(Lyst stack);
value_t* adm_AmpAgent_op_&&(Lyst stack);
value_t* adm_AmpAgent_op_||(Lyst stack);
value_t* adm_AmpAgent_op_!(Lyst stack);
value_t* adm_AmpAgent_op_abs(Lyst stack);
value_t* adm_AmpAgent_op_<(Lyst stack);
value_t* adm_AmpAgent_op_>(Lyst stack);
value_t* adm_AmpAgent_op_<=(Lyst stack);
value_t* adm_AmpAgent_op_>=(Lyst stack);
value_t* adm_AmpAgent_op_!=(Lyst stack);
value_t* adm_AmpAgent_op_==(Lyst stack);
value_t* adm_AmpAgent_op_<<(Lyst stack);
value_t* adm_AmpAgent_op_>>(Lyst stack);
value_t* adm_AmpAgent_op_STOR(Lyst stack);

#endif //ADM_AMPAGENT_IMPL_H_
