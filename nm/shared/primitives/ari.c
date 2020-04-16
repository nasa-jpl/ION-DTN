/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
  ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: ari.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of AMM Resource Identifiers (ARIs). Every object in
 **              the AMM can be uniquely identified using an ARI.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/18/18  E. Birrane     Initial implementation ported from previous
 **                           implementation of MIDs for earlier AMP spec (JHU/APL)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"
#include "../utils/nm_types.h"
#include "../utils/db.h"

#include "ari.h"
#include "tnv.h"

/*
 * +--------------------------------------------------------------------------+
 * |					   Private Functions  								  +
 * +--------------------------------------------------------------------------+
 */

/******************************************************************************
 * Private helper function to de-serialize a literal ARI.
 *
 * \returns The deserialized ARI.
 *
 * \param[in|out]  it       The first value of the serialized ARI.
 * \param[out]     success  Whether the deserialization succeeded.
 *
 * \note
 *   1. Assumes that parameter checking is performed by the caller.
 *****************************************************************************/

static ari_t p_ari_deserialize_lit(QCBORDecodeContext *it, uint8_t byte, int *success)
{
	ari_t result;

	ari_init(&result);

	result.type = byte & 0xF;
	result.as_lit.type = ((byte >> 4) & 0xF) + AMP_TYPE_BOOL;

	*success = tnv_deserialize_val_by_type(it, &(result.as_lit));

	if(*success != AMP_OK)
	{
		AMP_DEBUG_ERR("p_ari_deserialize_lit","Can't get ARI literal value.", NULL);
		return ari_null();
	}

	return result;
}



/******************************************************************************
 * Private helper function to de-serialize a regular ARI.
 *
 * \returns The deserialized ARI.
 *
 * \param[in|out]  it       The first value of the serialized ARI.
 * \param[out]     success  Whether the deserialization succeeded.
 *
 * \note
 *   1. Assumes that parameter checking is performed by the caller.
 *****************************************************************************/

static ari_t p_ari_deserialize_reg(QCBORDecodeContext *it, uint8_t flags, int *success)
{
	ari_t result;
	uvast temp;

	ari_init(&result);

	result.type = ARI_GET_FLAG_TYPE(flags);
	result.as_reg.flags = flags;

	/* Get the nickname, if one exists. */
	if( ARI_GET_FLAG_NN(flags) )
	{
		/* Get the UVAST nickname. */
		*success = cut_get_cbor_numeric(it, AMP_TYPE_UVAST, &temp);
		VDB_ADD_NN(temp, &(result.as_reg.nn_idx));
	}

	/* Get the name. */
	if(*success == AMP_OK)
	{
		result.as_reg.name = blob_deserialize(it, success);
	}

	/* Get the Parameters, if given */
	if((*success == AMP_OK) && ARI_GET_FLAG_PARM(flags))
	{
		tnvc_t tmp;
#if AMP_VERSION < 7
		blob_t blob = blob_deserialize(it, success);
		tmp = tnvc_deserialize_raw(&blob, success);
		blob_release(&blob, 0);
#else
		QCBORDecode_StartOctets(it);
		tmp = tnvc_deserialize(it, success);
		QCBORDecode_EndOctets(it);
#endif
		if(*success != AMP_OK)
		{
			ari_release(&result, 0);
			return result;
		}
		else
		{
			result.as_reg.parms = tmp;
		}
	}

	/* Get the Issuer, if defined */
	if((*success == AMP_OK) && ARI_GET_FLAG_ISS(flags))
	{
#if AMP_VERSION < 8 // V8 defines issuer as a blob for added flexibility
		cut_get_cbor_numeric(it, AMP_TYPE_UVAST, &temp);

		VDB_ADD_ISS(temp, &(result.as_reg.iss_idx));
#else
		blob_t issuer = blob_deserialize(it, success);
		if (*success == AMP_OK)
		{
			*success = VDB_ADD_ISS(issuer, &(result.as_reg.iss_idx));
			blob_release(&issuer, 0);
		}
#endif
	}

	/* Get the Tag, if defined */
	if((*success == AMP_OK) && ARI_GET_FLAG_TAG(flags))
	{
		blob_t tag = blob_deserialize(it, success);

		if(*success == AMP_OK)
		{
			*success = VDB_ADD_TAG(tag, &(result.as_reg.tag_idx));
			blob_release(&tag, 0); // TODO avoid this needeless re-alloc.
		}
	}

	if(*success != AMP_OK)
	{
		ari_release(&result, 0);
		return result;
	}

	return result;
}


/******************************************************************************
 * Private helper function to serialize a literal ARI.
 *
 * \returns The serialization status code.
 *
 * \param[in|out]  encoder  The CBOR encoder.
 * \param[in]      ari      The object being serialized.
 *
 * \note
 *   1. Encoders track the extra memory needed if encoding continues past an
 *      out-of-memory error. So, do not stop encoding in that instance. Out
 *      of memory errors will be caught at a higher layer.
 *****************************************************************************/

static int p_ari_serialize_lit(QCBOREncodeContext *encoder, ari_t *ari)
{
	uint8_t byte;

	if((encoder == NULL) || (ari == NULL))
	{
		return AMP_FAIL;
	}

	/* Serialize the AMM Object type in accordance with spec. */
	byte = ((ari->as_lit.type & 0xF) << 4) | (AMP_TYPE_LIT % 0xF);

	if ( cut_enc_byte(encoder, byte) != AMP_OK)
	{
		return AMP_FAIL;
	}

	return tnv_serialize_value(encoder, &(ari->as_lit));
}


/******************************************************************************
 * Private helper function to serialize a regular ARI.
 *
 * \returns The serialization status code.
 *
 * \param[in|out]  encoder  The CBOR encoder.
 * \param[in]      ari      The object being serialized.
 *
 * \note
 *   1. Encoders track the extra memory needed if encoding continues past an
 *      out-of-memory error. So, do not stop encoding in that instance. Out
 *      of memory errors will be caught at a higher layer.
 *****************************************************************************/

static int p_ari_serialize_reg(QCBOREncodeContext *encoder, ari_t *ari)
{
	blob_t *result;

	if((encoder == NULL) || (ari == NULL))
	{
		return AMP_FAIL;
	}

	// Encode Flags as BYTE
	if ( cut_enc_byte(encoder, ari->as_reg.flags) != AMP_OK)
	{
		AMP_DEBUG_ERR("p_ari_serialize_reg","CBOR Error", NULL);
		return AMP_FAIL;
	}

	// Encode Nickname (if defined)
	if(ARI_GET_FLAG_NN(ari->as_reg.flags))
	{
		uvast *nn = (uvast *) VDB_FINDIDX_NN(ari->as_reg.nn_idx);

		if (nn != NULL)
		{
		   QCBOREncode_AddUInt64(encoder, *nn);
		}
	}

	// Encode Name as BYTESTR
	if ( blob_serialize(encoder, &(ari->as_reg.name)) != AMP_OK )
	{
		AMP_DEBUG_ERR("p_ari_serialize_reg","CBOR Error", NULL);
		return AMP_FAIL;
	}

	// Encode Parameters
	if(ARI_GET_FLAG_PARM(ari->as_reg.flags))
	{
#if AMP_VERSION < 7
		blob_t *result = tnvc_serialize_wrapper(&(ari->as_reg.parms));
		blob_serialize(encoder, result);
		blob_release(result, 1);
#else
		QCBOREncode_OpenArray(encoder);
		tnvc_serialize(encoder, &(ari->as_reg.parms) );
		QCBOREncode_CloseArrayOctet(encoder);
#endif
	}

	// Encode the issuer, if defined
	if(ARI_GET_FLAG_ISS(ari->as_reg.flags))
	{
#if AMP_VERSION < 8
		uvast *iss = (uvast *)VDB_FINDIDX_ISS(ari->as_reg.iss_idx);

		if (iss != NULL)
		{
		   QCBOREncode_AddUInt64(encoder, *iss);
		}
#else
		result = (blob_t *)VDB_FINDIDX_ISS(ari->as_reg.iss_idx);
		blob_serialize(encoder, result);

#endif
	}

	// Encode the Tag, if defined
	if(ARI_GET_FLAG_TAG(ari->as_reg.flags))
	{
		result = (blob_t *)VDB_FINDIDX_TAG(ari->as_reg.tag_idx);
		blob_serialize(encoder, result);
	}

	return AMP_OK;
}



/*
 * +--------------------------------------------------------------------------+
 * |					   Public Functions  								  +
 * +--------------------------------------------------------------------------+
 */

/******************************************************************************
 * Appends a set of parameters to an ARI.
 *
 * \returns AMP status code.
 *
 * \param[in|out]  ari    The SRI receiving the parameters.
 * \param[in]      parms  The new parameters.
 *
 * \note
 *****************************************************************************/

int ari_add_parm_set(ari_t *ari, tnvc_t *parms)
{
	if((ari == NULL) ||
	   (ari->type == AMP_TYPE_LIT) ||
	   (ARI_GET_FLAG_PARM(ari->as_reg.flags) == 0))
	{
		return AMP_FAIL;
	}

	return tnvc_append(&(ari->as_reg.parms), parms);
}



/******************************************************************************
 *
 * Appends a single parameter to the end of an ARI parm set.
 *
 * \returns AMP status code
 *
 * \param[in, out] ari   The ARI receiving the new parameter.
 * \param[in]      parm  The new parameter.
 *
 * \notes
 *  1. The new parameter is shallow copied and MUST NOT be
 *     released by the calling function.
 *****************************************************************************/

int ari_add_parm_val(ari_t *ari, tnv_t *parm)
{
	if((ari == NULL) ||
	   (ari->type == AMP_TYPE_LIT) ||
	   (ARI_GET_FLAG_PARM(ari->as_reg.flags) == 0))
	{
		return AMP_FAIL;
	}

	return tnvc_insert(&(ari->as_reg.parms), parm);
}


int ari_cb_comp_no_parm_fn(void *i1, void *i2)
{
 	return ari_compare((ari_t*)i1, (ari_t*)i2, 0);
}

int ari_cb_comp_fn(void *i1, void *i2)
{
	return ari_compare((ari_t*)i1, (ari_t*)i2, 1);
}

void* ari_cb_copy_fn(void *item)
{
	return ari_copy_ptr((ari_t*)item);
}

void ari_cb_del_fn(void *item)
{
	ari_release((ari_t*)item, 1);
}


/******************************************************************************
 *
 * Hashes an ARI to an index in a rhht_t hash table.
 *
 * \returns The hash index, or # buckets on error.
 *
 * \param[in] table  The hash table that would receive the ARI.
 * \param[in] key    The ARI identifying the object to be inserted.
 *
 * \notes
 *  1. This is based on a sample BKDR hash function provided by
 *     http://www.partow.net/programming/hashfunctions/
 *  2. We assume this is only called from a hash table and, therefore, ht
 *     cannot be NULL.
 *****************************************************************************/

rh_idx_t  ari_cb_hash(void *table, void *key)
{
	unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
    unsigned int hash = 0;
	unsigned int i    = 0;
	rhht_t *ht = (rhht_t*) table;
	ari_t *id = (ari_t*) key;

	if(id == NULL)
	{
		AMP_DEBUG_ERR("ari_cb_hash","Bad parms.", NULL);
		return ht->num_bkts;
	}

	/* Add the type */
	hash = (hash * seed) + id->type;

	/* Based on type hash ther body. */
	if(id->type == AMP_TYPE_LIT)
	{
		hash = (hash * seed) + id->as_lit.flags;
		hash = (hash * seed) + id->as_lit.value.as_uvast;
	}
	else
	{
		hash = (hash * seed) + id->as_reg.flags;
			hash = (hash * seed) + id->as_reg.iss_idx;
			hash = (hash * seed) + id->as_reg.nn_idx;
			hash = (hash * seed) + id->as_reg.tag_idx;
		for(i = 0; i < id->as_reg.name.length; i++)
		{
			hash = (hash * seed) + id->as_reg.name.value[i];
		}
	}

   return hash % ht->num_bkts;
}



/******************************************************************************
 *
 * Removes an entry in a hash table of ARIs.
 *
 * \param[in, out] elt  The hash table element being released.
 *
 * \notes
 *  1. Since this is a hash table of ARIs, it is likely that they key and the
 *     value are the same item, so be careful to not release the same ARI
 *     two times in this function.
 *****************************************************************************/

void ari_cb_ht_del(rh_elt_t *elt)
{
	if(elt == NULL)
	{
		return;
	}

	/*
	 * Only release the key if it is different from the value. This is a
	 * strange case that occurs if we have a hash tbale of ARIs where each
	 * ARI is identified by itself.
	 */
	if(elt->key != elt->value)
	{
		ari_release((ari_t*)elt->key, 1);
	}

	ari_release((ari_t*)elt->value, 1);
	elt->value = NULL;
	elt->key = NULL;
}



/******************************************************************************
 *
 * Determines if 2 ARIs represent the same object. This comparison can be
 * performed considering, or ignoring, parameters.
 *
 * \returns -1   : Error or ari1 < ari2
 * 			 0   : ari1 == ari2
 * 			 >0  : ari1 > ari2
 *
 *
 *
 * \param[in] ari1      First ari being compared.
 * \param[in] ari2      Second ari being compared.
 * \param[in] use_parms Whether to compare paramaters as well.
 *
 * \par Notes:
 *		1. This function should only check for equivalence (== 0), not order
 *         since we do not differentiate between ari1 < ari2 and error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field.
 *  09/18/18  E. Birrane     Update to ARI.
 *****************************************************************************/

int ari_compare(ari_t *ari1, ari_t *ari2, int parms)
{
    AMP_DEBUG_ENTRY("ari_compare","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
    		         (uaddr) ari1, (uaddr) ari2);

    if((ari1 == NULL) || (ari2 == NULL))
    {
    	return -1;
    }

    if(ari1->type != ari2->type)
    {
    	return 1;
    }
    else if(ari1->type == AMP_TYPE_LIT)
    {
    	return tnv_compare(&(ari1->as_lit), &(ari2->as_lit));
    }
    else
    {
    	if( (ari1->as_reg.flags    != ari2->as_reg.flags) ||
    		(ari1->as_reg.iss_idx  != ari2->as_reg.iss_idx) ||
			(ari1->as_reg.nn_idx   != ari2->as_reg.nn_idx) ||
			(ari1->as_reg.tag_idx  != ari2->as_reg.tag_idx))
    	{
    		return 1;
    	}
    	else if(blob_compare(&(ari1->as_reg.name), &(ari2->as_reg.name)))
    	{
    		return 1;
    	}

    	/* IFF both ARIs have actual parms, do a parm compare.
    	 *
    	 * We only get here if the flags for both ARIs are equal, so
    	 * if both ARIs have the parm flag set, but only 1 (or 0) ARIs
    	 * have actual parms, then we are comparing an ARI prototype
    	 * versus an actual ARI, or comparing 2 ARI prototypes. In those
    	 * cases, we do not (and cannot) compare parms, but that does
    	 * not mean there isn't a match.
    	 */
    	if(parms)
    	{
    		if((tnvc_get_count(&(ari1->as_reg.parms)) > 0) &&
    				(tnvc_get_count(&(ari2->as_reg.parms)) > 0))
    		{
    			return tnvc_compare(&(ari1->as_reg.parms), &(ari2->as_reg.parms));
    		}
    	}
    	else
    	{
    		return 0;
    	}
    }

    return 0;
}



/******************************************************************************
 *
 * \par Function Name: ari_copy
 *
 * \par Duplicates an ARI object.
 *
 * \retval The copied ARI.
 *
 * \param[in]  src     The ARI being copied.
 * \param[out] success Whether the copy succeeded.
 *
 * \par Notes:
 *     1. ARI parameters are deep copied.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation (JHU/APL)
 *  09/29/18  E. Birrane     Updated to match copy pattern. (JHU/APL)
 *****************************************************************************/

ari_t ari_copy(ari_t val, int *success)
{
    ari_t result;

    /* Shallow copy bulk items. */
    result = val;

    /* Deep copy as needed. */
    if(result.type == AMP_TYPE_LIT)
    {
    	result.as_lit = tnv_copy(val.as_lit, success);
    	if(*success != AMP_OK)
    	{
    		tnv_release(&(result.as_lit), 0);
    	}
    }
    else
    {
    	if((*success = blob_copy(val.as_reg.name, &(result.as_reg.name))) != AMP_OK)
    	{
    		return result;
    	}

    	if((*success = tnvc_init(&(result.as_reg.parms), tnvc_get_count(&(val.as_reg.parms)))) != AMP_OK)
    	{
    		blob_release(&(result.as_reg.name), 0);
    		return result;
    	}

    	if((*success = tnvc_append(&(result.as_reg.parms), &(val.as_reg.parms))) != AMP_OK)
    	{
        	blob_release(&(result.as_reg.name), 0);
        	tnvc_release(&(result.as_reg.parms), 0);
        	return result;
    	}
    }

    return result;
}


ari_t *ari_copy_ptr(ari_t *val)
{
	ari_t *result = NULL;
	int success = AMP_OK;

	if(val == NULL)
	{
		return NULL;
	}

	result = ari_create(val->type);
	CHKNULL(result);

	*result = ari_copy(*val, &success);
	if(success != AMP_OK)
	{
		SRELEASE(result); // don't call ari_release here since ari_copy cleans up anyway.
		result = NULL;
	}

	return result;
}



ari_t* ari_create(amp_type_e type)
{
	ari_t *result = STAKE(sizeof(ari_t));
	CHKNULL(result);
	result->type = type;
	if(type != AMP_TYPE_LIT)
	{
		if(tnvc_init(&(result->as_reg.parms), 0) != AMP_OK)
		{
			SRELEASE(result);
			result = NULL;
		}
	}
	return result;
}


/******************************************************************************
 *
 * \par Function Name: ari_deserialize
 *
 * \par Builds an ARI from a CBOR stream.
 *
 * \retval The deserialized structure.
 *
 * \param[in|out]  it       The CBOR value iterator being read/advanced
 * \param[out]     success  Whether the deserialization succeeded.
 *
 * \par Notes:
 *   - A deserialization error will return  an ARI with unknown type.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field.
 *  09/18/18  E. Birrane     Move to ARI from MID
 *****************************************************************************/
ari_t ari_deserialize(QCBORDecodeContext *it, int *success)
{
	uint8_t flag;

    AMP_DEBUG_ENTRY("ari_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

    CHKUSR(it, ari_null());
    CHKUSR(success, ari_null());

    /* Grab the first byte to see what we've got. */
    *success = cut_get_cbor_numeric(it, AMP_TYPE_BYTE, &flag);
    if (*success != AMP_OK)
    {
    	AMP_DEBUG_ERR("ari_deserialize", "Can't get first byte: %d", success);
    	return ari_null();
    }

    if(ARI_GET_FLAG_TYPE(flag) == AMP_TYPE_LIT)
    {
       return p_ari_deserialize_lit(it, flag, success);
    }

    return p_ari_deserialize_reg(it, flag, success);
}


ari_t *ari_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	ari_t tmp;
	ari_t *result = NULL;

	*success = AMP_FAIL;

	tmp = ari_deserialize(it, success);
	if(*success == AMP_OK)
	{
		result = ari_copy_ptr(&tmp);
		ari_release(&tmp, 0);
	}

	return result;
}


ari_t* ari_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext parser;

	*success = AMP_FAIL;

	if(data == NULL)
	{
		return NULL;
	}

	QCBORDecode_Init(&parser,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	ari_t *tmp = ari_deserialize_ptr(&parser, success);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&parser);

	return tmp;
}



/*
 * Retrieve mid from unsigned long value representing mid in hex
 * 0x31801...
 *
 * 1/5/18
 *
 *to ARI 9/19/18
 */

ari_t*   ari_from_uvast(uvast val)
{
	QCBORDecodeContext it;
	int success = AMP_FAIL;
	ari_t *result = NULL;
    
	QCBORDecode_Init(&it,
					 (UsefulBufC){&val,sizeof(uvast)},
					 QCBOR_DECODE_MODE_NORMAL);

	result = ari_deserialize_ptr(&it, &success);

	if(success != AMP_OK)
	{
		ari_release(result, 1);
		result = NULL;
	}

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return result;
}


// Must not free. Shallow pointer
tnv_t* ari_get_param(ari_t *ari, int i)
{
	CHKNULL(ari);

	if((ari->type == AMP_TYPE_LIT) ||
	   (ARI_GET_FLAG_PARM(ari->as_reg.flags) == 0))
	{
		return NULL;
	}

	return tnvc_get(&(ari->as_reg.parms), i);
}



uint8_t  ari_get_num_parms(ari_t *ari)
{

	CHKZERO(ari);

	if((ari->type == AMP_TYPE_LIT) ||
	   (ARI_GET_FLAG_PARM(ari->as_reg.flags) == 0))
	{
		return 0;
	}

	return tnvc_get_count(&(ari->as_reg.parms));
}


void ari_init(ari_t *ari)
{
	CHKVOID(ari);
	memset(ari, 0, sizeof(ari_t));
	ari->type = AMP_TYPE_UNK;
}




ari_t ari_null()
{
	static int i = 0;
	static ari_t result;

	if(i == 0)
	{
		ari_init(&result);
		i = 1;
	}
	return result;
}



/******************************************************************************
 *
 * \par Function Name: mid_release
 *
 * \par Purpose: Frees resources associated with a MID
 *
 * \param[in,out] mid  The MID being released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *  09/18/18  E. Birrane     Move to ARI from MID
 *****************************************************************************/

void ari_release(ari_t *ari, int destroy)
{
	CHKVOID(ari);

	if(ari->type == AMP_TYPE_LIT)
	{
		tnv_release(&(ari->as_lit), 0);
	}
	else
	{
		blob_release(&(ari->as_reg.name), 0);
		tnvc_release(&(ari->as_reg.parms), 0);
	}

	if(destroy)
	{
		SRELEASE(ari);
	}
}

int ari_replace_parms(ari_t *ari, tnvc_t *new_parms)
{
	CHKUSR(ari, AMP_FAIL);
	CHKUSR((ari->type != AMP_TYPE_LIT), AMP_FAIL);

	tnvc_clear(&(ari->as_reg.parms));
	return ari_add_parm_set(ari, new_parms);
}

/*
 * Create a version of the parsm that does parm flow-down from
 * source to cur.
 *
 * returned parms must be freed.
 * TODO: Make more efficient. Do we really need the deep copies?
 */
tnvc_t *ari_resolve_parms(tnvc_t *src_parms, tnvc_t *parent_parms)
{
	uint8_t idx;
	tnvc_t *result = NULL;


	if((src_parms == NULL) && (parent_parms == NULL))
	{
		return NULL;
	}

	result = tnvc_copy(src_parms);

	if(result == NULL)
	{
		return NULL;
	}

	if((parent_parms == NULL) ||
	   (tnvc_size(src_parms) == 0) ||
	   (tnvc_size(parent_parms) == 0))
	{
		return result;
	}

	for(idx = 0; idx < tnvc_size(result); idx++)
	{
		tnv_t *cur_val = tnvc_get(result, idx);

		if(TNV_IS_MAP(cur_val->flags))
		{
			uint8_t parent_idx = cur_val->value.as_uint;
			tnv_t *parent_val = tnvc_get(parent_parms, parent_idx);
			tnv_t *new_tnv = tnv_copy_ptr(parent_val);
			if(tnvc_update(result, idx, new_tnv) != AMP_OK)
			{
				AMP_DEBUG_ERR("ari_resolve_parms",
						      "Can't apply parm map: %d -> %d", idx, parent_idx);
				tnv_release(new_tnv, 1);
				tnvc_release(result, 1);
				result = NULL;
				break;
			}
		}
	}

	return result;
}

int ari_serialize(QCBOREncodeContext *encoder, void *item)
{
	ari_t *ari = (ari_t *) item;
	CHKUSR(item, AMP_FAIL);

	if(ari->type == AMP_TYPE_LIT)
	{
		return p_ari_serialize_lit(encoder, ari);
	}

	return p_ari_serialize_reg(encoder, ari);
}

blob_t* ari_serialize_wrapper(ari_t *ari)
{
	return cut_serialize_wrapper(ARI_DEFAULT_ENC_SIZE, ari, (cut_enc_fn)ari_serialize);
}




int ac_append(ac_t *dest, ac_t *src)
{
	if((dest == NULL) || (src == NULL))
	{
		return AMP_FAIL;
	}

	if(vec_num_entries(src->values) <= 0)
	{
		return AMP_OK;
	}

	return vec_append(&(dest->values), &(src->values));
}


void ac_clear(ac_t *ac)
{
	CHKVOID(ac);

	vec_clear(&(ac->values));
}


int ac_init(ac_t *ac)
{
	int success;

	CHKUSR(ac, AMP_FAIL);

	ac->values = vec_create(0, ari_cb_del_fn, ari_cb_comp_fn, ari_cb_copy_fn, VEC_FLAG_AS_STACK, &success);

	return success;
}


ac_t *ac_create()
{
	ac_t *result;
	int success;

	if((result = STAKE(sizeof(ac_t))) == NULL)
	{
		AMP_DEBUG_ERR("ac_create","Can't allocate ac", NULL);
		return NULL;
	}

	result->values = vec_create(0, ari_cb_del_fn, ari_cb_comp_fn, ari_cb_copy_fn, VEC_FLAG_AS_STACK, &success);

	if(success != AMP_OK)
	{
		vec_release(&(result->values), 0);
		SRELEASE(result);
		result = NULL;
	}

	return result;
}


ac_t ac_copy(ac_t *src)
{
	vecit_t it;
	ari_t *cur_ari = NULL;
	ari_t *new_ari = NULL;
	int success = AMP_FAIL;
	ac_t result;

	memset(&result, 0, sizeof(ac_t));

	CHKUSR(src, result);

	result.values = vec_create(0, ari_cb_del_fn, ari_cb_comp_fn, ari_cb_copy_fn, VEC_FLAG_AS_STACK, &success);


	for(it = vecit_first(&(src->values)); vecit_valid(it); it = vecit_next(it))
	{
		cur_ari = vecit_data(it);
		new_ari = ari_copy_ptr(cur_ari);
		if((success = vec_insert(&(result.values), new_ari, NULL)) != AMP_OK)
		{
			AMP_DEBUG_ERR("ac_copy","Error copying AC.", NULL);
			ac_release(&result, 0);
			memset(&result, 0, sizeof(ac_t));
			break;
		}
	}

	return result;
}

ac_t*     ac_copy_ptr(ac_t *src)
{
	ac_t *result = NULL;

	CHKNULL(src);

	if((result = ac_create()) == NULL)
	{
		return NULL;
	}

	*result = ac_copy(src);

	return result;
}

/*
int ac_compare(ac_t *a1, ac_t *a2)
{
	if((a1 == NULL) || (a2 == NULL))
	{
		return -1;
	}

	return vec_
}
*/

/*
 * If bad success, result is undefined.
 */

ac_t ac_deserialize(QCBORDecodeContext *it, int *success)
{
	QCBORItem item;
	QCBORError err;
	ac_t result;
	size_t length;
	size_t i;

	AMP_DEBUG_ENTRY("ac_deserialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")", (uaddr)it, (uaddr)success);

	memset(&result, 0, sizeof(ac_t));

	CHKUSR(it, result);
	CHKUSR(success, result);

	*success = AMP_FAIL;

	// Get Next Item (Array Container)
	err = QCBORDecode_GetNext(it, &item);
	if ( err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
		AMP_DEBUG_ERR("ac_deserialize","AC not a container. Error is %d", err);
		return result;
	}

	length = item.val.uCount;

	result.values = vec_create(length, ari_cb_del_fn, ari_cb_comp_fn, ari_cb_copy_fn, VEC_FLAG_AS_STACK, success);

	for(i = 0; i < length; i++)
	{
#if AMP_VERSION < 7
		blob_t *blob = blob_deserialize_ptr(it, success);
		ari_t *cur_ari = ari_deserialize_raw(blob, success);
		blob_release(blob, 1);
#else
		QCBORDecode_StartOctets(it);
		ari_t *cur_ari = ari_deserialize_ptr(it, success);
		QCBORDecode_EndOctets(it);
#endif
		
		if((*success = ac_insert(&result, cur_ari)) != AMP_OK)
		{
			AMP_DEBUG_ERR("ac_deserialize","Can't grab ARI #%d.", i);
			ac_release(&result, 0);
			return result;
		}
	}

	/* NOTE: We are not checking if we successfully completed array traversal here. 
	 *	To do so with provided APIs requires access to last retrieved sub-item, not available with current API
	 *	Caller should use QCBORDecode_Finish() to check state on completion to catch errors.
	 */
    
	*success = AMP_OK;
	return result;
}



ac_t* ac_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	ac_t *result = ac_create();

	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(result);

	*result = ac_deserialize(it, success);
	if(*success != AMP_OK)
	{
		ac_release(result, 1);
		result = NULL;
	}

	return result;
}

ac_t ac_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext parser;

	*success = AMP_FAIL;
	if(data == NULL)
	{
		ac_t tmp;
		memset(&tmp, 0, sizeof(ac_t));
		return tmp;
	}

	QCBORDecode_Init(&parser,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);
	
	ac_t tmp = ac_deserialize(&parser, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&parser);

	return tmp;
}

/*
 * Shallow get.
 */
ari_t* ac_get(ac_t* ac, uint8_t index)
{
	ari_t *result;

	CHKNULL(ac);

	result = (ari_t *) vec_at(&(ac->values), index);

	return result;
}


uint8_t   ac_get_count(ac_t* ac)
{
	CHKZERO(ac);
	return vec_num_entries(ac->values);
}

int ac_insert(ac_t* ac, ari_t *ari)
{
	CHKUSR(ac, AMP_FAIL);
	CHKUSR(ari, AMP_FAIL);

	return vec_push(&(ac->values), ari);
}

void ac_release(ac_t *ac, int destroy)
{
	if(ac == NULL)
	{
		return;
	}

	vec_release(&(ac->values), 0);
	if(destroy)
	{
		SRELEASE(ac);
	}
}


int ac_serialize(QCBOREncodeContext *encoder, void *item)
{
	vec_idx_t i;
	vec_idx_t max;
	ac_t *ac = (ac_t*) item;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(ac, AMP_FAIL);

	max = vec_num_entries(ac->values);
	QCBOREncode_OpenArray(encoder);

	for(i = 0; i < max; i++)
	{
#if AMP_VERSION < 7
		blob_t *result = ari_serialize_wrapper((ari_t*) vec_at(&(ac->values), i));

		if(result != NULL)
		{
			QCBOREncode_AddBytes(encoder, (UsefulBufC){result->value,result->length} );
			blob_release(result, 1);
		}
#else
		QCBOREncode_OpenArray(encoder);
		ari_serialize(encoder, vec_at(&(ac->values), i) );
		QCBOREncode_CloseArrayOctet(encoder); // Close Octets Sequence in parent Container
#endif
	}
	QCBOREncode_CloseArray(encoder);

	return AMP_OK;
}


blob_t*	 ac_serialize_wrapper(ac_t *ac)
{
	CHKNULL(ac);

	return cut_serialize_wrapper(vec_num_entries(ac->values) * ARI_DEFAULT_ENC_SIZE, ac, (cut_enc_fn)ac_serialize);
}


