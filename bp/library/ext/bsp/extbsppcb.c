/*
 * extbsppcb.c
 *
 *  Created on: Jul 25, 2010
 *      Author: brownrp1
 */

#include "extbsputil.h"
#include "extbsppcb.h"

#if (PCB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

#include "crypto.h"


/*****************************************************************************
 *                     PCB EXTENSIONS INTERFACE FUNCTIONS                    *
 *                                                                           *
 *                                Call Order                                 *
 *                                                                           *
 *       SEND SIDE                                      RECEIVE SIDE         *  
 *                                                                           *
 *   bsp_pcbOffer                                                            *
 *   bsp_pcbProcessOnDequeue                                                 *
 *   bsp_pcbRelease                                                          *
 *                                                   bsp_pcbAcquire          *
 *                                                   bsp_pcbCheck            *
 *                                                   bsp_pcbClear            *
 *                                                                           *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_pcbAcquire
 *
 * \par Purpose: This callback is called when a serialized PCB block is
 *               encountered during bundle reception.  This callback will
 *               deserialize the block into a scratchpad object.
 *
 * \retval int -- 1 - The block was deserialized into a structure in the
 *                    scratchpad
 *                0 - The block was deserialized but does not appear valid.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  The block whose serialized bytes will be deserialized
 *                      in the block's scratchpad.
 * \param[in]      wk   The work area associated with this bundle acquisition.
 *
 * \par Notes:
 *****************************************************************************/

int bsp_pcbAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
    int result = -1;
    BspAbstractSecurityBlock *asb = NULL;

    PCB_DEBUG_PROC("+ bsp_pcbAcquire(%x, %x)",
                  (unsigned long)blk, (unsigned long)wk);

    CHKERR(blk);
    /* Allocate a scratchpad object to hold a structured view of the block. */
    blk->size = sizeof(BspAbstractSecurityBlock);
    if((blk->object = MTAKE(blk->size)) == NULL)
    {
        PCB_DEBUG_ERR("x bsp_pcbAcquire:  MTAKE failed on size %d", blk->size);
        blk->size = 0;
        result = -1;
    }
    else
    {
        /* Clear out the block's scratchpad information */
        asb = (BspAbstractSecurityBlock *) blk->object;
        memset((char *) asb,0, blk->size);

        /* Populate the scratchpad object's ASB. */
        result = bsp_deserializeASB(blk, wk, BSP_PCB_TYPE);

        PCB_DEBUG_INFO("i bsp_pcbAcquire: Deserialize result %d", result);
    }

    PCB_DEBUG_PROC("- bsp_pcbAcquire -> %d", result);

    return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbClear
 *
 * \par Purpose: This callback removes all memory allocated by the BSP module
 *               during the block's acquisition process. This function is the
 *               same for both PRE and POST payload blocks.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the BSP module.
 *      2. The length field associated with each pointer field is accurate
 *      3. A length of 0 implies no memory is allocated to the associated
 *         data field.
 *****************************************************************************/

void    bsp_pcbClear(AcqExtBlock *blk)
{
    PCB_DEBUG_PROC("+ bsp_pcbClear(%x)", (unsigned long) blk);

    CHKVOID(blk);
    if(blk->size > 0)
    {
       BspAbstractSecurityBlock *asb = (BspAbstractSecurityBlock *) blk->object;
       if(asb->resultLen > 0)
       {
          PCB_DEBUG_INFO("i bsp_pcbClear: Release result of len %ld",
                         asb->resultLen);
          MRELEASE(asb->resultData);
          asb->resultData = 0;
          asb->resultLen = 0;
       }
       PCB_DEBUG_INFO("i bsp_pcbClear: Release ASB of len %d", blk->size);

       MRELEASE(blk->object);
       blk->object = NULL;
       blk->size = 0;
    }

    PCB_DEBUG_PROC("- bsp_pcbClear(%c)", ' ');

    return;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a PCB
 * 		 block to a new block that is a copy of the original.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  newBlk The new copy of this extension block.
 * \param[in]      oldBlk The original extension block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *****************************************************************************/

int  bsp_pcbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
   Sdr bpSdr = getIonsdr();
   BspAbstractSecurityBlock asb;
   int result = -1;

   PCB_DEBUG_PROC("+ bsp_pcbCopy(%x, %x)",
                  (unsigned long) newBlk, (unsigned long) oldBlk);

   CHKERR(newBlk);
   CHKERR(oldBlk);
   sdr_read(bpSdr, (char *) &asb, oldBlk->object ,sizeof asb);

   /*
    * Reserve space for the scratchpad object in the block. We aren't actually
    * sure that we will be using the PCB, so we don't go through the hassle of
    * allocating objects in the SDR yet, as that would incur a larger
    * performance penalty on bundles that do not use the PCB.
    */

   newBlk->size = sizeof asb;
   newBlk->object = sdr_malloc(bpSdr, sizeof asb);
   if (newBlk->object == 0)
   {
      PCB_DEBUG_ERR("x bsp_pcbCopy: Failed to SDR allocate of size: %d",
			sizeof asb);
      result = -1;
   }
   else
   {
      sdr_write(bpSdr, newBlk->object, (char *) &asb, sizeof asb);
      result = 0;
   }

   PCB_DEBUG_PROC("- bsp_pcbCopy -> %d", result);

   return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbOffer
 *
 * \par Purpose: This callback determines whether a PCB block is necessary for
 *               this particular bundle, based on local security policies.
 *               However, at this point we may not have enough information
 *               (such as EIDs) to query the security policy. Therefore, the
 *               offer callback ALWAYS adds the PCB block.  When we do the
 *               process on dequeue callback we will have enough information
 *               to determine whether or not the PCB extension block should
 *               be populated or scratched.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that may/may not be added to the bundle.
 * \param[in]      bundle The bundle that might hold this block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *****************************************************************************/

int bsp_pcbOffer(ExtensionBlock *blk, Bundle *bundle)
{
   Sdr bpSdr = getIonsdr();
   BspAbstractSecurityBlock asb;
   int result = -1;

   PCB_DEBUG_PROC("+ bsp_pcbOffer(%x, %x)",
                  (unsigned long) blk, (unsigned long) bundle);

   CHKERR(blk);
   memset((char *) &asb,0,sizeof(BspAbstractSecurityBlock));

   /*
    * Also, we do not allocate the bytes for this block. It is too early to
    * know the size for this block.
    */
   blk->length = 0;
   blk->bytes = 0;

   /*
    * Reserve space for the scratchpad object in the block. We aren't actually
    * sure that we will be using the PCB, so we don't go through the hassle of
    * allocating objects in the SDR yet, as that would incur a larger performance
    * penalty on bundles that do not use the PCB.
    */
   blk->size = sizeof(BspAbstractSecurityBlock);

   blk->object = sdr_malloc(bpSdr, blk->size);
   if (blk->object == 0)
   {
      PCB_DEBUG_ERR("x bsp_pcbOffer: Failed to SDR allocate of size: %d",
            blk->size);
      result = -1;
   }
   else
   {
      sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);
      result = 0;
   }

   PCB_DEBUG_PROC("- bsp_pcbOffer -> %d", result);

   return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCheck
 *
 * \par Purpose: This callback checks a PCB block, upon bundle receipt
 *               to decrypt the payload data in the bundle
 *               For PCB, a long term key is looked up to decrypt a session
 *               key present in the security result.  The decrypted session
 *               key is used to decrypt the payload. If we aren't the security 
 *               destination, this block is quietly ignored.
 *
 * \retval int 0 - The block check was inconclusive
 *             1 - The block check failed.
 *             2 - The block check succeed or aren't the security destination
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *****************************************************************************/

int bsp_pcbCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
    BspAbstractSecurityBlock *asb = NULL;
    BspSecurityInfo secInfo;
    BspPayloadReplaceKit *bprk;
    unsigned char *ltKeyValue = NULL, *sessionKeyValue = NULL;
    unsigned char *decryptedData = NULL;
    unsigned int ltKeyLen = 0, sessionKeyLen = 0;
    char *srcNode, *destNode;
    Sdr bpSdr = getIonsdr();
    Bundle *bundle = &(wk->bundle);

    PCB_DEBUG_PROC("+ bsp_pcbCheck(%x, %x)", (unsigned long) blk, (unsigned long) wk);

    // ***************************************************************************
    // *                             Sanity Checks                              *
    // **************************************************************************

    if((blk == NULL) || (blk->object == NULL) || (wk == NULL))
    {
        discardExtensionBlock(blk);
        PCB_DEBUG_ERR("x bsp_pcbCheck:  Blocks are NULL. %x",
		     (unsigned long) blk);
        PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", -1);
        return -1;
    }

    // Grab ASB 
    asb = (BspAbstractSecurityBlock *) blk->object;
  
    // If we are not the security destination, skip but keep the block.
    if(eidIsLocal(asb->secDest, wk->dictionary) == 0)
    {
	PCB_DEBUG_INFO("- bsp_pcbCheck - Node is not security dest.", NULL);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", 2);
        // Set the processing flag "Forwarded without processing"
        blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
	return 2;  // Maybe return 0.
    }

    srcNode = NULL;
    destNode = NULL;
          // Get the src and dest needed to get key
    oK(printEid(&asb->secSrc, wk->dictionary, &srcNode));
    oK(printEid(&asb->secDest, wk->dictionary, &destNode));
    if (srcNode == NULL || destNode == NULL)
    {
        if (srcNode) MRELEASE(srcNode);
	if (destNode) MRELEASE(destNode);
        discardExtensionBlock(blk);
        PCB_DEBUG_ERR("x bsp_pcbCheck: Error retrieving src and dest nodes to find key.", NULL);
        PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", -1);
        return -1;
    }

    // First get our long term key
    bsp_getSecurityInfo(bundle, BSP_PCB_TYPE, 1,
                        srcNode,
                        destNode,
                        &secInfo);
    MRELEASE(srcNode); MRELEASE(destNode);

    if (secInfo.cipherKeyName[0] == '\0')
    {
        discardExtensionBlock(blk);
	PCB_DEBUG_ERR("x bsp_pcbCheck: No key found for pcb block.", NULL);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", 0);
        return 0;
    }

    // Grab the actual key.
    if((ltKeyValue = (unsigned char *) bsp_retrieveKey((int *)&ltKeyLen, secInfo.cipherKeyName)) == NULL)
    {
        MRELEASE(ltKeyValue);
        discardExtensionBlock(blk);
	PCB_DEBUG_ERR("x bsp_pcbCheck: Can't retrieve long term key %s.",
		  secInfo.cipherKeyName);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", -1);
        return -1;
    }

    getBspItem(BSP_CSPARM_KEY_INFO, asb->resultData, asb->resultLen, &sessionKeyValue,
	       &sessionKeyLen);

    if(sessionKeyValue == NULL) 
    {
        MRELEASE(ltKeyValue);
        discardExtensionBlock(blk);
	PCB_DEBUG_ERR("x bsp_pcbCheck: Can't retrieve session key %s.",
		  sessionKeyValue);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", -1);
        return -1;
    }

    // Decrypt our session key with the long term key
    if( (decryptedData = bsp_pcbCryptSessionKey(sessionKeyValue,
                                                sessionKeyLen,
                                                ltKeyValue,
                                                ltKeyLen)) == NULL)
    {
        MRELEASE(ltKeyValue);
        discardExtensionBlock(blk);
        PCB_DEBUG_ERR("x bsp_pcbCheck: Can't decrypt \
             session key, len %d", sessionKeyLen);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", -1);
        return -1;
    }
    memcpy((char *) sessionKeyValue, (char *) decryptedData, sessionKeyLen);
    // No longer need our long term key and decryptedData
    MRELEASE(ltKeyValue); MRELEASE(decryptedData);

    PCB_DEBUG_PROC("- bsp_pcbCheck: decrypted session key is %s, len %d.",
		  decryptedData, sessionKeyLen);

    // Initialize our payload replacement kit
    if((bprk = bsp_pcbPrepReplaceKit(bundle->payload.content, bundle->payload.length,
                                 wk->headerLength, wk->trailerLength)) == NULL) {
        discardExtensionBlock(blk);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", 2);
        return -1;
    }

    // Use our kit to tear apart the bundle and replace it:
    // isolatePayload isolates the payload and extracts header/trailer                             
    // constructDecryptedPayload decryptes the old payload and rebuilds it with the header/trailer
    if(bsp_pcbIsolatePayload(bpSdr, bprk) != 0 || 
       bsp_pcbConstructDecryptedPayload(bpSdr, bprk, sessionKeyValue, sessionKeyLen) != 0) {
        MRELEASE(bprk);
        discardExtensionBlock(blk);
	PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", -1);
        return -1;
    }

    // Finally, replace the old bundle zco object, with our new one with decrypted payload
    CHKERR(sdr_begin_xn(bpSdr));
    zco_destroy(bpSdr, bundle->payload.content);
    if (sdr_end_xn(bpSdr) < 0)
    {
	    putErrmsg("Transaction failed.", NULL);
    }

    bundle->payload.content = bprk->newBundle;
    MRELEASE(bprk);

    // success, remove the block
    discardExtensionBlock(blk);
    PCB_DEBUG_PROC("- bsp_pcbCheck --> %d", 2);
    return 2;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbProcessOnDequeue
 *
 * \par Purpose: This callback constructs the post-payload PCB block for
 *               inclusion in a bundle at the end of the dequeue phase
 *
 * \retval int 0 - The post-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk  The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 *
 * \par Notes:
 *      1. No other blocks will be added to the bundle, and no existing blocks
 *         in the bundle will be modified.  This must be the last block
 *         modification performed on the bundle before it is transmitted.
 *****************************************************************************/

int bsp_pcbProcessOnDequeue(ExtensionBlock *blk,
                                Bundle *bundle,
                                void *parm)
{
   DequeueContext   *ctxt = (DequeueContext *) parm;
   BspAbstractSecurityBlock asb;
   BspSecurityInfo secInfo;
   Sdr bpSdr = getIonsdr();
   Object encryptedPayloadZco = 0;
   Lyst eidRefs = NULL;
   unsigned char *raw_asb = NULL;
   unsigned char *ltKeyValue = NULL;
   unsigned char *sessionKeyValue = NULL;
   unsigned char *encryptedData = NULL;
   char *srcNode = NULL, *destNode = NULL;
   unsigned int ltKeyLen = 0;
   unsigned int sessionKeyLen = 0;
   Sdnv sessionKeySdnv;
   int result = 0;

   PCB_DEBUG_PROC("+ bsp_pcbProcessOnDequeue(%x, %x, %x)",
                  (unsigned long) blk,
                  (unsigned long) bundle,
                  (unsigned long) ctxt);

    /*
     * Sanity Check...
     */
    if (bundle == NULL || parm == NULL || blk == NULL) {
       PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Bundle or ASB were not \
as expected.", NULL);
       PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
       scratchExtensionBlock(blk);
       return -1;
    }

    // Check the processing flag "Forwarded without processing"
    if(blk->blkProcFlags & BLK_FORWARDED_OPAQUE) 
    {
        // do nothing.. the bytes on the block should be correct and ready for transmission,
        // everything was transferred and serialized just in the recordExtensionBlocks() function 
        // so we just pass it along
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue --> %d", 0);
        return 0;
    }
    else if(blk->size != sizeof(BspAbstractSecurityBlock))
    {
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: block scratchpad ASB size is not \
as expected.", NULL);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue blk->size %d", blk->size);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue --> %d", -1);
	scratchExtensionBlock(blk);
	return -1;
    }
   
    // Grab the ASB.
    sdr_read(bpSdr, (char *) &asb, blk->object, blk->size);

    srcNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
    destNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);

    if(setSecPointsTrans(blk, bundle, &asb, &eidRefs, BSP_PCB_TYPE, ctxt, srcNode, destNode) != 0)
    {  
	MRELEASE(srcNode); MRELEASE(destNode);
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: setSecPointsTrans failed.", NULL);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue --> %d", -1);
	scratchExtensionBlock(blk);
	return -1;
    }
    else if(srcNode == NULL || destNode == NULL)
    {  
	MRELEASE(srcNode); MRELEASE(destNode);
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: a node address is unexpectedly null! \
		       srcNode:%s, destNode:%s", srcNode, destNode);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue --> %d", -1);
	scratchExtensionBlock(blk);
	return -1;
    }

    // If we have no key, then there is either no rule or no key associated 
    // with PCBs over this hop, so we don't need a PCB block in this bundle.
    bsp_getSecurityInfo(bundle, BSP_PCB_TYPE, 1,
			srcNode, destNode, &secInfo);
    MRELEASE(srcNode); MRELEASE(destNode);

    if (secInfo.cipherKeyName[0] == '\0')
    {
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: No key found for pcb block.", NULL);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", 0);
	scratchExtensionBlock(blk);
	return 0; 
    }

    /* Grab the actual key. */
    if((ltKeyValue = (unsigned char *) bsp_retrieveKey((int *)&ltKeyLen, 
						       secInfo.cipherKeyName)) == NULL)
    {   
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Can't retrieve key %s.",
		 secInfo.cipherKeyName);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
	scratchExtensionBlock(blk);
	return -1;
    } 

    /***************************************************************************
     *                      Populate ASB                          *
     ***************************************************************************/

    /* For now, use our ARC4 cipher everytime */
    asb.cipher = BSP_CSTYPE_PCB_ARC4;
    asb.cipherFlags |= BSP_ASB_RES;

    // Grab our session key
    if( (sessionKeyValue = bsp_pcbGenSessionKey(&sessionKeyLen)) == NULL) 
    {
	MRELEASE(ltKeyValue);
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Can't generate \
	   session key, len %d", sessionKeyLen);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
	scratchExtensionBlock(blk);
	return -1;
    }

    // Encrypt our session key with the long term key
    if( (encryptedData = bsp_pcbCryptSessionKey(sessionKeyValue, sessionKeyLen,
						ltKeyValue, ltKeyLen)) == NULL) 
    { 
	MRELEASE(ltKeyValue); MRELEASE(sessionKeyValue);  
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Can't encrypt \
	     session key, len %d", sessionKeyLen);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
	scratchExtensionBlock(blk);
	return -1;
    }

    // Encode key length into sdnv
    encodeSdnv(&sessionKeySdnv, sessionKeyLen);
    asb.resultLen = sessionKeyLen + 1 + sessionKeySdnv.length;
    PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue asb->resultLen(%d)", asb.resultLen);

    if((asb.resultData = MTAKE(sessionKeyLen + 1 + sessionKeySdnv.length)) == NULL)
    {
	MRELEASE(ltKeyValue); MRELEASE(sessionKeyValue); MRELEASE(asb.resultData);
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Can't allocate \
	  ASB result, len %ld.", sessionKeyLen + 2);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
	scratchExtensionBlock(blk);
	return -1;
    }

    /* Construct the new security result. */
    *(asb.resultData) = BSP_CSPARM_KEY_INFO;
    memcpy(asb.resultData + 1, sessionKeySdnv.text, sessionKeySdnv.length);
    memcpy(asb.resultData + 1 + sessionKeySdnv.length, encryptedData, sessionKeyLen);

    // We can release this now since it's no longer needed
    MRELEASE(encryptedData); MRELEASE(ltKeyValue);

    // Encrypt payload and get its encrypted data
    if((bsp_pcbCryptPayload(&encryptedPayloadZco, bundle->payload.content,
			    PCB_ZCO_ENCRYPT_FILENAME,
			    bundle->payload.length, sessionKeyValue, 
			    sessionKeyLen)) < 0) 
    {
	MRELEASE(sessionKeyValue); MRELEASE(asb.resultData);
	if(eidRefs != NULL) lyst_destroy(eidRefs);
	PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Unable to encrypt session key", NULL);
	PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
	scratchExtensionBlock(blk);
	return -1;
    }

    // Replace payload data with encrypted data in place, after destroying original payload data
    CHKERR(sdr_begin_xn(bpSdr));
    zco_destroy(bpSdr, bundle->payload.content);
    if (sdr_end_xn(bpSdr) < 0)
    {
	    putErrmsg("Transaction failed.", NULL);
    }

    bundle->payload.content = encryptedPayloadZco;

    /* Serialize the Abstract Security Block. */
    CHKERR(sdr_begin_xn(bpSdr));
    raw_asb = bsp_serializeASB(&(blk->dataLength), &(asb));

    if((raw_asb == NULL) || (blk->dataLength == 0))
    {
        MRELEASE(sessionKeyValue); MRELEASE(asb.resultData);
        if(eidRefs != NULL) lyst_destroy(eidRefs);
        PCB_DEBUG_ERR("x bsp_pcbProcessOnDequeue: Unable to serialize \
                    ASB. blk->dataLength = %d", blk->dataLength);
        PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", -1);
        scratchExtensionBlock(blk);
        return -1;
    }

    // Store the updated ASB for this block
    sdr_write(bpSdr, blk->object, (char *)&(bundle->payload.content), sizeof(unsigned int));
    if (sdr_end_xn(bpSdr) < 0)
    {
	    putErrmsg("Transaction failed.", NULL);
    }

    MRELEASE(raw_asb);

    // store this block in its bytes array. This is necessary as the
    // block will be added to the bundle when this function exits.
    result = serializeExtBlk(blk, eidRefs, (char *) raw_asb);
    /* Clear the eid references if we used them. */
    if(eidRefs != NULL) lyst_destroy(eidRefs);

    PCB_DEBUG_PROC("- bsp_pcbProcessOnDequeue(%d)", result);
    return result;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbRelease
 *
 * \par Purpose: This callback removes memory allocated by the BSP module
 *               from a particular extension block.
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated memory pools must be
 *                      released.
 *
 * \par Notes:
 *      1. It is OK to simply free the entire scratchpad object, without
 *         explicitly freeing the result data part of the ASB.
 *         bsp_pcbProcessOnDequeue
 *         calculates the security result and inserts it directly into the
 *         bytes array of the block, which is freed by the main ION library.
 *
 *****************************************************************************/

void    bsp_pcbRelease(ExtensionBlock *blk)
{
    PCB_DEBUG_PROC("+ bsp_pcbRelease(%x)", (unsigned long) blk);

    CHKVOID(blk);
    if(blk->size > 0)
    {
       sdr_free(getIonsdr(), blk->object);
    }

    PCB_DEBUG_PROC("- bsp_pcbRelease(%c)", ' ');

    return;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbPrepReplaceKit
 *
 * \par Purpose: This is a helper function for the check callback.
 *               It takes a number of parameters about the existing bundle payload
 *               and initializes a BspPayloadReplaceKit object.
 *
 * \retval BspPayloadReplaceKit * Newly created/initialized replace kit object
 *
 * \param[in] bundle      The address to the old payload where the whole bundle
 *                        is residing
 * \param[in] payloadLen  Length of payload in the bundle payload object
 * \param[in] headerLen   Length of header in the bundle payload object
 * \param[in] trailerLen  Length of trailer in the bundle payload object
 *
 * \par Notes:
 *      1. This function is simply to prep an object that makes managing the surgery
 *         on the payload (taking header/trailer off, decrypting payload,
 *         putting it back) easier.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  09/13/11  R. Brown        Initial Implementation. 
 *****************************************************************************/

BspPayloadReplaceKit *bsp_pcbPrepReplaceKit(Object bundle, unsigned int payloadLen, 
                                        unsigned int headerLen, unsigned int trailerLen) 
{
    BspPayloadReplaceKit *bprk = MTAKE(sizeof(BspPayloadReplaceKit));
    CHKNULL(bprk);

    bprk->headerLen = headerLen;
    bprk->payloadLen = payloadLen;
    bprk->trailerLen = trailerLen;
    bprk->oldBundle = bundle;
    bprk->newBundle = 0;

    PCB_DEBUG_PROC("- bsp_pcbPrepReplaceKit--> %d", 0);
    return bprk;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbConstructDecryptedPayload
 *
 * \par Purpose: This is a helper function for the check callback.
 *               It takes a BspPayloadReplaceKit object and reconstructs the
 *               new payload object, right after using the given key to encrypt the
 *               old payload data.
 *
 * \retval       The value indicating outcome: 0 for success,
 *                                            -1 for failure
 *
 * \param[in] bpSdr       ion's sdr object
 * \param[in] *bprk       Pointer to the replaceKit object

 * \param[in] sessionKeyValue   Pointer to the key to use to encrypt the old data
 * \param[in] sessionKeyLen     Length of the key
 *****************************************************************************/

int bsp_pcbConstructDecryptedPayload(Sdr bpSdr, BspPayloadReplaceKit *bprk,
                                     unsigned char *sessionKeyValue, unsigned int sessionKeyLen)   
{
    Object newSdrAddr = 0;
    int temp = 0;
    unsigned int bundleLen = bprk->payloadLen +  
                              bprk->headerLen + bprk->trailerLen;
    CHKERR(sdr_begin_xn(bpSdr));
    // Put the header onto a new payload object, though it is actually
    // going to be in the "source" portion
    if(bprk->headerLen > 0) {
        if((newSdrAddr = sdr_malloc(bpSdr, bprk->headerLen)) == 0)
        {
            // Error, cleanup
            BSP_DEBUG_ERR("bsp_pcbConstructDecryptedPayload: unable to allocate more\
                           sdr memory for header, len %d", bprk->headerLen);
            sdr_cancel_xn(bpSdr);
            return -1;
        }
        sdr_write(bpSdr, newSdrAddr, (char *)bprk->headerBuff, bprk->headerLen);
        bprk->newBundle = zco_create(bpSdr, ZcoSdrSource, newSdrAddr,
                                         0, bprk->headerLen, ZcoInbound);
    }
    else
    {
        bprk->newBundle = zco_create(bpSdr, ZcoSdrSource, 0, 0, 0, ZcoInbound);
    }

    if (sdr_end_xn(bpSdr) < 0 || bprk->newBundle == (Object) ERROR
    || bprk->newBundle == 0)
    {
	    putErrmsg("Transaction failed.", NULL);
    }

    // Decrypt payload and append the references to resulting file onto our new object
    if((bsp_pcbCryptPayload(&bprk->newBundle, bprk->oldBundle,
                            PCB_ZCO_DECRYPT_FILENAME,
                            bprk->payloadLen, sessionKeyValue,
                            sessionKeyLen)) < 0)
    {
        CHKERR(sdr_begin_xn(bpSdr));
        zco_destroy(bpSdr, bprk->newBundle);
	oK(sdr_end_xn(bpSdr));
        PCB_DEBUG_ERR("x bsp_pcbConstructDecryptedPayload: Unable to decrypt the payload.", NULL);
        return -1;
    }

    CHKERR(sdr_begin_xn(bpSdr));
    // Finally, append the trailer on our now decrypted bundle zco blob
    // going to be in the "source" portion
    if(bprk->trailerLen > 0) 
    {
        if((newSdrAddr = sdr_malloc(bpSdr, bprk->trailerLen)) == 0)
        {
            zco_destroy(bpSdr, bprk->newBundle);
            PCB_DEBUG_ERR("x bsp_pcbConstructDecryptedPayload: unable to \
allocate more sdr memory for trailer, len %d", bprk->trailerLen);
            oK(sdr_end_xn(bpSdr));
            return -1;
        }

        sdr_write(bpSdr, newSdrAddr, (char *) bprk->trailerBuff,
			bprk->trailerLen);
        if (zco_append_extent(bpSdr, bprk->newBundle, ZcoSdrSource,
                          newSdrAddr, 0, bprk->trailerLen) <= 0)
	{
            zco_destroy(bpSdr, bprk->newBundle);
            PCB_DEBUG_ERR("x bsp_pcbConstructDecryptedPayload: unable to \
append extent to trailer ZCO, len %d", bprk->trailerLen);
            oK(sdr_end_xn(bpSdr));
            return -1;
	}
    }

    // Now our new object should have the whole bundle in the source portion
    // make sure the length is correct
    if((temp = zco_length(bpSdr, bprk->newBundle)) != bundleLen)
    {
        // Something went wrong, bail
        zco_destroy(bpSdr, bprk->newBundle);
        PCB_DEBUG_ERR("x bsp_pcbConstructDecryptedPayload: Length of rebuilt bundle with decrypted data != \
             length of original bundle. It is %d should be %d", temp, bundleLen);
        oK(sdr_end_xn(bpSdr));
        return -1;
    }

    if (sdr_end_xn(bpSdr) < 0)
    {
	    putErrmsg("Transaction failed.", NULL);
	    return -1;
    }

    PCB_DEBUG_PROC("- bsp_pcbConstructDecryptedPayload--> %d", 0);
    return 0;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbIsolatePayload
 *
 * \par Purpose: This is a helper function for the check callback.
 *               It takes a BspPayloadReplaceKit object and strips of the old
 *               payload object's header and trailer, saved for placement onto
 *               a new payload object later. In the process, only the true payload
 *               data in the bundle payload object will remain.
 *
 * \retval       The value indicating outcome: 0 for success,
 *                                            -1 for failure
 *
 * \param[in] bpSdr       ion's sdr object
 * \param[in] *bprk       Pointer to the replaceKit object
 *
 *****************************************************************************/

int bsp_pcbIsolatePayload(Sdr bpSdr, BspPayloadReplaceKit *bprk) 
{ 
    int bytesTransmitted, temp = 0;
    ZcoReader reader;

    // We Take apart the old payload content since it contains the ENTIRE
    // bundle in it.. we separate it into the various parts, grab the header, trailer
    // then isolate the payload.  We do this to put it back together
    // with decrypted payload later
    CHKERR(sdr_begin_xn(bpSdr));

    // grab the header & trailer
    zco_start_receiving(bprk->oldBundle, &reader);
    if(bprk->headerLen > 0)
    {
        bprk->headerBuff = MTAKE(bprk->headerLen);
        if(bprk->headerBuff == NULL ||
           (bytesTransmitted = zco_receive_headers(bpSdr, &reader, bprk->headerLen, (char *)bprk->headerBuff)) != bprk->headerLen)
        {
            PCB_DEBUG_ERR("x bsp_pcbIsolatePayload: Failed to assign memory to buffer and grab full header\nreceived %d bytes instead of needed %d",
		bytesTransmitted, bprk->headerLen);
            oK(sdr_end_xn(bpSdr));
            return -1;
        }
        zco_delimit_source(bpSdr, bprk->oldBundle, bprk->headerLen,
			bprk->payloadLen);
    } 
    if(bprk->trailerLen > 0)
    {
        bprk->trailerBuff = MTAKE(bprk->trailerLen);
        if(bprk->trailerBuff == NULL || 
           (bytesTransmitted = zco_receive_trailers(bpSdr, &reader, bprk->trailerLen, (char *)bprk->trailerBuff)) < bprk->trailerLen)
        {
            PCB_DEBUG_ERR("x bsp_pcbIsolatePayload: Failed to assign memory to buffer and grab full trailer\nreceived %d bytes instead of needed %d",
		bytesTransmitted, bprk->trailerLen);
            oK(sdr_end_xn(bpSdr));
            return -1;
        }
    }

    // Now Isolate the payload
    zco_strip(bpSdr, bprk->oldBundle);

    // The length should be == to the payload length now
    if((temp = zco_length(bpSdr, bprk->oldBundle)) != bprk->payloadLen)
    {
        // Something went wrong, bail
        PCB_DEBUG_ERR("x bsp_pcbIsolatePayload: Failed to isolate payload data \
             length of zco %d, should be payload length of %d", temp, bprk->payloadLen);
        oK(sdr_end_xn(bpSdr));
        return -1;
    }

    if (sdr_end_xn(bpSdr) < 0)
    {
	    putErrmsg("Transaction failed.", NULL);
	    return -1;
    }

    PCB_DEBUG_PROC("- bsp_pcbIsolatePayload:--> %d", 0);
    return 0;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCryptPayload
 *
 * \par Purpose: Encrypts/decrypts given payload data and puts it into
 *               a new file. The references to that data will be put into the
 *               given resultZco object. 
 *
 * \retval int - The value indicating outcome: 0 for success,
 *                                            -1 for failure
 *
 * \param[in]  resultZco     - The object which the function will point the
 *                             new data to.
 * \param[in]  payloadData   - The payload data
 * \param[in]  fname         - Part of the filename indicating whether its
 *                             encryption or decryption
 * \param[in]  payloadLen    - Length of the payload data
 * \param[in]  keyValue      - The key to use.
 *
 * \par Notes:
 *      1. Currently, only arc4 is implemented.
 *      2. This function should be updated for a more parameterized security
 *         result based on different ciphersuites.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown       Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

int bsp_pcbCryptPayload(Object *resultZco, Object payloadData, char *fname,  
                        unsigned int payloadLen,
                        unsigned char *keyValue,
                        unsigned int keyLen)
                  
{
   Sdr bpSdr = getIonsdr();
   unsigned char *dataBuffer = NULL;
   Object fileRef = 0;
   ZcoReader dataReader;
   arc4_context arcContext;
   unsigned int bytesRemaining = 0;
   unsigned int chunkSize = PCB_ENCRYPTION_CHUNK_SIZE;
   unsigned int bytesRetrieved = 0;

   PCB_DEBUG_INFO("+ bsp_pcbCryptPayload(0x%x, %ld, %s %d)",
                  (unsigned long) payloadData,
                  payloadLen,
                  keyValue,
                  keyLen);

   CHKERR(keyValue);

   /*   Allocate a data buffer. */
   dataBuffer = MTAKE(PCB_ENCRYPTION_CHUNK_SIZE);
   CHKERR(dataBuffer);

   /*   Prepare the data for processing. */
   CHKERR(sdr_begin_xn(bpSdr));
   zco_start_transmitting(payloadData, &dataReader);

   /* Setup the context for arc4. */
   arc4_setup(&arcContext, keyValue, keyLen);

   bytesRemaining = payloadLen;

   PCB_DEBUG_INFO("i bsp_pcbCryptPayload: size is %d", bytesRemaining);

   while(bytesRemaining > 0)
   {
     // Clear last round's buffer data
     memset(dataBuffer, 0, sizeof(PCB_ENCRYPTION_CHUNK_SIZE));
     
     /* See how many bytes we wish to read in at this time. */
     if(bytesRemaining < chunkSize)
     {
       chunkSize = bytesRemaining;
     }

     /* Read in the appropriate number of bytes. */
     bytesRetrieved = zco_transmit(bpSdr, &dataReader, chunkSize, (char *)dataBuffer);

     if(bytesRetrieved != chunkSize)
     {
        PCB_DEBUG_ERR("x bsp_pcbCryptPayload: Read %d bytes, but expected %d.",
           bytesRetrieved, chunkSize);

        oK(sdr_end_xn(bpSdr));

        PCB_DEBUG_PROC("- bsp_pcbCryptPayload--> %d", -1);
        MRELEASE(dataBuffer);
        return -1;
     }

     /* encrypted chunk and store in resultData */
     #if DEBUG_ENCRYPT_PAYLOAD == 1
     arc4_crypt(&arcContext, bytesRetrieved, dataBuffer, dataBuffer);
     #endif

     if (sdr_end_xn(bpSdr) < 0)
     {
	     putErrmsg("Transaction failed.", NULL);
             MRELEASE(dataBuffer);
	     return -1;
     }

     // Transfer chunk into file and to our resultObject        
     if(transferToZcoFileSource(bpSdr, resultZco, &fileRef, fname, 
                                (char *)dataBuffer, (int) bytesRetrieved) < 0)
     {
        PCB_DEBUG_ERR("x bsp_pcbCryptPayload: Transfer of chunk has failed..", NULL);
        MRELEASE(dataBuffer); 
        return -1;
     }
     CHKERR(sdr_begin_xn(bpSdr));

     bytesRemaining -= bytesRetrieved;
   }

   oK(sdr_end_xn(bpSdr));
   MRELEASE(dataBuffer);
  
   return 0;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCryptSessionKey
 *                                    
 * \par Purpose: Encrypts/decrypts given session key using the given long term
 *               key
 *
 * \retval unsigned char * - The pointer to the now encrypted/decrypted key
 *
 * \param[in]  sessionKey    - The pointer that contains the session key string
 * \param[in]  sessionKeyLen - The length of the session key
 * \param[in]  ltKeyValue    - The longterm key to use to encrypt/decrypt session key
 * \param[in]  ltKeyLen      - Length of the long term key
 *
 * \par Notes:
 *      1. Currently, only arc4 is implemented.
 *      2. This function should be updated for a more parameterized security
 *         result based on different ciphersuites.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation.
 *****************************************************************************/

unsigned char *bsp_pcbCryptSessionKey(unsigned char *sessionKey, 
                                      unsigned int sessionKeyLen, 
                                      unsigned char *ltKeyValue,
                                      unsigned int ltKeyLen) 
{
    unsigned char *encryptedKey = MTAKE(sessionKeyLen); 
    CHKNULL(encryptedKey);

    arc4_context arcContext;
    arc4_setup(&arcContext, ltKeyValue, ltKeyLen); 
    
    #if DEBUG_ENCRYPT_SESSION==1
    arc4_crypt(&arcContext, sessionKeyLen, sessionKey, encryptedKey);
    #else
    memcpy((char *)encryptedKey, (char *)sessionKey, sessionKeyLen);
    #endif 

    return encryptedKey;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pcbGenSessionKey
 *                                    
 * \par Purpose: Generates a random session key string
 *
 * \retval unsigned char * - A pointer to the session key string generated
 *
 * \param[in]  sessionKeyLen - Pointer to put the length of the session key in
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation.
 *****************************************************************************/

unsigned char *bsp_pcbGenSessionKey(unsigned int *sessionKeyLen) 
{
    int i = 0;
    char possibleChar[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";//-_+=[]{};:\\|'\",.<>?/!@#$%^&*()~`";
    unsigned char *sessionKey = MTAKE(PCB_SESSION_KEY_LENGTH + 1);
    CHKNULL(sessionKey);

    // Randomly select out of the possible char and put in string
    for(i = 0; i < PCB_SESSION_KEY_LENGTH; i++, sessionKey++) 
    {
        *sessionKey = possibleChar[(rand() % (sizeof(possibleChar)-1))];
    }
    // Put on the string terminator (needed for arc4)
    *sessionKey = '\0';
    *sessionKeyLen = PCB_SESSION_KEY_LENGTH + 1;
    
    return (sessionKey-PCB_SESSION_KEY_LENGTH);
}
