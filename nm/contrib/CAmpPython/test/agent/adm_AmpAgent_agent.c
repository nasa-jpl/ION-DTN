/****************************************************************************
 **
 ** File Name: ./agent/adm_AmpAgent_agent.c
 **
 ** Description: 
 **
 ** Notes: 
 **
 ** Assumptions: 
 **
 ** Modification History: 
 **  MM/DD/YY  AUTHOR           DESCRIPTION
 **  --------  --------------   ------------------------------------------------
 **  2017-11-21  AUTO           Auto generated c file 
 **
****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_AmpAgent.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_AmpAgent_impl.h"
#include "rda.h"

#define _HAVE_AMPAGENT_ADM_
#ifdef _HAVE_AMPAGENT_ADM_

void adm_AmpAgent_init()
{
	adm_AmpAgent_init_edd();
	adm_AmpAgent_init_variables();
	adm_AmpAgent_init_controls();
	adm_AmpAgent_init_constants();
	adm_AmpAgent_init_macros();
	adm_AmpAgent_init_metadata();
	adm_AmpAgent_init_ops();
	adm_AmpAgent_init_reports();
\/\* START CUSTOM INIT FUNCTIONS HERE \*
 *              TO DO                 *
 *                                    *
\/\* END CUSTOM INIT FUNCTIONS HERE \*\/}

void adm_AmpAgent_init_edd()
{
	adm_add_edd(ADM_AMPAGENT_EDD_NUMREPORTS_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numReports, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_SENTREPORTS_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_sentReports, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_NUMTRL_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numTrl, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_RUNTRL_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_runTrl, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_NUMSRL_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numSrl, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_RUNSRL_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_runSrl, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_NUMCONST_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numConst, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_NUMVAR_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numVar, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_NUMMACROS_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numMacros, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_RUNMACROS_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_runMacros, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_NUMCTRL_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_numCTRL, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_RUNCTRL_MID, AMP_TYPE_UINT, 0, adm_AmpAgent_get_runCTRL, NULL, NULL);
	adm_add_edd(ADM_AMPAGENT_EDD_CURTIME_MID, AMP_TYPE_TS, 0, adm_AmpAgent_get_curTime, NULL, NULL);
}

void adm_AmpAgent_init_variables()
{
	adm_add_var(ADM_AMPAGENT_VAR_NUMRULES_MID, adm_AmpAgent_var_numRules);
}

void adm_AmpAgent_init_controls()
{
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTADMS_MID, adm_AmpAgent_ctrl_listADMs);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDVAR_MID, adm_AmpAgent_ctrl_addVar);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELVAR_MID, adm_AmpAgent_ctrl_delVar);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTVAR_MID, adm_AmpAgent_ctrl_listVar);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCVAR_MID, adm_AmpAgent_ctrl_descVar);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDRPTTPL_MID, adm_AmpAgent_ctrl_addRptTpl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELRPTTPL_MID, adm_AmpAgent_ctrl_delRptTpl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTRPTTPL_MID, adm_AmpAgent_ctrl_listRptTpl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCRPTTPL_MID, adm_AmpAgent_ctrl_descRptTpl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_GENRPTS_MID, adm_AmpAgent_ctrl_genRpts);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDMACRO_MID, adm_AmpAgent_ctrl_addMacro);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELMACRO_MID, adm_AmpAgent_ctrl_delMacro);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTMACRO_MID, adm_AmpAgent_ctrl_listMacro);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCMACRO_MID, adm_AmpAgent_ctrl_descMacro);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDTRL_MID, adm_AmpAgent_ctrl_addTrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELTRL_MID, adm_AmpAgent_ctrl_delTrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTTRL_MID, adm_AmpAgent_ctrl_listTrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCTRL_MID, adm_AmpAgent_ctrl_desCTRL);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDSRL_MID, adm_AmpAgent_ctrl_addSrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELSRL_MID, adm_AmpAgent_ctrl_delSrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTSRL_MID, adm_AmpAgent_ctrl_listSrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCSRL_MID, adm_AmpAgent_ctrl_descSrl);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_STOREVAR_MID, adm_AmpAgent_ctrl_storeVar);
	adm_add_ctrl(ADM_AMPAGENT_CTRL_RESETCOUNTS_MID, adm_AmpAgent_ctrl_resetCounts);
}

void adm_AmpAgent_init_constants()
{
	adm_add_const(ADM_AMPAGENT_CONST_AMPEPOCH_MID, adm_AmpAgent_const_AMPEpoch);
}

void adm_AmpAgent_init_macros()
{
	adm_add_macro(ADM_AMPAGENT_MACRO_USERLIST_MID, adm_AmpAgent_macro_userList);
}

void adm_AmpAgent_init_metadata()
{
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_AMPAGENT_META_NAME_MID, AMP_TYPE_STR, 0, adm_AmpAgent_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(ADM_AMPAGENT_META_NAMESPACE_MID, AMP_TYPE_STR, 0, adm_AmpAgent_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(ADM_AMPAGENT_META_VERSION_MID, AMP_TYPE_STR, 0, adm_AmpAgent_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(ADM_AMPAGENT_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, adm_AmpAgent_meta_organization, adm_print_string, adm_size_string);
}

void adm_AmpAgent_init_ops()
{
	adm_add_op(ADM_AMPAGENT_OP_+INT_MID, name_op_+INT);
	adm_add_op(ADM_AMPAGENT_OP_+UINT_MID, name_op_+UINT);
	adm_add_op(ADM_AMPAGENT_OP_+VAST_MID, name_op_+VAST);
	adm_add_op(ADM_AMPAGENT_OP_+UVAST_MID, name_op_+UVAST);
	adm_add_op(ADM_AMPAGENT_OP_+REAL32_MID, name_op_+REAL32);
	adm_add_op(ADM_AMPAGENT_OP_+REAL64_MID, name_op_+REAL64);
	adm_add_op(ADM_AMPAGENT_OP_-INT_MID, name_op_-INT);
	adm_add_op(ADM_AMPAGENT_OP_-UINT_MID, name_op_-UINT);
	adm_add_op(ADM_AMPAGENT_OP_-VAST_MID, name_op_-VAST);
	adm_add_op(ADM_AMPAGENT_OP_-UVAST_MID, name_op_-UVAST);
	adm_add_op(ADM_AMPAGENT_OP_-REAL32_MID, name_op_-REAL32);
	adm_add_op(ADM_AMPAGENT_OP_-REAL64_MID, name_op_-REAL64);
	adm_add_op(ADM_AMPAGENT_OP_*INT_MID, name_op_*INT);
	adm_add_op(ADM_AMPAGENT_OP_*UINT_MID, name_op_*UINT);
	adm_add_op(ADM_AMPAGENT_OP_*VAST_MID, name_op_*VAST);
	adm_add_op(ADM_AMPAGENT_OP_*UVAST_MID, name_op_*UVAST);
	adm_add_op(ADM_AMPAGENT_OP_*REAL32_MID, name_op_*REAL32);
	adm_add_op(ADM_AMPAGENT_OP_*REAL64_MID, name_op_*REAL64);
	adm_add_op(ADM_AMPAGENT_OP_/INT_MID, name_op_/INT);
	adm_add_op(ADM_AMPAGENT_OP_/UINT_MID, name_op_/UINT);
	adm_add_op(ADM_AMPAGENT_OP_/VAST_MID, name_op_/VAST);
	adm_add_op(ADM_AMPAGENT_OP_/UVAST_MID, name_op_/UVAST);
	adm_add_op(ADM_AMPAGENT_OP_/REAL32_MID, name_op_/REAL32);
	adm_add_op(ADM_AMPAGENT_OP_/REAL64_MID, name_op_/REAL64);
	adm_add_op(ADM_AMPAGENT_OP_MODINT_MID, name_op_MODINT);
	adm_add_op(ADM_AMPAGENT_OP_MODUINT_MID, name_op_MODUINT);
	adm_add_op(ADM_AMPAGENT_OP_MODVAST_MID, name_op_MODVAST);
	adm_add_op(ADM_AMPAGENT_OP_MODUVAST_MID, name_op_MODUVAST);
	adm_add_op(ADM_AMPAGENT_OP_MODREAL32_MID, name_op_MODREAL32);
	adm_add_op(ADM_AMPAGENT_OP_MODREAL64_MID, name_op_MODREAL64);
	adm_add_op(ADM_AMPAGENT_OP_^INT_MID, name_op_^INT);
	adm_add_op(ADM_AMPAGENT_OP_^UINT_MID, name_op_^UINT);
	adm_add_op(ADM_AMPAGENT_OP_^VAST_MID, name_op_^VAST);
	adm_add_op(ADM_AMPAGENT_OP_^UVAST_MID, name_op_^UVAST);
	adm_add_op(ADM_AMPAGENT_OP_^REAL32_MID, name_op_^REAL32);
	adm_add_op(ADM_AMPAGENT_OP_^REAL64_MID, name_op_^REAL64);
	adm_add_op(ADM_AMPAGENT_OP_&_MID, name_op_&);
	adm_add_op(ADM_AMPAGENT_OP_|_MID, name_op_|);
	adm_add_op(ADM_AMPAGENT_OP_#_MID, name_op_#);
	adm_add_op(ADM_AMPAGENT_OP_~_MID, name_op_~);
	adm_add_op(ADM_AMPAGENT_OP_&&_MID, name_op_&&);
	adm_add_op(ADM_AMPAGENT_OP_||_MID, name_op_||);
	adm_add_op(ADM_AMPAGENT_OP_!_MID, name_op_!);
	adm_add_op(ADM_AMPAGENT_OP_ABS_MID, name_op_abs);
	adm_add_op(ADM_AMPAGENT_OP_<_MID, name_op_<);
	adm_add_op(ADM_AMPAGENT_OP_>_MID, name_op_>);
	adm_add_op(ADM_AMPAGENT_OP_<=_MID, name_op_<=);
	adm_add_op(ADM_AMPAGENT_OP_>=_MID, name_op_>=);
	adm_add_op(ADM_AMPAGENT_OP_!=_MID, name_op_!=);
	adm_add_op(ADM_AMPAGENT_OP_==_MID, name_op_==);
	adm_add_op(ADM_AMPAGENT_OP_<<_MID, name_op_<<);
	adm_add_op(ADM_AMPAGENT_OP_>>_MID, name_op_>>);
	adm_add_op(ADM_AMPAGENT_OP_STOR_MID, name_op_STOR);
}

void adm_AmpAgent_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = lyst_create();
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NAME, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_VERSION, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMREPORTS, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_SENTREPORTS, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMTRL, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMSRL, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_RUNSRL, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMCONST, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMVARIABLES, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMMACROS, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_RUNMACROS, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMCONTROLS, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_RUNCONTROLS, ADM_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMRULES, ADM_ALLOC, &used));

	adm_add_rpt(ADM_AMPAGENT_RPT_FULLREPORT_MID, rpt);

	midcol_destroy(&rpt);

	names_add_name("ADM_AMPAGENT_RPT_FULLREPORT_MID", "This is all known meta-data, EDD, and VAR values known by the agent.", ADM_AMPAGENT, ADM_AMPAGENT_RPT_FULLREPORT_MID);


}

#endif // _HAVE_AMPAGENT_ADM_
