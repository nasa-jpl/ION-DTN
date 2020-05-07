/****************************************************************************
 **
 ** File Name: adm_amp_agent_mgr.c
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
#include "metadata.h"
#include "nm_mgr_ui.h"




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

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_AMP_AGENT, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_AMP_AGENT, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_AMP_AGENT, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_META_IDX], AMP_AGENT_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_AMP_AGENT, "organization", "The name of the issuing organization of the ADM.");

}

void amp_agent_init_cnst()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_amp_agent_idx[ADM_CONST_IDX], AMP_AGENT_CNST_AMP_EPOCH);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_TS, id, ADM_ENUM_AMP_AGENT, "amp_epoch", "This constant is the time epoch for the Agent.");

}

void amp_agent_init_edd()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_RPT_TPLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_rpt_tpls", "This is the number of report templates known to the Agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBL_TPLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_tbl_tpls", "This is the number of table templates known to the Agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_SENT_REPORTS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "sent_reports", "This is the number of reports sent by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_TBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_tbr", "This is the number of time-based rules running on the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_TBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "run_tbr", "This is the number of time-based rules run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_SBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_sbr", "This is the number of state-based rules running on the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_SBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "run_sbr", "This is the number of state-based rules run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_CONST);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_const", "This is the number of constants known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_VAR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_var", "This is the number of variables known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_MACROS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_macros", "This is the number of macros known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_MACROS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "run_macros", "This is the number of macros run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_NUM_CONTROLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_controls", "This is the number of controls known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_RUN_CONTROLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "run_controls", "This is the number of controls run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_amp_agent_idx[ADM_EDD_IDX], AMP_AGENT_EDD_CUR_TIME);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_TV, id, ADM_ENUM_AMP_AGENT, "cur_time", "This is the current system time.");

}

void amp_agent_init_op()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "plusINT", "Int32 addition");

	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "plusUINT", "Unsigned Int32 addition");

	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "plusVAST", "Int64 addition");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "plusUVAST", "Unsigned Int64 addition");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "plusREAL32", "Real32 addition");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "plusREAL64", "Real64 addition");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "minusINT", "Int32 subtraction");

	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "minusUINT", "Unsigned Int32 subtraction");

	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "minusVAST", "Int64 subtraction");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "minusUVAST", "Unsigned Int64 subtraction");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "minusREAL32", "Real32 subtraction");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MINUSREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "minusREAL64", "Real64 subtraction");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "multINT", "Int32 multiplication");

	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "multUINT", "Unsigned Int32 multiplication");

	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "multVAST", "Int64 multiplication");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "multUVAST", "Unsigned Int64 multiplication");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "multREAL32", "Real32 multiplication");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MULTREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "multREAL64", "Real64 multiplication");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "divINT", "Int32 division");

	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "divUINT", "Unsigned Int32 division");

	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "divVAST", "Int64 division");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "divUVAST", "Unsigned Int64 division");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "divREAL32", "Real32 division");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_DIVREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "divREAL64", "Real64 division");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "modINT", "Int32 modulus division");

	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "modUINT", "Unsigned Int32 modulus division");

	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "modVAST", "Int64 modulus division");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "modUVAST", "Unsigned Int64 modulus division");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "modREAL32", "Real32 modulus division");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_MODREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "modREAL64", "Real64 modulus division");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "expINT", "Int32 exponentiation");

	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "expUINT", "Unsigned int32 exponentiation");

	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "expVAST", "Int64 exponentiation");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "expUVAST", "Unsigned Int64 exponentiation");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "expREAL32", "Real32 exponentiation");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EXPREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "expREAL64", "Real64 exponentiation");

	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITAND);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitAND", "Bitwise and");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitOR", "Bitwise or");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITXOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitXOR", "Bitwise xor");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITNOT);
	adm_add_op_ari(id, 1, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitNOT", "Bitwise not");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LOGAND);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "logAND", "Logical and");

	meta_add_parm(meta, "O1", AMP_TYPE_BOOL);
	meta_add_parm(meta, "O2", AMP_TYPE_BOOL);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LOGOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "logOR", "Logical or");

	meta_add_parm(meta, "O1", AMP_TYPE_BOOL);
	meta_add_parm(meta, "O2", AMP_TYPE_BOOL);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LOGNOT);
	adm_add_op_ari(id, 1, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "logNOT", "Logical not");

	meta_add_parm(meta, "O1", AMP_TYPE_BOOL);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_ABS);
	adm_add_op_ari(id, 1, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "abs", "absolute value");

	meta_add_parm(meta, "O1", AMP_TYPE_VAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LESSTHAN);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "lessThan", "<");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_GREATERTHAN);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "greaterThan", ">");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_LESSEQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "lessEqual", "<=");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_GREATEREQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "greaterEqual", ">=");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_NOTEQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "notEqual", "!=");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_EQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "Equal", "==");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITSHIFTLEFT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitShiftLeft", "<<");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_BITSHIFTRIGHT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitShiftRight", ">>");

	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

	id = adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_STOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UNK, id, ADM_ENUM_AMP_AGENT, "STOR", "Store value of parm 2 in parm 1");

	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);
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
	meta_add_var(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "num_rules", "This is the number of rules known to the Agent (#TBR + #SBR).");

}

void amp_agent_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* ADD_VAR */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_VAR);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "add_var", "This control configures a new variable definition on the Agent.");

	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "def", AMP_TYPE_EXPR);
	meta_add_parm(meta, "type", AMP_TYPE_BYTE);

	/* DEL_VAR */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_VAR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "del_var", "This control removes one or more variable definitions from the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* ADD_RPTT */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_RPTT);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "add_rptt", "This control configures a new report template definition on the Agent.");

	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "template", AMP_TYPE_AC);

	/* DEL_RPTT */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_RPTT);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "del_rptt", "This control removes one or more report template definitions from the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* DESC_RPTT */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DESC_RPTT);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "desc_rptt", "This control produces a detailed description of one or more report template  identifier(ARI) known to the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* GEN_RPTS */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_GEN_RPTS);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "gen_rpts", "This control causes the Agent to produce a report entry for each identified report templates and send them to one or more identified managers(ARIs).");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);
	meta_add_parm(meta, "rxmgrs", AMP_TYPE_TNVC);

	/* GEN_TBLS */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_GEN_TBLS);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "gen_tbls", "This control causes the Agent to produce a table for each identified table templates and send them to one or more identified managers(ARIs).");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);
	meta_add_parm(meta, "rxmgrs", AMP_TYPE_TNVC);

	/* ADD_MACRO */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_MACRO);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "add_macro", "This control configures a new macro definition on the Agent.");

	meta_add_parm(meta, "name", AMP_TYPE_STR);
	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "def", AMP_TYPE_AC);

	/* DEL_MACRO */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_MACRO);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "del_macro", "This control removes one or more macro definitions from the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* DESC_MACRO */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DESC_MACRO);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "desc_macro", "This control produces a detailed description of one or more macro identifier(ARI) known to the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* ADD_TBR */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_TBR);
	adm_add_ctrldef_ari(id, 6, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "add_tbr", "This control configures a new time-based rule(TBR) definition on the Agent.");

	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "period", AMP_TYPE_TV);
	meta_add_parm(meta, "count", AMP_TYPE_UVAST);
	meta_add_parm(meta, "action", AMP_TYPE_AC);
	meta_add_parm(meta, "description", AMP_TYPE_STR);

	/* ADD_SBR */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_ADD_SBR);
	adm_add_ctrldef_ari(id, 7, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "add_sbr", "This control configures a new state-based rule(SBR) definition on the Agent.");

	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "state", AMP_TYPE_EXPR);
	meta_add_parm(meta, "max_eval", AMP_TYPE_UVAST);
	meta_add_parm(meta, "count", AMP_TYPE_UVAST);
	meta_add_parm(meta, "action", AMP_TYPE_AC);
	meta_add_parm(meta, "description", AMP_TYPE_STR);

	/* DEL_RULE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DEL_RULE);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "del_rule", "This control removes one or more rule definitions from the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* DESC_RULE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_DESC_RULE);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "desc_rule", "This control produces a detailed description of one or more rules known to the Agent.");

	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* STORE_VAR */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_STORE_VAR);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "store_var", "This control stores variables.");

	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "value", AMP_TYPE_EXPR);

	/* RESET_COUNTS */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_amp_agent_idx[ADM_CTRL_IDX], AMP_AGENT_CTRL_RESET_COUNTS);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "reset_counts", "This control resets all Agent ADM statistics reported in the Agent ADM report.");

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
	meta_add_rpttpl(def->id, ADM_ENUM_AMP_AGENT, "full_report", "This is all known meta-data, EDD, and VAR values known by the agent.");
}

void amp_agent_init_tblt()
{

	tblt_t *def = NULL;

	/* ADMS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_ADMS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "adm_name");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "adms", "This table lists all the adms that are supported by the agent.");

	/* VARIABLES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_VARIABLES), NULL);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "variables", "This table lists the ARI for every variable that is known to the agent.");

	/* RPTTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RPTTS), NULL);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "rptts", "This table lists the ARI for every report template that is known to the agent.");

	/* MACROS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_MACROS), NULL);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "macros", "This table lists the ARI for every macro that is known to the agent.");

	/* RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RULES), NULL);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "rules", "This table lists the ARI for every rule that is known to the agent.");

	/* TBLTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_amp_agent_idx[ADM_TBLT_IDX], AMP_AGENT_TBLT_TBLTS), NULL);
	tblt_add_col(def, AMP_TYPE_ARI, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "tblts", "This table lists the ARI for every table template that is known to the agent.");
}

#endif // _HAVE_AMP_AGENT_ADM_
