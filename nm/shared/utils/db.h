/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: db.h
 **
 ** Description: This file contains functions to access AMM objects stored
 **              either persistently in the ION SDR or in a RAM data store.
 **              Additionally, this file defines the data stores that
 **              hold these data.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/29/13  E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  09/25/18  E. Birrane     Update to hold all DB abd vDB structures and
 **                           migrate to AMP v05. (JHU/APL)
 *****************************************************************************/
#ifndef DB_H_
#define DB_H_

#include "platform.h"
#include "sdr.h"

#include "nm_types.h"
#include "rhht.h"
#include "vector.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */
#define DB_MAX_ATOMIC 300
#define DB_MAX_CTRL 50
#define DB_MAX_CTRLDEF 150
#define DB_MAX_MACDEF  50
#define DB_MAX_OP   100
#define DB_MAX_RPTT 50
#define DB_MAX_SBR  50
#define DB_MAX_TBLT 50
#define DB_MAX_TBR  50
#define DB_MAX_VAR  50
#define DB_MAX_NN   100
#define DB_MAX_ISS  20
#define DB_MAX_TAG  25


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

#define VDB_ADD_EDD(key, value)     rhht_insert(&(gVDB.adm_edds),  key, value, NULL)
#define VDB_ADD_CONST(key, value)   rhht_insert(&(gVDB.adm_atomics),  key, value, NULL)
#define VDB_ADD_LIT(key, value)     rhht_insert(&(gVDB.adm_atomics),  key, value, NULL)
#define VDB_ADD_CTRL(value, idx)    vec_insert(&(gVDB.ctrls),         value, idx)
#define VDB_ADD_CTRLDEF(key, value) rhht_insert(&(gVDB.adm_ctrl_defs),key, value, NULL)
#define VDB_ADD_MACDEF(key, value)  rhht_insert(&(gVDB.macdefs),      key, value, NULL)
#define VDB_ADD_OP(key, value)      rhht_insert(&(gVDB.adm_ops),      key, value, NULL)
#define VDB_ADD_RPTT(key, value)    rhht_insert(&(gVDB.rpttpls),      key, value, NULL)
#define VDB_ADD_RULE(key, value)    rhht_insert(&(gVDB.rules),        key, value, NULL)
#define VDB_ADD_TBLT(key, value)    rhht_insert(&(gVDB.adm_tblts),    key, value, NULL)
#define VDB_ADD_VAR(key, value)     rhht_insert(&(gVDB.vars),         key, value, NULL)
#define VDB_ADD_NN(value, idx)      vec_uvast_add(&(gVDB.nicknames),  value, idx)
#if AMP_VERSION < 7
#define VDB_ADD_ISS(value, idx)     vec_uvast_add(&(gVDB.issuers),    value, idx)
#else
#define VDB_ADD_ISS(value, idx)     vec_blob_add(&(gVDB.issuers),    value, idx)
#endif
#define VDB_ADD_TAG(value, idx)     vec_blob_add(&(gVDB.tags),        value, idx)

#define VDB_FINDKEY_EDD(key)     rhht_retrieve_key(&(gVDB.adm_edds),  key)
#define VDB_FINDKEY_CONST(key)   rhht_retrieve_key(&(gVDB.adm_atomics),  key)
#define VDB_FINDKEY_LIT(key)     rhht_retrieve_key(&(gVDB.adm_atomics),  key)
#define VDB_FINDKEY_CTRLDEF(key) rhht_retrieve_key(&(gVDB.adm_ctrl_defs),key)
#define VDB_FINDKEY_MACDEF(key)  rhht_retrieve_key(&(gVDB.macdefs),      key)
#define VDB_FINDKEY_OP(key)      rhht_retrieve_key(&(gVDB.adm_ops),      key)
#define VDB_FINDKEY_RPTT(key)    rhht_retrieve_key(&(gVDB.rpttpls),      key)
#define VDB_FINDKEY_RULE(key)    rhht_retrieve_key(&(gVDB.rules),        key)
#define VDB_FINDKEY_TBLT(key)    rhht_retrieve_key(&(gVDB.adm_tblts),    key)
#define VDB_FINDKEY_VAR(key)     rhht_retrieve_key(&(gVDB.vars),         key)

#define VDB_FINDIDX_EDD(idx)     rhht_retrieve_idx(&(gVDB.adm_edds),   idx)
#define VDB_FINDIDX_CONST(idx)   rhht_retrieve_idx(&(gVDB.adm_atomics),   idx)
#define VDB_FINDIDX_LIT(idx)     rhht_retrieve_idx(&(gVDB.adm_atomics),   idx)
#define VDB_FINDIDX_CTRL(idx, s) vec_at(&(gVDB.ctrls),     idx, s)
#define VDB_FINDIDX_CTRLDEF(idx) rhht_retrieve_idx(&(gVDB.adm_ctrl_defs), idx)
#define VDB_FINDIDX_MACDEF(idx)  rhht_retrieve_idx(&(gVDB.macdefs),       idx)
#define VDB_FINDIDX_OP(idx)      rhht_retrieve_idx(&(gVDB.adm_ops),       idx)
#define VDB_FINDIDX_RPTT(idx)    rhht_retrieve_idx(&(gVDB.rpttpls),       idx)
#define VDB_FINDIDX_RULE(idx)    rhht_retrieve_idx(&(gVDB.rules),         idx)
#define VDB_FINDIDX_TBLT(idx)    rhht_retrieve_idx(&(gVDB.adm_tblts),     idx)
#define VDB_FINDIDX_VAR(idx)     rhht_retrieve_idx(&(gVDB.vars),          idx)
#define VDB_FINDIDX_NN(idx)    vec_at(&gVDB.nicknames, idx)
#define VDB_FINDIDX_ISS(idx)   vec_at(&gVDB.issuers,   idx)
#define VDB_FINDIDX_TAG(idx)   vec_at(&gVDB.tags,      idx)

#define VDB_DELKEY_EDD(key)     rhht_del_key(&(gVDB.adm_edds),  key)
#define VDB_DELKEY_CONST(key)   rhht_del_key(&(gVDB.adm_atomics),  key)
#define VDB_DELKEY_LIT(key)     rhht_del_key(&(gVDB.adm_atomics),  key)
#define VDB_DELKEY_CTRLDEF(key) rhht_del_key(&(gVDB.adm_ctrl_defs),key)
#define VDB_DELKEY_MACDEF(key)  rhht_del_key(&(gVDB.macdefs),       key)
#define VDB_DELKEY_OP(key)      rhht_del_key(&(gVDB.adm_ops),      key)
#define VDB_DELKEY_RPTT(key)    rhht_del_key(&(gVDB.rpttpls),      key)
#define VDB_DELKEY_RULE(key)    rhht_del_key(&(gVDB.rules),        key)
#define VDB_DELKEY_TBLT(key)    rhht_del_key(&(gVDB.adm_tblts),    key)
#define VDB_DELKEY_VAR(key)     rhht_del_key(&(gVDB.vars),         key)

#define VDB_DELIDX_EDD(idx)     rhht_del_idx(&(gVDB.adm_edds),   idx)
#define VDB_DELIDX_CONST(idx)   rhht_del_idx(&(gVDB.adm_atomics),   idx)
#define VDB_DELIDX_LIT(idx)     rhht_del_idx(&(gVDB.adm_atomics),   idx)
#define VDB_DELIDX_CTRL(idx)    vec_del_idx(&(gVDB.ctrls),          idx)
#define VDB_DELIDX_CTRLDEF(idx) rhht_del_idx(&(gVDB.adm_ctrl_defs), idx)
#define VDB_DELIDX_MACDEF(idx)  rhht_del_idx(&(gVDB.macdefs),       idx)
#define VDB_DELIDX_OP(idx)      rhht_del_idx(&(gVDB.adm_ops),       idx)
#define VDB_DELIDX_RPTT(idx)    rhht_del_idx(&(gVDB.rpttpls),       idx)
#define VDB_DELIDX_RULE(idx)    rhht_del_idx(&(gVDB.rules),         idx)
#define VDB_DELIDX_TBLT(idx)    rhht_del_idx(&(gVDB.adm_tblts),     idx)
#define VDB_DELIDX_VAR(idx)     rhht_del_idx(&(gVDB.vars),          idx)
/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * TODO: This description needs to be updated.
 *
 * This structure implements an SDR database which keeps a list
 * of all AMM information that must be persisted across a reset.
 *
 * Each object in the database represents an sdr_list. Each list is populated
 * with a series of pointers to "descriptor" objects.  Each descriptor object
 * captures meta-data associated with the associated AMP object and a
 * pointer to that object. AMM Objects are stored in the SDR in their
 * message-serialized form as that comprises the most space-efficient
 * representation of the object.
 *
 * On startup, AMM object types are deserialized and copied into
 * associated VDB data stores.
 *
 * For example, the tbrs Object in the database is an sdr_list. Each
 * item in the tbrs list is an Object which points to a
 * db_desc_t object.  This descriptor object captures information
 * such as the execution state of the TBR. One of the entries of the
 * db_desc_t object is an Object pointer to the serialized
 * TBR in the SDR.  On system startup, the application will grab the descriptor
 * object and use it to find the serialized TBR. The rule will be de-serialized
 * into a new tbr_t structure.  The meta-data associated with the rule will be
 * populated by additional meta-data in the rule's descriptor object.
 *
 * All entities in the DB operate in this way.
 */

typedef struct
{
   Object  ctrls;
   Object  macdefs;
   Object  rpttpls;
   Object  rules;
   Object  vars;
   Object  descObj;  /**> The pointer to the store object in the SDR. */
} db_store_t;


/*
 * This structure captures the "volatile" database, which is the set of
 * information relating to configured items kept in the agent's memory.
 */

typedef struct
{
	rhht_t adm_atomics;   /**> Set by ADM support only. */
	rhht_t adm_edds;      /**> Set by ADM support only. */
	vector_t ctrls;
	rhht_t adm_ctrl_defs; /**> Set by ADM support only. */
	rhht_t macdefs;
	rhht_t adm_ops;       /**> Set by ADM support only. */
	rhht_t rpttpls;
	rhht_t rules;
	rhht_t adm_tblts;     /**> Set by ADM support only. */
	rhht_t vars;

	vector_t nicknames;
	vector_t issuers;
	vector_t tags;
} vdb_store_t;


/**
 * This is a generic structure used to capture AMM objects stored in
 * the ION SDR.
 */
typedef struct
{
	Object itemObj;     /**> Serialized object in an SDR. */
	uint32_t itemSize;  /**> Size of object in itemObj.   */

    /* Below is not kept in the SDR. */
	Object descObj;     /**> This descriptor in SDR.      */
} db_desc_t;



typedef blob_t* (*db_serialize_fn)(void *item);
typedef int     (*vdb_init_cb_fn)(blob_t *data, db_desc_t desc);

extern vdb_store_t gVDB;
extern db_store_t  gDB;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */



int  db_forget(db_desc_t *desc, Object list);


int  db_persist(blob_t *blob, db_desc_t *desc, Object list);

int  db_persist_ctrl(void* item);
int  db_persist_macdef(void* item);
int  db_persist_rpttpl(void* item);
int  db_persist_rule(void* item);
int  db_persist_var(void* item);

int  db_read_objs(char *name);

void db_destroy();

int db_init(char *name, void (*adm_init_cb)());


int vdb_obj_init(Object sdr_list, vdb_init_cb_fn init_cb);

int vdb_db_init_ctrl(blob_t *data, db_desc_t desc);

int vdb_db_init_macdef(blob_t *data, db_desc_t desc);
int vdb_db_init_rpttpl(blob_t *data, db_desc_t desc);
int vdb_db_init_rule(blob_t *data, db_desc_t desc);
int vdb_db_init_var(blob_t *data, db_desc_t desc);


#endif /* DB_H_ */
