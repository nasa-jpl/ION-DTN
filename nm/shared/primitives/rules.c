/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file rules.c
 **
 **
 ** Description:
 **
 **\todo Add more checks for overbounds reads on deserialization.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/04/12  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  07/05/16  E. Birrane     Fixed time offsets when creating TRL & SRL (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#include "platform.h"

#include "rules.h"

#include "../utils/utils.h"

#include "../msg/pdu.h"

#include "../msg/msg_ctrl.h"

#include "../primitives/mid.h"
#include "../primitives/rules.h"


/* 9/4/2015 */
srl_t*   srl_copy(srl_t *srl)
{
	srl_t *result = NULL;

	if(srl == NULL)
	{
		AMP_DEBUG_ERR("srl_copy","Bad Args.", NULL);
		return NULL;
	}

	if((result = (srl_t *) STAKE(sizeof(srl_t))) == NULL)
	{
		AMP_DEBUG_ERR("srl_copy","Can't Alloc %d bytes.", sizeof(srl_t));
		return NULL;
	}

	result->mid = mid_copy(srl->mid);
	result->time = srl->time;
	result->expr = expr_copy(srl->expr);
    result->count = srl->count;
    result->action = midcol_copy(srl->action);

    result->countdown_ticks = srl->countdown_ticks;
    result->desc = srl->desc;

	return result;
}

srl_t*   srl_create(mid_t *mid, time_t time, expr_t *expr, uvast count, Lyst action)
{
	srl_t *srl = NULL;

	AMP_DEBUG_ENTRY("srl_create",
			          "(0x" ADDR_FIELDSPEC ",0x" ADDR_FIELDSPEC ",0x" ADDR_FIELDSPEC ",0x" UHF ",0x" ADDR_FIELDSPEC ")",
		(uaddr) mid, (uaddr) time, (uaddr) expr, count, (uaddr) action);

	/* Step 0: Sanity Check. */
	if((mid == NULL) || (expr == NULL) || (action == NULL))
	{
		AMP_DEBUG_ERR("srl_create","Bad Args.",NULL);
		AMP_DEBUG_EXIT("srl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((srl = (srl_t*) STAKE(sizeof(srl_t))) == NULL)
	{
		AMP_DEBUG_ERR("srl_create","Can't alloc %d bytes.", sizeof(srl_t));
		AMP_DEBUG_EXIT("srl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	srl->mid = mid_copy(mid);
	srl->time = time;
	srl->expr = expr_copy(expr);
	srl->count = count;
	srl->action = midcol_copy(action);

	if((srl->mid == NULL) || (srl->expr == NULL) || (srl->action == NULL))
	{
		srl_release(srl);
		srl = NULL;
		AMP_DEBUG_ERR("srl_create","Can't alloc SRL.", NULL);
		AMP_DEBUG_EXIT("srl_create","->NULL",NULL);
		return NULL;
	}

	srl->desc.num_evals = srl->count;

	if(srl->time <= AMP_RELATIVE_TIME_EPOCH)
	{
		srl->countdown_ticks = srl->time;
	}
	else
	{
		srl->countdown_ticks = (srl->time - getUTCTime());
	}


	AMP_DEBUG_EXIT("srl_create", ADDR_FIELDSPEC, (uaddr) srl);

	return srl;
}


srl_t*   srl_deserialize(uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	srl_t *srl = NULL;
	uvast tmp = 0;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("srl_deserialize",ADDR_FIELDSPEC ", %d, " ADDR_FIELDSPEC ")", (uaddr)cursor, size, (uaddr) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("srl_deserialize","Bad Args.",NULL);
		AMP_DEBUG_EXIT("srl_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((srl = (srl_t*) STAKE(sizeof(srl_t))) == NULL)
	{
		AMP_DEBUG_ERR("srl_deserialize","Can't Alloc %d Bytes.", sizeof(srl_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("srl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(srl,0,sizeof(srl_t));
	}

	/* Step 2: Deserialize the SRL MID. */
	if((srl->mid = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("srl_deserialize","Can't grab MID.",NULL);

		*bytes_used = 0;
		srl_release(srl);
		AMP_DEBUG_EXIT("srl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Grab the time. */
	bytes = decodeSdnv(&tmp, cursor);
	srl->time = tmp;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    /* Step 4: Grab the expression. */
	if((srl->expr = expr_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("srl_deserialize","Can't grab expression.",NULL);

		*bytes_used = 0;
		srl_release(srl);
		AMP_DEBUG_EXIT("srl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 5: Grab the count. */
	bytes = decodeSdnv(&tmp, cursor);
	srl->count = tmp;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    /* Step 6: Grab the action macro. */
	if((srl->action = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("srl_deserialize","Can't grab action.",NULL);

		*bytes_used = 0;
		srl_release(srl);
		AMP_DEBUG_EXIT("srl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	AMP_DEBUG_EXIT("srl_deserialize","->" ADDR_FIELDSPEC, (uaddr) srl);

	return srl;
}


/*
 * NULL mutex OK.
 */
void srl_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy)
{
	LystElt elt;
	srl_t *entry = NULL;

	AMP_DEBUG_ENTRY("srl_lyst_clear",
			          "(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC ", %d)",
			          (uaddr) list, (uaddr) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	AMP_DEBUG_ERR("srl_lyst_clear","Bad Params.", NULL);
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
		if((entry = (srl_t *) lyst_data(elt)) == NULL)
		{
			AMP_DEBUG_ERR("srl_lyst_clear","Can't get SRL from lyst!", NULL);
		}
		else
		{
			srl_release(entry);
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

	AMP_DEBUG_EXIT("srl_lyst_clear","->.",NULL);

}



void     srl_release(srl_t *srl)
{
	if(srl == NULL)
	{
		return;
	}

	if(srl->mid != NULL)
	{
		mid_release(srl->mid);
	}

	if(srl->expr != NULL)
	{
		expr_release(srl->expr);
	}

	if(srl->action != NULL)
	{
		midcol_destroy(&(srl->action));
	}

	SRELEASE(srl);
}


uint8_t* srl_serialize(srl_t *srl, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time_sdnv;
	Sdnv count_sdnv;

	uint8_t *mid = NULL;
	uint32_t mid_len = 0;

	uint8_t *expr = NULL;
	uint32_t expr_len = 0;

	uint8_t *action = NULL;
	uint32_t action_len = 0;

	AMP_DEBUG_ENTRY("srl_serialize","(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC ")", (uaddr) srl, (uaddr) len);

	/* Step 0: Sanity Checks. */
	if((srl == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("srl_serialize","Bad Args",NULL);
		AMP_DEBUG_EXIT("srl_serialize","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize contents individually. */
	encodeSdnv(&time_sdnv, srl->time);
	encodeSdnv(&count_sdnv, srl->count);

	if((mid = mid_serialize(srl->mid, &mid_len)) == NULL)
	{
		AMP_DEBUG_ERR("srl_serialize","Can't serialize contents.",NULL);

		AMP_DEBUG_EXIT("srl_serialize","->NULL",NULL);
		return NULL;
	}

	if((expr = expr_serialize(srl->expr, &expr_len)) == NULL)
	{
		AMP_DEBUG_ERR("srl_serialize","Can't serialize contents.",NULL);

		SRELEASE(mid);
		AMP_DEBUG_EXIT("srl_serialize","->NULL",NULL);
		return NULL;
	}

	if((action = midcol_serialize(srl->action, &action_len)) == NULL)
	{
		AMP_DEBUG_ERR("srl_serialize","Can't serialize contents.",NULL);

		SRELEASE(mid);
		SRELEASE(expr);

		AMP_DEBUG_EXIT("srl_serialize","->NULL",NULL);
		return NULL;
	}


	/* Step 2: Figure out the length. */
	*len = mid_len + time_sdnv.length + count_sdnv.length + expr_len + action_len;

	/* STEP 3: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("srl_serialize","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(mid);
		SRELEASE(expr);
		SRELEASE(action);

		AMP_DEBUG_EXIT("srl_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, mid, mid_len);
	cursor += mid_len;

	memcpy(cursor,time_sdnv.text,time_sdnv.length);
	cursor += time_sdnv.length;

	memcpy(cursor,expr,expr_len);
	cursor += expr_len;

	memcpy(cursor,count_sdnv.text,count_sdnv.length);
	cursor += count_sdnv.length;

	memcpy(cursor, action, action_len);
	cursor += action_len;

	SRELEASE(mid);
	SRELEASE(expr);
	SRELEASE(action);

	/* Step 5: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("srl_serialize","Wrote %d bytes but allocated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("srl_serialize","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("srl_serialize", "->" ADDR_FIELDSPEC, (uaddr)result);

	return result;
}



trl_t*   trl_create(mid_t *mid, time_t time, uvast period, uvast count, Lyst action)
{
	trl_t *trl = NULL;

	AMP_DEBUG_ENTRY("trl_create",
			          "(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC "," UVAST_FIELDSPEC "," UVAST_FIELDSPEC "," ADDR_FIELDSPEC ")",
		(uaddr) mid, (uaddr) time, period, count, (uaddr) action);

	/* Step 0: Sanity Check. */
	if((mid == NULL) || (action == NULL))
	{
		AMP_DEBUG_ERR("trl_create","Bad Args.",NULL);
		AMP_DEBUG_EXIT("trl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((trl = (trl_t*) STAKE(sizeof(trl_t))) == NULL)
	{
		AMP_DEBUG_ERR("trl_create","Can't alloc %d bytes.", sizeof(trl_t));
		AMP_DEBUG_EXIT("trl_create","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	trl->mid = mid_copy(mid);
	trl->time = time;
	trl->period = period;
	trl->count = count;
	trl->action = midcol_copy(action);

	if((trl->mid == NULL) || (trl->action == NULL))
	{
		trl_release(trl);
		trl = NULL;
		AMP_DEBUG_ERR("trl_create","Can't alloc SRL.", NULL);
		AMP_DEBUG_EXIT("trl_create","->NULL",NULL);
		return NULL;
	}

	trl->desc.num_evals = trl->count;
	trl->desc.interval_ticks = trl->period;

	if(trl->time <= AMP_RELATIVE_TIME_EPOCH)
	{
		trl->countdown_ticks = trl->time;
	}
	else
	{
		trl->countdown_ticks = (trl->time - getUTCTime());
	}


	AMP_DEBUG_EXIT("trl_create", ADDR_FIELDSPEC, (uaddr) trl);

	return trl;
}


trl_t*   trl_deserialize(uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	trl_t *trl = NULL;
	uvast tmp = 0;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("trl_deserialize", ADDR_FIELDSPEC ", %d, " ADDR_FIELDSPEC ")", (uaddr) cursor, size, (uaddr) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("trl_deserialize","Bad Args.",NULL);
		AMP_DEBUG_EXIT("trl_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((trl = (trl_t*) STAKE(sizeof(trl_t))) == NULL)
	{
		AMP_DEBUG_ERR("trl_deserialize","Can't Alloc %d Bytes.", sizeof(trl_t));
		*bytes_used = 0;
		AMP_DEBUG_EXIT("trl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(trl,0,sizeof(trl_t));
	}

	/* Step 2: Deserialize the TRL MID. */
	if((trl->mid = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("trl_deserialize","Can't grab MID.",NULL);

		*bytes_used = 0;
		trl_release(trl);
		AMP_DEBUG_EXIT("trl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Grab the time. */
	bytes = decodeSdnv(&tmp, cursor);
	trl->time = tmp;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    /* Step 4: Grab the period. */
	bytes = decodeSdnv(&tmp, cursor);
	trl->period = tmp;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    /* Step 5: Grab the count. */
	bytes = decodeSdnv(&tmp, cursor);
	trl->count = tmp;
    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

    /* Step 6: Grab the action macro. */
	if((trl->action = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("trl_deserialize","Can't grab action.",NULL);

		*bytes_used = 0;
		trl_release(trl);
		AMP_DEBUG_EXIT("trl_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	AMP_DEBUG_EXIT("trl_deserialize", "->" ADDR_FIELDSPEC, (uaddr) trl);

	return trl;
}


/*
 * NULL mutex OK.
 */
void trl_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy)
{
	LystElt elt;
	trl_t *entry = NULL;

	AMP_DEBUG_ENTRY("trl_lyst_clear",
			          "(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC ", %d)",
			          (uaddr) list, (uaddr) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	AMP_DEBUG_ERR("trl_lyst_clear","Bad Params.", NULL);
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
		if((entry = (trl_t *) lyst_data(elt)) == NULL)
		{
			AMP_DEBUG_ERR("trl_lyst_clear","Can't get TRL from lyst!", NULL);
		}
		else
		{
			trl_release(entry);
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

	AMP_DEBUG_EXIT("trl_lyst_clear","->.",NULL);

}



void     trl_release(trl_t *trl)
{
	if(trl == NULL)
	{
		return;
	}

	if(trl->mid != NULL)
	{
		mid_release(trl->mid);
		trl->mid = NULL;
	}

	if(trl->action != NULL)
	{
		midcol_destroy(&(trl->action));
		trl->action = NULL;
	}

	SRELEASE(trl);
}


uint8_t* trl_serialize(trl_t *trl, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time_sdnv;
	Sdnv period_sdnv;
	Sdnv count_sdnv;

	uint8_t *mid = NULL;
	uint32_t mid_len = 0;

	uint8_t *action = NULL;
	uint32_t action_len = 0;

	AMP_DEBUG_ENTRY("trl_serialize", "(" ADDR_FIELDSPEC "," ADDR_FIELDSPEC ")", (uaddr) trl, (uaddr) len);

	/* Step 0: Sanity Checks. */
	if((trl == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("trl_serialize","Bad Args",NULL);
		AMP_DEBUG_EXIT("trl_serialize","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize contents individually. */
	encodeSdnv(&time_sdnv,   trl->time);
	encodeSdnv(&period_sdnv, trl->period);
	encodeSdnv(&count_sdnv,  trl->count);

	if((mid = mid_serialize(trl->mid, &mid_len)) == NULL)
	{
		AMP_DEBUG_ERR("trl_serialize","Can't serialize contents.",NULL);

		AMP_DEBUG_EXIT("trl_serialize","->NULL",NULL);
		return NULL;
	}

	if((action = midcol_serialize(trl->action, &action_len)) == NULL)
	{
		AMP_DEBUG_ERR("trl_serialize","Can't serialize contents.",NULL);

		SRELEASE(mid);

		AMP_DEBUG_EXIT("trl_serialize","->NULL",NULL);
		return NULL;
	}


	/* Step 2: Figure out the length. */
	*len = mid_len + time_sdnv.length + period_sdnv.length + count_sdnv.length + action_len;

	/* STEP 3: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("trl_serialize","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(mid);
		SRELEASE(action);

		AMP_DEBUG_EXIT("trl_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, mid, mid_len);
	cursor += mid_len;

	memcpy(cursor,time_sdnv.text,time_sdnv.length);
	cursor += time_sdnv.length;

	memcpy(cursor,period_sdnv.text,period_sdnv.length);
	cursor += period_sdnv.length;

	memcpy(cursor,count_sdnv.text,count_sdnv.length);
	cursor += count_sdnv.length;

	memcpy(cursor, action, action_len);
	cursor += action_len;

	SRELEASE(mid);
	SRELEASE(action);

	/* Step 5: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("trl_serialize","Wrote %d bytes but allocated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("trl_serialize","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("trl_serialize","->" ADDR_FIELDSPEC, (uaddr)result);

	return result;
}
