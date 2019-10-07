/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
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
 **  10/04/18  E. Birrane      Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#include "ingest.h"
#include "instr.h"
#include "nmagent.h"
#include "lcc.h"
#include "../shared/utils/db.h"
#include "../shared/primitives/ctrl.h"




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
 *  10/04/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

void *rx_thread(int *running) {
#ifndef mingw
    AMP_DEBUG_ENTRY("rx_thread","(0x%X)",(unsigned long) pthread_self());
#endif
    AMP_DEBUG_INFO("rx_thread","Receiver thread running...", NULL);
    

    vecit_t it;
    blob_t *result = NULL;
    int success;
    msg_grp_t *grp = NULL;
    msg_metadata_t meta;
    int msg_type;

    /* 
     * g_running controls the overall execution of threads in the
     * NM Agent.
     */
    while(*running)
    {
        
    	result = iif_receive(&ion_ptr, &meta, NM_RECEIVE_TIMEOUT_SEC, &success);
    	if(success != AMP_OK)
    	{
    		*running = 0;
    	}
    	else if(result != NULL)
        {
    		/* This MUST be a message group. Get the group. */
    		grp = msg_grp_deserialize(result, &success);
    		blob_release(result, 1);

    		if((grp == NULL) || (success != AMP_OK))
    		{
    			AMP_DEBUG_ERR("rx_thread","Discarding invalid message.", NULL);
    			break;
    		}

            AMP_DEBUG_ALWAYS("rx_thread","Group had %d msgs", vec_num_entries(grp->msgs));
            AMP_DEBUG_ALWAYS("rx_thread","Group timestamp %lu", grp->time);

            /* For each message in the bundle. */
            for(it = vecit_first(&grp->msgs); vecit_valid(it); it = vecit_next(it))
            {
            	vec_idx_t i = vecit_idx(it);
            	/* Get the message type. */
            	msg_type = msg_grp_get_type(grp, i);

            	switch(msg_type)
            	{
            		case MSG_TYPE_PERF_CTRL:
            			AMP_DEBUG_ALWAYS("rx_thread","Received perform control msg.", NULL);
            			rx_handle_perf_ctrl(&meta, vecit_data(it));
            			break;
            		default:
            			AMP_DEBUG_ERR("rx_thread","Unknown Msg. id %d, type %d.",i,msg_type);
            			break;
            	}
            }

            msg_grp_release(grp, 1);
        }
    }
   
    AMP_DEBUG_ALWAYS("rx_thread","Shutting Down Agent Receive Thread.",NULL);
    AMP_DEBUG_EXIT("rx_thread","->.", NULL);
    pthread_exit(NULL);
    return NULL; /* Defensive. */
}




/******************************************************************************
 *
 * \par Function Name: rx_handle_perf_ctrl
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
 *  10/04/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

void rx_handle_perf_ctrl(msg_metadata_t *meta, blob_t *contents)
{
	msg_ctrl_t *msg = NULL;
	int success;
	vec_idx_t i;

	AMP_DEBUG_ENTRY("rx_handle_perf_ctrl","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
					(uaddr)meta, (uaddr) contents);

	if((meta == NULL) || (contents == NULL))
	{
		return;
	}

	/* Step 1: Deserialize the message. */
	msg = msg_ctrl_deserialize(contents, &success);

	if((msg == NULL) || (success != AMP_OK))
	{
		AMP_DEBUG_ERR("rx_handle_perf_ctrl", "Can't get control message.", NULL);
		return;
	}

	/* Process each item in the ARI collection. */
	for(i = 0; i < ac_get_count(msg->ac); i++)
	{
		/* Grab the item ID from the list. */
		ari_t *cur_ari = ac_get(msg->ac, i);
		ctrl_t *ctrl = ctrl_create(cur_ari);

		if(ctrl == NULL)
		{
			AMP_DEBUG_ERR("rx_handle_perf_ctrl","Can't make ctrl %d", i);
			break;
		}

		ctrl_set_exec(ctrl, msg->start, meta->senderEid);


		if(ctrl->start == 0)
		{
			lcc_run_ctrl(ctrl, NULL);
			ctrl_release(ctrl, 1);
		}
		else
		{

			if(db_persist_ctrl(ctrl) != AMP_OK)
			{
				AMP_DEBUG_ERR("rx_ingest_ctrl", "Cannot persist ctrl.", NULL);
				ctrl_release(ctrl, 1);
				break;
			}


			/* Load macro to VDB. */
			if(VDB_ADD_CTRL(ctrl, NULL) != AMP_OK)
			{
				db_forget(&(ctrl->desc), gDB.ctrls);
				AMP_DEBUG_ERR("rx_ingest_ctrl", "Cannot store ctrl in RAM.", NULL);
				ctrl_release(ctrl, 1);
				break;
			}
		}
	}

	msg_ctrl_release(msg, 1);
}





