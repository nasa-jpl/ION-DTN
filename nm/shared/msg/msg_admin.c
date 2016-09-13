/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file msg_admin.c
 **
 ** Description:
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/21/12  E. Birrane     Initial Implementation (JHU/APL)
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "../utils/utils.h"
#include "../utils/nm_types.h"
#include "../msg/msg_admin.h"


/**
 * \brief serializes a register agent message into a buffer.
 *
 * \author Ed Birrane
 *
 * \note The returned message must be de-allocated from the memory pool.
 *
 * \return NULL - Failure
 *         !NULL - The serialized message.
 *
 * \param[in]  msg  The message to serialize.
 * \param[out] len  The length of the serialized message.
 */
uint8_t *msg_serialize_reg_agent(adm_reg_agent_t *msg, uint32_t *len)
{
	Sdnv id;

	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	AMP_DEBUG_ENTRY("msg_serialize_reg_agent","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("msg_serialize_reg_agent","Bad Args",NULL);
		AMP_DEBUG_EXIT("msg_serialize_reg_agent","->NULL",NULL);
		return NULL;
	}

	/*
	 * STEP 1: Figure out the size of the entire message. That includes the
	 *         length of the header, acl list, SDNV holding length, and data.
	 */
	int id_len = strlen(msg->agent_id.name);
	encodeSdnv(&id,id_len);
	*len = id.length + id_len;

	/* STEP 4: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_reg_agent","Can't alloc %d bytes", *len);
		*len = 0;

		AMP_DEBUG_EXIT("msg_serialize_reg_agent","->NULL",NULL);
		return NULL;
	}

	/* Step 5: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, id.text, id.length);
	cursor += id.length;

	memcpy(cursor, msg->agent_id.name, id_len);
	cursor += id_len;

	/* Step 6: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("msg_serialize_reg_agent","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("msg_serialize_reg_agent","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("msg_serialize_reg_agent","->0x%x",(unsigned long)result);
	return result;
}



/**
 * \brief serializes a report policy message into a buffer.
 *
 * \author Ed Birrane
 *
 * \note The returned message must be de-allocated from the memory pool.
 *
 * \return NULL - Failure
 *         !NULL - The serialized message.
 *
 * \param[in]  msg  The message to serialize.
 * \param[out] len  The length of the serialized message.
 */

uint8_t *msg_serialize_rpt_policy(adm_rpt_policy_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	AMP_DEBUG_ENTRY("msg_serialize_rpt_policy","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("msg_serialize_rpt_policy","Bad Args",NULL);
		AMP_DEBUG_EXIT("msg_serialize_rpt_policy","->NULL",NULL);
		return NULL;
	}


	/*
	 * STEP 1: Figure out the size of the entire message. That includes the
	 *         length of the header, acl list, and 1 byte for the mask.
	 */
	*len = 1;

	/* STEP 4: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_rpt_policy","Can't alloc %d bytes", *len);
		*len = 0;

		AMP_DEBUG_EXIT("msg_serialize_rpt_policy","->NULL",NULL);
		return NULL;
	}

	/* Step 5: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, &(msg->mask),1);
	cursor += 1;

	/* Step 6: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("msg_serialize_rpt_policy","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("msg_serialize_rpt_policy","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("msg_serialize_rpt_policy","->0x%x",(unsigned long)result);
	return result;
}



/**
 * \brief serializes a status message into a buffer.
 *
 * \author Ed Birrane
 *
 * \note The returned message must be de-allocated from the memory pool.
 *
 * \return NULL - Failure
 *         !NULL - The serialized message.
 *
 * \param[in]  msg  The message to serialize.
 * \param[out] len  The length of the serialized message.
 */

uint8_t *msg_serialize_stat_msg(adm_stat_msg_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *code = NULL;
	uint32_t code_size = 0;

	uint8_t *list = NULL;
	uint32_t list_size = 0;

	Sdnv time;

	AMP_DEBUG_ENTRY("msg_serialize_stat_msg","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("msg_serialize_stat_msg","Bad Args",NULL);
		AMP_DEBUG_EXIT("msg_serialize_stat_msg","->NULL",NULL);
		return NULL;
	}


	/* STEP 3: Serialize the Code. */
	if((code = mid_serialize(msg->code, &code_size)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_stat_msg","Can't serialize code.",
				         NULL);
		AMP_DEBUG_EXIT("msg_serialize_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* STEP 4: Serialize the MID Collection. */
	if((list = midcol_serialize(msg->generators, &list_size)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_stat_msg","Can't serialize code.",
				         NULL);
		SRELEASE(code);
		AMP_DEBUG_EXIT("msg_serialize_stat_msg","->NULL",NULL);
		return NULL;

	}

	/* Step 5: Build the timestamp SDNV. */
	encodeSdnv(&time, msg->time);

	/* STEP 6: Figure out the size of the entire message. */
	*len = code_size + list_size + time.length;


	/* STEP 7: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_stat_msg","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(code);
		SRELEASE(list);
		AMP_DEBUG_EXIT("msg_serialize_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* Step 8: Populate the serialized message. */
	cursor = result;

	memcpy(cursor,code,code_size);
	cursor += code_size;
	SRELEASE(code);

	memcpy(cursor, time.text, time.length);
	cursor += time.length;

	memcpy(cursor, list, list_size);
	cursor += list_size;
	SRELEASE(list);

	/* Step 9: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("msg_serialize_stat_msg","Wrote %d bytes but alloc %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("msg_serialize_stat_msg","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("msg_serialize_stat_msg","->0x%x",(unsigned long)result);
	return result;
}


/* Deserialize functions. */

/**
 * \brief Creates a register agent message from a buffer.
 *
 * \author Ed Birrane
 *
 * \note
 *   - On failure (NULL return) we do NOT de-allocate the passed-in header.
 *
 * \return NULL - failure
 *         !NULL - message.
 *
 * \param[in]  cursor      The buffer holding the message.
 * \param[in]  size        The remaining buffer size.
 * \param[out] bytes_used  Bytes consumed in the deserialization.
 */
adm_reg_agent_t *msg_deserialize_reg_agent(uint8_t *cursor,
		                                   uint32_t size,
		                                   uint32_t *bytes_used)
{
	adm_reg_agent_t *result = NULL;

	AMP_DEBUG_ENTRY("msg_deserialize_reg_agent","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("msg_deserialize_reg_agent","Bad Args.",NULL);
		AMP_DEBUG_EXIT("msg_deserialize_reg_agent","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (adm_reg_agent_t*)STAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_reg_agent","Can't Alloc %d Bytes.",
				        sizeof(adm_reg_agent_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("msg_deserialize_reg_agent","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(adm_reg_agent_t));
	}

	/* Step 2: Deserialize the message. */

	/* Grab and check the size, as an SDNV */
	int sdnv_len = 0;
	uvast sdnv_tmp = 0;

	sdnv_len = decodeSdnv(&(sdnv_tmp), cursor);
	if(sdnv_len > AMP_MAX_EID_LEN)
	{
		AMP_DEBUG_ERR("msg_deserialize_reg_agent", "EID size %d > max %d.",
				        sdnv_tmp, AMP_MAX_EID_LEN);

		msg_release_reg_agent(result);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("msg_deserialize_reg_agent","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += sdnv_len;
		size -= sdnv_len;
		*bytes_used += sdnv_len;
	}

	// Copy EID.
	memcpy(result->agent_id.name,cursor,sdnv_tmp);
	cursor += sdnv_tmp;
	size -= sdnv_tmp;
	*bytes_used += sdnv_tmp;

	AMP_DEBUG_EXIT("msg_deserialize_reg_agent","->0x%x",(unsigned long)result);
	return result;
}



/**
 * \brief Creates a report policy message from a buffer.
 *
 * \author Ed Birrane
 *
 * \note
 *   - On failure (NULL return) we do NOT de-allocate the passed-in header.
 *
 * \return NULL - failure
 *         !NULL - message.
 *
 * \param[in]  cursor      The buffer holding the message.
 * \param[in]  size        The remaining buffer size.
 * \param[out] bytes_used  Bytes consumed in the deserialization.
 */
adm_rpt_policy_t *msg_deserialize_rpt_policy(uint8_t *cursor,
        									 	 uint32_t size,
        									 	 uint32_t *bytes_used)
{
	adm_rpt_policy_t *result = NULL;

	AMP_DEBUG_ENTRY("msg_deserialize_rpt_policy","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size,
			          (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("msg_deserialize_rpt_policy","Bad Args.",NULL);
		AMP_DEBUG_EXIT("msg_deserialize_rpt_policy","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (adm_rpt_policy_t*)STAKE(sizeof(adm_rpt_policy_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_rpt_policy","Can't Alloc %d Bytes.",
				        sizeof(adm_rpt_policy_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("msg_deserialize_rpt_policy","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(adm_rpt_policy_t));
	}

	/* Step 2: Deserialize the message. */

	/* Grab the mask */
	result->mask = *cursor;
	cursor++;
	size--;
	*bytes_used += 1;


	AMP_DEBUG_EXIT("msg_deserialize_rpt_policy","->0x%x",
			         (unsigned long)result);
	return result;
}



/**
 * \brief Creates a status message from a buffer.
 *
 * \author Ed Birrane
 *
 * \note
 *   - On failure (NULL return) we do NOT de-allocate the passed-in header.
 *
 * \return NULL - failure
 *         !NULL - message.
 *
 * \param[in]  cursor      The buffer holding the message.
 * \param[in]  size        The remaining buffer size.
 * \param[out] bytes_used  Bytes consumed in the deserialization.
 */

adm_stat_msg_t   *msg_deserialize_stat_msg(uint8_t *cursor,
        								   uint32_t size,
        								   uint32_t *bytes_used)
{
	adm_stat_msg_t *result = NULL;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("msg_deserialize_stat_msg","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size,
			          (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("msg_deserialize_stat_msg","Bad Args.",NULL);
		AMP_DEBUG_EXIT("msg_deserialize_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (adm_stat_msg_t*)STAKE(sizeof(adm_stat_msg_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_stat_msg","Can't Alloc %d Bytes.",
				        sizeof(adm_stat_msg_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("msg_deserialize_stat_msg","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(adm_stat_msg_t));
	}

	/* Step 2: Deserialize the message. */

	/* Grab the mask */
	if((result->code = mid_deserialize(cursor,size,&bytes)) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_stat_msg","Can't get code MID.",NULL);
		*bytes_used = 0;
		msg_release_stat_msg(result);

		AMP_DEBUG_EXIT("msg_deserialize_stat_msg","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Grab the timestamp */
	uvast val = 0;
	if((bytes = utils_grab_sdnv(cursor, size, &val)) == 0)
	{
		AMP_DEBUG_ERR("msg_deserialize_stat_msg","Can't get timestamp.",NULL);
		*bytes_used = 0;
		msg_release_stat_msg(result);

		AMP_DEBUG_EXIT("msg_deserialize_stat_msg","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
		result->time = val;
	}

	/* Grab the Lyst. */
	if((result->generators = midcol_deserialize(cursor,size,&bytes)) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_stat_msg","Can't get generators.",NULL);
		*bytes_used = 0;
		msg_release_stat_msg(result);

		AMP_DEBUG_EXIT("msg_deserialize_stat_msg","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("msg_deserialize_stat_msg","->0x%x",
			         (unsigned long)result);
	return result;
}






/*******

prod_rule *createProdRule(pdu* cur_pdu) 
{
    unsigned char* cursor;
    int sdnv_len = 0;
    unsigned long sdnv_tmp = 0;
    prod_rule *rule;
    
    DTNMP_DEBUG_PROC("+ PDU: createRule", NULL); 
    
    / * Step 0: Sanity check the pdu type to ensure it contains a rule * /
    if(cur_pdu->hdr.type != MSG_TYPE_CTRL_PERIOD_PROD)
    {
        DTNMP_DEBUG_ERR("x PDU: Cannot create rule from msg of type %d",
        		        cur_pdu->hdr.type);
        DTNMP_DEBUG_PROC("- PDU: createRule -> NULL", NULL);
        return NULL;
    }

    / * Step 1: Allocate the new rule. * /
    if((rule = (prod_rule*) STAKE(sizeof(prod_rule))) == NULL)
    {
        DTNMP_DEBUG_ERR("x PDU: Unable to allocate new rule.", NULL);
        DTNMP_DEBUG_PROC("- PDU: createRule -> NULL", NULL);
        return NULL;        
    }
    rule->mids = lyst_create();
    
    cursor = cur_pdu->content;    
    
    DTNMP_DEBUG_INFO("i  PDU: Cursor is %x", (unsigned long) cursor);
    
    
    / * Step 2: Extract the Offset for the Rule. * /
    if((sdnv_len = decodeSdnv(&sdnv_tmp, cursor)) == 0)
    {
        DTNMP_DEBUG_ERR("x PDU: No offset field in rule msg.", NULL);

        lyst_destroy(rule->mids);
        SRELEASE(rule);
        
        DTNMP_DEBUG_PROC("- PDU: createRule -> NULL", NULL);
        return NULL;
    }
        
    
    rule->offset = sdnv_tmp;
    cursor += sdnv_len;
   
    DTNMP_DEBUG_INFO("i  PDU: Rule has offset value of %d", rule->offset);
    
    / * Step 3: Extract the Period for the Rule. * /
    if((sdnv_len = decodeSdnv(&(sdnv_tmp), cursor)) == 0)
    {
        DTNMP_DEBUG_ERR("x PDU: No period field in rule msg.", NULL);

        lyst_destroy(rule->mids);
        SRELEASE(rule);
        
        DTNMP_DEBUG_PROC("- PDU: createRule -> NULL", NULL);
        return NULL;
    }
    
    //rule->interval_ticks = ntohl(sdnv_tmp);
    rule->interval_ticks = sdnv_tmp;
    rule->countdown_ticks = rule->interval_ticks;
    cursor += sdnv_len;
    DTNMP_DEBUG_INFO("i  PDU: Rule has interval value of %d", rule->interval_ticks);

    
    / * Step 4: Extract the # Evaluations for this Rule * /
    if((sdnv_len = decodeSdnv(&(sdnv_tmp), cursor)) == 0)
    {
        DTNMP_DEBUG_ERR("x PDU: No eval count field in rule msg.", NULL);
        
        lyst_destroy(rule->mids);
        SRELEASE(rule);
        
        DTNMP_DEBUG_PROC("- PDU: createRule -> NULL", NULL);
        return NULL;
    }
    
    //rule->num_evals = ntohl(sdnv_tmp);
    rule->num_evals = sdnv_tmp;
    if(rule->num_evals == 0)
    {
        rule->num_evals = DTNMP_RULE_EXEC_ALWAYS;
    }
    
    cursor += sdnv_len;
    DTNMP_DEBUG_INFO("i  PDU: Rule has num evals value of %d", rule->num_evals);

    
    / * Step 5: Grab the list of MIDs to be produced by this rule. * /
    mid_t *cur_mid = NULL;
    unsigned long mid_used = 0;

    DTNMP_DEBUG_INFO("cursor %x content %x data is %x.",
    		        (unsigned long)cursor, (unsigned long) cur_pdu->content,
    		        (unsigned long) cur_pdu->data_size);


    while(cursor < (cur_pdu->content + cur_pdu->data_size))
    {
        cur_mid = build_mid(cursor, (cur_pdu->content + cur_pdu->data_size) - cursor, &mid_used);

        
        if(cur_mid != NULL)
        {
        	char *mid_str = mid_string(cur_mid);
            lyst_insert_last(rule->mids, cur_mid);
            cursor += mid_used;
            DTNMP_DEBUG_INFO("i  PDU: Added MID %s to this RUle.", mid_str);
            SRELEASE(mid_str);

        }
        else
        {
        	DTNMP_DEBUG_ERR("x  PDU: Unknown MID.", NULL);
            lyst_destroy(rule->mids);
            SRELEASE(rule);
            return NULL;
        }
    }
    
    if(cursor != (cur_pdu->content + cur_pdu->data_size))
    {
        DTNMP_DEBUG_WARN("w MID: Unexpected size mismatch: cursor (%x) end data (%x)",
                         (unsigned long) cursor, (unsigned long) (cur_pdu->content + cur_pdu->data_size));        
    }
    
    strcpy(rule->sender.name, cur_pdu->meta.senderEid.name);
    DTNMP_DEBUG_INFO("i PDU: PDU sender EID is %s", rule->sender.name);    
    
    DTNMP_DEBUG_PROC("- PDU: createProdRule -> %x", (unsigned long) rule);
    
    return rule;
}


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

    /  * Step 3: Grab the list of MIDs to be produced by this rule. * /

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
***/
