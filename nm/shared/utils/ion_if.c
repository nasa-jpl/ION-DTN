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
 **  08/10/11  V.Ramachandran Initial Implementation
 **  11/13/12  E. Birrane     Technical review, comment updates.
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

#include "bp.h"

#include "shared/utils/nm_types.h"
#include "shared/utils/ion_if.h"
#include "shared/utils/utils.h"

#include "shared/msg/pdu.h"



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
 *  08/10/11  V.Ramachandran Initial implementation,
 *****************************************************************************/

uint8_t iif_deregister_node(iif_t *iif)
{
    DTNMP_DEBUG_ENTRY("iif_deregister_node","(%#llx)", (unsigned long)iif);

    /* Step 0: Sanity Check */
    if(iif == NULL)
    {
    	DTNMP_DEBUG_ERR("iif_deregister_node","Null IIF.", NULL);
        DTNMP_DEBUG_EXIT("iif_deregister_node","-> %d", 0);
    	return 0;
    }

    bp_close(iif->sap);
    bp_detach();
    memset(iif->local_eid.name,0, 1024);

    DTNMP_DEBUG_EXIT("iif_deregister_node","-> %d", 1);
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
 *  08/10/11  V.Ramachandran Initial implementation,
 *****************************************************************************/

eid_t iif_get_local_eid(iif_t *iif)
{
	DTNMP_DEBUG_ENTRY("iif_get_local_eid","(%#llx)", iif);

	if(iif == NULL)
	{
		eid_t result;
		DTNMP_DEBUG_ERR("iif_get_local_eid","Bad args.",NULL);
		memset(&result,0,sizeof(eid_t));
		DTNMP_DEBUG_EXIT("iif_get_local_eid","->0.",NULL);
		return result;
	}

	DTNMP_DEBUG_EXIT("iif_get_local_eid","->1.",NULL);
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
 *  08/10/11  V.Ramachandran Initial implementation,
 *****************************************************************************/

uint8_t iif_is_registered(iif_t *iif)
{
	uint8_t result = 0;

	DTNMP_DEBUG_ENTRY("iif_is_registered","(%#llx)", iif);

	if(iif == NULL)
	{
		DTNMP_DEBUG_ERR("iif_is_registered","Bad args.",NULL);
		DTNMP_DEBUG_EXIT("iif_is_registered","->0.",NULL);
		return result;
	}

	result = (iif->local_eid.name[0] != 0) ? 1 : 0;

	DTNMP_DEBUG_EXIT("iif_is_registered","->%d.",NULL);
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
 *  08/10/11  V.Ramachandran Initial implementation,
 *****************************************************************************/

uint8_t *iif_receive(iif_t *iif, uint32_t *size, pdu_metadata_t *meta, int timeout)
{
    BpDelivery dlv;
    ZcoReader reader;
    int dataLength;
    int content_len;
    Sdr sdr = bp_get_sdr();
    uint8_t *buffer = NULL;
    uint8_t *cursor = NULL;
    pdu_msg_t *pdu = NULL;
    uint32_t bytes = 0;
    uint32_t buf_size = 0;
    int result;

    DTNMP_DEBUG_ENTRY("iif_receive", "(0x%x, %d)",
    		         (unsigned long) iif, timeout);

    DTNMP_DEBUG_INFO("iif_rceive", "Received bundle.", NULL);

    /* Step 1: Receive the bundle.*/
    if((result = bp_receive(iif->sap, &dlv, timeout)) < 0)
    {
    	DTNMP_DEBUG_ERR("iif_receive","bp_receive failed. Result: %d.", result);
    	exit(0);
    }
    else
    {
    	switch(dlv.result)
    	{
    		case BpEndpointStopped:
    			/* The endpoint stopped? Panic.*/
    			DTNMP_DEBUG_ERR("iif_receive","Endpoint stopped.", NULL);
    			exit(0);

    		case BpPayloadPresent:
    			/* Clear to process the payload. */
    			DTNMP_DEBUG_ERR("iif_receive", "Payload present.", NULL);
    			break;

    		default:
    			/* No message. Return NULL. */
    			return NULL;
    			break;
    	}
    }
    content_len = zco_source_data_length(sdr, dlv.adu);

    /* Step 2: Allocate result space. */
    *size = content_len;
    if((buffer = (uint8_t*) MTAKE(content_len)) == NULL)
    {
    	DTNMP_DEBUG_ERR("iif_receive","Can't alloc %d of msg.", content_len);
    	DTNMP_DEBUG_ERR("iif_receive","Timeout is %d.", timeout);

    	DTNMP_DEBUG_EXIT("iif_receive","->NULL",NULL);
    	return NULL;
    }

    /* Step 2: Read the bundle in from the ZCO. */
    sdr_begin_xn(sdr);
    zco_start_receiving(dlv.adu, &reader);
    dataLength = zco_receive_source(sdr, &reader, content_len, (char*)buffer);

    if(sdr_end_xn(sdr) < 0 || dataLength < 0)
    {
        DTNMP_DEBUG_ERR("iif_receive", "Unable to process received bundle.", NULL);
        MRELEASE(buffer);

        DTNMP_DEBUG_EXIT("iif_receive","-> NULL", NULL);
        return NULL;
    }

    /* Step 5: Set up the metadata. */

    strcpy(meta->senderEid.name, dlv.bundleSourceEid);
    strcpy(meta->originatorEid.name, dlv.bundleSourceEid);
    strcpy(meta->recipientEid.name, iif->local_eid.name);

    DTNMP_DEBUG_EXIT("iif_receive", "->0x%x", (unsigned long) buffer);
    return buffer;
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
 *  08/10/11  V.Ramachandran Initial implementation,
 *****************************************************************************/

uint8_t iif_register_node(iif_t *iif, eid_t eid)
{
    DTNMP_DEBUG_ENTRY("iif_register_node","(%s)", eid.name);
    
    /* Step 0: Sanity Check */
    if(iif == NULL)
    {
    	DTNMP_DEBUG_ERR("iif_register_node","Null IIF.", NULL);
        DTNMP_DEBUG_EXIT("iif_register_node","-> %d", 0);
    	return 0;
    }

    iif->local_eid = eid;

    if(bp_attach() < 0)
    {
        DTNMP_DEBUG_ERR("iif_register_node","Failed to attach.", NULL);
        DTNMP_DEBUG_EXIT("iif_register_node","-> %d", 0);
        return 0;
    }
    
    if(bp_open((char *)eid.name, &(iif->sap)) < 0)
    {
        DTNMP_DEBUG_ERR("iif_register_node","Failed to open %s.", eid.name);
        DTNMP_DEBUG_EXIT("iif_register_node","-> %d", 0);
        return 0;
    }

    DTNMP_DEBUG_INFO("iif_register_node","Registered Agent as %s.", eid.name);

    
    DTNMP_DEBUG_EXIT("iif_register_node","-> %d", 1);
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
 *  08/10/11  V.Ramachandran Initial implementation,
 *  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/

uint8_t iif_send(iif_t *iif, pdu_group_t *group, char *recipient)
{
    BpExtendedCOS extendedCOS = {0, 0, 0};
    Object extent;

    Object newBundle;
    int sdrDataLength; // Space allocated in SDR

    uint8_t *data = NULL;
    uint32_t len;

    DTNMP_DEBUG_ENTRY("iif_send","(%#llx, %#llx, %#llx)", iif, group, recipient);

    /* Step 0 - Sanity checks. */
    if((iif == NULL) || (group == NULL) || (recipient == NULL))
    {
    	DTNMP_DEBUG_ERR("iif_send","Bad Args.", NULL);
    	DTNMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;
    }

    /* Step 1 - Serialize the bundle. */
    data = pdu_serialize_group(group, &len);

    if(len == 0)
    {
    	MRELEASE(data);
    	DTNMP_DEBUG_ERR("iif_send","Bad message of length 0.", NULL);
    	DTNMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;
    }

    /* Information on bitstream we are sending. */
    DTNMP_DEBUG_INFO("iif_send","Sending following data of length %d",len);
    utils_print_hex(data, len);


    /* Step 2 - Get the SDR, insert the message as an SDR transaction.*/
    Sdr sdr = bp_get_sdr();

    CHKZERO(sdr_begin_xn(sdr));
    extent = sdr_malloc(sdr, len);
    if(extent)
    {
       sdr_write(sdr, extent, (char *) data, len);
    }
    else
    {
    	DTNMP_DEBUG_ERR("iif_send","Can't write to NULL extent.", NULL);
    }

   // Object sdrObj = sdr_insert(sdr, (char *) data, len);
    if (sdr_end_xn(sdr) < 0)
    {
    	DTNMP_DEBUG_ERR("iif_send","Can't close transaction?", NULL);
    }
        
    /* Step 3 - Great ZCO in an SDR transaction.*/
    Object content = ionCreateZco(ZcoSdrSource, extent, 0, len, 0, 0, 0, NULL);

    //Object content = zco_create(sdr, ZcoSdrSource, sdrObj, 0, len);
    if(!content)
    {
        DTNMP_DEBUG_ERR("iif_send","Zero-Copy Object creation failed.", NULL);
        DTNMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;
    }

    /* Step 4 - Pass on to the BPA to send.*/
    int res = 0;
    if((res = bp_send(
				iif->sap, 		// BpSAP reference
				recipient,              // recipient
				NULL,			// report-to
				300,			// lifespan (?)
				BP_STD_PRIORITY,	// Class-of-Service / Priority
				NoCustodyRequested,	// Custody Switch
				0,			// SRR Flags
				0,			// ACK Requested
				&extendedCOS,		// Extended COS
				content,		// ADU
				&newBundle		// New Bundle
				)) != 1)
    {
        DTNMP_DEBUG_ERR("iif_send","Send failed (%d).", res);
        MRELEASE(data);
        DTNMP_DEBUG_EXIT("iif_send", "->0.", NULL);
    	return 0;

    }

    MRELEASE(data);
    DTNMP_DEBUG_EXIT("iif_send", "->1.", NULL);
    return 1;
}

 
 
