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
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef _NM_AGENT_H
#define _NM_AGENT_H
#define DEBUG 1

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"
#include "lyst.h"

// Application includes
#include "shared/utils/nm_types.h"
#include "shared/utils/ion_if.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/rules.h"

#include "shared/msg/pdu.h"
#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_ctrl.h"
#include "shared/msg/msg_admin.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

static const int32_t NM_RECEIVE_TIMEOUT_MILLIS = 3600;



/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */





// Synchronously add a new production rule to the agent's production set (Thread-safe).
void addRule(rule_time_prod_t *r);

// Synchronously add a report definition to the agent's known set (Thread-safe).
void addCustomReportDefinition(def_gen_t *def);

void add_macro_definition(def_gen_t *macro_def);

void addControl(ctrl_exec_t *ctrl);

/**
 * Appends a new rule result to a buffer that will go into a report. This function
 * executes the rule and appends the result to the report buffer.
 *
 * @param rule the rule to execute
 * @param report_buf the buffer to append the rule result to
 * @param offset the position in the buffer to add the new rule
 * @return the number of bytes added to the report_buf
 **/
int appendReport(rule_time_prod_t *rule, unsigned char* report_buf, int offset);

/**
 * Returns the Endpoint Identifier (EID) of the network node responsible for
 * network management.
 *
 * @return the EID of the Network Manager
 **/
eid_t getNetworkMgrEid();



/*
 * +--------------------------------------------------------------------------+
 * |						 GLOBAL DATA DEFINITIONS  		         		  +
 * +--------------------------------------------------------------------------+
 */

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
extern Lyst rules_active;
extern ResourceLock rules_active_mutex;

/**
 * Holding area for rules that are ready to be evaluated and added
 * to a pending report. References to this object should be made
 * within mutexes to make it thread-safe.
 **/
extern Lyst rules_expired;
extern ResourceLock rules_expired_mutex;

extern Lyst custom_defs;
extern ResourceLock custom_defs_mutex;

extern Lyst macro_defs;
extern ResourceLock macro_defs_mutex;

extern Lyst exec_defs;
extern ResourceLock exec_defs_mutex;


/**
 * The endpoint identifier (EID) of the network manager node.
 **/
extern eid_t manager_eid;

/**
 * The endpoint identifier (EID) of this agent node.
 **/
extern eid_t agent_eid;

/**
 * The interface object the ION system.
 **/
extern iif_t ion_ptr;

#endif //_NM_AGENT_H_
