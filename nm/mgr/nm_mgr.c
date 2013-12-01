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
 ** \file nm_mgr.c
 **
 ** File Name: nm_mgr.c
 **
 ** Subsystem:
 **          Network Manager Application
 **
 ** Description: This file implements the DTNMP Manager user interface
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR          DESCRIPTION
 **  --------  ------------    ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/19/13  E. Birrane      Documentation clean up and code review comments.
 *****************************************************************************/

// Application headers.
#include "nm_mgr.h"
#include "nm_mgr_ui.h"
#include "nm_mgr_db.h"
#include "shared/primitives/rules.h"


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

// This function looks to be completely unused at this time.
// To prevent compilation warnings, Josh Schendel commented it out on
//    Aug 22, 2013.
// Per comments regarding it's signal registerer, I suspect it should be
// uncommented once the UI thread is deprecated and signal handler routines
// are reactivated.
//
//static void     mgr_signal_handler();


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
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

int main(int argc, char *argv[])
{
    pthread_t rx_thr;
    pthread_t ui_thr;
    pthread_t daemon_thr;

    char rx_thr_name[]     = "rx_thread";
    char ui_thr_name[]     = "ui_thread";
    char daemon_thr_name[] = "run_daemon";

    errno = 0;

    if(argc != 2)
    {
    	fprintf(stderr,"Usage: nm_mgr <manager eid>\n");
        return 1;
    }

    /* Indicate that the threads should run once started. */
    g_running = 1;

    /* Initialize the DTNMP Manager. */
    if(mgr_init(argv) != 0)
    {
    	DTNMP_DEBUG_ERR("main","Can't init DTNMP Manager.", NULL);
    	exit(EXIT_FAILURE);
    }

    DTNMP_DEBUG_INFO("main","Manager EID: %s", argv[1]);

    /* Register signal handlers. */
    /* DOn't use signal handlers until we deprecate the UI thread... */
/*	isignal(SIGINT, mgr_signal_handler);
	isignal(SIGTERM, mgr_signal_handler);*/

    /* Spawn threads for receiving msgs, user interface, and db connection. */
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

#ifdef HAVE_MYSQL
    if(pthread_create(&daemon_thr, NULL, db_mgt_daemon, (void *)daemon_thr_name))
    {
    	DTNMP_DEBUG_ERR("main","Can't create pthread %s, errnor = %s",
    			daemon_thr_name, strerror(errno));
    	exit(EXIT_FAILURE);
    }
#endif

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

#ifdef HAVE_MYSQL
    if (pthread_join(daemon_thr, NULL))
    {
    	DTNMP_DEBUG_ERR("main","Can't join pthread %s. Errnor = %s",
    			        daemon_thr_name, strerror(errno));
    	exit(EXIT_FAILURE);
    }
#endif

    DTNMP_DEBUG_ALWAYS("main","Shutting down manager.", NULL);
    mgr_cleanup();

    DTNMP_DEBUG_INFO("main","Exiting Manager after cleanup.", NULL);
    exit(0);
}



/******************************************************************************
 *
 * \par Function Name: mgr_agent_get
 *
 * \par Retrieve an agent from the manager list of known agents.
 *
 * \param[in]  in_eid  - The endpoint identifier for the agent.
 *
 * \return NULL - Error
 *        !NULL - The retrieved agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

agent_t* mgr_agent_get(eid_t* in_eid)
{
	Object 	 *entry 		= NULL;
	agent_t *agent			= NULL;

	DTNMP_DEBUG_ENTRY("mgr_agent_get","(0x%#llx)", (unsigned long) in_eid);

	/* Step 0: Sanity Check. */
	if(in_eid == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_get","Null argument", NULL);
		DTNMP_DEBUG_EXIT("mgr_agent_get", "->NULL", NULL);
		return NULL;
	}

	/* Step 1: Lock agent mutex, walk list looking for match. */
	lockResource(&agents_mutex);

	LystElt elt = lyst_first(known_agents);
	while(elt != NULL)
	{
		agent = (agent_t *) lyst_data(elt);
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

	unlockResource(&agents_mutex);

	if(agent == NULL)
	{
		DTNMP_DEBUG_EXIT("mgr_agent_get", "->NULL", NULL);
	}
	else
	{
		DTNMP_DEBUG_EXIT("mgr_agent_get","->Agent %s", agent->agent_eid.name);
	}

	return agent;
}




/******************************************************************************
 *
 * \par Function Name: mgr_agent_add
 *
 * \par Add an agent to the manager list of known agents.
 *
 * \param[in]  in_eid  - The endpoint identifier for the new agent.
 *
 * \return -1 - Error
 *          0 - Duplicate, agent not added.
 *          1 - Success.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

int mgr_agent_add(eid_t agent_eid)
{
	int result = -1;
	agent_t *agent = NULL;

	DTNMP_DEBUG_ENTRY("mgr_agent_add","(%s)", agent_eid.name);

	lockResource(&agents_mutex);

	/* Check if the agent is already known. */
	if((agent = mgr_agent_get(&agent_eid)) != NULL)
	{
		unlockResource(&agents_mutex);
		result = 0;
		DTNMP_DEBUG_ERR("mgr_agent_add","Trying to add an already-known agent.", NULL);
		DTNMP_DEBUG_EXIT("mgr_agent_add","->%d.", result);

		return result;
	}

	/* Create and store the new agent. */
	if((agent = mgr_agent_create(&agent_eid)) == NULL)
	{
		unlockResource(&agents_mutex);

		DTNMP_DEBUG_ERR("mgr_agent_add","Failed to create agent.", NULL);
		DTNMP_DEBUG_EXIT("mgr_agent_add","->%d.", result);

		return result;
	}

	DTNMP_DEBUG_INFO("mgr_agent_add","Registered agent: %s", agent_eid.name);
	unlockResource(&agents_mutex);

	result = 1;
	DTNMP_DEBUG_EXIT("mgr_agent_add","->%d.", result);
	return result;
}




/******************************************************************************
 *
 * \par Function Name: mgr_agent_create
 *
 * \par Allocate and initialize a new agent structure.
 *
 * \param[in]  in_eid  - The endpoint identifier for the new agent.
 *
 * \return NULL - Error
 *         !NULL - Allocated, initialized agent structure.
 *
 * \par Notes:
 *		- We assume the agent mutex is already taken when this function is
 *		  called.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

agent_t* mgr_agent_create(eid_t *in_eid)
{
	Object  *entry = NULL;
	agent_t *agent	= NULL;

	DTNMP_DEBUG_ENTRY("mgr_agent_create", "(%s)", in_eid->name);

	/* Step 0: Sanity Check. */
	if(in_eid == NULL)
	{
		DTNMP_DEBUG_ENTRY("mgr_agent_create", "(NULL)", NULL);
		DTNMP_DEBUG_EXIT("mgr_agent_create", "->NULL", NULL);
		return NULL;
	}

	/* Step 1:  Allocate space for the new agent. */
	if((agent = (agent_t*)MTAKE(sizeof(agent_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_create",
				        "Unable to allocate %d bytes for new agent %s",
				        sizeof(agent_t), in_eid->name);
		DTNMP_DEBUG_EXIT("mgr_agent_create", "->NULL", NULL);
		return NULL;
	}

	/* Step 2: Copy over the name. */
	strncpy(agent->agent_eid.name, in_eid->name, MAX_EID_LEN);

	/* Step 3: Create associated lists. */
	if((agent->custom_defs = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_create","Unable to create custom definition lyst for agent %s",
				in_eid->name);
		MRELEASE(agent);

		DTNMP_DEBUG_EXIT("mgr_agent_create","->NULL", NULL);
		return NULL;
	}

	if((agent->reports = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_create","Unable to create report lyst for agent %s",
				in_eid->name);
		lyst_destroy(agent->custom_defs);
		MRELEASE(agent);

		DTNMP_DEBUG_EXIT("mgr_agent_create","->NULL", NULL);
		return NULL;
	}

	if(lyst_insert(known_agents, agent) == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_create","Unable to insert agent %s into known agents lyst",
				in_eid->name);
		lyst_destroy(agent->custom_defs);
		lyst_destroy(agent->reports);
		MRELEASE(agent);

		DTNMP_DEBUG_EXIT("mgr_agent_create","->NULL", NULL);
		return NULL;
	}

	initResourceLock(&(agent->mutex));

	DTNMP_DEBUG_EXIT("mgr_agent_create", "->New Agent %s", agent->agent_eid.name);
	return agent;
}





/******************************************************************************
 *
 * \par Function Name: mgr_agent_remove
 *
 * \par Remove and deallocate the agent identified by the given EID from the
 *      collection of known agents.
 *
 * \param[in]  in_eid  - The endpoint identifier for the agent.
 *
 * \return -1 - Error
 *         0  - Agent not found.
 *         1  - Success.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

int mgr_agent_remove(eid_t* in_eid)
{
	agent_t 	*agent 		= NULL;
	Object 		*entry		= NULL;
	LystElt		elt;

	DTNMP_DEBUG_ENTRY("mgr_agent_remove","(0x%#llx)", (unsigned long) in_eid);

	/* Step 0: Sanity Checks. */
	if(in_eid == NULL)
	{
		DTNMP_DEBUG_ERR("remove_agent","Specified EID was null.", NULL);
		DTNMP_DEBUG_EXIT("remove_agent","", NULL);
		return -1;
	}

	lockResource(&agents_mutex);

	elt = lyst_first(known_agents);
	while(elt != NULL)
	{
		if(strcmp(in_eid->name, ((agent_t *) lyst_data(elt))->agent_eid.name) == 0)
		{
			agent = (agent_t *) lyst_data(elt);
			lyst_delete(elt);
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

	rpt_clear_lyst(&(agent->reports), &(agent->mutex), 1);
	def_lyst_clear(&(agent->custom_defs), &(agent->mutex), 1);

	killResourceLock(&(agent->mutex));
	MRELEASE(agent);

	DTNMP_DEBUG_EXIT("remove_agent", "->1", NULL);
	return 1;
}




/******************************************************************************
 *
 * \par Function Name: mgr_agent_remove_cb
 *
 * \par Lyst callback to remove/delete agents in a lyst.
 *
 * \param[in]  elt - The lyst element holding the agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

void mgr_agent_remove_cb(LystElt elt, void *nil)
{
	eid_t   *agent_eid	= NULL;
	agent_t	*agent		= NULL;
	Object	*entry		= NULL;

	if(elt == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_remove_cb",
				        "Specified Lyst element was null.", NULL);
		DTNMP_DEBUG_EXIT("mgr_agent_remove_cb","", NULL);
		return;
	}

	lockResource(&agents_mutex);

	if((agent = (agent_t *) lyst_data(elt)) == NULL)
	{
		DTNMP_DEBUG_ERR("mgr_agent_remove_cb",
				        "Specified Lyst data was null.", NULL);
	}
	else
	{
		rpt_clear_lyst(&(agent->reports), &(agent->mutex), 1);
		def_lyst_clear(&(agent->custom_defs), &(agent->mutex), 1);

		killResourceLock(&(agent->mutex));
		MRELEASE(agent);
	}

	unlockResource(&agents_mutex);

	DTNMP_DEBUG_EXIT("mgr_agent_remove_cb","", NULL);

	return;
}




/******************************************************************************
 *
 * \par Function Name: mgr_cleanup
 *
 * \par Clesn resources before exiting the manager.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/

int mgr_cleanup()
{
	lyst_apply(known_agents, mgr_agent_remove_cb, NULL);
	lyst_destroy(known_agents);
	killResourceLock(&agents_mutex);

	def_lyst_clear(&(macro_defs), &(macro_defs_mutex), 1);
	killResourceLock(&macro_defs_mutex);

	return 0;
}



/******************************************************************************
 *
 * \par Function Name: mgr_init
 *
 * \par Initialize the manager...
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 *****************************************************************************/
int mgr_init(char *argv[])
{

	DTNMP_DEBUG_ENTRY("mgr_init","(0x%x)",(unsigned long) argv);

    strcpy((char *) manager_eid.name, argv[1]);

    if(iif_register_node(&ion_ptr, manager_eid) == 0)
    {
        DTNMP_DEBUG_ERR("mgr_init","Unable to register BP Node. Exiting.", NULL);
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
        DTNMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
        return -1;
    }

    g_sdr = getIonsdr();

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

#ifdef HAVE_MYSQL
	db_mgt_init("localhost", "root", "NetworkManagement", "dtnmp", 1);
#endif

	DTNMP_DEBUG_EXIT("mgr_init","->0.",NULL);
    return 0;
}





/******************************************************************************
 *
 * \par Function Name: mgr_signal_handler
 *
 * \par Catches kill signals and gracefully shuts down the manager.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  08/18/13  E. Birrane    Initial Implementation
 *****************************************************************************/

// This function looks to be completely unused at this time.
// To prevent compilation warnings, Josh Schendel commented it out on
//    Aug 22, 2013.
// Per comments regarding it's signal registerer, I suspect it should be
// uncommented once the UI thread is deprecated and signal handler routines
// are reactivated.
//
// static void mgr_signal_handler()
// {
// 	isignal(SIGINT, mgr_signal_handler);
// 	isignal(SIGTERM, mgr_signal_handler);
// 
// 	g_running = 0;
// }


