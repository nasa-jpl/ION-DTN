/*****************************************************************************
 **
 ** \file cd.c
 **
 **
 ** Description: Structures that capture computed data definitions.
 **
 ** Notes:
 **   These functions were originally part of the def.c package, and later
 **   customized as the cd package to support the sticky bit.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/05/16  E. Birrane     Initial implementation
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"
#include "shared/utils/nm_types.h"

#include "cd.h"


/******************************************************************************
 *
 * \par Function Name: cd_create
 *
 * \par Create a computed data definition.
 *
 * \retval NULL Failure
 *        !NULL The new Computed Data definition
 *
 * \param[in] mid   The identifier for the new CD.
 * \param[in] type  The type of the CD value.
 * \param[in] init  The CD definition.
 * \param[in] val   Optional value of the CD object.
 *
 * \par Notes:
 *		1. The ID is SHALLOW COPIED into this object and
 *		   MUST NOT be released by the calling function.
 *		2 The init expr and VAL are DEEP COPIED if used, and otherwise not. Either
 *		  way it MUST BE released by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/
cd_t *cd_create(mid_t *id,
				dtnmp_type_e type,
				expr_t *init,
				value_t val)
{
	cd_t *result = NULL;

	DTNMP_DEBUG_ENTRY("cd_create","(0x"ADDR_FIELDSPEC",%d,0x"ADDR_FIELDSPEC",val (type %d))", (uaddr) id, type, (uaddr) init, val.type);

	/* Step 0: Sanity Check. */
	if(id == NULL)
	{
		DTNMP_DEBUG_ERR("cd_create","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("cd_create","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the message. */
	if((result = (cd_t*)STAKE(sizeof(cd_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_create","Can't alloc %d bytes.",
				        sizeof(cd_t));
		DTNMP_DEBUG_EXIT("cd_create","->NULL",NULL);
		return NULL;
	}

	memset(result, 0, sizeof(cd_t));

	/* Step 2: Populate the message. */
	result->id = id;

	if(val.type != DTNMP_TYPE_UNK)
	{
		result->value = val_copy(val);
	}
	else if(init == NULL)
	{
		DTNMP_DEBUG_ERR("cd_create","NULL val and init expr.", NULL);
		cd_release(result);
		DTNMP_DEBUG_EXIT("cd_create","->NULL", NULL);
		return NULL;
	}
	else if(type == DTNMP_TYPE_EXPR)
	{
		result->value.type = DTNMP_TYPE_EXPR;
		result->value.value.as_ptr = (uint8_t *)expr_copy(init);
	}
	else
	{
		result->value = expr_eval(init);
		if(result->value.type == DTNMP_TYPE_UNK)
		{
			char *init_str = expr_to_string(init);
			DTNMP_DEBUG_ERR("cd_create", "Can't evaluate initial expression %s", init_str);
			SRELEASE(init_str);
			cd_release(result);
			DTNMP_DEBUG_EXIT("cd_create","->NULL",NULL);
			return NULL;
		}
	}

	DTNMP_DEBUG_EXIT("cd_create","->0x"ADDR_FIELDSPEC,(uaddr)result);
	return result;
}


/******************************************************************************
 *
 * \par Function Name: cd_create_from_parms
 *
 * \par Create a computed data definition from a set of parameters held in
 *      a Lyst.
 *
 * \retval NULL Failure
 *        !NULL The new Computed Data definition
 *
 * \param[in] parms The set of parameters describing the new CD.
 *
 * \par Notes:
 *		1. The parms MUSt be as follows: (MID Id, UINT Sticky, MC Definition)
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

cd_t *cd_create_from_parms(Lyst parms)
{
	cd_t *result = NULL;
	LystElt elt = NULL;
	blob_t *cur = NULL;
	mid_t *mid = NULL;
	uint32_t bytes = 0;
	expr_t *init = NULL;
	uint8_t type;

	/* Step 0: Sanity Check. */
	if(lyst_length(parms) != 3)
	{
		DTNMP_DEBUG_ERR("cd_create_from_parms","Bad # params. Need 3, received %d", lyst_length(parms));
		return NULL;
	}

	/* Step 1: Grab the MID defining the new computed definition. */
	elt = lyst_first(parms);
	cur = lyst_data(elt);
	if((mid = mid_deserialize(cur->value, cur->length, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_create_from_parms","Can't grab Parm 1 (MID).", NULL);
		return NULL;
	}

	/* Step 2: Grab UINT holding the type */
	elt = lyst_next(elt);
	cur = lyst_data(elt);
	if((bytes = utils_grab_byte(cur->value, cur->length, &type)) != 1)
	{
		mid_release(mid);
		DTNMP_DEBUG_ERR("cd_create_from_parms","Can't grab Parm 2 (BYTE).", NULL);
		return NULL;
	}

	/* Step 3: Grab the initializing expression capturing the definition. */
	elt = lyst_next(elt);
	cur = lyst_data(elt);
	if((init = expr_deserialize(cur->value, cur->length, &bytes)) == NULL)
	{
		mid_release(mid);
		DTNMP_DEBUG_ERR("cd_create_from_parms","Can't grab Parm 3 (EXPR).", NULL);
		return NULL;
	}

	/* Step 4: Create the new definition.This shallow-copies the MID and
	 * deep copies the expr. */
	value_t val;
	val_init(&val);
	result = cd_create(mid, type, init, val);
	expr_release(init);

	DTNMP_DEBUG_EXIT("cd_create_from_parms","->0x"ADDR_FIELDSPEC,(uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: cd_deserialize
 *
 * \par Construct a CD from a serialized byte stream.
 *
 * \retval NULL Failure
 *        !NULL The new Computed Data definition
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

cd_t *cd_deserialize(uint8_t *cursor,
		             uint32_t size,
		             uint32_t *bytes_used)
{
	cd_t *result = NULL;
	uint32_t bytes = 0;
	mid_t *id = NULL;
    value_t val;

	DTNMP_DEBUG_ENTRY("cd_deserialize",
			          "(0x"ADDR_FIELDSPEC",%d,0x"ADDR_FIELDSPEC")",
			          (uaddr)cursor, size, (uaddr) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		DTNMP_DEBUG_ERR("cd_deserialize","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("cd_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Deserialize the message. */
	val_init(&val);

	/* Step 1.1: Grab the ID MID. */
	if((id = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_deserialize","Can't grab ID MID.",NULL);
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("cd_deserialize","->NULL",NULL);
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
	if((bytes <= 0) || (val.type == DTNMP_TYPE_UNK))
	{
		DTNMP_DEBUG_ERR("cd_deserialize", "Can't grab VAL.", NULL);
		*bytes_used = 0;
		mid_release(id);
		DTNMP_DEBUG_EXIT("cd_deserialize", "->NULL", NULL);
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
	result = cd_create(id, val.type, NULL, val);

	val_release(&val, 0);

	DTNMP_DEBUG_EXIT("cd_deserialize","->0x"ADDR_FIELDSPEC,(uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: cd_duplicate
 *
 * \par Deep copy a computed data definition.
 *
 * \retval NULL Failure
 *        !NULL The copied Computed Data definition
 *
 * \param[in] orig   The CD object to copy.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

cd_t *cd_duplicate(cd_t *orig)
{
	mid_t *new_id;
	cd_t *result = NULL;

	/* Step 0: Sanity Check. */
	if(orig == NULL)
	{
		DTNMP_DEBUG_ERR("cd_duplicate","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("cd_duplicate","-->NULL",NULL);
		return NULL;
	}

	/* Step 1: Deep copy specific structures. */

	/* Step 1.1: Deep copy ID. */
	if((new_id = mid_copy(orig->id)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_duplicate","Can't copy ID.",NULL);
		DTNMP_DEBUG_EXIT("cd_duplicate","-->NULL",NULL);
		return NULL;
	}

	/* Step 1.2: Deep copy value. */

	if((result = cd_create(new_id, orig->value.type, NULL, orig->value)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_duplicate","Can't duplicate DEF.", NULL);
		mid_release(new_id);
		DTNMP_DEBUG_EXIT("cd_duplicate","-->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("cd_duplicate","-->0x"ADDR_FIELDSPEC, (uaddr) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: cd_find_by_id
 *
 * \par Find a CD object by its MID in a Lyst of CD objects.
 *
 * \retval NULL Failure
 *        !NULL The found Computed Data object
 *
 * \param[in]     cds    The list of CD objects.
 * \param[in|out] mutex  The mutex (if any) protecting access to the list.
 * \param[in]     id     The MID of the CD to find.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

cd_t *cd_find_by_id(Lyst cds, ResourceLock *mutex, mid_t *id)
{
	LystElt elt;
	cd_t *cur_cd = NULL;

	DTNMP_DEBUG_ENTRY("cd_find_by_id","(0x"ADDR_FIELDSPEC", 0x"ADDR_FIELDSPEC", 0x"ADDR_FIELDSPEC")", (uaddr) cds, (uaddr) mutex, (uaddr) id);

	/* Step 0: Sanity Check. */
	if((cds == NULL) || (id == NULL))
	{
		DTNMP_DEBUG_ERR("cd_find_by_id","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("cd_find_by_id","->NULL.", NULL);
		return NULL;
	}

	if(mutex != NULL)
	{
		lockResource(mutex);
	}
    for (elt = lyst_first(cds); elt; elt = lyst_next(elt))
    {
        /* Grab the definition */
        if((cur_cd = (cd_t*) lyst_data(elt)) == NULL)
        {
        	DTNMP_DEBUG_ERR("cd_find_by_id","Can't grab cd from lyst!", NULL);
        }
        else
        {
        	if(mid_compare(id, cur_cd->id, 1) == 0)
        	{
        		DTNMP_DEBUG_EXIT("cd_find_by_id","->0x%x.", cur_cd);
        	    if(mutex != NULL)
        	    {
        	    	unlockResource(mutex);
        	    }
        		return cur_cd;
        	}
        }
    }

    if(mutex != NULL)
    {
    	unlockResource(mutex);
    }

	DTNMP_DEBUG_EXIT("cd_find_by_id","->NULL.", NULL);
	return NULL;
}



/******************************************************************************
 *
 * \par Function Name: cd_lyst_clear
 *
 * \par Releases all CDs in a list of CDs.
 *
 *
 * \param[out]    cds      The list of CD objects.
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

void cd_lyst_clear(Lyst *cds, ResourceLock *mutex, int destroy)
{
	 LystElt elt;
	 cd_t *cur_cd = NULL;

	 DTNMP_DEBUG_ENTRY("cd_lyst_clear","(0x"ADDR_FIELDSPEC", 0x"ADDR_FIELDSPEC", %d)", (uaddr) cds, (uaddr) mutex, destroy);

	 if((cds == NULL) || (*cds == NULL))
	 {
		 DTNMP_DEBUG_ERR("cd_lyst_clear","Bad Params.", NULL);
		 return;
	 }

	 if(mutex != NULL)
	 {
		 lockResource(mutex);
	 }

	 /* Free any reports left in the reports list. */
	 for (elt = lyst_first(*cds); elt; elt = lyst_next(elt))
	 {
		 /* Grab the current report */
		 if((cur_cd = (cd_t *) lyst_data(elt)) == NULL)
		 {
			 DTNMP_DEBUG_ERR("cd_lyst_clear","Can't get CD from list!", NULL);
		 }
		 else
		 {
			 cd_release(cur_cd);
		 }
	 }
	 lyst_clear(*cds);

	 if(destroy != 0)
	 {
		 lyst_destroy(*cds);
		 *cds = NULL;
	 }

	 if(mutex != NULL)
	 {
		 unlockResource(mutex);
	 }

	 DTNMP_DEBUG_EXIT("cd_lyst_clear","->.",NULL);
}


/******************************************************************************
 *
 * \par Function Name: cd_release
 *
 * \par Releases memory associated with a CD Object
 *
 *
 * \param[in|out]    cd      The CD object being freed.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/05/16  E. Birrane     Initial implementation, modified from def.c
 *****************************************************************************/

void cd_release(cd_t *cd)
{
	DTNMP_DEBUG_ENTRY("cd_release", "(0x"ADDR_FIELDSPEC")", (uaddr) cd);

	if(cd != NULL)
	{
		if(cd->id != NULL)
		{
			mid_release(cd->id);
		}

		val_release(&(cd->value), 0);

		SRELEASE(cd);
	}

	DTNMP_DEBUG_EXIT("cd_release","->.",NULL);
}



/******************************************************************************
 *
 * \par Function Name: cd_serialize
 *
 * \par Serializes a CD object to a byte stream.
 *
 * \retval NULL Failure
 *        !NULL The serialized CD object as a byte stream
 *
 * \param[in]  cd   The CD object to represent.
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

uint8_t *cd_serialize(cd_t *cd, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *id = NULL;
	uint32_t id_len = 0;

	uint8_t *val = NULL;
	uint32_t val_len = 0;

	DTNMP_DEBUG_ENTRY("cd_serialize",
			          "(0x"ADDR_FIELDSPEC",0x"ADDR_FIELDSPEC")",
			          (uaddr)cd, (uaddr) len);

	/* Step 0: Sanity Checks. */
	if((cd == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("cd_serialize","Bad Args",NULL);
		DTNMP_DEBUG_EXIT("cd_serialize","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* STEP 1: Serialize the ID. */
	if((id = mid_serialize(cd->id, &id_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_serialize","Can't serialize hdr.",NULL);

		DTNMP_DEBUG_EXIT("cd_serialize","->NULL",NULL);
		return NULL;
	}

	/* STEP 2: Serialize the value. */
	if((val = val_serialize(cd->value, &val_len, 1)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_serialize","Can't serialize value.",NULL);
		SRELEASE(id);

		DTNMP_DEBUG_EXIT("cd_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Figure out the length. */
	*len = id_len + val_len;

	/* STEP 4: Allocate the serialized message. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_serialize","Can't alloc %d bytes", *len);
		*len = 0;
		SRELEASE(id);
		SRELEASE(val);

		DTNMP_DEBUG_EXIT("cd_serialize","->NULL",NULL);
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
		DTNMP_DEBUG_ERR("cd_serialize","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		DTNMP_DEBUG_EXIT("cd_serialize","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("cd_serialize","->0x"ADDR_FIELDSPEC,(uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: cd_to_string
 *
 * \par Creates a string representation of a CD object.
 *
 * \retval NULL Failure
 *        !NULL The string representation of the CD object.
 *
 *
 * \param[in] cd  The CD object to represent.
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

char *cd_to_string(cd_t *cd)
{
	char *result = NULL;
	char *id_str = NULL;
	char *val_str = NULL;
	uint32_t len = 0;

	/* Step 0: Sanity Check. */
	if(cd == NULL)
	{
		DTNMP_DEBUG_ERR("cd_to_string", "Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Create string representation of basic objects. */
	id_str = mid_to_string(cd->id);
	val_str = val_to_string(cd->value);

	/*
	 * Step 2: Calculate length of final string:
	 * Format is:
	 * CD MID(%s) Sticky(%d) Def(%s) Val(%s)
	 */

	len = 9 + strlen(id_str) + 1;   // "CD MID(%s) "
	len += 5 + strlen(val_str) + 1; // "Val(%s) "
	len += 1;                       // null term

	/* Step 3: Allocate final string. */
	if((result = STAKE(len)) == NULL)
	{
		DTNMP_DEBUG_ERR("cd_to_string", "Can't allocate length %d", len);
		SRELEASE(id_str);
		SRELEASE(val_str);
		return NULL;
	}

	/* Step 4: Construct new string. */
	sprintf(result,
			"CD MID(%s) Val(%s)",
			id_str, val_str);

	/* Step 5: Free resources. */
	SRELEASE(id_str);
	SRELEASE(val_str);

	return result;
}


