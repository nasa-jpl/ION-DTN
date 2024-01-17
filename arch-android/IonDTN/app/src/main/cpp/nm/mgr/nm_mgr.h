/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 ** \file nm_mgr.h
 **
 ** File Name: nm_mgr.h
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
 **  09/01/11  V. Ramachandran Initial Implementation (JHU/APL)
 **  08/19/13  E. Birrane      Documentation clean up and code review comments. (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef NM_MGR_H
#define NM_MGR_H

// Standard includes
#include "stdint.h"
#include "pthread.h"
#include "unistd.h"

// ION includes
#include "platform.h"
#include "lyst.h"
#include "sdr.h"

// Application includes

#include "../shared/utils/nm_types.h"
#include "../shared/utils/ion_if.h"

#include "../shared/adm/adm.h"

#include "../shared/primitives/mid.h"
#include "../shared/primitives/report.h"

#include "../shared/msg/pdu.h"
#include "../shared/msg/msg_admin.h"
#include "../shared/msg/msg_ctrl.h"

/* Constants */
static const int32_t NM_RECEIVE_TIMEOUT_MILLIS = 3600;
static const int32_t NM_RECEIVE_TIMEOUT_SEC = 2;

static const int32_t MSG_TYPE_SIZE             = 1; // Change this
static const int32_t TIMESTAMP_SIZE            = 4; // Change this too?
static const int32_t PENDING_LIST_LEN_SIZE     = 2; // Change this too.
static const char	 MGR_SDR_PROFILE_NAME[]	   = "NM_MGR";
static const int32_t MGR_SDR_HEAP_SIZE		   = 131072; // Do this intelligently?

// Likely number of managed agents.  Used to initialize the hashtable of agents.
#define EST_NUM_AGENTS (5)



// Indicates the allowable types of messages that an agent can send to
// the manager.
//static const unsigned char MSG_TYPE_REPORT[MSG_TYPE_SIZE] = {MSG_TYPE_RPT_DATA_RPT};


/**
 * Data structure representing a managed remote agent.
 **/
typedef struct {
	/**
	 * Agent's endpoint identifier.
	 **/
	eid_t agent_eid;
	
	/**
	 * Custom MID definitions that have been sent to this agent.
	 **/
	Lyst custom_defs;
	
	/**
	 * Reports that have been received from this agent.
	 **/
	Lyst reports;

	/**
	 * Mutex controlling read/write access to this structure.
	 **/
	ResourceLock mutex;
} agent_t;




// ============================= Global Data ===============================
/**
 * Indicates if the thread loops should continue to run. This
 * value is updated by the main() and read by the subordinate
 * threads.
 **/
 extern uint8_t g_running;

/**
 * Storage list for production rules sent by a manager and received
 * by the agent. These will be executed at a perscribed time (measured
 * in ticks) and when ready will be placed in the rules_pending list
 * for execution. References to this object should be made
 * within mutexes to make it thread-safe.
 **/
extern Object agents_hashtable;
extern Lyst known_agents;
extern ResourceLock agents_mutex;

extern Sdr g_sdr;

extern uint32_t g_reports_total;

/**
 * The endpoint identifier (EID) of the network manager node.
 **/
extern eid_t manager_eid;

/**
 * The interface object the ION system.
 **/
extern iif_t ion_ptr;



/* Function Prototypes */
int      main(int argc, char *argv[]);

agent_t* mgr_agent_get(eid_t* agent_eid);
int      mgr_agent_add(eid_t agent_eid);
agent_t* mgr_agent_create(eid_t* agent_eid);
int      mgr_agent_remove(eid_t* agent_eid);
void     mgr_agent_remove_cb(LystElt elt, void *nil);

int      mgr_cleanup();
int      mgr_init(char *argv[]);
void*    mgr_rx_thread(int *running);


#endif // NM_MGR_H
