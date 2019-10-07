/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **  07/10/13  S. Jacobs      Initial Implementation (JHU/APL)
 **  08/19/13  E. Birrane     Documentation clean up and code review comments. (JHU/APL)
 **  08/22/15  E. Birrane     Updates for new schema and dynamic user permissions. (Secure DTN - NASA: NNX14CS58P)
 **  10/20/18  E. Birrane     Updates for AMPv0.5 (JHU/APL)
 *****************************************************************************/
#ifdef HAVE_MYSQL

#ifndef NM_MGR_DB_H
#define NM_MGR_DB_H

/* System Headers */
#include "stdio.h"
#include "unistd.h"
#include <mysql.h>

/* ION headers. */
#include "platform.h"

/* Application headers. */
#include "../shared/adm/adm.h"
#include "../shared/msg/ion_if.h"
#include "../shared/msg/msg.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/rules.h"
#include "../shared/primitives/ctrl.h"

#include "../shared/utils/db.h"
#include "../shared/utils/utils.h"
#include "../shared/utils/vector.h"

#include "nm_mgr_ui.h"
#include "agents.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

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
#define RX_READY (1) /* An incoming message group is done being received. */
#define RX_PROC  (2) /* An incoming message group has been processed. */


/*
 * The DB schema uses a table of tables to identify types of information
 * stored in outgoing messages.  These enumerations capture supported
 * table identifiers.
 */

#define UNKNOWN_MSG (0)
#define EXEC_CTRL_MSG (3)

/*
 * Constants relating to how long to try and reconnect to the DB when
 * a connection has failed.
 */

#define SQL_RECONN_TIME_MSEC 500
#define SQL_CONN_TRIES 10
#define SQL_MAX_QUERY 8192

#define UI_SQL_SERVERLEN (80)
#define UI_SQL_ACCTLEN   (20)
#define UI_SQL_DBLEN     (20)
#define UI_SQL_TOTLEN    (UI_SQL_SERVERLEN + UI_SQL_ACCTLEN + UI_SQL_ACCTLEN + UI_SQL_DBLEN)

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


#define CHKCONN db_mgt_connected();

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * This structure holds the constants needed to store SQL database
 * information.
 */
typedef struct
{
    ResourceLock lock;

	char server[UI_SQL_SERVERLEN];
	char username[UI_SQL_ACCTLEN];
	char password[UI_SQL_ACCTLEN];
	char database[UI_SQL_DBLEN];

	db_desc_t desc;
} sql_db_t;

/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

/* Functions to write primitives to associated database tables. */
int32_t db_add_agent(eid_t agent_eid);

/* Database Management Functions. */
void    *db_mgt_daemon(int *running);
uint32_t db_mgt_init(sql_db_t parms, uint32_t clear, uint32_t log);
int      db_mgt_clear();
int      db_mgt_clear_table(char *table);
void     db_mgt_close();
int      db_mgt_connected();
int32_t  db_mgt_query_fetch(MYSQL_RES **res, char *format, ...);
int32_t  db_mgt_query_insert(uint32_t *idx, char *format, ...);
void     db_mgt_txn_start();
void     db_mgt_txn_commit();
void     db_mgt_txn_rollback();

int      db_mgr_sql_persist();
void     db_mgr_sql_info_deserialize(blob_t *data);
blob_t*  db_mgr_sql_info_serialize();
int      db_mgr_sql_init();



/* Functions to process outgoing message tables. */
int32_t  db_tx_msg_groups(MYSQL_RES *sql_res);
int32_t  db_tx_build_group(int32_t grp_idx, msg_grp_t *msg_group);
int      db_tx_collect_agents(int32_t grp_idx, vector_t *vec);
int      db_outgoing_ready(MYSQL_RES **sql_res);


/* Functions to process incoming messages. */
int32_t db_incoming_initialize(time_t timestamp, eid_t sender_eid);
int32_t db_incoming_finalize(uint32_t incomingID);
int32_t db_incoming_process_message(int32_t incomingID, blob_t *data);

agent_t* db_fetch_agent(int32_t id);
int32_t  db_fetch_agent_idx(eid_t *sender);
ac_t*    db_fetch_ari_col(int idx);

#if 0
/* Functions to write primitives to associated database tables. */
int32_t db_add_adm(char *name, char *version, char *oid_root);
int32_t db_add_agent(eid_t agent_eid);
int32_t db_add_tdc(tdc_t tdc);
int32_t db_add_mid(mid_t *mid);
int32_t db_add_mc(Lyst mc);
int32_t db_add_nn(oid_nn_t *nn);
int32_t db_add_oid(oid_t oid);
int32_t db_add_oid_str(char *oid_str);
int32_t db_add_parms(oid_t oid);
int32_t db_add_protomid(mid_t *mid, ui_parm_spec_t *spec, amp_type_e type);
int32_t db_add_protoparms(ui_parm_spec_t *spec);


/* Functions to fetch primitives from associated database tables. */

int32_t           db_fetch_adm_idx(char *name, char *version);
tdc_t             db_fetch_tdc(int32_t tdc_idx);
blob_t*           db_fetch_tdc_entry_from_row(MYSQL_ROW row, amp_type_e *type);
mid_t*            db_fetch_mid(int32_t idx);
Lyst              db_fetch_mid_col(int idx);
mid_t*            db_fetch_mid_from_row(MYSQL_ROW row);
int32_t           db_fetch_mid_idx(mid_t *mid);
int32_t           db_fetch_nn(uint32_t idx);
int32_t           db_fetch_nn_idx(uint32_t nn);
uint8_t*          db_fetch_oid_val(uint32_t idx, uint32_t *size);
oid_t             db_fetch_oid(uint32_t nn_idx, uint32_t parm_idx, uint32_t oid_idx);
int32_t           db_fetch_oid_idx(oid_t oid);
Lyst			  db_fetch_parms(uint32_t idx);
int32_t           db_fetch_protomid_idx(mid_t *mid);




#endif

#endif

#endif // HAVE_MYSQL


