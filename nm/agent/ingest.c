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
 **  08/31/11  V. Ramachandran Initial Implementation
 **  01/10/13  E. Birrane      Updates to lastest version of DTNMP spec.
 **  06/27/13  E. Birrane      Support persisted rules.
 *****************************************************************************/

#include "pthread.h"

#include "platform.h"

#include "nmagent.h"

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"
#include "shared/utils/ion_if.h"
#include "shared/msg/pdu.h"

#include "shared/msg/msg_admin.h"
#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_ctrl.h"
#include "shared/primitives/rules.h"
#include "shared/primitives/instr.h"

#include "shared/utils/utils.h"

#include "ingest.h"

extern eid_t manager_eid;

/*
 * \brief Determine whether a lyst contains valid, recognized MIDs.
 *
 * \author Ed Birrane
 *
 * \note
 *   - A NULL list is always bad.
 *
 * \return 0 - Lyst failed to validate.
 *         !0 - Valid lyst.
 *
 * \param[in]  mids       The list of mids to validate.
 * \param[in]  passEmpty  Whether an empty list of OK (1) or not (0)
 */

int rx_validate_mid_mc(Lyst mids, int passEmpty)
{
    LystElt elt;
    mid_t *cur_mid = NULL;
    int i = 0;

    DTNMP_DEBUG_ENTRY("rx_validate_mid_mc","(0x%x, %d",
    		         (unsigned long) mids, passEmpty);

    /* Step 0 : Sanity Check. */
    if (mids == NULL)
    {
        DTNMP_DEBUG_ERR("rx_validate_mid_mc", "Bad Args.", NULL);
        DTNMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
        return 0;
    }

    /* Step 1: Walk the list of MIDs. */
    for (elt = lyst_first(mids); elt; elt = lyst_next(elt))
    {
    	i++;

        /* Grab the next mid...*/
        if((cur_mid = (mid_t*) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("rx_validate_mid_mc","Found unexpected NULL mid.", NULL);
            DTNMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
            return 0;
        }

        char *mid_str = mid_to_string(cur_mid);

        /**EJBDEBUG
        char *temp = mid_pretty_print(cur_mid);
        fprintf(stderr,"EJB %s", temp);
        MRELEASE(temp);
         **/

        /* Is this a valid MID? */
        if(mid_sanity_check(cur_mid) == 0)
        {
            DTNMP_DEBUG_ERR("rx_validate_mid_mc","Malformed MID: %s.", mid_str);
            MRELEASE(mid_str);
            DTNMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
            return 0;
        }

        /* Do we know this MID? */
        if((adm_find_datadef(cur_mid) == NULL) &&
           (def_find_by_id(gAgentVDB.reports, &(gAgentVDB.reports_mutex), cur_mid) == NULL))
        {
            DTNMP_DEBUG_ERR("rx_validate_mid_mc","Unknown MID %s.", mid_str);
            DTNMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
            return 0;
        }

        DTNMP_DEBUG_INFO("rx_validate_mid_mc","MID %s is recognized.", mid_str);

        MRELEASE(mid_str);
    }


    if((i == 0) && (passEmpty == 0))
    {
        DTNMP_DEBUG_ERR("rx_validate_mid_mc","Empty MID list not allowed.", NULL);
        DTNMP_DEBUG_EXIT("rx_validate_mid_mc","-> 0", NULL);
        return 0;
    }

    DTNMP_DEBUG_EXIT("rx_validate_mid_mc","-> 1", NULL);
    return 1;
}


/**
 * \brief Determines whether a production rule is correct.
 *
 * \author Ed Birrane
 *
 * \return 0 - Bad rule.
 * 		   1 - Good rule.
 *
 * \param[in] rule  The rule being evaluated.
 */

int rx_validate_rule(rule_time_prod_t *rule)
{
    int result = 1; /* Optimisim...*/
    
    DTNMP_DEBUG_ENTRY("rx_validate_rule","(0x%x)", (unsigned long) rule);

    /* Step 0: Sanity Check. */
    if(rule == NULL)
    {
    	DTNMP_DEBUG_ERR("rx_validate_rule","NULL rule.", NULL);
    	DTNMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

    /* Is the interval correct? */
    if(rule->desc.interval_ticks == 0)
    {
    	DTNMP_DEBUG_ERR("rx_validate_rule","Bad interval ticks: 0.", NULL);
    	DTNMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

    /* Do we understand the sender EID? */
    if(memcmp(&(rule->desc.sender), &(manager_eid), MAX_EID_LEN) != 0)
    {
    	DTNMP_DEBUG_ERR("rx_validate_rule","Unknown EID: %s.", rule->desc.sender.name);
    	DTNMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

    /* Is each MID valid and recognized? */
    if(rx_validate_mid_mc(rule->mids, 0) == 0)
    {
    	DTNMP_DEBUG_ERR("rx_validate_rule","Unknown MIDs",NULL);
    	DTNMP_DEBUG_EXIT("rx_validate_rule","-> 0", NULL);
    	return 0;
    }

	DTNMP_DEBUG_EXIT("rx_validate_rule","-> 1", NULL);

    return result;
}



/**
 * \brief Receives and processes a DTNMP message.
 *
 * \author Ed Birrane
 *
 * \return NULL - Error
 *         !NULL - Some thread thing.
 *
 * \param[in] threadId The thread identifier.
 */

void *rx_thread(void *threadId) {
   
    DTNMP_DEBUG_ENTRY("rx_thread","(0x%x)",(unsigned long) threadId);
    
    DTNMP_DEBUG_INFO("rx_thread","Receiver thread running...", NULL);
    
    /* \todo: Grab initial # msgs header first. */
    uint32_t num_msgs = 0;
    uint8_t *buf = NULL;
    uint8_t *cursor = NULL;
    uint32_t bytes = 0;
    uint32_t i = 0;
    pdu_header_t *hdr = NULL;
    pdu_acl_t *acl = NULL;
    uint32_t size = 0;
    pdu_metadata_t meta;
    uvast val;
    time_t group_timestamp = 0;

    /* 
     * g_running controls the overall execution of threads in the
     * NM Agent.
     */
    while(g_running) {
        
        /* Step 1: Receive a message from the Bundle Protocol Agent. */
        buf = iif_receive(&ion_ptr, &size, &meta, NM_RECEIVE_TIMEOUT_MILLIS);

        if(buf != NULL)
        {
            DTNMP_DEBUG_INFO("rx_thread","Received buf (%x) of size %d",
            		         (unsigned long) buf, size);

            // EJB
            utils_print_hex(buf, size);


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

            DTNMP_DEBUG_INFO("rx_thread","Group had %d msgs", num_msgs);
            DTNMP_DEBUG_INFO("rx_thread","Group time stamp %lu", (unsigned long) group_timestamp);

            /* For each message in the bundle. */
            for(i = 0; i < num_msgs; i++)
            {
            	hdr = pdu_deserialize_hdr(cursor, size, &bytes);
            	cursor += bytes;
            	size -= bytes;

                DTNMP_DEBUG_INFO("rx_thread","Hdr took up %d", bytes);

            	switch (hdr->id)
            	{
                	case MSG_TYPE_CTRL_PERIOD_PROD:
                	{
                		DTNMP_DEBUG_ALWAYS("NM Agent :","Received Periodic Production Message.", NULL);
                		rx_handle_time_prod(&meta, cursor,size,&bytes);
                	}
                	break;
                
                	case MSG_TYPE_DEF_CUST_RPT:
                	{
                		DTNMP_DEBUG_ALWAYS("NM Agent :","Received Custom Report Definition.", NULL);
                		rx_handle_rpt_def(&meta, cursor,size,&bytes);
                	}
                	break;

                	case MSG_TYPE_CTRL_EXEC:
                	{
                		DTNMP_DEBUG_ALWAYS("NM Agent :","Received Perform Control Message.", NULL);
                		rx_handle_exec(&meta, cursor,size,&bytes);
                	}
                	break;

                	case MSG_TYPE_DEF_MACRO:
                	{
                		DTNMP_DEBUG_ALWAYS("NM Agent :","Received Macro Definition.", NULL);
                		rx_handle_macro_def(&meta, cursor,size,&bytes);
                	}
                	break;

                	default:
                	{
                		DTNMP_DEBUG_INFO("rx_thread","Unknown type: %d",
                				         hdr->type);
                		DTNMP_DEBUG_ALWAYS("NM Agent :","Received Unsupported message of type 0x%x", hdr->id);

                	}
                	break;
            	}
            }
        }
    }
   
    DTNMP_DEBUG_EXIT("rx_thread","->.", NULL);
    pthread_exit(NULL);
}


void rx_handle_rpt_def(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	def_gen_t* rpt_def = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("rx_handle_rpt_def","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);

    DTNMP_DEBUG_INFO("nm_receive_thread",
    		        "Processing a report definition.", NULL);

    rpt_def = def_deserialize_gen(cursor, size, &bytes);

    if((rpt_def == NULL) || (bytes == 0))
    {
    	DTNMP_DEBUG_ERR("rx_handle_rpt_def","Can;t deserialize.",NULL);
    	def_release_gen(rpt_def);
    	*bytes_used = 0;
    	DTNMP_DEBUG_EXIT("rx_handle_rpt_def","->.",NULL);
    	return;
    }

    *bytes_used = bytes;

//    def_print_gen(rpt_def);
	DTNMP_DEBUG_INFO("rx_handle_rpt_def","Adding new report definition.", NULL);

	agent_db_report_persist(rpt_def);

	agent_vdb_reports_init(getIonsdr());
	ADD_REPORT(rpt_def);
	gAgentInstr.num_rpt_defs++;


}

void rx_handle_exec(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	ctrl_exec_t* ctrl = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("rx_handle_exec","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);

    DTNMP_DEBUG_INFO("rx_handle_exec",
    		        "Processing a control.", NULL);

    ctrl = ctrl_deserialize_exec(cursor, size, &bytes);

    if((ctrl == NULL) || (bytes == 0))
    {
    	DTNMP_DEBUG_ERR("rx_handle_exec","Can't deserialize.",NULL);
    	ctrl_release_exec(ctrl);
    	*bytes_used = 0;
    	DTNMP_DEBUG_EXIT("rx_handle_exec","->.",NULL);
    	return;
    }

    /* \todo: Handle relative and absolute times */
	ctrl->countdown_ticks = ctrl->time;
	ctrl->desc.state = CONTROL_ACTIVE;
	strcpy(ctrl->desc.sender.name, meta->senderEid.name);


    *bytes_used = bytes;

	DTNMP_DEBUG_INFO("rx_handle_exec","Performing control.", NULL);
	ADD_CTRL(ctrl);
	gAgentInstr.num_ctrls++;

}

void rx_handle_time_prod(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
    /* Grab the production rule object. */
	rule_time_prod_t *new_rule = NULL;
    uint32_t bytes = 0;

    DTNMP_DEBUG_INFO("rx_handle_time_prod",
    		         "Processing a production rule.", NULL);

    if((new_rule = ctrl_deserialize_time_prod_entry(cursor, size, &bytes)) != NULL)
    {
    	new_rule->desc.num_evals = new_rule->count;
    	new_rule->desc.interval_ticks = new_rule->period;
    	new_rule->countdown_ticks = new_rule->desc.interval_ticks;

    	strcpy(new_rule->desc.sender.name, meta->senderEid.name);

        if(new_rule->desc.num_evals == 0)
        {
            new_rule->desc.num_evals = DTNMP_RULE_EXEC_ALWAYS;
        }

        *bytes_used = bytes;
    }
    else
    {
    	DTNMP_DEBUG_ERR("rx_handle_time_prod","Can't deserialize!",NULL);
    	return;
    }

    if(rx_validate_rule(new_rule) == 1)
    {
    	DTNMP_DEBUG_INFO("rx_handle_time_prod",
    			         "Adding new production rule.", NULL);

    	agent_db_rule_persist(new_rule);

    	ADD_RULE(new_rule);
    	gAgentInstr.num_time_rules++;

    }

}

void rx_handle_macro_def(pdu_metadata_t *meta, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
	def_gen_t* macro_def = NULL;
	uint32_t bytes = 0;

	DTNMP_DEBUG_ENTRY("rx_handle_macro_def","(0x%x, %d, 0x%x)",
			          (unsigned long)cursor, size, (unsigned long) bytes_used);

    macro_def = def_deserialize_gen(cursor, size, &bytes);

    if((macro_def == NULL) || (bytes == 0))
    {
    	DTNMP_DEBUG_ERR("rx_handle_macro_def","Can;t deserialize.",NULL);
    	def_release_gen(macro_def);
    	*bytes_used = 0;
    	DTNMP_DEBUG_EXIT("rx_handle_macro_def","->.",NULL);
    	return;
    }

    *bytes_used = bytes;

	DTNMP_DEBUG_INFO("rx_handle_macro_def","Adding new report definition.", NULL);

	agent_db_macro_persist(macro_def);
	ADD_MACRO(macro_def);
	gAgentInstr.num_macros++;


}

