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

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/mid.h"
#include "../shared/primitives/report.h"



#define UI_MAIN_MENU  0
#define UI_ADMIN_MENU 1
#define UI_RPT_MENU   2
#define UI_CTRL_MENU  3
#define UI_DB_MENU	  4

extern int gContext;
extern Lyst gParmSpec;


#define MAX_PARMS 5
#define MAX_PARM_NAME 16


#define UI_ADD_PARMSPEC_1(str, n1, p1) \
	   ui_add_parmspec(str, 1, n1, p1, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

#define UI_ADD_PARMSPEC_2(str, n1, p1, n2, p2) \
	   ui_add_parmspec(str, 2, n1, p1, n2, p2, NULL, 0, NULL, 0, NULL, 0);

#define UI_ADD_PARMSPEC_3(str, n1, p1, n2, p2, n3, p3) \
	   ui_add_parmspec(str, 3, n1, p1, n2, p2, n3, p3, NULL, 0, NULL, 0);

#define UI_ADD_PARMSPEC_4(str, n1, p1, n2, p2, n3, p3, n4, p4) \
	   ui_add_parmspec(str, 4, n1, p1, n2, p2, n3, p3, n4, p4, NULL, 0);

#define UI_ADD_PARMSPEC_5(str, n1, p1, n2, p2, n3, p3, n4, p4, n5, p5) \
	   ui_add_parmspec(str, 5, n1, p1, n2, p2, n3, p3, n4, p4, n5, p5);


/*
 * The parameter spec keeps a list of known parameters
 * for individual, known parameterized MIDs.
 *
 * Currently, only controls and literals can be parameterized.
 */
typedef struct
{
	mid_t *mid;
	uint8_t num_parms;
	uint8_t parm_type[MAX_PARMS];
	char parm_name[MAX_PARMS][MAX_PARM_NAME];
} ui_parm_spec_t;


void           ui_add_parmspec(char *mid_str,
						       uint8_t num,
		                       char *n1, uint8_t p1,
		                       char *n2, uint8_t p2,
		                       char *n3, uint8_t p3,
		                       char *n4, uint8_t p4,
		                       char *n5, uint8_t p5);

ui_parm_spec_t* ui_get_parmspec(mid_t *mid);


void ui_clear_reports(agent_t* agent);

agent_t *ui_select_agent();

// \todo: Be able to select multiple agents.
// \todo: Be able to have multiple commands in a command scratch area.

void ui_build_control(agent_t* agent);
void ui_send_raw(agent_t* agent, uint8_t enter_ts);
void ui_send_file(agent_t* agent, uint8_t enter_ts);


int ui_test_mid(mid_t *mid, const char *mid_str);

void ui_define_mid_params(char *name, ui_parm_spec_t* parmspec, mid_t *mid);

void ui_register_agent();
void ui_deregister_agent();

void ui_event_loop();


mid_t * ui_get_mid(int adm_type, int mid_id, uint32_t opt);


void ui_list_adms();
void ui_list_atomic();
void ui_list_compdef();
void ui_list_ctrls();
void ui_list_gen(int adm_type, int mid_id);
void ui_list_literals();
void ui_list_macros();
void ui_list_mids();
void ui_list_ops();
void ui_list_rpts();

void ui_postprocess_ctrl(mid_t *mid);

void ui_print_menu_admin();
void ui_print_menu_ctrl();
void ui_print_menu_main();
void ui_print_menu_rpt();

#ifdef HAVE_MYSQL
void ui_print_menu_db();

void ui_db_conn();
void ui_db_disconn();
void ui_db_set_parms();
void ui_db_print_parms();
void ui_db_reset();
void ui_db_clear_rpt();
void ui_db_read();
void ui_db_write();

#endif

void ui_print_nop();

void ui_run_tests();


void *ui_thread(int *running);

#endif // _NM_MGR_UI_H
