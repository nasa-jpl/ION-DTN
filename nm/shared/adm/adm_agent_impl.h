/*****************************************************************************
 **
 ** File Name: adm_agent_impl.h
 **
 ** Description: This implements the private aspects of a DTNMP agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 *****************************************************************************/

#ifndef ADM_AGENT_IMPL_H_
#define ADM_AGENT_IMPL_H_

#ifdef AGENT_ROLE

#include "shared/adm/adm_agent.h"
#include "shared/adm/adm_bp.h"
#include "shared/adm/adm_ion.h"
#include "shared/adm/adm_ltp.h"
#include "shared/primitives/instr.h"
#include "agent/expr.h"

void agent_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/


/* Retrieval Functions */

/* DTNMP AGENT */

/* MEtadata Functions */
value_t agent_md_name(Lyst params);
value_t agent_md_ver(Lyst params);

/* Collect Functions */
value_t agent_get_num_rpt(Lyst params);
value_t agent_get_sent_rpt(Lyst params);
value_t agent_get_num_trl(Lyst params);
value_t agent_get_run_trl(Lyst params);
value_t agent_get_num_srl(Lyst params);
value_t agent_get_run_srl(Lyst params);
value_t agent_get_num_lit(Lyst params);
value_t agent_get_num_cust(Lyst params);
value_t agent_get_num_mac(Lyst params);
value_t agent_get_run_mac(Lyst params);
value_t agent_get_num_ctrl(Lyst params);
value_t agent_get_run_ctrl(Lyst params);

/* Control Functions */

tdc_t* agent_ctl_adm_lst(eid_t *def_mgr, Lyst params, uint8_t *status);

tdc_t* agent_ctl_cd_add(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_cd_del(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_cd_lst(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_cd_dsc(eid_t *def_mgr, Lyst params, uint8_t *status);

tdc_t* agent_ctl_rpt_add(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_rpt_del(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_rpt_lst(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_rpt_dsc(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_rpt_gen(eid_t *def_mgr, Lyst params, uint8_t *status);


tdc_t* agent_ctl_mac_add(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_mac_del(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_mac_lst(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_mac_dsc(eid_t *def_mgr, Lyst params, uint8_t *status);

tdc_t* agent_ctl_trl_add(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_trl_del(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_trl_lst(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_trl_dsc(eid_t *def_mgr, Lyst params, uint8_t *status);

tdc_t* agent_ctl_srl_add(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_srl_del(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_srl_lst(eid_t *def_mgr, Lyst params, uint8_t *status);
tdc_t* agent_ctl_srl_dsc(eid_t *def_mgr, Lyst params, uint8_t *status);

tdc_t* agent_ctl_lit_lst(eid_t *def_mgr, Lyst params, uint8_t *status);


/* OP Functions. */

value_t adm_agent_op_plus(Lyst parms);
value_t adm_agent_op_minus(Lyst parms);
value_t adm_agent_op_mult(Lyst parms);
value_t adm_agent_op_div(Lyst parms);
value_t adm_agent_op_mod(Lyst parms);
value_t adm_agent_op_exp(Lyst parms);
value_t adm_agent_op_bitand(Lyst parms);
value_t adm_agent_op_bitor(Lyst parms);
value_t adm_agent_op_bitxor(Lyst parms);
value_t adm_agent_op_bitnot(Lyst parms);
value_t adm_agent_op_logand(Lyst parms);
value_t adm_agent_op_logor(Lyst parms);
value_t adm_agent_op_lognot(Lyst parms);
value_t adm_agent_op_abs(Lyst parms);
value_t adm_agent_op_lt(Lyst parms);
value_t adm_agent_op_gt(Lyst parms);
value_t adm_agent_op_lte(Lyst parms);
value_t adm_agent_op_gte(Lyst parms);
value_t adm_agent_op_neq(Lyst parms);
value_t adm_agent_op_eq(Lyst parms);

#endif // AGENT_ROLE
#endif // ADM_AGENT_IMPL_H_



