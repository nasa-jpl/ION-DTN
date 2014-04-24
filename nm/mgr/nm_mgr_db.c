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
 **  08/19/13  E. Birrane     Documentation clean up and code review comments.
 *****************************************************************************/

#ifdef HAVE_MYSQL

#include <string.h>

#include "nm_mgr.h"
#include "nm_mgr_db.h"


/* Global conngection to the MYSQL Server. */
static MYSQL *gConn;



/******************************************************************************
 *
 * \par Function Name: db_add_agent()
 *
 * \par Adds a Registered Agent to the database
 *
 * \return 0 Failure
 *        !0 Success
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
 *****************************************************************************/
uint32_t db_add_agent(eid_t agent_eid)
{
	char query[1024];

	/* Step 1: Create Query */
	sprintf(query, "INSERT INTO dbtRegisteredAgents(AgentIdSDNV) "
			"VALUES(%s)", agent_eid.name);

	if (mysql_query(gConn, query)) {
		DTNMP_DEBUG_ERR("db_add_agent", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_agent", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_ERR("db_add_agent", "-->1", NULL);
	return 1;
}

/******************************************************************************
 *
 * \par Function Name: db_add_mid
 *
 * \par Creates a MID in the database. This function will populate both the
 *      dbtMIDs table and dbtMIDDetails table.
 *
 * \retval 0  Failure
 *         >0 The index of the inserted MID from the dbtMIDs table.
 *
 * \param[in] attr    - DB attribute of the new MID.
 * \param[in] flag    - The flag byte of the new MID.
 * \param[in] issuer  - The issuer, or NULL if no issuer.
 * \param[in] OID     - The serialized OID for the MID.
 * \param[in] tag     - The tag, or NULL if no tag.
 * \param[in] mib     - MIB information for the MID.
 * \param[in] mib_iso - MIB ISO information for MIS descriptor table.
 * \param[in] name    - MIB item name.
 * \param[in] descr   - MIB item description.
 *
 * \par Notes:
 * 		- This function allows null names, decriptions, mib and mib_iso
 * 		  values. However, an OID is required.
 * 		- We count duplicate MID and other failures together.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
uint32_t db_add_mid(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag,
		char *mib, char *mib_iso, char *name, char *descr)
{
	char query[1024];
	uint32_t result = 0;
	char last_insert_id[1024];

	DTNMP_DEBUG_ENTRY("db_add_mid", "(%d, %d, %ld, %s, %ld, %s, %s, %s, %s)",
			           attr, flag, (unsigned long) issuer, OID,
			           (unsigned long) tag, mib, mib_iso, name, descr);

	/* Step 0: Sanity check arguments. */
	if(OID == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_mid","Can't add mid, bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_mid","-->0",NULL);
		return 0;
	}

	/* Step 1: Make sure the ID is not already in the DB. */
	if ((result = db_fetch_mid_idx(attr, flag, issuer, OID, tag)) > 0)
	{
		DTNMP_DEBUG_WARN("db_add_mid", "Can't add dup. MID (idx is %d).", result);
		DTNMP_DEBUG_EXIT("db_add_mid", "-->0", NULL);
		return 0;
	}

	/*
	 * Step 2: Build and execute query to add row to dbtMIDs. Also, store the
	 *         row ID of the inserted row.
	 */
	sprintf(query,
			"INSERT INTO dbtMIDs \
(Attributes,Type,Category,IssuerFlag,TagFlag,OIDType,IssuerID,OIDValue,TagValue) \
VALUES (%d,b'%d',b'%d',b'%d',b'%d',%d,"UVAST_FIELDSPEC",'%s',"UVAST_FIELDSPEC")",
			attr,
			MID_GET_FLAG_TYPE(flag),
			MID_GET_FLAG_CAT(flag),
			(MID_GET_FLAG_ISS(flag)) ? 1 : 0,
			(MID_GET_FLAG_TAG(flag)) ? 1 : 0,
			MID_GET_FLAG_OID(flag),
			issuer,
			(OID == NULL) ? "0" : OID,
			tag);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_mid", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_mid", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_mid", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_mid", "-->0", NULL);
		return 0;
	}

	/* Step 3: Build and execute query to add row to dbtMIDDetails. */
	sprintf(query,
			"INSERT INTO dbtMIDDetails \
(MIDID,MIBName,MIBISO,Name,Description) VALUES \
(%d,'%s','%s','%s','%s')",
            result,
            (mib == NULL) ? "" : mib,
            (mib_iso == NULL) ? "" : mib_iso,
            (name == NULL) ? "" : name,
            (descr == NULL) ? "" : descr);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_mid", "Database Error: %s", mysql_error(gConn));

		/* Step 3.1: If we can't update dbtMIDDetails, remove row from dbtMids */
		sprintf(query, "DELETE FROM dbtMIDs WHERE ID=%d", result);
		if (mysql_query(gConn, query))
		{
			DTNMP_DEBUG_ERR("db_add_mid",
					        "Can't remove dbtMID row %d as part of rollback.",
					        result);
		}

		/* Step 3.2 reset the auto increment to maintain numbering. */
		sprintf(query, "ALTER TABLE dbtMIDs AUTO_INCREMENT=%d", result);
		if (mysql_query(gConn, query))
		{
			DTNMP_DEBUG_ERR("db_add_mid",
					        "Error resetting auto increment during rollback.",
					        NULL);
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
 * \par Fetches a control structure from the associated controls table.
 *
 * \retval NULL Failure
 *        !NULL The built command structure.
 *
 * \param[in] id - The Primary Key of the message controls row used to find
 *                 the command.
 *
 * \par Notes:
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

ctrl_exec_t *db_fetch_ctrl(int id)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];
	ctrl_exec_t *result = NULL;

	/* Step 1: Build Query */
	sprintf(query, "SELECT * FROM dbtMessagesControls WHERE ID=%d", id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_ctrl", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Parse the result. */
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_ctrl", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->NULL", NULL);
		return NULL;
	}
	if ((row = mysql_fetch_row(res)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_ctrl", "Unable to find ctrl with ID of %d\n",
				id);
		mysql_free_result(res);

		DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->NULL",NULL);
		return NULL;
	}

	/* Step 3: Validate and parse the structure. */
	int type = atoi(row[1]);

	if (type != EXEC_CTRL_MSG)
	{
		DTNMP_DEBUG_ERR("db_fetch_ctrl",
				        "Bad row type. Expecting %d and got %d\n",
				        EXEC_CTRL_MSG, type);
		mysql_free_result(res);

		DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->NULL",NULL);
		return NULL;
	}

	time_t time = (time_t) atoll(row[2]);
	Lyst contents = db_fetch_mid_col(atoi(row[6]));
	if (contents == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_ctrl", "Can't grab mid col.", NULL);
		mysql_free_result(res);

		DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->NULL",NULL);
		return NULL;
	}

	/* Step 4: Build the control structure. */
	result = ctrl_create_exec(time, contents);

	/* Step 5: Clean up memory. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_ctrl", "-->%llu", (unsigned long)result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_data_col
 *
 * \par Creates a data collection from dbtDataCollections in the database
 *
 * \retval NULL Failure
 *        !NULL The built Data collection.
 *
 * \param[in] id - The Primary Key in the dbtDataCollection table.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
Lyst db_fetch_data_col(int dc_id)
{
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	Lyst result;
	datacol_entry_t *dc_entry;

	/* Step 1: Construct/run the Query and capture results. */
	sprintf(query, "SELECT * FROM dbtDataCollection WHERE CollectionID=%d",
			dc_id);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col", "SQL Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_data_col", "-->NULL", NULL);
		return NULL;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col", "SQL Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_data_col", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Allocate a Lyst to hold the collection. */
	result = lyst_create();

	/* Step 3: For each entry returned as part of the collection. */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		if((dc_entry = db_fetch_data_col_entry_from_row(row)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_data_col", "Can't get entry.", NULL);
			utils_datacol_destroy(&result);
			mysql_free_result(res);

			DTNMP_DEBUG_EXIT("db_fetch_data_col","-->NULL",NULL);
			return NULL;
		}

		lyst_insert_last(result, dc_entry);
	}

	/* Step 4: Free results. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_data_col", "-->%llu", (unsigned long) result);
	return result;
}



/*******************************************************************************
 *
 * \par Function Name: db_fetch_data_col_entry
 *
 * \par Fetches the appropriate data collection entry
 *
 * \retval NULL The entry could not be retrieved
 *         !NULL a datacol_entry_t was created
 *
 * \param[in] id    - The row of the entry
 * \param[in] order - The order of the entry within a collection (1,2,3...)
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/24/13  S. Jacobs      Initial implementation,
 ******************************************************************************/

datacol_entry_t *db_fetch_data_col_entry(int id, int order)
{
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	datacol_entry_t *result = NULL;

	/* Step 1: Construct and execute query to get the entry data. */
	sprintf(query,
			"SELECT * FROM dbtDataCollection WHERE CollectionID=%d AND DataOrder=%d",
			id, order);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetchdata_col_entry", "-->NULL", NULL);
		return NULL;
	}

	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetchdata_col_entry", "-->NULL", NULL);
		return NULL;
	}

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		if((result = db_fetch_data_col_entry_from_row(row)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_data_col_entry", "Can't get entry.", NULL);
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry", "No rows", NULL)
	}

	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_data_col_entry","-->0x%#llx",
			         (unsigned long) result);
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
 *  08/19/13  E. Birrane      Initial implementation,
 ******************************************************************************/

datacol_entry_t *db_fetch_data_col_entry_from_row(MYSQL_ROW row)
{
	datacol_entry_t *result = NULL;

	DTNMP_DEBUG_ENTRY("db_fetch_data_col_entry_from_row","(0x%#llx)",
					  (unsigned long) row);

	/* Step 1: Allocate space for the entry. */
	if((result = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry_from_row",
				        "Can't allocate %d bytes.",
					    sizeof(datacol_entry_t));

		DTNMP_DEBUG_EXIT("db_fetch_data_col_entry_from_row", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Populate the entry. */
	result->length = atoi(row[1]);
	result->value = (uint8_t*) MTAKE(result->length);

	if((result->length == 0) || (result->value == 0))
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry_from_row",
				        "length : %lu",result->length);
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry_from_row",
				        "value : %lu",result->value);

		MRELEASE(result);

		DTNMP_DEBUG_EXIT("db_fetch_data_col_entry_from_row", "-->NULL", NULL);
		return NULL;
	}

	memcpy(result->value, row[2], result->length);

	DTNMP_DEBUG_EXIT("db_fetch_data_col_entry_from_row", "-->%0x#llx",
			         (unsigned long) result);
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
 * 		- We assume, for now, that there is only 1 type of definition
 * 		  in the table: custom report definitions.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

def_gen_t *db_fetch_def(int id)
{
	def_gen_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];

	/*Step 1: Construct and run the query to grab the def information. */
	sprintf(query, "SELECT * FROM dbtMessagesDefinitions WHERE ID=%d", id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_def", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_def", "-->NULL", NULL);
		return NULL;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_def", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_def", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Grab information from the row, if we got a row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		int mid_id = atoi(row[2]);
		int mc_id = atoi(row[3]);
		mid_t *mid = NULL;
		Lyst contents = NULL;

		/*Step 2.1: Find the MID identifying the definition. */
		if((mid = db_fetch_mid(mid_id)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_def","Cannot fetch mid (%d)",mid_id);
		}
		else
		{
			/* Step 2.2: Find the Mid Collection for the definition. */
			if((contents = db_fetch_mid_col(mc_id)) == NULL)
			{
				DTNMP_DEBUG_ERR("db_fetch_def","Cannot fetch MC (%d)",mc_id);
			}
			else
			{
				/* Step 2.3: Build the definition. */
				result = def_create_gen(mid, contents);
			}
		}
	}
	else
	{
		DTNMP_DEBUG_ENTRY("db_fetch_def", "No rows found for %d.", id);
	}

	/* Step 3: Free the db result. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_def", "-->%ld", (unsigned long) result);

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
 * \par Notes:
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

	/* Step 1: Construct and run the query to get the MID information. */
	sprintf(query, "SELECT * FROM dbtMIDs WHERE ID=%d", id);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_mid", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid", "-->NULL", NULL);
		return NULL;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_mid", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		uint8_t type    = (uint8_t) row[2][0];
		uint8_t cat     = (uint8_t) row[3][0];
		uint8_t issFlag = (uint8_t) atoi(row[4]);
		uint8_t tagFlag = (uint8_t) atoi(row[5]);
		uint8_t oidType = (uint8_t) row[6][0];
		uvast issuer    = (uvast) atoll(row[7]);
		oid_t *oid      = db_fetch_oid(id, oidType, row[8]);
		uvast tag = (uvast) atoll(row[9]);

		if(oid == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_mid","Cannot fetch the oid: %s",row[8]);
		}
		else
		{
			if ((result = mid_construct(type,
					                    cat,
					                    issFlag ? &issuer : NULL,
					                    tagFlag ? &tag : NULL,
					                    oid)) == NULL)
			{
				DTNMP_DEBUG_ERR("db_fetch_mid", "Cannot construct MID", NULL);
			}
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_mid", "Did not find MID with ID of %d\n", id);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	/* Step 4: Sanity check the returned MID. */
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
 * \par Notes:
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

	/* Step 1: Construct and run the query to get MC DB info. */
	sprintf(query,
			"SELECT MIDID FROM dbtMIDCollection WHERE CollectionID=%d ORDER BY MIDOrder",
			id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_col", "SQL Error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_col", "-->NULL",NULL);
		return NULL;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_col", "SQL Error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_col", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: For each MID in the collection... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{

		/* Step 2.1: For each row, build a MID and add it to the collection. */
		if((new_mid = db_fetch_mid(atoi(row[0]))) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_mid_col", "Can't grab MID with ID %d.",
					        atoi(row[0]));

			midcol_destroy(&result);
			mysql_free_result(res);

			DTNMP_DEBUG_EXIT("db_fetch_mid_col", "-->NULL", NULL);
			return NULL;
		}

		lyst_insert_last(result, new_mid);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_mid_col", "-->%d", (unsigned long) result);
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
 * \param[in] attr    - the attribute of the MID
 * \param[in] flag    - MID flag byte.
 * \param[in] issuer  - MID issuer
 * \param[in] OID     - MID OID
 * \param[in] tag     - MID tag value.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

int db_fetch_mid_idx(int attr, uint8_t flag, uvast issuer, char *OID, uvast tag)
{
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
AND TagFlag=%d AND OIDType=%d AND IssuerID="UVAST_FIELDSPEC" AND OIDValue='%s' \
AND TagValue="UVAST_FIELDSPEC,
			attr,
			MID_GET_FLAG_TYPE(flag),
			MID_GET_FLAG_CAT(flag),
			(MID_GET_FLAG_ISS(flag)) ? 1 : 0,
			(MID_GET_FLAG_TAG(flag)) ? 1 : 0,
			MID_GET_FLAG_OID(flag),
			issuer,
			(OID == NULL) ? "0" : OID,
			tag);

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
		DTNMP_DEBUG_INFO("db_fetch_mid_idx", "Can't find MID.", NULL);
	}

	mysql_free_result(res);

	/* Step 3: Return the IDX. */
	DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->%d", result);
	return result;
}



/*****************************************************************************
 *
 * \par Function Name: db_fetch_oid
 *
 * \par Grabs an OID from the database.
 *
 * \retval NULL OID could not be fetched
 *        !NULL The OID
 *
 * \param[in]  mid_id   - The id of the MID whose OID is being fetched
 * \param[in]  oid_type - The OID type (full, parm, comp full, comp parm)
 * \param[out] oid_root - The root OID with any other necessary attachments
 *
 * \par Notes:
 * 		- Currently, compressed OIDs are not supported.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  S. Jacobs      Initial implementation,
 ******************************************************************************/

oid_t *db_fetch_oid(int mid_id, int oid_type, char *oid_root)
{
	oid_t *result = NULL;

	switch (oid_type)
	{
		case OID_TYPE_FULL:
			return db_fetch_oid_full(mid_id, oid_root);
			break;

		case OID_TYPE_PARAM:
			return db_fetch_oid_parms(mid_id, oid_root);
			break;

/*		case OID_TYPE_COMP_FULL:
			oid_root = oid_deserialize_comp(data, size, &bytes);
			break;

		case OID_TYPE_COMP_PARAM:
			oid_root = oid_deserialize_comp_param(data, size, &bytes);
			break;
*/
		default:
			DTNMP_DEBUG_ERR("db_fetch_oid", "Unknown OID Type %d", oid_type);
			break;
	}

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
 * \param[in] mid_id   - The id of the MID whose OID that is being fetched
 * \param[in] oid_root - The root OID
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  S. Jacobs      Initial implementation,
 ******************************************************************************/

oid_t *db_fetch_oid_full(int mid_id, char *oid_root)
{
	oid_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;


	DTNMP_DEBUG_ENTRY("db_fetch_oid_full","(%d,%s)", mid_id, oid_root);

	/* Step 1: Convert the oid_root to hexadecimal. For the full OID, the
	 *         root is the entire OID.
	 */
	if((data = utils_string_to_hex((unsigned char *) oid_root, &size)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_full","cannot convert to string",NULL);
		return NULL;
	}

	/* Step 2: Deserialize the OID into the OID type structure. */
	if((result = oid_deserialize_full(data, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_full","Cannot deserialize oid",NULL);
	}

	MRELEASE(data);

	DTNMP_DEBUG_EXIT("db_fetch_oid_full","-->%llu",(unsigned long) result);
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
 * \param[in] mid_id   - The id of the MID whose OID that is being fetched
 * \param[in] oid_root - The root OID which will have the parameters appended
 *                       to the end
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/25/13  S. Jacobs      Initial implementation,
 ******************************************************************************/

oid_t *db_fetch_oid_parms(int mid_id, char* oid_root)
{
	oid_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;
	uint8_t *parms = NULL;
	uint32_t parms_size = 0;
	uint32_t num_parms = 0;

	DTNMP_DEBUG_ENTRY("db_fetch_oid_parms","(%d, %s)", mid_id, oid_root);

	/* Step 1: grab the OID parameters String. */
	if((parms = db_fetch_parms_str(mid_id, &parms_size, &num_parms)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_parms", "Can't fetch parms string", NULL);
		return NULL;
	}

	/* Step 2: Allocate data to hold everything. */
	size = strlen(oid_root) + parms_size;
	if ((data = (uint8_t *) MTAKE(size)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_parms", "Can't allocate data of size %d", size);
		MRELEASE(parms);
		return NULL;
	}

	/* Step 3: Copy everything into data. */
	uint8_t *cursor = data;

	uint32_t root_size = 0;
	uint8_t *root_data = utils_string_to_hex((unsigned char *) oid_root,
			                                 &root_size);

	if(root_data == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_parms","Can't convert %s to hex.", oid_root);
		MRELEASE(data);
		MRELEASE(parms);
		return NULL;
	}

	memcpy(cursor, root_data, root_size);
	cursor += root_size;
	MRELEASE(root_data);

	memcpy(cursor, parms, parms_size);
	cursor += parms_size;

	/* Step 4: Build OID */
	if((result = oid_deserialize_param(data, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_parms","Cannot deserialize param",NULL);
	}

	/* Step 5: Release stuff. */
	MRELEASE(data);
	MRELEASE(parms);

	DTNMP_DEBUG_EXIT("db_fetch_oid_parms","-->%llu",(unsigned long) result);
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
 *  07/25/13  S. Jacobs      Initial implementation,
 *******************************************************************************/

uint8_t *db_fetch_parms_str(int mid_id, uint32_t *parm_size, uint32_t *num_parms)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];
	uint8_t *result = NULL;
	Lyst parms_datacol;

	*parm_size = 0;
	*num_parms = 0;

	DTNMP_DEBUG_ENTRY("db_fetch_parms_str","(%d, %llu, %llu)", mid_id,
			(unsigned long) parm_size, (unsigned long) num_parms);

	/* Step 0: Sanity Check. */
	if((parm_size == NULL) || (num_parms == NULL))
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","Bad args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	/*Step 1: Query the data base for the Data Collection. */
	sprintf(query,
			"SELECT DataCollectionID FROM dbtMIDParameterizedOIDs WHERE MIDID=%d",
			mid_id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	/*Step 2: Grab the Data Collection ID for the DC holding the parameters.*/
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	/* Step 3: Grab the data collection representing the parameters. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		int dc_idx = atoi(row[0]);
		if((parms_datacol = db_fetch_data_col(dc_idx)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_parms_str","Can't fetch data col.",NULL);
			mysql_free_result(res);
			DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
			return NULL;
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","MID row was empty: %d",mid_id);
		mysql_free_result(res);

		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	mysql_free_result(res);

	/* Step 4: Grab number of parameters extracted */
	*num_parms = lyst_length(parms_datacol);

	if(*num_parms == 0)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","No parameters found.",NULL);

		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	/* Step 5: Serialize the parameters. */
	if((result = utils_datacol_serialize(parms_datacol, parm_size)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms_str","Could not serialize the parms",NULL);
		utils_datacol_destroy(&parms_datacol);

		DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->NULL", NULL);
		return NULL;
	}

	utils_datacol_destroy(&parms_datacol);

	DTNMP_DEBUG_EXIT("db_fetch_parms_str", "-->%llu", (unsigned long) result);
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

adm_reg_agent_t *db_fetch_reg_agent(int id)
{
	adm_reg_agent_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];

	DTNMP_DEBUG_ENTRY("db_fetch_reg_agent","(%d)", id);

	/*Step 1: Build query. */
	sprintf(query, "SELECT * FROM dbtRegisteredAgents WHERE ID=%d", id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ENTRY("db_fetch_reg_agent", "(%d)", id);
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent", "-->%ld",
				(unsigned long) result);
		return NULL;
	}

	/*Step 2: Parse results. */
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ENTRY("db_fetch_reg_agent", "(%d)", id);
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent", "-->%ld",
				(unsigned long) result);
		return NULL;
	}

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		eid_t eid;
		strncpy(eid.name, row[1], MAX_EID_LEN);

		/* Step 3: Create structure for agent */
		if((result = msg_create_reg_agent(eid)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_reg_agent","Cannot create a registered agent",NULL);
			mysql_free_result(res);
			return NULL;
		}
	}

	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_reg_agent", "-->%ld", (unsigned long) result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: db_fetch_table_type
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

int db_fetch_table_type(int table_idx, int entry_idx)
{
	int result = UNKNOWN_MSG;
	MYSQL_RES *res = NULL;
	MYSQL_RES *entry_res = NULL;
	MYSQL_ROW row;
	MYSQL_ROW entry_row;
	char query[1024];

	DTNMP_DEBUG_ENTRY("db_fetch_table_type","(%d,%d)", table_idx, entry_idx);

	/* Step 1: Grab the table name associated with this ID. */
	sprintf(query, "SELECT TableName from lvtMessageTablesList WHERE ID=%d",
			table_idx);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_table_type", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_table_type", "-->%d", result);
		return result;
	}

	if ((res = mysql_store_result(gConn)) != NULL)
	{
		row = mysql_fetch_row(res);

		if (strcmp(row[0], "dbtMessagesDefinitions") == 0)
		{
			mysql_free_result(entry_res);
			return CUST_RPT;
		}
		else if (strcmp(row[0], "dbtMessagesControls") == 0)
		{
			/* See the type from the message controls table. */
			sprintf(query, "SELECT TYPE FROM dbtMessagesControls where ID=%d",
					entry_idx);
			if (mysql_query(gConn, query))
			{
				DTNMP_DEBUG_ERR("db_fetch_table_type", "Database Error: %s",
						mysql_error(gConn));

				mysql_free_result(entry_res);

				DTNMP_DEBUG_EXIT("db_fetch_table_type", "-->%d", result);
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
					default:
						DTNMP_DEBUG_ERR("db_fetch_table_type", "Unknown type %d", type);
				}
			}
			else
			{
				DTNMP_DEBUG_ERR("db_fetch_table_type", "Can't find message control.", NULL);
			}

			mysql_free_result(entry_res);
		}
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
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

rule_time_prod_t *db_fetch_time_rule(int id)
{
	rule_time_prod_t *result = NULL;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_time_rule","(%d)", id);

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
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_time_rule", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_time_rule", "-->NULL", NULL);
		return NULL;
	}

	if ((row = mysql_fetch_row(res)) != NULL)
	{
		/* Step 2.1: Grab the type and make sure it is correct. */
		int type = atoi(row[1]);

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
				mysql_free_result(res);
				return NULL;
			}

			result = rule_create_time_prod_entry(time, count, period, contents);

			if(result == NULL)
			{
				DTNMP_DEBUG_ERR("db_fetch_time_rule","Cannot create time_prod_entry",NULL);
				mysql_free_result(res);
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
	DTNMP_DEBUG_EXIT("db_fetch_time_rule","-->%llu", (unsigned long) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_incoming_initialize
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

int db_incoming_initialize(time_t timestamp)
{
	MYSQL_RES *res = NULL;
    MYSQL_ROW row;
	char query[1024];
	int result = 0;

	DTNMP_DEBUG_ENTRY("db_incoming_initialize","(%llu)", timestamp);

	/* Step 1: insert message into dbtIncoming*/
	sprintf(query, "INSERT INTO dbtIncoming(ReceivedTS,GeneratedTS,State) "
		    			  "VALUES(NOW(),%lu,0)", (unsigned long) timestamp);
	if (mysql_query(gConn, query))
    {
		DTNMP_DEBUG_ERR("db_incoming_initialize", "Database Error: %s",
		    	  		mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
		return 0;
    }

	/* Step 2: Get the id of the inserted message*/
	sprintf(query, "SELECT LAST_INSERT_ID() FROM dbtIncoming");
    if (mysql_query(gConn, query))
    {
		DTNMP_DEBUG_ERR("db_incoming_initialize", "Database Error: %s",
		    	    	mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
		return 0;
	}

    /* Step 3: Store the result*/
    if((res = mysql_store_result(gConn)) == NULL)
    {
		DTNMP_DEBUG_ERR("db_incoming_initialize", "Database Error: %s",
		    	    	mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
		return 0;
    }

    if ((row = mysql_fetch_row(res)) != NULL)
    {
    	result = atoi(row[0]);

    	/* Step 3.1: Update State to ready */
        sprintf(query,"UPDATE dbtIncoming SET State = State + 1 WHERE ID = %d",result);
        if (mysql_query(gConn, query))
        {
        	DTNMP_DEBUG_ERR("db_incoming_initialize", "Database Error: %s",
        		    	    mysql_error(gConn));
            mysql_free_result(res);

        	DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
        	return 0;
        }
    }

    mysql_free_result(res);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_incoming_finalize
 *
 * \par Finalize processing of the incoming messages.
 *
 * \retval 0 failure
 *        !0 success
 *
 * \param[in] id - The incoming message group ID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *****************************************************************************/

int db_incoming_finalize(uint32_t id)
{
	char query[1024];

	/* Step 1: Update dbtIncoming to processed */
	sprintf(query,"UPDATE dbtIncoming SET State = State + 1 WHERE ID = %d", id);
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_incoming_finalize", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_incoming_finalize", "-->0", NULL);
		return 0;
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: db_incoming_process_message
 *
 * \par Returns number of incoming message groups.
 *
 * \retval 0 no message groups ready.
 *        !0 There are message groups ready.
 *
 * \param[in] id     - The ID for the incoming message.
 * \param[in] cursor - Cursor pointing to start of message.
 * \param[in] size   - The size of the incoming message.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *****************************************************************************/
int db_incoming_process_message(int id, uint8_t *cursor, uint32_t size)
{
	char *query;
	char *result_data = NULL;

	if((query = (char *) MTAKE(size * 2 + 256)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_incoming_process_message","Can't alloc %d bytes.",
				        size * 2 + 256);
		DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		return 0;
	}

	if((result_data = (char *) MTAKE(size * 2 + 1)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_incoming_process_message","Can't alloc %d bytes.",
				        size * 2 + 1);
		MRELEASE(query);
		DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		return 0;
	}

	mysql_real_escape_string(gConn, result_data, (char *) cursor, size);

	sprintf(query,"INSERT INTO dbtIncomingMessages(IncomingID,Content)"
			       "VALUES(%d,'%s')",id, result_data);
	MRELEASE(result_data);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_incoming_process_message", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		MRELEASE(query);
		return 0;
	}

	MRELEASE(query);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_daemon
 *
 * \par Returns number of outgoing message groups ready to be sent.
 *
 * \return Thread Information...
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
 *****************************************************************************/

void *db_mgt_daemon(void * threadId)
{
	MYSQL_RES *sql_res;
	struct timeval start_time;
	vast delta = 0;

	DTNMP_DEBUG_ENTRY("db_mgt_daemon","(0x%#llx)", threadId);

	DTNMP_DEBUG_ALWAYS("db_mgt_daemon","Starting Manager Database Daemon",NULL);

	while (g_running)
	{
    	getCurrentTime(&start_time);

		if (db_outgoing_ready(&sql_res))
		{
			db_outgoing_process(sql_res);
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

	DTNMP_DEBUG_ALWAYS("db_mgt_daemon","Cleaning up Manager Database Daemon", NULL);

	db_mgt_close();

	DTNMP_DEBUG_ALWAYS("db_mgt_daemon","Manager Database Daemon Finished.",NULL);
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
 *****************************************************************************/
int db_mgt_init(char *server, char *user, char *pwd, char *database, int clear)
{

	DTNMP_DEBUG_ENTRY("db_mgt_init","(%s, %s, %s, %s, %d)", server, user, pwd, database, clear);

	gConn = mysql_init(NULL);

	DTNMP_DEBUG_ENTRY("db_mgt_init", "(%s,%s,%s,%s)", server, user, pwd, database);

	if (!mysql_real_connect(gConn, server, user, pwd, database, 0, NULL, 0)) {
		DTNMP_DEBUG_ERR("db_mgt_init", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_init", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_INFO("db_mgt_init", "gConnected to Database.", NULL);

	if(clear != 0)
	{
		db_mgt_clear();
	}

	/* Step 2: Make sure the DB knows about the MIDs we need. */
    db_mgt_verify_mids();

	DTNMP_DEBUG_EXIT("db_mgt_init", "-->1", NULL);
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
 *****************************************************************************/

int db_mgt_clear()
{

	DTNMP_DEBUG_ENTRY("db_mgt_clear", "()", NULL);

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
		DTNMP_DEBUG_ERR("db_mgt_clear", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_clear", "--> 0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_mgt_clear", "--> 1", NULL);
	return 1;
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
	DTNMP_DEBUG_ENTRY("db_mgt_close","()",NULL);
	mysql_close(gConn);
	DTNMP_DEBUG_EXIT("db_mgt_close","-->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: db_mgt_verify_mids
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
void db_mgt_verify_mids()
{
	int i = 0;
	int size = 0;
	char *oid_str = NULL;
	LystElt elt;
	adm_datadef_t *admData = NULL;

	DTNMP_DEBUG_ENTRY("db_mgt_verify_mids","()", NULL);

	/* Step 1: For each ADM item defined... */
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
			DTNMP_DEBUG_ERR("db_mgt_verify_mids", "%s", oid_str);
			MRELEASE(oid_str);
		}
	}

	DTNMP_DEBUG_EXIT("db_mgt_verify_mid","-->.", NULL);
	return;
}



/******************************************************************************
 *
 * \par Function Name: db_outgoing_process
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

int db_outgoing_process(MYSQL_RES *sql_res)
{
	MYSQL_ROW row;
	pdu_group_t *msg_group = NULL;
	uint32_t idx = 0;
	Lyst agents;
	mid_t *id;
	def_gen_t *debugPrint;
	LystElt elt;
	adm_reg_agent_t *agent_reg = NULL;
	char query[128];

	DTNMP_DEBUG_ENTRY("db_outgoing_process","(0x%#llx)",(unsigned long) sql_res);

	/* Step 1: For each message group that is ready to go... */
	while ((row = mysql_fetch_row(sql_res)) != NULL)
	{
		/* Step 1.1 Create and populate the message group. */
		idx = atoi(row[0]);

		if((msg_group = pdu_create_empty_group()) == NULL)
		{
			DTNMP_DEBUG_ERR("db_outgoing_process","Cannot create group.", NULL);
			return 0;
		}

		int result = db_outgoing_process_messages(idx, msg_group);

		if(result != 0)
		{
			/* Step 1.2: Collect list of agents receiving the group. */
			agents = db_outgoing_process_recipients(idx);
			if(agents == NULL)
			{
				DTNMP_DEBUG_ERR("db_outgoing_process","Cannot process outgoing recipients",NULL);
				pdu_release_group(msg_group);
				return 0;
			}

			/* Step 1.3: For each agent, send a message. */
			for (elt = lyst_first(agents); elt; elt = lyst_next(elt))
			{
				agent_reg = (adm_reg_agent_t *) lyst_data(elt);
				DTNMP_DEBUG_INFO("db_outgoing_process", "Sending to name %s", agent_reg->agent_id.name);
				iif_send(&ion_ptr, msg_group, agent_reg->agent_id.name);
				msg_release_reg_agent(agent_reg);
			}

			lyst_destroy(agents);
			agents = NULL;
		}
		else
		{
			DTNMP_DEBUG_ERR("db_outgoing_process","Cannot process out going message",NULL);
			pdu_release_group(msg_group);
			return 0;
		}

		/* Step 1.4: Release the message group. */
		pdu_release_group(msg_group);
		msg_group = NULL;

		/* Step 1.5: Update the state of the message group in the database. */
		sprintf(query, "UPDATE dbtOutgoing SET State=2 WHERE ID=%d", idx);
		if (mysql_query(gConn, query)) {
			DTNMP_DEBUG_ERR("db_outgoing_process", "Database Error: %s",
					mysql_error(gConn));
			DTNMP_DEBUG_EXIT("db_outgoing_process", "-->0", NULL);
			return 0;
		}

	}

	DTNMP_DEBUG_EXIT("db_outgoing_process", "-->1", NULL);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: db_outgoing_process_messages
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

int db_outgoing_process_messages(uint32_t idx, pdu_group_t *msg_group)
{
	int result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_outgoing_process_messages","(%d, 0x%#llx)",
			          idx, (unsigned long) msg_group);

	/* Step 1: Find all messages for this outgoing group. */
	sprintf(query, "SELECT * FROM dbtOutgoingMessages WHERE OutgoingID=%d",
			idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_outgoing_process_messages", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_outgoing_process_messages", "-->%d", result);
		return result;
	}

	/* Step 2: Parse the row and populate the structure. */
	while ((res = mysql_store_result(gConn)) != NULL)
	{
		row = mysql_fetch_row(res);
		int table_idx = atoi(row[2]);
		int entry_idx = atoi(row[3]);

		if (db_outgoing_process_one_message(table_idx, entry_idx, msg_group,
				row) == 0)
		{
			DTNMP_DEBUG_ERR("db_outgoing_process_messages",
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
 * \par Function Name: db_outgoing_process_one_message
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

int db_outgoing_process_one_message(uint32_t table_idx, uint32_t entry_idx,
			                        pdu_group_t *msg_group, MYSQL_ROW row)
{
	int result = 1;

	DTNMP_DEBUG_ENTRY("db_outgoing_process_one_message","(%d, %d, 0x%#llx, 0x%#llx)",
			           table_idx, entry_idx, (unsigned long) msg_group,
			           (unsigned long) row);

	/*
	 * Step 1: Find out what kind of message we have based on the
	 *         table that is holding it.
	 */
	switch (db_fetch_table_type(table_idx, entry_idx))
	{
		case TIME_PROD_MSG:
		{
			rule_time_prod_t *entry = NULL;
			uint32_t size = 0;
			uint8_t *data = NULL;
			pdu_msg_t *pdu_msg = NULL;

			result = 0;

			if((entry = db_fetch_time_rule(entry_idx)) != NULL)
			{

				if((data = ctrl_serialize_time_prod_entry(entry, &size)) != NULL)
				{
					if((pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_PERIOD_PROD, data,
														size, NULL)) != NULL)
					{
						pdu_add_msg_to_group(msg_group, pdu_msg);
						result = 1;
					}
					else
					{
						DTNMP_DEBUG_ERR("db_outgoing_process_one_message",
								        "Cannot create msg", NULL);
					}
				}
				else
				{
					DTNMP_DEBUG_ERR("db_outgoing_process_one_message",
							        "Cannot serialize time_prod_entry",NULL);
				}

				rule_release_time_prod_entry(entry);
			}
			else
			{
				DTNMP_DEBUG_ERR("db_outgoing_process_one_message","cannot fetch time rule",NULL);
			}
		}
		break;

		case CUST_RPT:
		{
			def_gen_t *entry = NULL;
			uint32_t size = 0;
			uint8_t *data = NULL;
			pdu_msg_t *pdu_msg = NULL;

			result = 0;

			if((entry = db_fetch_def(entry_idx)) != NULL)
			{
				if((data = def_serialize_gen(entry, &size)) != NULL)
				{
					if((pdu_msg = pdu_create_msg(MSG_TYPE_DEF_CUST_RPT, data, size,
							NULL)) != NULL)
					{
						pdu_add_msg_to_group(msg_group, pdu_msg);
						result = 1;
					}
					else
					{
						DTNMP_DEBUG_ERR("db_outgoing_process_one_message",
								        "Cannot create msg", NULL);
					}
				}
				else
				{
					DTNMP_DEBUG_ERR("db_outgoing_process_one_message",
							        "Cannot serialize def_gen_t",NULL);
				}

				def_release_gen(entry);
			}
			else
			{
				DTNMP_DEBUG_ERR("db_outgoing_process_one_message",
						        "Cannot fetch definition",NULL);
			}
		}
		break;

		case EXEC_CTRL_MSG:
		{
			ctrl_exec_t *entry = NULL;
			uint32_t size = 0;
			uint8_t *data = NULL;
			pdu_msg_t *pdu_msg = NULL;

			result = 0;

			if((entry = db_fetch_ctrl(entry_idx)) != NULL)
			{
				if((data = ctrl_serialize_exec(entry, &size)) != NULL)
				{
					if((pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size,
														NULL)) != NULL)
					{
						pdu_add_msg_to_group(msg_group, pdu_msg);
						result = 1;
					}
					else
					{
						DTNMP_DEBUG_ERR("db_outgoing_process_one_message","Cannot create msg", NULL);
					}
				}
				else
				{
					DTNMP_DEBUG_ERR("db_outgoing_process_one_message","Cannot serialize control",NULL);
				}

				ctrl_release_exec(entry);
			}
			else
			{
				DTNMP_DEBUG_ERR("db_outgoing_process_one_message", "Can't construct control.", NULL);
			}
		}
		break;

		default:
		{
			DTNMP_DEBUG_ERR("db_outgoing_process_one_message",
							"Unknown table type (%d) entry_idx (%d)",
							table_idx, entry_idx);
		}
	}

	DTNMP_DEBUG_EXIT("db_outgoing_process_one_message", "-->%d", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_outgoing_process_recipients
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

Lyst db_outgoing_process_recipients(uint32_t outgoingId)
{
	Lyst result;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	adm_reg_agent_t *reg_agent = NULL;
	char query[1024];
	int cur_row = 0;
	int max_row = 0;

	DTNMP_DEBUG_ENTRY("db_outgoing_process_recipients","(%d)", outgoingId);

	result = lyst_create();

	/* Step 1: Query the database */
	sprintf(query, "SELECT AgentID FROM dbtOutgoingRecipients "
			"WHERE OutgoingID=%d", outgoingId);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_outgoing_process_recipients", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_outgoing_process_recipients", "-->", NULL);
		return NULL;
	}

	/* Step 2: Parse the results and fetch agent */
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_outgoing_process_recipients", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_outgoing_process_recipients", "-->", NULL);
		return NULL;
	}

	/* Step 3: For each row returned.... */
	max_row = mysql_num_rows(res);
	for(cur_row = 0; cur_row < max_row; cur_row++)
	{
		if ((row = mysql_fetch_row(res)) != NULL)
		{
			/* Step 3.1: Grab the agent information.. */
			if((reg_agent = db_fetch_reg_agent(atoi(row[0]))) != NULL)
			{
				DTNMP_DEBUG_INFO("db_outgoing_process_recipients",
						         "Adding agent name %s.",
						         reg_agent->agent_id.name);

				lyst_insert_last(result, reg_agent);
			}
			else
			{
				DTNMP_DEBUG_ERR("db_outgoing_process_recipients",
						        "Cannot fetch registered agent",NULL);
			}
		}
	}

	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_outgoing_process_recipients","-->0x%#llx",
			         (unsigned long) result);

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

int db_outgoing_ready(MYSQL_RES **sql_res)
{
	int result = 0;
	char query[1024];

	*sql_res = NULL;

	DTNMP_DEBUG_ENTRY("db_outgoing_ready","(0x%#llx)", (unsigned long) sql_res);

	/* Step 0: Sanity check. */
	if(sql_res == NULL)
	{
		DTNMP_DEBUG_ERR("db_outgoing_ready", "Bad Parms.", NULL);
		DTNMP_DEBUG_EXIT("db_outgoing_ready","-->0",NULL);
		return 0;
	}

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
	}

	/* Step 3: Return whether we have results waiting. */
	DTNMP_DEBUG_EXIT("db_outgoing_ready", "-->%d", result);
	return result;
}



#endif // HAVE_MYSQL
