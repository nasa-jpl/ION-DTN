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
 **  10/07/18  E. Birrane      Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#include "nm_mgr.h"
#include "platform.h"
#include "agents.h"

#include <pthread.h>

#include "../shared/msg/ion_if.h"
#include "../shared/utils/nm_types.h"
#include "../shared/utils/utils.h"
#include "../shared/utils/debug.h"

#include "../shared/msg/msg.h"

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif

#include "nm_mgr_print.h" // For direct report file logging


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

void rx_data_rpt(msg_metadata_t *meta, msg_rpt_t *msg)
{
    agent_t *agent = NULL;
    int result = -1;

    CHKVOID(meta);
    CHKVOID(msg);

    // TODO: Check to see if we are listed as a recipient for this report.

	/* Step 1: Retrieve stored information about this agent. */
	if((agent = agent_get(&(meta->senderEid))) == NULL)
	{
		AMP_DEBUG_WARN("msg_rx_data_rpt",
				        "Received group is from an unknown sender (%s); ignoring it.",
						meta->senderEid.name);
        agent_t *agent = (agent_t*) vec_at(&gMgrDB.agents, 0);
	}
	else
	{
		vecit_t it;

		for(it = vecit_first(&(msg->rpts)); vecit_valid(it); it = vecit_next(it))
		{
			rpt_t *rpt = vecit_data(it);
            int status = vec_push(&(agent->rpts), rpt);

            if (agent->log_fd != NULL)
            {
                if (agent_log_cfg.rx_rpt)
                {
                    ui_print_cfg_t fd = INIT_UI_PRINT_CFG_FD(agent->log_fd);
                    ui_fprint_report(&fd, rpt);
                    agent->log_fd_cnt++;
                }
#ifdef USE_JSON
                if (agent_log_cfg.rx_json_rpt)
                {
                    ui_print_cfg_t fd = INIT_UI_PRINT_CFG_FD(agent->log_fd);
                    ui_fprint_json_report(&fd, rpt);
                    agent->log_fd_cnt++;
                }
#endif
            }
            
            if (status == VEC_OK)
            {
                gMgrDB.tot_rpts++;
            }
            else // Vector may be full.  Discard (and release) report
            {
                // TODO: Consider retrying after a vec_pop() to replace oldest report
                AMP_DEBUG_WARN("rx_data_rpt", "Failed to push rpt, discarding", NULL);
                rpt_release(rpt, 1);
            }
		}

        if (agent->log_fd != NULL && agent_log_cfg.rx_rpt) {
            fflush(agent->log_fd); // Flush file after we've written set

            // And check for file rotation (we won't break up a set between files)
            agent_rotate_log(agent,0);
        }
	}

	// Make sure we don't delete items when we delete report
	// since we shallow-copied them into the agent report list.
	msg->rpts.delete_fn = NULL;
	msg_rpt_release(msg, 1);
}

/******************************************************************************
 *
 * \par Function Name: msg_rx_data_tbl
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

void rx_data_tbl(msg_metadata_t *meta, msg_tbl_t *msg)
{
    agent_t *agent = NULL;
    int result = -1;

    CHKVOID(meta);
    CHKVOID(msg);

    // TODO: Check to see if we are listed as a recipient for this report.

	/* Step 1: Retrieve stored information about this agent. */
	if((agent = agent_get(&(meta->senderEid))) == NULL)
	{
		AMP_DEBUG_WARN("msg_rx_data_tbl",
				        "Received group is from an unknown sender (%s); ignoring it.",
						meta->senderEid);
	}
	else
	{
		vecit_t it;

		for(it = vecit_first(&(msg->tbls)); vecit_valid(it); it = vecit_next(it))
		{
			tbl_t *tbl = vecit_data(it);
            int status = vec_push(&(agent->tbls), tbl);

            if (agent->log_fd != NULL)
            {
                if(agent_log_cfg.rx_tbl)
                {
                    ui_print_cfg_t fd = INIT_UI_PRINT_CFG_FD(agent->log_fd);
                    ui_fprint_table(&fd, tbl);
                    agent->log_fd_cnt++;
                }
#ifdef USE_JSON
                if (agent_log_cfg.rx_json_tbl)
                {
                    ui_print_cfg_t fd = INIT_UI_PRINT_CFG_FD(agent->log_fd);
                    ui_fprint_json_table(&fd, tbl);
                    agent->log_fd_cnt++;
                }
#endif

            }

            if (status == VEC_OK)
            {
                gMgrDB.tot_tbls++;
            }
            else // Vector may be full.  Discard (and release) report
            {
                // TODO: Consider retrying after a vec_pop() to replace oldest report
                AMP_DEBUG_WARN("rx_data_tbl", "Failed to push tbl, discarding", NULL);
                tbl_release(tbl, 1);
            }
		}

        if (agent->log_fd != NULL && agent_log_cfg.rx_tbl) {
            fflush(agent->log_fd); // Flush file after we've written set

            // And check for file rotation (we won't break up a set between files)
            agent_rotate_log(agent,0);
        }
	}

	// Make sure we don't delete items when we delete report
	// since we shallow-copied them into the agent report list.
	msg->tbls.delete_fn = NULL;
	msg_tbl_release(msg, 1);
}

void rx_agent_reg(msg_metadata_t *meta, msg_agent_t *msg)
{
    CHKVOID(meta);
    CHKVOID(msg);

	agent_add(msg->agent_id);

#ifdef HAVE_MYSQL
	db_add_agent(msg->agent_id);
#endif

	msg_agent_release(msg, 1);
}

/******************************************************************************
 *
 * \par Function Name: mgr_rx_thread
 *
 * \par Process incoming messages from the DTNMP agent.
 *
 * \return POSIX thread info...
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

    AMP_DEBUG_ENTRY("mgr_rx_thread","(0x%x)", (size_t) running);
    
    AMP_DEBUG_INFO("mgr_rx_thread","Receiver thread running...", NULL);
    
    vecit_t it;

    int success;
    blob_t *buf = NULL;
    msg_grp_t *grp = NULL;
    msg_metadata_t meta;
    int msg_type;


    /* 
     * g_running controls the overall execution of threads in the
     * NM Agent.
     */
    while(*running) {

        /* Step 1: Receive a message from the Bundle Protocol Agent. */
        buf = iif_receive(&ion_ptr, &meta, NM_RECEIVE_TIMEOUT_SEC, &success);
        if(success != AMP_OK)
        {
        	*running = 0;
        }
        else if(buf != NULL)
        {
            if (agent_log_cfg.rx_cbor == 1) {
                agent_t *agent = agent_get(&(meta.senderEid));
                if (agent && agent->log_fd) {
                    char *tmp = utils_hex_to_string(buf->value, buf->length);
                    fprintf(agent->log_fd, "RX: msgs:%s\n", tmp);
                    fflush(agent->log_fd);
                    SRELEASE(tmp);
                }
            }
#if 1 // DEBUG
            char *tmp = utils_hex_to_string(buf->value, buf->length);
            printf("RX from %s: msgs:%s\n", meta.senderEid.name, tmp);
            SRELEASE(tmp);
#endif

        	grp = msg_grp_deserialize(buf, &success);
        	blob_release(buf, 1);

    		if((grp == NULL) || (success != AMP_OK))
    		{
    			AMP_DEBUG_ERR("mgr_rx_thread","Discarding invalid message.", NULL);
    			continue;
    		}

    		AMP_DEBUG_INFO("mgr_rx_thread","Group had %d msgs", vec_num_entries(grp->msgs));
    		AMP_DEBUG_INFO("mgr_rx_thread","Group timestamp %lu", grp->time);

#ifdef HAVE_MYSQL
            /* Copy the message group to the database tables */
            int32_t incoming_idx = db_incoming_initialize(grp->time, meta.senderEid);
#endif

            /* For each message in the group. */
            for(it = vecit_first(&(grp->msgs)); vecit_valid(it); it = vecit_next(it))
            {
            	vec_idx_t i = vecit_idx(it);
            	blob_t *msg_data = (blob_t*) vecit_data(it);

#ifdef HAVE_MYSQL
            	if(msg_data != NULL)
            	{
            		db_incoming_process_message(incoming_idx, msg_data);
            	}
#endif

            	/* Get the message type. */
            	msg_type = msg_grp_get_type(grp, i);
            	success = AMP_FAIL;
            	switch(msg_type)
            	{
            		case MSG_TYPE_RPT_SET:
            		{
            			msg_rpt_t *rpt_msg = msg_rpt_deserialize(msg_data, &success);
            			rx_data_rpt(&meta, rpt_msg);
            			break;
            		}
            		case MSG_TYPE_TBL_SET:
            		{
            			msg_tbl_t *tbl_msg = msg_tbl_deserialize(msg_data, &success);
            			rx_data_tbl(&meta, tbl_msg);
            			break;
            		}
            		case MSG_TYPE_REG_AGENT:
            		{
            			msg_agent_t *agent_msg = msg_agent_deserialize(msg_data, &success);
            			rx_agent_reg(&meta, agent_msg);
            			break;
            		}
            		default:
            			AMP_DEBUG_WARN("mgr_rx_thread","Unknown message type: %d", msg_type);
            			break;
            	}

            }

            msg_grp_release(grp, 1);
#ifdef HAVE_MYSQL
            db_incoming_finalize(incoming_idx);
#endif
            memset(&meta, 0, sizeof(meta));
        }
    }
   

#ifdef HAVE_MYSQL
	db_mgt_close();
#endif

    AMP_DEBUG_ALWAYS("mgr_rx_thread", "Exiting.", NULL);
    AMP_DEBUG_EXIT("mgr_rx_thread","->.", NULL);
    pthread_exit(NULL);
    return NULL;
}

