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
 ** \file oid.c
 **
 ** Description: This file contains the implementation of functions that
 **              operate on Object Identifiers (OIDs). This file also implements
 **              global values, including the OID nickname database.
 **
 ** Notes:
 **	     1. In the current implementation, the nickname database is not
 **	        persistent.
 **	     2. We do not support a "create" function for OIDs as, so far, any
 **	        need to create OIDs can be met by calling the appropriate
 **	        deserialize function.
 **
 ** Assumptions:
 **      1. We limit the size of an OID in the system to reduce the amount
 **         of pre-allocated memory in this embedded system. Non-embedded
 **         implementations may wish to dynamically allocate MIDs as they are
 **         received.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/29/12  E. Birrane     Initial Implementation
 **  11/14/12  E. Birrane     Code review comments and documentation update.
 *****************************************************************************/

#include "platform.h"

#include "shared/utils/utils.h"

#include "shared/primitives/oid.h"


Lyst nn_db;
ResourceLock nn_db_mutex;


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
 *****************************************************************************/
int oid_add_param(oid_t *oid, uint8_t *value, uint32_t len)
{
	int result = 0;
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

	/* Step 3: Make sure we have room for the passed-in parameter. */
	encodeSdnv(&len_sdnv, len);

	if((oid_calc_size(oid) + len) > MAX_OID_SIZE)
	{
		DTNMP_DEBUG_ERR("oid_add_param","Parm too long. OID %d MAX %d Len %d",
				        oid_calc_size(oid), MAX_OID_SIZE, len);
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

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
		DTNMP_DEBUG_ERR("oid_add_param","Can't alloc %d bytes.",
				        entry->length);
		MRELEASE(entry);
		DTNMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	cursor = entry->value;

//	memcpy(cursor, len_sdnv.text, len_sdnv.length);
//	cursor += len_sdnv.length;

	memcpy(cursor, value, len);
	cursor += len;

	lyst_insert_last(oid->params, entry);

	DTNMP_DEBUG_EXIT("oid_add_param","->%d.",result);
	return result;
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

    /* Step 1: Free parameters list. */
    utils_datacol_destroy(&(oid->params));

    /* Step 2: Bzero rest of the OID. */
    memset(oid, 0, sizeof(oid_t));

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
    		result = utils_datacol_compare(oid1->params, oid2->params);
    	}
    }

    DTNMP_DEBUG_EXIT("oid_compare","->%d.", result);

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
    result->params = utils_datacol_copy(src_oid->params);

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
		if((new_oid->params = utils_datacol_deserialize(cursor, size, &bytes)) == NULL)
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

	uint32_t size = oid_calc_size(oid);
	if(size > MAX_OID_SIZE)
	{
        DTNMP_DEBUG_ERR("oid_sanity_check","Parm size %d bigger than max %d",
        			    size, MAX_OID_SIZE);
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

	/* Step 2: Sanity check the size. */
	if(*size > MAX_OID_SIZE)
	{
		DTNMP_DEBUG_ERR("oid_serialize","Size %d bigger than max of %d.",
				        *size, MAX_OID_SIZE);
		*size = 0;
		DTNMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Allocate the serialized buffer.*/
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

	/* Step 4: Copy in the nickname portion. */
	if((oid->type == OID_TYPE_COMP_FULL) || (oid->type == OID_TYPE_COMP_PARAM))
	{
		encodeSdnv(&nn_sdnv, oid->nn_id);
		memcpy(cursor, nn_sdnv.text, nn_sdnv.length);
		cursor += nn_sdnv.length;
	}

	/* Step 5: Copy in the value portion. */
	memcpy(cursor,oid->value, oid->value_size);
	cursor += oid->value_size;

	/* Step 6: Copy in the parameters portion. */
	if((oid->type == OID_TYPE_PARAM) || (oid->type == OID_TYPE_COMP_PARAM))
	{
		parms = utils_datacol_serialize(oid->params, &parm_size);
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

	/* Step 7: Final sanity check */
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



/******************************************************************************
 *
 * \par Function Name: oid_nn_add
 *
 * \par Purpose: Adds a nickname to the database.
 *
 * \retval 0 Failure.
 *         !0 Success.
 *
 * \param[in] oid The OID whose string representation is being calculated.
 *
 * \par Notes:
 *		1. We will allocate our own entry on addition, the passed-in structure
 *		   may be deleted or changed after this call.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/
int oid_nn_add(oid_nn_t *nn)
{
	oid_nn_t *new_nn = NULL;

	DTNMP_DEBUG_ENTRY("oid_nn_add","(%#llx)",(unsigned long)nn);

	/* Step 0: Sanity check. */
	if(nn == NULL)
	{
		DTNMP_DEBUG_ERR("oid_nn_add","Bad args.",NULL);
		DTNMP_DEBUG_EXIT("oid_nn_add","->0",NULL);
		return 0;
	}

	/* Need to lock early so our uniqueness check (step 1) doesn't change by
	 * the time we go to insert in step 4. */
	lockResource(&nn_db_mutex);

	/* Step 1: Make sure entry doesn't already exist. */
	if(oid_nn_exists(nn->id))
	{
		DTNMP_DEBUG_ERR("oid_nn_add","Id 0x%x already exists in db.",
				         nn->id);

		unlockResource(&nn_db_mutex);

		DTNMP_DEBUG_EXIT("oid_nn_add","->0",NULL);
		return 0;
	}

	/* Step 2: Allocate new entry. */
	if ((new_nn = (oid_nn_t*)MTAKE(sizeof(oid_nn_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_nn_add","Can't take %d bytes for new nn.",
						sizeof(oid_nn_t));

		unlockResource(&nn_db_mutex);

		DTNMP_DEBUG_EXIT("oid_nn_add","->0",NULL);
		return 0;
	}

	/* Step 3: Populate new entry with passed-in data. */
	memcpy(new_nn, nn, sizeof(oid_nn_t));


	/* Step 4: Add new entry to the db. */

	lyst_insert_first(nn_db, new_nn);
    unlockResource(&nn_db_mutex);


	DTNMP_DEBUG_EXIT("oid_nn_add","->1",NULL);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_cleanup
 *
 * \par Purpose: Breaks down the nickname database.
 *
 * \retval void
 *
 * \par Notes:
 *		1.  We assume there are no other people who will use the nn_db after
 *		    this!
 *		2.  We also kill the mutex around the database.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

void oid_nn_cleanup()
{
    LystElt elt;
    oid_nn_t *entry = NULL;

    DTNMP_DEBUG_ENTRY("oid_nn_cleanup","()",NULL);

    /* Step 0: Sanity Check. */
    if(nn_db == NULL)
    {
    	DTNMP_DEBUG_WARN("oid_nn_cleanup","NN database already cleaned.",NULL);
    	return;
    }


    /* Step 1: Wait for folks to be done with the database. */
    lockResource(&nn_db_mutex);

    /* Step 2: Release each entry. */
    for (elt = lyst_first(nn_db); elt; elt = lyst_next(elt))
    {
    	entry = (oid_nn_t*) lyst_data(elt);
    	if (entry != NULL)
    	{
    		MRELEASE(entry);
    	}
    	else
    	{
    		DTNMP_DEBUG_WARN("oid_nn_cleanup","Found NULL entry in nickname db.",
    				         NULL);
    	}
    }
    lyst_destroy(nn_db);

    /* Step 3: Clean up locking mechanisms too. */
    unlockResource(&nn_db_mutex);
    killResourceLock(&nn_db_mutex);
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_delete
 *
 * \par Purpose: Removes a nickname from the database.
 *
 * \retval 0 - Entry not found (or other error)
 * 		   !0 - Success.
 *
 * \param[in] nn_id The ID of the entry to remove.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/
int oid_nn_delete(uvast nn_id)
{
	oid_nn_t *cur_nn = NULL;
	LystElt tmp_elt;
	int result = 0;

	DTNMP_DEBUG_ENTRY("oid_nn_delete","(%#llx)",nn_id);

	/* Step 1: Need to lock to preserve validity of the lookup result. . */
	lockResource(&nn_db_mutex);

	/* Step 2: If you find it, delete it. */
	if((tmp_elt = oid_nn_exists(nn_id)) != NULL)
	{
    	cur_nn = (oid_nn_t*) lyst_data(tmp_elt);
		lyst_delete(tmp_elt);
		MRELEASE(cur_nn);
		result = 1;
	}

    unlockResource(&nn_db_mutex);

	DTNMP_DEBUG_EXIT("oid_nn_delete","->%d",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_exists
 *
 * \par Purpose: Determines if an ID is in the nickname db.
 *
 * \retval NULL - Not found.
 * 		   !NULL - ELT pointing to the found element.
 *
 * \param[in] nn_id The ID of the nickname whose existence is in question.
 *
 * \todo There is probably a smarter way to do this with a lyst find function
 * 	     and a search callback.
 *
 * \par Notes:
 *		1. LystElt is a pointer. Handle this handle with care.
 *		2. We break abstraction here and return a Lyst structure because this
 *		   lookup function is often called in the context of lyst maintenance,
 *		   but if we were to change the underlying nickname database storage
 *		   approach, this function would, clearly, need to change. Those who
 *		   do not like this approach are referred to the much less deprecable
 *		   function: oid_find.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/
LystElt oid_nn_exists(uvast nn_id)
{
	oid_nn_t *cur_nn = NULL;
	LystElt tmp_elt = NULL;
	LystElt result = NULL;

	DTNMP_DEBUG_ENTRY("oid_nn_exists","(%#llx)",nn_id);

	/* Step 0: Make sure no +/-'s while we search. */
	lockResource(&nn_db_mutex);

    for(tmp_elt = lyst_first(nn_db); tmp_elt; tmp_elt = lyst_next(tmp_elt))
    {
    	cur_nn = (oid_nn_t*) lyst_data(tmp_elt);
    	if(cur_nn != NULL)
    	{
    		if(cur_nn->id == nn_id)
    		{
    			result = tmp_elt;
    			break;
    		}
    	}
    	else
    	{
    		DTNMP_DEBUG_WARN("oid_nn_exists","Encountered NULL nn?",NULL);
    	}
    }

    unlockResource(&nn_db_mutex);

	DTNMP_DEBUG_EXIT("oid_nn_delete","->%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_find
 *
 * \par Purpose: Convenience function to grab a nickname entry from its ID.
 *
 * \retval NULL - Item not found.
 *         !NULL - Handle to the found item.
 *
 * \param[in] nn_id  The ID of the nickname to find.
 *
 * \todo There is probably a smarter way to do this with a lyst find function
 * 	     and a search callback.
 *
 * \par Notes:
 *		1. The returned pointer should NOT be released. It points directly into
 *		   the nickname list.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

oid_nn_t* oid_nn_find(uvast nn_id)
{
	LystElt tmpElt = NULL;
	oid_nn_t *result = NULL;

	DTNMP_DEBUG_ENTRY("oid_nn_find","(%#llx)",nn_id);

	/* Step 0: Call exists function (exists should block mutex, so we don't. */
	if((tmpElt = oid_nn_exists(nn_id)) != NULL)
	{
		result = (oid_nn_t*) lyst_data(tmpElt);
	}

	DTNMP_DEBUG_EXIT("oid_nn_find","->%#llx",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_init
 *
 * \par Purpose: Initialize the nickname database for OIDs.
 *
 * \retval <0 - Failure.
 * 		    0 - Success.
 *
 * \param[in] nn_id  The ID of the nickname to find.
 *
 * \todo Add functions here to read the nickname database from persistent
 *       storage, such as an SDR.
 *
 * \par Notes:
 *		1. nn_db must only hold items that have been dynamically allocated
 *		   from the	memory pool.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

int oid_nn_init()
{
	DTNMP_DEBUG_ENTRY("oid_init_nn_db","()",NULL);

	/* Step 0: Sanity Check. */
	if(nn_db != NULL)
	{
		DTNMP_DEBUG_WARN("oid_nn_init","Trying to init existing NN db.",NULL);
		return 0;
	}

	if((nn_db = lyst_create()) == NULL)
	{
		DTNMP_DEBUG_ERR("oid_nn_init","Can't allocate NN DB!", NULL);
		DTNMP_DEBUG_EXIT("oid_nn_init","->-1.",NULL);
		return -1;
	}

	if(initResourceLock(&nn_db_mutex))
	{
        DTNMP_DEBUG_ERR("oid_init_nn_db","Unable to initialize mutex, errno = %s",
        		        strerror(errno));
        DTNMP_DEBUG_EXIT("oid_init_nn_db","->-1.",NULL);
        return -1;
	}

    DTNMP_DEBUG_EXIT("oid_init_nn_db","->0.",NULL);
	return 0;
}













