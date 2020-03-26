/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ion_if.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to connect to
 **              the local BPA.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/10/11  V.Ramachandran Initial Implementation (JHU/APL)
 **  11/13/12  E. Birrane     Technical review, comment updates. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  06/30/16  E. Birrane     Doc. Updates and Bug Fixes (Secure DTN - NASA: NNX14CS58P)
 **  10/02/18  E. Birrane     Update to AMP v0.5 (JHUAPL)
 *****************************************************************************/

#include "bp.h"

#include "../utils/nm_types.h"
#include "ion_if.h"
#include "../utils/utils.h"

#include "msg.h"



/******************************************************************************
 *
 * \par Function Name: iif_deregister_node
 *
 * \par Deregisters the current application with the DTN network.
 *
 * \retval 0 - Could not Register
 * 		   1 - Registered.
 *
 * \param[in,out] iif  The Interface being deregistered.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation, (JHU/APL)
 *  06/30/16  E. Birrane     Fix EID init. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int iif_deregister_node(iif_t *iif)
{
    AMP_DEBUG_ENTRY("iif_deregister_node","(%#llx)", (size_t)iif);

    /* Step 0: Sanity Check */
    if(iif == NULL)
    {
    	AMP_DEBUG_ERR("iif_deregister_node","Null IIF.", NULL);
        AMP_DEBUG_EXIT("iif_deregister_node","-> %d", 0);
    	return 0;
    }

    bp_close(iif->sap);
    bp_detach();
    memset(iif->local_eid.name,0, AMP_MAX_EID_LEN);

    AMP_DEBUG_EXIT("iif_deregister_node","-> %d", 1);
    return 1;
}



/******************************************************************************
 *
 * \par Function Name: iif_get_local_eid
 *
 * \par Returns the EID of the local node.
 *
 * \retval The EID associated with the IIF.
 *
 * \param[in] iif  The Interface whose local EID is being queried.
 *
 * \par Notes:
 *         1. Assumes the IIF exists at this point.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation. (JHU/APL)
 *****************************************************************************/

eid_t iif_get_local_eid(iif_t *iif)
{
	AMP_DEBUG_ENTRY("iif_get_local_eid","(%#llx)", iif);

	if(iif == NULL)
	{
		eid_t result;
		AMP_DEBUG_ERR("iif_get_local_eid","Bad args.",NULL);
		memset(&result,0,sizeof(eid_t));
		AMP_DEBUG_EXIT("iif_get_local_eid","->0.",NULL);
		return result;
	}

	AMP_DEBUG_EXIT("iif_get_local_eid","->1.",NULL);
    return iif->local_eid;
}



/******************************************************************************
 *
 * \par Function Name: iif_is_registered
 *
 * \par Returns 1 if the DTN connection is active, 0 otherwise.
 *
 * \retval 1 - IIF is registered
 *         0 - IIF is not registered.
 *
 * \param[in] iif  The Interface whose registration status is being queried.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation. (JHU/APL)
 *****************************************************************************/

int iif_is_registered(iif_t *iif)
{
	uint8_t result = 0;

	AMP_DEBUG_ENTRY("iif_is_registered","(%#llx)", iif);

	if(iif == NULL)
	{
		AMP_DEBUG_ERR("iif_is_registered","Bad args.",NULL);
		AMP_DEBUG_EXIT("iif_is_registered","->0.",NULL);
		return result;
	}

	result = (iif->local_eid.name[0] != 0) ? 1 : 0;

	AMP_DEBUG_EXIT("iif_is_registered","->%d.",NULL);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: iif_receive
 *
 * \par Blocking receive. Receives a message from the BPA.
 *
 * \retval NULL - Error
 *         !NULL - The received serialized payload.
 *
 * \param[in]  iif     The registered interface.
 * \param[out] size    The size of the msg bundle.
 * \param[out] meta    The sender information from the convergence layer for all msgs.
 * \param[in]  timeout The # seconds to wait on a receive before timing out
 *
 * \par Notes:
 *   - The returned data must be freed via zco_destroy_reference()
 *
 * \todo
 *   - Use ZCOs and handle large message sizes.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation (JHU/APL)
 *  10/04/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

blob_t *iif_receive(iif_t *iif, msg_metadata_t *meta, int timeout, int *success)
{
    BpDelivery dlv;
    ZcoReader reader;
    int content_len;
    Sdr sdr = bp_get_sdr();
    blob_t *result;
    int status;


    AMP_DEBUG_ENTRY("iif_receive", "("ADDR_FIELDSPEC", %d)",
    		         (uaddr) iif, timeout);

    AMP_DEBUG_INFO("iif_receive", "Received bundle.", NULL);

	*success = AMP_SYSERR;

    CHKNULL(iif);
    CHKNULL(meta);

	*success = AMP_OK;

	memset(&dlv, 0, sizeof(BpDelivery));

    /* Step 1: Receive the bundle.*/
    if((status = bp_receive(iif->sap, &dlv, timeout)) < 0)
    {
    	AMP_DEBUG_INFO("iif_receive","bp_receive failed. Result: %d.", result);
    	*success = AMP_SYSERR;
    	return NULL;
    }
    else
    {
    	switch(dlv.result)
    	{
    		case BpEndpointStopped:
    			/* The endpoint stopped? Panic.*/
//    			AMP_DEBUG_INFO("iif_receive","Endpoint stopped.", NULL);
    			return NULL;

    		case BpPayloadPresent:
    			/* Clear to process the payload. */
 //   			AMP_DEBUG_INFO("iif_receive", "Payload present.", NULL);
    			break;

    		default:
    			/* No message. Return NULL. */
    			return NULL;
    			break;
    	}
    }
    content_len = zco_source_data_length(sdr, dlv.adu);

    /* Step 2: Allocate result space. */
    if((result = blob_create(NULL, 0, content_len)) == NULL)
    {
    	*success = AMP_SYSERR;

    	AMP_DEBUG_ERR("iif_receive","Can't alloc %d of msg.", content_len);
    	AMP_DEBUG_ERR("iif_receive","Timeout is %d.", timeout);
        bp_release_delivery(&dlv, 1);
    	return NULL;
    }

    /* Step 3: Read the bundle in from the ZCO. */
    if (sdr_begin_xn(sdr) < 0)
    {
    	*success = AMP_SYSERR;

    	blob_release(result, 1);
    	AMP_DEBUG_ERR("iif_receive","Can't start transaction.", NULL);
        bp_release_delivery(&dlv, 1);
        return NULL;
    }

    zco_start_receiving(dlv.adu, &reader);
    result->length = zco_receive_source(sdr, &reader, result->alloc, (char*)result->value);

    if((sdr_end_xn(sdr) < 0) || (result->length == 0))
    {
    	*success = AMP_SYSERR;

        AMP_DEBUG_ERR("iif_receive", "Unable to process received bundle.", NULL);
        blob_release(result, 1);
        bp_release_delivery(&dlv, 1);
        return NULL;
    }

    /* Step 5: Set up the metadata. */

    istrcpy(meta->senderEid.name, dlv.bundleSourceEid,
		    sizeof meta->senderEid.name);
    istrcpy(meta->originatorEid.name, dlv.bundleSourceEid,
		    sizeof meta->originatorEid.name);
    istrcpy(meta->recipientEid.name, iif->local_eid.name,
		    sizeof meta->recipientEid.name);

    bp_release_delivery(&dlv, 1);
    
    AMP_DEBUG_EXIT("iif_receive", "->"ADDR_FIELDSPEC, (uaddr) result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: iif_register_node
 *
 * \par Registers the current application with the DTN network.
 *
 * \retval 0 - Could not Register
 * 		   1 - Registered.
 *
 * \param[out] iif  Updated IIF structure.
 * \param[in]  eid  EID of the node we are registering.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation . (JHU/APL)
 *****************************************************************************/

int iif_register_node(iif_t *iif, eid_t eid)
{
    AMP_DEBUG_ENTRY("iif_register_node","(%s)", eid.name);
    
    /* Step 0: Sanity Check */
    if(iif == NULL)
    {
    	AMP_DEBUG_ERR("iif_register_node","Null IIF.", NULL);
        AMP_DEBUG_EXIT("iif_register_node","-> %d", 0);
    	return 0;
    }

    memset((char*)iif, 0, sizeof(iif_t));
    iif->local_eid = eid;

    if(bp_attach() < 0)
    {
        AMP_DEBUG_ERR("iif_register_node","Failed to attach.", NULL);
        AMP_DEBUG_EXIT("iif_register_node","-> %d", 0);
        return 0;
    }
    
    if(bp_open((char *)eid.name, &(iif->sap)) < 0)
    {
        AMP_DEBUG_ERR("iif_register_node","Failed to open %s.", eid.name);
        AMP_DEBUG_EXIT("iif_register_node","-> %d", 0);
        return 0;
    }

    AMP_DEBUG_INFO("iif_register_node","Registered Agent as %s.", eid.name);

    
    AMP_DEBUG_EXIT("iif_register_node","-> %d", 1);
    return 1;
}



/******************************************************************************
 *
 * \par Function Name: iif_send
 *
 * \par Sends a text string to the recipient node.
 *
 * \retval Whether the send succeeded (1) or failed (0)
 *
 * \param[in] iif     The registered interface
 * \param[in] data    The data to send.
 * \param[in] len     The length of data to send, in bytes.
 * \param[in] rx_eid  The EID of the recipient of the data.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  08/10/11  V.Ramachandran Initial implementation. (JHU/APL)
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 *  03/??/16  E. Birrane     Fix BP Send to latest ION version. (Secure DTN - NASA: NNX14CS58P)
 *  10/02/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int iif_send_grp(iif_t *iif, msg_grp_t *group, char *rx)
{
    Object extent = 0;
    Object newBundle = 0;
    blob_t *data = NULL;

    AMP_DEBUG_ENTRY("iif_send","("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
    		(uaddr)iif, (uaddr)group, (uaddr)rx);

    CHKZERO(iif);
    CHKZERO(group);
    CHKZERO(rx);

    /* Step 1 - Serialize the bundle. */
    if((data = msg_grp_serialize_wrapper(group)) == NULL)
    {
    	AMP_DEBUG_ERR("iif_send","Bad message of length 0.", NULL);
    	AMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;
    }

    if(data->length == 0)
    {
    	AMP_DEBUG_ERR("iif_send","Cannot send empty data.", NULL);
    	blob_release(data, 1);
    	return AMP_FAIL;
    }

    /* Information on bitstream we are sending. */
    char *msg_str = utils_hex_to_string(data->value, data->length);
    AMP_DEBUG_ALWAYS("iif_send","Sending msgs:%s to %s:", msg_str, rx);
    SRELEASE(msg_str);


    /* Step 2 - Get the SDR, insert the message as an SDR transaction.*/
    Sdr sdr = bp_get_sdr();

    if (sdr_begin_xn(sdr) < 0)
    {
    	blob_release(data, 1);
	    AMP_DEBUG_ERR("iif_send","Unable to start transaction.", NULL);
	    return 0;
    }

    extent = sdr_malloc(sdr, data->length);
    if(extent)
    {
       sdr_write(sdr, extent, (char *) data->value, data->length);
    }
    else
    {
    	blob_release(data, 1);
    	AMP_DEBUG_ERR("iif_send","Can't write to NULL extent.", NULL);
    	sdr_cancel_xn(sdr);
    	return 0;
    }

    if (sdr_end_xn(sdr) < 0)
    {
    	AMP_DEBUG_ERR("iif_send","Can't close transaction?", NULL);
    }
        
    /* Step 3 - Create ZCO.*/
    Object content = ionCreateZco(ZcoSdrSource, extent, 0, data->length, BP_STD_PRIORITY, 0, ZcoOutbound, NULL);

    if(content == 0 || content == (Object) ERROR)
    {
    	blob_release(data, 1);
        AMP_DEBUG_ERR("iif_send","Zero-Copy Object creation failed.", NULL);
        AMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;
    }

    /* Step 4 - Pass on to the BPA to send.*/
    int res = 0;
    if((res = bp_send(
				iif->sap, 		// BpSAP reference
				rx,              // recipient
				NULL,			// report-to
				300,			// lifespan (?)
				BP_STD_PRIORITY,	// Class-of-Service / Priority
				NoCustodyRequested,	// Custody Switch
				0,			// SRR Flags
				0,			// ACK Requested
				NULL,		// Extended COS
				content,		// ADU
				&newBundle		// New Bundle
				)) != 1)
    {
        AMP_DEBUG_ERR("iif_send","Send failed (%d) to %s.", res, rx);
        blob_release(data, 1);
        AMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;

    }

    blob_release(data, 1);
    AMP_DEBUG_EXIT("iif_send", "->1.", NULL);
    return 1;
}

// Caller MUST release msg.
int iif_send_msg(iif_t *iif, int msg_type, void *msg, char *recipient)
{
	msg_grp_t *grp = msg_grp_create(1);
	int success;

	CHKERR(msg);
	CHKERR(grp);

	switch(msg_type)
	{
		case MSG_TYPE_REG_AGENT:
			success = msg_grp_add_msg_agent(grp, msg);
			break;
		case MSG_TYPE_RPT_SET:
			success = msg_grp_add_msg_rpt(grp, msg);
			break;
		case MSG_TYPE_PERF_CTRL:
			success = msg_grp_add_msg_ctrl(grp, msg);
			break;
		case MSG_TYPE_TBL_SET:
            success = msg_grp_add_msg_tbl(grp, msg);
            break;
		default:
			success = AMP_FAIL;
			break;
	}

	if(success == AMP_OK)
	{
		success = iif_send_grp(iif, grp, recipient);
	}

	msg_grp_release(grp, 1);
	return success;
}
 





