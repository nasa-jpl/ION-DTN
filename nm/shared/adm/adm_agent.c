/*****************************************************************************
 **
 ** File Name: adm_agent.c
 **
 ** Description: This implements the public aspects of a DTNMP agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 *****************************************************************************/

#include "ion.h"
#include "platform.h"


#include "shared/adm/adm_agent.h"
#include "shared/utils/utils.h"
#include "shared/primitives/instr.h"
#include "shared/primitives/def.h"
#include "shared/primitives/nn.h"
#include "shared/primitives/report.h"

#ifdef AGENT_ROLE
#include "shared/adm/adm_agent_impl.h"
#include "rda.h"
#else
#include "mgr/nm_mgr_names.h"
#include "mgr/nm_mgr_ui.h"
#endif

void adm_agent_init()
{
	adm_agent_init_atomic();
	adm_agent_init_computed();
	adm_agent_init_controls();
	adm_agent_init_literals();
	adm_agent_init_macros();
	adm_agent_init_metadata();
	adm_agent_init_ops();
	adm_agent_init_reports();
}



void adm_agent_init_atomic()
{

#ifdef AGENT_ROLE
	adm_add_datadef(ADM_AGENT_AD_NUMRPT_MID,  DTNMP_TYPE_UINT, 0, agent_get_num_rpt,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_SENTRPT_MID, DTNMP_TYPE_UINT, 0, agent_get_sent_rpt, NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMTRL_MID,  DTNMP_TYPE_UINT, 0, agent_get_num_trl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNTRL_MID,  DTNMP_TYPE_UINT, 0, agent_get_run_trl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMSRL_MID,  DTNMP_TYPE_UINT, 0, agent_get_num_srl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNSRL_MID,  DTNMP_TYPE_UINT, 0, agent_get_run_srl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMLIT_MID,  DTNMP_TYPE_UINT, 0, agent_get_num_lit,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMCUST_MID, DTNMP_TYPE_UINT, 0, agent_get_num_cust, NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMMAC_MID,  DTNMP_TYPE_UINT, 0, agent_get_num_mac,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNMAC_MID,  DTNMP_TYPE_UINT, 0, agent_get_run_mac,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMCTRL_MID, DTNMP_TYPE_UINT, 0, agent_get_num_ctrl, NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNCTRL_MID, DTNMP_TYPE_UINT, 0, agent_get_run_ctrl, NULL, NULL);
#else
	adm_add_datadef(ADM_AGENT_AD_NUMRPT_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMRPT_MID", "# Reports", ADM_AGENT, ADM_AGENT_AD_NUMRPT_MID);

	adm_add_datadef(ADM_AGENT_AD_SENTRPT_MID, DTNMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_SENTRPT_MID", "# Sent Reports", ADM_AGENT, ADM_AGENT_AD_SENTRPT_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMTRL_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMTRL_MID", "# Time-Based Rules", ADM_AGENT, ADM_AGENT_AD_NUMTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNTRL_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNTRL_MID", "# Time-Based Rules Run", ADM_AGENT, ADM_AGENT_AD_RUNTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMSRL_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMSRL_MID", "# State-Based Rules", ADM_AGENT, ADM_AGENT_AD_NUMSRL_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNSRL_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNSRL_MID", "# State-Based Rules Run", ADM_AGENT, ADM_AGENT_AD_RUNSRL_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMLIT_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMLIT_MID", "# Literals", ADM_AGENT, ADM_AGENT_AD_NUMLIT_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMCUST_MID, DTNMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMCUST_MID", "# Custom Definitions", ADM_AGENT, ADM_AGENT_AD_NUMCUST_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMMAC_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMMAC_MID", "# Macros", ADM_AGENT, ADM_AGENT_AD_NUMMAC_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNMAC_MID,  DTNMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNMAC_MID", "# Macros Run", ADM_AGENT, ADM_AGENT_AD_RUNMAC_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMCTRL_MID, DTNMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMCTRL_MID", "# Controls", ADM_AGENT, ADM_AGENT_AD_NUMCTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNCTRL_MID, DTNMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNCTRL_MID", "# Controls Run", ADM_AGENT, ADM_AGENT_AD_RUNCTRL_MID);

#endif

}


void adm_agent_init_computed()
{
	uint32_t used = 0;

    // NUMRULE = #_TRL  #_SRL  +
	Lyst def = lyst_create();
	lyst_insert_last(def,mid_deserialize_str(ADM_AGENT_AD_NUMTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(def,mid_deserialize_str(ADM_AGENT_AD_NUMSRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(def,mid_deserialize_str(ADM_AGENT_OP_PLUS_MID, ADM_MID_ALLOC, &used));

	adm_add_computeddef(ADM_AGENT_CD_NUMRULE_MID, DTNMP_TYPE_UINT, def);

//	midcol_destroy(&def);

#ifndef AGENT_ROLE
	names_add_name("ADM_AGENT_CD_NUMRULE_MID", "Total # Rules", ADM_AGENT, ADM_AGENT_CD_NUMRULE_MID);
#endif


}


void adm_agent_init_controls()
{

#ifdef AGENT_ROLE
	adm_add_ctrl(ADM_AGENT_CTL_LSTADM_MID, agent_ctl_adm_lst);

    adm_add_ctrl(ADM_AGENT_CTL_ADDCD_MID,  agent_ctl_cd_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELCD_MID,  agent_ctl_cd_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTCD_MID,  agent_ctl_cd_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCCD_MID,  agent_ctl_cd_dsc);

    adm_add_ctrl(ADM_AGENT_CTL_ADDRPT_MID, agent_ctl_rpt_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELRPT_MID, agent_ctl_rpt_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTRPT_MID, agent_ctl_rpt_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCRPT_MID, agent_ctl_rpt_dsc);
    adm_add_ctrl(ADM_AGENT_CTL_GENRPT_MID, agent_ctl_rpt_gen);

	adm_add_ctrl(ADM_AGENT_CTL_ADDMAC_MID,  agent_ctl_mac_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELMAC_MID,  agent_ctl_mac_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTMAC_MID,  agent_ctl_mac_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCMAC_MID,  agent_ctl_mac_dsc);

	adm_add_ctrl(ADM_AGENT_CTL_ADDTRL_MID,  agent_ctl_trl_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELTRL_MID,  agent_ctl_trl_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTTRL_MID,  agent_ctl_trl_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCTRL_MID,  agent_ctl_trl_dsc);

	adm_add_ctrl(ADM_AGENT_CTL_ADDSRL_MID,  agent_ctl_srl_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELSRL_MID,  agent_ctl_srl_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTSRL_MID,  agent_ctl_srl_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCSRL_MID,  agent_ctl_srl_dsc);

#else
    ui_parm_spec_t spec;

	adm_add_ctrl(ADM_AGENT_CTL_LSTADM_MID, NULL);
	names_add_name("ADM_AGENT_CTL_LSTADM_MID", "List ADMs", ADM_AGENT, ADM_AGENT_CTL_LSTADM_MID);
	ui_add_parmspec(ADM_AGENT_CTL_LSTADM_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_ADDCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDCD_MID", "Add Computed Definition", ADM_AGENT, ADM_AGENT_CTL_ADDCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDCD_MID, 3, DTNMP_TYPE_MID, DTNMP_TYPE_EXPR, DTNMP_TYPE_SDNV, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DELCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELCD_MID", "Delete Computed Definition", ADM_AGENT, ADM_AGENT_CTL_DELCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELCD_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTCD_MID", "List Computed Definition", ADM_AGENT, ADM_AGENT_CTL_LSTCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_LSTCD_MID, 0, 0, 0, 0, 0, 0);

	adm_add_ctrl(ADM_AGENT_CTL_DSCCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCCD_MID", "Describe Computed Definition", ADM_AGENT, ADM_AGENT_CTL_DSCCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCCD_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_ADDRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_ADDRPT_MID", "Add Report", ADM_AGENT, ADM_AGENT_CTL_ADDRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDRPT_MID, 2, DTNMP_TYPE_MID, DTNMP_TYPE_MC, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DELRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_DELRPT_MID", "Delete Report", ADM_AGENT, ADM_AGENT_CTL_DELRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELRPT_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_LSTRPT_MID", "List Report", ADM_AGENT, ADM_AGENT_CTL_LSTRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_LSTRPT_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_DSCRPT_MID", "Describe Report", ADM_AGENT, ADM_AGENT_CTL_DSCRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCRPT_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_GENRPT_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_GENRPT_MID", "Generate Report", ADM_AGENT, ADM_AGENT_CTL_GENRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_GENRPT_MID, 2, DTNMP_TYPE_MC, DTNMP_TYPE_DC, 0, 0, 0);

	adm_add_ctrl(ADM_AGENT_CTL_ADDMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDMAC_MID", "Add Macro", ADM_AGENT, ADM_AGENT_CTL_ADDMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDMAC_MID, 3, DTNMP_TYPE_STRING, DTNMP_TYPE_MID, DTNMP_TYPE_MC, 0, 0);

	adm_add_ctrl(ADM_AGENT_CTL_DELMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELMAC_MID", "Delete Macro", ADM_AGENT, ADM_AGENT_CTL_DELMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELMAC_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTMAC_MID", "List Macro", ADM_AGENT, ADM_AGENT_CTL_LSTMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_LSTMAC_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCMAC_MID", "Describe Macro", ADM_AGENT, ADM_AGENT_CTL_DSCMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCMAC_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

	adm_add_ctrl(ADM_AGENT_CTL_ADDTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDTRL_MID", "Add Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_ADDTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDTRL_MID, 5, DTNMP_TYPE_MID, DTNMP_TYPE_TS, DTNMP_TYPE_SDNV, DTNMP_TYPE_SDNV, DTNMP_TYPE_MC);

    adm_add_ctrl(ADM_AGENT_CTL_DELTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELTRL_MID", "Delete Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DELTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELTRL_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTTRL_MID", "List Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_LSTTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_LSTTRL_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCTRL_MID", "Describe Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DSCTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCTRL_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

	adm_add_ctrl(ADM_AGENT_CTL_ADDSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDSRL_MID", "Add State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_ADDSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDSRL_MID, 5, DTNMP_TYPE_MID, DTNMP_TYPE_TS, DTNMP_TYPE_EXPR, DTNMP_TYPE_SDNV, DTNMP_TYPE_MC);

    adm_add_ctrl(ADM_AGENT_CTL_DELSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELSRL_MID", "Delete State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DELSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELSRL_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTSRL_MID", "List State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_LSTSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_LSTSRL_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCSRL_MID", "Describe State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DSCSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCSRL_MID, 1, DTNMP_TYPE_MC, 0, 0, 0, 0);

#endif

}


void adm_agent_init_literals()
{
	/* Step 7: Register Literals */
    value_t result;

    result.type = DTNMP_TYPE_UINT;
    result.value.as_uint = 1348025776;
    result.length = sizeof(uint32_t);

    adm_add_lit(ADM_AGENT_LIT_EPOCH_MID, result, NULL);

    result.type = DTNMP_TYPE_UNK;
    result.value.as_uint = 0;
    result.length = 0;

    adm_add_lit(ADM_AGENT_LIT_USRINT_MID,  result, adm_agent_user_int);
    adm_add_lit(ADM_AGENT_LIT_USRUINT_MID, result, adm_agent_user_uint);
    adm_add_lit(ADM_AGENT_LIT_USRFLT_MID,  result, adm_agent_user_float);
    adm_add_lit(ADM_AGENT_LIT_USRDBL_MID,  result, adm_agent_user_double);
    adm_add_lit(ADM_AGENT_LIT_USRSTR_MID,  result, adm_agent_user_string);
    adm_add_lit(ADM_AGENT_LIT_USRBLOB_MID, result, adm_agent_user_blob);

#ifndef AGENT_ROLE
	names_add_name("ADM_AGENT_LIT_EPOCH_MID", "DTNMP EPOCH", ADM_AGENT, ADM_AGENT_LIT_EPOCH_MID);
	names_add_name("ADM_AGENT_LIT_USRINT_MID", "User-Defined Signed Integer", ADM_AGENT, ADM_AGENT_LIT_USRINT_MID);
	names_add_name("ADM_AGENT_LIT_USRUINT_MID", "User-Defined Unsigned Integer", ADM_AGENT, ADM_AGENT_LIT_USRUINT_MID);
	names_add_name("ADM_AGENT_LIT_USRFLT_MID", "User-Defined Float", ADM_AGENT, ADM_AGENT_LIT_USRFLT_MID);
	names_add_name("ADM_AGENT_LIT_USRDBL_MID", "User-Defined Double", ADM_AGENT, ADM_AGENT_LIT_USRDBL_MID);
	names_add_name("ADM_AGENT_LIT_USRSTR_MID", "User-Defined String", ADM_AGENT, ADM_AGENT_LIT_USRSTR_MID);
	names_add_name("ADM_AGENT_LIT_USRBLOB_MID", "User-Defined Blob", ADM_AGENT, ADM_AGENT_LIT_USRBLOB_MID);
#endif
}


void adm_agent_init_macros()
{
	uint32_t used = 0;

	/* Step 8: Register Macros. */
	Lyst macro = lyst_create();
	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_LSTADM_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_DSCAD_MID, ADM_MID_ALLOC, &used));

	adm_add_macro(ADM_AGENT_MAC_FULL_MID, macro);

	midcol_destroy(&macro);

#ifndef AGENT_ROLE
	names_add_name("ADM_AGENT_MAC_FULL_MID", "List All ADMs", ADM_AGENT, ADM_AGENT_MAC_FULL_MID);
#endif

}


void adm_agent_init_metadata()
{
	/* Step 1: Register Nicknames */

	oid_nn_add_parm(AGENT_ADM_MD_NN_IDX,   AGENT_ADM_MD_NN_STR);
	oid_nn_add_parm(AGENT_ADM_AD_NN_IDX,   AGENT_ADM_AD_NN_STR);
	oid_nn_add_parm(AGENT_ADM_CD_NN_IDX,   AGENT_ADM_CD_NN_STR);
	oid_nn_add_parm(AGENT_ADM_RPT_NN_IDX,  AGENT_ADM_RPT_NN_STR);
	oid_nn_add_parm(AGENT_ADM_CTRL_NN_IDX, AGENT_ADM_CTRL_NN_STR);
	oid_nn_add_parm(AGENT_ADM_LTRL_NN_IDX, AGENT_ADM_LTRL_NN_STR);
	oid_nn_add_parm(AGENT_ADM_MAC_NN_IDX,  AGENT_ADM_MAC_NN_STR);
	oid_nn_add_parm(AGENT_ADM_OP_NN_IDX,   AGENT_ADM_OP_NN_STR);
	oid_nn_add_parm(AGENT_ADM_ROOT_NN_IDX,   AGENT_ADM_ROOT_NN_STR);


	/* Step 2: Register Metadata Information. */
#ifdef AGENT_ROLE
	adm_add_datadef(ADM_AGENT_MD_NAME_MID, DTNMP_TYPE_STRING, 0, agent_md_name, adm_print_string, adm_size_string);
	adm_add_datadef(ADM_AGENT_MD_VER_MID,  DTNMP_TYPE_STRING, 0, agent_md_ver,  adm_print_string, adm_size_string);
#else
	adm_add_datadef(ADM_AGENT_MD_NAME_MID, DTNMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AGENT_MD_NAME_MID", "Agent Name", ADM_AGENT, ADM_AGENT_MD_NAME_MID);

	adm_add_datadef(ADM_AGENT_MD_VER_MID,  DTNMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AGENT_MD_VER_MID", "Agent Version", ADM_AGENT, ADM_AGENT_MD_VER_MID);
#endif

}


void adm_agent_init_ops()
{
	/* Step 9: Register Ops. */
#ifdef AGENT_ROLE
	adm_add_op(ADM_AGENT_OP_PLUS_MID,   2, adm_agent_op_plus);
	adm_add_op(ADM_AGENT_OP_MINUS_MID,  2, adm_agent_op_minus);
	adm_add_op(ADM_AGENT_OP_MULT_MID,   2, adm_agent_op_mult);
	adm_add_op(ADM_AGENT_OP_DIV_MID,    2, adm_agent_op_div);
	adm_add_op(ADM_AGENT_OP_MOD_MID,    2, adm_agent_op_mod);
	adm_add_op(ADM_AGENT_OP_EXP_MID,    2, adm_agent_op_exp);
	adm_add_op(ADM_AGENT_OP_BITAND_MID, 2, adm_agent_op_bitand);
	adm_add_op(ADM_AGENT_OP_BITOR_MID,  2, adm_agent_op_bitor);
	adm_add_op(ADM_AGENT_OP_BITXOR_MID, 2, adm_agent_op_bitxor);
	adm_add_op(ADM_AGENT_OP_BITNOT_MID, 1, adm_agent_op_bitnot);
	adm_add_op(ADM_AGENT_OP_LOGAND_MID, 2, adm_agent_op_logand);
	adm_add_op(ADM_AGENT_OP_LOGOR_MID,  2, adm_agent_op_logor);
	adm_add_op(ADM_AGENT_OP_LOGNOT_MID, 1, adm_agent_op_lognot);
	adm_add_op(ADM_AGENT_OP_ABS_MID,    1, adm_agent_op_abs);
	adm_add_op(ADM_AGENT_OP_LT_MID,     2, adm_agent_op_lt);
	adm_add_op(ADM_AGENT_OP_GT_MID,     2, adm_agent_op_gt);
	adm_add_op(ADM_AGENT_OP_LTE_MID,    2, adm_agent_op_lte);
	adm_add_op(ADM_AGENT_OP_GTE_MID,    2, adm_agent_op_gte);
	adm_add_op(ADM_AGENT_OP_NEQ_MID,    2, adm_agent_op_neq);
	adm_add_op(ADM_AGENT_OP_EQ_MID,     2, adm_agent_op_eq);
#else
	adm_add_op(ADM_AGENT_OP_PLUS_MID,   2, NULL);
	adm_add_op(ADM_AGENT_OP_MINUS_MID,  2, NULL);
	adm_add_op(ADM_AGENT_OP_MULT_MID,   2, NULL);
	adm_add_op(ADM_AGENT_OP_DIV_MID,    2, NULL);
	adm_add_op(ADM_AGENT_OP_MOD_MID,    2, NULL);
	adm_add_op(ADM_AGENT_OP_EXP_MID,    2, NULL);
	adm_add_op(ADM_AGENT_OP_BITAND_MID, 2, NULL);
	adm_add_op(ADM_AGENT_OP_BITOR_MID,  2, NULL);
	adm_add_op(ADM_AGENT_OP_BITXOR_MID, 2, NULL);
	adm_add_op(ADM_AGENT_OP_BITNOT_MID, 1, NULL);
	adm_add_op(ADM_AGENT_OP_LOGAND_MID, 2, NULL);
	adm_add_op(ADM_AGENT_OP_LOGOR_MID,  2, NULL);
	adm_add_op(ADM_AGENT_OP_LOGNOT_MID, 1, NULL);
	adm_add_op(ADM_AGENT_OP_ABS_MID,    1, NULL);
	adm_add_op(ADM_AGENT_OP_LT_MID,     2, NULL);
	adm_add_op(ADM_AGENT_OP_GT_MID,     2, NULL);
	adm_add_op(ADM_AGENT_OP_LTE_MID,    2, NULL);
	adm_add_op(ADM_AGENT_OP_GTE_MID,    2, NULL);
	adm_add_op(ADM_AGENT_OP_NEQ_MID,    2, NULL);
	adm_add_op(ADM_AGENT_OP_EQ_MID,     2, NULL);

	names_add_name("ADM_AGENT_OP_PLUS_MID", "+", ADM_AGENT, ADM_AGENT_OP_PLUS_MID);
	names_add_name("ADM_AGENT_OP_MINUS_MID", "-", ADM_AGENT, ADM_AGENT_OP_MINUS_MID);
	names_add_name("ADM_AGENT_OP_MULT_MID", "*", ADM_AGENT, ADM_AGENT_OP_MULT_MID);
	names_add_name("ADM_AGENT_OP_DIV_MID", "/", ADM_AGENT, ADM_AGENT_OP_DIV_MID);
	names_add_name("ADM_AGENT_OP_MOD_MID", "%", ADM_AGENT, ADM_AGENT_OP_MOD_MID);
	names_add_name("ADM_AGENT_OP_EXP_MID", "^", ADM_AGENT, ADM_AGENT_OP_EXP_MID);
	names_add_name("ADM_AGENT_OP_BITAND_MID", "&", ADM_AGENT, ADM_AGENT_OP_BITAND_MID);
	names_add_name("ADM_AGENT_OP_BITOR_MID", "|", ADM_AGENT, ADM_AGENT_OP_BITOR_MID);
	names_add_name("ADM_AGENT_OP_BITXOR_MID", "#", ADM_AGENT, ADM_AGENT_OP_BITXOR_MID);
	names_add_name("ADM_AGENT_OP_BITNOT_MID", "!", ADM_AGENT, ADM_AGENT_OP_BITNOT_MID);
	names_add_name("ADM_AGENT_OP_LOGAND_MID", "&&", ADM_AGENT, ADM_AGENT_OP_LOGAND_MID);
	names_add_name("ADM_AGENT_OP_LOGOR_MID", "||", ADM_AGENT, ADM_AGENT_OP_LOGOR_MID);
	names_add_name("ADM_AGENT_OP_LOGNOT_MID", "!", ADM_AGENT, ADM_AGENT_OP_LOGNOT_MID);
	names_add_name("ADM_AGENT_OP_ABS_MID", "abs", ADM_AGENT, ADM_AGENT_OP_ABS_MID);
	names_add_name("ADM_AGENT_OP_LT_MID", "<", ADM_AGENT, ADM_AGENT_OP_LT_MID);
	names_add_name("ADM_AGENT_OP_GT_MID", ">", ADM_AGENT, ADM_AGENT_OP_GT_MID);
	names_add_name("ADM_AGENT_OP_LTE_MID", "<=", ADM_AGENT, ADM_AGENT_OP_LTE_MID);
	names_add_name("ADM_AGENT_OP_GTE_MID", ">=", ADM_AGENT, ADM_AGENT_OP_GTE_MID);
	names_add_name("ADM_AGENT_OP_NEQ_MID", "!=", ADM_AGENT, ADM_AGENT_OP_NEQ_MID);
	names_add_name("ADM_AGENT_OP_EQ_MID", "==", ADM_AGENT, ADM_AGENT_OP_EQ_MID);

#endif
}



void adm_agent_init_reports()
{
	uint32_t used = 0;
	Lyst rpt = lyst_create();

	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_MD_NAME_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_MD_VER_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMRPT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_SENTRPT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_RUNTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMSRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_RUNSRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMLIT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMCUST_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMMAC_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_RUNMAC_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_NUMCTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_AD_RUNCTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(rpt,mid_deserialize_str(ADM_AGENT_CD_NUMRULE_MID, ADM_MID_ALLOC, &used));

	adm_add_rpt(ADM_AGENT_RPT_FULL_MID, rpt);

	midcol_destroy(&rpt);

#ifndef AGENT_ROLE
	names_add_name("ADM_AGENT_RPT_FULL_MID", "Full Report", ADM_AGENT, ADM_AGENT_RPT_FULL_MID);
#endif

}



value_t adm_agent_user_int(mid_t *id)
{
	value_t result;
	datacol_entry_t *entry = mid_get_param(id, 0);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_vast = 0;

	if(entry != NULL)
	{
		if(entry->length <= result.length)
		{
			result.type = DTNMP_TYPE_VAST;
			result.length = sizeof(vast);
			/* \todo watch network byte order. */
			memcpy(&(result.value.as_vast), entry->value, MIN(result.length, entry->length));
		}
		else
		{
			DTNMP_DEBUG_ERR("adm_agent_user_int","User length %d > Data len %d.",entry->length, result.length);
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_user_int","No user data found.",NULL);
	}

	return result;
}

value_t adm_agent_user_uint(mid_t *id)
{
	value_t result;
	datacol_entry_t *entry = mid_get_param(id, 0);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_vast = 0;

	if(entry != NULL)
	{
		if(entry->length <= result.length)
		{
			result.type = DTNMP_TYPE_UVAST;
			result.length = sizeof(uvast);
			/* \todo watch network byte order. */
			memcpy(&(result.value.as_uvast), entry->value, MIN(result.length, entry->length));
		}
		else
		{
			DTNMP_DEBUG_ERR("adm_agent_user_uint","User length %d > Data len %d.",entry->length, result.length);
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_user_uint","No user data found.",NULL);
	}

	return result;
}

value_t adm_agent_user_float(mid_t *id)
{
	value_t result;
	datacol_entry_t *entry = mid_get_param(id, 0);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_vast = 0;

	if(entry != NULL)
	{
		if(entry->length <= result.length)
		{
			result.type = DTNMP_TYPE_REAL32;
			result.length = sizeof(float);
			/* \todo watch network byte order. */
			memcpy(&(result.value.as_real32), entry->value, MIN(result.length, entry->length));
		}
		else
		{
			DTNMP_DEBUG_ERR("adm_agent_user_float","User length %d > Data len %d.",entry->length, result.length);
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_user_float","No user data found.",NULL);
	}

	return result;
}

value_t adm_agent_user_double(mid_t *id)
{
	value_t result;
	datacol_entry_t *entry = mid_get_param(id, 0);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_real64 = 0;

	if(entry != NULL)
	{
		if(entry->length <= result.length)
		{
			result.type = DTNMP_TYPE_REAL64;
			result.length = sizeof(double);
			/* \todo watch network byte order. */
			memcpy(&(result.value.as_real64), entry->value, MIN(result.length, entry->length));
		}
		else
		{
			DTNMP_DEBUG_ERR("adm_agent_user_double","User length %d > Data len %d.",entry->length, result.length);
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_user_double","No user data found.",NULL);
	}

	return result;
}

value_t adm_agent_user_string(mid_t *id)
{
	value_t result;
	datacol_entry_t *entry = mid_get_param(id, 0);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_real64 = 0;

	if(entry != NULL)
	{
		if((result.value.as_ptr = (uint8_t*) MTAKE(entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("adm_agent_user_string","Cannot take %d bytes.", entry->length);
			DTNMP_DEBUG_EXIT("adm_agent_user_string","-> unknown.",NULL);
			return result;
		}

		result.length = entry->length;
		result.type = DTNMP_TYPE_STRING;
		/* \todo watch network byte order. */
		memcpy(&(result.value.as_ptr), entry->value, result.length);
	}
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_user_string","No user data found.",NULL);
	}

	return result;
}

value_t adm_agent_user_blob(mid_t *id)
{
	value_t result;
	datacol_entry_t *entry = mid_get_param(id, 0);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_real64 = 0;

	if(entry != NULL)
	{
		if((result.value.as_ptr = (uint8_t*) MTAKE(entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("adm_agent_user_blob","Cannot take %d bytes.", entry->length);
			DTNMP_DEBUG_EXIT("adm_agent_user_blob","-> unknown.",NULL);
			return result;
		}

		result.length = entry->length;
		result.type = DTNMP_TYPE_BLOB;
		/* \todo watch network byte order. */
		memcpy(&(result.value.as_ptr), entry->value, result.length);
	}
	else
	{
		DTNMP_DEBUG_ERR("adm_agent_user_blob","No user data found.",NULL);
	}

	return result;
}

