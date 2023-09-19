/*****************************************************************************
 **
 ** File Name: mgr_db.h
 **
 ** Description: This module captures the functions, structures, and operations
 **              necessary to store and retrieve user-defined content from the
 **              management daemon on system startup.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/18/15  E. Birrane     Initial Implementation from mgr_db.[c|h] (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "../shared/adm/adm.h"
#include "../shared/utils/db.h"

#include "mgr_db.h"

#include "../shared/primitives/var.h"
#include "../shared/adm/adm_bp.h"
#include "../shared/adm/adm_agent.h"

#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/report.h"



int  mgr_db_compdata_forget(mid_t *mid)
{
	var_t *item = mgr_vdb_compdata_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_compdata_forget","bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.compdata, item->desc.itemObj, item->desc.descObj);
}



int  mgr_db_compdata_persist(var_t *item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_compdata_persist","bad params.",NULL);
		return -1;
	}


	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */

	if(item->desc.itemObj == 0)
	{
		uint8_t *data = NULL;
		int result = 0;

		/* Step 1.1: Serialize the item to go into the SDR.. */
		if((data = var_serialize(item, &(item->desc.size))) == NULL)
		{
			AMP_DEBUG_ERR("mgr_db_compdata_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(var_desc_t), &(item->desc.descObj),
				            gMgrDB.compdata);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_compdata_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		var_desc_t temp;

		CHKERR(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(var_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(var_desc_t));

		sdr_end_xn(sdr);
	}


	return 1;
}




int  mgr_db_ctrl_forget(mid_t *mid)
{
	ctrl_exec_t *item = mgr_vdb_ctrl_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_ctrl_forget","bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.ctrls, item->desc.itemObj, item->desc.descObj);
}


/******************************************************************************
 *
 * \par Function Name: mgr_db_ctrl_persist
 *
 * \par Persist a control to the mgr SDR database.
 *
 * \param[in]  item  The control to persist.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int  mgr_db_ctrl_persist(ctrl_exec_t* item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.descObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.descObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_ctrl_persist","bad params.",NULL);
		return -1;
	}


	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */

	if(item->desc.itemObj == 0)
	{
		uint8_t *data = NULL;
		int result = 0;

		/* Step 1.1: Serialize the item to go into the SDR.. */
		if((data = ctrl_serialize(item, &(item->desc.size))) == NULL)
		{
			AMP_DEBUG_ERR("mgr_db_ctrl_persist",
					       "Unable to serialize new ctrl.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(ctrl_exec_desc_t), &(item->desc.descObj),
				            gMgrDB.ctrls);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_ctrl_persist","Unable to persist def.",NULL);
			return -1;
		}

		AMP_DEBUG_INFO("mgr_db_ctrl_persist","Persisted new ctrl", NULL);
	}
	else
	{
		ctrl_exec_desc_t temp;

		CHKERR(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(ctrl_exec_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(ctrl_exec_desc_t));

		AMP_DEBUG_INFO("mgr_db_ctrl_persist","Updated ctrl", NULL);

		sdr_end_xn(sdr);
	}

	return 1;
}


int  mgr_db_defgen_persist(Object db, def_gen_t* item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_defgen_persist","bad params.",NULL);
		return -1;
	}


	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */

	if(item->desc.itemObj == 0)
	{
		uint8_t *data = NULL;
		int result = 0;

		/* Step 1.1: Serialize the item to go into the SDR.. */
		if((data = def_serialize_gen(item, &(item->desc.size))) == NULL)
		{
			AMP_DEBUG_ERR("mgr_db_defgen_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(def_gen_desc_t), &(item->desc.descObj),
				            db);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_defgen_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		def_gen_desc_t temp;

		CHKERR(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(def_gen_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(def_gen_desc_t));

		sdr_end_xn(sdr);
	}


	return 1;
}



int  mgr_db_forget(Object db, Object itemObj, Object descObj)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if(((itemObj == 0) && (descObj != 0)) ||
	   ((itemObj != 0) && (descObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_Forget","bad params.",NULL);
		return -1;
	}

	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */
	if(itemObj != 0)
	{
		int result = db_forget(&(itemObj), &(descObj), db);

		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_forget","Unable to forget def.",NULL);
			return -1;
		}
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: mgr_db_init
 *
 * \par Initialize items from the mgr SDR database.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *  08/29/15  E. Birrane     Updated to include SQL Account Information
 *****************************************************************************/

int  mgr_db_init()
{
	Sdr sdr;

	sdr = getIonsdr();

	// * Initialize the non-volatile database. * /
	memset((char*) &gMgrDB, 0, sizeof(MgrDB));

	/* Recover the Mgr database, creating it if necessary. */
	CHKERR(sdr_begin_xn(sdr));

	gMgrDB.descObj = sdr_find(sdr, "mgrdb", NULL);
	switch(gMgrDB.descObj)
	{
		case -1:  // SDR error. * /
			sdr_cancel_xn(sdr);
			AMP_DEBUG_ERR("mgr_db_init", "Can't search for Mgr DB in SDR.", NULL);
			return -1;

		case 0: // Not found; Must create new DB. * /

			gMgrDB.descObj = sdr_malloc(sdr, sizeof(MgrDB));
			if(gMgrDB.descObj == 0)
			{
				sdr_cancel_xn(sdr);
				AMP_DEBUG_ERR("mgr_db_init", "No space for mgr database.", NULL);
				return -1;
			}
			AMP_DEBUG_ALWAYS("mgr_db_init", "Creating DB", NULL);

			gMgrDB.compdata = sdr_list_create(sdr);
			gMgrDB.ctrls = sdr_list_create(sdr);
			gMgrDB.macros = sdr_list_create(sdr);
			gMgrDB.reports = sdr_list_create(sdr);
			gMgrDB.trls = sdr_list_create(sdr);
			gMgrDB.srls = sdr_list_create(sdr);
#ifdef HAVE_MYSQL
			gMgrDB.sqldb = sdr_list_create(sdr);
#endif
			sdr_write(sdr, gMgrDB.descObj, (char *) &gMgrDB, sizeof(MgrDB));
			sdr_catlg(sdr, "mgrdb", 0, gMgrDB.descObj);

			break;

		default:  /* Found DB in the SDR */
			/* Read in the Database. */
			sdr_read(sdr, (char *) &gMgrDB, gMgrDB.descObj, sizeof(MgrDB));

			AMP_DEBUG_ALWAYS("mgr_db_init", "Found DB", NULL);
	}

	if(sdr_end_xn(sdr))
	{
		AMP_DEBUG_ERR("mgr_db_init", "Can't create Mgr database.", NULL);
		return -1;
	}

	return 1;
}


int  mgr_db_macro_forget(mid_t *mid)
{
	def_gen_t *item = mgr_vdb_macro_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_macro_forget","bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.macros, item->desc.itemObj, item->desc.descObj);
}

int  mgr_db_macro_persist(def_gen_t* ctrl)
{
	return mgr_db_defgen_persist(gMgrDB.macros, ctrl);
}



int  mgr_db_report_forget(mid_t *mid)
{
	def_gen_t *item = mgr_vdb_report_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_report_forget","bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.reports, item->desc.itemObj, item->desc.descObj);
}


/******************************************************************************
 *
 * \par Function Name: mgr_db_report_persist
 *
 * \par Persist a custom report definition to the mgr SDR database.
 *
 * \param[in]  item  The definition to persist.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int  mgr_db_report_persist(def_gen_t* item)
{
	return mgr_db_defgen_persist(gMgrDB.reports, item);
}




int  mgr_db_srl_forget(mid_t *mid)
{
	srl_t *item = mgr_vdb_srl_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_srl_forget","bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.srls, item->desc.itemObj, item->desc.descObj);
}



/******************************************************************************
 *
 * \par Function Name: mgr_db_srl_persist
 *
 * \par Persist a state-based rule to the mgr SDR database.
 *
 * \param[in]  item  The rule to persist.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/26/15  E. Birrane     Initial implementation.
 *****************************************************************************/

int  mgr_db_srl_persist(srl_t *item)
{

	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_srl_persist","bad params.",NULL);
		return -1;
	}


	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */

	if(item->desc.itemObj == 0)
	{
		uint8_t *data = NULL;
		int result = 0;

		/* Step 1.1: Serialize the item to go into the SDR.. */
		if((data = srl_serialize(item, &(item->desc.size))) == NULL)
		{
			AMP_DEBUG_ERR("mgr_db_srl_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(srl_desc_t), &(item->desc.descObj),
				            gMgrDB.srls);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_srl_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		srl_desc_t temp;

		CHKERR(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, 0);
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(srl_desc_t));

		if(sdr_end_xn(sdr))
		{
			AMP_DEBUG_ERR("mgr_db_srl_persist", "Can't create Mgr database.", NULL);
			return -1;
		}
	}

	return 1;
}


int  mgr_db_trl_forget(mid_t *mid)
{
	trl_t *item = mgr_vdb_trl_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_trl_forget","bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.trls, item->desc.itemObj, item->desc.descObj);
}


/******************************************************************************
 *
 * \par Function Name: mgr_db_trl_persist
 *
 * \par Persist a time-based rule to the mgr SDR database.
 *
 * \param[in]  item  The rule to persist.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *  06/26/15  E. Birrane     Updated to new TRL structure/def.
 *****************************************************************************/

int  mgr_db_trl_persist(trl_t *item)
{

	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_trl_persist","bad params.",NULL);
		return -1;
	}


	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */

	if(item->desc.itemObj == 0)
	{
		uint8_t *data = NULL;
		int result = 0;

		/* Step 1.1: Serialize the item to go into the SDR.. */
		if((data = trl_serialize(item, &(item->desc.size))) == NULL)
		{
			AMP_DEBUG_ERR("mgr_db_trl_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(trl_desc_t), &(item->desc.descObj),
				            gMgrDB.trls);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_trl_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		trl_desc_t temp;

		CHKERR(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, 0);
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(trl_desc_t));

		if(sdr_end_xn(sdr))
		{
			AMP_DEBUG_ERR("mgr_db_trl_persist", "Can't create Mgr database.", NULL);
			return -1;
		}
	}

	return 1;
}


#ifdef HAVE_MYSQL

/******************************************************************************
 *
 * \par Function Name: mgr_db_sql_forget
 *
 * \par Remove SQL DB information from the MGR SDR database.
 *
 * \param[in]  item  The sql account information.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation.
 *****************************************************************************/
int  mgr_db_sql_forget(ui_db_t* item)
{
	if(item == NULL)
	{
		AMP_DEBUG_ERR("mgr_db_sql_forget","Bad params.",NULL);
		return -1;
	}

	return mgr_db_forget(gMgrDB.sqldb, item->desc.itemObj, item->desc.descObj);
}


/******************************************************************************
 *
 * \par Function Name: mgr_db_sql_persist
 *
 * \par Persist SQL account information to the mgr SDR database.
 *
 * \param[in]  item  The SQL account information to persist.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        -1 - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/29/15  E. Birrane     Initial implementation.
 *****************************************************************************/

int  mgr_db_sql_persist(ui_db_t* item)
{

	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("mgr_db_sql_persist","bad params.",NULL);
		return -1;
	}


	/*
	 * Step 1: Determine if this is already in the SDR. We will assume
	 *         it is in the SDR already if its Object fields are nonzero.
	 */

	if(item->desc.itemObj == 0)
	{
		uint8_t data[UI_SQL_TOTLEN];
		uint8_t *cursor = NULL;
		int result = 0;

		/* Step 1.1: Serialize the item to go into the SDR.. */
		item->desc.size = UI_SQL_TOTLEN;

		cursor = &(data[0]);
		memcpy(cursor, item->database,UI_SQL_DBLEN);
		cursor += UI_SQL_DBLEN;
		memcpy(cursor, item->username,UI_SQL_ACCTLEN);
		cursor += UI_SQL_ACCTLEN;
		memcpy(cursor, item->password,UI_SQL_ACCTLEN);
		cursor += UI_SQL_ACCTLEN;
		memcpy(cursor, item->server,UI_SQL_SERVERLEN);
		cursor += UI_SQL_ACCTLEN;

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(def_gen_desc_t), &(item->desc.descObj),
				            gMgrDB.sqldb);

		if(result != 1)
		{
			AMP_DEBUG_ERR("mgr_db_sql_persist","Unable to persist SQL account information.",NULL);
			return -1;
		}
	}
	else
	{
		def_gen_desc_t temp;

		CHKERR(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, 0);
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(def_gen_desc_t));

		if(sdr_end_xn(sdr))
		{
			AMP_DEBUG_ERR("mgr_db_sql_persist", "Can't create SQL account info in the database.", NULL);
			return -1;
		}
	}

	return 1;
}

#endif

/******************************************************************************
 *
 * \par Function Name: mgr_vdb_add
 *
 * \par Add an item to a mutex-locked list.
 *
 * \param[in]  item  The item to add
 * \param[in]  list  The list to hold the item.
 * \param[in]  mutex The mute protecting the list.
 *
 * \par Notes:
 *		- This is a helper function used to add items to various mgr
 *		  lists.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void mgr_vdb_add(void *item, Lyst list, ResourceLock *mutex)
{
	lockResource(mutex);
    lyst_insert_last(list, item);
    unlockResource(mutex);
}


void mgr_vdb_compdata_init(Sdr sdr)
{
	uint32_t num = 0;

	num = mgr_vdb_defgen_init(sdr, gMgrDB.compdata, gMgrVDB.compdata, &(gMgrVDB.compdata_mutex));

	AMP_DEBUG_ALWAYS("", "Added %d Computed Data Definitions from DB.", num);
}


var_t *mgr_vdb_compdata_find(mid_t *mid)
{
	LystElt elt;
	var_t *cur = NULL;

	lockResource(&(gMgrVDB.compdata_mutex));

	for(elt = lyst_first(gMgrVDB.compdata); elt; elt = lyst_next(elt))
	{
		cur = (var_t *) lyst_data(elt);
		if(mid_compare(cur->id, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gMgrVDB.compdata_mutex));

	return cur;
}


void mgr_vdb_compdata_forget(mid_t *id)
{
	LystElt elt;
	var_t *cur = NULL;

	lockResource(&(gMgrVDB.compdata_mutex));

	for(elt = lyst_first(gMgrVDB.compdata); elt; elt = lyst_next(elt))
	{
		cur = (var_t *) lyst_data(elt);
		if(mid_compare(cur->id, id, 1) == 0)
		{
			var_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gMgrVDB.compdata_mutex));
}




/******************************************************************************
 *
 * \par Function Name: mgr_vdb_ctrls_init
 *
 * \par Read controls from the SDR database into memory lists.
 *
 * \param[in]  sdr   The SDR containing the controls information.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void mgr_vdb_ctrls_init(Sdr sdr)
{
	Object elt;
	Object descObj;
	ctrl_exec_desc_t cur_desc;
	ctrl_exec_t *cur_item;
	uint8_t *data = NULL;
	uint32_t bytes_used = 0;
	int num = 0;

	CHKVOID(sdr_begin_xn(sdr));

	/* Step 1: Read through SDR list.... */
	for (elt = sdr_list_first(sdr, gMgrDB.ctrls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc));

		/* Step 1.2: Save the descriptor. */
		cur_desc.descObj = descObj;

		/* Step 1.3: Allocate space for the item. */
		if((data = (uint8_t*) STAKE(cur_desc.size)) == NULL)
		{
			AMP_DEBUG_ERR("mgr_vdb_ctrls_init","Can't allocate %d bytes.",
					        cur_desc.size);
		}
		else
		{
			/* Step 1.4: Grab the serialized item */
			sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

			/* Step 1.5: Deserialize the item. */
			if((cur_item = ctrl_deserialize(data,cur_desc.size, &bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("mgr_vdb_ctrls_init","Failed to deserialize ctrl.",NULL);
			}
			else
			{
				/* Step 1.6: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_desc;

				/* Step 1.7: Add rule to list of active rules. */
				ADD_CTRL(cur_item);
			}

			/* Step 1.8: Release the serialized item. */
			SRELEASE(data);

			/* Step 1.9: Note that we have another control. */
			num++;
		}
	}

	sdr_end_xn(sdr);

	/* Step 2: Note to use number of controls read in. */
	AMP_DEBUG_ALWAYS("", "Added %d Controls from DB.", num);
}


ctrl_exec_t* mgr_vdb_ctrl_find(mid_t *mid)
{
	LystElt elt;
	ctrl_exec_t *cur = NULL;

	lockResource(&(gMgrVDB.ctrls_mutex));

	for(elt = lyst_first(gMgrVDB.ctrls); elt; elt = lyst_next(elt))
	{
		cur = (ctrl_exec_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gMgrVDB.ctrls_mutex));

	return cur;
}

void         mgr_vdb_ctrl_forget(mid_t *mid)
{
	LystElt elt;
	ctrl_exec_t *cur = NULL;

	lockResource(&(gMgrVDB.ctrls_mutex));

	for(elt = lyst_first(gMgrVDB.ctrls); elt; elt = lyst_next(elt))
	{
		cur = (ctrl_exec_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			ctrl_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gMgrVDB.ctrls_mutex));
}


uint32_t mgr_vdb_defgen_init(Sdr sdr, Object db, Lyst list, ResourceLock *mutex)
{
	Object elt;
	Object descObj;
	def_gen_desc_t cur_desc;
	def_gen_t *cur_item;
	uint8_t *data;
	uint32_t bytes_used = 0;
	int num = 0;

	CHKZERO(sdr_begin_xn(sdr));

	/* Step 1: Walk through report definitions. */
	for (elt = sdr_list_first(sdr, db); elt;
			elt = sdr_list_next(sdr, elt))
	{

		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc));

		cur_desc.descObj = descObj;

		/* Step 1.2: Allocate space for the def. */
		if((data = (uint8_t*) STAKE(cur_desc.size)) == NULL)
		{
			AMP_DEBUG_ERR("mgr_vdb_defgen_init","Can't allocate %d bytes.",
					        cur_desc.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized def */
			sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = def_deserialize_gen(data,
									  cur_desc.size,
									  &bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("mgr_vdb_defgen_init","Can't deserialize rpt.", NULL);
			}
			else
			{
				/* Step 1.5: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_desc;

				/* Step 1.6: Add report def to list of report defs. */
				mgr_vdb_add(cur_item, list, mutex);

				/* Step 1.7: Note that we have read a new report.*/
				num++;
			}

			/* Step 1.8: Release serialized rpt, we don't need it. */
			SRELEASE(data);
		}
	}
	sdr_end_xn(sdr);

	return num;
}

def_gen_t *mgr_vdb_defgen_find(mid_t *mid, Lyst list, ResourceLock *mutex)
{
	LystElt elt;
	def_gen_t *cur = NULL;

	lockResource(mutex);

	for(elt = lyst_first(list); elt; elt = lyst_next(elt))
	{
		cur = (def_gen_t *) lyst_data(elt);
		if(mid_compare(cur->id, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(mutex);

	return cur;
}

void mgr_vdb_defgen_forget(mid_t *id, Lyst list, ResourceLock *mutex)
{
	LystElt elt;
	def_gen_t *cur = NULL;

	lockResource(mutex);

	for(elt = lyst_first(list); elt; elt = lyst_next(elt))
	{
		cur = (def_gen_t *) lyst_data(elt);
		if(mid_compare(cur->id, id, 1) == 0)
		{
			def_release_gen(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(mutex);
}

/******************************************************************************
 *
 * \par Function Name: mgr_vdb_destroy
 *
 * \par Cleans up mgr memory lists.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void mgr_vdb_destroy()
{

	/* Step 1: Clear out data in lysts. */
	def_lyst_clear(&(gMgrVDB.compdata),    &(gMgrVDB.compdata_mutex), 1);
    ctrl_clear_lyst(&(gMgrVDB.ctrls),     &(gMgrVDB.ctrls_mutex),    1);
    def_lyst_clear(&(gMgrVDB.macros),      &(gMgrVDB.macros_mutex),   1);
    def_lyst_clear(&(gMgrVDB.reports),    &(gMgrVDB.reports_mutex),  1);
    trl_lyst_clear(&(gMgrVDB.trls),&(gMgrVDB.trls_mutex),    1);
    srl_lyst_clear(&(gMgrVDB.srls),&(gMgrVDB.srls_mutex),    1);

    /* Step 2: Release resource locks. */
    killResourceLock(&(gMgrVDB.compdata_mutex));
    killResourceLock(&(gMgrVDB.ctrls_mutex));
    killResourceLock(&(gMgrVDB.macros_mutex));
    killResourceLock(&(gMgrVDB.reports_mutex));
    killResourceLock(&(gMgrVDB.trls_mutex));
    killResourceLock(&(gMgrVDB.srls_mutex));

}


/******************************************************************************
 *
 * \par Function Name: mgr_vdb_init
 *
 * \par Initializes mgr memory lists.
 *
 * \par Notes:
 *
 * \return 1 - Success.
 *        -1 - Failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *  08/29/15  E. Birrane     Update to include SQL account info.
 *****************************************************************************/

/* Initialize all of the VDB items. */
int  mgr_vdb_init()
{
	Sdr sdr = getIonsdr();
	int result = 1;

	AMP_DEBUG_ENTRY("mgr_vdb_init","()",NULL);

	/* Step 0: Clean the memory. */
	memset(&gMgrVDB, 0, sizeof(gMgrVDB));

	/* Step 1: Create lysts and associated resource locks. */

	if((gMgrVDB.compdata = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gMgrVDB.compdata_mutex))) result = -1;
    mgr_vdb_compdata_init(sdr);

	if((gMgrVDB.ctrls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gMgrVDB.ctrls_mutex))) result = -1;
    mgr_vdb_ctrls_init(sdr);

	if((gMgrVDB.macros = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gMgrVDB.macros_mutex))) result = -1;
    mgr_vdb_macros_init(sdr);

	if((gMgrVDB.reports = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gMgrVDB.reports_mutex))) result = -1;
    mgr_vdb_reports_init(sdr);

	if((gMgrVDB.trls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gMgrVDB.trls_mutex))) result = -1;
    mgr_vdb_trls_init(sdr);

	if((gMgrVDB.srls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gMgrVDB.srls_mutex))) result = -1;
    mgr_vdb_srls_init(sdr);
#ifdef HAVE_MYSQL
    if(initResourceLock(&(gMgrVDB.sqldb_mutex))) result = -1;
    mgr_vdb_sql_init(sdr);
#endif

    AMP_DEBUG_EXIT("mgr_vdb_init","-->%d",result);

    return result;
}


void mgr_vdb_macros_init(Sdr sdr)
{
	int num = 0;

	num = mgr_vdb_defgen_init(sdr, gMgrDB.macros, gMgrVDB.macros, &(gMgrVDB.macros_mutex));

	AMP_DEBUG_ALWAYS("", "Added %d Macros from DB.", num);
}

def_gen_t *mgr_vdb_macro_find(mid_t *mid)
{
	return mgr_vdb_defgen_find(mid, gMgrVDB.macros, &(gMgrVDB.macros_mutex));
}

void mgr_vdb_macro_forget(mid_t *id)
{
	mgr_vdb_defgen_forget(id, gMgrVDB.macros, &(gMgrVDB.macros_mutex));
}



/******************************************************************************
 *
 * \par Function Name: mgr_vdb_reports_init
 *
 * \par Read report definitions from the SDR database into memory lists.
 *
 * \param[in]  sdr   The SDR containing the report information.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void mgr_vdb_reports_init(Sdr sdr)
{
	int num = 0;

	num = mgr_vdb_defgen_init(sdr, gMgrDB.reports, gMgrVDB.reports, &(gMgrVDB.reports_mutex));

	AMP_DEBUG_ALWAYS("", "Added %d Reports from DB.", num);
}

def_gen_t *mgr_vdb_report_find(mid_t *mid)
{
	return mgr_vdb_defgen_find(mid, gMgrVDB.reports, &(gMgrVDB.reports_mutex));
}

void mgr_vdb_report_forget(mid_t *id)
{
	mgr_vdb_defgen_forget(id, gMgrVDB.reports, &(gMgrVDB.reports_mutex));
}


void       mgr_vdb_srls_init(Sdr sdr)
{
	Object elt;
	Object descObj;
	srl_desc_t cur_descr;
	srl_t *cur_item;
	uint8_t *data = NULL;
	uint32_t bytes_used = 0;
	int num = 0;

	CHKVOID(sdr_begin_xn(sdr));

	/* Step 1: Read in active rules. */
	for (elt = sdr_list_first(sdr, gMgrDB.srls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_descr, descObj, sizeof(cur_descr));

		cur_descr.descObj = descObj;

		/* Step 1.2: Allocate space for the rule. */
		if((data = (uint8_t*) STAKE(cur_descr.size)) == NULL)
		{
			AMP_DEBUG_ERR("mgr_vdb_srls_init","Can't allocate %d bytes.",
					        cur_descr.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized rule */
			sdr_read(sdr, (char *) data, cur_descr.itemObj, cur_descr.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = srl_deserialize(data,cur_descr.size,&bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("mgr_vdb_srls_init","Can't deserialize rule.", NULL);
			}
			else
			{
				/* Step 1.5: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_descr;

				/* Step 1.6: Add rule to list of active rules. */
				ADD_SRL(cur_item);

				/* Step 1.7: Note that another rule has been read. */
				num++;
			}

			/* Step 1.8: Release serialized rule, we don't need it. */
			SRELEASE(data);
		}
	}

	sdr_end_xn(sdr);

	/* Step 2: Print to user total number of rules read.*/
	AMP_DEBUG_ALWAYS("", "Added %d SRLs from DB.", num);
}


srl_t*     mgr_vdb_srl_find(mid_t *mid)
{
	LystElt elt;
	srl_t *cur = NULL;

	lockResource(&(gMgrVDB.srls_mutex));

	for(elt = lyst_first(gMgrVDB.srls); elt; elt = lyst_next(elt))
	{
		cur = (srl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gMgrVDB.srls_mutex));

	return cur;
}


void       mgr_vdb_srl_forget(mid_t *mid)
{
	LystElt elt;
	srl_t *cur = NULL;

	lockResource(&(gMgrVDB.srls_mutex));

	for(elt = lyst_first(gMgrVDB.srls); elt; elt = lyst_next(elt))
	{
		cur = (srl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			srl_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gMgrVDB.srls_mutex));
}

void mgr_vdb_trls_init(Sdr sdr)
{
	Object elt;
	Object descObj;
	trl_desc_t cur_descr;
	trl_t *cur_item;
	uint8_t *data = NULL;
	uint32_t bytes_used = 0;
	int num = 0;

	CHKVOID(sdr_begin_xn(sdr));

	/* Step 1: Read in active rules. */
	for (elt = sdr_list_first(sdr, gMgrDB.trls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_descr, descObj, sizeof(cur_descr));

		cur_descr.descObj = descObj;

		/* Step 1.2: Allocate space for the rule. */
		if((data = (uint8_t*) STAKE(cur_descr.size)) == NULL)
		{
			AMP_DEBUG_ERR("mgr_vdb_trls_init","Can't allocate %d bytes.",
					        cur_descr.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized rule */
			sdr_read(sdr, (char *) data, cur_descr.itemObj, cur_descr.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = trl_deserialize(data,cur_descr.size,&bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("mgr_vdb_trls_init","Can't deserialize rule.", NULL);
			}
			else
			{
				/* Step 1.5: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_descr;

				/* Step 1.6: Add rule to list of active rules. */
				ADD_TRL(cur_item);

				/* Step 1.7: Note that another rule has been read. */
				num++;
			}

			/* Step 1.8: Release serialized rule, we don't need it. */
			SRELEASE(data);
		}
	}

	sdr_end_xn(sdr);

	/* Step 2: Print to user total number of rules read.*/
	AMP_DEBUG_ALWAYS("", "Added %d TRLs from DB.", num);
}


trl_t*     mgr_vdb_trl_find(mid_t *mid)
{
	LystElt elt;
	trl_t *cur = NULL;

	lockResource(&(gMgrVDB.trls_mutex));

	for(elt = lyst_first(gMgrVDB.trls); elt; elt = lyst_next(elt))
	{
		cur = (trl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gMgrVDB.trls_mutex));

	return cur;
}


void       mgr_vdb_trl_forget(mid_t *mid)
{
	LystElt elt;
	trl_t *cur = NULL;

	lockResource(&(gMgrVDB.trls_mutex));

	for(elt = lyst_first(gMgrVDB.trls); elt; elt = lyst_next(elt))
	{
		cur = (trl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			trl_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gMgrVDB.trls_mutex));
}




#ifdef HAVE_MYSQL
void mgr_vdb_sql_init(Sdr sdr)
{
	Object elt;
	Object descObj;
	uint8_t *data = NULL;
	uint8_t *cursor = NULL;

	CHKVOID(sdr_begin_xn(sdr));


	/* Step 1: Grab the description for the account info. */
	if((elt = sdr_list_first(sdr, gMgrDB.sqldb)) == 0)
	{

		sdr_end_xn(sdr);

		return;
	}

	if((descObj = sdr_list_data(sdr, elt)) == 0)
	{
		AMP_DEBUG_ERR("mgr_vdb_sql_init","Bad SQL Account descriptor.", NULL);
		sdr_end_xn(sdr);

		return;
	}

	lockResource(&(gMgrVDB.sqldb_mutex));


	sdr_read(sdr, (char *) &(gMgrVDB.sqldb.desc), descObj, sizeof(gMgrVDB.sqldb.desc));
	gMgrVDB.sqldb.desc.descObj = descObj;


	/* Step 2: Allocate and populate the descriptor field. */
	if((data = (uint8_t*) STAKE(gMgrVDB.sqldb.desc.size)) == NULL)
	{
		AMP_DEBUG_ERR("mgr_vdb_sql_init","Can't allocate %d bytes.",
				gMgrVDB.sqldb.desc.size);
		unlockResource(&(gMgrVDB.sqldb_mutex));
		sdr_end_xn(sdr);
		return;
	}

	sdr_read(sdr, (char *) data, gMgrVDB.sqldb.desc.itemObj, gMgrVDB.sqldb.desc.size);

	sdr_end_xn(sdr);


	/* Step 3: Populate the account info. */
	cursor = &(data[0]);

	memcpy(gMgrVDB.sqldb.database, cursor, UI_SQL_DBLEN);
	cursor += UI_SQL_DBLEN;
	memcpy(gMgrVDB.sqldb.username, cursor, UI_SQL_ACCTLEN);
	cursor += UI_SQL_ACCTLEN;
	memcpy(gMgrVDB.sqldb.password, cursor, UI_SQL_ACCTLEN);
	cursor += UI_SQL_ACCTLEN;
	memcpy(gMgrVDB.sqldb.server, cursor, UI_SQL_SERVERLEN);
	cursor += UI_SQL_ACCTLEN;

	SRELEASE(data);

    unlockResource(&(gMgrVDB.sqldb_mutex));

	/* Step 2: Print to user total number of rules read.*/
	AMP_DEBUG_ALWAYS("", "Added SQL Account Information from DB.", NULL);
}


ui_db_t*   mgr_vdb_sql_find()
{
	ui_db_t *result = NULL;

	lockResource(&(gMgrVDB.sqldb_mutex));

	result = &(gMgrVDB.sqldb);

	unlockResource(&(gMgrVDB.sqldb_mutex));

	return result;
}


void mgr_vdb_sql_forget()
{
	lockResource(&(gMgrVDB.sqldb_mutex));

	memset(&(gMgrVDB.sqldb), 0, sizeof(ui_db_t));

	unlockResource(&(gMgrVDB.sqldb_mutex));
}

#endif
