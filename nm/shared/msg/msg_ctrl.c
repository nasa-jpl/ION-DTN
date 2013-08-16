/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file msg_ctrl.c
 **
 **
 ** Description: Defines the serialization and de-serialization methods
 **              used to translate data types into byte streams associated
 **              with DTNMP messages, and vice versa.
 **
 ** Notes:
 **				 Not all fields of internal structures are serialized into/
 **				 deserialized from DTNMP messages.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/04/12  E. Birrane     Redesign of messaging architecture.
 **  01/17/13  E. Birrane     Updated to use primitive types.
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "shared/utils/utils.h"
#include "shared/msg/msg_ctrl.h"


/* Create functions. */



/* Serialize functions. */
uint8_t *ctrl_serialize_time_prod_entry(rule_time_prod_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time_sdnv;
	Sdnv period_sdnv;
	Sdnv count_sdnv;

	uint8_t *contents = NULL;
	uint32_t contents_len = 0;

	DTNMP_DEBUG_ENTRY("ctrl_serialize_time_prod_entry","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_time_prod_entry","Bad Args",NULL);
		DTNMP_DEBUG_EXIT("ctrl_serialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize contents individually. */
	encodeSdnv(&time_sdnv, msg->time);
	encodeSdnv(&period_sdnv, msg->period);
	encodeSdnv(&count_sdnv, msg->count);

	if((contents = midcol_serialize(msg->mids, &contents_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_time_prod_entry","Can't serialize contents.",NULL);

		DTNMP_DEBUG_EXIT("ctrl_serialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Figure out the length. */
	*len = time_sdnv.length + period_sdnv.length + count_sdnv.length + contents_len;

	/* STEP 3: Allocate the serialized message. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_time_prod_entry","Can't alloc %d bytes", *len);
		*len = 0;
		MRELEASE(contents);

		DTNMP_DEBUG_EXIT("ctrl_serialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Populate the serialized message. */
	cursor = result;

	memcpy(cursor,time_sdnv.text,time_sdnv.length);
	cursor += time_sdnv.length;

	memcpy(cursor,period_sdnv.text,period_sdnv.length);
	cursor += period_sdnv.length;

	memcpy(cursor,count_sdnv.text,count_sdnv.length);
	cursor += count_sdnv.length;

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	MRELEASE(contents);

	/* Step 5: Last sanity check. */
	if((cursor - result) != *len)
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_time_prod_entry","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("ctrl_serialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("ctrl_serialize_time_prod_entry","->0x%x",(unsigned long)result);
	return result;
}

uint8_t *ctrl_serialize_pred_prod_entry(rule_pred_prod_t *msg, uint32_t *len)
{
	return NULL;
}

uint8_t *ctrl_serialize_exec(ctrl_exec_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time_sdnv;

	uint8_t *contents = NULL;
	uint32_t contents_len = 0;

	DTNMP_DEBUG_ENTRY("ctrl_serialize_exec","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_exec","Bad Args",NULL);
		DTNMP_DEBUG_EXIT("ctrl_serialize_exec","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize contents individually. */
	encodeSdnv(&time_sdnv, msg->time);

	if((contents = midcol_serialize(msg->contents, &contents_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_exec","Can't serialize contents.",NULL);

		DTNMP_DEBUG_EXIT("ctrl_serialize_exec","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Figure out the length. */
	*len = time_sdnv.length + contents_len;

	/* STEP 3: Allocate the serialized message. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_exec","Can't alloc %d bytes", *len);
		*len = 0;
		MRELEASE(contents);

		DTNMP_DEBUG_EXIT("ctrl_serialize_exec","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Populate the serialized message. */
	cursor = result;

	memcpy(cursor,time_sdnv.text,time_sdnv.length);
	cursor += time_sdnv.length;

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	MRELEASE(contents);

	/* Step 5: Last sanity check. */
	if((cursor - result) != *len)
	{
		DTNMP_DEBUG_ERR("ctrl_serialize_exec","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("ctrl_serialize_exec","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("ctrl_serialize_exec","->0x%x",(unsigned long)result);
	return result;
}


/* Deserialize functions. */
rule_time_prod_t *ctrl_deserialize_time_prod_entry(uint8_t *cursor,
		                       	   	   	   	   	   uint32_t size,
		                       	   	   	   	   	   uint32_t *bytes_used)
{
	rule_time_prod_t *result = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("ctrl_deserialize_time_prod_entry","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			           size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		DTNMP_DEBUG_ERR("ctrl_deserialize_time_prod_entry","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("ctrl_deserialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (rule_time_prod_t*)MTAKE(sizeof(rule_time_prod_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_deserialize_time_prod_entry","Can't Alloc %d Bytes.",
				        sizeof(rule_time_prod_t));
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("ctrl_deserialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(rule_time_prod_t));
	}


	/* Step 2: Deserialize the message. */
	uvast val;
    bytes = decodeSdnv(&val, cursor);
    result->time = val;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    bytes = decodeSdnv(&val,cursor);
    result->period = val;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    bytes = decodeSdnv(&val,cursor);
    result->count = val;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

	/* Grab the list of contents. */
	if((result->mids = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_deserialize_time_prod_entry","Can't grab contents.",NULL);

		*bytes_used = 0;
		rule_release_time_prod_entry(result);
		DTNMP_DEBUG_EXIT("ctrl_deserialize_time_prod_entry","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	DTNMP_DEBUG_EXIT("ctrl_deserialize_time_prod_entry","->0x%x",
			         (unsigned long)result);
	return result;
}

rule_pred_prod_t *ctrl_deserialize_pred_prod_entry(uint8_t *cursor,
		                       	   	   	   	       uint32_t size,
		                       	   	   	   	       uint32_t *bytes_used)
{
	*bytes_used = 0;
	return NULL;
}

ctrl_exec_t *ctrl_deserialize_exec(uint8_t *cursor,
								   uint32_t size,
		                       	   uint32_t *bytes_used)
{
	ctrl_exec_t *result = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("ctrl_deserialize_exec","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			           size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		DTNMP_DEBUG_ERR("ctrl_deserialize_exec","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("ctrl_deserialize_exec","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (ctrl_exec_t*)MTAKE(sizeof(ctrl_exec_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_deserialize_exec","Can't Alloc %d Bytes.",
				        sizeof(ctrl_exec_t));
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("ctrl_deserialize_exec","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(ctrl_exec_t));
	}


	/* Step 2: Deserialize the message. */
	uvast val;
    bytes = decodeSdnv(&val, cursor);
    result->time = val;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

	/* Grab the list of contents. */
	if((result->contents = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_deserialize_exec","Can't grab contents.",NULL);

		*bytes_used = 0;
		ctrl_release_exec(result);
		DTNMP_DEBUG_EXIT("ctrl_deserialize_exec","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	DTNMP_DEBUG_EXIT("ctrl_deserialize_exec","->0x%x",
			         (unsigned long)result);
	return result;}


