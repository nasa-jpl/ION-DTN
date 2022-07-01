/*****************************************************************************
 **
 ** File Name: bpsec_policy_eventset.c
 **
 ** Description: Eventsets are named collections of events (and their actions)
 **              that can be associated with policy rules.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YYYY  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  01/07/2021  E. Birrane &   Initial implementation
 **              S. Heiner
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_policy_eventset.h"

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/




/******************************************************************************
 * @brief Add an eventset to the bpsec policy engine
 *
 * Create an event set in working memory. This function first checks that the
 * eventset name is unique (there is not a named eventset with the same name
 * already defined). The eventset is then created and associated with an
 * auto-generated ID.
 *
 * @param[in]  wm			 PsmPartition ION working memory.
 * @param[in]  name  		 Name of the event set to be added.
 * @param[in]  desc  		 The (optional) description of the event set.
 *
 * @note
 * The new Event Set is not associated with any security policy rules
 * when it is first created. An Event Set must be created before it
 * can be used by a security policy rule.
 * \par
 * The new Event Set is not associated with any Events when it is first created.
 *
 * @retval -1  - Error.
 * @retval  0  - Failure to add event set
 * @retval >0  - Event set successfully added
 *****************************************************************************/

int bsles_add(PsmPartition wm, char *name, char *desc)
{
	PsmAddress addr = 0;
	SecVdb	*secvdb = getSecVdb();

	CHKERR(name);
	if (secvdb == NULL) return -1;

	/* Verify that event set name is unique */
	if(bsles_get_ptr(wm, name) != NULL)
	{
		writeMemoNote("This event set is already defined", name);
		return 0;
	}

	/* Create and persist the event set. */
	if(bsles_create(wm, name, desc, 0, &addr))
	{
		sm_rbt_insert(wm, secvdb->bpsecEventSet, addr, bsles_cb_rbt_key_comp, name);
		bsles_sdr_persist(wm, addr);
		return 1;
	}

	return -1;
}



/******************************************************************************
 * @brief Add an event to an eventset
 *
 * Added an event to an eventset involves both updating the eventset in memory
 * and updating the eventset in the SDR.
 *
 * @param[in]   wm        PsmPartition ION working memory.
 * @param[out]  esPtr     The eventset being updated with a new event
 * @param[in]   eventAddr The address of the event object
 * @param[in]   eventId   The ID of the event
 *
 * @retval -1  - Error.
 * @retval  0  - Event not added
 * @retval  1  - Event added to the event set
 *****************************************************************************/

int bsles_add_event(PsmPartition wm, BpSecEventSet *esPtr, PsmAddress eventAddr, BpSecEventId eventId)
{
	int result = 0;

	CHKERR(wm);
	CHKERR(esPtr);
	CHKERR(eventAddr);

	if(bsles_set_event(esPtr, eventId) <= 0)
	{
		writeMemo("[?] Could not configure event for eventset");
		return 0;
	}

	/*
	 * Adding a unique event is just adding the event object to the end of the
	 * list of events for this event set.
	 */
	CHKERR(sm_list_insert_last(wm, esPtr->events, eventAddr));

	PsmAddress esAddr = bsles_get_addr(wm, esPtr->name);

	/* Re-persist the new event set by deleting/re-saving to the SDR. */
	if((result = bsles_sdr_forget(wm, esPtr->name)) > 0)
	{
		result = bsles_sdr_persist(wm, esAddr);
	}

	return result;
}



/******************************************************************************
 * @brief Remove an event from an eventset
 *
 * @param[in]     wm      - PsmPartition ION working memory.
 * @param[in|out] esPtr   - The eventset whose event is being removed
 * @param[in]     eventId - The event to be removed from the event set
 *
 * @note
 * If the event was not set, we still return 1 to indicate that the event is
 * not in the eventset
 *
 * @retval -1  - Error.
 * @retval  0  - Event could not be cleared
 * @retval  1  - Event cleared
 *****************************************************************************/

int	bsles_clear_event(PsmPartition wm, BpSecEventSet *esPtr, BpSecEventId eventId)
{
	PsmAddress elt = 0;
	PsmAddress curEventAddr = 0;
	BpSecEvent *curEventPtr = NULL;

	/* Sanity checks. */
	CHKERR(esPtr);

	if(eventId <= 0)
	{
		return 1;
	}

	/* Remove the event from the event mask if it was there. */
	esPtr->mask &= ~eventId;

	/* Find the event object and remove it from the event set list of events. */
	for(elt = sm_list_first(wm, esPtr->events); elt; elt = sm_list_next(wm, elt))
	{
		curEventAddr = sm_list_data(wm, elt);
		curEventPtr = (BpSecEvent *) psp(wm, curEventAddr);
		if(curEventPtr->id == eventId)
		{
			/* Free the event and remove the (freed) address from the list. */
			psm_free(wm, curEventAddr);
			memset(curEventPtr, 0, sizeof(BpSecEvent));
			sm_list_delete(wm, elt, NULL, NULL);
			break;
		}
	}

	return 1;
}



/******************************************************************************
 * @brief Create (allocate) an event set.
 *
 * This function creates an eventset, but does not add it to the list of
 * eventsets. This is for future support of anonymous event sets which are
 * created, but stored in a policyrule, and not in a named eventset list.
 *
 * @param[in]  wm      - PsmPartition ION working memory.
 * @param[in]  name    - The name of the new eventset.
 * @param[in]  desc    - The description of the new eventset.
 * @param[in]  ruleCnt - How many rules are associated with this eventset.
 * @param[out] addr    - The address of the allocated eventset.
 *
 * @note
 * The rule count may be 1 if we are creating an anonymous eventset. Otherwise
 * it should start at 0.
 *
 * @retval !NULL - The created eventset object
 * @retval NULL  - There was an error creating the eventset.
 *****************************************************************************/

BpSecEventSet *bsles_create(PsmPartition wm, char *name, char *desc, uint8_t ruleCnt, PsmAddress *addr)
{
	BpSecEventSet *esPtr = NULL;
	CHKNULL(addr);

	*addr = psm_zalloc(wm, sizeof(BpSecEventSet));

	if(*addr)
	{
		esPtr = (BpSecEventSet *) psp(wm, *addr);
		memset(esPtr, 0, sizeof(BpSecEventSet));

		/* Populate the new event set */
		if(name)
		{
			istrcpy(esPtr->name, name, MAX_EVENT_SET_NAME_LEN);
		}
		if(desc)
		{
			istrcpy(esPtr->desc, desc, MAX_EVENT_SET_DESC_LEN);
		}
		esPtr->ruleCount = ruleCnt;
		esPtr->events = sm_list_create(wm);

		return esPtr;
	}

	writeMemoNote("[?] Could not allocate eventset", name);
	return NULL;
}



/******************************************************************************
 * @brief Delete an eventset from the policy engine and underling SDR.
 *
 * This function checks to ensure the event set identified by name exists in
 * the system and is not referenced by any security policy rules before
 * deleting it from the database.
 *
 * @param[in] wm      - PsmPartition ION working memory.
 * @param[in] name - The name of the eventset to be deleted.
 *
 * @note
 * An Eventset cannot be deleted if it is currently referenced by one or more
 * security policy rules.
 * \par
 * Attempting to delete a nonexistent event set is seen as success because the
 * eventset is no longer in existence after the call.
 * \par
 * The deleted eventset is also destroyed and MUST NOT be referenced after a
 * call to this function.
 *
 * @retval -1  - Error.
 * @retval  0  - Eventset could not be deleted
 * @retval  1  - Eventset deleted
 *****************************************************************************/

int bsles_delete(PsmPartition wm, char *name)
{
	BpSecEventSet *esPtr = NULL;
	PsmAddress esAddr = 0;
	SecVdb	*secvdb = getSecVdb();

	CHKERR(wm);
	CHKERR(name);
	if (secvdb == NULL) return -1;

	/* Verify that eventset is currently defined */
	if ((esAddr = bsles_get_addr(wm, name)) == 0)
	{
		return 1;
	}

	esPtr = (BpSecEventSet*) psp(wm, esAddr);
	CHKERR(esPtr);

	if (esPtr->ruleCount > 0)
	{
		putErrmsg("Can't delete event set - referenced by policy rule", esPtr->name);
		return 0;
	}

	/* Remove the event set from the RBT. */
	sm_rbt_delete(wm, secvdb->bpsecEventSet, bsles_cb_rbt_key_comp, (void*) esPtr, NULL, NULL);

	/* Delete the event set from the SDR. */
	if(bsles_sdr_forget(wm, esPtr->name) <= 0)
	{
		putErrmsg("Could not remove eventset from SDR.", esPtr->name);
	}

	/* Remove the event set from working memory. */
	return bsles_destroy(wm, esAddr, esPtr);
}



/******************************************************************************
 * @brief Release all resources associated with an eventset
 *
 * @param[in] wm      - PsmPartition ION working memory.
 * @param[in] esAddr  - The address of the eventset object
 * @param[in] esPtr   - The eventset object
 *
 * @note
 * The eventset (and any events) MUST NOT be used after a call to this function.
 *
 * @retval  1  - Eventset destroyed
 *****************************************************************************/

int bsles_destroy(PsmPartition wm, PsmAddress esAddr, BpSecEventSet *esPtr)
{
	/* Destroy all events associated with the eventset. */
	sm_list_destroy(wm, esPtr->events, bsles_cb_smlist_del, NULL);

	/* Reset space used by eventset and free memory. */
	memset(esPtr, 0, sizeof(BpSecEventSet));
	psm_free(wm, esAddr);

	return 1;
}



/******************************************************************************
 * @brief Retrieve the address of an eventset given its name
 *
 * @param[in] wm      - PsmPartition ION working memory.
 * @param[in] name - The name of the desired eventset
 *
 * @retval !0 - The address of the eventset
 * @retval  0 - The eventset could not be found.
 *****************************************************************************/

PsmAddress bsles_get_addr(PsmPartition wm, char *name)
{
	PsmAddress nodeAddr = 0;
	SecVdb	*secvdb = getSecVdb();

	CHKZERO(name);
	if (secvdb == NULL) return 0;

	nodeAddr = sm_rbt_search(wm, secvdb->bpsecEventSet, bsles_cb_rbt_key_comp, name, NULL);

	return (nodeAddr) ? sm_rbt_data(wm, nodeAddr) : 0;
}



/******************************************************************************
 * @brief Retrieve all eventsets defined in the system
 *
 * @param[in] wm      - PsmPartition ION working memory.
 *
 * @retval !NULL - A list of all known evensets
 * @retval  NULL - There was an error
 *****************************************************************************/

Lyst bsles_get_all(PsmPartition wm)
{
	Lyst eventsets = lyst_create();
	PsmAddress elt = 0;
	PsmAddress esAddr = 0;
	BpSecEventSet *esPtr = NULL;
	SecVdb	*secvdb = getSecVdb();

	CHKNULL(eventsets);
	if (secvdb == NULL) return NULL;

	for (elt = sm_rbt_first(wm, secvdb->bpsecEventSet);
		 elt;
		 elt = sm_rbt_next(wm, elt))
	{
		esAddr = sm_rbt_data(wm, elt);
		esPtr = (BpSecEventSet *) psp(wm, esAddr);
		if(esPtr)
		{
			lyst_insert_last(eventsets, esPtr);
		}
		else
		{
			putErrmsg("NULL Event set found in ES RBT.", NULL);
		}
	}

	return eventsets;
}



/******************************************************************************
 * @brief Retrieve an event object associated with an eventset
 *
 * @param[in] wm      - PsmPartition ION working memory.
 * @param[in] esPtr   - The eventset holding the event object
 * @param[in] eventId - the identifier for the event beinr requested.
 *
 * @retval !NULL - The retrieved event object
 * @retval  NULL - The event was not found, or there was an error
 *****************************************************************************/

BpSecEvent *bsles_get_event(PsmPartition wm, BpSecEventSet *esPtr, BpSecEventId eventId)
{
	PsmAddress elt = 0;
	BpSecEvent *event = NULL;

	CHKNULL(esPtr);

	for(elt = sm_list_first(wm, esPtr->events); elt; elt = sm_list_next(wm, elt))
	{
		event = (BpSecEvent *) psp(wm, sm_list_data(wm, elt));

		if (event->id == eventId)
		{
			return event;
		}
	}

	return NULL;
}



/******************************************************************************
 * @brief Retrieve an existing eventset object given its name
 *
 * @param[in] wm   - PsmPartition ION working memory.
 * @param[in] name - The name of the eventset being sought
 *
 * @retval !NULL - The retrieved eventset object
 * @retval  NULL - The eventset was not found, or there was an error
 *****************************************************************************/

BpSecEventSet* bsles_get_ptr(PsmPartition wm, char *name)
{
	return psp(wm, bsles_get_addr(wm, name));
}



/******************************************************************************
 * @brief Determine if an evenset matches a given name
 *
 * @param[in] es1  - An eventset
 * @param[in] name - an eventset name
 *
 * @note
 * It isn't clear what the behavior of this callback should be on bad input.
 *
 * @retval result of strcmp. 0 means the name matches the eventset.
 * @retval -1 error
 *****************************************************************************/

int bsles_match(BpSecEventSet *es1, char *name)
{
	CHKERR(es1);
	CHKERR(name);

	return strcmp(es1->name, name);
}



/******************************************************************************
 * @brief Sets an event flag noting a new eventin an eventset.
 *
 * @param[in|out] esPtr   - Eventset whose mask value should be modified
 * @param[in]     eventId - Identifier of event for which mask bit should be set
 *
 * @retval -1  - Error.
 * @retval  0  - Failure to set bit.
 * @retval >0  - Event bit set.
 *****************************************************************************/

int	bsles_set_event(BpSecEventSet *esPtr, BpSecEventId eventId)
{
	CHKERR(esPtr);

	if(eventId == 0)
	{
		return 0;
	}

	esPtr->mask |= eventId;
	return 1;
}



/******************************************************************************
 * @brief Construct an eventset from a serialized buffer.
 *
 * @param[in]     wm           - PsmPartition ION working memory.
 * @param[out]    eventSetAddr - The address of the new eventset.
 * @param[in]     buffer       - The buffer holding the serialized eventset
 * @param[in,out] bytes_left   - Bytes remaining in the buffer.
 *
 * @retval -1  - Error.
 * @retval  0  - The evenset was not deserialized.
 * @retval  >0 - The number of bytes read into the buffer.
 *****************************************************************************/

int bsles_sdr_deserialize(PsmPartition wm, PsmAddress *eventSetAddr, char *buffer, int *bytes_left)
{
	BpSecEventSet *eventSetPtr = NULL;
	char *cursor = buffer;
	uint8_t max_events = 0;
	PsmAddress curEventAddr = 0;
	BpSecEvent *curEventPtr = NULL;
	int i = 0;

	/* Allocate a new eventset and initialize. */
	if((*eventSetAddr = psm_zalloc(wm, sizeof(BpSecEventSet))) == 0)
	{
		return 0;
	}

	eventSetPtr = (BpSecEventSet*) psp(wm, *eventSetAddr);
	memset(eventSetPtr, 0, sizeof(BpSecEventSet));


	/*
	 * Walk through the serialized buffer and extract information into the
	 * newly created eventset object.
	 */
	cursor += bsl_bufread(&(eventSetPtr->name),     cursor, sizeof(eventSetPtr->name),      bytes_left);
	cursor += bsl_bufread(&(eventSetPtr->mask),     cursor, sizeof(eventSetPtr->mask),      bytes_left);
	cursor += bsl_bufread(&(eventSetPtr->ruleCount),cursor, sizeof(eventSetPtr->ruleCount), bytes_left);

	/*
	 * Read the number of events in the serialized buffer for this eventset,
	 * create the list to hold them, and then deserialize the events and
	 * add them to the list.
	 */
	cursor += bsl_bufread(&(max_events), cursor, sizeof(uint8_t), bytes_left);

	eventSetPtr->events = sm_list_create(wm);

	for(i = 0; i < max_events; i++)
	{
		curEventAddr = psm_zalloc(wm, sizeof(BpSecEvent));
		curEventPtr = (BpSecEvent *) psp(wm, curEventAddr);
		if(curEventPtr)
		{
			memset(curEventPtr,0,sizeof(BpSecEvent));
			cursor += bslevt_sdr_restore(curEventPtr, cursor, bytes_left);
			sm_list_insert_last(wm, eventSetPtr->events, curEventAddr);
		}
		else
		{
			putErrmsg("Unable to allocate event object.", NULL);
			bsles_destroy(wm, *eventSetAddr, eventSetPtr);
			*eventSetAddr = 0;
			return -1;
		}
	}

	return (cursor-buffer);
}



/******************************************************************************
 * @brief Remove an eventset from the SDR.
 *
 * @param[in] wm   - The shared memory partition
 * @param[in] name - The name of the eventset to remove.
 *
 * @retval  1 - The eventset was found in the SDR and removed.
 * @retval  0 - The eventset was not found in the SDR.
 * @retval -1 - Error.
 *****************************************************************************/

int bsles_sdr_forget(PsmPartition wm, char *name)
{
	SecDB *secdb = getSecConstants();
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecEventSet curEventSet;
	Object sdrElt = 0;
	Object dataElt = 0;
	int success = 0;

	CHKERR(name);
	if (secdb == NULL) return -1;

	CHKERR(sdr_begin_xn(ionsdr));

	for(sdrElt = sdr_list_first(ionsdr, secdb->bpSecEventSets);
		sdrElt;
		sdrElt = sdr_list_next(ionsdr, sdrElt))
	{
		dataElt = sdr_list_data(ionsdr, sdrElt);
		sdr_read(ionsdr, (char *) &entry, dataElt, sizeof(BpSecPolicyDbEntry));
		sdr_read(ionsdr, (char *) &curEventSet, entry.entryObj, sizeof(curEventSet));

		if(strcmp(name, curEventSet.name) == 0)
		{
			sdr_free(ionsdr, entry.entryObj);
			sdr_free(ionsdr, dataElt);
			sdr_list_delete(ionsdr, sdrElt, NULL, NULL);
			success = 1;
			break;
		}
	}

	if (sdr_end_xn(ionsdr) < 0)
	{
		putErrmsg("Can't remove event set.", NULL);
		success = -1;
	}

	return success;
}



/******************************************************************************
 * @brief Writes a serialized version of the eventset into the SDR.
 *
 * @param[in] wm           - The shared memory partition
 * @param[in] eventSetAddr - The address of the eventset being persisted
 *
 * @retval  1 - The eventset was written to the SDR
 * @retval  0 - The eventset was not written to the SDR.
 * @retval -1 - Error.
 *****************************************************************************/

int bsles_sdr_persist(PsmPartition wm, PsmAddress eventSetAddr)
{
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecEventSet *eventSetPtr = NULL;
	char *buffer = NULL;
	int bytes_left = 0;
	SecDB *secdb = getSecConstants();

	CHKERR(wm);
	if (secdb == NULL) return -1;
	eventSetPtr = (BpSecEventSet *) psp(wm, eventSetAddr);
	CHKERR(eventSetPtr);

	entry.size = bsles_sdr_size(wm, eventSetAddr);
	if((buffer = MTAKE(entry.size)) == NULL)
	{
		putErrmsg("Cannot allocate workspace for eventset.", NULL);
		return -1;
	}

	bytes_left = entry.size;

	if(bsles_sdr_serialize_buffer(wm, eventSetPtr, buffer, &bytes_left) <= 0)
	{
		MRELEASE(buffer);
		putErrmsg("Cannot allocate workspace for eventset.", NULL);
		return -1;
	}

	int result = bsl_sdr_insert(ionsdr, buffer, entry, secdb->bpSecEventSets);
	MRELEASE(buffer);

	return result;
}



/******************************************************************************
 * @brief Reads a serialized version of an eventset from the SDR.
 *
 * @param[in] wm    - The shared memory partition
 * @param[in] entry - The entry pointing to the eventset to restore in the SDR.
 *
 * @retval  1 - The eventset was found in the SDR and restored.
 * @retval  0 - The eventset was not found in the SDR.
 * @retval -1 - Error.
 *****************************************************************************/

int bsles_sdr_restore(PsmPartition wm, BpSecPolicyDbEntry entry)
{
	PsmAddress eventSetAddr = 0;
	char *buffer = NULL;
	char *cursor = NULL;
	Sdr ionsdr = getIonsdr();
	int bytes_left = 0;
	BpSecEventSet *esPtr = NULL;
	SecVdb	*secvdb = getSecVdb();

	if (secvdb == NULL) return -1;
	bytes_left = entry.size;
	cursor = buffer = MTAKE(entry.size);
	CHKERR(buffer);

	if(!sdr_begin_xn(ionsdr))
	{
		MRELEASE(buffer);
		return -1;
	}

	sdr_read(ionsdr, buffer, entry.entryObj, entry.size);
	sdr_end_xn(ionsdr);

	cursor += bsles_sdr_deserialize(wm, &eventSetAddr, buffer, &bytes_left);

	if(cursor != (buffer + entry.size))
	{
		putErrmsg("Error restoring eventset.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	MRELEASE(buffer);
	esPtr = (BpSecEventSet*) psp(wm, eventSetAddr);
	CHKERR(esPtr);

	return sm_rbt_insert(wm, secvdb->bpsecEventSet, eventSetAddr, bsles_cb_rbt_key_comp, esPtr->name);
}



/******************************************************************************
 * @brief Serialize an eventset into a provided buffer
 *
 * @param[in]     wm          - PsmPartition ION working memory.
 * @param[in]     eventSetPtr - The eventset to serialize
 * @param[out]    buffer      - The buffer holding the serialized eventset object
 * @param[in,out] bytes_left  - Bytes remaining in the buffer.
 *
 * @retval -1  - Error.
 * @retval  0  - The evenset was not serialized.
 * @retval  >0 - The number of bytes written into the buffer.
 *****************************************************************************/

int bsles_sdr_serialize_buffer(PsmPartition wm, BpSecEventSet *eventSetPtr, char *buffer, int *bytes_left)
{
	char *cursor = buffer;
	uint8_t max_events = 0;
	PsmAddress elt = 0;
	PsmAddress eventAddr = 0;

	CHKZERO(wm);
	CHKZERO(eventSetPtr);
	CHKZERO(buffer);
	CHKZERO(bytes_left);

	cursor += bsl_bufwrite(cursor, &(eventSetPtr->name), sizeof(eventSetPtr->name), bytes_left);
	cursor += bsl_bufwrite(cursor, &(eventSetPtr->mask), sizeof(eventSetPtr->mask), bytes_left);
	cursor += bsl_bufwrite(cursor, &(eventSetPtr->ruleCount), sizeof(eventSetPtr->ruleCount), bytes_left);

	max_events = sm_list_length(wm, eventSetPtr->events);
	cursor += bsl_bufwrite(cursor, &(max_events), sizeof(max_events), bytes_left);

	for(elt = sm_list_first(wm, eventSetPtr->events); elt; elt = sm_list_next(wm, elt))
	{
		eventAddr = sm_list_data(wm, elt);
		cursor += bslevt_sdr_persist(cursor, (BpSecEvent *) psp(wm, eventAddr), bytes_left);
	}

	return (cursor - buffer);
}



/******************************************************************************
 * @brief Calculate the expected serialized size of an eventset object.
 *
 * @param[in] wm           - The shared memory partition
 * @param[in] eventSetAddr - The address of the eventset being sized.
 *
 * @retval  >0 - The expected size of the eventset
 * @retval   0 - The eventset could not be sized
 * @retval  -1 - Error.
 *****************************************************************************/

int bsles_sdr_size(PsmPartition wm, PsmAddress eventSetAddr)
{
	BpSecEventSet *eventSet = NULL;
	int size = 0;

	CHKZERO(wm);
	CHKZERO(eventSetAddr);

	eventSet = (BpSecEventSet *) psp(wm, eventSetAddr);
	CHKZERO(eventSet);

	size = sizeof(eventSet->name) + sizeof(eventSet->mask) + sizeof(eventSet->ruleCount);
	size += sizeof(uint8_t);// record max number of events in the set.
	size += sm_list_length(wm, eventSet->events) * sizeof(BpSecEvent); // Size of events in the event set.

	return size;
}



/******************************************************************************
 * @brief Determine if an eventset in a RBtree matches a given criteria
 *
 * This function implements key comparison for the bpsecEventSet red-black
 * tree, using string compare to find a match in the names of the event sets.
 *
 * @param[in] wm         - PsmPartition
 * @param[in] refData    - Data (eventset name) to compare to
 * @param[in] dataBuffer - Eventset pointer to extract name from for comparison
 *
 * @retval -1  - Error.
 * @retval  0  - Event set names do not match
 * @retval >0  - Event set names match
 *****************************************************************************/

int bsles_cb_rbt_key_comp(PsmPartition wm, PsmAddress refData, void *dataBuffer)
{
	BpSecEventSet *esPtr = NULL;
	char *name = (char *) dataBuffer;

	esPtr = (BpSecEventSet *) psp(wm, refData);

	return bsles_match(esPtr, name);
}



/******************************************************************************
 * @brief delete an eventset from its containing red-black tree.
 *
 * @param[in] wm      - PsmPartition
 * @param[in] refData - Data (eventset name) to delete
 * @param[in] arg     - Optional argument. Unused.
 *****************************************************************************/

void bsles_cb_rbt_key_del(PsmPartition wm, PsmAddress refData, void *arg)
{
	psm_free(wm, refData);
}



/******************************************************************************
 * @brief delete an event in the sm_list of events for an eventset.
 *
 * @param[in] wm  - PsmPartition
 * @param[in] elt - The event object in the eventset list.
 * @param[in] arg - Optional argument. Unused.
 *****************************************************************************/

void bsles_cb_smlist_del(PsmPartition wm, PsmAddress elt, void *arg)
{
	psm_free(wm, sm_list_data(wm, elt));
}

