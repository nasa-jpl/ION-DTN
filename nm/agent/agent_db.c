/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

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
 **  06/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  05/17/15  E. Birrane     Update to new data types (Secure DTN - NASA: NNX14CS58P)
 **  06/21/15  E. Birrane     Add support for computed data storage. (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane     Update naming to AMP v0.3. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "../shared/adm/adm.h"
#include "../shared/utils/db.h"

#include "agent_db.h"
#include "ingest.h"
#include "rda.h"

#include "../shared/adm/adm_bp.h"
#include "../shared/adm/adm_agent.h"
#include "adm_ltp_priv.h"
#include "adm_ion_priv.h"

#include "../shared/primitives/ctrl.h"
#include "instr.h"


int  agent_db_var_persist(var_t *item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("agent_db_var_persist","bad params.",NULL);
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
			AMP_DEBUG_ERR("agent_db_var_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(var_desc_t), &(item->desc.descObj),
							gAgentDB.vars);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("agent_db_var_persist","Unable to persist cd.",NULL);
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

int  agent_db_var_forget(mid_t *mid)
{
	var_t *item = agent_vdb_var_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("agent_db_var_forget","bad params.",NULL);
		return -1;
	}

	return agent_db_forget(gAgentDB.vars, item->desc.itemObj, item->desc.descObj);
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
		AMP_DEBUG_ERR("agent_db_ctrl_persist","bad params.",NULL);
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
			AMP_DEBUG_ERR("agent_db_ctrl_persist",
					       "Unable to serialize new ctrl.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(ctrl_exec_desc_t), &(item->desc.descObj),
				            gAgentDB.ctrls);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("agent_db_ctrl_persist","Unable to persist def.",NULL);
			return -1;
		}

		AMP_DEBUG_INFO("agent_db_ctrl_persist","Persisted new ctrl", NULL);
	}
	else
	{
		ctrl_exec_desc_t temp;

		oK(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(ctrl_exec_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(ctrl_exec_desc_t));

		AMP_DEBUG_INFO("agent_db_ctrl_persist","Updated ctrl", NULL);

		sdr_end_xn(sdr);
	}

	return 1;
}

int  agent_db_ctrl_forget(mid_t *mid)
{
	ctrl_exec_t *item = agent_vdb_ctrl_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("agent_db_ctrl_forget","bad params.",NULL);
		return -1;
	}

	return agent_db_forget(gAgentDB.ctrls, item->desc.itemObj, item->desc.descObj);
}

int  agent_db_forget(Object db, Object itemObj, Object descObj)
{
	/* Step 0: Sanity Checks. */
	if(((itemObj == 0) && (descObj != 0)) ||
	   ((itemObj != 0) && (descObj == 0)))
	{
		AMP_DEBUG_ERR("agent_db_Forget","bad params.",NULL);
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
			AMP_DEBUG_ERR("agent_db_forget","Unable to forget def.",NULL);
			return -1;
		}
	}

	return 1;
}


int  agent_db_defgen_persist(Object db, def_gen_t* item)
{
	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("agent_db_defgen_persist","bad params.",NULL);
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
			AMP_DEBUG_ERR("agent_db_defgen_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(def_gen_desc_t), &(item->desc.descObj),
				            db);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("agent_db_defgen_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		def_gen_desc_t temp;

		oK(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(def_gen_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(def_gen_desc_t));

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
			AMP_DEBUG_ERR("agent_db_init", "Can't search for Agent DB in SDR.", NULL);
			return -1;

		case 0: // Not found; Must create new DB. * /

			gAgentDB.descObj = sdr_malloc(sdr, sizeof(AgentDB));
			if(gAgentDB.descObj == 0)
			{
				sdr_cancel_xn(sdr);
				AMP_DEBUG_ERR("agent_db_init", "No space for agent database.", NULL);
				return -1;
			}
			AMP_DEBUG_ALWAYS("agent_db_init", "Creating DB", NULL);

			gAgentDB.vars = sdr_list_create(sdr);
			gAgentDB.ctrls = sdr_list_create(sdr);
			gAgentDB.macros = sdr_list_create(sdr);
			gAgentDB.reports = sdr_list_create(sdr);
			gAgentDB.trls = sdr_list_create(sdr);
			gAgentDB.srls = sdr_list_create(sdr);

			sdr_write(sdr, gAgentDB.descObj, (char *) &gAgentDB, sizeof(AgentDB));
			sdr_catlg(sdr, "agentdb", 0, gAgentDB.descObj);

			break;

		default:  /* Found DB in the SDR */
			/* Read in the Database. */
			sdr_read(sdr, (char *) &gAgentDB, gAgentDB.descObj, sizeof(AgentDB));

			AMP_DEBUG_ALWAYS("agent_db_init", "Found DB", NULL);
	}

	if(sdr_end_xn(sdr))
	{
		AMP_DEBUG_ERR("agent_db_init", "Can't create Agent database.", NULL);
		return -1;
	}

	return 1;
}


int  agent_db_macro_persist(def_gen_t* ctrl)
{
	return agent_db_defgen_persist(gAgentDB.macros, ctrl);
}

int  agent_db_macro_forget(mid_t *mid)
{
	def_gen_t *item = agent_vdb_macro_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("agent_db_macro_forget","bad params.",NULL);
		return -1;
	}

	return agent_db_forget(gAgentDB.macros, item->desc.itemObj, item->desc.descObj);
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
	return agent_db_defgen_persist(gAgentDB.reports, item);
}

int  agent_db_report_forget(mid_t *mid)
{
	def_gen_t *item = agent_vdb_report_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("agent_db_report_forget","bad params.",NULL);
		return -1;
	}

	return agent_db_forget(gAgentDB.reports, item->desc.itemObj, item->desc.descObj);
}




/******************************************************************************
 *
 * \par Function Name: agent_db_srl_persist
 *
 * \par Persist a state-based rule to the agent SDR database.
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

int  agent_db_srl_persist(srl_t *item)
{

	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("agent_db_srl_persist","bad params.",NULL);
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
			AMP_DEBUG_ERR("agent_db_srl_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(srl_t), &(item->desc.descObj),
				            gAgentDB.srls);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("agent_db_srl_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		srl_desc_t temp;

		oK(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, 0);
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(srl_desc_t));

		if(sdr_end_xn(sdr))
		{
			AMP_DEBUG_ERR("agent_db_srl_persist", "Can't create Agent database.", NULL);
			return -1;
		}
	}

	return 1;
}


int  agent_db_srl_forget(mid_t *mid)
{
	srl_t *item = agent_vdb_srl_find(mid);
	int result = 0;

	if(item == NULL)
	{
		AMP_DEBUG_ERR("agent_db_srl_forget","bad params.",NULL);
		return -1;
	}

	result =  agent_db_forget(gAgentDB.srls, item->desc.itemObj, item->desc.descObj);
//	srl_release(item);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: agent_db_trl_persist
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
 *  06/26/15  E. Birrane     Updated to new TRL structure/def.
 *****************************************************************************/

int  agent_db_trl_persist(trl_t *item)
{

	Sdr sdr = getIonsdr();

	/* Step 0: Sanity Checks. */
	if((item == NULL) ||
	   ((item->desc.itemObj == 0) && (item->desc.itemObj != 0)) ||
	   ((item->desc.itemObj != 0) && (item->desc.itemObj == 0)))
	{
		AMP_DEBUG_ERR("agent_db_trl_persist","bad params.",NULL);
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
			AMP_DEBUG_ERR("agent_db_trl_persist",
					       "Unable to serialize new item.", NULL);
			return -1;
		}

		result = db_persist(data, item->desc.size, &(item->desc.itemObj),
				            &(item->desc), sizeof(trl_t), &(item->desc.descObj),
				            gAgentDB.trls);

		SRELEASE(data);
		if(result != 1)
		{
			AMP_DEBUG_ERR("agent_db_trl_persist","Unable to persist def.",NULL);
			return -1;
		}
	}
	else
	{
		trl_desc_t temp;

		oK(sdr_begin_xn(sdr));

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, 0);
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(trl_desc_t));

		if(sdr_end_xn(sdr))
		{
			AMP_DEBUG_ERR("agent_db_trl_persist", "Can't create Agent database.", NULL);
			return -1;
		}
	}

	return 1;
}


int  agent_db_trl_forget(mid_t *mid)
{
	trl_t *item = agent_vdb_trl_find(mid);

	if(item == NULL)
	{
		AMP_DEBUG_ERR("agent_db_trl_forget","bad params.",NULL);
		return -1;
	}


	return agent_db_forget(gAgentDB.trls, item->desc.itemObj, item->desc.descObj);
}




uint32_t agent_db_count(Lyst list, ResourceLock *mutex)
{
	uint32_t result = 0;

	if(mutex != NULL)
	{
		lockResource(mutex);
	}
	if(list != NULL)
	{
		result = lyst_length(list);
	}
	if(mutex != NULL)
	{
		unlockResource(mutex);
	}

	return result;
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


uint32_t agent_vdb_var_init(Sdr sdr)
{
	Object elt;
	Object descObj;
	var_desc_t cur_desc;
	var_t *cur_item;
	uint8_t *data;
	uint32_t bytes_used = 0;
	uint32_t num = 0;

    oK(sdr_begin_xn(sdr));

    /* Step 1: Walk through report definitions. */
    for (elt = sdr_list_first(sdr, gAgentDB.vars); elt;
    		elt = sdr_list_next(sdr, elt))
    {

    	/* Step 1.1: Grab the descriptor. */
    	descObj = sdr_list_data(sdr, elt);
    	oK(sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc)));

    	cur_desc.descObj = descObj;

    	/* Step 1.2: Allocate space for the def. */
    	if((data = (uint8_t*) STAKE(cur_desc.size)) == NULL)
    	{
    		AMP_DEBUG_ERR("agent_vdb_var_init","Can't allocate %d bytes.",
    					cur_desc.size);
    	}
    	else
    	{
    		/* Step 1.3: Grab the serialized rule */
    		oK(sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size));

    		/* Step 1.4: Deserialize into a rule object. */
    		if((cur_item = var_deserialize(data,
    				cur_desc.size,
					&bytes_used)) == NULL)
    		{
    			AMP_DEBUG_ERR("agent_vdb_var_init","Can't deserialize cd.", NULL);
    		}
    		else
    		{
    			/* Step 1.5: Copy current descriptor to cur_rule. */
    			cur_item->desc = cur_desc;

    			/* Step 1.6: Add report def to list of report defs. */
    			agent_vdb_add(cur_item, gAgentVDB.vars, &(gAgentVDB.var_mutex));

    			/* Step 1.7: Note that we have read a new report.*/
    			num++;
    		}

    		/* Step 1.8: Release serialized cd, we don't need it. */
    		SRELEASE(data);
    	}
    }

    sdr_end_xn(sdr);

    AMP_DEBUG_ALWAYS("", "Added %d Variable Definitions from DB.", num);

    return num;
}

var_t *agent_vdb_var_find(mid_t *mid)
{
	LystElt elt;
	var_t *cur = NULL;

	lockResource(&(gAgentVDB.var_mutex));

	for(elt = lyst_first(gAgentVDB.vars); elt; elt = lyst_next(elt))
	{
		cur = (var_t *) lyst_data(elt);
		if(mid_compare(cur->id, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gAgentVDB.var_mutex));

	return cur;

}

void agent_vdb_var_forget(mid_t *id)
{
	LystElt elt;
	var_t *cur = NULL;

	lockResource(&(gAgentVDB.var_mutex));

	for(elt = lyst_first(gAgentVDB.vars); elt; elt = lyst_next(elt))
	{
		cur = (var_t *) lyst_data(elt);
		if(mid_compare(cur->id, id, 1) == 0)
		{
			var_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gAgentVDB.var_mutex));
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
	Object descObj;
	ctrl_exec_desc_t cur_desc;
	ctrl_exec_t *cur_item;
	uint8_t *data = NULL;
	uint32_t bytes_used = 0;
	int num = 0;

	CHKVOID(sdr_begin_xn(sdr));

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
		if((data = (uint8_t*) STAKE(cur_desc.size)) == NULL)
		{
			AMP_DEBUG_ERR("agent_vdb_ctrls_init","Can't allocate %d bytes.",
					        cur_desc.size);
		}
		else
		{
			/* Step 1.4: Grab the serialized item */
			sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

			/* Step 1.5: Deserialize the item. */
			if((cur_item = ctrl_deserialize(data,cur_desc.size, &bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("agent_vdb_ctrls_init","Failed to deserialize ctrl.",NULL);
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


ctrl_exec_t* agent_vdb_ctrl_find(mid_t *mid)
{
	LystElt elt;
	ctrl_exec_t *cur = NULL;

	lockResource(&(gAgentVDB.ctrls_mutex));

	for(elt = lyst_first(gAgentVDB.ctrls); elt; elt = lyst_next(elt))
	{
		cur = (ctrl_exec_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gAgentVDB.ctrls_mutex));

	return cur;
}

void         agent_vdb_ctrl_forget(mid_t *mid)
{
	LystElt elt;
	ctrl_exec_t *cur = NULL;

	lockResource(&(gAgentVDB.ctrls_mutex));

	for(elt = lyst_first(gAgentVDB.ctrls); elt; elt = lyst_next(elt))
	{
		cur = (ctrl_exec_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			ctrl_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gAgentVDB.ctrls_mutex));
}


uint32_t agent_vdb_defgen_init(Sdr sdr, Object db, Lyst list, ResourceLock *mutex)
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
			AMP_DEBUG_ERR("agent_vdb_defgen_init","Can't allocate %d bytes.",
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
				AMP_DEBUG_ERR("agent_vdb_defgen_init","Can't deserialize rpt.", NULL);
			}
			else
			{
				/* Step 1.5: Copy current descriptor to cur_rule. */
				cur_item->desc = cur_desc;

				/* Step 1.6: Add report def to list of report defs. */
				agent_vdb_add(cur_item, list, mutex);

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

def_gen_t *agent_vdb_defgen_find(mid_t *mid, Lyst list, ResourceLock *mutex)
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

void agent_vdb_defgen_forget(mid_t *id, Lyst list, ResourceLock *mutex)
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
	var_lyst_clear(&(gAgentVDB.vars),    &(gAgentVDB.var_mutex), 1);
    ctrl_clear_lyst(&(gAgentVDB.ctrls),     &(gAgentVDB.ctrls_mutex),    1);
    def_lyst_clear(&(gAgentVDB.macros),      &(gAgentVDB.macros_mutex),   1);
    def_lyst_clear(&(gAgentVDB.reports),    &(gAgentVDB.reports_mutex),  1);
    trl_lyst_clear(&(gAgentVDB.trls),&(gAgentVDB.trls_mutex),    1);
    srl_lyst_clear(&(gAgentVDB.srls),&(gAgentVDB.srls_mutex),    1);

    /* Step 2: Release resource locks. */
    killResourceLock(&(gAgentVDB.var_mutex));
    killResourceLock(&(gAgentVDB.ctrls_mutex));
    killResourceLock(&(gAgentVDB.macros_mutex));
    killResourceLock(&(gAgentVDB.reports_mutex));
    killResourceLock(&(gAgentVDB.trls_mutex));
    killResourceLock(&(gAgentVDB.srls_mutex));

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

	AMP_DEBUG_ENTRY("agent_vdb_init","()",NULL);

	/* Step 0: Clean the memory. */
	memset(&gAgentVDB, 0, sizeof(gAgentVDB));

	/* Step 1: Create lysts and associated resource locks. */

	if((gAgentVDB.vars = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.var_mutex))) result = -1;
    agent_vdb_var_init(sdr);

	if((gAgentVDB.ctrls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.ctrls_mutex))) result = -1;
    agent_vdb_ctrls_init(sdr);

	if((gAgentVDB.macros = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.macros_mutex))) result = -1;
    agent_vdb_macros_init(sdr);

	if((gAgentVDB.reports = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.reports_mutex))) result = -1;
    agent_vdb_reports_init(sdr);

	if((gAgentVDB.trls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.trls_mutex))) result = -1;
    agent_vdb_trls_init(sdr);

	if((gAgentVDB.srls = lyst_create()) == NULL) result = -1;
    if(initResourceLock(&(gAgentVDB.srls_mutex))) result = -1;
    agent_vdb_srls_init(sdr);

    AMP_DEBUG_EXIT("agent_vdb_init","-->%d",result);

    return result;
}


void agent_vdb_macros_init(Sdr sdr)
{
	int num = 0;

	num = agent_vdb_defgen_init(sdr, gAgentDB.macros, gAgentVDB.macros, &(gAgentVDB.macros_mutex));

	AMP_DEBUG_ALWAYS("", "Added %d Macros from DB.", num);
}

def_gen_t *agent_vdb_macro_find(mid_t *mid)
{
	return agent_vdb_defgen_find(mid, gAgentVDB.macros, &(gAgentVDB.macros_mutex));
}

void agent_vdb_macro_forget(mid_t *id)
{
	agent_vdb_defgen_forget(id, gAgentVDB.macros, &(gAgentVDB.macros_mutex));
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
	int num = 0;

	num = agent_vdb_defgen_init(sdr, gAgentDB.reports, gAgentVDB.reports, &(gAgentVDB.reports_mutex));

	AMP_DEBUG_ALWAYS("", "Added %d Reports from DB.", num);
}

def_gen_t *agent_vdb_report_find(mid_t *mid)
{
	return agent_vdb_defgen_find(mid, gAgentVDB.reports, &(gAgentVDB.reports_mutex));
}

void agent_vdb_report_forget(mid_t *id)
{
	agent_vdb_defgen_forget(id, gAgentVDB.reports, &(gAgentVDB.reports_mutex));
}


void       agent_vdb_srls_init(Sdr sdr)
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
	for (elt = sdr_list_first(sdr, gAgentDB.srls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_descr, descObj, sizeof(cur_descr));

		cur_descr.descObj = descObj;

		/* Step 1.2: Allocate space for the rule. */
		if((data = (uint8_t*) STAKE(cur_descr.size)) == NULL)
		{
			AMP_DEBUG_ERR("agent_vdb_srls_init","Can't allocate %d bytes.",
					        cur_descr.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized rule */
			sdr_read(sdr, (char *) data, cur_descr.itemObj, cur_descr.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = srl_deserialize(data,cur_descr.size,&bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("agent_vdb_srls_init","Can't deserialize rule.", NULL);
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

	gAgentInstr.num_srls = agent_db_count(gAgentVDB.srls, &(gAgentVDB.srls_mutex));

	/* Step 2: Print to user total number of rules read.*/
	AMP_DEBUG_ALWAYS("", "Added %d SRLs from DB.", num);
}


srl_t*     agent_vdb_srl_find(mid_t *mid)
{
	LystElt elt;
	srl_t *cur = NULL;
	srl_t *result = NULL;

	lockResource(&(gAgentVDB.srls_mutex));

	for(elt = lyst_first(gAgentVDB.srls); elt; elt = lyst_next(elt))
	{
		cur = (srl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
		//	result = srl_copy(cur);
			result = cur;
			break;
		}
	}

	unlockResource(&(gAgentVDB.srls_mutex));

	return result;
}


void       agent_vdb_srl_forget(mid_t *mid)
{
	LystElt elt;
	LystElt del_elt;
	srl_t *cur = NULL;

	lockResource(&(gAgentVDB.srls_mutex));

	for(elt = lyst_first(gAgentVDB.srls); elt; )
	{

		cur = (srl_t *) lyst_data(elt);
		del_elt = elt;
		elt = lyst_next(elt);

		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			srl_release(cur);
			lyst_delete(del_elt);
			break;
		}
	}

	unlockResource(&(gAgentVDB.srls_mutex));
}

void agent_vdb_trls_init(Sdr sdr)
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
	for (elt = sdr_list_first(sdr, gAgentDB.trls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1.1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_descr, descObj, sizeof(cur_descr));

		cur_descr.descObj = descObj;

		/* Step 1.2: Allocate space for the rule. */
		if((data = (uint8_t*) STAKE(cur_descr.size)) == NULL)
		{
			AMP_DEBUG_ERR("agent_vdb_trls_init","Can't allocate %d bytes.",
					        cur_descr.size);
		}
		else
		{
			/* Step 1.3: Grab the serialized rule */
			sdr_read(sdr, (char *) data, cur_descr.itemObj, cur_descr.size);

			/* Step 1.4: Deserialize into a rule object. */
			if((cur_item = trl_deserialize(data,cur_descr.size,&bytes_used)) == NULL)
			{
				AMP_DEBUG_ERR("agent_vdb_trls_init","Can't deserialize rule.", NULL);
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

	gAgentInstr.num_trls = agent_db_count(gAgentVDB.trls, &(gAgentVDB.trls_mutex));

	/* Step 2: Print to user total number of rules read.*/
	AMP_DEBUG_ALWAYS("", "Added %d TRLs from DB.", num);
}


trl_t*     agent_vdb_trl_find(mid_t *mid)
{
	LystElt elt;
	trl_t *cur = NULL;

	lockResource(&(gAgentVDB.trls_mutex));

	for(elt = lyst_first(gAgentVDB.trls); elt; elt = lyst_next(elt))
	{
		cur = (trl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			break;
		}
		cur = NULL;
	}

	unlockResource(&(gAgentVDB.trls_mutex));

	return cur;
}


void       agent_vdb_trl_forget(mid_t *mid)
{
	LystElt elt;
	trl_t *cur = NULL;

	lockResource(&(gAgentVDB.trls_mutex));

	for(elt = lyst_first(gAgentVDB.trls); elt; elt = lyst_next(elt))
	{
		cur = (trl_t *) lyst_data(elt);
		if(mid_compare(cur->mid, mid, 1) == 0)
		{
			trl_release(cur);
			lyst_delete(elt);
			break;
		}
	}

	unlockResource(&(gAgentVDB.trls_mutex));
}
