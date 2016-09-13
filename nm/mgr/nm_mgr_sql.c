/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 ** 	This software assumes that there are no other applications modifying
 ** 	the DTNMP database tables.
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
 *****************************************************************************/

#ifdef HAVE_MYSQL

#include <string.h>

#include "nm_mgr.h"
#include "nm_mgr_sql.h"
#include "nm_mgr_names.h"

#include "../shared/adm/adm_agent.h"
#include "../shared/adm/adm_bp.h"

/* Global connection to the MYSQL Server. */
static MYSQL *gConn;
static ui_db_t gParms;



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
 * \return 0 Failure
 *        !0 The index of the inserted Agent.
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
 *****************************************************************************/

uint32_t db_add_adm(char *name, char *version, char *oid_root)
{
	char query[1024];
	uint32_t result = 0;
	oid_t *oid = NULL;
	uint32_t oid_idx = 0;
	uint8_t *data = NULL;
	uint32_t datasize = 0;

	/* Step 0: Sanity check. */
	if((name == NULL) || (version == NULL) || (oid_root == NULL))
	{
		DTNMP_DEBUG_ERR("db_add_adm","Bad Args.", NULL);
		return 0;
	}

	CHKCONN

	/* Step 1: if the adm is already in the DB, just return the index. */
	if((result = db_fetch_adm_idx(name, version)) != 0)
	{
		return result;
	}

	/* Step 2 - Build an OID to put into the DB. */

	if((data = utils_string_to_hex(oid_root,&datasize)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_adm","Can't convert OID of %s.", oid_root);
		return 0;
	}

	if((oid = oid_construct(OID_TYPE_FULL, NULL, 0, data, datasize)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_adm","Can't create OID.",NULL);
		SRELEASE(data);
		DTNMP_DEBUG_EXIT("db_add_adm","-->0",NULL);
		return 0;
	}

	SRELEASE(data);

	if((oid_idx = db_add_oid(oid)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_adm","Can't add ADM OID to DB.",NULL);
		oid_release(oid);

		DTNMP_DEBUG_EXIT("db_add_adm","-->0",NULL);
		return 0;
	}

	oid_release(oid);

	/* Step 2: Add the adm. */
	sprintf(query, "INSERT INTO dbtADMs(Label, Version, OID) "
			"VALUES('%s','%s',%d)", name, version, oid_idx);


	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_adm", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_adm", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_adm", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_adm", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_add_adm", "-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_add_agent()
 *
 * \par Adds a Registered Agent to the dbtRegisteredAgents table.
 *
 * Tables Effected:
 *    1. dbtRegisteredAgents
 *       +---------------+------------+---------------+--------------------------+
 *       | Column Object |   Type     | Default Value | Comment                  |
 *       +---------------+------------+---------------+--------------------------+
 *       |     ID*       | Int32      | (unsigned)    | Used as a primary key    |
 *       |               |            | Auto Incr.    |                          |
 *       +---------------+------------+---------------+--------------------------+
 *       |  AgentId      |VARCHAR(128)|  'ipn:0.0'    |                          |
 *       +---------------+------------+---------------+--------------------------+
 *
 * \return 0 Failure
 *        !0 The index of the inserted Agent.
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
 *****************************************************************************/
uint32_t db_add_agent(eid_t agent_eid)
{
	char query[1024];
	uint32_t result = 0;

	CHKCONN

	/* Step 1: if the agent is already in the DB, just return the index. */
	if((result = db_fetch_reg_agent_idx(&agent_eid)) != 0)
	{
		return result;
	}

	/* Step 2: Add the agent. */
	sprintf(query, "INSERT INTO dbtRegisteredAgents(AgentId) "
			"VALUES('%s')", agent_eid.name);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_agent", "Database error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_agent", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_agent", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_agent", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_add_agent", "-->%d", result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: db_add_dc
 *
 * \par Adds OID parameters to the database and returns the index of the
 *      parameters table.
 *
 * Tables Effected:
 *    1. dbtDataCollections
 *       +---------------+------------+---------------+-----------------------+
 *       | Column Object |     Type   | Default Value | Comment               |
 *       +---------------+------------+---------------+-----------------------+
 *       |      ID*      | Int32      | auto-         | Used as primary key   |
 *       |               |(unsigned)  | incrementing  |                       |
 *       +---------------+------------+---------------+-----------------------+
 *       | Label         |VARCHAR(255)| Unnamed Data  | Description...        |
 *       +---------------+------------+---------------+-----------------------+
 *
 *    2. dbtDataCollection
 *       +---------------+------------+---------------+-----------------------+
 *       | Column Object |     Type   | Default Value | Comment               |
 *       +---------------+------------+---------------+-----------------------+
 *       | CollectionID* | Int32      | 0             | Used as primary key   |
 *       |               |(unsigned)  |               |                       |
 *       +---------------+------------+---------------+-----------------------+
 *       | Data Order    | Int32      | 0             | The order the data    |
 *       |               |(unsigned)  |               | appears in list.      |
 *       +---------------+------------+---------------+-----------------------+
 *       | Data Type     | Int32      |               | Foreign key to        |
 *       |               | (unsigned) |               | lvtDataTypes.ID       |
 *       +---------------+------------+---------------+-----------------------+
 *       | DataBlob      | BLOB       |               | Binary data           |
 *       +---------------+------------+---------------+-----------------------+
 *
 * \return 0 Failure or no Parameters
 *        !0 The index of the dbtDataCollections row for this collection.
 *
 * \param[in]  dc   - The DC being added to the DB.
 * \param[in]  spec - The types of data held in the DC entry.
 *
 * \par Notes:
 *		- Comments for the dc are not included.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/22/15  E. Birrane     Initial Implementation
 *  09/10/15  E. Birrane     Update to to db spec.
 *****************************************************************************/

uint32_t db_add_dc(Lyst dc, ui_parm_spec_t *spec)
{
	char query[1024];
	char *query2 = NULL;
	uint32_t result = 0;
	char *content = NULL;
	uint32_t content_len = 0;
	blob_t *entry = NULL;
	dtnmp_type_e type;
	LystElt elt;
	int i = 0;

	DTNMP_DEBUG_ENTRY("db_add_dc", "("ADDR_FIELDSPEC", %d)",
					  (uaddr) entry, type);

	/* Step 0: Sanity check arguments. */
	if(dc == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_dc","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_dc","-->0",NULL);
		return 0;
	}

	CHKCONN

	/*
	 * Step 1: Build and execute query to add row to dbtDataCollections. Also, store the
	 *         row ID of the inserted row.
	 */
	sprintf(query,
			"INSERT INTO dbtDataCollections (Label)"
			"VALUE (NULL)");

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_dc", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_dc", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_dc", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_dc", "-->0", NULL);
		return 0;
	}

	for(elt = lyst_first(dc); elt; elt = lyst_next(elt))
	{
		entry = (blob_t *) lyst_data(elt);
		type = spec->parm_type[i];
		i++;

		if((content = utils_hex_to_string(entry->value, entry->length)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_add_dc","Can't cvt %d bytes to hex str.", entry->length);
			DTNMP_DEBUG_EXIT("db_add_dc", "-->0", NULL);
			return 0;
		}

		content_len = strlen(content);
		if((query2 = (char *) STAKE(content_len + 256)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_add_dc","Can't alloc %d bytes.",
					         content_len + 256);
			SRELEASE(content);

			DTNMP_DEBUG_EXIT("db_add_dc", "-->0", NULL);
			return 0;
		}

		/*
		 * Content starts with "0x" which we do not want in the DB
		 * so we skip over the first 2 characters when making the query.
		 */
		sprintf(query2,"INSERT INTO dbtDataCollection"
				        "(CollectionID, DataOrder, DataType,DataBlob)"
				 	    "VALUES(%d,1,%d,'%s')",result,type,content+2);
		SRELEASE(content);

		if (mysql_query(gConn, query2))
		{
			DTNMP_DEBUG_ERR("db_add_dc", "Database Error: %s", mysql_error(gConn));
			SRELEASE(query2);
			return 0;
		}

		SRELEASE(query2);
	}

	DTNMP_DEBUG_EXIT("db_add_mid", "-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_add_mid
 *
 * \par Creates a MID in the database.
 *
 * Tables Effected:
 *    1. dbtMIDs
 *       +--------------+---------------------+------+-----+----------+-------------+
 *       | Field        | Type                | Null | Key | Default  | Extra       |
 *       +--------------+---------------------+------+-----+----------+-------------+
 *       | ID           | int(10) unsigned    | NO   | PRI | NULL     | auto_incr   |
 *       | NicknameID   | int(10) unsigned    | YES  | MUL | NULL     |             |
 *       | OID          | int(10) unsigned    | NO   | MUL | NULL     |             |
 *       | ParametersID | int(10) unsigned    | YES  | MUL | NULL     |             |
 *       | Type         | int(10) unsigned    | NO   | MUL | NULL     |             |
 *       | Category     | int(10) unsigned    | NO   | MUL | NULL     |             |
 *       | IssuerFlag   | bit(1)              | NO   |     | b'0'     |             |
 *       | TagFlag      | bit(1)              | NO   |     | b'0'     |             |
 *       | OIDType      | int(10) unsigned    | YES  | MUL | NULL     |             |
 *       | IssuerID     | bigint(20) unsigned | NO   |     | 0        |             |
 *       | TagValue     | bigint(20) unsigned | NO   |     | 0        |             |
 *       | DataType     | int(10) unsigned    | NO   | MUL | NULL     |             |
 *       | Name         | varchar(50)         | NO   |     | Unnamed  |             |
 *       | Description  | varchar(255)        | NO   |     | None     |             |
 *       +--------------+---------------------+------+-----+----------+-------------+
 *
 * \retval 0  Failure
 *         >0 The index of the inserted MID from the dbtMIDs table.
 *
 * \param[in] mid     - The MID to be persisted in the DB.
 * \param[in] spec    - Parameter spec defining parameters for this MID.
 * \param[in] type    - The type of the MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/12/13  S. Jacobs      Initial implementation,
 *  08/23/15  E. Birrane     Update to new DB Schema.
 *****************************************************************************/
uint32_t db_add_mid(mid_t *mid, ui_parm_spec_t *spec, dtnmp_type_e type)
{
	char query[1024];
	uint32_t result = 0;
	uint32_t nn_idx = 0;
	uint32_t oid_idx = 0;
	uint32_t parm_idx = 0;
	uint32_t num_parms = 0;

	DTNMP_DEBUG_ENTRY("db_add_mid", "("ADDR_FIELDSPEC")", (uaddr)mid);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_mid","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_mid","-->0",NULL);
		return 0;
	}

	CHKCONN

	/* Step 1: Make sure the ID is not already in the DB. */
	if ((result = db_fetch_mid_idx(mid)) > 0)
	{
		DTNMP_DEBUG_EXIT("db_add_mid", "-->%d", result);
		return result;
	}

	/* Step 2: If this MID has a nickname, grab the index. */
	if((MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_FULL) ||
	   (MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_PARAM))
	{
		if((nn_idx = db_fetch_nn_idx(mid->oid->nn_id)) == 0)
		{
			DTNMP_DEBUG_ERR("db_add_mid","MID references unknown Nickname %d", mid->oid->nn_id);
			DTNMP_DEBUG_EXIT("db_add_mid", "-->0", NULL);
			return 0;
		}
	}

	/* Step 3: Get the index for the OID. */
	if((oid_idx = db_add_oid(mid->oid)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_mid", "Can't add OID.", NULL);
		DTNMP_DEBUG_EXIT("db_add_mid", "-->0", NULL);
		return 0;
	}

	/* Step 4: Get the index for parameters, if any. */
	if((num_parms = oid_get_num_parms(mid->oid)) > 0)
	{
		parm_idx = db_add_parms(mid->oid, spec);
	}

	/*
	 * Step 5: Build and execute query to add row to dbtMIDs. Also, store the
	 *         row ID of the inserted row.
	 */
	sprintf(query,
			"INSERT INTO dbtMIDs"
			"(NicknameID,OID,ParametersID,Type,Category,IssuerFlag,TagFlag,"
			"OIDType,IssuerID,TagValue,DataType,Name,Description)"
			"VALUES (%s, %d, %s, %d, %d, %d, %d, %d, "UVAST_FIELDSPEC","UVAST_FIELDSPEC",%d,'%s','%s')",
			(nn_idx == 0) ? "NULL" : itoa(nn_idx),
			oid_idx,
			(parm_idx == 0) ? "NULL" : itoa(parm_idx),
			MID_GET_FLAG_TYPE(mid->flags),
			MID_GET_FLAG_CAT(mid->flags),
			(MID_GET_FLAG_ISS(mid->flags)) ? 1 : 0,
			(MID_GET_FLAG_TAG(mid->flags)) ? 1 : 0,
			MID_GET_FLAG_OID(mid->flags),
			mid->issuer,
			mid->tag,
			type,
			"No Name",
			"No Descr");

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

	DTNMP_DEBUG_EXIT("db_add_mid", "-->%d", result);
	return result;
}




/******************************************************************************
 *
 * \par Function Name: db_add_mc
 *
 * \par Creates a MID Collection in the database.
 *
 * Tables Effected:
 *    1. dbtMIDCollections
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
 * \retval 0  Failure
 *         >0 The index of the inserted MC from the dbtMIDCollections table.
 *
 * \param[in] mc     - The MC being added to the DB.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/23/15  E. Birrane     Initial Implementation
 *****************************************************************************/
uint32_t db_add_mc(Lyst mc)
{
	char query[1024];
	uint32_t result = 0;
	LystElt elt = NULL;
	mid_t *mid;
	uint32_t i = 0;
	uint32_t mid_idx = 0;

	DTNMP_DEBUG_ENTRY("db_add_mc", "("ADDR_FIELDSPEC")", (uaddr)mc);

	/* Step 0 - Sanity check arguments. */
	if(mc == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_mc","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_mc","-->0",NULL);
		return 0;
	}

	CHKCONN

	/* Step 1 - Create a new entry in the dbtMIDCollections DB. */
	sprintf(query,
			"INSERT INTO dbtMIDCollections (Comment) VALUES ('No Comment')");

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_mc", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_mc", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_mc", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_mc", "-->0", NULL);
		return 0;
	}

	/* Step 2 - For each MID in the MC, add the MID into the dbtMIDCollection. */
	for(elt = lyst_first(mc); elt; elt = lyst_next(elt))
	{
		if((mid = (mid_t *) lyst_data(elt)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_add_mc","Can't get MID.", NULL);
			DTNMP_DEBUG_EXIT("db_add_mc", "-->0", NULL);
			return 0;
		}

		if((mid_idx = db_fetch_mid_idx(mid)) == 0)
		{
			DTNMP_DEBUG_ERR("db_add_mc","Can't get MID Idx.", NULL);
			DTNMP_DEBUG_EXIT("db_add_mc", "-->0", NULL);
			return 0;
		}

		sprintf(query,
				"INSERT INTO dbtMIDCollection"
				"(CollectionID, MIDID, MIDOrder)"
				"VALUES (%d, %d, %d",
				result, mid_idx, i);

		if (mysql_query(gConn, query))
		{
			DTNMP_DEBUG_ERR("db_add_mc", "Database Error: %s", mysql_error(gConn));
			DTNMP_DEBUG_EXIT("db_add_mc", "-->0", NULL);
			return 0;
		}
		i++;
	}

	DTNMP_DEBUG_EXIT("db_add_mc", "-->%d", result);
	return result;
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
 * \retval 0  Failure
 *         >0 The index of the inserted NN from the dbtADMNicknames table.
 *
 * \param[in] nn     - The Nickname being added to the DB.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial Implementation
 *****************************************************************************/

uint32_t db_add_nn(oid_nn_t *nn)
{
	char query[1024];
	uint32_t result = 0;
	oid_t *oid = NULL;
	uint32_t oid_idx = 0;
	uint32_t adm_idx = 0;

	DTNMP_DEBUG_ENTRY("db_add_nn", "("ADDR_FIELDSPEC")", (uaddr)nn);

	/* Step 0 - Sanity check arguments. */
	if(nn == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_nn","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_nn","-->0",NULL);
		return 0;
	}

	CHKCONN

	/*
	 * Step 1 - See if this nickname is already in the db.
	 * If so, then return that as the index.
	 */
	if((result = db_fetch_nn_idx(nn->id)) != 0)
	{
		DTNMP_DEBUG_EXIT("db_add_nn","-->%d", result);
		return result;
	}

	/* Step 2 - Add the nickname's OID into the OID table. */
	if((oid = oid_construct(OID_TYPE_FULL, NULL, 0, nn->raw, nn->raw_size)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_nn","Can't create OID.",NULL);
		DTNMP_DEBUG_EXIT("db_add_nn","-->0",NULL);
		return 0;
	}

	if((oid_idx = db_add_oid(oid)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_nn","Can't add nickname OID to DB.",NULL);
		oid_release(oid);

		DTNMP_DEBUG_EXIT("db_add_nn","-->0",NULL);
		return 0;
	}

	oid_release(oid);

	if((adm_idx = db_fetch_adm_idx(nn->adm_name, nn->adm_ver)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_nn","Can't Find ADM.",NULL);

		DTNMP_DEBUG_EXIT("db_add_nn","-->0",NULL);
		return 0;
	}

	/* Step 3 - Create a new entry in the dbtADMNicknames DB. */
	sprintf(query,
			"INSERT INTO dbtADMNicknames (ADM_ID, Nickname_UID, Nickname_Label, OID)"
			"VALUES (%d, "UVAST_FIELDSPEC", 'No Comment', %d)",
			adm_idx, nn->id, oid_idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_nn", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_nn", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_nn", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_nn", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_add_nn", "-->%d", result);
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
 * \return 0 Failure
 *        !0 The index of the inserted OID.
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
 *****************************************************************************/

uint32_t db_add_oid(oid_t *oid)
{
	char query[1024];
	uint32_t result = 0;
	uint32_t num_parms = 0;
	char *oid_str = NULL;

	DTNMP_DEBUG_ENTRY("db_add_oid", "("ADDR_FIELDSPEC")",
					  (uaddr) oid);

	/* Step 0: Sanity check arguments. */
	if(oid == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_oid","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_oid","-->0",NULL);
		return 0;
	}

	CHKCONN

	/*
	 * Step 1: Make sure the ID is not already in the DB.
	 * If it is, we are done.
	 */
	if ((result = db_fetch_oid_idx(oid)) > 0)
	{
		DTNMP_DEBUG_EXIT("db_add_oid","-->%d", result);
		return result;
	}

	if((oid_str = oid_to_string(oid)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_oid","Can't get string rep of OID.", NULL);
		DTNMP_DEBUG_EXIT("db_add_oid","-->0",NULL);
		return 0;
	}

	/*
	 * Step 2: Build and execute query to add row to dbtOIDs. Also, store the
	 *         row ID of the inserted row.
	 */
	sprintf(query,
			"INSERT INTO dbtOIDs"
			"(IRI_Label, Dot_Label, Encoded, Description)"
			"VALUES ('empty','empty','%s','empty')",
		    oid_str);

	SRELEASE(oid_str);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_oid", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_oid", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_oid", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_oid", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_add_oid", "-->%d", result);
	return result;
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
 * \return 0 Failure or no Parameters
 *        !0 The index of the dbtMIDParmaters row for these parameters.
 *
 * \param[in]  oid  - The OID whose parameters are being added to the DB.
 * \param[in]  spec - The parm spec that gives the types of OID parms.
 *
 * \par Notes:
 *		- Comments for the parameters are not included.
 *		- A return of 0 is only an error if the oid has parameters.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/22/15  E. Birrane     Initial Implementation
 *  09/10/15  E. Birrane     Removed dbtMIDParameters
 *****************************************************************************/

uint32_t db_add_parms(oid_t *oid, ui_parm_spec_t *spec)
{
	char query[1024];

	uint32_t i = 0;
	uint32_t num_parms = 0;
	uint32_t result = 0;

	DTNMP_DEBUG_ENTRY("db_add_parms", "("ADDR_FIELDSPEC")",
					  (uaddr) oid);

	/* Step 0: Sanity check arguments. */
	if((oid == NULL) || (spec == NULL))
	{
		DTNMP_DEBUG_ERR("db_add_parms","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_parms","-->0",NULL);
		return 0;
	}

	CHKCONN

	return db_add_dc(oid->params, spec);
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
 * \retval 0  Failure
 *         >0 The index of the inserted MID from the dbtMIDs table.
 *
 * \param[in] mid     - The MID to be persisted in the DB.
 * \param[in] spec    - Parameter spec defining parameters types for this MID.
 * \param[in] type    - The type of the MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/28/15  E. Birrane     Initial implementation,
 *****************************************************************************/
uint32_t db_add_protomid(mid_t *mid, ui_parm_spec_t *spec, dtnmp_type_e type)
{
	char query[1024];
	uint32_t result = 0;
	uint32_t nn_idx = 0;
	uint32_t oid_idx = 0;
	uint32_t parm_idx = 0;
	uint32_t num_parms = 0;

	DTNMP_DEBUG_ENTRY("db_add_protomid",
			          "("ADDR_FIELDSPEC","ADDR_FIELDSPEC",%d)",
			          (uaddr)mid, (uaddr) spec, type);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_protomid","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_protomid","-->0",NULL);
		return 0;
	}

	CHKCONN

	/* Step 1: Make sure the ID is not already in the DB. */
	if ((result = db_fetch_protomid_idx(mid)) > 0)
	{
		DTNMP_DEBUG_WARN("db_add_protomid", "Already in DB. Returning.", NULL);
		DTNMP_DEBUG_EXIT("db_add_protomid", "-->%d", result);
		return result;
	}

	/* Step 2: If this MID has a nickname, grab the index. */
	if((MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_FULL) ||
	   (MID_GET_FLAG_OID(mid->flags) == OID_TYPE_COMP_PARAM))
	{
		if((nn_idx = db_fetch_nn_idx(mid->oid->nn_id)) == 0)
		{
			DTNMP_DEBUG_ERR("db_add_protomid","MID references unknown Nickname %d", mid->oid->nn_id);
			DTNMP_DEBUG_EXIT("db_add_protomid", "-->0", NULL);
			return 0;
		}
	}

	/* Step 3: Get the index for the OID. */
	if((oid_idx = db_add_oid(mid->oid)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_protomid", "Can't add OID.", NULL);
		DTNMP_DEBUG_EXIT("db_add_protomid", "-->0", NULL);
		return 0;
	}

	/* Step 4: Get the index for parameters, if any. */
	if((parm_idx = db_add_protoparms(spec)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_protomid", "Can't add protoparms.", NULL);
		DTNMP_DEBUG_EXIT("db_add_protomid", "-->0", NULL);
		return 0;
	}

	/*
	 * Step 5: Build and execute query to add row to dbtMIDs. Also, store the
	 *         row ID of the inserted row.
	 */
	sprintf(query,
			"INSERT INTO dbtProtoMIDs"
			"(NicknameID,OID,ParametersID,Type,Category,"
			"OIDType,DataType,Name,Description)"
			"VALUES (%s, %d, %d, %d, %d, %d, %d, '%s','%s')",
			(nn_idx == 0) ? "NULL" : itoa(nn_idx),
			oid_idx,
			parm_idx,
			MID_GET_FLAG_TYPE(mid->flags),
			MID_GET_FLAG_CAT(mid->flags),
			MID_GET_FLAG_OID(mid->flags),
			type,
			"No Name",
			"No Descr");

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_protomid", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_protomid", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_protomid", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_protomid", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_add_protomid", "-->%d", result);
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
 * \return 0 Failure or no Parameters
 *        !0 The index of the dbtProtoMIDParmaters row for these parameters.
 *
 * \param[in]  spec - The parm spec that gives the types of OID parms.
 *
 * \par Notes:
 *		- Comments for the parameters are not included.
 *		- A return of 0 is only an error if the oid has parameters.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/28/15  E. Birrane     Initial Implementation
 *****************************************************************************/

uint32_t db_add_protoparms(ui_parm_spec_t *spec)
{
	char query[1024];

	uint32_t i = 0;
	uint32_t num_parms = 0;
	uint32_t result = 0;

	DTNMP_DEBUG_ENTRY("db_add_protoparms", "("ADDR_FIELDSPEC")",
					  (uaddr) spec);

	/* Step 0: Sanity check arguments. */
	if(spec == NULL)
	{
		DTNMP_DEBUG_ERR("db_add_protoparms","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_add_protoparms","-->0",NULL);
		return 0;
	}

	CHKCONN

	if((spec->num_parms == 0) || (spec->num_parms >= MAX_PARMS))
	{
		DTNMP_DEBUG_ERR("db_add_protoparms","Bad # parms.",NULL);
		DTNMP_DEBUG_EXIT("db_add_protoparms","-->0",NULL);
		return 0;
	}


	/* Step 1: Add an entry in the parameters table. */
	sprintf(query,
			"INSERT INTO dbtProtoMIDParameters (Comment) "
			"VALUES ('No comment')");

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_add_protoparms", "Database Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_add_protoparms", "-->0", NULL);
		return 0;
	}

	if((result = (uint32_t) mysql_insert_id(gConn)) == 0)
	{
		DTNMP_DEBUG_ERR("db_add_protoparms", "Unknown last inserted row.", NULL);
		DTNMP_DEBUG_EXIT("db_add_protoparms", "-->0", NULL);
		return 0;
	}

	/* Step 2: For each parameter, get the DC, add the DC into the DB,
	 * and then add an entry into dbtMIDParameter
	 */

	for(i = 0; i < spec->num_parms; i++)
	{

		/* Step 2.2: Add entry into dbtMIDParameter */
		sprintf(query,
				"INSERT INTO dbtProtoMIDParameter "
				"(CollectionID, ParameterOrder, ParameterTypeID) "
				"VALUES (%d, %d, %d)",
				result, i, spec->parm_type[i]);

		if (mysql_query(gConn, query))
		{
			DTNMP_DEBUG_ERR("db_add_protoparms", "Database Error: %s", mysql_error(gConn));
			DTNMP_DEBUG_EXIT("db_add_protoparms", "-->0", NULL);
			return 0;
		}
	}

	DTNMP_DEBUG_EXIT("db_add_protoparms", "-->%d", result);
	return result;
}


/******************************************************************************
 * \par Function Name: db_fetch_adm_idx
 *
 * \par Gets the ADM index given an ADM description
 *
 * \retval 0 Failure
 *        !0 The ADM index.
 *
 * \param[in] name    - The ADM name.
 * \param[in] version - The ADM version.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation,
 *****************************************************************************/

uint32_t db_fetch_adm_idx(char *name, char *version)
{
	uint32_t result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_adm_idx","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					  (uaddr)name, (uaddr) version);

	if((name == NULL) || (version == NULL))
	{
		DTNMP_DEBUG_ERR("db_fetch_adm_idx","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_adm_idx","-->0", NULL);
		return 0;
	}

	CHKCONN

	sprintf(query, "SELECT * FROM dbtADMs WHERE Label='%s' AND Version='%s'",
			name, version);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_adm_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_adm_idx", "-->0", NULL);
		return 0;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_adm_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_adm_idx", "-->0", NULL);
		return 0;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_adm_idx","-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: db_fetch_dc
 *
 * \par Creates a data collection from dbtDataCollections in the database
 *
 * \retval NULL Failure
 *        !NULL The built Data collection.
 *
 * \param[in] id - The Primary Key in the dbtDataCollections table.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/23/13  S. Jacobs      Initial implementation,
 *  08/23/15  E. Birrane     Update to new schema.
 *****************************************************************************/
Lyst db_fetch_dc(int dc_idx)
{
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	Lyst result;
	blob_t *dc_entry;

	DTNMP_DEBUG_ENTRY("db_fetch_dc", "(%d)", dc_idx);

	CHKCONN

	/* Step 1: Construct/run the Query and capture results. */
	sprintf(query, "SELECT * FROM dbtDataCollection "
			"WHERE CollectionID=%d "
			"ORDER BY DataOrder",
			dc_idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_dc", "SQL Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_dc", "-->NULL", NULL);
		return NULL;
	}

	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_dc", "SQL Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_dc", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Allocate a Lyst to hold the collection. */
	if((result = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_dc", "Can't alloc lyst", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_dc", "-->NULL", NULL);
		return NULL;
	}

	/* Step 3: For each entry returned as part of the collection. */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		if((dc_entry = db_fetch_data_col_entry_from_row(row)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_dc", "Can't get entry.", NULL);
			dc_destroy(&result);
			mysql_free_result(res);

			DTNMP_DEBUG_EXIT("db_fetch_dc","-->NULL",NULL);
			return NULL;
		}

		lyst_insert_last(result, dc_entry);
	}

	/* Step 4: Free results. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_dc", "-->"ADDR_FIELDSPEC, (uaddr) result);
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

blob_t *db_fetch_data_col_entry_from_row(MYSQL_ROW row)
{
	blob_t *result = NULL;

	DTNMP_DEBUG_ENTRY("db_fetch_data_col_entry_from_row","(0x%#llx)",
					  (unsigned long) row);

	CHKCONN

	/* Step 1: Allocate space for the entry. */
	if((result = (blob_t*) STAKE(sizeof(blob_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry_from_row",
				        "Can't allocate %d bytes.",
					    sizeof(blob_t));

		DTNMP_DEBUG_EXIT("db_fetch_data_col_entry_from_row", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Populate the entry. */

	result->value = utils_string_to_hex(row[3], &(result->length));

	if((result->length == 0) || (result->value == 0))
	{
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry_from_row",
				        "length : %lu",result->length);
		DTNMP_DEBUG_ERR("db_fetch_data_col_entry_from_row",
				        "value : %lu",result->value);

		SRELEASE(result);

		DTNMP_DEBUG_EXIT("db_fetch_data_col_entry_from_row", "-->NULL", NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("db_fetch_data_col_entry_from_row", "-->%0x#llx",
			         (unsigned long) result);
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
 *****************************************************************************/

mid_t *db_fetch_mid(int idx)
{
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	mid_t *result = NULL;

	DTNMP_DEBUG_ENTRY("db_fetch_mid", "(%d)", idx);

	CHKCONN

	/* Step 1: Construct and run the query to get the MID information. */
	sprintf(query, "SELECT * FROM dbtMIDs WHERE ID=%d", idx);

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

		oid_t *oid = NULL;

		if((oid = db_fetch_oid(nn_idx, parm_idx, oid_idx)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_mid","Cannot fetch the oid: %d", oid_idx);
		}
		else
		{
			oid->type = oidType;
			if ((result = mid_construct(type,
					                    cat,
					                    issFlag ? &issuer : NULL,
					                    tagFlag ? &tag : NULL,
					                    oid)) == NULL)
			{
				DTNMP_DEBUG_ERR("db_fetch_mid", "Cannot construct MID", NULL);
			}

			/* mid_construct deep-copies the OID. We can release it either way. */
			oid_release(oid);
		}
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_mid", "Did not find MID with ID of %d\n", idx);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	/* Step 4: Sanity check the returned MID. */
	if (mid_sanity_check(result) == 0)
	{
		char *data = mid_pretty_print(result);
		DTNMP_DEBUG_ERR("db_fetch_mid", "Failed MID sanity check. %s", data);
		SRELEASE(data);
		mid_release(result);
		result = NULL;
	}

	DTNMP_DEBUG_EXIT("db_fetch_mid", "-->"ADDR_FIELDSPEC, (uaddr) result);
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
 *  07/12/13  S. Jacobs      Initial implementation
 *  08/23/15  E. Birrane     Update to new database schema
 *****************************************************************************/
Lyst db_fetch_mid_col(int idx)
{
	Lyst result = lyst_create();
	mid_t *new_mid = NULL;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_mid_col","(%d)", idx);

	CHKCONN

	/* Step 1: Construct and run the query to get MC DB info. */
	sprintf(query,
			"SELECT MIDID FROM dbtMIDCollection WHERE CollectionID=%d ORDER BY MIDOrder",
			idx);

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
 *****************************************************************************/

uint32_t db_fetch_mid_idx(mid_t *mid)
{
	char query[1024];
	char query2[1024];
	uint32_t result = 0;
	uint32_t cur_idx = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_mid_idx","("ADDR_FIELDSPEC")", (uaddr)mid);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx","Bad args",NULL);
		return 0;
	}

	CHKCONN

	/* Step 1: Build and execute query. */
	sprintf(query,
			"SELECT * FROM dbtMIDs WHERE "
			"Type=%d AND Category=%d AND IssuerFlag=%d AND TagFlag=%d "
			"AND OIDType=%d AND IssuerID="UVAST_FIELDSPEC" "
			"AND TagValue="UVAST_FIELDSPEC,
			MID_GET_FLAG_TYPE(mid->flags),
			MID_GET_FLAG_CAT(mid->flags),
			(MID_GET_FLAG_ISS(mid->flags)) ? 1 : 0,
			(MID_GET_FLAG_TAG(mid->flags)) ? 1 : 0,
			MID_GET_FLAG_OID(mid->flags),
			mid->issuer,
			mid->tag);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->0", 0);
		return 0;
	}

	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx", "SQL Error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->NULL", NULL);
		return 0;
	}

	/* Step 2: For each matching MID, check other items... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		cur_idx = (row[0] == NULL) ? 0 : atoi(row[0]);

		uint32_t nn_idx = (row[1] == NULL) ? 0 : atoi(row[1]);
		uint32_t oid_idx = (row[2] == NULL) ? 0 : atoi(row[2]);
		uint32_t parm_idx = (row[3] == NULL) ? 0 : atoi(row[3]);
		oid_t *oid = NULL;


		oid = db_fetch_oid(nn_idx, parm_idx, oid_idx);

		if(oid_compare(oid, mid->oid, 1) == 0)
		{
			oid_release(oid);
			result = cur_idx;
			break;
		}

		oid_release(oid);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	/* Step 4: Return the IDX. */
	DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->%d", result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_nn
 *
 * \par Gets the nickname UID given a primary key index into the Nickname table.
 *
 * \retval 0 Failure
 *        !0 The nickname UID.
 *
 * \param[in] idx  - Index of the nickname UID being queried.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *****************************************************************************/

uint32_t db_fetch_nn(uint32_t idx)
{
	uint32_t result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_nn","(%d)", idx);

	if(idx == 0)
	{
		DTNMP_DEBUG_ERR("db_fetch_nn","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_nn","-->0", NULL);
		return 0;
	}

	CHKCONN

	sprintf(query, "SELECT * FROM dbtADMNicknames WHERE ID=%d", idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_nn", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_nn", "-->0", NULL);
		return 0;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_nn", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_nn", "-->0", NULL);
		return 0;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[2]);
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_nn", "Did not find NN with ID of %d\n", idx);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_nn","-->%d", result);
	return result;
}



/******************************************************************************
 * \par Function Name: db_fetch_nn_idx
 *
 * \par Gets the index of a nickname UID.
 *
 * \retval 0 Failure
 *        !0 The nickname index.
 *
 * \param[in] nn  - The nickname UID whose index is being queried.
 *
 *  Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *****************************************************************************/

uint32_t db_fetch_nn_idx(uint32_t nn)
{
	uint32_t result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_nn_idx","(%d)", nn);

	CHKCONN

	sprintf(query, "SELECT * FROM dbtADMNicknames WHERE Nickname_UID=%d", nn);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_nn_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_nn_idx", "-->0", NULL);
		return 0;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_nn_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_nn_idx", "-->0", NULL);
		return 0;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_nn_idx","-->%d", result);
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
 *****************************************************************************/

uint8_t* db_fetch_oid_val(uint32_t idx, uint32_t *size)
{
	uint8_t *result = NULL;
	char query[1024];
	char valstr[256];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_oid_val","(%d,"ADDR_FIELDSPEC")",
			          idx, (uaddr)size);

	if((idx == 0) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_val","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_oid_val","-->NULL", NULL);
		return NULL;
	}

	CHKCONN

	sprintf(query, "SELECT Encoded FROM dbtOIDs WHERE ID=%d", idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_val", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_oid_val", "-->NULL", NULL);
		return NULL;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_val", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_oid_val", "-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = utils_string_to_hex(row[0],size);
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_val", "Did not find OID with ID of %d\n", idx);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_oid_val","-->%d", result);
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
 ******************************************************************************/

oid_t *db_fetch_oid(uint32_t nn_idx, uint32_t parm_idx, uint32_t oid_idx)
{
	oid_t *result = NULL;
	Lyst parms = NULL;
	uint32_t nn_id = 0;
	uint32_t val_size = 0;
	uint8_t *val = NULL;
	uint32_t oid_type = OID_TYPE_FULL;

	DTNMP_DEBUG_ENTRY("db_fetch_oid","(%d, %d, %d)",
					  nn_idx, parm_idx, oid_idx);

	/* Step 0: Sanity Check. */
	if(oid_idx == 0)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_oid","-->NULL", NULL);
		return NULL;
	}

	CHKCONN

	/* Step 1: Grab the OID value string. */
	if((val = db_fetch_oid_val(oid_idx, &val_size)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid","Can't get OID for idx %d.", oid_idx);
		DTNMP_DEBUG_EXIT("db_fetch_oid","-->NULL", NULL);
		return NULL;
	}

	/* Step 2: Grab parameters, if the OID has them. */
	if(parm_idx > 0)
	{
		parms = db_fetch_dc(parm_idx);

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
	result = oid_construct(oid_type, parms, nn_id, val, val_size);

	if(val != NULL)
	{
		SRELEASE(val);
	}
	if(parms != NULL)
	{
		dc_destroy(&parms);
	}

	DTNMP_DEBUG_EXIT("db_fetch_oid","-->"ADDR_FIELDSPEC, (uaddr)result);
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
 ******************************************************************************/

uint32_t db_fetch_oid_idx(oid_t *oid)
{

	uint32_t result = 0;
	char query[1024];
	char *oid_str = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_oid_idx","("ADDR_FIELDSPEC")", (uaddr)oid);

	if(oid == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_idx","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_oid_idx","-->0", NULL);
		return 0;
	}

	CHKCONN

	if((oid_str = oid_to_string(oid)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_idx","Can't get string rep of OID.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_oid_idx","-->0",NULL);
		return 0;
	}

	sprintf(query, "SELECT * FROM dbtOIDs WHERE Encoded='%s'", oid_str);

	SRELEASE(oid_str);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_oid_idx", "-->0", NULL);
		return 0;
	}

	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_oid_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_oid_idx", "-->0", NULL);
		return 0;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_oid_idx","-->%d", result);
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
 *****************************************************************************/

Lyst db_fetch_parms(uint32_t idx)
{
	char query[1024];
	Lyst result = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	uint32_t dc_idx = 0;
	blob_t* entry = NULL;

	DTNMP_DEBUG_ENTRY("db_fetch_parms", "(%d)", idx);

	/* Step 0: Sanity check arguments. */
	if(idx == 0)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms","Bad args",NULL);
		DTNMP_DEBUG_EXIT("db_fetch_parms","-->NULL",NULL);
		return NULL;
	}

	CHKCONN

	/* Step 1: Allocate the return lyst. */
	if((result = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_parms","Can't allocate lyst",NULL);
		DTNMP_DEBUG_EXIT("db_fetch_parms","-->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab all of the DC IDs Associated with this parm set. */
	sprintf(query,
			"SELECT DataCollectionID FROM dbtMIDParameter "
			"WHERE CollectionID=%d ORDER BY ItemOrder",
			idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->0", 0);
		return 0;
	}

	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_mid_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_mid_idx", "-->0", NULL);
		return 0;
	}

	/* Step 3: For each matching parameter... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		if((entry = db_fetch_data_col_entry_from_row(row)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_fetch_dc", "Can't get entry.", NULL);
			dc_destroy(&result);
			mysql_free_result(res);

			DTNMP_DEBUG_EXIT("db_fetch_dc","-->NULL",NULL);
			return NULL;
		}

		lyst_insert_last(result, entry);
	}

	/* Step 4: Free results. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_parms", "-->"ADDR_FIELDSPEC, (uaddr)result);
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
 *****************************************************************************/

uint32_t db_fetch_protomid_idx(mid_t *mid)
{
	char query[1024];
	char query2[1024];
	uint32_t result = 0;
	uint32_t cur_idx = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_protomid_idx","("ADDR_FIELDSPEC")", (uaddr)mid);

	/* Step 0: Sanity check arguments. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_protomid_idx","Bad args",NULL);
		return 0;
	}

	CHKCONN

	/* Step 1: Build and execute query. */
	sprintf(query,
			"SELECT * FROM dbtProtoMIDs WHERE "
			"Type=%d AND Category=%d AND OIDType=%d",
			MID_GET_FLAG_TYPE(mid->flags),
			MID_GET_FLAG_CAT(mid->flags),
			MID_GET_FLAG_OID(mid->flags));

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_protomid_idx", "Database Error: %s",
				mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_protomid_idx", "-->0", 0);
		return 0;
	}

	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_protomid_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_protomid_idx", "-->0", NULL);
		return 0;
	}


	/* Step 2: For each matching MID, check other items... */
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		cur_idx = atoi(row[0]);

		uint32_t nn_idx = atoi(row[1]);
		uint32_t oid_idx = atoi(row[2]);
		oid_t *oid = NULL;

		oid = db_fetch_oid(nn_idx, 0, oid_idx);

		if(oid_compare(oid, mid->oid, 0) == 0)
		{
			oid_release(oid);
			result = cur_idx;
			break;
		}

		oid_release(oid);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	/* Step 4: Return the IDX. */
	DTNMP_DEBUG_EXIT("db_fetch_protomid_idx", "-->%d", result);
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

adm_reg_agent_t *db_fetch_reg_agent(uint32_t id)
{
	adm_reg_agent_t *result = NULL;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	char query[1024];

	DTNMP_DEBUG_ENTRY("db_fetch_reg_agent","(%d)", id);

	CHKCONN

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
		strncpy(eid.name, row[1], AMP_MAX_EID_LEN);

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
 *****************************************************************************/

uint32_t db_fetch_reg_agent_idx(eid_t *eid)
{
	uint32_t result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	DTNMP_DEBUG_ENTRY("db_fetch_reg_agent_idx","("ADDR_FIELDSPEC")", (uaddr) eid);

	if(eid == 0)
	{
		DTNMP_DEBUG_ERR("db_fetch_reg_agent_idx","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent_idx","-->0", NULL);
		return 0;
	}

	CHKCONN

	sprintf(query, "SELECT * FROM dbtRegisteredAgents WHERE AgentId='%s'", eid->name);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_fetch_reg_agent_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent_idx", "-->0", NULL);
		return 0;
	}
	if((res = mysql_store_result(gConn)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_fetch_reg_agent_idx", "Database error: %s",
				        mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_fetch_reg_agent_idx", "-->0", NULL);
		return 0;
	}

	/* Step 2: Parse information out of the returned row. */
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		result = atoi(row[0]);
	}
	else
	{
		DTNMP_DEBUG_ERR("db_fetch_reg_agent_idx", "Did not find EID with ID of %s\n", eid->name);
	}

	/* Step 3: Free database resources. */
	mysql_free_result(res);

	DTNMP_DEBUG_EXIT("db_fetch_reg_agent_idx","-->%d", result);
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
 * \param[in] timestamp  - the generated timestamp
 * \param[in] sender_eid - Who sent the messages.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/07/13  S. Jacobs      Initial implementation,
 *  08/29/15  E. Birrane     Added sender EID.
 *****************************************************************************/

int db_incoming_initialize(time_t timestamp, eid_t *sender_eid)
{
	MYSQL_RES *res = NULL;
    MYSQL_ROW row;
	char query[1024];
	int result = 0;
	uint32_t agent_idx = 0;

	DTNMP_DEBUG_ENTRY("db_incoming_initialize","(%llu)", timestamp);

	/* Step 0: Sanity check. */
	if(sender_eid == NULL)
	{
		DTNMP_DEBUG_ERR("db_incoming_initialize","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
		return result;
	}

	CHKCONN

	/* Step 1: Find the agent ID, or try to add it. */
	if((agent_idx = db_fetch_reg_agent_idx(sender_eid)) == 0)
	{
		if((agent_idx = db_add_agent(*sender_eid)) == 0)
		{
			DTNMP_DEBUG_ERR("db_incoming_initialize","Can't find agent id.", NULL);
			DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
			return result;
		}
	}

	/* Step 1: insert message into dbtIncoming*/
	sprintf(query, "INSERT INTO dbtIncomingMessageGroup(ReceivedTS,GeneratedTS,State,AgentID) "
		    			  "VALUES(NOW(),%lu,0,%d)", (unsigned long) timestamp, agent_idx);

	if (mysql_query(gConn, query))
    {
		DTNMP_DEBUG_ERR("db_incoming_initialize", "Database Error: %s",
		    	  		mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_incoming_initialize", "-->%d", result);
		return 0;
    }

	/* Step 2: Get the id of the inserted message*/
	sprintf(query, "SELECT LAST_INSERT_ID() FROM dbtIncomingMessageGroup");
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

	CHKCONN

	/* Step 1: Update dbtIncoming to processed */
	sprintf(query,"UPDATE dbtIncomingMessageGroup SET State = State + 1 WHERE ID = %d", id);
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
	char *query = NULL;
	char *result_data = NULL;
	int result_size = 0;


	CHKCONN

	if((result_data = utils_hex_to_string(cursor, size)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_incoming_process_message","Can't cvt %d bytes to hex str.",
				        size);
		DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		return 0;
	}

	result_size = strlen(result_data);
	if((query = (char *) STAKE(result_size + 256)) == NULL)
	{
		DTNMP_DEBUG_ERR("db_incoming_process_message","Can't alloc %d bytes.",
				        result_size + 256);
		SRELEASE(result_data);

		DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		return 0;
	}

	/*
	 * result_data starts with "0x" which we do not want in the DB
	 * so we skip over the first 2 characters when making the query.
	 */
	sprintf(query,"INSERT INTO dbtIncomingMessages(IncomingID,Content)"
			       "VALUES(%d,'%s')",id, result_data+2);
	SRELEASE(result_data);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_incoming_process_message", "Database Error: %s",
				mysql_error(gConn));
		SRELEASE(query);

		DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->0", NULL);
		return 0;
	}

	SRELEASE(query);
	DTNMP_DEBUG_EXIT("db_incoming_process_message", "-->1", NULL);
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
 *  08/29/15  E. Birrane     Only query DB if we have an active connection.
 *  04/24/16  E. Birrane     Accept global "running" flag.
 *****************************************************************************/

void *db_mgt_daemon(int *running)
{
	MYSQL_RES *sql_res;
	struct timeval start_time;
	vast delta = 0;

	DTNMP_DEBUG_ENTRY("db_mgt_daemon","(0x%#llx)", running);

	DTNMP_DEBUG_ALWAYS("db_mgt_daemon","Starting Manager Database Daemon",NULL);

	while (*running)
	{
    	getCurrentTime(&start_time);

    	if(db_mgt_connected() == 0)
    	{
    		if (db_outgoing_ready(&sql_res))
    		{
    			db_outgoing_process(sql_res);
    			mysql_free_result(sql_res);
    			sql_res = NULL;
    		}
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
uint32_t db_mgt_init(ui_db_t parms, uint32_t clear)
{

	DTNMP_DEBUG_ENTRY("db_mgt_init","(parms, %d)", clear);

	gConn = mysql_init(NULL);
	gParms = parms;

	DTNMP_DEBUG_ENTRY("db_mgt_init", "(%s,%s,%s,%s)", parms.server, parms.username, parms.password, parms.database);

	if (!mysql_real_connect(gConn, parms.server, parms.username, parms.password, parms.database, 0, NULL, 0)) {
		//DTNMP_DEBUG_ERR("db_mgt_init", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_init", "-->0", NULL);
		return 0;
	}

	DTNMP_DEBUG_INFO("db_mgt_init", "Connected to Database.", NULL);

	if(clear != 0)
	{
		db_mgt_clear();
	}

	/* Step 2: Make sure the DB knows about the MIDs we need. */
   // db_mgt_verify_mids();

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
 *  08/27/15  E. Birrane     Updated to latest schema
 *****************************************************************************/


int db_mgt_clear()
{

	DTNMP_DEBUG_ENTRY("db_mgt_clear", "()", NULL);

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
		DTNMP_DEBUG_ERR("db_mgt_clear", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_clear", "--> 0", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("db_mgt_clear", "--> 1", NULL);
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
	char query[1024];

	if(table == NULL)
	{
		return 1;
	}


	sprintf(query,"SET FOREIGN_KEY_CHECKS=0");

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_mgt_clear_table", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
		return 1;
	}

	sprintf(query,"TRUNCATE %s", table);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_mgt_clear_table", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
		return 1;
	}

	sprintf(query,"SET FOREIGN_KEY_CHECKS=1");
	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_mgt_clear_table", "SQL Error: %s", mysql_error(gConn));
		DTNMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
		return 1;
	}


	DTNMP_DEBUG_EXIT("db_mgt_clear_table", "--> 0", NULL);
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
	DTNMP_DEBUG_ENTRY("db_mgt_close","()",NULL);
	if(gConn != NULL)
	{
		mysql_close(gConn);
		gConn = NULL;
	}
	DTNMP_DEBUG_EXIT("db_mgt_close","-->.", NULL);
}


/******************************************************************************
 *
 * \par Function Name: db_mgt_connected
 *
 * \par Adds MIDS to the DB, if necessary, to make sure that dbtMIDs and
 *      dbtMIDDetails contain all MIDs known to this manager.
 *
 * \par Notes:
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
			db_mgt_init(gParms, 0);
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
 *  08/27/15  E. Birrane      Update to latest schema and data model.
 *****************************************************************************/

void handle_mid(mid_t *mid)
{
	if(mid != NULL)
	{
		char *name = names_get_name(mid);
		ui_parm_spec_t* spec = ui_get_parmspec(mid);

		/* If this is a MID defined in an ADM with no parameters, it
		 * is a regular MID.
		 */
		if((spec == NULL) || (spec->num_parms == 0))
		{
			db_add_mid(mid, spec, DTNMP_TYPE_MID);
		}
		/* Otherwise, if this MID is in the ADM and takes parameters
		 * it is a MID template, so it goes in protomids.
		 */
		else
		{
			db_add_protomid(mid, spec, DTNMP_TYPE_MID);
		}
	}
}


void db_mgt_verify_mids()
{
	LystElt elt;

	DTNMP_DEBUG_ENTRY("db_mgt_verify_mids","()", NULL);

	/* Step 1: For each known ADM. */
	if(db_fetch_adm_idx("AGENT","v0.1") == 0)
	{
		db_add_adm("AGENT","v0.1",AGENT_ADM_ROOT_NN_STR);
	}

	if(db_fetch_adm_idx("BP","6") == 0)
	{
		db_add_adm("BP","6", BP_ADM_ROOT_NN_STR);
	}


	/* Step 2: For each known nickname. */
	for(elt = lyst_first(nn_db); elt; elt = lyst_next(elt))
	{
		oid_nn_t* nn = (oid_nn_t*) lyst_data(elt);
		db_add_nn(nn);
	}

	/* Step 3: For each ADM atomic data defined... */
	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		adm_datadef_t *data = (adm_datadef_t *) lyst_data(elt);
		handle_mid(data->mid);
	}

	/* Step 4: For each ADM computed data defined... */
	for(elt = lyst_first(gAdmComputed); elt; elt = lyst_next(elt))
	{
		cd_t *data = (cd_t *) lyst_data(elt);
		handle_mid(data->id);
	}

	/* Step 5: For each ADM control defined... */
	for(elt = lyst_first(gAdmCtrls); elt; elt = lyst_next(elt))
	{
		adm_ctrl_t *data = (adm_ctrl_t *) lyst_data(elt);
		handle_mid(data->mid);
	}

	/* Step 6: For each ADM literal defined... */
	for(elt = lyst_first(gAdmLiterals); elt; elt = lyst_next(elt))
	{
		lit_t *data = (lit_t *) lyst_data(elt);
		handle_mid(data->id);
	}

	/* Step 7: For each ADM literal defined... */
	for(elt = lyst_first(gAdmOps); elt; elt = lyst_next(elt))
	{
		adm_op_t *data = (adm_op_t *) lyst_data(elt);
		handle_mid(data->mid);
	}

	/* Step 8: For each ADM report defined... */
	for(elt = lyst_first(gAdmRpts); elt; elt = lyst_next(elt))
	{
		def_gen_t *data = (def_gen_t *) lyst_data(elt);
		handle_mid(data->id);
	}

	/* Step 8: For each ADM report defined... */
	for(elt = lyst_first(gAdmMacros); elt; elt = lyst_next(elt))
	{
		def_gen_t *data = (def_gen_t *) lyst_data(elt);
		handle_mid(data->id);
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
 *  09/27/13  E. Birrane      Configure each agent with custom rpt, if applicable.
 *  08/27/15  E. Birrane      Update to new data model, schema
 *****************************************************************************/

int db_outgoing_process(MYSQL_RES *sql_res)
{
	MYSQL_ROW row;
	pdu_group_t *msg_group = NULL;
	uint32_t idx = 0;
	uint32_t agent_idx = 0;
	mid_t *id;
	def_gen_t *debugPrint;
	LystElt elt;
	LystElt def_elt;
	def_gen_t *cur_entry = NULL;
	def_gen_t *new_entry = NULL;
	agent_t *agent = NULL;
	adm_reg_agent_t *agent_reg = NULL;
	char query[128];

	DTNMP_DEBUG_ENTRY("db_outgoing_process","("ADDR_FIELDSPEC")",(uaddr) sql_res);

	CHKCONN

	/* Step 1: For each message group that is ready to go... */
	while ((row = mysql_fetch_row(sql_res)) != NULL)
	{
		/* Step 1.1 Create and populate the message group. */
		idx = atoi(row[0]);
		agent_idx = atoi(row[4]);

		if((msg_group = pdu_create_empty_group()) == NULL)
		{
			DTNMP_DEBUG_ERR("db_outgoing_process","Cannot create group.", NULL);
			return 0;
		}

		int result = db_outgoing_process_messages(idx, msg_group);

		if(result != 0)
		{
			agent_reg = db_fetch_reg_agent(agent_idx);

			if(agent_reg == NULL)
			{
				DTNMP_DEBUG_ERR("db_outgoing_process","Can't get agent from idx %d", agent_idx);
			}
			else
			{
				DTNMP_DEBUG_INFO("db_outgoing_process", "Sending to name %s", agent_reg->agent_id.name);
				iif_send(&ion_ptr, msg_group, agent_reg->agent_id.name);
				msg_release_reg_agent(agent_reg);

				/* Step 1.3.2: Make sure the manager knows about this agent. */
				if((agent = mgr_agent_get(&(agent_reg->agent_id))) == NULL)
				{
					DTNMP_DEBUG_WARN("db_outgoing_process","DB Agent not known to Mgr. Adding.", NULL);

					if(mgr_agent_add(agent_reg->agent_id) != 1)
					{
						DTNMP_DEBUG_WARN("db_outgoing_process","Sending to unknown agent.", NULL);
					}
					else
					{
						agent = mgr_agent_get(&(agent_reg->agent_id));

						if(agent != NULL)
						{
							DTNMP_DEBUG_WARN("db_outgoing_process","Added DB agent to Mgr.", NULL);
						}
						else
						{
							DTNMP_DEBUG_ERR("db_outgoing_process","Failed to add DB agent to Mgr.", NULL);
						}
					}
				}
			}
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
		sprintf(query, "UPDATE dbtOutgoingMessageGroup SET State=2 WHERE ID=%d", idx);
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
 * \param[in] idx -       the index of the message that corresponds to
 * 			              outgoing messages
 * \param[in] msg_group - the group that the message is in.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/13/13  E. Birrane      Initial implementation,
 *  09/27/13  E. Birrane      Collect any rpt defs from this message.
 *  08/27/15  E. Birrane      Update to latest data model and schema.
 *****************************************************************************/

int db_outgoing_process_messages(uint32_t idx, pdu_group_t *msg_group)
{
	int result = 0;
	char query[1024];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	uint32_t mc_idx = 0;
	Lyst mc = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;

	DTNMP_DEBUG_ENTRY("db_outgoing_process_messages",
					  "(%d, "ADDR_FIELDSPEC")",
			          idx, (uaddr) msg_group);

	CHKCONN

	/* Step 1: Find all messages for this outgoing group. */
	sprintf(query,
			"SELECT MidCollID FROM dbtOutgoingMessages WHERE OutgoingID=%d",
			idx);

	if (mysql_query(gConn, query))
	{
		DTNMP_DEBUG_ERR("db_outgoing_process_messages",
				        "Database Error: %s",
			 	        mysql_error(gConn));

		DTNMP_DEBUG_EXIT("db_outgoing_process_messages",
				         "-->%d",
						 result);
		return result;
	}

	/* Step 2: Parse the row and populate the structure. */
    res = mysql_store_result(gConn);

    while((row = mysql_fetch_row(res)) != NULL)
    {
		mc_idx = atoi(row[0]);

		if((mc = db_fetch_mid_col(mc_idx)) == NULL)
		{
			DTNMP_DEBUG_ERR("db_outgoing_process_messages",
						    "Can't grab MC for idx %d", mc_idx);
			result = 0;
			break;
		}

		// \todo: SQL has no way of adding an offset to running a control!
		msg_perf_ctrl_t *ctrl = msg_create_perf_ctrl(0, mc);

		/* Step 2: Construct a PDU to hold the primitive. */
		uint8_t *data = msg_serialize_perf_ctrl(ctrl, &size);

		char *str = utils_hex_to_string(data, size);
		DTNMP_DEBUG_ALWAYS("SQL Sending: ", "(size %d): %s", size, str);
		SRELEASE(str);

		/* This is a shallow copy. Do not release data. */
		pdu_msg_t *pdu_msg = pdu_create_msg(MSG_TYPE_CTRL_EXEC, data, size, NULL);

		/* This is a shallow copy. Do not release pdu_msg. */
		pdu_add_msg_to_group(msg_group, pdu_msg);

		result = 1;
	}

	mysql_free_result(res);

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

	CHKCONN

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
 *  08/27/15  E. Birrane      Updated to newer schema
 *****************************************************************************/

int db_outgoing_ready(MYSQL_RES **sql_res)
{
	int result = 0;
	char query[1024];

	*sql_res = NULL;

	DTNMP_DEBUG_ENTRY("db_outgoing_ready","("ADDR_FIELDSPEC")", (uaddr) sql_res);

	CHKCONN

	/* Step 0: Sanity check. */
	if(sql_res == NULL)
	{
		DTNMP_DEBUG_ERR("db_outgoing_ready", "Bad Parms.", NULL);
		DTNMP_DEBUG_EXIT("db_outgoing_ready","-->0",NULL);
		return 0;
	}

	/* Step 1: Build and execute query. */
	sprintf(query, "SELECT * FROM dbtOutgoingMessageGroup WHERE State=%d", TX_READY);
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
