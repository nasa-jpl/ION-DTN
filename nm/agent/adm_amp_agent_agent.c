/****************************************************************************
 **
 ** File Name: adm_amp_agent_agent.c
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
 **  2018-10-16  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "../shared/adm/adm_amp_agent.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_amp_agent_impl.h"
#include "rda.h"

#define _HAVE_AMP_AGENT_ADM_
#ifdef _HAVE_AMP_AGENT_ADM_

static vec_idx_t gAmpAgentIdx[11];

void amp_agent_init()
{
	/* Sarah: Only generate NN's for elements that we have in the ADM. */
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_CONST_IDX), &(gAmpAgentIdx[ADM_CONST_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_CTRL_IDX),  &(gAmpAgentIdx[ADM_CTRL_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_EDD_IDX),   &(gAmpAgentIdx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_MAC_IDX),   &(gAmpAgentIdx[ADM_MAC_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_OPER_IDX),  &(gAmpAgentIdx[ADM_OPER_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_RPTT_IDX),  &(gAmpAgentIdx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_SBR_IDX),   &(gAmpAgentIdx[ADM_SBR_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_TBLT_IDX),  &(gAmpAgentIdx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_TBR_IDX),   &(gAmpAgentIdx[ADM_TBR_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_VAR_IDX),   &(gAmpAgentIdx[ADM_VAR_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_META_IDX),   &(gAmpAgentIdx[ADM_META_IDX]));

	amp_agent_setup();
	amp_agent_init_meta();
	amp_agent_init_cnst();
	amp_agent_init_edd();
	amp_agent_init_op();

	amp_agent_init_var();

	amp_agent_init_ctrl();
	amp_agent_init_macro();

	amp_agent_init_rptt();
	amp_agent_init_tblt();
}

void amp_agent_init_meta()
{
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_NAME),         amp_agent_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_NAMESPACE),    amp_agent_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_VERSION),      amp_agent_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_ORGANIZATION), amp_agent_meta_organization);
}

void amp_agent_init_cnst()
{
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_CONST_IDX], AMP_AGENT_AMP_EPOCH), amp_agent_get_amp_epoch);
}

void amp_agent_init_edd()
{
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_RPT_TPLS), amp_agent_get_num_rpt_tpls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBL_TPLS), amp_agent_get_num_tbl_tpls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_SENT_REPORTS), amp_agent_get_sent_reports);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBR),      amp_agent_get_num_tbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_TBR),      amp_agent_get_run_tbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_SBR),      amp_agent_get_num_sbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_SBR),      amp_agent_get_run_sbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_CONST),    amp_agent_get_num_const);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_VAR),      amp_agent_get_num_var);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_MACROS),   amp_agent_get_num_macros);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_MACROS),   amp_agent_get_run_macros);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_CONTROLS), amp_agent_get_num_controls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_CONTROLS), amp_agent_get_run_controls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_CUR_TIME),     amp_agent_get_cur_time);



}

void amp_agent_init_op()
{
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSINT,       2, amp_agent_op_plusint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSUINT,      2, amp_agent_op_plusuint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSVAST,      2, amp_agent_op_plusvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSUVAST,     2, amp_agent_op_plusuvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSREAL32,    2, amp_agent_op_plusreal32);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSREAL64,    2, amp_agent_op_plusreal64);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSINT,      2, amp_agent_op_minusint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSUINT,     2, amp_agent_op_minusuint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSVAST,     2, amp_agent_op_minusvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSUVAST,    2, amp_agent_op_minusuvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSREAL32,   2, amp_agent_op_minusreal32);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSREAL64,   2, amp_agent_op_minusreal64);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTINT,       2, amp_agent_op_multint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTUINT,      2, amp_agent_op_multuint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTVAST,      2, amp_agent_op_multvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTUVAST,     2, amp_agent_op_multuvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTREAL32,    2, amp_agent_op_multreal32);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTREAL64,    2, amp_agent_op_multreal64);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVINT,        2, amp_agent_op_divint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVUINT,       2, amp_agent_op_divuint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVVAST,       2, amp_agent_op_divvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVUVAST,      2, amp_agent_op_divuvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVREAL32,     2, amp_agent_op_divreal32);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVREAL64,     2, amp_agent_op_divreal64);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODINT,        2, amp_agent_op_modint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODUINT,       2, amp_agent_op_moduint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODVAST,       2, amp_agent_op_modvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODUVAST,      2, amp_agent_op_moduvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODREAL32,     2, amp_agent_op_modreal32);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODREAL64,     2, amp_agent_op_modreal64);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPINT,        2, amp_agent_op_expint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPUINT,       2, amp_agent_op_expuint);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPVAST,       2, amp_agent_op_expvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPUVAST,      2, amp_agent_op_expuvast);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPREAL32,     2, amp_agent_op_expreal32);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPREAL64,     2, amp_agent_op_expreal64);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITAND,        2, amp_agent_op_bitand);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITOR,         2, amp_agent_op_bitor);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITXOR,        2, amp_agent_op_bitxor);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITNOT,        1, amp_agent_op_bitnot);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LOGAND,        2, amp_agent_op_logand);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LOGOR,         2, amp_agent_op_logor);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LOGNOT,        1, amp_agent_op_lognot);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_ABS,           1, amp_agent_op_abs);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LESSTHAN,      2, amp_agent_op_lessthan);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_GREATERTHAN,   2, amp_agent_op_greaterthan);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LESSEQUAL,     2, amp_agent_op_lessequal);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_GREATEREQUAL,  2, amp_agent_op_greaterequal);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_NOTEQUAL,      2, amp_agent_op_notequal);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EQUAL,         2, amp_agent_op_equal);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITSHIFTLEFT,  2, amp_agent_op_bitshiftleft);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITSHIFTRIGHT, 2, amp_agent_op_bitshiftright);
	adm_add_op(gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_STOR,          2, amp_agent_op_stor);
}

void amp_agent_init_var()
{
	expr_t *expr = NULL;

	expr = expr_create(AMP_TYPE_UINT);
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBR));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_SBR));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSUINT));

	adm_add_var_from_expr(adm_build_ari(AMP_TYPE_VAR, 0, gAmpAgentIdx[ADM_VAR_IDX], AMP_AGENT_NUM_RULES), AMP_TYPE_UINT, expr);

}

void amp_agent_init_ctrl()
{

	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_VAR,   4, amp_agent_ctrl_add_var);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_VAR,   1, amp_agent_ctrl_del_var);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_RPTT,  2, amp_agent_ctrl_add_rptt);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_RPTT,  1, amp_agent_ctrl_del_rptt);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_RPTT, 1, amp_agent_ctrl_desc_rptt);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_GEN_RPTS,  2, amp_agent_ctrl_gen_rpts);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_GEN_TBLS,  1, amp_agent_ctrl_gen_tbls);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_MACRO, 3, amp_agent_ctrl_add_macro);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_MACRO, 1, amp_agent_ctrl_del_macro);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_MACRO,1, amp_agent_ctrl_desc_macro);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_TBR,   6, amp_agent_ctrl_add_tbr);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_TBR,   1, amp_agent_ctrl_del_tbr);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_TBR,  1, amp_agent_ctrl_desc_tbr);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_SBR,   6, amp_agent_ctrl_add_sbr);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_SBR,   1, amp_agent_ctrl_del_sbr);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_SBR,  1, amp_agent_ctrl_desc_sbr);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_STORE_VAR, 1, amp_agent_ctrl_store_var);
	adm_add_ctrldef(gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_RESET_COUNTS, 1, amp_agent_ctrl_reset_counts);
}

void amp_agent_init_macro()
{
	macdef_t *def;

	def = macdef_create(3, adm_build_ari(AMP_TYPE_MAC, 1, gAmpAgentIdx[ADM_MAC_IDX], AMP_AGENT_USER_DESC));

	adm_add_macdef_ctrl(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_CTRL, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_RPTT, tnv_from_map(AMP_TYPE_UINT, 0)));
	adm_add_macdef_ctrl(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_CTRL, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_TBR,  tnv_from_map(AMP_TYPE_UINT, 0)));
	adm_add_macdef_ctrl(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_CTRL, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_SBR,  tnv_from_map(AMP_TYPE_UINT, 0)));

	adm_add_macdef(def);
}

void amp_agent_init_rptt()
{

	ari_t *rpt_entry_id;

	rpttpl_t *def;

	/* Create the report template definition, including any entries. */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_FULL_REPORT));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_NAME));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_VERSION));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_RPT_TPLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBL_TPLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_SENT_REPORTS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_TBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_SBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_SBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_CONST));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_MACROS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_MACROS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_CONTROLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_CONTROLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_VAR, 0, gAmpAgentIdx[ADM_VAR_IDX], AMP_AGENT_NUM_RULES));


	/* When all entries are added to the report, add the report to the list of knwon templates. */
	adm_add_rpttpl(def);
}

void amp_agent_init_tblt()
{
	tblt_t *def;

	/* ADMS */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_ADM), adm_amp_agent_tbl_adms);
	tblt_add_col(def, AMP_TYPE_STR, "adm_name");
	adm_add_tblt(def);

	/* VARIABLE */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_VARIABLE), adm_amp_agent_tbl_variables);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);

	/* RPTT */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RPTT), adm_amp_agent_tbl_rptt);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);

	/* MACRO */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_MACRO), adm_amp_agent_tbl_macro);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);

	/* RULE */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RULE), adm_amp_agent_tbl_rule);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);

	/* TBLT */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_TBLT), adm_amp_agent_tbl_tblt);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);
}


#endif // _HAVE_AMP_AGENT_ADM_
