/****************************************************************************
 **
 ** File Name: adm_amp_agent_mgr.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** TODO - We need to rethink this approach. We should auto-generate a data
 **        table and look through that table in these functions. Right
 **        now these functions represent loop rollouts which in some cases
 **        can be more efficient, but greatly increases code size.
 **
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
#include "metadata.h"
#include "nm_mgr_ui.h"


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
	ari_t *id;

	id = adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR,id, ADM_ENUM_AMP_AGENT, "NAME", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR,id, ADM_ENUM_AMP_AGENT, "NAMESPACE", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR,id, ADM_ENUM_AMP_AGENT, "VERSION", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_META_IDX], AMP_AGENT_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR,id, ADM_ENUM_AMP_AGENT, "ORGANIZATION", "The name of the issuing organization of the ADM.");
}

void amp_agent_init_cnst()
{
	ari_t *id;

	id = adm_build_ari(AMP_TYPE_CNST, 0, gAmpAgentIdx[ADM_CONST_IDX], AMP_AGENT_AMP_EPOCH);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_TS, id, ADM_ENUM_AMP_AGENT, "AMP_EPOCH", "This constant is the time epoch for the Agent.");
}


void amp_agent_init_edd()
{
	ari_t *id;

	/*
	 * Sarah, we only define metadata_t *meta (and assigned it in calls to meta_add_edd) if we need to
	 * add parms. Otherwise, strict compilers will give an "unused variable" error.
	 */

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_RPT_TPLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_RPT_TPLS", "This is the number of report templates known to the Agent.");

	//meta_add_parm(meta, "parm_name", AMP_TYPE_??? Sarah: Here is how you add a parm to a metadata structure.

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBL_TPLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_TBL_TPLS", "This is the number of table templates known to the Agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_SENT_REPORTS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "SENT_REPORTS", "This is the number of reports sent by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_TBR", "This is the number of time-based rules running on the agent.");


	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_TBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "RUN_TBR", "This is the number of time-based rules run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_SBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_SBR", "This is the number of state-based rules running on the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_SBR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "RUN_SBR", "This is the number of state-based rules run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_CONST);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_CONST", "This is the number of constants known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_VAR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_VAR", "This is the number of variables known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_MACROS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_MACROS", "This is the number of macros known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_MACROS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "RUN_MACROS", "This is the number of macros run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_CONTROLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "NUM_CONTROLS", "This is the number of controls known by the agent.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_RUN_CONTROLS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT,id, ADM_ENUM_AMP_AGENT, "RUN_CONTROLS", "This is the number of controls run by the agent since the last reset.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_CUR_TIME);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_TV,id, ADM_ENUM_AMP_AGENT, "CUR_TIME", "This is the current system time.");
}

void amp_agent_init_op()
{
	ari_t *id;
	metadata_t *meta;

	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "PLUSINT","Int32 Addition");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);

	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "PLUSUINT","Unsigned Int32 Addition");
	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "PLUSVAST","Int64 Addition");
	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "PLUSUVAST","Unsigned Int64 Addition");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "PLUSREAL32","Real32 Addition");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "PLUSREAL64","Real64 Addition");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);





	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "MINUSINT","Int32 Subtraction");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "MINUSUINT","Unsigned Int32 Subtraction");
	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "MINUSVAST","Int64 Subtraction");
	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "MINUSUVAST","Unsigned Int64 Subtraction");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "MINUSREAL32","Real32 Subtraction");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MINUSREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "MINUSREAL64","Real64 Subtraction");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);




	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "MULTINT","Int32 Multiplication");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "MULTUINT","Unsigned Int32 Multiplication");
	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "MULTVAST","Int64 Multiplication");
	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "MULTUVAST","Unsigned Int64 Multiplication");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "MULTREAL32","Real32 Multiplication");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MULTREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "MULTREAL64","Real64 Multiplication");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);




	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "DIVINT","Int32 Division");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "DIVUINT","Unsigned Int32 Division");
	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "DIVVAST","Int64 Division");
	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "DIVUVAST","Unsigned Int64 Division");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "DIVREAL32","Real32 Division");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_DIVREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "DIVREAL64","Real64 Division");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "MODINT","Int32 Modulus division");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "MODUINT","Unsigned Int32 Modulus division");
	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "MODVAST","Int64 Modulus division");
	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "MODUVAST","Unsigned Int64 Modulus division");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "MODREAL32","Real32 Modulus division");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_MODREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "MODREAL64","Real64 Modulus division");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);





	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_INT, id, ADM_ENUM_AMP_AGENT, "EXPINT","Int32 Exponentiation");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);
	meta_add_parm(meta, "O2", AMP_TYPE_INT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPUINT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "EXPUINT","Unsigned Int32 Exponentiation");
	meta_add_parm(meta, "O1", AMP_TYPE_UINT);
	meta_add_parm(meta, "O2", AMP_TYPE_UINT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_VAST, id, ADM_ENUM_AMP_AGENT, "EXPVAST","Int64 Exponentiation");
	meta_add_parm(meta, "O1", AMP_TYPE_VAST);
	meta_add_parm(meta, "O2", AMP_TYPE_VAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPUVAST);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "EXPUVAST","Unsigned Int64 Exponentiation");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPREAL32);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL32, id, ADM_ENUM_AMP_AGENT, "EXPREAL32","Real32 Exponentiation");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL32);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL32);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EXPREAL64);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_REAL64, id, ADM_ENUM_AMP_AGENT, "EXPREAL64","Real64 Exponentiation");
	meta_add_parm(meta, "O1", AMP_TYPE_REAL64);
	meta_add_parm(meta, "O2", AMP_TYPE_REAL64);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITAND);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "BITAND","Bitwise and");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "BITOR","Bitwise or");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITXOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "BITOR","Bitwise xor");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITNOT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "BITOR","Bitwise not");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);




	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LOGAND);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "LOGAND","Logical and");
	meta_add_parm(meta, "O1", AMP_TYPE_BOOL);
	meta_add_parm(meta, "O2", AMP_TYPE_BOOL);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LOGOR);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "LOGOR","Logical or");
	meta_add_parm(meta, "O1", AMP_TYPE_BOOL);
	meta_add_parm(meta, "O2", AMP_TYPE_BOOL);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LOGNOT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "LOGNOT","Logical not");
	meta_add_parm(meta, "O1", AMP_TYPE_BOOL);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_ABS);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "ABS","Int32 Absolute Value");
	meta_add_parm(meta, "O1", AMP_TYPE_INT);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LESSTHAN);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "LessThan","<");
	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_GREATERTHAN);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "GreaterThan",">");
	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_LESSEQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "LessEqual","<=");
	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_GREATEREQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "greaterEqual",">=");
	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_NOTEQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "notEqual","!=");
	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_EQUAL);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_BOOL, id, ADM_ENUM_AMP_AGENT, "equal","==");
	meta_add_parm(meta, "O1", AMP_TYPE_UNK);
	meta_add_parm(meta, "O2", AMP_TYPE_UNK);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITSHIFTLEFT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitShiftLeft","<<");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);


	id = adm_build_ari(AMP_TYPE_OPER, 1, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_BITSHIFTRIGHT);
	adm_add_op_ari(id, 2, NULL);
	meta = meta_add_op(AMP_TYPE_UVAST, id, ADM_ENUM_AMP_AGENT, "bitShiftRight",">>");
	meta_add_parm(meta, "O1", AMP_TYPE_UVAST);
	meta_add_parm(meta, "O2", AMP_TYPE_UVAST);

}


void amp_agent_init_var()
{
	ari_t *id = NULL;
	expr_t *expr = NULL;

	/* NUM_RULES */
	id = adm_build_ari(AMP_TYPE_VAR, 0, gAmpAgentIdx[ADM_VAR_IDX], AMP_AGENT_NUM_RULES);

	expr = expr_create(AMP_TYPE_UINT);
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_TBR));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_EDD_IDX], AMP_AGENT_NUM_SBR));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_OPER, 0, gAmpAgentIdx[ADM_OPER_IDX], AMP_AGENT_PLUSUINT));

	adm_add_var_from_expr(id, AMP_TYPE_UINT, expr);
	meta_add_var(AMP_TYPE_UINT, id, ADM_ENUM_AMP_AGENT, "NUM_RULES", "This is the number of rules known to the Agent (#TBR + #SBR).");
}

void amp_agent_init_ctrl()
{
	ari_t *id = NULL;
	metadata_t *meta = NULL;

	/* ADD_VAR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_VAR);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "ADD_VAR", "This control configures a new variable definition on the Agent.");
	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "def", AMP_TYPE_EXPR);
	meta_add_parm(meta, "type", AMP_TYPE_BYTE);

	/* DEL_VAR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_VAR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DEL_VAR", "This control removes one or more variable definitions from the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* ADD_RPTT */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_RPTT);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "ADD_RPTT", "This control configures a new report template definition on the Agent.");
	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "template", AMP_TYPE_AC);

	/* DEL_RPTT */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_RPTT);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DEL_RPTT", "This control removes one or more report template definitions from the Agent.");
	meta_add_parm(meta, "id", AMP_TYPE_AC);

	/* DESC_RPTT */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_RPTT);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DESC_RPTT", "This control produces a detailed description of one or more report template  identifier(ARI) known to the Agent.");
	meta_add_parm(meta, "id", AMP_TYPE_AC);

	/* GEN_RPTS */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_GEN_RPTS);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "GEN_RPTS", "This control causes the Agent to produce a report entry for each identified report templates and send them to one or more identified managers(ARIs).");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);
	meta_add_parm(meta, "rxmgrs", AMP_TYPE_TNVC);

	/* GEN_TBLS */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_GEN_TBLS);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "GEN_TBLS", "This control causes the Agent to produce a table for each identified rtable templates and send them to one or more identified managers(ARIs).");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);
	meta_add_parm(meta, "rxmgrs", AMP_TYPE_TNVC);

	/* ADD_MACRO */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_MACRO);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "ADD_MACRO", "This control configures a new macro definition on the Agent.");
	meta_add_parm(meta, "name", AMP_TYPE_STR);
	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "def", AMP_TYPE_AC);

	/* DEL_MACRO */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_MACRO);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DEL_MACRO", "This control removes one or more macro definitions from the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* DESC_MACRO */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_MACRO);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DESC_MACRO", "This control produces a detailed description of one or more macro identifier(ARI) known to the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* ADD_TBR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_TBR);
	adm_add_ctrldef_ari(id, 6, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "ADD_TBR", "This control configures a new time-based rule(TBR) definition on the Agent.");
	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "period", AMP_TYPE_TV);
	meta_add_parm(meta, "count", AMP_TYPE_UVAST);
	meta_add_parm(meta, "action", AMP_TYPE_AC);
	meta_add_parm(meta, "description", AMP_TYPE_STR);


	/* DEL_TBR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_TBR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DEL_TBR", "This control removes one or more TBR definitions from the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* DESC_TBR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_TBR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DESC_TBR", "This control produces a detailed description of one or more TBR identifier(ARI)s known to the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);


	/* ADD_SBR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_ADD_SBR);
	adm_add_ctrldef_ari(id, 6, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "ADD_SBR", "This control configures a new state-based rule(SBR) definition on the Agent.");
	meta_add_parm(meta, "id", AMP_TYPE_ARI);
	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "state", AMP_TYPE_EXPR);
	meta_add_parm(meta, "count", AMP_TYPE_UVAST);
	meta_add_parm(meta, "action", AMP_TYPE_AC);
	meta_add_parm(meta, "description", AMP_TYPE_STR);


	/* DEL_SBR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DEL_SBR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DEL_SBR", "This control removes one or more SBR definitions from the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* DESC_SBR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_SBR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "DESC_SBR", "This control produces a detailed description of one or more SBR identifier(ARI)s known to the Agent.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* STOR_VAR */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_STORE_VAR);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "STORE_VAR", "This control stores variables.");
	meta_add_parm(meta, "ids", AMP_TYPE_AC);

	/* RESET_COUNTS */
	id = adm_build_ari(AMP_TYPE_CTRL, 1, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_RESET_COUNTS);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_AMP_AGENT, "RESET_COUNTS", "This control resets all Agent ADM statistics reported in the Agent ADM report.");
}


void amp_agent_init_macro()
{
	macdef_t *def;

	/* USER DESC */
	def = macdef_create(3, adm_build_ari(AMP_TYPE_MAC, 1, gAmpAgentIdx[ADM_MAC_IDX], AMP_AGENT_USER_DESC));

	adm_add_macdef_ctrl(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_CTRL, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_RPTT, tnv_from_map(AMP_TYPE_UINT, 0)));
	adm_add_macdef_ctrl(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_CTRL, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_TBR,  tnv_from_map(AMP_TYPE_UINT, 0)));
	adm_add_macdef_ctrl(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_CTRL, gAmpAgentIdx[ADM_CTRL_IDX], AMP_AGENT_DESC_SBR,  tnv_from_map(AMP_TYPE_UINT, 0)));

	adm_add_macdef(def);

	meta_add_macro(def->ari, ADM_ENUM_AMP_AGENT, "USER_DESCR", "This macro lists all of the user defined data.");
}



void amp_agent_init_rptt()
{
	rpttpl_t *def;

	/* FULL_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_FULL_REPORT));

	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NAME));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_VERSION));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_RPT_TPLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_TBL_TPLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_SENT_REPORTS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_TBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_RUN_TBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_SBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_RUN_SBR));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_CONST));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_MACROS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_RUN_MACROS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_CONTROLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_RUN_CONTROLS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_VAR, 0, gAmpAgentIdx[ADM_RPTT_IDX], AMP_AGENT_NUM_RULES));

	/* When all entries are added to the report, add the report to the list of knwon templates. */
	adm_add_rpttpl(def);
	meta_add_rpttpl(def->id, ADM_ENUM_AMP_AGENT, "FULL_REPORT", "This is all known meta-data, EDD, and VAR values known by the agent.");
}

void amp_agent_init_tblt()
{
	tblt_t *def;

	/* ADMS */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_ADM), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "adm_name");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "ADM", "This table lists all the adms that are supported by the agent.");

	/* VARIABLE */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_VARIABLE), NULL);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "VARIABLE", "This table lists the ARI for every variable that is known to the agent.");

	/* RPTT */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RPTT), NULL);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "RPTT", "This table lists the ARI for every report template that is known to the agent.");

	/* MACRO */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_MACRO), NULL);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "MACRO", "This table lists the ARI for every macro that is known to the agent.");

	/* RULE */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_RULE), NULL);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "RULE", "This table lists the ARI for every rule that is known to the agent.");

	/* TBLT */
	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, gAmpAgentIdx[ADM_TBLT_IDX], AMP_AGENT_TBLT_TBLT), NULL);
	tblt_add_col(def, AMP_TYPE_AC, "ids");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_AMP_AGENT, "TBLT", "This table lists the ARI for every table template that is known to the agent.");
}

#endif // _HAVE_AMP_AGENT_ADM_
