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
 ** \file msg_reports.c
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
 **  11/04/12  E. Birrane     Redesign of messaging architecture.
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t.
 *****************************************************************************/
#include "platform.h"

#include "shared/utils/utils.h"

#include "shared/msg/pdu.h"

#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_ctrl.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/rules.h"




/* Create functions. */
rule_time_prod_t *rule_create_time_prod_entry(time_t time,
											  uvast count,
											  uvast period,
											  Lyst contents)
{
	rule_time_prod_t *result = NULL;

	DTNMP_DEBUG_ENTRY("rule_create_time_prod_entry","(%d, %d, %d, 0x%x)",
			          time, count, period, (unsigned long) contents);

	/* Step 0: Sanity Check. */
	if(contents == NULL)
	{
		DTNMP_DEBUG_ERR("rule_create_time_prod_entry","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rule_create_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (rule_time_prod_t*)MTAKE(sizeof(rule_time_prod_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rule_create_time_prod_entry","Can't alloc %d bytes.",
				        sizeof(rule_time_prod_t));
		DTNMP_DEBUG_EXIT("rule_create_time_prod_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->time = time;
	result->period = period;
	result->count = count;
	result->mids = contents;

	DTNMP_DEBUG_EXIT("rule_create_time_prod_entry","->0x%x",result);
	return result;
}


rule_pred_prod_t *rule_create_pred_prod_entry(time_t time,
		   	   	   	   	   	   	   	   	      Lyst predicate,
		   	   	   	   	   	   	   	   	      uvast count,
		   	   	   	   	   	   	   	   	      Lyst contents)
{
	/* \todo Implement this. */
	return NULL;
}

ctrl_exec_t *ctrl_create_exec(time_t time, Lyst contents)
{
	ctrl_exec_t *result = NULL;

	DTNMP_DEBUG_ENTRY("ctrl_create_exec","(%d, 0x%x)",
			          time, (unsigned long) contents);

	/* Step 0: Sanity Check. */
	if(contents == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_create_exec","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("ctrl_create_exec","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (ctrl_exec_t*)MTAKE(sizeof(ctrl_exec_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("ctrl_create_exec","Can't alloc %d bytes.",
				        sizeof(ctrl_exec_t));
		DTNMP_DEBUG_EXIT("ctrl_create_exec","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->time = time;
	result->contents = contents;

	DTNMP_DEBUG_EXIT("ctrl_create_exec","->0x%x",result);
	return result;
}





/* Release functions.*/
void rule_release_time_prod_entry(rule_time_prod_t *msg)
{
	if(msg != NULL)
	{
		midcol_destroy(&(msg->mids));
		MRELEASE(msg);
	}
}

void rule_release_pred_prod_entry(rule_pred_prod_t *msg)
{
	/* \todo Implement this. */
	return;
}

void ctrl_release_exec(ctrl_exec_t *msg)
{
	if(msg != NULL)
	{
		midcol_destroy(&(msg->contents));
		MRELEASE(msg);
	}
}



void rule_time_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
	LystElt elt;
	rule_time_prod_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("rule_time_clear_lyst","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	DTNMP_DEBUG_ERR("rule_time_clear_lyst","Bad Params.", NULL);
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
		if((entry = (rule_time_prod_t *) lyst_data(elt)) == NULL)
		{
			DTNMP_DEBUG_ERR("rule_time_clear_lyst","Can't get report from lyst!", NULL);
		}
		else
		{
			rule_release_time_prod_entry(entry);
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

	DTNMP_DEBUG_EXIT("rule_time_clear_lyst","->.",NULL);
}

void rule_pred_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
	LystElt elt;
	rule_pred_prod_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("rule_pred_clear_lyst","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	DTNMP_DEBUG_ERR("rule_pred_clear_lyst","Bad Params.", NULL);
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
		if((entry = (rule_pred_prod_t *) lyst_data(elt)) == NULL)
		{
			DTNMP_DEBUG_ERR("rule_pred_clear_lyst","Can't get report from lyst!", NULL);
		}
		else
		{
			rule_release_pred_prod_entry(entry);
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

	DTNMP_DEBUG_EXIT("rule_pred_clear_lyst","->.",NULL);
}

void ctrl_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
	LystElt elt;
	ctrl_exec_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("ctrl_clear_lyst","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	DTNMP_DEBUG_ERR("ctrl_clear_lyst","Bad Params.", NULL);
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
			DTNMP_DEBUG_ERR("ctrl_clear_lyst","Can't get report from lyst!", NULL);
		}
		else
		{
			ctrl_release_exec(entry);
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

	DTNMP_DEBUG_EXIT("ctrl_clear_lyst","->.",NULL);
}

