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
#include "../shared/primitives/rules.h"

#define NM_LOG_FILE "nm_mgr.log"

#define AGENT_ADD_VAR_STR "ADD_VAR"
#define AGENT_DEL_VAR_STR "DEL_VAR"
#define AGENT_ADD_RPTT_STR "ADD_RPTT"
#define AGENT_DEL_RPTT_STR "DEL_RPTT"
#define AGENT_ADD_MAC_STR "ADD_MACRO"
#define AGENT_DEL_MAC_STR "DEL_MACRO"
#define AGENT_ADD_SBR_STR "ADD_SBR"
#define AGENT_DEL_SBR_STR "DEL_SBR"
#define AGENT_ADD_TBR_STR "ADD_TBR"
#define AGENT_DEL_TBR_STR "DEL_TBR"





#define UI_MAIN_MENU  0
#define UI_ADMIN_MENU 1
#define UI_CTRL_MENU  2
#define UI_DB_MENU	  3


typedef enum mgr_ui_mode_enum {
   MGR_UI_STANDARD, // Standard Shell-Based UI
   MGR_UI_NCURSES, // NCURSES-Based UI (currently a compile-time flag mutually exclusive with MGR_UI_STANDARD)
   MGR_UI_AUTOMATOR, // Special Altenrative UI Optimized for Automation
} mgr_ui_mode_enum;
extern mgr_ui_mode_enum mgr_ui_mode;

extern int gContext;

int ui_build_control(agent_t* agent);
void ui_clear_reports(agent_t* agent);
void ui_clear_tables(agent_t* agent);

rpttpl_t* ui_create_rpttpl_from_parms(tnvc_t parms);
var_t* ui_create_var_from_parms(tnvc_t parms);
macdef_t *ui_create_macdef_from_parms(tnvc_t parms);
rule_t *ui_create_sbr_from_parms(tnvc_t parms);
rule_t *ui_create_tbr_from_parms(tnvc_t parms);


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
   TYPE_CHECK_REGEXP,
   TYPE_CHECK_BOOL,
} form_types_enum;

typedef struct form_fields_t {
   char* title;
   char* value;
   uint32_t width; // Maximum length for value field (null-char included)
   int opts_off; // NCURSES Field Options to disable
   form_types_enum type; // If not NULL, use validation
   void *parsed_value;
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

typedef struct ui_print_cfg_t
{
   FILE *fd;
#ifdef USE_CIVETWEB
   struct mg_connection *conn;
#endif
} ui_print_cfg_t;

#ifdef USE_CIVETWEB
#define INIT_UI_PRINT_CFG_FD(fd) {fd, NULL};
#define INIT_UI_PRINT_CFG_CONN(conn) { NULL, conn };
#else
#define INIT_UI_PRINT_CFG_FD(fd) {fd};
#endif

/** Callback function prototype for ui_menu_listing
 * @param[in] idx Index into the menu listing configuration for the currently selected item.
 * @param[in] keypress The key that the user pressed to make the current selection. 
 *    For NCURSES mode, this is any keypress that does not cause the menu to navigate or cancel.
 *    This is not applicable in stdio/fallback mode, where 0 will always be given here.
 * @param[in,out] data The optional user data field associated with the list menu definition.  
 * @param[in,out] status_msg A copy of the status_msg buffer given to ui_menu_listing. If not NULL,
 *    callback may update the contents of this message and return 2 to indicate menu should
 *    refresh status message and continue.
 * @returns ui_cb_return_values_t Return value determines the subsequent behavior of the menu.  See
 *    the type description and ui_menu_listing for details.
 */
typedef ui_cb_return_values_t (*ui_menu_listing_cb_fn)(int idx, int keypress, void* data, char* status_msg);

/** Display a menu of options for the user to select from 
 * @param title  A title to display for this menu
 * @param choices An array of strings used or the menu selection names.
 * @param descriptions An optional array of detailed descriptions for each menu item.
 * @param n_choices The number of choices to present. This must match the length of the choices
 *    array and, if present, the descriptions array.
 * @param msg An optional user-defined message to display at the bottom of the menu.
 *    This is intended for informational or error message from the previous action.
 * @return -1 if the user cancels the operation or an error occurs, the 0-based index into the 
 *    choices array representing the user selection otherwise.
 */
int ui_menu(char* title, char** choices, char** descriptions, int n_choices, char* msg);

/** Display a form of one or more fields for the user to fill out.
 * @param title A title to display for this form
 * @param msg An optional user-defined message to display at the bottom of the menu.
 * @param fields An array of field definitions.  See form_feilds_t for details.
 *    Note: NCURSES is required for full functionality, such as regex validation.
 * @param num_fields The number of elements in the fields array.
 * @returns 1 on submission, 0 if user cancelled input, or -1 on error.
 */
int ui_form(char* title, char* msg, form_fields_t *fields, int num_fields);

/** Display the user with a simple confirmation dialog with 1-3 options.
 *    Specify NULL or choiceB or C to suppress display of the corresponding choices.
 * @returns User selection
 */
int ui_prompt(char* title, char* choiceA, char* choiceB, char* choiceC);

/** Displays a configurable menu of options for the user to select from with optional callback function.
 * @param title  A title to display for this menu
 * @param list An array of ui_menu_list_t entry definitions.  Each entry defines an item name, an optional
 *    description, and an optional user data field that will be passed to the callback handler.
 * @param n_choices The number of items in the list
 * @param msg An optional user-defined message to display at the bottom of the menu.
 *    This is intended for informational or error message from the previous action.
 * @param default_idx If a positive value is defined, the corresponding list index will be selected by
 *    default (NCURSES mode only).
 * @param usage_msg Optional usage directions to be displayed below the menu.
 * @param fn If defined, this callback function will be called:
 *    - upon user selection of a menu item.
 *    - NCURSES Mode Only: Any keypress not used for navigation or cancellation of menu.
 *    The return value of the callback will determine if the menu will continue to be displayed,
 *      a user defined status message updated, or if this menu shall exit.  
 *      See ui_menu_lsiting_cb_fn definition for details.
 * @returns Index of user selection.
 */
int ui_menu_listing(char* title, ui_menu_list_t* list, int n_choices,
                    char* status_msg, int default_idx, char* usage_msg,
                    ui_menu_listing_cb_fn fn, int flags);

/** This is a variant of ui_menu supporting a multi-column layout (NCURSES-mode only) */
int ui_menu_select(char* title, const char* const* choices, const char* const* descriptions, int n_choices, char* msg, int menu_cols);

/** ui_display_to_file
 *  Redirect subsequent ui_init() and ui_printf() output to the specified file.
 *  The file will be closed and normal behavior restored when ui_display_exec() 
 *  is called (or ui_display_to_file_close).
 * @returns AMP_OK on success, AMP_FAIL otherwise.
 */
int ui_display_to_file(char* filename);

/** This function will end the redirection of ui_printf() and close the open file (if any). 
 *    See ui_display_to_file() for details.
 */
void ui_display_to_file_close();

/** A wrapper function to output data to the UI using standard printf style formatting.
 *   If file redirection (ui_display_to_file()) is active, then this function will write
 *     the formatted string to the open file in place of the normal output mode.
 *   In NCURSES mode, output is written to a special buffer displayed when ui_display_exec()
 *     is called.
 *   In stdio mode, this is a wrapper to printf.
 */
#define ui_printf(...) ui_fprintf(NULL, __VA_ARGS__)
void ui_fprintf(ui_print_cfg_t *fd, const char* format, ...);

/** Signals the completion of a ui_printf based dialog.
 *    If file redirection is active, this function will cause the file to be closed
 *      and normal ui_printf output behavior restored.
 *    In NCURSES mode, this triggers the buffer to be displayed to the screen.
 *    In STDIO mode, this prints demarcation text to indicate the end of an output section.
 */
int ui_display_exec();

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/** The following functions have alternate prototypes or macro definitions dependent on USE_NCURSES flag */
#ifdef USE_NCURSES

/** Initialize the UI internal components.  This function should be called once on startup. */
void ui_init();

/** This function will reset the internal display buffer and output the specified title banner with
 *    appropriate formatting to the buffer.  The buffer will be appended to be subsequent calls to 
 *    ui_printf(), and displayed with ui_display_exec().  The display function can be suppressed
 *    in favor of file logging by calling ui_display_to_file() first.
 */
void ui_display_init(char* title);
#else

/** ui_init() is a placeholder macro when NCURSES support is disabled */
#define ui_init()

/** ui_display_init() Displays the specified title with demarcating line breaks for clarity. */
#define ui_display_init(title) ui_printf("\n\n--------------------\n%s\n--------------------\n", title)
#endif

/** Log information on transmitted messages (dependent on logging settings) */
void ui_log_transmit_msg(agent_t* agent, msg_ctrl_t *msg);

#endif // _NM_MGR_UI_H
