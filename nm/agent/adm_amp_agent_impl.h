/****************************************************************************
 **
 ** File Name: adm_amp_agent_impl.h
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
 **  2020-04-16  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_AMP_AGENT_IMPL_H_
#define ADM_AMP_AGENT_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */

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
} amp_agent_op_e;

/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */

void amp_agent_collect_ari_keys(rh_elt_t *elt, void *tag);
int amp_agent_build_ari_table(tbl_t *table, rhht_t *ht);
int adm_agent_op_prep(uint8_t num, tnv_t **lval, tnv_t **rval, vector_t *stack);
tnv_t *amp_agent_binary_num_op(amp_agent_op_e op, vector_t *stack, amp_type_e result_type);
tnv_t *adm_agent_unary_num_op(amp_agent_op_e op, vector_t *stack, amp_type_e result_type);

/*   STOP CUSTOM FUNCTIONS HERE  */

void amp_agent_setup();
void amp_agent_cleanup();


/* Metadata Functions */
tnv_t *amp_agent_meta_name(tnvc_t *parms);
tnv_t *amp_agent_meta_namespace(tnvc_t *parms);
tnv_t *amp_agent_meta_version(tnvc_t *parms);
tnv_t *amp_agent_meta_organization(tnvc_t *parms);

/* Constant Functions */
tnv_t *amp_agent_get_amp_epoch(tnvc_t *parms);

/* Collect Functions */
tnv_t *amp_agent_get_num_rpt_tpls(tnvc_t *parms);
tnv_t *amp_agent_get_num_tbl_tpls(tnvc_t *parms);
tnv_t *amp_agent_get_sent_reports(tnvc_t *parms);
tnv_t *amp_agent_get_num_tbr(tnvc_t *parms);
tnv_t *amp_agent_get_run_tbr(tnvc_t *parms);
tnv_t *amp_agent_get_num_sbr(tnvc_t *parms);
tnv_t *amp_agent_get_run_sbr(tnvc_t *parms);
tnv_t *amp_agent_get_num_const(tnvc_t *parms);
tnv_t *amp_agent_get_num_var(tnvc_t *parms);
tnv_t *amp_agent_get_num_macros(tnvc_t *parms);
tnv_t *amp_agent_get_run_macros(tnvc_t *parms);
tnv_t *amp_agent_get_num_controls(tnvc_t *parms);
tnv_t *amp_agent_get_run_controls(tnvc_t *parms);
tnv_t *amp_agent_get_cur_time(tnvc_t *parms);


/* Control Functions */
tnv_t *amp_agent_ctrl_add_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_del_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_add_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_del_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_desc_rptt(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_gen_rpts(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_gen_tbls(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_add_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_del_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_desc_macro(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_add_tbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_add_sbr(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_del_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_desc_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_store_var(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *amp_agent_ctrl_reset_counts(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */
tnv_t *amp_agent_op_plusint(vector_t *stack);
tnv_t *amp_agent_op_plusuint(vector_t *stack);
tnv_t *amp_agent_op_plusvast(vector_t *stack);
tnv_t *amp_agent_op_plusuvast(vector_t *stack);
tnv_t *amp_agent_op_plusreal32(vector_t *stack);
tnv_t *amp_agent_op_plusreal64(vector_t *stack);
tnv_t *amp_agent_op_minusint(vector_t *stack);
tnv_t *amp_agent_op_minusuint(vector_t *stack);
tnv_t *amp_agent_op_minusvast(vector_t *stack);
tnv_t *amp_agent_op_minusuvast(vector_t *stack);
tnv_t *amp_agent_op_minusreal32(vector_t *stack);
tnv_t *amp_agent_op_minusreal64(vector_t *stack);
tnv_t *amp_agent_op_multint(vector_t *stack);
tnv_t *amp_agent_op_multuint(vector_t *stack);
tnv_t *amp_agent_op_multvast(vector_t *stack);
tnv_t *amp_agent_op_multuvast(vector_t *stack);
tnv_t *amp_agent_op_multreal32(vector_t *stack);
tnv_t *amp_agent_op_multreal64(vector_t *stack);
tnv_t *amp_agent_op_divint(vector_t *stack);
tnv_t *amp_agent_op_divuint(vector_t *stack);
tnv_t *amp_agent_op_divvast(vector_t *stack);
tnv_t *amp_agent_op_divuvast(vector_t *stack);
tnv_t *amp_agent_op_divreal32(vector_t *stack);
tnv_t *amp_agent_op_divreal64(vector_t *stack);
tnv_t *amp_agent_op_modint(vector_t *stack);
tnv_t *amp_agent_op_moduint(vector_t *stack);
tnv_t *amp_agent_op_modvast(vector_t *stack);
tnv_t *amp_agent_op_moduvast(vector_t *stack);
tnv_t *amp_agent_op_modreal32(vector_t *stack);
tnv_t *amp_agent_op_modreal64(vector_t *stack);
tnv_t *amp_agent_op_expint(vector_t *stack);
tnv_t *amp_agent_op_expuint(vector_t *stack);
tnv_t *amp_agent_op_expvast(vector_t *stack);
tnv_t *amp_agent_op_expuvast(vector_t *stack);
tnv_t *amp_agent_op_expreal32(vector_t *stack);
tnv_t *amp_agent_op_expreal64(vector_t *stack);
tnv_t *amp_agent_op_bitand(vector_t *stack);
tnv_t *amp_agent_op_bitor(vector_t *stack);
tnv_t *amp_agent_op_bitxor(vector_t *stack);
tnv_t *amp_agent_op_bitnot(vector_t *stack);
tnv_t *amp_agent_op_logand(vector_t *stack);
tnv_t *amp_agent_op_logor(vector_t *stack);
tnv_t *amp_agent_op_lognot(vector_t *stack);
tnv_t *amp_agent_op_abs(vector_t *stack);
tnv_t *amp_agent_op_lessthan(vector_t *stack);
tnv_t *amp_agent_op_greaterthan(vector_t *stack);
tnv_t *amp_agent_op_lessequal(vector_t *stack);
tnv_t *amp_agent_op_greaterequal(vector_t *stack);
tnv_t *amp_agent_op_notequal(vector_t *stack);
tnv_t *amp_agent_op_equal(vector_t *stack);
tnv_t *amp_agent_op_bitshiftleft(vector_t *stack);
tnv_t *amp_agent_op_bitshiftright(vector_t *stack);
tnv_t *amp_agent_op_stor(vector_t *stack);


/* Table Build Functions */
tbl_t *amp_agent_tblt_adms(ari_t *id);
tbl_t *amp_agent_tblt_variables(ari_t *id);
tbl_t *amp_agent_tblt_rptts(ari_t *id);
tbl_t *amp_agent_tblt_macros(ari_t *id);
tbl_t *amp_agent_tblt_rules(ari_t *id);
tbl_t *amp_agent_tblt_tblts(ari_t *id);

#endif //ADM_AMP_AGENT_IMPL_H_
