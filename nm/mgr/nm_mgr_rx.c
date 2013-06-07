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
    uint64_t val;
    eid_t *sender_eid = NULL;
    Agent_rx *agent = NULL;

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

            /* Retrieve stored information about this agent. */
            if((agent = get_agent(sender_eid)) == NULL)
            {
            	DTNMP_DEBUG_INFO("mgr_rx_thread","Received bundle is from an unknown sender (%s); ignoring it.",
            		sender_eid->name);
            	continue;
            }

            /* Grab # messages in this bundle. */
            cursor = buf;
            bytes = utils_grab_sdnv(cursor, size, &val);
            num_msgs = val;
            cursor += bytes;
            size -= bytes;

            /* For each message in the bundle. */
            for(i = 0; i < num_msgs; i++)
            {
            	hdr = pdu_deserialize_hdr(cursor, size, &bytes);
            	cursor += bytes;
            	size -= bytes;
            
            	switch (hdr->id)
            	{
                	case MSG_TYPE_RPT_DATA_RPT:
                	{
                		DTNMP_DEBUG_INFO("mgr_rx_thread",
                				         "Processing a data report.\n\n", NULL);

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
                	break;
                
                	default:
                	{
                		DTNMP_DEBUG_WARN("mgr_rx_thread","Unknown message type: %d",
                				hdr->type);
                	}
                	break;
            	}
            }
        }
    }
   
    DTNMP_DEBUG_EXIT("mgr_rx_thread","->.", NULL);
    pthread_exit(NULL);
}

