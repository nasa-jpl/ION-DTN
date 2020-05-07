/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
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
 **  09/01/11  V. Ramachandran Initial Implementation (JHU/APL)
 **  01/10/13  E. Birrane      Update to lasted DTNMP Spec. (JHU/APL)
 **  06/10/13  E. Birrane      Added SDR data persistence. (JHU/APL)
 **  10/04/18  E. Birrane      Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"

// Application headers.
#include "../shared/nm.h"
#include "../shared/adm/adm.h"
#include "../shared/utils/db.h"

#include "nmagent.h"
#include "ingest.h"
#include "rda.h"

#include "instr.h"

static void agent_signal_handler(int signum);


// Definitions of global data.
iif_t        ion_ptr;
uint8_t      gRunning;
eid_t        manager_eid;
eid_t        agent_eid;



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
 *  10/04/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

void agent_register()
{
	msg_agent_t *msg = NULL;

	if((msg = msg_agent_create()) == NULL)
	{
		AMP_DEBUG_ERR("agent_register","Unable to create agent registration.",NULL);
		return;
	}

	msg_agent_set_agent(msg, agent_eid);

	if(iif_send_msg(&ion_ptr, MSG_TYPE_REG_AGENT, msg, manager_eid.name) != AMP_OK)
	{
		AMP_DEBUG_ERR("agent_register","Couldn't send agent reg.", NULL);
	}

	msg_agent_release(msg, 1);
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
 **  02/23/15  E. Birrane      Updated to support ION_LWT targets
 **  10/04/18  E. Birrane      Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#if defined (ION_LWT) || defined(TEST_MAIN)
int	nmagent(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{

	/* Step 0: Sanity check. */
	int argc = 3;
	char *argv[3];

	argv[0] = (char *) a1;
	argv[1] = (char *) a2;
	argv[2] = (char *) a3;

#else
int	main(int argc, char *argv[])
{
#endif

    pthread_t ingest_thr;
    pthread_t rda_thr;
    
    char ingest_thr_name[]  = "ingest_thread";
    char rda_thr_name[]     = "rda_thread";
    int rc;
    errno = 0;


    AMP_DEBUG_ENTRY("agent_main","(%d,"ADDR_FIELDSPEC")", argc, (uaddr)argv);

    /* Step 1: Process Command Line Arguments. */
    if(argc != 3)
    {
        printf("Usage: nmagent <agent eid> <manager eid>\n");
        printf("AMP Protocol Version %d - %s/%02d, built on %s %s\n", AMP_VERSION, AMP_PROTOCOL_URL, AMP_VERSION, __DATE__, __TIME__);
        return 0;
    }
    
    if(((argv[0] == NULL) || (strlen(argv[0]) <= 0)) ||
       ((argv[1] == NULL) || (strlen(argv[1]) <= 0) || (strlen(argv[1]) >= AMP_MAX_EID_LEN)) ||
       ((argv[2] == NULL) || (strlen(argv[2]) <= 0) || (strlen(argv[2]) >= AMP_MAX_EID_LEN))
        )
    {
		AMP_DEBUG_ERR("agent_main", "Invalid Parameters (NULL or 0).", NULL);
		return -1;
    }

    AMP_DEBUG_INFO("agent main","Agent EID: %s, Mgr EID: %s", argv[1], argv[2]);
    
    strcpy((char *) manager_eid.name, argv[2]);
    strcpy((char *) agent_eid.name, argv[1]);


    /* Step 2: Make sure that ION is running and we can attach. */
	if (ionAttach() < 0)
	{
		AMP_DEBUG_ERR("agent_main", "Agent can't attach to ION.", NULL);
		return -1;
	}

    if(iif_register_node(&ion_ptr, agent_eid) != 1)
    {
        AMP_DEBUG_ERR("agent_main","Unable to register BP Node. Exiting.",
        		         NULL);
        return -1;
    }
   
    if (iif_is_registered(&ion_ptr))
    {
        AMP_DEBUG_INFO("agent_main","Agent registered with ION, EID: %s",
        		           iif_get_local_eid(&ion_ptr).name);
    }
    else
    {
        AMP_DEBUG_ERR("agent_main","Failed to register agent with ION, EID %s",
        		         iif_get_local_eid(&ion_ptr).name);
    	return -1;
    }
   

	/* Step 3: Initialize objects and instrumentation. */

	if((utils_mem_int()       != AMP_OK) ||
	   (db_init("nmagent_db",&adm_init) != AMP_OK))
	{
		db_destroy();
		AMP_DEBUG_ERR("agent_main","Unable to initialize DB.", NULL);
		return -1;
	}

	agent_instr_init();

    /* Step 4: Register signal handlers. */
    isignal(SIGINT, agent_signal_handler);
    isignal(SIGTERM, agent_signal_handler);


    /* Step 5: Start agent threads. */
    gRunning = 1;
    /*! use pthread_begin() so thread can be named and have its stacksize adjusted on some OS's */
    /*! and provide threads with a pointer to gRunning, so threads will shutdown */
    //rc = pthread_create(&ingest_thr, NULL, (void *)rx_thread, (void *)ingest_thr_name);
    rc = pthread_begin(&ingest_thr, NULL, (void *)rx_thread, (void *)&gRunning, "nmagent_ingest");

    if (rc)
    {
        AMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
        		ingest_thr_name, strerror(errno));

        db_destroy();

    	AMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    //rc = pthread_create(&rda_thr, NULL, (void *)rda_thread, (void *)rda_thr_name);
    rc = pthread_begin(&rda_thr, NULL, (void *)rda_thread, (void *)&gRunning, "nmagent_rda");

    if (rc)
    {
       AMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
    		           rda_thr_name, strerror(errno));

       db_destroy();

       AMP_DEBUG_EXIT("agent_main","->-1",NULL);
       return -1;
    }
   
    AMP_DEBUG_ALWAYS("agent_main","Threads started...", NULL);


    /* Step 6: Send out agent broadcast message. */
    agent_register();

    /* Step 7: Join threads and wait for them to complete. */
    if (pthread_join(ingest_thr, NULL))
    {
        AMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           ingest_thr_name, strerror(errno));

        db_destroy();

        AMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }

    if (pthread_join(rda_thr, NULL))
    {
        AMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           rda_thr_name, strerror(errno));

           db_destroy();

        AMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }
   
    /* Step 8: Cleanup. */
    AMP_DEBUG_ALWAYS("agent_main","Cleaning Agent Resources.",NULL);

    db_destroy();

    AMP_DEBUG_ALWAYS("agent_main","Stopping Agent.",NULL);

    AMP_DEBUG_INFO("agent_main","Exiting Agent after cleanup.", NULL);

	utils_mem_teardown();
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

static void agent_signal_handler(int signum)
{
	isignal(SIGINT,agent_signal_handler);
	isignal(SIGTERM, agent_signal_handler);

	gRunning = 0;
}
