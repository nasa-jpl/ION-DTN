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

#include "nmagent.h"
#include "ingest.h"
#include "rda.h"

// System signal handler.
void signalHandler(int signum);

// Definitions of global data.
iif_t        ion_ptr;
uint8_t      g_running;
Lyst         rules_active;
Lyst         rules_expired;
Lyst 		 custom_defs;
Lyst 		 exec_defs;
Lyst         macro_defs;
eid_t        manager_eid;
eid_t        agent_eid;
ResourceLock rules_active_mutex;
ResourceLock rules_expired_mutex;
ResourceLock custom_defs_mutex;
ResourceLock exec_defs_mutex;
ResourceLock macro_defs_mutex;

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
        		          (char *) iif_get_local_eid(&ion_ptr).name);
    }
    else
    {
        DTNMP_DEBUG_ERR("agent_main","Failed to register agent with ION, EID %s",
        		        (char *) iif_get_local_eid(&ion_ptr).name);
        //MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    // List of reporting rules that the agent knows about as defined in the
    // configuration messages sent from the manager.
    rules_active      = lyst_create();
    if (initResourceLock(&rules_active_mutex))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to initialize rules_active_mutex, errno = %s",
        		        strerror(errno));
       // MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    // Reporting rules that have been selected to form a report that are
    // awaiting their execution times.
    rules_expired = lyst_create();
    if (initResourceLock(&rules_expired_mutex))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to initialize rules_expired_mutex, errno = %s",
        		        strerror(errno));
        //MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }
   
    custom_defs = lyst_create();
    if (initResourceLock(&custom_defs_mutex))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to initialize custom_defs_mutex, errno = %s",
        		        strerror(errno));
        //MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }

    macro_defs = lyst_create();
    if (initResourceLock(&macro_defs_mutex))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to initialize macro_defs_mutex, errno = %s",
        		        strerror(errno));
        //MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }

    exec_defs = lyst_create();
    if (initResourceLock(&exec_defs_mutex))
    {
        DTNMP_DEBUG_ERR("agent_main","Unable to initialize exec_defs_mutex, errno = %s",
        		        strerror(errno));
        //MRELEASE(ion_ptr);
    	DTNMP_DEBUG_EXIT("agent_main","->-1",NULL);
    	return -1;
    }


    adm_init();

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
   
    DTNMP_DEBUG_INFO("agent_main","Threads started...", NULL);
   
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
   
    // Cleanup lists and mutexes when done.
    LystElt elt;
    rule_time_prod_t* rule_p = NULL;

    clearDefsLyst(custom_defs);
    killResourceLock(&custom_defs_mutex);

    clearDefsLyst(macro_defs);
    killResourceLock(&macro_defs_mutex);

    for (elt = lyst_first(rules_active); elt; elt = lyst_next(elt))
    {
        rule_p = (rule_time_prod_t*) lyst_data(elt);
        if (rule_p != NULL)
        {
            MRELEASE(rule_p);
        }
    }

   lyst_destroy(rules_active);
   killResourceLock(&rules_active_mutex);
   
   for (elt = lyst_first(rules_expired); elt; elt = lyst_next(elt))
      {
      rule_p = (rule_time_prod_t*) lyst_data(elt);
      if (rule_p != NULL)
         {
            MRELEASE(rule_p);
         }
      }

   lyst_destroy(rules_expired);
   killResourceLock(&rules_expired_mutex);
   
    DTNMP_DEBUG_INFO("agent_main","Exiting Agent after cleanup.", NULL);
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

void addRule(rule_time_prod_t* r)
{
   lockResource(&rules_active_mutex);
   lyst_insert_first(rules_active, r);
   unlockResource(&rules_active_mutex);
}

void addCustomReportDefinition(def_gen_t *r)
{
	lockResource(&custom_defs_mutex);
    lyst_insert_last(custom_defs, r);
    unlockResource(&custom_defs_mutex);
}

void add_macro_definition(def_gen_t *macro_def)
{
	lockResource(&macro_defs_mutex);
    lyst_insert_last(macro_defs, macro_def);
    unlockResource(&macro_defs_mutex);
}

void addControl(ctrl_exec_t *ctrl)
{
	lockResource(&exec_defs_mutex);
    lyst_insert_last(exec_defs, ctrl);
    unlockResource(&exec_defs_mutex);
}


int appendReport(rule_time_prod_t rule, unsigned char* report_buf, int offset)
{
   // TODO:This function will need to be implemented at a later time.
   // Consider pulling this logic into another function because the
   // number of MIDs will be large.
    
    return 0;
}

