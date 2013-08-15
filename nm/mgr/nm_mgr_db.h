/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 ** \file nm_mgr_db.h
 **
 ** File Name: nm_mgr_db.h
 **
 **
 ** Subsystem:
 **          Network Manager Daemon: Database Utilities
 **
 ** Description: This file implements the DTNMP Manager interface to a back-
 **              end SQL database.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/10/13  S. Jacobs      Initial Implementation
 *****************************************************************************/
#ifdef HAVE_MYSQL

#ifndef NM_MGR_DB_H
#define NM_MGR_DB_H


# include <stdio.h>
// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "shared/primitives/mid.h"
#include "shared/primitives/report.h"
#include "shared/primitives/rules.h"
#include "shared/primitives/admin.h"
#include "shared/adm/adm.h"
#include "shared/utils/db.h"
#include "shared/utils/utils.h"
#include "shared/msg/pdu.h"
#include "shared/primitives/report.h"
#include "shared/utils/ion_if.h"

#include <mysql.h>

//#include "nmagent.h"
//#include "ingest.h"
//#include "rda.h"

// Enumerations

#define TX_INIT  (0)
#define TX_READY (1)
#define TX_PROC  (2)
#define TX_SENT  (3)

#define RX_INIT  (0)
#define RX_READT (1)
#define RX_PROC  (2)

/* Table types for fast look up of message table types.
 * \todo: Add other table types.
 */
#define UNKNOWN_MSG (0)
#define TIME_PROD_MSG (1)
#define PRED_PROD_MSG (2)
#define EXEC_CTRL_MSG (3)
#define CUST_RPT	  (4)


// Initialization Functions
int  db_init(char *server, char *user, char *pwd, char *database);
int  db_clear();
void db_close();
void db_verify_mids();

// Functions to write primitives to the database.
// These correspond to messages sent from agent to mgr.
uint32_t db_add_agent(eid_t agent_eid);
uint32_t db_add_mid(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag,
		            char *mib, char *mib_iso, char *name, char *descr);
uint32_t db_add_mid_collection(Lyst collection, char *comment);
uint32_t db_add_rpt_data(rpt_data_t *rpt_data);
uint32_t db_add_rpt_defs(rpt_defs_t *defs);
uint32_t db_add_rpt_items(rpt_items_t *items);
uint32_t db_add_rpt_sched(rpt_prod_t *sched);

/*
 * Single dbtDataCollection table.
 * ID, Comment, LEngth, BLOB (to some max size?) VARBLOB?
 *
 * Get rid of dbtDataCollections table.
 */


// Functions to retrieve primitives from the database.
// These correspond to messages sent from mgr to agent.
ctrl_exec_t      *db_fetch_ctrl(int id);
def_gen_t        *db_fetch_def(int id);
mid_t            *db_fetch_mid(int id); /* \todo Fix this for parm OIDs */
Lyst              db_fetch_mid_col(int id);

datacol_entry_t	 *db_fetch_data_col_entry(int dc_id, int order);
Lyst              db_fetch_data_col(int dc_id);


int               db_fetch_mid_idx(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag);
rule_pred_prod_t *db_fetch_pred_rule(int id);
adm_reg_agent_t  *db_fetch_reg_agent(int id);
rule_time_prod_t *db_fetch_time_rule(int id);


// Functions to write user input to the database.
int db_ui_define_mid();

/* Add to the MIDs Collection
   table.

   The user will input MIDs
   that they want in a collection
   and give the name of the collection.
   Later the name will be determined by
   a SHA-2.
***************************/
void db_ui_add_mid_col();


// Functions to process outgoing messages provided by the DB.
int  db_outgoing_ready(MYSQL_RES **sql_res);
int  db_process_outgoing(MYSQL_RES *sql_res);
Lyst db_process_outgoing_recipients(uint32_t id);
int  db_process_outgoing_messages(uint32_t idx, pdu_group_t *msg_group);
int  db_process_outgoing_one_message(uint32_t idx, uint32_t entry_idx, pdu_group_t *msg_group, MYSQL_ROW row);

int db_get_table_type(int table_idx, int entry_idx);

// Functions to process incoming messages to provide to DB.
int db_insert_incoming_initialize(time_t timestamp);
int db_insert_incoming_message(int incomingID, uint8_t *cursor, uint32_t size);
int db_finalize_incoming(uint32_t incomingID);

#endif

#endif // HAVE_MYSQL


