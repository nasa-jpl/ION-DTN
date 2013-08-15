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
 ** \file report.c
 **
 **
 ** Description:
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/11/13  E. Birrane     Redesign of primitives architecture.
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t.
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"

#include "shared/msg/pdu.h"

#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_ctrl.h"

#include "report.h"



/* Create functions. */

/**
 * \brief Convenience function to build data report message.
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
 * \param[in[ contents The data IDs available.
 */
rpt_items_t *rpt_create_lst(Lyst contents)
{
	rpt_items_t *result = NULL;

	DTNMP_DEBUG_ENTRY("rpt_create_lst","(0x%x)", (unsigned long) contents);

	/* Step 0: Sanity Check. */
	if(contents == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_lst","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rpt_create_lst","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (rpt_items_t*)MTAKE(sizeof(rpt_items_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_lst","Can't alloc %d bytes.",
				        sizeof(rpt_items_t));
		DTNMP_DEBUG_EXIT("rpt_create_lst","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->contents = contents;

	DTNMP_DEBUG_EXIT("rpt_create_lst","->0x%x",result);
	return result;
}



/**
 * \brief Convenience function to build a data definition report.
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
 * \param[in[ defs  The data definitions.
 */
rpt_defs_t* rpt_create_defs(Lyst defs)
{
	rpt_defs_t *result = NULL;

	DTNMP_DEBUG_ENTRY("rpt_create_defs","(0x%x)", (unsigned long) defs);

	/* Step 0: Sanity Check. */
	if(defs == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_defs","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rpt_create_defs","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (rpt_defs_t*)MTAKE(sizeof(rpt_defs_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_defs","Can't alloc %d bytes.",
				        sizeof(rpt_defs_t));
		DTNMP_DEBUG_EXIT("rpt_create_defs","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->defs = defs;

	DTNMP_DEBUG_EXIT("rpt_create_defs","->0x%x",result);
	return result;
}



/**
 * \brief Convenience function to build a data report.
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
 * \param[in] time     The time when the report was created.
 * \param[in[ reports  The data reports.
 */
rpt_data_t *rpt_create_data(time_t time,
		                    Lyst reports,
		                    eid_t recipient)
{
	rpt_data_t *result = NULL;

	DTNMP_DEBUG_ENTRY("rpt_create_data","(%d,0x%x,%s)",
			          time, (unsigned long) reports, recipient.name);

	/* Step 0: Sanity Check. */
	if(reports == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_data","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rpt_create_data","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (rpt_data_t*) MTAKE(sizeof(rpt_data_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_data","Can't alloc %d bytes.",
				        sizeof(rpt_data_t));
		DTNMP_DEBUG_EXIT("rpt_create_data","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->time = time;
	result->reports = reports;
	result->recipient = recipient;
	result->size = 0;

	DTNMP_DEBUG_EXIT("rpt_create_data","->0x%x",result);
	return result;
}


/**
 * \brief Convenience function to build a data report.
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
 * \param[in[ defs  The data reports.
 */
rpt_prod_t *rpt_create_prod(Lyst defs)
{
	rpt_prod_t *result = NULL;

	DTNMP_DEBUG_ENTRY("rpt_create_prod","(0x%x)", (unsigned long) defs);

	/* Step 0: Sanity Check. */
	if(defs == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_prod","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rpt_create_prod","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (rpt_prod_t*)MTAKE(sizeof(rpt_prod_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_create_prod","Can't alloc %d bytes.",
				        sizeof(rpt_prod_t));
		DTNMP_DEBUG_EXIT("rpt_create_prod","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->defs = defs;

	DTNMP_DEBUG_EXIT("rpt_create_prod","->0x%x",result);
	return result;
}

/* Release functions.*/
void rpt_release_lst(rpt_items_t *msg)
{

	DTNMP_DEBUG_ENTRY("rpt_release_lst","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		if(msg->contents != NULL)
		{
			midcol_destroy(&(msg->contents));
		}
		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("rpt_release_lst","->.",NULL);
}

void rpt_release_defs(rpt_defs_t *msg)
{
	DTNMP_DEBUG_ENTRY("rpt_release_defs","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		if(msg->defs != NULL)
		{
			midcol_destroy(&(msg->defs));
		}

		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("rpt_release_defs","->.",NULL);
}

void rpt_release_data_entry(rpt_data_entry_t *entry)
{
	if(entry != NULL)
	{
		MRELEASE(entry->contents);
		mid_release(entry->id);
		MRELEASE(entry);
	}

}

void rpt_release_data(rpt_data_t *msg)
{
	DTNMP_DEBUG_ENTRY("rpt_release_data","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		LystElt elt;
		rpt_data_entry_t *item;

		for(elt = lyst_first(msg->reports); elt; elt = lyst_next(elt))
		{
			item = (rpt_data_entry_t *) lyst_data(elt);
			rpt_release_data_entry(item);

			/* \todo Double check we don't need to kill the ELT here versus as
			 * part of lyst_destroy.
			 */
		}

		lyst_destroy(msg->reports);

		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("rpt_release_data","->.",NULL);
}


void rpt_release_prod(rpt_prod_t *msg)
{

	DTNMP_DEBUG_ENTRY("rpt_release_prod","(0x%x)",
			          (unsigned long) msg);

	if(msg != NULL)
	{
		LystElt elt;
		rule_time_prod_t *item;

		for(elt = lyst_first(msg->defs); elt; elt = lyst_next(elt))
		{
			item = (rule_time_prod_t *) lyst_data(elt);

			rule_release_time_prod_entry(item);
			/* \todo Double check we don't need to kill the ELT here versus as
			 * part of lyst_destroy.
			 */
		}

		lyst_destroy(msg->defs);

		MRELEASE(msg);
	}

	DTNMP_DEBUG_EXIT("rpt_release_prod","->.",NULL);
}


void rpt_print_data_entry(rpt_data_entry_t *entry)
{
	char *id_str = NULL;

	if(entry == NULL)
	{
		fprintf(stderr,"NULL ENTRY.\n");
	}

	id_str = mid_pretty_print(entry->id);
	fprintf(stderr,"DATA ENTRY:\n%s\n", id_str);
	MRELEASE(id_str);

	fprintf(stderr,"SIZE: %d\n", (uint32_t)entry->size);
	utils_print_hex(entry->contents, entry->size);
}


/**
 * \brief Cleans up a lyst of data reports.
 *
 * \author Ed Birrane
 *
 * \param[in,out] reportLyst  THe lyst to be cleared.
 */
void rpt_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
    LystElt elt;
    rpt_data_t *cur_rpt = NULL;

    DTNMP_DEBUG_ENTRY("rpt_clear_lyst","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

    if((list == NULL) || (*list == NULL))
    {
    	DTNMP_DEBUG_ERR("rpt_clear_lyst","Bad Params.", NULL);
    	return;
    }

    if(mutex != NULL)
    {
    	lockResource(mutex);
    }

    /* Free any reports left in the reports list. */
    for (elt = lyst_first(*list); elt; elt = lyst_next(elt))
    {
        /* Grab the current report */
        if((cur_rpt = (rpt_data_t*) lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_WARN("rpt_clear_lyst","Can't get report from lyst!", NULL);
        }
        else
        {
        	rpt_release_data(cur_rpt);
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

    DTNMP_DEBUG_EXIT("rpt_clear_lyst","->.", NULL);
}
