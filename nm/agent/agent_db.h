/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: agent_db.h
 **
 ** Description: This module captures the functions, structures, and operations
 **              necessary to store and retrieve user-defined content from the
 **              embedded agent on system startup.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  05/17/15  E. Birrane     Update to new data types (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane     Update naming to AMP v0.3. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef _NM_AGENT_DB_H
#define _NM_AGENT_DB_H

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"
#include "lyst.h"

#include "../shared/primitives/var.h"
#include "../shared/utils/nm_types.h"
#include "../shared/utils/ion_if.h"

#include "../shared/primitives/mid.h"
#include "../shared/primitives/rules.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/def.h"
#include "../shared/msg/pdu.h"
#include "../shared/msg/msg_ctrl.h"
#include "../shared/msg/msg_admin.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


#define ADD_VAR(x)    agent_vdb_add(x, gAgentVDB.vars,     &(gAgentVDB.var_mutex));
#define ADD_CTRL(x)   agent_vdb_add(x, gAgentVDB.ctrls,    &(gAgentVDB.ctrls_mutex));
#define ADD_MACRO(x)  agent_vdb_add(x, gAgentVDB.macros,   &(gAgentVDB.macros_mutex));
#define ADD_REPORT(x) agent_vdb_add(x, gAgentVDB.reports,  &(gAgentVDB.reports_mutex));
#define ADD_TRL(x)    agent_vdb_add(x, gAgentVDB.trls,     &(gAgentVDB.trls_mutex));
#define ADD_SRL(x)    agent_vdb_add(x, gAgentVDB.srls,     &(gAgentVDB.srls_mutex));

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/*
 * This structure implements the AMP Agent SDR database which keeps a list
 * of all agent information that must be persisted across a reset.
 *
 * Each object in the database represents an sdr_list. Each list is populated
 * with a series of pointers to "descriptor" objects.  Each descriptor object
 * captures meta-data associated with the associated AMP object and a
 * pointer to that object. AMP objects are stored in the SDR in their
 * message-serialized form as that comprises the most space-efficient
 * representation of the object.
 *
 * On agent startup,AMP object types are deserialized and copied into
 * associated RAM data types.
 *
 * For example, the trls Object in the database is an sdr_list. Each
 * item in the trls list is an Object which points to a
 * trl_desc_t object.  This descriptor object captures information
 * such as the execution state of the TRL. One of the entries of the
 * trl_desc_t object is an Object pointer to the serialized
 * TRL in the SDR.  On system startup, the agent will grab the descriptor
 * object and use it to find the serialized TRL. The rule will be de-serialized
 * into a new trl_t structure.  The meta-data associated with the rule will be
 * populated by additional meta-data in the rule's descriptor object.
 *
 * All entities in the DB operate in this way, as follows.
 *
 * LIST          Descriptor Type          Type that we deserialize into
 * -------------+-----------------------+------------------------------
 * vars         | var_desc_t            | var_t
 * ctrls        | ctrl_desc_t           | ctrl_t
 * macros       | def_gen_desc_t        | def_gen_t
 * reports      | def_gen_desc_t        | rpt_t
 * trls         | trl_desc_t            | trl_t
 * srls         | srl_desc_t            | srl_t
 *
 */

typedef struct
{
   Object  vars;
   Object  ctrls;
   Object  macros;
   Object  reports;
   Object  trls;
   Object  srls;
   Object  descObj;  /**> The pointer to the AgentDB object in the SDR. */
} AgentDB;


/*
 * This structure captures the "volatile" database, which is the set of
 * information relating to configured items kept in the agent's memory.
 */

typedef struct
{
	Lyst  vars;
	Lyst  ctrls;
	Lyst  macros;
	Lyst  reports;
	Lyst  trls;
	Lyst  srls;

	ResourceLock var_mutex;
	ResourceLock ctrls_mutex;
	ResourceLock macros_mutex;
	ResourceLock reports_mutex;
	ResourceLock trls_mutex;
	ResourceLock srls_mutex;
} AgentVDB;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int  agent_db_var_persist(var_t* item);
int  agent_db_var_forget(mid_t *mid);
int  agent_db_ctrl_persist(ctrl_exec_t* item);
int  agent_db_ctrl_forget(mid_t *mid);
int  agent_db_defgen_persist(Object db, def_gen_t* item);
int  agent_db_forget(Object db, Object itemObj, Object descObj);
int  agent_db_init();
int  agent_db_macro_persist(def_gen_t* item);
int  agent_db_macro_forget(mid_t *mid);
int  agent_db_report_persist(def_gen_t* item);
int  agent_db_report_forget(mid_t *mid);
int  agent_db_srl_persist(srl_t* item);
int  agent_db_srl_forget(mid_t *mid);
int  agent_db_trl_persist(trl_t* item);
int  agent_db_trl_forget(mid_t *mid);


uint32_t agent_db_count(Lyst list, ResourceLock *mutex);

void agent_vdb_add(void *item, Lyst list, ResourceLock *mutex);

uint32_t   agent_vdb_var_init(Sdr sdr);
var_t*     agent_vdb_var_find(mid_t *mid);
void       agent_vdb_var_forget(mid_t *id);

void         agent_vdb_ctrls_init(Sdr sdr);
ctrl_exec_t* agent_vdb_ctrl_find(mid_t *mid);
void         agent_vdb_ctrl_forget(mid_t *mid);

uint32_t   agent_vdb_defgen_init(Sdr sdr, Object db, Lyst list, ResourceLock *mutex);
def_gen_t* agent_vdb_defgen_find(mid_t *mid, Lyst list, ResourceLock *mutex);
void       agent_vdb_defgen_forget(mid_t *id, Lyst list, ResourceLock *mutex);

void agent_vdb_destroy();
int  agent_vdb_init();

void       agent_vdb_macros_init(Sdr sdr);
def_gen_t* agent_vdb_macro_find(mid_t *mid);
void       agent_vdb_macro_forget(mid_t *id);

void       agent_vdb_reports_init(Sdr sdr);
def_gen_t* agent_vdb_report_find(mid_t *mid);
void       agent_vdb_report_forget(mid_t *id);

void       agent_vdb_srls_init(Sdr sdr);
srl_t*     agent_vdb_srl_find(mid_t *mid);
void       agent_vdb_srl_forget(mid_t *mid);

void       agent_vdb_trls_init(Sdr sdr);
trl_t*     agent_vdb_trl_find(mid_t *mid);
void       agent_vdb_trl_forget(mid_t *mid);


extern AgentVDB gAgentVDB;
extern AgentDB gAgentDB;

#endif // _AGENT_DB_H
