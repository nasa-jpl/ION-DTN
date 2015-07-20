
/*****************************************************************************
 **
 ** File Name: lit.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of literal definitions.
 **
 ** Notes:
 **
 **  (1) Parameterized literals are stored in a parameterless state as they are
 **       defined as taking parameters, but the definition of course) does not
 **       include any given set or instance of parameters.
 **
 **
 ** Assumptions:
 **
 **
 *****************************************************************************/

#include "ion.h"
#include "platform.h"

#include "shared/primitives/lit.h"


/******************************************************************************
 *
 * \par Function Name: lit_create
 *
 * \par Builds a literal structure from user parameters.
 *
 * \retval NULL Failure
 *        !NULL The created literal structure.
 *
 * \param[in] mid   The ID of the new literal.
 * \param[in] value Value of a non-parameterized literal.
 * \param[in] calc  Function calculating value of parameterized literal.
 *
 * \par Notes:
 *		1. The returned literal structure is allocated and must be
 *		   released by the caller.
 *****************************************************************************/

lit_t *lit_create(mid_t *id, value_t value, lit_val_fn calc)
{
	lit_t *result = NULL;

	DTNMP_DEBUG_ENTRY("lit_create","(0x%x,(%d, %d, %d), 0x%x)",
			          (unsigned long) id, value.type, value.value.as_int, value.length,
			          (unsigned long) calc);

	/* Step 0: Sanity Check. */
	if((id == NULL) || (id->oid == NULL))
	{
		DTNMP_DEBUG_ERR("lit_create","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("lit_create","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Sanity check. */
	if((MID_GET_FLAG_TYPE(id->flags) != MID_TYPE_LITERAL) ||
		(MID_GET_FLAG_CAT(id->flags) != MID_CAT_ATOMIC))
	{
		DTNMP_DEBUG_ERR("lit_create","Malformed MID for literal. ",NULL);
		DTNMP_DEBUG_EXIT("lit_create","->NULL",NULL);
		return NULL;
	}

	if((id->oid->type == OID_TYPE_PARAM) ||
		(id->oid->type == OID_TYPE_COMP_PARAM))
	{
		/* Step 1.1: If this is parameterized, we need a calculate function. */
		if(calc == NULL)
		{
			DTNMP_DEBUG_ERR("lit_create","Parameterized literal needs calc function.",NULL);
			DTNMP_DEBUG_EXIT("lit_create","->NULL",NULL);
			return NULL;
		}
	}
	else
	{
		/* Step 1.1: If this is not parameterized, we need a valid value. */
		if(value.type == DTNMP_TYPE_UNK)
		{
			DTNMP_DEBUG_ERR("lit_create","Unparameterized literal needs valid value.",NULL);
			DTNMP_DEBUG_EXIT("lit_create","->NULL",NULL);
			return NULL;
		}
	}

	/* Step 2: Allocate the message. */
	if((result = (lit_t*)MTAKE(sizeof(lit_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("lit_create","Can't alloc %d bytes.",
				        sizeof(lit_t));
		DTNMP_DEBUG_EXIT("lit_create","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Populate the literal. */
	result->id = id;
	result->value = value;
	result->calc = calc;

	DTNMP_DEBUG_EXIT("lit_create","->0x%x",result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: lit_find_by_id
 *
 * \par Finds a literal definition from a lyst of literal definitions.
 *
 * \retval NULL - The definition was not found.
 *        !NULL - The definition was found and returned.
 *
 * \param[in] list   The list of literal structures.
 * \param[in] mutex  Mutex resource protecting the list of literal structures.
 * \param[in] id     The literal identifier being searched for.
 *
 * \par Notes:
 *		1. If the given search MID is parameterized, then provided parameters
 *		   will not be matched, just the non-parameter portion of the MID.
 *
 *		2. The returned literal structure is a pointer into the given list and
 *		   MUST NOT be released by the calling function.
 *****************************************************************************/

lit_t *lit_find_by_id(Lyst list, ResourceLock *mutex, mid_t *id)
{
	LystElt elt;
	lit_t *cur_lit = NULL;

	DTNMP_DEBUG_ENTRY("lit_find_by_id","(0x%x, 0x%x, 0x%x)",
			          (unsigned long) list,
			          (unsigned long) mutex,
			          (unsigned long) id);

	/* Step 0: Sanity Check. */
	if((list == NULL) || (id == NULL))
	{
		DTNMP_DEBUG_ERR("lit_find_by_id","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("lit_find_by_id","->NULL.", NULL);
		return NULL;
	}

	/* Step 1: Sanity check. */
	if((MID_GET_FLAG_TYPE(id->flags) != MID_TYPE_LITERAL) ||
		(MID_GET_FLAG_CAT(id->flags) != MID_CAT_ATOMIC))
	{
		DTNMP_DEBUG_ERR("lit_find_by_id","Malformed MID for literal. ",NULL);
		DTNMP_DEBUG_EXIT("lit_find_by_id","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Go looking. */
	if(mutex != NULL)
	{
		lockResource(mutex);
	}

	/* Step 2.1: For each literal structure. */
    for (elt = lyst_first(list); elt; elt = lyst_next(elt))
    {
        /* Step 2.1: Grab the definition */
        if((cur_lit = (lit_t*) lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_ERR("lit_find_by_id","Can't grab literal from lyst!", NULL);
        }
        else
        {
        	/* Step 2.2: Compare it. We do not match on parameters for literals. */
        	if(mid_compare(id, cur_lit->id,0) == 0)
        	{
        		DTNMP_DEBUG_EXIT("lit_find_by_id","->0x%x.", cur_lit);
        	    if(mutex != NULL)
        	    {
        	    	unlockResource(mutex);
        	    }
        		return cur_lit;
        	}
        }
    }

    if(mutex != NULL)
    {
    	unlockResource(mutex);
    }

	DTNMP_DEBUG_EXIT("lit_find_by_id","->NULL.", NULL);
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: lit_get_value
 *
 * \par Returns the value associated with a literal.
 *
 * \retval expr_result_t - The literal evaluation result.
 *
 * \param[in] lit  The literal definition whose value is being evaluated.
 *
 * \par Notes:
 *		1. If there is an error, the resulting value will have the unknown type.
 *
 *		2. Parameterized Literal definitions use their calc function.
 *****************************************************************************/

value_t lit_get_value(lit_t *lit)
{
	value_t result;

	DTNMP_DEBUG_ENTRY("lit_get_value","(0x%x)",
			          (unsigned long) lit);

	result.type = DTNMP_TYPE_UNK;
	result.length = 0;
	result.value.as_int = 0;

	/* Step 0: Sanity Check. */
	if(lit == NULL)
	{
		DTNMP_DEBUG_ERR("lit_get_value","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("lit_get_value","-> Bad Expr.", NULL);
		return result;
	}

	/* Step 1: Determine how we resolve value. */
	if(mid_get_num_parms(lit->id) > 0)
	{
		if(lit->calc == NULL)
		{
			DTNMP_DEBUG_ERR("lit_get_value","No calc function for parameterized literal.",NULL);
			DTNMP_DEBUG_EXIT("lit_get_value","-> Bad Expr.", NULL);
			return result;
		}

		DTNMP_DEBUG_INFO("lit_get_value","Using literal calc function.",NULL);

		result = lit->calc(lit->id);
	}
	else
	{
		DTNMP_DEBUG_INFO("lit_get_value","Using literal predefined value.",NULL);

		result = lit->value;
	}

	DTNMP_DEBUG_EXIT("lit_get_value","-> Resultant type: %d.", result.type);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: lit_lyst_clear
 *
 * \par Releases a list of literal structure definitions.
 *
 * \param[in] list    The list of literal structure definitions.
 * \param[in] mutex   Mutex protecting the list.
 * \param[in] destroy Whether to destroy the list and mutex when finished.
 *
 * \par Notes:
 *		1. If the destroy flag is set, the list MUST NOT be referenced.
 *		2. This method does not modify the Mutex regardless of the destroy flag.
 *****************************************************************************/

void lit_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy)
{

	 LystElt elt;
	 lit_t *cur = NULL;

	 DTNMP_DEBUG_ENTRY("lit_lyst_clear","(0x%x, 0x%x, %d)",
			          (unsigned long) list, (unsigned long) mutex, destroy);

	 /* Step 0: Sanity Checks. */
	 if((list == NULL) || (*list == NULL))
	 {
		 DTNMP_DEBUG_ERR("lit_lyst_clear","Bad Params.", NULL);
		 return;
	 }

	 /* Step 1: Lock access to the list. */
	 if(mutex != NULL)
	 {
		 lockResource(mutex);
	 }

	 /* \todo: Move all list NULL checks inside of mutex! */

	 /* Step 2: Free any literals left in the reports list. */
	 for (elt = lyst_first(*list); elt; elt = lyst_next(elt))
	 {
		 /* Grab the current report */
		 if((cur = (lit_t *) lyst_data(elt)) == NULL)
		 {
			 DTNMP_DEBUG_ERR("lit_lyst_clear","Can't get literal from lyst!", NULL);
		 }
		 else
		 {
			 lit_release(cur);
		 }
	 }
	 lyst_clear(*list);

	 if(destroy != 0)
	 {
		 DTNMP_DEBUG_INFO("lit_lyst_clear","Destroying list", NULL);

		 lyst_destroy(*list);
		 *list = NULL;
	 }

	 if(mutex != NULL)
	 {
		 unlockResource(mutex);
	 }
	 DTNMP_DEBUG_EXIT("lit_lyst_clear","->.",NULL);

}



/******************************************************************************
 *
 * \par Function Name: lit_release
 *
 * \par Releases the resources associated with a literal value.
 *
 * \param[in|out] lit The literal being released.
 *
 * \par Notes:
 *		1. After this function exits, the literal MUST NOT be accessed again.
 *		2. The function is NULL-safe (you can pass NULL to it).
 *****************************************************************************/

void lit_release(lit_t *lit)
{

	DTNMP_DEBUG_ENTRY("lit_release","(0x%x)", (unsigned long) lit);

	if(lit != NULL)
	{
		if(lit->id != NULL)
		{
			mid_release(lit->id);
		}

		MRELEASE(lit);
	}

	DTNMP_DEBUG_EXIT("lit_release","->.",NULL);
}



/******************************************************************************
 *
 * \par Function Name: lit_to_string
 *
 * \par Generate a string version of a literal value..
 *
 * \retval char * - The string representation of the literal.
 *
 * \param[in] lit The literal being pritned.
 *
 * \par Notes:
 *		1. The returned string MUST be released by the caller.
 *		2. The resultant string looks like: <mid> - <value>.
 *****************************************************************************/

char *lit_to_string(lit_t *lit)
{
	char *val = NULL;
	char *mid = NULL;
	char *result = NULL;
	uint32_t size = 0;

	 DTNMP_DEBUG_ENTRY("lit_to_string","(0x%x)", (unsigned long) lit);

	 /* Step 0: Sanity Checks. */
	 if(lit == NULL)
	 {
		 DTNMP_DEBUG_ERR("lit_to_string","Bad Params.", NULL);
		 DTNMP_DEBUG_EXIT("lit_to_string","->NULL", NULL);

		 return NULL;
	 }

	 /* Step 1: Capture the value associated with the literal. */
	 if((val = val_to_string(&(lit->value))) == NULL)
	 {
		 DTNMP_DEBUG_ERR("lit_to_string","Unable to to_string val.", NULL);
		 DTNMP_DEBUG_EXIT("lit_to_string","->NULL", NULL);

		 return NULL;
	 }

	 /* Step 2: Capture the MID associated with the literal. */
	 if((mid = mid_to_string(lit->id)) == NULL)
	 {
		 MRELEASE(val);

		 DTNMP_DEBUG_ERR("lit_to_string","Unable to to_string mid.", NULL);
		 DTNMP_DEBUG_EXIT("lit_to_string","->NULL", NULL);

		 return NULL;
	 }

	 /* Step 3: Calculate the size of the result string.
	  * "<mid> - <val>" requires 4 extra bytes for the " - " and null term.
	  */
	 size = strlen(val) + strlen(mid) + 4;

	 /* Step 4: Allocate the string. */
	 if((result = (char*)MTAKE(size)) == NULL)
	 {
		 MRELEASE(val);
		 MRELEASE(mid);

		 DTNMP_DEBUG_ERR("lit_to_string", "Can't alloc %d bytes.", size);
		 DTNMP_DEBUG_EXIT("lit_to_string","->NULL",NULL);
		 return NULL;
	 }

	 /* Step 5: Construct the result. */
	 snprintf(result,size,"%s - %s", mid, val);

	 /* Step 6: Cleanup. */
	 MRELEASE(val);
	 MRELEASE(mid);

	 DTNMP_DEBUG_EXIT("lit_to_string","->0x%x",(unsigned long) result);
	 return result;
}


