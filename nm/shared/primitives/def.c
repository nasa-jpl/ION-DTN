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
 **  06/21/15  E. Birrane     Added <type> for definitions.
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"
#include "shared/utils/nm_types.h"

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
 * \param[in] type     The type of the new definition.
 * \param[in[ contents The new definition as an ordered set of MIDs.
 */
def_gen_t *def_create_gen(mid_t *id,
						  uint32_t type,
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
	result->type = type;
	result->contents = contents;

	DTNMP_DEBUG_EXIT("def_create_gen","->0x%x",result);
	return result;
}

// \todo: Move to utilities?
def_gen_t *def_create_from_rpt_parms(Lyst parms)
{
	def_gen_t *result = NULL;
	LystElt elt = NULL;
	datacol_entry_t *cur = NULL;
	mid_t *mid = NULL;
	uint32_t bytes = 0;
	Lyst expr = NULL;

	/* Step 0: Sanity Check. */
	if(lyst_length(parms) != 2)
	{
		DTNMP_DEBUG_ERR("def_create_from_rpt_parms","Bad # params. Need 2, received %d", lyst_length(parms));
		return NULL;
	}

	/* Step 1: Grab the MID defining the new computed definition. */
	elt = lyst_first(parms);
	cur = lyst_data(elt);
	if((mid = mid_deserialize(cur->value, cur->length, &bytes)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Grab the expression capturing the definition. */
	elt = lyst_next(elt);
	cur = lyst_data(elt);
	if((expr = midcol_deserialize(cur->value, cur->length, &bytes)) == NULL)
	{
		mid_release(mid);
		return NULL;
	}

	/* Step 3: Create the new definition. This is a shallow copy sp
	 * don't release the mis and expr.  */
	result = def_create_gen(mid, DTNMP_TYPE_DEF, expr);

	return result;
}

/**
 * \brief Duplicates a custom definition.
 *
 * \author Ed Birrane
 *
 * \note
 *   - This is a deep copy.
 *
 * \return NULL - Failure
 * 		   !NULL - Created definition
 *
 * \param[in] def  The custom definition to duplicate.
 */

def_gen_t *def_duplicate(def_gen_t *def)
{
	Lyst new_contents;
	mid_t *new_id;
	def_gen_t *result = NULL;
	LystElt elt;

	if(def == NULL)
	{
		DTNMP_DEBUG_ERR("def_duplicate","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("def_duplicate","-->NULL",NULL);
		return NULL;
	}

	if((new_id = mid_copy(def->id)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_duplicate","Can't copy DEF ID.",NULL);
		DTNMP_DEBUG_EXIT("def_duplicate","-->NULL",NULL);
		return NULL;
	}

	if((new_contents = midcol_copy(def->contents)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_duplicate","Can't copy DEF contents.", NULL);
		mid_release(new_id);

		DTNMP_DEBUG_EXIT("def_duplicate","-->NULL",NULL);
		return NULL;
	}

	if((result = def_create_gen(new_id, def->type, new_contents)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_duplicate","Can't duplicate DEF.", NULL);
		mid_release(new_id);
		midcol_destroy(&new_contents);
		DTNMP_DEBUG_EXIT("def_duplicate","-->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("def_duplicate","-->%x", (unsigned long) result);
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

	fprintf(stderr,"Definition:\n----------ID:%s\nType:%d\nMC:\n%s\n\n----------",
			id_str, def->type, mc_str);

	MRELEASE(id_str);
	MRELEASE(mc_str);
}




/**
 * \brief serializes a definition message into a buffer.
 *
 * \author Ed Birrane
 *
 * \note The returned message must be de-allocated from the memory pool.
 *
 * \return NULL - Failure
 *         !NULL - The serialized message.
 *
 * \param[in]  msg  The message to serialize.
 * \param[out] len  The length of the serialized message.
 */
uint8_t *def_serialize_gen(def_gen_t *def, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *id = NULL;
	uint32_t id_len = 0;

	Sdnv type_sdnv;

	uint8_t *contents = NULL;
	uint32_t contents_len = 0;

	DTNMP_DEBUG_ENTRY("def_serialize_gen","(0x%x, 0x%x)",
			          (unsigned long)def, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((def == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Bad Args",NULL);
		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* STEP 1: Serialize the ID. */
	if((id = mid_serialize(def->id, &id_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Can't serialize hdr.",NULL);

		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	/* STEP 2: Serialize the type as an SDNV. */
	encodeSdnv(&type_sdnv, def->type);

	/* STEP 3: Serialize the Contents. */
	if((contents = midcol_serialize(def->contents, &contents_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Can't serialize hdr.",NULL);
		MRELEASE(id);

		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Figure out the length. */
	*len = id_len + type_sdnv.length + contents_len;

	/* STEP 5: Allocate the serialized message. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Can't alloc %d bytes", *len);
		*len = 0;
		MRELEASE(id);
		MRELEASE(contents);

		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 6: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, id, id_len);
	cursor += id_len;
	MRELEASE(id);

	memcpy(cursor, type_sdnv.text, type_sdnv.length);
	cursor += type_sdnv.length;

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	MRELEASE(contents);

	/* Step 7: Last sanity check. */
	if((cursor - result) != *len)
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("def_serialize_gen","->0x%x",(unsigned long)result);
	return result;
}


/**
 * \brief Creates a definition message from a buffer.
 *
 * \author Ed Birrane
 *
 * \note
 *   - On failure (NULL return) we do NOT de-allocate the passed-in header.
 *
 * \return NULL - failure
 *         !NULL - message.
 *
 * \param[in]  cursor      The buffer holding the message.
 * \param[in]  size        The remaining buffer size.
 * \param[out] bytes_used  Bytes consumed in the deserialization.
 */
def_gen_t *def_deserialize_gen(uint8_t *cursor,
		                       uint32_t size,
		                       uint32_t *bytes_used)
{
	def_gen_t *result = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("def_deserialize_gen","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			           size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		DTNMP_DEBUG_ERR("def_deserialize_gen","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("def_deserialize_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate the new message structure. */
	if((result = (def_gen_t*)MTAKE(sizeof(def_gen_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("def_deserialize_gen","Can't Alloc %d Bytes.",
				        sizeof(def_gen_t));
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("def_deserialize_gen","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(def_gen_t));
	}

	/* Step 2: Deserialize the message. */

	/* Grab the ID MID. */
	if((result->id = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_deserialize_gen","Can't grab ID MID.",
				        sizeof(def_gen_t));
		*bytes_used = 0;
		def_release_gen(result);
		DTNMP_DEBUG_EXIT("def_deserialize_gen","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Grab the definition type. */
	uvast tmp;
	if((bytes = utils_grab_sdnv(cursor, size, &(tmp))) <= 0)
	{
		DTNMP_DEBUG_ERR("def_deserialize_gen", "Can't grab type SDNV.", NULL);
		*bytes_used = 0;
		def_release_gen(result);
		DTNMP_DEBUG_EXIT("def_deserialize_gen", "->NULL", NULL);
		return NULL;
	}
	else
	{
		result->type = (uint32_t) tmp;
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}


	/* Grab the list of contents. */
	if((result->contents = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_deserialize_gen","Can't grab contents.",NULL);

		*bytes_used = 0;
		def_release_gen(result);
		DTNMP_DEBUG_EXIT("def_deserialize_gen","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	DTNMP_DEBUG_EXIT("def_deserialize_gen","->0x%x",(unsigned long)result);
	return result;
}
