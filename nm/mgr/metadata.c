/*****************************************************************************
 **
 ** \file nm_mgr_names.c
 **
 **
 ** Description: Helper file holding optional hard-coded human-readable
 **              names and descriptions for supported ADM entries.
 **
 **
 ** Notes:
 ** - Metadata can be quite large and long-lived because it includes strings and, in
 **  particular, strings capturing descriptions of data and controls. Since
 **  the nm_mgr is always meant to run on a non-embedded platform, this
 **  meta-data is currently allocated/freed using malloc and free and not
 **  through the ION PSM system. This reduces artificially constraining
 **  ION memory for users who keep using default values.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/06/18  E. Birrane     Inital Implementation  (JHU/APL)
 *****************************************************************************/

#include "metadata.h"




/*
 * Will release id on failure.
 */
metadata_t *meta_add_edd(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(base, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t *meta_add_cnst(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(base, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t* meta_add_op(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(base, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t* meta_add_var(amp_type_e base, ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(base, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t* meta_add_ctrl(ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(AMP_TYPE_UNK, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t* meta_add_macro(ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(AMP_TYPE_UNK, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t* meta_add_rpttpl(ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(AMP_TYPE_UNK, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

metadata_t* meta_add_tblt(ari_t *id, uint8_t adm_id, char *name, char *descr)
{
	metadata_t* meta = meta_create(AMP_TYPE_UNK, id, adm_id, name, descr);
	CHKNULL(meta);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return meta;
}

int meta_add_parm(metadata_t *meta, char *name, amp_type_e type)
{
	meta_fp_t *new_parm;

	if((meta == NULL) || (name == NULL) || (type == AMP_TYPE_UNK))
	{
		return AMP_FAIL;
	}

	new_parm = (meta_fp_t *) malloc(sizeof(meta_fp_t));
	CHKUSR(new_parm, AMP_FAIL);

	strncpy(new_parm->name, name, META_PARM_NAME-1);
	new_parm->type = type;

	if(vec_push(&(meta->parmspec),new_parm) != VEC_OK)
	{
		AMP_DEBUG_ERR("meta_add_parm", "Error assing parm %s", name);
		free(new_parm);
		return AMP_FAIL;
	}

	return AMP_OK;
}

void meta_cb_del(rh_elt_t *elt)
{
	CHKVOID(elt);
	meta_release(elt->value, 1);
}

void meta_cb_filter(rh_elt_t *elt, void *tag)
{
	meta_col_t *col = (meta_col_t*) tag;
	metadata_t *meta;

	if((elt == NULL) || (elt->value == NULL) || (tag == NULL))
	{
		return;
	}

	meta = (metadata_t*) elt->value;

	if(((col->adm_id == ADM_ENUM_ALL) || (meta->adm_id == col->adm_id)) &&
	   ((col->type == AMP_TYPE_UNK) || (meta->id->type == col->type)))
	{
		vec_push(&(col->results), meta);
	}
}


// Shallow copy of ARI.
metadata_t *meta_create(amp_type_e type, ari_t *id, uint32_t adm_id, char *name, char *desc)
{
	metadata_t *result = NULL;
	int success;

	CHKNULL(id);

	result = malloc(sizeof(metadata_t));
	CHKNULL(result);
	result->adm_id = adm_id;
	result->id = id;
	result->type = type;
	strncpy(result->name, name, META_NAME_MAX-1);
	strncpy(result->descr, desc, META_DESCR_MAX-1);

	result->parmspec = vec_create(0, vec_simple_del, NULL, NULL, VEC_FLAG_AS_STACK, &success);
	if(success != VEC_OK)
	{
		free(result);
		result = NULL;
	}

	return result;
}




meta_col_t* meta_filter(uint32_t adm_id, amp_type_e type)
{
	meta_col_t *result = metacol_create();

	CHKNULL(result);
	result->adm_id = adm_id;
	result->type = type;

	rhht_foreach(&(gMgrDB.metadata), meta_cb_filter, result);

	return result;
}

meta_fp_t *meta_get_parm(metadata_t *meta, uint8_t idx)
{
	CHKNULL(meta);
	return vec_at(&(meta->parmspec), idx);
}


void meta_release(metadata_t *meta, int destroy)
{
	CHKVOID(meta);

	vec_release(&(meta->parmspec), 0);

	if(destroy)
	{
		free(meta);
	}
}


meta_col_t* metacol_create()
{
	meta_col_t *result = malloc(sizeof(meta_col_t));
	int success;

	CHKNULL(result);
	result->results = vec_create(0, NULL, NULL, NULL, 0, &success);
	if(success != VEC_OK)
	{
		free(result);
		result = NULL;
	}
	return result;
}

void metacol_release(meta_col_t*col, int destroy)
{
	CHKVOID(col);
	vec_release(&(col->results), 0);
	if(destroy)
	{
		free(col);
	}
}
