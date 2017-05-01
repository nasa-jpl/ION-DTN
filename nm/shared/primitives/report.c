/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file report.c
 **
 **
 ** Description:
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/11/13  E. Birrane     Redesign of primitives architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  07/02/15  E. Birrane     Migrated to Typed Data Collections (TDCs) (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"

#include "../msg/pdu.h"

#include "../primitives/tdc.h"
#include "../primitives/mid.h"
#include "../msg/msg_ctrl.h"

#include "report.h"



/******************************************************************************
 *
 * \par Function Name: rpt_add_entry
 *
 * \par Add an entry to an existing report.
 *
 * \param[in]  rpt   The report receiving the new entry.
 * \param[in]  entry The new entry.
 *
 * \par Notes:
 *  1. The entry is shallow-copied into the report. It MUST NOT be deallocated
 *     by the calling function.
 *
 * \return SUCCESS or FAILURE
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Initial implementation.
 *****************************************************************************/

int      rpt_add_entry(rpt_t *rpt, rpt_entry_t *entry)
{
	/* Step 0: Sanity Checks. */
	if((rpt == NULL) || (rpt->entries == NULL) || (entry == NULL))
	{
		AMP_DEBUG_ERR("rpt_add_entry","Bad Args.",NULL);
		return FAILURE;
	}

	/* Step 1: Add the entry. */
	lyst_insert_last(rpt->entries, entry);

	return SUCCESS;
}



/******************************************************************************
 *
 * \par Function Name: rpt_create
 *
 * \par Create a new report entry.
 *
 * \param[in]  time       The creation timestamp for the report.
 * \param[in]  entries    The list of current report entries (or NULL)
 * \param[in]  recipient  The recipient for this report.
 *
 * \todo
 *  - Support multiple recipients.
 *
 * \par Notes:
 *  1. The entries are shallow-copied into the report, so they MUST NOT
 *     be de-allocated by the calling function.
 *
 * \return The created report, or NULL on failure.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

rpt_t*   rpt_create(time_t time, Lyst entries, eid_t recipient)
{
	rpt_t *result = NULL;

	AMP_DEBUG_ENTRY("rpt_create","(%d,"ADDR_FIELDSPEC",%s)",
			          time, (uaddr) entries, recipient.name);

	/* Step 1: Allocate the message. */
	if((result = (rpt_t*) STAKE(sizeof(rpt_t))) == NULL)
	{
		AMP_DEBUG_ERR("rpt_create","Can't alloc %d bytes.",
				        sizeof(rpt_t));
		AMP_DEBUG_EXIT("rpt_create","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Populate the report. */
	result->time = time;
	result->entries = entries;
	if(result->entries == NULL)
	{
		if((result->entries = lyst_create()) == NULL)
		{
			AMP_DEBUG_ERR("rpt_create","Can't create entries lyst.", NULL);
			SRELEASE(result);
			return NULL;
		}
	}
	result->recipient = recipient;

	AMP_DEBUG_EXIT("rpt_create","->0x%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_clear_lyst
 *
 * \par Cleans up a lyst of reports.
 *
 * \param[in|out]  list     The lyst being cleared and maybe destroyed.
 * \param[in|out]  mutex    The mutex protected the lyst (or NULL).
 * \param[in]      destroy  Whether to destroy the list or just clear it.
 *
 * \todo: On destroy, should we also destroy the mutex?
 * \todo: Should destroy be an option?
 *
 * \par Notes:
 *  1. If destroyed, the lyst MUST NOT be used by the calling function
 *     anymore.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

void  rpt_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
    LystElt elt;
    rpt_t *cur_rpt = NULL;

    AMP_DEBUG_ENTRY("rpt_clear_lyst","("ADDR_FIELDSPEC","ADDR_FIELDSPEC", %d)",
			          (uaddr) list, (uaddr) mutex, destroy);

    /* Step 0: Sanity Checks. */
    if((list == NULL) || (*list == NULL))
    {
    	AMP_DEBUG_ERR("rpt_clear_lyst","Bad Params.", NULL);
    	return;
    }

    /* Step 1: Lock access to the list, if we have a mutex. */
    if(mutex != NULL)
    {
    	lockResource(mutex);
    }

    /* Step 2: Free any reports left in the reports list. */
    for (elt = lyst_first(*list); elt; elt = lyst_next(elt))
    {
        /* Grab the current report */
        if((cur_rpt = (rpt_t*) lyst_data(elt)) == NULL)
        {
        	AMP_DEBUG_WARN("rpt_clear_lyst","Can't get report from lyst!", NULL);
        }
        else
        {
        	rpt_release(cur_rpt);
        }
    }

    /* Step 3: Make sure the lyst is empty. */
    lyst_clear(*list);

    /* Step 4: Destroy the lyst, if necessary. */
    if(destroy != 0)
    {
    	lyst_destroy(*list);
    	*list = NULL;
    }

    /* Step 5: Allow access back to the lyst, if we have a mutex */
    if(mutex != NULL)
    {
    	unlockResource(mutex);
    }

    AMP_DEBUG_EXIT("rpt_clear_lyst","->.", NULL);
}




/******************************************************************************
 *
 * \par Function Name: rpt_deserialize_data
 *
 * \par Extracts a Report from a byte buffer. When serialized, a
 *      Report is a timestamp, the # entries as an SDNV, then a
 *      series of entries. Each entry is of type rpt_entry_t.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized Report.
 *
 * \param[in]  cursor       The byte buffer holding the data
 * \param[in]  size         The # bytes available in the buffer
 * \param[out] bytes_used   The # of bytes consumed in the deserialization.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

rpt_t*  rpt_deserialize_data(uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	rpt_t *result = NULL;
	int i = 0;
	uint32_t bytes = 0;
	uvast num_entries = 0;
	uvast time_val = 0;
	uvast rx_len = 0;
	Lyst entries = NULL;
	rpt_entry_t *cur_entry = NULL;
	char *rx = NULL;

	AMP_DEBUG_ENTRY("rpt_deserialize","("ADDR_FIELDSPEC", %d,"ADDR_FIELDSPEC")",
			          (uaddr)cursor, size, (uaddr) bytes_used);

	/* Step 0: Sanity Checks. */
	if((cursor == NULL) || (bytes_used == 0))
	{
		AMP_DEBUG_ERR("rpt_deserialize","Bad Args.",NULL);
		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}

	*bytes_used = 0;

	/* Step 1: Grab the length of the recipient. */
	if((bytes = utils_grab_sdnv(cursor, size, &rx_len)) == 0)
	{
		AMP_DEBUG_ERR("rpt_deserialize","Can't Grab time SDNV.", NULL);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 2: Copy in the recipient. */
	if((rx = (char *) STAKE(rx_len + 1)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_deserialize","Can't Alloc %d rx bytes.", rx_len + 1);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		memcpy(rx, cursor, rx_len);
		rx[rx_len] = '\0';
		cursor += rx_len;
		size -= rx_len;
		*bytes_used += rx_len;
	}

	/* Step 1: Grab the timestamp for the report. */
	if((bytes = utils_grab_sdnv(cursor, size, &time_val)) == 0)
	{
		AMP_DEBUG_ERR("rpt_deserialize","Can't Grab time SDNV.", NULL);
		*bytes_used = 0;
		SRELEASE(rx);

		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 2: Grab the # entries. */
	if((bytes = utils_grab_sdnv(cursor, size, &num_entries)) == 0)
	{
		AMP_DEBUG_ERR("rpt_deserialize","Can't Grab entries SDNV.", NULL);
		*bytes_used = 0;
		SRELEASE(rx);

		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Create the lyst of report entires. */
	if((entries = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("rpt_deserialize","Can't create entries lyst.", NULL);
		*bytes_used = 0;
		SRELEASE(rx);

		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Deserialize the entries. */
	for(i = 0; i < num_entries; i++)
	{
		if((cur_entry = rpt_entry_deserialize(cursor, size, &bytes)) == NULL)
		{
			AMP_DEBUG_ERR("rpt_deserialize","Can't create entries # %d of %d.", i, num_entries);
			*bytes_used = 0;

			rpt_entry_clear_lyst(&entries, NULL, 1);
			SRELEASE(rx);

			AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
			return NULL;
		}
		else
		{
			cursor += bytes;
			size -= bytes;
			*bytes_used += bytes;

			lyst_insert_last(entries, cur_entry);
		}
	}

	/* Step 5: Create the report. */
	eid_t tmp;

	/*	Inferred fix.  Needed to get past MacOS compile error
	 *	but may not be what is intended.  TODO			*/

	istrcpy(tmp.name, rx, sizeof tmp.name);

	/*	End of inferred fix.					*/

	if((result = rpt_create((time_t) time_val, entries, tmp)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_deserialize","Can't create entries # %d of %d.", i, num_entries);
		*bytes_used = 0;

		rpt_entry_clear_lyst(&entries, NULL, 1);
		SRELEASE(rx);

		AMP_DEBUG_EXIT("rpt_deserialize","->NULL",NULL);
		return NULL;
	}

	memcpy(result->recipient.name, rx, strlen(rx) + 1);
	SRELEASE(rx);

	AMP_DEBUG_EXIT("rpt_deserialize_data","->"ADDR_FIELDSPEC,(uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_release
 *
 *
 * \par Releases all memory allocated in the Report. Clears all entries.
 *
 * \param[in,out]  rpt    The report to be released.
 *
 * \par Notes:
 *      - The report itself is destroyed and MUST NOT be referenced after a
 *        call to this function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

void     rpt_release(rpt_t *rpt)
{
	if(rpt != NULL)
	{
		rpt_entry_clear_lyst(&(rpt->entries), NULL, 1);
		SRELEASE(rpt);
	}
}



/******************************************************************************
 *
 * \par Function Name: rpt_serialize
 *
 * \par Purpose: Generate full, serialized version of a Report. A
 *      serialized Report is of the form:
 *
 * \par +-------+--------+--------+    +--------+
 *      | Time  |    #   |  Entry |    |  Entry |
 *      |       |Entries |    1   |... |    2   |
 *      |  [TS] | [SDNV] |[BYTES] |    |[BYTES] |
 *      +-------+--------+--------+    +--------+
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized collection.
 *
 * \param[in]  rpt    The Report to be serialized.
 * \param[out] len    The size of the resulting serialized Report.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *      2. This function serializes each entry separately then allocates a
 *          consolidated buffer for the overall report.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

uint8_t* rpt_serialize(rpt_t *rpt, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	uint8_t *entries = NULL;
	uint32_t entries_len = 0;

	Sdnv time;
	Sdnv num_entries_sdnv;
	Sdnv rx_sdnv;
	uint32_t num_entries = 0;


	AMP_DEBUG_ENTRY("rpt_serialize","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
			          (uaddr)rpt, (uaddr) len);

	/* Step 0: Sanity Checks. */
	if((rpt == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("rpt_serialize","Bad Args",NULL);
		AMP_DEBUG_EXIT("rpt_serialize","->NULL",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Generate SDNVs for the timestamp and # entries. */
	encodeSdnv(&time, rpt->time);
	num_entries = lyst_length(rpt->entries);
	encodeSdnv(&num_entries_sdnv, num_entries);
	encodeSdnv(&rx_sdnv, strlen(rpt->recipient.name));

	/* Step 2: Serialize the report entries list. */
	if((entries = rpt_entry_serialize_lst(rpt->entries, &entries_len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_serialize","Can't serialize entries.",NULL);
		AMP_DEBUG_EXIT("rpt_serialize","->NULL",NULL);
		return NULL;
	}

	/* Step 3. Calculate final size of the full report set. */
	*len = time.length + num_entries_sdnv.length + entries_len + rx_sdnv.length + strlen(rpt->recipient.name);

	/* Step 4: Allocate the final full report. */
	if((result = (uint8_t*)STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_serialize", "Can't alloc %d bytes.", *len);
		SRELEASE(entries);

		*len = 0;
		AMP_DEBUG_EXIT("rpt_serialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(result,0,*len);
	}

	/* Step 5: Copy data into the buffer. */
	cursor = result;

	memcpy(cursor, rx_sdnv.text, rx_sdnv.length);
	cursor += rx_sdnv.length;

	memcpy(cursor, rpt->recipient.name, strlen(rpt->recipient.name));
	cursor += strlen(rpt->recipient.name);

	memcpy(cursor,time.text, time.length);
	cursor += time.length;

	memcpy(cursor, num_entries_sdnv.text, num_entries_sdnv.length);
	cursor += num_entries_sdnv.length;

	memcpy(cursor, entries, entries_len);
	cursor += entries_len;
	SRELEASE(entries);

	/* Step 6: Last sanity check. */
	if((cursor - result) != *len)
	{
		AMP_DEBUG_ERR("rpt_serialize","Wrote %d bytes but allocated %d",
				(unsigned long) (cursor - result), *len);
		*len = 0;
		SRELEASE(result);

		AMP_DEBUG_EXIT("rpt_serialize","->NULL",NULL);
		return NULL;
	}

	AMP_DEBUG_EXIT("rpt_serialize","->"ADDR_FIELDSPEC,(uaddr)result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_to_str
 *
 * \par Purpose: Generate a human-readable string representation of the report.
 *
 * \retval NULL - Failure
 * 		   !NULL - String representation of the report.
 *
 * \param[in]  rpt    The Report to be made into a string.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *      2. The resultant string is NULL-terminated.
 *      3. The header size MUST BE UPDATED if any changes are made to the format
 *         of the printed string in this function!
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

char*    rpt_to_str(rpt_t *rpt)
{
	char *result = NULL;
	char *entries = NULL;

	uint32_t hdr_size = 64;
	uint32_t size = 0;

	/* Step 0: Sanity Check. */
	if(rpt == NULL)
	{
		AMP_DEBUG_ERR("rpt_to_str","Bad Args.",NULL);
		return NULL;
	}

	/* Step 1: First, try and to-str the list of entries. */
	if((entries = rpt_entry_lst_to_str(rpt->entries)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_to_str","Can't to_str entry list.",NULL);
		return NULL;
	}

	/* Step 2: Calculate the overall size of the report string. */
	size = hdr_size + strlen(entries) + 1;

	/* Step 3: Allocate the return string. */
	if((result = (char *) STAKE(size)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_to_str","Can't allocate %d bytes.", size);
		SRELEASE(entries);
		return NULL;
	}

	/* Step 4: Populate the return string. */
	sprintf(result,
			"Timestamp: %d\n# Entries: %d\n------\n%s\n",
			(int) (rpt->time), (int) lyst_length(rpt->entries), entries);

	/* Step 5: Release entries memory (copied into result) and return. */
	SRELEASE(entries);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_clear_lyst
 *
 * \par Cleans up a lyst of report entries.
 *
 * \param[in|out]  list     The lyst being cleared and maybe destroyed.
 * \param[in|out]  mutex    The mutex protected the lyst (or NULL).
 * \param[in]      destroy  Whether to destroy the list or just clear it.
 *
 * \todo: On destroy, should we also destroy the mutex?
 * \todo: Should destroy be an option?
 *
 * \par Notes:
 *  1. If destroyed, the lyst MUST NOT be used by the calling function
 *     anymore.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

void rpt_entry_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy)
{
    LystElt elt;
    rpt_entry_t *cur_entry = NULL;

    AMP_DEBUG_ENTRY("rpt_entry_clear_lyst","("ADDR_FIELDSPEC","ADDR_FIELDSPEC", %d)",
			          (uaddr) list, (uaddr) mutex, destroy);

    /* Step 0: Sanity Checks. */
    if((list == NULL) || (*list == NULL))
    {
    	AMP_DEBUG_ERR("rpt_entry_clear_lyst","Bad Params.", NULL);
    	return;
    }

    /* Step 1: Lock access to the list, if we have a mutex. */
    if(mutex != NULL)
    {
    	lockResource(mutex);
    }

    /* Step 2: Free any reports left in the reports list. */
    for (elt = lyst_first(*list); elt; elt = lyst_next(elt))
    {
        /* Grab the current report */
        if((cur_entry = (rpt_entry_t*) lyst_data(elt)) == NULL)
        {
        	AMP_DEBUG_WARN("rpt_entry_clear_lyst","Can't get report from lyst!", NULL);
        }
        else
        {
        	rpt_entry_release(cur_entry);
        }
    }

    /* Step 3: Make sure the lyst is empty. */
    lyst_clear(*list);

    /* Step 4: Destroy the lyst, if necessary. */
    if(destroy != 0)
    {
    	lyst_destroy(*list);
    	*list = NULL;
    }

    /* Step 5: Allow access back to the lyst, if we have a mutex */
    if(mutex != NULL)
    {
    	unlockResource(mutex);
    }

    AMP_DEBUG_EXIT("rpt_entry_clear_lyst","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_create
 *
 * \par Creates a report entry.
 *
 * \retval NULL - Failure
 *         !NULL - The created Report Entry.
 *
 * \param[in]  mid       The identifier for this entry.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/15  E. Birrane     Initial Implementation.
 *****************************************************************************/

rpt_entry_t* rpt_entry_create(mid_t *mid)
{
	rpt_entry_t *result = NULL;

	if((result = (rpt_entry_t *) STAKE(sizeof(rpt_entry_t))) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_create","Can't allocate %d bytes.", sizeof(rpt_entry_t));
		return NULL;
	}

	if((result->contents = tdc_create(NULL, NULL, 0)) == NULL)
	{
		SRELEASE(result);
		AMP_DEBUG_ERR("rpt_entry_create","Can't allocate TDC.", NULL);
		return NULL;
	}

	if(mid != NULL)
	{
		result->id = mid_copy(mid);
	}
	else
	{
		result->id = NULL;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_deserialize
 *
 * \par Extracts a Report Entry from a byte buffer.
 *
 * \retval NULL - Failure
 *         !NULL - The created/deserialized Report Entry.
 *
 * \param[in]  cursor       The byte buffer holding the data
 * \param[in]  size         The # bytes available in the buffer
 * \param[out] bytes_used   The # of bytes consumed in the deserialization.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

rpt_entry_t* rpt_entry_deserialize(uint8_t *cursor,
		                           uint32_t size,
		                           uint32_t *bytes_used)
{
	rpt_entry_t *result = NULL;
	uint32_t bytes = 0;

	/* Step 0: Sanity Check. */
	if((cursor == NULL) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("rpt_entry_deserialize","Bad Args.",NULL);
		return NULL;
	}

	*bytes_used = 0;

	/* Step 1: Allocate the new entry. */
	if((result = (rpt_entry_t*)STAKE(sizeof(rpt_entry_t))) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_deserialize","Can't alloc %d bytes.",
				        sizeof(rpt_entry_t));

		AMP_DEBUG_EXIT("rpt_entry_deserialize","->NULL",NULL);
		return NULL;
	}

	/* Step 2: Grab the MID. */
	if((result->id = mid_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_deserialize","Can't grab MID.", NULL);
		rpt_entry_release(result);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("rpt_entry_deserialize","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: Grab the types data collection. */
	if((result->contents = tdc_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_deserialize","Can't Grab TDC.", NULL);
		rpt_entry_release(result);
		*bytes_used = 0;

		AMP_DEBUG_EXIT("rpt_deserialize_data","->NULL",NULL);
		return NULL;
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_lst_to_str
 *
 * \par Purpose: Generate a human-readable string representation of a list of
 *               report entries.
 *
 * \retval NULL - Failure
 * 		   !NULL - String representation of the report.
 *
 * \param[in]  entries  The Report Entries to be made into a string.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *      2. The resultant string is NULL-terminated.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

char *rpt_entry_lst_to_str(Lyst entries)
{
	uint8_t success = 1;
	char *result = NULL;
	char *cursor = NULL;
	char **temp_space = NULL;

	uint32_t num_entries = 0;
	uint32_t *temp_len = NULL;
	uint32_t len = 0;

	LystElt elt;
	int i = 0;
	rpt_entry_t *entry = NULL;

	/* Step 0: Sanity Check. */
	if(entries == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_lst_to_str","Bad Args",NULL);
		AMP_DEBUG_EXIT("rpt_entry_lst_to_str","->NULL",NULL);
		return NULL;
	}

	num_entries = lyst_length(entries);

	/* Step 1: Allocate individual storage for each entry. */
	temp_space = (char **) STAKE(num_entries * sizeof(char *));
	temp_len   = (uint32_t*) STAKE(num_entries * sizeof(uint32_t));

	if((temp_space == NULL) || (temp_len == NULL))
	{
		AMP_DEBUG_ERR("rpt_entry_lst_to_str",
				        "Can't allocate space for %d entries.", num_entries);

		SRELEASE(temp_space);
		SRELEASE(temp_len);
		AMP_DEBUG_EXIT("rpt_entry_lst_to_str","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(temp_space,0, num_entries * sizeof(uint8_t *));
		memset(temp_len,  0, num_entries * sizeof(uint32_t));
	}

	/* Step 2: Capture each serialized entry. */
	i = 0;
	len = 0;

	for(elt = lyst_first(entries); (elt && success); elt = lyst_next(elt))
	{
		if((entry = (rpt_entry_t*)lyst_data(elt)) != NULL)
		{
			temp_space[i] = rpt_entry_to_str(entry);
			temp_len[i] = (temp_space[i] != NULL) ? strlen(temp_space[i]) : 0;
			success = (temp_space[i] != NULL) && (temp_len[i] != 0);
			len += temp_len[i];

			i++;
		}
		else
		{
    		AMP_DEBUG_WARN("rpt_entry_lst_to_str", "%d entry NULL.",i);
    		success = 0;
		}
	}

	/* Step 3: If there was a problem, roll-back. */
	if(success == 0)
	{
		AMP_DEBUG_ERR("rpt_entry_lst_to_str","Can't to_str entries.",NULL);

		for(i = 0; i < num_entries; i++)
		{
			SRELEASE(temp_space[i]);
		}
		SRELEASE(temp_space);
		SRELEASE(temp_len);

		AMP_DEBUG_EXIT("rpt_entry_lst_to_str","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Allocate space for the serialization buffer. */
	if((result = (char *) STAKE(len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_lst_to_str","Can't allocate result of size %d.", len);

		for(i = 0; i < num_entries; i++)
		{
			SRELEASE(temp_space[i]);
		}
		SRELEASE(temp_space);
		SRELEASE(temp_len);

		AMP_DEBUG_EXIT("rpt_entry_lst_to_str","->NULL",NULL);
		return NULL;
	}

	/* Step 5: Populate the serialization buffer. */
	cursor = result;
	for(i = 0; i < num_entries; i++)
	{
		memcpy(cursor,temp_space[i], temp_len[i]);
		cursor += temp_len[i];
		SRELEASE(temp_space[i]);
	}
	SRELEASE(temp_space);
	SRELEASE(temp_len);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_release
 *
 * \par Releases all memory allocated in the Report Entry.
 *
 * \param[in,out]  entry    The report entry to be released.
 *
 * \par Notes:
 *      - The report entry is destroyed and MUST NOT be referenced after a
 *        call to this function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

void rpt_entry_release(rpt_entry_t *entry)
{
	if(entry != NULL)
	{
		mid_release(entry->id);
		tdc_destroy(&(entry->contents));
		SRELEASE(entry);
	}
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_serialize
 *
 * \par Purpose: Generate full, serialized version of a Report entry.
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized Report Entry.
 *
 * \param[in]  entry    The Report Entry to be serialized.
 * \param[out] len      The size of the resulting serialized Report Entry.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

uint8_t *rpt_entry_serialize(rpt_entry_t *entry, uint32_t *len)
{
	uint8_t *result = NULL;
	uint8_t *mid_data = NULL;
	uint8_t *tdc_data = NULL;
	uint8_t *cursor = NULL;

	uint32_t mid_len = 0;
	uint32_t tdc_len = 0;

	/* Step 0: Sanity check. */
	if((entry == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("rpt_entry_serialize","Bad Args.",NULL);
		return NULL;
	}

	*len = 0;

	/* Step 1: Serialize the MID in the entry. */
	if((mid_data = mid_serialize(entry->id, &mid_len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_serialize","Can't serialize mid.",NULL);
		return NULL;
	}

	/* Step 2: Serialize the typed data collection. */
	if((tdc_data = tdc_serialize(entry->contents, &tdc_len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_serialize","Can't serialize TDC.",NULL);
		SRELEASE(mid_data);
		return NULL;
	}

	/* Step 3: Allocate the return buffer. */
	*len = mid_len + tdc_len;
	if((result = (uint8_t *) STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_serialize","Can't allocate %d bytes.", *len);
		SRELEASE(mid_data);
		SRELEASE(tdc_data);
		return NULL;
	}

	/* Step 4: Populate the return buffer. */
	cursor = result;
	memcpy(cursor, mid_data, mid_len);
	cursor += mid_len;
	memcpy(cursor, tdc_data, tdc_len);

	SRELEASE(mid_data);
	SRELEASE(tdc_data);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: rpt_entry_serialize_lst
 *
 * \par Purpose: Generate full, serialized version of a list
 *      of report entries. This is a little tricky, since each
 *      individual entry can have variable size.
 *
 * \retval NULL - Failure serializing
 * 		   !NULL - Serialized entry list.
 *
 * \param[in]  entries  The List to be serialized.
 * \param[out] len      The size of the resulting serialized entry list.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/02/15  E. Birrane     Initial Implementation.
 *****************************************************************************/

uint8_t *rpt_entry_serialize_lst(Lyst entries, uint32_t *len)
{
	uint8_t success = 1;
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	uint8_t **temp_space = NULL;

	uint32_t num_entries = 0;
	uint32_t *temp_len = NULL;

	LystElt elt;
	int i = 0;
	rpt_entry_t *entry = NULL;

	/* Step 0: Sanity Check. */
	if((entries == NULL) || (len == NULL))
	{
		AMP_DEBUG_ERR("rpt_entry_serialize_lst","Bad Args",NULL);
		AMP_DEBUG_EXIT("rpt_entry_serialize_lst","->NULL",NULL);
		return NULL;
	}

	*len = 0;
	num_entries = lyst_length(entries);

	if(num_entries == 0)
	{
		AMP_DEBUG_ERR("rpt_entry_serialize_lst","No entries to serialize.",NULL);
		AMP_DEBUG_EXIT("rpt_entry_serialize_lst","->NULL",NULL);
		return NULL;
	}

	/* Step 1: Allocate individual storage for each entry. */
	temp_space = (uint8_t**) STAKE(num_entries * sizeof(uint8_t *));
	temp_len   = (uint32_t*) STAKE(num_entries * sizeof(uint32_t));

	if((temp_space == NULL) || (temp_len == NULL))
	{
		AMP_DEBUG_ERR("rpt_entry_serialize_lst",
				        "Can't allocate space for %d entries.", num_entries);

		SRELEASE(temp_space);
		SRELEASE(temp_len);
		*len = 0;
		AMP_DEBUG_EXIT("rpt_entry_serialize_lst","->NULL",NULL);
		return NULL;
	}
	else
	{
		memset(temp_space,0, num_entries * sizeof(uint8_t *));
		memset(temp_len,  0, num_entries * sizeof(uint32_t));
	}

	/* Step 2: Capture each serialized entry. */
	i = 0;

	for(elt = lyst_first(entries); (elt && success); elt = lyst_next(elt))
	{
		if((entry = (rpt_entry_t*)lyst_data(elt)) != NULL)
		{
			temp_space[i] = rpt_entry_serialize(entry, &(temp_len[i]));
			success = (temp_space[i] != NULL) && (temp_len[i] != 0);
			*len += temp_len[i];

			i++;
		}
		else
		{
    		AMP_DEBUG_WARN("rpt_entry_serialize_lst", "%d entry NULL.",i);
    		success = 0;
		}
	}

	/* Step 3: If there was a problem, roll-back. */
	if(success == 0)
	{
		AMP_DEBUG_ERR("rpt_entry_serialize_lst","Can't serialize reports.",NULL);

		for(i = 0; i < num_entries; i++)
		{
			SRELEASE(temp_space[i]);
		}
		SRELEASE(temp_space);
		SRELEASE(temp_len);

		*len = 0;
		AMP_DEBUG_EXIT("rpt_entry_serialize_lst","->NULL",NULL);
		return NULL;
	}

	/* Step 4: Allocate space for the serialization buffer. */
	if((result = (uint8_t*) STAKE(*len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_serialize_lst","Can't allocate result of size %d.",*len);

		for(i = 0; i < num_entries; i++)
		{
			SRELEASE(temp_space[i]);
		}
		SRELEASE(temp_space);
		SRELEASE(temp_len);

		*len = 0;
		AMP_DEBUG_EXIT("rpt_entry_serialize_lst","->NULL",NULL);
		return NULL;
	}

	/* Step 5: Populate the serialization buffer. */
	cursor = result;
	for(i = 0; i < num_entries; i++)
	{
		memcpy(cursor,temp_space[i], temp_len[i]);
		cursor += temp_len[i];
		SRELEASE(temp_space[i]);
	}
	SRELEASE(temp_space);
	SRELEASE(temp_len);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: rpt_entry_to_str
 *
 * \par Purpose: Generate a human-readable string representation of a
 *               Report Entry.
 *
 * \retval NULL - Failure
 * 		   !NULL - String representation of the report entry.
 *
 * \param[in]  entry    The Report Entry to be made into a string.
 *
 * \par Notes:
 *		1. The result is allocated on the memory pool and must be released when
 *         no longer needed.
 *      2. The resultant string is NULL-terminated.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/03/15  E. Birrane     Report cleanup and transition to TDCs.
 *****************************************************************************/

char *rpt_entry_to_str(rpt_entry_t *entry)
{
	char *tdc_str = NULL;
	char *mid_str = NULL;
	char *result = NULL;
	uint32_t tdc_len = 0;
	uint32_t mid_len = 0;
	uint32_t overhead = 20;

	/* Step 0: Sanity Check. */
	if(entry == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_to_str","Bad Args.", NULL);
		return NULL;
	}

	/* Step 1: Make a string for the MID. */
	if((mid_str = mid_to_string(entry->id)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_to_str","Can't to_str mid.", NULL);
		return NULL;
	}
	mid_len = strlen(mid_str);

	/* Step 2: Make a string for the TDC. */
	if((tdc_str = tdc_to_str(entry->contents)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_to_str","Can't to_str tdc.", NULL);
		SRELEASE(mid_str);
		return NULL;
	}
	tdc_len = strlen(tdc_str);

	/* Step 3: Allocate the result. */
	if((result = (char *) STAKE(overhead+mid_len+tdc_len)) == NULL)
	{
		AMP_DEBUG_ERR("rpt_entry_to_str","Can't allocate %d bytes.", overhead+mid_len+tdc_len);
		SRELEASE(mid_str);
		SRELEASE(tdc_str);
		return NULL;
	}

	/* Step 4: Populate the result. */
	sprintf(result,"MID:%s\nCONTENTS:\n%s\n", mid_str, tdc_str);

	/* Step 5: Release memory. */
	SRELEASE(mid_str);
	SRELEASE(tdc_str);

	return result;
}
