/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/


/*****************************************************************************
 **
 ** File Name: adm_agent.c
 **
 **
 ** Description: This file implements the public aspects of an AMP Agent ADM.
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


#include "ion.h"
#include "platform.h"


#include "../adm/adm_agent.h"
#include "../utils/utils.h"
#include "../primitives/def.h"
#include "../primitives/nn.h"
#include "../primitives/report.h"
#include "../primitives/blob.h"

#ifdef AGENT_ROLE
#include "../../agent/adm_agent_impl.h"
#include "../../agent/rda.h"
#include "../../agent/instr.h"

#else
#include "../../mgr/nm_mgr_names.h"
#include "../../mgr/nm_mgr_ui.h"
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
	adm_add_datadef(ADM_AGENT_AD_NUMRPT_MID,  AMP_TYPE_UINT, 0, adm_agent_get_num_rpt,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_SENTRPT_MID, AMP_TYPE_UINT, 0, adm_agent_get_sent_rpt, NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMTRL_MID,  AMP_TYPE_UINT, 0, adm_agent_get_num_trl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNTRL_MID,  AMP_TYPE_UINT, 0, adm_agent_get_run_trl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMSRL_MID,  AMP_TYPE_UINT, 0, adm_agent_get_num_srl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNSRL_MID,  AMP_TYPE_UINT, 0, adm_agent_get_run_srl,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMLIT_MID,  AMP_TYPE_UINT, 0, adm_agent_get_num_lit,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMCUST_MID, AMP_TYPE_UINT, 0, adm_agent_get_num_var,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMMAC_MID,  AMP_TYPE_UINT, 0, adm_agent_get_num_mac,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNMAC_MID,  AMP_TYPE_UINT, 0, adm_agent_get_run_mac,  NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_NUMCTRL_MID, AMP_TYPE_UINT, 0, adm_agent_get_num_ctrl, NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_RUNCTRL_MID, AMP_TYPE_UINT, 0, adm_agent_get_run_ctrl, NULL, NULL);
	adm_add_datadef(ADM_AGENT_AD_CURTIME_MID, AMP_TYPE_TS,   0, adm_agent_get_curtime, NULL, NULL);

#else
	adm_add_datadef(ADM_AGENT_AD_NUMRPT_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMRPT_MID", "# Reports", ADM_AGENT, ADM_AGENT_AD_NUMRPT_MID);

	adm_add_datadef(ADM_AGENT_AD_SENTRPT_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_SENTRPT_MID", "# Sent Reports", ADM_AGENT, ADM_AGENT_AD_SENTRPT_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMTRL_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMTRL_MID", "# Time-Based Rules", ADM_AGENT, ADM_AGENT_AD_NUMTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNTRL_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNTRL_MID", "# Time-Based Rules Run", ADM_AGENT, ADM_AGENT_AD_RUNTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMSRL_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMSRL_MID", "# State-Based Rules", ADM_AGENT, ADM_AGENT_AD_NUMSRL_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNSRL_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNSRL_MID", "# State-Based Rules Run", ADM_AGENT, ADM_AGENT_AD_RUNSRL_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMLIT_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMLIT_MID", "# Literals", ADM_AGENT, ADM_AGENT_AD_NUMLIT_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMCUST_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMCUST_MID", "# Custom Definitions", ADM_AGENT, ADM_AGENT_AD_NUMCUST_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMMAC_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMMAC_MID", "# Macros", ADM_AGENT, ADM_AGENT_AD_NUMMAC_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNMAC_MID,  AMP_TYPE_UINT, 0, NULL,  NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNMAC_MID", "# Macros Run", ADM_AGENT, ADM_AGENT_AD_RUNMAC_MID);

	adm_add_datadef(ADM_AGENT_AD_NUMCTRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_NUMCTRL_MID", "# Controls", ADM_AGENT, ADM_AGENT_AD_NUMCTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_RUNCTRL_MID, AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_RUNCTRL_MID", "# Controls Run", ADM_AGENT, ADM_AGENT_AD_RUNCTRL_MID);

	adm_add_datadef(ADM_AGENT_AD_CURTIME_MID, AMP_TYPE_TS,   0, NULL, NULL, NULL);
	names_add_name("ADM_AGENT_AD_CURTIME_MID", "Agent Time", ADM_AGENT, ADM_AGENT_AD_CURTIME_MID);

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

	expr_t *expr = expr_create(AMP_TYPE_UINT, def);

	adm_add_computeddef(ADM_AGENT_CD_NUMRULE_MID, AMP_TYPE_EXPR, expr);
	expr_release(expr);

//	midcol_destroy(&def);

#ifndef AGENT_ROLE
	adm_add_datadef(ADM_AGENT_CD_NUMRULE_MID, AMP_TYPE_UINT, 1, NULL, adm_print_unsigned_long, adm_size_unsigned_long);
	names_add_name("ADM_AGENT_CD_NUMRULE_MID", "Total # Rules", ADM_AGENT, ADM_AGENT_CD_NUMRULE_MID);

#endif


}


void adm_agent_init_controls()
{

#ifdef AGENT_ROLE
	adm_add_ctrl(ADM_AGENT_CTL_LSTADM_MID, adm_agent_ctl_adm_lst);

    adm_add_ctrl(ADM_AGENT_CTL_ADDCD_MID,  adm_agent_ctl_var_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELCD_MID,  adm_agent_ctl_var_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTCD_MID,  adm_agent_ctl_var_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCCD_MID,  adm_agent_ctl_var_dsc);

    adm_add_ctrl(ADM_AGENT_CTL_ADDRPT_MID, adm_agent_ctl_rptt_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELRPT_MID, adm_agent_ctl_rptt_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTRPT_MID, adm_agent_ctl_rptt_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCRPT_MID, adm_agent_ctl_rptt_dsc);
    adm_add_ctrl(ADM_AGENT_CTL_GENRPT_MID, adm_agent_ctl_rpt_gen);

	adm_add_ctrl(ADM_AGENT_CTL_ADDMAC_MID,  adm_agent_ctl_mac_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELMAC_MID,  adm_agent_ctl_mac_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTMAC_MID,  adm_agent_ctl_mac_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCMAC_MID,  adm_agent_ctl_mac_dsc);

	adm_add_ctrl(ADM_AGENT_CTL_ADDTRL_MID,  adm_agent_ctl_trl_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELTRL_MID,  adm_agent_ctl_trl_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTTRL_MID,  adm_agent_ctl_trl_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCTRL_MID,  adm_agent_ctl_trl_dsc);

	adm_add_ctrl(ADM_AGENT_CTL_ADDSRL_MID,  adm_agent_ctl_srl_add);
    adm_add_ctrl(ADM_AGENT_CTL_DELSRL_MID,  adm_agent_ctl_srl_del);
    adm_add_ctrl(ADM_AGENT_CTL_LSTSRL_MID,  adm_agent_ctl_srl_lst);
    adm_add_ctrl(ADM_AGENT_CTL_DSCSRL_MID,  adm_agent_ctl_srl_dsc);

    adm_add_ctrl(ADM_AGENT_CTL_STOR_MID,    adm_agent_ctl_stor);

    adm_add_ctrl(ADM_AGENT_CTL_RESET_CNTS,  adm_agent_ctl_reset_cnt);

#else
	adm_add_ctrl(ADM_AGENT_CTL_LSTADM_MID, NULL);
	names_add_name("ADM_AGENT_CTL_LSTADM_MID", "List ADMs", ADM_AGENT, ADM_AGENT_CTL_LSTADM_MID);
//	ui_add_parmspec(ADM_AGENT_CTL_LSTADM_MID, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_ADDCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDCD_MID", "Add Computed Definition", ADM_AGENT, ADM_AGENT_CTL_ADDCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDCD_MID, 3, "Name", AMP_TYPE_MID, "Expression", AMP_TYPE_EXPR, "Type", AMP_TYPE_SDNV, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DELCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELCD_MID", "Delete Computed Definition", ADM_AGENT, ADM_AGENT_CTL_DELCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELCD_MID, 1, "MC to Delete", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTCD_MID", "List Computed Definition", ADM_AGENT, ADM_AGENT_CTL_LSTCD_MID);
//	ui_add_parmspec(ADM_AGENT_CTL_LSTCD_MID, 0, 0, 0, 0, 0, 0);

	adm_add_ctrl(ADM_AGENT_CTL_DSCCD_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCCD_MID", "Describe Computed Definition", ADM_AGENT, ADM_AGENT_CTL_DSCCD_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCCD_MID, 1, "MC to Describe", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_ADDRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_ADDRPT_MID", "Add Report", ADM_AGENT, ADM_AGENT_CTL_ADDRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDRPT_MID, 2, "Report Name", AMP_TYPE_MID, "Rpt Def.", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DELRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_DELRPT_MID", "Delete Report", ADM_AGENT, ADM_AGENT_CTL_DELRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELRPT_MID, 1, "Rpts To Del", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_LSTRPT_MID", "List Report", ADM_AGENT, ADM_AGENT_CTL_LSTRPT_MID);
//	ui_add_parmspec(ADM_AGENT_CTL_LSTRPT_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCRPT_MID, NULL);
	names_add_name("ADM_AGENT_CTL_DSCRPT_MID", "Describe Report", ADM_AGENT, ADM_AGENT_CTL_DSCRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCRPT_MID, 1, "Rpts to Descr.", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_GENRPT_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_GENRPT_MID", "Generate Report", ADM_AGENT, ADM_AGENT_CTL_GENRPT_MID);
	ui_add_parmspec(ADM_AGENT_CTL_GENRPT_MID, 2, "Rpts to Gen.", AMP_TYPE_MC, "Mgr to Send", AMP_TYPE_DC, NULL, 0, NULL, 0, NULL, 0);

	adm_add_ctrl(ADM_AGENT_CTL_ADDMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDMAC_MID", "Add Macro", ADM_AGENT, ADM_AGENT_CTL_ADDMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDMAC_MID, 3, "Name", AMP_TYPE_STRING, "MID ID", AMP_TYPE_MID, "Macro Def", AMP_TYPE_MC, NULL, 0, NULL, 0);

	adm_add_ctrl(ADM_AGENT_CTL_DELMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELMAC_MID", "Delete Macro", ADM_AGENT, ADM_AGENT_CTL_DELMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELMAC_MID, 1, "Macros to Del", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTMAC_MID", "List Macro", ADM_AGENT, ADM_AGENT_CTL_LSTMAC_MID);
//	ui_add_parmspec(ADM_AGENT_CTL_LSTMAC_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCMAC_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCMAC_MID", "Describe Macro", ADM_AGENT, ADM_AGENT_CTL_DSCMAC_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCMAC_MID, 1, "Macros to Desc", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	adm_add_ctrl(ADM_AGENT_CTL_ADDTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDTRL_MID", "Add Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_ADDTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDTRL_MID, 5, "TRL ID", AMP_TYPE_MID, "Timestamp", AMP_TYPE_SDNV, "Period", AMP_TYPE_SDNV, "Count", AMP_TYPE_SDNV, "Action", AMP_TYPE_MC);

    adm_add_ctrl(ADM_AGENT_CTL_DELTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELTRL_MID", "Delete Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DELTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELTRL_MID, 1, "TRLs to Del", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTTRL_MID", "List Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_LSTTRL_MID);
//	ui_add_parmspec(ADM_AGENT_CTL_LSTTRL_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCTRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCTRL_MID", "Describe Time-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DSCTRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCTRL_MID, 1, "TRLs to Desc", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	adm_add_ctrl(ADM_AGENT_CTL_ADDSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_ADDSRL_MID", "Add State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_ADDSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_ADDSRL_MID, 5, "SRL ID", AMP_TYPE_MID, "Timestamp", AMP_TYPE_SDNV, "State Expr", AMP_TYPE_EXPR, "Count", AMP_TYPE_SDNV, "Action", AMP_TYPE_MC);

    adm_add_ctrl(ADM_AGENT_CTL_DELSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DELSRL_MID", "Delete State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DELSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DELSRL_MID, 1, "SRLs to Del", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_LSTSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_LSTSRL_MID", "List State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_LSTSRL_MID);
//	ui_add_parmspec(ADM_AGENT_CTL_LSTSRL_MID, 0, 0, 0, 0, 0, 0);

    adm_add_ctrl(ADM_AGENT_CTL_DSCSRL_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_DSCSRL_MID", "Describe State-Based Rule", ADM_AGENT, ADM_AGENT_CTL_DSCSRL_MID);
	ui_add_parmspec(ADM_AGENT_CTL_DSCSRL_MID, 1, "SRLs to Desc", AMP_TYPE_MC, NULL, 0, NULL, 0, NULL, 0, NULL, 0);


    adm_add_ctrl(ADM_AGENT_CTL_STOR_MID,  NULL);
	names_add_name("ADM_AGENT_CTL_STOR_MID", "Store EXPR to CD", ADM_AGENT, ADM_AGENT_CTL_STOR_MID);
	ui_add_parmspec(ADM_AGENT_CTL_STOR_MID, 2, "CD MID", AMP_TYPE_MID, "Expression", AMP_TYPE_EXPR, NULL, 0, NULL, 0, NULL, 0);

    adm_add_ctrl(ADM_AGENT_CTL_RESET_CNTS,  NULL);
	names_add_name("ADM_AGENT_CTL_RESET_CNTS", "Reset Counts", ADM_AGENT, ADM_AGENT_CTL_RESET_CNTS);

#endif

}


void adm_agent_init_literals()
{
	/* Step 7: Register Literals */
    value_t result;

    result.type = AMP_TYPE_UINT;
    result.value.as_uint = 1348025776;
//    result.length = sizeof(uint32_t);

    adm_add_lit(ADM_AGENT_LIT_EPOCH_MID, result, NULL);

    result.type = AMP_TYPE_UNK;
    result.value.as_uint = 0;
 //   result.length = 0;

    adm_add_lit(ADM_AGENT_LIT_USRVAST_MID,  result, adm_agent_user_vast);
    adm_add_lit(ADM_AGENT_LIT_USRUVAST_MID, result, adm_agent_user_uvast);
    adm_add_lit(ADM_AGENT_LIT_USRFLT_MID,   result, adm_agent_user_float);
    adm_add_lit(ADM_AGENT_LIT_USRDBL_MID,   result, adm_agent_user_double);
    adm_add_lit(ADM_AGENT_LIT_USRSTR_MID,   result, adm_agent_user_string);
    adm_add_lit(ADM_AGENT_LIT_USRBLOB_MID,  result, adm_agent_user_blob);

#ifndef AGENT_ROLE
	names_add_name("ADM_AGENT_LIT_EPOCH_MID", "DTNMP EPOCH", ADM_AGENT, ADM_AGENT_LIT_EPOCH_MID);

	names_add_name("ADM_AGENT_LIT_USRVAST_MID", "User-Defined Signed Vast", ADM_AGENT, ADM_AGENT_LIT_USRVAST_MID);
	ui_add_parmspec(ADM_AGENT_LIT_USRVAST_MID, 1, "8 byte VAST", AMP_TYPE_VAST, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	names_add_name("ADM_AGENT_LIT_USRUVAST_MID", "User-Defined Unsigned Vast", ADM_AGENT, ADM_AGENT_LIT_USRUVAST_MID);
	ui_add_parmspec(ADM_AGENT_LIT_USRUVAST_MID, 1, "UVAST", AMP_TYPE_SDNV, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	names_add_name("ADM_AGENT_LIT_USRFLT_MID", "User-Defined Float", ADM_AGENT, ADM_AGENT_LIT_USRFLT_MID);
	ui_add_parmspec(ADM_AGENT_LIT_USRFLT_MID, 1, "Serialized Float", AMP_TYPE_BLOB, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	names_add_name("ADM_AGENT_LIT_USRDBL_MID", "User-Defined Double", ADM_AGENT, ADM_AGENT_LIT_USRDBL_MID);
	ui_add_parmspec(ADM_AGENT_LIT_USRDBL_MID, 1, "Serialized Double", AMP_TYPE_BLOB, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	names_add_name("ADM_AGENT_LIT_USRSTR_MID", "User-Defined String", ADM_AGENT, ADM_AGENT_LIT_USRSTR_MID);
	ui_add_parmspec(ADM_AGENT_LIT_USRSTR_MID, 1, "Value", AMP_TYPE_STRING, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

	names_add_name("ADM_AGENT_LIT_USRBLOB_MID", "User-Defined Blob", ADM_AGENT, ADM_AGENT_LIT_USRBLOB_MID);
	ui_add_parmspec(ADM_AGENT_LIT_USRSTR_MID, 1, "Value", AMP_TYPE_BLOB, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

#endif
}


void adm_agent_init_macros()
{
	uint32_t used = 0;

	/* Step 8: Register Macros. */
	Lyst macro = lyst_create();
	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_LSTCD_MID, ADM_MID_ALLOC, &used));

	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_LSTRPT_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_LSTMAC_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_LSTTRL_MID, ADM_MID_ALLOC, &used));
	lyst_insert_last(macro,mid_deserialize_str(ADM_AGENT_CTL_LSTSRL_MID, ADM_MID_ALLOC, &used));

	adm_add_macro(ADM_AGENT_MAC_FULL_MID, macro);

	midcol_destroy(&macro);

#ifndef AGENT_ROLE
	names_add_name("ADM_AGENT_MAC_FULL_MID", "List User Data", ADM_AGENT, ADM_AGENT_MAC_FULL_MID);
#endif

}


void adm_agent_init_metadata()
{
	/* Step 1: Register Nicknames */

	oid_nn_add_parm(AGENT_ADM_MD_NN_IDX,   AGENT_ADM_MD_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_AD_NN_IDX,   AGENT_ADM_AD_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_CD_NN_IDX,   AGENT_ADM_CD_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_RPT_NN_IDX,  AGENT_ADM_RPT_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_CTRL_NN_IDX, AGENT_ADM_CTRL_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_LTRL_NN_IDX, AGENT_ADM_LTRL_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_MAC_NN_IDX,  AGENT_ADM_MAC_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_OP_NN_IDX,   AGENT_ADM_OP_NN_STR, "AGENT","v0.1");
	oid_nn_add_parm(AGENT_ADM_ROOT_NN_IDX,   AGENT_ADM_ROOT_NN_STR, "AGENT","v0.1");


	/* Step 2: Register Metadata Information. */
#ifdef AGENT_ROLE
	adm_add_datadef(ADM_AGENT_MD_NAME_MID, AMP_TYPE_STRING, 0, adm_agent_md_name, adm_print_string, adm_size_string);
	adm_add_datadef(ADM_AGENT_MD_VER_MID,  AMP_TYPE_STRING, 0, adm_agent_md_ver,  adm_print_string, adm_size_string);
#else
	adm_add_datadef(ADM_AGENT_MD_NAME_MID, AMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_AGENT_MD_NAME_MID", "Agent Name", ADM_AGENT, ADM_AGENT_MD_NAME_MID);

	adm_add_datadef(ADM_AGENT_MD_VER_MID,  AMP_TYPE_STRING, 0, NULL, adm_print_string, adm_size_string);
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
	adm_add_op(ADM_AGENT_OP_LSFHT_MID,  2, adm_agent_op_lshft);
	adm_add_op(ADM_AGENT_OP_RSHFT_MID,  2, adm_agent_op_rshft);
	adm_add_op(ADM_AGENT_OP_ASSGN_MID,  2, adm_agent_op_stor);


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
	adm_add_op(ADM_AGENT_OP_LSFHT_MID,  2, NULL);
	adm_add_op(ADM_AGENT_OP_RSHFT_MID,  2, NULL);
	adm_add_op(ADM_AGENT_OP_ASSGN_MID,  2, NULL);

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
	names_add_name("ADM_AGENT_OP_LSFHT_MID", "<<", ADM_AGENT, ADM_AGENT_OP_LSFHT_MID);
	names_add_name("ADM_AGENT_OP_RSHFT_MID", ">>", ADM_AGENT, ADM_AGENT_OP_RSHFT_MID);
	names_add_name("ADM_AGENT_OP_ASSGN_MID", "=", ADM_AGENT, ADM_AGENT_OP_ASSGN_MID);

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


value_t adm_agent_user_vast(mid_t *id)
{
	value_t result;
	int8_t success = 0;
	vast tmp = 0;

	result.type = AMP_TYPE_UNK;
	result.value.as_vast = 0;

	if(id == NULL)
	{
		return result;
	}

	tmp = adm_extract_vast(id->oid.params, 0, &success);
	if(success <= 0)
	{
		AMP_DEBUG_ERR("adm_agent_user_int","Can't grab first parameter.",NULL);
		return result;
	}

	result.type = AMP_TYPE_VAST;
	result.value.as_vast = tmp;

	return result;
}

value_t adm_agent_user_uvast(mid_t *id)
{
	value_t result;
	int8_t success = 0;
	uvast tmp = 0;

	result.type = AMP_TYPE_UNK;
	result.value.as_vast = 0;

	if(id == NULL)
	{
		return result;
	}

	tmp = adm_extract_sdnv(id->oid.params, 0, &success);

	if(success <= 0)
	{
		AMP_DEBUG_ERR("adm_agent_user_uvast","Can't grab first parameter.",NULL);
		return result;
	}

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = tmp;

	return result;
}

value_t adm_agent_user_float(mid_t *id)
{
	value_t result;
	int8_t success = 0;
	blob_t *entry = NULL;

	result.type = AMP_TYPE_UNK;
	result.value.as_vast = 0;

	if(id == NULL)
	{
		return result;
	}

	if((entry = adm_extract_blob(id->oid.params, 0, &success)))
	{
		if(entry->length <=  sizeof(float))
		{
			result.type = AMP_TYPE_REAL32;
			/* \todo watch network byte order. */
			memcpy(&(result.value.as_real32), entry->value, MIN( sizeof(float), entry->length));
		}
		else
		{
			AMP_DEBUG_ERR("adm_agent_user_float","User length %d > Data len %d.",entry->length,  sizeof(float));
		}

		blob_destroy(entry, 1);
		return result;
	}

	AMP_DEBUG_ERR("adm_agent_user_float","No user data found.",NULL);
	return result;
}

value_t adm_agent_user_double(mid_t *id)
{
	value_t result;
	int8_t success = 0;
	blob_t *entry = NULL;

	result.type = AMP_TYPE_UNK;
	result.value.as_real64 = 0;

	if(id == NULL)
	{
		return result;
	}

	if((entry = adm_extract_blob(id->oid.params, 0, &success)))
	{
		if(entry->length <= sizeof(double))
		{
			result.type = AMP_TYPE_REAL64;
			/* \todo watch network byte order. */
			memcpy(&(result.value.as_real64), entry->value, MIN(sizeof(double), entry->length));
		}
		else
		{
			AMP_DEBUG_ERR("adm_agent_user_double","User length %d > Data len %d.",entry->length, sizeof(double));
		}

		blob_destroy(entry, 1);
		return result;
	}

	AMP_DEBUG_ERR("adm_agent_user_double","No user data found.",NULL);
	return result;
}

value_t adm_agent_user_string(mid_t *id)
{
	value_t result;
	int8_t success = 0;
	char *str = NULL;

	result.type = AMP_TYPE_UNK;
	result.value.as_real64 = 0;

	if(id == NULL)
	{
		return result;
	}

	if((str = adm_extract_string(id->oid.params, 0, &success)))
	{
		result.type = AMP_TYPE_STRING;
		result.value.as_ptr = (uint8_t *) str;
	}
	else
	{
		AMP_DEBUG_ERR("adm_agent_user_string","No user data found.",NULL);
	}

	return result;
}

value_t adm_agent_user_blob(mid_t *id)
{
	value_t result;
	int8_t success = 0;
	blob_t *entry = NULL;

	result.type = AMP_TYPE_UNK;
	result.value.as_real64 = 0;

	if(id == NULL)
	{
		return result;
	}

	if((entry = adm_extract_blob(id->oid.params, 0, &success)) != NULL)
	{
		result.type = AMP_TYPE_BLOB;
		result.value.as_ptr = (uint8_t *) entry;
	}
	else
	{
		AMP_DEBUG_ERR("adm_agent_user_blob","No user data found.",NULL);
	}

	return result;
}

