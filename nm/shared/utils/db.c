/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: db.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to interact with
 **              SDRs to persistently store information.
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
 **  09/02/18  E. Birrane     Cleanup and update to latest spec. (JHU/APL)
 *****************************************************************************/

#include "db.h"
#include "rhht.h"
#include "utils.h"

#include "../primitives/ari.h"
#include "../primitives/ctrl.h"
#include "../primitives/report.h"
#include "../primitives/rules.h"
#include "../primitives/edd_var.h"
#include "../primitives/table.h"

#include "../adm/adm.h"


vdb_store_t gVDB;
db_store_t  gDB;



int  db_forget(db_desc_t *desc, Object list)
{
	Sdr sdr = getIonsdr();
	Object elt;

	CHKERR(desc);
	CHKERR(list != 0);

	CHKERR(sdr_begin_xn(sdr));

	/* Free the item wherever it is in the SDR. */
	if(desc->itemObj != 0)
	{
	  sdr_free(sdr, desc->itemObj);
	  desc->itemObj = 0;
	  desc->itemSize = 0;
	}

	/* Remove the object descriptor from the list. */
	if(desc->descObj != 0)
	{
	   elt = sdr_list_first(sdr, list);
	   while(elt)
	   {
		   if(sdr_list_data(sdr, elt) == desc->descObj)
		   {
			   sdr_list_delete(sdr, elt, NULL, NULL);
			   sdr_free(sdr, desc->descObj);
			   desc->descObj = 0;
			   elt = 0;
		   }
		   else
		   {
			   elt = sdr_list_next(sdr, elt);
		   }
	   }
	}

	sdr_end_xn(sdr);

	return AMP_OK;
}


int  db_read_objs(char *name)
{
	Sdr sdr = getIonsdr();

	CHKUSR(name, AMP_FAIL);

	// * Initialize the non-volatile database. * /
	memset((char*) &gDB, 0, sizeof(gDB));

	/* Recover the Agent database, creating it if necessary. */
	CHKERR(sdr_begin_xn(sdr));

	gDB.descObj = sdr_find(sdr, name, NULL);
	switch(gDB.descObj)
	{
		case -1:  // SDR error. * /
			sdr_cancel_xn(sdr);
			AMP_DEBUG_ERR("db_read_objs", "Can't search for DB in SDR.", NULL);
			return -1;

		case 0: // Not found; Must create new DB. * /

			if((gDB.descObj = sdr_malloc(sdr, sizeof(gDB))) == 0)
			{
				sdr_cancel_xn(sdr);
				AMP_DEBUG_ERR("db_read_objs", "No space for database.", NULL);
				return -1;
			}
			AMP_DEBUG_ALWAYS("db_read_objs", "Creating DB: %s", name);

			gDB.ctrls = sdr_list_create(sdr);
			gDB.macdefs = sdr_list_create(sdr);
			gDB.rpttpls = sdr_list_create(sdr);
			gDB.rules = sdr_list_create(sdr);
			gDB.vars = sdr_list_create(sdr);

			sdr_write(sdr, gDB.descObj, (char *) &gDB, sizeof(gDB));
			sdr_catlg(sdr, name, 0, gDB.descObj);

			break;

		default:  /* Found DB in the SDR */
			/* Read in the Database. */
			sdr_read(sdr, (char *) &gDB, gDB.descObj, sizeof(gDB));
			AMP_DEBUG_ALWAYS("db_read_objs", "Found DB", NULL);
	}

	if(sdr_end_xn(sdr))
	{
		AMP_DEBUG_ERR("db_read_objs", "Can't create Agent database.", NULL);
		return -1;
	}

	return 1;
}

/*
 * This function writes an item and its associated descriptor into the SDR,
 * allocating space for each, and adding the SDR descriptor pointer to a
 * given SDR list.
 *
 * blob    : The serialized item to store in the SDR.
 * desc    : The db descriptor holding where this should live
 * list    : The SDR list holding the item descriptor (at *descrObj).
 */
int  db_persist(blob_t *blob, db_desc_t *desc, Object list)
{
	Sdr sdr = getIonsdr();

	CHKUSR(blob, AMP_FAIL);
	CHKUSR(desc, AMP_FAIL);

	/* Step 0: Sanity Checks. */
	if (((desc->itemObj == 0) && (desc->itemObj != 0)) ||
	   ((desc->itemObj != 0) && (desc->itemObj == 0)))
	{
		AMP_DEBUG_ERR("db_persist","bad params.",NULL);
		return AMP_FAIL;
	}


	/*
	 * If the object is already in the SDR, remove it. We will
	 * be writing over it, and we could have changed size since the
	 * last time we persisted the object.
	 */
	if(desc->itemObj != 0)
	{
		db_forget(desc, list);
	}

	desc->itemSize = blob->length;

	CHKUSR(sdr_begin_xn(sdr), AMP_FAIL);

   /* Step 1: Allocate a descriptor object for this item in the SDR. */
   if((desc->descObj = sdr_malloc(sdr, sizeof(db_desc_t))) == 0)
   {
	   sdr_cancel_xn(sdr);
	   AMP_DEBUG_ERR("db_persist",
			   	     "Can't allocate descriptor of size %d.",
					 sizeof(db_desc_t));
	   return AMP_SYSERR;
   }

   /* Step 2: Allocate space for the serialized rule in the SDR. */
   if((desc->itemObj = sdr_malloc(sdr, desc->itemSize)) == 0)
   {
	   sdr_free(sdr, desc->descObj);
	   sdr_cancel_xn(sdr);
	   desc->descObj = 0;
	   AMP_DEBUG_ERR("db_persist",
			         "Unable to allocate Item in SDR. Size %d.",
					 desc->itemSize);
	   return AMP_SYSERR;
   }

   /* Step 3: Write the item to the SDR. */
   sdr_write(sdr, desc->itemObj, (char *) blob->value, desc->itemSize);

   /* Step 4: Write the item descriptor to the SDR. */
   sdr_write(sdr, desc->descObj, (char *) desc, sizeof(db_desc_t));

   /* Step 5: Save the descriptor in the AgentDB active rules list. */
   if (sdr_list_insert_last(sdr, list, desc->descObj) == 0)
   {
	   db_forget(desc, list);

	   sdr_cancel_xn(sdr);
	   AMP_DEBUG_ERR("db_persist",
			         "Unable to insert item Descr. in SDR.", NULL);
	   return AMP_SYSERR;
   }

   if(sdr_end_xn(sdr))
   {
	   AMP_DEBUG_ERR("db_persist", "Can't persist db item.", NULL);
	   return AMP_SYSERR;
   }

   return AMP_OK;
}


int  db_persist_ctrl(void* item)
{
	int result;
	ctrl_t *ctrl = (ctrl_t *) item;
	blob_t *blob = ctrl_db_serialize(ctrl);

	CHKERR(blob);
	result = db_persist(blob, &(ctrl->desc), gDB.ctrls);
	blob_release(blob, 1);
	return result;
}


int  db_persist_macdef(void* item)
{
	int result;
	macdef_t *def = (macdef_t*) item;
	blob_t *blob = macdef_serialize_wrapper(def);

	CHKERR(blob);
	result = db_persist(blob, &(def->desc), gDB.macdefs);
	blob_release(blob, 1);
	return result;
}

int  db_persist_rpttpl(void *item)
{
	int result;
	rpttpl_t* rpttpl = (rpttpl_t*) item;
	blob_t *blob = rpttpl_serialize_wrapper(rpttpl);

	CHKERR(blob);
	result = db_persist(blob, &(rpttpl->desc), gDB.rpttpls);
	blob_release(blob, 1);
	return result;
}

int  db_persist_rule(void* item)
{
	int result;
	rule_t *rule = (rule_t *) item;
	blob_t *blob = rule_db_serialize_wrapper(rule);

	CHKERR(blob);
	result = db_persist(blob, &(rule->desc), gDB.rules);
	blob_release(blob, 1);
	return result;
}


int  db_persist_var(void* item)
{
	int result;
	var_t *var = (var_t *) item;
	blob_t *blob = var_serialize_wrapper(var);

	if(blob == NULL)
	{
		return AMP_FAIL;
	}
	result = db_persist(blob, &(var->desc), gDB.vars);
	blob_release(blob, 1);
	return result;
}



/*
 * Initialize VDB list from a list in the SDR.
 */

int vdb_obj_init(Object sdr_list, vdb_init_cb_fn init_cb)
{
	Object elt;
	Object descObj;
	db_desc_t cur_desc;
	blob_t *data;
	uint32_t num = 0;
	Sdr sdr = getIonsdr();

	CHKUSR(sdr_begin_xn(sdr), AMP_FAIL);

	/* Step 1: Walk through report definitions. */
	for (elt = sdr_list_first(sdr, sdr_list); elt;
	    		elt = sdr_list_next(sdr, elt))
	{

		/* Step 1.1: Grab the descriptor. */
	    descObj = sdr_list_data(sdr, elt);
	    oK(sdr_read(sdr, (char *) &cur_desc, descObj, sizeof(cur_desc)));

	    cur_desc.descObj = descObj;

	    /* Step 1.2: Allocate space for the def. */
	    if((data = blob_create(NULL, 0, cur_desc.itemSize)) == NULL)
	    {
	    	AMP_DEBUG_ERR("vdb_init","Can't allocate %d bytes.",
	    					cur_desc.itemSize);
	    }
	    else
	    {
	    	/* Step 1.3: Grab the serialized rule */
	    	oK(sdr_read(sdr, (char *) data->value, cur_desc.itemObj, cur_desc.itemSize));
	    	data->length = cur_desc.itemSize;

	    	if(init_cb(data, cur_desc) != AMP_OK)
	    	{
		    	AMP_DEBUG_ERR("vdb_init","Unable to insert new data item. Removing item.", NULL);
		    	//db_forget(&cur_desc, sdr_list);
	    	}
	    	else
	    	{
	    		num++;
	    	}

	    	blob_release(data, 1);
	    }
	}
	sdr_end_xn(sdr);

	return num;
}


int vdb_db_init_ctrl(blob_t *data, db_desc_t desc)
{
	ctrl_t *ctrl = NULL;

	CHKUSR(data, AMP_FAIL);

	if((ctrl = ctrl_db_deserialize(data)) == NULL)
	{
		AMP_DEBUG_ERR("vdb_db_init_cb_ctrl","Can't deserialize raw control.", NULL);
		return AMP_FAIL;
	}

	ctrl->desc = desc;

	if(VDB_ADD_CTRL(ctrl, NULL) != RH_OK)
	{
		AMP_DEBUG_ERR("vdb_db_init_cb_ctrl","Can't add new control.", NULL);
		ctrl_release(ctrl, 1);
		return AMP_FAIL;
	}

	return AMP_OK;
}

int vdb_db_init_macdef(blob_t *data, db_desc_t desc)
{
	macdef_t *mac = STAKE(sizeof(macdef_t));
	int success;
	CHKUSR(data, AMP_FAIL);
	CHKUSR(mac, AMP_FAIL);

	*mac = macdef_deserialize_raw(data, &success);
	if(success != AMP_OK)
	{
		AMP_DEBUG_ERR("vdb_db_init_macro","Can't deserialize raw macro.", NULL);
		SRELEASE(mac);
		return success;
	}

	mac->desc = desc;

	if(VDB_ADD_MACDEF(mac->ari, mac) != RH_OK)
	{
		AMP_DEBUG_ERR("vdb_db_init_macro","Can't add new macro.", NULL);
		macdef_release(mac, 1);
		return AMP_FAIL;
	}

	return AMP_OK;
}

int vdb_db_init_rpttpl(blob_t *data, db_desc_t desc)
{
	rpttpl_t *rptt = NULL;
	int success;

	CHKUSR(data, AMP_FAIL);

	if((rptt = rpttpl_deserialize_raw(data, &success)) == NULL)
	{
		AMP_DEBUG_ERR("vdb_db_init_rpttpl","Can't deserialize raw report.", NULL);
		return AMP_FAIL;
	}

	rptt->desc = desc;

	if(VDB_ADD_RPTT(rptt->id, rptt) != RH_OK)
	{
		AMP_DEBUG_ERR("vdb_db_init_rpttpl","Can't add new report.", NULL);
		rpttpl_release(rptt, 1);
		return AMP_FAIL;
	}

	return AMP_OK;
}

int vdb_db_init_rule(blob_t *data, db_desc_t desc)
{
	rule_t *rule = NULL;
	int success;

	CHKUSR(data, AMP_FAIL);

	if((rule = rule_db_deserialize_raw(data, &success)) == NULL)
	{
		AMP_DEBUG_ERR("vdb_db_init_rule","Can't deserialize raw rule.", NULL);
		return AMP_FAIL;
	}

	rule->desc = desc;

	if(VDB_ADD_RULE(&(rule->id), rule) != RH_OK)
	{
		AMP_DEBUG_ERR("vdb_db_init_rule","Can't add new rule.", NULL);
		rule_release(rule, 1);
		return AMP_FAIL;
	}

	return AMP_OK;
}



int vdb_db_init_var(blob_t *data, db_desc_t desc)
{
	var_t *var = NULL;
	int success;

	CHKUSR(data, AMP_FAIL);

	if((var = var_deserialize_raw(data, &success)) == NULL)
	{
		AMP_DEBUG_ERR("vdb_db_init_var","Can't deserialize raw var.", NULL);
		return AMP_FAIL;
	}

	var->desc = desc;

	if(VDB_ADD_VAR(var->id, var) != RH_OK)
	{
		AMP_DEBUG_ERR("vdb_db_init_var","Can't add new var.", NULL);
		var_release(var, 1);
		return AMP_FAIL;
	}

	return AMP_OK;
}


void db_destroy()
{
	rhht_release(&(gVDB.adm_atomics), 0);
	rhht_release(&(gVDB.adm_edds), 0);
	rhht_release(&(gVDB.adm_ctrl_defs), 0);
	rhht_release(&(gVDB.adm_ops), 0);
	rhht_release(&(gVDB.adm_tblts), 0);
	vec_release(&(gVDB.ctrls), 0);
	rhht_release(&(gVDB.macdefs), 0);
	rhht_release(&(gVDB.rpttpls), 0);
	rhht_release(&(gVDB.rules), 0);
	rhht_release(&(gVDB.vars), 0);

	vec_release(&(gVDB.issuers), 0);
	vec_release(&(gVDB.nicknames), 0);
	vec_release(&(gVDB.tags), 0);
}


int db_init(char *name, void (*adm_init_cb)()) 
{
	int success = AMP_FAIL;
	int num;
	memset(&gVDB, 0, sizeof(gVDB));

	gVDB.adm_atomics = rhht_create(DB_MAX_ATOMIC, ari_cb_comp_no_parm_fn, ari_cb_hash, edd_cb_ht_del, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.adm_edds = rhht_create(DB_MAX_ATOMIC, ari_cb_comp_no_parm_fn, ari_cb_hash, edd_cb_ht_del, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.ctrls =  vec_create(DB_MAX_CTRL, ctrl_cb_del_fn, ctrl_cb_comp_fn, ctrl_cb_copy_fn, 0, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.adm_ctrl_defs = rhht_create(DB_MAX_CTRLDEF, ari_cb_comp_no_parm_fn, ari_cb_hash, ctrldef_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.macdefs = rhht_create(DB_MAX_MACDEF, ari_cb_comp_fn, ari_cb_hash, macdef_cb_ht_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.adm_ops = rhht_create(DB_MAX_OP, ari_cb_comp_no_parm_fn, ari_cb_hash, op_cb_ht_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.rpttpls = rhht_create(DB_MAX_RPTT, ari_cb_comp_fn, ari_cb_hash, rpttpl_cb_ht_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.rules = rhht_create(DB_MAX_SBR, ari_cb_comp_fn, ari_cb_hash, rule_cb_ht_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.adm_tblts = rhht_create(DB_MAX_TBLT, ari_cb_comp_no_parm_fn, ari_cb_hash, tblt_cb_ht_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.vars = rhht_create(DB_MAX_VAR, ari_cb_comp_fn, ari_cb_hash, var_cb_ht_del_fn, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.nicknames = vec_create(DB_MAX_NN, vec_simple_del, vec_uvast_comp, vec_uvast_copy, 0, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.issuers = vec_create(DB_MAX_NN,
#if AMP_VERSION < 7
		vec_simple_del, vec_uvast_comp, vec_uvast_copy,
#else
		vec_blob_del, vec_blob_comp, vec_blob_copy,
#endif
		0, &success);
	CHKUSR(success == AMP_OK, success);

	gVDB.tags = vec_create(DB_MAX_NN, vec_blob_del, vec_blob_comp, vec_blob_copy, 0, &success);
	CHKUSR(success == AMP_OK, success);

	adm_common_init();
	if (adm_init_cb == NULL)
	{
		AMP_DEBUG_ERR("vdb_init", "Error: adm_init_cb not registered.", NULL);
	}
	else
	{
		adm_init_cb();
	}


	success = db_read_objs(name);

	num = vdb_obj_init(gDB.ctrls, vdb_db_init_ctrl);
	AMP_DEBUG_ALWAYS("vdb_init", "Added %d Controls from DB.", num);

	num = vdb_obj_init(gDB.macdefs,  vdb_db_init_macdef);
	AMP_DEBUG_ALWAYS("vdb_init", "Added %d Macro Definitions from DB.", num);

	num = vdb_obj_init(gDB.rpttpls, vdb_db_init_rpttpl);
	AMP_DEBUG_ALWAYS("vdb_init", "Added %d Report Template Definitions from DB.", num);

	num = vdb_obj_init(gDB.rules,   vdb_db_init_rule);
	AMP_DEBUG_ALWAYS("vdb_init", "Added %d Rule Definitions from DB.", num);

	num = vdb_obj_init(gDB.vars,    vdb_db_init_var);
	AMP_DEBUG_ALWAYS("vdb_init", "Added %d Variable Definitions from DB.", num);

	return success;
}
