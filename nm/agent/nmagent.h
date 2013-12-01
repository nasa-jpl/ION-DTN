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

static const int32_t NM_RECEIVE_TIMEOUT_SEC = 1;



/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


#define ADD_COMPDATA(x) agent_vdb_add(x, gAgentVDB.compdata, &(gAgentVDB.compdata_mutex));
#define ADD_CONST(x) agent_vdb_add(x, gAgentVDB.consts, &(gAgentVDB.consts_mutex));
#define ADD_CTRL(x) agent_vdb_add(x, gAgentVDB.ctrls, &(gAgentVDB.ctrls_mutex));
#define ADD_MACRO(x) agent_vdb_add(x, gAgentVDB.macros, &(gAgentVDB.macros_mutex));
#define ADD_OP(x) agent_vdb_add(x, gAgentVDB.ops, &(gAgentVDB.ops_mutex));
#define ADD_REPORT(x) agent_vdb_add(x, gAgentVDB.reports, &(gAgentVDB.reports_mutex));
#define ADD_RULE(x) agent_vdb_add(x, gAgentVDB.rules, &(gAgentVDB.rules_mutex));




/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/*
 * This structure implements the DTNMP Agent SDR database which keeps a list
 * of all agent information that must be persisted across a reset.
 *
 * Each object in the database represents an sdr_list. Each list is populated
 * with a series of pointers to "descriptor" objects.  Each descriptor object
 * captures meta-data associated with the associated DTNMP object and a
 * pointer to that object. DTNMP objects are stored in the SDR in their
 * message-serialized form as that comprises the most space-efficient
 * representation of the object.
 *
 * On agent startup, DTNMP object types are deserialized and copied into
 * associated RAM data types.
 *
 * For example, the active_rules Object in the database is an sdr_list. Each
 * item in the active_rules list is an Object which points to a
 * rule_time_prod_desc_t object.  This descriptor object captures information
 * such as the execution state of the active rule. One of the entries of the
 * rule_time_prod_desc_t object is an Object pointer to the serialized
 * rule in the SDR.  On system startup, the agent will grab the descriptor
 * object and use it to find the serialized rule. The rule will be de-serialized
 * into a new rule structure.  The meta-data associated with the rule will be
 * populated by additional meta-data in the rule's descriptor object.
 *
 * All entities in the DB operate in this way, as follows.
 *
 * LIST          Descriptor Type          Type that we deserialize into
 * -------------+-----------------------+------------------------------
 * active_rules | rule_time_prod_desc_t | rule_time_prod_t
 * custom_defs  | def_gen_desc_t        | def_gen_t
 * macro_defs   | def_gen_desc_t        | def_gen_t
 * exec_defs    | ctrl_exec_desc_t      | ctrl_exec_t
 *
 */

typedef struct
{
   Object  compdata;    /* SDR list: def_gen_descr_t        */
   Object  consts;
   Object  ctrls;      /* SDR list: ctrl_exec_descr_t      */
   Object  macros;     /* SDR list: def_gen_descr_t        */
   Object  ops;
   Object  reports;
   Object  rules;   /* SDR list: rule_time_prod_descr_t */

   Object  descObj; /**> The pointer to the AgentDB object in the SDR. */
} AgentDB;


/*
 * This structure captures the "volatile" database, which is the set of
 * information relating to configured items kept in the agent's memory.
 */

typedef struct
{
	Lyst  compdata;
	Lyst  consts;
	Lyst  ctrls;
	Lyst  macros;
	Lyst  ops;
	Lyst  reports;
	Lyst  rules;


	ResourceLock compdata_mutex;
	ResourceLock consts_mutex;
	ResourceLock ctrls_mutex;
	ResourceLock macros_mutex;
	ResourceLock ops_mutex;
	ResourceLock reports_mutex;
	ResourceLock rules_mutex;

} AgentVDB;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int  agent_db_compdata_persist(ctrl_exec_t* item);
int  agent_db_const_persist(ctrl_exec_t* item);
int  agent_db_ctrl_persist(ctrl_exec_t* item);
int  agent_db_init();
int  agent_db_macro_persist(def_gen_t* item);
int  agent_db_op_persist(def_gen_t* item);
int  agent_db_report_persist(def_gen_t* item);
int  agent_db_rule_persist(rule_time_prod_t* item);


void agent_register();

void agent_vdb_add(void *item, Lyst list, ResourceLock *mutex);
void agent_vdb_compdata_init(Sdr sdr);
void agent_vdb_consts_init(Sdr sdr);
void agent_vdb_ctrls_init(Sdr sdr);
void agent_vdb_destroy();
int  agent_vdb_init();
void agent_vdb_macros_init(Sdr sdr);
void agent_vdb_ops_init(Sdr sdr);
void agent_vdb_reports_init(Sdr sdr);
void agent_vdb_rules_init(Sdr sdr);



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

/**
 * Holding area for rules that are ready to be evaluated and added
 * to a pending report. References to this object should be made
 * within mutexes to make it thread-safe.
 **/

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

extern AgentVDB gAgentVDB;
extern AgentDB gAgentDB;

#endif //_NM_AGENT_H_
