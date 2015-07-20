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
 **  08/19/13  E. Birrane     Documentation clean up and code review comments.
 *****************************************************************************/
#ifdef HAVE_MYSQL

#ifndef NM_MGR_DB_H
#define NM_MGR_DB_H

/* System Headers */
#include "stdio.h"
#include "unistd.h"
#include "mysql.h"

/* ION headers. */
#include "platform.h"
#include "lyst.h"

/* Application headers. */
#include "shared/adm/adm.h"
#include "shared/msg/pdu.h"
#include "shared/primitives/admin.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/report.h"
#include "shared/primitives/rules.h"
#include "shared/utils/db.h"
#include "shared/utils/ion_if.h"
#include "shared/utils/utils.h"


/* Enumerations */

/*
 * Transmit enumerations govern the state associated with messages stored in
 * the database to be sent to an agent. Given that the database may have
 * multiple writers, these states serve as a synchronization mechanism.
 */
#define TX_INIT  (0) /* An outgoing message group is being written to the db. */
#define TX_READY (1) /* An outgoing message group is ready to be processed. */
#define TX_PROC  (2) /* The message group is being processed. */
#define TX_SENT  (3) /* The message group has been processed and sent. */

/*
 * Receive enumerations govern the state associated with messages stored in
 * the database that have been received by an agent.
 */
#define RX_INIT  (0) /* An incoming message group is being received. */
#define RX_READT (1) /* An incoming message group is done being received. */
#define RX_PROC  (2) /* An incoming message group has been processed. */


/*
 * The DB schema uses a table of tables to identify types of information
 * stored in outgoing messages.  These enumerations capture supported
 * table identifiers.
 */

#define UNKNOWN_MSG (0)
#define TIME_PROD_MSG (1)
#define PRED_PROD_MSG (2)
#define EXEC_CTRL_MSG (3)
#define CUST_RPT	  (4)



/* Functions to write primitives to associated database tables. */
uint32_t db_add_agent(eid_t agent_eid);
uint32_t db_add_mid(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag,
		            char *mib, char *mib_iso, char *name, char *descr);
uint32_t db_add_mid_collection(Lyst collection, char *comment);
uint32_t db_add_rpt_data(rpt_data_t *rpt_data);
uint32_t db_add_rpt_defs(rpt_defs_t *defs);
uint32_t db_add_rpt_items(rpt_items_t *items);
uint32_t db_add_rpt_sched(rpt_prod_t *sched);


/* Functions to fetch primitives from associated database tables. */
ctrl_exec_t      *db_fetch_ctrl(int id);
Lyst              db_fetch_data_col(int dc_id);
datacol_entry_t	 *db_fetch_data_col_entry(int dc_id, int order);
datacol_entry_t  *db_fetch_data_col_entry_from_row(MYSQL_ROW row);
def_gen_t        *db_fetch_def(int id);
mid_t            *db_fetch_mid(int id); /* \todo Fix this for parm OIDs */
Lyst              db_fetch_mid_col(int id);
int               db_fetch_mid_idx(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag);
oid_t            *db_fetch_oid(int mid_id, int oid_type, char *oid_root);
oid_t            *db_fetch_oid_full(int mid_id, char *oid_root);
oid_t            *db_fetch_oid_parms(int mid_id, char* oid_root);
uint8_t          *db_fetch_parms_str(int mid_id, uint32_t *parm_size, uint32_t *num_parms);
rule_pred_prod_t *db_fetch_pred_rule(int id);
adm_reg_agent_t  *db_fetch_reg_agent(int id);
int               db_fetch_table_type(int table_idx, int entry_idx);
rule_time_prod_t *db_fetch_time_rule(int id);

/* Functions to process incoming messages. */
int db_incoming_initialize(time_t timestamp);
int db_incoming_finalize(uint32_t incomingID);
int db_incoming_process_message(int incomingID, uint8_t *cursor, uint32_t size);

/* Database Management Functions. */
void *db_mgt_daemon(void *threadId);
int   db_mgt_init(char *server, char *user, char *pwd, char *database, int clear);
int   db_mgt_clear();
void  db_mgt_close();
void  db_mgt_verify_mids();

/* Functions to process outgoing message tables. */
int  db_outgoing_process(MYSQL_RES *sql_res);
int  db_outgoing_process_messages(uint32_t idx, pdu_group_t *msg_group, Lyst defs);
int  db_outgoing_process_one_message(uint32_t idx, uint32_t entry_idx, pdu_group_t *msg_group, MYSQL_ROW row, Lyst defs);
Lyst db_outgoing_process_recipients(uint32_t id);
int  db_outgoing_ready(MYSQL_RES **sql_res);


#endif

#endif // HAVE_MYSQL


