/*****************************************************************************
 **
 ** File Name: bpsec_policy_event.c
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
#include "bpsec.h"
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



int bslevt_create(PsmPartition wm, BpSecEventId eventId, uint8_t actions,
	BpSecEvtActionParms *actionParms, PsmAddress *addr)
{
	/* Step 1: Allocate the new event */
	BpSecEvent *eventPtr = NULL;

	bslevt_validate_actions(eventId, &actions);
	if(actions == 0)
	{
		return 0;
	}

	*addr = psm_zalloc(wm, sizeof(BpSecEvent));

	if(*addr)
	{
		eventPtr = (BpSecEvent *) psp(wm, *addr);
		memset(eventPtr, 0, sizeof(BpSecEvent));

		/* Step 2: Populate the new event */

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
 *
 * \par Function Name: bslevt_add
 *
 * \par Purpose: Create an event and associate it with an existing event set.
 *
 * \retval int -1  - Error.
 *              0  - Event not created.
 *             >0  - Event successfully created.
 *
 * \param[in]  wm			 PsmPartition ION working memory.
 * \param[in]  esName  		 Name of the event set to associate event with.
 * \param[in]  evenId    	 Security operation event ID.
 * \param[in]  actionMask 	 Processing action(s) to enable for the event.
 * \param[in]  actionParms   Optional parameters associated witrh actions
 *
 * \par Notes:
 *	    1. Validation of the event set name parameter performed by the
 *	    findEventSet function.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/08/21  Sarah Heiner   Initial Implementation
 *  01/20/21  E. Birrane     Cleanup. Take in action parameters
 *****************************************************************************/
int bslevt_add(PsmPartition wm, char *esName, BpSecEventId eventId, uint8_t actionMask, BpSecEvtActionParms *actionParms)
{
	CHKERR(wm);
	CHKERR(esName);
	CHKERR(eventId);

	if(actionMask == 0)
	{
		return 0;
	}

	BpSecEventSet *esPtr = NULL;

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
 *
 * \par Function Name: bslevt_delete
 *
 * \par Purpose: Delete an existing event from the ION Security Database and
 * 				 remove it from its associated event set.
 *
 * \retval int -1  - Error.
 *              0  - Event not deleted.
 *             >0  - Event deleted successfully.
 *
  \TODO: revise comment
 *****************************************************************************/
int bslevt_delete(PsmPartition wm, char *esName, BpSecEventId eventId)
{
	/*
	 * check if event is set
	 * clear any associated parms
	 * free event
	 * set event to NULL
	 */

	/* Step 1: Find the eventset to remove the event from */
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

//TODO
int bslevt_change_actions(char *esName, BpSecEventId eventId, uint8_t actions)
{
	return -1;
}

/******************************************************************************
 *
 * \par Function Name: bsl_event_validate_actions
 *
 * \par Purpose: When provided with a security operation event and configured
 * 				 actions for that event, this function ensures that only
 * 				 approved actions are configured for the event. This function
 * 				 modifies the provided action mask by disabling unapproved
 * 				 actions.
 *
 * \retval int -1  - Error.
 *              0  - Action mask could not be processed.
 *             >0  - Action mask successfully processed/validated.
 *
 * \param[in]      event   - Security operation event for which actions are
 *                           configured
 * \param[in|out]  actions - Configured actions for the event
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 * 01/08/21   Sarah Heiner   Initial Implementation
 * 01/20/21   E. Birrane     Cleanup.
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





/******************************************************************************
 *
 * \par Function Name: bslevt_get_id
 *
 * \par Purpose: Given a string representing the name of a security operation
 * 				 event, this function returns the event for which actions
 * 				 can be configured. This function is used to validate user
 * 				 input to ensure that only valid security operation events are
 *
 * \retval BpSecEventId
 *
 * \param[in]   name - User-provided security operation event name
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/11/21   Sarah Heiner   Initial Implementation
 *  01/19/21   E. Birrane     Cleanup. Migrated to lookup map.
 *****************************************************************************/
BpSecEventId bslevt_get_id(char *name)
{
	int i = 0;

	CHKZERO(name);

	while(gEventNameMap[i].key != NULL)
	{
		if(strcmp(gEventNameMap[i].key, name) == 0)
		{
			return gEventNameMap[i].value;
		}
		i++;
	}

	return unsupported;
}

// this is a const char.
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

// Made a function in case striping a structure isn't portable...
Address bslevt_sdr_persist(char *cursor, BpSecEvent *event, int *bytes_left)
{
	return bsl_sdr_bufwrite(cursor, event, sizeof(BpSecEvent), bytes_left);
}

// Made a function in case striping a structure isn't portable...

Address bslevt_sdr_restore(BpSecEvent *event, char *cursor, int *bytes_left)
{
	return bsl_sdr_bufread(event, cursor, sizeof(BpSecEvent), bytes_left);
}




