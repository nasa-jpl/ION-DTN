//
//  nmagent.h
//  DTN NM Agent


#ifndef NM_MGR_H
#define NM_MGR_H

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"
#include "lyst.h"
#include "sdr.h"

// Application includes

#include "shared/utils/nm_types.h"
#include "shared/utils/ion_if.h"

#include "shared/adm/adm.h"

#include "shared/primitives/mid.h"

#include "shared/msg/pdu.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_admin.h"
#include "shared/msg/msg_ctrl.h"

/* Constants */
static const int32_t NM_RECEIVE_TIMEOUT_MILLIS = 3600;
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

/**
 * Retrieve an Agent from the agents_hashtable collection.
 *
 * @param agent_eid Endpoint identifier of desired agent
 * @return Reference to agent information, or NULL if not found.
 */
agent_t* get_agent(eid_t* agent_eid);

int add_agent(eid_t agent_eid);


/**
 * Create and store a metadata object for information about a new agent.
 *
 * @param agent_eid Endpoint identifier of agent
 * @return Reference to new agent, or NULL on error.
 */
agent_t* create_agent(eid_t* agent_eid);

/**
 * Remove and deallocate the agent identified by agent_eid from the collection
 * of known agents.
 *
 * @param agent_eid Endpoint identifier of agent.
 * @return 1 if the agent was successfully removed, 0 if no such agent found,
 * -1 on any error.
 */
int remove_agent(eid_t* agent_eid);

/**
 * The DTN Network Management Agent receive loop function.
 * This function is intended to be run from within a Posix thread.
 * Awaits and receives production rules and report definitions
 * from the network managemer, interprets them, and adds them to
 * the rules list and reports list.
 *
 * @param threadId the Posix thread ID
 **/
void* mgr_rx_thread(void* threadId);

/**
 * Back-end daemon that checks the database and builds messages from it.
 */
void *run_daemon(void *threadId);

/**
 * Returns the Endpoint Identifier (EID) of the network node responsible for
 * network management.
 *
 * @return the EID of the Network Manager
 **/
eid_t get_network_mgr_eid();


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

extern Lyst macro_defs;
extern ResourceLock macro_defs_mutex;

extern uint32_t g_reports_total;

/**
 * The endpoint identifier (EID) of the network manager node.
 **/
extern eid_t manager_eid;

/**
 * The interface object the ION system.
 **/
extern iif_t ion_ptr;

#endif // NM_MGR_H
