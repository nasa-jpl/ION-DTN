/*****************************************************************************
 **
 ** File Name: agent_db.c
 **
 ** Description: This module captures the functions, structures, and operations
 **              necessary to store and retrieve user-defined content from the
 **              embedded agent on system startup.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/17/15  E. Birrane     Initial Implementation
 *****************************************************************************/

// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "shared/adm/adm.h"
#include "shared/utils/db.h"

#include "agent_db.h"
#include "ingest.h"
#include "rda.h"

#include "shared/adm/adm_bp.h"
#include "shared/adm/adm_agent.h"
#include "adm_ltp_priv.h"
#include "adm_ion_priv.h"

#include "shared/primitives/ctrl.h"



void agent_db_compdata_init(Sdr sdr)
{
	/* \todo Implement. */
}

int  agent_db_compdata_persist(ctrl_exec_t* ctrl)
{
	/* \todo Implement. */
	return -1;
}

void agent_db_const_init(Sdr sdr)
{
	/* \todo Implement. */
}

int  agent_db_const_persist(ctrl_exec_t* ctrl)
{
	/* \todo Implement. */
	return -1;
}




/******************************************************************************
 *
 * \par Function Name: agent_db_ctrl_persist
 *
 * \par Persist a control to the agent SDR database.
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

int  agent_db_ctrl_persist(ctrl_exec_t* item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.descObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.descObj == 0)))
	{
		DTNMP_DEBUG_ERR("agent_db_ctrl_persist","bad params.",NULL);
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
			DTNMP_DEBUG_ERR("agent_db_ctrl_persist",
					       "Unable to serialize new ctrl.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(ctrl_exec_desc_t), &(item->desc.descObj),
				            gAgentDB.ctrls);

		MRELEASE(data);
		if(result != 1)
		{
			DTNMP_DEBUG_ERR("agent_db_ctrl_persist","Unable to persist def.",NULL);
			return -1;
		}

		DTNMP_DEBUG_INFO("agent_db_ctrl_persist","Persisted new ctrl", NULL);
	}
	else
	{
		ctrl_exec_desc_t temp;

		sdr_begin_xn(sdr);

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(ctrl_exec_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(ctrl_exec_desc_t));

		DTNMP_DEBUG_INFO("agent_db_ctrl_persist","Updated ctrl", NULL);

		sdr_end_xn(sdr);
	}

	return 1;
}


/******************************************************************************
 *
 * \par Function Name: agent_db_init
 *
 * \par Initialize items from the agent SDR database.
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

int  agent_db_init()
{
	Sdr sdr;

	sdr = getIonsdr();

	// * Initialize the non-volatile database. * /
	memset((char*) &gAgentDB, 0, sizeof(AgentDB));

	/* Recover the Agent database, creating it if necessary. */
	CHKERR(sdr_begin_xn(sdr));

	gAgentDB.descObj = sdr_find(sdr, "agentdb", NULL);
	switch(gAgentDB.descObj)
	{
		case -1:  // SDR error. * /
			sdr_cancel_xn(sdr);
			DTNMP_DEBUG_ERR("agent_db_init", "Can't search for Agent DB in SDR.", NULL);
			return -1;

		case 0: // Not found; Must create new DB. * /

			gAgentDB.descObj = sdr_malloc(sdr, sizeof(AgentDB));
			if(gAgentDB.descObj == 0)
			{
				sdr_cancel_xn(sdr);
				DTNMP_DEBUG_ERR("agent_db_init", "No space for agent database.", NULL);
				return -1;
			}
			DTNMP_DEBUG_ALWAYS("agent_db_init", "Creating DB", NULL);

			gAgentDB.compdata = sdr_list_create(sdr);
			gAgentDB.consts = sdr_list_create(sdr);
			gAgentDB.ctrls = sdr_list_create(sdr);
			gAgentDB.macros = sdr_list_create(sdr);
			gAgentDB.ops = sdr_list_create(sdr);
			gAgentDB.reports = sdr_list_create(sdr);
			gAgentDB.rules = sdr_list_create(sdr);

			sdr_write(sdr, gAgentDB.descObj, (char *) &gAgentDB, sizeof(AgentDB));
			sdr_catlg(sdr, "agentdb", 0, gAgentDB.descObj);

			break;

		default:  /* Found DB in the SDR */
			/* Read in the Database. */
			sdr_read(sdr, (char *) &gAgentDB, gAgentDB.descObj, sizeof(AgentDB));

			DTNMP_DEBUG_ALWAYS("agent_db_init", "Found DB", NULL);
	}

	if(sdr_end_xn(sdr))
	{
		DTNMP_DEBUG_ERR("agent_db_init", "Can't create Agent database.", NULL);
		return -1;
	}

	return 1;
}

int  agent_db_macro_persist(def_gen_t* ctrl)
{
	/* \todo Implement. */
	return -1;
}

int  agent_db_op_persist(def_gen_t* ctrl)
{
	/* \todo Implement. */
	return -1;
}



/******************************************************************************
 *
 * \par Function Name: agent_db_report_persist
 *
 * \par Persist a custom report definition to the agent SDR database.
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

int  agent_db_report_persist(def_gen_t* item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		DTNMP_DEBUG_ERR("agent_db_report_persist","bad params.",NULL);
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
			DTNMP_DEBUG_ERR("agent_db_report_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(def_gen_desc_t), &(item->desc.descObj),
				            gAgentDB.reports);

		MRELEASE(data);
		if(result != 1)
		{
			DTNMP_DEBUG_ERR("agent_db_report_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		def_gen_desc_t temp;

		sdr_begin_xn(sdr);

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(def_gen_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(def_gen_desc_t));

		sdr_end_xn(sdr);
	}


	return 1;
}



/******************************************************************************
 *
 * \par Function Name: agent_db_rule_persist
 *
 * \par Persist a time-based rule to the agent SDR database.
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
 *****************************************************************************/

int  agent_db_rule_persist(rule_time_prod_t *item)
{

	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		DTNMP_DEBUG_ERR("agent_db_rule_persist","bad params.",NULL);
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
		if((data = ctrl_serialize_time_prod_entry(item, &(item->desc.size))) == NULL)
		{
			DTNMP_DEBUG_ERR("agent_db_rule_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(rule_time_prod_desc_t), &(item->desc.descObj),
				            gAgentDB.rules);

		MRELEASE(data);
		if(result != 1)
		{
			DTNMP_DEBUG_ERR("agent_db_rule_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		rule_time_prod_desc_t temp;

		sdr_begin_xn(sdr);

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, 0); //sizeof(rule_time_prod_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(rule_time_prod_desc_t));

		if(sdr_end_xn(sdr))
		{
			DTNMP_DEBUG_ERR("agentDbInit", "Can't create Agent database.", NULL);
			return -1;
		}
	}

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: agent_vdb_add
 *
 * \par Add an item to a mutex-locked list.
 *
 * \param[in]  item  The item to add
 * \param[in]  list  The list to hold the item.
 * \param[in]  mutex The mute protecting the list.
 *
 * \par Notes:
 *		- This is a helper function used to add items to various agent
 *		  lists.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void agent_vdb_add(void *item, Lyst list, ResourceLock *mutex)
{
	lockResource(mutex);
    lyst_insert_last(list, item);
    unlockResource(mutex);
}


void agent_vdb_compdata_init(Sdr sdr)
{
	/* \todo: Implement. */
}

void agent_vdb_consts_init(Sdr sdr)
{
	/* \todo: Implement. */
}



/******************************************************************************
 *
 * \par Function Name: agent_vdb_ctrls_init
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

void agent_vdb_ctrls_init(Sdr sdr)
{
	Object elt;
	Object itemObj;
	Object descObj;
	ctrl_exec_desc_t cur_desc;
	ctrl_exec_t *cur_item;
	uint8_t *data = NULL;
	uint32_t bytes_used = 0;
	int num = 0;

	sdr_begin_xn(sdr);

	/* Step 1: Read through SDR list.... */
	for (elt = sdr_list_first(sdr, gAgentDB.ctrls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc));

		/* Step 1.2: Save the descriptor. */
		cur_desc.descObj = descObj;

		/* Step 1.3: Allocate space for the item. */
		if((data = (uint8_t*) MTAKE(cur_desc.size)) == NULL)
		{
			DTNMP_DEBUG_ERR("agent_vdb_ctrls_init","Can't allocate %d bytes.",
					        cur_desc.size);
		}
		else
		{
			/* Step 1.4: Grab the serialized item */
			sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

			/* Step 1.5: Deserialize the item. */
			if((cur_item = ctrl_deserialize(data,cur_desc.size, &bytes_used)) == NULL)
			{
				DTNMP_DEBUG_ERR("agent_vdb_ctrls_init","Failed to deserialize ctrl.",NULL);
			}
			else
			{
				/* Step 1.6: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_desc;

				/* Step 1.7: Add rule to list of active rules. */
				ADD_CTRL(cur_item);
			}

			/* Step 1.8: Release the serialized item. */
			MRELEASE(data);

			/* Step 1.9: Note that we have another control. */
			num++;
		}
	}

	sdr_end_xn(sdr);

	/* Step 2: Note to use number of controls read in. */
	DTNMP_DEBUG_ALWAYS("", "Added %d Controls from DB.", num);
}


/******************************************************************************
 *
 * \par Function Name: agent_vdb_destroy
 *
 * \par Cleans up agent memory lists.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void agent_vdb_destroy()
{

	/* Step 1: Clear out data in lysts. */
	def_lyst_clear(&(gAgentVDB.compdata),    &(gAgentVDB.compdata_mutex), 1);
	def_lyst_clear(&(gAgentVDB.consts),      &(gAgentVDB.consts_mutex),   1);
    ctrl_clear_lyst(&(gAgentVDB.ctrls),     &(gAgentVDB.ctrls_mutex),    1);
    def_lyst_clear(&(gAgentVDB.macros),      &(gAgentVDB.macros_mutex),   1);
    def_lyst_clear(&(gAgentVDB.ops),         &(gAgentVDB.ops_mutex),      1);
    rpt_clear_lyst(&(gAgentVDB.reports),    &(gAgentVDB.reports_mutex),  1);
    rule_time_clear_lyst(&(gAgentVDB.rules),&(gAgentVDB.rules_mutex),    1);

    /* Step 2: Release resource locks. */
    killResourceLock(&(gAgentVDB.compdata_mutex));
    killResourceLock(&(gAgentVDB.consts_mutex));
    killResourceLock(&(gAgentVDB.ctrls_mutex));
    killResourceLock(&(gAgentVDB.macros_mutex));
    killResourceLock(&(gAgentVDB.ops_mutex));
    killResourceLock(&(gAgentVDB.reports_mutex));
    killResourceLock(&(gAgentVDB.rules_mutex));
}


/******************************************************************************
 *
 * \par Function Name: agent_vdb_init
 *
 * \par Initializes agent memory lists.
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
 *****************************************************************************/

/* Initialize all of the VDB items. */
int  agent_vdb_init()
{
	Sdr sdr = getIonsdr();
	int result = 1;

	DTNMP_DEBUG_ENTRY("agent_vdb_init","()",NULL);

	/* Step 0: Clean the memory. */
	memset(&gAgentVDB, 0, sizeof(gAgentVDB));

	/* Step 1: Create lysts and associated resource locks. */

	if((gAgentVDB.compdata = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.compdata_mutex))) result = -1;
    agent_vdb_compdata_init(sdr);

	if((gAgentVDB.consts = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.consts_mutex))) result = -1;
    agent_vdb_consts_init(sdr);

	if((gAgentVDB.ctrls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.ctrls_mutex))) result = -1;
    agent_vdb_ctrls_init(sdr);

	if((gAgentVDB.macros = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.macros_mutex))) result = -1;
    agent_vdb_macros_init(sdr);

	if((gAgentVDB.ops = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.ops_mutex))) result = -1;
    agent_vdb_ops_init(sdr);

	if((gAgentVDB.reports = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.reports_mutex))) result = -1;
    agent_vdb_reports_init(sdr);

	if((gAgentVDB.rules = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.rules_mutex))) result = -1;
    agent_vdb_rules_init(sdr);

    DTNMP_DEBUG_EXIT("agent_vdb_init","-->%d",result);

    return result;
}


void agent_vdb_macros_init(Sdr sdr)
{
	/* \todo: implement. */
}

void agent_vdb_ops_init(Sdr sdr)
{
	/* \todo: implement. */
}



/******************************************************************************
 *
 * \par Function Name: agent_vdb_reports_init
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

void agent_vdb_reports_init(Sdr sdr)
{
	Object elt;
	Object defObj;
	Object descObj;
	def_gen_desc_t cur_desc;
	def_gen_t *cur_item;
	uint8_t *data;
	uint32_t bytes_used = 0;
	int num = 0;

	sdr_begin_xn(sdr);

	/* Step 1: Walk through report definitions. */
	for (elt = sdr_list_first(sdr, gAgentDB.reports); elt;
			elt = sdr_list_next(sdr, elt))
	{

		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc));

		cur_desc.descObj = descObj;

		/* Step 1.2: Allocate space for the def. */
		if((data = (uint8_t*) MTAKE(cur_desc.size)) == NULL)
		{
			DTNMP_DEBUG_ERR("agent_vdb_reports_init","Can't allocate %d bytes.",
					        cur_desc.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized rule */
			sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = def_deserialize_gen(data,
									  cur_desc.size,
									  &bytes_used)) == NULL)
			{
				DTNMP_DEBUG_ERR("agent_vdb_reports_init","Can't deserialize rpt.", NULL);
			}
			else
			{
				/* Step 1.5: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_desc;

				/* Step 1.6: Add report def to list of report defs. */
				ADD_REPORT(cur_item);

				/* Step 1.7: Note that we have read a new report.*/
				num++;
			}

			/* Step 1.8: Release serialized rpt, we don't need it. */
			MRELEASE(data);
		}
	}
	sdr_end_xn(sdr);

	/* Step 2: Print to user total number of reports read.*/
	DTNMP_DEBUG_ALWAYS("", "Added %d Reports from DB.", num);
}



/******************************************************************************
 *
 * \par Function Name: agent_vdb_rules_init
 *
 * \par Read time-based rule definitions from the SDR database into memory lists.
 *
 * \param[in]  sdr   The SDR containing the rule information.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void agent_vdb_rules_init(Sdr sdr)
{
	Object elt;
	Object itemObj;
	Object descObj;
	rule_time_prod_desc_t cur_descr;
	rule_time_prod_t *cur_item;
	uint8_t *data = NULL;
	uint32_t bytes_used = 0;
	int num = 0;

	sdr_begin_xn(sdr);

	/* Step 1: Read in active rules. */
	for (elt = sdr_list_first(sdr, gAgentDB.rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_descr, descObj, sizeof(cur_descr));

		cur_descr.descObj = descObj;

		/* Step 1.2: Allocate space for the rule. */
		if((data = (uint8_t*) MTAKE(cur_descr.size)) == NULL)
		{
			DTNMP_DEBUG_ERR("agent_vdb_reports_init","Can't allocate %d bytes.",
					        cur_descr.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized rule */
			sdr_read(sdr, (char *) data, cur_descr.itemObj, cur_descr.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = ctrl_deserialize_time_prod_entry(data,
				                                    cur_descr.size,
				                                    &bytes_used)) == NULL)
			{
				DTNMP_DEBUG_ERR("agent_vdb_reports_init","Can't deserialize rule.", NULL);
			}
			else
			{
				/* Step 1.5: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_descr;

				/* Step 1.6: Add rule to list of active rules. */
				ADD_RULE(cur_item);

				/* Step 1.7: Note that another rule has been read. */
				num++;
			}

			/* Step 1.8: Release serialized rule, we don't need it. */
			MRELEASE(data);
		}
	}

	sdr_end_xn(sdr);

	/* Step 2: Print to user total number of rules read.*/
	DTNMP_DEBUG_ALWAYS("", "Added %d Rules from DB.", num);
}


