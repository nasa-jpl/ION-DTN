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
 ** File Name: nmagent.h
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



// System signal handler.
void signalHandler(int signum);

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
	}
	else
	{
		ctrl_exec_desc_t temp;

		sdr_begin_xn(sdr);

		sdr_stage(sdr, (char*) &temp, item->desc.descObj, sizeof(ctrl_exec_desc_t));
		temp = item->desc;
		sdr_write(sdr, item->desc.descObj, (char *) &temp, sizeof(ctrl_exec_desc_t));

		sdr_end_xn(sdr);
	}


	return 1;
}

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

		default:  // Found DB in the SDR * /
			// Read in the Database. * /
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

void agent_vdb_ctrls_init(Sdr sdr)
{
	Object elt;
	Object itemObj;
	Object descObj;
	ctrl_exec_desc_t cur_desc;
	ctrl_exec_t *cur_item;
	uint8_t *data;
	uint32_t bytes_used = 0;
	int num = 0;

	sdr_begin_xn(sdr);

	/* Read through SDR list.... */
	for (elt = sdr_list_first(sdr, gAgentDB.ctrls); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc));

		cur_desc.descObj = descObj;

		/* Step 2: Allocate space for the item. */
		data = (uint8_t*) MTAKE(cur_desc.size);

		/* Step 3: Grab the serialized item */
		sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

		/* Step 4: Deserialize the item. */
		cur_item = ctrl_deserialize_exec(data,cur_desc.size, &bytes_used);

		/* Step 5: Copy current descriptor to cur_rule. */
		cur_item->desc = cur_desc;

		/* Step 6: Add rule to list of active rules. */
		ADD_CTRL(cur_item);

		num++;
	}

	sdr_end_xn(sdr);

	DTNMP_DEBUG_ALWAYS("", "Added %d Controls from DB.", num);

}



void agent_register()
{
	adm_reg_agent_t *reg = NULL;
	uint8_t *data = NULL;
	uint32_t len = 0;
	pdu_msg_t *pdu_msg = NULL;
	pdu_group_t *pdu_group = NULL;

	reg = msg_create_reg_agent(agent_eid);
	data = msg_serialize_reg_agent(reg, &len);
    pdu_msg = pdu_create_msg(MSG_TYPE_ADMIN_REG_AGENT, data, len, NULL);
    pdu_group = pdu_create_group(pdu_msg);

    iif_send(&ion_ptr, pdu_group, manager_eid.name);
    pdu_release_group(pdu_group);
    msg_release_reg_agent(reg);
}


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

/* Initialize all of the VDB items. */
int  agent_vdb_init()
{
	Sdr sdr = getIonsdr();

	/* Step 0: Clean the memory. */
	memset(&gAgentVDB, 0, sizeof(gAgentVDB));

	/* Step 1: Create lysts and associated resource locks. */

	if((gAgentVDB.compdata = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.compdata_mutex))) return -1;
    agent_vdb_compdata_init(sdr);

	if((gAgentVDB.consts = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.consts_mutex))) return -1;
    agent_vdb_consts_init(sdr);

	if((gAgentVDB.ctrls = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.ctrls_mutex))) return -1;
    agent_vdb_ctrls_init(sdr);

	if((gAgentVDB.macros = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.macros_mutex))) return -1;
    agent_vdb_macros_init(sdr);

	if((gAgentVDB.ops = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.ops_mutex))) return -1;
    agent_vdb_ops_init(sdr);

	if((gAgentVDB.reports = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.reports_mutex))) return -1;
    agent_vdb_reports_init(sdr);

	if((gAgentVDB.rules = lyst_create()) == NULL) return -1;
    if(initResourceLock(&(gAgentVDB.rules_mutex))) return -1;
    agent_vdb_rules_init(sdr);

    return 1;
}


void agent_vdb_macros_init(Sdr sdr)
{
	/* \todo: implement. */
}

void agent_vdb_ops_init(Sdr sdr)
{
	/* \todo: implement. */
}

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

	/* Read in active rules. */

	sdr_begin_xn(sdr);

	for (elt = sdr_list_first(sdr, gAgentDB.reports); elt;
			elt = sdr_list_next(sdr, elt))
	{

		/* Step 1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc));

		cur_desc.descObj = descObj;

		/* Step 2: Allocate space for the def. */
		data = (uint8_t*) MTAKE(cur_desc.size);

		/* Step 3: Grab the serialized rule */
		sdr_read(sdr, (char *) data, cur_desc.itemObj, cur_desc.size);

		/* Step 4: Deserialize into a rule object. */
		cur_item = def_deserialize_gen(data,
									  cur_desc.size,
									  &bytes_used);

		/* Step 5: Copy current descriptor to cur_rule. */
		cur_item->desc = cur_desc;

		/* Step 6: Add report def to list of report defs. */
		ADD_REPORT(cur_item);

		num++;
	}
	sdr_end_xn(sdr);

	DTNMP_DEBUG_ALWAYS("", "Added %d Reports from DB.", num);
}

void agent_vdb_rules_init(Sdr sdr)
{
	Object elt;
	Object itemObj;
	Object descObj;
	rule_time_prod_desc_t cur_descr;
	rule_time_prod_t *cur_item;
	uint8_t *data;
	uint32_t bytes_used = 0;
	int num = 0;

	sdr_begin_xn(sdr);

	/* Read in active rules. */
	for (elt = sdr_list_first(sdr, gAgentDB.rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		/* Step 1: Grab the descriptor. */
		descObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &cur_descr, descObj, sizeof(cur_descr));

		cur_descr.descObj = descObj;

		/* Step 2: Allocate space for the rule. */
		data = (uint8_t*) MTAKE(cur_descr.size);

		/* Step 3: Grab the serialized rule */
		sdr_read(sdr, (char *) data, cur_descr.itemObj, cur_descr.size);

		/* Step 4: Deserialize into a rule object. */
		cur_item = ctrl_deserialize_time_prod_entry(data,
				                                    cur_descr.size,
				                                    &bytes_used);

		/* Step 5: Copy current descriptor to cur_rule. */
		cur_item->desc = cur_descr;

		/* Step 6: Add rule to list of active rules. */
		ADD_RULE(cur_item);

		num++;
	}

	sdr_end_xn(sdr);

	DTNMP_DEBUG_ALWAYS("", "Added %d Rules from DB.", num);
}



int main(int argc, char *argv[])
{
    pthread_t ingest_thr;
    pthread_t rda_thr;
    
    char ingest_thr_name[]  = "ingest_thread";
    char rda_thr_name[]     = "rda_thread";
    int rc;
    errno = 0;

    DTNMP_DEBUG_ENTRY("agent_main","(%d, 0x%x)", argc, (unsigned long)argv);
    
    if(argc != 3) {
        fprintf(stderr,"Usage: nmagent <agent eid> <manager eid>\n");
        return 1;
    }
    
    DTNMP_DEBUG_INFO("agent main","Agent EID: %s, Mgr EID: %s", argv[1], argv[2]);
    

    // Indicate that the threads should run once started.
    g_running = 1;

    // Setup the ION interface.
    /*
    if((ion_ptr = (iif_t *)MTAKE(sizeof(iif_t))) == NULL)
    {
    	DTNMP_DEBUG_ERR("agent_main","Can;'t alloc %d bytes.", sizeof(iif_t));
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
*/
    strcpy((char *) manager_eid.name, argv[2]);
    strcpy((char *) agent_eid.name, argv[1]);
   
	if (ionAttach() < 0)
	{
		DTNMP_DEBUG_ERR("agentDbInit", "Agent can't attach to ION.", NULL);
		return -1;
	}

    if(iif_register_node(&ion_ptr, agent_eid) != 1)
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to regster BP Node. Exiting.",
        		         NULL);
       // MRELEASE(ion_ptr);
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
        //MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   

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

    adm_init();

    agent_adm_init_bp();
#ifdef _HAVE_LTP_ADM_
    agent_adm_init_ltp();
#endif

#ifdef _HAVE_ION_ADM_
    agent_adm_init_ion();
#endif

    agent_adm_init_agent();

    // Start the receive thread. This thread will run continuously until the
    // g_running variable defined in nmagent.h is set to false.
    rc = pthread_create(&ingest_thr, NULL, rx_thread,
                        (void *)ingest_thr_name);
    if (rc)
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
        		ingest_thr_name, strerror(errno));
       // MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    // Start the production thread. This thread will run continuously until the
    // g_running variable defined in nmagent.h is set to false.
    rc = pthread_create(&rda_thr, NULL,
                        rda_thread,
                        (void *)rda_thr_name);
    if (rc)
    {
       DTNMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
    		           rda_thr_name, strerror(errno));
      // MRELEASE(ion_ptr);
       DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
       return -1;
    }
   
    DTNMP_DEBUG_ALWAYS("agent_main","Threads started...", NULL);
   
    DTNMP_DEBUG_ALWAYS("agent_main","Registering with Manager,", NULL);
    agent_register();



    // Program will wait until its child threads complete.
    if (pthread_join(ingest_thr, NULL))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           ingest_thr_name, strerror(errno));
        //MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }
       
    if (pthread_join(rda_thr, NULL))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           rda_thr_name, strerror(errno));
        //MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }
   

    adm_destroy();
    agent_vdb_destroy();

    DTNMP_DEBUG_INFO("agent_main","Exiting Agent after cleanup.", NULL);

    return 0;
}


/**
 * Signal handler for the nmagent main program.
 *
 * @param signum The received system signal number.
 **/
void signalHandler(int signum)
{
   if (signum == SIGINT || signum == SIGKILL || signum == SIGTERM)
      {
      g_running = 0;
      sleep(1); // Give threads a moment to terminate.
      fprintf(stderr,"nmagent terminated by user. Done.\n");
      exit(EXIT_SUCCESS);
      }
   else
      {
      perror("ERROR");
      fprintf(stderr,"nmagent terminated abnormally.\n");
      exit(EXIT_FAILURE);
      }
}





/***OLD
/ *
  * Create a rule descriptor. Serialize the rule. Populate descriptor.
 * If rule already in SDR, just over-write the descriptor portion in the SDR.
 * /
int persistDefinition(def_gen_t* def)
{
	Sdr sdr = getIonsdr();
	uint8_t *data;

	/ * Step 0: Sanity Checks. * /
	if((def == NULL) ||
	   ((def->desc.defObj == 0) && (def->desc.descObj != 0)) ||
	   ((def->desc.defObj != 0) && (def->desc.descObj == 0)))
	{
		DTNMP_DEBUG_ERR("persistDefinition","bad params.",NULL);
		return -1;
	}

	sdr_begin_xn(sdr);

	/ *
	 * Step 1: Determine if this def is already in the SDR. We will assume
	 *         the def is in the SDR already if its Object fields are nonzero.
	 * /

	if(def->desc.defObj == 0)
	{
	   DTNMP_DEBUG_INFO("persistDefinition","Adding new definition to SDR.",NULL);

	   / * Step 1.1: Allocate a descriptor object for this def in the SDR. * /
	   if((def->desc.descObj = sdr_malloc(sdr, sizeof(def_gen_desc_t))) == 0)
	   {
		   sdr_cancel_xn(sdr);

		   DTNMP_DEBUG_ERR("persistDefinition","Can't allocate def descriptor of size %d.",
			  	           sizeof(def_gen_desc_t));
		   return -1;
	   }

	   / * Step 1.2: Serialize the def to go into the SDR.. * /
	   if((data = def_serialize_gen(def, &(def->desc.def_size))) == NULL)
	   {

		   sdr_free(sdr, def->desc.descObj);
		   sdr_cancel_xn(sdr);

		   DTNMP_DEBUG_ERR("persistDefinition", "Unable to serialize new def.", NULL);
		   return -1;
	   }

	   / *  Step 1.3: Allocate space for the serialized rule in the SDR. * /
	   if((def->desc.defObj = sdr_malloc(sdr, def->desc.def_size)) == 0)
	   {
		   MRELEASE(data);
		   sdr_free(sdr, def->desc.descObj);
		   sdr_cancel_xn(sdr);
		   DTNMP_DEBUG_ERR("persistDefinition", "Unable to allocate Def in SDR. Size %d.",
				           def->desc.def_size);
		   return -1;
	   }

	   / * Step 1.4: Write the rule to the SDR. * /
	   sdr_write(sdr, def->desc.defObj, (char *) data, def->desc.def_size);
	   MRELEASE(data);

	   / * Step 1.5: Write the rule descriptor to the SDR. * /
		sdr_write(sdr, def->desc.descObj, (char *) &(def->desc),
				  sizeof(def_gen_desc_t));

		/ * Step 1.6: Save the descriptor in the AgentDB custom defs list. * /
		if (sdr_list_insert_last(sdr, gAgentDB.custom_defs, def->desc.descObj) == 0)
		{
			sdr_free(sdr, def->desc.defObj);
			sdr_free(sdr, def->desc.descObj);
			sdr_cancel_xn(sdr);
			DTNMP_DEBUG_ERR("persistDefinition", "Unable to insert Def Descr. in SDR.", NULL);
			return -1;
		}
	}
	else
	{
		def_gen_desc_t temp;

	    DTNMP_DEBUG_INFO("persistDefinition","Updating definition in SDR.",NULL);

		sdr_stage(sdr, (char*) &temp, def->desc.descObj, sizeof(def_gen_desc_t));
		temp = def->desc;
		sdr_write(sdr, def->desc.descObj, (char *) &temp, sizeof(def_gen_desc_t));
	}

	sdr_end_xn(sdr);

	return 1;
}





***/






