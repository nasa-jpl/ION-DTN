/*****************************************************************************
 **
 ** \file oid.c
 **
 ** Description: This file contains the implementation of functions that
 **              operate on Object Identifiers (OIDs). This file also implements
 **              global values, including the OID nickname database.
 **
 ** Notes:
 **	     1. In the current implementation, the nickname database is not
 **	        persistent.
 **
 ** Assumptions:
 **      1. We limit the size of an OID in the system to reduce the amount
 **         of pre-allocated memory in this embedded system. Non-embedded
 **         implementations may wish to dynamically allocate MIDs as they are
 **         received.
 **      2. Parameters for OIDs, which can be quite large, are dynamically
 **         allocated.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/29/12  E. Birrane     Initial Implementation
 **  11/14/12  E. Birrane     Code review comments and documentation update.
 **  08/29/15  E. Birrane     Removed length restriction from OID parms.
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"

#include "shared/primitives/oid.h"



/******************************************************************************
 *
 * \par Function Name: oid_add_param
 *
 * \par Adds a parameter to a parameterized OID.
 *
 * \retval 0 Failure
 *        !0 Success
 *
 * \param[in,out] oid    The OID getting a new param.
 * \param[in]     value  The value of the new parameter
 * \param[in]     len    The length, in bytes, of the new parameter.
 *
 * \par Notes:
 *		1. The new parameter is allocated into the OID and, upon exit,
 *		   the passed-in value may be released if necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation,
 *  08/29/15  E. Birrane     Parms no longer count against MAX_OID_SIZE.
 *****************************************************************************/
int oid_add_param(oid_t *oid, uint8_t *value, uint32_t len)
{
	uint8_t *cursor = NULL;
	Sdnv len_sdnv;
	datacol_entry_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("oid_add_param","(%#llx, %#llx, %ld)", oid, value, len);

	/* Step 0: Sanity Check.*/
	if((oid == NULL) || (value == NULL) || (len == 0))
	{
		DTNMP_DEBUG_ERR("oid_add_param","Bad Args", NULL);
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	/* Step 1: Make sure the OID is a parameterized one. */
	if((oid->type != OID_TYPE_PARAM)  &&
	   (oid->type != OID_TYPE_COMP_PARAM))
	{
		DTNMP_DEBUG_ERR("oid_add_param","Can't add parameter to OID of type %d.",
				        oid->type);
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	/* Step 2: Make sure this doesn't give us too many params. */
	if((lyst_length(oid->params)+1) > MAX_OID_PARM)
	{
		DTNMP_DEBUG_ERR("oid_add_param","OID has %d params, no room for more.",
				         lyst_length(oid->params));
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	/* Step 3: Grab parameter length. */
	encodeSdnv(&len_sdnv, len);

	/* Step 4: Allocate the entry. */
	if((entry = (datacol_entry_t*)MTAKE(sizeof(datacol_entry_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_add_param","Can't alloc %d bytes.",
				        sizeof(datacol_entry_t));
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	entry->length = len;

	if((entry->value = (uint8_t*) MTAKE(entry->length)) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_add_param","Cannot alloc %d bytes.",
				        entry->length);
		MRELEASE(entry);
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	cursor = entry->value;

	memcpy(cursor, value, len);
	cursor += len;

	lyst_insert_last(oid->params, entry);

	DTNMP_DEBUG_EXIT("oid_add_param","->%d.",1);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: oid_add_params
 *
 * \par Adds a set of parameters to a parameterized OID.
 *
 * \retval 0 Failure
 *        !0 Success
 *
 * \param[in,out] oid     The OID getting a new param.
 * \param[in]     params  The Lyst of new parameters.
 *
 * \par Notes:
 *		1. The new parameter is allocated into the OID and, upon exit,
 *		   the passed-in value may be released if necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/28/15  E. Birrane     Initial implementation,
 *****************************************************************************/
int oid_add_params(oid_t *oid, Lyst params)
{
	LystElt elt;
	datacol_entry_t* entry;

	for(elt = lyst_first(params); elt; elt = lyst_next(elt))
	{
		entry = (datacol_entry_t*) lyst_data(elt);
		if(oid_add_param(oid, entry->value, entry->length) == 0)
		{
			DTNMP_DEBUG_ERR("oid_add_params","Unable to add a parameter.",NULL);
			DTNMP_DEBUG_EXIT("oid_add_params","-->0", NULL);
			return 0;
		}
	}

	DTNMP_DEBUG_EXIT("oid_add_params","-->1", NULL);
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: oid_calc_size
 *
 * \par Purpose: Calculate size of the OID once serialized. THe serialized OID
 *      does *not* include the OID type, but will include the following info
 *      based on type:
 *      - # Bytes in the OID  (always)
 *      - # Parms (for any parameterized OID)
 *      - SDNV per parm (for any parameterized OID)
 *		- Nickname (for any compressed OID).
 *		Therefore, the largest possible OID has the following form:
 *
 *     <..opt..> <..........required........> <..........optional..........>
 *	   +--------+-------+-------+     +------+--------+-------+     +-------+
 *	   |Nickname|# Bytes| Byte 1| ... |Byte N| # Parms| Parm 1| ... | Parm N|
 *	   | (SDNV) | (SDNV)| (Byte)|     |(Byte)| (SDNV) | (SDNV)|     |(SDNV) |
 *	   +--------+-------+-------+     +------+--------+-------+     +-------+
 *
 * \retval 0 - Error
 *         >0 The serialized size of the OID.
 *
 * \param[in] oid        The OID whose serialized size is to be calculated.
 * \param[in] expand_nn  Whether to expand the nickname (!0) or not (0).
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *****************************************************************************/

uint32_t oid_calc_size(oid_t *oid)
{
	uint32_t size = 0;
	Sdnv tmp;
	datacol_entry_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("oid_calc_size","(%#llx)",(unsigned long)oid);

	/* Step 0: Sanity Check. */
	if(oid == NULL)
	{
		DTNMP_DEBUG_ERR("oid_calc_size","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("oid_calc_size","->0",NULL);
		return 0;
	}

	/* Step 1: How big is the nickname portion of the OID? */
	if((oid->type == OID_TYPE_COMP_FULL) || (oid->type == OID_TYPE_COMP_PARAM))
	{
		encodeSdnv(&tmp, oid->nn_id);
		size += tmp.length;
	}

	/* Step 2: Add the # bytes SDNV and the OID data size. */
	size += oid->value_size;

	DTNMP_DEBUG_INFO("oid_calc_size","val is %d\n", oid->value_size);

	/*
	 * Step 3: If we have parameters, add them too. This is simple because we
	 *         keep the serialized parameters around, including the SDNV at
	 *         the beginning precisely to make subsequent serialization (and
	 *         sizing) calculations easier.
	 */
	if(lyst_length(oid->params) > 0)
	{
		LystElt elt;

		/* Step 3a: Make room for number of parameters. */
		encodeSdnv(&tmp, lyst_length(oid->params));
		size += tmp.length;

		/* Step 3b: Add room for each parameter, which is a # bytes, followed
		 * by the bytes.
		 */
		for(elt = lyst_first(oid->params); elt; elt = lyst_next(elt))
		{
	    	entry = (datacol_entry_t *) lyst_data(elt);
	    	if(entry != NULL)
	    	{
	    		encodeSdnv(&tmp, entry->length);
	    		/*/todo update spec: parms is # bytes, followed by bytes. */
	    		size += tmp.length + entry->length;
	    	}
		}
	}

	DTNMP_DEBUG_EXIT("oid_calc_size","->%d",size);
	return size;
}


/******************************************************************************
 *
 * \par Function Name: oid_clear
 *
 * \par Purpose: Resets the values associated with an OID structure.
 *
 * \retval void
 *
 * \param[in,out]  oid  The OID being cleared.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *  08/29/15  E. Birrane     Cleanup from valgrind.
 *****************************************************************************/

void oid_clear(oid_t *oid)
{

    DTNMP_DEBUG_ENTRY("oid_clear","(%#llx)", (unsigned long) oid);

    /* Step 0: Sanity Check. */
    if(oid == NULL)
    {
        DTNMP_DEBUG_ERR("oid_clear","Clearing NULL OID.", NULL);
        DTNMP_DEBUG_EXIT("oid_clear","<->",NULL);
        return;
    }

    /* Step 1: Free parameters list, if they exist. */
    if(oid->params != NULL)
    {
    	dc_destroy(&(oid->params));
    }

    DTNMP_DEBUG_EXIT("oid_clear","<->", NULL);
}



/******************************************************************************
 *
 * \par Function Name: oid_compare
 *
 * \par Purpose: Determines equivalence of two OIDs.
 *
 * \retval  !0 : Not equal (including error)
 * 			 0 : oid1 == oid2
 *
 * \param[in]  oid1       First OID being compared.
 * \param[in]  oid2       Second OID being compared.
 * \param[in]  use_parms  Whether to compare parms as well.
 *
 * \par Notes:
 *		1. This function should only check for equivalence (== 0), not order
 *         since we do not differentiate between oid1 < oid2 and error.
 *      2. Can compare a non-NULL OID and a NULL OID. They will not be
 *         equivalent.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *  06/04/15  E. Birrane     Consider NN when comparing.
 *****************************************************************************/
int oid_compare(oid_t *oid1, oid_t *oid2, uint8_t use_parms)
{
	int result = 0;

    DTNMP_DEBUG_ENTRY("oid_compare","(%#llx, %#llx)",
    		         (unsigned long) oid1,
    		         (unsigned long) oid2);

    /* Step 0: Sanity check and shallow comparison in one. */

    if(((oid1 == NULL) || (oid2 == NULL)) ||
       ((oid1->value_size != oid2->value_size)) ||
       ((oid1->nn_id != oid2->nn_id)) ||
       ((oid1->type != oid2->type)))
    {
        DTNMP_DEBUG_EXIT("oid_compare","->-1.", NULL);
        return -1;
    }
    
    if(use_parms != 0)
    {
    	if(lyst_length(oid1->params) != lyst_length(oid2->params))
    	{
    		DTNMP_DEBUG_EXIT("oid_compare","->-1.", NULL);
    		return -1;
    	}
    }

    /* Step 1: Compare the value version of the oid */
    result = memcmp(oid1->value, oid2->value, oid1->value_size);

    /* Step 2: If OIDs match so far, and if there are parameters, then we
     *         need to check the parameters...
     */
    if(use_parms != 0)
    {
    	if((result == 0) && (lyst_length(oid1->params) > 0))
    	{
    		result = dc_compare(oid1->params, oid2->params);
    	}
    }

    DTNMP_DEBUG_EXIT("oid_compare","->%d.", result);

    return result;
}




/******************************************************************************
 *
 * \par Function Name: oid_construct
 *
 * \par Purpose: Create an OID from parameters.
 *
 * \retval  NULL: The construct failed
 *         !NULL: The constructed OID.
 *
 * \param[in]  type    The type of the OID.
 * \param[in]  parms   Parameters for the OID, if any.
 * \param[in]  nn_id   Nickname for the OID, if any.
 * \param[in]  value   OID bytes
 * \param[in]  size    Size of value bytes.
 *
 * \par Notes:
 *		1. The lyst and value are deep-copied.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *****************************************************************************/
oid_t* oid_construct(uint8_t type, Lyst parms, uvast nn_id, uint8_t *value, uint32_t size)
{
	oid_t *result = NULL;

    DTNMP_DEBUG_ENTRY("oid_construct",
    		          "(%d, "UVAST_FIELDSPEC","UVAST_FIELDSPEC","UVAST_FIELDSPEC",%d)",
					  type, (uvast)parms, nn_id, (uvast)value, size);

    /* Step 0: Sanity checks. */
    if((value == NULL) || (size == 0))
    {
    	DTNMP_DEBUG_ERR("oid_construct","Bad args.", NULL);
        DTNMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
    	return NULL;
    }
    else if(size > MAX_OID_SIZE)
    {
    	DTNMP_DEBUG_ERR("oid_construct","New OID size %d > MAX OID size %d.",
    					size, MAX_OID_SIZE);
        DTNMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
    	return NULL;
    }

    /* Step 1: Allocate the new OID. */
    if((result = (oid_t *) MTAKE(sizeof(oid_t))) == NULL)
    {
    	DTNMP_DEBUG_ERR("oid_construct","Can't allocate OID.", NULL);
        DTNMP_DEBUG_EXIT("oid_construct", "-->"UVAST_FIELDSPEC, result);
    	return result;
    }

    /* Step 2: Assign the OID Type. */
    result->type = type;

    /* Step 3: Assign the NN, checking if one is required.
     *
     * \todo: Defer this check, as nn_id 0 is valid for now...
    if((type == OID_TYPE_COMP_FULL) ||
       (type == OID_TYPE_COMP_PARAM))
    {
    	if(nn_id == 0)
    	{
    		DTNMP_DEBUG_ERR("oid_construct", "Expected nn with type %d", type);
    		MRELEASE(result);
            DTNMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
        	return NULL;
    	}
    }
*/

    result->nn_id = nn_id;

    /* Step 4: Assign parameters, if required. */
    if((type == OID_TYPE_PARAM) ||
       (type == OID_TYPE_COMP_PARAM))
    {
    	if(parms == NULL)
    	{
    		DTNMP_DEBUG_ERR("oid_construct", "Expected parms with type %d", type);
    		MRELEASE(result);
            DTNMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
        	return NULL;
    	}
    	if((result->params = dc_copy(parms)) == NULL)
    	{
    		DTNMP_DEBUG_ERR("oid_construct", "Can't copy parms.", NULL);
    		MRELEASE(result);
            DTNMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
        	return NULL;
    	}
    }
    else
    {
    	result->params = NULL;
    }

    /* Step 5: Assign OID bytes. */
    bzero(result->value, MAX_OID_SIZE);
    memcpy(result->value, value, size);

    result->value_size = size;

    DTNMP_DEBUG_EXIT("oid_construct", "-->"UVAST_FIELDSPEC, result);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: oid_copy
 *
 * \par Purpose: Duplicates an OID.
 *
 * \retval  NULL: The copy failed
 *         !NULL: The deep-copied OID.
 *
 * \param[in]  src_oid  The OID being copied.
 *
 * \par Notes:
 *		1. The desintation OID is allocated and must be released when done.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *****************************************************************************/
oid_t *oid_copy(oid_t *src_oid)
{
    oid_t *result = NULL;

    DTNMP_DEBUG_ENTRY("oid_copy","(%#llx)", (unsigned long) src_oid);

    /* Step 0: Sanity Check */
    if(src_oid == NULL)
    {
        DTNMP_DEBUG_ERR("oid_copy","Cannot copy from NULL source OID.", NULL);
        DTNMP_DEBUG_EXIT("oid_copy","->NULL",NULL);
        return NULL;
    }

    /* Step 1: Allocate the new OID. */
    if((result = (oid_t*) MTAKE(sizeof(oid_t))) == NULL)
    {
        DTNMP_DEBUG_ERR("oid_copy","Can't allocate %d bytes.", sizeof(oid_t));
        DTNMP_DEBUG_EXIT("oid_copy","->NULL",NULL);
        return NULL;
    }

    /* Step 2: Deep copy parameters. */
    result->params = dc_copy(src_oid->params);

    /* Step 3: Shallow copies the rest. */
    result->type = src_oid->type;
    result->nn_id = src_oid->nn_id;
    result->value_size = src_oid->value_size;
    memcpy(result->value, src_oid->value, result->value_size);

    DTNMP_DEBUG_EXIT("oid_copy","->%d", result);
    return result;
}




/******************************************************************************
 *
 * \par Function Name: oid_deserialize_comp
 *
 * \par Purpose: Extracts a compressed OID from a buffer.
 *
 * \retval  NULL  - Failure.
 * 		   !NULL - The extracted OID.
 *
 * \param[in]  buffer      The effective start of the buffer.
 * \param[in]  size        The size of the buffer, in bytes. Don't go past this.
 * \param[out] bytes_used  The number of consumed buffer bytes.
 *
 * \par Notes:
 *		1. The returned OID is dynamically allocated and must be freed when
 *		   no longer needed.
 *		2. The caller must increment the buffer pointed by the number of bytes
 *		   used before trying to deserialize the next thing in the buffer.
 *		3. We don't validate the oid here because if the oid is bad, we may
 *		   need to plow through anyway to get to the next object in the buffer,
 *		   so bailing early helps no-one.  However, it behooves the caller to
 *		   check that sanity of the OID with a call to oid_sanity_check.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *****************************************************************************/

oid_t *oid_deserialize_comp(unsigned char *buffer,
		                    uint32_t size,
		                    uint32_t *bytes_used)
{
	uvast nn_id = 0;
	uint32_t bytes = 0;
	oid_t *new_oid = NULL;
	unsigned char *cursor = NULL;

	DTNMP_DEBUG_ENTRY("oid_deserialize_comp","(%#llx,%d,%#llx)",
					  (unsigned long) buffer,
					  size,
					  (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((buffer == NULL) || (bytes_used == NULL) || (size == 0))
	{
		DTNMP_DEBUG_ERR("oid_deserialize_comp", "Bad args.", NULL);
		DTNMP_DEBUG_EXIT("oid_deserialize_comp", "->NULL", NULL);
		return NULL;
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/* Step 1: Grab the nickname, which is in an SDNV. */
	 if((bytes = utils_grab_sdnv(cursor, size, &nn_id)) <= 0)
	 {
		 DTNMP_DEBUG_ERR("oid_deserialize_comp", "Can't grab nickname.", NULL);
		 *bytes_used = 0;

		 DTNMP_DEBUG_EXIT("oid_deserialize_comp", "->0", NULL);
		 return 0;
	 }
	 else
	 {
		 cursor += bytes;
		 size -= bytes;
		 *bytes_used += bytes;
	 }


	/* Step 2: grab the remaining OID, which looks just like a full OID. */
	if((new_oid = oid_deserialize_full(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_comp", "Can't grab remaining OID.", NULL);
		*bytes_used = 0;

		DTNMP_DEBUG_EXIT("oid_deserialize_comp", "->NULL", NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Store extra information in the new OID. */
	new_oid->nn_id = nn_id;
	new_oid->type = OID_TYPE_COMP_FULL;


	DTNMP_DEBUG_EXIT("oid_deserialize_comp","-> %x.", (unsigned long)new_oid);
	return new_oid;
}



/******************************************************************************
 *
 * \par Function Name: oid_deserialize_comp_param
 *
 * \par Purpose: Extracts a compressed, parameterized OID from a buffer.
 *
 * \retval  NULL  - Failure.
 * 		   !NULL - The extracted OID.
 *
 * \param[in]  buffer      The effective start of the buffer.
 * \param[in]  size        The size of the buffer, in bytes. Don't go past this.
 * \param[out] bytes_used  The number of consumed buffer bytes.
 *
 * \par Notes:
 *		1. The returned OID is dynamically allocated and must be freed when
 *		   no longer needed.
 *		2. The caller must increment the buffer pointed by the number of bytes
 *		   used before trying to deserialize the next thing in the buffer.
 *		3. We don't validate the oid here because if the oid is bad, we may
 *		   need to plow through anyway to get to the next object in the buffer,
 *		   so bailing early helps no-one.  However, it behooves the caller to
 *		   check that sanity of the OID with a call to oid_sanity_check.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *****************************************************************************/
oid_t *oid_deserialize_comp_param(unsigned char *buffer,
    					         uint32_t size,
    					         uint32_t *bytes_used)
{
	uvast nn_id = 0;
	uint32_t bytes = 0;
	oid_t *new_oid = NULL;
	unsigned char *cursor = NULL;

	DTNMP_DEBUG_ENTRY("oid_deserialize_comp_param","(%#llx,%d,%#llx)",
					  (unsigned long) buffer,
					  size,
					  (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((buffer == NULL) || (bytes_used == NULL) || (size == 0))
	{
		DTNMP_DEBUG_ERR("oid_deserialize_comp_param", "Bad args.", NULL);
		DTNMP_DEBUG_EXIT("oid_deserialize_comp_param", "->NULL", NULL);
		return NULL;
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/* Step 1: Grab the nickname, which is in an SDNV. */
	 if((bytes = utils_grab_sdnv(cursor, size, &nn_id)) <= 0)
	 {
		 DTNMP_DEBUG_ERR("oid_deserialize_comp_param", "Can't grab nickname.", NULL);
		 *bytes_used = 0;

		 DTNMP_DEBUG_EXIT("oid_deserialize_comp_param", "->0", NULL);
		 return NULL;
	 }
	 else
	 {
		 cursor += bytes;
		 size -= bytes;
		 *bytes_used += bytes;
	 }


	/* Step 2: grab the parameterized, remaining OID. */
	if((new_oid = oid_deserialize_param(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_comp_param", "Can't grab remaining OID.",
				        NULL);
		*bytes_used = 0;

		DTNMP_DEBUG_EXIT("oid_deserialize_comp_param", "->NULL", NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Store extra information in the new OID. */
	new_oid->nn_id = nn_id;
	new_oid->type = OID_TYPE_COMP_PARAM;

	DTNMP_DEBUG_EXIT("oid_deserialize_comp_param","-> %x.", (unsigned long)new_oid);
	return new_oid;
}



/******************************************************************************
 *
 * \par Function Name: oid_deserialize_full
 *
 * \par Purpose: Extracts a regular OID from a buffer.
 *
 * 	\todo We currently assume that the length field is a single byte (0-127).
 * 	      We need to code this to follow BER rules for large definite data
 * 	      form. Like SDNV but not quite: if high-bit of first byte set, bits
 * 	      7-1 give # octets that comprise length. Then, concatenate, big-endian,
 * 	      those octets to build	length. Ex: length 435 = 0x8201B3
 *
 * \retval  NULL  - Failure.
 * 		   !NULL - The extracted OID.
 *
 * \param[in]  buffer      The effective start of the buffer.
 * \param[in]  size        The size of the buffer, in bytes. Don't go past this.
 * \param[out] bytes_used  The number of consumed buffer bytes.
 *
 * \par Notes:
 *		1. The returned OID is dynamically allocated and must be freed when
 *		   no longer needed.
 *		2. The caller must increment the buffer pointed by the number of bytes
 *		   used before trying to deserialize the next thing in the buffer.
 *		3. We don't validate the oid here because if the oid is bad, we may
 *		   need to plow through anyway to get to the next object in the buffer,
 *		   so bailing early helps no-one.  However, it behooves the caller to
 *		   check that sanity of the OID with a call to oid_sanity_check.
 *		4. We DO bail if the OID is larger than we can accommodate, though.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *****************************************************************************/
oid_t *oid_deserialize_full(unsigned char *buffer,
			                uint32_t size,
			                uint32_t *bytes_used)
{
	uint32_t oid_size = 0;
	oid_t *new_oid = NULL;
	uint8_t val = 0;
	unsigned char *cursor = NULL;

	DTNMP_DEBUG_ENTRY("oid_deserialize_full","(%#llx,%d,%#llx)",
			  (       unsigned long) buffer, size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((buffer == NULL) || (bytes_used == NULL) || (size == 0))
	{
		DTNMP_DEBUG_ERR("oid_deserialize_full", "Bad args.", NULL);
		DTNMP_DEBUG_EXIT("oid_deserialize_full", "->NULL", NULL);
		return NULL;
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}


	/*
	 * Step 1: Grab # SDNVs in the OID
	 * \todo: Check if this should be a byte or an SDNV.
	 */
	if((*bytes_used = utils_grab_byte(cursor, size, &val)) != 1)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_full", "Can't grab size byte.", NULL);
		DTNMP_DEBUG_EXIT("oid_deserialize_full", "-> NULL", NULL);
		return NULL;
	}
	else
	{
		cursor += *bytes_used;
		size -= *bytes_used;

		oid_size = val;
	}

	/*
	 * Step 2: Make sure OID fits within our size. We add 1 to oid_size to
	 *         account for the # bytes parameter.
	 * \todo Change the 1 here to something else if me move to SDNV for #bytes.
	 */
	if((oid_size + 1) >= MAX_OID_SIZE)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_full","Size %d exceeds supported size %d",
				        oid_size + 1, MAX_OID_SIZE);
		*bytes_used = 0;

		DTNMP_DEBUG_EXIT("oid_deserialize_full","-> NULL", NULL);
		return NULL;
	}

	/* Step 3: Make sure we have oid_size bytes left in the buffer. */
	if(oid_size > size)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_full","Can't read %d bytes from %d sized buf.",
				        oid_size, size);

		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("oid_deserialize_full","-> NULL", NULL);
		return NULL;
	}

	/* Step 4: Allocate the target OID object. */
	if((new_oid = (oid_t*)MTAKE(sizeof(oid_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_full","Cannot allocate new OID.", NULL);

		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("oid_deserialize_full","-> NULL", NULL);
		return NULL;
	}

	/* Step 5: Copy in the data contents of the OID.  We don't interpret the
	 *         values here, just put them in.
	 */
	new_oid->value[0] = oid_size;
	memcpy(&(new_oid->value[1]), cursor, oid_size);
	new_oid->value_size = oid_size + 1;
	*bytes_used += oid_size;

	if((new_oid->params = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_deserialize_full","Cannot allocate param lyst.", NULL);
		MRELEASE(new_oid);
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("oid_deserialize_full","-> NULL", NULL);
		return NULL;
	}

	new_oid->type = OID_TYPE_FULL;

	DTNMP_DEBUG_EXIT("oid_deserialize_full","-> new_oid: %#llx  bytes_used %d",
			         (unsigned long)new_oid, *bytes_used);

	return new_oid;
}



/******************************************************************************
 *
 * \par Function Name: oid_deserialize_param
 *
 * \par Purpose: Extracts a parameterized OID from a buffer.
 *
 * \retval  NULL  - Failure.
 * 		   !NULL - The extracted OID.
 *
 * \param[in]  buffer      The effective start of the buffer.
 * \param[in]  size        The size of the buffer, in bytes. Don't go past this.
 * \param[out] bytes_used  The number of consumed buffer bytes.
 *
 * \par Notes:
 *		1. The returned OID is dynamically allocated and must be freed when
 *		   no longer needed.
 *		2. The caller must increment the buffer pointed by the number of bytes
 *		   used before trying to deserialize the next thing in the buffer.
 *		3. We don't validate the oid here because if the oid is bad, we may
 *		   need to plow through anyway to get to the next object in the buffer,
 *		   so bailing early helps no-one.  However, it behooves the caller to
 *		   check that sanity of the OID with a call to oid_sanity_check.
 *		4. We DO bail if the OID is larger than we can accommodate, though.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *****************************************************************************/

oid_t *oid_deserialize_param(unsigned char *buffer,
		                     uint32_t size,
		                     uint32_t *bytes_used)
{
	oid_t *new_oid;
	uint32_t bytes = 0;
	uvast value = 0;
	uint32_t idx = 0;
	unsigned char *cursor = NULL;

	DTNMP_DEBUG_ENTRY("oid_deserialize_param","(%#llx,%d,%#llx)",
					  (unsigned long) buffer,
					  size,
					  (unsigned long) bytes_used);

	/* Step 0: Sanity Checks... */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		DTNMP_DEBUG_ERR("oid_deserialize_param","NULL input values!",NULL);
		DTNMP_DEBUG_EXIT("oid_deserialize_param","->NULL",NULL);
		return NULL;
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/* Step 1: Grab the regular OID and store it. */
	new_oid = oid_deserialize_full(cursor, size, &bytes);

	if((bytes == 0) || (new_oid == NULL))
	{
		DTNMP_DEBUG_ERR("oid_deserialize_param","Could not grab OID.", NULL);

		oid_release(new_oid); /* release just in case bytes was 0. */
		*bytes_used = 0;

		DTNMP_DEBUG_EXIT("oid_deserialize_param","-> NULL", NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 2: Grab the data collection representing parameters. Only if there
	 *         are parameters to grab.
	 */
	if(size > 0)
	{
		if((new_oid->params = dc_deserialize(cursor, size, &bytes)) == NULL)
		{
			DTNMP_DEBUG_ERR("oid_deserialize_param","Could not grab params.", NULL);

			oid_release(new_oid); /* release just in case bytes was 0. */
			*bytes_used = 0;

			DTNMP_DEBUG_EXIT("oid_deserialize_param","-> NULL", NULL);
			return NULL;
		}
		else
		{
			cursor += bytes;
			size -= bytes;
			*bytes_used += bytes;
		}
	}

	/* Step 3: Fill in any other parts of the OID. */
	new_oid->type = OID_TYPE_PARAM;

	DTNMP_DEBUG_EXIT("oid_deserialize_param","-> %#llx.", (unsigned long) new_oid);
	return new_oid;
}

uint8_t  oid_get_num_parms(oid_t *oid)
{
	DTNMP_DEBUG_ENTRY("oid_get_num_parms","(%#llx)", (unsigned long) oid);

	/* Step 1 - Sanity Check. */
	if(oid == NULL)
	{
		DTNMP_DEBUG_ERR("oid_get_num_parms","NULL OID.",NULL);
		DTNMP_DEBUG_EXIT("oid_get_num_parms","->NULL.",NULL);
		return 0;
	}

	/* Step 2 - Make sure this OID has parameters. */
	if(oid->params == NULL)
	{
		DTNMP_DEBUG_ERR("oid_get_num_parms","OID has no parameters.",NULL);
		DTNMP_DEBUG_EXIT("oid_get_num_parms","->NULL.",NULL);
		return 0;
	}

	return lyst_length(oid->params);
}


/******************************************************************************
 *
 * \par Function Name: oid_get_param
 *
 * \par Purpose: Retrieves the ith parameter of a parameterized OID.
 *
 * \retval  NULL: Could not find parameter.
 *         !NULL: The datacol entry representing this parameter.
 *
 * \param[in]  oid  The OID whose parameter is being queried.
 * \param[in]  i    Which parameter to retrieve. 0 based.
 *
 * \par Notes:
 *
 *****************************************************************************/

datacol_entry_t *oid_get_param(oid_t *oid, int i)
{
	datacol_entry_t *result = NULL;
	LystElt elt = 0;

	DTNMP_DEBUG_ENTRY("oid_get_param","(%#llx, %d)", (unsigned long) oid, i);

	/* Step 1 - Sanity Check. */
	if(oid == NULL)
	{
		DTNMP_DEBUG_ERR("oid_get_param","NULL OID.",NULL);
		DTNMP_DEBUG_EXIT("oid_get_param","->NULL.",NULL);
		return NULL;
	}

	/* Step 2 - Make sure this OID has parameters, and enough. */
	if(oid->params == NULL)
	{
		DTNMP_DEBUG_ERR("oid_get_param","OID has no parameters.",NULL);
		DTNMP_DEBUG_EXIT("oid_get_param","->NULL.",NULL);
		return NULL;
	}

	if(lyst_length(oid->params) <= i)
	{
		DTNMP_DEBUG_ERR("oid_get_param","OID has %d parameters not %d,",lyst_length(oid->params), i);
		DTNMP_DEBUG_EXIT("oid_get_param","->NULL.",NULL);
		return NULL;
	}

	int cur_idx = 0;
	for(elt = lyst_first(oid->params); elt != NULL; elt = lyst_next(elt))
	{
		if(cur_idx == i)
		{
			result = (datacol_entry_t *) lyst_data(elt);
			break;
		}
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_pretty_print
 *
 * \par Purpose: Builds a string representation of the OID suitable for
 *               diagnostic viewing.
 *
 * \retval  NULL: Failure to construct a string representation of the OID.
 *         !NULL: The string representation of the OID string.
 *
 * \param[in]  oid  The OID whose string representation is being created.
 *
 * \par Notes:
 *		1. This is a slow, wasteful function and should not be called in
 *		   embedded systems.  For something to dump in a log, try the function
 *		   oid_to_string instead.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

char *oid_pretty_print(oid_t *oid)
{
	int size = 0;
	char *str = NULL;
	char *result = NULL;
	char *cursor = NULL;
	LystElt elt;
	datacol_entry_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("oid_pretty_print","(%#llx)", (unsigned long) oid);

	/* Step 0: Sanity Check. */
	if(oid == NULL)
	{
		DTNMP_DEBUG_ERR("oid_pretty_print","NULL OID.",NULL);
		DTNMP_DEBUG_EXIT("oid_pretty_print","->NULL.",NULL);
		return NULL;
	}

	/* Step 1: Guestimate size. This is, at best, a dark art and prone to
	 *         exaggeration. Numerics are assumed to take 5 bytes, unless I
	 *         think they will take 3 instead (indices vs. values). Other
	 *         values are based on constant strings in the function. One must
	 *         update this calculation if one changes string constants, else
	 *         one will be sorry.
	 */
	size = 33 +   									 /* Header */
		   11 +   									 /* OID type as text */
		   (lyst_length(oid->params) * 22) +         /* Each parameter index */
		   (MAX_OID_PARM * (20 * MAX_OID_SIZE)) +    /* Parameters */
		   12 +                                      /* nn_id */
		   18 +   								     /* value_size */
		   7 + (oid->value_size * 4) +               /* OID values */
		   22;  									 /* Footer. */


	/* Step 2: Allocate the string. */
	if((result = (char*)MTAKE(size)) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_pretty_print", "Can't alloc %d bytes.", size);
		DTNMP_DEBUG_EXIT("oid_pretty_print","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Populate the allocated string. Keep an eye on the size. */
	cursor = result;
	cursor += sprintf(cursor,"OID:\n---------------------\nType: ");

	switch(oid->type)
	{
	case 0: cursor += sprintf(cursor,"FULL\n"); break;
	case 1: cursor += sprintf(cursor,"PARAM\n"); break;
	case 2: cursor += sprintf(cursor,"COMP_FULL\n"); break;
	case 3: cursor += sprintf(cursor,"COMP_PARAM\n"); break;
	default: cursor += sprintf(cursor,"UNKNOWN\n"); break;
	}

	cursor += sprintf(cursor,"num_parm: %ld\n", lyst_length(oid->params));

	int i = 0;
	for(elt = lyst_first(oid->params); elt; elt = lyst_next(elt))
	{
		entry = (datacol_entry_t*)lyst_data(elt);
		str = utils_hex_to_string(entry->value, entry->length);
		cursor += sprintf(cursor, "Parm %d:%s\n",i,str);
		MRELEASE(str);
		i++;
	}

	cursor += sprintf(cursor, "nn_id: %d\n", (uint32_t)oid->nn_id);
	cursor += printf(cursor, "value_size: %d\n", oid->value_size);

	str = oid_to_string(oid);
	cursor += sprintf(cursor, "value: %s\n---------------------\n\n", str);
	MRELEASE(str);

	/* Step 4: Sanity check. */
	if((cursor - result) > size)
	{
		DTNMP_DEBUG_ERR("oid_pretty_print", "OVERWROTE! Alloc %d, wrote %llu.",
				        size, (cursor-result));
		MRELEASE(result);
		DTNMP_DEBUG_EXIT("oid_pretty_print","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_INFO("oid_pretty_print","Wrote %llu into %d string.",
			         (cursor -result), size);

	DTNMP_DEBUG_EXIT("oid_pretty_print","->%#llx",result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_release
 *
 * \par Purpose: Frees resources associated with an OID.
 *
 * \retval void
 *
 * \param[in,out] oid The OID being freed.
 *
 * \par Notes:
 *		1. Very little to do here as an OID is statically loaded structure.
 *		   Function remains for if/when we refactor OIDs to be more dynamic.
 *		2. The OID pointer is garbage after this call and must not be
 *		   referenced again.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

void oid_release(oid_t *oid)
{
    DTNMP_DEBUG_ENTRY("oid_release", "(%#llx)", (unsigned long) oid);

    if(oid != NULL)
    {
        oid_clear(oid);
        lyst_destroy(oid->params);
        MRELEASE(oid);
        oid = NULL;
    }

    DTNMP_DEBUG_EXIT("oid_release", "-> NULL", NULL);
}



/******************************************************************************
 *
 * \par Function Name: oid_sanity_check
 *
 * \par Purpose: Checks an oid structure for proper values.
 *
 * \retval <=0  - Failure. The OID is not sane.
 * 		   >0   - Success. The OID is sane.
 *
 * \param[in]  oid  The OID whose sanity is in question.
 *
 * \par Notes:
 *		1. We don't bail on the first failure so as to better populate the
 *		   error log in instances where we have an OID with multiple problems.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *  08/29/15  E. Birrane     Allow parms of size > MAX_OID_SIZE
 *****************************************************************************/

int oid_sanity_check(oid_t *oid)
{
	int result = 1;
    DTNMP_DEBUG_ENTRY("oid_sanity_check","(%#llx)", (unsigned long) oid);

	/* NULL Checks. Perform all so that we get lots of logging in case
	 * there are multiple problems with the OID.  */
	if(oid == NULL)
	{
        DTNMP_DEBUG_ERR("oid_sanity_check","NULL oid.", NULL);
        result = 0;
	}

	if(oid->type > 3)
	{
        DTNMP_DEBUG_ERR("oid_sanity_check","Bad type: %d.", oid->type);
        result = 0;
	}

	if(lyst_length(oid->params) > MAX_OID_PARM)
	{
        DTNMP_DEBUG_ERR("oid_sanity_check","Too many params: %d.",
        		         lyst_length(oid->params));
        result = 0;
	}

	if(oid->value_size > MAX_OID_SIZE)
	{
        DTNMP_DEBUG_ERR("oid_sanity_check","Val size %d bigger than max %d",
        			    oid->value_size, MAX_OID_SIZE);
        result = 0;
	}

    DTNMP_DEBUG_EXIT("oid_sanity_check","->%d", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_serialize
 *
 * \par Purpose: Generate full, serialized version of the OID.
 *
 * \todo Add a mechanism to determine whether we should expand a nickname or
 *       not when serializing the OID.
 *
 * \todo Instead of counting exact size, it will be faster to simply allocate
 *       the MAX_OID_SIZE and populate up to that point. Not sure if we are
 *       going to be space or time constrained just yet. While the nn_id lookup
 *       stays expensive we may not want to call it 2x as part of serialization
 *       (once to calculate size and again to grab values).
 *
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized stream.
 *
 * \param[in]  oid        The OID to be serialized.
 * \param[out] size       The size of the resulting serialized OID.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *  08/28/15  E. Birrane     Allow parms > MAX_OID_SIZE.
 *****************************************************************************/

uint8_t *oid_serialize(oid_t *oid, uint32_t *size)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	uint8_t *parms = NULL;
	uint32_t parm_size = 0;

	uint32_t idx = 0;
	Sdnv nn_sdnv;

	DTNMP_DEBUG_ENTRY("oid_serialize","(%#llx,%#llx)",
			          (unsigned long)oid, (unsigned long)size);

	/* Step 0: Sanity Check */
	if((oid == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("oid_serialize","Bad args.",NULL);
		DTNMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Count up the total size of the OID. */
	if((*size = oid_calc_size(oid)) == 0)
	{
		DTNMP_DEBUG_ERR("oid_serialize","Bad size %d.",*size);
		*size = 0;
		DTNMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Allocate the serialized buffer.*/
	if((result = (uint8_t *) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_serialize","Couldn't allocate %d bytes.",
				        *size);
		*size = 0;
		DTNMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor = result;
	}

	/* Step 3: Copy in the nickname portion. */
	if((oid->type == OID_TYPE_COMP_FULL) || (oid->type == OID_TYPE_COMP_PARAM))
	{
		encodeSdnv(&nn_sdnv, oid->nn_id);
		memcpy(cursor, nn_sdnv.text, nn_sdnv.length);
		cursor += nn_sdnv.length;
	}

	/* Step 4: Copy in the value portion. */
	memcpy(cursor,oid->value, oid->value_size);
	cursor += oid->value_size;

	/* Step 5: Copy in the parameters portion. */
	if((oid->type == OID_TYPE_PARAM) || (oid->type == OID_TYPE_COMP_PARAM))
	{
		parms = dc_serialize(oid->params, &parm_size);
		if((parms == NULL) || (parm_size == 0))
		{
			DTNMP_DEBUG_ERR("oid_serialize","Can't serialize parameters.",NULL);
			*size = 0;
			MRELEASE(result);
			DTNMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
			return NULL;
		}

		memcpy(cursor, parms, parm_size);
		cursor += parm_size;
		MRELEASE(parms);
	}

	/* Step 6: Final sanity check */
	if((cursor-result) > *size)
	{
		DTNMP_DEBUG_ERR("oid_serialize","Serialized %d bytes but counted %d!",
				        (cursor-result), *size);
		*size = 0;
		MRELEASE(result);
		DTNMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("oid_serialize","->%#llx",(unsigned long)result);
	return result;
}




/******************************************************************************
 *
 * \par Function Name: oid_to_string
 *
 * \par Purpose: Create a compact string representation of the OID.
 *
 * \retval NULL   - Failure building string version of the OID.
 * 		   !NULL  - The compact string representing the OID.
 *
 * \param[in] oid The OID whose string representation is being calculated.
 *
 * \par Notes:
 *		1. This function allocates memory from the memory pool.  The returned
 *         string MUST be released when no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

char *oid_to_string(oid_t *oid)
{
    char *result = NULL;
    uint32_t size = 0;

    DTNMP_DEBUG_ENTRY("oid_to_string","(%#llx)",(unsigned long) oid);

    /* Step 0: Sanity Check. */
    if(oid == NULL)
    {
        DTNMP_DEBUG_ERR("oid_to_string","NULL OID",NULL);
        DTNMP_DEBUG_EXIT("oid_to_string","->NULL.",NULL);
    	return NULL;
    }

    /* Step 1: Walk through and collect size. */


    /* For now, we just show the raw MID. */
	result = utils_hex_to_string(oid->value, oid->value_size);

    DTNMP_DEBUG_EXIT("oid_to_string","->%s.", result);
	return result;
}








