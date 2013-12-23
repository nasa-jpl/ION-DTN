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
 ** \file def.c
 **
 **
 ** Description: Structures that capture protocol custom definitions.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/17/13  E. Birrane     Redesign of messaging architecture.
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"

#include "def.h"


/**
 * \brief Convenience function to build custom definition messages.
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
 * \param[in] id       The ID of the new definition.
 * \param[in[ contents The new definition as an ordered set of MIDs.
 */
def_gen_t *def_create_gen(mid_t *id,
						  Lyst contents)
{
	def_gen_t *result = NULL;

	DTNMP_DEBUG_ENTRY("def_create_gen","(0x%x,0x%x)",
			          (unsigned long) id, (unsigned long) contents);

	/* Step 0: Sanity Check. */
	if((id == NULL) || (contents == NULL))
	{
		DTNMP_DEBUG_ERR("def_create_gen","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("def_create_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (def_gen_t*)MTAKE(sizeof(def_gen_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("def_create_gen","Can't alloc %d bytes.",
				        sizeof(def_gen_t));
		DTNMP_DEBUG_EXIT("def_create_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the message. */
	result->id = id;
	result->contents = contents;

	DTNMP_DEBUG_EXIT("def_create_gen","->0x%x",result);
	return result;
}

/* Release functions.*/

/**
 * \brief Release any definition-category message.
 *
 * \author Ed Birrane
 *
 * \param[in,out] def  The message being released.
 */
void def_release_gen(def_gen_t *def)
{
	DTNMP_DEBUG_ENTRY("def_release_gen","(0x%x)",
			          (unsigned long) def);

	if(def != NULL)
	{
		if(def->id != NULL)
		{
			mid_release(def->id);
		}
		if(def->contents != NULL)
		{
			midcol_destroy(&(def->contents));
		}

		MRELEASE(def);
	}

	DTNMP_DEBUG_EXIT("def_release_gen","->.",NULL);
}



/**
 * \brief Find custom definition by MID.
 *
 * \author Ed Birrane
 *
 * \return NULL - No definition found.
 *         !NULL - The found definition.
 *
 * \param[in] defs  The lyst of definitions.
 * \param[in] mutex Resource mutex protecting the defs list (or NULL)
 * \param[in] id    The ID of the report we are looking for.
 */
def_gen_t *def_find_by_id(Lyst defs, ResourceLock *mutex, mid_t *id)
{
	LystElt elt;
	def_gen_t *cur_def = NULL;

	DTNMP_DEBUG_ENTRY("def_find_by_id","(0x%x, 0x%x",
			          (unsigned long) defs, (unsigned long) id);

	/* Step 0: Sanity Check. */
	if((defs == NULL) || (id == NULL))
	{
		DTNMP_DEBUG_ERR("def_find_by_id","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("def_find_by_id","->NULL.", NULL);
		return NULL;
	}

	if(mutex != NULL)
	{
		lockResource(mutex);
	}
    for (elt = lyst_first(defs); elt; elt = lyst_next(elt))
    {
        /* Grab the definition */
        if((cur_def = (def_gen_t*) lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_ERR("def_find_by_id","Can't grab def from lyst!", NULL);
        }
        else
        {
        	if(mid_compare(id, cur_def->id,1) == 0)
        	{
        		DTNMP_DEBUG_EXIT("def_find_by_id","->0x%x.", cur_def);
        	    if(mutex != NULL)
        	    {
        	    	unlockResource(mutex);
        	    }
        		return cur_def;
        	}
        }
    }

    if(mutex != NULL)
    {
    	unlockResource(mutex);
    }

	DTNMP_DEBUG_EXIT("def_find_by_id","->NULL.", NULL);
	return NULL;
}


/**
 * \brief Clears list of definition messages.
 *
 * \author Ed Birrane
 *
 * \param[in,out] defs  The lyst of definition messages to be cleared
 */

void def_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy)
{
	 LystElt elt;
	 def_gen_t *cur_def = NULL;

	 DTNMP_DEBUG_ENTRY("def_lyst_clear","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

	 if((list == NULL) || (*list == NULL))
	 {
		 DTNMP_DEBUG_ERR("def_lyst_clear","Bad Params.", NULL);
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
		 if((cur_def = (def_gen_t *) lyst_data(elt)) == NULL)
		 {
			 DTNMP_DEBUG_ERR("clearDefsLyst","Can't get report from lyst!", NULL);
		 }
		 else
		 {
			 def_release_gen(cur_def);
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
	 DTNMP_DEBUG_EXIT("clearDefsLyst","->.",NULL);
}



void def_print_gen(def_gen_t *def)
{
	char *id_str;
	char *mc_str;
	if(def == NULL)
	{
		fprintf(stderr,"NULL Definition.\n");
	}

	id_str = mid_pretty_print(def->id);
	mc_str = midcol_pretty_print(def->contents);

	fprintf(stderr,"Definition:\n----------ID:\n%s\n\nMC:\n%s\n\n----------",
			id_str, mc_str);

	MRELEASE(id_str);
	MRELEASE(mc_str);
}



