/*****************************************************************************
 **
 ** File Name: mgr_db.h
 **
 ** Description: This module captures the functions, structures, and operations
 **              necessary to store and retrieve user-defined content from the
 **              management daemon on system startup.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/18/15  E. Birrane     Initial Implementation from agent_db.[c|h] (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef _NM_MGR_DB_H
#define _NM_MGR_DB_H

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"
#include "lyst.h"

#include "../shared/utils/nm_types.h"
#include "../shared/utils/ion_if.h"

#include "../shared/primitives/mid.h"
#include "../shared/primitives/rules.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/def.h"

#include "../shared/msg/pdu.h"
#include "../shared/msg/msg_ctrl.h"
#include "../shared/msg/msg_admin.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif

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


#define ADD_VAR(x) mgr_vdb_add(x, gMgrVDB.compdata, &(gMgrVDB.compdata_mutex));
#define ADD_CTRL(x)     mgr_vdb_add(x, gMgrVDB.ctrls,    &(gMgrVDB.ctrls_mutex));
#define ADD_MACRO(x)    mgr_vdb_add(x, gMgrVDB.macros,   &(gMgrVDB.macros_mutex));
#define ADD_REPORT(x)   mgr_vdb_add(x, gMgrVDB.reports,  &(gMgrVDB.reports_mutex));
#define ADD_TRL(x)      mgr_vdb_add(x, gMgrVDB.trls,     &(gMgrVDB.trls_mutex));
#define ADD_SRL(x)      mgr_vdb_add(x, gMgrVDB.srls,     &(gMgrVDB.srls_mutex));

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



/*
 * This structure implements the DTNMP Mgmt Daemon SDR database which keeps a list
 * of all information that must be persisted across a reset.
 *
 * Each object in the database represents an sdr_list. Each list is populated
 * with a series of pointers to "descriptor" objects.  Each descriptor object
 * captures meta-data associated with the associated DTNMP object and a
 * pointer to that object. DTNMP objects are stored in the SDR in their
 * message-serialized form as that comprises the most space-efficient
 * representation of the object.
 *
 * On startup, DTNMP object types are deserialized and copied into
 * associated RAM data types.
 *
 * For example, the active_rules Object in the database is an sdr_list. Each
 * item in the active_rules list is an Object which points to a
 * rule_time_prod_desc_t object.  This descriptor object captures information
 * such as the execution state of the active rule. One of the entries of the
 * rule_time_prod_desc_t object is an Object pointer to the serialized
 * rule in the SDR.  On system startup, the mgr will grab the descriptor
 * object and use it to find the serialized rule. The rule will be de-serialized
 * into a new rule structure.  The meta-data associated with the rule will be
 * populated by additional meta-data in the rule's descriptor object.
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
   Object  sqldb;      /* SDR list: ui_db_t */
   Object  descObj;    /**> The pointer to the MgrDB object in the SDR. */
} MgrDB;


/*
 * This structure captures the "volatile" database, which is the set of
 * information relating to configured items kept in the mgr's memory.
 */

typedef struct
{
	Lyst  compdata;
	Lyst  ctrls;
	Lyst  macros;
	Lyst  reports;
	Lyst  trls;
	Lyst  srls;
#ifdef HAVE_MYSQL
	ui_db_t sqldb;
#endif

	ResourceLock compdata_mutex;
	ResourceLock ctrls_mutex;
	ResourceLock macros_mutex;
	ResourceLock reports_mutex;
	ResourceLock trls_mutex;
	ResourceLock srls_mutex;
	ResourceLock sqldb_mutex;
} MgrVDB;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int  mgr_db_compdata_forget(mid_t *mid);
int  mgr_db_compdata_persist(var_t* item);
int  mgr_db_ctrl_forget(mid_t *mid);
int  mgr_db_ctrl_persist(ctrl_exec_t* item);
int  mgr_db_defgen_persist(Object db, def_gen_t* item);
int  mgr_db_forget(Object db, Object itemObj, Object descObj);
int  mgr_db_init();
int  mgr_db_macro_forget(mid_t *mid);
int  mgr_db_macro_persist(def_gen_t* item);
int  mgr_db_report_forget(mid_t *mid);
int  mgr_db_report_persist(def_gen_t* item);
int  mgr_db_srl_forget(mid_t *mid);
int  mgr_db_srl_persist(srl_t* item);
int  mgr_db_trl_forget(mid_t *mid);
int  mgr_db_trl_persist(trl_t* item);
#ifdef HAVE_MYSQL
int  mgr_db_sql_forget(ui_db_t* item);
int  mgr_db_sql_persist(ui_db_t* item);
#endif



void mgr_vdb_add(void *item, Lyst list, ResourceLock *mutex);

void       mgr_vdb_compdata_init(Sdr sdr);
var_t*      mgr_vdb_compdata_find(mid_t *mid);
void       mgr_vdb_compdata_forget(mid_t *id);

void         mgr_vdb_ctrls_init(Sdr sdr);
ctrl_exec_t* mgr_vdb_ctrl_find(mid_t *mid);
void         mgr_vdb_ctrl_forget(mid_t *mid);

uint32_t   mgr_vdb_defgen_init(Sdr sdr, Object db, Lyst list, ResourceLock *mutex);
def_gen_t* mgr_vdb_defgen_find(mid_t *mid, Lyst list, ResourceLock *mutex);
void       mgr_vdb_defgen_forget(mid_t *id, Lyst list, ResourceLock *mutex);

void mgr_vdb_destroy();
int  mgr_vdb_init();

void       mgr_vdb_macros_init(Sdr sdr);
def_gen_t* mgr_vdb_macro_find(mid_t *mid);
void       mgr_vdb_macro_forget(mid_t *id);

void       mgr_vdb_reports_init(Sdr sdr);
def_gen_t* mgr_vdb_report_find(mid_t *mid);
void       mgr_vdb_report_forget(mid_t *id);

void       mgr_vdb_srls_init(Sdr sdr);
srl_t*     mgr_vdb_srl_find(mid_t *mid);
void       mgr_vdb_srl_forget(mid_t *mid);

void       mgr_vdb_trls_init(Sdr sdr);
trl_t*     mgr_vdb_trl_find(mid_t *mid);
void       mgr_vdb_trl_forget(mid_t *mid);

#ifdef HAVE_MYSQL
void       mgr_vdb_sql_init(Sdr sdr);
ui_db_t*   mgr_vdb_sql_find();
void       mgr_vdb_sql_forget();
#endif

extern MgrVDB gMgrVDB;
extern MgrDB gMgrDB;

#endif // _MGR_DB_H
