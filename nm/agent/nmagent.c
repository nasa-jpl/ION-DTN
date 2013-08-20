/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: nmagent.c
 **
 ** Description: This implements NM Agent main processing.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR          DESCRIPTION
 **  --------  ------------    ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  01/10/13  E. Birrane      Update to lasted DTNMP Spec.
 **  06/10/13  E. Birrane      Added SDR data persistence.
 *****************************************************************************/

// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "shared/adm/adm.h"
#include "shared/utils/db.h"

#include "nmagent.h"
#include "ingest.h"
#include "rda.h"

#include "adm_agent_priv.h"
#include "adm_bp_priv.h"
#include "adm_ltp_priv.h"
#include "adm_ion_priv.h"


static void agent_signal_handler();


// Definitions of global data.
iif_t        ion_ptr;
uint8_t      g_running;
eid_t        manager_eid;
eid_t        agent_eid;
AgentDB      gAgentDB;
AgentVDB     gAgentVDB;



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
		if((data = ctrl_serialize_exec(item, &(item->desc.size))) == NULL)
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
			if((cur_item = ctrl_deserialize_exec(data,cur_desc.size, &bytes_used)) == NULL)
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
 * \par Function Name: agent_register
 *
 * \par Send a broadcast registration message for this agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void agent_register()
{
	adm_reg_agent_t *reg = NULL;
	uint8_t *data = NULL;
	uint32_t len = 0;
	pdu_msg_t *pdu_msg = NULL;
	pdu_group_t *pdu_group = NULL;

	/* Step 0: Build an agent registration message. */
	if((reg = msg_create_reg_agent(agent_eid)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_register","Unable to create agent registration.",NULL);
		return;
	}

	/* Step 1: Serialize the message. */
	if((data = msg_serialize_reg_agent(reg, &len)) == NULL)
	{
		DTNMP_DEBUG_ERR("agent_register","Unable to serialize message.", NULL);
		msg_release_reg_agent(reg);
		return;
	}

	/* Step 2: Create the DTNMP message. */
    if((pdu_msg = pdu_create_msg(MSG_TYPE_ADMIN_REG_AGENT, data, len, NULL)) == NULL)
    {
    	DTNMP_DEBUG_ERR("agent_register","Unable to create PDU message.", NULL);
    	msg_release_reg_agent(reg);
    	MRELEASE(data);
    	return;
    }

    /* Step 3: Create a group for this message. */
    if((pdu_group = pdu_create_group(pdu_msg)) == NULL)
    {
    	DTNMP_DEBUG_ERR("agent_register","Unable to create PDU message.", NULL);
    	msg_release_reg_agent(reg);
    	MRELEASE(data);
    	pdu_release_msg(pdu_msg);
    	return;
    }

    /* Step 4: Send the message. */
    iif_send(&ion_ptr, pdu_group, manager_eid.name);

    /*
     * Step 5: Release resources.  Releasing the group releases both the
     *         PDU message as well as the data (which is shallow-copied
     *         into the pdu_msg.
     */
    pdu_release_group(pdu_group);
    msg_release_reg_agent(reg);
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



/******************************************************************************
 *
 * \par Function Name: main
 *
 * \par Main agent processing function.
 *
 * \param[in]  argc    # command line arguments.
 * \param[in]  argv[]  Command-line arguments.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  01/10/13  E. Birrane      Update to lasted DTNMP Spec.
 **  06/10/13  E. Birrane      Added SDR data persistence.
 *****************************************************************************/

int main(int argc, char *argv[])
{
    pthread_t ingest_thr;
    pthread_t rda_thr;
    
    char ingest_thr_name[]  = "ingest_thread";
    char rda_thr_name[]     = "rda_thread";
    int rc;
    errno = 0;

    DTNMP_DEBUG_ENTRY("agent_main","(%d, 0x%#llx)", argc, (unsigned long)argv);
    
    /* Step 0: Sanity check. */
    if(argc != 3) {
        DTNMP_DEBUG_ALWAYS("main","Usage: nmagent <agent eid> <manager eid>\n", NULL);
        return 1;
    }
    
    DTNMP_DEBUG_INFO("agent main","Agent EID: %s, Mgr EID: %s", argv[1], argv[2]);
    

    /* Step 1: Indicate that the threads should run once started. */
    g_running = 1;

    /* Step 2: Note command-line arguments. */
    strcpy((char *) manager_eid.name, argv[2]);
    strcpy((char *) agent_eid.name, argv[1]);
   
    /* Step 3: Attach to ION and register. */
	if (ionAttach() < 0)
	{
		DTNMP_DEBUG_ERR("agentDbInit", "Agent can't attach to ION.", NULL);
		return -1;
	}

    if(iif_register_node(&ion_ptr, agent_eid) != 1)
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to regster BP Node. Exiting.",
        		         NULL);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }
   
    if (iif_is_registered(&ion_ptr))
    {
        DTNMP_DEBUG_INFO("agent_main","Agent registered with ION, EID: %s",
        		           iif_get_local_eid(&ion_ptr).name);
    }
    else
    {
        DTNMP_DEBUG_ERR("agent_main","Failed to register agent with ION, EID %s",
        		         iif_get_local_eid(&ion_ptr).name);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    /* Step 4: Read information from SDR and initialize memory lists.*/
    agent_db_init();

    if(agent_vdb_init() == -1)
    {
    	agent_vdb_destroy();

        DTNMP_DEBUG_ERR("agent_main","Unable to initialize VDB, errno = %s",
        		        strerror(errno));
       // MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }


    /* Step 5: Initialize ADM support. */
    adm_init();

    agent_adm_init_bp();
#ifdef _HAVE_LTP_ADM_
    agent_adm_init_ltp();
#endif

#ifdef _HAVE_ION_ADM_
    agent_adm_init_ion();
#endif

    agent_adm_init_agent();

    /* Step 6: Register signal handlers. */
    isignal(SIGINT, agent_signal_handler);
    isignal(SIGTERM, agent_signal_handler);

    /* Step 7: Start agent threads. */
    rc = pthread_create(&ingest_thr, NULL, rx_thread, (void *)ingest_thr_name);
    if (rc)
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
        		ingest_thr_name, strerror(errno));
        adm_destroy();
        agent_vdb_destroy();

    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    rc = pthread_create(&rda_thr, NULL, rda_thread, (void *)rda_thr_name);
    if (rc)
    {
       DTNMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
    		           rda_thr_name, strerror(errno));
       adm_destroy();
       agent_vdb_destroy();

       DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
       return -1;
    }
   
    DTNMP_DEBUG_ALWAYS("agent_main","Threads started...", NULL);


    /* Step 8: Send out agent broadcast message. */
    agent_register();

    /* Step 9: Join threads and wait for them to complete. */
    if (pthread_join(ingest_thr, NULL))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           ingest_thr_name, strerror(errno));
        adm_destroy();
        agent_vdb_destroy();

        DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }

    if (pthread_join(rda_thr, NULL))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           rda_thr_name, strerror(errno));
        adm_destroy();
        agent_vdb_destroy();

        DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }
   
    /* Step 10: Cleanup. */
    DTNMP_DEBUG_ALWAYS("agent_main","Cleaning Agent Resources.",NULL);
    adm_destroy();
    agent_vdb_destroy();


    DTNMP_DEBUG_ALWAYS("agent_main","Stopping Agent.",NULL);

    DTNMP_DEBUG_INFO("agent_main","Exiting Agent after cleanup.", NULL);
    return 0;
}



/******************************************************************************
 *
 * \par Function Name: agent_signal_handler
 *
 * \par Catches kill signals and gracefully shuts down the agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  08/18/13  E. Birrane    Initial Implementation
 *****************************************************************************/

static void agent_signal_handler()
{
	isignal(SIGINT,agent_signal_handler);
	isignal(SIGTERM, agent_signal_handler);

	g_running = 0;
}




