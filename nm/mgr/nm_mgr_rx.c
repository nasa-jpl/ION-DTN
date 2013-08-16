//
//  nm_receive_thread.cpp
//  DTN NM Agent
//
//  Created by Ramachandran, Vignesh R. (Vinny) on 8/31/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "pthread.h"

#include "nm_mgr.h"
#include "platform.h"

#include "shared/utils/ion_if.h"
#include "shared/utils/nm_types.h"
#include "shared/utils/utils.h"
#include "shared/utils/debug.h"

#include "shared/msg/pdu.h"
#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_admin.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_ctrl.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_db.h"
#endif

int validate_msg()
{
    int result = 0;
    
    DTNMP_DEBUG_ENTRY("validate_msg","()", NULL);

    DTNMP_DEBUG_EXIT("validate_msg","->%d.", result);
    return result;
}

void *mgr_rx_thread(void * threadId) {
   
    DTNMP_DEBUG_ENTRY("mgr_rx_thread","(0x%x)", (unsigned long) threadId);
    
    DTNMP_DEBUG_INFO("mgr_rx_thread","Receiver thread running...", NULL);
    
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
    while(g_running) {
        
        /* Step 1: Receive a message from the Bundle Protocol Agent. */

        buf = iif_receive(&ion_ptr, &size, &meta, NM_RECEIVE_TIMEOUT_MILLIS);
        sender_eid = &(meta.originatorEid);
        
        if(buf != NULL)
        {
            DTNMP_DEBUG_INFO("mgr_rx_thread","Received buf (%x) of size %d",
            		(unsigned long) buf, size);


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

            DTNMP_DEBUG_INFO("mgr_rx_thread","# Msgs %d, TS %llu", num_msgs, group_timestamp);

#ifdef HAVE_MYSQL
            /* Copy the message group to the database tables */
            incoming_idx = db_insert_incoming_initialize(group_timestamp);
#endif

            /* For each message in the group. */
            for(i = 0; i < num_msgs; i++)
            {
            	hdr = pdu_deserialize_hdr(cursor, size, &bytes);
            	cursor += bytes;
            	size -= bytes;
            	hdr_len = bytes;

            	DTNMP_DEBUG_INFO("mgr_rx_thread","Header id %d with len %d", hdr->id, hdr_len);
            	switch (hdr->id)
            	{
                	case MSG_TYPE_RPT_DATA_RPT:
                	{
                		DTNMP_DEBUG_ALWAYS("mgr_rx_thread",
                				         "Processing a data report.\n\n", NULL);

                        /* Retrieve stored information about this agent. */
                        if((agent = get_agent(sender_eid)) == NULL)
                        {
                        	DTNMP_DEBUG_WARN("mgr_rx_thread","Received group is from an unknown sender (%s); ignoring it.",
                        		sender_eid->name);
                        }
                        else
                        {
                           rpt_data_t *report = NULL;
                		   report = rpt_deserialize_data(cursor, size, &bytes);
                		   cursor += bytes;
                		   size -= bytes;

                		   /* \todo Ignoring ACL right now! */
                		   DTNMP_DEBUG_INFO("mgr_rx_thread",
                			   	            "Adding new data report.", NULL);
                		   lockResource(&(agent->mutex));
                		   lyst_insert_last(agent->reports, report);
                		   unlockResource(&(agent->mutex));
                		   g_reports_total++;
                        }
                	}
                	break;
                
                	case MSG_TYPE_ADMIN_REG_AGENT:
                	{
                		DTNMP_DEBUG_ALWAYS("mgr_rx_thread",
                						   "Processing Agent Registration.\n\n",
                						   NULL);
                		adm_reg_agent_t *reg = NULL;
                		reg = msg_deserialize_reg_agent(cursor, size, &bytes);
                		cursor += bytes;
                		size -= bytes;

                		add_agent(reg->agent_id);

                		msg_release_reg_agent(reg);

                	}
                	break;

                	default:
                	{
                		DTNMP_DEBUG_WARN("mgr_rx_thread","Unknown message type: %d",
                				hdr->type);
                		bytes = 0;
                	}
                	break;
            	}


#ifdef HAVE_MYSQL
            	if(bytes > 0)
            	{
            		db_insert_incoming_message(incoming_idx, cursor - (hdr_len + bytes), hdr_len + bytes);
            	}
#endif

            }
#ifdef HAVE_MYSQL
            db_finalize_incoming(incoming_idx);
#endif
        }
    }
   
    DTNMP_DEBUG_EXIT("mgr_rx_thread","->.", NULL);
    pthread_exit(NULL);
}

