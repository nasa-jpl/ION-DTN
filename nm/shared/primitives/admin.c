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
 ** \file admin.c
 **
 **
 ** Description: Administrative primitives.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/17/13  E. Birrane     Redesign of messaging architecture.
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t.
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"


#include "shared/primitives/mid.h"

#include "admin.h"




/* Create functions. */

/**
 * \brief Convenience function to build Register Agent message.
 *
 * \author Ed Birrane
 *
 * \note
 *   - We shallow copy information into the message. So, don't release anything
 *     provided as an argument to this function.
 *
 * \return NULL - Failure
 * 		   !NULL - Created message.
 *
 * \param[in] eid  The agent EID.
 */
adm_reg_agent_t *msg_create_reg_agent(eid_t eid)
{
	adm_reg_agent_t *result = NULL;

	DTNMP_DEBUG_ENTRY("msg_create_reg_agent","(%s)", eid.name);

	/* Step 1: Allocate the message. */
	if((result = (adm_reg_agent_t*) MTAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("msg_create_reg_agent","Can't alloc %d bytes.",
				        sizeof(adm_reg_agent_t));
		DTNMP_DEBUG_EXIT("msg_create_reg_agent","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->agent_id = eid;

	DTNMP_DEBUG_EXIT("msg_create_reg_agent","->0x%x",result);
	return result;
}



/**
 * \brief COnvenience function to build Report Policy message.
 *
 * \author Ed Birrane
 *
 * \note
 *   - We shallow copy information into the message. So, don't release anything
 *     provided as an argument to this function.
 *
 * \return NULL - Failure
 * 		   !NULL - Created message.
 *
 * \param[in] eid  The report policy mask.
 */
adm_rpt_policy_t *msg_create_rpt_policy(uint8_t mask)
{
	adm_rpt_policy_t *result = NULL;

	DTNMP_DEBUG_ENTRY("msg_create_rpt_policy","(0x%x)", mask);

	/* Step 1: Allocate the message. */
	if((result = (adm_rpt_policy_t*)MTAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("msg_create_rpt_policy","Can't alloc %d bytes.",
				        sizeof(adm_reg_agent_t));
		DTNMP_DEBUG_EXIT("msg_create_rpt_policy","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->mask = mask;

	DTNMP_DEBUG_EXIT("msg_create_rpt_policy","->0x%x",result);
	return result;
}




/**
 * \brief Convenience function to build a Status message.
 *
 * \author Ed Birrane
 *
 * \note
 *   - We shallow copy information into the message. So, don't release anything
 *     provided as an argument to this function.
 *
 * \return NULL - Failure
 * 		   !NULL - Created message.
 *
 * \param[in] code       The status, as a MID.
 * \param[in] time       When the status occured.
 * \param[in] generators The MIDs causing the status.
 */
adm_stat_msg_t *msg_create_stat_msg(mid_t *code,
									time_t time,
									Lyst generators)
{
	adm_stat_msg_t *result = NULL;

	DTNMP_DEBUG_ENTRY("msg_create_stat_msg","(code,0x%x,0x%x)",
			          time, (unsigned long) generators);

	/* Step 0: Sanity Check. */
	if((code == NULL) || (generators == NULL))
	{
		DTNMP_DEBUG_ERR("msg_create_stat_msg","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("msg_create_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (adm_stat_msg_t*)MTAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("msg_create_stat_msg","Can't alloc %d bytes.",
				        sizeof(adm_reg_agent_t));
		DTNMP_DEBUG_EXIT("msg_create_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->code = code;
	result->time = time;
	result->generators = generators;

	DTNMP_DEBUG_EXIT("msg_create_stat_msg","->0x%x",result);
	return result;
}


/* Release functions.*/
/**
 * \brief Release Register Agent message.
 *
 * \author Ed Birrane
 *
 * \param[in,out] msg  The message being released.
 */
void msg_release_reg_agent(adm_reg_agent_t *msg)
{
	DTNMP_DEBUG_ENTRY("msg_release_reg_agent","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("msg_release_reg_agent","->.",NULL);
}


/**
 * \brief Release Report Policy message.
 *
 * \author Ed Birrane
 *
 * \param[in,out] msg  The message being released.
 */
void msg_release_rpt_policy(adm_rpt_policy_t *msg)
{
	DTNMP_DEBUG_ENTRY("msg_release_rpt_policy","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("msg_release_rpt_policy","->.",NULL);
}



/**
 * \brief Release Status message.
 *
 * \author Ed Birrane
 *
 * \param[in,out] msg  The message being released.
 */
void msg_release_stat_msg(adm_stat_msg_t *msg)
{
	DTNMP_DEBUG_ENTRY("msg_release_stat_msg","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		mid_release(msg->code);
		midcol_destroy(&(msg->generators));
		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("msg_release_stat_msg","->.",NULL);
}

