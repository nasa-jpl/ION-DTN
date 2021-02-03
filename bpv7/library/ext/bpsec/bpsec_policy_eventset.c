/*****************************************************************************
 **
 ** File Name: bpsec_policy_eventset.c
 **
 ** Description:
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **                           Initial implementation
 **
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_policy_eventset.h"

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/



/******************************************************************************
 *
 * \par Function Name: bsles_add
 *
 * \par Purpose: Create an event set in working memory. This function
 * 				 first checks that the eventset name is unique (there is not
 * 				 a named eventset with the same name already defined). The
 * 				 eventset is then created and associated with an
 * 				 auto-generated ID.
 *
 * \retval int -1  - Error.
 *              0  - Failure to add event set
 *             >0  - Event set successfully added
 *
 * \param[in]   wm    PsmPartition
 * 				name  The name of the event set to be added.
 *
 * \par Notes:
 *	    1. The new Event Set is not associated with any security policy rules
 *	       when it is first created. An Event Set must be created before it
 *	       can be used by a security policy rule.
 *	    2. The new Event Set is not associated with any Events when it is
 *	       first created.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/07/21  Sarah Heiner   Initial Implementation
 *****************************************************************************/
int bsles_add(PsmPartition wm, char *name)
{
	CHKERR(name);

	BpSecEventSet *esPtr = bsles_get_ptr(wm, name);

	/* Verify that event set name is unique */

	if(esPtr != NULL)
	{
		writeMemoNote("This event set is already defined", name);
		return 0;
	}

	else
	{
		PsmAddress addr = 0;
		if(bsles_create(wm, name, 0, &addr))
		{
			sm_rbt_insert(wm, getSecVdb()->bpsecEventSet, addr, bsles_cb_rbt_key_comp, name);
			bsles_sdr_persist(wm, addr);
			return 1;
		}
	}

	return -1;
}

// TODO: comment. This function creates the event set but does not add it to the rb tree.
// used for anonymous event sets
BpSecEventSet *bsles_create(PsmPartition wm, char *name, uint8_t ruleCnt, PsmAddress *addr)
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
		esPtr->ruleCount = ruleCnt;
		esPtr->events = sm_list_create(wm);

		return esPtr;
	}
	else
	{
		writeMemoNote("[?] Could not allocate eventset", name);
		return NULL;
	}
}

int bsles_add_event(PsmPartition wm, BpSecEventSet *esPtr, PsmAddress eventAddr, BpSecEventId eventId)
{
	CHKERR(wm);
	CHKERR(esPtr);
	CHKERR(eventAddr);

	if(bsles_set_event(esPtr, eventId) <= 0)
	{
		writeMemo("[?] Could not configure event for eventset");
		return 0;
	}

	CHKERR(sm_list_insert_last(wm, esPtr->events, eventAddr));

	PsmAddress esAddr = bsles_get_addr(wm, esPtr->name);

	/* Re-persist the new event set.*/
	bsles_sdr_forget(wm, esPtr->name);
	bsles_sdr_persist(wm, esAddr);

	return 1;
}




/******************************************************************************
*
* \par Function Name: bsles_clear_event
*
* \par Purpose: Clear the bit in the eventset's mask corresponding to the
* 			    event identified.
*
* \retval int -1  - Error.
*              0  - Failure to clear bit.
*             >0  - Event bit cleared.
*
* \param[in|out] esPtr - Pointer to eventset for which the mask value should be
*                        modified
* \param[in]     event - Identifier of event for which mask bit should be cleared
*
* Modification History:
*  MM/DD/YY  AUTHOR         DESCRIPTION
*  --------  ------------   ---------------------------------------------
*  01/11/21  Sarah Heiner   Initial Implementation
*****************************************************************************/
int	bsles_clear_event(PsmPartition wm, BpSecEventSet *esPtr, BpSecEventId eventId)
{
	PsmAddress elt;
	PsmAddress curEventAddr = 0;
	BpSecEvent *curEventPtr = NULL;

	CHKERR(esPtr);
	CHKZERO(eventId > 0);

	esPtr->mask &= ~eventId;

	for(elt = sm_list_first(wm, esPtr->events); elt; elt = sm_list_next(wm, elt))
	{
		curEventAddr = sm_list_data(wm, elt);
		curEventPtr = (BpSecEvent *) psp(wm, curEventAddr);
		if(curEventPtr->id == eventId)
		{
			psm_free(wm, curEventAddr);
			memset(curEventPtr, 0, sizeof(BpSecEvent));
			sm_list_delete(wm, elt, NULL, NULL);
			break;
		}
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: bsles_delete
 *
 * \par Purpose: Delete an existing event set from the ION Security Database.
 * 				 This function checks to ensure the event set identified by
 * 				 name exists in the system and is not referenced by any
 * 				 security policy rules before deleting it from the database.
 *
 * \retval int -1  - Error.
 *              0  - Failure to delete event set
 *             >0  - Event set successfully deleted
 *
 * \param[in]  name  The name of the event set to be deleted.
 *
 * \todo This function must free all associated Events.
 *
 * \par Notes:
 *	    1. An Eventset cannot be deleted if it is currently referenced
 *	       by one or more security policy rules.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *                           Initial Implementation
 *****************************************************************************/
int bsles_delete(PsmPartition wm, char *name)
{
	BpSecEventSet *esPtr = NULL;
	PsmAddress esAddr = 0;

	CHKERR(name);

	/* Verify that eventset is currently defined */

	if ((esAddr = bsles_get_addr(wm, name)) == 0)
	{
		putErrmsg("Event set is not defined", name);
		return 0;
	}

	esPtr = (BpSecEventSet*) psp(wm, esAddr);
	CHKERR(esPtr);

	if (esPtr->ruleCount > 0)
	{
		putErrmsg("Can't delete event set - referenced by policy rule", esPtr->name);
		return 0;
	}

	/* Remove the event set from the RBT. */
	sm_rbt_delete(wm, getSecVdb()->bpsecEventSet, bsles_cb_rbt_key_comp, (void*) esPtr, NULL, NULL);

	/* Delete the event set from the SDR. */
	bsles_sdr_forget(wm, esPtr->name);


	/* Remove the event set from working memory. */
	return bsles_destroy(wm, esAddr, esPtr);
}


int bsles_destroy(PsmPartition wm, PsmAddress esAddr, BpSecEventSet *esPtr)
{
	/* Verify that the eventset is not referenced by an existing policy rule */
	sm_list_destroy(wm, esPtr->events, bsles_cb_smlist_del, NULL);
	memset(esPtr, 0, sizeof(BpSecEventSet));
	psm_free(wm, esAddr);

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: bsles_get_addr
 *
 * \par Purpose: Find and retrieve an existing eventset's PsmAddress from the
 *               global named eventset red-black tree.
 *
 * \retval int -1  - Error.
 *              0  - Eventset not found.
 *             >0  - Eventset found and address retrieved successfully.
 *
 * \param[in]  	   name  - The name of the event set to be located.
 * \param[in|out]  addr  - Pointer to the located eventset's address.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/07/21  S. Heiner      Initial Implementation
 *****************************************************************************/
PsmAddress bsles_get_addr(PsmPartition wm, char *name)
{
	PsmAddress nodeAddr;

	CHKZERO(name);

	nodeAddr = sm_rbt_search(wm, getSecVdb()->bpsecEventSet, bsles_cb_rbt_key_comp, name, NULL);

	return (nodeAddr) ? sm_rbt_data(wm, nodeAddr) : 0;
}



// TODO !!waiting until use of sdr or shared memory is decided
// Lyst is fine because this is only used in the context of
// the calling app and not shared.
Lyst bsles_get_all(PsmPartition wm)
{
	Lyst eventsets = lyst_create();
	PsmAddress elt = 0;
	PsmAddress esAddr = 0;
	BpSecEventSet *esPtr = NULL;

	CHKNULL(eventsets);

	for (elt = sm_rbt_first(wm, getSecVdb()->bpsecEventSet);
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
 *
 * \par Function Name: bsles_get_event
 *
 * \par Purpose: Retrieve an existing event set from the global named eventset
 * 				 red-black tree.
 *
 * \retval pointer to the event.
 *
 * \param[in]     name  - The name of the event set to be retrieved.
 * \param[in|out] esPtr - Pointer to the located event set.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/08/21  S. Heiner      Initial Implementation
 *****************************************************************************/
BpSecEvent *bsles_get_event(PsmPartition wm, BpSecEventSet *esPtr, BpSecEventId eventId)
{
	PsmAddress elt;
	BpSecEvent *event = NULL;

	CHKNULL(esPtr);
	CHKNULL(eventId);

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
 *
 * \par Function Name: bsl_eventset_get_ptr
 *
 * \par Purpose: Retrieve an existing event set from the global named eventset
 * 				 red-black tree.
 *
 * \retval void
 *
 * \param[in]     name  - The name of the event set to be retrieved.
 * \param[in|out] esPtr - Pointer to the located event set.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  12/21/20  S. Heiner      Initial Implementation
 *****************************************************************************/
BpSecEventSet* bsles_get_ptr(PsmPartition wm, char *name)
{
	return psp(wm, bsles_get_addr(wm, name));
}






int bsles_match(BpSecEventSet *es1, char *name)
{
	CHKZERO(es1);
	CHKZERO(name);

	return strcmp(es1->name, name);
}





/******************************************************************************
*
* \par Function Name: bsles_set_event
*
* \par Purpose: Set the bit in the eventset's mask corresponding to the
* 			    event identified.
*
* \retval int -1  - Error.
*              0  - Failure to set bit.
*             >0  - Event bit set.
*
* \param[in|out] esPtr - Pointer to eventset for which the mask value should be
*                        modified
* \param[in]     event - Identifier of event for which mask bit should be set
*
* Modification History:
*  MM/DD/YY  AUTHOR         DESCRIPTION
*  --------  ------------   ---------------------------------------------
*  01/11/21  Sarah Heiner   Initial Implementation
*****************************************************************************/
int	bsles_set_event(BpSecEventSet *esPtr, BpSecEventId eventId)
{
	CHKERR(esPtr);
	CHKZERO(eventId);

	esPtr->mask |= eventId;
	return 1;
}


// TODO: More error rollback.
int bsles_sdr_deserialize(PsmPartition wm, PsmAddress *eventSetAddr, char *buffer, int *bytes_left)
{
	BpSecEventSet *eventSetPtr = NULL;
	char *cursor = buffer;
	uint8_t max_events = 0;
	PsmAddress curEventAddr = 0;
	BpSecEvent *curEventPtr = NULL;
	int i = 0;


	if((*eventSetAddr = psm_zalloc(wm, sizeof(BpSecEventSet))) == 0)
	{
		return 0;
	}

	eventSetPtr = (BpSecEventSet*) psp(wm, *eventSetAddr);
	memset(eventSetPtr, 0, sizeof(BpSecEventSet));

	cursor += bsl_sdr_bufread(&(eventSetPtr->name),     cursor, sizeof(eventSetPtr->name),      bytes_left);
	cursor += bsl_sdr_bufread(&(eventSetPtr->mask),     cursor, sizeof(eventSetPtr->mask),      bytes_left);
	cursor += bsl_sdr_bufread(&(eventSetPtr->ruleCount),cursor, sizeof(eventSetPtr->ruleCount), bytes_left);

	cursor += bsl_sdr_bufread(&(max_events), cursor, sizeof(uint8_t), bytes_left);

	eventSetPtr->events = sm_list_create(wm);

	for(i = 0; i < max_events; i++)
	{
		curEventAddr = psm_zalloc(wm, sizeof(BpSecEvent));
		curEventPtr = (BpSecEvent *) psp(wm, curEventAddr);
		if(curEventPtr)
		{
			cursor += bslevt_sdr_restore(curEventPtr, cursor, bytes_left);
			sm_list_insert_last(wm, eventSetPtr->events, curEventAddr);
		}
	}

	return (cursor-buffer);
}


int bsles_sdr_forget(PsmPartition wm, char *name)
{
	SecDB *secdb = getSecConstants();
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecEventSet curEventSet;
	Object sdrElt = 0;
	Object dataElt = 0;

	CHKERR(name);

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
			break;
		}
	}

	if (sdr_end_xn(ionsdr) < 0)
	{
		putErrmsg("Can't remove event set.", NULL);
		return -1;
	}

	return 0;
}



int bsles_sdr_persist(PsmPartition wm, PsmAddress eventSetAddr)
{
	Sdr ionsdr = getIonsdr();
	BpSecPolicyDbEntry entry;
	BpSecEventSet *eventSetPtr = NULL;
	char *buffer = NULL;
	int bytes_left = 0;

	CHKERR(wm);
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

	int result = bsl_sdr_insert(ionsdr, buffer, entry, getSecConstants()->bpSecEventSets);
	return result;
}



int bsles_sdr_restore(PsmPartition wm, BpSecPolicyDbEntry entry)
{
	PsmAddress eventSetAddr = 0;

	char *buffer = NULL;
	char *cursor = NULL;
	Sdr ionsdr = getIonsdr();
	int bytes_left = 0;
	BpSecEventSet *esPtr = NULL;

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

	return sm_rbt_insert(wm, getSecVdb()->bpsecEventSet, eventSetAddr, bsles_cb_rbt_key_comp, esPtr->name);
}


int bsles_sdr_serialize_buffer(PsmPartition wm, BpSecEventSet *eventSetPtr, char *buffer, int *bytes_left)
{
	char *cursor = buffer;
	uint8_t max_events = 0;
	PsmAddress elt;
	PsmAddress eventAddr = 0;

	CHKZERO(wm);
	CHKZERO(eventSetPtr);
	CHKZERO(buffer);
	CHKZERO(bytes_left);

	cursor += bsl_sdr_bufwrite(cursor, &(eventSetPtr->name), sizeof(eventSetPtr->name), bytes_left);
	cursor += bsl_sdr_bufwrite(cursor, &(eventSetPtr->mask), sizeof(eventSetPtr->mask), bytes_left);
	cursor += bsl_sdr_bufwrite(cursor, &(eventSetPtr->ruleCount), sizeof(eventSetPtr->ruleCount), bytes_left);

	max_events = sm_list_length(wm, eventSetPtr->events);
	cursor += bsl_sdr_bufwrite(cursor, &(max_events), sizeof(max_events), bytes_left);

	for(elt = sm_list_first(wm, eventSetPtr->events); elt; elt = sm_list_next(wm, elt))
	{
		eventAddr = sm_list_data(wm, elt);
		cursor += bslevt_sdr_persist(cursor, (BpSecEvent *) psp(wm, eventAddr), bytes_left);
	}

	return (cursor - buffer);

}

int bsles_sdr_size(PsmPartition wm, PsmAddress eventSetAddr)
{
	BpSecEventSet *eventSet = NULL;
	int size = 0;

	CHKZERO(wm);
	CHKZERO(eventSetAddr);

	eventSet = (BpSecEventSet *) psp(wm, eventSetAddr);
	CHKZERO(eventSet);

	//size = sizeof(eventSet->id) +
	size = sizeof(eventSet->name) + sizeof(eventSet->mask) + sizeof(eventSet->ruleCount);
	size += sizeof(uint8_t);// record max number of events in the set.
	size += sm_list_length(wm, eventSet->events) * sizeof(BpSecEvent); // Size of events in the event set.

	return size;
}



/******************************************************************************
 *
 * \par Function Name: bslpol_eventset_rbt_key_comp
 *
 * \par Purpose: This function implements key comparison for the bpsecEventSet
 * 	             red-black tree, using string compare to find a match in the
 * 	             names of the event sets.
 *
 * \retval int -1  - Error.
 *              0  - Event set names do not match
 *             >0  - Event set names match
 *
 * \param[in]   wm          PsmPartition
 * 				refData     Data (eventset name) to compare to
 * 				dataBuffer  Eventset pointer to extract name from for comparison
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/07/21  Sarah Heiner   Initial Implementation
 *****************************************************************************/
int bsles_cb_rbt_key_comp(PsmPartition wm, PsmAddress refData, void *dataBuffer)
{
	BpSecEventSet *esPtr = NULL;
	char *name = (char *) dataBuffer;

	esPtr = (BpSecEventSet *) psp(wm, refData);

	return bsles_match(esPtr, name);
}


void bsles_cb_rbt_key_del(PsmPartition wm, PsmAddress refData, void *arg)
{
	psm_free(wm, refData);
}



void bsles_cb_smlist_del(PsmPartition wm, PsmAddress elt, void *arg)
{
	psm_free(wm, sm_list_data(wm, elt));
}

