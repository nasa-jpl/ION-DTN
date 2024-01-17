/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 ** \file nm_mgr_rx.c
 **
 ** File Name: nm_mgr_rx.c
 **
 **
 ** Subsystem:
 **          Network Manager Daemon: Receive Thread
 **
 ** Description: This file implements the management receive thread that
 ** 		     accepts information from DTNMP agents.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR          DESCRIPTION
 **  --------  ------------    ---------------------------------------------
 **  08/31/11  V. Ramachandran Initial Implementation (JHU/APL)
 **  08/19/13  E. Birrane      Documentation clean up and code review comments. (JHU/APL)
 **  08/21/16  E. Birrane      Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#include "pthread.h"

#include "nm_mgr.h"
#include "platform.h"

#include "../shared/utils/ion_if.h"
#include "../shared/utils/nm_types.h"
#include "../shared/utils/utils.h"
#include "../shared/utils/debug.h"

#include "../shared/msg/pdu.h"
#include "../shared/msg/msg_admin.h"
#include "../shared/msg/msg_ctrl.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif




/******************************************************************************
 *
 * \par Function Name: msg_rx_data_rpt
 *
 * \par Process incoming data report message.
 *
 * \return 0 - Success
 *        -1 - Failure
 *
 * \param[in]  sender_eid  - The agent providing the report.
 * \param[in]  cursor      - Start of the serialized message.
 * \param[in]  size        - The size of the serialized stream holding the msg
 * \param[out] bytes_used  - Bytes used in deserializing the report.
 *
 * \par Notes:
 *		- \todo: We do not process Access Control Lists (ACLs) at this time.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/20/13  E. Birrane     Initial Implementation.
 *****************************************************************************/

int msg_rx_data_rpt(eid_t *sender_eid, uint8_t *cursor, uint32_t size, uint32_t *bytes_used)
{
    agent_t *agent = NULL;
    int result = -1;

	AMP_DEBUG_ENTRY("msg_rx_data_rpt","()",NULL);


	/* Step 0: Sanity Check */
	if((sender_eid == NULL) || (cursor == NULL) || (bytes_used == NULL))
	{
		AMP_DEBUG_ERR("msg_rx_data_rpt","Bad Parms", NULL);
		AMP_DEBUG_EXIT("msg_rx_data_rpt","-->-1",NULL);
		return -1;
	}

//	DTNMP_DEBUG_ALWAYS("msg_rx_data_rpt", "Processing a data report.\n", NULL);
	*bytes_used = 0;

	/* Step 1: Retrieve stored information about this agent. */
	if((agent = mgr_agent_get(sender_eid)) == NULL)
	{
		AMP_DEBUG_WARN("msg_rx_data_rpt",
				         "Received group is from an unknown sender (%s); ignoring it.",
				         sender_eid->name);
	}
	else
	{
		rpt_t *report = NULL;

		if((report = rpt_deserialize_data(cursor, size, bytes_used)) == NULL)
		{
			AMP_DEBUG_ERR("msg_rx_data_rpt","Can't deserialize rpt",NULL);
		}
		else
		{
			/* Step 1.1: Add the report. */
			lockResource(&(agent->mutex));
			lyst_insert_last(agent->reports, report);
			unlockResource(&(agent->mutex));

			result = 0;

			/* Step 1.2: Update statistics. */
			g_reports_total++;
		}
	}

	AMP_DEBUG_EXIT("msg_rx_data_rpt","-->%d", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: mgr_rx_thread
 *
 * \par Process incoming messages from the DTNMP agent.
 *
 * \return POSIC thread info...
 *
 * \param[in]  threadId - Thread identifier...
 *
 * \par Notes:
 *		- \todo: We do not process Access Control Lists (ACLs) at this time.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/20/13  E. Birrane     Code cleanup and documentation.
 *****************************************************************************/

void *mgr_rx_thread(int *running)
{
   
    AMP_DEBUG_ENTRY("mgr_rx_thread","(0x%x)", (unsigned long) running);
    
    AMP_DEBUG_INFO("mgr_rx_thread","Receiver thread running...", NULL);
    
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
    eid_t *sender_eid = NULL;
    agent_t *agent = NULL;
    time_t group_timestamp;
    uint32_t incoming_idx = 0;
    uint32_t hdr_len = 0;

    /* 
     * g_running controls the overall execution of threads in the
     * NM Agent.
     */
    while(*running) {

        /* Step 1: Receive a message from the Bundle Protocol Agent. */
        buf = iif_receive(&ion_ptr, &size, &meta, NM_RECEIVE_TIMEOUT_SEC);
        
        if(buf != NULL)
        {
            AMP_DEBUG_INFO("mgr_rx_thread","Received buf (%x) of size %d",
            		(unsigned long) buf, size);

            sender_eid = &(meta.originatorEid);

            /* Grab # messages in, and timestamp for, this group. */
            cursor = buf;

            bytes = utils_grab_sdnv(cursor, size, &val);
            num_msgs = val;
            cursor += bytes;
            size -= bytes;

            bytes = utils_grab_sdnv(cursor, size, &val);
            group_timestamp = val;
            cursor += bytes;
            size -= bytes;

            AMP_DEBUG_INFO("mgr_rx_thread","# Msgs %d, TS %llu", num_msgs, group_timestamp);

#ifdef HAVE_MYSQL
            /* Copy the message group to the database tables */
            incoming_idx = db_incoming_initialize(group_timestamp, sender_eid);
#endif

            /* For each message in the group. */
            for(i = 0; i < num_msgs; i++)
            {
            	hdr = pdu_deserialize_hdr(cursor, size, &bytes);
            	cursor += bytes;
            	size -= bytes;
            	hdr_len = bytes;

            	AMP_DEBUG_INFO("mgr_rx_thread","Header id %d with len %d", hdr->id, hdr_len);
            	switch (hdr->id)
            	{
                	case MSG_TYPE_RPT_DATA_RPT:
                	{
                		AMP_DEBUG_ALWAYS("mgr_rx_thread",
                				         "Received a data report.\n\n", NULL);

                		msg_rx_data_rpt(sender_eid, cursor, size, &bytes);

                		cursor += bytes;
                		size -= bytes;
                	}
                	break;
                
                	case MSG_TYPE_ADMIN_REG_AGENT:
                	{
                		AMP_DEBUG_ALWAYS("mgr_rx_thread",
                						   "Processing Agent Registration.\n\n",
                						   NULL);

                		adm_reg_agent_t *reg = NULL;
                		reg = msg_deserialize_reg_agent(cursor, size, &bytes);
                		cursor += bytes;
                		size -= bytes;

                		mgr_agent_add(reg->agent_id);

#ifdef HAVE_MYSQL
                		/* Add agent to agent database. */
                		db_add_agent(reg->agent_id);
#endif

                		msg_release_reg_agent(reg);

                	}
                	break;

                	default:
                	{
                		AMP_DEBUG_WARN("mgr_rx_thread","Unknown message type: %d",
                				hdr->type);
                		bytes = 0;
                	}
                	break;
            	}

#ifdef HAVE_MYSQL
            	if(bytes > 0)
            	{
            		db_incoming_process_message(incoming_idx, cursor - (hdr_len + bytes), hdr_len + bytes);
            	}
#endif

            	pdu_release_hdr(hdr);
            	hdr = NULL;
            }
#ifdef HAVE_MYSQL
            db_incoming_finalize(incoming_idx);
#endif

            SRELEASE(buf);
            buf = NULL;
        }
    }
   

#ifdef HAVE_MYSQL
	db_mgt_close();
#endif

    AMP_DEBUG_ALWAYS("mgr_rx_thread", "Exiting.", NULL);
    AMP_DEBUG_EXIT("mgr_rx_thread","->.", NULL);
    pthread_exit(NULL);
}

