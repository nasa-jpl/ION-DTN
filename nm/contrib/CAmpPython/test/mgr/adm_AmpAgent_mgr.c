/****************************************************************************
 **
 ** File Name: ./mgr/adm_AmpAgent_mgr.c
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
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"
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
}

void adm_AmpAgent_init_edd()
{
	adm_add_edd(ADM_AMPAGENT_EDD_NUMREPORTS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMREPORTS_MID", "This is the number of reports known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMREPORTS_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_SENTREPORTS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_SENTREPORTS_MID", "This is the number of reports sent by the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_SENTREPORTS_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_NUMTRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMTRL_MID", "This is the number of time-based rules running on the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMTRL_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_RUNTRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_RUNTRL_MID", "This is the number of time-based rules run by the agent since the last reset.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_RUNTRL_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_NUMSRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMSRL_MID", "This is the number of state-based rules running on the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMSRL_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_RUNSRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_RUNSRL_MID", "This is the number of state-based rules run by the agent since the last reset.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_RUNSRL_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_NUMCONST_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMCONST_MID", "This is the number of constants known by the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMCONST_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_NUMVAR_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMVAR_MID", "This is the number of variables known by the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMVAR_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_NUMMACROS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMMACROS_MID", "This is the number of macros known by the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMMACROS_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_RUNMACROS_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_RUNMACROS_MID", "This is the number of macros run by the agent since the last reset.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_RUNMACROS_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_NUMCTRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_NUMCTRL_MID", "This is the number of controls known by the agent.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_NUMCTRL_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_RUNCTRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_RUNCTRL_MID", "This is the number of controls run by the agent since the last reset.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_RUNCTRL_MID);

	adm_add_edd(ADM_AMPAGENT_EDD_CURTIME_MID, AMP_TYPE_TS, 0, NULL, NULL, NULL);
	names_add_name("ADM_AMPAGENT_EDD_CURTIME_MID", "This is the current system time.", ADM_AMPAGENT, ADM_AMPAGENT_EDD_CURTIME_MID);

}

void adm_AmpAgent_init_variables()
{
	adm_add_var(ADM_AMPAGENT_VAR_NUMRULES_MID, NULL);
		names_add_name("ADM_AMPAGENT_VAR_NUMRULES_MID", "This is the number of rules known to the Agent(#TRL + #SRl).", ADM_AMPAGENT, ADM_AMPAGENT_VAR_NUMRULES_MID);
}

void adm_AmpAgent_init_controls()
{
	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTADMS_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_LISTADMS_MID", "This control causes the Agent to produce a report entry detailing the name of each ADM supported by the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_LISTADMS_MID);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDVAR_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_ADDVAR_MID", "This control configures a new variable definition on the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_ADDVAR_MID);
	UI_ADD_PARMSPEC_4(ADM_AMPAGENT_CTRL_ADDVAR_MID, "id", AMP_TYPE_MID, "def", AMP_TYPE_EXPR, "type", AMP_TYPE_BYTE, "flg", AMP_TYPE_BYTE);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELVAR_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DELVAR_MID", "This control removes one or more variable definitions from the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DELVAR_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DELVAR_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTVAR_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_LISTVAR_MID", "This control produces a listing of every variable identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_LISTVAR_MID);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCVAR_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DESCVAR_MID", "This control produces a detailed description of one or more variable identifier(MID)s known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DESCVAR_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DESCVAR_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDRPTTPL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_ADDRPTTPL_MID", "This control configures a new report template definition on the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_ADDRPTTPL_MID);
	UI_ADD_PARMSPEC_2(ADM_AMPAGENT_CTRL_ADDRPTTPL_MID, "id", AMP_TYPE_MID, "template", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELRPTTPL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DELRPTTPL_MID", "This control removes one or more report template definitions from the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DELRPTTPL_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DELRPTTPL_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTRPTTPL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_LISTRPTTPL_MID", "This control produces a listing of every report template identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_LISTRPTTPL_MID);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCRPTTPL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DESCRPTTPL_MID", "This control produces a detailed description of one or more report template  identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DESCRPTTPL_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DESCRPTTPL_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_GENRPTS_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_GENRPTS_MID", "This control causes the Agent to produce a report entry for each identified report templates and send them to one or more identified managers(MIDs).", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_GENRPTS_MID);
	UI_ADD_PARMSPEC_2(ADM_AMPAGENT_CTRL_GENRPTS_MID, "ids", AMP_TYPE_MC, "rxmgrs", AMP_TYPE_DC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDMACRO_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_ADDMACRO_MID", "This control configures a new macro definition on the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_ADDMACRO_MID);
	UI_ADD_PARMSPEC_3(ADM_AMPAGENT_CTRL_ADDMACRO_MID, "name", AMP_TYPE_STR, "id", AMP_TYPE_MID, "def", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELMACRO_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DELMACRO_MID", "This control removes one or more macro definitions from the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DELMACRO_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DELMACRO_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTMACRO_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_LISTMACRO_MID", "This control produces a listing of every macro identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_LISTMACRO_MID);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCMACRO_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DESCMACRO_MID", "This control produces a detailed description of one or more macro identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DESCMACRO_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DESCMACRO_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDTRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_ADDTRL_MID", "This control configures a new time-based rule(TRL) definition on the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_ADDTRL_MID);
	UI_ADD_PARMSPEC_5(ADM_AMPAGENT_CTRL_ADDTRL_MID, "id", AMP_TYPE_MID, "start", AMP_TYPE_TS, "period", AMP_TYPE_INT, "count", AMP_TYPE_INT, "action", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELTRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DELTRL_MID", "This control removes one or more TRL definitions from the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DELTRL_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DELTRL_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTTRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_LISTTRL_MID", "This control produces a listing of every TRL identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_LISTTRL_MID);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCTRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DESCTRL_MID", "This control produces a detailed description of one or more TRL identifier(MID)s known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DESCTRL_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DESCTRL_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_ADDSRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_ADDSRL_MID", "This control configures a new state-based rule(SRL) definition on the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_ADDSRL_MID);
	UI_ADD_PARMSPEC_5(ADM_AMPAGENT_CTRL_ADDSRL_MID, "id", AMP_TYPE_MID, "start", AMP_TYPE_TS, "state", AMP_TYPE_PRED, "cnt", AMP_TYPE_INT, "action", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DELSRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DELSRL_MID", "This control removes one or more SRL definitions from the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DELSRL_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DELSRL_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_LISTSRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_LISTSRL_MID", "This control produces a listing of every macro identifier(MID) known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_LISTSRL_MID);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_DESCSRL_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_DESCSRL_MID", "This control produces a detailed description of one or more SRL identifier(MID)s known to the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_DESCSRL_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_DESCSRL_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_STOREVAR_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_STOREVAR_MID", "This control stores variables.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_STOREVAR_MID);
	UI_ADD_PARMSPEC_1(ADM_AMPAGENT_CTRL_STOREVAR_MID, "ids", AMP_TYPE_MC);

	adm_add_ctrl(ADM_AMPAGENT_CTRL_RESETCOUNTS_MID, NULL);
	names_add_name("ADM_AMPAGENT_CTRL_RESETCOUNTS_MID", "This control resets all Agent ADM statistics reported in the Agent ADM report.", ADM_AMPAGENT, ADM_AMPAGENT_CTRL_RESETCOUNTS_MID);


}

void adm_AmpAgent_init_constants()
{
	adm_add_const(ADM_AMPAGENT_CONST_AMPEPOCH_MID, NULL);
		names_add_name("ADM_AMPAGENT_CONST_AMPEPOCH_MID", "This constant is the time epoch for the Agent.", ADM_AMPAGENT, ADM_AMPAGENT_CONST_AMPEPOCH_MID);


}

void adm_AmpAgent_init_macros()
{
	adm_add_macro(ADM_AMPAGENT_MACRO_USERLIST_MID, NULL);
		names_add_name("ADM_AMPAGENT_MACRO_USERLIST_MID", "This macro lists all of the user defined data.", ADM_AMPAGENT, ADM_AMPAGENT_MACOR_USERLIST_MID);


}

void adm_AmpAgent_init_metadata()
{
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_AMPAGENT_META_NAME_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AMPAGENT_META_NAME_MID", "The human-readable name of the ADM.", ADM_AMPAGENT, ADM_AMPAGENT_META_NAME_MID);
	adm_add_edd(ADM_AMPAGENT_META_NAMESPACE_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AMPAGENT_META_NAMESPACE_MID", "The namespace of the ADM.", ADM_AMPAGENT, ADM_AMPAGENT_META_NAMESPACE_MID);
	adm_add_edd(ADM_AMPAGENT_META_VERSION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AMPAGENT_META_VERSION_MID", "The version of the ADM.", ADM_AMPAGENT, ADM_AMPAGENT_META_VERSION_MID);
	adm_add_edd(ADM_AMPAGENT_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AMPAGENT_META_ORGANIZATION_MID", "The name of the issuing organization of the ADM.", ADM_AMPAGENT, ADM_AMPAGENT_META_ORGANIZATION_MID);

}

void adm_AmpAgent_init_ops()
{
	adm_add_op(ADM_AMPAGENT_OP_+INT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_+INT_MID", "Int32 addition", ADM_AMPAGENT, ADM_AMPAGENT_OP_+INT_MID);

	adm_add_op(ADM_AMPAGENT_OP_+UINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_+UINT_MID", "Unsigned int32 addition", ADM_AMPAGENT, ADM_AMPAGENT_OP_+UINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_+VAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_+VAST_MID", "Int64 addition", ADM_AMPAGENT, ADM_AMPAGENT_OP_+VAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_+UVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_+UVAST_MID", "Unsigned int64 addition", ADM_AMPAGENT, ADM_AMPAGENT_OP_+UVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_+REAL32_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_+REAL32_MID", "Real32 addition", ADM_AMPAGENT, ADM_AMPAGENT_OP_+REAL32_MID);

	adm_add_op(ADM_AMPAGENT_OP_+REAL64_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_+REAL64_MID", "Real64 addition", ADM_AMPAGENT, ADM_AMPAGENT_OP_+REAL64_MID);

	adm_add_op(ADM_AMPAGENT_OP_-INT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_-INT_MID", "Int32 subtraction", ADM_AMPAGENT, ADM_AMPAGENT_OP_-INT_MID);

	adm_add_op(ADM_AMPAGENT_OP_-UINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_-UINT_MID", "Unsigned int32 subtraction", ADM_AMPAGENT, ADM_AMPAGENT_OP_-UINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_-VAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_-VAST_MID", "Int64 subtraction", ADM_AMPAGENT, ADM_AMPAGENT_OP_-VAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_-UVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_-UVAST_MID", "Unsigned int64 subtraction", ADM_AMPAGENT, ADM_AMPAGENT_OP_-UVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_-REAL32_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_-REAL32_MID", "Real32 subtraction", ADM_AMPAGENT, ADM_AMPAGENT_OP_-REAL32_MID);

	adm_add_op(ADM_AMPAGENT_OP_-REAL64_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_-REAL64_MID", "Real64 subtraction", ADM_AMPAGENT, ADM_AMPAGENT_OP_-REAL64_MID);

	adm_add_op(ADM_AMPAGENT_OP_*INT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_*INT_MID", "Int multiplication", ADM_AMPAGENT, ADM_AMPAGENT_OP_*INT_MID);

	adm_add_op(ADM_AMPAGENT_OP_*UINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_*UINT_MID", "Unsigned int32 multiplication", ADM_AMPAGENT, ADM_AMPAGENT_OP_*UINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_*VAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_*VAST_MID", "Int64 multiplication", ADM_AMPAGENT, ADM_AMPAGENT_OP_*VAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_*UVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_*UVAST_MID", "Unsigned int64 multiplication", ADM_AMPAGENT, ADM_AMPAGENT_OP_*UVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_*REAL32_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_*REAL32_MID", "Real32 multiplication", ADM_AMPAGENT, ADM_AMPAGENT_OP_*REAL32_MID);

	adm_add_op(ADM_AMPAGENT_OP_*REAL64_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_*REAL64_MID", "Real64 multiplication", ADM_AMPAGENT, ADM_AMPAGENT_OP_*REAL64_MID);

	adm_add_op(ADM_AMPAGENT_OP_/INT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_/INT_MID", "Int32 division", ADM_AMPAGENT, ADM_AMPAGENT_OP_/INT_MID);

	adm_add_op(ADM_AMPAGENT_OP_/UINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_/UINT_MID", "Unsigned int32 division", ADM_AMPAGENT, ADM_AMPAGENT_OP_/UINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_/VAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_/VAST_MID", "Int64 division", ADM_AMPAGENT, ADM_AMPAGENT_OP_/VAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_/UVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_/UVAST_MID", "Unsigned int64 division", ADM_AMPAGENT, ADM_AMPAGENT_OP_/UVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_/REAL32_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_/REAL32_MID", "Real32 division", ADM_AMPAGENT, ADM_AMPAGENT_OP_/REAL32_MID);

	adm_add_op(ADM_AMPAGENT_OP_/REAL64_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_/REAL64_MID", "Real64 division", ADM_AMPAGENT, ADM_AMPAGENT_OP_/REAL64_MID);

	adm_add_op(ADM_AMPAGENT_OP_MODINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_MODINT_MID", "Int32 modulus division", ADM_AMPAGENT, ADM_AMPAGENT_OP_MODINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_MODUINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_MODUINT_MID", "Unsigned int32 modulus division", ADM_AMPAGENT, ADM_AMPAGENT_OP_MODUINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_MODVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_MODVAST_MID", "Int64 modulus division", ADM_AMPAGENT, ADM_AMPAGENT_OP_MODVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_MODUVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_MODUVAST_MID", "Unsigned int64 modulus division", ADM_AMPAGENT, ADM_AMPAGENT_OP_MODUVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_MODREAL32_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_MODREAL32_MID", "Real32 modulus division", ADM_AMPAGENT, ADM_AMPAGENT_OP_MODREAL32_MID);

	adm_add_op(ADM_AMPAGENT_OP_MODREAL64_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_MODREAL64_MID", "Real64 modulus division", ADM_AMPAGENT, ADM_AMPAGENT_OP_MODREAL64_MID);

	adm_add_op(ADM_AMPAGENT_OP_^INT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_^INT_MID", "Int32 exponentiation", ADM_AMPAGENT, ADM_AMPAGENT_OP_^INT_MID);

	adm_add_op(ADM_AMPAGENT_OP_^UINT_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_^UINT_MID", "Unsigned int32 exponentiation", ADM_AMPAGENT, ADM_AMPAGENT_OP_^UINT_MID);

	adm_add_op(ADM_AMPAGENT_OP_^VAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_^VAST_MID", "Int64 exponentiation", ADM_AMPAGENT, ADM_AMPAGENT_OP_^VAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_^UVAST_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_^UVAST_MID", "Unsigned int64 exponentiation", ADM_AMPAGENT, ADM_AMPAGENT_OP_^UVAST_MID);

	adm_add_op(ADM_AMPAGENT_OP_^REAL32_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_^REAL32_MID", "Real32 exponentiation", ADM_AMPAGENT, ADM_AMPAGENT_OP_^REAL32_MID);

	adm_add_op(ADM_AMPAGENT_OP_^REAL64_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_^REAL64_MID", "Real64 exponentiation", ADM_AMPAGENT, ADM_AMPAGENT_OP_^REAL64_MID);

	adm_add_op(ADM_AMPAGENT_OP_&_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_&_MID", "Bitwise and", ADM_AMPAGENT, ADM_AMPAGENT_OP_&_MID);

	adm_add_op(ADM_AMPAGENT_OP_|_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_|_MID", "Bitwise or", ADM_AMPAGENT, ADM_AMPAGENT_OP_|_MID);

	adm_add_op(ADM_AMPAGENT_OP_#_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_#_MID", "Bitwise xor", ADM_AMPAGENT, ADM_AMPAGENT_OP_#_MID);

	adm_add_op(ADM_AMPAGENT_OP_~_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_~_MID", "Bitwise not", ADM_AMPAGENT, ADM_AMPAGENT_OP_~_MID);

	adm_add_op(ADM_AMPAGENT_OP_&&_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_&&_MID", "Logical and", ADM_AMPAGENT, ADM_AMPAGENT_OP_&&_MID);

	adm_add_op(ADM_AMPAGENT_OP_||_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_||_MID", "Logical or", ADM_AMPAGENT, ADM_AMPAGENT_OP_||_MID);

	adm_add_op(ADM_AMPAGENT_OP_!_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_!_MID", "Logical not", ADM_AMPAGENT, ADM_AMPAGENT_OP_!_MID);

	adm_add_op(ADM_AMPAGENT_OP_ABS_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_ABS_MID", "absolute value", ADM_AMPAGENT, ADM_AMPAGENT_OP_ABS_MID);

	adm_add_op(ADM_AMPAGENT_OP_<_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_<_MID", "Less than", ADM_AMPAGENT, ADM_AMPAGENT_OP_<_MID);

	adm_add_op(ADM_AMPAGENT_OP_>_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_>_MID", "Greater than", ADM_AMPAGENT, ADM_AMPAGENT_OP_>_MID);

	adm_add_op(ADM_AMPAGENT_OP_<=_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_<=_MID", "Less than or equal to", ADM_AMPAGENT, ADM_AMPAGENT_OP_<=_MID);

	adm_add_op(ADM_AMPAGENT_OP_>=_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_>=_MID", "Greater than or equal to", ADM_AMPAGENT, ADM_AMPAGENT_OP_>=_MID);

	adm_add_op(ADM_AMPAGENT_OP_!=_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_!=_MID", "Not equal", ADM_AMPAGENT, ADM_AMPAGENT_OP_!=_MID);

	adm_add_op(ADM_AMPAGENT_OP_==_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_==_MID", "Equal to", ADM_AMPAGENT, ADM_AMPAGENT_OP_==_MID);

	adm_add_op(ADM_AMPAGENT_OP_<<_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_<<_MID", "Bitwise left shift", ADM_AMPAGENT, ADM_AMPAGENT_OP_<<_MID);

	adm_add_op(ADM_AMPAGENT_OP_>>_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_>>_MID", "Bitwise right shift", ADM_AMPAGENT, ADM_AMPAGENT_OP_>>_MID);

	adm_add_op(ADM_AMPAGENT_OP_STOR_MID, NULL);
	names_add_name("ADM_AMPAGENT_OP_STOR_MID", "Store value of parm 2 in parm 1", ADM_AMPAGENT, ADM_AMPAGENT_OP_STOR_MID);


}

void adm_AmpAgent_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = lyst_create();
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NAME_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_VERSION_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMREPORTS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_SENTREPORTS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMSRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_RUNSRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMCONST_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMVARIABLES_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMMACROS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_RUNMACROS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMCONTROLS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_RUNCONTROLS_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AMPAGENT_NUMRULES_MID, ADM_MID_ALLOC, &used));

	adm_add_rpt(ADM_AMPAGENT_RPT_FULLREPORT_MID, rpt);

	midcol_destroy(&rpt);

}

#endif // _HAVE_AMPAGENT_ADM_
