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

#define NM_LOG_FILE "nm_mgr.log"

#define AGENT_ADD_VAR_STR "ADD_VAR"
#define AGENT_DEL_VAR_STR "DEL_VAR"
#define AGENT_ADD_RPTT_STR "ADD_RPTT"
#define AGENT_DEL_RPTT_STR "DEL_RPTT"
#define AGENT_ADD_MAC_STR "ADD_MAC"
#define AGENT_DEL_MAC_STR "DEL_MAC"
#define AGENT_ADD_SBR_STR "ADD_SBR"
#define AGENT_DEL_SBR_STR "DEL_SBR"
#define AGENT_ADD_TBR_STR "ADD_TBR"
#define AGENT_DEL_TBR_STR "DEL_TBR"





#define UI_MAIN_MENU  0
#define UI_ADMIN_MENU 1
#define UI_CTRL_MENU  2
#define UI_DB_MENU	  3

extern int gContext;

int ui_build_control(agent_t* agent);
void ui_clear_reports(agent_t* agent);

rpttpl_t* ui_create_rpttpl_from_parms(tnvc_t parms);
var_t* ui_create_var_from_parms(tnvc_t parms);

void ui_deregister_agent();
void ui_event_loop();

void ui_list_objs(uint8_t adm_id, uvast mask, ari_t **result);

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

void ui_print_nop();
void *ui_thread(int *running);

#ifdef HAVE_MYSQL
int ui_menu_sql_do(uint8_t choice);
void ui_menu_sql_show();


int ui_db_conn();
void ui_db_disconn();
void ui_db_set_parms();
void ui_db_print_parms();
int ui_db_reset();
int ui_db_clear_rpt();
void ui_db_read();
void ui_db_write();

#endif

/** NCURSES Style UI Helper API.  These functions will gracefully fallback to non-curses implementations if not available **/
// UI Helper Structures
typedef enum form_types_enum {
   TYPE_CHECK_NONE = 0,
   TYPE_CHECK_ALPHA,
   TYPE_CHECK_ALNUM,
   TYPE_CHECK_ENUM,
   TYPE_CHECK_INT,
   TYPE_CHECK_NUM,
   TYPE_CHECK_REGEXP
} form_types_enum;

typedef struct form_fields_t {
   char* title;
   char* value;
   uint32_t width; // Maximum length for value field (null-char included)
   int opts_off; // NCURSES Field Options to disable
   form_types_enum type; // If not NULL, use validation
   union args {
      // For ALPHA or ALNUM types
      int width; // Minimum length for value field (excluded null-char)
      struct en { // Enums
         char ** valuelist;
         int checkcase;
         int checkunique;
      } en;
      struct num { // Used for integer and numeric types
         int padding;
         int vmin;
         int vmax;
      } num;
      // Regex type
      char *regex;
   } args;
} form_fields_t;

typedef struct ui_menu_list_t
{
   char* name;
   char* description;
   char* data; /**< User data field */
} ui_menu_list_t;

/** UI Menu Options 
 *   Unless otherwise indicated, these options have no effect in no-curses mode.
 */
#define UI_OPT_AUTO_LABEL        0x1
#define UI_OPT_ENTER_SEL         0x2
#define UI_OPT_SPLIT_DESCRIPTION 0x4
#define UI_OPT_AUTOMATCH         0x8 // Not currently implemented
/** End UI Menu Options **/

#ifdef USE_NCURSES
#include "form.h"
#else
// Explicitly define missing macros (most of which will have no effect in this mode)
#define O_NULLOK 1
#define O_EDIT   2
#endif

typedef enum ui_cb_return_values_t
{
   UI_CB_RTV_ERR = -1, /**< Abort menu with error code. Any value <0 will be returned in this manner */
   UI_CB_RTV_CONTINUE = 0, /**< Continue displaying menu */
   UI_CB_RTV_STATUS = 1,  /**< Continue displaying menu with updated status message (if ui_menu_listing was given a status_msg buffer) */
   UI_CB_RTV_CHOICE = 2, /** Abort menu with current selection index as return value */
   
} ui_cb_return_values_t;

/** Callback function for ui_menu_listing
 * @param status_msg A copy of the status_msg buffer given to ui_menu_listing. If not NULL,
 *    callback may update the contents of this message and return 2 to indicate menu should
 *    refresh status message and continue.
 * TODO: Create an enum for the callback return values.
 */
typedef int (*ui_menu_listing_cb_fn)(int idx, int keypress, void* data, char* status_msg);

int ui_menu(char* title, char** choices, char** descriptions, int n_choices, char* msg);
int ui_form(char* title, char* msg, form_fields_t *fields, int num_fields);
int ui_prompt(char* title, char* choiceA, char* choiceB, char* choiceC);
int ui_menu_listing(char* title, ui_menu_list_t* list, int n_choices,
                    char* status_msg, int default_idx, char* usage_msg,
                    ui_menu_listing_cb_fn fn, int flags);
int ui_menu_select(char* title, const char* const* choices, const char* const* descriptions, int n_choices, char* msg, int menu_cols);

int ui_display_to_file(char* filename);
void ui_display_to_file_close();
void ui_printf(const char* format, ...);
int ui_display_exec();

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/** The following functions have alternate prototypes or macro definitions dependent on USE_NCURSES flag */
#ifdef USE_NCURSES
/* ui_dialog_win is the target for ui_printf, and is displayed with ui_display_show() */
void ui_init();
void ui_display_init(char* title);
#else
/* Without ncurses, ui_printf() adn ui_display() options become simple macro wrappers */
#define ui_init()
#define ui_display_init(title) ui_printf("\n\n--------------------\n%s\n--------------------\n", title)
#endif


#endif // _NM_MGR_UI_H
