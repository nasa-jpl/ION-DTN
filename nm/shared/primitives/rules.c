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
 *****************************************************************************/
#include "platform.h"

#include "shared/utils/utils.h"

#include "shared/msg/pdu.h"

#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_ctrl.h"

#include "shared/primitives/mid.h"
#include "shared/primitives/rules.h"




/* Create functions. */
rule_time_prod_t *rule_create_time_prod_entry(uint32_t time,
											  uint64_t count,
											  uint64_t period,
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


rule_pred_prod_t *rule_create_pred_prod_entry(uint32_t time,
		   	   	   	   	   	   	   	   	      Lyst predicate,
		   	   	   	   	   	   	   	   	      uint64_t count,
		   	   	   	   	   	   	   	   	      Lyst contents)
{
	/* \todo Implement this. */
	return NULL;
}

ctrl_exec_t *ctrl_create_exec(uint32_t time, Lyst contents)
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



