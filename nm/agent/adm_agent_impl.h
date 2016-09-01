/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/


/*****************************************************************************
 **
 ** File Name: adm_agent_impl.h
 **
 **
 ** Description: This file implements the private (agent-specific) aspects of
 **              an AMP Agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation (JHU/APL)
 **  07/03/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef ADM_AGENT_IMPL_H_
#define ADM_AGENT_IMPL_H_

#include "../shared/adm/adm_agent.h"
#include "../shared/adm/adm_bp.h"
#include "../shared/adm/adm_ion.h"
#include "../shared/adm/adm_ltp.h"
#include "instr.h"
#include "../shared/primitives/expr.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

typedef enum
{
  PLUS,
  MINUS,
  MULT,
  DIV,
  MOD,
  EXP,
  BITAND,
  BITOR,
  BITXOR,
  BITNOT,
  LOGAND,
  LOGOR,
  LOGNOT,
  ABS,
  LT,
  GT,
  LTE,
  GTE,
  NEG,
  NEQ,
  EQ,
  BITLSHFT,
  BITRSHFT
} agent_adm_op_e;


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

/* Metadata Functions. */
value_t adm_agent_md_name(tdc_t params);
value_t adm_agent_md_ver(tdc_t params);



/* Collect Functions */
value_t adm_agent_get_num_rpt(tdc_t params);
value_t adm_agent_get_sent_rpt(tdc_t params);
value_t adm_agent_get_num_trl(tdc_t params);
value_t adm_agent_get_run_trl(tdc_t params);
value_t adm_agent_get_num_srl(tdc_t params);
value_t adm_agent_get_run_srl(tdc_t params);
value_t adm_agent_get_num_lit(tdc_t params);
value_t adm_agent_get_num_var(tdc_t params);
value_t adm_agent_get_num_mac(tdc_t params);
value_t adm_agent_get_run_mac(tdc_t params);
value_t adm_agent_get_num_ctrl(tdc_t params);
value_t adm_agent_get_run_ctrl(tdc_t params);
value_t adm_agent_get_curtime(tdc_t params);

/* Control Functions */

tdc_t* adm_agent_ctl_adm_lst(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_var_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_var_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_var_lst(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_var_dsc(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_rptt_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_rptt_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_rptt_lst(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_rptt_dsc(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_rpt_gen(eid_t *def_mgr, tdc_t params, int8_t *status);


tdc_t* adm_agent_ctl_mac_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_mac_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_mac_lst(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_mac_dsc(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_trl_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_trl_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_trl_lst(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_trl_dsc(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_srl_add(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_srl_del(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_srl_lst(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_agent_ctl_srl_dsc(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_lit_lst(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_stor(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_agent_ctl_reset_cnt(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions. */

value_t adm_agent_binary_num_op(agent_adm_op_e op, Lyst stack);
value_t adm_agent_binary_log_op(agent_adm_op_e op, Lyst stack);
value_t adm_agent_unary_num_op(agent_adm_op_e op, Lyst stack);
value_t adm_agent_unary_log_op(agent_adm_op_e op, Lyst stack);


value_t adm_agent_op_plus(Lyst stack);
value_t adm_agent_op_minus(Lyst stack);
value_t adm_agent_op_mult(Lyst stack);
value_t adm_agent_op_div(Lyst stack);
value_t adm_agent_op_mod(Lyst stack);
value_t adm_agent_op_exp(Lyst stack);
value_t adm_agent_op_bitand(Lyst stack);
value_t adm_agent_op_bitor(Lyst stack);
value_t adm_agent_op_bitxor(Lyst stack);
value_t adm_agent_op_bitnot(Lyst stack);
value_t adm_agent_op_logand(Lyst stack);
value_t adm_agent_op_logor(Lyst stack);
value_t adm_agent_op_lognot(Lyst stack);
value_t adm_agent_op_abs(Lyst stack);
value_t adm_agent_op_lt(Lyst stack);
value_t adm_agent_op_gt(Lyst stack);
value_t adm_agent_op_lte(Lyst stack);
value_t adm_agent_op_gte(Lyst stack);
value_t adm_agent_op_neg(Lyst stack);
value_t adm_agent_op_neq(Lyst stack);
value_t adm_agent_op_eq(Lyst stack);
value_t adm_agent_op_lshft(Lyst stack);
value_t adm_agent_op_rshft(Lyst stack);
value_t adm_agent_op_stor(Lyst stack);


#endif // ADM_AGENT_IMPL_H_



