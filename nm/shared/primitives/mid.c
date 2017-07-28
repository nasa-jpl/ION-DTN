/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
  ******************************************************************************/

/*****************************************************************************
 **
 ** \file mid.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of DTNMP Managed Identifiers (MIDs).
 **
 ** Notes:
 ** \todo 1. Need to make sure that all serialize and deserialize methods
 **          do regular bounds checking whenever writing and often when
 **          reading.
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
 **  10/21/11  E. Birrane     Code comments and functional updates. (JHU/APL)
 **  10/22/12  E. Birrane     Update to latest version of DTNMP. Cleanup. (JHU/APL)
 **  06/25/13  E. Birrane     New spec. rev. Remove priority from MIDs (JHU/APL)
 **  04/19/16  E. Birrane     Put OIDs on stack and not heap. (Secure DTN - NASA: NNX14CS58P)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"
#include "../utils/nm_types.h"

#include "../primitives/mid.h"


/******************************************************************************
 *
 * \par Function Name: mid_add_param
 *
 * \par Adds a parameter to the parameterized OID within a MID.
 *
 * \retval 0 Failure
 *        !0 Success
 *
 * \param[in,out] mid    The MID whose OID is getting a new param.
 * \param[in]     blob   The new parameter.
 *
 * \par Notes:
 *		1. The new parameter is allocated into the new OID and, upon exit,
 *		   the passed-in value may be released if necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation,
 *  04/15/16  E. Birrane     Updated to use blob_t
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/
int mid_add_param(mid_t *mid, amp_type_e type, blob_t *blob)
{
	int result = 0;

	AMP_DEBUG_ENTRY("mid_add_param","(%#llx, %#llx)", mid, blob);

	/* Step 0: Sanity Check.*/
	if((mid == NULL) || (blob == NULL))
	{
		AMP_DEBUG_ERR("mid_add_param","Bad Args", NULL);
		AMP_DEBUG_EXIT("mid_add_param","->0.",NULL);
		return 0;
	}

	/* Step 1: Add to the OID. */
	result = oid_add_param(&(mid->oid), type, blob);

	AMP_DEBUG_EXIT("mid_add_param","->%d.",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: mid_clear
 *
 * \par Resets the values associated with a MID. Basically, a structure-aware
 *      bzero.
 *
 * \retval void
 *
 * \param[in,out] mid  The MID being cleared.
 *
 * \par Notes:
 *		1. Clearing a MID is different than destroying a MID. This just clears
 *		   allocated members. The MID itself may be re-used.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *****************************************************************************/
void mid_clear(mid_t *mid)
{

    AMP_DEBUG_ENTRY("mid_clear","(%#llx)", (unsigned long) mid);

    if(mid == NULL)
    {
        AMP_DEBUG_ERR("mid_clear","Clearing NULL MID.", NULL);
        AMP_DEBUG_EXIT("mid_clear","->NULL.",NULL);
        return;
    }

    if(mid->raw != NULL)
    {
        SRELEASE(mid->raw);
        mid->raw = NULL;
    }

   	oid_release(&(mid->oid));

    memset(mid, 0, sizeof(mid_t));
    AMP_DEBUG_EXIT("mid_clear","", NULL);
}



/******************************************************************************
 *
 * \par Function Name: mid_clear_parms
 *
 * \par Clears the parameters associated with a MID.
 *
 * \param[in] mid The MID whose parameters are to be cleared.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/19/16  E. Birrane     Initial implementation,
 *****************************************************************************/

void mid_clear_parms(mid_t *mid)
{
	CHKVOID(mid);
	oid_clear_parms(&(mid->oid));
	mid_internal_serialize(mid);
}

/******************************************************************************
 *
 * \par Function Name: mid_compare
 *
 * \par Determines equivalence of two MIDs.
 *
 * \retval -1,<0: Error or mid1 < mid2
 * 			0   : mid1 == mid2
 * 			>0  : mid1 > mid2
 *
 * \param[in] mid1      First MID being compared.
 * \param[in] mid2      Second MID being compared.
 * \param[in] use_parms Whether to compare OID paramaters as well.
 *
 * \par Notes:
 *		1. This function should only check for equivalence (== 0), not order
 *         since we do not differentiate between mid1 < mid2 and error.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field.
 *****************************************************************************/

int mid_compare(mid_t *mid1, mid_t *mid2, uint8_t use_parms)
{
	int result = 1;

    AMP_DEBUG_ENTRY("mid_compare","(%#llx, %#llx)",
    		         (unsigned long) mid1,
    		         (unsigned long) mid2);

    if((mid1 == NULL) || (mid2 == NULL))
    {
        return -1;
    }

    if(use_parms != 0)
    {
    	if(mid1->raw_size != mid2->raw_size)
    	{
    		return -1;
    	}
    }

    /* Step 3: Check if flag and MID length bytes match. */
    if((mid1->flags == mid2->flags) &&
       (mid1->issuer == mid2->issuer) &&
       (mid1->tag == mid2->tag))
    {
    	result = oid_compare(mid1->oid, mid2->oid, use_parms);
    }

    AMP_DEBUG_EXIT("mid_compare","->%d.", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: mid_construct
 *
 * \par Constructs a MID object from arguments.
 *
 * \retval NULL - Failure
 *         !NULL - The constructed MID
 *
 * \param[in] id       The Id of the structure encapsulated by the MID.
 * \param[in] issuer   The MID issuer, or NULL for no issuer.
 * \param[in] tag      The MID tag, or NULL for no tag.
 * \param[in] oid      The OID encapsulated in the MID.
 *
 * \par Notes:
 *		1. The returned MID is allocated and must be freed when no longer needed.
 *		2. The passed-in OID is *deep copied* and may be released before releasing
 *		   the MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/17/13  E. Birrane     Updated to ION 3.1.3, switched to uvast
 *  06/25/13  E. Birrane     Removed references to priority field.
 *  07/04/16  E. Birrane     Moved MID type/cat to ID.
 *****************************************************************************/

mid_t *mid_construct(uint8_t id, uvast *issuer, uvast *tag, oid_t oid)
{
	mid_t *mid = NULL;
	AMP_DEBUG_ENTRY("mid_construct","(%d, %#llx, %#llx, oid)",
			         id,
			         (unsigned long) issuer, (unsigned long) tag);

        if(id == MID_ANY)
        {
          AMP_DEBUG_ERR("mid_construct","Bad id.", NULL);
          return NULL;
        }

	/* Step 1: Allocate the MID. */
	if((mid = (mid_t *)STAKE(sizeof(mid_t))) == NULL)
	{
		AMP_DEBUG_ERR("mid_construct","Can't allocate %d bytes.",
				        sizeof(mid_t));
		AMP_DEBUG_EXIT("mid_construct","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the MID. */

	/* Flag */
	mid->flags =  (oid.type & 0x03) << 6;
	mid->flags |= (tag != NULL) ? 0x20 : 0x00;
	mid->flags |= (issuer != NULL) ? 0x10 : 0x00;
	mid->flags |= (id & 0x0F);

	/* Shallow copies */
	mid->id       = id;
	mid->issuer   = (issuer != NULL) ? *issuer : 0;
	mid->tag      = (tag != NULL) ? *tag : 0;

	mid->oid = oid_copy(oid);

	if(mid->oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("mid_construct","Failed to copy OID.",NULL);

		SRELEASE(mid);
		AMP_DEBUG_EXIT("mid_construct","->NULL",NULL);
		return NULL;
	}

	/* Step 3: Build the serialized MID. */
	if (mid_internal_serialize(mid) == 0)
	{
		AMP_DEBUG_ERR("mid_construct","Failed to serialize MID.",NULL);

		mid_release(mid);

		AMP_DEBUG_EXIT("mid_construct","->NULL",NULL);
		return NULL;
	}


	AMP_DEBUG_EXIT("mid_construct","->0x%x",(unsigned long)mid);
	return mid;
}



/******************************************************************************
 *
 * \par Function Name: mid_copy
 *
 * \par Duplicates a MID object.
 *
 * \retval NULL - Failure
 *         !NULL - The copied MID
 *
 * \param[in] src_mid The MID being copied.
 *
 * \par Notes:
 *		1. The returned MID is allocated and must be freed when no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *****************************************************************************/

mid_t *mid_copy(mid_t *src_mid)
{
    mid_t *result = 0;
    
    AMP_DEBUG_ENTRY("mid_copy","(%#llx)",
    		          (unsigned long) src_mid);
    
    /* Step 0: Sanity Check */
    if(src_mid == NULL)
    {
        AMP_DEBUG_ERR("mid_copy","Cannot copy from NULL source MID.", NULL);
        AMP_DEBUG_EXIT("mid_copy","->NULL",NULL);
        return NULL;
    }

    /* Step 1: Allocate the new MID. */
    if((result = (mid_t *)STAKE(sizeof(mid_t))) == NULL)
    {
        AMP_DEBUG_ERR("mid_copy","Can't allocate %d bytes", sizeof(mid_t));
        AMP_DEBUG_EXIT("mid_copy","->NULL",NULL);
        return NULL;
    }

    /* Step 2: Start with a shallow copy. */
    memcpy(result, src_mid, sizeof(mid_t));

    /* Step 3: Now, deep copy the pointers. */
    result->oid = oid_copy(src_mid->oid);

    if((result->raw = (uint8_t *)STAKE(src_mid->raw_size)) == NULL)
    {
        AMP_DEBUG_ERR("mid_copy","Can't allocate %d bytes",
        		        src_mid->raw_size);
        oid_release(&(result->oid));
        mid_release(result);

        AMP_DEBUG_EXIT("mid_copy","->NULL",NULL);
        return NULL;
    }

    memcpy(result->raw, src_mid->raw, src_mid->raw_size);

    AMP_DEBUG_EXIT("mid_copy","->%d", result);
    return result;
}


/******************************************************************************
 *
 * \par Function Name: mid_copy_parms
 *
 * \par Deep-copies parameters from a source MID to a dest MID.
 *
 * \retval ERROR - Problem
 *         SUCCESS - OK.
 *
 * \param[in|out] dest  The MID to receive a copy of the parameters.
 * \param[in]     src   The MID providing the parameters to copy.
 *
 * \par Notes:
 *		1. The destination MID MUST NOT already have parameters.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/19/16  E. Birrane     Initial implementation,
 *****************************************************************************/

uint8_t  mid_copy_parms(mid_t *dest, mid_t *src)
{
	CHKERR(dest);
	CHKERR(src);

	return oid_copy_parms(&(dest->oid), &(src->oid));
}

/******************************************************************************
 *
 * \par Function Name: mid_deserialize
 *
 * \par Extracts a MID from a byte buffer.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized MID.
 *
 * \param[in]  buffer       The byte buffer holding the MID data
 * \param[in]  buffer_size  The # bytes available in the buffer
 * \param[out] bytes_used   The # of bytes consumed in the deserialization.
 *
 * \par Notes:
 * \todo: Allow a NULL bytes_used for cases where we are not deserializing
 *        from a larger stream.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field.
 *****************************************************************************/
mid_t *mid_deserialize(unsigned char *buffer,
		               uint32_t  buffer_size,
		               uint32_t *bytes_used)
{
    mid_t *result = NULL;
    unsigned char *cursor = NULL;
    uint32_t cur_bytes = 0;
    uint32_t bytes_left=0;

    AMP_DEBUG_ENTRY("mid_deserialize","(%#llx, %d, %#llx)",
                     (unsigned long) buffer,
                     (unsigned long) buffer_size,
                     (unsigned long) bytes_used);

    /* Step 1: Sanity checks. */
    if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
    {
        AMP_DEBUG_ERR("mid_deserialize","Bad params.",NULL);
        AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	*bytes_used = 0;
    	cursor = buffer;
    	bytes_left = buffer_size;
    }

    /* Step 2: Allocate/Zero the MID. */
    if((result = (mid_t *) STAKE(sizeof(mid_t))) == NULL)
    {
        AMP_DEBUG_ERR("mid_deserialize","Cannot allocate MID of size %d",
        		        sizeof(mid_t));
        AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
        memset(result,0,sizeof(mid_t));
    }

    /* Step 3: Grab the MID flag */
    if(utils_grab_byte(cursor, bytes_left, &(result->flags)) != 1)
    {
    	AMP_DEBUG_ERR("mid_deserialize","Can't grab flag byte.", NULL);
    	mid_release(result);

        AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	cursor += 1;
    	bytes_left -= 1;
    	*bytes_used += 1;
    }

    /* Step 4: Break out flag...*/
    result->id = MID_GET_FLAG_ID(result->flags);

    /* Step 5: Grab issuer, if present. Issuers MUST exist for non-atomic.*/
    if(MID_GET_FLAG_ISS(result->flags))
    {
    	cur_bytes = utils_grab_sdnv(cursor, bytes_left, &(result->issuer));
        if( cur_bytes == 0)
        {
        	AMP_DEBUG_ERR("mid_deserialize","Can't grab issuer.", NULL);
        	mid_release(result);

            AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
            return NULL;
        }
        else
        {
        	cursor += cur_bytes;
        	bytes_left -= cur_bytes;
        	*bytes_used += cur_bytes;
        }
    }

    /* Step 6: Grab the OID. */
    switch(MID_GET_FLAG_OID(result->flags))
    {
    	case OID_TYPE_FULL:
    		result->oid = oid_deserialize_full(cursor, bytes_left, &cur_bytes);
    		break;
    	case OID_TYPE_PARAM:
    		result->oid = oid_deserialize_param(cursor, bytes_left, &cur_bytes);
    		break;
    	case OID_TYPE_COMP_FULL:
    		result->oid = oid_deserialize_comp(cursor, bytes_left, &cur_bytes);
    		break;
    	case OID_TYPE_COMP_PARAM:
    		result->oid = oid_deserialize_comp_param(cursor, bytes_left, &cur_bytes);
    		break;
    	default:
    		result->oid.type = OID_TYPE_UNK;
    		cur_bytes = 0;
    		break;
    }

    if(result->oid.type == OID_TYPE_UNK)
    {
    	AMP_DEBUG_ERR("mid_deserialize","Can't grab oid.", NULL);
    	mid_release(result);

        AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
        return NULL;
    }
    else
    {
    	cursor += cur_bytes;
    	bytes_left -= cur_bytes;
    	*bytes_used += cur_bytes;
    	result->oid.type = MID_GET_FLAG_OID(result->flags);
    }


    /* Step 7: Populate Tag if preset. */
    if(MID_GET_FLAG_TAG(result->flags))
    {
    	cur_bytes = utils_grab_sdnv(cursor, bytes_left, &(result->tag));
        if( cur_bytes == 0)
        {
        	AMP_DEBUG_ERR("mid_deserialize","Can't grab tag.", NULL);
        	mid_release(result);

            AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
            return NULL;
        }
        else
        {
        	cursor += cur_bytes;
        	bytes_left -= cur_bytes;
        	*bytes_used += cur_bytes;
        }
    }

    /* Step 8: Is this a sane MID? */
    if(mid_sanity_check(result) == 0)
    {
    	AMP_DEBUG_ERR("mid_deserialize","Flags Sanity Check Failed.", NULL);
    	mid_release(result);

        AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
        return NULL;
    }

    /* Step 9: Copy MID into the raw value. */
    if((result->raw = (unsigned char *) STAKE(*bytes_used)) == NULL)
    {
        AMP_DEBUG_ERR("mid_deserialize","Can't TAKE raw mid of size (%d)",
        		        *bytes_used);
        mid_release(result);

        AMP_DEBUG_EXIT("mid_deserialize","-> NULL", NULL);
        return NULL;
    }

    memcpy(result->raw, buffer, *bytes_used);
    result->raw_size = *bytes_used;

    AMP_DEBUG_EXIT("mid_deserialize","-> %#llx", (unsigned long) result);
    return result;
}


mid_t*   mid_deserialize_str(char *mid_str,
							 uint32_t buffer_size,
							 uint32_t *bytes_used)
{
	mid_t *result = NULL;
	uint32_t hex_size = 0;
	uint8_t *mid_hex = NULL;
	uint32_t used = 0;

	AMP_DEBUG_ENTRY("mid_deserialize_str","(%s, %d, %#llx)",mid_str, buffer_size, (unsigned long) bytes_used);

	mid_hex = utils_string_to_hex(mid_str, &hex_size);
	if(mid_hex == NULL)
	{
		AMP_DEBUG_ERR("mid_deserialize_str","Can't made hex from %s.", mid_str);
		AMP_DEBUG_EXIT("mid_deserialize_str","-> 0.", NULL);
		return NULL;
	}

	result = mid_deserialize(mid_hex, hex_size, &used);
	SRELEASE(mid_hex);

	if(result == NULL)
	{
		AMP_DEBUG_ERR("mid_deserialize_str","Can't deserialize from %s.", mid_str);
		AMP_DEBUG_EXIT("mid_deserialize_str","-> 0.", NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("mid_deserialize_str","-> %#llx", (unsigned long) result);

	return result;
}


/*
 * Retrieve mid from a string representing the MID in hex
 * "0x31801...."
 */
mid_t* mid_from_string(char *mid_str)
{
	mid_t *result = NULL;
	uint8_t *tmp = NULL;
	uint32_t len = 0;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("mid_from_string","(0x%x)", mid_str);

	/* Step 0: Sanity check. */
	if(mid_str == NULL)
	{
		AMP_DEBUG_ERR("mid_from_string","Bad args.", NULL);
		AMP_DEBUG_EXIT("mid_from_string","->NULL", NULL);
		return NULL;
	}

	/* Step 1: Convert the string into a binary buffer. */
    if((tmp = utils_string_to_hex(mid_str, &len)) == NULL)
    {
    	AMP_DEBUG_ERR("mid_from_string","Can't Parse MID ID of %s.", mid_str);
		AMP_DEBUG_EXIT("mid_from_string","->NULL", NULL);
		return NULL;
    }

    /* Step 2: Build a mid by "deserializing" the STRING into a MID. */
    result = mid_deserialize(tmp, len, &bytes);

    SRELEASE(tmp);

	AMP_DEBUG_EXIT("mid_from_string","->0x%x", (unsigned long) result);

	return result;
}

blob_t *mid_get_param(mid_t *id, int i, amp_type_e *type)
{
	AMP_DEBUG_ENTRY("mid_get_param","(%#llx, i)",(unsigned long) id, i);

	if(id == NULL)
	{
		AMP_DEBUG_ERR("mid_get_param","Bad args.",NULL);
		AMP_DEBUG_EXIT("mid_get_param","->0",NULL);
		return NULL;
	}

	return oid_get_param(id->oid, i, type);
}

uint8_t  mid_get_num_parms(mid_t *mid)
{
	AMP_DEBUG_ENTRY("mid_get_num_parms","(%#llx)",(unsigned long) mid);

	if(mid == NULL)
	{
		AMP_DEBUG_ERR("mid_get_num_parms","Bad args.",NULL);
		AMP_DEBUG_EXIT("mid_get_num_parms","->0",NULL);
		return 0;
	}

	return oid_get_num_parms(mid->oid);
}

/******************************************************************************
 *
 * \par Function Name: mid_internal_serialize
 *
 * \par populates the raw value of a MID with it's serialized representation.
 *
 * \par Since MIDs are used heavily in the agent, and since they are received in
 * a serialized form, it is efficient to keep the serialized version of the mid
 * to avoid having to re-calculate it.  However, when the MID changes, or when
 * software generates the MID initially, the serialized representation must
 * be created.
 *
 * \par
 *  +--------+--------+--------+--------+
 *  | Flags  | Issuer |   OID  |   Tag  |
 *  | [BYTE] | [SDNV] | [SDNV] | [SDNV] |
 *  |        | (opt.) |        | (opt)  |
 *  +--------+--------+--------+--------+
 *
 * \retval 0 - Failure
 *         !0 - Success serializing the MID.
 *
 * \param[in,out] mid The MID being serialized
 *
 * \par Notes:
 *      1. The serialized version of the MID exists on the memory pool and must be
 *         released when finished.
 *      2. Note that the issuer and tag are capped in this implementation.
 *      3. On error, the state of the mid->raw content is unknown.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field.
 *****************************************************************************/

int mid_internal_serialize(mid_t *mid)
{
	Sdnv iss;
	Sdnv tag;
	uint32_t oid_size = 0;
	uint8_t *oid_val = NULL;
	uint8_t *cursor = NULL;

	AMP_DEBUG_ENTRY("mid_internal_serialize","(%#llx)",(unsigned long) mid);

	/* Step 0: Sanity Check. */
	if(mid == NULL)
	{
		AMP_DEBUG_ERR("mid_internal_serialize","Bad args.",NULL);
		AMP_DEBUG_EXIT("mid_internal_serialize","->0",NULL);
		return 0;
	}

	/* Step 1: Serialize the OID for sizing. */
	if((oid_val = oid_serialize(mid->oid, &oid_size)) == NULL)
	{
		AMP_DEBUG_ERR("mid_internal_serialize","Can't serialize OID.",NULL);
		AMP_DEBUG_EXIT("mid_internal_serialize","->0",NULL);
		return 0;
	}

	/* Step 2: If there is a serialized version of the MID already, wipe it.*/
	if(mid->raw != NULL)
	{
		SRELEASE(mid->raw);
		mid->raw = NULL;
	}
	mid->raw_size = 0;

	/* Step 3: Build the SDNVs. */
	encodeSdnv(&iss, mid->issuer);
	encodeSdnv(&tag, mid->tag);


	/* Step 4: Figure out the size of the serialized MID. */
	mid->raw_size = 1 + oid_size;

	/* Add issuer if present. */
	if(MID_GET_FLAG_ISS(mid->flags))
	{
		mid->raw_size += iss.length;
	}

	/* Add tag if present. */
	if(MID_GET_FLAG_TAG(mid->flags))
	{
		mid->raw_size += tag.length;
	}


	/* Step 5: Allocate space for the serialized MID. */
	if((mid->raw = (uint8_t *)STAKE(mid->raw_size)) == NULL)
	{
		AMP_DEBUG_ERR("mid_internal_serialize","Can't alloc %d bytes.",
				        mid->raw_size);
		mid->raw_size = 0;
		SRELEASE(oid_val);

		AMP_DEBUG_EXIT("mid_internal_serialize","->0",NULL);
		return 0;
	}

	/* Step 6: Populate the serialized MID. */
	cursor = mid->raw;

	*cursor = mid->flags;
	cursor++;

	/* Add issuer if present. */
	if(MID_GET_FLAG_ISS(mid->flags))
	{
		memcpy(cursor, iss.text, iss.length);
		cursor += iss.length;
	}

	/* Copy the OID. */
	memcpy(cursor,oid_val, oid_size);
	cursor += oid_size;

	/* Add tag if present. */
	if(MID_GET_FLAG_TAG(mid->flags))
	{
		memcpy(cursor, tag.text, tag.length);
		cursor += tag.length;
	}

	/* Step 7: Final sanity check. */
	if((cursor - mid->raw) != mid->raw_size)
	{
		AMP_DEBUG_ERR("mid_internal_serialize","Copied %d bytes, expected %d bytes.",
				        (cursor - mid->raw), mid->raw_size);
		mid->raw_size = 0;
		SRELEASE(mid->raw);
		mid->raw = NULL;
		SRELEASE(oid_val);

		AMP_DEBUG_EXIT("mid_internal_serialize","->0",NULL);
		return 0;

	}

	SRELEASE(oid_val);

	AMP_DEBUG_EXIT("mid_internal_serialize","->1",NULL);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: mid_pretty_print
 *
 * \par Purpose: Builds a string representation of the MID suitable for
 *               diagnostic viewing.
 *
 * \retval  NULL: Failure to construct a string representation of the MID.
 *         !NULL: The string representation of the MID string.
 *
 * \param[in]  mid  The MID whose string representation is being created.
 *
 * \par Notes:
 *		1. This is a slow, wasteful function and should not be called in
 *		   embedded systems.  For something to dump in a log, try the function
 *		   mid_to_string instead.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field.
 *****************************************************************************/

char *mid_pretty_print(mid_t *mid)
{
	int size = 0;
	int oid_size = 0;
	int raw_size = 0;
	char *oid_str = NULL;
	char *raw_str;
	char *result = NULL;
	char *cursor = NULL;

	AMP_DEBUG_ENTRY("mid_pretty_print","(%#llx)", (unsigned long) mid);

	/* Step 0: Sanity Check. */
	if(mid == NULL)
	{
		AMP_DEBUG_ERR("mid_pretty_print","NULL MID.",NULL);
		AMP_DEBUG_EXIT("mid_pretty_print","->NULL.",NULL);
		return NULL;
	}


	/* Step 1: Grab the pretty-print of the encapsulated OID. We will need to
	 *         do this anyway and it will help with the sizing.
	 */
	if(mid->oid.type == OID_TYPE_UNK)
	{
		oid_str = oid_pretty_print(mid->oid);
		oid_size = strlen((char *)oid_str) + 1;
	}

	if(oid_str == NULL)
	{
		oid_size = strlen("NULL_OID") + 1;
		if((oid_str = (char *) STAKE(oid_size)) != NULL)
		{
			memset(oid_str,0,oid_size);
			strncpy((char *)oid_str,"NULL OID",oid_size);
		}
		else
		{
			AMP_DEBUG_ERR("mid_pretty_print","Can't alloc %d bytes for OID.",
						    oid_size);

			AMP_DEBUG_EXIT("mid_pretty_print","->NULL.",NULL);
			return NULL;
		}
	}

	/* Step 2: Grab the raw version of the MID string. */
	if((raw_str = mid_to_string(mid)) == NULL)
	{
		raw_size = strlen("NO RAW!") + 1;
		if((raw_str = (char *) STAKE(raw_size)) != NULL)
		{
			memset(raw_str, 0, raw_size);
			strncpy(raw_str,"NO RAW!", raw_size);
		}
		else
		{
			AMP_DEBUG_ERR("mid_pretty_print","Can't alloc %d bytes for RAW.",
						    raw_size);
			SRELEASE(oid_str);

			AMP_DEBUG_EXIT("mid_pretty_print","->NULL.",NULL);
			return NULL;
		}
	}
	else
	{
		raw_size = strlen((char*)raw_str) + 1;
	}

	/* Step 3: Guestimate size. This is, at best, a dark art and prone to
	 *         exaggeration. Numerics are assumed to take 5 bytes, unless I
	 *         think they will take 3 instead (indices vs. values). Other
	 *         values are based on constant strings in the function. One must
	 *         update this calculation if one changes string constants, else
	 *         one will be sorry.
	 */
	size = 28 +             /* BANNER------ */
		   14 +   			/* Flag : <...> */
		   18 +   			/* Type : <...> */
		   17 +   			/* Cat : <...>  */
		   17 +        		/* ISS : <...>  */
		   7 + oid_size +   /* OID : <...>  */
		   17 +             /* Tag : <...>  */
		   7 + raw_size +   /* Raw : <...>  */
		   28;				/* BANNER ----- */

	/* Step 4: Allocate the string. */
	if((result = (char*)STAKE(size)) == NULL)
	{
		AMP_DEBUG_ERR("mid_pretty_print", "Can't alloc %d bytes.", size);

		SRELEASE(oid_str);
		SRELEASE(raw_str);

		AMP_DEBUG_EXIT("mid_pretty_print","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,size);
	}

	/* Step 5: Populate the allocated string. Keep an eye on the size. */
	cursor = result;
	cursor += sprintf(cursor,"MID:\n---------------------\nFlag: %#x",mid->flags);

	cursor += sprintf(cursor,"\nType : ");
	switch(mid->id)
	{
	case MID_ATOMIC:   cursor += sprintf(cursor,"AD\n"); break;
	case MID_COMPUTED: cursor += sprintf(cursor,"CD\n"); break;
	case MID_REPORT:   cursor += sprintf(cursor,"RPT\n"); break;
	case MID_CONTROL:  cursor += sprintf(cursor,"CTRL\n"); break;
	case MID_TRL:      cursor += sprintf(cursor,"TRL\n"); break;
	case MID_SRL:      cursor += sprintf(cursor,"SRL\n"); break;
	case MID_MACRO:    cursor += sprintf(cursor,"MACRO\n"); break;
	case MID_LITERAL:  cursor += sprintf(cursor,"LIT\n"); break;
	case MID_OPERATOR: cursor += sprintf(cursor,"OP\n"); break;
	default: cursor += sprintf(cursor,"UNKNOWN\n"); break;
	}

	if(MID_GET_FLAG_ISS(mid->flags))
	{
		cursor += sprintf(cursor,UVAST_FIELDSPEC"\n",mid->issuer);
	}
	else
	{
		cursor += sprintf(cursor,"None.\n");
	}

	cursor += sprintf(cursor,"OID : %s", oid_str);
	SRELEASE(oid_str);

	if(MID_GET_FLAG_TAG(mid->flags))
	{
		cursor += sprintf(cursor,UVAST_FIELDSPEC"\n",mid->tag);
	}
	else
	{
		cursor += sprintf(cursor,"None.\n");
	}

	cursor += sprintf(cursor,"RAW : %s", raw_str);
	SRELEASE(raw_str);

	/* Step 6: Sanity check. */
	if((cursor - result) > size)
	{
		AMP_DEBUG_ERR("mid_pretty_print", "OVERWROTE! Alloc %d, wrote %llu.",
				        size, (cursor-result));
		SRELEASE(result);
		AMP_DEBUG_EXIT("mid_pretty_print","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_INFO("mid_pretty_print","Wrote %llu into %d string.",
			         (cursor -result), size);

	AMP_DEBUG_EXIT("mid_pretty_print","->%#llx",result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: mid_release
 *
 * \par Purpose: Frees resources associated with a MID
 *
 * \param[in,out] mid  The MID being released.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

void mid_release(mid_t *mid)
{
    AMP_DEBUG_ENTRY("mid_release", "(%#llx)", (unsigned long) mid);

    if(mid != NULL)
    {
        mid_clear(mid);
        SRELEASE(mid);
        mid = NULL;
    }

    AMP_DEBUG_EXIT("mid_release", "-> NULL", NULL);
}



/******************************************************************************
 *
 * \par Function Name: mid_sanity_check
 *
 * \par Purpose: Evaluates the consistency of MID member values.
 *
 * \retval  0: The MID failed its check.
 *         !0: The MID passed its check.
 *
 * \param[in] mid  The MID being released.
 *
 * \par Notes:
 *		1. This function keeps going after first sanity failure to collect
 *		   all MID problems in the error log.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *  06/25/13  E. Birrane     Removed references to priority field. Removed
 *                           type/cat checks as spec allows many new combos.
 *  07/04/16  E. Birrane     Removed all type/cat checks.
 *****************************************************************************/

int mid_sanity_check(mid_t *mid)
{
	int result = 1;
    AMP_DEBUG_ENTRY("mid_sanity_check","(%#llx)", (unsigned long) mid);

	/* NULL Checks */
	if(mid == NULL)
	{
        AMP_DEBUG_ERR("mid_sanity_check","NULL mid.", NULL);
        return 0;
	}

	/* Range Checks */
	if(mid->id >= MID_ANY)
	{
        AMP_DEBUG_ERR("mid_sanity_check","id out of range (%d)",
        		        mid->id);
        result = 0;
	}

    AMP_DEBUG_EXIT("mid_sanity_check","-> %d", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: mid_serialize
 *
 * \par Purpose: Returns a serialized version of the MID.
 *
 * \retval  NULL - Failure
 *         !NULL - The serialized MID.
 *
 * \param[in,out] mid   The MID being serialized
 * \param[out]    size  The # bytes of the serialized MID.
 *
 * \par Notes:
 *		1. The serialized version of the MID exists on the memory pool and must be
 *         released when finished.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

uint8_t *mid_serialize(mid_t *mid, uint32_t *size)
{
	uint8_t *result = NULL;

	AMP_DEBUG_ENTRY("mid_serialize","(%#llx, %#llx)",
			          (unsigned long) mid, (unsigned long) size);

	/* Step 0: Sanity Check. */
	if((mid == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("mid_serialize","Bad args.", NULL);
		AMP_DEBUG_EXIT("mid_serialize","->NULL", NULL);
		return NULL;
	}

	/* Step 1: Make sure the MID is serialized. */
	if((mid->raw == NULL) || (mid->raw_size == 0))
	{
		if(mid_internal_serialize(mid) == 0)
		{
			AMP_DEBUG_ERR("mid_serialize","Can't serialize mid.", NULL);
			*size = 0;
			AMP_DEBUG_EXIT("mid_serialize","->NULL", NULL);
			return NULL;
		}
	}

	*size = mid->raw_size;

	/* Step 2: Allocate result. */
	if((result = (uint8_t*) STAKE(*size)) == NULL)
	{
		AMP_DEBUG_ERR("mid_serialize","Can't alloc %d bytes.", *size);
		*size = 0;
		AMP_DEBUG_EXIT("mid_serialize","->NULL", NULL);
		return NULL;
	}

	/* Step 3: Copy in the result. */
	memcpy(result, mid->raw, mid->raw_size);

	AMP_DEBUG_EXIT("mid_serialize","->%#llx", (unsigned long)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: mid_to_string
 *
 * \par Purpose: Create a string representation of the MID.
 *
 * \retval  NULL - Failure
 *         !NULL - The string representation of the raw MID.
 *
 * \param[in] mid The MID whose string representation is being calculated.
 *
 * \par Notes:
 *		1. The string is dynamically allocated from the memory pool and must
 *		   be deallocated when no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

char *mid_to_string(mid_t *mid)
{
    char *result = NULL;

    AMP_DEBUG_ENTRY("mid_to_string","(%#llx)",(unsigned long) mid);

    /* For now, we just show the raw MID. */
	result = utils_hex_to_string(mid->raw, mid->raw_size);

    AMP_DEBUG_EXIT("mid_to_string","->%s.", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: midcol_clear
 *
 * \par Purpose: Clear MID collection in a Lyst.
 *
 * \param[in,out] mc The lyst being cleared.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/30/15  E. Birrane     Initial implementation,
 *****************************************************************************/

void midcol_clear(Lyst mc)
{
	LystElt elt;
	LystElt del_elt;
	mid_t *cur_mid = NULL;

	AMP_DEBUG_ENTRY("midcol_clear","(" ADDR_FIELDSPEC ")", (uaddr) mc);

	/*
	 * Step 0: Make sure we even have a lyst.
	 */
	if(mc == NULL)
	{
		AMP_DEBUG_WARN("midcol_clear","NULL mc.",NULL);
		AMP_DEBUG_EXIT("midcol_clear","->.", NULL);
		return;
	}

	/* Step 1: Walk through the MIDs releasing as you go. */
    for(elt = lyst_first(mc); elt;)
    {
    	cur_mid = (mid_t *) lyst_data(elt);

    	if(cur_mid != NULL)
    	{
    		mid_release(cur_mid);
    	}
    	del_elt = elt;
    	elt = lyst_next(elt);
    	lyst_delete(del_elt);
    }

    AMP_DEBUG_EXIT("midcol_clear","->.", NULL);
}


/******************************************************************************
 *
 * \par Function Name: midcol_copy
 *
 * \par Purpose: Copies a MID collection
 *
 * \retval  NULL - Failure
 *         !NULL - The copied MID collection
 *
 * \param[in]  mids  MID collection to be copied.
 *
 * \par Notes:
 *		1. This is a deep copy.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/
Lyst midcol_copy(Lyst mids)
{
	Lyst result = NULL;
	LystElt elt;
	mid_t *cur_mid = NULL;
	mid_t *new_mid = NULL;

	AMP_DEBUG_ENTRY("midcol_copy","(%#llx)",(unsigned long) mids);

	/* Step 0: Sanity Check. */
	if(mids == NULL)
	{
		AMP_DEBUG_ERR("midcol_copy","Bad Args.",NULL);
		AMP_DEBUG_EXIT("midcol_copy","->NULL.",NULL);
		return NULL;
	}

	/* Build the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("midcol_copy","Unable to create lyst.",NULL);
		AMP_DEBUG_EXIT("midcol_copy","->NULL.",NULL);
		return NULL;
	}

	/* Walking copy. */
	for(elt = lyst_first(mids); elt; elt = lyst_next(elt))
	{
		cur_mid = (mid_t *) lyst_data(elt);
		if((new_mid = mid_copy(cur_mid)) == NULL)
		{
			AMP_DEBUG_ERR("midcol_copy","Fsiled to copy MID.",NULL);
			midcol_destroy(&result);

			AMP_DEBUG_EXIT("midcol_copy","->NULL.",NULL);
			return NULL;
		}
		else
		{
			lyst_insert_last(result,new_mid);
		}
	}

	AMP_DEBUG_EXIT("midcol_copy","->%#llx.",(unsigned long) result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: midcol_destroy
 *
 * \par Purpose: Destroy MID collection in a Lyst.
 *
 * \retval  NULL - Failure
 *         !NULL - The copied MID collection
 *
 * \param[in,out] mids The lyst being destroyed.
 *
 * \par Notes:
 *		1. We pass a double pointer for the honor of making sure the lyst is set
 *         to NULL when finished.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *  08/30/15  E. Birrane     Use midcol_clear as helper.
 *****************************************************************************/
void midcol_destroy(Lyst *mids)
{

	AMP_DEBUG_ENTRY("midcol_destroy","(%#llx)", (unsigned long) mids);

	/*
	 * Step 0: Make sure we even have a lyst. Not an error if not, since we
	 * are destroying anyway.
	 */
	if((mids == NULL) || (*mids == NULL))
	{
		AMP_DEBUG_ERR("midcol_destroy","Bad Args.",NULL);
		AMP_DEBUG_EXIT("midcol_destroy","->.", NULL);
		return;
	}

	/* Step 1: Remove all of the MIDs in the MC. */
	midcol_clear(*mids);

    /* Step 2: Destroy and zero out the lyst. */
    lyst_destroy(*mids);
    *mids = NULL;

    AMP_DEBUG_EXIT("midcol_destroy","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: midcol_deserialize
 *
 * \par Purpose: Deserialize a MID collection into a Lyst.
 *
 * \retval NULL - Failure
 *        !NULL - The created Lyst.
 *
 * \param[in]  buffer      The buffer holding the MC.
 * \param[in]  buffer_size The size of the buffer, in bytes.
 * \param[out] bytes_used  The # bytes consumed in the deserialization.
 *
 * \par Notes:
 *		1. The created Lyst must be freed when done.
 *		2. Reminder: A Lyst is a pointer to a LystStruct.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *  06/17/13  E. Birrane     Updated to ION 3.1.3, moved to uvast
 *****************************************************************************/
Lyst midcol_deserialize(unsigned char *buffer,
		                uint32_t buffer_size,
		                uint32_t *bytes_used)
{
	unsigned char *cursor = NULL;
	Lyst result = NULL;
	uint32_t bytes = 0;
	uvast num = 0;
	mid_t *cur_mid = NULL;
	uint32_t i = 0;

	AMP_DEBUG_ENTRY("midcol_deserialize","(%#llx,%d,%#llx)",
			          (unsigned long) buffer, buffer_size,
			          (unsigned long) bytes_used);

	/* Step 0: Sanity Check. */
	if((buffer == NULL) || (buffer_size == 0) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("mid_deserialize_mc","Bad Args", NULL);
		AMP_DEBUG_EXIT("mid_deserialize_mc","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;
	cursor = buffer;

	/* Step 1: Create the Lyst. */
	if((result = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("midcol_deserialize","Can't create lyst.", NULL);
		AMP_DEBUG_EXIT("midcol_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab # MIDs in the collection. */
	if((bytes = utils_grab_sdnv(cursor, buffer_size, &num)) == 0)
	{
		AMP_DEBUG_ERR("midcol_deserialize","Can't parse SDNV.", NULL);
		lyst_destroy(result);

		AMP_DEBUG_EXIT("midcol_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		buffer_size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Grab Mids. */
	for(i = 0; i < num; i++)
	{
		/* Deserialize ith MID. */
		if((cur_mid = mid_deserialize(cursor, buffer_size, &bytes)) == NULL)
		{
			AMP_DEBUG_ERR("mid_deserialize_mc","Can't grab MID #%d.", i);
			midcol_destroy(&result);

			AMP_DEBUG_EXIT("mid_deserialize_mc","->NULL",NULL);
			return NULL;
		}
		else
		{
			cursor += bytes;
			buffer_size -= bytes;
			*bytes_used += bytes;
		}

		/* Drop it in the lyst in order. */
		lyst_insert_last(result, cur_mid);
	}

	AMP_DEBUG_EXIT("midcol_deserialize","->%#llx",(unsigned long)result);
	return result;
}





/******************************************************************************
 *
 * \par Function Name: midcol_pretty_print
 *
 * \par Purpose: Builds a string representation of a MID collection suitable for
 *               diagnostic viewing.
 *
 * \retval NULL: Failure to construct a string representation of the MID coll.
 *         !NULL: The string representation of the MID coll string.
 *
 * \param[in]  mc  The MID collection whose string rep is being created.
 *
 * \par Notes:
 *		1. This is a slow, wasteful function and should not be called in
 *		   embedded systems.  For something to dump in a log, try the function
 *		   midcol_to_string instead.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

char *midcol_pretty_print(Lyst mc)
{
	LystElt elt;
	char *result = NULL;
	char *cursor = NULL;
	int num_items = 0;
	int i = 0;

	char **mid_strs = NULL;
	int tot_size = 0;

	AMP_DEBUG_ENTRY("midcol_pretty_print","(%#llx)", (unsigned long) mc);

	/* Step 0: Sanity Check. */
	if(mc == NULL)
	{
		AMP_DEBUG_ERR("midcol_pretty_print","Bad Args.", NULL);
		AMP_DEBUG_EXIT("midcol_pretty_print","->NULL", NULL);
		return NULL;
	}

	/* Step 1: Grab the number of MIDs in the collection, and pre-allocate
	 *         space to store their printed information.
	 */
	num_items = (int) lyst_length(mc);

	mid_strs = (char**) STAKE(num_items * sizeof(char*));
	if(mid_strs == NULL)
	{
		AMP_DEBUG_ERR("midcol_pretty_print","Can't alloc %d bytes.",
				        num_items * sizeof(char *));
		AMP_DEBUG_EXIT("midcol_pretty_print","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab the pretty-print of individual MIDs. We need this anyway
	 *         and it will help us get the sizing right.
	 */
	for(elt = lyst_first(mc); (elt && (i < num_items)); elt=lyst_next(elt))
	{
		mid_t *cur_mid = (mid_t*) lyst_data(elt);
		mid_strs[i] = mid_pretty_print(cur_mid);
		tot_size += strlen(mid_strs[i]);
		i++;
	}

	/* Step 3: Calculate size of the MID collection print and allocate it. */
	tot_size += 28 +       /* HEADER and ---'s */
			    28 +       /* Trailer ---'s */
			    num_items; /* newline per MID. */

	if((result = (char *) STAKE(tot_size)) == NULL)
	{
		AMP_DEBUG_ERR("midcol_pretty_print","Can't alloc %d bytes.",
				        tot_size);
		for(i = 0; i < num_items; i++)
		{
			SRELEASE(mid_strs[i]);
		}
		SRELEASE(mid_strs);

		AMP_DEBUG_EXIT("midcol_pretty_print","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor = result;
	}

	/* Step 4: Fill in the MID collection string. */
	cursor += sprintf(cursor,"MID COLLECTION:\n-------------\n");
	for(i = 0; i < num_items; i++)
	{
		cursor += sprintf(cursor,"%s\n",mid_strs[i]);
		SRELEASE(mid_strs[i]);
	}
	cursor += sprintf(cursor,"--------------\n");
	SRELEASE(mid_strs);

	/* Step 5: Sanity check. */
	if((cursor - result) > tot_size)
	{
		AMP_DEBUG_ERR("midcol_pretty_print", "OVERWROTE! Alloc %d, wrote %llu.",
				tot_size, (cursor-result));
		SRELEASE(result);
		AMP_DEBUG_EXIT("mid_pretty_print","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_INFO("midcol_pretty_print","Wrote %llu into %d string.",
					(cursor -result), tot_size);

	AMP_DEBUG_EXIT("midcol_pretty_print","->%#llx",result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: midcol_to_string
 *
 * \par Purpose: Create a string representation of a MID collection.
 *
 * \retval  NULL - Failure
 *         !NULL - The string representation of the MID collection.
 *
 * \param[in] mc The collection whose string representation is being calculated.
 *
 * \par Notes:
 *		1. The string is dynamically allocated from the memory pool and must
 *		   be deallocated when no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/23/12  E. Birrane     Initial implementation,
 *****************************************************************************/

char *midcol_to_string(Lyst mc)
{
	LystElt elt;
	char *result = NULL;
	char *cursor = NULL;
	int num_items = 0;
	int i = 0;

	char **mid_strs = NULL;
	int tot_size = 0;

	AMP_DEBUG_ENTRY("midcol_to_string","(%#llx)", (unsigned long) mc);

	/* Step 0: Sanity Check. */
	if(mc == NULL)
	{
		AMP_DEBUG_ERR("midcol_to_string","Bad Args.", NULL);
		AMP_DEBUG_EXIT("midcol_to_string","->NULL", NULL);
		return NULL;
	}

	/* Step 1: Grab the number of MIDs in the collection, and pre-allocate
	 *         space to store their printed information.
	 */
	num_items = (int) lyst_length(mc);

	mid_strs = (char**) STAKE(num_items * sizeof(char*));
	if(mid_strs == NULL)
	{
		AMP_DEBUG_ERR("midcol_to_string","Can't alloc %d bytes.",
				        num_items * sizeof(char *));
		AMP_DEBUG_EXIT("midcol_to_string","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab the pretty-print of individual MIDs. We need this anyway
	 *         and it will help us get the sizing right.
	 */
	for(elt = lyst_first(mc); (elt && (i < num_items)); elt=lyst_next(elt))
	{
		mid_t *cur_mid = (mid_t*) lyst_data(elt);
		mid_strs[i] = mid_to_string(cur_mid);
		tot_size += strlen(mid_strs[i]);
		i++;
	}

	/* Step 3: Calculate size of the MID collection print and allocate it. */
	tot_size += 5 +        /* "MC : " */
				2 +        /* trailer */
			    num_items + /* space between MIDs. */
				2 +         /* Period and newline. */
				1;          /* NULL terminator */

	if((result = (char *) STAKE(tot_size)) == NULL)
	{
		AMP_DEBUG_ERR("midcol_to_string","Can't alloc %d bytes.",
				        tot_size);
		for(i = 0; i < num_items; i++)
		{
			SRELEASE(mid_strs[i]);
		}
		SRELEASE(mid_strs);

		AMP_DEBUG_EXIT("midcol_to_string","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor = result;
	}

	/* Step 4: Fill in the MID collection string. */
	cursor += sprintf(cursor,"MC : ");
	for(i = 0; i < num_items; i++)
	{
		cursor += sprintf(cursor,"%s ",mid_strs[i]);
		SRELEASE(mid_strs[i]);
	}
	cursor += sprintf(cursor,".\n");
	SRELEASE(mid_strs);

	/* Step 5: Sanity check. */
	if((cursor - result) > tot_size)
	{
		AMP_DEBUG_ERR("midcol_to_string", "OVERWROTE! Alloc %d, wrote %llu.",
				tot_size, (cursor-result));
		SRELEASE(result);
		AMP_DEBUG_EXIT("mid_to_string","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_INFO("midcol_to_string","Wrote %llu into %d string.",
					(cursor -result), tot_size);

	AMP_DEBUG_EXIT("midcol_to_string","->%#llx",result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: midcol_serialize
 *
 * \par Purpose: Serialize a MID collection.
 *
 * \retval NULL - Failure
 *         !NULL - Serialized MC.
 *
 *  \param[in]  mids  The list of MIDs comprising the collection.
 *  \param[out] size  The number of bytes of serialized MC.
 *
 * \par Notes:
 *		1. The serialized collection is allocated on the memory pool and must
 *         be returned when no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/23/12  E. Birrane     Initial implementation,
 *****************************************************************************/

uint8_t *midcol_serialize(Lyst mids, uint32_t *size)
{
	uint8_t *result = NULL;
	Sdnv num_sdnv;
	LystElt elt;

	AMP_DEBUG_ENTRY("midcol_serialize","(%#llx, %#llx)",
			          (unsigned long) mids, (unsigned long) size);

	/* Step 0: Sanity Check */
	if((mids == NULL) || (size == NULL))
	{
		AMP_DEBUG_ERR("midcol_serialize","Bad args.", NULL);
		AMP_DEBUG_EXIT("midcol_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Calculate the size. */

	/* Consider the size of the SDNV holding # MIDS.*/
	encodeSdnv(&num_sdnv, lyst_length(mids));

	*size = num_sdnv.length;

	/* Walk through each MID, make sure it is serialized, and look at size. */
    for(elt = lyst_first(mids); elt; elt = lyst_next(elt))
    {
    	mid_t *cur_mid = (mid_t *) lyst_data(elt);

    	if(cur_mid != NULL)
    	{
    		if((cur_mid->raw == NULL) || (cur_mid->raw_size == 0))
    		{
    			/* \todo check return code. */
    			mid_internal_serialize(cur_mid);
    		}
    		if((cur_mid->raw == NULL) || (cur_mid->raw_size == 0))
    		{
        		AMP_DEBUG_WARN("midcol_serialize","MID didn't serialize.", NULL);
    		}
    		else
    		{
    			*size += cur_mid->raw_size;
    		}
    	}
    	else
    	{
    		AMP_DEBUG_WARN("midcol_serialize","Found NULL MID?", NULL);
    	}
    }

    /* Step 3: Allocate the space for the serialized list. */
    if((result = (uint8_t*) STAKE(*size)) == NULL)
    {
		AMP_DEBUG_ERR("midcol_serialize","Can't alloc %d bytes", *size);
		*size = 0;

		AMP_DEBUG_EXIT("midcol_serialize","->NULL",NULL);
		return NULL;
    }

    /* Step 4: Walk through list again copying as we go. */
    uint8_t *cursor = result;

    /* COpy over the number of MIDs in the collection. */
    memcpy(cursor, num_sdnv.text, num_sdnv.length);
    cursor += num_sdnv.length;

    for(elt = lyst_first(mids); elt; elt = lyst_next(elt))
    {
    	mid_t *cur_mid = (mid_t *) lyst_data(elt);

    	if(cur_mid != NULL)
    	{
    		if((cur_mid->raw != NULL) && (cur_mid->raw_size > 0))
    		{
    			memcpy(cursor,cur_mid->raw, cur_mid->raw_size);
    			cursor += cur_mid->raw_size;
    		}
    	}
    	else
    	{
    		AMP_DEBUG_WARN("midcol_serialize","Found NULL MID?", NULL);
    	}
    }

    /* Step 5: Final sanity check. */
    if((cursor - result) != *size)
    {
		AMP_DEBUG_ERR("midcol_serialize","Wrote %d bytes not %d bytes",
				        (cursor - result), *size);
		*size = 0;
		SRELEASE(result);
		AMP_DEBUG_EXIT("midcol_serialize","->NULL",NULL);
		return NULL;
    }

	AMP_DEBUG_EXIT("midcol_serialize","->%#llx",(unsigned long) result);
	return result;
}




