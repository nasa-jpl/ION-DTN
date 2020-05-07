/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file nm_mgr_ui.c
 **
 **
 ** Description: A text-based AMP Manager.
 **
 ** Notes:
 **		1. Currently we do not support ACLs.
 **		2. \todo When defining new MID, add to list so it can be IDX selected.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/18/13  E. Birrane     Code comments and cleanup (JHU/APL)
 **  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  07/26/17  E. Birrane     Added batch test file capabilities (JHU/APL)
 **  10/07/18  E. Birrane     Update to AMP v0.5. (JHU/APL)
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "platform.h"
#include "../shared/nm.h"
#include "../shared/utils/utils.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/rules.h"
#include "../shared/msg/msg.h"

#include "nm_mgr_ui.h"
#include "ui_input.h"
#include "nm_mgr_print.h"
#include "metadata.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif

#ifdef USE_CIVETWEB
#include "civetweb.h"
#endif

#ifdef USE_NCURSES
#include <curses.h>
#include <form.h>
#include <menu.h>
#include <panel.h>


// UI Display Constants
#define MENU_START_LINE 4
#define STARTX 15
#define FORM_STARTX (COLS/2)
#define FORM_MAX_WIDTH (COLS-FORM_STARTX)
#define STARTY 4
#define WIDTH 25

/* ui_dialog_win is the target for ui_printf, and is displayed with ui_display_show() */
#define UI_DIALOG_PAGES 10
#define UI_DIALOG_WIDTH COLS
WINDOW *ui_dialog_win;
PANEL *ui_dialog_pan;

#endif // USE_NCURSES

char *main_menu_choices[] = {
   "Version",
   "Register Agent",
   "List & Manage Registered Agent(s)",
   "List AMM Object Information",
   "Activate Automator UI Prompt (limited functionality, optimized for scripting)",
#ifdef HAVE_MYSQL
   "Database Menu",
#endif
#ifdef USE_NCURSES
   // Log file is currently written to only when NCURSES is enabled, so we will hide this menu option otherwise
   "View Log File",
#endif
   "Logging Configuration",
   "Exit",
                  };
typedef enum main_menu_t {
   MAIN_MENU_VERSION = 0,
   MAIN_MENU_REGISTER,
   MAIN_MENU_LIST_AGENTS,
   MAIN_MENU_LIST_AMM,
   MAIN_MENU_AUTOMATOR_UI,
#ifdef HAVE_MYSQL
   MAIN_MENU_DB,
#endif
#ifdef USE_NCURSES
   MAIN_MENU_LOG,
#endif
   MAIN_MENU_LOG_CFG,
   MAIN_MENU_EXIT
} main_menu_t;

char *ctrl_menu_list_choices[] = {
   "List all supported ADMs.      ",
   "List External Data Definitions",
   "List Atomics (CNST, LIT)      ",
   "List Control Definitions      ",
   "List Macro Definitions        ",
   "List Operator Definitions     ",
   "List Report Templates         ",
   "List Rules                    ",
   "List Table Templates          ",
   "List Variables                ",
};

char *bool_menu_choices[] = { "Yes", "No" };

#ifdef HAVE_MYSQL
char *db_menu_choices[] = {
   "Set Database Connection Information",
   "Print Database Connection Information",
   "Reset Database to ADMs",
   "Clear Received Reports",
   "Disconnect From DB",
   "Connect to DB",
   "Write DB Info to File",
   "Read DB Info from file"
};
form_fields_t db_conn_form_fields[] = {
   {"Database Server", gMgrDB.sql_info.server, UI_SQL_SERVERLEN-1, 0, 0},
   {"Database Name", gMgrDB.sql_info.database, UI_SQL_DBLEN-1, 0, 0},
   {"Database Username", gMgrDB.sql_info.username, UI_SQL_ACCTLEN-1, 0, 0},
   {"Database Password", gMgrDB.sql_info.password, UI_SQL_ACCTLEN-1, 0, 0}
};

#endif

#ifdef USE_NCURSES
#define MGR_UI_DEFAULT MGR_UI_NCURSES
#else
#define MGR_UI_DEFAULT MGR_UI_STANDARD
#endif

int gContext;
int *global_nm_running = NULL;
mgr_ui_mode_enum mgr_ui_mode = MGR_UI_DEFAULT;

/* Prototypes */
void ui_eventLoop(int *running);
void ui_ctrl_list_menu(int *running);

#ifdef HAVE_MYSQL
void ui_db_menu(int *running);
void ui_db_parms(int do_edit);
#endif

#ifdef USE_NCURSES
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);
void ui_shutdown();
#else
#define ui_shutdown() ui_display_init("Shutting down . . . ");
#endif

/** Utility function for logging a transmitted Ctrl Message to file
 *   NOTE: This is intended for debug/testing purposes.
 */
void ui_log_transmit_msg(agent_t* agent, msg_ctrl_t *msg) {
   blob_t *data;
   char *msg_str;
   
   if (agent_log_cfg.tx_cbor && agent->log_fd) {
      data = msg_ctrl_serialize_wrapper(msg);
      if (data) {
         msg_str = utils_hex_to_string(data->value, data->length);
         if (msg_str) {
            fprintf(agent->log_fd, "TX: msg:%s\n", msg_str);
            fflush(agent->log_fd);
            SRELEASE(msg_str);
         }
         blob_release(data, 1);
      }
   }
}

int ui_build_control(agent_t* agent)
{
	ari_t *id = NULL;
	uvast ts;
	msg_ctrl_t *msg;
    int rtv;

	AMP_DEBUG_ENTRY("ui_build_control","("ADDR_FIELDSPEC")", (uaddr)agent);

    if (agent == NULL)
    {
       AMP_DEBUG_ERR("ui_build_control","No agent given.",NULL);
       return 0;
    }

#ifdef USE_NCURSES
    char title[40];
    char tsc[16] = "";
    form_fields_t fields[] = {
       {"Control Timestamp", tsc, 16, 0, TYPE_CHECK_INT}//, {.num=0, 0, 0xFFFFFF} } // @VERIFY valid range
       };
    fields[0].args.num.padding = 0;
    fields[0].args.num.vmin = 0;
    fields[0].args.num.vmax = 0xFFFFFFFF;
    
    
    sprintf(title, "Build Control for agent %s", agent->eid.name);
    ui_form(title, NULL, fields, ARRAY_SIZE(fields) );
    ts = atoi(tsc);
#else
    ts = ui_input_uint("Control Timestamp");
#endif
    if((id = ui_input_ari("Control MID:", ADM_ENUM_ALL, TYPE_AS_MASK(AMP_TYPE_CTRL))) == NULL)
    {
       AMP_DEBUG_ERR("ui_build_control","Can't get control.",NULL);
       return AMP_FAIL;
    }


	ui_postprocess_ctrl(id);

#if 0 // DEBUG
    /* Information on bitstream we are sending. */
    blob_t *data = ari_serialize_wrapper(id);
    char *msg_str = utils_hex_to_string(data->value, data->length);
    AMP_DEBUG_ALWAYS("iif_send","Sending ari:%s to %s:", msg_str, agent->eid.name);
    SRELEASE(msg_str);
    blob_release(data, 1);
#endif

	if((msg = msg_ctrl_create_ari(id)) != NULL)
	{
		msg->start = ts;
        ui_log_transmit_msg(agent, msg);
		rtv = iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);
		msg_ctrl_release(msg, 1);
        return rtv;
	}
	else
	{
		ari_release(id, 1);
        return AMP_FAIL;
	}
}



/******************************************************************************
 *
 * \par Function Name: ui_clear_reports
 *
 * \par Clears the list of received data reports from an agent.
 *
 * \par Notes:
 *	\todo - Add ability to clear reports from a particular agent, or of a
 *	\todo   particular type, or for a particular timeframe.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation,
 *  01/18/13  E. Birrane     Debug updates.
 *  04/18/13  V.Ramachandran Multiple-agent support (added param)
 *  10/06/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/
void ui_clear_reports(agent_t* agent)
{
	CHKVOID(agent);

	gMgrDB.tot_rpts -= vec_num_entries(agent->rpts);

	vec_clear(&(agent->rpts));
}

/******************************************************************************
 *
 * \par Function Name: ui_clear_tables
 *
 * \par Clears the list of received data tables from an agent.
 *
 * \par Notes:
 *	\todo - Add ability to clear tables from a particular agent, or of a
 *	\todo   particular type, or for a particular timeframe.
 *
 *****************************************************************************/
void ui_clear_tables(agent_t* agent)
{
	CHKVOID(agent);

	gMgrDB.tot_tbls -= vec_num_entries(agent->tbls);

	vec_clear(&(agent->tbls));
}

/******************************************************************************
 *
 * \par Function Name: ui_create_rpttpl_from_rpt_parms
 *
 * \par Release resources associated with a report template.
 *
 * \param[in|out]  rpttpl  The template to be released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/09/18  E. Birrane     Initial implementation.
 *  10/06/18  E. Birrane     Update for AMP v0.5 (JHU/APL)
 *****************************************************************************/
rpttpl_t *ui_create_rpttpl_from_parms(tnvc_t parms)
{
	rpttpl_t *result = NULL;

	ari_t *ari = (ari_t *) adm_get_parm_obj(&parms, 0, AMP_TYPE_ARI);
	ac_t *ac = (ac_t *) adm_get_parm_obj(&parms, 1, AMP_TYPE_AC);

	CHKNULL(ari);
	CHKNULL(ac);

	ari_t *a1 = ari_copy_ptr(ari);
	ac_t ac2 = ac_copy(ac);

	if((result = rpttpl_create(a1, ac2)) == NULL)
	{
		ari_release(a1, 1);
		ac_release(&ac2, 0);
		result = NULL;
	}

	return result;
}


var_t *ui_create_var_from_parms(tnvc_t parms)
{
	int success;
	tnv_t tmp;


	ari_t *id = adm_get_parm_obj(&parms, 0, AMP_TYPE_ARI);
	amp_type_e type = adm_get_parm_uint(&parms, 2, &success);

	tnv_init(&tmp, type);
	/* We don't need the actual value, just the ID and type. */
	return var_create_from_tnv(ari_copy_ptr(id), tmp);
}

macdef_t *ui_create_macdef_from_parms(tnvc_t parms)
{
	int success;
	int i, num;
	ari_t *id = adm_get_parm_obj(&parms, 1, AMP_TYPE_ARI);
	ac_t *def = adm_get_parm_obj(&parms, 2, AMP_TYPE_AC);
	macdef_t *result = NULL;

	if((id == NULL) || (def == NULL))
	{
		AMP_DEBUG_ERR("ADD_MACRO", "Bad parameters for control", NULL);
		return result;
	}

	num = ac_get_count(def);
	result = macdef_create(num, ari_copy_ptr(id));

	for(i = 0; i < num; i++)
	{
		ctrl_t *cur_ctrl = ctrl_create(ac_get(def, i));
		macdef_append(result, ctrl_copy_ptr(cur_ctrl));
	}

	return result;
}

rule_t *ui_create_tbr_from_parms(tnvc_t parms)
{
	tbr_def_t def;
	rule_t *tbr = NULL;
	int success;

	ari_t *id = adm_get_parm_obj(&parms, 0, AMP_TYPE_ARI);
	uvast start = adm_get_parm_uvast(&parms, 1, &success);
	def.period = adm_get_parm_uvast(&parms, 2, &success);
	def.max_fire = adm_get_parm_uvast(&parms, 3, &success);
	ac_t action = ac_copy(adm_get_parm_obj(&parms, 4, AMP_TYPE_AC));

	if((tbr = rule_create_tbr(*id, start, def, action)) == NULL)
	{
		AMP_DEBUG_ERR("ADD_TBR", "Unable to create TBR structure.", NULL);
		return tbr;
	}

	return tbr;
}

rule_t *ui_create_sbr_from_parms(tnvc_t parms)
{
	sbr_def_t def;
	rule_t *sbr = NULL;
	int success;

	ari_t *id = adm_get_parm_obj(&parms, 0, AMP_TYPE_ARI);
	uvast start = adm_get_parm_uvast(&parms, 1, &success);
	expr_t *state = adm_get_parm_obj(&parms, 2, AMP_TYPE_EXPR);
	def.expr = *state;
	SRELEASE(state);
	def.max_eval = adm_get_parm_uvast(&parms, 3, &success);
	def.max_fire = adm_get_parm_uvast(&parms, 4, &success);
	ac_t action = ac_copy(adm_get_parm_obj(&parms, 5, AMP_TYPE_AC));

	if((sbr = rule_create_sbr(*id, start, def, action)) == NULL)
	{
		AMP_DEBUG_ERR("ADD_TBR", "Unable to create TBR structure.", NULL);
		return sbr;
	}

	return sbr;
}


/******************************************************************************
 *
 * \par Function Name: ui_deregister_agent
 *
 * \par Remove and deallocate an agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/23/13  V.Ramachandran Initial Implementation
 *****************************************************************************/
void ui_deregister_agent(agent_t* agent)
{
	CHKVOID(agent);
	AMP_DEBUG_ENTRY("ui_deregister_agent","(%s)",agent->eid.name);
	vec_del(&(gMgrDB.agents), agent->idx);
}

void ui_show_log(char *title, char *fn)
{
   FILE *fp;
   char str[80];
   
   ui_display_init(title);
   fp = fopen(fn, "r");
   if (fp==NULL)
   {
      ui_printf("ERROR: Unable to open file '%s'\n", fn);
      return;
   }
   while(fgets(str, 80, fp) != NULL)
   {
      ui_printf("%s", str);
   }
   fclose(fp);
   
   ui_display_exec();
}

void ui_log_cfg_menu()
{
   int status;
   vecit_t it;
   
   // Build Form
   form_fields_t log_cfg_fields[] = {
      {"Enable logging to files", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.enabled},
      // FUTURE: Enable logging to UDP Socket or global file. (And corresponding port/server settings)
      //{"Enable logging to socket", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.net_enabled},
      // TX IP, Port (used for TX+RX).  Protocol fixed as UDP
      {"Log TX CBOR", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.tx_cbor},
      {"Log RX CBOR", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.rx_cbor},
      {"Log Received Reports (ASCII)", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.rx_rpt},
      {"Log Received Tables (ASCII)", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.rx_tbl},
#ifdef USE_JSON
      {"Log Received Reports (JSON)", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.rx_json_rpt},
      {"Log Received Tables (JSON)", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.rx_json_tbl},
#endif
      {"Use discrete directories per agent", NULL, 8, 0, TYPE_CHECK_BOOL, &agent_log_cfg.agent_dirs},
      {"Max Entries Per Log File", NULL, 8, 0, TYPE_CHECK_NUM, &agent_log_cfg.limit},
      {"Root Log directory", agent_log_cfg.dir, 32, 0, 0},
   };

   // Display Form
   status = ui_form("Logging Configuration Options", NULL, log_cfg_fields, ARRAY_SIZE(log_cfg_fields) );

   // If user submitted (ie: didn't cancel it), reset file logging as appropriate
   if (status == 1)
   {
      printf("Settings updated. Forcing log rotation as applicable.\n");

      for(it = vecit_first(&(gMgrDB.agents)); vecit_valid(it); it = vecit_next(it))
      {
         agent_rotate_log((agent_t *) vecit_data(it), 1 );
      }
   }
   else
   {
      printf("Cancelled.  If settings have been altered, they MAY not have been activated.\n");
   }
}

// Automator Mode Helper Defines
// Space-delimited UI inputs
#define AUT_DELIM " "
#define AUT_GET_TOK()  strtok(NULL, AUT_DELIM)
#define AUT_GET_NEXT() token = AUT_GET_TOK(); CHKZERO(token)



/** Automator Mode is an alternate UI interface in which all commands
 * and responses are single-line text optimized for automated parsing.
 *
 * The following commands are currently supported:
 *
 * - EXIT_UI           Exit the AUTOMATOR UI Interface and return to the primary UI
 * - EXIT_SHUTDOWN     Shutdown the nm_mgr service.
 * - V                 Display version information
 * - R $EID            Register an agent with the given $EID
 * - H $EID [$TS] $HEX Send the ARI specified as a CBOR-encoded HEX string to the agent $EID.
 *                        TS is an optional timestamp parameter. If omitted, a value of 0 will be used.
 * - CR $EID           Clear all received reports from given Agent
 * - CT $EID           Clear all received tables from given Agent.
 * - L                 List all registered agents. Output will be a comma seperated list beginning with "Agents: "
 *
 * Note: It is recommended to use file-based logging or DB support
 * (future) for automated handling of received reports and tables.
 */
int ui_automator_parse_input(char *str)
{
   char *token, *token2;
   eid_t agent_eid;
   agent_t *agent = NULL;
   int ts, err_cnt, i;
   char tmp;
   const char s[2] = AUT_DELIM;
   CHKZERO(str);

   int success;

   // Get Command (first space-delimited token)
   //  Note: We currently look at only the first character of the first token
   token = strtok(str, s);

   switch(token[0])
   {
   case '?': // Help menu
      printf(
         "?                  Display this help menu\n"
         "CT $EID            Clear all received Tables for specified agent\n"
         "CR $EID            Clear all received Reports for specified agent\n"
         "H $EID [$TS] $HEX  Send the RAW HEX-Encoded CBOR ARI to the specified agent with optional timestamp\n"
         "L                  List registered agents\n"
         "R $EID             Register Agent with specified EID\n"
         "V                  Display version and build information\n"
         "EXIT_UI            Return to main UI menu\n"
         "EXIT_SHUTDOWN      Exit NM Manager\n"
         );
      break;
   case 'C': // Clear Commands
      tmp = token[1];
      if (strlen(token) != 2 && (tmp == 'T' || tmp == 'R'))
      {
         printf("Invalid command %s\n", token);
         return -1;
      }
      
      AUT_GET_NEXT();
      strcpy(agent_eid.name, token);
      agent = agent_get((eid_t*)token);
      CHKZERO(agent);

      if (tmp == 'T')
      {
         ui_clear_tables(agent);
         printf("Tables cleared for %s\n", token);
      }
      else if (tmp == 'R')
      {
         ui_clear_reports(agent);
         printf("Reports cleared for %s\n", token);
      }
      break;
   case 'E': // Exit Commands
      if (strncmp(token, "EXIT_UI", 8) == 0)
      {
         // To minimize errors in automation, the full string must match to exit to standard UI
         mgr_ui_mode = MGR_UI_DEFAULT;
         return 1;
      }
      else if (strncmp(token, "EXIT_SHUTDOWN", 16) == 0)
      {
         printf("Signaling Manager Shutdown . . . \n");
         *global_nm_running = 0;
         return 1;
      }
      break;
   case 'H': // Send Raw HEX CBOR
      // Get EID
      AUT_GET_NEXT();
      agent = agent_get((eid_t*)token);
      CHKZERO(agent);

      // Timestamp - Optional parameter
      AUT_GET_NEXT();
      token2 = AUT_GET_TOK();
      if (token2)
      {
         ts = atoi(token);
      }
      else // Timestamp not specified
      {
         ts = 0;
         token2 = token;
      }

      // Get Input string
      blob_t *data = utils_string_to_hex(token2);
      ari_t *id = ari_deserialize_raw(data, &success);
      blob_release(data, 1);
      CHKZERO(id);

      // Parse the control
      ui_postprocess_ctrl(id);

      msg_ctrl_t *msg;
      if ((msg = msg_ctrl_create_ari(id)) == NULL)
      {
         ari_release(id, 1);
         return 0;
      }
      msg->start = ts;
      ui_log_transmit_msg(agent, msg);
      iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);
      msg_ctrl_release(msg, 1);
      break;
   case 'L': // List Agents
      printf("Agents: ");
      for(i = vec_num_entries_ptr(&gMgrDB.agents); i > 0; i--)
      {
         agent = (agent_t*)vec_at(&gMgrDB.agents, i-1);
         printf("%s", agent->eid.name);
         if (i != 1) {
            printf(",");
         }
      }
      printf("\n");
      break;
   case 'R': // Register agent
      AUT_GET_NEXT();
      strcpy(agent_eid.name, token);
      if (agent_add(agent_eid) == AMP_OK)
      {
         printf("Successfully registered agent %s\n", token);
      }
      else
      {
         printf("ERROR: Unable to register agent %s\n", token);
      }
      break;
   case 'V': // Version command
      printf("VERSION: Built on %s %s\nAMP Protocol Version %s\nBP Version %s",
             __DATE__, __TIME__,
             AMP_VERSION_STR,
             
#ifdef BUILD_BPv6
                            "6"
#elif defined(BUILD_BPv7)
                            "7"
#else
                            "?"
#endif
         );
      return 1; 
   default:
      printf("Unrecognized Command\n");
      return -1;
   }
   
   return 1;
}
void ui_automator_run(int *running)
{
   char line[MAX_INPUT_BYTES];
   int len;

   while(mgr_ui_mode == MGR_UI_AUTOMATOR && *running)
   {
      // Print prompt
      printf("\n#-NM->");
      fflush(stdout); // Show the prompt without a newline

      // Read Input Line
      if(igets(fileno(stdin), line, MAX_INPUT_BYTES, &len) == NULL)
      {
         AMP_DEBUG_ERR("ui_automator_run", "igets failed.", NULL);
         return;
      }

      // Parse & Execute Line
      if (len > 0)
      {
         if (ui_automator_parse_input(line) != 1)
         {
            printf("\nFailed: Unable to execute last command\n");
         }
      }
   }
}

/******************************************************************************
 *
 * \par Function Name: ui_eventLoop
 *
 * \par Main event loop for the NCURSES UI thread.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/15/18  D.Edell        Initial NCURSES implementation based on original UI
 *****************************************************************************/
void ui_eventLoop(int *running)
{
   int choice; // Last user menu selection
   char msg[128] = ""; // User (error) message to append to menu
   int n_choices = ARRAY_SIZE(main_menu_choices);
   int new_msg = 0;
   
   ui_init();
   
   while(*running)
   {
      if (mgr_ui_mode == MGR_UI_AUTOMATOR)
      {
         ui_automator_run(running);
      }
      else
      {
         choice = ui_menu("Main Menu", main_menu_choices, NULL, n_choices, ((new_msg==0) ? NULL : msg) );
         new_msg = 0;
      
         if (choice == MAIN_MENU_EXIT)
         {
            *running = 0;
            break;
         } else {
            switch(choice)
            {
            case MAIN_MENU_VERSION:
               sprintf(msg, "VERSION: Built on %s %s\nAMP Protocol Version %d - %s/%02d",
                       __DATE__, __TIME__, AMP_VERSION, AMP_PROTOCOL_URL, AMP_VERSION);
               new_msg = 1;            
               break;
            case MAIN_MENU_REGISTER: // Register new Agent
               ui_register_agent(msg);
               new_msg = 1;
               break;
            case MAIN_MENU_LIST_AGENTS:
               if (ui_print_agents() == 0)
               {
                  new_msg = 1;
                  sprintf(msg, "No Agents Defined");
               }
               break;
            case MAIN_MENU_LIST_AMM: // List Object Information (old Control Menu merged with Admmin Menu's List Agents)
               ui_ctrl_list_menu(running);
               break;
#ifdef HAVE_MYSQL
            case MAIN_MENU_DB: // DB
               ui_db_menu(running);
               break;
#endif
#ifdef USE_NCURSES // Log file is currently written to only when NCURSES is enabled
            case MAIN_MENU_LOG:
               fflush(stderr); // Flush the log
               ui_show_log("NM Log File", NM_LOG_FILE);
               break;
#endif
            case MAIN_MENU_LOG_CFG:
               ui_log_cfg_menu();
               break;
            case MAIN_MENU_AUTOMATOR_UI:
               mgr_ui_mode = MGR_UI_AUTOMATOR;
               printf("Switching to alternate AUTOMATOR interface. Type 'EXIT_UI' to return to this menu, '?' for usage.");
               break;
            default:
               new_msg = 1;
               sprintf(msg, "ERROR: Menu choice %d is not valid.", choice);
            }
         }
      }
   }
   
   ui_shutdown();
}

// this is a mess. clean it up.
void ui_list_objs(uint8_t adm_id, uvast mask, ari_t **result)
{
   char title[100];
   ui_menu_list_t *list;
   int num_objs, num_parms;
   int i, rtv;
   meta_col_t *col;
   vecit_t it;
   metadata_t *meta = NULL;
   amp_type_e type = AMP_TYPE_UNK;

   type = ui_input_ari_type(mask);
   
   /* Unknown type means cancel. This selects ARIs, so no numerics. */
   if((type == AMP_TYPE_UNK) || type_is_numeric(type))
   {
	   return;
   }

   /* We don't select literals from a list. We enter them as LIT ARIs.*/
   if(type == AMP_TYPE_LIT && result != NULL)
   {
	   *result = ui_input_ari_lit(NULL);
   }

    if (adm_id == ADM_ENUM_ALL)
    {
       sprintf(title, "Listing all %s objects", type_to_str(type));
    }
    else
    {
       sprintf(title, "Listing Objects for ADM ID %d, Type %s", adm_id, type_to_str(type));
    }

    col =  meta_filter(adm_id, type);
    
    num_objs = vec_num_entries(col->results);
    if (num_objs == 0)
    {
       ui_prompt("No matching options.", "Continue", NULL, NULL);
       metacol_release(col, 1);
       return;
    }
    list = calloc(num_objs, sizeof(ui_menu_list_t));

    for(i = 0, it = vecit_first(&(col->results)); vecit_valid(it); it = vecit_next(it),i++)
    {
       meta = vecit_data(it);

       list[i].name = malloc(META_DESCR_MAX); // NAME + Parameters should be less than the description length
       list[i].description = malloc(META_DESCR_MAX);
       list[i].data = (char*)(meta->id);
       
       strncpy(list[i].description, meta->descr, META_DESCR_MAX);
       strncpy(list[i].name, meta->name, META_NAME_MAX);
       num_parms = vec_num_entries(meta->parmspec);
       if (num_parms > 0)
       {
          vecit_t itp;
          int j = 0;
          
          strcat( list[i].name, "(");
          for(j=0, itp = vecit_first(&(meta->parmspec)); vecit_valid(itp); itp = vecit_next(itp), j++)
          {
             meta_fp_t *parm = (meta_fp_t *) vecit_data(itp);
             if(j != 0 && j != num_parms)
             {
                strcat(list[i].name, ", ");
             }
             sprintf( (list[i].name + strlen(list[i].name)),
                      "%s %s",
                      type_to_str(parm->type),
                      parm->name
             );
          }
          strcat( list[i].name, ")");
       }
       
    }
   

    rtv = ui_menu_listing(title,
                   list,
                   num_objs,
                   NULL,0,NULL, NULL, UI_OPT_AUTO_LABEL | UI_OPT_ENTER_SEL | UI_OPT_SPLIT_DESCRIPTION
   );
   if (result != NULL && rtv >= 0)
   {
      *result = ari_copy_ptr(((ari_t*)list[rtv].data));
   }

   for(i = 0; i < num_objs; i++)
    {
       free(list[i].name);
       free(list[i].description);
    }

   free(list);

   metacol_release(col, 1);
}

/******************************************************************************
 *
 * \par Function Name: ui_postprocess_ctrl
 *
 * \par Evaluates a control prior to sending it to agents to see if the manager
 *      needs to perform any postprocessing of the control.
 *
 * \par Notes:
 * 1. Post-processing includes the following activities
 * 		- Verifying that definitions are unique.
 * 		- Remembering the definitions of any custom-defined data
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/18/15  E. Birrane      Initial implementation,
 *****************************************************************************/

void ui_postprocess_ctrl(ari_t *id)
{
	metadata_t *meta;

	CHKVOID(id);
	CHKVOID(id->type == AMP_TYPE_CTRL);

	// TODO: Put locks around these retrieve calls.
	meta = rhht_retrieve_key(&(gMgrDB.metadata), id);

	if(meta == NULL)
	{
		return;
	}

	if(strcmp(meta->name, AGENT_ADD_VAR_STR) == 0)
	{
		var_t *var = ui_create_var_from_parms(id->as_reg.parms);
		if(var != NULL)
		{
			VDB_ADD_VAR(var->id, var);
			db_persist_var(var);
		}
		else
		{
			AMP_DEBUG_ERR("ui_postprocess_ctrl","Unable to persist new VAR.", NULL);
		}
	}
	else if(strcmp(meta->name, AGENT_DEL_VAR_STR) == 0)
	{
		ac_t *ac = (ac_t *) adm_get_parm_obj(&(id->as_reg.parms), 0, AMP_TYPE_AC);
		vecit_t it;

		for(it = vecit_first(&(ac->values)); vecit_valid(it); it = vecit_next(it))
		{
			ari_t *var_id = vecit_data(it);
			var_t *var = VDB_FINDKEY_VAR(var_id);

			if(var != NULL)
			{
				db_forget(&(var->desc), gDB.vars);
				VDB_DELKEY_VAR(id);
			}
			else
			{
				char *tmp = ui_str_from_ari(var_id, NULL, 0);
				AMP_DEBUG_WARN("ui_postprocess_ctrl","Can't find var %s ", tmp);
				SRELEASE(tmp);
			}
		}
	}
	else if(strcmp(meta->name, AGENT_ADD_RPTT_STR) == 0)
	{
		rpttpl_t *def = ui_create_rpttpl_from_parms(id->as_reg.parms);
		if(def != NULL)
		{
			VDB_ADD_RPTT(def->id, def);
			db_persist_rpttpl(def);
		}
		else
		{
			AMP_DEBUG_ERR("ui_postprocess_ctrl","Unable to persist new RPTT.", NULL);
		}
	}
	else if(strcmp(meta->name, AGENT_DEL_RPTT_STR) == 0)
	{
		ac_t *ac = (ac_t *) adm_get_parm_obj(&(id->as_reg.parms), 0, AMP_TYPE_AC);
		vecit_t it;

		for(it = vecit_first(&(ac->values)); vecit_valid(it); it = vecit_next(it))
		{
			ari_t *rppt_id = vecit_data(it);
			rpttpl_t *def = VDB_FINDKEY_RPTT(rppt_id);

			if(def != NULL)
			{
				db_forget(&(def->desc), gDB.rpttpls);
				VDB_DELKEY_RPTT(id);
			}
			else
			{
				char *tmp = ui_str_from_ari(rppt_id, NULL, 0);
				AMP_DEBUG_WARN("ui_postprocess_ctrl","Can't find def for %s ", tmp);
				SRELEASE(tmp);
			}
		}
	}
	else if(strcmp(meta->name, AGENT_ADD_MAC_STR) == 0)
	{
		macdef_t *macro = ui_create_macdef_from_parms(id->as_reg.parms);
		if(adm_add_macdef(macro) != AMP_OK)
		{
			AMP_DEBUG_ERR("ADD_MACRO", "Error adding new macro.", NULL);
		}
		else if(db_persist_macdef(macro) != AMP_OK)
		{
			AMP_DEBUG_ERR("ADD_MACRO", "Unable to persist new macro.", NULL);
		}
	}
	else if(strcmp(meta->name, AGENT_DEL_MAC_STR) == 0)
	{
		ac_t *ac = (ac_t *) adm_get_parm_obj(&(id->as_reg.parms), 0, AMP_TYPE_AC);
		vecit_t it;

		for(it = vecit_first(&(ac->values)); vecit_valid(it); it = vecit_next(it))
		{
			ari_t *mac_id = vecit_data(it);
			macdef_t *def = VDB_FINDKEY_MACDEF(mac_id);

			if(def != NULL)
			{
				db_forget(&(def->desc), gDB.macdefs);
				VDB_DELKEY_MACDEF(mac_id);
			}
			else
			{
				char *tmp = ui_str_from_ari(mac_id, NULL, 0);
				AMP_DEBUG_WARN("ui_postprocess_ctrl","Can't find def for %s ", tmp);
				SRELEASE(tmp);
			}
		}
	}
	else if(strcmp(meta->name, AGENT_ADD_SBR_STR) == 0)
	{
		rule_t *sbr = ui_create_sbr_from_parms(id->as_reg.parms);

		int rh_code = VDB_ADD_RULE(&(sbr->id), sbr);
		if((rh_code != RH_OK) && (rh_code != RH_DUPLICATE))
		{
			AMP_DEBUG_ERR("ADD_SBR", "Unable to remember SBR.", NULL);
			rule_release(sbr, 1);
		}
		else if(db_persist_rule(sbr) != AMP_OK)
		{
			AMP_DEBUG_ERR("ADD_TBR", "Unable to persist new rule.", NULL);
		}
	}
	else if(strcmp(meta->name, AGENT_ADD_TBR_STR) == 0)
	{
		rule_t *tbr = ui_create_tbr_from_parms(id->as_reg.parms);

		int rh_code = VDB_ADD_RULE(&(tbr->id), tbr);
		if((rh_code != RH_OK) && (rh_code != RH_DUPLICATE))
		{
			AMP_DEBUG_ERR("ADD_TBR", "Unable to remember TBR.", NULL);
			rule_release(tbr, 1);
		}
		else if(db_persist_rule(tbr) != AMP_OK)
		{
			AMP_DEBUG_ERR("ADD_TBR", "Unable to persist new rule.", NULL);
		}
	}
	else if( (strcmp(meta->name, AGENT_DEL_TBR_STR) == 0) ||
			 (strcmp(meta->name, AGENT_DEL_SBR_STR) == 0))
	{
		ac_t *ac = (ac_t *) adm_get_parm_obj(&(id->as_reg.parms), 0, AMP_TYPE_AC);
		vecit_t it;

		for(it = vecit_first(&(ac->values)); vecit_valid(it); it = vecit_next(it))
		{
			ari_t *rule_id = vecit_data(it);
			rule_t *def = VDB_FINDKEY_RULE(rule_id);

			if(def != NULL)
			{
				db_forget(&(def->desc), gDB.rules);
				VDB_DELKEY_RULE(rule_id);
			}
			else
			{
				char *tmp = ui_str_from_ari(rule_id, NULL, 0);
				AMP_DEBUG_WARN("ui_postprocess_ctrl","Can't find def for %s ", tmp);
				SRELEASE(tmp);
			}
		}
	}

}



/******************************************************************************
 *
 * \par Function Name: ui_register_agent
 *
 * \par Register a new agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/18/13  V.Ramachandran Initial Implementation
 *****************************************************************************/
void ui_register_agent(char* msg)
{
	char line[AMP_MAX_EID_LEN] = "ipn:x.y";
	eid_t agent_eid;

	AMP_DEBUG_ENTRY("register_agent", "()", NULL);

#ifdef USE_NCURSES
    form_fields_t fields[] = {
       {"EID", &line[0], AMP_MAX_EID_LEN, O_AUTOSKIP|O_NULLOK, TYPE_CHECK_REGEXP }
    };
    fields[0].args.regex = "ipn:([0-9]+)\\.([0-9]+)";
    if (ui_form("Define new Agent", NULL, &fields[0], 1) <= 0)
    {
       // User cancelled form or an error occurred
#else
    memset(line,0, AMP_MAX_EID_LEN);
	/* Grab the new agent's EID. */
	if(ui_input_get_line("Enter EID of new agent:", (char **)&line, AMP_MAX_EID_LEN-1) == 0)
	{
#endif
		AMP_DEBUG_ERR("register_agent","Unable to read user input.", NULL);
		AMP_DEBUG_EXIT("register_agent","->.", NULL);
        if (msg != NULL)
        {
           sprintf(msg, "Agent registration aborted");
        }
		return;
	}
	else
		AMP_DEBUG_INFO("register_agent", "User entered agent EID name %s", line);


	/* Check if the agent is already known. */
	sscanf(line, "%s", agent_eid.name);
	agent_add(agent_eid);

	AMP_DEBUG_EXIT("register_agent", "->.", NULL);
    if (msg != NULL)
    {
       sprintf(msg, "Successfully registered new agent: %s", line);
    }
}



/******************************************************************************
 *
 * \par Function Name: ui_select_agent
 *
 * \par Prompts the user to select a known agent from a list.
 *
 * \par Notes:
 *
 * \par Returns the selected agent's EID, or NULL if cancelled (or an error occurs).
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/18/13  V.Ramachandran Initial implementation
 *  06/17/13  E. Birrane     Working implementation
 *  07/04/16  E. Birrane     Auto-select if only 1 agent known.
 *  10/07/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/
agent_t* ui_select_agent()
{
	char line[10];
	int idx = -1;
	int total;
	agent_t *agent = NULL;

	printf("Select an Agent:\n");
	total = ui_print_agents();

	if(total == 0)
	{
		printf("No agents registered.\n");
		return NULL;
	}

	if(total == 1)
	{
		idx = 0;
		printf("Auto-selecting sole known agent.\n");
	}
	else if((idx = ui_input_int("Agent (#), or 0 to cancel:")) == 0)
	{
		printf("No agent selected.\n");
		return NULL;
	}

	if((agent = vec_at(&(gMgrDB.agents), idx-1)) == NULL)
	{
		AMP_DEBUG_ERR("ui_select_agent","Error selecting agent #%d", idx);
		return NULL;
	}

	return agent;
}



void ui_send_file(agent_t* agent, uint8_t enter_ts)
{
	ari_t *cur_id = NULL;
	uint32_t offset = 0;
	time_t ts = 0;
	blob_t *contents = NULL;
	char *cursor = NULL;
	char *saveptr = NULL;
	uint32_t bytes = 0;
	blob_t *value = NULL;
	int success;

	CHKVOID(agent);
#ifdef mingw
        AMP_DEBUG_ERR("ui_send_file", "Is not currently available for this platform", NULL);
        return;
#else
	ts = (enter_ts) ? ui_input_uint("Control Timestamp") : 0;


	if((contents = ui_input_file_contents("Enter file name containing commands:")) == NULL)
	{
		AMP_DEBUG_ERR("ui_send_file", "Can't read file contents.", NULL);
		AMP_DEBUG_EXIT("ui_send_file","->.",NULL);
		return;
	}

	cursor = strtok_r((char *) contents->value,"\n",&saveptr);

	while(cursor != NULL)
	{
		if(strlen(cursor) <= 0)
		{
			cursor = strtok_r(NULL, "\n", &saveptr);

			continue;
		}

		if((cursor[0] == '#') || (cursor[0] == ' '))
		{
			cursor = strtok_r(NULL, "\n", &saveptr);
			continue;
		}

		if(strncmp(cursor,"WAIT",4) == 0)
		{
			int delay = 0;
			sscanf(cursor,"WAIT %d", &delay);
			fprintf(stderr,"Sleeping for %ds: ", delay);
			int sum = 0;

			int dur = 5;

			if(delay < dur)
			{
				sleep(delay);
			}
			else
			{
				while(delay > 0)
				{
					if(delay >= dur)
					{
						sleep(dur);
						delay -= dur;
						sum += dur;
						fprintf(stderr,"%d...", sum);
					}
					else
					{
						sleep(delay);
						delay = 0;
						sum += delay;
						fprintf(stderr,"%d...", sum);
					}
				}
			}
			fprintf(stderr,"Done waiting!\n");
			cursor = strtok_r(NULL, "\n", &saveptr);
			continue;
		}

		if((value = utils_string_to_hex(cursor)) == NULL)
		{
			AMP_DEBUG_ERR("ui_send_file", "Can't make value from %s", cursor);
			blob_release(contents, 1);
			return;
		}

		cur_id = ari_deserialize_raw(value, &success);
		blob_release(value, 1);
		if(cur_id == NULL)
		{
			AMP_DEBUG_ERR("ui_send_file", "Can't make ari from %s", cursor);
			blob_release(contents, 1);
			return;
		}

		ui_postprocess_ctrl(cur_id);

		msg_ctrl_t *msg;
		if((msg = msg_ctrl_create_ari(cur_id)) == NULL)
		{
           AMP_DEBUG_ERR("ui_send_file", "Can't make ctrl from %s", cursor);
           ari_release(cur_id, 1);
           blob_release(contents, 1);
           return;
		}

		msg->start = ts;
        ui_log_transmit_msg(agent, msg);
		iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);

		msg_ctrl_release(msg, 1);
		cursor = strtok_r(NULL, "\n", &saveptr);
	}

	blob_release(contents, 1);

	AMP_DEBUG_EXIT("ui_send_file","->.", NULL);
#endif
}


void ui_send_raw(agent_t* agent, uint8_t enter_ts)
{
	ari_t *id = NULL;
	time_t ts = 0;
	msg_ctrl_t *msg = NULL;

	CHKVOID(agent);

	ts = (enter_ts) ? ui_input_uint("Control Timestamp") : 0;

	printf("Enter raw ARI to send.\n");
	id = ui_input_ari_raw(1);

	if(id == NULL)
	{
		AMP_DEBUG_ERR("ui_send_raw","Can't get control MID.",NULL);
		return;
	}

	ui_postprocess_ctrl(id);

	if((msg = msg_ctrl_create_ari(id)) == NULL)
	{
		ari_release(id, 1);
		return;
	}
	msg->start = ts;
    ui_log_transmit_msg(agent, msg);
	iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);

	msg_ctrl_release(msg, 1);
}


void *ui_thread(int *running)
{
	AMP_DEBUG_ENTRY("ui_thread","(0x%x)", (size_t) running);

    // Cache running as an NM UI Global for simplicity. This is always the entrypoint to ui
    global_nm_running = running;

	ui_eventLoop(running);

	AMP_DEBUG_ALWAYS("ui_thread","Exiting.", NULL);

	AMP_DEBUG_EXIT("ui_thread","->.", NULL);

#ifdef HAVE_MYSQL
	db_mgt_close();
#endif


	pthread_exit(NULL);

	return NULL;
}


#ifdef HAVE_MYSQL

void ui_db_menu(int *running)
{
   int n_choices = ARRAY_SIZE(db_menu_choices);
   int choice;
   int new_msg = 0;
   char msg[128] = "";
   
   while(*running)
   {
      choice = ui_menu("Database Menu", db_menu_choices, NULL, n_choices,
                       ((new_msg==0) ? NULL : msg)
      );
      new_msg = 0;
      if (choice < 0 || choice == (n_choices-1))
      {
         break;
      }
      else
      {
         switch(choice)
         {
         case 0 : ui_db_parms(1); break; // New Connection Parameters
         case 1 : ui_db_parms(0); break;
         case 2 : // Reset Tables
            if (ui_db_reset())
            {
               sprintf(msg, "non-ADM tables cleared");
            }
            else
            {
               sprintf(msg, "Unable to clear tables. See error log for details.");
            }
            new_msg = 1;
            break; 
         case 3 :
            // Clear Received Reports
            if (ui_db_clear_rpt())
            {
               sprintf(msg, "Reports Cleared");
            }
            else
            {
               sprintf(msg, "Unable to clear reports. See error log for details.");
            }
            new_msg = 1;
            break; 
         case 4 :
            // Disconnect from DB
            ui_db_disconn();
            new_msg = 1;
            sprintf(msg, "Database Disconnected");
            break; 
         case 5 :
            // Connect to DB
            if (ui_db_conn())
            {
               sprintf(msg, "Successfully connected");
            }
            else
            {
               sprintf(msg, "Connection failed. See error log for details.");
            }
            new_msg = 1;
            break; 
         case 6 : ui_db_write(); break; // Write DB info to file.
         case 7 : ui_db_read(); break; // Read DB infor from file.
         }
      }
   }
}

int ui_db_conn()
{
    sql_db_t parms;

	ui_db_disconn();

	memset(&parms, 0, sizeof(sql_db_t));

	lockResource(&(gMgrDB.sql_info.lock));

	memcpy(&parms, &(gMgrDB.sql_info), sizeof(sql_db_t));

	unlockResource(&(gMgrDB.sql_info.lock));

	return db_mgt_init(parms, 0, 1);
}

void ui_db_disconn()
{
    db_mgt_close();
}


void ui_db_write()
{
  FILE *fp = 0;
  char *tmp = NULL;
  
  tmp = ui_input_string("Enter file name.");

  if((fp = fopen(tmp, "w+")) == NULL)
  {
    printf("Can't open or create %s.\n", tmp);
    SRELEASE(tmp);
    return; 
  }


 lockResource(&(gMgrDB.sql_info.lock));

 fwrite(&(gMgrDB.sql_info.server), UI_SQL_SERVERLEN-1, 1, fp);
 fwrite(&(gMgrDB.sql_info.database), UI_SQL_DBLEN-1, 1, fp);
 fwrite(&(gMgrDB.sql_info.username), UI_SQL_ACCTLEN-1,1, fp);
 fwrite(&(gMgrDB.sql_info.password), UI_SQL_ACCTLEN-1,1, fp);

 unlockResource(&(gMgrDB.sql_info.lock));

fclose(fp);
  printf("Database infor written to %s.\n", tmp);
 
 SRELEASE(tmp);
}

void ui_db_read()
{
  FILE *fp = NULL;
  char *tmp = NULL;

  tmp = ui_input_string("Enter file name.");

  if(tmp == NULL)
  {
    printf("Error reading string.\n");
    return;  
  }
  if ((fp = fopen(tmp, "r")) == NULL)
  {
    printf("Can't open file %s.\n", tmp);
    SRELEASE(tmp);
    return;
  }

  lockResource(&(gMgrDB.sql_info.lock));

  if(fread(&(gMgrDB.sql_info.server), UI_SQL_SERVERLEN-1, 1, fp) <= 0)
    printf("Error reading server.\n");

  if(fread(&(gMgrDB.sql_info.database), UI_SQL_DBLEN-1, 1, fp) <= 0)
    printf("Error reading database.\n");

  if(fread(&(gMgrDB.sql_info.username), UI_SQL_ACCTLEN-1,1, fp) <= 0)
    printf("Error reading username.\n");

  if(fread(&(gMgrDB.sql_info.password), UI_SQL_ACCTLEN-1,1, fp) <= 0)
    printf("Error reading password.r\n");
 
  db_mgr_sql_persist();

  unlockResource(&(gMgrDB.sql_info.lock));
  fclose(fp);

  printf("Read from %s.\n", tmp);
  SRELEASE(tmp);
  return;
}

void ui_db_parms(int do_edit)
{
   int i;
   int n_choices = ARRAY_SIZE(db_conn_form_fields);
   if (do_edit)
   {
      lockResource(&(gMgrDB.sql_info.lock));
   }

   for(i = 0; i < n_choices; i++)
   {
      if (do_edit)
      {
         // Ensure form is editable
         db_conn_form_fields[i].opts_off &= ~O_EDIT;
      }
      else
      {
         // Ensure form is readonly
         db_conn_form_fields[i].opts_off |= O_EDIT;
      }
   }
   
   ui_form("SQL Database Connection Information",
           ((do_edit) ? "Update Connection Information" : "This form is read-only"),
           db_conn_form_fields,
           n_choices
   );
   AMP_DEBUG_ERR("ui_db","DEBUG: server=%d='%s'",strlen(gMgrDB.sql_info.server), gMgrDB.sql_info.server);
   if (do_edit)
   {
      db_mgr_sql_persist();
      unlockResource(&(gMgrDB.sql_info.lock));
   }

}

int ui_db_reset()
{
   int rtv;
   ui_printf("Clearing non-ADM tables in the Database....\n");
	rtv = db_mgt_clear();
	ui_printf("Done!\n\n");
    return rtv;
}

int ui_db_clear_rpt()
{
	ui_printf("Not implemented yet.\n");
    return 0;
}

#endif

void ui_ctrl_list_menu(int *running)
{
   int choice;
   int n_choices = ARRAY_SIZE(ctrl_menu_list_choices);
   char msg[128] = "";
   int new_msg = 0, i;
   char *ctrl_menu_list_descriptions[10];
   uvast mask = 0;

   ctrl_menu_list_descriptions[0] = NULL;
   for(i = 1; i < 10; i++)
   {
      ctrl_menu_list_descriptions[i] = malloc(32);
   }
   sprintf(ctrl_menu_list_descriptions[1], "(%d known)", gVDB.adm_edds.num_elts);
   sprintf(ctrl_menu_list_descriptions[2], "(%d known)",  gVDB.adm_atomics.num_elts);
   sprintf(ctrl_menu_list_descriptions[3], "(%d known)",  gVDB.adm_ctrl_defs.num_elts);
   sprintf(ctrl_menu_list_descriptions[4], "(%d known)",  gVDB.macdefs.num_elts);
   sprintf(ctrl_menu_list_descriptions[5], "(%d known)",  gVDB.adm_ops.num_elts);
   sprintf(ctrl_menu_list_descriptions[6], "(%d known)",  gVDB.rpttpls.num_elts);
   sprintf(ctrl_menu_list_descriptions[7], "(%d known)",  gVDB.rules.num_elts);
   sprintf(ctrl_menu_list_descriptions[8], "(%d known)",  gVDB.adm_tblts.num_elts);
   sprintf(ctrl_menu_list_descriptions[9], "(%d known)",  gVDB.vars.num_elts);
   

   while(*running)
   {
      choice = ui_menu("ADM Object Information Lists", ctrl_menu_list_choices, ctrl_menu_list_descriptions, n_choices, 
                       ((new_msg==0) ? NULL : msg)
      );
      new_msg = 0;
      
      if (choice < 0 || choice > (n_choices-1))
      {
         break;
      }
      else
      {
         switch(choice)
         {
         case 0: ui_list_objs(ui_input_adm_id(), TYPE_MASK_ALL, NULL);
            break;
         case 1 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_EDD), NULL);
            break;
         case 2:
        	 mask = TYPE_AS_MASK(AMP_TYPE_CNST) | TYPE_AS_MASK(AMP_TYPE_LIT);
            ui_list_objs(ui_input_adm_id(), mask, NULL);
            break;

         case 3 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_CTRL), NULL);
            break;

         case 4 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_MAC), NULL);
            break;

         case 5 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_OPER), NULL);
            break;

         case 6 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_RPTTPL), NULL);
            break;

         case 7 :
        	 mask = TYPE_AS_MASK(AMP_TYPE_SBR) | TYPE_AS_MASK(AMP_TYPE_TBR);
             ui_list_objs(ui_input_adm_id(), mask, NULL);
            break;

         case 8 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_TBLT), NULL);
            break;

         case 9 : ui_list_objs(ui_input_adm_id(), TYPE_AS_MASK(AMP_TYPE_VAR), NULL);
            break;

         default:
            new_msg = 1;
            sprintf(msg, "ERROR: Menu choice %d is not currently supported.", choice);
         }

      }
   }

   for(i = 1; i < 10; i++)
   {
      free(ctrl_menu_list_descriptions[i]);
   }

}

FILE *display_fd = NULL;

/** Close any open stdout redirects and restore normal output */
void ui_display_to_file_close()
{
   if (display_fd != NULL)
   {
      fclose(display_fd);
      
      display_fd = NULL;
   }
}
   
/** ui_display_to_file
 *  Redirect subsequent ui_init() and ui_printf() output to the specified file.
 *  The file will be closed and normal behavior restored when ui_display_exec() 
 *  is called.
 */
int ui_display_to_file(char* filename)
{
   if (display_fd != NULL)
   {
      ui_display_to_file_close();
   }

   display_fd = fopen(filename, "w");
   if (display_fd == NULL)
   {
      return AMP_FAIL;
   }
   return AMP_OK;
}

void ui_fprintf(ui_print_cfg_t *fd, const char* format, ...)
{
   va_list args;
   va_start(args, format);
   if (fd != NULL) {
      if (fd->fd != NULL)
      {
         vfprintf(fd->fd, format, args);
      }
#ifdef USE_CIVETWEB
      else if (fd->conn != NULL)
      {
         mg_vprintf(fd->conn, format, args);
      }
#endif
      else
      {
         printf("NM UI ERROR: ui_fprintf called with illegal arguments\n");
      }
   }
   else if (display_fd != NULL)
   {
      vfprintf(display_fd, format, args);
   } else {
#ifdef USE_NCURSES
      vw_printw(ui_dialog_win, format, args);
#else
      vprintf(format, args);
#endif
   }
   va_end(args);
}

#ifdef USE_NCURSES
void ui_init()
{
    /* Redirect STDERR (ie: AMP_DEBUG_*) to file */
    fflush(stderr);
    if (freopen(NM_LOG_FILE, "w", stderr) == NULL)
    {
       printf("WARNING: stderr file redirection failed (%i). Warning messages may interfere with NCURSES menus", errno);
    }
   
    /* Initialize curses */
	initscr();
    start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

    /* Initialize a few color pairs */
   	init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);

    /* Initialize default dialog window */
    ui_dialog_win = newpad(LINES * UI_DIALOG_PAGES, UI_DIALOG_WIDTH);
    keypad(ui_dialog_win, TRUE);
    ui_dialog_pan = new_panel(ui_dialog_win);

    scrollok(ui_dialog_win, TRUE);

    hide_panel(ui_dialog_pan);
    update_panels();
    doupdate();
}
void ui_shutdown()
{
   endwin();
   ui_display_to_file_close();
}

/** Clear the default window of all content and append the specified title.
 *   To subsequently show this window, call ui_display_exec().  To add
 *   content to the display, call ui_printf.
 */
void ui_display_init(char* title)
{
   if (display_fd != NULL)
   {
      // Print to file: Use Markdown-style heading
      ui_printf("%s\n============\n", title);
   }
   else
   {
      wclear(ui_dialog_win);
      print_in_middle(ui_dialog_win, 1, 0, COLS + 4, title, COLOR_PAIR(1));
      wmove(ui_dialog_win, 3, 0); // Add a break after the title
   }
}

/** Displays default dialog window (populated with ui_printf). 
 *   The first non-navigation keyboard input will cause the window
 *   to be hidden and the input character returned to the user.
 *
 *   If window content exceeds the visible window, arrow, page and home/end
 *   keys may be used for navigation.
 */
int ui_display_exec()
{
   int c;
   int running = 1;
   int pos = 0;

   if (display_fd != NULL)
   {
      ui_display_to_file_close();
      return AMP_OK;
   }
   
   show_panel(ui_dialog_pan);
   update_panels();
   doupdate();
   
   while(running)
   {
      refresh();
      prefresh(ui_dialog_win, pos, 0, 0, 0, LINES-1, COLS);

      c = getch();
      switch(c)
      {
      case KEY_HOME:
         pos = 0;
         break;
      case KEY_END:
         // TODO
         break;
      case ' ':
      case KEY_NPAGE:
         pos += LINES-5;
         break;
      case KEY_PPAGE:
         pos -= LINES-5;
         break;
      case KEY_UP:
         pos--;
         break;
      case KEY_DOWN:
         pos++;
         break;

      default:
         // FIXME: Quit confirmation is used here primarily to hide a bug where parent menu may not refresh when this pad panel is hidden
         running = ui_menu("Return to previous screen?", bool_menu_choices, NULL, 2, NULL);;
      }
      if (pos < 0)
      {
         pos = 0;
      }
   }
   hide_panel(ui_dialog_pan);
   update_panels();
   doupdate();

   return c;
}

void ui_update_line(WINDOW *win, char* msg, int line, chtype color)
{
   wmove(win,line,2); // Move to start of line
   wclrtoeol(win); // Clear the line
   
   // Write updated status message
   wattron(win, color);
   mvwprintw(win,line, 2, msg);
   wattroff(win, color);
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

int ui_prompt(char* title, char* choiceA, char* choiceB, char* choiceC)
{

   ITEM *my_items[4];
   WINDOW *my_win;
   MENU *my_menu;
   PANEL *my_panel;
   int running = 1;
   int rtv = 0;
   int c, maxChoiceLen;
   int ncols = 8;

   // Build first menu item
   my_items[0] = new_item(choiceA, NULL);
   
   // Calculate Dialog Width
   if (choiceB == NULL)
   {
      // This is a single-choice dialog
      maxChoiceLen = strlen(choiceA);
      my_items[1] = NULL;
   }
   else
   {
      maxChoiceLen = MAX(strlen(choiceA), strlen(choiceB));
      my_items[1] = new_item(choiceB, NULL);

      if(choiceC != NULL)
      {
         maxChoiceLen = MAX(maxChoiceLen, strlen(choiceC));
         ncols += maxChoiceLen + 4;
         my_items[2] = new_item(choiceC, NULL);
         my_items[3] = NULL;         
      }
      else
      {
         my_items[2] = NULL;
      }
   }

   ncols += maxChoiceLen*2;
   ncols = MAX(ncols, strlen(title)+4);

   my_menu = new_menu(my_items);

   // Create a new Window
   my_win = newwin(5, // height
                   ncols, // width
                   LINES/2, // start y
                   (COLS-ncols)/2); // start x
   my_panel = new_panel(my_win);
   
   keypad(my_win, TRUE);
   set_menu_win(my_menu, my_win);
   set_menu_sub(my_menu, derwin(my_win, 0, 0, 3, 2)); // Menu position within window
   set_menu_format(my_menu, 1, 3);
   set_menu_spacing(my_menu, TABSIZE, 0, 0);
   menu_opts_off(my_menu, O_SHOWDESC | O_NONCYCLIC);

   // Add a title and optional border
   box(my_win, 0, 0);
   print_in_middle(my_win, 1, 0, ncols, title, COLOR_PAIR(1));
   
   post_menu(my_menu);
   wrefresh(my_win);

   update_panels();
   doupdate();
   
   while(running)
   {
      c = wgetch(my_win);
      switch(c)
      {
      case KEY_END:
         running = 0;
         rtv = 0;
         break;
      case KEY_LEFT:
      case KEY_UP:
         menu_driver(my_menu, REQ_LEFT_ITEM);
         break;
      case KEY_RIGHT:
      case KEY_DOWN:
         menu_driver(my_menu, REQ_RIGHT_ITEM);
         break;
      case ' ':
      case KEY_ENTER:
      case 10: // return key
         running = 0;
         rtv = item_index(current_item(my_menu));
         break;
      }
   }

   del_panel(my_panel);
   unpost_menu(my_menu);
   free_menu(my_menu);
   free_item(my_items[0]);
   free_item(my_items[1]);
   if (choiceC != NULL)
   {
      free_item(my_items[2]);
   }   
   delwin(my_win);
   endwin();             
   return rtv;
}

/** Trim any trailing whitespace from given string.
 * Operation is performed in place by writing null characters, however the
 *  original string pointer is returned for user convenience.
 */
char* trimstring(char* str)
{
   int i;
   for(i = strlen(str); i > 0; i--)
   {
      switch(str[i])
      {
      case '\0':
      case '\n':
      case '\t':
      case ' ':
         str[i] = '\0';
         break;
      default:
         // Break at any non-whitespace character
         return str;
      }
   }
   return str;
}

/**
 * @returns -1 on error, 0 if user cancelled input, 1 if user submitted form.
 */
int ui_form(char* title, char* msg, form_fields_t *fields, int num_fields)
{
    FIELD **field = calloc(num_fields+1, sizeof(FIELD*));
    WINDOW *my_form_win;
    PANEL *my_pan;
	FORM  *my_form;
	int ch, i, w;
    int rows, cols;
    int running = 1;
    int status = 0;
    char tmp[32];
	
    /* Initialize the fields */
	for(i = 0; i < num_fields; ++i)
    {
       int w = fields[i].width-1;
       if (w > FORM_MAX_WIDTH)
       {
          w = FORM_MAX_WIDTH;
       }
       field[i] = new_field(1, w, STARTY + i * 2, FORM_STARTX, 0, 0);

       /* Set field options */
       set_field_back(field[i], A_UNDERLINE); 	/* Print a line for the option 	*/
       if (fields[i].width > FORM_MAX_WIDTH)
       {
          // Allow this field to be scrollable
          field_opts_off(field[i], O_STATIC);

          // Up to the defined maximum width
          set_max_field(field[i], fields[i].width-1);
       }

       // Disable selected options
       if (fields[i].opts_off != 0)
       {
          field_opts_off(field[i], fields[i].opts_off);
       }
       
    }
	field[num_fields] = NULL;


    /* Field Labels & Default Values */
    for(i = 0; i < num_fields; i++)
    {
       set_field_just(field[i], JUSTIFY_RIGHT); /* Center Justification */

       // Default value
       if (fields[i].parsed_value != NULL)
       {
          switch(fields[i].type)
          {
          case TYPE_CHECK_INT:
          case TYPE_CHECK_NUM:
             sprintf(tmp, "%i", *((int*)fields[i].parsed_value) );
             break;
          case TYPE_CHECK_BOOL:
             if ( (*((int*)fields[i].parsed_value)) == 1)
             {
                sprintf(tmp, "True");
             }
             else
             {
                sprintf(tmp, "False");
             }
             break;
          default:
             tmp[0] = 0; // Leave it blank / unimplemented
          }
          
          // Set it
          set_field_buffer(field[i], 0, tmp);

          // Mark as unmodified
          set_field_status(field[i], FALSE);
       }
       else if (fields[i].value != NULL && strlen(fields[i].value) > 0)
       {
          // Set it
          set_field_buffer(field[i], 0, fields[i].value);

          // Mark as unmodified
          set_field_status(field[i], FALSE);
       }

       // Field Validation
       switch(fields[i].type)
       {
       case TYPE_CHECK_ALPHA:
          set_field_type(field[i], TYPE_ALPHA, fields[i].args.width);
          break;

       case TYPE_CHECK_ALNUM:
          set_field_type(field[i], TYPE_ALNUM, fields[i].args.width);
          break;

       case TYPE_CHECK_ENUM:
          set_field_type(field[i], TYPE_ENUM,
                         fields[i].args.en.valuelist, fields[i].args.en.checkcase, fields[i].args.en.checkunique);
          break;

       case TYPE_CHECK_INT:
          set_field_type(field[i], TYPE_INTEGER,
                         fields[i].args.num.padding, fields[i].args.num.vmin, fields[i].args.num.vmax);
          break;          
       case TYPE_CHECK_NUM:
          set_field_type(field[i], TYPE_NUMERIC,
                         fields[i].args.num.padding, fields[i].args.num.vmin, fields[i].args.num.vmax);
          break;          

       case TYPE_CHECK_REGEXP:
          set_field_type(field[i], TYPE_REGEXP, fields[i].args.regex);
          break;
       case TYPE_CHECK_BOOL:
          // Nothing to do here, validated below
          // TODO: Define this as TYPE_ALNUM matching True|False|0|1|t|f
       case TYPE_CHECK_NONE:
       default:
             // Nothing to do
          break;
       }
    }

	/* Create the form and post it */
	my_form = new_form(field);
    scale_form(my_form, &rows, &cols);

    /* Create the window */
    my_form_win = newwin(0,0,0,0);
    keypad(my_form_win, TRUE);
    my_pan = new_panel(my_form_win);
    set_form_win(my_form, my_form_win);
    set_form_sub(my_form, derwin(my_form_win, rows, cols, 0, 0)); // ???
	post_form(my_form);
	wrefresh(my_form_win);

    // Print title /border
    // NOTE: In example, this is done before refresh - but here doing so prevents title from appearing
    box(my_form_win, 0, 0);
    print_in_middle(my_form_win, 1, 0, cols + 4, title, COLOR_PAIR(1));
    
    // Print footer
    mvwprintw(my_form_win,LINES - 3, 4, "F1 to Cancel, F2 to Submit");
    if (msg != NULL)
    {
       wattron(my_form_win, COLOR_PAIR(2));       
       mvwprintw(my_form_win,LINES - 2, 4, msg);
       wattroff(my_form_win, COLOR_PAIR(2));
    }

    // Field Labels
    for(i = 0; i < num_fields; i++)
    {
       mvwprintw(my_form_win,STARTY+i*2, STARTX - 10, fields[i].title);
    }

	refresh();

    // Ensure first field has focus
    form_driver(my_form, REQ_FIRST_FIELD);

	/* Loop through to get user requests */
	while(running && (ch = wgetch(my_form_win)) != KEY_F(1))
	{
       show_panel(my_pan);
       update_panels();
       doupdate();
       
       switch(ch)
		{
        case KEY_LEFT:
           form_driver(my_form, REQ_PREV_CHAR);
           break;
        case KEY_RIGHT:
           form_driver(my_form, REQ_NEXT_CHAR);
           break;
        case KEY_UP:
           /* Go to previous field */
           form_driver(my_form, REQ_PREV_FIELD);
           form_driver(my_form, REQ_END_LINE);
           break;
       case KEY_BACKSPACE:
       case 127:
          form_driver(my_form, REQ_DEL_PREV);
          break;
        case KEY_DC: // Delete key (under cursor)
           form_driver(my_form, REQ_DEL_CHAR);
           break;
        case KEY_DOWN:
        case 10: // return key
        case KEY_ENTER: // numpad enter key
           // Check if this is the last entry
           if (field_index(current_field(my_form)) == num_fields-1)
           {
              if (ch == KEY_DOWN)
              {
                 // Do nothing for KEY_DOWN
                 break;
              }
              else
              {
                 // Prompt User if they wish to save or continue.
                 i = ui_prompt("Submit Form?",  "Submit", "Abort", "Cancel");
                 if (i == 2)
                 {
                    // Continue Editing
                    touchwin(my_form_win);
                    wrefresh(my_form_win);
                    refresh();
                    break;
                 }
                 else if (i == 1)
                 {
                    // Abort without saving
                    running = 0;
                    break;
                 }
                 // else fall through to F2 handling of Save/Submit (after clearing prompt, in case validation fails)
                 touchwin(my_form_win);
                 wrefresh(my_form_win);
                 refresh();

              }
           }
           else
           {
              /* Go to next field */
              form_driver(my_form, REQ_NEXT_FIELD);
              /* Go to the end of the present buffer */
              /* Leaves nicely at the last character */
              form_driver(my_form, REQ_END_LINE);
              break;
           }

          // Submit:
        case KEY_F(2):
           // Check that current field is validated before proceeding
           if (form_driver(my_form, REQ_VALIDATION) != E_OK)
           {
              ui_update_line(my_form_win,
                             "Field validation failed. Please correct input for current field.",
                             LINES-2,
                             COLOR_PAIR(2)
              );
              break;
           }

           // Ensure current field is flushed
           form_driver(my_form, REQ_NEXT_FIELD);

           // Check that all required fields (O_NULLOK disabled) have been defined
           for(i = 0; i < num_fields; i++)
           {
              if ((fields[i].opts_off & O_NULLOK) && field_status(field[i]) == FALSE)
              {
                 set_current_field(my_form, field[i]);
                 ui_update_line(my_form_win,
                                "ERROR: This field is required",
                                LINES-2,
                                COLOR_PAIR(2)
                 );
                 i = -1;
                 break;
              }
           }
           if (i < 0)
           {
              break; // At least one field did not validate
           }
           
           // Stop the loop
           running = 0;
           status = 1;
           
           // Retrieve content (do not copy back static fields)
           for(i = 0; i < num_fields; i++)
           {
              if ( !(fields[i].opts_off & O_EDIT) )
              {
                 /* Copy value back from primary buffer. 
                  *   Additional buffers (seocnd arg) not currently used.
                  *   NCurses automatically pads all fields with spaces, so we trim it before copying back
                  */
                 if (fields[i].value != NULL)
                 {
                    strcpy(fields[i].value, trimstring(field_buffer(field[i], 0)));
                 }
                 if (fields[i].parsed_value != NULL)
                 {
                    switch(fields[i].type)
                    {
                    case TYPE_CHECK_INT:
                    case TYPE_CHECK_NUM:
                       *((int*)fields[i].parsed_value) = atoi(field_buffer(field[i], 0));
                       break;
                    case TYPE_CHECK_BOOL:
                       switch(field_buffer(field[i], 0)[0])
                       {
                       case '1':
                       case 't':
                       case 'T':
                       case '+':
                          *((int*)fields[i].parsed_value)=1;
                          break;
                       case '0':
                       case 'f':
                       case 'F':
                       case '-':
                       default: // For now, anything that isn't a valid truth string is considered false
                          // (Alternatively, default case could fail validation ... but that seems unnecessary for BOOL)
                          *((int*)fields[i].parsed_value)=0;
                          break;
            
                       }
                    default:
                       break; // Unsupported
                    }
                 }
              }
           }
           break;
        default:
           /* If this is a normal character, it is added to buffer*/
           form_driver(my_form, ch);
           break;
		}
       //wrefresh(my_form_win);
       //refresh();
	}

    hide_panel(my_pan);
    update_panels();
    doupdate();
    
	/* Un post form and free the memory */
	unpost_form(my_form);
	free_form(my_form);

    for(i = 0; i < num_fields; i++)
    {
       free_field(field[i]);
    }

    free(field);
    del_panel(my_pan);
    delwin(my_form_win);
	endwin();
	return status;
}

int ui_menu(char* title, char** choices, char** descriptions, int n_choices, char* msg)
{
   WINDOW *my_menu_win;
   PANEL *my_pan;
   ITEM **my_items;
	int c;				
	MENU *my_menu;
	int i;
	ITEM *cur_item;
	int running = 1;
    int rtv = -1;
    char label[4];
		
	my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));

	for(i = 0; i < n_choices; ++i)
    {
       my_items[i] = new_item(choices[i], (descriptions == NULL) ? NULL : descriptions[i]);
    }
	my_items[n_choices] = (ITEM *)NULL;

	my_menu = new_menu((ITEM **)my_items);

    // Create a new window
    my_menu_win = newwin(0,0,0,0);
    keypad(my_menu_win, TRUE);
    my_pan = new_panel(my_menu_win);
    set_menu_win(my_menu, my_menu_win);
    set_menu_sub(my_menu, derwin(my_menu_win, LINES-4, 0,
                                 MENU_START_LINE,
                                 4 //Menu Start Column
    ));

    // Add a title and optional border
    box(my_menu_win, 0, 0);
    print_in_middle(my_menu_win, 1, 0, COLS + 4, title, COLOR_PAIR(1));
    
    // Menu Formatting
    set_menu_mark(my_menu, " * ");

    // Quick Select Labels (TODO: support for menus with > 10 items)
    for(i = 0; i < n_choices && n_choices <= 10; i++)
    {
       sprintf(label,"%hd.",i);
       mvwprintw(my_menu_win, MENU_START_LINE+i, 1, label);
    }
    
	mvwprintw(my_menu_win,LINES - 3, 2, "F1 or 'e' to Exit");
    if (msg != NULL)
    {
       wattron(my_menu_win, COLOR_PAIR(2));
       mvwprintw(my_menu_win,LINES - 2, 2, msg);
       wattroff(my_menu_win, COLOR_PAIR(2));
    }
	post_menu(my_menu);
	wrefresh(my_menu_win);

	while(*global_nm_running && running && (c = wgetch(my_menu_win)) != KEY_F(1))
	{
       show_panel(my_pan);
       update_panels();
       doupdate();

       switch(c)
       {
       case 'e':
       case 'E':
          running = 0;
          break;
       case KEY_DOWN:
          menu_driver(my_menu, REQ_DOWN_ITEM);
          break;
       case KEY_UP:
          menu_driver(my_menu, REQ_UP_ITEM);
          break;
       case KEY_ENTER: // numpad enter key
       case 10: // return key
          running = 0;
          rtv = item_index(current_item(my_menu));
          break;
       default:
          if (c >= '0' && c <= '9')
          {
             running = 0;
             rtv = c - '0';
          }
       }
       wrefresh(my_menu_win);
	}

    hide_panel(my_pan);
    update_panels();
    doupdate();

    unpost_menu(my_menu);
    free_menu(my_menu);
    for(i = 0; i < n_choices; i++)
    {
       free_item(my_items[i]);
    }
	endwin();
    del_panel(my_pan);
    delwin(my_menu_win);
    return rtv;
}

// Only navigation keys and F1 to exit are processed by this function by default.
// Selection handling is reserved for the user callback fn unless UI_OPT_ENTER_SEL flag is set.
int ui_menu_listing(
   char* title, ui_menu_list_t* list, int n_choices,
   char* status_msg, int default_idx, char* usage_msg,
   ui_menu_listing_cb_fn fn, int flags)
{
   int menu_cols = 1; // TOOD: Make this a parameter
   WINDOW *my_menu_win, *my_subwin=NULL;
   PANEL * my_pan;
    ITEM **my_items;
	int c;				
	MENU *my_menu;
	int i, status;
	ITEM *cur_item;
	int running =1;
    int rtv = -1;
    char label[4];
    int menu_height = LINES-8;
		
	my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));

	for(i = 0; i < n_choices; ++i)
    {
       if (UI_OPT_SPLIT_DESCRIPTION & flags)
       {
          my_items[i] = new_item(list[i].name, NULL);
       }
       else
       {
          my_items[i] = new_item(list[i].name, list[i].description);
       }
    }
	my_items[n_choices] = (ITEM *)NULL;

	my_menu = new_menu((ITEM **)my_items);

    // Create a new window
    my_menu_win = newwin(0,0,0,0); // full size window
    keypad(my_menu_win, TRUE);
    my_pan = new_panel(my_menu_win);
    set_menu_win(my_menu, my_menu_win);
    set_menu_sub(my_menu, derwin(my_menu_win,
                                 LINES-4, // Menu height
                                 0, // Menu width
                                 MENU_START_LINE,
                                 4 //Menu Start Column
    ));
    if (UI_OPT_SPLIT_DESCRIPTION & flags)
    {
       menu_height = menu_height / 2;

       my_subwin = subwin(my_menu_win,
                          (LINES-8)/2, // Height of child window
                          0, // Number of columns - make it full width
                          LINES/2, // Start line - about halfway down the page
                          4 // Start column
       );
       set_menu_format(my_menu, (LINES-8)/2, menu_cols);
    }
    else
    {
       set_menu_format(my_menu, LINES-8, menu_cols);
    }

    // Add a title and optional border
    box(my_menu_win, 0, 0);
    print_in_middle(my_menu_win, 1, 0, COLS + 4, title, COLOR_PAIR(1));
    
    // Menu Formatting
    set_menu_mark(my_menu, " * ");

    // Quick Select Labels
    if (UI_OPT_AUTO_LABEL & flags)
    {
       for(i = 0; i < n_choices && n_choices < 10; i++)
       {
          sprintf(label,"%hd.",i);
          mvwprintw(my_menu_win, MENU_START_LINE+i, 1, label);
       }
    }

    if (usage_msg != NULL)
    {
       mvwprintw(my_menu_win,LINES - 4, 2, usage_msg);
    }
    if (status_msg != NULL)
    {
       wattron(my_menu_win, COLOR_PAIR(2));
       mvwprintw(my_menu_win,LINES - 2, 2, status_msg);
       wattroff(my_menu_win, COLOR_PAIR(2));
    }
	post_menu(my_menu);
	wrefresh(my_menu_win);

    if (default_idx > 0)
    {
       set_current_item(my_menu, my_items[i]);
    }

	while(running && *global_nm_running)
	{
       i = item_index(current_item(my_menu));

       if (UI_OPT_SPLIT_DESCRIPTION & flags)
       {
          wclear(my_subwin);

          // Add a border
          box(my_subwin, 0, 0);
          
          // Title - echo current selection
          print_in_middle(my_subwin, 1, 0, COLS, list[i].name, COLOR_PAIR(1) );

          // Content
          if (list[i].description != NULL)
          {
             mvwprintw(my_subwin, 3,4, list[i].description);
          }
          
          touchwin(my_menu_win);
          wrefresh(my_menu_win);
          wrefresh(my_subwin);
       }
       else
       {
          touchwin(my_menu_win);
          wrefresh(my_menu_win);
       }
       show_panel(my_pan);
       update_panels();
              
       c = wgetch(my_menu_win);
       
       switch(c)
       {
       case KEY_F(1):
          running = 0;
          break;
       case KEY_DOWN:
          menu_driver(my_menu, REQ_DOWN_ITEM);
          break;
       case KEY_UP:
          menu_driver(my_menu, REQ_UP_ITEM);
          break;
       case KEY_ENTER: // numpad enter key
       case 10: // return key
          if (flags & UI_OPT_ENTER_SEL) {
             running = 0;
             rtv = item_index(current_item(my_menu));
             break;
          } // else fall through to default case
          // WARNING: Above will fall through to default in select cases. Update with caution.
       default:
          // If auto-labeling is active, check for quick-select entries (first 10 entries only)
          if ((flags & UI_OPT_AUTO_LABEL) && c >= '0' && c <= '9')
          {
             running = 0;
             rtv = c - '0';
          }
          else if (fn != NULL)
          {
             // Hide panel during callback processing to minimize unexpected display glitches
             hide_panel(my_pan);
             update_panels();

             status = fn(i, c, list[i].data, status_msg);
             if (status < 0)
             {
                rtv = status;
                running = 0;
             }
             else if (status == UI_CB_RTV_STATUS)
             {
                // Continue processing but show an updated message
                if (status_msg != NULL)
                {
                   ui_update_line(my_menu_win,
                                  status_msg,
                                  LINES-2,
                                  COLOR_PAIR(2)
                   );
                }
             }
             else if (status == UI_CB_RTV_CHOICE)
             {
                rtv = i;
                running = 0;
             }
             // else continue
          }
       }
       
       //wrefresh(my_menu_win);
       //refresh();
	}

    hide_panel(my_pan);
    update_panels();
    doupdate();

    
    unpost_menu(my_menu);
    free_menu(my_menu);
    for(i = 0; i < n_choices; i++)
    {
       free_item(my_items[i]);
    }
	endwin();
    del_panel(my_pan);
    if(my_subwin)
    {
       delwin(my_subwin);
    }
    delwin(my_menu_win);
    return rtv;
}

int ui_menu_select(char* title, const char* const* choices, const char* const* descriptions, int n_choices, char* msg, int menu_cols)
{
   WINDOW *my_menu_win;
   PANEL *my_pan;
   ITEM **my_items;
	int c;				
	MENU *my_menu;
	int i;
	ITEM *cur_item;
	int running =1;
    int rtv = -1;
    char label[4];
		
	my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));

	for(i = 0; i < n_choices; ++i)
    {
       my_items[i] = new_item(choices[i], (descriptions == NULL) ? NULL : descriptions[i]);
    }
	my_items[n_choices] = (ITEM *)NULL;

	my_menu = new_menu((ITEM **)my_items);

    // Create a new window
    my_menu_win = newwin(0,0,0,0);
    keypad(my_menu_win, TRUE);
    my_pan = new_panel(my_menu_win);
    set_menu_win(my_menu, my_menu_win);
    set_menu_sub(my_menu, derwin(my_menu_win, LINES-4, 0,
                                 MENU_START_LINE,
                                 4 //Menu Start Column
    ));

    /* Set menu option not to show the description */
	menu_opts_off(my_menu, O_SHOWDESC | O_NONCYCLIC);
   
    // Add a title and optional border
    box(my_menu_win, 0, 0);
    print_in_middle(my_menu_win, 1, 0, COLS + 4, title, COLOR_PAIR(1));
    
    // Menu Formatting
    set_menu_mark(my_menu, " * ");
	set_menu_format(my_menu, LINES-8, menu_cols);

	mvwprintw(my_menu_win,LINES - 3, 2, "F1 to Exit");
    if (msg != NULL)
    {
       wattron(my_menu_win, COLOR_PAIR(2));
       mvwprintw(my_menu_win,LINES - 2, 2, msg);
       wattroff(my_menu_win, COLOR_PAIR(2));
    }
	post_menu(my_menu);
	wrefresh(my_menu_win);

	while(running && (c = wgetch(my_menu_win)) != KEY_F(1))
	{
       show_panel(my_pan);
       update_panels();
       doupdate();

       switch(c)
       {
       case KEY_LEFT:
          menu_driver(my_menu, REQ_LEFT_ITEM);
          break;
       case KEY_RIGHT:
          menu_driver(my_menu, REQ_RIGHT_ITEM);
          break;
       case KEY_DOWN:
          menu_driver(my_menu, REQ_DOWN_ITEM);
          break;
       case KEY_UP:
          menu_driver(my_menu, REQ_UP_ITEM);
          break;
       case KEY_NPAGE:
          menu_driver(my_menu, REQ_SCR_DPAGE);
          break;
       case KEY_PPAGE:
          menu_driver(my_menu, REQ_SCR_UPAGE);
          break;
       case KEY_ENTER: // numpad enter key
       case 10: // return key
          running = 0;
          rtv = item_index(current_item(my_menu));
          break;
       default:
          if (c >= '0' && c <= '9')
          {
             running = 0;
             rtv = c - '0';
          }
       }
       wrefresh(my_menu_win);
	}

    hide_panel(my_pan);
    update_panels();
    doupdate();

    unpost_menu(my_menu);
    free_menu(my_menu);
    for(i = 0; i < n_choices; i++)
    {
       free_item(my_items[i]);
    }
    endwin();
    del_panel(my_pan);
    delwin(my_menu_win);
    return rtv;
}


#else // !USE_NCURSES

int ui_prompt(char* title, char* choiceA, char* choiceB, char* choiceC)
{
   int rtv = 0, cnt = 0;
   ui_display_init(title);

   if (choiceA != NULL && choiceB==NULL && choiceC == NULL)
   {
      printf("\t %s\n", choiceA);
   }
   else
   {
      if (choiceA != NULL)
      {
         printf("0. %s\n", choiceA);
      }
      if (choiceB != NULL)
      {
         printf("1. %s\n", choiceB);
      }
      if (choiceC != NULL)
      {
         printf("2. %s\n", choiceC);
      }
      printf("\n");
      rtv = ui_input_uint("Select by #:");
   }
   ui_display_exec();
   return rtv;
}

int ui_menu(char* title, char** choices, char** descriptions, int n_choices, char* msg)
{
   int i;
   ui_display_init(title);

   while(*global_nm_running) {

      for(i = 0; i < n_choices; i++)
      {
         printf("%i. %s", i, choices[i]);
         if (descriptions != NULL && descriptions[i] != NULL)
         {
            printf("\t %s\n", descriptions[i]);
         }
         else
         {
            printf("\n");
         }
      }
      
      if (msg != NULL)
      {
         printf("\n %s \n\n" ,msg);
      }

      i = ui_input_uint("Select by # (-1 to cancel, -2 to repeat menu):");

      if (i != -2) {
         break;
      }
   }
   
   ui_display_exec();

   return i;
}

int ui_menu_listing(
   char* title, ui_menu_list_t* list, int n_choices,
   char* status_msg, int default_idx, char* usage_msg,
   ui_menu_listing_cb_fn fn, int flags)
{
   int i = -1;
   int status;
   int running = 1;
   char line[20];

   while(running && *global_nm_running)
   {
      ui_display_init(title);

      for(i = 0; i < n_choices; i++)
      {
         printf("%3d) %s", i, list[i].name);
         if (list[i].description != NULL)
         {
        	int len = strlen(list[i].description);
        	printf("\n");
        	printf("     ---------------------------------------------------------------------------\n");
        	if(len > 74)
        	{
            	char tmp[75];
            	int remaining = len;
            	int idx = 0;
            	int delta = 0;
            	int space_idx;
            	while(remaining > 0)
            	{
            		delta = (remaining > 74) ? 74 : remaining;

            		if(delta == 74)
            		{
            			for(space_idx = delta-1; space_idx >= 0; space_idx--)
            			{
            				if(list[i].description[idx+space_idx] == ' ')
            				{
            					space_idx++;
            					break;
            				}
            			}
            			if(space_idx < 0)
            			{
            				space_idx = delta;
            			}
            		}
            		else
            		{
            			space_idx = delta;
            		}

            		memset(tmp, 0, 75);
            		strncat(tmp, (char *) list[i].description+idx, space_idx);
            		printf("     %.74s", tmp);

            		idx += space_idx;
            		remaining -= space_idx;
            		printf("\n");
            	}
        	}
        	else
        	{
        		printf("     %s\n", list[i].description);
        	}
        	printf("\n");
         }
         else
         {
            printf("\n");
         }
      }

      if (status_msg != NULL)
      {
         printf("\n %s \n\n" ,status_msg);
      }

      if (usage_msg != NULL)
      {
         printf("\n %s \n\n" ,usage_msg);
      }

      if (n_choices == 0)
      {
         printf("Error: No choices given\n");
         return -1;
      }
#if 0 // Disable: Accepting enter to continue does not work reliably due to seemingly phantom keystrokes from getchar()
      else if (n_choices == 1)
      {
         printf("Press enter to select sole choice, or any other key to abort\n");
         i = getchar();
         if (i == '0' || i == '\n')
         {
            // Select sole choice
            i = 0;
         }
         else
         {
            // Abort
            i = -1;
            running = 0;
            break;

         }
      }
#endif
      else
      {
         i = ui_input_uint("Select by # (-1 to cancel, -2 to repeat menu)");

         if (i == -2)
         {
            continue;
         }
         else if (i < 0 || i >= n_choices)
         {
            printf("Cancelling ...\n");
            running = 0;
            break;
         }
      }

      if (fn != NULL)
      {
         status = fn(i,
                     0, // keypress not used in this mode
                     list[i].data,
                     status_msg
         );
         if (status < 0)
         {
            i = status;
            running = 0;
         }
         else if (status == UI_CB_RTV_STATUS)
         {
            if (status_msg != NULL)
            {
               printf("\n%s\n", status_msg);
            }
         }
         else if (status == UI_CB_RTV_CHOICE)
         {
            running = 0;
         }
         // Else CONTINUE
      }
      else
      {
         // If there is no callback, then we always exit after a selection
         running = 0;
      }
   
      ui_display_exec();

   }

   return i;   

}

int ui_display_exec()
{
   if(display_fd != NULL) {
      ui_display_to_file_close();
   }
   else
   {
      printf("\n--------------------\n");
   }
   return AMP_OK;
}

/**
 * Create an interactive form containing multiple fields from configuration structure.
 *
 * - At end of input, user can confirm all fields entered and choose to edit or submit
 * - Optional field validation.
 * -  For bool, output will be normalized to a strlen=0 string if false
 * -  For numeric types, caller should call atoi onv alue to convert
 */
#define UI_FORM_LEN 128

static void ui_form_show_value(form_fields_t *field)
{
   if (field->parsed_value != NULL) {
      switch(field->type) {
      case TYPE_CHECK_INT:
      case TYPE_CHECK_NUM:
         printf(BOLD("%i"), *( (int*)(field->parsed_value) ) );
         break;
      case TYPE_CHECK_BOOL:
         if ( *((int*)field->parsed_value) == 0) {
            printf( BOLD("False" ) );
         } else {
            printf( BOLD("True" ) );
         }
         break;
      default:
         printf(BOLD("%i"), *( (int*)(field->parsed_value) ) );
      }
   } else if (field->value != NULL) {
      printf("%s", field->value);
   } else {
      printf( KRED "ERROR: Illegal field definition. No var defined to store result." RST);
   }
}

// Returns -1 if invalid, 1 if valid, 0 if empty and not required
// field->value and field->parsed_value will be populated if appropriate.
static int ui_form_field_validate(form_fields_t *field, char *value)
{
   int tmp, j, len;
         
   if (field == NULL)
   {
      return -1;
   }

   if (value == NULL)
   {
      // TODO: Is this field required?
      return 0;
   }

               
   // Check for validation (not all validation options shall be implemented)
   switch(field->type) {
      // TODO: What was the difference between these in ncurses?
   case TYPE_CHECK_INT:
   case TYPE_CHECK_NUM:
      // Iterate through chars in string and verify that each is numeric
      //   loop and call isdigit(in[i]) where fn provided by string.h
      for(j = 0, len = strlen(value); j < len; j++) {
         if(isdigit(value[j] == 0) ) {
            printf("*ERROR: Not a Number - ");
            return -1;
         }
      }
      if (field->parsed_value != NULL)
      {
         *((int*)field->parsed_value) = atoi(value);
      }      
      break;
   case TYPE_CHECK_BOOL:
      if (strlen(value) == 0) {
         tmp=0;
      } else {
         switch(value[0])
         {
         case '1':
         case 't':
         case 'T':
         case '+':
            tmp=1;
            break;
         case '0':
         case 'f':
         case 'F':
         case '-':
         default: // For now, anything that isn't a valid truth string is considered false
            // (Alternatively, default case could fail validation ... but that seems unnecessary for BOOL)
            tmp=0;
            break;
            
         }
      }
      if (field->parsed_value != NULL) {
         *((int*)field->parsed_value) = tmp;
      }
      if (field->value != NULL) {
         if (tmp == 0) {
            strncpy(field->value, "False", field->width);
         } else {
            strncpy(field->value, "True", field->width);
         }
      }
      return 1; // BOOL is a special case where we bypass nominal copying
      
   default:
      break; // Nothing to be done
      
   }
   if (field->value != NULL)
   {
      strncpy(field->value, value, field->width);
   }
   return 1;
}

static int do_ui_form_confirm(char* title, int status, form_fields_t *fields, int num_fields)
{
   int i;
   char tmp;
   
   // Print recap of all fields.  TODO: Check if any fields fail validation
   printf("-------\n" KGRN "%s (summary)\n" RST "-----\n", title);

  
   // If validation fails, prevent submission.
   for(i = 0; i < num_fields; i++)
   {
      form_fields_t *field = &fields[i];
      int tmp = 0;

      printf("%s:\t", field->title);
      ui_form_show_value(field);
      printf("\n");
   }

   if (status == 0) {
      printf("\n ERROR: One or more fields failed validation. Press 'e' to edit or 'c' to cancel.");
   } else {
      printf("\n Press 's' to submit, 'e' to edit, or 'c' to cancel\n");
   }
   while( (tmp = getchar()) ) {
      if (tmp == 'e') {
         // Try again
         return -1;
      } else if (tmp == 'c') {
         return 0;
      } else if (tmp == 's' && status != 0) {
         // NOTE: We require 's', because checking for newline is not always relaible
         return 1;
      }
   }
   return -2; // Input Error
}
static int do_ui_form(char* title, char* msg, form_fields_t *fields, int num_fields)
{
   int status = 1;
   char in[UI_FORM_LEN] = "";
   int i;

   if (msg != NULL) {
      printf(KGRN "%s\n" RST "%s\n-----\n", title, msg);
   }
      
   if (do_ui_form_confirm(title, status, fields, num_fields) != -1)
   {
      // User does not wish to edit settings
      return 0;
   }
   
   for(i = 0; i < num_fields && status; i++)
   {
      form_fields_t *field = &fields[i];
      int skipFlag = 0;
      
      // (Re-)Print Title Line and prompt
      // TODO: Update prompt
      printf(KGRN "%s (field %i of %i)\n" RST "-----\n%s: ",
             title,
             i+1, num_fields,
             field->title
         );
      ui_form_show_value(field);
      printf("\n");

      // Prompt User for input
      while(1) {
         int len;
         
         switch(field->type) {
            case TYPE_CHECK_INT:
            case TYPE_CHECK_NUM:
               printf("INT :->");
               break;
         case TYPE_CHECK_BOOL:
            printf("BOOL :->");
            break;
         default:
            printf(":->");
         }
         fflush(stdout);
         if (igets(STDIN_FILENO, in, UI_FORM_LEN, &len) == NULL || len == 0)
         {          
            if (field->parsed_value != NULL || (field->value != NULL && strlen(field->value) > 0) )
            {
               // Use default value
               /* NOTE:
                * If only field->value is defined with non-0 length, it shall have a valid default value
                * If parsed_value is defined, it is assummed to contain a valid default value.
                *  TODO: Additional validation options may be added in future if needed.
               */ 
               break;
            }
            else if (skipFlag==0)
            {
               // Require confirmation to skip
               printf("No input.  Press enter again to use default (if defined) or cancel.\n");
               skipFlag=1;
            }
            else
            {
               if (field->opts_off & O_NULLOK)
               {
                  printf(KRED "ERROR: This field is required\n" RST);
                  status = 0;
               }
               else
               {
                  printf("Skipping optional field\n");
               }
               break;
            }
         }
         else if (ui_form_field_validate(field, in) == 1)
         {
            break;
         }
         else
         {
            printf("Invalid Input (Press enter twice to skip)\n");
         }

      }
      
   }
   // 1 for succcess, 0 for failure or user-cancelled input.
   return do_ui_form_confirm(title, status, fields, num_fields); 
}
int ui_form(char* title, char* msg, form_fields_t *fields, int num_fields)
{
   int status;
   do
   {
      status = do_ui_form(title,msg,fields,num_fields);
   } while ( status == -1);
   return status;
}


#endif // End !USE_NCURSES
