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
 ** \file msg_def.c
 **
 ** Description:
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/02/12  E. Birrane     Initial Implementation
 *****************************************************************************/

#include "platform.h"
#include "ion.h"


#include "shared/primitives/def.h"
#include "shared/msg/msg_def.h"


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

	/* STEP 2: Serialize the Contents. */
	if((contents = midcol_serialize(def->contents, &contents_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Can't serialize hdr.",NULL);
		MRELEASE(id);

		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 5: Figure out the length. */
	*len = id_len + contents_len;

	/* STEP 6: Allocate the serialized message. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("def_serialize_gen","Can't alloc %d bytes", *len);
		*len = 0;
		MRELEASE(id);
		MRELEASE(contents);

		DTNMP_DEBUG_EXIT("def_serialize_gen","->NULL",NULL);
		return NULL;
	}

	/* Step 7: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, id, id_len);
	cursor += id_len;
	MRELEASE(id);

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	MRELEASE(contents);

	/* Step 6: Last sanity check. */
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


/* Deserialize functions. */

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
