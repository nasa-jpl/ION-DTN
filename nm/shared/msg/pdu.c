/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file pdu.c
 **
 **
 ** Description:
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/24/12  E. Birrane     Initial Implementation (JHU/APL)
 **  11/01/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  06/26/13  E. Birrane     Added group timestamp (JHU/APL)
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "../adm/adm.h"
#include "../msg/pdu.h"
#include "../primitives/mid.h"
#include "../utils/utils.h"

pdu_group_t *pdu_create_empty_group()
{
	pdu_group_t *result = NULL;

	result = (pdu_group_t*) STAKE(sizeof(pdu_group_t));
	CHKNULL(result);

	result->msgs = lyst_create();
	result->time = time(NULL);
	return result;
}


pdu_group_t *pdu_create_group(pdu_msg_t *msg)
{
	pdu_group_t *result = NULL;

	result = pdu_create_empty_group();
	lyst_insert_last(result->msgs, msg);
	result->time = time(NULL);
	return result;
}

pdu_header_t *pdu_create_hdr(uint8_t id, uint8_t ack, uint8_t nack, uint8_t acl)
{
	pdu_header_t *result = NULL;

	AMP_DEBUG_ENTRY("pdu_create_hdr","(%d, %d, %d, %d, %d)",id ,ack,
			           nack, acl);

	if((result = (pdu_header_t *) STAKE(sizeof(pdu_header_t))) == NULL)
	{
		AMP_DEBUG_ERR("pdu_create_hdr","Can't alloc %d bytes",
				        sizeof(pdu_header_t));
		AMP_DEBUG_EXIT("pdu_create_hdr","->NULL",NULL);
		return NULL;
	}

	result->id = id;
	result->type = id & 0x07;
	result->context = (id >> 3) & 0x03;
	result->ack = ack;
	result->nack = nack;
	result->acl = acl;


	AMP_DEBUG_EXIT("pdu_create_hdr","->0x%x",(unsigned long)result);
	return result;
}

pdu_msg_t *pdu_create_msg(uint8_t id,
		                  uint8_t *data,
		                  uint32_t data_size,
		                  pdu_acl_t *acl)
{
	pdu_msg_t *result = NULL;

	AMP_DEBUG_ENTRY("pdu_create_msg","(%d, 0x%x, %d, 0x%x)",
			          id, (unsigned long) data, data_size, (unsigned long) acl);


	/* Step 0: Sanity Check. */
	if(data == NULL)
	{
		AMP_DEBUG_ERR("pdu_create_msg","Bad args",NULL);
		AMP_DEBUG_EXIT("pdu_create_msg","->0x%x",result);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (pdu_msg_t*)STAKE(sizeof(pdu_msg_t))) == NULL)
	{
		AMP_DEBUG_ERR("pdu_create_msg","Can't Alloc %d bytes",
				        sizeof(pdu_msg_t));
		AMP_DEBUG_EXIT("pdu_create_msg","->0x%x",result);
		return NULL;
	}

	/* Step 3: Shallow-copy the result. */
	result->hdr = pdu_create_hdr(id,0,0,0);
	result->contents = data;
	result->size = data_size;
	result->acl = acl;

	AMP_DEBUG_EXIT("pdu_create_msg","->0x%x",result);
	return result;
}



void pdu_release_hdr(pdu_header_t *hdr)
{
	if(hdr != NULL)
	{
		SRELEASE(hdr);
	}
}

void pdu_release_meta(pdu_metadata_t *meta)
{
	if(meta != NULL)
	{
		SRELEASE(meta);
	}
}

void pdu_release_acl(pdu_acl_t *acl)
{
	if(acl != NULL)
	{
		SRELEASE(acl);
	}
}

void pdu_release_msg(pdu_msg_t *pdu)
{
	if(pdu != NULL)
	{
		SRELEASE(pdu->hdr);
		SRELEASE(pdu->contents);
		SRELEASE(pdu->acl);
		SRELEASE(pdu);
	}
}

void pdu_release_group(pdu_group_t *group)
{
	LystElt elt;

	if(group == NULL)
	{
		return;
	}

	for(elt = lyst_first(group->msgs); elt; elt = lyst_next(elt))
	{
		pdu_msg_t *cur_msg = (pdu_msg_t*) lyst_data(elt);
		pdu_release_msg(cur_msg);
	}
	lyst_destroy(group->msgs);
	SRELEASE(group);
}


/**
 * \brief builds a serialized header from a header structure.
 *
 * \author Ed Birrane
 *
 * \note
 *   - The returned serialized header has been taken from the memory pool and
 *     must be returned to the pool when no longer needed.
 *   - Right now, the whole header is a byte, but we allocate anyway for future
 *     expansion.
 *
 * \return NULL - Failure.
 * 		   !NULL - The serialized header.
 *
 * \param[in]  hdr  The header being serialized.
 * \param[out] len  The length of the serialized header, in bytes.
 */
uint8_t *pdu_serialize_hdr(pdu_header_t *hdr, uint32_t *len)
{
	uint8_t *result = NULL;
	uint32_t result_len = 1;

	AMP_DEBUG_ENTRY("pdu_serialize_hdr","(0x%x, 0x%x)",
			          (unsigned long) hdr, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((hdr == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("pdu_serialize_hdr","Bad Args.",NULL);
		AMP_DEBUG_EXIT("pdu_serialize_hdr","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Grab space. */
	if((result = (uint8_t*)STAKE(result_len)) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_hdr","Can't allocate %d bytes.",
				        result_len);

		*len = 0;
		AMP_DEBUG_EXIT("pdu_serialize_hdr","->NULL",NULL);
		return NULL;
	}

	*len = 1;
	/* Step 2: Populate the space. */
	*result =  (hdr->type    & 0x7)  << 5;
	*result |= (hdr->context & 0x3)  << 3;
	*result |= (hdr->ack     & 0x01) << 2;
	*result |= (hdr->nack    & 0x01) << 1;
	*result |= (hdr->acl     & 0x01);

	AMP_DEBUG_EXIT("pdu_serialize_hdr","->0x%x",(unsigned long)result);
	return result;
}



/**
 * \brief builds a serialized ACL from an acl structure.
 *
 * \author Ed Birrane
 *
 * \note
 *   - The returned serialized ACL has been taken from the memory pool and
 *     must be returned to the pool when no longer needed.
 *
 * \todo
 *   - Implement this.
 *
 * \return NULL - Failure.
 * 		   !NULL - The serialized ACL.
 *
 * \param[in]  acl  The acl being serialized.
 * \param[out] len  The length of the serialized header, in bytes.
 */
uint8_t *pdu_serialize_acl(pdu_acl_t *acl, uint32_t *len)
{
	uint8_t *result = NULL;
	uint32_t result_len = sizeof(pdu_acl_t);

	AMP_DEBUG_ENTRY("pdu_serialize_acl","(0x%x, 0x%x)",
			          (unsigned long) acl, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((acl == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("pdu_serialize_acl","Bad Args.",NULL);
		AMP_DEBUG_EXIT("pdu_serialize_acl","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Grab space. */
	if((result = (uint8_t*)STAKE(result_len)) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_acl","Can't allocate %d bytes.",
				        result_len);

		*len = 0;
		AMP_DEBUG_EXIT("pdu_serialize_acl","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the space. */

	/* \todo: Implement this. */
	AMP_DEBUG_ERR("pdu_serialize_acl","Not implemented yet.",NULL);
	SRELEASE(result);
	result = NULL;

	AMP_DEBUG_EXIT("pdu_serialize_acl","->0x%x",(unsigned long)result);
	return result;
}


uint8_t *pdu_serialize_msg(pdu_msg_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *hdr = NULL;
	uint32_t hdr_len = 0;

	uint8_t *acl = NULL;
	uint32_t acl_len = 0;

	AMP_DEBUG_ENTRY("pdu_serialize_msg","(0x%x, 0x%x)",
			          (unsigned long) msg, (unsigned long) len);

	if((hdr = pdu_serialize_hdr(msg->hdr, &hdr_len)) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_msg","Can't serialize hdr",NULL);
		AMP_DEBUG_EXIT("pdu_serialize_msg","->NULL",NULL);
		return NULL;
	}

	if(msg->acl != NULL)
	{
		if((acl = pdu_serialize_acl(msg->acl, &acl_len)) == NULL)
		{
			AMP_DEBUG_ERR("pdu_serialize_msg","Can't serialize acl",NULL);

			SRELEASE(hdr);
			AMP_DEBUG_EXIT("pdu_serialize_msg","->NULL",NULL);
			return NULL;
		}
	}

	*len = hdr_len + msg->size + acl_len;

	if((result = (uint8_t *) STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_msg","Can't alloc %d bytes",*len);

		SRELEASE(hdr);
		SRELEASE(acl);
		AMP_DEBUG_EXIT("pdu_serialize_msg","->NULL",NULL);
		return NULL;
	}

	cursor = result;

	memcpy(cursor,hdr,hdr_len);
	cursor += hdr_len;
	SRELEASE(hdr);

	memcpy(cursor,msg->contents, msg->size);
	cursor += msg->size;

	if(msg->acl != NULL)
	{
		memcpy(cursor, acl, acl_len);
		cursor += acl_len;
		SRELEASE(acl);
	}

	if((cursor-result) != *len)
	{
		AMP_DEBUG_ERR("pdu_serialize_msg","Wrote %d not %d bytes!",
				        (unsigned long)(cursor-result), *len);
		SRELEASE(result);
		AMP_DEBUG_EXIT("pdu_serialize_msg","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("pdu_serialize_msg","->0x%x",(unsigned long)result);
	return result;
}

uint8_t *pdu_serialize_group(pdu_group_t *group, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t **tmp_data = NULL;
	uint32_t *tmp_size = NULL;

	uint32_t num_msgs = 0;
	Sdnv num_msgs_sdnv;
	Sdnv time_sdnv;

	uint32_t i = 0;
	uint32_t tot_size = 0;
	LystElt elt;

	AMP_DEBUG_ENTRY("pdu_serialize_group","(0x%x,0x%x)",
			          (unsigned long) group, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((group == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("pdu_serialize_group","Bad Args.", NULL);
		AMP_DEBUG_EXIT("pdu_serialize_group","->NULL.", NULL);
		return NULL;
	}

	num_msgs = lyst_length(group->msgs);

	/* Step 1: Allocate space to store serialized msgs. */
	if((tmp_data = (uint8_t **) STAKE(num_msgs * sizeof(uint8_t *))) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_group","Can't Alloc %d bytes.",
					    num_msgs * sizeof(uint8_t *));
		AMP_DEBUG_EXIT("pdu_serialize_group","->NULL.", NULL);
		return NULL;
	}
	else
	{
		memset(tmp_data,0,num_msgs * sizeof(uint8_t*));
	}

	if((tmp_size = (uint32_t *) STAKE(num_msgs * sizeof(uint32_t))) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_group","Can't Alloc %d bytes.",
					    num_msgs * sizeof(uint32_t));
		SRELEASE(tmp_data);
		AMP_DEBUG_EXIT("pdu_serialize_group","->NULL.", NULL);
		return NULL;
	}
	else
	{
		memset(tmp_size, 0, num_msgs * sizeof(uint32_t));
	}

	/* Step 2: Serialize messages in turn. */
	tot_size = 0;
	for(elt = lyst_first(group->msgs); elt; elt = lyst_next(elt))
	{
		pdu_msg_t *cur_msg = (pdu_msg_t*) lyst_data(elt);

		if(cur_msg == NULL)
		{
			AMP_DEBUG_WARN("pdu_serialize_group","Null %dth msg", i);
		}
		else
		{
			if((tmp_data[i] = pdu_serialize_msg(cur_msg,&(tmp_size[i]))) != NULL)
			{
				tot_size += tmp_size[i];
				i++;
			}
			else
			{
				AMP_DEBUG_WARN("pdu_serialize_group",
						         "Can't serialize %dth msg", i);
			}
		}
	}

	/* Step 3: If we had any problems, time to bail. */
	if(i < num_msgs)
	{
		AMP_DEBUG_ERR("pdu_serialize_group","Problems serializing.",NULL);
		int j = 0;
		for(j = 0; j < i; j++)
		{
			SRELEASE(tmp_data[j]);
		}
		SRELEASE(tmp_data);
		SRELEASE(tmp_size);

		AMP_DEBUG_EXIT("pdu_serialize_group","->NULL.", NULL);
		return NULL;
	}

	/* Step 4: Add size and time and allocate final result. */
	encodeSdnv(&num_msgs_sdnv, num_msgs);
	encodeSdnv(&time_sdnv, group->time);

	*len = num_msgs_sdnv.length + time_sdnv.length + tot_size;

	AMP_DEBUG_INFO("pdu_serialize_group", "msgs is %d, time is %d, total is %d", num_msgs_sdnv.length, time_sdnv.length, tot_size);
	if((result = (uint8_t*) STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("pdu_serialize_group","Can't alloc %d bytes.",*len);
		int j = 0;
		for(j = 0; j < i; j++)
		{
			SRELEASE(tmp_data[j]);
		}
		SRELEASE(tmp_data);
		SRELEASE(tmp_size);

		AMP_DEBUG_EXIT("pdu_serialize_group","->NULL.", NULL);
		return NULL;
	}
	cursor = result;

	/* Step 5: Copy data into the serialize buffer. */

	memcpy(cursor, num_msgs_sdnv.text, num_msgs_sdnv.length);
	cursor += num_msgs_sdnv.length;

	memcpy(cursor, time_sdnv.text, time_sdnv.length);
	cursor += time_sdnv.length;

	for(i = 0; i < num_msgs; i++)
	{
		memcpy(cursor, tmp_data[i], tmp_size[i]);
		SRELEASE(tmp_data[i]);
		cursor += tmp_size[i];
	}

	SRELEASE(tmp_data);
	SRELEASE(tmp_size);

	AMP_DEBUG_EXIT("pdu_serialize_group","->0x%x",(unsigned long) result);
	return result;
}

/**
 * \brief Constructs a header from a serialized stream.
 *
 * \author Ed Birrane
 *
 * \note
 *   - The returned header object is allocated on the memory pool and must
 *     be released when finished.
 *
 * \param[in] cursor       The buffer holding the header.
 * \param[in] size         The size of the buffer, in bytes.
 * \param[out] bytes_used  The # bytes used to construct the header.
 */
pdu_header_t *pdu_deserialize_hdr(uint8_t *cursor,
		                          uint32_t size,
		                          uint32_t *bytes_used)
{
	pdu_header_t *result = NULL;

	AMP_DEBUG_ENTRY("pdu_deserialize_hdr","(0x%x,%d,0x%x)",
			          (unsigned long) cursor, size, (unsigned long) bytes_used);

	/* Step 0: Sanity Check */
	if((cursor == 0) || (size <= 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("pdu_deserialize_hdr","Bad args.", NULL);
		AMP_DEBUG_EXIT("pdu_deserialize_hdr","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the header object. */
	if((result = (pdu_header_t*) STAKE(sizeof(pdu_header_t))) == NULL)
	{
		AMP_DEBUG_ERR("pdu_deserialize_hdr","Can't allocate %d bytes.",
				        sizeof(pdu_header_t));
		*bytes_used = 0;

		AMP_DEBUG_EXIT("pdu_deserialize_hdr","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the object. */
	uint8_t byte = *cursor;
	result->type    = (byte & 0xE0) >> 5;
	result->context = (byte & 0x18) >> 3;
	result->ack     = (byte & 0x04) >> 2;
	result->nack    = (byte & 0x02) >> 1;
	result->acl     = (byte & 0x01);
	result->id = (result->context << 3) | result->type;
	*bytes_used = 1;

	AMP_DEBUG_EXIT("pdu_deserialize_hdr","->0x%x",result);
	return result;
}




/**
 * \brief Constructs an ACL from a serialized stream.
 *
 * \author Ed Birrane
 *
 * \note
 *   - The returned ACL object is allocated on the memory pool and must
 *     be released when finished.
 *
 * \param[in] cursor       The buffer holding the acl.
 * \param[in] size         The size of the buffer, in bytes.
 * \param[out] bytes_used  The # bytes used to construct the acl.
 */
pdu_acl_t *pdu_deserialize_acl(uint8_t *cursor,
		                          uint32_t size,
		                          uint32_t *bytes_used)
{
	pdu_acl_t *result = NULL;

	AMP_DEBUG_ENTRY("pdu_deserialize_acl","(0x%x,%d,0x%x)",
			          (unsigned long) cursor, size, (unsigned long) bytes_used);

	/* Step 0: Sanity Check */
	if((cursor == 0) || (size <= 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("pdu_deserialize_acl","Bad args.", NULL);
		AMP_DEBUG_EXIT("pdu_deserialize_acl","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the header object. */
	if((result = (pdu_acl_t*)STAKE(sizeof(pdu_acl_t))) == NULL)
	{
		AMP_DEBUG_ERR("pdu_deserialize_acl","Can't allocate %d bytes.",
				        sizeof(pdu_acl_t));
		*bytes_used = 0;

		AMP_DEBUG_EXIT("pdu_deserialize_acl","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the object. */
	/* \todo: Implement this. */
	AMP_DEBUG_ERR("pdu_deserialize_acl","Not implemented yet.",NULL);
	SRELEASE(result);
	result = NULL;

	AMP_DEBUG_EXIT("pdu_deserialize_acl","->0x%x",result);
	return result;
}


int pdu_add_msg_to_group(pdu_group_t *group, pdu_msg_t *msg)
{
	int result = 0;
	lyst_insert_last(group->msgs, msg);
	return result;
}





/***
nm_custom_report* createCustomReport(pdu *cur_pdu)
{
    unsigned char* cursor;
    int sdnv_len = 0;
    unsigned long sdnv_tmp = 0;
    nm_custom_report *report;
    mid_t *cur_mid = NULL;
    unsigned long mid_used = 0;
    char *mid_str = NULL;

    DTNMP_DEBUG_PROC("+ PDU: createCustomReport(0x%x)", (unsigned long) cur_pdu);

    / * Step 0: Sanity check the pdu type to ensure it contains a rule * /
    if(cur_pdu->hdr.type != MSG_TYPE_DEF_CUST_RPT)
    {
        DTNMP_DEBUG_ERR("x PDU: Cannot create custom report from msg of type %d", cur_pdu->hdr.type);
        DTNMP_DEBUG_PROC("- PDU: createCustomReport -> NULL", NULL);
        return NULL;
    }

    / * Step 1: Allocate the new custom report definition. * /
    if((report = (nm_custom_report*) STAKE(sizeof(nm_custom_report))) == NULL)
    {
        DTNMP_DEBUG_ERR("x PDU: Unable to allocate new custom report definition.", NULL);
        DTNMP_DEBUG_PROC("- PDU: createRule -> NULL", NULL);
        return NULL;
    }
    report->mids = lyst_create();

    cursor = cur_pdu->content;

    DTNMP_DEBUG_INFO("i  PDU: Cursor is %x", (unsigned long) cursor);

    / * Step 2: Extract the custom report ID. * /
    report->report_id = build_mid(cursor, (cur_pdu->content + cur_pdu->data_size) - cursor, &mid_used);
    mid_str = mid_string(report->report_id);
    DTNMP_DEBUG_INFO("i PDU: Report has ID of %s. Used is %d.", mid_str, mid_used);
    SRELEASE(mid_str);

    cursor += mid_used;

    / * Step 3: Grab the list of MIDs to be produced by this rule. * /

    while(cursor < (cur_pdu->content + cur_pdu->data_size))
    {
    	cur_mid = build_mid(cursor, (cur_pdu->content + cur_pdu->data_size) - cursor, &mid_used);
        if(cur_mid != NULL)
        {
        	char *name = mid_string(cur_mid);
            lyst_insert_last(report->mids, cur_mid);
            cursor += mid_used;
            DTNMP_DEBUG_INFO("i  PDU: Added MID %s of size %d to report.", name, mid_used);
            SRELEASE(name);
        }
    }

    if(cursor != (cur_pdu->content + cur_pdu->data_size))
    {
        DTNMP_DEBUG_WARN("w MID: Unexpected size mismatch: cursor (%x) end data (%x)",
                         (unsigned long) cursor, (unsigned long) (cur_pdu->content + cur_pdu->data_size));
    }

    strcpy(report->sender.name, cur_pdu->meta.senderEid.name);
    DTNMP_DEBUG_INFO("i PDU: PDU sender EID is %s", report->sender.name);

    DTNMP_DEBUG_PROC("- PDU: createProdRule -> %x", (unsigned long) report);

    return report;
}



nm_report *createDataReport(pdu *cur_pdu)
{
    unsigned char* cursor;
    int sdnv_len = 0;
    unsigned long sdnv_tmp = 0;
    nm_report *report;

    DTNMP_DEBUG_PROC("+ PDU: createDataReport(0x%x)", (unsigned long)cur_pdu);

    / * Step 0: Sanity check the pdu type to ensure it contains a rule * /
    if(cur_pdu->hdr.type != MSG_TYPE_RPT_DATA_RPT)
    {
        DTNMP_DEBUG_ERR("x PDU: Cannot create report from msg of type %d", cur_pdu->hdr.type);
        DTNMP_DEBUG_PROC("- PDU: createDataReport -> NULL", NULL);
        return NULL;
    }

    / * Step 1: Allocate the new report. * /
    if((report = (nm_report*) STAKE(sizeof(nm_report))) == NULL)
    {
        DTNMP_DEBUG_ERR("x PDU: Unable to allocate new report of size %d.",
        				 sizeof(nm_report));
        DTNMP_DEBUG_PROC("- PDU: createDataReport -> NULL", NULL);
        return NULL;
    }

    report->length = 0;
    report->report_data = lyst_create();

    cursor = cur_pdu->content;

    DTNMP_DEBUG_INFO("i  PDU: Cursor is %x", (unsigned long) cursor);

    / * Step 2: Extract the Report Timestamp * /
    if((sdnv_len = decodeSdnv(&(sdnv_tmp), cursor)) == 0)
    {
        DTNMP_DEBUG_ERR("x PDU: No timestamp field in report msg.", NULL);

        lyst_destroy(report->report_data);
        SRELEASE(report);

        DTNMP_DEBUG_PROC("- PDU: createDataReport -> NULL", NULL);
        return NULL;
    }
    report->timestamp = sdnv_tmp;
    cursor += sdnv_len;
    DTNMP_DEBUG_INFO("i  PDU: Report has timestamp of %ld", report->timestamp);

    / * Step 3: Grab the data entries contained in this report. * /
    nm_report_entry *cur_entry = NULL;
    unsigned long mid_size = 0;
    char *mid_str;
    while(cursor < (cur_pdu->content + cur_pdu->data_size))
    {
    	/ * Allocate the entry * /
    	cur_entry = (nm_report_entry*) STAKE(sizeof(nm_report_entry));

    	/ * Grab the MID * /
        if((cur_entry->mid = build_mid(cursor,
        	   			        (cur_pdu->content + cur_pdu->data_size) - cursor,
        				        &mid_size)) == NULL)
        {
        	DTNMP_DEBUG_ERR("x PDU: Unable to build MID!", NULL);
        	SRELEASE(report);
            DTNMP_DEBUG_PROC("- PDU: createDataReport -> NULL", NULL);
            return NULL;
        }
        cursor += mid_size;
        report->length += mid_size;

        DTNMP_DEBUG_INFO("i PDU: MID size is %d", mid_size);

    	/ * Grab the data length. * /

        sdnv_len = decodeSdnv(&(sdnv_tmp), cursor);
        cur_entry->data_size = sdnv_tmp;
        report->length += sdnv_tmp;
        cursor += sdnv_len;

        DTNMP_DEBUG_INFO("i PDU: Got data size of %d", cur_entry->data_size);

    	/ * Grab the data * /
        cur_entry->data = (uint8_t*) STAKE(cur_entry->data_size);
        memcpy(cur_entry->data, cursor, cur_entry->data_size);
        cursor += cur_entry->data_size;

        mid_str = mid_string(cur_entry->mid);
        DTNMP_DEBUG_INFO("i PDU: Added entry for mid %s of size %d",
        			      mid_str, cur_entry->data_size);
        SRELEASE(mid_str);

        / * Add the entry to the entry list. * /
        lyst_insert_last(report->report_data, cur_entry);
    }

    if(cursor != (cur_pdu->content + cur_pdu->data_size))
    {
        DTNMP_DEBUG_WARN("w MID: Unexpected size mismatch: cursor (%x) end data (%x)",
                         (unsigned long) cursor, (unsigned long) (cur_pdu->content + cur_pdu->data_size));
    }

    strcpy(report->recipient.name, cur_pdu->meta.recipientEid.name);

    DTNMP_DEBUG_PROC("- PDU: createProdRule -> %x", (unsigned long) report);

    return report;

}

uint8_t *buildProdRulePDU(int offset, int period, int evals, Lyst mids, int mid_size, int *msg_len)
{
	uint8_t *pdu;
	Sdnv offset_tmp;
	Sdnv period_tmp;
	Sdnv evals_tmp;
	uint32_t length = 0;
	LystElt elt;
	int idx = 0;

	DTNMP_DEBUG_PROC("+ PDU: buildProdRulePDU(%d, %d, %d, 0x%x", offset, period, evals, (unsigned long)mids);

	/ * Translate the SDNVs * /
	encodeSdnv(&offset_tmp,offset);
	encodeSdnv(&period_tmp,period);
	encodeSdnv(&evals_tmp, evals);

	/ * Figure out the length of the resulting buffer * /
	length = 1; / * Message Type. * /

	length += offset_tmp.length;
	length += period_tmp.length;
	length += evals_tmp.length;
	length += mid_size;

	if((pdu = (uint8_t*) STAKE(length)) == NULL)
	{
		DTNMP_DEBUG_ERR("x PDU: Failed allocating pdu of size %d", length);
		return NULL;
	}

	idx = 0;
	pdu[idx++] = (uint8_t) MSG_TYPE_CTRL_PERIOD_PROD;

	memcpy(&(pdu[idx]), offset_tmp.text, offset_tmp.length);
	idx += offset_tmp.length;

	memcpy(&(pdu[idx]), period_tmp.text, period_tmp.length);
	idx += period_tmp.length;

	memcpy(&(pdu[idx]), evals_tmp.text, evals_tmp.length);
	idx += evals_tmp.length;

	for (elt = lyst_first(mids); elt; elt = lyst_next(elt))
	{
		nm_adu_entry *entry = (nm_adu_entry*) lyst_data(elt);

		if((idx + entry->mid_len) > length)
		{
			DTNMP_DEBUG_ERR("x PDU: Invalid sizes. Length %d, idx+mid %d",
							 length, idx + entry->mid_len);
			SRELEASE(pdu);
			return NULL;
		}
		memcpy(&pdu[idx], entry->mid, entry->mid_len);
		idx += entry->mid_len;
	}

	*msg_len = length;
	DTNMP_DEBUG_PROC("- PDU: buildProdRulePDU --> (0x%x)", pdu);
	return pdu;
}

uint8_t *buildReportDefPDU(mid_t *report_id, Lyst mids, uint32_t mid_size, int *msg_len)
{
	uint8_t *pdu = NULL;
	uint32_t idx = 0;
	LystElt elt;

	DTNMP_DEBUG_INFO("i report_id size %d mis_size %d i %d", report_id->raw_size, mid_size, 1);
	*msg_len = report_id->raw_size + mid_size + 1;
	pdu = (uint8_t *) STAKE(*msg_len);

	idx = 0;
	pdu[idx++] = (uint8_t) MSG_TYPE_DEF_CUST_RPT;

	memcpy(&(pdu[idx]), report_id->raw, report_id->raw_size);
	idx += report_id->raw_size;

	for (elt = lyst_first(mids); elt; elt = lyst_next(elt))
	{
		mid_t *cur_mid = (mid_t*) lyst_data(elt);
		char *mid_str = NULL;

		if((idx + cur_mid->raw_size) > *msg_len)
		{
			DTNMP_DEBUG_ERR("x PDU: Invalid sizes. Length %d, idx+mid %d",
							 *msg_len, idx + cur_mid->raw_size);
			SRELEASE(pdu);
			return NULL;
		}
		mid_str = mid_string(cur_mid);
		DTNMP_DEBUG_INFO("i PFU: Adding mid %s to custom report.", mid_str);
		SRELEASE(mid_str);
		memcpy(&pdu[idx], cur_mid->raw, cur_mid->raw_size);
		idx += cur_mid->raw_size;
	}

	DTNMP_DEBUG_PROC("- PDU: buildProdRulePDU --> (0x%x)", pdu);
	return pdu;
}
*/
