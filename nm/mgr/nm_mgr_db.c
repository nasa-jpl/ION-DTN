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
 ** \file nm_mgr_db.c
 **
 ** File Name: nm_mgr_db.c
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

#include <stdio.h>
#include <mysql.h>
#include <string.h>

#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "shared/adm/adm.h"
#include "shared/utils/db.h"
#include "shared/utils/utils.h"
#include "shared/primitives/admin.h"
#include "shared/primitives/rules.h"
#include "shared/msg/pdu.h"
#include "shared/primitives/report.h"
#include "shared/utils/ion_if.h"

#include "nm_mgr.h"
#include "nm_mgr_db.h"

MYSQL *gConn; //The gConnection to the server   /*Needs to be global*/
extern int gNumAduData;

/******************************************************************************
 *
 * \par Function Name: db_init
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
 *****************************************************************************/
int db_init(char *server, char *user, char *pwd, char *database) {
    /* char *server = "localhost";
	 char *user = "root";//user must be the root
	 char *password = "NetworkManagement";
	 char *database = "dtnmp";*/

	gConn = mysql_init(NULL);

	DTNMP_DEBUG_ENTRY("db_init", "(%s,%s,%s,%s)", server, user, pwd, database);

	if (!mysql_real_connect(gConn, server, user, pwd, database, 0, NULL, 0)) {
		DTNMP_DEBUG_ERR("db_init", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_init", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_INFO("db_init", "gConnected to Database.", NULL);

	db_clear(); // TODO: This should not necessarily be done on every run.  Perhaps add a runtime switch. VRR 2013-08-09

	/* Step 2: Make sure the DB knowns about the MIDs we need. */
    db_verify_mids();

	DTNMP_DEBUG_EXIT("db_init", "-->1", NULL);
	return 1;
}

/******************************************************************************
 *
 * \par Function Name: db_clear
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
 *****************************************************************************/
int db_clear() {

	DTNMP_DEBUG_ENTRY("db_clear", "()", NULL);

	if ((mysql_query(gConn, "TRUNCATE TABLE dbtMIDs"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtIncomingMessages"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtIncoming"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtMessagesDefinitions"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtMIDDetails"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtMIDCollections"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtMIDCollection"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtMessagesControls"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtOutgoingRecipients"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtRegisteredAgents"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtMIDParameterizedOIDs"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtDataCollections"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtDataCollection"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtOutgoing"))
			|| (mysql_query(gConn, "TRUNCATE TABLE dbtOutgoingMessages"))) {
		DTNMP_DEBUG_ERR("db_clear", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_clear", "--> 0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_clear", "--> 1", NULL);
	return 1;
}

/******************************************************************************
 *
 * \par Function Name: db_close
 *
 * \par Close the database gConnection.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
void db_close() {
	mysql_close(gConn);
}

// Make sure DB has defined all of the atomic MIDs we know about.
/******************************************************************************
 *
 * \par Function Name: db_verify_mids
 *
 * \par Adds MIDS to the DB, if necessary, to make sure that dbtMIDs and
 *      dbtMIDDetails contain all MIDs known to this manager.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *  08/01/13  S. Jacobs	      Reflect Changes in MIDs
 *****************************************************************************/
void db_verify_mids() {
	int i = 0;
	int size = 0;
	char *oid_str = NULL;
	LystElt elt;
	adm_datadef_t *admData = NULL;

	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		admData = (adm_datadef_t *) lyst_data(elt);
		mid_t *mid = admData->mid;
		int attr = 1;
		uint8_t flags = mid->flags;
		uvast issuer = mid->issuer;
		oid_t *oid = mid->oid;

		char *oid_str = utils_hex_to_string(oid->value, oid->value_size);
		uvast tag = mid->tag;

		if(oid_str != NULL)
		{
			uint32_t idx = db_add_mid(attr,flags,issuer,&(oid_str[2]),tag,0,0,0,0);
			MRELEASE(oid_str);
		}
		else
		{
			DTNMP_DEBUG_ERR("db_verify_mids", "%s", oid_str);
			MRELEASE(oid_str);
		}
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name: db_add_agent()
 *
 * \par Adds a Registered Agent to the database
 *
 * \retval 0 Failure
 *        !0 Success
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
uint32_t db_add_agent(eid_t agent_eid) {
	char query[1024];

	/* Step 1: Create Query */
	sprintf(query, "INSERT INTO dbtRegisteredAgents(AgentIdSDNV) "
			"VALUES(%s)", agent_eid.name);

	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_add_agent", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_agent", "-->NULL", NULL);
		return 1;
	}

	DTNMP_DEBUG_ERR("db_add_agent", "-->", NULL);
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: db_add_mid
 *
 * \par Creates a MID in the database.
 *
 * \retval 0 Failure
 *        !0 The index of the inserted MID from the dbtMIDs table.
 *
 * \param[in] attr   - DB attribute of the new MID.
 * \param[in] flag   - The flag byte of the new MID.
 * \param[in] issuer - The issuer, or NULL if no issuer.
 * \param[in] OID    - The serialized OID for the MID.
 * \param[in] tag    - The tag, or NULL if no tag.
 *
 * \par Notes: The format of the dbtMIDs table is as follows.
 *
 * +------------+---------------------+------+-----+---------+----------------+
 * | Field      | Type                | Null | Key | Default | Extra          |
 * +------------+---------------------+------+-----+---------+----------------+
 * | ID         | int(10) unsigned    | NO   | PRI | NULL    | auto_increment |
 * | Attributes | int(10) unsigned    | NO   |     | 0       |                |
 * | Type       | bit(2)              | NO   |     | b'0'    |                |
 * | Category   | bit(2)              | NO   |     | b'0'    |                |
 * | IssuerFlag | bit(1)              | NO   |     | b'0'    |                |
 * | TagFlag    | bit(1)              | NO   |     | b'0'    |                |
 * | OIDType    | bit(2)              | NO   |     | b'0'    |                |
 * | IssuerID   | bigint(20) unsigned | NO   |     | 0       |                |
 * | OIDValue   | varchar(64)         | YES  |     | NULL    |                |
 * | TagValue   | bigint(20) unsigned | NO   |     | 0       |                |
 * +------------+---------------------+------+-----+---------+----------------+
 *
 *  We also update the dbtMIDDetails table.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
uint32_t db_add_mid(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag,
		char *mib, char *mib_iso, char *name, char *descr) {
	char query[1024];
	uint32_t result = 0;
	uint32_t idx = 0;
	char last_insert_id[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_add_mid", "(%d, %d, %ld, %s, %ld, %s, %s, %s, %s)",
			attr, flag, (unsigned long) issuer, OID, (unsigned long) tag, mib, mib_iso, name, descr);

	// TODO: Some sanity checking should still be done, but just on OID it seems.  Remove the other null checks. VRR 2013-08-09
	/* Step 0: Sanity check arguments. */
	/*if(         OID == NULL     ||
			    mib == NULL     ||
			    mib_iso == NULL ||
			    name == NULL    ||
			    descr == NULL    )
	{
		DTNMP_DEBUG_ERR("db_add_mid","Can't add mid, bad args",NULL);
		DTNMP_DEBUG_ERR("db_add_mid","OID: %s,  mib: %s, mib_iso: %s, name: %s, descr: %s", OID,mib,mib_iso,name,descr );
		return 0;
	}*/

	/* Make sure the ID is not already in the DB. */
	if ((idx = db_fetch_mid_idx(attr, flag, issuer, OID, tag)) > 0) {
		DTNMP_DEBUG_WARN("db_add_mid", "Can't add duplication MID (idx is %d).",
				idx);
		DTNMP_DEBUG_EXIT("db_add_mid", "-->0", NULL);
		return 0;
	}

	/* Step 1: Build insert query. */
	sprintf(query,
			"INSERT INTO dbtMIDs \
(Attributes,Type,Category,IssuerFlag,TagFlag,OIDType,IssuerID,OIDValue,TagValue) \
VALUES (%d,b'%d',b'%d',b'%d',b'%d',%d,%lu,'%s',%lu)",
			attr, MID_GET_FLAG_TYPE(flag), MID_GET_FLAG_CAT(flag),
			(MID_GET_FLAG_ISS(flag)) ? 1 : 0, (MID_GET_FLAG_TAG(flag)) ? 1 : 0,
			MID_GET_FLAG_OID(flag), (uint64_t) issuer,
			(OID == NULL) ? "0" : OID, (uint64_t) tag);

	/* Step 2: Execute query. */
	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_add_mid", "Database Error: %s", mysql_error(gConn));
		result = 0;
	} else {
		/* Step 3: Update MID details. */
		result = (uint32_t) mysql_insert_id(gConn); // TODO: Check for bad return values (0 or false). VRR 2013-08-09
		sprintf(query,
				"INSERT INTO dbtMIDDetails \
(MIDID,MIBName,MIBISO,Name,Description) VALUES \
(%d,'%s','%s','%s','%s')",
				result, (mib == NULL) ? "" : mib,
				(mib_iso == NULL) ? "" : mib_iso, (name == NULL) ? "" : name,
				(descr == NULL) ? "" : descr);

		if (mysql_query(gConn, query)) {
			DTNMP_DEBUG_ERR("db_add_mid", "Database Error: %s",
					mysql_error(gConn));
			result = 0;
			/* Step 3.1 remove the dbtMIDs entry if there is a problem */
			/* Step 3.1.1 find the last inserted id into dbtMIDs*/
			sprintf(query, "SELECT LAST_INSERT_ID() FROM dbtMIDs");
			if (mysql_query(gConn, query)) {
				DTNMP_DEBUG_ERR("db_delete_bad_mid", "Database Error: %s",
						mysql_error(gConn));
			}

			res = mysql_store_result(gConn);
			if ((row = mysql_fetch_row(res)) != NULL) {
				strcpy(last_insert_id, row[0]);
			}
			/* Step 3.1.2 delete the last inserted id */
			sprintf(query, "DELETE FROM dbtMIDs WHERE ID=%s", last_insert_id);
			if (mysql_query(gConn, query)) {
				DTNMP_DEBUG_ERR("db_delete_bad_mid", "Database Error: %s",
						mysql_error(gConn));
			}

			/* Step 3.1.3 reset the auto increment */
			sprintf(query, "ALTER TABLE dbtMIDs AUTO_INCREMENT=%s",
					last_insert_id);
			if (mysql_query(gConn, query)) {
				DTNMP_DEBUG_ERR("db_delete_bad_mid", "Database Error: %s",
						mysql_error(gConn));
			}
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
		}
	}

	DTNMP_DEBUG_EXIT("db_add_mid", "-->%d", result);
	return result;
}

uint32_t db_add_mid_collection(Lyst collection, char *comment) {
	DTNMP_DEBUG_ERR("db_add_mid_collection", "NOT IMPLEMENTED YET", NULL);
	return 0;
}

uint32_t db_add_rpt_data(rpt_data_t *rpt_data) {
	DTNMP_DEBUG_ERR("db_add_rpt_data", "NOT IMPLEMENTED YET", NULL);
	return 0;
}

uint32_t db_add_rpt_defs(rpt_defs_t *defs) {
	DTNMP_DEBUG_ERR("db_add_rpt_defs", "NOT IMPLEMENTED YET", NULL);
	return 0;
}

uint32_t db_add_rpt_items(rpt_items_t *items) {
	DTNMP_DEBUG_ERR("db_add_rpt_items", "NOT IMPLEMENTED YET", NULL);
	return 0;
}

uint32_t db_add_rpt_sched(rpt_prod_t *sched) {
	DTNMP_DEBUG_ERR("db_add_rpt_sched", "NOT IMPLEMENTED YET", NULL);
	return 0;
}

/******************************************************************************
 *
 * \par Function Name: db_fetch_ctrl
 *
 * \par Creates a command structure from the database.
 *
 * \retval NULL Failure
 *        !NULL The built command structure.
 *
 * \param[in] id - The Primary Key of the desired command.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
ctrl_exec_t *db_fetch_ctrl(int id) {
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];
	ctrl_exec_t *result = NULL;

	/* Step 1: Build Query */
	sprintf(query, "SELECT * FROM dbtMessagesControls WHERE ID=%d", id);
	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_fetch_ctrl", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->NULL", NULL);
		return result;
	}

	/* Step 2: Parse the row and populate the structure. */
	res = mysql_store_result(gConn);
	if ((row = mysql_fetch_row(res)) != NULL) {
		/* Step 2.1: Grab the type and make sure it is correct. */
		int type = atoi(row[1]);

		if (type != EXEC_CTRL_MSG) {
			DTNMP_DEBUG_ERR("db_fetch_ctrl",
					"Bad row type. Expecting 3 and got %d\n", type);
		} else {
			time_t time = (time_t) atoll(row[2]);
			Lyst contents = db_fetch_mid_col(atoi(row[6]));

			if (contents == NULL) {
				DTNMP_DEBUG_ERR("db_fetch_ctrl", "Can't grab mid col.", NULL);
				result = NULL;
			} else {
				result = ctrl_create_exec(time, contents);
			}
		}
	} else {
		DTNMP_DEBUG_ERR("db_fetch_ctrl", "Unable to find ctrl with ID of %d\n",
				id);
		DTNMP_DEBUG_EXIT("db_fetch_crtl", "-->%ld", (unsigned long) result);
	}
	// TODO: Free the resultset 'res' if it is not NULL. VRR 2013-08-09
	DTNMP_DEBUG_ENTRY("db_fetch_cmd", "(%d)", id);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_fetch_def
 *
 * \par Creates a def structure from the database.
 *
 * \retval NULL Failure
 *        !NULL The built def structure.
 *
 * \param[in] id - The Primary Key of the desired def.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

def_gen_t *db_fetch_def(int id) {
	def_gen_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];
	/*
	 * We are going to assume that the type is always 1 for now
	 */

	/*Step 1: Query the data base for the def*/
	sprintf(query, "SELECT * FROM dbtMessagesDefinitions WHERE ID=%d", id);
	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_fetch_def", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_def", "-->NULL", NULL);
		return NULL;
	}

	/*Step 2: Parse results*/
	res = mysql_store_result(gConn);
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		int DefinitionID = atoi(row[2]);
		int DefinitionMC = atoi(row[3]);

		/*Step 3: Create definition*/
		mid_t *mid = db_fetch_mid(DefinitionID);

		if(mid == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_def","Cannot fetch mid",NULL);
			return NULL;
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
		}

		Lyst contents = db_fetch_mid_col(DefinitionMC);

		if(contents == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_def","Cannot fetch mid collection",NULL);
			return NULL;
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
		}

		result = def_create_gen(mid, contents);

	}
	else
	{
		DTNMP_DEBUG_ENTRY("db_fetch_def", "(%d)", id);
		return NULL;
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
	}

	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_def", "-->%ld", (unsigned long) result);

	return result;
}

/*****************************************************************************
 *
 * \par Function Name: db_fetch_parms_str
 *
 * \par Gets number of params and the params and turns them into a
 *  string, then serializes the string for the parameterized OID.
 *
 *  +----------+---------+--------+     +--------+
 *  | Base OID | # Parms | Parm 1 | ... | Parm N |
 *  |   [VAR]  |  [SDNV] |  [DC]  |     |  [DC]  |
 *  +----------+---------+--------+     +--------+
 *
 * \retval NULL the parameters could not be fetched
 *        !NULL the parameters where converted into a string
 *
 * \param[in] mid_id the id of the MID whose OID that is being fetched
 * \param[out] parm_size the size of the parms needed to be changed into strings
 * \param[out] num_parms the total number of parameters
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  E. Birrane	 Initial implementation,
 *  07/25/13  S. Jacobs      Initial implementation,
 *******************************************************************************/
uint8_t *db_fetch_parms_str(int mid_id, uint32_t *parm_size,
	uint32_t *num_parms) {
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];
	uint8_t *result = NULL;
	Lyst parms_datacol;

	*parm_size = 0;
	*num_parms = 0;

	/*Step 1: Query the data base for the Data Collection*/
	sprintf(query,
			"SELECT DataCollectionID FROM dbtMIDParameterizedOIDs WHERE MIDID=%d",
			mid_id);
	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_fetch_parms_str", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	/*Step 2: Grab the Data Collection holding the OID Parameters.*/
	res = mysql_store_result(gConn);

	if ((row = mysql_fetch_row(res)) != NULL) {
		int dc_idx = atoi(row[0]);
		parms_datacol = db_fetch_data_col(dc_idx);
		if (parms_datacol == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_parms_str","The lyst is empty",NULL);
			return NULL;
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","MID row was empty: %d",mid_id);
		return NULL;
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
	}

	mysql_free_result(res);

	/* Step 3: Grab number of parameters extracted */
	*num_parms = lyst_length(parms_datacol);

	if(num_parms == 0)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","lyst is empty",NULL);
		return NULL;
	}

	result = utils_datacol_serialize(parms_datacol, parm_size);
	if(result == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","Could not serialize the parms",NULL);
		utils_datacol_destroy(&parms_datacol);
		return NULL;
	}

	char *tmp = utils_hex_to_string(result, *parm_size);
	if(tmp == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","The serialized parms: %s (%d)",tmp, *parm_size);
		MRELEASE(tmp);
	}

	MRELEASE(tmp);
	utils_datacol_destroy(&parms_datacol);

	DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->%ld", (unsigned long) result);

	return result;
}

/*****************************************************************************
 *
 * \par Function Name: db_fetch_oid_parms
 *
 * \par Gets all the parameters for the parameterized OID
 *
 *  +----------+---------+--------+     +--------+
 *  | Base OID | # Parms | Parm 1 | ... | Parm N |
 *  |   [VAR]  |  [SDNV] |  [DC]  |     |  [DC]  |
 *  +----------+---------+--------+     +--------+
 *
 * \retval NULL the parameters could not be fetched
 *        !NULL the parameters where found
 *
 * \param[in] mid_id - The id of the MID whose OID that is being fetched
 * \param[in] oid_root - The root OID which will have the parameters appended to the end
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  E. Birrane	 Initial implementation,
 *  07/25/13  S. Jacobs      Initial implementation,
 *******************************************************************************/
oid_t *db_fetch_oid_parms(int mid_id, char* oid_root) {
	oid_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;
	uint8_t *parms = NULL;
	uint32_t parms_size = 0;
	uint32_t num_parms = 0;

	/* Step 1: grab the OID parameters String. */
	parms = db_fetch_parms_str(mid_id, &parms_size, &num_parms);

	if (parms == NULL) {
		DTNMP_DEBUG_ERR("db_fetch_oid_parms", "Can't fetch parms string", NULL);
	//	MRELEASE(parms);
		return NULL;
	}

	/* Step 3: Allocate data to hold everything. */
	size = strlen(oid_root) + parms_size;
	if ((data = (uint8_t *) MTAKE(size)) == NULL) {
		DTNMP_DEBUG_ERR("db_fetch_oid_parms", "Can't allocate data of size %d", size);
		MRELEASE(parms);
		return NULL;
	}

	/* Step 4: Copy everything into data. */
	uint8_t *cursor = data;

	uint32_t root_size = 0;
	uint8_t *root_data = utils_string_to_hex((unsigned char *) oid_root, &root_size);
	memcpy(cursor, root_data, root_size);
	cursor += root_size;
	MRELEASE(root_data);

	memcpy(cursor, parms, parms_size);
	cursor += parms_size;

	/* Step 5: Build OID */
	result = oid_deserialize_param(data, size, &bytes);
	if(result == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_parms","Cannot deserialize param",NULL);
		MRELEASE(data);
		MRELEASE(parms);
		return NULL;
	}

	/* Step 6: Release stuff. */
	MRELEASE(data);
	MRELEASE(parms);

	return result;
}

/*****************************************************************************
 *
 * \par Function Name: db_fetch_oid_full
 *
 * \par Gets the full OID
 *
 * \retval NULL OID could not be fetched
 *        !NULL OID found
 *
 * \param[in] mid_id - The id of the MID whose OID that is being fetched
 * \param[in] oid_root - The root OID
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  E. Birrane	 Initial implementation,
 *  07/25/13  S. Jacobs      Initial implementation,
 *******************************************************************************/
oid_t *db_fetch_oid_full(int mid_id, char *oid_root) {
	oid_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;

	/* For FULL OID, root *is* the OID. */
	data = utils_string_to_hex((unsigned char *) oid_root, &size);

	if(data == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_full","cannot convert to string",NULL);
		return NULL;
	}

	result = oid_deserialize_full(data, size, &bytes);

	if(result == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_full","Cannot deserialize oid",NULL);
		MRELEASE(data);
		return NULL;
	}

	MRELEASE(data);
	return result;

}


/*****************************************************************************
 *
 * \par Function Name: db_fetch_oid
 *
 * \par Determines which type of OID to go fetch
 *
 * \retval NULL OID could not be fetched
 *        !NULL OID found
 *
 * \param[in] mid_id - The id of the MID whose OID that is being fetched
 * \param[in] oid_type - The type of OID; full,parameterized, compressed full, compressed
 * 						 parameterized
 * \param[out] oid_root - The root OID with any other necessary attachments
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  E. Birrane	 Initial implementation,
 *  07/25/13  S. Jacobs      Initial implementation,
 *******************************************************************************/
oid_t *db_fetch_oid(int mid_id, int oid_type, char *oid_root) {
	oid_t *result = NULL;

	switch (oid_type) {
	case OID_TYPE_FULL:
		return db_fetch_oid_full(mid_id, oid_root);
		break;

	case OID_TYPE_PARAM:
		return db_fetch_oid_parms(mid_id, oid_root);
		break;

//		case OID_TYPE_COMP_FULL:
//			oid_root = oid_deserialize_comp(data, size, &bytes);
//			break;

//		case OID_TYPE_COMP_PARAM:
//			oid_root = oid_deserialize_comp_param(data, size, &bytes);
//			break;
	default:
		DTNMP_DEBUG_ERR("db_fetch_mid", "Unknown OID Type %d", oid_type);
		break;
	}

	return result;
}

/*******************************************************************************
 * \par Function Name: db_fetch_data_col_entry
 *
 * \par Fetches the appropriate data collection entry
 *
 * \retval NULL The entry could not be retrieved
 *         !NULL a datacol_entry_t was created
 *
 * \param[in] id - The row of the entry
 * \param[in] order - The order of the entry within a collection such as 1, 2, 3...
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/24/13  E. Birrane	 Initial implementation,
 *  07/24/13  S. Jacobs      Initial implementation,
 *
 **********************************************************************************/

datacol_entry_t *db_fetch_data_col_entry(int id, int order) {
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	datacol_entry_t *result = NULL;

	sprintf(query,
			"SELECT * FROM dbtDataCollection WHERE CollectionID=%d AND DataOrder=%d",
			id, order);
	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetchdata_col_entry", "-->NULL", NULL);
		return NULL;
	}
	else
	{
		res = mysql_store_result(gConn);
		if ((row = mysql_fetch_row(res)) != NULL) {
			result = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t));
			result->length = atoi(row[1]);
			result->value = (uint8_t*) MTAKE(result->length);
			memcpy(result->value, row[2], result->length);
		}
		else
		{
			DTNMP_DEBUG_ERR("db_fetch_data_col_entry", "No rows", NULL)
		}
	 }
	
	mysql_free_result(res);
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
 * \param[in] id - The Primary Key of the desired MID in the dbtMIDs table.
 *
 * \par Notes: The format of the dbtMIDs table is as follows.
 *
 * +------------+---------------------+------+-----+---------+----------------+
 * | Field      | Type                | Null | Key | Default | Extra          |
 * +------------+---------------------+------+-----+---------+----------------+
 * | ID         | int(10) unsigned    | NO   | PRI | NULL    | auto_increment |
 * | Attributes | int(10) unsigned    | NO   |     | 0       |                |
 * | Type       | bit(2)              | NO   |     | b'0'    |                |
 * | Category   | bit(2)              | NO   |     | b'0'    |                |
 * | IssuerFlag | bit(1)              | NO   |     | b'0'    |                |
 * | TagFlag    | bit(1)              | NO   |     | b'0'    |                |
 * | OIDType    | bit(2)              | NO   |     | b'0'    |                |
 * | IssuerID   | bigint(20) unsigned | NO   |     | 0       |                |
 * | OIDValue   | varchar(64)         | YES  |     | NULL    |                |
 * | TagValue   | bigint(20) unsigned | NO   |     | 0       |                |
 * +------------+---------------------+------+-----+---------+----------------+
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

mid_t *db_fetch_mid(int id)
{
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	mid_t *result = NULL;

	DTNMP_DEBUG_ENTRY("db_fetch_mid", "(%d)", id);

	/* Step 1: Build and execute the Query. */
	sprintf(query, "SELECT * FROM dbtMIDs WHERE ID=%d", id);

	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_fetch_mid", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Retrieve and parse the result. */
	res = mysql_store_result(gConn);

	if ((row = mysql_fetch_row(res)) != NULL) {
		uint8_t type = (uint8_t) row[2][0]; //atoi(row[2]);
		uint8_t cat = (uint8_t) row[3][0]; //atoi(row[3]);
		uint8_t issFlag = atoi(row[4]);

		uint8_t tagFlag = atoi(row[5]);
		uint8_t oidType = (uint8_t) row[6][0]; //atoi(row[6]);

		uvast issuer = (uvast) atoll(row[7]);
		oid_t *oid = db_fetch_oid(id, oidType, row[8]);
		if(oid == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_mid","Cannot fetch the oid: %s",row[8]);
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
			return NULL;
		}
		uvast tag = (uvast) atoll(row[9]);

		if ((result = mid_construct(type, cat, issFlag ? &issuer : NULL,
		tagFlag ? &tag : NULL,
		oid)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_mid", "Cannot construct MID", NULL);
			// TODO: Free the resultset 'res'.  VRR 2013-08-09
			return NULL;
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_mid", "Did not find MID with ID of %d\n", id);
	}

	mysql_free_result(res);

	if (mid_sanity_check(result) == 0)
	{
		char *data = mid_pretty_print(result);
		DTNMP_DEBUG_ERR("db_fetch_mid", "Failed MID sanity check. %s", data);
		MRELEASE(data);
		mid_release(result);
		result = NULL;
	}

	DTNMP_DEBUG_EXIT("db_fetch_mid", "-->%ld", (unsigned long) result);
	return result;
}
//TODO: CODE REVIEW 2013-08-09 ENDED HERE. VRR
/******************************************************************************
 *
 * \par Function Name: db_fetch_mid_col
 *
 * \par Creates a MID collection from a row in the dbtMIDCollection database.
 *
 * \retval NULL Failure
 *        !NULL The built MID collection.
 *
 * \param[in] id - The Primary Key in the dbtMIDCollection table.
 *
 * \par Notes: The format of the dbtMIDCollection table is as follows.
 * +--------------+------------------+------+-----+---------+-------+
 * | Field        | Type             | Null | Key | Default | Extra |
 * +--------------+------------------+------+-----+---------+-------+
 * | CollectionID | int(10) unsigned | NO   | PRI | 0       |       |
 * | MIDID        | int(10) unsigned | NO   | PRI | 0       |       |
 * | MIDOrder     | int(10) unsigned | NO   | PRI | 0       |       |
 * +--------------+------------------+------+-----+---------+-------+
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
Lyst db_fetch_mid_col(int id)
{
	Lyst result = lyst_create();
	mid_t *new_mid = NULL;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	/* Step 1: Construct and execute the query. */
	sprintf(query,
			"SELECT MIDID FROM dbtMIDCollection WHERE CollectionID=%d ORDER BY MIDOrder",
			id);

	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_fetch_mid_col", "SQL Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_col", "-->%d", (unsigned long) result);
		return result;
	}

	/* Step 2: Grab result and walk through each row. */
	res = mysql_store_result(gConn);
	while ((row = mysql_fetch_row(res)) != NULL) {

		/* Step 2.1: For each row, build a MID and add it to the collection. */
		new_mid = db_fetch_mid(atoi(row[0]));
		if (new_mid == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_mid_col", "Can't grab MID with ID %d.",
					atoi(row[0]));
			return NULL;
		}
		else
		{
			lyst_insert_last(result, new_mid);
		}
	}

	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_mid_col", "-->%d", (unsigned long) result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_fetch_data_col
 *
 * \par Creates a data collection from dbtDataCollections in the database
 *
 * \retval 0 Failure
 *        !0 The built Data collection.
 *
 * \param[in] id - The Primary Key in the dbtDataCollection table.
 *
 * \par Notes: The format of the dbtDataCollection table is as follows.
 *
 * +--------------+------------------+------+-----+---------+-------+
 * | Field        | Type             | Null | Key | Default | Extra |
 * +--------------+------------------+------+-----+---------+-------+
 * | CollectionID | int(10) unsigned | NO   | PRI | 0       |       |
 * | DataLength   | int(10) unsigned | NO   |     | 0       |       |
 * | DataBlob     | mediumblob       | YES  |     | NULL    |       |
 * | DataOrder    | int(11)          | NO   | PRI | 0       |       |
 * +--------------+------------------+------+-----+---------+-------+
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
Lyst db_fetch_data_col(int dc_id) {
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	Lyst result = lyst_create();
	datacol_entry_t *dc_entry;

	/* Step 1: Construct the Query */
	sprintf(query, "SELECT * FROM dbtDataCollection WHERE CollectionID=%d",
			dc_id);

	if (mysql_query(gConn, query))
	{
		lyst_destroy(result);
		DTNMP_DEBUG_ERR("db_fetch_data_col", "SQL Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_data_col", "-->%d", (unsigned long) result);
		return result;
	}
	else
	{
		/* Step 2: Get Collection ID*/
		res = mysql_store_result(gConn);
		while ((row = mysql_fetch_row(res)) != NULL) {
			datacol_entry_t *cur_entry = NULL;

			cur_entry = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t));
			cur_entry->length = atoi(row[1]);
			cur_entry->value = (uint8_t*) MTAKE(cur_entry->length);

			if((cur_entry->length == 0) || (cur_entry->value == 0))
			{
				DTNMP_DEBUG_ERR("db_fetch_data_col","cur_entry->length : %lu",cur_entry->length);
				DTNMP_DEBUG_ERR("db_fetch_data_col","cur_entry->value : %lu",cur_entry->value);
				lyst_destroy(result);
				return NULL;
			}
			else
			{
				memcpy(cur_entry->value, row[2], cur_entry->length);

				lyst_insert_last(result, cur_entry);
			}
		}
	}
	mysql_free_result(res);

	return result;
}

/******************************************************************************
 * \par Function Name: db_fetch_mid_idx
 *
 * \par Gets a MID and returns the index of the MID
 *
 *\retval 0 Failure
 *        !0 The index of the MID
 *
 * \param[in] attr - the attribute of the MID
 * \param[in] flag - the type and cat of the MID
 * \param[in] issuer
 * \param[in] OID
 * \param[in] tag
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *
 *****************************************************************************/
int db_fetch_mid_idx(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag) {
	char query[1024];
	int result = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	/* Step 0: Sanity check arguments. */
	if(OID == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx","Can't fetch mid idx, bad args",NULL);
		return 0;
	}

	/* Step 1: Build and execute query. */
	sprintf(query,
			"SELECT * FROM dbtMIDs WHERE \
Attributes=%d AND Type=%d AND Category=%d AND IssuerFlag=%d \
AND TagFlag=%d AND OIDType=%d AND IssuerID=%lu AND OIDValue='%s' \
AND TagValue=%lu",
			attr, MID_GET_FLAG_TYPE(flag), MID_GET_FLAG_CAT(flag),
			(MID_GET_FLAG_ISS(flag)) ? 1 : 0, (MID_GET_FLAG_TAG(flag)) ? 1 : 0,
			MID_GET_FLAG_OID(flag), (uint64_t) issuer,
			(OID == NULL) ? "0" : OID, (uint64_t) tag);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->%d", result);
		return result;
	}

	/* Step 2: If we found a row, remember the IDX. */
	res = mysql_store_result(gConn);

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}
	else
	{
		DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->%d", result);
	}

	mysql_free_result(res);

	/* Step 3: Return the IDX. */
	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_fetch_pred_rule
 *
 * \par Creates a pred rule structure from the database.
 *
 * \retval NULL Failure
 *        !NULL The built pred rule structure.
 *
 * \param[in] id - The Primary Key of the desired pred rule.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

rule_pred_prod_t *db_fetch_pred_rule(int id) {
	rule_pred_prod_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_pred_rule", "(%d)", id);

	DTNMP_DEBUG_ERR("db_fetch_pred_rule", "NOT IMPLEMENTED YET!", NULL);

	DTNMP_DEBUG_EXIT("db_fetch_pred_rule", "-->%ld", (unsigned long) result);

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
 *****************************************************************************/

adm_reg_agent_t *db_fetch_reg_agent(int Agentid) {
	adm_reg_agent_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];

	/*Step 1: Build query */
	sprintf(query, "SELECT * FROM dbtRegisteredAgents WHERE ID=%d", Agentid);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ENTRY("db_fetch_reg_agent", "(%d)", Agentid);
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent", "-->%ld",
				(unsigned long) result);
		return NULL;
	}

	/*Step 2: Parse results*/
	res = mysql_store_result(gConn);

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		eid_t eid;
		strncpy(eid.name, row[1], MAX_EID_LEN);

		/* Step 3: Create structure for agent */
		result = msg_create_reg_agent(eid);
		if(result == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_reg_agent","Cannot create a registered agent",NULL);
			return NULL;
		}
	}
	else
	{
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent", "-->%ld", (unsigned long) result);
	}

	mysql_free_result(res);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_fetch_time_rule
 *
 * \par Creates a time rule from a row in the database.
 *
 * \retval NULL Failure
 *        !NULL The built time rule.
 *
 * \param[in] id - The Primary Key of the desired time rule.
 *
 * \par Notes: The format of the dbtMessagesControls table is as follows.
 *
 * +------------+---------------------+------+-----+---------+----------------+
 * | Field      | Type                | Null | Key | Default | Extra          |
 * +------------+---------------------+------+-----+---------+----------------+
 * | ID         | int(10) unsigned    | NO   | PRI | NULL    | auto_increment |
 * | Type       | tinyint(3) unsigned | NO   |     | 0       |                |
 * | Start      | bigint(20) unsigned | NO   |     | 0       |                |
 * | Periods    | bigint(20) unsigned | NO   |     | 1       |                |
 * | Predicate  | int(10) unsigned    | NO   |     | 0       |                |
 * | Count      | bigint(20) unsigned | NO   |     | 0       |                |
 * | Collection | int(10) unsigned    | NO   |     | 0       |                |
 * +------------+---------------------+------+-----+---------+----------------+
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
rule_time_prod_t *db_fetch_time_rule(int id) {
	rule_time_prod_t *result = NULL;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	/* Step 1: Build the query and execute it. */
	sprintf(query, "SELECT * FROM dbtMessagesControls WHERE ID=%d", id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_time_rule", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_time_rule", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Parse the row and populate the structure. */
	res = mysql_store_result(gConn);
	if ((row = mysql_fetch_row(res)) != NULL) {
		/* Step 2.1: Grab the type and make sure it is correct. */
		int type = atoi(row[1]);

		/* \todo: Make this an enum. */
		if (type != TIME_PROD_MSG)
		{
			DTNMP_DEBUG_ERR("db_fetch_time_rule",
					"Bad row type. Expecting 1 and got %d\n", type);
		}
		else
		{
			time_t time = (time_t) atoll(row[2]);
			uvast period = (uvast) atoll(row[3]);
			uvast count = (uvast) atoll(row[5]);
			Lyst contents = db_fetch_mid_col(atoi(row[6]));

			if(contents == NULL)
			{
				DTNMP_DEBUG_ERR("db_fetch_time_rule","Cannot fetch mid collection",NULL);
				return NULL;
			}

			result = rule_create_time_prod_entry(time, count, period, contents);

			if(result == NULL)
			{
				DTNMP_DEBUG_ERR("db_fetch_time_rule","Cannot create time_prod_entry",NULL);
				return NULL;
			}
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_time_rule",
				"Unable to find rule with ID of %d\n", id);
		DTNMP_DEBUG_EXIT("db_fetch_time_rule", "-->%ld",
				(unsigned long) result);
	}

	mysql_free_result(res);

	/* Step 3: Return the created structure. */
	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_insert_incoming_initialize
 *
 * \par Returns the id of the last insert into dbtIncoming.
 *
 * \retval 0 message was not inserted.
 *        !0 message was inserted into database.
 *
 * \param[in] timestamp - the generated timestamp
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
int db_insert_incoming_initialize(time_t timestamp)
{
	MYSQL_RES *res = NULL;
    MYSQL_ROW row;
	char query[1024];
	int result = 0;

	/* Step 1: insert message into dbtIncoming*/
	sprintf(query, "INSERT INTO dbtIncoming(ReceivedTS,GeneratedTS,State) "
		    			  "VALUES(NOW(),%lu,0)", (unsigned long) timestamp);
	if (mysql_query(gConn, query))
    {
		DTNMP_DEBUG_ERR("db_insert_incoming_group", "Database Error: %s",
		    	  		mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_insert_incoming_group", "-->%d", result);
		return 0;
    }

	/* Step 2: Get the id of the inserted message*/
	sprintf(query, "SELECT LAST_INSERT_ID() FROM dbtIncoming");
    if (mysql_query(gConn, query))
    {
		DTNMP_DEBUG_ERR("db_insert_incoming_group", "Database Error: %s",
		    	    	mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_insert_incoming_group", "-->%d", result);
		return 0;
	}

    /* Step 3: Store the result*/
    res = mysql_store_result(gConn);

    if ((row = mysql_fetch_row(res)) != NULL)
    {
    	result = atoi(row[0]);
    }
    else
    {
    	DTNMP_DEBUG_EXIT("db_insert_incoming_initialize", "-->%d", result);
    }

    /* Step 4: Update State to ready */
    sprintf(query,"UPDATE dbtIncoming SET State = State + 1 WHERE ID = %d",result);
    if (mysql_query(gConn, query))
    {
    	DTNMP_DEBUG_ERR("db_insert_incoming_group", "Database Error: %s",
    		    	    mysql_error(gConn));
    	DTNMP_DEBUG_EXIT("db_insert_incoming_group", "-->%d", result);
    	return 0;
    }

    mysql_free_result(res);

    /* Step 5: return the id*/
	return result;
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
 *****************************************************************************/
int db_outgoing_ready(MYSQL_RES **sql_res) {
	int result = 0;
	char query[1024];

	*sql_res = NULL;

	/* Step 1: Build and execute query. */
	sprintf(query, "SELECT * FROM dbtOutgoing WHERE State=%d", TX_READY);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_outgoing_ready", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
		return result;
	}

	/* Step 2: Parse the row and populate the structure. */
	if ((*sql_res = mysql_store_result(gConn)) != NULL)
	{
		result = mysql_num_rows(*sql_res);
	}
	else
	{
		DTNMP_DEBUG_ERR("db_outgoing_ready", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
	}

	/* Step 3: Return whether we have results waiting. */

	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_insert_incoming_message
 *
 * \par Returns number of incoming message groups.
 *
 * \retval 0 no message groups ready.
 *        !0 There are message groups ready.
 *
 * \param[out] sql_res - The outgoing messages.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
int db_insert_incoming_message(int incomingID, uint8_t *cursor, uint32_t size)
{
	char *query;
	char *result_data = NULL;

	query = (char *) MTAKE(size * 2 + 256);
	result_data = (char *) MTAKE(size * 2 + 1);
	mysql_real_escape_string(gConn, result_data, (char *) cursor, size);

	sprintf(query,"INSERT INTO dbtIncomingMessages(IncomingID,Content)"
			       "VALUES(%d,'%s')",incomingID, result_data);
	MRELEASE(result_data);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_insert_incoming_message", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_insert_incoming_message", "-->0", NULL);
		MRELEASE(query);
		return 0;
	}

	MRELEASE(query);
	return 0;
}

int db_finalize_incoming(uint32_t incomingID)
{
	char query[1024];

	/* Step 3: Update dbtIncoming to processed */
	sprintf(query,"UPDATE dbtIncoming SET State = State + 1 WHERE ID = %d", incomingID);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_insert_incoming_message", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_insert_incoming_message", "-->0", NULL);
		return 0;
	}
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: db_process_outgoing
 *
 * \par Returns 1 if the message is ready to be sent
 *
 * \retval 0 no message groups ready.
 *        !0 There are message groups ready to be sent.
 *
 * \param[out] sql_res - The outgoing messages.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *  07/18/13  S. Jacobs       Added outgoing agents
 *****************************************************************************/
int db_process_outgoing(MYSQL_RES *sql_res)
{
	MYSQL_ROW row;
	pdu_group_t *msg_group = NULL;
	uint32_t idx = 0;
	Lyst agents;
	mid_t *id;
	def_gen_t *debugPrint;
	LystElt elt;
	eid_t *agent_eid = NULL;
	char query[128];

	/* Step 1: For each message group that is ready to go... */
	while ((row = mysql_fetch_row(sql_res)) != NULL)
	{
		/* Step 1.1 Create and populate the message group. */
		idx = atoi(row[0]);
		msg_group = pdu_create_empty_group();
		int result = db_process_outgoing_messages(idx, msg_group);

		if(result != 0)
		{
			/* Step 1.2: Collect list of agents receiving the group. */
			agents = db_process_outgoing_recipients(idx);
			if(agents == NULL)
			{
				DTNMP_DEBUG_ERR("db_process_outgoing","Cannot process outgoing recipients",NULL);
				lyst_destroy(agents);
				return 0;
			}

			/* Step 1.3: For each agent, send a message. */
			for (elt = lyst_first(agents); elt; elt = lyst_next(elt)) {
				agent_eid = (eid_t *) lyst_data(elt);
				DTNMP_DEBUG_INFO("db_process_outgoing", "Sending to name %s", agent_eid->name);
				iif_send(&ion_ptr, msg_group, agent_eid->name);
				MRELEASE(agent_eid);
			}

			lyst_destroy(agents);
			agents = NULL;
		}
		else
		{
			DTNMP_DEBUG_ERR("db_process_outgoing","Cannot process out going message",NULL);
			return 0;
		}

		/* Step 1.4: Release the message group. */
		pdu_release_group(msg_group);
		msg_group = NULL;

		/* Step 1.5: Update the state of the message group in the database. */
		sprintf(query, "UPDATE dbtOutgoing SET State=2 WHERE ID=%d", idx);
		if (mysql_query(gConn, query)) {
			DTNMP_DEBUG_ERR("db_outgoing", "Database Error: %s",
					mysql_error(gConn));
			DTNMP_DEBUG_EXIT("db_outgoing", "-->0", NULL);
			return 0;
		}

	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: db_process_outgoing_messages
 *
 * \par Returns 1 if the outgoing messages have been processed
 *
 * \retval 0 no message groups ready.
 *        !0 There are message groups ready to be sent.
 *
 * \param[in] idx - the index of the message that corresponds to outgoing and
 * 			   outgoing messages
 * \param[in] msg_group - the group that the message is in.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *****************************************************************************/
int db_process_outgoing_messages(uint32_t idx, pdu_group_t *msg_group) {
	int result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	/* Step 1: Find all messages for this outgoing group. */
	sprintf(query, "SELECT * FROM dbtOutgoingMessages WHERE OutgoingID=%d",
			idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_outgoing_ready", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
		return result;
	}

	/* Step 2: Parse the row and populate the structure. */
	while ((res = mysql_store_result(gConn)) != NULL)
	{
		row = mysql_fetch_row(res);
		int table_idx = atoi(row[2]);
		int entry_idx = atoi(row[3]);

		if (db_process_outgoing_one_message(table_idx, entry_idx, msg_group,
				row) == 0)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_messages",
					"Error processing message.", NULL);
			result = 0;
			break;
		}

		result = 1;
	}

	mysql_free_result(res);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_process_outgoing_one_message
 *
 * \retval 0 no message groups ready.
 *        !0 There are message groups ready to be sent.
 *
 * \param[in] table_idx - the index of the table used
 * \param[in] entry_idx - the row in the table needed
 * \param[in] message_group - the group name
 * \param[in] row - a result from a query
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/15/13  E. Birrane      Initial implementation,
 *  07/15/13  S. Jacobs		  Initial implementation,
 *  07/16/13  S. Jacobs       Added custom report case,
 *****************************************************************************/
int db_process_outgoing_one_message(uint32_t table_idx, uint32_t entry_idx,
		pdu_group_t *msg_group, MYSQL_ROW row)
{
	int result = 1;

	/* Step 1: Find out what kind of message we have based on the
	 *         table that is holding it.
	 */
	switch (db_get_table_type(table_idx, entry_idx)) {
	case TIME_PROD_MSG: {
		rule_time_prod_t *entry = NULL;
		entry = db_fetch_time_rule(entry_idx);

		if(entry == NULL)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_one_message","cannot fetch time rule",NULL);
			return 0;
		}

		uint32_t size = 0;
		uint8_t *data = ctrl_serialize_time_prod_entry(entry, &size);

		if(data == NULL)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot serialize time_prod_entry",NULL);
			return 0;
		}

		pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_PERIOD_PROD, data,
				size, NULL);

		if(pdu_msg == NULL)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot create msg", NULL);
			return 0;
		}

		pdu_add_msg_to_group(msg_group, pdu_msg);
		rule_release_time_prod_entry(entry);
	}
		break;
	case CUST_RPT: {
		def_gen_t *entry = NULL;
		entry = db_fetch_def(entry_idx);

		if(entry == NULL)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot fetch definition",NULL);
			return 0;
		}

		uint32_t size = 0;
		uint8_t *data = def_serialize_gen(entry, &size);

		if(data == NULL)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot serialize def_gen_t",NULL);
			return 0;
		}
		pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_DEF_CUST_RPT, data, size,
				NULL);

		if(pdu_msg == NULL)
		{
			DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot create msg", NULL);
			return 0;
		}

		pdu_add_msg_to_group(msg_group, pdu_msg);
		def_release_gen(entry);

	}
		break;
	case EXEC_CTRL_MSG: {
		ctrl_exec_t *entry = NULL;
		entry = db_fetch_ctrl(entry_idx);
		uint32_t size = 0;

		if (entry != NULL) {

			uint8_t *data = ctrl_serialize_exec(entry, &size);

			if(data == NULL)
			{
				DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot serialize control",NULL);
				return 0;
			}

			pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size,
					NULL);

			if(pdu_msg == NULL)
			{
				DTNMP_DEBUG_ERR("db_process_outgoing_one_message","Cannot create msg", NULL);
				return 0;
			}

			pdu_add_msg_to_group(msg_group, pdu_msg);


			ctrl_release_exec(entry);
		}
		else
		{
			DTNMP_DEBUG_ERR("...", "Can't construct control.", NULL);
			result = 0;
		}
	}
		break;

	default: {
		DTNMP_DEBUG_ERR("db_process_outgoing_one_message",
				"Unknown table type (%d) entry_idx (%d)", table_idx, entry_idx);
	}
	}

	DTNMP_DEBUG_EXIT("db_process_outgoing_one_message", "-->%d", result);

	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_process_outgoing_recipients
 *
 * \par Returns a lyst of the agents to send a message to
 *
 * \retval 0 no recipients.
 *        !0 There are recipients to be sent to.
 *
 * \param[in] outgoingId - The id in the table that holds the receiving agents that
 * correspond to the id on the outgoing messages
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/18/13  S. Jacobs       Initial Implementation
 *****************************************************************************/
Lyst db_process_outgoing_recipients(uint32_t outgoingId) {
	Lyst result = lyst_create();
	LystElt elt;
	agent_t *agent;
	eid_t *cur_name;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	adm_reg_agent_t *agentName;
	char query[1024];

	/* Step 1: Query the database */
	sprintf(query, "SELECT AgentID FROM dbtOutgoingRecipients "
			"WHERE OutgoingID=%d", outgoingId);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_process_outgoing_recipients", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_process_outgoing_recipients", "-->", NULL);
		return NULL;
	}

	/* Step 2: Parse the results and fetch agent */
	res = mysql_store_result(gConn);
	int i;

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		elt = lyst_first(result);
		for (i = 0; i < mysql_num_rows(res); i++)
		{
			/* Step 3: Fetch agent and create lyst */
			agentName = db_fetch_reg_agent(atoi(row[0]));

			if(agentName == NULL)
			{
				DTNMP_DEBUG_ERR("db_process_outgoing_recipients","Cannot fetch registered agent",NULL);
				lyst_destroy(result);
				return NULL;
			}

			/* Step 4: create lyst */
			cur_name = (eid_t*) MTAKE(sizeof(eid_t));
			memcpy(cur_name->name, agentName->agent_id.name,
					sizeof(agentName->agent_id.name));
			DTNMP_DEBUG_INFO("db_process_outgoing_recipients", "Adding agent name %s.", cur_name->name);
			lyst_insert_last(result, cur_name);
			elt = lyst_next(elt);

			msg_release_reg_agent(agentName);
		}

	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: db_get_table_type
 *
 * \par Returns a value corresponding to a macro for a specific message type
 * \par UNKNOWN_MSG (0)
 * \par TIME_PROD_MSG (1)
 * \par PRED_PROD_MSG (2)
 * \par EXEC_CTRL_MSG (3)
 * \par CUST_RPT (4)
 *
 * \retval 0 uknown message type.
 *        !0 A specific message type mentioned about.
 *
 * \param[in] table_idx - the table the message is located in
 * \param[in] entry_idx - the row in the table the message will be.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/14/13  S. Jacobs       Initial Implementation
 *****************************************************************************/
int db_get_table_type(int table_idx, int entry_idx) {
	int result = UNKNOWN_MSG;
	MYSQL_RES *res = NULL;
	MYSQL_RES *entry_res = NULL;
	MYSQL_ROW row;
	MYSQL_ROW entry_row;
	char query[1024];

	/* Step 1: Grab the table name associated with this ID. */
	sprintf(query, "SELECT TableName from lvtMessageTablesList WHERE ID=%d",
			table_idx);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_get_table_type", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_get_table_type", "-->%d", result);
		return result;
	}

	if ((res = mysql_store_result(gConn)) != NULL) {
		row = mysql_fetch_row(res);

		if (strcmp(row[0], "dbtMessagesDefinitions") == 0) {
			return CUST_RPT;
		}

		if (strcmp(row[0], "dbtMessagesControls") == 0) {
			/* See the type from the message controls table. */
			sprintf(query, "SELECT TYPE FROM dbtMessagesControls where ID=%d",
					entry_idx);
			if (mysql_query(gConn, query)) {
				DTNMP_DEBUG_ERR("db_outgoing_ready", "Database Error: %s",
						mysql_error(gConn));
				DTNMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
				return result;
			}

			if ((entry_res = mysql_store_result(gConn)) != NULL)
			{
				entry_row = mysql_fetch_row(entry_res);
				int type = atoi(entry_row[0]);

				switch (type) {
				case 1:
					result = TIME_PROD_MSG;
					break;
				case 2:
					result = PRED_PROD_MSG;
					break;
				case 3:
					result = EXEC_CTRL_MSG;
					break;
				default: {
					DTNMP_DEBUG_ERR("db_get_table_type", "Unknown type %d", type);
				}
				}
			} else {
				DTNMP_DEBUG_ERR("db_get_table_type", "Can't find message control.", NULL);
			}

			mysql_free_result(entry_res);
		}
	}

	mysql_free_result(res);

	return result;
}


void * run_daemon(void * threadId) {
	time_t start_time = 0;
	MYSQL_RES *sql_res;
	char *server = "localhost";
	char *user = "root"; //user must be the root
	char *password = "NetworkManagement";
	char *database = "dtnmp";

	while (g_running) {
		start_time = getUTCTime();

		if (db_outgoing_ready(&sql_res)) {
			db_process_outgoing(sql_res);
			mysql_free_result(sql_res);
			sql_res = NULL;
		}

		microsnooze((unsigned int) (2000000 - (getUTCTime() - start_time)));
	}

	db_close();
	pthread_exit(NULL);
}

#endif // HAVE_MYSQL
