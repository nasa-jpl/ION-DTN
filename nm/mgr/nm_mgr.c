//
//  nmagent.cpp
//  DTN NM Agent
//
//  Created by Ramachandran, Vignesh R. (Vinny) on 9/1/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

// System headers.

#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"
#include "sdr.h"
#include "sdrhash.h"

// Application headers.
#include "nm_mgr.h"
#include "nm_mgr_ui.h"
#include "shared/primitives/rules.h"


// System signal handler.
void signal_handler(int signum);

// Definitions of global data.
iif_t ion_ptr;

uint8_t      g_running;

Object		 agents_hashtable;
Lyst		 known_agents;
ResourceLock agents_mutex;

Lyst 		 macro_defs;
ResourceLock macro_defs_mutex;

eid_t        manager_eid;
eid_t        agent_eid;

uint32_t	 g_reports_total = 0;

Sdr 		 g_sdr;

int mgr_init(char *argv[])
{

	DTNMP_DEBUG_ENTRY("mgr_init","(0x%x)",(unsigned long) argv);

    // Setup the ION interface.
  //  ion_ptr = (iif_t*) MTAKE(sizeof(iif_t));

    strcpy((char *) manager_eid.name, argv[1]);
//    strcpy((char *) agent_eid.name, argv[2]);

    if(iif_register_node(&ion_ptr, manager_eid) == 0)
    {
        DTNMP_DEBUG_ERR("mgr_init","Unable to register BP Node. Exiting.", NULL);
    	//MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
        return -1;
    }

    if (iif_is_registered(&ion_ptr))
    {
        DTNMP_DEBUG_INFO("mgr_init", "Mgr registered with ION, EID: %s",
        		         iif_get_local_eid(&ion_ptr).name);
    }
    else
    {
        DTNMP_DEBUG_ERR("mgr_init","Failed to register mgr with ION, EID %s",
        				 iif_get_local_eid(&ion_ptr).name);
        //MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
        return -1;
    }

//	if(sdr_load_profile(
//			(char *) MGR_SDR_PROFILE_NAME,
//			SDR_IN_DRAM,
//			MGR_SDR_HEAP_SIZE,
//			SM_NO_KEY,
//			(char *) MGR_SDR_PROFILE_NAME) != 0)
//	{
//		DTNMP_DEBUG_ERR("mgr_init","Unable to initialize SDR profile. Exiting.", NULL);
//		//MRELEASE(ion_ptr);
//		DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
//		return -1;
//	}
//
//	if((g_sdr = sdr_start_using((char *) MGR_SDR_PROFILE_NAME)) == NULL)
//	{
//		DTNMP_DEBUG_ERR("mgr_init","Unable to start using SDR profile. Exiting.", NULL);
//		//MRELEASE(ion_ptr);
//		DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
//		return -1;
//	}

    g_sdr = getIonsdr();

//    sdr_begin_xn(g_sdr);
//    printf("Sdr in transaction: %d\n", sdr_in_xn(g_sdr));
//    if((agents_hashtable = sdr_hash_create(
//    		g_sdr,
//    		MAX_EID_LEN,
//    		EST_NUM_AGENTS,
//    		2
//    	)) == 0)
//    {
//        DTNMP_DEBUG_ERR("mgr_init","Failed to create agents hash table.",NULL);
//        //MRELEASE(ion_ptr);
//        sdr_cancel_xn(g_sdr);
//        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
//        return -1;
//    }
//    sdr_end_xn(g_sdr);

    if((known_agents = lyst_create()) == NULL)
        {
            DTNMP_DEBUG_ERR("mgr_init","Failed to create known agents list.",NULL);
            //MRELEASE(ion_ptr);
            DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
            return -1;
        }

    if (initResourceLock(&agents_mutex))
    {
    	DTNMP_DEBUG_ERR("mgr_init","Can't initialize rcv rpt list. . errno = %s",
    			        strerror(errno));
        //MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
    	return -1;
    }

    if((macro_defs = lyst_create()) == NULL)
    {
        DTNMP_DEBUG_ERR("mgr_init","Failed to create macro def list.%s",NULL);
        //MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
        return -1;
    }

    if (initResourceLock(&macro_defs_mutex))
    {
    	DTNMP_DEBUG_ERR("mgr_init","Cant init macro def list mutex. errno = %s",
    			        strerror(errno));
        //MRELEASE(ion_ptr);
        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
    	return -1;
    }


	adm_init();

	DTNMP_DEBUG_EXIT("mgr_init","->0.",NULL);
    return 0;
}

Agent_rx* get_agent(eid_t* in_eid)
{
	Object 	 *entry 		= NULL;
	Agent_rx *agent			= NULL;

	if(in_eid == NULL)
	{
		DTNMP_DEBUG_ERR("get_agent","Null argument", NULL);
		DTNMP_DEBUG_EXIT("get_agent", "->NULL", NULL);
		return NULL;
	}
	else
		DTNMP_DEBUG_ENTRY("get_agent", "(%s)", in_eid->name);

	lockResource(&agents_mutex);

	// Try to find existing entry
//	if(sdr_hash_retrieve(
//			g_sdr,
//			agents_hashtable,
//			in_eid->name,
//			(Address *)agent,
//			entry) != 1)
	LystElt elt = lyst_first(known_agents);
	while(elt != NULL)
	{
		agent = (Agent_rx *) lyst_data(elt);
		if(strcmp(in_eid->name, agent->agent_eid.name) == 0)
		{
			break;
		}
		else
		{
			agent = NULL;
			elt = lyst_next(elt);
		}
	}
	if(agent == NULL)
	{
		DTNMP_DEBUG_ERR("get_agent", "Unable to retrieve agent %s from hashtable", in_eid->name);
		DTNMP_DEBUG_EXIT("get_agent", "->NULL", NULL);
		unlockResource(&agents_mutex);
		return NULL;
	}
	else
	{
		DTNMP_DEBUG_EXIT("get_agent","->Agent %s", agent->agent_eid.name);
	}

	unlockResource(&agents_mutex);

	return agent;
}

Agent_rx* create_agent(eid_t *in_eid)
{
	Object 	 *entry 		= NULL;
	Agent_rx *agent			= NULL;

	if(in_eid == NULL)
	{
		DTNMP_DEBUG_ENTRY("create_agent", "(NULL)", NULL);
		DTNMP_DEBUG_EXIT("create_agent", "->NULL", NULL);
		return NULL;
	}

	DTNMP_DEBUG_ENTRY("create_agent", "(%s)", in_eid->name);

	// Allocate new agent metadata
	if((agent = (Agent_rx*)MTAKE(sizeof(Agent_rx))) == NULL)
	{
		DTNMP_DEBUG_ERR("create_agent","Unable to allocate %d bytes for new agent %s",
				sizeof(Agent_rx), in_eid->name);
		DTNMP_DEBUG_EXIT("create_agent", "->NULL", NULL);
		unlockResource(&agents_mutex);
		return NULL;
	}

	strcpy(agent->agent_eid.name, in_eid->name);

	if((agent->custom_defs = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("create_agent","Unable to create custom definition lyst for agent %s",
				in_eid->name);
		DTNMP_DEBUG_EXIT("create_agent","->NULL", NULL);
		MRELEASE(agent);
		unlockResource(&agents_mutex);
		return NULL;
	}

	if((agent->reports = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("create_agent","Unable to create report lyst for agent %s",
				in_eid->name);
		DTNMP_DEBUG_EXIT("create_agent","->NULL", NULL);
		lyst_destroy(agent->custom_defs);
		MRELEASE(agent);
		unlockResource(&agents_mutex);
		return NULL;
	}

	// Place the new agent in the hashtable.
//	if(sdr_hash_insert(
//			g_sdr,
//			agents_hashtable,
//			agent->agent_eid.name,
//			(Address) agent,
//			entry) != 0)
//	{
//		DTNMP_DEBUG_ERR("create_agent","Unable to insert agent %s into hashtable",
//				in_eid->name);
//		DTNMP_DEBUG_EXIT("create_agent","->NULL", NULL);
//		lyst_destroy(agent->custom_defs);
//		lyst_destroy(agent->reports);
//		MRELEASE(agent);
//		unlockResource(&agents_mutex);
//		return NULL;
//	}

	// Place the name of the agent in the known agents list.
//	if(lyst_insert(known_agents, &(agent->agent_eid)) == NULL)
	if(lyst_insert(known_agents, agent) == NULL)
	{
		DTNMP_DEBUG_ERR("create_agent","Unable to insert agent %s into known agents lyst",
				in_eid->name);
		DTNMP_DEBUG_EXIT("create_agent","->NULL", NULL);
		lyst_destroy(agent->custom_defs);
		lyst_destroy(agent->reports);
//		sdr_hash_delete_entry(g_sdr, *entry);
		MRELEASE(agent);
		unlockResource(&agents_mutex);
		return NULL;
	}

	initResourceLock(&(agent->mutex));

	unlockResource(&agents_mutex);
	DTNMP_DEBUG_EXIT("create_agent", "->New Agent %s", agent->agent_eid.name);
	return agent;
}

int remove_agent(eid_t* in_eid)
{
	Agent_rx 	*agent 		= NULL;
	Object 		*entry		= NULL;
	LystElt		elt;

	if(in_eid == NULL)
	{
		DTNMP_DEBUG_ENTRY("remove_agent","(NULL)", NULL);
		DTNMP_DEBUG_ERR("remove_agent","Specified EID was null.", NULL);
		DTNMP_DEBUG_EXIT("remove_agent","", NULL);
		return -1;
	}

	DTNMP_DEBUG_ENTRY("remove_agent","(%s)", in_eid->name);

	lockResource(&agents_mutex);


//	if(sdr_hash_remove(
//			g_sdr,
//			agents_hashtable,
//			agent_eid->name,
//			(Address *)agent) != 1)
//	{
//		DTNMP_DEBUG_ERR("remove_agent","Unable to retrieve specified agent from hashtable.", NULL);
//		DTNMP_DEBUG_EXIT("remove_agent","", NULL);
//		unlockResource(&agents_mutex);
//		return 0;
//	}

	elt = lyst_first(known_agents);
	while(elt != NULL)
	{
		if(strcmp(in_eid->name, ((Agent_rx *) lyst_data(elt))->agent_eid.name) == 0)
		{
			agent = (Agent_rx *) lyst_data(elt);
			break;
		}
		else
		{
			elt = lyst_next(elt);
		}
	}

	unlockResource(&agents_mutex);

	if(agent == NULL)
	{
		DTNMP_DEBUG_ERR("remove_agent", "No agent %s found in hashtable", in_eid->name);

		DTNMP_DEBUG_EXIT("remove_agent", "->0", NULL);
		return 0;
	}
	else
	{
		lockResource(&(agent->mutex));
		rpt_clear_lyst(agent->reports);
		clearDefsLyst(agent->custom_defs);
		unlockResource(&(agent->mutex));
		killResourceLock(&(agent->mutex));
		MRELEASE(agent);

		DTNMP_DEBUG_EXIT("remove_agent", "->1", NULL);
		return 1;
	}

}

void _remove_agent(LystElt elt, void *nil)
{
	eid_t 		*agent_eid	= NULL;
	Agent_rx	*agent		= NULL;
	Object		*entry		= NULL;

	if(elt == NULL)
	{
		DTNMP_DEBUG_ERR("remove_agent","Specified Lyst element was null.", NULL);
		DTNMP_DEBUG_EXIT("remove_agent","", NULL);
		return;
	}

	lockResource(&agents_mutex);

//	if((agent_eid = (eid_t *) lyst_data(elt)) == NULL)
	if((agent = (Agent_rx *) lyst_data(elt)) == NULL)
	{
		DTNMP_DEBUG_ERR("remove_agent","Specified Lyst data was null.", NULL);
		DTNMP_DEBUG_EXIT("remove_agent","", NULL);
		unlockResource(&agents_mutex);
		return;
	}

//	if(sdr_hash_remove(
//			g_sdr,
//			agents_hashtable,
//			agent_eid->name,
//			(Address *)agent) != 1)
//	{
//		DTNMP_DEBUG_ERR("remove_agent","Unable to retrieve specified agent from hashtable.", NULL);
//		DTNMP_DEBUG_EXIT("remove_agent","", NULL);
//		unlockResource(&agents_mutex);
//		return;
//	}

	unlockResource(&agents_mutex);

	lockResource(&(agent->mutex));
	rpt_clear_lyst(agent->reports);
	clearDefsLyst(agent->custom_defs);
	unlockResource(&(agent->mutex));
	killResourceLock(&(agent->mutex));
	MRELEASE(agent);

}

int mgr_cleanup()
{
	lyst_apply(known_agents, _remove_agent, NULL);
	lyst_destroy(known_agents);
	killResourceLock(&agents_mutex);

//	sdr_hash_destroy(g_sdr, agents_hashtable);

	clearDefsLyst(macro_defs);
	lyst_destroy(macro_defs);
	killResourceLock(&macro_defs_mutex);

	return 0;
}


int main(int argc, char *argv[])
{
    pthread_t rx_thr;
    pthread_t ui_thr;

    char rx_thr_name[]  = "rx_thread";
    char ui_thr_name[] = "ui_thread";

    errno = 0;
    
    if(argc != 2) {
    	fprintf(stderr,"Usage: nm_mgr <manager eid>\n");
        return 1;
    }
    
    DTNMP_DEBUG_INFO("main","Manager EID: %s", argv[1]);
    
    // Indicate that the threads should run once started.
    g_running = 1;
   
    /* Initialize the DTNMP Manager. */
    if(mgr_init(argv) != 0)
    {
    	DTNMP_DEBUG_ERR("main","Can't init DTNMP Manager.", NULL);
    	exit(EXIT_FAILURE);
    }

    /* Spawn a thread to asynchronously receive reports */
    if(pthread_create(&rx_thr, NULL, mgr_rx_thread, (void *)rx_thr_name))
    {
        DTNMP_DEBUG_ERR("main","Can't create pthread %s, errnor = %s",
        		        rx_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&ui_thr, NULL, ui_thread, (void *)ui_thr_name))
    {
        DTNMP_DEBUG_ERR("main","Can't create pthread %s, errnor = %s",
        		        ui_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    DTNMP_DEBUG_INFO("main","Spawning threads...", NULL);

    if (pthread_join(rx_thr, NULL))
    {
        DTNMP_DEBUG_ERR("main","Can't join pthread %s. Errnor = %s",
        		        rx_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ui_thr, NULL))
    {
        DTNMP_DEBUG_ERR("main","Can't join pthread %s. Errnor = %s",
        		         ui_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    mgr_cleanup();

    DTNMP_DEBUG_INFO("main","Exiting Manager after cleanup.", NULL);

    exit(0);
}


/**
 * Signal handler for the nmmgr main program.
 *
 * @param signum The received system signal number.
 **/
void signal_handler(int signum)
{
   if (signum == SIGINT || signum == SIGKILL || signum == SIGTERM)
      {
      g_running = 0;
      sleep(1); // Give threads a moment to terminate.
      DTNMP_DEBUG_INFO("sig_handler","mgr terminated by user. Done.", NULL);
      exit(EXIT_SUCCESS);
      }
   else
      {
      perror("ERROR");
      DTNMP_DEBUG_ERR("sig_handler","mgr terminated abnormally.", NULL);
      exit(EXIT_FAILURE);
      }
}

