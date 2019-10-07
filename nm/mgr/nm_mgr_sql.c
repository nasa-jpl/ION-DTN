/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 ** \file nm_mgr_sql.c
 **
 ** File Name: nm_mgr_sql.c
 **
 **
 ** Subsystem:
 **          Network Manager Daemon: Database Utilities
 **
 ** Description: This file implements a SQL interface to the ION AMP manager.
 **
 ** Notes:
 ** 	This software assumes that there are no other applications modifying
 ** 	the AMP database tables.
 **
 ** 	These functions do not, generally, rollback DB writes on error.
 ** 	\todo: Add transactions.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/10/13  S. Jacobs      Initial Implementation (JHU/APL)
 **  08/19/13  E. Birrane     Documentation clean up and code review comments. (JHU/APL)
 **  08/22/15  E. Birrane     Updates for new schema and dynamic user permissions. (Secure DTN - NASA: NNX14CS58P)
 **  01/24/17  E. Birrane     Updates to latest AMP IOS 3.5.0 (JHU/APL)
 **  10/20/18  E. Birrane     Updates for AMPv0.5 (JHU/APL)
 *****************************************************************************/

#ifdef HAVE_MYSQL

#include <string.h>

#include "ion.h"

#include "nm_mgr.h"
#include "nm_mgr_sql.h"


/* Global connection to the MYSQL Server. */
static MYSQL *gConn;
static sql_db_t gParms;
static uint8_t gInTxn;


/******************************************************************************
 *
 * \par Function Name: db_add_agent()
 *
 * \par Adds a Registered Agent to the dbtRegisteredAgents table.
 *
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in]  agent_eid  - The Agent EID being added to the DB.
 *
 * \par Notes:
 *		- Only the agent EID is kept in the database, and used as a recipient
 *		  ID.  No other agent information is persisted at this time.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  08/22/15  E. Birrane     Updated to new database schema.
 *  01/24/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *  10/20/18  E. Birrane     Update to AMPv0.5 (JHU/APL)
 *****************************************************************************/
int32_t db_add_agent(eid_t agent_eid)
{
	uint32_t row_idx = 0;

	AMP_DEBUG_WARN("db_add_agent","Not implemented.", NULL);

	return 0;
}


/******************************************************************************
 *
 * \par Function Name: db_incoming_initialize
 *
 * \par Returns the id of the last insert into dbtIncoming.
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] timestamp  - the generated timestamp
 * \param[in] sender_eid - Who sent the messages.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *  08/29/15  E. Birrane     Added sender EID.
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_incoming_initialize(time_t timestamp, eid_t sender_eid)
{
	MYSQL_RES *res = NULL;
    MYSQL_ROW row;
	char timebuf[256];
	uint32_t result = 0;
	uint32_t agent_idx = 0;

	AMP_DEBUG_ENTRY("db_incoming_initialize","("ADDR_FIELDSPEC",%s)",
			        (uaddr)timestamp, sender_eid.name);

	db_mgt_txn_start();

	/* Step 1: Find the agent ID, or try to add it. */
	if((agent_idx = db_add_agent(sender_eid)) <= 0)
	{
		AMP_DEBUG_ERR("db_incoming_initialize","Can't find agent id.", NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", agent_idx);
		return agent_idx;
	}

	/* Step 2: Create a SQL time */
	struct tm tminfo;
	localtime_r(&timestamp, &tminfo);

	isprintf(timebuf, 256, "%d-%d-%d %d:%d:%d",
			tminfo.tm_year+1900,
			tminfo.tm_mon,
			tminfo.tm_mday,
			tminfo.tm_hour,
			tminfo.tm_min,
			tminfo.tm_sec);

	/* Step 2: Insert the TS. */
	if(db_mgt_query_insert(&result,
			              "INSERT INTO dbtIncomingMessageGroup"
			              "(ReceivedTS,GeneratedTS,State,AgentID) "
					      "VALUES(NOW(),'%s',0,%d)",
						  timebuf, agent_idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_incoming_initialize","Can't insert Timestamp", NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_incoming_initialize","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_incoming_initialize","-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_incoming_finalize
 *
 * \par Finalize processing of the incoming messages.
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] id - The incoming message group ID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_incoming_finalize(uint32_t id)
{

	db_mgt_txn_start();

	/* Step 2: Insert the TS. */
	if(db_mgt_query_insert(NULL,
			               "UPDATE dbtIncomingMessageGroup SET State = State + 1 WHERE ID = %d",
						   id) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_incoming_finalize","Can't insert Timestamp", NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_incoming_finalize","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_incoming_finalize","-->%d", AMP_OK);
	return AMP_OK;
}



/******************************************************************************
 *
 * \par Function Name: db_incoming_process_message
 *
 * \par Returns number of incoming message groups.
 *
 * \return # groups. -1 on error.
 *
 * \param[in] id     - The ID for the incoming message.
 * \param[in] cursor - Cursor pointing to start of message.
 * \param[in] size   - The size of the incoming message.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *  01/26/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/
int32_t db_incoming_process_message(int32_t id, blob_t *data)
{
	char *query = NULL;
	char *result_data = NULL;
	int32_t result_size = 0;

	AMP_DEBUG_ENTRY("db_incoming_process_message","(%d,"ADDR_FIELDSPEC")",
			        id, (uaddr)data);

	/* Step 0: Sanity Check. */
	if(data == NULL)
	{
		AMP_DEBUG_ERR("db_incoming_process_message","Bad args.",NULL);
		AMP_DEBUG_EXIT("db_incoming_process_message", "-->-1", NULL);
		return -1;
	}

	/* Step 1: Convert the incoming message to a string for processing.*/
	if((result_data = utils_hex_to_string(data->value, data->length)) == NULL)
	{
		AMP_DEBUG_ERR("db_incoming_process_message","Can't cvt %d bytes to hex str.",
				        data->length);
		AMP_DEBUG_EXIT("db_incoming_process_message", "-->-1", NULL);
		return -1;
	}

	result_size = strlen(result_data);

	db_mgt_txn_start();

	/*
	 * Step 2: Allocate a query for inserting into the DB. We allocate our own
	 *         because this could be large.
	 */
	if((query = (char *) STAKE(result_size + 256)) == NULL)
	{
		AMP_DEBUG_ERR("db_incoming_process_message","Can't alloc %d bytes.",
				        result_size + 256);
		SRELEASE(result_data);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		return 0;
	}

	/* Step 3: Convert the query using allocated query structure. */
	snprintf(query,result_size + 255,"INSERT INTO dbtIncomingMessages(IncomingID,Content)"
			   "VALUES(%d,'%s')",id, result_data+2);
	SRELEASE(result_data);

	/* Step 4: Run the query. */
	if(db_mgt_query_insert(NULL, query) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_incoming_process_message","Can't insert Timestamp", NULL);
		SRELEASE(query);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_incoming_process_message","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	SRELEASE(query);
	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_incoming_process_message", "-->1", NULL);
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: db_mgt_daemon
 *
 * \par Returns number of outgoing message groups ready to be sent.
 *
 * \return  .
 *
 * \param[in] threadId - The POSIX thread.
 *
 * \par Notes:
 *  - We are being very inefficient here, as we grab the full result and
 *    then ignore it, presumably to query it again later. We should
 *    optimize this.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  S. Jacobs      Initial implementation,
 *  08/29/15  E. Birrane     Only query DB if we have an active connection.
 *  04/24/16  E. Birrane     Accept global "running" flag.
 *  01/26/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

void *db_mgt_daemon(int *running)
{
	MYSQL_RES *sql_res;
	struct timeval start_time;
	vast delta = 0;


	AMP_DEBUG_ALWAYS("db_mgt_daemon","Starting Manager Database Daemon",NULL);

	while (*running)
	{
    	getCurrentTime(&start_time);

    	if(db_mgt_connected() == 0)
    	{
    		if (db_outgoing_ready(&sql_res))
    		{
    			db_tx_msg_groups(sql_res);
    		}

			mysql_free_result(sql_res);
			sql_res = NULL;
    	}

        delta = utils_time_cur_delta(&start_time);

        // Sleep for 1 second (10^6 microsec) subtracting the processing time.
        if((delta < 2000000) && (delta > 0))
        {
        	microsnooze((unsigned int)(2000000 - delta));
        }
	}

	AMP_DEBUG_ALWAYS("db_mgt_daemon","Cleaning up Manager Database Daemon", NULL);

	db_mgt_close();

	AMP_DEBUG_ALWAYS("db_mgt_daemon","Manager Database Daemon Finished.",NULL);
	pthread_exit(NULL);
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_init
 *
 * \par Initializes the gConnection to the database.
 *
 * \retval 0 Failure
 *        !0 Success
 *
 * \param[in]  server - The machine hosting the SQL database.
 * \param[in]  user - The username for the SQL database.
 * \param[in]  pwd - The password for this user.
 * \param[in]  database - The database housing the DTNMP tables.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  01/26/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/
uint32_t db_mgt_init(sql_db_t parms, uint32_t clear, uint32_t log)
{

	AMP_DEBUG_ENTRY("db_mgt_init","(parms, %d)", clear);

	if(gConn == NULL)
	{
		gConn = mysql_init(NULL);
		gParms = parms;
		gInTxn = 0;

		AMP_DEBUG_ENTRY("db_mgt_init", "(%s,%s,%s,%s)", parms.server, parms.username, parms.password, parms.database);

		if (!mysql_real_connect(gConn, parms.server, parms.username, parms.password, parms.database, 0, NULL, 0))
		{
			if(log > 0)
            {
				AMP_DEBUG_WARN("db_mgt_init", "SQL Error: %s", mysql_error(gConn));
            }
			AMP_DEBUG_EXIT("db_mgt_init", "-->0", NULL);
			return 0;
		}

		AMP_DEBUG_INFO("db_mgt_init", "Connected to Database.", NULL);
	}

	if(clear != 0)
	{
		db_mgt_clear();
	}


	AMP_DEBUG_EXIT("db_mgt_init", "-->1", NULL);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_clear
 *
 * \par Clears all of the database tables used by the DTNMP Management Daemon.
 *
 * \retval 0 Failure
 *        !0 Success
 *
 *
 * \todo Add support to clear all tables. Maybe add a parm to select a
 *        table to clear (perhaps a string?)
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  08/27/15  E. Birrane     Updated to latest schema
 *****************************************************************************/


int db_mgt_clear()
{

	AMP_DEBUG_ENTRY("db_mgt_clear", "()", NULL);

	if( db_mgt_clear_table("dbtMIDs") ||
		db_mgt_clear_table("dbtIncomingMessages") ||
		db_mgt_clear_table("dbtOIDs") ||
		db_mgt_clear_table("dbtADMs") ||
		db_mgt_clear_table("dbtADMNicknames") ||
		db_mgt_clear_table("dbtIncomingMessageGroup") ||
		db_mgt_clear_table("dbtOutgoingMessageGroup") ||
		db_mgt_clear_table("dbtRegisteredAgents") ||
		db_mgt_clear_table("dbtDataCollections") ||
		db_mgt_clear_table("dbtDataCollection") ||
		db_mgt_clear_table("dbtMIDCollections") ||
		db_mgt_clear_table("dbtMIDCollection") ||
		db_mgt_clear_table("dbtMIDParameters") ||
		db_mgt_clear_table("dbtMIDParameter"))
	{
		AMP_DEBUG_ERR("db_mgt_clear", "SQL Error: %s", mysql_error(gConn));
		AMP_DEBUG_EXIT("db_mgt_clear", "--> 0", NULL);
		return 0;
	}

	AMP_DEBUG_EXIT("db_mgt_clear", "--> 1", NULL);
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: db_mgt_clear_table
 *
 * \par Clears a database table used by the DTNMP Management Daemon.
 *
 * Note:
 *   We don't use truncate here because of foreign key constraints. Delete
 *   is able to remove items from a table, but does not reseed the
 *   auto-incrementing for the table, so an alter table command is also
 *   used.
 *
 * \retval !0 Failure
 *          0 Success
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation,
 *****************************************************************************/

int db_mgt_clear_table(char *table)
{
	if(table == NULL)
	{
		return 1;
	}

	if (db_mgt_query_insert(NULL,"SET FOREIGN_KEY_CHECKS=0",NULL) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_mgt_clear_table", "SQL Error: %s", mysql_error(gConn));
		AMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
		return 1;
	}

	if (db_mgt_query_insert(NULL,"TRUNCATE %s", table) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_mgt_clear_table", "SQL Error: %s", mysql_error(gConn));
		AMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
		return 1;
	}

	if (db_mgt_query_insert(NULL,"SET FOREIGN_KEY_CHECKS=1", NULL) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_mgt_clear_table", "SQL Error: %s", mysql_error(gConn));
		AMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
		return 1;
	}

	AMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
	return 0;
}


/******************************************************************************
 *
 * \par Function Name: db_mgt_close
 *
 * \par Close the database gConnection.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

void db_mgt_close()
{
	AMP_DEBUG_ENTRY("db_mgt_close","()",NULL);
	if(gConn != NULL)
	{
		mysql_close(gConn);
		mysql_library_end();
		gConn = NULL;
	}
	AMP_DEBUG_EXIT("db_mgt_close","-->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_connected
 *
 * \par Checks to see if the database connection is still active and, if not,
 *      try to reconnect up to some configured number of times.
 *
 * \par Notes:
 *
 * \retval !0 Error
 *          0 Success
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/27/15  E. Birrane     Updated to try and reconnect to DB.
 *****************************************************************************/

int   db_mgt_connected()
{
	int result = -1;
	uint8_t num_tries = 0;

	if(gConn == NULL)
	{
		return -1;
	}

	result = mysql_ping(gConn);
	if(result != 0)
	{
		while(num_tries < SQL_CONN_TRIES)
		{
			db_mgt_init(gParms, 0, 0);
			if((result = mysql_ping(gConn)) == 0)
			{
				return 0;
			}

			microsnooze(SQL_RECONN_TIME_MSEC);
			num_tries++;
		}
	}

	return result;
}


int  db_mgr_sql_persist()
{
	int success = AMP_OK;
	Sdr sdr = getIonsdr();

	if(gMgrDB.sql_info.desc.descObj == 0)
	{
		gMgrDB.sql_info.desc.descObj = sdr_malloc(sdr, sizeof(gMgrDB.sql_info.desc));
	}

	blob_t *data = db_mgr_sql_info_serialize(&(gMgrDB.sql_info));

	CHKERR(sdr_begin_xn(sdr));

	if(gMgrDB.sql_info.desc.itemObj != 0)
	{
		sdr_free(sdr, gMgrDB.sql_info.desc.itemObj);
	}

	gMgrDB.sql_info.desc.itemObj = sdr_malloc(sdr, data->length);
	gMgrDB.sql_info.desc.itemSize = data->length;

	sdr_write(sdr, gMgrDB.sql_info.desc.itemObj, (char *) data->value, data->length);
	sdr_write(sdr, gMgrDB.sql_info.desc.descObj, (char *) &(gMgrDB.sql_info.desc), sizeof(gMgrDB.sql_info.desc));

	sdr_end_xn(sdr);

	blob_release(data, 1);
	return success;
}


void db_mgr_sql_info_deserialize(blob_t *data)
{
	QCBORError err;
	QCBORItem item;
	QCBORDecodeContext it;
	size_t length;

	QCBORDecode_Init(&it,
					 (UsefulBuf){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	err = QCBORDecode_GetNext(it, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
		AMP_DEBUG_ERR("mgr_sql_info_deserialize","Not a container. Error is %d Type %d", err, item.uDataType);
		return NULL;
	}
	else if (item.val.uCount != 4)
	{
		AMP_DEBUG_ERR("mgr_sql_info_deserialize","Bad length. %d not 4", item.val.uCount);
		return;
	}

	cut_get_cbor_str_ptr(&it, gMgrDB.sql_info.server, UI_SQL_SERVERLEN);
	cut_get_cbor_str_ptr(&it, gMgrDB.sql_info.username, UI_SQL_ACCTLEN);
	cut_get_cbor_str_ptr(&it, gMgrDB.sql_info.password, UI_SQL_ACCTLEN);
	cut_get_cbor_str_ptr(&it, gMgrDB.sql_info.database, UI_SQL_DBLEN);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return;
}

blob_t*	  db_mgr_sql_info_serialize(sql_db_t *item)
{
	QCBOREncodeContext encoder;
	QCBORItem item;

	blob_t *result = blob_create(NULL, 0, 2 * sizeof(sql_db_t));

	QCBOREncode_Init(&encoder, (UsefulBuf){result->value, result->alloc});
	QCBOREncode_OpenArray(&encoder);

	QCBOREncode_AddSZString(&encoder, item->server);
	QCBOREncode_AddSZString(&encoder, item->username);
	QCBOREncode_AddSZString(&encoder, item->password);
	QCBOREncode_AddSZString(&encoder, item->database);

	QCBOREncode_CloseArray(&encoder);

	UsefulBufC Encoded;
	if(QCBOREncode_Finish(&EC, &Encoded)) {
		AMP_DEBUG_ERR("db_mgr_sql_info_serialize", "Encoding failed", NULL);
		blob_release(result,1);
		return NULL;
	}
	
	result->length = Encoded.len;
	return result;
}


int  db_mgr_sql_init()
{

	Sdr sdr = getIonsdr();
	char *name = "mgr_sql";

	// * Initialize the non-volatile database. * /
	memset((char*) &(gMgrDB.sql_info), 0, sizeof(gMgrDB.sql_info));

	initResourceLock(&(gMgrDB.sql_info.lock));

	/* Recover the Agent database, creating it if necessary. */
	CHKERR(sdr_begin_xn(sdr));

	gMgrDB.sql_info.desc.descObj = sdr_find(sdr, name, NULL);
	switch(gMgrDB.sql_info.desc.descObj)
	{
		case -1:  // SDR error. * /
			sdr_cancel_xn(sdr);
			AMP_DEBUG_ERR("db_mgr_sql_init", "Can't search for DB in SDR.", NULL);
			return -1;

		case 0: // Not found; Must create new DB. * /

			if((gMgrDB.sql_info.desc.descObj = sdr_malloc(sdr, sizeof(gMgrDB.sql_info.desc))) == 0)
			{
				sdr_cancel_xn(sdr);
				AMP_DEBUG_ERR("db_mgr_sql_init", "No space for database.", NULL);
				return -1;
			}
			AMP_DEBUG_ALWAYS("db_mgr_sql_init", "Creating DB: %s", name);

			sdr_write(sdr, gMgrDB.sql_info.desc.descObj, (char *) &(gMgrDB.sql_info.desc), sizeof(gMgrDB.sql_info.desc));
			sdr_catlg(sdr, name, 0, gMgrDB.sql_info.desc.descObj);

			break;

		default:  /* Found DB in the SDR */
			/* Read in the Database. */
			sdr_read(sdr, (char *) &(gMgrDB.sql_info.desc), gMgrDB.sql_info.desc.descObj, sizeof(gMgrDB.sql_info.desc));
			AMP_DEBUG_ALWAYS("db_mgr_sql_init", "Found DB", NULL);

			if(gMgrDB.sql_info.desc.itemSize > 0)
			{
				blob_t *data = blob_create(NULL, 0, gMgrDB.sql_info.desc.itemSize);
				if(data != NULL)
				{
					sdr_read(sdr, (char *) data->value, gMgrDB.sql_info.desc.itemObj, gMgrDB.sql_info.desc.itemSize);
					data->length = gMgrDB.sql_info.desc.itemSize;
					db_mgr_sql_info_deserialize(data);
					blob_release(data, 1);
				}
			}


	}

	if(sdr_end_xn(sdr))
	{
		AMP_DEBUG_ERR("db_mgr_sql_init", "Can't create Agent database.", NULL);
		return -1;
	}

	return 1;
}




/******************************************************************************
 *
 * \par Function Name: db_mgt_query_fetch
 *
 * \par Runs a fetch in the database given a query and returns the result, if
 *      a result field is provided..
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[out] res    - The result.
 * \param[in]  format - Format to build query
 * \param[in]  ...    - Var args to build query given format string.
 *
 * \par Notes:
 *   - The res structure should be a pointer but without being allocated. This
 *     function will create the storage.
 *   - If res is NULL that's ok, but no result will be returned.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/26/17  E. Birrane     Initial implementation (JHU/APL).
 *****************************************************************************/

int32_t db_mgt_query_fetch(MYSQL_RES **res, char *format, ...)
{
	char query[1024];

	AMP_DEBUG_ENTRY("db_mgt_query_fetch","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
			        (uaddr)res, (uaddr)format);

	/* Step 0: Sanity check. */
	if(format == NULL)
	{
		AMP_DEBUG_ERR("db_mgt_query_fetch", "Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/*
	 * Step 1: Assert the DB connection. This should not only check
	 *         the connection as well as try and re-establish it.
	 */
	if(db_mgt_connected() == 0)
	{
		va_list args;

		va_start(args, format); // format is last parameter before "..."
		vsnprintf(query, 1024, format, args);
		va_end(args);

		if (mysql_query(gConn, query))
		{
			AMP_DEBUG_ERR("db_mgt_query_fetch", "Database Error: %s",
					mysql_error(gConn));
			AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_FAIL);
			return AMP_FAIL;
		}

		if((*res = mysql_store_result(gConn)) == NULL)
		{
			AMP_DEBUG_ERR("db_mgt_query_fetch", "Can't get result.", NULL);
			AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_FAIL);
			return AMP_FAIL;
		}
	}
	else
	{
		AMP_DEBUG_ERR("db_mgt_query_fetch", "DB not connected.", NULL);
		AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_SYSERR);
		return AMP_SYSERR;
	}

	AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_OK);
	return AMP_OK;
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_query_insert
 *
 * \par Runs an insert in the database given a query and returns the
 *      index of the inserted item.
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[out] idx    - The index of the inserted row.
 * \param[in]  format - Format to build query
 * \param[in]  ...    - Var args to build query given format string.
 *
 * \par Notes:
 *   - The idx may be NULL if the insert index is not needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/26/17  E. Birrane     Initial implementation (JHU/APL).
 *****************************************************************************/

int32_t db_mgt_query_insert(uint32_t *idx, char *format, ...)
{
	char query[SQL_MAX_QUERY];

	AMP_DEBUG_ENTRY("db_mgt_query_insert","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",(uaddr)idx, (uaddr)format);
/*EJB
	if(idx == NULL)
	{
		AMP_DEBUG_ERR("db_mgt_query_insert", "Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_mgt_query_insert", "-->%d", AMP_FAIL);
		return AMP_FAIL;
	}
*/
	if(db_mgt_connected() == 0)
	{
		va_list args;

		va_start(args, format); // format is last parameter before "..."
		if(vsnprintf(query, SQL_MAX_QUERY, format, args) == SQL_MAX_QUERY)
		{
			AMP_DEBUG_ERR("db_mgt_query_insert", "query is too long. Maximum length is %d", SQL_MAX_QUERY);
		}
		va_end(args);

		if (mysql_query(gConn, query))
		{
			AMP_DEBUG_ERR("db_mgt_query_insert", "Database Error: %s",
					mysql_error(gConn));
			AMP_DEBUG_EXIT("db_mgt_query_insert", "-->%d", AMP_FAIL);
			return AMP_FAIL;
		}

		if(idx != NULL)
		{
			if((*idx = (uint32_t) mysql_insert_id(gConn)) == 0)
			{
				AMP_DEBUG_ERR("db_mgt_query_insert", "Unknown last inserted row.", NULL);
				AMP_DEBUG_EXIT("db_mgt_query_insert", "-->%d", AMP_FAIL);
				return AMP_FAIL;
			}
		}
	}
	else
	{
		AMP_DEBUG_ERR("db_mgt_query_fetch", "DB not connected.", NULL);
		AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_SYSERR);
		return AMP_SYSERR;
	}

	AMP_DEBUG_EXIT("db_mgt_query_fetch", "-->%d", AMP_OK);
	return AMP_OK;
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_txn_start
 *
 * \par Starts a transaction in the database, if we are not already in a txn.
 *
 * \par Notes:
 *   - This function is not multi-threaded. We assume that we are the only
 *     input into the database and that there is only one "active" transaction
 *     at a time.
 *   - This function does not support nested transactions.
 *   - If a transaction is already open, this function assumes that is the
 *     transaction to use.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/26/17  E. Birrane     Initial implementation (JHU/APL).
 *****************************************************************************/

void db_mgt_txn_start()
{
	if(gInTxn == 0)
	{
		if(db_mgt_query_insert(NULL,"START TRANSACTION",NULL) == AMP_OK)
		{
			gInTxn = 1;
		}
	}
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_txn_commit
 *
 * \par Commits a transaction in the database, if we are in a txn.
 *
 * \par Notes:
 *   - This function is not multi-threaded. We assume that we are the only
 *     input into the database and that there is only one "active" transaction
 *     at a time.
 *   - This function does not support nested transactions.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/26/17  E. Birrane     Initial implementation (JHU/APL).
 *****************************************************************************/

void db_mgt_txn_commit()
{
	if(gInTxn == 1)
	{
		if(db_mgt_query_insert(NULL,"COMMIT",NULL) == AMP_OK)
		{
			gInTxn = 0;
		}
	}
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_txn_rollback
 *
 * \par Rolls back a transaction in the database, if we are in a txn.
 *
 * \par Notes:
 *   - This function is not multi-threaded. We assume that we are the only
 *     input into the database and that there is only one "active" transaction
 *     at a time.
 *   - This function does not support nested transactions.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/26/17  E. Birrane     Initial implementation (JHU/APL).
 *****************************************************************************/

void db_mgt_txn_rollback()
{
	if(gInTxn == 1)
	{
		if(db_mgt_query_insert(NULL,"ROLLBACK",NULL) == AMP_OK)
		{
			gInTxn = 0;
		}
	}
}



/******************************************************************************
 *
 * \par Function Name: db_tx_msg_groups
 *
 * \par Returns 1 if the message is ready to be sent
 *
 * \retval AMP_SYSERR on system error
 *         AMP_FAIL   if no message groups ready.
 *         AMP_OK     If there are message groups ready to be sent.
 *
 * \param[out] sql_res - The outgoing messages.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *  07/18/13  S. Jacobs       Added outgoing agents
 *  09/27/13  E. Birrane      Configure each agent with custom rpt, if applicable.
 *  08/27/15  E. Birrane      Update to new data model, schema
 *  01/26/17  E. Birrane      Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_tx_msg_groups(MYSQL_RES *sql_res)
{
	MYSQL_ROW row;
	msg_grp_t *msg_group = NULL;
	int32_t idx = 0;
	int32_t agent_idx = 0;
	int32_t result = AMP_SYSERR;
	agent_t *agent = NULL;

	AMP_DEBUG_ENTRY("db_tx_msg_groups","("ADDR_FIELDSPEC")",(uaddr) sql_res);

	/* Step 0: Sanity Check. */
	if(sql_res == NULL)
	{
		AMP_DEBUG_ERR("db_tx_msg_groups","Bad args.", NULL);
		AMP_DEBUG_EXIT("db_tx_msg_groups","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: For each message group that is ready to go... */
	while ((row = mysql_fetch_row(sql_res)) != NULL)
	{
		/* Step 1.1 Create and populate the message group. */
		idx = atoi(row[0]);
		agent_idx = atoi(row[4]);

		/* Step 1.2: Create an AMP PDU for this outgoing message. */
		if((msg_group = msg_grp_create(1)) == NULL)
		{
			AMP_DEBUG_ERR("db_tx_msg_groups","Cannot create group.", NULL);
			AMP_DEBUG_EXIT("db_tx_msg_groups","-->%d",AMP_SYSERR);
			return AMP_SYSERR;
		}

		/*
		 * Step 1.3: Populate the message group with outgoing messages.
		 *           If there are no message groups, Quietly go home,
		 *           it isn't an error, it's just disappointing.
		 */
		if((result = db_tx_build_group(idx, msg_group)) != AMP_OK)
		{
			msg_grp_release(msg_group, 1);
			AMP_DEBUG_EXIT("db_tx_msg_groups","-->%d",result);
			return result;
		}

		/* Step 1.4: Figure out the agent receiving this message. */
		if((agent = db_fetch_agent(agent_idx)) == NULL)
		{
			AMP_DEBUG_ERR("db_tx_msg_groups","Can't get agent for idx %d", agent_idx);
			msg_grp_release(msg_group, 1);
			AMP_DEBUG_EXIT("db_tx_msg_groups","-->%d",AMP_FAIL);
			return AMP_FAIL;
		}

		/*
		 * Step 1.5: The database knows about the agent but the management
		 *           daemon might not. Make sure that the management daemon
		 *           knows about this agent so that it isn't a surprise when
		 *           the agent starts sending data back.
		 *
		 *           If we can't add the agent to the manager daemon (which
		 *           would be very odd) we send the message group along and
		 *           accept that there might be confusion when the agent
		 *           sends information back.
		 */

		if(agent_get(&(agent->eid)) == NULL)
		{
			if(agent_add(agent->eid) == -1)
			{
				AMP_DEBUG_WARN("db_tx_msg_groups","Sending to unknown agent.", NULL);
			}
		}

		/* Step 1.6: Send the message group.*/
		AMP_DEBUG_INFO("db_tx_msg_groups",
				       "Sending to name %s",
					   agent->eid.name);

		iif_send_grp(&ion_ptr, msg_group, agent->eid.name);

		/* Step 1.7: Release resources. */
		agent_release(agent, 1);
		msg_grp_release(msg_group, 1);
		msg_group = NULL;

		/*
		 * Step 1.8: Update the state of the message group in the database.
		 *           \todo: Consider aborting message group if this happens.
		 */
		if(db_mgt_query_insert(NULL,
				               "UPDATE dbtOutgoingMessageGroup SET State=2 WHERE ID=%d",
				               idx)!= AMP_OK)
		{
			AMP_DEBUG_WARN("db_tx_msg_groups","Could not update DB send status.", NULL);
		}
	}

	AMP_DEBUG_EXIT("db_tx_msg_groups", "-->%d", AMP_OK);

	return AMP_OK;
}


/******************************************************************************
 *
 * \par Function Name: db_tx_build_group
 *
 * \par This function populates an AMP message group with messages
 *      for this group from the database.
 *
 * \retval AMP_SYSERR on system error
 *         AMP_FAIL   if no message groups ready.
 *         AMP_OK     If there are message groups ready to be sent.
 *
 * \param[in]  grp_idx   - The DB identifier of the message group
 * \param[out] msg_group - The message group being populated
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *  09/27/13  E. Birrane      Collect any rpt defs from this message.
 *  08/27/15  E. Birrane      Update to latest data model and schema.
 *  01/26/17  E. Birrane      Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_tx_build_group(int32_t grp_idx, msg_grp_t *msg_group)
{
	int32_t result = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_tx_build_group",
					  "(%d, "ADDR_FIELDSPEC")",
			          grp_idx, (uaddr) msg_group);

	/* Step 0: Sanity check. */
	if(msg_group == NULL)
	{
		AMP_DEBUG_ERR("db_tx_build_group","Bad args.", NULL);
		AMP_DEBUG_EXIT("db_tx_build_group","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: Find all messages for this outgoing group. */
	if(db_mgt_query_fetch(&res,
			              "SELECT MidCollID FROM dbtOutgoingMessages WHERE OutgoingID=%d",
			              grp_idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_tx_build_group",
					  "Can't find messages for %d", grp_idx);
		AMP_DEBUG_EXIT("db_tx_build_group","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: For each message that belongs in this group....*/
    while((res != NULL) && ((row = mysql_fetch_row(res)) != NULL))
    {
    	int32_t ac_idx = atoi(row[0]);
    	ac_t *ac;

    	/*
    	 * Step 2.1: An outgoing message in AMP is a "run controls"
    	 *           message, which accepts a series of controls to
    	 *           run. This series is stored as a
    	 *           MID Collection (MC). So, grab the MC.
    	 */
		if((ac = db_fetch_ari_col(ac_idx)) == NULL)
		{
			AMP_DEBUG_ERR("db_tx_build_group",
						    "Can't grab AC for idx %d", ac_idx);
			result = AMP_FAIL;
			break;
		}

		/*
		 * Step 2.2: Create the "run controls" message, passing in
		 *           the MC of controls to run.
		 *           \todo: SQL currently has no place to store a
		 *                  time offset associated with a control. We
		 *                  currently jam that to 0 (which means run
		 *                  immediately).
		 */
		msg_ctrl_t *ctrl = msg_ctrl_create();
		ctrl->ac = ac;

		result = msg_grp_add_msg_ctrl(msg_group, ctrl);
	}

	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_tx_build_group","-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_tx_collect_agents
 *
 * \par Returns a vector of the agents to send a message to
 *
 * \retval NULL no recipients.
 *        !NULL There are recipients to be sent to.
 *
 * \param[in] grp_idx - The index of the message group being sent.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/18/13  S. Jacobs       Initial Implementation
 *  01/26/17  E. Birrane      Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int db_tx_collect_agents(int32_t grp_idx, vector_t *vec)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	agent_t *agent = NULL;
	int cur_row = 0;
	int max_row = 0;
	int success;

	AMP_DEBUG_ENTRY("db_tx_collect_agents","(%d)", grp_idx);

	/*
	 * Step 1: Grab the list of agents from the DB for this
	 *         message group.
	 */

	if(db_mgt_query_fetch(&res,
			              "SELECT AgentID FROM dbtOutgoingRecipients "
			              "WHERE OutgoingID=%d", grp_idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_tx_collect_agents",
				        "Can't get agents for grp: %d", grp_idx);
		AMP_DEBUG_EXIT("db_tx_collect_agents","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}


	/* Step 3: For each row returned.... */
	max_row = mysql_num_rows(res);
	*vec = vec_create(1, NULL, NULL, NULL, 0, &success);
	for(cur_row = 0; cur_row < max_row; cur_row++)
	{
		if ((row = mysql_fetch_row(res)) != NULL)
		{
			/* Step 3.1: Grab the agent information.. */
			if((agent = db_fetch_agent(atoi(row[0]))) != NULL)
			{
				AMP_DEBUG_INFO("db_outgoing_process_recipients",
						         "Adding agent name %s.",
						         agent->eid.name);

				vec_push(vec, agent);
			}
			else
			{
				AMP_DEBUG_ERR("db_outgoing_process_recipients",
						        "Cannot fetch registered agent",NULL);
			}
		}
	}

	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_outgoing_process_recipients","-->0x%#llx",
			         vec_num_entries(*vec));

	return AMP_OK;
}



/******************************************************************************
 *
 * \par Function Name: db_outgoing_ready
 *
 * \par Returns number of outgoing message groups ready to be sent.
 *
 * \retval 0 no message groups ready.
 *        !0 There are message groups ready to be sent.
 *
 * \param[out] sql_res - The outgoing messages.
 *
 * \par Notes:
 *  - We are being very inefficient here, as we grab the full result and
 *    then ignore it, presumably to query it again later. We should
 *    optimize this.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *  08/27/15  E. Birrane      Updated to newer schema
 *****************************************************************************/

int db_outgoing_ready(MYSQL_RES **sql_res)
{
	int result = 0;
	char query[1024];

	*sql_res = NULL;

	AMP_DEBUG_ENTRY("db_outgoing_ready","("ADDR_FIELDSPEC")", (uaddr) sql_res);

	CHKCONN

	/* Step 0: Sanity check. */
	if(sql_res == NULL)
	{
		AMP_DEBUG_ERR("db_outgoing_ready", "Bad Parms.", NULL);
		AMP_DEBUG_EXIT("db_outgoing_ready","-->0",NULL);
		return 0;
	}

	/* Step 1: Build and execute query. */
	sprintf(query, "SELECT * FROM dbtOutgoingMessageGroup WHERE State=%d", TX_READY);
	if (mysql_query(gConn, query))
	{
		AMP_DEBUG_ERR("db_outgoing_ready", "Database Error: %s",
				mysql_error(gConn));
		AMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
		return result;
	}

	/* Step 2: Parse the row and populate the structure. */
	if ((*sql_res = mysql_store_result(gConn)) != NULL)
	{
		result = mysql_num_rows(*sql_res);
	}
	else
	{
		AMP_DEBUG_ERR("db_outgoing_ready", "Database Error: %s",
				mysql_error(gConn));
	}

        //EJB
        if(result > 0)
        {
          AMP_DEBUG_ERR("db_outgoing_ready","There are %d rows ready.", result);
        }

	/* Step 3: Return whether we have results waiting. */
	AMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
	return result;
}






/******************************************************************************
 *
 * \par Function Name: db_fetch_reg_agent
 *
 * \par Creates an adm_reg_agent_t structure from the database.
 *
 * \retval NULL Failure
 *        !NULL The built adm_reg_agent_t  structure.
 *
 * \param[in] id - The Primary Key of the desired registered agent.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

agent_t *db_fetch_agent(int32_t id)
{
	agent_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_agent","(%d)", id);

	/* Step 1: Grab the OID row. */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtRegisteredAgents WHERE ID=%d",
						  id) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_agent","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_agent","-->NULL", NULL);
		return NULL;
	}

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		eid_t eid;
		strncpy(eid.name, row[1], AMP_MAX_EID_LEN);

		/* Step 3: Create structure for agent */
		if((result = agent_create(&eid)) == NULL)
		{
			AMP_DEBUG_ERR("db_fetch_agent","Cannot create a registered agent",NULL);
			mysql_free_result(res);
			AMP_DEBUG_EXIT("db_fetch_agent","-->NULL", NULL);
			return NULL;
		}
	}

	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_agent", "-->"ADDR_FIELDSPEC, (uaddr) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_reg_agent_idx
 *
 * \par Retrieves the index associated with an agent's EID.
 *
 * \retval 0 Failure
 *        !0 The index of the agent.
 *
 * \param[in] eid - The EID of the agent being queried.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_fetch_agent_idx(eid_t *eid)
{
	int32_t result = AMP_FAIL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_agent_idx","("ADDR_FIELDSPEC")", (uaddr) eid);

	/* Step 0: Sanity Check.*/
	if(eid == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_agent_idx","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_fetch_agent_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: Grab the OID row. */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtRegisteredAgents WHERE AgentId='%s'",
						  eid->name) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_agent_idx","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_agent_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}
	else
	{
		AMP_DEBUG_ERR("db_fetch_agent_idx", "Did not find EID with ID of %s\n", eid->name);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_agent_idx","-->%d", result);
	return result;
}



ac_t*    db_fetch_ari_col(int idx)
{
	AMP_DEBUG_ERR("db_fetch_ari_col","Not Implemented.", NULL);
	return NULL;
}


#if 0


//TODO - Add transactions
//TODO - Make -1 the system error return and 0 the non-system-error return.
//TODO - Update the comments.

/******************************************************************************
 *
 * \par Function Name: db_add_adm()
 *
 * \par Adds an ADM to the DB list of supported ADMs.
 *
 * Tables Effected:
 *    1. dbtADMs
 *
 * +---------+------------------+------+-----+---------+----------------+
 * | Field   | Type             | Null | Key | Default | Extra          |
 * +---------+------------------+------+-----+---------+----------------+
 * | ID      | int(10) unsigned | NO   | PRI | NULL    | auto_increment |
 * | Label   | varchar(255)     | NO   | UNI |         |                |
 * | Version | varchar(255)     | NO   |     |         |                |
 * | OID     | int(10) unsigned | NO   | MUL | NULL    |                |
 * +---------+------------------+------+-----+---------+----------------+
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in]  name     - The name of the ADM.
 * \param[in]  version  - Version of the ADM.
 * \param[in]  oid_root - ADM root OID.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation,
 *  04/02/16  E. Birrane     Check connection
 *  01/24/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/

int32_t db_add_adm(char *name, char *version, char *oid_root)
{
	uint32_t oid_idx = 0;
	uint32_t row_idx = 0;

	AMP_DEBUG_ENTRY("db_add_adm,"ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
			        (uaddr)name, (uaddr)version, (uaddr)oid_root);

	/* Step 0: Sanity check. */
	if((name == NULL) || (version == NULL) || (oid_root == NULL))
	{
		AMP_DEBUG_ERR("db_add_adm","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_add_adm","-->0",NULL);
		return AMP_FAIL;
	}

	/*
	 * Step 1: If the adm is already in the DB, return the index.
	 *         If there was a system error, return that.
	 */
	if((row_idx = db_fetch_adm_idx(name, version)) != 0)
	{
		return row_idx;
	}

	db_mgt_txn_start();

	/* Step 2 - Put the OID in the Database and save the index. */
	if((oid_idx = db_add_oid_str(oid_root)) <= 0)
	{
		AMP_DEBUG_ERR("db_add_adm","Can't add ADM OID to DB.",NULL);

		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_adm","-->%d",oid_idx);
		return oid_idx;
	}

	/* Step 3: Write the ADM entry into the DB. */
	if(db_mgt_query_insert(&row_idx, "INSERT INTO dbtADMs(Label, Version, OID) "
			"VALUES('%s','%s',%d)", name, version, oid_idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_adm","Can't add ADM to DB.",NULL);

		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_adm","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_add_adm", "-->%d", row_idx);
	return row_idx;
}





/******************************************************************************
 *
 * \par Function Name: db_add_tdc
 *
 * \par Adds a TDC to the database and returns the index of the
 *      parameters table.
 *
 * Tables Effected:
 *    1. dbtDataCollections
 *
 *   +-------+------------------+------+-----+-------------------------+----------------+
 *   | Field | Type             | Null | Key | Default                 | Extra          |
 *   +-------+------------------+------+-----+-------------------------+----------------+
 *   | ID    | int(10) unsigned | NO   | PRI | NULL                    | auto_increment |
 *   | Label | varchar(255)     | NO   |     | Unnamed Data Collection |                |
 *   +-------+------------------+------+-----+-------------------------+----------------+
 *
 *
 *    2. dbtDataCollection
 *
 *   +--------------+------------------+------+-----+---------+-------+
 *   | Field        | Type             | Null | Key | Default | Extra |
 *   +--------------+------------------+------+-----+---------+-------+
 *   | CollectionID | int(10) unsigned | NO   | PRI | NULL    |       |
 *   | DataOrder    | int(10) unsigned | NO   | PRI | 0       |       |
 *   | DataType     | int(10) unsigned | NO   | MUL | NULL    |       |
 *   | DataBlob     | blob             | YES  |     | NULL    |       |
 *   +--------------+------------------+------+-----+---------+-------+
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in]  tdc  - The TDC being added to the DB.
 *
 * \par Notes:
 *		- Comments for the dc are not included.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/22/15  E. Birrane     Initial Implementation
 *  09/10/15  E. Birrane     Update to to db spec.
 *  11/12/16  E. Birrane     Update to new schema. Optimizations. (JHU/APL)
 *  01/24/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/

int32_t db_add_tdc(tdc_t tdc)
{
	uint32_t tdc_idx = 0;
	LystElt elt;

	AMP_DEBUG_ENTRY("db_add_tdc", "(%d)", tdc.hdr.length);

	/* Step 0: Sanity check arguments. */
	if(tdc.hdr.length == 0)
	{
		AMP_DEBUG_WARN("db_add_tdc","Not persisting empty TDC",NULL);
		AMP_DEBUG_EXIT("db_add_tdc","-->0",NULL);
		return AMP_FAIL;
	}

	db_mgt_txn_start();

	/*
	 * Step 1: Build and execute query to add row to dbtDataCollections. Also, store the
	 *         new DC index.
	 */

	if(db_mgt_query_insert(&tdc_idx, "INSERT INTO dbtDataCollections (Label) "
			"VALUE(NULL)") != AMP_OK)
	{
		AMP_DEBUG_WARN("db_add_tdc","Can't insert TDC",NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_add_tdc","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	/*
	 *  Step 2: For each BLOB in the data collection, add it to the data collection
	 *          entries table.
	 */
	for(elt = lyst_first(tdc.datacol); elt; elt = lyst_next(elt))
	{
		blob_t *entry = (blob_t *) lyst_data(elt);
		int i = 0;
		char *content = NULL;
		uint32_t content_len = 0;
		uint32_t dc_idx = 0;

		/* Step 2.1: Built sting version of the BLOB data. */
		if((content = utils_hex_to_string(entry->value, entry->length)) == NULL)
		{
			AMP_DEBUG_ERR("db_add_tdc","Can't cvt %d bytes to hex str.", entry->length);

			db_mgt_txn_rollback();

			AMP_DEBUG_EXIT("db_add_tdc", "-->%d", AMP_FAIL);
			return AMP_FAIL;
		}

		/* Step 2.2: Write the data into the DC collections table and associated
		 *           the entry with the data collection idx.
		 *           NOTE: content+2 is used to "skip over" the leading "0x"
		 *           characters that preface the contents information.
		 */

		if(db_mgt_query_insert(&dc_idx,
				           "INSERT INTO dbtDataCollection(CollectionID, DataOrder, DataType,DataBlob)"
		 	                "VALUES(%d,1,%d,'%s')",
							tdc_idx,
							tdc.hdr.data[i],
							content+2) != AMP_OK)
		{
			AMP_DEBUG_ERR("db_add_tdc","Can't insert entry %d.", i);
			SRELEASE(content);
			db_mgt_txn_rollback();

			AMP_DEBUG_EXIT("db_add_tdc", "-->%d", AMP_FAIL);
			return AMP_FAIL;
		}

		i++;
		SRELEASE(content);
	}

	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_add_tdc", "-->%d", tdc_idx);
	return tdc_idx;
}



/******************************************************************************
 *
 * \par Function Name: db_add_mid
 *
 * \par Creates a MID in the database.
 *
 * Tables Effected:
 *    1. dbtMIDs
 *
 *    +--------------+---------------------+------+-----+--------------------+----------------+
 *    | Field        | Type                | Null | Key | Default            | Extra          |
 *    +--------------+---------------------+------+-----+--------------------+----------------+
 *    | ID           | int(10) unsigned    | NO   | PRI | NULL               | auto_increment |
 *    | NicknameID   | int(10) unsigned    | YES  | MUL | NULL               |                |
 *    | OID          | int(10) unsigned    | NO   | MUL | NULL               |                |
 *    | ParametersID | int(10) unsigned    | YES  | MUL | NULL               |                |
 *    | Type         | int(10) unsigned    | NO   | MUL | NULL               |                |
 *    | Category     | int(10) unsigned    | NO   | MUL | NULL               |                |
 *    | IssuerFlag   | bit(1)              | NO   |     | b'0'               |                |
 *    | TagFlag      | bit(1)              | NO   |     | b'0'               |                |
 *    | OIDType      | int(10) unsigned    | YES  | MUL | NULL               |                |
 *    | IssuerID     | bigint(20) unsigned | NO   |     | 0                  |                |
 *    | TagValue     | bigint(20) unsigned | NO   |     | 0                  |                |
 *    | DataType     | int(10) unsigned    | NO   | MUL | NULL               |                |
 *    | Name         | varchar(50)         | NO   |     | Unnamed MID        |                |
 *    | Description  | varchar(255)        | NO   |     | No MID Description |                |
 *    +--------------+---------------------+------+-----+--------------------+----------------+
 *
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] mid     - The MID to be persisted in the DB.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  08/23/15  E. Birrane     Update to new DB Schema.
 *  01/24/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/
int32_t db_add_mid(mid_t *mid)
{
	int32_t nn_idx = 0;
	int32_t oid_idx = 0;
	int32_t parm_idx = 0;
	int32_t num_parms = 0;
	uint32_t mid_idx = 0;

	AMP_DEBUG_ENTRY("db_add_mid", "("ADDR_FIELDSPEC",%d)", (uaddr)mid);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		AMP_DEBUG_ERR("db_add_mid","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_mid","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: Make sure the ID is not already in the DB (or not failure). */
	if ((mid_idx = db_fetch_mid_idx(mid)) != 0)
	{
		AMP_DEBUG_WARN("db_add_mid","MID already exists.",NULL);
		AMP_DEBUG_EXIT("db_add_mid", "-->%d", mid_idx);
		return mid_idx;
	}

	/* Step 2: If this MID has a nickname, grab the index. */
	if((MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_FULL) ||
	   (MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_PARAM))
	{
		if((nn_idx = db_fetch_nn_idx(mid->oid.nn_id)) <= 0)
		{
			AMP_DEBUG_ERR("db_add_mid","MID references unknown Nickname %d", mid->oid.nn_id);
			AMP_DEBUG_EXIT("db_add_mid", "-->%d", nn_idx);
			return nn_idx;
		}
	}

	db_mgt_txn_start();

	/* Step 3: Get the index for the OID. */
	if((oid_idx = db_add_oid(mid->oid)) <= 0)
	{
		AMP_DEBUG_ERR("db_add_mid", "Can't add OID.", NULL);
		AMP_DEBUG_EXIT("db_add_mid", "-->%d", oid_idx);
		return oid_idx;
	}

	/* Step 4: Get the index for parameters, if any. */
	if((num_parms = oid_get_num_parms(mid->oid)) > 0)
	{
		if((parm_idx = db_add_parms(mid->oid)) <= 0)
		{
			AMP_DEBUG_ERR("db_add_mid", "Can't add PARMS.", NULL);

			db_mgt_txn_rollback();

			AMP_DEBUG_EXIT("db_add_mid", "-->%d", parm_idx);
			return parm_idx;
		}
	}

	/*
	 * Step 5: Build and execute query to add row to dbtMIDs. Also, store the
	 *         row ID of the inserted row.
	 */

	if(db_mgt_query_insert(&mid_idx,
			            "INSERT INTO dbtMIDs(NicknameID,OID,ParametersID,Type,Category,IssuerFlag,TagFlag,"
			            "OIDType,IssuerID,TagValue,DataType,Name,Description)"
			            "VALUES (%s, %d, %s, %d, %d, %d, %d, %d, "UVAST_FIELDSPEC","UVAST_FIELDSPEC",%d,'%s','%s')",
			            (nn_idx == 0) ? "NULL" : itoa(nn_idx),
			            oid_idx,
			            (parm_idx == 0) ? "NULL" : itoa(parm_idx),
			            0,
			            MID_GET_FLAG_ID(mid->flags),
			            (MID_GET_FLAG_ISS(mid->flags)) ? 1 : 0,
			            (MID_GET_FLAG_TAG(mid->flags)) ? 1 : 0,
			            MID_GET_FLAG_OID(mid->flags),
			            mid->issuer,
			            mid->tag,
			            AMP_TYPE_MID,
			            "No Name",
			            "No Descr") != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_mid", "Can't add MID.", NULL);

		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_mid", "-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	db_mgt_txn_commit();

	AMP_DEBUG_EXIT("db_add_mid", "-->%d", mid_idx);
	return mid_idx;
}




/******************************************************************************
 *
 * \par Function Name: db_add_mc
 *
 * \par Creates a MID Collection in the database.
 *
 * Tables Effected:
 *    1. dbtMIDCollections
 *
 *   +---------+------------------+------+-----+---------+----------------+
 *   | Field   | Type             | Null | Key | Default | Extra          |
 *   +---------+------------------+------+-----+---------+----------------+
 *   | ID      | int(10) unsigned | NO   | PRI | NULL    | auto_increment |
 *   | Comment | varchar(255)     | YES  |     | NULL    |                |
 *   +---------+------------------+------+-----+---------+----------------+

 *       +---------------+------------+---------------+-----------------------+
 *       | Column Object |     Type   | Default Value | Comment               |
 *       +---------------+------------+---------------+-----------------------+
 *       |      ID*      | Int32      | auto-         | Used as primary key   |
 *       |               |(unsigned)  | incrementing  |                       |
 *       +---------------+------------+---------------+-----------------------+
 *       | Comment       |VARCHAR(255)| ''            | Optional Comment      |
 *       +---------------+------------+---------------+-----------------------+
 *
 *    2. dbtMIDCollection
 *       +---------------+------------+---------------+-----------------------+
 *       | Column Object |     Type   | Default Value | Comment               |
 *       +---------------+------------+---------------+-----------------------+
 *       | CollectionID  | Int32      | 0             | Foreign key into      |
 *       |               |(unsigned)  |               | dbtMIDCollection.ID   |
 *       +---------------+------------+---------------+-----------------------+
 *       | MIDID         | Int32      | 0             | Foreign key into      |
 *       |               | (unsigned) |               | dbtMIDs.ID            |
 *       +---------------+------------+---------------+-----------------------+
 *       | MIDOrder      | Int32      | 0             | Order of MID in the   |
 *       |               | (unsigned) |               | Collection.           |
 *       +---------------+------------+---------------+-----------------------+
 *
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] mc     - The MC being added to the DB.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/23/15  E. Birrane     Initial Implementation
 *  01/25/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/
int32_t db_add_mc(Lyst mc)
{
	uint32_t mc_idx = 0;
	LystElt elt = NULL;
	mid_t *mid = NULL;
	uint32_t i = 0;
	int32_t mid_idx = 0;

	AMP_DEBUG_ENTRY("db_add_mc", "("ADDR_FIELDSPEC")", (uaddr)mc);

	/* Step 0 - Sanity check arguments. */
	if(mc == NULL)
	{
		AMP_DEBUG_ERR("db_add_mc","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_mc","-->%d", AMP_SYSERR);
		return AMP_SYSERR;
	}

	db_mgt_txn_start();

	/* Step 1 - Create a new entry in the dbtMIDCollections DB. */
	if(db_mgt_query_insert(&mc_idx,
			            "INSERT INTO dbtMIDCollections (Comment) VALUES ('No Comment')") != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_mc","Can't insert MC",NULL);

		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_mc","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2 - For each MID in the MC, add the MID into the dbtMIDCollection. */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		/* Step 2a: Extract nth MID from MC. */
		if((mid = (mid_t *) lyst_data(elt)) == NULL)
		{
			AMP_DEBUG_ERR("db_add_mc","Can't get MID.", NULL);

			db_mgt_txn_rollback();
			AMP_DEBUG_EXIT("db_add_mc", "-->%d", AMP_SYSERR);
			return AMP_SYSERR;
		}

		/* Step 2b: Make sure MID is in the DB. */
		if((mid_idx = db_add_mid(mid)) > 0)
		{
			AMP_DEBUG_ERR("db_add_mc","MID not there and can't insert.", NULL);

			db_mgt_txn_rollback();
			AMP_DEBUG_EXIT("db_add_mc", "-->%d", mid_idx);
			return mid_idx;
		}

		/* Step 2c - Insert entry into DB MC list from this MC. */
		if(db_mgt_query_insert(NULL,
				            "INSERT INTO dbtMIDCollection"
							"(CollectionID, MIDID, MIDOrder)"
							"VALUES (%d, %d, %d",
							mc_idx, mid_idx, i) != AMP_OK)
		{
			AMP_DEBUG_ERR("db_add_mc","Can't insert MID %d", i);

			db_mgt_txn_rollback();

			AMP_DEBUG_EXIT("db_add_mc","-->%d", AMP_FAIL);
			return AMP_FAIL;
		}

		i++;
	}

	db_mgt_txn_commit();

	AMP_DEBUG_EXIT("db_add_mc", "-->%d", mc_idx);
	return mc_idx;
}



/******************************************************************************
 *
 * \par Function Name: db_add_nn
 *
 * \par Creates a Nickname in the database.
 *
 * Tables Effected:
 *
 *    1. dbtMIDCollections
 *
 * +----------------+------------------+------+-----+--------------------------+----------------+
 * | Field          | Type             | Null | Key | Default                  | Extra          |
 * +----------------+------------------+------+-----+--------------------------+----------------+
 * | ID             | int(10) unsigned | NO   | PRI | NULL                     | auto_increment |
 * | ADM_ID         | int(10) unsigned | NO   | MUL | NULL                     |                |
 * | Nickname_UID   | int(10) unsigned | NO   |     | NULL                     |                |
 * | Nickname_Label | varchar(25)      | NO   |     | Undefined ADM Tree Value |                |
 * | OID            | int(10) unsigned | NO   | MUL | NULL                     |                |
 * +----------------+------------------+------+-----+--------------------------+----------------+
 *
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] nn     - The Nickname being added to the DB.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial Implementation
 *  01/25/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/

int32_t db_add_nn(oid_nn_t *nn)
{
	uint32_t nn_idx = 0;
	oid_t oid;
	int32_t oid_idx = 0;
	int32_t adm_idx = 0;

	AMP_DEBUG_ENTRY("db_add_nn", "("ADDR_FIELDSPEC")", (uaddr)nn);

	/* Step 0 - Sanity check arguments. */
	if(nn == NULL)
	{
		AMP_DEBUG_ERR("db_add_nn","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_nn","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	/*
	 * Step 1 - Duplicate check.
	 */
	if((nn_idx = db_fetch_nn_idx(nn->id)) > 0)
	{
		AMP_DEBUG_EXIT("db_add_nn","-->%d", nn_idx);
		return nn_idx;
	}

	db_mgt_txn_start();

	/* Step 2 - Ensure OID. */
	oid = oid_construct(OID_TYPE_FULL, NULL, 0, nn->raw, nn->raw_size);
	if( (oid.type == OID_TYPE_UNK) ||
	    ((oid_idx = db_add_oid(oid)) <= 0))
	{
		AMP_DEBUG_ERR("db_add_nn","Can't create OID.",NULL);
		oid_release(&oid);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_add_nn","-->%d",oid_idx);
		return oid_idx;
	}

	oid_release(&oid);

	/* Step 3 - Add the ADM. */
	if((adm_idx = db_fetch_adm_idx(nn->adm_name, nn->adm_ver)) <= 0)
	{
		AMP_DEBUG_ERR("db_add_nn","Can't Find ADM.",NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_add_nn","-->%d",adm_idx);
		return adm_idx;
	}


	/* Step 3 - Create a new entry in the dbtADMNicknames DB. */
	if(db_mgt_query_insert(&nn_idx,
			            "INSERT INTO dbtADMNicknames (ADM_ID, Nickname_UID, Nickname_Label, OID)"
						"VALUES (%d, "UVAST_FIELDSPEC", 'No Comment', %d)",
						adm_idx, nn->id, oid_idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_nn","Can't insert Nickname", NULL);

		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_nn","-->%d", AMP_SYSERR);
		return AMP_SYSERR;
	}

	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_add_nn", "-->%d", nn_idx);
	return nn_idx;
}



/******************************************************************************
 *
 * \par Function Name: db_add_oid_str
 *
 * \par Adds an OID to the database given a serialized string rep of the OID.
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] oid_str - The string representation of the OID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial Implementation
 *  01/25/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/

int32_t db_add_oid_str(char *oid_str)
{
	uint8_t *data = NULL;
	uint32_t datasize = 0;
	int32_t result = 0;
	oid_t oid;

	AMP_DEBUG_ENTRY("db_add_oid_str", "("ADDR_FIELDSPEC")", (uaddr)oid_str);

	/* Step 1: Sanity checks. */
	if(oid_str == NULL)
	{
		AMP_DEBUG_ERR("db_add_oid_str","Bad args", NULL);
		AMP_DEBUG_EXIT("db_add_oid_str","-->%d", AMP_SYSERR);

		return AMP_FAIL;
	}

	/* Step 2: Assume input is not in hex and convert to hex string. */
	if((data = utils_string_to_hex(oid_str,&datasize)) == NULL)
	{
		AMP_DEBUG_ERR("db_add_oid_str","Can't convert OID of %s.", oid_str);
		return AMP_FAIL;
	}

	/* Step 3: Build an OID. */
	oid = oid_construct(OID_TYPE_FULL, NULL, 0, data, datasize);
	SRELEASE(data);

	if(oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("db_add_oid_str","Can't create OID.",NULL);
		SRELEASE(data);
		AMP_DEBUG_EXIT("db_add_oid_str","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	result = db_add_oid(oid);

	oid_release(&oid);

	AMP_DEBUG_EXIT("db_add_oid_str", "-->%d", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_add_oid()
 *
 * \par Adds an Object Identifier to the database, or returns the index of
 *      a matching object identifier.
 *
 * Tables Effected:
 *    1. dbtOIDs
 *  +-------------+------------------+------+-----+---------------------+----------------+
 *  | Field       | Type             | Null | Key | Default             | Extra          |
 *  +-------------+------------------+------+-----+---------------------+----------------+
 *  | ID          | int(10) unsigned | NO   | PRI | NULL                | auto_increment |
 *  | IRI_Label   | varchar(255)     | NO   | MUL | Undefined IRI value |                |
 *  | Dot_Label   | varchar(255)     | NO   |     | 190.239.254.237     |                |
 *  | Encoded     | varchar(255)     | NO   |     | BEEFFEED            |                |
 *  | Description | varchar(255)     | NO   |     |                     |                |
 *  +-------------+------------------+------+-----+---------------------+----------------+
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in]  oid  - The OID being added to the DB.
 * \param[in]  spec - Listing of types of oid parms, if they exist.
 * \par Notes:
 *		- Only the encoded OID is persisted in the database.
 *		  No other OID information is persisted at this time.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/22/15  E. Birrane     Initial Implementation
 *  01/25/17  E. Birrane     Update to AMP IOS 3.5.0. (JHU/APL)
 *****************************************************************************/

int32_t db_add_oid(oid_t oid)
{
	uint32_t oid_idx = 0;
	int32_t num_parms = 0;
	char *oid_str = NULL;

	AMP_DEBUG_ENTRY("db_add_oid", "(%d)", oid.type);

	/* Step 0: Sanity check arguments. */
	if(oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("db_add_oid","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_oid","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	/*
	 * Step 1: Return existing ID, or failure code.
	 */
	if ((oid_idx = db_fetch_oid_idx(oid)) != 0)
	{
		AMP_DEBUG_EXIT("db_add_oid","-->%d", oid_idx);
		return oid_idx;
	}

	/* Step 2: Convert OID to string for storage. */
	if((oid_str = oid_to_string(oid)) == NULL)
	{
		AMP_DEBUG_ERR("db_add_oid","Can't get string rep of OID.", NULL);
		AMP_DEBUG_EXIT("db_add_oid","-->%d", AMP_SYSERR);
		return AMP_SYSERR;
	}

	db_mgt_txn_start();

	/* Step 3: Build and execute query to add row to dbtOIDs. */

	if(db_mgt_query_insert(&oid_idx,
			            "INSERT INTO dbtOIDs"
						"(IRI_Label, Dot_Label, Encoded, Description)"
						"VALUES ('empty','empty','%s','empty')",
					    oid_str) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_oid","Can't insert Nickname", NULL);

		SRELEASE(oid_str);
		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_oid","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	SRELEASE(oid_str);

	db_mgt_txn_commit();

	AMP_DEBUG_EXIT("db_add_oid", "-->%d", oid_idx);
	return oid_idx;
}



/******************************************************************************
 *
 * \par Function Name: db_add_parms
 *
 * \par Adds OID parameters to the database and returns the index of the
 *      parameters table.
 *
 * Tables Effected:
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in]  oid  - The OID whose parameters are being added to the DB.
 *
 * \par Notes:
 *		- Comments for the parameters are not included.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/22/15  E. Birrane     Initial Implementation
 *  09/10/15  E. Birrane     Removed dbtMIDParameters
 *  01/24/17  E. Birrane     Updated to new AMP implementation for ION 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_add_parms(oid_t oid)
{
	int32_t result = 0;

	AMP_DEBUG_ENTRY("db_add_parms", "(%d)", oid.type);

	/* Step 0: Sanity check arguments. */
	if(oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("db_add_parms","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_parms","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	result = db_add_tdc(oid.params);

	AMP_DEBUG_EXIT("db_add_parms", "-->%d", result);

	return result;
}




/******************************************************************************
 *
 * \par Function Name: db_add_protomid
 *
 * \par Creates a MID template in the database.
 *
 * Tables Effected:
 *    1. dbtProtoMIDs
 *
 * +--------------+------------------+------+-----+--------------------+----------------+
 * | Field        | Type             | Null | Key | Default            | Extra          |
 * +--------------+------------------+------+-----+--------------------+----------------+
 * | ID           | int(10) unsigned | NO   | PRI | NULL               | auto_increment |
 * | NicknameID   | int(10) unsigned | YES  | MUL | NULL               |                |
 * | OID          | int(10) unsigned | NO   | MUL | NULL               |                |
 * | ParametersID | int(10) unsigned | YES  | MUL | NULL               |                |
 * | DataType     | int(10) unsigned | NO   | MUL | 0                  |                |
 * | OIDType      | int(10) unsigned | NO   | MUL | 0                  |                |
 * | Type         | int(10) unsigned | NO   | MUL | 0                  |                |
 * | Category     | int(10) unsigned | NO   | MUL | 0                  |                |
 * | Name         | varchar(50)      | NO   |     | Unnamed MID        |                |
 * | Description  | varchar(255)     | NO   |     | No MID Description |                |
 * +--------------+------------------+------+-----+--------------------+----------------+
 *
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in] mid     - The MID to be persisted in the DB.
 * \param[in] spec    - The parameter spec for this protomid.
 * \param[in] type    - The type of the MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/28/15  E. Birrane     Initial implementation,
 *  01/24/17  E. Birrane     Update to latest version of AMP 3.5.0 (JHU/APL)
 *****************************************************************************/
int32_t db_add_protomid(mid_t *mid, ui_parm_spec_t *spec, amp_type_e type)
{
	int32_t result = 0;
	uint32_t nn_idx = 0;
	uint32_t oid_idx = 0;
	uint32_t parm_idx = 0;
	uint32_t num_parms = 0;

	AMP_DEBUG_ENTRY("db_add_protomid", "("ADDR_FIELDSPEC","ADDR_FIELDSPEC",%d)",
			       (uaddr)mid, (uaddr)spec, type);

	/* Step 0: Sanity check arguments. */
	if((mid == NULL) || (spec==NULL))
	{
		AMP_DEBUG_ERR("db_add_protomid","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_protomid","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: Make sure the ID is not already in the DB. */
	if ((result = db_fetch_protomid_idx(mid)) != 0)
	{
		AMP_DEBUG_EXIT("db_add_protomid", "-->%d", result);
		return result;
	}

	/* Step 2: If this MID has a nickname, grab the index. */
	if((MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_FULL) ||
	   (MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_PARAM))
	{
		if((nn_idx = db_fetch_nn_idx(mid->oid.nn_id)) <= 0)
		{
			AMP_DEBUG_ERR("db_add_protomid","MID references unknown Nickname %d", mid->oid.nn_id);
			AMP_DEBUG_EXIT("db_add_protomid", "-->%d", nn_idx);
			return nn_idx;
		}
	}

	db_mgt_txn_start();

	/* Step 3: Get the index for the OID. */
	if((oid_idx = db_add_oid(mid->oid)) <= 0)
	{
		AMP_DEBUG_ERR("db_add_protomid", "Can't add OID.", NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_add_protomid", "-->%d", oid_idx);
		return oid_idx;
	}

	/* Step 4: Get the index for parameters, if any. */
	if((parm_idx = db_add_protoparms(spec)) <= 0)
	{
		AMP_DEBUG_ERR("db_add_protomid", "Can't add protoparms.", NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_add_protomid", "-->%d", parm_idx);
		return parm_idx;
	}

	if(db_mgt_query_insert((uint32_t*)&result,
			"INSERT INTO dbtProtoMIDs"
						"(NicknameID,OID,ParametersID,Type,Category,"
						"OIDType,DataType,Name,Description)"
						"VALUES (%s, %d, %d, %d, %d, %d, %d, '%s','%s')",
						(nn_idx == 0) ? "NULL" : itoa(nn_idx),
						oid_idx,
						parm_idx,
						0,
						MID_GET_FLAG_ID(mid->flags),
						MID_GET_FLAG_OID(mid->flags),
						type,
						"No Name",
						"No Descr") != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_protomid", "Can't add protomid.", NULL);
		db_mgt_txn_rollback();
		AMP_DEBUG_EXIT("db_add_protomid", "-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	db_mgt_txn_commit();

	AMP_DEBUG_EXIT("db_add_protomid", "-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_add_protoparms
 *
 * \par Adds parameter specs to the database and returns the index of the
 *      parameters table.
 *
 * Tables Effected:
 *    1. dbtProtoMIDParameters
 *
 * +---------+------------------+------+-----+------------+----------------+
 * | Field   | Type             | Null | Key | Default    | Extra          |
 * +---------+------------------+------+-----+------------+----------------+
 * | ID      | int(10) unsigned | NO   | PRI | NULL       | auto_increment |
 * | Comment | varchar(255)     | NO   |     | No Comment |                |
 * +---------+------------------+------+-----+------------+----------------+
 *
 *
 *    2. dbtProtoMIDParameter
 *
 * +-----------------+------------------+------+-----+---------+----------------+
 * | Field           | Type             | Null | Key | Default | Extra          |
 * +-----------------+------------------+------+-----+---------+----------------+
 * | ID              | int(10) unsigned | NO   | PRI | NULL    | auto_increment |
 * | CollectionID    | int(10) unsigned | YES  | MUL | NULL    |                |
 * | ParameterOrder  | int(10) unsigned | NO   |     | 0       |                |
 * | ParameterTypeID | int(10) unsigned | YES  | MUL | NULL    |                |
 * +-----------------+------------------+------+-----+---------+----------------+
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The index of the inserted item.
 *
 * \param[in]  spec - The parm spec that gives the types of OID parms.
 *
 * \par Notes:
 *		- Comments for the parameters are not included.
 *		- A return of AMP_FAIL is only an error if the oid has parameters.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/28/15  E. Birrane     Initial Implementation
 *  01/25/17  E. Birrane     Update to latest version of AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_add_protoparms(ui_parm_spec_t *spec)
{
	int32_t i = 0;
	int32_t num_parms = 0;
	uint32_t result = 0;
	uint32_t parm_idx = 0;

	AMP_DEBUG_ENTRY("db_add_protoparms", "("ADDR_FIELDSPEC")",
					  (uaddr) spec);

	/* Step 0: Sanity check arguments. */
	if(spec == NULL)
	{
		AMP_DEBUG_ERR("db_add_protoparms","Bad args",NULL);
		AMP_DEBUG_EXIT("db_add_protoparms","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	if((spec->num_parms == 0) || (spec->num_parms >= MAX_PARMS))
	{
		AMP_DEBUG_ERR("db_add_protoparms","Bad # parms.",NULL);
		AMP_DEBUG_EXIT("db_add_protoparms","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	db_mgt_txn_start();

	/* Step 1: Add an entry in the parameters table. */
	if(db_mgt_query_insert(&result,
			"INSERT INTO dbtProtoMIDParameters (Comment) "
						"VALUES ('No comment')") != AMP_OK)
	{
		AMP_DEBUG_ERR("db_add_protoparms","Can't insert Protoparm", NULL);

		db_mgt_txn_rollback();

		AMP_DEBUG_EXIT("db_add_protoparms","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: For each parameter, get the DC, add the DC into the DB,
	 * and then add an entry into dbtMIDParameter
	 */

	for(i = 0; i < spec->num_parms; i++)
	{

		if(db_mgt_query_insert(&parm_idx,
				"INSERT INTO dbtProtoMIDParameter "
								"(CollectionID, ParameterOrder, ParameterTypeID) "
								"VALUES (%d, %d, %d)",
								result, i, spec->parm_type[i]) != AMP_OK)
		{
			AMP_DEBUG_ERR("db_add_protoparms","Can't insert Parm", NULL);

			db_mgt_txn_rollback();

			AMP_DEBUG_EXIT("db_add_protoparms","-->%d", AMP_FAIL);
			return AMP_FAIL;
		}
	}

	db_mgt_txn_commit();
	AMP_DEBUG_EXIT("db_add_protoparms", "-->%d", result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_adm_idx
 *
 * \par Gets the ADM index given an ADM description
 *
 * \return AMP_SYSERR - System Error
 *         AMP_FAIL   - Non-fatal issue.
 *         >0         - The fetched idx.
 *
 * \param[in] name    - The ADM name.
 * \param[in] version - The ADM version.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to latest version of AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_fetch_adm_idx(char *name, char *version)
{
	int32_t result = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_adm_idx","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					  (uaddr)name, (uaddr) version);

	if((name == NULL) || (version == NULL))
	{
		AMP_DEBUG_ERR("db_fetch_adm_idx","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_fetch_adm_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	if(db_mgt_query_fetch(&res,
			"SELECT * FROM dbtADMs WHERE Label='%s' AND Version='%s'",
			name, version) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_adm_idx","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_adm_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_adm_idx","-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_tdc
 *
 * \par Creates a typed data collection from dbtDataCollections in the database
 *
 * \retval a TDC object (with type UNKNOWN on error).
 *
 * \param[in] id - The Primary Key in the dbtDataCollections table.
 *
 * \par Notes:
 *  - A TDC with a length of 0 indicates an error retrieving the TDC.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *  08/23/15  E. Birrane     Update to new schema.
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/
tdc_t db_fetch_tdc(int32_t tdc_idx)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	tdc_t result;
	blob_t *entry;

	AMP_DEBUG_ENTRY("db_fetch_tdc", "(%d)", tdc_idx);

	tdc_init(&result);

	/* Step 1: Construct/run the Query and capture results. */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtDataCollection "
			              "WHERE CollectionID=%d "
						  "ORDER BY DataOrder",
						  tdc_idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_tdc","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_tdc","-->NULL", NULL);
		return result;
	}

	/* Step 2: For each entry returned as part of the collection. */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		amp_type_e type;

		if((entry = db_fetch_tdc_entry_from_row(row, &type)) == NULL)
		{
			AMP_DEBUG_ERR("db_fetch_tdc", "Can't get entry.", NULL);
			tdc_clear(&result);
			mysql_free_result(res);

			tdc_init(&result);

			AMP_DEBUG_EXIT("db_fetch_tdc","-->NULL", NULL);
			return result;
		}

		tdc_insert(&result, type, entry->value, entry->length);
	}

	/* Step 4: Free results. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_dc", "-->%d", result.hdr.length);
	return result;
}



/*******************************************************************************
 *
 * \par Function Name: db_fetch_data_col_entry_from_row
 *
 * \par Parses a data collection entry from a database row from the
 *      dbtataCollection table.
 *
 * \retval NULL The entry could not be retrieved
 *         !NULL The created data collection entry.
 *
 * \param[in] row  - The row containing the data col entry information.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/19/13  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 ******************************************************************************/

blob_t* db_fetch_tdc_entry_from_row(MYSQL_ROW row, amp_type_e *type)
{
	blob_t *result = NULL;
	uint8_t *value = NULL;
	uint32_t length = 0;

	AMP_DEBUG_ENTRY("db_fetch_tdc_entry_from_row","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					  (uaddr)row, (uaddr)type);

	/* Step 1: grab data from the row. */
	value = utils_string_to_hex(row[3], &length);
	if((value == NULL) || (length == 0))
	{
		AMP_DEBUG_ERR("db_fetch_tdc_entry_from_row", "Can't grab value for %s", row[3]);
		AMP_DEBUG_EXIT("db_fetch_tdc_entry_from_row", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Create the blob representing the entry. */
	if((result = blob_create(value,length)) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_tdc_entry_from_row", "Can't make blob", NULL);
		SRELEASE(value);
		AMP_DEBUG_EXIT("db_fetch_tdc_entry_from_row", "-->NULL", NULL);
		return NULL;
	}

	/* Step 3: Store the type. */
	*type = atoi(row[2]);

	AMP_DEBUG_EXIT("db_fetch_tdc_entry_from_row", "-->"ADDR_FIELDSPEC, (uaddr) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_mid
 *
 * \par Creates a MID structure from a row in the dbtMIDs database.
 *
 * \retval NULL Failure
 *        !NULL The built MID structure.
 *
 * \param[in] idx - The Primary Key of the desired MID in the dbtMIDs table.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  08/23/15  E. Birrane     Update to new schema.
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

mid_t *db_fetch_mid(int32_t idx)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	mid_t *result = NULL;

	AMP_DEBUG_ENTRY("db_fetch_mid", "(%d)", idx);

	/* Step 1: Construct and run the query to get the MID information. */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtMIDs WHERE ID=%d", idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_mid","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_mid","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_mid","Can't grab row", NULL);
		mysql_free_result(res);
		AMP_DEBUG_EXIT("db_fetch_mid","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 3: Build MID from the row. */
	if((result = db_fetch_mid_from_row(row)) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_mid","Can't build MID from row", NULL);
		mysql_free_result(res);
		AMP_DEBUG_EXIT("db_fetch_mid","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_mid", "-->"ADDR_FIELDSPEC, (uaddr) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_mid_col
 *
 * \par Creates a MID collection from a row in the dbtMIDCollection database.
 *
 * \retval NULL Failure
 *        !NULL The built MID collection.
 *
 * \param[in] idx - The Primary Key in the dbtMIDCollection table.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation
 *  08/23/15  E. Birrane     Update to new database schema
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/
Lyst db_fetch_mid_col(int idx)
{
	Lyst result = lyst_create();
	mid_t *new_mid = NULL;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_mid_col","(%d)", idx);

	/* Step 1: Construct and run the query to get the MID information. */
	if(db_mgt_query_fetch(&res,
			              "SELECT MIDID FROM dbtMIDCollection WHERE CollectionID=%d ORDER BY MIDOrder",
						  idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_mid_col","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_mid_col","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: For each MID in the collection... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{

		/* Step 2.1: For each row, build a MID and add it to the collection. */
		if((new_mid = db_fetch_mid(atoi(row[0]))) == NULL)
		{
			AMP_DEBUG_ERR("db_fetch_mid_col", "Can't grab MID with ID %d.",
					        atoi(row[0]));

			midcol_destroy(&result);
			mysql_free_result(res);

			AMP_DEBUG_EXIT("db_fetch_mid_col", "-->NULL", NULL);
			return NULL;
		}

		lyst_insert_last(result, new_mid);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_mid_col", "-->"ADDR_FIELDSPEC, (uaddr) result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_mid_from_row
 *
 * \par Gets a MID from the MID table.
 *
 * \retval  NULL Failure
 *         !NULL The fetched MID
 *
 * \param[in] row  - The row containing the data col entry information.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/25/17  E. Birrane     Initial implementation, (JHU/APL)
 *****************************************************************************/

mid_t* db_fetch_mid_from_row(MYSQL_ROW row)
{
	oid_t oid;
	mid_t *result = NULL;

	AMP_DEBUG_ENTRY("db_fetch_mid_from_row", "("ADDR_FIELDSPEC")", (uaddr)row);

	/* Step 0: Sanity check. */
	if(row == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_mid_from_row","Bad args", NULL);
		AMP_DEBUG_EXIT("db_fetch_mid_from_row","-->NULL", NULL);
		return NULL;
	}

	/* Step 1: Build parametrs from row. */
	uint32_t nn_idx   = (row[1] == NULL) ? 0 : atoi(row[1]);
	uint32_t oid_idx  = (row[2] == NULL) ? 0 : atoi(row[2]);
	uint32_t parm_idx = (row[3] == NULL) ? 0 : atoi(row[3]);
	uint8_t type      = (row[4] == NULL) ? 0 : atoi(row[4]);
	uint8_t cat       = (row[5] == NULL) ? 0 : atoi(row[5]);
	uint8_t issFlag   = (row[6] == NULL) ? 0 : atoi(row[6]);
	uint8_t tagFlag   = (row[7] == NULL) ? 0 : atoi(row[7]);
	uint8_t oidType   = (row[8] == NULL) ? 0 : atoi(row[8]);
	uvast issuer      = (uvast) (row[9] == NULL) ? 0 : atoll(row[9]);
	uvast tag         = (uvast) (row[10] == NULL) ? 0 : atoll(row[10]);
	uint32_t dtype    = (row[11] == NULL) ? 0 : atoi(row[11]);
 	uint32_t mid_type = 0;

	/* Step 2: Create the OID. */
	oid = db_fetch_oid(nn_idx, parm_idx, oid_idx);
	if(oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("db_fetch_mid_from_row","Cannot fetch the oid: %d", oid_idx);
		oid_release(&oid);
		AMP_DEBUG_EXIT("db_fetch_mid_from_row","-->NULL", NULL);
		return NULL;
	}

	oid.type = oidType;

        switch(cat)
        {
          case 0: mid_type = MID_ATOMIC; break;
          case 1: mid_type = MID_COMPUTED; break;
          case 2: mid_type = MID_REPORT; break;
          case 3: mid_type = MID_CONTROL; break;
	  case 4: mid_type = MID_SRL; break;
          case 5: mid_type = MID_TRL; break;
          case 6: mid_type = MID_MACRO; break;
          case 7: mid_type = MID_LITERAL; break;
          case 8: mid_type = MID_OPERATOR; break;
          default: mid_type = MID_ANY;
        }
          
	if ((result = mid_construct(mid_type,
			                    issFlag ? &issuer : NULL,
					            tagFlag ? &tag : NULL,
				                oid)) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_mid_from_row", "Cannot construct MID", NULL);
		oid_release(&oid);
		AMP_DEBUG_EXIT("db_fetch_mid_from_row","-->NULL", NULL);
		return NULL;
	}

	oid_release(&oid);

	if (mid_sanity_check(result) == 0)
	{
		char *data = mid_pretty_print(result);
		AMP_DEBUG_ERR("db_fetch_mid_from_row", "Failed MID sanity check. %s", data);
		SRELEASE(data);
		mid_release(result);
		result = NULL;
	}

	AMP_DEBUG_EXIT("db_fetch_mid_from_row","-->"ADDR_FIELDSPEC, (uaddr)result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_mid_idx
 *
 * \par Gets a MID and returns the index of the MID
 *
 * \retval 0 Failure
 *        !0 The index of the MID
 *
 * \param[in] mid    - the MID whose index is being queried
 *
 * Note: There is probably a much better way to do this.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *  08/24/15  E. Birrane     Update to latest schema
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_fetch_mid_idx(mid_t *mid)
{
	int32_t result = 0;
	int32_t cur_idx = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_mid_idx","("ADDR_FIELDSPEC")", (uaddr)mid);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_mid_idx","Bad args",NULL);
		AMP_DEBUG_EXIT("db_fetch_mid_idx","-->NULL", NULL);
		return AMP_FAIL;
	}

	if(db_mgt_query_fetch(&res,
			            "SELECT * FROM dbtMIDs WHERE "
						"Type=%d AND Category=%d AND IssuerFlag=%d AND TagFlag=%d "
						"AND OIDType=%d AND IssuerID="UVAST_FIELDSPEC" "
						"AND TagValue="UVAST_FIELDSPEC,
						0,
						MID_GET_FLAG_ID(mid->flags),
						(MID_GET_FLAG_ISS(mid->flags)) ? 1 : 0,
						(MID_GET_FLAG_TAG(mid->flags)) ? 1 : 0,
						MID_GET_FLAG_OID(mid->flags),
						mid->issuer,
						mid->tag) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_mid_col","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_mid_col","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: For each matching MID, check other items... */
	result = AMP_FAIL;

	while ((row = mysql_fetch_row(res)) != NULL)
	{
		cur_idx = (row[0] == NULL) ? 0 : atoi(row[0]);

		int32_t nn_idx   = (row[1] == NULL) ? 0 : atoi(row[1]);
		int32_t oid_idx  = (row[2] == NULL) ? 0 : atoi(row[2]);
		int32_t parm_idx = (row[3] == NULL) ? 0 : atoi(row[3]);
		oid_t   oid      = db_fetch_oid(nn_idx, parm_idx, oid_idx);

		if((oid.type != OID_TYPE_UNK) &&
	       (oid_compare(oid, mid->oid, 1) == 0))
		{
			oid_release(&oid);
			result = cur_idx;
			break;
		}

		oid_release(&oid);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	/* Step 4: Return the IDX. */
	AMP_DEBUG_EXIT("db_fetch_mid_idx", "-->%d", result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_nn
 *
 * \par Gets the nickname UID given a primary key index into the Nickname table.
 *
 * \retval -1 system error
 *          0 non-fatal error
 *         >0 The nickname UID.
 *
 * \param[in] idx  - Index of the nickname UID being queried.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_fetch_nn(uint32_t idx)
{
	int32_t result = AMP_FAIL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_nn","(%d)", idx);

	/* Step 0: Sanity checks. */
	if(idx == 0)
	{
		AMP_DEBUG_ERR("db_fetch_nn","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_fetch_nn","-->AMP_FAIL", NULL);
		return AMP_FAIL;
	}

	/* Step 1: Grab the NN row */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtADMNicknames WHERE ID=%d",
						   idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_nn","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_nn","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[2]);
	}
	else
	{
		AMP_DEBUG_ERR("db_fetch_nn", "Did not find NN with ID of %d\n", idx);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_nn","-->%d", result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_nn_idx
 *
 * \par Gets the index of a nickname UID.
 *
 * \retval -1 system error
 *          0 non-fatal error
 *         >0 The nickname index.
 *
 * \param[in] nn  - The nickname UID whose index is being queried.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_fetch_nn_idx(uint32_t nn)
{
	int32_t result = AMP_FAIL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_nn_idx","(%d)", nn);


	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtADMNicknames WHERE Nickname_UID=%d",
						  nn) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_nn_idx","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_nn_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_nn_idx","-->%d", result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_oid_val
 *
 * \par Gets OID encoded value of an OID from the database.
 *
 * \retval NULL Failure
 *        !NULL The encoded value as a series of bytes.
 *
 * \param[in]  idx  - Index of the OID being queried.
 * \param[out] size - Size of the returned encoded value.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

uint8_t* db_fetch_oid_val(uint32_t idx, uint32_t *size)
{
	uint8_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_oid_val","(%d,"ADDR_FIELDSPEC")",
			          idx, (uaddr)size);

	/* Step 0: Sanity check. */
	if((idx == 0) || (size == NULL))
	{
		AMP_DEBUG_ERR("db_fetch_oid_val","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_fetch_oid_val","-->NULL", NULL);
		return NULL;
	}


	if(db_mgt_query_fetch(&res,
			              "SELECT Encoded FROM dbtOIDs WHERE ID=%d",
						  idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_oid_val","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_oid_val","-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = utils_string_to_hex(row[0],size);
	}
	else
	{
		AMP_DEBUG_ERR("db_fetch_oid_val", "Did not find OID with ID of %d\n", idx);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_oid_val","-->%d", result);
	return result;
}



/*****************************************************************************
 *
 * \par Function Name: db_fetch_oid
 *
 * \par Grabs an OID from the database, querying from the nickname and
 *      parameter tables as well to create a full OID structure.
 *
 * \retval NULL OID could not be fetched
 *        !NULL The OID
 *
 * \param[in]  nn_idx   - The index for the OID nickname, or 0.
 * \param[in]  parm_idx - The index for the OIDParms, or 0.
 * \param[out] oid_idx  - The index for the OID value.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  S. Jacobs      Initial implementation,
 *  08/24/15  E. Birrane     Updated to new schema.
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 ******************************************************************************/

oid_t db_fetch_oid(uint32_t nn_idx, uint32_t parm_idx, uint32_t oid_idx)
{
	oid_t result;
	tdc_t parms;
	uint32_t nn_id = 0;
	uint32_t val_size = 0;
	uint8_t *val = NULL;
	uint32_t oid_type = OID_TYPE_FULL;

	AMP_DEBUG_ENTRY("db_fetch_oid","(%d, %d, %d)",
					  nn_idx, parm_idx, oid_idx);

	oid_init(&result);
	tdc_init(&parms);

	/* Step 0: Sanity Check. */
	if(oid_idx == 0)
	{
		AMP_DEBUG_ERR("db_fetch_oid","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_fetch_oid","-->OID_TYPE_UNK", NULL);
		return result;
	}

	/* Step 1: Grab the OID value string. */
	if((val = db_fetch_oid_val(oid_idx, &val_size)) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_oid","Can't get OID for idx %d.", oid_idx);
		AMP_DEBUG_EXIT("db_fetch_oid","-->OID_TYPE_UNK", NULL);
		return result;
	}

	/* Step 2: Grab parameters, if the OID has them. */
	if(parm_idx > 0)
	{
		parms = db_fetch_tdc(parm_idx);

		if(nn_idx == 0)
		{
			oid_type = OID_TYPE_PARAM;
		}
	}

	/* Step 3: Grab the nickname, if the OID has one. */
	if(nn_idx > 0)
	{
		nn_id = db_fetch_nn(nn_idx);

		oid_type = (parm_idx == 0) ? OID_TYPE_COMP_FULL : OID_TYPE_COMP_PARAM;
	}

	/*
	 * Step 4: Construct the OID. This deep-copies parameters so we can
	 *          release the parms and value afterwards.
	 */
	result = oid_construct(oid_type, &parms, nn_id, val, val_size);

	SRELEASE(val);

	tdc_clear(&parms);

	AMP_DEBUG_EXIT("db_fetch_oid","-->%d", result.type);
	return result;
}



/*****************************************************************************
 *
 * \par Function Name: db_fetch_oid_idx
 *
 * \par Retrieves the index of an OID value in the OID table.
 *
 * \retval 0 The OID is not found in the DB.
 *        !0 The index of the OID value.
 *
 * \param[in]  oid   - The OID whose index is being queried.
 *
 * Note: This function only matches on the OID value and ignores
 *       nicknames and parameters.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to newest AMP implementation
 ******************************************************************************/

int32_t db_fetch_oid_idx(oid_t oid)
{
	int32_t result = AMP_FAIL;
	char *oid_str = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_oid_idx","(%d)", oid.type);

	/* Step 0: Sanity checks. */
	if(oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("db_fetch_oid_idx","Bad Args.", NULL);
		AMP_DEBUG_EXIT("db_fetch_oid_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: Build string version of OID for searching. */
	if((oid_str = oid_to_string(oid)) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_oid_idx","Can't get string rep of OID.", NULL);
		AMP_DEBUG_EXIT("db_fetch_oid_idx","-->%d",AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: Grab the OID row. */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtOIDs WHERE Encoded='%s'",
						  oid_str) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_oid_idx","Can't fetch", NULL);
		SRELEASE(oid_str);
		AMP_DEBUG_EXIT("db_fetch_oid_idx","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 3: Grab the row idx. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}

	mysql_free_result(res);
	SRELEASE(oid_str);

	AMP_DEBUG_EXIT("db_fetch_oid_idx","-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_parms
 *
 * \par Retrieves the set of parameters associated with a parameter index.
 *
 * Tables Effected:
 *
 * \return NULL Failure
 *        !NULL The parameters.
 *
 * \param[in]  idx  - The index of the parameters
 *
 * \par Notes:
 *		- If there are no parameters, but no error, then a Lyst with no entries
 *		  is returned.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial Implementation
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

Lyst db_fetch_parms(uint32_t idx)
{
	Lyst result = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	uint32_t dc_idx = 0;
	blob_t* entry = NULL;

	AMP_DEBUG_ENTRY("db_fetch_parms", "(%d)", idx);

	/* Step 0: Sanity check arguments. */
	if(idx == 0)
	{
		AMP_DEBUG_ERR("db_fetch_parms","Bad args",NULL);
		AMP_DEBUG_EXIT("db_fetch_parms","-->NULL",NULL);
		return NULL;
	}

	/* Step 1: Grab the OID row. */
	if(db_mgt_query_fetch(&res,
			              "SELECT DataCollectionID FROM dbtMIDParameter "
			              "WHERE CollectionID=%d ORDER BY ItemOrder",
			              idx) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_parms","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_parms","-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Allocate the return lyst. */
	if((result = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_parms","Can't allocate lyst",NULL);
		mysql_free_result(res);
		AMP_DEBUG_EXIT("db_fetch_parms","-->NULL",NULL);
		return NULL;
	}

	/* Step 3: For each matching parameter... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		amp_type_e type;

		if((entry = db_fetch_tdc_entry_from_row(row, &type)) == NULL)
		{
			AMP_DEBUG_ERR("db_fetch_parms", "Can't get entry.", NULL);
			dc_destroy(&result);
			mysql_free_result(res);

			AMP_DEBUG_EXIT("db_fetch_parms","-->NULL",NULL);
			return NULL;
		}

		lyst_insert_last(result, entry);
	}

	/* Step 4: Free results. */
	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_parms", "-->"ADDR_FIELDSPEC, (uaddr)result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_protomid_idx
 *
 * \par Gets a MID and returns the index of the matching proto mid.
 *
 * \retval 0 Failure finding index.
 *        !0 The index of the MID
 *
 * \param[in] mid    - the MID whose proto index is being queried
 *
 * Note: There is probably a much better way to do this.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/28/15  E. Birrane     Initial implementation,
 *  01/25/17  E. Birrane     Update to AMP 3.5.0 (JHU/APL)
 *****************************************************************************/

int32_t db_fetch_protomid_idx(mid_t *mid)
{
	int32_t result = AMP_FAIL;
	int32_t cur_idx = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	AMP_DEBUG_ENTRY("db_fetch_protomid_idx","("ADDR_FIELDSPEC")", (uaddr)mid);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		AMP_DEBUG_ERR("db_fetch_protomid_idx","Bad args",NULL);
		AMP_DEBUG_EXIT("db_fetch_parms", "-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 1: Grab the OID row. */
	if(db_mgt_query_fetch(&res,
			              "SELECT * FROM dbtProtoMIDs WHERE "
						  "Type=%d AND Category=%d AND OIDType=%d",
						  0,
						  MID_GET_FLAG_ID(mid->flags),
						  MID_GET_FLAG_OID(mid->flags)) != AMP_OK)
	{
		AMP_DEBUG_ERR("db_fetch_parms","Can't fetch", NULL);
		AMP_DEBUG_EXIT("db_fetch_parms","-->%d", AMP_FAIL);
		return AMP_FAIL;
	}

	/* Step 2: For each matching MID, check other items... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{

		cur_idx = atoi(row[0]);

		int32_t nn_idx = atoi(row[1]);
		int32_t oid_idx = atoi(row[2]);
		oid_t oid = db_fetch_oid(nn_idx, 0, oid_idx);

		if(oid_compare(oid, mid->oid, 0) == 0)
		{
			oid_release(&oid);
			result = cur_idx;
			break;
		}

		oid_release(&oid);
	}

	mysql_free_result(res);

	AMP_DEBUG_EXIT("db_fetch_protomid_idx", "-->%d", result);
	return result;
}






#endif

#endif

//#endif // HAVE_MYSQL
