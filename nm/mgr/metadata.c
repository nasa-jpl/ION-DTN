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
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/26/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "metadata.h"





int meta_add_parm(metadata_t *meta, char *name, amp_type_e type)
{
	parm_t *new_parm;

	CHKUSR(meta, AMP_FAIL);
	CHKUSR(name, AMP_FAIL);
	CHKUSR(type != AMP_TYPE_UNK, AMP_FAIL);

	new_parm = (parm_t *) STAKE(sizeof(parm_t));
	CHKUSR(new_parm, AMP_FAIL);

	strncpy(new_parm->name, name, AMP_MAX_EID_LEN);
	new_parm->type = type;

	if(vec_push(&(meta->parmspec),new_parm) != VEC_OK)
	{
		AMP_DEBUG_ERR("meta_add_parm", "Error assing parm %s", name);
		SRELEASE(new_parm);
		return AMP_FAIL;
	}

	return AMP_OK;
}

void meta_cb_del(rh_elt_t *elt)
{

	CHKVOID(elt);
	meta_release(elt->value, 1);
}

void meta_cb_filter(void *value, void *tag)
{
	meta_col_t *col = (meta_col_t*) tag;
	metadata_t *meta = (metadata_t*) value;

	if(((col->adm_id == ADM_ENUM_ALL) || (meta->adm_id == col->adm_id)) &&
	   ((col->type == AMP_TYPE_UNK) || (meta->id->type == col->type)))
	{
		vec_push(&(col->results), meta);
	}
}


// Shallow copy of ARI.
metadata_t *meta_create(ari_t *id, uint32_t adm_id, char *name, char *desc)
{
	metadata_t *result = NULL;
	int success;

	CHKNULL(id);

	result = STAKE(sizeof(metadata_t));
	CHKNULL(result);
	result->adm_id = adm_id;
	result->id = id;
	strncpy(result->name, name, META_NAME_MAX);
	strncpy(result->descr, desc, META_DESCR_MAX);

	result->parmspec = vec_create(0, vec_simple_del, NULL, NULL, VEC_FLAG_AS_STACK, &success);
	if(success != VEC_OK)
	{
		SRELEASE(result);
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

parm_t *meta_get_parm(metadata_t *meta, uint8_t idx)
{
	parm_t *result;
	CHKNULL(meta);
	return vec_at(meta->parmspec, idx);
}


void meta_release(metadata_t *meta, int destroy)
{
	CHKVOID(meta);
	ari_release(meta->id, 1);
	vec_release(&(meta->parmspec), 0);

	if(destroy)
	{
		SRELEASE(meta);
	}
}


int meta_store_mac(metadata_t *meta, macdef_t *macdef)
{
	adm_add_macdef(macdef);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return AMP_OK;
}

int meta_store_rpttpl(metadata_t *meta, rpttpl_t *rpttpl)
{
	adm_add_rpttpl(rpttpl);
	rhht_insert(&(gMgrDB.metadata), meta->id, meta, NULL);
	return AMP_OK;
}

int meta_store_obj(metadata_t *meta)
{
	CHKUSR(meta, AMP_FAIL);

	switch(meta->id->type)
	{
	case AMP_TYPE_CNST:
		adm_add_cnst(meta->id);
		break;
	case AMP_TYPE_CTRL:
		adm_add_ctrldef(meta->id, vec_num_entries(meta->parmspec), meta->adm_id, NULL);
		break;
	case AMP_TYPE_EDD:
		adm_add_edd(meta->id, NULL);
		break;
	case AMP_TYPE_LIT:
		adm_add_lit(meta->id);
		break;
	case AMP_TYPE_OPER:
		adm_add_op(meta->id,vec_num_entries(meta->parmspec), NULL);
		break;
	default:
		break;
	}
	return AMP_OK;

}

meta_col_t* metacol_create()
{
	meta_col_t *result = STAKE(sizeof(meta_col_t));
	int success;

	CHKNULL(result);
	result->results = vec_create(0, NULL, NULL, NULL, 0, &success);
	if(success != VEC_OK)
	{
		SRELEASE(result);
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
		SRELEASE(col);
	}
}







