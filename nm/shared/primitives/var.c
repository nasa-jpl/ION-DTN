/*****************************************************************************
 **
 ** \file var.c
 **
 **
 ** Description: Structures that capture variable definitions.
 **
 ** Notes:
 **   These functions were originally part of the def.c package.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/05/16  E. Birrane     Initial implementation (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane     Renamed CD to VAR. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"
#include "../utils/nm_types.h"

#include "var.h"


/******************************************************************************
 *
 * \par Function Name: var_create
 *
 * \par Create a variable definition.
 *
 * \retval NULL Failure
 *        !NULL The new variable definition
 *
 * \param[in] mid   The identifier for the new VAR.
 * \param[in] type  The type of the VAR value.
 * \param[in] init  The VAR definition.
 * \param[in] val   Optional value of the VAR object.
 *
 * \par Notes:
 *		1. The ID is SHALLOW COPIED into this object and
 *		   MUST NOT be released by the calling function.
 *		2 The init expr and VAL are DEEP COPIED if used, and otherwise not. Either
 *		  way they MUST BE released by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/
var_t *var_create(mid_t *id,
				amp_type_e type,
				expr_t *init,
				value_t val)
{
	var_t *result = NULL;

	AMP_DEBUG_ENTRY("var_create","(0x" ADDR_FIELDSPEC ",%d,0x" ADDR_FIELDSPEC ",val (type %d))",
			          (uaddr) id, type, (uaddr) init, val.type);

	/* Step 0: Sanity Check. */
	if(id == NULL)
	{
		AMP_DEBUG_ERR("var_create","Bad Args.",NULL);
		AMP_DEBUG_EXIT("var_create","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (var_t*)STAKE(sizeof(var_t))) == NULL)
	{
		AMP_DEBUG_ERR("var_create","Can't alloc %d bytes.",
				        sizeof(var_t));
		AMP_DEBUG_EXIT("var_create","->NULL",NULL);
		return NULL;
	}

	memset(result, 0, sizeof(var_t));

	/* Step 2: Populate the message. */
	result->id = id;

	if(val.type != AMP_TYPE_UNK)
	{
		result->value = val_copy(val);
	}
	else if(init == NULL)
	{
		AMP_DEBUG_ERR("var_create","NULL val and init expr.", NULL);
		var_release(result);
		AMP_DEBUG_EXIT("var_create","->NULL", NULL);
		return NULL;
	}
	else if(type == AMP_TYPE_EXPR)
	{
		result->value.type = AMP_TYPE_EXPR;
		result->value.value.as_ptr = (uint8_t *)expr_copy(init);
	}
	else
	{
		result->value = expr_eval(init);
		if(result->value.type == AMP_TYPE_UNK)
		{
			char *init_str = expr_to_string(init);
			AMP_DEBUG_ERR("var_create", "Can't evaluate initial expression %s", init_str);
			SRELEASE(init_str);
			var_release(result);
			AMP_DEBUG_EXIT("var_create","->NULL",NULL);
			return NULL;
		}
	}

	AMP_DEBUG_EXIT("var_create", "->0x" ADDR_FIELDSPEC, (uaddr)result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: var_create_from_parms
 *
 * \par Create a variable definition from a set of parameters held in
 *      a Lyst.
 *
 * \retval NULL Failure
 *        !NULL The new variable definition
 *
 * \param[in] parms The set of parameters describing the new VAR.
 *
 * \par Notes:
 *		1. The parms MUSt be as follows: (MID Id, UINT Sticky, MC Definition)
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

var_t *var_create_from_parms(Lyst parms)
{
	var_t *result = NULL;
	LystElt elt = NULL;
	blob_t *cur = NULL;
	mid_t *mid = NULL;
	uint32_t bytes = 0;
	expr_t *init = NULL;
	uint8_t type;

	/* Step 0: Sanity Check. */
	if(lyst_length(parms) != 3)
	{
		AMP_DEBUG_ERR("var_create_from_parms","Bad # params. Need 3, received %d", lyst_length(parms));
		return NULL;
	}

	/* Step 1: Grab the MID defining the new computed definition. */
	elt = lyst_first(parms);
	cur = lyst_data(elt);
	if((mid = mid_deserialize(cur->value, cur->length, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("var_create_from_parms","Can't grab Parm 1 (MID).", NULL);
		return NULL;
	}

	/* Step 2: Grab UINT holding the type */
	elt = lyst_next(elt);
	cur = lyst_data(elt);
	if((bytes = utils_grab_byte(cur->value, cur->length, &type)) != 1)
	{
		mid_release(mid);
		AMP_DEBUG_ERR("var_create_from_parms","Can't grab Parm 2 (BYTE).", NULL);
		return NULL;
	}

	/* Step 3: Grab the initializing expression capturing the definition. */
	elt = lyst_next(elt);
	cur = lyst_data(elt);
	if((init = expr_deserialize(cur->value, cur->length, &bytes)) == NULL)
	{
		mid_release(mid);
		AMP_DEBUG_ERR("var_create_from_parms","Can't grab Parm 3 (EXPR).", NULL);
		return NULL;
	}

	/* Step 4: Create the new definition.This shallow-copies the MID and
	 * deep copies the expr. */
	value_t val;
	val_init(&val);
	result = var_create(mid, type, init, val);
	expr_release(init);

	AMP_DEBUG_EXIT("var_create_from_parms", "->0x" ADDR_FIELDSPEC, (uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: var_deserialize
 *
 * \par Construct a VAR from a serialized byte stream.
 *
 * \retval NULL Failure
 *        !NULL The new variable definition
 *
 * \param[in] cursor       The start of the byte stream
 * \param[in] size         The length of the byte stream in bytes
 * \param[out] bytes_used  Number of bytes read from the stream.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

var_t *var_deserialize(uint8_t *cursor,
		             uint32_t size,
		             uint32_t *bytes_used)
{
	var_t *result = NULL;
	uint32_t bytes = 0;
	mid_t *id = NULL;
    value_t val;

	AMP_DEBUG_ENTRY("var_deserialize",
			          "(0x" ADDR_FIELDSPEC ",%d,0x" ADDR_FIELDSPEC ")",
			          (uaddr)cursor, size, (uaddr) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("var_deserialize","Bad Args.",NULL);
		AMP_DEBUG_EXIT("var_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Deserialize the message. */
	val_init(&val);

	/* Step 1.1: Grab the ID MID. */
	if((id = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("var_deserialize","Can't grab ID MID.",NULL);
		*bytes_used = 0;
		AMP_DEBUG_EXIT("var_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 1.2: Grab the value. */
	val = val_deserialize(cursor, size, &bytes);
	if((bytes <= 0) || (val.type == AMP_TYPE_UNK))
	{
		AMP_DEBUG_ERR("var_deserialize", "Can't grab VAL.", NULL);
		*bytes_used = 0;
		mid_release(id);
		AMP_DEBUG_EXIT("var_deserialize", "->NULL", NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 2: Create the new definition. This is a shallow copy so
	 * don't release the mid and expr.  */
	result = var_create(id, val.type, NULL, val);

	val_release(&val, 0);

	AMP_DEBUG_EXIT("var_deserialize", "->0x" ADDR_FIELDSPEC, (uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: var_duplicate
 *
 * \par Deep copy a variable definition.
 *
 * \retval NULL Failure
 *        !NULL The copied variable definition
 *
 * \param[in] orig   The VAR object to copy.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

var_t *var_duplicate(var_t *orig)
{
	mid_t *new_id;
	var_t *result = NULL;

	/* Step 0: Sanity Check. */
	if(orig == NULL)
	{
		AMP_DEBUG_ERR("var_duplicate","Bad Args.",NULL);
		AMP_DEBUG_EXIT("var_duplicate","-->NULL",NULL);
		return NULL;
	}

	/* Step 1: Deep copy specific structures. */

	/* Step 1.1: Deep copy ID. */
	if((new_id = mid_copy(orig->id)) == NULL)
	{
		AMP_DEBUG_ERR("var_duplicate","Can't copy ID.",NULL);
		AMP_DEBUG_EXIT("var_duplicate","-->NULL",NULL);
		return NULL;
	}

	/* Step 1.2: Deep copy value. */

	if((result = var_create(new_id, orig->value.type, NULL, orig->value)) == NULL)
	{
		AMP_DEBUG_ERR("var_duplicate","Can't duplicate DEF.", NULL);
		mid_release(new_id);
		AMP_DEBUG_EXIT("var_duplicate","-->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("var_duplicate", "-->0x" ADDR_FIELDSPEC, (uaddr) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: var_find_by_id
 *
 * \par Find a VAR object by its MID in a Lyst of VAR objects.
 *
 * \retval NULL Failure
 *        !NULL The found variable object
 *
 * \param[in]     vars    The list of VAR objects.
 * \param[in|out] mutex  The mutex (if any) protecting access to the list.
 * \param[in]     id     The MID of the VAR to find.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

var_t *var_find_by_id(Lyst vars, ResourceLock *mutex, mid_t *id)
{
	LystElt elt;
	var_t *cur_var = NULL;

	AMP_DEBUG_ENTRY("var_find_by_id","(0x" ADDR_FIELDSPEC ", 0x" ADDR_FIELDSPEC ", 0x" ADDR_FIELDSPEC ")",
			          (uaddr) vars, (uaddr) mutex, (uaddr) id);

	/* Step 0: Sanity Check. */
	if((vars == NULL) || (id == NULL))
	{
		AMP_DEBUG_ERR("var_find_by_id","Bad Args.",NULL);
		AMP_DEBUG_EXIT("var_find_by_id","->NULL.", NULL);
		return NULL;
	}

	if(mutex != NULL)
	{
		lockResource(mutex);
	}
    for (elt = lyst_first(vars); elt; elt = lyst_next(elt))
    {
        /* Grab the definition */
        if((cur_var = (var_t*) lyst_data(elt)) == NULL)
        {
        	AMP_DEBUG_ERR("var_find_by_id","Can't grab var from lyst!", NULL);
        }
        else
        {
        	if(mid_compare(id, cur_var->id, 1) == 0)
        	{
        		AMP_DEBUG_EXIT("var_find_by_id","->0x%x.", cur_var);
        	    if(mutex != NULL)
        	    {
        	    	unlockResource(mutex);
        	    }
        		return cur_var;
        	}
        }
    }

    if(mutex != NULL)
    {
    	unlockResource(mutex);
    }

	AMP_DEBUG_EXIT("var_find_by_id","->NULL.", NULL);
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: var_lyst_clear
 *
 * \par Releases all VARs in a list of VARs.
 *
 *
 * \param[out]    vars      The list of VAR objects.
 * \param[in|out] mutex    The mutex (if any) protecting access to the list.
 * \param[in]     destroy  Whether to destroy the list (1) or not (0).
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

void var_lyst_clear(Lyst *vars, ResourceLock *mutex, int destroy)
{
	 LystElt elt;
	 var_t *cur_var = NULL;

	 AMP_DEBUG_ENTRY("var_lyst_clear","(0x" ADDR_FIELDSPEC ", 0x" ADDR_FIELDSPEC ", %d)",
			          (uaddr) vars, (uaddr) mutex, destroy);

	 if((vars == NULL) || (*vars == NULL))
	 {
		 AMP_DEBUG_ERR("var_lyst_clear","Bad Params.", NULL);
		 return;
	 }

	 if(mutex != NULL)
	 {
		 lockResource(mutex);
	 }

	 /* Free any reports left in the reports list. */
	 for (elt = lyst_first(*vars); elt; elt = lyst_next(elt))
	 {
		 /* Grab the current report */
		 if((cur_var = (var_t *) lyst_data(elt)) == NULL)
		 {
			 AMP_DEBUG_ERR("var_lyst_clear","Can't get VAR from list!", NULL);
		 }
		 else
		 {
			 var_release(cur_var);
		 }
	 }
	 lyst_clear(*vars);

	 if(destroy != 0)
	 {
		 lyst_destroy(*vars);
		 *vars = NULL;
	 }

	 if(mutex != NULL)
	 {
		 unlockResource(mutex);
	 }

	 AMP_DEBUG_EXIT("var_lyst_clear","->.",NULL);
}


/******************************************************************************
 *
 * \par Function Name: var_release
 *
 * \par Releases memory associated with a VAR Object
 *
 *
 * \param[in|out]    var      The VAR object being freed.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

void var_release(var_t *var)
{
	AMP_DEBUG_ENTRY("var_release",
			          "(0x" ADDR_FIELDSPEC ")",
			          (uaddr) var);

	if(var != NULL)
	{
		if(var->id != NULL)
		{
			mid_release(var->id);
		}

		val_release(&(var->value), 0);

		SRELEASE(var);
	}

	AMP_DEBUG_EXIT("var_release","->.",NULL);
}



/******************************************************************************
 *
 * \par Function Name: var_serialize
 *
 * \par Serializes a VAR object to a byte stream.
 *
 * \retval NULL Failure
 *        !NULL The serialized VAR object as a byte stream
 *
 * \param[in]  var   The VAR object to represent.
 * \param[out] len  The length of the byte stream
 *
 * \par Notes:
 *   1. The byte stream is allocated on the heap and MUST be released by the
 *      calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation
 *****************************************************************************/

uint8_t *var_serialize(var_t *var, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *id = NULL;
	uint32_t id_len = 0;

	uint8_t *val = NULL;
	uint32_t val_len = 0;

	AMP_DEBUG_ENTRY("var_serialize",
			          "(0x" ADDR_FIELDSPEC ",0x" ADDR_FIELDSPEC ")",
			          (uaddr)var, (uaddr) len);

	/* Step 0: Sanity Checks. */
	if((var == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("var_serialize","Bad Args",NULL);
		AMP_DEBUG_EXIT("var_serialize","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* STEP 1: Serialize the ID. */
	if((id = mid_serialize(var->id, &id_len)) == NULL)
	{
		AMP_DEBUG_ERR("var_serialize","Can't serialize hdr.",NULL);

		AMP_DEBUG_EXIT("var_serialize","->NULL",NULL);
		return NULL;
	}

	/* STEP 2: Serialize the value. */
	if((val = val_serialize(var->value, &val_len, 1)) == NULL)
	{
		AMP_DEBUG_ERR("var_serialize","Can't serialize value.",NULL);
		SRELEASE(id);

		AMP_DEBUG_EXIT("var_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Figure out the length. */
	*len = id_len + val_len;

	/* STEP 4: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("var_serialize","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(id);
		SRELEASE(val);

		AMP_DEBUG_EXIT("var_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 5: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, id, id_len);
	cursor += id_len;
	SRELEASE(id);

	memcpy(cursor, val, val_len);
	cursor += val_len;
	SRELEASE(val);

	/* Step 6: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("var_serialize","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("var_serialize","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("var_serialize", "->0x" ADDR_FIELDSPEC, (uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: var_to_string
 *
 * \par Creates a string representation of a VAR object.
 *
 * \retval NULL Failure
 *        !NULL The string representation of the VAR object.
 *
 *
 * \param[in] var  The VAR object to represent.
 *
 * \par Notes:
 *   1. The string is allocated on the heap and MUST be released by the
 *      calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation
 *****************************************************************************/

char *var_to_string(var_t *var)
{
	char *result = NULL;
	char *id_str = NULL;
	char *val_str = NULL;
	uint32_t len = 0;

	/* Step 0: Sanity Check. */
	if(var == NULL)
	{
		AMP_DEBUG_ERR("var_to_string", "Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Create string representation of basic objects. */
	id_str = mid_to_string(var->id);
	val_str = val_to_string(var->value);

	/*
	 * Step 2: Calculate length of final string:
	 * Format is:
	 * VAR MID(%s) Val(%s)
	 */

	len = 10 + strlen(id_str) + 1;   // "VAR MID(%s) "
	len += 5 + strlen(val_str) + 1; // "Val(%s) "
	len += 1;                       // null term

	/* Step 3: Allocate final string. */
	if((result = STAKE(len)) == NULL)
	{
		AMP_DEBUG_ERR("var_to_string", "Can't allocate length %d", len);
		SRELEASE(id_str);
		SRELEASE(val_str);
		return NULL;
	}

	/* Step 4: Construct new string. */
	sprintf(result,
			"VAR MID(%s) Val(%s)",
			id_str, val_str);

	/* Step 5: Free resources. */
	SRELEASE(id_str);
	SRELEASE(val_str);

	return result;
}


