/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file ctrl.c
 **
 ** Description: This module contains the functions, structures, and other
 **              information used to capture both control objects and
 **              macro objects.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  05/17/15  E. Birrane     Redesign around DTNMP v0.1 (Secure DTN - NASA: NNX14CS58P)
 **  09/01/18  E. Birrane     Updated to encoding and data structures (JHU/APL)
 *****************************************************************************/

#include "ctrl.h"

#include "../primitives/ari.h"


void ctrl_cb_del_fn(void *item)
{
	ctrl_t *ctrl = (ctrl_t *)item;

	CHKVOID(ctrl);

	ctrl_release(ctrl, 1);
}

int  ctrl_cb_comp_fn(void *i1, void *i2)
{
	ctrl_t *c1 = (ctrl_t *)i1;
	ctrl_t *c2 = (ctrl_t *)i2;
	ari_t *a1 = ctrl_get_id(c1);
	ari_t *a2 = ctrl_get_id(c2);

	int result = 0;

	CHKUSR(c1, -1);
	CHKUSR(c2, -1);

	if(c1->type != c2->type)
	{
		return -1;
	}

	if((result = ari_compare(a1, a2, 0)) == 0)
	{
		if(ARI_GET_FLAG_PARM(a1->as_reg.flags) ||
    	   ARI_GET_FLAG_PARM(a2->as_reg.flags))
		{
			result = tnvc_compare(c1->parms, c2->parms);
		}
	}

	return result;
}

void *ctrl_cb_copy_fn(void *item)
{
	ctrl_t *ctrl = (ctrl_t *) item;
	CHKNULL(ctrl);
	return ctrl_copy_ptr(ctrl);
}

rh_idx_t ctrl_cb_hash(void *table, void *key)
{
	unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
	unsigned int hash = 0;
	unsigned int i    = 0;
	rhht_t *ht = (rhht_t*) table;

	uint8_t *val = (uint8_t*) key;

	CHKUSR(val, ht->num_bkts);

	for (i = 0; i < sizeof(ctrl_t); ++i)
	{
		hash = (hash * seed) + val[i];
	}

	return hash % ht->num_bkts;
}


ctrl_t *ctrl_copy_ptr(ctrl_t *src)
{
	ctrl_t *result = NULL;

	if(src == NULL)
	{
		return NULL;
	}

	if((result = STAKE(sizeof(ctrl_t))) == NULL)
	{
		return NULL;
	}

	result->def = src->def;
	result->start = src->start;
	result->parms = tnvc_copy(src->parms);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: ctrl_create
 *
 * \par Constructs a control instance given a ID and information about the
 *      execution of the control.
 *
 * \retval NULL - Error in creating the control.
 *        !NULL - The control instance
 *
 * \param[in] id        The ARI defining this control instance.
 *
 * Notes:
 *   - The ARI is not stored in the control and can be discarded by the caller.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation. (JHU/APL)
 *  05/17/15  E. Birrane     Moved to ctrl.c, updated to DTNMP V0.1 (Secure DTN - NASA: NNX14CS58P)
 *  09/20/18  E. Birrane     Updated to latest AMP spec. Optimizations. (JHU/APL)
 *****************************************************************************/

ctrl_t *ctrl_create(ari_t *ari)
{
	ctrl_t *result = NULL;

	CHKNULL(ari);


	if((ari->type != AMP_TYPE_CTRL) && (ari->type != AMP_TYPE_MAC))
	{
		AMP_DEBUG_ERR("ctrl_create","Bad ARI type %d", ari->type);
		return NULL;
	}

	if((result = STAKE(sizeof(ctrl_t))) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_create","Can't allocate CTRL.", NULL);
		return NULL;
	}

	result->type = ari->type;

	/* Step 1: Grab the basic information for this control. */
	if(ari->type == AMP_TYPE_CTRL)
	{
		if((result->def.as_ctrl = VDB_FINDKEY_CTRLDEF(ari)) == NULL)
		{
			AMP_DEBUG_ERR("ctrl_create","Can't find base ctrldef.", NULL);
			SRELEASE(result);
			return NULL;
		}
	}
	else
	{
		if((result->def.as_mac = VDB_FINDKEY_MACDEF(ari)) == NULL)
		{
			AMP_DEBUG_ERR("ctrl_create","Can't find base macdef.", NULL);
			SRELEASE(result);
			return NULL;
		}
	}

    /* Store any parameters needed for this control. */
    if(ARI_GET_FLAG_PARM(ari->as_reg.flags))
    {
    	result->parms = tnvc_copy(&(ari->as_reg.parms));
    }
    else
    {
    	result->parms = NULL;
    }

  	AMP_DEBUG_EXIT("ctrl_create","->"ADDR_FIELDSPEC".",(uaddr)result);
	return result;
}


ctrl_t *ctrl_db_deserialize(blob_t *data)
{
	QCBORDecodeContext it;
	ctrl_t *result;
	uvast start;
	eid_t caller;
	int success;

	CHKNULL(data);
	memset(&caller, 0, sizeof(caller));
	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	/* Step 1: Deserialize the main control. */
	if((result = ctrl_deserialize_ptr(&it, &success)) == NULL)
	{
		return NULL;
	}

	if (cut_get_cbor_numeric(&it, AMP_TYPE_TV, &start) != AMP_OK)
	{
		ctrl_release(result, 1);
		return NULL;
	}

	ctrl_set_exec(result, start, caller);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return result;
}


ari_t *ctrl_get_id(ctrl_t *ctrl)
{
	CHKNULL(ctrl);
	return (ctrl->type == AMP_TYPE_CTRL) ? ctrl->def.as_ctrl->ari : ctrl->def.as_mac->ari;
}

/*
 * 16 bytes for bytestring encoding.
 * 8 bytes for uvast time
 * AMP_MAX_EID_LEN for caller
 * 1 for null terminator.
 */
blob_t *ctrl_db_serialize(ctrl_t *ctrl)
{
	blob_t *result = NULL;
	QCBOREncodeContext encoder;
	QCBORError err;
	uint8_t data[25 + AMP_MAX_EID_LEN];
	size_t length = 25 + AMP_MAX_EID_LEN;
	UsefulBufC Encoded;

	result = ctrl_serialize_wrapper(ctrl);
	CHKNULL(result);

	QCBOREncode_Init(&encoder, (UsefulBuf){data,length});
	QCBOREncode_AddUInt64(&encoder, ctrl->start);
	QCBOREncode_AddSZString(&encoder, ctrl->caller.name);

	err = QCBOREncode_Finish(&encoder, &Encoded);
	if (err != QCBOR_SUCCESS)
	{
	   AMP_DEBUG_ERR("ctrl_db_serialize","CBOR Encoding Err %d", err);
	   blob_release(result,1);
	   return NULL;
	}

	if(blob_append(result, data, Encoded.len) != AMP_OK)
	{
		AMP_DEBUG_ERR("ctrl_db_serialize","Can't append data of length %ld", Encoded.len);
		blob_release(result, 1);
		return NULL;
	}

	return result;
}



void*    ctrl_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	ctrl_t *result = NULL;

#if AMP_VERSION < 7
	blob_t *blob = blob_deserialize_ptr(it, success);
	ari_t *ari = ari_deserialize_raw(blob, success);
	blob_release(blob, 1);
#else
	QCBORDecode_StartOctets(it);
	ari_t *ari = ari_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif
	
	if((ari == NULL) || (*success != AMP_OK))
	{
		return NULL;
	}

	result = ctrl_create(ari);
	ari_release(ari, 1);
	return result;
}

ctrl_t* ctrl_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(data);
	
	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	ctrl_t *tmp = ctrl_deserialize_ptr(&it, success);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}

/*
 * Do not release the def. That is a shallow pointer.
 */
void ctrl_release(ctrl_t *ctrl, int destroy)
{
	CHKVOID(ctrl);

	tnvc_release(ctrl->parms, 1);

	if(destroy)
	{
		SRELEASE(ctrl);
	}
}

int ctrl_serialize(QCBOREncodeContext *encoder, void *item)
{
	ctrl_t *ctrl = (ctrl_t *) item;
	ari_t *ctrl_id = NULL;
	tnvc_t parms;
	int err = AMP_FAIL;

	if((encoder == NULL) || (ctrl == NULL))
	{
		AMP_DEBUG_ERR("ctrl_serialize","Bad Parms", NULL);
		return AMP_FAIL;
	}

	if((ctrl_id = ctrl_get_id(ctrl)) == NULL)
	{
		return AMP_FAIL;
	}

	/* If the control has parms, swap them in */
	if(ctrl->parms != NULL)
	{
		parms = ctrl_id->as_reg.parms;
		ctrl_id->as_reg.parms = *(ctrl->parms);
	}

#if AMP_VERSION < 7
	blob_t *blob = ari_serialize_wrapper(ctrl_id);
	if(blob != NULL)
	{
		err = blob_serialize(encoder, blob);
		blob_release(blob, 1);
	}
	else
	{
		AMP_DEBUG_ERR("ctrl_serialize","Error serializing control.", NULL);
	}
#else
	QCBOREncode_OpenArray(encoder);
	err = ari_serialize(encoder, ctrl_id);
	QCBOREncode_CloseArrayOctet(encoder);
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("ctrl_serialize","Error serializing control.", NULL);
	}
#endif
	if(ctrl->parms != NULL)
	{
		ctrl_id->as_reg.parms = parms;
	}

	return err;
}

blob_t*   ctrl_serialize_wrapper(ctrl_t *ctrl)
{
	blob_t *result = NULL;
	ari_t *ctrl_id = NULL;
	ari_t *tmp_ari = NULL;

	if(ctrl == NULL)
	{
		return NULL;
	}

	if((ctrl_id = ctrl_get_id(ctrl)) == NULL)
	{
		return NULL;
	}

	if(ctrl->parms != NULL)
	{

		tmp_ari = ari_copy_ptr(ctrl_id);
		CHKNULL(tmp_ari);

		if(ari_add_parm_set(tmp_ari, ctrl->parms) == AMP_OK)
		{
			result = ari_serialize_wrapper(tmp_ari);
		}

		ari_release(tmp_ari, 1);
	}
	else
	{
		result = ari_serialize_wrapper(ctrl_id);
	}

	return result;

}



/******************************************************************************
 *
 * \par Function Name: ctrl_set_exec
 *
 * \par Sets meta-data associated with the control related to how it should be
 *      run on the agent.
 *
 * \retval Non.
 *
 * \param[out] ctrl    The control being updated.
 * \param[in]  time      The timestamp to be associated with this control.
 * \param[in]  caller    Control requester.
 *
 * Notes:
 *   - The start time is always converted to an absolute time. This is because
 *     the control may be persisted to the SDR, and re-read upon startup at
 *     which point a relative time may lead to incorrect execution.
 *   - A timestamp of 0 is an exception to the above, and is always treated
 *     to mean execute now.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/03/18  E. Birrane     Initial implementation. (JHU/APL)
 *****************************************************************************/
void ctrl_set_exec(ctrl_t *ctrl, time_t start, eid_t caller)
{
	CHKVOID(ctrl);

	if(start < AMP_RELATIVE_TIME_EPOCH && start != 0)
	{
		ctrl->start = getCtime() + start;
	}
	else
	{
		ctrl->start = start;
	}

	ctrl->caller = caller;
}


// Shallow copy.
ctrldef_t *ctrldef_create(ari_t *ari, uint8_t num, ctrldef_run_fn run)
{
	ctrldef_t *result = NULL;

	CHKNULL(ari);

	if((result = STAKE(sizeof(ctrldef_t))) == NULL)
	{
		return NULL;
	}

	result->ari = ari;
	result->num_parms = num;
	result->run =run;

	return result;
}

void ctrldef_del_fn(rh_elt_t *elt)
{
	CHKVOID(elt);

	ctrldef_t *def = (ctrldef_t*)elt->value;
	ctrldef_release(def, 1);
}

void    ctrldef_release(ctrldef_t *def, int destroy)
{
	CHKVOID(def);

	ari_release(def->ari, 1);
	if(destroy)
	{
		SRELEASE(def);
	}
}


/* shallow copies ctrl */

int macdef_append(macdef_t *mac, ctrl_t *ctrl)
{
	if((mac == NULL) || (ctrl == NULL))
	{
		return AMP_FAIL;
	}

	return vec_push(&(mac->ctrls), ctrl);
}

int macdef_append_ac(macdef_t *mac, ac_t *ac)
{
	int i = 0;
	int num = 0;
	int success = AMP_FAIL;

	if((mac == NULL) || (ac == NULL))
	{
		return success;
	}

	num = ac_get_count(ac);

	for(i = 0; i < num; i++)
	{
		ctrl_t *cur_ctrl = ctrl_create(ac_get(ac, i));
		macdef_append(mac, cur_ctrl);
	}

	return success;
}

void    macdef_cb_del_fn(void *item)
{
	macdef_release((macdef_t*)item, 1);
}

int     macdef_cb_comp_fn(void *i1, void *i2)
{
	macdef_t *m1 = (macdef_t *)i1;
	macdef_t *m2 = (macdef_t *)i2;
	CHKUSR(m1, -1);
	CHKUSR(m2, -1);
	return ari_cb_comp_fn(m1->ari, m2->ari);
}

void    macdef_cb_ht_del_fn(rh_elt_t *elt)
{
	CHKVOID(elt);
	macdef_cb_del_fn(elt->value);
}

void    macdef_clear(macdef_t *mac)
{
	CHKVOID(mac);
	vec_clear(&(mac->ctrls));
}

macdef_t macdef_copy(macdef_t *src, int *success)
{
	macdef_t result;

	result.ari = ari_copy_ptr(src->ari);
	result.ctrls = vec_copy(&(src->ctrls), success);
	if(*success != VEC_OK)
	{
		ari_release(result.ari, 1);
		*success = AMP_FAIL;
	}
	else
	{
		*success = AMP_OK;
	}

	return result;
}

macdef_t *macdef_copy_ptr(macdef_t *src)
{
	macdef_t *result = (macdef_t*) STAKE(sizeof(macdef_t));

	if(result != NULL)
	{
		int success = AMP_FAIL;
		*result = macdef_copy(src, &success);
		if(success != AMP_OK)
		{
			macdef_release(result, 1);
			result = NULL;
		}
	}

	return result;
}

// Shallow copy.
macdef_t*  macdef_create(size_t num, ari_t *ari)
{
	macdef_t *result = NULL;
	int success;

	if((result = STAKE(sizeof(macdef_t))) == NULL)
	{
		return NULL;
	}
	result->ari = ari;
	result->ctrls = vec_create(num, ctrl_cb_del_fn, ctrl_cb_comp_fn, ctrl_cb_copy_fn, VEC_FLAG_AS_STACK, &success);

	if(success != AMP_OK)
	{
		macdef_release(result, 1);
		result = NULL;
	}

	return result;
}

int macdef_init(macdef_t *mac, size_t num, ari_t *ari)
{
	int success;

	if((mac == NULL) || (ari == NULL))
	{
		return AMP_FAIL;
	}
	memset(mac, 0, sizeof(macdef_t));
	mac->ari = ari;
	mac->ctrls = vec_create(num, ctrl_cb_del_fn, ctrl_cb_comp_fn, ctrl_cb_copy_fn, VEC_FLAG_AS_STACK, &success);
	return success;
}


macdef_t  macdef_deserialize(QCBORDecodeContext *it, int *success)
{
	macdef_t result;
	ari_t *new_ari = NULL;
	int err;

	memset(&result,0,sizeof(result));
	*success = AMP_FAIL;

#if AMP_VERSION < 7
	blob_t *tmp = blob_deserialize_ptr(it, success);
	new_ari = ari_deserialize_raw(tmp, success);
	blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(it);
	new_ari = ari_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif
	
	if((new_ari == NULL) || (*success != AMP_OK))
	{
		return result;
	}

	result.ari = new_ari;
	result.ctrls = vec_create(0, ctrl_cb_del_fn, ctrl_cb_comp_fn, ctrl_cb_copy_fn, VEC_FLAG_AS_STACK, success);

	err = cut_deserialize_vector(&(result.ctrls), it, ctrl_deserialize_ptr);
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("macdef_deserialize", "Error deserializing ctrls: %d", err);
		macdef_release(&result, 0);
	}
	else
	{
		*success = AMP_OK;
	}

	return result;
}

macdef_t*  macdef_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	macdef_t *result = (macdef_t *) STAKE(sizeof(macdef_t));
	CHKNULL(result);
	*result = macdef_deserialize(it, success);
	if(*success != AMP_OK)
	{
		SRELEASE(result);
		result = NULL;
	}
	return result;

}

macdef_t macdef_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;
	macdef_t result;

	*success = AMP_FAIL;
	memset(&result, 0, sizeof(macdef_t));
	CHKUSR(data, result);
    
	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	result = macdef_deserialize(&it, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return result;
}

ctrl_t* macdef_get(macdef_t* mac, uint8_t index)
{
	ctrl_t *result = NULL;
	CHKNULL(mac);

	result = (ctrl_t*) vec_at(&mac->ctrls, index);
	return result;
}

uint8_t macdef_get_count(macdef_t* mac)
{
	CHKZERO(mac);
	return vec_num_entries(mac->ctrls);
}

void    macdef_release(macdef_t *mac, int destroy)
{
	CHKVOID(mac);

	ari_release(mac->ari, 1);
	vec_release(&(mac->ctrls), 0);

	if(destroy)
	{
		SRELEASE(mac);
	}
}


int macdef_serialize(QCBOREncodeContext *encoder, void *item)
{
	macdef_t *mac = (macdef_t*) item;
	int err;

	if((encoder == NULL) || (mac == NULL))
	{
		return AMP_FAIL;
	}

	/* Step 1: Encode the ARI. */
#if AMP_VERSION < 7
	blob_t *result = ari_serialize_wrapper(mac->ari);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = ari_serialize(encoder, mac->ari);
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("macdef_serialize","CBOR Error: %d", err);
		return err;
	}

	/* Step 2: Encode the contents. */
	err = cut_serialize_vector(encoder, &(mac->ctrls), (cut_enc_fn)ctrl_serialize);
	if (err != AMP_OK)
	{
		AMP_DEBUG_ERR("macdef_serialize", "CBOR Error: %d", err);
		return err;
	}
	
	return AMP_OK;
}


blob_t*  macdef_serialize_wrapper(macdef_t *mac)
{
	CHKNULL(mac);

	return cut_serialize_wrapper((vec_num_entries(mac->ctrls) + 1) * ARI_DEFAULT_ENC_SIZE, mac, (cut_enc_fn)macdef_serialize);
}
