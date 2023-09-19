/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **  10/27/12  E. Birrane     Initial Implementation (JHU/APL)
 **  11/13/12  E. Birrane     Technical review, comment updates. (JHU/APL)
 **  03/11/15  E. Birrane     Removed NN from OID into NN. (Secure DTN - NASA: NNX14CS58P)
 **  08/29/15  E. Birrane     Removed length restriction from OID parms. (Secure DTN - NASA: NNX14CS58P)
 **  06/11/16  E. Birrane     Updated parameters to be of type TDC. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"

#include "../primitives/oid.h"

#define UHF UVAST_HEX_FIELDSPEC


oid_t oid_get_null()
{
	static int i = 0;
	static oid_t result;

	if(i == 0)
	{
		oid_init(&result);
		i = 1;
	}
	return result;
}

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
 *  04/15/16  E. Birrane     Updated to use BLOB type
 *  04/19/16  E. Birrane     Keep OIDs on stack not heap.
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/
int8_t    oid_add_param(oid_t *oid, amp_type_e type, blob_t *blob)
{
	blob_t *entry = NULL;

	AMP_DEBUG_ENTRY("oid_add_param","(%#llx, %#llx)", oid, blob);

	/* Step 0: Sanity Check.*/
	if((blob == NULL) || (type == AMP_TYPE_UNK))
	{
		AMP_DEBUG_ERR("oid_add_param","Bad Args", NULL);
		AMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	/* Step 1: Make sure the OID is a parameterized one. */
	if((oid->type != OID_TYPE_PARAM)  &&
	   (oid->type != OID_TYPE_COMP_PARAM))
	{
		AMP_DEBUG_ERR("oid_add_param","Can't add parameter to OID of type %d.",
				        oid->type);
		AMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	/* Step 2: Add the parameter. */
	if(tdc_insert(&(oid->params), type, blob->value, blob->length) == AMP_TYPE_UNK)
	{
		AMP_DEBUG_ERR("oid_add_param","Cannot add parameter.", NULL);
		AMP_DEBUG_EXIT("oid_add_param","->0.",NULL);
		return 0;
	}

	AMP_DEBUG_EXIT("oid_add_param","->%d.",1);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: oid_add_params
 *
 * \par Adds a set of parameters to a parameterized OID.
 *
 * \retval -1 System Error
 *          0 User Error
 *          1 Success
 *
 * \param[in,out] oid     The OID getting a new param.
 * \param[in]     params  The TDC of new parameters.
 *
 * \par Notes:
 *		1. The new parameter is allocated into the OID and, upon exit,
 *		   the passed-in value may be released if necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  05/28/15  E. Birrane     Initial implementation,
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/
int8_t oid_add_params(oid_t *oid, tdc_t *params)
{
	return tdc_append(&(oid->params), params);
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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

void oid_clear(oid_t* oid)
{

    AMP_DEBUG_ENTRY("oid_clear","(OID)",NULL);

    CHKVOID(oid);

    oid_clear_parms(oid);
    oid_init(oid);

    AMP_DEBUG_EXIT("oid_clear","<->", NULL);
}



/******************************************************************************
 *
 * \par Function Name: oid_clear_parms
 *
 * \par Clears the parameters associated with an OID.
 *
 * \param[in] oid The OID whose parameters are to be cleared.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/19/16  E. Birrane     Initial implementation,
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

void oid_clear_parms(oid_t* oid)
{
	tdc_clear(&(oid->params));
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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/
int8_t oid_compare(oid_t oid1, oid_t oid2, uint8_t use_parms)
{
	int result = 0;

    AMP_DEBUG_ENTRY("oid_compare","(oid1, oid2)", NULL);

    /* Step 0: Sanity check and shallow comparison in one. */

    if(((oid1.value_size != oid2.value_size)) ||
       ((oid1.nn_id != oid2.nn_id)) ||
       ((oid1.type != oid2.type)))
    {
        AMP_DEBUG_EXIT("oid_compare","->-1.", NULL);
        return -1;
    }
    

    result = memcmp(&(oid1.value), &(oid2.value), oid1.value_size);

    /* Step 1: Compare the OID value without parameters. */
    if(result != 0)
    {
        AMP_DEBUG_EXIT("oid_compare","->-1.", NULL);
        return -1;
    }

    /* Step 2: If we are matching so far, look at the parameters. */
    if(use_parms != 0)
    {
    	result = tdc_compare(&(oid1.params), &(oid2.params));
    }

    AMP_DEBUG_EXIT("oid_compare","->%d.", result);

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
 *		1. The parameters and value are deep-copied and MUST be released
 *		   by the calling function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/24/15  E. Birrane     Initial implementation,
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/
oid_t oid_construct(uint8_t type, tdc_t *parms, uvast nn_id, uint8_t *value, uint32_t size)
{
	oid_t result;

    AMP_DEBUG_ENTRY("oid_construct",
    		          "(%d, "ADDR_FIELDSPEC","UVAST_FIELDSPEC","ADDR_FIELDSPEC",%d)", type, (uaddr)parms, nn_id, (uaddr)value, size);

    oid_init(&result);

    /* Step 0: Sanity checks. */
    if((value == NULL) || (size == 0))
    {
    	AMP_DEBUG_ERR("oid_construct","Bad args.", NULL);
        AMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
    	return oid_get_null();
    }
    else if(size > MAX_OID_SIZE)
    {
    	AMP_DEBUG_ERR("oid_construct","New OID size %d > MAX OID size %d.",
    					size, MAX_OID_SIZE);
        AMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
    	return oid_get_null();
    }

    /* Step 2: Assign the OID Type. */
    result.type = type;

    /* Step 3: Assign the NN, checking if one is required.
     *
     * \todo: Defer this check, as nn_id 0 is valid for now...
    if((type == OID_TYPE_COMP_FULL) ||
       (type == OID_TYPE_COMP_PARAM))
    {
    	if(nn_id == 0)
    	{
    		AMP_DEBUG_ERR("oid_construct", "Expected nn with type %d", type);
    		SRELEASE(result);
            AMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
        	return NULL;
    	}
    }
     */

    result.nn_id = nn_id;

    /* Step 4: Assign parameters, if required. */
    if((type == OID_TYPE_PARAM) ||
       (type == OID_TYPE_COMP_PARAM))
    {
    	if(tdc_append(&(result.params), parms) == ERROR)
    	{
    		AMP_DEBUG_ERR("oid_construct", "Cannot add params.", type);
    		AMP_DEBUG_EXIT("oid_construct", "-->NULL", NULL);
        	return oid_get_null();
    	}
    }

    /* Step 5: Assign OID bytes. */
    memset(result.value, 0, MAX_OID_SIZE);
    memcpy(result.value, value, size);

    result.value_size = size;

    AMP_DEBUG_EXIT("oid_construct", "-->%d", result.type);
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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

oid_t oid_copy(oid_t src_oid)
{
    return oid_construct(src_oid.type, &(src_oid.params), src_oid.nn_id, src_oid.value, src_oid.value_size);
}



/******************************************************************************
 *
 * \par Function Name: oid_copy_parms
 *
 * \par Deep-copies parameters from a source OID to a dest OID.
 *
 * \retval ERROR - Problem
 *         SUCCESS - OK.
 *
 * \param[in|out] dest  The OID to receive a copy of the parameters.
 * \param[in]     src   The OID providing the parameters to copy.
 *
 * \par Notes:
 *		1. The destination OID MUST NOT already have parameters.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/19/16  E. Birrane     Initial implementation,
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

uint8_t oid_copy_parms(oid_t *dest, oid_t *src)
{
	int8_t num_parms = oid_get_num_parms(*dest);

	if(num_parms != 0)
	{
		AMP_DEBUG_ERR("oid_copy_parms",
				        "Cannot copy parms into OID that already has %d parms.",
				        num_parms);
		return ERROR;
	}

	return oid_add_params(dest, &(src->params));
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

oid_t oid_deserialize_comp(unsigned char *buffer,
		                    uint32_t size,
		                    uint32_t *bytes_used)
{
	uvast nn_id = 0;
	uint32_t bytes = 0;
	oid_t new_oid;
	unsigned char *cursor = NULL;

	AMP_DEBUG_ENTRY("oid_deserialize_comp","(%#llx,%d,%#llx)",
					  (unsigned long) buffer,
					  size,
					  (unsigned long) bytes_used);

	oid_init(&new_oid);

	/* Step 0: Sanity Checks. */
	if((buffer == NULL) || (bytes_used == NULL) || (size == 0))
	{
		AMP_DEBUG_ERR("oid_deserialize_comp", "Bad args.", NULL);
		AMP_DEBUG_EXIT("oid_deserialize_comp", "->NULL", NULL);
		return oid_get_null();
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/* Step 1: Grab the nickname, which is in an SDNV. */
	 if((bytes = utils_grab_sdnv(cursor, size, &nn_id)) <= 0)
	 {
		 AMP_DEBUG_ERR("oid_deserialize_comp", "Can't grab nickname.", NULL);
		 *bytes_used = 0;

		 AMP_DEBUG_EXIT("oid_deserialize_comp", "->0", NULL);
		 return oid_get_null();
	 }
	 else
	 {
		 cursor += bytes;
		 size -= bytes;
		 *bytes_used += bytes;
	 }


	/* Step 2: grab the remaining OID, which looks just like a full OID. */
	new_oid = oid_deserialize_full(cursor, size, &bytes);
	if(new_oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("oid_deserialize_comp", "Can't grab remaining OID.", NULL);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("oid_deserialize_comp", "->NULL", NULL);
		return oid_get_null();
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Store extra information in the new OID. */
	new_oid.nn_id = nn_id;
	new_oid.type = OID_TYPE_COMP_FULL;


	AMP_DEBUG_EXIT("oid_deserialize_comp","-> OID.", NULL);
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
oid_t oid_deserialize_comp_param(unsigned char *buffer,
    					         uint32_t size,
    					         uint32_t *bytes_used)
{
	uvast nn_id = 0;
	uint32_t bytes = 0;
	oid_t new_oid;
	unsigned char *cursor = NULL;

	AMP_DEBUG_ENTRY("oid_deserialize_comp_param","(%#llx,%d,%#llx)",
					  (unsigned long) buffer,
					  size,
					  (unsigned long) bytes_used);

	oid_init(&new_oid);

	/* Step 0: Sanity Checks. */
	if((buffer == NULL) || (bytes_used == NULL) || (size == 0))
	{
		AMP_DEBUG_ERR("oid_deserialize_comp_param", "Bad args.", NULL);
		AMP_DEBUG_EXIT("oid_deserialize_comp_param", "->NULL", NULL);
		return oid_get_null();
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/* Step 1: Grab the nickname, which is in an SDNV. */
	 if((bytes = utils_grab_sdnv(cursor, size, &nn_id)) <= 0)
	 {
		 AMP_DEBUG_ERR("oid_deserialize_comp_param", "Can't grab nickname.", NULL);
		 *bytes_used = 0;

		 AMP_DEBUG_EXIT("oid_deserialize_comp_param", "->0", NULL);
		 return oid_get_null();
	 }
	 else
	 {
		 cursor += bytes;
		 size -= bytes;
		 *bytes_used += bytes;
	 }


	/* Step 2: grab the parameterized, remaining OID. */
	new_oid = oid_deserialize_param(cursor, size, &bytes);
	if(new_oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("oid_deserialize_comp_param", "Can't grab remaining OID.",
				        NULL);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("oid_deserialize_comp_param", "->NULL", NULL);
		return oid_get_null();
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Store extra information in the new OID. */
	new_oid.nn_id = nn_id;
	new_oid.type = OID_TYPE_COMP_PARAM;

	AMP_DEBUG_EXIT("oid_deserialize_comp_param","-> OID.", NULL);
	return new_oid;
}



/******************************************************************************
 *
 * \par Function Name: oid_deserialize_full
 *
 * \par Purpose: Extracts a regular OID from a buffer.
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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/
oid_t oid_deserialize_full(unsigned char *buffer,
			               uint32_t size,
			               uint32_t *bytes_used)
{
	oid_t new_oid;
	unsigned char *cursor = NULL;
	blob_t *value = NULL;

	AMP_DEBUG_ENTRY("oid_deserialize_full","(%#llx,%d,%#llx)",
			         (unsigned long) buffer, size, (unsigned long) bytes_used);

	oid_init(&new_oid);

	/* Step 0: Sanity Checks. */
	if((buffer == NULL) || (bytes_used == NULL) || (size == 0))
	{
		AMP_DEBUG_ERR("oid_deserialize_full", "Bad args.", NULL);
		AMP_DEBUG_EXIT("oid_deserialize_full", "->NULL", NULL);
		return oid_get_null();
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/*
	 * Step 1: Grab the OID, which is model as a BLOB.
	 */
	if((value = blob_deserialize(cursor, size, bytes_used)) == NULL)
	{
		AMP_DEBUG_ERR("oid_deserialize_full", "Can't get OID BLOB.", NULL);
		AMP_DEBUG_EXIT("oid_deserialize_full", "-> NULL", NULL);
		return oid_get_null();
	}
	else
	{
		cursor += *bytes_used;
		size -= *bytes_used;
	}

	/*
	 * Step 2: Make sure OID fits within our size. We add 1 to oid_size to
	 *         account for the # bytes parameter.
	 */
	if((value->length) >= MAX_OID_SIZE)
	{
		AMP_DEBUG_ERR("oid_deserialize_full","Size %d exceeds supported size %d",
				        value->length, MAX_OID_SIZE);
		*bytes_used = 0;
		blob_destroy(value, 1);
		AMP_DEBUG_EXIT("oid_deserialize_full","-> NULL", NULL);
		return oid_get_null();
	}

	/* Step 3: Copy in the data contents of the OID.  We don't interpret the
	 *         values here, just put them in.
	 */

	new_oid.value_size = value->length;
	memcpy(new_oid.value, value->value, value->length);
	blob_destroy(value, 1);

	new_oid.type = OID_TYPE_FULL;

	AMP_DEBUG_EXIT("oid_deserialize_full","-> new_oid: OID  bytes_used %d",
			         *bytes_used);

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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

oid_t oid_deserialize_param(unsigned char *buffer,
		                     uint32_t size,
		                     uint32_t *bytes_used)
{
	oid_t new_oid;
	uint32_t bytes = 0;
	unsigned char *cursor = NULL;

	AMP_DEBUG_ENTRY("oid_deserialize_param","(%#llx,%d,%#llx)",
					  (unsigned long) buffer,
					  size,
					  (unsigned long) bytes_used);

	oid_init(&new_oid);

	/* Step 0: Sanity Checks... */
	if((buffer == NULL) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("oid_deserialize_param","NULL input values!",NULL);
		AMP_DEBUG_EXIT("oid_deserialize_param","->NULL",NULL);
		return oid_get_null();
	}
	else
	{
		*bytes_used = 0;
		cursor = buffer;
	}

	/* Step 1: Grab the regular OID and store it. */
	new_oid = oid_deserialize_full(cursor, size, &bytes);

	if((bytes == 0) || (new_oid.type == OID_TYPE_UNK))
	{
		AMP_DEBUG_ERR("oid_deserialize_param","Could not grab OID.", NULL);

		oid_release(&new_oid); /* release just in case bytes was 0. */
		*bytes_used = 0;

		AMP_DEBUG_EXIT("oid_deserialize_param","-> NULL", NULL);
		return oid_get_null();
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
		tdc_t *cur_tdc = tdc_deserialize(cursor, size, &bytes);

		if(cur_tdc == NULL)
		{
			AMP_DEBUG_ERR("oid_deserialize_param","Could not grab params.", NULL);

			oid_release(&new_oid); /* release just in case bytes was 0. */
			*bytes_used = 0;

			AMP_DEBUG_EXIT("oid_deserialize_param","-> NULL", NULL);
			return oid_get_null();
		}
		else
		{
			oid_add_params(&new_oid, cur_tdc);
			tdc_destroy(&cur_tdc);

			cursor += bytes;
			size -= bytes;
			*bytes_used += bytes;
		}
	}

	/* Step 3: Fill in any other parts of the OID. */
	new_oid.type = OID_TYPE_PARAM;

	AMP_DEBUG_EXIT("oid_deserialize_param","-> %d.", new_oid.type);
	return new_oid;
}



/*  06/11/16  E. Birrane     Cleanup parameters, use TDC. */

int8_t  oid_get_num_parms(oid_t oid)
{
	return tdc_get_count(&(oid.params));
}



void oid_init(oid_t *oid)
{
	CHKVOID(oid);

	oid->type = OID_TYPE_UNK;
	memset(&(oid->params), 0, sizeof(tdc_t));
	oid->nn_id = 0;
	oid->value_size = 0;
	memset(oid->value, 0, MAX_OID_SIZE);
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
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/27/12  E. Birrane     Initial implementation,
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

blob_t*  oid_get_param(oid_t oid, uint32_t idx, amp_type_e *type)
{
	blob_t *result = NULL;
	LystElt elt = 0;

	AMP_DEBUG_ENTRY("oid_get_param","(OID, %d,"ADDR_FIELDSPEC")", idx, (uaddr)type);

	CHKNULL(type);

	result = tdc_get_colentry(&(oid.params), idx);
	*type = tdc_get_type(&(oid.params), idx);

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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

char *oid_pretty_print(oid_t oid)
{
	int size = 0;
	char *str = NULL;
	char *result = NULL;
	char *cursor = NULL;
	LystElt elt;
	blob_t *entry = NULL;
	uint32_t parm_str_len = 0;
	char *parm_str = NULL;

	AMP_DEBUG_ENTRY("oid_pretty_print","(OID)", NULL);

	/* Step 0: Sanity Check. */
	if(oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("oid_pretty_print","NULL OID.",NULL);
		AMP_DEBUG_EXIT("oid_pretty_print","->NULL.",NULL);
		return NULL;
	}

	parm_str = tdc_to_str(&(oid.params));
	parm_str_len = strlen(parm_str);

	/* Step 1: Guestimate size. This is, at best, a dark art and prone to
	 *         exaggeration. Numerics are assumed to take 5 bytes, unless I
	 *         think they will take 3 instead (indices vs. values). Other
	 *         values are based on constant strings in the function. One must
	 *         update this calculation if one changes string constants, else
	 *         one will be sorry.
	 */
	size = 33 +   					   /* Header */
		   11 +   					   /* OID type as text */
		   22 +                        /* "num_parm: %ld" */
		   parm_str_len + 1 +          /* Parameters */
		   12 +                        /* nn_id */
		   18 +   					   /* value_size */
		   7 + (oid.value_size * 4) +  /* OID values */
		   22;  					   /* Footer. */


	/* Step 2: Allocate the string. */
	if((result = (char*)STAKE(size)) == NULL)
	{
		AMP_DEBUG_ERR("oid_pretty_print", "Can't alloc %d bytes.", size);
		AMP_DEBUG_EXIT("oid_pretty_print","->NULL",NULL);
		/*	TODO  Must destroy parm_str at this point.	*/
		return NULL;
	}

	/* Step 3: Populate the allocated string. Keep an eye on the size. */
	cursor = result;
	cursor += sprintf(cursor,"OID:\n---------------------\nType: ");

	switch(oid.type)
	{
	case 0: cursor += sprintf(cursor,"FULL\n"); break;
	case 1: cursor += sprintf(cursor,"PARAM\n"); break;
	case 2: cursor += sprintf(cursor,"COMP_FULL\n"); break;
	case 3: cursor += sprintf(cursor,"COMP_PARAM\n"); break;
	default: cursor += sprintf(cursor,"UNKNOWN\n"); break;
	}

	cursor += sprintf(cursor,"num_parm: %d\n", oid_get_num_parms(oid));
	cursor += sprintf(cursor,"parms: %s\n", parm_str);
	cursor += sprintf(cursor, "nn_id: " UVAST_FIELDSPEC "\n", oid.nn_id);
	cursor += printf(cursor, "value_size: %d\n", oid.value_size);

	str = oid_to_string(oid);
	cursor += sprintf(cursor, "value: %s\n---------------------\n\n", str);
	SRELEASE(parm_str);
	SRELEASE(str);

	/* Step 4: Sanity check. */
	if((cursor - result) > size)
	{
		AMP_DEBUG_ERR("oid_pretty_print", "OVERWROTE! Alloc %d, wrote %llu.",
				        size, (cursor-result));
		SRELEASE(result);
		AMP_DEBUG_EXIT("oid_pretty_print","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_INFO("oid_pretty_print","Wrote %llu into %d string.",
			         (cursor -result), size);

	AMP_DEBUG_EXIT("oid_pretty_print","->%#llx",result);

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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

void oid_release(oid_t* oid)
{
    AMP_DEBUG_ENTRY("oid_release", "(OID)", NULL);

    CHKVOID(oid);

    if(oid->type != OID_TYPE_UNK)
    {
        oid_clear(oid);
        tdc_clear(&(oid->params));
        oid_init(oid);
    }

    AMP_DEBUG_EXIT("oid_release", "-> NULL", NULL);
}


/******************************************************************************
 *
 * \par Function Name: oid_serialize
 *
 * \par Purpose: Generate full, serialized version of the OID.
 *
 *     <..opt..> <..........required........> <..........optional..........>
 *	   +--------+-------+-------+     +------+--------+-------+     +-------+
 *	   |Nickname|# Bytes| Byte 1| ... |Byte N| # Parms| Parm 1| ... | Parm N|
 *	   | (SDNV) | (SDNV)| (Byte)|     |(Byte)| (SDNV) | (SDNV)|     |(SDNV) |
 *	   +--------+-------+-------+     +------+--------+-------+     +-------+

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
 *  06/11/16  E. Birrane     Cleanup parameters, use TDC.
 *****************************************************************************/

uint8_t *oid_serialize(oid_t oid, uint32_t *size)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	uint8_t *parms = NULL;
	uint32_t parm_size = 0;
	blob_t value;
	uint8_t *data = NULL;
	uint32_t data_size = 0;

	uint32_t idx = 0;
	Sdnv nn_sdnv;

	AMP_DEBUG_ENTRY("oid_serialize","(oid,%#llx)",
			          (unsigned long)size);

	/* Step 0: Sanity Check */
	if((oid.type == OID_TYPE_UNK) || (size == NULL))
	{
		AMP_DEBUG_ERR("oid_serialize","Bad args.",NULL);
		AMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Model the OID value as a BLOB. */
	value.length = oid.value_size;
	value.value = oid.value;

	/* Step 2: Serialize the OID value. */
	if((data = blob_serialize(&value, &data_size)) == NULL)
	{
		AMP_DEBUG_ERR("oid_serialize","Can't serialize OID data.", NULL);
		return NULL;
	}

	/* Step 3: Calculate the serialized size. */
	*size = 0;


	if((oid.type == OID_TYPE_COMP_FULL) || (oid.type == OID_TYPE_COMP_PARAM))
	{
		encodeSdnv(&nn_sdnv, oid.nn_id);
		*size += nn_sdnv.length;
	}

	*size += data_size;

	if((oid.type == OID_TYPE_PARAM) || (oid.type == OID_TYPE_COMP_PARAM))
	{
		parms = tdc_serialize(&(oid.params), &parm_size);
		*size += parm_size;
	}

	if(*size == 0)
	{
		AMP_DEBUG_ERR("oid_serialize","Bad size %d.",*size);
		*size = 0;
		if(parms != NULL)
		{
			SRELEASE(parms);
		}
		SRELEASE(data);
		AMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Allocate the serialized buffer.*/
	if((result = (uint8_t *) STAKE(*size)) == NULL)
	{
		AMP_DEBUG_ERR("oid_serialize","Couldn't allocate %d bytes.",
				        *size);
		*size = 0;
		if(parms != NULL)
		{
			SRELEASE(parms);
		}
		SRELEASE(data);
		AMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor = result;
	}

	/* Step 3: Copy in the nickname portion. */
	if((oid.type == OID_TYPE_COMP_FULL) || (oid.type == OID_TYPE_COMP_PARAM))
	{
		memcpy(cursor, nn_sdnv.text, nn_sdnv.length);
		cursor += nn_sdnv.length;
	}

	/* Step 4: Copy in the value portion. */
	memcpy(cursor,data, data_size);
	cursor += data_size;
	SRELEASE(data);

	/* Step 5: Copy in the parameters portion. */
	if((oid.type == OID_TYPE_PARAM) || (oid.type == OID_TYPE_COMP_PARAM))
	{
		if((parms == NULL) || (parm_size == 0))
		{
			AMP_DEBUG_ERR("oid_serialize","Can't serialize parameters.",NULL);
			*size = 0;
			if(parms != NULL)
			{
				SRELEASE(parms);
			}

			SRELEASE(result);
			AMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
			return NULL;
		}

		memcpy(cursor, parms, parm_size);
		cursor += parm_size;
		SRELEASE(parms);
	}

	/* Step 6: Final sanity check */
	if((cursor-result) > *size)
	{
		AMP_DEBUG_ERR("oid_serialize","Serialized %d bytes but counted %d!",
				        (cursor-result), *size);
		*size = 0;
		SRELEASE(result);
		AMP_DEBUG_EXIT("oid_serialize","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("oid_serialize","->%#llx",(unsigned long)result);
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

char *oid_to_string(oid_t oid)
{
    char *result = NULL;

    AMP_DEBUG_ENTRY("oid_to_string","(OID)",NULL);

    /* Step 0: Sanity Check. */
    if(oid.type == OID_TYPE_UNK)
    {
        AMP_DEBUG_ERR("oid_to_string","NULL OID",NULL);
        AMP_DEBUG_EXIT("oid_to_string","->NULL.",NULL);
    	return NULL;
    }

    /* Step 1: Walk through and collect size. */


    /* For now, we just show the raw MID. */
	result = utils_hex_to_string(oid.value, oid.value_size);

    AMP_DEBUG_EXIT("oid_to_string","->%s.", result);
	return result;
}

