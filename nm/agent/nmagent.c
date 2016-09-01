/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
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
 *****************************************************************************/

// System headers.
#include "unistd.h"

// ION headers.
#include "platform.h"
#include "lyst.h"

// Application headers.
#include "../shared/adm/adm.h"
#include "../shared/utils/db.h"

#include "nmagent.h"
#include "ingest.h"
#include "rda.h"

#include "../shared/adm/adm_bp.h"
#include "../shared/adm/adm_agent.h"
#include "adm_ltp_priv.h"
#include "adm_ion_priv.h"

#include "../shared/primitives/nn.h"
#include "instr.h"

static void agent_signal_handler(int);


// Definitions of global data.
iif_t        ion_ptr;
uint8_t      gRunning;
eid_t        manager_eid;
eid_t        agent_eid;
AgentDB      gAgentDB;
AgentVDB     gAgentVDB;




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
		AMP_DEBUG_ERR("agent_register","Unable to create agent registration.",NULL);
		return;
	}

	/* Step 1: Serialize the message. */
	if((data = msg_serialize_reg_agent(reg, &len)) == NULL)
	{
		AMP_DEBUG_ERR("agent_register","Unable to serialize message.", NULL);
		msg_release_reg_agent(reg);
		return;
	}

	/* Step 2: Create the DTNMP message. */
    if((pdu_msg = pdu_create_msg(MSG_TYPE_ADMIN_REG_AGENT, data, len, NULL)) == NULL)
    {
    	AMP_DEBUG_ERR("agent_register","Unable to create PDU message.", NULL);
    	msg_release_reg_agent(reg);
    	SRELEASE(data);
    	return;
    }

    /* Step 3: Create a group for this message. */
    if((pdu_group = pdu_create_group(pdu_msg)) == NULL)
    {
    	AMP_DEBUG_ERR("agent_register","Unable to create PDU message.", NULL);
    	msg_release_reg_agent(reg);
    	SRELEASE(data);
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
 *****************************************************************************/

#if defined (ION_LWT)
int	nmagent(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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


    AMP_DEBUG_ENTRY("agent_main","(%d, 0x%#llx)", argc, (unsigned long)argv);

    /* Step 1: Sanity check. */
    if(argc != 3) {
        AMP_DEBUG_ALWAYS("main","Usage: nmagent <agent eid> <manager eid>\n", NULL);
        return 1;
    }
    
    if(((argv[0] == NULL) || (strlen(argv[0]) <= 0)) ||
       ((argv[1] == NULL) || (strlen(argv[1]) <= 0)) ||
       ((argv[2] == NULL) || (strlen(argv[2]) <= 0)))
    {
		AMP_DEBUG_ERR("agent_main", "Invalid Parameters (NULL or 0).", NULL);
		return -1;
    }

    AMP_DEBUG_INFO("agent main","Agent EID: %s, Mgr EID: %s", argv[1], argv[2]);
    
    /* Step 2: Note command-line arguments. */
    strcpy((char *) manager_eid.name, argv[2]);
    strcpy((char *) agent_eid.name, argv[1]);
   
    /* Step 3: Attach to ION and register. */
	if (ionAttach() < 0)
	{
		AMP_DEBUG_ERR("agentDbInit", "Agent can't attach to ION.", NULL);
		return -1;
	}

	utils_mem_int();

    if(iif_register_node(&ion_ptr, agent_eid) != 1)
    {
        AMP_DEBUG_ERR("agent_main","Unable to regster BP Node. Exiting.",
        		         NULL);
    	AMP_DEBUG_EXIT("agent_main","->-1",NULL);
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
    	AMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   

    /* Step 4: Read information from SDR and initialize memory lists.*/
    agent_instr_init();


    oid_nn_init();



    agent_db_init();

    if(agent_vdb_init() == -1)
    {
    	agent_vdb_destroy();

        AMP_DEBUG_ERR("agent_main","Unable to initialize VDB, errno = %s",
        		        strerror(errno));
       // SRELEASE(ion_ptr);
    	AMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }


    /* Step 5: Initialize ADM support. */
    adm_init();



    /* Step 6: Register signal handlers. */
    isignal(SIGINT, agent_signal_handler);
    isignal(SIGTERM, agent_signal_handler);

    /* Step 7: Start agent threads. */
    gRunning = 1;
    /*! use pthread_begin() so thread can be named and have its stacksize adjusted on some OS's */
    /*! and provide threads with a pointer to gRunning, so threads will shutdown */
    //rc = pthread_create(&ingest_thr, NULL, (void *)rx_thread, (void *)ingest_thr_name);
    rc = pthread_begin(&ingest_thr, NULL, (void *)rx_thread, (void *)&gRunning);

    if (rc)
    {
        AMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
        		ingest_thr_name, strerror(errno));
        adm_destroy();
        agent_vdb_destroy();

    	AMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    //rc = pthread_create(&rda_thr, NULL, (void *)rda_thread, (void *)rda_thr_name);
    rc = pthread_begin(&rda_thr, NULL, (void *)rda_thread, (void *)&gRunning);

    if (rc)
    {
       AMP_DEBUG_ERR("agent_main","Unable to create pthread %s, errno = %s",
    		           rda_thr_name, strerror(errno));
       adm_destroy();
       agent_vdb_destroy();

       AMP_DEBUG_EXIT("agent_main","->-1",NULL);
       return -1;
    }
   
    AMP_DEBUG_ALWAYS("agent_main","Threads started...", NULL);


    /* Step 8: Send out agent broadcast message. */
    agent_register();

    /* Step 9: Join threads and wait for them to complete. */
    if (pthread_join(ingest_thr, NULL))
    {
        AMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           ingest_thr_name, strerror(errno));
        adm_destroy();
        agent_vdb_destroy();

        AMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }

    if (pthread_join(rda_thr, NULL))
    {
        AMP_DEBUG_ERR("agent_main","Unable to join pthread %s, errno = %s",
     		           rda_thr_name, strerror(errno));
        adm_destroy();
        agent_vdb_destroy();

        AMP_DEBUG_EXIT("agent_main","->-1",NULL);
        return -1;
    }
   
    /* Step 10: Cleanup. */
    AMP_DEBUG_ALWAYS("agent_main","Cleaning Agent Resources.",NULL);
    adm_destroy();
    agent_vdb_destroy();

    oid_nn_cleanup();

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

static void agent_signal_handler(int i)
{
	isignal(SIGINT,agent_signal_handler);
	isignal(SIGTERM, agent_signal_handler);

	gRunning = 0;
}




