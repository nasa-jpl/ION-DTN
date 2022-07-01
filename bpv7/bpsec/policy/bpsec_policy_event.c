/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bpsec_policy_event.c
 **
 ** Description: BPSec policy events associate events of a security block
 **              lifecycle with predefined actions to be taken by a
 **              node in response to those events.
 **
 **              Events exist in the context of an event set.
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YYYY  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  01/08/2021  E. Birrane &   Initial implementation
 **              S. Heiner
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/
#include "bpsec_asb.h"
#include "bpsec_util.h"
#include "bpsec_policy_event.h"
#include "bpsec_policy_eventset.h"


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/

static struct {char *key; int value;} gEventNameMap[] = {
	{"source_for_sop",                src_for_sop},
	{"src_for_sop",                   src_for_sop},
	{"sop_added_at_source",           sop_added_at_src},
	{"sop_added_at_src",              sop_added_at_src},
	{"sop_misconfigured_at_source",   sop_misconf_at_src},
	{"sop_misconf_at_src",            sop_misconf_at_src},
	{"verifier_for_sop",              verifier_for_sop},
	{"sop_misconfigured_at_verifier", sop_misconf_at_verifier},
	{"sop_misconf_at_verifier",       sop_misconf_at_verifier},
	{"sop_missing_at_verifier",       sop_missing_at_verifier},
	{"sop_corrupted_at_verifier",     sop_corrupt_at_verifier},
	{"sop_corrupt_at_verifier",       sop_corrupt_at_verifier},
	{"sop_verified",                  sop_verified},
	{"acceptor_for_sop",              acceptor_for_sop},
	{"sop_misconfigured_at_acceptor", sop_misconf_at_acceptor},
	{"sop_misconf_at_acceptor",       sop_misconf_at_acceptor},
	{"sop_missing_at_acceptor",       sop_missing_at_acceptor},
	{"sop_corrupted_at_acceptor",     sop_corrupt_at_acceptor},
	{"sop_corrupt_at_acceptor",       sop_corrupt_at_acceptor},
	{"sop_processed",                 sop_processed},
	{NULL,0}
};



/******************************************************************************
 * @brief Add an event to an event set.
 *
 * @param[in]  wm			 PsmPartition ION working memory.
 * @param[in]  esName  		 Name of the event set to associate event with.
 * @param[in]  evenId    	 Security operation event ID.
 * @param[in]  actionMask 	 Processing action(s) to enable for the event.
 * @param[in]  actionParms   Optional parameters associated with actions
 *
 * @note
 * Validation of the event set name parameter performed by the
 * findEventSet function.
 *
 * @retval -1  - Error.
 * @retval  0  - Event not added.
 * @retval >0  - Event successfully added.
 *****************************************************************************/

int bslevt_add(PsmPartition wm, char *esName, BpSecEventId eventId, uint8_t actionMask, BpSecEvtActionParms *actionParms)
{
	BpSecEventSet *esPtr = NULL;

	/* Sanity checks */
	CHKERR(wm);
	CHKERR(esName);

	if((eventId == unsupported) || (actionMask == 0))
	{
		return 0;
	}

	/* Step 1: Find the eventset to associate new event with */
	if ((esPtr = bsles_get_ptr(wm, esName)) == NULL)
	{
		writeMemoNote("[?] Eventset not found", esName);
		return 0;
	}
	/* Step 2: Determine if the event has been configured for the eventset in the past */
	else if(BSLEVT_IS_SET(esPtr->mask, eventId))
	{
		writeMemo("[?] Event is already set");
		return 0;
	}
	/* Step 3: Add event to the event set. */
	else
	{
		/* Step 3.1: Allocate the new event */
		PsmAddress eventAddr = 0;

		if(bslevt_create(wm, eventId, actionMask, actionParms, &eventAddr))
		{
			if(bsles_add_event(wm, esPtr, eventAddr, eventId) <= 0)
			{
				writeMemo("[?] Could not configure event for eventset");
				psm_free(wm, eventAddr);
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}
	return -1;
}



/******************************************************************************
 * @brief Create an event object that can be added to an event set
 *
 * @param[in]  wm            PsmPartition ION working memory.
 * @param[in]  eventId       Security operation event ID.
 * @param[in]  actions       Processing action(s) to enable for the event.
 * @param[in]  actionParms   Optional parameters associated with actions
 * @param[out] addr          Address of the created event object
 *
 * @note
 * An event can be invalid if it is configured for actions that are disallowed
 * for that event.
 * \par
 * There are only 3 actions defined with parameters. It is less space and less
 * processing to have an array of 3 parameters. This function assumes that the
 * passed-in parms is a static-sized array of the maximum number of parameters
 *
 *
 * @retval -1  - Error.
 * @retval  0  - Event not created.
 * @retval >0  - Event successfully created.
 *****************************************************************************/

int bslevt_create(PsmPartition wm, BpSecEventId eventId, uint8_t actions,
	              BpSecEvtActionParms *actionParms, PsmAddress *addr)
{
	uint8_t pre_actions = actions;

	/*
	 * Step 1: Validate the actions for the event. This will remove from the
	 *         action mask any actions that are not appropriate for this
	 *         event. If that occurs, then the event should be discarded as
	 *         it will not have the desired behavior expected by the caller.
	 */
	bslevt_validate_actions(eventId, &actions);
	if((actions == 0) || (pre_actions != actions))
	{
		writeMemoNote("[x] At least one action not allowed for eventId", bslevt_get_name(eventId));
		return 0;
	}


	/* Step 2: Allocate and populate the new event */
	*addr = psm_zalloc(wm, sizeof(BpSecEvent));

	if(*addr)
	{
		BpSecEvent *eventPtr = (BpSecEvent *) psp(wm, *addr);
		memset(eventPtr, 0, sizeof(BpSecEvent));

		eventPtr->id = eventId;
		eventPtr->action_mask = actions;
		if(actionParms)
		{
			memcpy(&(eventPtr->action_parms), actionParms, BSLACT_MAX_PARM * sizeof(BpSecEvtActionParms));
		}

		return 1;
	}

	return 0;
}



/******************************************************************************
 * @brief Remove an event from an event set
 *
 * @param[in]  wm        PsmPartition ION working memory.
 * @param[in]  esName    Name of the event set losing the event.
 * @param[in]  eventId   Event to remove from the event set.
 *
 * @note
 *
 *
 * @retval -1  - Error.
 * @retval  0  - Event not deleted.
 * @retval >0  - Event successfully deleted.
 *****************************************************************************/

int bslevt_delete(PsmPartition wm, char *esName, BpSecEventId eventId)
{
	BpSecEventSet *esPtr = bsles_get_ptr(wm, esName);

	if (esPtr == NULL)
	{
		writeMemoNote("[?] Eventset not found", esName);
		return 0;
	}
	else
	{
		/* Step 1.1: Validate the security operation event name */
		if (eventId == unsupported)
		{
			writeMemo("[?] Security operation event is not supported");
			return 0;
		}

		/* Step 2: Determine if the event is configured for the eventset */
		if (!BSLEVT_IS_SET(esPtr->mask, eventId))
		{
			writeMemo("[?] No event to delete");
			return 0;
		}
		else
		{
			return bsles_clear_event(wm, esPtr, eventId);
		}
	}

	return -1;
}



/******************************************************************************
 * @brief Convert an event set string name into its enumerated identifier
 *
 * @param[in]  name - The name of the event
 *
 * @retval The enumerated event ID, or the special value for unsupported.
 *****************************************************************************/

BpSecEventId bslevt_get_id(char *name)
{
	int i = 0;

	if(name)
	{
		/* Walk the string->id mapping of event names to event IDs. */
		while(gEventNameMap[i].key != NULL)
		{
			if(strcmp(gEventNameMap[i].key, name) == 0)
			{
				return gEventNameMap[i].value;
			}
			i++;
		}
	}

	return unsupported;
}




/******************************************************************************
 * @brief Convert an enumerated event identifier to an event set string name
 *
 * @param[in] eventId - The enumerated event ID
 *
 * @notes
 * This function returns a constant char*.  This MUST NOT be freed by the
 * calling function.
 *
 * @retval !NULL the name of the event
 *****************************************************************************/

char *bslevt_get_name(BpSecEventId eventId)
{
	int i = 0;

	while(gEventNameMap[i].key != NULL)
	{
		if(gEventNameMap[i].value == eventId)
		{
			return gEventNameMap[i].key;
		}
		i++;
	}

	return "unsupported";

}



/******************************************************************************
 * @brief Serialize an event into a provided memory buffer
 *
 * @param[in,out] cursor     - The memory buffer holding serialized components.
 * @param[in]     event      - The event object being serialized.
 * @param[out]    bytes_left - The number of bytes left in the buffer.
 *
 * @notes
 * This is currently a simple copy of the structure.
 *
 * @retval The number of bytes written into the serialized buffer.
 *****************************************************************************/

Address bslevt_sdr_persist(char *cursor, BpSecEvent *event, int *bytes_left)
{
	return bsl_bufwrite(cursor, event, sizeof(BpSecEvent), bytes_left);
}



/******************************************************************************
 * @brief Deserialize an event from a provided memory buffer
 *
 * @param[out]    event      - The event object.
 * @param[in,out] cursor     - The memory buffer holding serialized components.
 * @param[out]    bytes_left - The number of bytes left in the buffer.
 *
 * @notes
 * This is currently a simple copy of the structure.
 *
 * @retval The number of bytes read from the serialized buffer.
 *****************************************************************************/

Address bslevt_sdr_restore(BpSecEvent *event, char *cursor, int *bytes_left)
{
	return bsl_bufread(event, cursor, sizeof(BpSecEvent), bytes_left);
}



/******************************************************************************
 *
 * @brief Scrub action mask of actions not valid for a given event.
 *
 * When provided with a security operation event and configured actions for
 * that event, this function ensures that only approved actions are
 * configured for the event. This function modifies the provided action mask
 * by disabling unapproved actions.
 *
 * @param[in]      eventId - Event for which actions are configured
 * @param[in|out]  actions - Configured action mask for the event
 *
 * @retval  -1  - Error.
 * @retval   0  - Action mask could not be processed.
 * @retval  >0  - Action mask successfully processed/validated.
 *****************************************************************************/

int bslevt_validate_actions(BpSecEventId eventId, uint8_t *actions)
{
	CHKZERO(actions);

	switch(eventId)
	{
		case src_for_sop:             *actions &= BSLEVT_SRC_FOR_SOP_MASK;             break;
		case sop_added_at_src:        *actions &= BSLEVT_SOP_ADDED_AT_SRC_MASK;        break;
		case sop_misconf_at_src:      *actions &= BSLEVT_SOP_MISCONF_AT_SRC_MASK;      break;
		case verifier_for_sop:        *actions &= BSLEVT_VERIFIER_FOR_SOP_MASK;        break;
		case sop_misconf_at_verifier: *actions &= BSLEVT_SOP_MISCONF_AT_VERIFIER_MASK; break;
		case sop_missing_at_verifier: *actions &= BSLEVT_SOP_MISSING_AT_VERIFIER_MASK; break;
		case sop_corrupt_at_verifier: *actions &= BSLEVT_SOP_CORRUPT_AT_VERIFIER_MASK; break;
		case sop_verified:            *actions &= BSLEVT_SOP_VERIFIED_MASK;            break;
		case acceptor_for_sop:        *actions &= BSLEVT_ACCEPTOR_FOR_SOP_MASK;        break;
		case sop_misconf_at_acceptor: *actions &= BSLEVT_SOP_MISCONF_AT_ACCEPTOR_MASK; break;
		case sop_missing_at_acceptor: *actions &= BSLEVT_SOP_MISSING_AT_ACCEPTOR_MASK; break;
		case sop_corrupt_at_acceptor: *actions &= BSLEVT_SOP_CORRUPT_AT_ACCEPTOR_MASK; break;
		case sop_processed:           *actions &= BSLEVT_SOP_PROCESSED_MASK;           break;
		default: return -1;
	}

	return 1;
}
