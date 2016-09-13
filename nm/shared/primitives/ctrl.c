/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file ctrl.c
 **
 ** Description: This module contains the functions, structures, and other
 **              information used to capture both controls and macros.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  05/17/15  E. Birrane     Redesign around DTNMP v0.1 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "ctrl.h"

#include "../primitives/mid.h"

void ctrl_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
	LystElt elt;
	ctrl_exec_t *entry = NULL;

	AMP_DEBUG_ENTRY("ctrl_clear_lyst","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	AMP_DEBUG_ERR("ctrl_clear_lyst","Bad Params.", NULL);
    	return;
    }

	if(mutex != NULL)
	{
		lockResource(mutex);
	}

	/* Free any reports left in the reports list. */
	for (elt = lyst_first(*list); elt; elt = lyst_next(elt))
	{
		/* Grab the current item */
		if((entry = (ctrl_exec_t *) lyst_data(elt)) == NULL)
		{
			AMP_DEBUG_ERR("ctrl_clear_lyst","Can't get report from lyst!", NULL);
		}
		else
		{
			ctrl_release(entry);
		}
	}

	lyst_clear(*list);

	if(destroy != 0)
	{
		lyst_destroy(*list);
		*list = NULL;
	}

	if(mutex != NULL)
	{
		unlockResource(mutex);
	}

	AMP_DEBUG_EXIT("ctrl_clear_lyst","->.",NULL);
}


/******************************************************************************
 *
 * \par Function Name: ctrl_create
 *
 * \par Constructs a control instance given a MID and information about the
 *      execution of the control.
 *
 * \retval NULL - Error in creating the control.
 *        !NULL - The control instance
 *
 * \param[in] time      The timestamp to be associated with this control.
 * \param[in] id        The MID defining this control instance.
 * \param[in] sender    Control requester.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation. (JHU/APL)
 *  05/17/15  E. Birrane     Moved to ctrl.c, updated to DTNMP V0.1 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

ctrl_exec_t* ctrl_create(time_t time, mid_t *mid, eid_t sender)
{
	ctrl_exec_t *result = NULL;

	AMP_DEBUG_ENTRY("ctrl_create","(%d, 0x%x)",
			          time, (unsigned long) mid);

	/* Step 0: Sanity Check. */
	if(mid == NULL)
	{
		AMP_DEBUG_ERR("ctrl_create","Bad Args.",NULL);
		AMP_DEBUG_EXIT("ctrl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (ctrl_exec_t*)STAKE(sizeof(ctrl_exec_t))) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_create","Can't alloc %d bytes.",
				        sizeof(ctrl_exec_t));
		AMP_DEBUG_EXIT("ctrl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Find the adm_ctrl_t associated with this control. */
	if((result->adm_ctrl = adm_find_ctrl(mid)) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_create","Can't find ADM ctrl.",NULL);
		SRELEASE(result);
		AMP_DEBUG_EXIT("ctrl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Copy the sender information. */
	memcpy(&(result->sender), &sender, sizeof(eid_t));

	/* Step 4: Calculate time information for the control. */

	result->time = time;
	result->mid = mid_copy(mid);

    if(result->time <= AMP_RELATIVE_TIME_EPOCH)
    {
    	/* Step 4a: If relative time, that is # seconds. */
    	result->countdown_ticks = result->time;
    }
    else
    {
    	/*
    	 * Step 4b: If absolute time, # seconds if difference
    	 * from now until then.
    	 */
    	result->countdown_ticks = (result->time - getUTCTime());
    }

    /* Step 5: Populate dynamic parts of the control. */
	result->status = CONTROL_ACTIVE;

	AMP_DEBUG_EXIT("ctrl_create","->0x%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: ctrl_deserialize
 *
 * \par Construct a control instance from an incoming message byte stream.
 *
 * \retval NULL - Error in deserializing the control.
 *        !NULL - The control instance
 *
 * \param[in] cursor      Pointer to the start of the serialized control.
 * \param[in] size        The size of the remaining serialized data
 * \param[out] bytes_used The number of bytes consumed in processing this ctrl.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation. (JHU/APL)
 *  05/17/15  E. Birrane     Moved to ctrl.c, updated to DTNMP V0.1 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
ctrl_exec_t *ctrl_deserialize(uint8_t *cursor,
							  uint32_t size,
		                      uint32_t *bytes_used)
{
	ctrl_exec_t *result = NULL;
	uint32_t bytes = 0;
	uvast val = 0;

	AMP_DEBUG_ENTRY("ctrl_deserialize","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			           size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("ctrl_deserialize","Bad Args.",NULL);
		AMP_DEBUG_EXIT("ctrl_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (ctrl_exec_t*)STAKE(sizeof(ctrl_exec_t))) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_deserialize","Can't Alloc %d Bytes.",
				        sizeof(ctrl_exec_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("ctrl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(ctrl_exec_t));
	}

	bytes = decodeSdnv(&val, cursor);
	result->time = val;
	cursor += bytes;
	size -= bytes;
	*bytes_used += bytes;

	bytes = decodeSdnv(&val,cursor);
	result->status = val;
	cursor += bytes;
	size -= bytes;
	*bytes_used += bytes;

	memcpy(&(result->sender), cursor, sizeof(eid_t));
	cursor += sizeof(eid_t);
	size -= sizeof(eid_t);
	*bytes_used += sizeof(eid_t);

	if((result->mid = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_deserialize","Can't grab contents.",NULL);

		*bytes_used = 0;
		SRELEASE(result);
		AMP_DEBUG_EXIT("ctrl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	AMP_DEBUG_EXIT("ctrl_deserialize","->0x%x",
			         (unsigned long)result);
	return result;
}



void ctrl_release(ctrl_exec_t *ctrl)
{
	if(ctrl != NULL)
	{
		mid_release(ctrl->mid);
		SRELEASE(ctrl);
	}
}


uint8_t *ctrl_serialize(ctrl_exec_t *ctrl, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time_sdnv;
	Sdnv status_sdnv;

	uint8_t *contents = NULL;
	uint32_t contents_len = 0;

	AMP_DEBUG_ENTRY("ctrl_serialize","(0x%x, 0x%x)",
			          (unsigned long)ctrl, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((ctrl == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("ctrl_serialize","Bad Args",NULL);
		AMP_DEBUG_EXIT("ctrl_serialize","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize contents individually. */
	encodeSdnv(&time_sdnv, ctrl->time);

	encodeSdnv(&status_sdnv, ctrl->status);

	if((contents = mid_serialize(ctrl->mid, &contents_len)) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_serialize","Can't serialize MID.",NULL);

		AMP_DEBUG_EXIT("ctrl_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Figure out the length. */
	*len = time_sdnv.length + status_sdnv.length + sizeof(eid_t) + contents_len;

	/* STEP 3: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("ctrl_serialize","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(contents);

		AMP_DEBUG_EXIT("ctrl_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Populate the serialized message. */
	cursor = result;

	memcpy(cursor,time_sdnv.text,time_sdnv.length);
	cursor += time_sdnv.length;

	memcpy(cursor,status_sdnv.text,status_sdnv.length);
	cursor += status_sdnv.length;

	memcpy(cursor,&(ctrl->sender),sizeof(eid_t));
	cursor += sizeof(eid_t);

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	SRELEASE(contents);

	/* Step 5: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("ctrl_serialize","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("ctrl_serialize","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("ctrl_serialize","->0x%x",(unsigned long)result);
	return result;
}

