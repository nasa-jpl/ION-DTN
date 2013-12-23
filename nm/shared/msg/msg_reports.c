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
 ** \file msg_reports.c
 **
 **
 ** Description: Defines the serialization and de-serialization methods
 **              used to translate data types into byte streams associated
 **              with DTNMP messages, and vice versa.
 **
 **\todo Add more checks for overbounds reads on deserialization.
 **
 ** Notes:
 ** 			 Not all fields of internal structures are serialized into/
 **				 deserialized from DTNMP messages.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/09/11  M. Reid        Initial Implementation (in other files)
 **  11/02/12  E. Birrane     Redesign of messaging architecture.
 **  01/11/13  E. Birrane     Migrate data structures to primitives.
 *****************************************************************************/

#include "platform.h"
#include "ion.h"

#include "shared/utils/utils.h"

#include "shared/primitives/mid.h"

#include "shared/msg/pdu.h"
#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_ctrl.h"


/* Serialize functions. */
uint8_t *rpt_serialize_lst(rpt_items_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	uint8_t *contents = NULL;
	uint32_t contents_len = 0;

	DTNMP_DEBUG_ENTRY("rpt_serialize_lst","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("rpt_serialize_lst","Bad Args",NULL);
		DTNMP_DEBUG_EXIT("rpt_serialize_lst","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* STEP 2: Serialize the Contents. */
	if((contents = midcol_serialize(msg->contents, &contents_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_lst","Can't serialize hdr.",NULL);

		DTNMP_DEBUG_EXIT("rpt_serialize_lst","->NULL",NULL);
		return NULL;
	}


	/* Step 4: Figure out the length. */
	*len = contents_len;

	/* STEP 5: Allocate the serialized message. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_lst","Can't alloc %d bytes", *len);
		*len = 0;
		MRELEASE(contents);

		DTNMP_DEBUG_EXIT("rpt_serialize_lst","->NULL",NULL);
		return NULL;
	}

	/* Step 6: Populate the serialized message. */
	cursor = result;

	memcpy(cursor, contents, contents_len);
	cursor += contents_len;
	MRELEASE(contents);

	/* Step 7: Last sanity check. */
	if((cursor - result) != *len)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_lst","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("rpt_serialize_lst","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("rpt_serialize_lst","->0x%x",(unsigned long)result);
	return result;
}


uint8_t *rpt_serialize_defs(rpt_defs_t *msg, uint32_t *len)
{
	/* \todo Implement this. */
	*len = 0;
	return NULL;
}

uint8_t *rpt_serialize_data_entry(rpt_data_entry_t *entry, uint32_t *len)
{
	uint8_t *id = NULL;
	uint32_t id_len = 0;

	Sdnv size;

	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	/* Step 0: Sanity Check. */
	if((entry == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data_entry", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("rpt_serialize_data_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Serialize the ID */
	if((id = mid_serialize(entry->id, &id_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data_entry", "Can't serialize id.", NULL);
		*len = 0;

		DTNMP_DEBUG_EXIT("rpt_serialize_data_entry","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Serialize the size value in an SDNV. */
	encodeSdnv(&size,entry->size);

	/* Step 3: Calculate the size of the serialized entry. */
	*len = id_len + entry->size + size.length;

	/* Step 4: Allocate the serialized entry space. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data_entry", "Can't alloc %d bytes.",
				         *len);
		MRELEASE(id);
		*len = 0;

		DTNMP_DEBUG_EXIT("rpt_serialize_data_entry","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,*len);
	}

	/* Step 5: Copy in the pieces. */
	cursor = result;
	memcpy(cursor,id,id_len);
	cursor += id_len;
	MRELEASE(id);

	memcpy(cursor,size.text, size.length);
	cursor += size.length;

	memcpy(cursor,entry->contents,entry->size);
	cursor += entry->size;

	/* Step 6: Last sanity check. */
	if((cursor - result) != *len)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data_entry",
				        "Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("rpt_serialize_data_entry","->NULL",NULL);
		return NULL;
	}


	DTNMP_DEBUG_EXIT("rpt_serialize_data_entry","->0x%x",
			         (unsigned long) result);
	return result;
}


uint8_t *rpt_serialize_data(rpt_data_t *msg, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	Sdnv time;
	Sdnv num_rpts_sdnv;
	uint32_t num_rpts;


	DTNMP_DEBUG_ENTRY("rpt_serialize_data","(0x%x, 0x%x)",
			          (unsigned long)msg, (unsigned long) len);

	/* Step 0: Sanity Checks. */
	if((msg == NULL) || (len == NULL))
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data","Bad Args",NULL);
		DTNMP_DEBUG_EXIT("rpt_serialize_data","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 2: Serialize the time and # reports. */
	encodeSdnv(&time, msg->time);
	num_rpts = lyst_length(msg->reports);
	encodeSdnv(&num_rpts_sdnv, num_rpts);


	/* STEP 4: Allocate individual storage for each report. */
	uint8_t **temp_space = NULL;
	uint32_t *temp_len = NULL;
	LystElt elt;

	temp_space = (uint8_t**)MTAKE(num_rpts * sizeof(uint8_t *));
	temp_len = (uint32_t*)MTAKE(num_rpts * sizeof(uint32_t));

	if((temp_space == NULL) || (temp_len == NULL))
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data","Can't allocate %d bytes.",
				        (num_rpts * sizeof(uint8_t *)));

		MRELEASE(temp_space);
		MRELEASE(temp_len);
		*len = 0;
		DTNMP_DEBUG_EXIT("rpt_serialize_data","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(temp_space,0,num_rpts * sizeof(uint8_t *));
		memset(temp_len,0,num_rpts * sizeof(uint32_t));
	}

	/* Step 5: Capture each serialized report. */
	int i = 0;
	uint8_t success = 1;
	uint32_t bytes = 0;
	for(elt = lyst_first(msg->reports); (elt && success); elt = lyst_next(elt))
	{
		rpt_data_entry_t *entry = (rpt_data_entry_t*)lyst_data(elt);
		if(entry != NULL)
		{
			temp_space[i] = rpt_serialize_data_entry(entry, &(temp_len[i]));
			success = (temp_space[i] != NULL) && (temp_len[i] != 0);
			bytes += temp_len[i];

			i++;
		}
		else
		{
    		DTNMP_DEBUG_WARN("rpt_serialize_data", "NULL item in report list.",NULL);
    		success = 0;
		}
	}

	if(success == 0)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data","Can't serialize reports.",NULL);

		for(i = 0; i < num_rpts; i++)
		{
			MRELEASE(temp_space[i]);
		}
		MRELEASE(temp_space);
		MRELEASE(temp_len);

		*len = 0;
		DTNMP_DEBUG_EXIT("rpt_serialize_data","->NULL",NULL);
		return NULL;
	}


	/* Step 6. Calculate final size of the full report set. */
	*len = time.length + num_rpts_sdnv.length + bytes;

	/* Step 7: Allocate the final full report. */
	if((result = (uint8_t*)MTAKE(*len)) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data","Can't alloc %d bytes.",
				        *len);

		for(i = 0; i < num_rpts; i++)
		{
			MRELEASE(temp_space[i]);
		}
		MRELEASE(temp_space);
		MRELEASE(temp_len);

		*len = 0;
		DTNMP_DEBUG_EXIT("rpt_serialize_data","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,*len);
	}

	/* Step 8: Copy data into the buffer. */
	cursor = result;

	memcpy(cursor,time.text, time.length);
	cursor += time.length;

	memcpy(cursor, num_rpts_sdnv.text, num_rpts_sdnv.length);
	cursor += num_rpts_sdnv.length;

	for(i = 0; i < num_rpts; i++)
	{
		memcpy(cursor,temp_space[i], temp_len[i]);
		cursor += temp_len[i];
		MRELEASE(temp_space[i]);
	}
	MRELEASE(temp_space);
	MRELEASE(temp_len);

	/* Step 9: Last sanity check. */
	if((cursor - result) != *len)
	{
		DTNMP_DEBUG_ERR("rpt_serialize_data","Wrote %d bytes but allcated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		MRELEASE(result);

		DTNMP_DEBUG_EXIT("rpt_serialize_data","->NULL",NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("rpt_serialize_data","->0x%x",(unsigned long)result);
	return result;
}


uint8_t *rpt_serialize_prod(rpt_prod_t *msg, uint32_t *len)
{
	/* \todo: Finish this one. */
	*len = 0;
	return NULL;
}


/* Deserialize functions. */
rpt_items_t *rpt_deserialize_lst(uint8_t *cursor,
		                         uint32_t size,
		                         uint32_t *bytes_used)
{
	rpt_items_t *result = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("rpt_deserialize_lst","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			           size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_lst","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rpt_deserialize_lst","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;

	/* Step 1: Allocate the new message structure. */
	if((result = (rpt_items_t*)MTAKE(sizeof(rpt_items_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_lst","Can't Alloc %d Bytes.",
				        sizeof(rpt_items_t));
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("rpt_deserialize_lst","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(rpt_items_t));
	}

	/* Step 2: Deserialize the message. */

	/* Grab the contents MC. */
	if((result->contents = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_lst","Can't grab ID MID.",
				        NULL);
		*bytes_used = 0;
		rpt_release_lst(result);
		DTNMP_DEBUG_EXIT("rpt_deserialize_lst","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	DTNMP_DEBUG_EXIT("rpt_deserialize_lst","->0x%x",(unsigned long)result);
	return result;
}

rpt_defs_t *rpt_deserialize_defs(uint8_t *cursor,
		                       	 uint32_t size,
		                       	 uint32_t *bytes_used)
{
	/* \todo: Implement this. */
	*bytes_used = 0;
	return NULL;
}

rpt_data_t *rpt_deserialize_data(uint8_t *cursor,
		                       	 uint32_t size,
		                       	 uint32_t *bytes_used)
{
	rpt_data_t *result = NULL;
	int i = 0;
	uint32_t bytes = 0;
	uvast num_rpts = 0;
	uvast tmp_val;
	rpt_data_entry_t *cur_entry = NULL;


	DTNMP_DEBUG_ENTRY("rpt_deserialize_data","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor,
			          size, (unsigned long) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_data","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;

	/* Step 1: Allocate the new message structure. */
	if((result = (rpt_data_t*)MTAKE(sizeof(rpt_data_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't Alloc %d Bytes.",
				        sizeof(rpt_data_t));
		*bytes_used = 0;
		DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(rpt_data_t));
		/* We build lyst in this function, so create it here. */
		result->reports = lyst_create();
		*bytes_used = 0;
	}


	/* Step 2: Deserialize the message. */

	/* Grab the timestamp. */
	if((bytes = utils_grab_sdnv(cursor, size, &tmp_val)) == 0)
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't Grab time SDNV.",
				        sizeof(rpt_data_t));
		rpt_release_data(result);
		*bytes_used = 0;

		DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
		result->time = tmp_val;
	}

	/* Grab the # reports. */
	if((bytes = utils_grab_sdnv(cursor, size, &tmp_val)) == 0)
	{
		DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't Grab time SDNV.",
				        sizeof(rpt_data_t));
		rpt_release_data(result);
		*bytes_used = 0;

		DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
		num_rpts = tmp_val;
	}

	/* For each report, deserialize and add it to the lyst. */
	for(i = 0; i < num_rpts; i++)
	{
		/* Allocate new entry type. */
		if((cur_entry = (rpt_data_entry_t*)MTAKE(sizeof(rpt_data_entry_t))) == NULL)
		{
			DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't alloc %d bytes.",
					        sizeof(rpt_data_entry_t));
			rpt_release_data(result);
			*bytes_used = 0;

			DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
			return NULL;
		}

		/* Grab the MID. */
		if((cur_entry->id = mid_deserialize(cursor,size,&bytes)) == NULL)
		{
			DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't grab %dth MID.",
					        i);
			rpt_release_data(result);
			MRELEASE(cur_entry);
			*bytes_used = 0;

			DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
			return NULL;
		}
		else
		{
			cursor += bytes;
			size -= bytes;
			*bytes_used += bytes;
		}

		/* Grab the size. */
		if((bytes = utils_grab_sdnv(cursor, size, &tmp_val)) == 0)
		{
			DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't Grab %dth size.",
					        i);
			rpt_release_data(result);
			MRELEASE(cur_entry);
			*bytes_used = 0;

			DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
			return NULL;
		}
		else
		{
			cursor += bytes;
			size -= bytes;
			*bytes_used += bytes;
			cur_entry->size = tmp_val;
		}

		/* Allocate the size. */
		if((cur_entry->contents = (uint8_t*)MTAKE(cur_entry->size)) == NULL)
		{
			DTNMP_DEBUG_ERR("rpt_deserialize_data","Can't Alloc %d.",
					        cur_entry->size);
			rpt_release_data(result);
			MRELEASE(cur_entry);
			*bytes_used = 0;

			DTNMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
			return NULL;
		}

		/* Copy over the data. */
		memcpy(cur_entry->contents, cursor, cur_entry->size);
		cursor += cur_entry->size;
		size -= cur_entry->size;
		*bytes_used += cur_entry->size;

		lyst_insert_last(result->reports, cur_entry);
	}

	result->size = *bytes_used;

	DTNMP_DEBUG_EXIT("rpt_deserialize_data","->0x%x",(unsigned long)result);
	return result;

}

rpt_prod_t *rpt_deserialize_prod(uint8_t *cursor,
		                         uint32_t size,
		                         uint32_t *bytes_used)
{
	/* \todo: Implement this. */
	*bytes_used = 0;
	return NULL;
}
