/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ingest.h
 **
 ** Description: This implements the data ingest thread to receive DTNMP msgs.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR          DESCRIPTION
 **  --------  ------------    ---------------------------------------------
 **  08/31/11  V. Ramachandran Initial Implementation (JHU/APL)
 **  01/10/13  E. Birrane      Updates to lastest version of DTNMP spec. (JHU/APL)
 **  06/27/13  E. Birrane      Support persisted rules. (JHU/APL)
 *****************************************************************************/

#include "pthread.h"

#include "platform.h"

#include "nmagent.h"

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/utils/ion_if.h"
#include "../shared/msg/pdu.h"

#include "../shared/msg/msg_admin.h"
#include "../shared/msg/msg_ctrl.h"
#include "../shared/primitives/rules.h"
#include "instr.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/def.h"

#include "../shared/utils/utils.h"

#include "ingest.h"
#include <pthread.h>


extern eid_t manager_eid;

/******************************************************************************
 *
 * \par Function Name: rx_validate_mid_mc
 *
 * \par Determine whether a lyst contains valid, recognized MIDs.
 *
 * \param[in]  mids       The list of mids to validate.
 * \param[in]  passEmpty  Whether an empty list is OK (1) or not (0)
 *
 * \par Notes:
 *   - A NULL list is always bad.
 *
 * \return 0 - Lyst failed to validate.
 *         !0 - Valid lyst.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int rx_validate_mid_mc(Lyst mids, int passEmpty)
{
    LystElt elt;
    mid_t *cur_mid = NULL;
    int i = 0;

    AMP_DEBUG_ENTRY("rx_validate_mid_mc","(0x%#llx, %d)",
    		         (unsigned long) mids, passEmpty);

    /* Step 0 : Sanity Check. */
    if (mids == NULL)
    {
        AMP_DEBUG_ERR("rx_validate_mid_mc", "Bad Args.", NULL);
        AMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
        return 0;
    }

    /* Step 1: Walk the list of MIDs. */
    for (elt = lyst_first(mids); elt; elt = lyst_next(elt))
    {
    	i++;

        /* Grab the next mid...*/
        if((cur_mid = (mid_t*) lyst_data(elt)) == NULL)
        {
            AMP_DEBUG_ERR("rx_validate_mid_mc","Found unexpected NULL mid.", NULL);
            AMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
            return 0;
        }

        char *mid_str = mid_to_string(cur_mid);

        /* Is this a valid MID? */
        if(mid_sanity_check(cur_mid) == 0)
        {
            AMP_DEBUG_ERR("rx_validate_mid_mc","Malformed MID: %s.", mid_str);
            SRELEASE(mid_str);
            AMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
            return 0;
        }

        /* Do we know this MID? */
        if((adm_find_datadef(cur_mid) == NULL) &&
           (def_find_by_id(gAgentVDB.reports, &(gAgentVDB.reports_mutex), cur_mid) == NULL))
        {
            AMP_DEBUG_ERR("rx_validate_mid_mc","Unknown MID %s.", mid_str);
            SRELEASE(mid_str);
            AMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
            return 0;
        }

        AMP_DEBUG_INFO("rx_validate_mid_mc","MID %s is recognized.", mid_str);

        SRELEASE(mid_str);
    }


    if((i == 0) && (passEmpty == 0))
    {
        AMP_DEBUG_ERR("rx_validate_mid_mc","Empty MID list not allowed.", NULL);
        AMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
        return 0;
    }

    AMP_DEBUG_EXIT("rx_validate_mid_mc","-> 1", NULL);
    return 1;
}



/******************************************************************************
 *
 * \par Function Name: rx_validate_rule
 *
 * \par Determines whether a production rule is correct.
 *
 * \param[in] rule  The rule being evaluated.
 *
 * \return 0 - Bad rule.
 * 		   1 - Good rule.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int rx_validate_rule(trl_t *rule)
{
    int result = 1;
    
    AMP_DEBUG_ENTRY("rx_validate_rule","(0x%x)", (unsigned long) rule);

    /* Step 0: Sanity Check. */
    if(rule == NULL)
    {
    	AMP_DEBUG_ERR("rx_validate_rule","NULL rule.", NULL);
    	AMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

    /* Is the interval correct? */
    if(rule->desc.interval_ticks == 0)
    {
    	AMP_DEBUG_ERR("rx_validate_rule","Bad interval ticks: 0.", NULL);
    	AMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

    /* Do we understand the sender EID? */
    if(memcmp(&(rule->desc.sender), &(manager_eid), AMP_MAX_EID_LEN) != 0)
    {
    	AMP_DEBUG_ERR("rx_validate_rule","Unknown EID: %s.", rule->desc.sender.name);
    	AMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

    /* Is each MID valid and recognized? */
    if(rx_validate_mid_mc(rule->action, 0) == 0)
    {
    	AMP_DEBUG_ERR("rx_validate_rule","Unknown MIDs",NULL);
    	AMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

	AMP_DEBUG_EXIT("rx_validate_rule","-> 1", NULL);

    return result;
}



/******************************************************************************
 *
 * \par Function Name: rx_thread
 *
 * \par Receives and processes a DTNMP message.
 *
 * \param[in] running	Operational loop state.
 *
 * \return NULL - Error
 *         !NULL - Some thread thing.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void *rx_thread(int *running) {
#ifndef mingw
    AMP_DEBUG_ENTRY("rx_thread","(0x%X)",(unsigned long) pthread_self());
#endif
    AMP_DEBUG_INFO("rx_thread","Receiver thread running...", NULL);
    
    uint32_t num_msgs = 0;
    uint8_t *buf = NULL;
    uint8_t *cursor = NULL;
    uint32_t bytes = 0;
    uint32_t i = 0;
    pdu_header_t *hdr = NULL;
    uint32_t size = 0;
    pdu_metadata_t meta;
    uvast val = 0;
    time_t group_timestamp = 0;

    /* 
     * g_running controls the overall execution of threads in the
     * NM Agent.
     */
    while(*running) {
        
        /* Step 1: Receive a message from the Bundle Protocol Agent. */
        buf = iif_receive(&ion_ptr, &size, &meta, NM_RECEIVE_TIMEOUT_SEC);

        if(buf != NULL)
        {
            AMP_DEBUG_INFO("rx_thread","Received buf (%x) of size %d",
            		         (unsigned long) buf, size);

            /* Grab # messages and timestamp for this group. */
            cursor = buf;

            bytes = utils_grab_sdnv(cursor, size, &val);
            num_msgs = val;
            cursor += bytes;
            size -= bytes;

            bytes = utils_grab_sdnv(cursor, size, &val);
            group_timestamp = val;
            cursor += bytes;
            size -= bytes;

            AMP_DEBUG_INFO("rx_thread","Group had %d msgs", num_msgs);
            AMP_DEBUG_INFO("rx_thread","Group time stamp %lu", (unsigned long) group_timestamp);

            /* For each message in the bundle. */
            for(i = 0; i < num_msgs; i++)
            {
            	hdr = pdu_deserialize_hdr(cursor, size, &bytes);
            	cursor += bytes;
            	size -= bytes;

            	switch (hdr->id)
            	{
                	case MSG_TYPE_CTRL_EXEC:
                	{
                		AMP_DEBUG_ALWAYS("NM Agent :","Received Perform Control Message.\n", NULL);
                		rx_handle_exec(&meta, cursor,size,&bytes);
                	}
                	break;

                	default:
                	{
                		AMP_DEBUG_WARN("rx_thread","Received unknown type: %d.\n", hdr->type);
                		AMP_DEBUG_ALWAYS("NM Agent :","Received Unsupported message of type 0x%x.\n", hdr->id);

                	}
                	break;
            	}

            	pdu_release_hdr(hdr);
            	hdr = NULL;

            	cursor += bytes;
            	size -= bytes;

            }

            SRELEASE(buf);
            buf = NULL;
        }
    }
   
    AMP_DEBUG_ALWAYS("rx_thread","Shutting Down Agent Receive Thread.",NULL);
    AMP_DEBUG_EXIT("rx_thread","->.", NULL);
    pthread_exit(NULL);
    return NULL; /* Defensive. */
}


/******************************************************************************
 *
 * \par Function Name: rx_handle_rpt_def
 *
 * \par Process a received custom report definition message. This function
 *      accepts a portion of a serialized message group, with the
 *      understanding that the custom report definition message is at the
 *      head of the serialized data stream.  This function extracts the
 *      current message, and returns the number of bytes consumed so that
 *      the called may then know where the next message in the serialized
 *      message group begins.
 *
 * \param[in] meta        The metadata associated with the message.
 * \param[in] cursor      Pointer to the start of the serialized message.
 * \param[in] size        The size of the remaining serialized message group
 * \param[out] bytes_used The number of bytes consumed in processing this msg.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void rx_handle_rpt_def(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	def_gen_t* rpt_def = NULL;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("rx_handle_rpt_def","(0x%#llx, %d, 0x%#llx)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);

	/* Step 0: Sanity checks. */
	if((meta == NULL) || (cursor == NULL) || (bytes_used == NULL))
	{
    	AMP_DEBUG_ERR("rx_handle_rpt_def","Bad args.",NULL);
    	AMP_DEBUG_EXIT("rx_handle_rpt_def","->.",NULL);
    	return;
	}

	/* Step 1: Attempt to deserialize the message. */
    rpt_def = def_deserialize_gen(cursor, size, &bytes);

    /* Step 2: If the deserialization failed, complain. */
    if((rpt_def == NULL) || (bytes == 0))
    {
    	AMP_DEBUG_ERR("rx_handle_rpt_def","Can't deserialize.",NULL);
    	def_release_gen(rpt_def);
    	*bytes_used = 0;
    	AMP_DEBUG_EXIT("rx_handle_rpt_def","->.",NULL);
    	return;
    }

    /* Step 3: Otherwise, note how many bytes were consumed. */
    *bytes_used = bytes;

	AMP_DEBUG_INFO("rx_handle_rpt_def","Adding new report definition.", NULL);


    /* Step 4: Persist this definition to our SDR. */
    agent_db_report_persist(rpt_def);

    /* Step 5: Persist this definition to our memory lists. */
//	agent_vdb_reports_init(getIonsdr());
	ADD_REPORT(rpt_def);

	/* Step 6: Update instrumentation counters. */
	gAgentInstr.num_rptt_defs++;
}



/******************************************************************************
 *
 * \par Function Name: rx_handle_exec
 *
 * \par Process a received control exec  message. This function
 *      accepts a portion of a serialized message group, with the
 *      understanding that the control exec message is at the
 *      head of the serialized data stream.  This function extracts the
 *      current message, and returns the number of bytes consumed so that
 *      the called may then know where the next message in the serialized
 *      message group begins.
 *
 * \param[in] meta        The metadata associated with the message.
 * \param[in] cursor      Pointer to the start of the serialized message.
 * \param[in] size        The size of the remaining serialized message group
 * \param[out] bytes_used The number of bytes consumed in processing this msg.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void rx_handle_exec(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	uint32_t bytes = 0;
	time_t time;
	Lyst ctrls;
	LystElt elt;

	AMP_DEBUG_ENTRY("rx_handle_exec","(0x%#llx, %d, 0x%#llx)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);


	/* Step 0: Sanity checks. */
	if((meta == NULL) || (cursor == NULL) || (bytes_used == NULL))
	{
    	AMP_DEBUG_ERR("rx_handle_exec","Bad args.",NULL);
    	AMP_DEBUG_EXIT("rx_handle_exec","->.",NULL);
    	return;
	}

	/* Step 1: Get the timestamp associated with these controls. */
	uvast val;
    bytes = decodeSdnv(&val, cursor);
    time = val;

    cursor += bytes;
    size -= bytes;
    *bytes_used += bytes;

	/* Step 2: Get the MID Collection for these controls. */
	/* Grab the list of contents. */
	if((ctrls = midcol_deserialize(cursor, size, &bytes)) == NULL)
	{
		AMP_DEBUG_ERR("rx_handle_exec","Can't grab Ctrl MC.",NULL);

		*bytes_used = 0;
		AMP_DEBUG_EXIT("rx_handle_exec","->.",NULL);
	}
	else
	{
		cursor += bytes;
		size -= bytes;
		*bytes_used += bytes;
	}

	/* Step 3: For each MID representing a control instance... */
	for(elt = lyst_first(ctrls); elt; elt = lyst_next(elt))
	{
		/* Step 3.1: Get the current MID. */
		mid_t *mid = lyst_data(elt);
		ctrl_exec_t *ctrl = NULL;

		char *str = mid_to_string(mid);
		AMP_DEBUG_ALWAYS("rx_handle_exec","Received control mid %.50s...\n", str);

		/* Step 3.2: Create a control instance from this MID. */
		if((ctrl = ctrl_create(time, mid, meta->senderEid)) != NULL)
		{

			/* Step 3.3: Persist this control to the SDR. */
			agent_db_ctrl_persist(ctrl);

			/* Step 3.4: Persist this control to memory. */
			AMP_DEBUG_INFO("rx_handle_exec","Performing control.", NULL);
			ADD_CTRL(ctrl);

			/* Step 3.5: Update instrumentation counters. */
			gAgentInstr.num_ctrls++;
		}
		else
		{
			AMP_DEBUG_ERR("rx_handle_exec","Cannot create control for %s", str);
		}

		SRELEASE(str);
	}

	/* Step 4: Release the MidCol. */
	midcol_destroy(&ctrls);
}


/******************************************************************************
 *
 * \par Function Name: rx_handle_time_prod
 *
 * \par Process a received time-based prod message. This function
 *      accepts a portion of a serialized message group, with the
 *      understanding that the time-based prod message is at the
 *      head of the serialized data stream.  This function extracts the
 *      current message, and returns the number of bytes consumed so that
 *      the called may then know where the next message in the serialized
 *      message group begins.
 *
 * \param[in] meta        The metadata associated with the message.
 * \param[in] cursor      Pointer to the start of the serialized message.
 * \param[in] size        The size of the remaining serialized message group
 * \param[out] bytes_used The number of bytes consumed in processing this msg.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void rx_handle_time_prod(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	trl_t *new_rule = NULL;
    uint32_t bytes = 0;

    AMP_DEBUG_INFO("rx_handle_time_prod",
    		         "Processing a production rule.", NULL);

	/* Step 0: Sanity checks. */
	if((meta == NULL) || (cursor == NULL) || (bytes_used == NULL))
	{
    	AMP_DEBUG_ERR("rx_handle_time_prod","Bad args.",NULL);
    	AMP_DEBUG_EXIT("rx_handle_time_prod","->.",NULL);
    	return;
	}

	/* Step 1: Attempt to deserialize the message. */
	new_rule = trl_deserialize(cursor, size, &bytes);

	/* Step 2: If the deserialization failed, complain. */
	if((new_rule == NULL) || (bytes == 0))
	{
		AMP_DEBUG_ERR("rx_handle_time_prod","Can't deserialize.",NULL);
		trl_release(new_rule);
		*bytes_used = 0;
		AMP_DEBUG_EXIT("rx_handle_time_prod","->.",NULL);
		return;
	}

    /* Step 3: Otherwise, note how many bytes were consumed. */
    *bytes_used = bytes;

    /* Step 4: Populate dynamic parts of the control. */
    /* \todo: Consider single-fire absolute-time rules. */
    new_rule->desc.num_evals = new_rule->count;
    new_rule->desc.interval_ticks = new_rule->period;
    new_rule->countdown_ticks = new_rule->desc.interval_ticks;

    strcpy(new_rule->desc.sender.name, meta->senderEid.name);

    if(new_rule->desc.num_evals == 0)
    {
    	new_rule->desc.num_evals = DTNMP_RULE_EXEC_ALWAYS;
    }

    /* Step 5: Validate the new rule. */
    if(rx_validate_rule(new_rule) == 0)
    {
		AMP_DEBUG_ERR("rx_handle_time_prod","New rule failed validation.",NULL);
		trl_release(new_rule);
		*bytes_used = 0;
		AMP_DEBUG_EXIT("rx_handle_time_prod","->.",NULL);
		return;
    }

    AMP_DEBUG_INFO("rx_handle_time_prod",
    			         "Adding new production rule.", NULL);

    /* Step 6: Persist this definition to our SDR. */
    agent_db_trl_persist(new_rule);

    /* Step 7: Persist this definition to our memory lists. */
	ADD_TRL(new_rule);

	/* Step 8: Update instrumentation counters. */
	gAgentInstr.num_trls++;
}




/******************************************************************************
 *
 * \par Function Name: rx_handle_macro_def
 *
 * \par Process a received macro def message. This function
 *      accepts a portion of a serialized message group, with the
 *      understanding that the macro def message is at the
 *      head of the serialized data stream.  This function extracts the
 *      current message, and returns the number of bytes consumed so that
 *      the called may then know where the next message in the serialized
 *      message group begins.
 *
 * \param[in] meta        The metadata associated with the message.
 * \param[in] cursor      Pointer to the start of the serialized message.
 * \param[in] size        The size of the remaining serialized message group
 * \param[out] bytes_used The number of bytes consumed in processing this msg.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/10/13  E. Birrane     Initial implementation.
 *****************************************************************************/
void rx_handle_macro_def(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	def_gen_t* macro_def = NULL;
	uint32_t bytes = 0;

	AMP_DEBUG_ENTRY("rx_handle_macro_def","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);


	/* Step 0: Sanity checks. */
	if((meta == NULL) || (cursor == NULL) || (bytes_used == NULL))
	{
    	AMP_DEBUG_ERR("rx_handle_time_prod","Bad args.",NULL);
    	AMP_DEBUG_EXIT("rx_handle_time_prod","->.",NULL);
    	return;
	}

	/* Step 1: Attempt to deserialize the message. */
    macro_def = def_deserialize_gen(cursor, size, &bytes);

	/* Step 2: If the deserialization failed, complain. */
    if((macro_def == NULL) || (bytes == 0))
    {
    	AMP_DEBUG_ERR("rx_handle_macro_def","Can;t deserialize.",NULL);
    	def_release_gen(macro_def);
    	*bytes_used = 0;
    	AMP_DEBUG_EXIT("rx_handle_macro_def","->.",NULL);
    	return;
    }

    /* Step 3: Otherwise, note how many bytes were consumed. */
    *bytes_used = bytes;

	AMP_DEBUG_INFO("rx_handle_macro_def","Adding new report definition.", NULL);

    /* Step 4: Persist this definition to our SDR. */
	agent_db_macro_persist(macro_def);

    /* Step 5: Persist this definition to our memory lists. */
	ADD_MACRO(macro_def);

	/* Step 6: Update instrumentation counters. */
	gAgentInstr.num_macros++;
}

