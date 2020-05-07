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
 **  2020-04-16  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_amp_agent.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_amp_agent_impl.h"
#include "agent/rda.h"



#define _HAVE_AMP_AGENT_ADM_
#ifdef _HAVE_AMP_AGENT_ADM_

//vec_idx_t g_amp_agent_idx[11];

void amp_agent_init()
{
	adm_add_adm_info("amp_agent", ADM_ENUM_AMP_AGENT);

	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_OPER_IDX), &(g_amp_agent_idx[ADM_OPER_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_RPTT_IDX), &(g_amp_agent_idx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_CONST_IDX), &(g_amp_agent_idx[ADM_CONST_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_EDD_IDX), &(g_amp_agent_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_CTRL_IDX), &(g_amp_agent_idx[ADM_CTRL_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_META_IDX), &(g_amp_agent_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_TBLT_IDX), &(g_amp_agent_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_AMP_AGENT * 20) + ADM_VAR_IDX), &(g_amp_agent_idx[ADM_VAR_IDX]));


	amp_agent_setup();
	amp_agent_init_meta();
	amp_agent_init_cnst();
	amp_agent_init_edd();
	amp_agent_init_op();
	amp_agent_init_var();
	amp_agent_init_ctrl();
	amp_agent_init_mac();
	amp_agent_init_rpttpl();
	amp_agent_init_tblt();
}

void amp_agent_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_NAME), amp_agent_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_NAMESPACE), amp_agent_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_VERSION), amp_agent_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_ORGANIZATION), amp_agent_meta_organization);
}

void amp_agent_init_cnst()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_CONST_IDX], AMP_AGENT_CNST_AMP_EPOCH), amp_agent_get_amp_epoch);
}

void amp_agent_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_RPT_TPLS), amp_agent_get_num_rpt_tpls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBL_TPLS), amp_agent_get_num_tbl_tpls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_SENT_REPORTS), amp_agent_get_sent_reports);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBR), amp_agent_get_num_tbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_TBR), amp_agent_get_run_tbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_SBR), amp_agent_get_num_sbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_SBR), amp_agent_get_run_sbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_CONST), amp_agent_get_num_const);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_VAR), amp_agent_get_num_var);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_MACROS), amp_agent_get_num_macros);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_MACROS), amp_agent_get_run_macros);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_CONTROLS), amp_agent_get_num_controls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_CONTROLS), amp_agent_get_run_controls);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_CUR_TIME), amp_agent_get_cur_time);
}

void amp_agent_init_op()
{

	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSINT, 2, amp_agent_op_plusint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSUINT, 2, amp_agent_op_plusuint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSVAST, 2, amp_agent_op_plusvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSUVAST, 2, amp_agent_op_plusuvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSREAL32, 2, amp_agent_op_plusreal32);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSREAL64, 2, amp_agent_op_plusreal64);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSINT, 2, amp_agent_op_minusint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSUINT, 2, amp_agent_op_minusuint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSVAST, 2, amp_agent_op_minusvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSUVAST, 2, amp_agent_op_minusuvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSREAL32, 2, amp_agent_op_minusreal32);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSREAL64, 2, amp_agent_op_minusreal64);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTINT, 2, amp_agent_op_multint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTUINT, 2, amp_agent_op_multuint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTVAST, 2, amp_agent_op_multvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTUVAST, 2, amp_agent_op_multuvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTREAL32, 2, amp_agent_op_multreal32);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTREAL64, 2, amp_agent_op_multreal64);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVINT, 2, amp_agent_op_divint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVUINT, 2, amp_agent_op_divuint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVVAST, 2, amp_agent_op_divvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVUVAST, 2, amp_agent_op_divuvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVREAL32, 2, amp_agent_op_divreal32);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVREAL64, 2, amp_agent_op_divreal64);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODINT, 2, amp_agent_op_modint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODUINT, 2, amp_agent_op_moduint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODVAST, 2, amp_agent_op_modvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODUVAST, 2, amp_agent_op_moduvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODREAL32, 2, amp_agent_op_modreal32);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODREAL64, 2, amp_agent_op_modreal64);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPINT, 2, amp_agent_op_expint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPUINT, 2, amp_agent_op_expuint);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPVAST, 2, amp_agent_op_expvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPUVAST, 2, amp_agent_op_expuvast);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPREAL32, 2, amp_agent_op_expreal32);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPREAL64, 2, amp_agent_op_expreal64);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITAND, 2, amp_agent_op_bitand);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITOR, 2, amp_agent_op_bitor);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITXOR, 2, amp_agent_op_bitxor);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITNOT, 1, amp_agent_op_bitnot);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LOGAND, 2, amp_agent_op_logand);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LOGOR, 2, amp_agent_op_logor);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LOGNOT, 1, amp_agent_op_lognot);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_ABS, 1, amp_agent_op_abs);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LESSTHAN, 2, amp_agent_op_lessthan);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_GREATERTHAN, 2, amp_agent_op_greaterthan);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LESSEQUAL, 2, amp_agent_op_lessequal);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_GREATEREQUAL, 2, amp_agent_op_greaterequal);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_NOTEQUAL, 2, amp_agent_op_notequal);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EQUAL, 2, amp_agent_op_equal);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITSHIFTLEFT, 2, amp_agent_op_bitshiftleft);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITSHIFTRIGHT, 2, amp_agent_op_bitshiftright);
	adm_add_op(g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_STOR, 2, amp_agent_op_stor);
}

void amp_agent_init_var()
{

	ari_t *id = NULL;

	expr_t *expr = NULL;


	/* NUM_RULES */

	id = adm_build_ari(AMP_TYPE_VAR, 0, g_amp_agent_idx[ADM_VAR_IDX], AMP_AGENT_VAR_NUM_RULES);
	expr = expr_create(AMP_TYPE_UINT);
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBR));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_SBR));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSUINT));
	adm_add_var_from_expr(id, AMP_TYPE_UINT, expr);
}

void amp_agent_init_ctrl()
{

	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_VAR, 3, amp_agent_ctrl_add_var);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_VAR, 1, amp_agent_ctrl_del_var);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_RPTT, 2, amp_agent_ctrl_add_rptt);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_RPTT, 1, amp_agent_ctrl_del_rptt);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DESC_RPTT, 1, amp_agent_ctrl_desc_rptt);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_GEN_RPTS, 2, amp_agent_ctrl_gen_rpts);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_GEN_TBLS, 2, amp_agent_ctrl_gen_tbls);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_MACRO, 3, amp_agent_ctrl_add_macro);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_MACRO, 1, amp_agent_ctrl_del_macro);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DESC_MACRO, 1, amp_agent_ctrl_desc_macro);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_TBR, 6, amp_agent_ctrl_add_tbr);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_SBR, 7, amp_agent_ctrl_add_sbr);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_RULE, 1, amp_agent_ctrl_del_rule);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DESC_RULE, 1, amp_agent_ctrl_desc_rule);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_STORE_VAR, 2, amp_agent_ctrl_store_var);
	adm_add_ctrldef(g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_RESET_COUNTS, 0, amp_agent_ctrl_reset_counts);
}

void amp_agent_init_mac()
{

}

void amp_agent_init_rpttpl()
{

	rpttpl_t *def = NULL;

	/* FULL_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 0, g_amp_agent_idx[ADM_RPTT_IDX], AMP_AGENT_RPTTPL_FULL_REPORT));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_NAME));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_VERSION));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_RPT_TPLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBL_TPLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_SENT_REPORTS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_TBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_SBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_SBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_CONST));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_VAR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_MACROS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_MACROS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_CONTROLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_CONTROLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_VAR, 0, g_amp_agent_idx[ADM_VAR_IDX], AMP_AGENT_VAR_NUM_RULES));
	adm_add_rpttpl(def);
}

void amp_agent_init_tblt()
{

	tblt_t *def = NULL;

	/* ADMS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_ADMS), amp_agent_tblt_adms);
	tblt_add_col(def, AMP_TYPE_STR, "adm_name");
	adm_add_tblt(def);

	/* VARIABLES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_VARIABLES), amp_agent_tblt_variables);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);

	/* RPTTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RPTTS), amp_agent_tblt_rptts);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);

	/* MACROS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_MACROS), amp_agent_tblt_macros);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);

	/* RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RULES), amp_agent_tblt_rules);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);

	/* TBLTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_TBLTS), amp_agent_tblt_tblts);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);
}

#endif // _HAVE_AMP_AGENT_ADM_
