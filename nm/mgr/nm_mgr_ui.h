/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
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
 **  01/18/13  E. Birrane     Code comments and cleanup
 *****************************************************************************/
#include "nm_mgr.h"

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"
#include "shared/primitives/mid.h"


#define UI_MAIN_MENU  0
#define UI_ADMIN_MENU 1
#define UI_DEF_MENU   2
#define UI_RPT_MENU   3
#define UI_CTRL_MENU  4

extern int gContext;


mid_t *ui_build_mid(char *mid_str);

void ui_clear_reports(agent_t* agent);

agent_t *ui_select_agent();

void ui_construct_ctrl_by_idx(agent_t* agent);
void ui_construct_time_rule_by_idx(agent_t* agent);
void ui_construct_time_rule_by_mid(agent_t* agent);

void ui_define_macro(agent_t* agent);
void ui_define_report(agent_t* agent);
void ui_define_mid_params(char *name, int num_parms, mid_t *mid);

void ui_register_agent();
void ui_deregister_agent();

void ui_event_loop();

int ui_get_user_input(char *prompt, char **line, int max_len);

mid_t *ui_input_mid();
int ui_input_mid_flag(uint8_t *flag);

Lyst ui_parse_mid_str(char *mid_str, int max_idx, int type);

void ui_print_ctrls();
int ui_print_agents();
void ui_print_custom_rpt(rpt_data_entry_t *rpt_entry, def_gen_t *rpt_def);
void ui_print_menu_admin();
void ui_print_menu_ctrl();
void ui_print_menu_def();
void ui_print_menu_main();
void ui_print_menu_rpt();
void ui_print_mids();
void ui_print_predefined_rpt(mid_t *mid, uint8_t *data, uint64_t data_size, uint64_t *data_used, adm_datadef_t *adu);
void ui_print_reports(agent_t *agent);

void ui_run_tests();


void *ui_thread(void * threadId);
