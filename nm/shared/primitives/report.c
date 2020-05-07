/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file report.c
 **
 **
 ** Description:
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/11/13  E. Birrane     Redesign of primitives architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  07/02/15  E. Birrane     Migrated to Typed Data Collections (TDCs) (Secure DTN - NASA: NNX14CS58P)
 **  01/10/18  E. Birrane     Added report templates and parameter maps (JHU/APL)
 **  09/28/18  E. Birrane     Update to latest AMP v0.5. (JHU/APL)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"

#include "report.h"

#include "tnv.h"



/******************************************************************************
 *
 * \par Function Name: rpt_add_entry
 *
 * \par Add an entry to an existing report.
 *
 * \param[in]  rpt   The report receiving the new entry.
 * \param[in]  entry The new entry.
 *
 * \par Notes:
 *  1. The entry is shallow-copied into the report. It MUST NOT be deallocated
 *     by the calling function.
 *
 * \return SUCCESS or FAILURE
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Initial implementation.
 *  09/28/18  E. Birrane     Switched to vectors for entries.
 *****************************************************************************/

int rpt_add_entry(rpt_t *rpt, tnv_t *entry)
{
	if((rpt == NULL) || (entry == NULL))
	{
		return AMP_FAIL;
	}

	return tnvc_insert(rpt->entries, entry);
}


int rpt_cb_comp_fn(void *i1, void *i2)
{
	rpt_t *r1 = (rpt_t*) i1;
	rpt_t *r2 = (rpt_t*) i2;

	CHKUSR(r1, -1);
	CHKUSR(r2, -1);

	return ari_cb_comp_fn(r1->id, r2->id);
}

void rpt_cb_del_fn(void *item)
{
	rpt_release((rpt_t*)item, 1);
}



/******************************************************************************
 *
 * \par Function Name: rpt_clear
 *
 * \par Cleans up a reports.
 *
 * \param[in|out]  rpt     The report whose entries are to be cleared
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *  09/28/18  E. Birrane     Switched to vectors for entries.
 *****************************************************************************/

void  rpt_clear(rpt_t *rpt)
{
    AMP_DEBUG_ENTRY("rpt_clear_lyst","("ADDR_FIELDSPEC")", (uaddr) rpt);
    CHKVOID(rpt);
    tnvc_clear(rpt->entries);
}

rpt_t* rpt_copy_ptr(rpt_t *src)
{
	ari_t *new_ari;
	tnvc_t *new_entries;
	rpt_t *result;

	CHKNULL(src);
	new_ari = ari_copy_ptr(src->id);
	new_entries = tnvc_copy(src->entries);

	if((result = rpt_create(new_ari, src->time, new_entries)) == NULL)
	{
		ari_release(new_ari, 1);
		tnvc_release(new_entries, 1);
		return NULL;
	}

	result->recipient = src->recipient;
	return result;
}

/******************************************************************************
 *
 * \par Function Name: rpt_create
 *
 * \par Create a new report entry.
 *
 * \param[in]  time       The creation timestamp for the report.
 * \param[in]  entries    The list of current report entries
 * \param[in]  recipient  The recipient for this report.
 *
 * \todo
 *  - Support multiple recipients.
 *
 * \par Notes:
 *  1. The entries are shallow-copied into the report, so they MUST NOT
 *     be de-allocated by the calling function.
 *
 * \return The created report, or NULL on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *  09/28/18  E. Birrane     Switched to vectors for entries.
 *****************************************************************************/

rpt_t* rpt_create(ari_t *id, time_t timestamp, tnvc_t *entries)
{
	rpt_t *result = NULL;

	AMP_DEBUG_ENTRY("rpt_create","("ADDR_FIELDSPEC",%d,entries)",
			        (uaddr) id, time);

	/* Step 1: Allocate the message. */
	if((result = (rpt_t*) STAKE(sizeof(rpt_t))) == NULL)
	{
		AMP_DEBUG_ERR("rpt_create","Can't alloc %d bytes.",
				        sizeof(rpt_t));
		AMP_DEBUG_EXIT("rpt_create","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the report. */
	result->id = id;
	result->time = timestamp;
	result->entries = (entries != NULL) ? entries : tnvc_create(0);

	if(result->entries == NULL)
	{
		AMP_DEBUG_ERR("rpt_create","Can't allocate TNVC for report.", NULL);
		SRELEASE(result);
		return NULL;
	}

	AMP_DEBUG_EXIT("rpt_create","->0x%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_deserialize_ptr
 *
 * \par Extracts a Report from a byte buffer. When serialized, a
 *      Report is a timestamp, the # entries as an SDNV, then a
 *      series of entries. Each entry is of type rpt_entry_t.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized Report.
 *
 * \param[in]  cursor       The byte buffer holding the data
 * \param[in]  size         The # bytes available in the buffer
 * \param[out] bytes_used   The # of bytes consumed in the deserialization.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *  09/28/18  E. Birrane     Switched to vectors and CBOR for entries.
 *****************************************************************************/


void* rpt_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	rpt_t *result = NULL;
	size_t len;
	time_t timestamp;
	ari_t *id;
	tnvc_t *entries;
	QCBORError err;
	QCBORItem item;

	AMP_DEBUG_ENTRY("rpt_deserialize_ptr",
			        "("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					(uaddr)it, (uaddr)success);

	/* Sanity Checks. */
	CHKNULL(success);
	*success = AMP_FAIL;
	CHKNULL(it);

	/* Step 1: Get Array & Determine how many elements are in the array */
	err = QCBORDecode_GetNext(it, &item);
	if (err != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY)
	{
		return NULL;
	}
	len = item.val.uCount;

	/* Step 2: grab the Id. */
#if AMP_VERSION < 7
	blob_t *blob = blob_deserialize_ptr(it, success);
	id = ari_deserialize_raw(blob, success);
	blob_release(blob, 1);
#else
	QCBORDecode_StartOctets(it);
	id = ari_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif
	
	if((id == NULL) || (*success != AMP_OK))
	{
		return NULL;
	}

	/* Step 3: Get timestamp, if it was included.	*/
	if(len == 3)
	{
		if(cut_get_cbor_numeric(it, AMP_TYPE_TS, &timestamp) != AMP_OK)
		{
			ari_release(id, 1);
			return NULL;
		}
	}
	else
	{
		timestamp = 0;
	}

#if AMP_VERSION < 7
	blob = blob_deserialize_ptr(it, success);
	entries = tnvc_deserialize_ptr_raw(blob, success);
	blob_release(blob, 1);
#else
	QCBORDecode_StartOctets(it);
	entries = tnvc_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif

	if(*success != AMP_OK)
	{
		ari_release(id, 1);
		return NULL;
	}

	if((result = rpt_create(id, timestamp, entries)) == NULL)
	{
		ari_release(id, 1);
		tnvc_release(entries, 1);
		*success = AMP_FAIL;
	}

	*success = AMP_OK;
	return result;
}


rpt_t*   rpt_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;

	if((data == NULL) || (success == NULL))
	{
		return NULL;
	}
	*success = AMP_FAIL;

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);
	
	rpt_t *tmp = rpt_deserialize_ptr(&it, success);

	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}





/******************************************************************************
 *
 * \par Function Name: rpt_release
 *
 *
 * \par Releases all memory allocated in the Report. Clears all entries.
 *
 * \param[in,out]  rpt    The report to be released.
 *
 * \par Notes:
 *      - The report itself is destroyed and MUST NOT be referenced after a
 *        call to this function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *  09/28/18  E. Birrane     Update to latest AMP. (JHU/APL)
 *****************************************************************************/

void rpt_release(rpt_t *rpt, int destroy)
{
	CHKVOID(rpt);

	ari_release(rpt->id, 1);
	tnvc_release(rpt->entries, 1);

	if(destroy)
	{
		SRELEASE(rpt);
	}
}


int rpt_serialize(QCBOREncodeContext *encoder, void *item)
{
#if AMP_VERSION < 7
	blob_t *result;
#endif
	int err;
	size_t num;
	rpt_t *rpt = (rpt_t *) item;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(rpt, AMP_FAIL);

	num = (rpt->time == 0) ? 2 : 3;

	/* Start a container. */
	QCBOREncode_OpenArray(encoder);

	/* Step 1: Encode the ARI. */
#if AMP_VERSION < 7
	result = ari_serialize_wrapper(rpt->id);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = ari_serialize(encoder, rpt->id);
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	
	if(err != AMP_OK)
	{
		return err;
	}
	
	if(num == 3)
	{
	   QCBOREncode_AddUInt64(encoder, rpt->time);
	}

	/* Step 3: Encode the entries. */
#if AMP_VERSION < 7
	result = tnvc_serialize_wrapper(rpt->entries);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = tnvc_serialize(encoder, rpt->entries);
	QCBOREncode_CloseArrayOctet(encoder);	
#endif

	QCBOREncode_CloseArray(encoder);
	return err;
}



blob_t*   rpt_serialize_wrapper(rpt_t *rpt)
{
	return cut_serialize_wrapper(RPT_DEFAULT_ENC_SIZE, rpt, (cut_enc_fn)rpt_serialize);
}




/******************************************************************************
 *
 * \par Function Name: rpttpl_add_item
 *
 * \par Add an item to a report template.
 *
 * * \retval 1 Success
 *           0 application error
 *           -1 system error
 *
 * \param[in|out]  rpttpl   The report template receiving a new item
 * \param[in]      item     THe item to add.
 *
 * \par Notes:
 *  1. Items are added at the end of the report template.
 *  2. This is a shallow-copy. The item MUST NOT be deleted by the caller.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/18  E. Birrane     Initial implementation.
 *  09/28/18  E. Birrane     Updated to AMP V0.5
 *****************************************************************************/

int rpttpl_add_item(rpttpl_t *rpttpl, ari_t *item)
{
	CHKERR(rpttpl);
	CHKERR(item);

	return vec_push(&(rpttpl->contents.values), item);
}



int rpttpl_cb_comp_fn(void *i1, void *i2)
{
	rpttpl_t *t1 = (rpttpl_t*)i1;
	rpttpl_t *t2 = (rpttpl_t*)i2;

	return ari_cb_comp_fn(t1->id, t2->id);
}

void rpttpl_cb_del_fn(void *item)
{
	rpttpl_release((rpttpl_t*)item, 1);
}

void rpttpl_cb_ht_del_fn(rh_elt_t *elt)
{
	CHKVOID(elt);
	elt->key = NULL;
	rpttpl_cb_del_fn(elt->value);
	elt->value = NULL;
}

rpttpl_t *rpttpl_copy_ptr(rpttpl_t *rpttpl)
{
	rpttpl_t *result = NULL;
	ari_t *new_ari = NULL;
	ac_t new_ac;

	CHKNULL(rpttpl);

	new_ari = ari_copy_ptr(rpttpl->id);
	new_ac = ac_copy(&(rpttpl->contents));

	if(((result = rpttpl_create(new_ari, new_ac))) == NULL)
	{
		ari_release(new_ari, 1);
		ac_clear(&new_ac);
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name: rpttpl_create
 *
 * \par Create a new report template.
 *
 * * \retval !NULL - The created report template.
 *            NULL - Error.
 *
 * \param[in]  id   The identifier for the template
 * \param[in]  items The items that comprise the template.
 *
 * \par Notes:
 *  1. Only 256 parameters per report entry are supported.
 *  2. Elements shallow copied in.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/09/18  E. Birrane     Initial implementation.
 *  09/28/18  E. Birrane     Updated to AMP V0.5
 *****************************************************************************/

rpttpl_t* rpttpl_create(ari_t *id, ac_t items)
{
	rpttpl_t *result = NULL;

	CHKNULL(id);

	if((result = (rpttpl_t *) STAKE(sizeof(rpttpl_t))) == NULL)
	{
		AMP_DEBUG_ERR("rpttpl_create","Can't allocate %d bytes.", sizeof(rpttpl_t));
		return NULL;
	}

	result->id = id;
	result->contents = items;

	return result;
}


rpttpl_t* rpttpl_create_id(ari_t *id)
{
	rpttpl_t *result = NULL;

	if((result = (rpttpl_t *) STAKE(sizeof(rpttpl_t))) == NULL)
	{
		AMP_DEBUG_ERR("rpttpl_create","Can't allocate %d bytes.", sizeof(rpttpl_t));
		return NULL;
	}

	result->id = id;
	ac_init(&(result->contents));

	return result;
}


rpttpl_t* rpttpl_deserialize_ptr(QCBORDecodeContext *it, int *success)
{
	rpttpl_t *result = NULL;
	ari_t *ari = NULL;
	ac_t new_ac;

	CHKNULL(success);
	*success = AMP_FAIL;

	CHKNULL(it);

#if AMP_VERSION < 7
	blob_t *tmp = blob_deserialize_ptr(it, success);
	ari = ari_deserialize_raw(tmp, success);
	blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(it);
	ari = ari_deserialize_ptr(it, success);
	QCBORDecode_EndOctets(it);
#endif
	
	if((ari == NULL) || (*success != AMP_OK))
	{
		return NULL;
	}

#if AMP_VERSION < 7
	tmp = blob_deserialize_ptr(it, success);
	new_ac = ac_deserialize_raw(tmp, success);
	blob_release(tmp, 1);
#else
	QCBORDecode_StartOctets(it);
	new_ac = ac_deserialize(it, success);
	QCBORDecode_EndOctets(it);
#endif
	if(*success != AMP_OK)
	{
		ari_release(ari, 1);
		return NULL;
	}

	if(((result = rpttpl_create(ari, new_ac))) == NULL)
	{
		*success = AMP_FAIL;
		ari_release(ari, 1);
		ac_clear(&new_ac);
		return NULL;
	}

	*success = AMP_OK;
	return result;
}

rpttpl_t* rpttpl_deserialize_raw(blob_t *data, int *success)
{
	QCBORDecodeContext it;

	if((data == NULL) || (success == NULL))
	{
		return NULL;
	}
	*success = AMP_FAIL;

	QCBORDecode_Init(&it,
					 (UsefulBufC){data->value,data->length},
					 QCBOR_DECODE_MODE_NORMAL);

	rpttpl_t *tmp = rpttpl_deserialize_ptr(&it, success);
	
	// Verify Decoding Completed Successfully
	cut_decode_finish(&it);

	return tmp;
}



/******************************************************************************
 *
 * \par Function Name: rpttpl_release
 *
 * \par Release resources associated with a report template.
 *
 * \param[in|out]  rpttpl  The template to be released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/09/18  E. Birrane     Initial implementation.
 *  09/29/18  E. Birrane     Updated to match release pattern.
 *****************************************************************************/

void rpttpl_release(rpttpl_t *rpttpl, int destroy)
{
	CHKVOID(rpttpl);

	ari_release(rpttpl->id, 1);
	ac_release(&(rpttpl->contents), 0);

	if(destroy)
	{
		SRELEASE(rpttpl);
	}
}



int rpttpl_serialize(QCBOREncodeContext *encoder, void *item)
{
	int err;;
	blob_t *result;
	int success;
	rpttpl_t *rpttpl = (rpttpl_t *)item;

	CHKUSR(encoder, AMP_FAIL);
	CHKUSR(rpttpl, AMP_FAIL);

	/* Step 1: Encode the ARI. */
#if AMP_VERSION < 7
	result = ari_serialize_wrapper(rpttpl->id);
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = ari_serialize(encoder, rpttpl->id);
	QCBOREncode_CloseArrayOctet(encoder);
#endif
	if(err != AMP_OK)
	{
		AMP_DEBUG_ERR("rpttpl_serialize","CBOR Error: %d", err);
		return err;
	}
	
	/* Step 2: Encode the type. */
#if AMP_VERSION < 7
	result = ac_serialize_wrapper(&(rpttpl->contents));
	err = blob_serialize(encoder, result);
	blob_release(result, 1);
#else
	QCBOREncode_OpenArray(encoder);
	err = ac_serialize(encoder, &(rpttpl->contents));
	QCBOREncode_CloseArrayOctet(encoder);
#endif

	return err;
}



blob_t*   rpttpl_serialize_wrapper(rpttpl_t *rpttpl)
{
	return cut_serialize_wrapper(RPTTPL_DEFAULT_ENC_SIZE, rpttpl, (cut_enc_fn)rpttpl_serialize);
}

