/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file nm_mgr_ui.h
 **
 **
 ** Description: A text-based DTNMP Manager. This manager provides the
 **              following functions associated with DTN network management:
 **
 **              1. Define and send custom report and macro definitions
 **              2. Provide tools to build all versions of MIDs and OIDs.
 **              3. Configure Agents for time- and state- based production
 **              4. Print data reports received from a DTNMP Agent
 **
 ** Notes:
 **		1. Currently we do not support ACLs.
 **		2. Currently, we do not support multiple DTNMP agents.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/18/13  E. Birrane     Code comments and cleanup (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  07/26/17  E. Birrane     Added batch testing. (JHU/APL)
 *****************************************************************************/
#ifndef _NM_MGR_UI_H
#define _NM_MGR_UI_H

#include "nm_mgr.h"
#include "agents.h"

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/report.h"



#define AGENT_ADD_VAR_STR "Add Var"
#define AGENT_DEL_VAR_STR "Del Var"
#define AGENT_ADD_RPTT_STR "Add Rptt"
#define AGENT_DEL_RPTT_STR "Del Rptt"
#define AGENT_ADD_MAC_STR "Add Mac"
#define AGENT_DEL_MAC_STR "Del Mac"
#define AGENT_ADD_SBR_STR "Add Sbr"
#define AGENT_DEL_SBR_STR "Del Sbr"
#define AGENT_ADD_TBR_STR "Add Tbr"
#define AGENT_DEL_TBR_STR "Del Tbr"





#define UI_MAIN_MENU  0
#define UI_ADMIN_MENU 1
#define UI_RPT_MENU   2
#define UI_CTRL_MENU  3
#define UI_DB_MENU	  4

extern int gContext;

void ui_build_control(agent_t* agent);
void ui_clear_reports(agent_t* agent);

rpttpl_t* ui_create_rpttpl_from_parms(tnvc_t parms);

void ui_deregister_agent();
void ui_event_loop();

void ui_list_objs();

void ui_postprocess_ctrl(ari_t *id);

void ui_register_agent();

agent_t *ui_select_agent();

void ui_send_file(agent_t* agent, uint8_t enter_ts);

void ui_send_raw(agent_t* agent, uint8_t enter_ts);

void ui_print_menu_main();
int ui_menu_admin_do(uint8_t choice);
void ui_menu_admin_show();
int ui_menu_ctrl_do(uint8_t choice);
void ui_menu_ctrl_show();

int ui_menu_rpt_do(uint8_t choice);
void ui_menu_rpt_show();

void ui_print_nop();
void *ui_thread(int *running);

#ifdef HAVE_MYSQL
void ui_menu_sql_do(uint8_t choice);
void ui_menu_sql_show();


void ui_db_conn();
void ui_db_disconn();
void ui_db_set_parms();
void ui_db_print_parms();
void ui_db_reset();
void ui_db_clear_rpt();
void ui_db_read();
void ui_db_write();

#endif


#endif // _NM_MGR_UI_H
