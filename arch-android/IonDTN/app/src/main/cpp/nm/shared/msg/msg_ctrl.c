/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
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
 **  11/04/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  01/17/13  E. Birrane     Updated to use primitive types. (JHU/APL)
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "../utils/utils.h"
#include "../msg/msg_ctrl.h"


/* Create functions. */

/*
 * Can destroy mc after.
 */
msg_perf_ctrl_t *msg_create_perf_ctrl(time_t ts, Lyst mc)
{
	msg_perf_ctrl_t *result = NULL;

	if(mc == NULL)
	{
		return NULL;
	}

	result = (msg_perf_ctrl_t *) STAKE(sizeof(msg_perf_ctrl_t));
	if(result == NULL)
	{
		return NULL;
	}

	result->ts = ts;
	result->mc = midcol_copy(mc);

	return result;
}


void msg_destroy_perf_ctrl(msg_perf_ctrl_t *ctrl)
{
	if(ctrl == NULL)
	{
		return;
	}

	midcol_destroy(&(ctrl->mc));
	SRELEASE(ctrl);
}



uint8_t *msg_serialize_perf_ctrl(msg_perf_ctrl_t *ctrl, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time_sdnv;

	uint8_t *contents = NULL;
	uint32_t contents_len = 0;

	AMP_DEBUG_ENTRY("msg_serialize_perf_ctrl","(0x%x, 0x%x)",
			          (unsigned long)ctrl, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((ctrl == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("msg_serialize_perf_ctrl","Bad Args",NULL);
		AMP_DEBUG_EXIT("msg_serialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize contents individually. */
	encodeSdnv(&time_sdnv, ctrl->ts);

	if((contents = midcol_serialize(ctrl->mc, &contents_len)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_perf_ctrl","Can't serialize contents.",NULL);

		AMP_DEBUG_EXIT("msg_serialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Figure out the length. */
	*len = time_sdnv.length + contents_len;

	/* STEP 3: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("msg_serialize_perf_ctrl","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(contents);

		AMP_DEBUG_EXIT("msg_serialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Populate the serialized message. */
	cursor = result;

	memcpy(cursor,time_sdnv.text,time_sdnv.length);
	cursor += time_sdnv.length;

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	SRELEASE(contents);

	/* Step 5: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("msg_serialize_perf_ctrl","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("msg_serialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("msg_serialize_perf_ctrl","->0x%x",(unsigned long)result);
	return result;
}


/* Deserialize functions. */


msg_perf_ctrl_t *msg_deserialize_perf_ctrl(uint8_t *cursor,
		                                   uint32_t size,
		                                   uint32_t *bytes_used)
{
	msg_perf_ctrl_t *result = NULL;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("msg_deserialize_perf_ctrl","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			           size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("msg_deserialize_perf_ctrl","Bad Args.",NULL);
		AMP_DEBUG_EXIT("msg_deserialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (msg_perf_ctrl_t*)STAKE(sizeof(msg_perf_ctrl_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_perf_ctrl","Can't Alloc %d Bytes.",
				        sizeof(msg_perf_ctrl_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("msg_deserialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(msg_perf_ctrl_t));
	}


	/* Step 2: Deserialize the message. */
	uvast val;
    bytes = decodeSdnv(&val, cursor);
    result->ts = val;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

	/* Grab the list of contents. */
	if((result->mc = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("msg_deserialize_perf_ctrl","Can't grab contents.",NULL);

		*bytes_used = 0;
		msg_destroy_perf_ctrl(result);
		AMP_DEBUG_EXIT("msg_deserialize_perf_ctrl","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	AMP_DEBUG_EXIT("msg_deserialize_perf_ctrl","->0x%x",
			         (unsigned long)result);
	return result;
}



