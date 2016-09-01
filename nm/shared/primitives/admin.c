/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **  01/17/13  E. Birrane     Redesign of messaging architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  06/30/16  E. Birrane     Update to AMP v0.3 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"
#include "../utils/utils.h"
#include "../primitives/mid.h"

#include "admin.h"


/* Create functions. */

/******************************************************************************
 *
 * \par Function Name: msg_create_reg_agent
 *
 * \par Convenience function to build Register Agent message.
 *
 * \retval NULL - Failure
 * 		   !NULL - Created message.
 *
 * \param[in]  eid   THe identifier of the Agent
 *
 * \par Notes:
 *		1. - We shallow copy information into the message. Do not release
 *		     anything provided as an argument to this function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/17/13  E. Birrane     Initial implementation (JHU/APL)
 *  06/30/16  E. Birrane     Doc Update. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

adm_reg_agent_t *msg_create_reg_agent(eid_t eid)
{
	adm_reg_agent_t *result = NULL;

	AMP_DEBUG_ENTRY("msg_create_reg_agent","(%s)", eid.name);

	/* Step 1: Allocate the message. */
	if((result = (adm_reg_agent_t*) STAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_create_reg_agent","Can't alloc %d bytes.",
				        sizeof(adm_reg_agent_t));
		AMP_DEBUG_EXIT("msg_create_reg_agent","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->agent_id = eid;

	AMP_DEBUG_EXIT("msg_create_reg_agent","->0x%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: msg_create_rpt_policy
 *
 * \par Convenience function to build Report Policy message.
 *
 * \retval NULL - Failure
 * 		   !NULL - Created message.
 *
 * \param[in]  mask   The report policy mask
 *
 * \par Notes:
 *		1. - We shallow copy information into the message. Do not release
 *		     anything provided as an argument to this function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/17/13  E. Birrane     Initial implementation (JHU/APL)
 *  06/30/16  E. Birrane     Doc Update. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

adm_rpt_policy_t *msg_create_rpt_policy(uint8_t mask)
{
	adm_rpt_policy_t *result = NULL;

	AMP_DEBUG_ENTRY("msg_create_rpt_policy","(0x%x)", mask);

	/* Step 1: Allocate the message. */
	if((result = (adm_rpt_policy_t*)STAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_create_rpt_policy","Can't alloc %d bytes.",
				        sizeof(adm_reg_agent_t));
		AMP_DEBUG_EXIT("msg_create_rpt_policy","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->mask = mask;

	AMP_DEBUG_EXIT("msg_create_rpt_policy","->0x%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: msg_create_stat_msg
 *
 * \par Convenience function to build a Status message.
 *
 * \retval NULL - Failure
 * 		   !NULL - Created message.
 *
 * \param[in] code       The status, as a MID.
 * \param[in] time       When the status occured.
 * \param[in] generators The MIDs causing the status.
 *
 * \par Notes:
 *		1. - We shallow copy information into the message. Do not release
 *		     anything provided as an argument to this function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/17/13  E. Birrane     Initial implementation (JHU/APL)
 *  06/30/16  E. Birrane     Doc Update. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

adm_stat_msg_t *msg_create_stat_msg(mid_t *code,
									time_t time,
									Lyst generators)
{
	adm_stat_msg_t *result = NULL;

	AMP_DEBUG_ENTRY("msg_create_stat_msg","(code,0x%x,0x%x)",
			          time, (unsigned long) generators);

	/* Step 0: Sanity Check. */
	if((code == NULL) || (generators == NULL))
	{
		AMP_DEBUG_ERR("msg_create_stat_msg","Bad Args.",NULL);
		AMP_DEBUG_EXIT("msg_create_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (adm_stat_msg_t*)STAKE(sizeof(adm_reg_agent_t))) == NULL)
	{
		AMP_DEBUG_ERR("msg_create_stat_msg","Can't alloc %d bytes.",
				        sizeof(adm_reg_agent_t));
		AMP_DEBUG_EXIT("msg_create_stat_msg","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->code = code;
	result->time = time;
	result->generators = generators;

	AMP_DEBUG_EXIT("msg_create_stat_msg","->0x%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: msg_release_reg_agent
 *
 * \par Release Register Agent message.
 *
 * \param[in,out] msg  The message being released.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/17/13  E. Birrane     Initial implementation (JHU/APL)
 *  06/30/16  E. Birrane     Doc Update. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void msg_release_reg_agent(adm_reg_agent_t *msg)
{
	AMP_DEBUG_ENTRY("msg_release_reg_agent","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		SRELEASE(msg);
	}

	AMP_DEBUG_EXIT("msg_release_reg_agent","->.",NULL);
}



/******************************************************************************
 *
 * \par Function Name: msg_release_rpt_policy
 *
 * \par Release Report Policy message.
 *
 * \param[in,out] msg  The message being released.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/17/13  E. Birrane     Initial implementation (JHU/APL)
 *  06/30/16  E. Birrane     Doc Update. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void msg_release_rpt_policy(adm_rpt_policy_t *msg)
{
	AMP_DEBUG_ENTRY("msg_release_rpt_policy","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		SRELEASE(msg);
	}

	AMP_DEBUG_EXIT("msg_release_rpt_policy","->.",NULL);
}



/******************************************************************************
 *
 * \par Function Name: msg_release_stat_msg
 *
 * \par Release Status message.
 *
 * \param[in,out] msg  The message being released.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/17/13  E. Birrane     Initial implementation (JHU/APL)
 *  06/30/16  E. Birrane     Doc Update. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void msg_release_stat_msg(adm_stat_msg_t *msg)
{
	AMP_DEBUG_ENTRY("msg_release_stat_msg","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		mid_release(msg->code);
		midcol_destroy(&(msg->generators));
		SRELEASE(msg);
	}

	AMP_DEBUG_EXIT("msg_release_stat_msg","->.",NULL);
}

