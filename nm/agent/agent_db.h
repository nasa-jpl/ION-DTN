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
 **  05/17/15  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef _NM_AGENT_DB_H
#define _NM_AGENT_DB_H

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"
#include "lyst.h"

#include "shared/utils/nm_types.h"
#include "shared/utils/ion_if.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/rules.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/def.h"
#include "shared/primitives/cd.h"

#include "shared/msg/pdu.h"
#include "shared/msg/msg_ctrl.h"
#include "shared/msg/msg_admin.h"

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


#define ADD_COMPDATA(x) agent_vdb_add(x, gAgentVDB.compdata, &(gAgentVDB.compdata_mutex));
#define ADD_CONST(x)    agent_vdb_add(x, gAgentVDB.consts,   &(gAgentVDB.consts_mutex));
#define ADD_CTRL(x)     agent_vdb_add(x, gAgentVDB.ctrls,    &(gAgentVDB.ctrls_mutex));
#define ADD_MACRO(x)    agent_vdb_add(x, gAgentVDB.macros,   &(gAgentVDB.macros_mutex));
//#define ADD_OP(x)       agent_vdb_add(x, gAgentVDB.ops,      &(gAgentVDB.ops_mutex));
#define ADD_REPORT(x)   agent_vdb_add(x, gAgentVDB.reports,  &(gAgentVDB.reports_mutex));
#define ADD_TRL(x)      agent_vdb_add(x, gAgentVDB.trls,     &(gAgentVDB.trls_mutex));
#define ADD_SRL(x)      agent_vdb_add(x, gAgentVDB.srls,     &(gAgentVDB.srls_mutex));

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
   Object  ctrls;      /* SDR list: ctrl_exec_descr_t      */
   Object  macros;     /* SDR list: def_gen_descr_t        */
   Object  reports;
   Object  trls;       /* SDR list: trl_descr_t */
   Object  srls;       /* SDR list: srl_descr_t */
   Object  descObj;    /**> The pointer to the AgentDB object in the SDR. */
} AgentDB;


/*
 * This structure captures the "volatile" database, which is the set of
 * information relating to configured items kept in the agent's memory.
 */

typedef struct
{
	Lyst  compdata;
	Lyst  ctrls;
	Lyst  macros;
	Lyst  reports;
	Lyst  trls;
	Lyst  srls;

	ResourceLock compdata_mutex;
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

int  agent_db_compdata_persist(cd_t* item);
int  agent_db_compdata_forget(mid_t *mid);
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

uint32_t   agent_vdb_compdata_init(Sdr sdr);
cd_t*      agent_vdb_compdata_find(mid_t *mid);
void       agent_vdb_compdata_forget(mid_t *id);

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
