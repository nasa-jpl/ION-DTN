/*
 * extbspbab.c
 *
 *  Created on: Jul 5, 2010
 *      Author: birraej1
 */

#include "extbsputil.h"
#include "extbspbab.h"

#if (BAB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/**
 *  \struct BspBabCollaborationBlock
 *  \brief Collaboration object used to carry data shared between BAB instances.
 *
 *  The BAB collaboration block carries meta-data associated with BAB block
 *  processing and is used to facilitate communication between BAB blocks in
 *  the BSP module.
 */

typedef struct
{
	CollabBlockHdr hdr;
	unsigned int correlator;
	unsigned int cipher;
	char cipherKeyName[BSP_KEY_NAME_LEN]; /** Cipherkey name used by this block.*/
	unsigned int rxFlags;        /** RX-side processing flags for this block. */
	int hmacLen;
	char expectedResult[BAB_HMAC_SHA1_RESULT_LEN];
} BspBabCollaborationBlock;

#include "crypto.h"

/*****************************************************************************
 *                     BAB EXTENSIONS INTERFACE FUNCTIONS                    *
 *                                                                           *
 *                                Call Order                                 *
 *                                                                           *
 *       SEND SIDE                                      RECEIVE SIDE         *  
 *                                                                           *
 *   bsp_babOffer (Pre)                                                      *
 *   bsp_babOffer (Post)                                                     *
 *   bsp_babProcessOnDequeue (Pre)                                           *
 *   bsp_babProcessOnDequeue (Post)                                          *
 *   bsp_babProcessOnTransmit (Post)                                         *
 *   bsp_babRelease (Pre)                                                    *
 *   bsp_babRelease (Post)                                                   *
 *                                                  bsp_babAcquire (Pre)     *
 *                                                  bsp_babAcquire (Post)    *
 *                                                  bsp_babCheck (Pre)       *
 *                                                  bsp_babCheck (Post)      *
 *                                                  bsp_babClear (Pre)       *
 *                                                  bsp_babClear (Post)      *
 *                                                                           *
 *****************************************************************************/


/******************************************************************************
 *
 * \par Function Name: bsp_babAcquire
 *
 * \par Purpose: This callback is called when a serialized BAB bundle is
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

int bsp_babAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
   int result = -1;
   BspAbstractSecurityBlock *asb = NULL;

   BAB_DEBUG_PROC("+ bsp_babAcquire(%x, %x)",
                  (unsigned long)blk, (unsigned long)wk);

   CHKERR(blk);
   /* Allocate a scratchpad object to hold a structured view of the block. */
   blk->size = sizeof(BspAbstractSecurityBlock);
   if((blk->object = MTAKE(blk->size)) == NULL)
   {
      BAB_DEBUG_ERR("x bsp_babAcquire:  MTAKE failed on size %d", blk->size);
      blk->size = 0;
      result = -1;
   }
   else
   {
      /* Clear out the block's scratchpad information */
      asb = (BspAbstractSecurityBlock *) blk->object;
      memset((char *) asb,0, blk->size);

      /* Populate the scratchpad object's ASB. */
      result = bsp_deserializeASB(blk, wk, BSP_BAB_TYPE);

      BAB_DEBUG_INFO("i bsp_babAcquire: Deserialize result %d", result);
   }

   BAB_DEBUG_PROC("- bsp_babAcquire -> %d", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_babClear
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

void	bsp_babClear(AcqExtBlock *blk)
{
   BAB_DEBUG_PROC("+ bsp_babClear(%x)", (unsigned long) blk);

   CHKVOID(blk);
   if(blk->size > 0)
   {
      BspAbstractSecurityBlock *asb = (BspAbstractSecurityBlock *) blk->object;
      if(asb->resultLen > 0)
      {
         BAB_DEBUG_INFO("i bsp_babClear: Release result of len %ld",
                        asb->resultLen);
         MRELEASE(asb->resultData);
         asb->resultData = 0;
         asb->resultLen = 0;
      }

      BAB_DEBUG_INFO("i bsp_babClear: Release ASB of len %d", blk->size);

      MRELEASE(blk->object);
      blk->object = NULL;
      blk->size = 0;
   }

   BAB_DEBUG_PROC("- bsp_babClear(%c)", ' ');

   return;
}

int	bsp_babCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
	Sdr	bpSdr = getIonsdr();
	char	*buffer;
	int	result = 0;

	BAB_DEBUG_PROC("+ bsp_babCopy(%x, %x)", (unsigned long) newBlk,
		   (unsigned long) oldBlk);
	CHKERR(newBlk);
	CHKERR(oldBlk);
	if (oldBlk->size == 0)
	{
		newBlk->object = 0;
		newBlk->size = 0;
	}
	else
	{
		buffer = MTAKE(oldBlk->size);
		if (buffer == NULL)
		{
      			BAB_DEBUG_ERR("x bsp_babCopy: Failed to allocate \
buffer of size: %d", oldBlk->size);
			result = -1;
		}
		else
		{
			sdr_read(bpSdr, buffer, oldBlk->object, oldBlk->size);
			newBlk->object = sdr_malloc(bpSdr, oldBlk->size);
			if (newBlk->object == 0)
			{
				BAB_DEBUG_ERR("x bsp_babCopy: Failed to SDR \
allocate object of size: %d", oldBlk->size);
				result = -1;
			}
			else
			{
				sdr_write(bpSdr, newBlk->object, buffer,
						oldBlk->size);
				newBlk->size = oldBlk->size;
			}

			MRELEASE(buffer);
		}
	}

	BAB_DEBUG_PROC("- bsp_babCopy(%c)", ' ');

	return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_babOffer
 *
 * \par Purpose: This callback determines whether a BAB block is necessary for
 *               this particular bundle, based on local security policies.
 *               However, at this point we may not have enough information
 *               (such as EIDs) to query the security policy. Therefore, the
 *               offer callback ALWAYS adds the BAB block.  When we do the
 *               process on dequeue callback we will have enough information
 *               to determine whether or not the BAB extension block should
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

int bsp_babOffer(ExtensionBlock *blk, Bundle *bundle)
{
   Sdr bpSdr = getIonsdr();
   BspAbstractSecurityBlock asb;
   int result = -1;

   BAB_DEBUG_PROC("+ bsp_babOffer(%x, %x)",
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
    * sure that we will be using the BAB, so we don't go through the hassle of
    * allocating objects in the SDR yet, as that would incur a larger performance
    * penalty on bundles that do not use the BAB.
    */
   blk->size = sizeof(BspAbstractSecurityBlock);

   blk->object = sdr_malloc(bpSdr, blk->size);
   if (blk->object == 0)
   {
      BAB_DEBUG_ERR("x bsp_babOffer: Failed to SDR allocate of size: %d",
            blk->size);
      result = -1;
   }
   else
   {
      sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);
      result = 0;
   }

   BAB_DEBUG_PROC("- bsp_babOffer -> %d", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_babPostCheck
 *
 * \par Purpose: This callback checks a post-payload block, upon bundle receipt
 *               to determine whether the block should be considered authentic.
 *               For a BAB, this implies that the security result encoded in
 *               this block is the correct hash for the bundle.
 *
 * \retval int 0 - The block is corrupt.
 *             1 - The block check was inconclusive.
 *             2 - The block check failed.
 *             3 - The block check succeeed.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *****************************************************************************/

static int	bsp_babPostCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
   BspAbstractSecurityBlock *asb = NULL;
   BspBabCollaborationBlock *collabBlk;
   LystElt collabBlkAddr;
   int retval = 0;
   unsigned char   *digest;
   unsigned int   digestLen;
   int         cmpResult;

   BAB_DEBUG_PROC("+ bsp_babPostCheck(%x, %x)",
                  (unsigned long) blk, (unsigned long) wk);

   /***************************************************************************
    *                             Sanity Checks                               *
    ***************************************************************************/

   if((blk == NULL) || (blk->object == NULL) || (wk == NULL))
   {
      BAB_DEBUG_ERR("x bspBabPostCheck:  Blocks are NULL. %x",
                    (unsigned long) blk);
      BAB_DEBUG_PROC("- bsp_babPostCheck --> %d", -1);
      return -1;
   }

   if (wk->authentic)
   {
      BAB_DEBUG_INFO("i Authenticity asserted. Accept on faith.", NULL);
      discardExtensionBlock(blk);
      BAB_DEBUG_PROC("- bsp_babPostCheck --> %d", 0);
      return 1;    /*   Authenticity asserted; bail.   */
   }

   /* Grab ASB and collab block. */

   asb = (BspAbstractSecurityBlock *) blk->object;
   collabBlkAddr = findAcqCollabBlock(wk, COR_BAB_TYPE, asb->correlator);
   if(collabBlkAddr == 0)
   {
      BAB_DEBUG_ERR("x bspBabPostCheck:  Can't find collab blk for corr %d.",
                  asb->correlator);
      discardExtensionBlock(blk);
      BAB_DEBUG_PROC("- bsp_babPostCheck --> %d", 0);
      return 1;		/*	No valid pre-payload BAB: ignore.	*/
   }
   collabBlk = (BspBabCollaborationBlock *) lyst_data(collabBlkAddr);

   getBspItem(BSP_CSPARM_INT_SIG, asb->resultData, asb->resultLen, &digest,
             &digestLen);

   retval = 2;
   /* The post-payload BAB block *must* have a security result. */
   if((asb->cipherFlags & BSP_ASB_RES) == 0)
   {
      BAB_DEBUG_ERR("x bspBabPostCheck:  No security results. Flags are %ld",
                   asb->cipherFlags);
   }

   /* Make sure that we found the correlated pre-payload block. */
   else if((collabBlk->rxFlags & BSP_BABSCRATCH_RXFLAG_CORR) == 0)
   {
      BAB_DEBUG_ERR("x bsp_babPostCheck: No pre-payload block found.%c",' ');
   }

   else if (collabBlk->hmacLen == 0)
   {
      BAB_DEBUG_ERR("x bsp_babPostCheck: SHA1 result not computed %s",
                collabBlk->cipherKeyName);
   }
   else if (digest == NULL || digestLen != BAB_HMAC_SHA1_RESULT_LEN)
   {
      BAB_DEBUG_ERR("x bsp_babPostCheck: no integrity signature: security \
result len %lu data %x", asb->resultLen, (unsigned long) asb->resultData);
   }
   else
   {
      /* Compare expected result to the computed digest. */
      cmpResult = memcmp(collabBlk->expectedResult, digest, digestLen);
      if(cmpResult == 0)
      {
         retval = 3;
      }
      else
      {
         BAB_DEBUG_ERR("x bsp_babPostCheck: memcmp failed: %d", cmpResult);
         retval = 2;
      }
   }

   /* We are done with this block. */
   discardExtensionBlock(blk);
   BAB_DEBUG_PROC("- bsp_babPostCheck(%d)", retval);

   return retval;
}

/******************************************************************************
 *
 * \par Function Name: bsp_babPostProcessOnDequeue
 *
 * \par Purpose: This callback constructs the post-payload BAB block for
 *               inclusion in a bundle right before the bundle is transmitted.
 *
 * \retval int 0 - The post-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  post_blk  The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 *
 * \par Notes:
 *      1. No other blocks will be added to the bundle, and no existing blocks
 *         in the bundle will be modified.  This must be the last block
 *         modification performed on the bundle before it is transmitted.
 *****************************************************************************/
static int	bsp_babPostProcessOnDequeue(ExtensionBlock *post_blk,
			Bundle *bundle, void *parm)
{
   DequeueContext   *ctxt = (DequeueContext *) parm;
   BspAbstractSecurityBlock asb;
   BspSecurityInfo secInfo;
   BspBabCollaborationBlock collabBlk;
   unsigned char *raw_asb = NULL;
   char *srcNode = NULL, *destNode = NULL;
   int result = 0;
   Sdr bpSdr = getIonsdr();
   Object collabAddr = 0;
   Sdnv digestSdnv;

   BAB_DEBUG_PROC("+ bsp_babPostProcessOnDequeue(%x, %x, %x)",
                  (unsigned long) post_blk,
                  (unsigned long) bundle,
                  (unsigned long) ctxt);

   /*
    * Sanity Check...
    */
   if (bundle == NULL   ||
      parm == NULL     ||
      post_blk == NULL ||
       post_blk->size != sizeof(BspAbstractSecurityBlock))
   {
     BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Bundle or ASB were not \
as expected.", NULL);
     BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", -1);
     return -1;
   }

   /*
    * Grab the ASB. The Dequeue function will write to it to store info used
    * when processing this block during the Process On Transmit callback.
    */
   sdr_read(bpSdr, (char *) &asb, post_blk->object, post_blk->size);

   /*
    * Grab the collaboration block associated with BABs for this bundle, if
    * such a block exists.
    */
   collabAddr = findCollaborationBlock(bundle, COR_BAB_TYPE, BAB_CORRELATOR);

   /***************************************************************************
    *                VERIFY WE CAN/SHOULD ADD POST-PAYLOAD BLOCK              *
    ***************************************************************************/

   // Take care of our addressing.. fill out eidRefs, sec Src, sec Dest (if applicable)
   // fill out our srcNode, destNode eid strings for sure
   srcNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
   destNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
   if(setSecPointsTrans(post_blk, bundle, &asb, NULL, 0, ctxt, srcNode, destNode) != 0)
   {
       MRELEASE(srcNode); MRELEASE(destNode);
       BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: setSecPointsTrans failed.", NULL);
       BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", -1);
       return -1;
   }
   else if(srcNode == NULL || destNode == NULL)
   {
       MRELEASE(srcNode); MRELEASE(destNode);
       BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: a node address is unexpectedly null! \
                      srcNode:%s, destNode:%s", srcNode, destNode);
       BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", -1);
       return -1;
   }

   /*
    * Check that we will need a BAB. If we have no key, then there is either
    * no rule or no key associated with BABs over this hop, so we don't need
    * a BAB block in this bundle.  This is not an error UNLESS we have already
    * added a pre-payload block to the bundle already, in which case, we have
    * somehow lost data coherency in the bundle.
    */

   bsp_getSecurityInfo(bundle, BSP_BAB_TYPE, 0,
                       srcNode,
                       destNode, &secInfo);
   MRELEASE(srcNode); MRELEASE(destNode);

   if (secInfo.cipherKeyName[0] == '\0')
   {
      int result = 0;
      if(collabAddr != 0)
      {
         BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: No key found for post- \
payload block, but found collab struct.", NULL);
         result = -1;
      }
      else
      {
         BAB_DEBUG_INFO("i bsp_babPostProcessOnDequeue: No key found for BAB. \
Not using BAB blocks for this bundle.", NULL);

      }

     scratchExtensionBlock(post_blk);
     BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", result);
     return result;
   }
   /*
    * Otherwise, if we want a BAB and found no collab block from a pre-payload
    * block, something has gone wrong.
    */
   else if(collabAddr == 0)
   {
      BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Trying to insert post-  \
payload block without collab struct.", NULL);
      scratchExtensionBlock(post_blk);
      BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", 0);
      return 0;	/*	No valid pre-payload BAB, so don't add post-.	*/
   }

   /* Grab the collaboration block structure to verify that we can continue */
   sdr_read(bpSdr, (char *) &collabBlk, collabAddr, sizeof(BspBabCollaborationBlock));

   if((collabBlk.hdr.type != COR_BAB_TYPE) ||
     (collabBlk.correlator != BAB_CORRELATOR) ||
     (collabBlk.cipher != BSP_CSTYPE_BAB_HMAC) ||
     (collabBlk.rxFlags != 0) ||
     (collabBlk.hmacLen != 0) ||
     (collabBlk.expectedResult[0] != '\0'))
   {
      BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Data mismatch in  \
collab struct.", NULL);
      scratchExtensionBlock(post_blk);
      BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", -1);
      return -1;
   }


   /***************************************************************************
    *                      Populate Post-Payload ASB                          *
    ***************************************************************************/

   /* post-payload BAB must be the last one in the bundle. */
   post_blk->blkProcFlags |= BLK_IS_LAST;
   asb.cipher = BSP_CSTYPE_BAB_HMAC;
   asb.cipherFlags = BSP_ASB_CORR | BSP_ASB_RES;
   asb.correlator = BAB_CORRELATOR;
   // Encode expected digest length into sdnv
   encodeSdnv(&digestSdnv, BAB_HMAC_SHA1_RESULT_LEN); 
   asb.resultLen = BAB_HMAC_SHA1_RESULT_LEN + 1 + digestSdnv.length;

   /*
    * Serialize the block.  This will do everything except populate
    * the security result data.
    */
   raw_asb = bsp_serializeASB(&(post_blk->dataLength), &(asb));
   if((raw_asb == NULL) || (post_blk->dataLength == 0))
   {
      BAB_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Serialization failed.\
raw_asb = %x", (unsigned long) raw_asb);
      scratchExtensionBlock(post_blk);
      result = -1;
   }
   else
   {
      /* Store the serialized ASB in the bytes array */
      result = serializeExtBlk(post_blk, NULL, (char *) raw_asb);

      /* Store the updated ASB for this block */
      sdr_write(bpSdr,
            post_blk->object,
            (char *) &asb,
            post_blk->size);
      MRELEASE(raw_asb);

      istrcpy(collabBlk.cipherKeyName, secInfo.cipherKeyName, BSP_KEY_NAME_LEN);

      updateCollaborationBlock(collabAddr, (CollabBlockHdr *) &collabBlk);
   }

   BAB_DEBUG_PROC("- bsp_babPostProcessOnDequeue(%d)", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_babPostProcessOnTransmit
 *
 * \par Purpose: This callback constructs the security result to be associated
 *               with the post-payload BAB block for a given bundle.  The
 *               security result cannot be calculated until the entire bundle
 *               has been serialized and is ready to transmit.  This callback
 *               will use the post-payload BAB block serialized from the
 *               bsp_babProcessOnDequeue function to calculate the security
 *               result, and will write a new post-payload BAB block in its
 *               place when the security result has been calculated.
 *
 * \retval int 0 - The post-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk     The block whose abstract security block structure
 *                         will be populated and then serialized into the
 *                         bundle.
 * \param[in]      bundle  The bundle holding the block.
 *
 * \par Notes:
 *      1. We assume that the post-payload BAB block is the last block of the
 *         bundle.
 *****************************************************************************/

static int	bsp_babPostProcessOnTransmit(ExtensionBlock *blk,
			Bundle *bundle, void *ctxt)
{
   Sdr bpSdr = getIonsdr();
   unsigned int rawBundleLength = 0;
   BspAbstractSecurityBlock asb;
   BspBabCollaborationBlock collabBlk;
   unsigned char   *digest;
   unsigned int   digestLen;
   unsigned char *raw_asb = NULL;
   int result = 0;
   Object collabAddr = 0;
   char *temp = NULL;
   char *keyValue = NULL;
   int keyLen = 0;
   Sdnv digestSdnv;
   unsigned int digestOffset = 0;

   BAB_DEBUG_PROC("+ bsp_babPostProcessOnTransmit: %x, %x, %x",
   (unsigned long) blk, (unsigned long) bundle,(unsigned long) ctxt);

   memset(&asb,0,sizeof(BspAbstractSecurityBlock));

   if((blk == NULL) || (bundle == NULL))
   {
      BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Bad Parms: Bundle %x",
                    (unsigned long) bundle);
      BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", -1);
      return -1;
   }

   /* Read in the abstract security block */
   sdr_read(bpSdr, (char *) &asb, blk->object, blk->size);

   /*
     * Grab the collaboration block associated with BABs for this bundle, if
     * such a block exists.
     */
    collabAddr = findCollaborationBlock(bundle, COR_BAB_TYPE, BAB_CORRELATOR);
    if(collabAddr == 0)
    {
       BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: No collab block found.",
                   NULL);
        BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", 0);
        return 0;	/*	No valid pre-payload block, so give up.	*/
    }

    /* Grab the collaboration block structure to verify that we can continue */
    sdr_read(bpSdr, (char *) &collabBlk, collabAddr, sizeof(BspBabCollaborationBlock));

    /* Grab the key. */
    if((keyValue = (char *) bsp_retrieveKey(&keyLen, collabBlk.cipherKeyName)) == NULL)
    {
   BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Can't retrieve key %s.",
                 collabBlk.cipherKeyName);

        BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", -1);
        return -1;
   }

   /*
    * Grab the serialized bundle. It lives in bundle.payload.
    * Get it ready to serialize, and grab the bundle length.
    */
   rawBundleLength = zco_length(bpSdr, bundle->payload.content) - asb.resultLen;
   digest = bsp_babGetSecResult(bundle->payload.content, rawBundleLength, keyValue, keyLen, &digestLen);
   MRELEASE(keyValue);

   // Encode real digest length into sdnv
   encodeSdnv(&digestSdnv, digestLen); 
   digestOffset = 1 + digestSdnv.length;

   /* Let's see if we got a good result */
   if((digest != NULL) && (digestLen == BAB_HMAC_SHA1_RESULT_LEN))   
   {
      collabBlk.hmacLen = digestLen;
      memcpy(collabBlk.expectedResult, digest, digestLen);
   }
   else
   {
      if(digest != NULL)
      {
        MRELEASE(digest);
      }
      BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Bad hash. hashData is 0x%x and length is %d.",
                    digest, digestLen);
      BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit--> 0", NULL);
      return 0;
   }

   /* 
    * In this next block of code we do some surgery on the bundle-to-be-transmitted.
    * We update our abstract security block to include the new security result,
    * then we re-serialize this block and replace the "old" version of this serialized
    * block with the new serialized contents in the serialized version of the bundle
    * about to be transmitted.
    */
   if((asb.resultData = MTAKE(digestLen + digestOffset)) == NULL)
   {
    BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Can't allocate \
 ASB result, len %ld.", digestLen + digestOffset);
    MRELEASE(digest);
       
       BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", -1);

       return -1;
   }

   /* Construct the new security result. */
   *(asb.resultData) = BSP_CSPARM_INT_SIG;
   memcpy(asb.resultData + 1, digestSdnv.text, digestSdnv.length);
   memcpy(asb.resultData + digestOffset, digest, digestLen);
   MRELEASE(digest);

   /* Locally serialize the ASB. */
   raw_asb = bsp_serializeASB(&(blk->dataLength), &(asb));
   if((raw_asb == NULL) || (blk->dataLength == 0))
   {
      BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Serialization \
failed. raw_asb = %x", (unsigned long) raw_asb);
   
      MRELEASE(asb.resultData);
      asb.resultData = NULL;
      asb.resultLen = 0;

      BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", -1);

      return -1;
   }

   /* Serialize the block into the extension block bytes array. */
   if((result = serializeExtBlk(blk, NULL, (char *) raw_asb)) < 0)
   {
      BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Failed \
serializing block. Result is %d", result);

      MRELEASE(raw_asb);
      MRELEASE(asb.resultData);
      asb.resultData = NULL;
      asb.resultLen = 0;

      BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", result);

      return result;
   }

   MRELEASE(raw_asb);

   /*
    * Put the new block into place.  First, grab the text
    * of the newly serialized block, from bytes array.  The
    * serializeExtBlk function dropped the serialized length
    * of the bytes array into blk.
    */
   
   if((temp = MTAKE(blk->length)) == NULL)
   {
           BAB_DEBUG_ERR("x bsp_babPostProcessOnTransmit: \
Allocation of %d bytes failed.", blk->length);
           MRELEASE(asb.resultData);
           asb.resultData = NULL;
           asb.resultLen = 0;

           BAB_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", -1);

           return -1;
   }

   sdr_read(bpSdr, temp, blk->bytes, blk->length);
   BAB_DEBUG_INFO("i bsp_babPostProcessOnTransmit: New trailer has length %d", blk->length);
   /* Throw away *old* post-payload block. */
   zco_discard_last_trailer(bpSdr, bundle->payload.content);
 
  /* Add in new post-payload block */
   oK(zco_append_trailer(bpSdr, bundle->payload.content, (char *) temp, blk->length));

   MRELEASE(temp);

   /* Done with the abstract security block. */
   MRELEASE(asb.resultData);
   asb.resultData = NULL;
   asb.resultLen = 0;

   BAB_DEBUG_INFO("- bsp_babPostProcessOnTransmit --> %d", result);
   return result;
}

int	bsp_babProcessOnTransmit(ExtensionBlock *blk, Bundle *bundle,
		void *ctxt)
{
	if (blk->occurrence == 1)
	{
		return bsp_babPostProcessOnTransmit(blk, bundle, ctxt);
	}

	return 0;
}

/******************************************************************************
 *
 * \par Function Name: bsp_babPreCheck
 *
 * \par Purpose: This callback checks a pre-payload block, upon bundle receipt
 *               to determine whether the block should be considered authentic.
 *               For a BAB, this is really just a sanity check on the block,
 *               making sure that the block fields are consistent.
 *       Also, at this point the entire bundle has been received
 *       into a zero-copy object, so we can compute the security
 *       result for the bundle and pass it on to the post-payload
 *       BAB check function.
 *
 * \retval int 0 - The block check was inconclusive
 *             1 - The block check failed.
 *             2 - The block check succeeed.
 *            -1 - There was a system error.
 *
 * \param[in]  pre_blk  The acquisition block being checked.
 * \param[in]  wk       The working area holding other acquisition blocks and
 *                      the rest of the received bundle.
 *
 * \par Notes:
 *      1. We assume that there is only 1 BAB block pair in a bundle.
 *****************************************************************************/

static int	bsp_babPreCheck(AcqExtBlock *pre_blk, AcqWorkArea *wk)
{
   BspAbstractSecurityBlock *pre_asb = NULL;
   BspBabCollaborationBlock collabBlk;
   BspSecurityInfo secInfo;

   Sdr         bpSdr = getIonsdr();
   int         resultLen = BAB_HMAC_SHA1_RESULT_LEN + 2;   /*   Item.   */
   unsigned int   rawBundleLen;
   int         lengthToHash;
   char         *keyValueBuffer;
   int         keyLen;
   int retval = 0;
   char *srcNode, *destNode;
   unsigned char *digest;
   unsigned int digestLen;

   BAB_DEBUG_PROC("+ bsp_babPreCheck(%x,%x)",
                  (unsigned long) pre_blk, (unsigned long) wk);

   memset(&collabBlk,0,sizeof(BspBabCollaborationBlock));
   collabBlk.hdr.type = COR_BAB_TYPE;
   collabBlk.hdr.id   = BAB_CORRELATOR;
   collabBlk.hdr.size = sizeof(BspBabCollaborationBlock);

   /***************************************************************************
    *                             Sanity Checks                               *
    ***************************************************************************/

   CHKERR(wk);
   /* We need blocks! */
   if((pre_blk == NULL) || (pre_blk->object == NULL))
   {
      BAB_DEBUG_ERR("x bsp_babPreCheck:  No blocks. pre_blk is %x",
                    (unsigned long) pre_blk);
      BAB_DEBUG_PROC("- bsp_babPreCheck --> %d", -1);
      return -1;
   }

   pre_asb = (BspAbstractSecurityBlock *) pre_blk->object;

   /*   The incoming serialized bundle, unaltered, is now
    *   in the bundle payload content of the work area; its
    *   BP header and trailer have not yet been stripped off.
    *   So we can now compute the correct security result
    *   for this bundle's BABs.
    */

   rawBundleLen = zco_length(bpSdr, wk->bundle.payload.content);
   lengthToHash = rawBundleLen - resultLen;
   if (lengthToHash < 0)
   {
      BAB_DEBUG_ERR("x bsp_babPreCheck: Can't hash %d bytes", lengthToHash);
      discardExtensionBlock(pre_blk);
      BAB_DEBUG_PROC("- bsp_babPreCheck --> 0", NULL);
      return 0;
   }

   BAB_DEBUG_INFO("i bsp_babPreCheck: len %d", resultLen);
   srcNode = NULL;
   destNode = NULL;
   oK(printEid(&pre_asb->secSrc, wk->dictionary, &srcNode));
   oK(printEid(&pre_asb->secDest, wk->dictionary, &destNode));
   if (srcNode == NULL || destNode == NULL)
   {
       if (srcNode) MRELEASE(srcNode);
       if (destNode) MRELEASE(destNode);
       BAB_DEBUG_ERR("x bsp_babPreCheck: Error retrieving src and dest nodes to find key.", NULL);
       BAB_DEBUG_PROC("- bsp_babPreCheck --> %d", -1);
       return -1;
   } 

   bsp_getSecurityInfo(&(wk->bundle), BSP_BAB_TYPE, 0,
                       srcNode,
                       destNode,
                       &secInfo);
   MRELEASE(srcNode); MRELEASE(destNode);

   if (secInfo.cipherKeyName[0] == '\0')
   {
      /*   No rule, or no key.               */
      BAB_DEBUG_INFO("i bsp_babPreCheck: No rule/key for BAB.", NULL);
      discardExtensionBlock(pre_blk);
      BAB_DEBUG_PROC("- bsp_babPreCheck --> 0", NULL);
      return 0;   /*   No hash computation.         */
   }

   keyValueBuffer = (char *) bsp_retrieveKey(&keyLen, secInfo.cipherKeyName);
   if (keyValueBuffer == NULL)
   {
      BAB_DEBUG_ERR("x bsp_babPreAcquire: Can't retrieve key %s for EID %s",
                 secInfo.cipherKeyName, wk->senderEid);

      /*   Note that collabBlk.hmacLen remains zero.  This is
       *   the indication, during the post-payload BAB check,
       *   that the key was not retrieved.
       */

      discardExtensionBlock(pre_blk);
      BAB_DEBUG_PROC("- bsp_babPreCheck --> 0", NULL);
      return 0;
   }

   /* Read data in chunks and hash. */
   digest = bsp_babGetSecResult(wk->bundle.payload.content, lengthToHash, keyValueBuffer, keyLen, &digestLen);

   MRELEASE(keyValueBuffer);

   /* Let's see if we got a good result */
   if((digest != NULL) && (digestLen == BAB_HMAC_SHA1_RESULT_LEN))   
   {
      collabBlk.hmacLen = digestLen;
      memcpy(collabBlk.expectedResult, digest, digestLen);
   }
   else
   {
      if(digest != NULL)
      {
        MRELEASE(digest);
      }
      BAB_DEBUG_ERR("x bsp_babPreCheck: Bad hash. digest is 0x%x and length is %d.",
                    digest, digestLen);
      discardExtensionBlock(pre_blk);
      BAB_DEBUG_PROC("- bsp_babPreCheck --> 0", NULL);
      return 0;
   }

   /*
    *  Security result for this bundle has now been calculated and
    *  can be used for post-payload BAB check.
    */

   /* The pre-payload block must have a correlator and no result. */
   if(((pre_asb->cipherFlags & BSP_ASB_CORR) == 0) ||
       ((pre_asb->cipherFlags & BSP_ASB_RES) != 0))
   {
      BAB_DEBUG_ERR("x bsp_babPreCheck: Bad Flags! Correlator missing and/or\
results present. Flags. %ld", pre_asb->cipherFlags);

      retval = 1;
   }

   /* Pre-block looks good.  Let's find the post-payload block. */
   else
   {
      AcqExtBlock *post_blk = bsp_findAcqExtBlk(wk,
                                                POST_PAYLOAD,
                                                BSP_BAB_TYPE);
      if(post_blk == NULL)
      {
         BAB_DEBUG_ERR("x bsp_babPreCheck:  Could not find post-payload \
BAB block! %c",  ' ');
         retval = 1;
      }
      else
      {
         collabBlk.correlator = pre_asb->correlator;
         collabBlk.rxFlags |=  BSP_BABSCRATCH_RXFLAG_CORR;
         memcpy(&(collabBlk.cipherKeyName), &(secInfo.cipherKeyName),
               BSP_KEY_NAME_LEN);

         addAcqCollabBlock(wk, (CollabBlockHdr *) &collabBlk);
         retval = 0;
      }
   }

   /*
    * We are done with the pre-block. Either the post-block was not found, or
    * we did find it and told it that it did, at one point, have a pre-block.
    * Either way, time to drop the pre-payload block.
    */

   MRELEASE(digest);
   discardExtensionBlock(pre_blk);
   BAB_DEBUG_PROC("- bsp_babPreCheck(%d)", retval);

   return retval;
}

int bsp_babCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
	if (blk->occurrence == 1)
	{
		return bsp_babPostCheck(blk, wk);
	}

	return bsp_babPreCheck(blk, wk);
}

/******************************************************************************
 *
 * \par Function Name: bsp_babPreProcessOnDequeue
 *
 * \par Purpose: This callback constructs the pre-payload BAB block for
 *               inclusion in a bundle right before the bundle is transmitted.
 *               This means constructing the abstract security block and
 *               serializing it into the block's bytes array.
 *
 * \retval int 0 - The pre-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk     The block whose abstract security block structure will
 *                         be populated and then serialized into the block's
 *                         bytes array.
 * \param[in]      bundle  The bundle holding the block.
 *
 * \par Notes:
 *****************************************************************************/

static int	bsp_babPreProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
			void *parm)
{
   DequeueContext   *ctxt = (DequeueContext *) parm;
   Lyst eidRefs = NULL;
   BspAbstractSecurityBlock asb;
   BspSecurityInfo secInfo;
   int result = 0;
   unsigned char *raw_asb = NULL;
   char *srcNode = NULL, *destNode = NULL;
   Sdr bpSdr = getIonsdr();

   BAB_DEBUG_PROC("+ bsp_babPreProcessOnDequeue(%x, %x, %x",
                  (unsigned long) blk,
                  (unsigned long) bundle,
                  (unsigned long) ctxt);

   if((bundle == NULL) ||
     (parm == NULL)   ||
     (blk == NULL)    ||
        (blk->size != sizeof(BspAbstractSecurityBlock)))
   {
      BAB_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Bundle or ASB were not as \
expected.", NULL);

      BAB_DEBUG_PROC("- bsp_babPreProcessOnDequeue --> %d", -1);
      return -1;
   }

   /*
    * Grab the scratchpad object and now get security information for
    * the final selected security destination.      //   SB 3 Aug 2009
    */

   sdr_read(bpSdr, (char *) &asb, blk->object, blk->size);

   srcNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
   destNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
   if(setSecPointsTrans(blk, bundle, &asb, &eidRefs, BSP_BAB_TYPE, ctxt, srcNode, destNode) != 0)
   {
       MRELEASE(srcNode); MRELEASE(destNode);
       if (eidRefs != NULL) lyst_destroy(eidRefs);
       BAB_DEBUG_ERR("x bsp_babPreProcessOnDequeue: setSecPointsTrans failed.", NULL);
       BAB_DEBUG_PROC("- bsp_babPreProcessOnDequeue --> %d", -1);
       return -1; 
   }
   else if(srcNode == NULL || destNode == NULL)
   {
       MRELEASE(srcNode); MRELEASE(destNode);
       if (eidRefs != NULL) lyst_destroy(eidRefs);
       BAB_DEBUG_ERR("x bsp_babPreProcessOnDequeue: a node address is unexpectedly null! \
                      srcNode:%s, destNode:%s", srcNode, destNode);
       BAB_DEBUG_PROC("- bsp_babPreProcessOnDequeue --> %d", -1);
       return -1; 
   }
   
   bsp_getSecurityInfo(bundle, BSP_BAB_TYPE, 0,
		    srcNode,
		    destNode, &secInfo);
   MRELEASE(srcNode); MRELEASE(destNode);

   /* If we have no rule, or no key, then we are not going to add BAB blocks
    * to this bundle. This is not an error, but a security policy decision.
    */
   if (secInfo.cipherKeyName[0] == '\0')
   {
     BAB_DEBUG_INFO("i bsp_babPreProcessOnDequeue: No key found for BAB. Not \
 using BAB blocks for this bundle.", NULL);

     if (eidRefs != NULL) lyst_destroy(eidRefs);
     scratchExtensionBlock(blk);
     BAB_DEBUG_PROC("- bsp_babPreProcessOnDequeue --> %d", 0);
     return 0;
   }

   if(result == 0)
   {
      /* We always use the HMAC-SHA1 cipher, for now. */
      asb.cipher = BSP_CSTYPE_BAB_HMAC;

      /*
       * All BAB blocks that we create will be correlated. Since each bundle
       * can only have 1 BAB set of blocks in it, we do not need to worry about
       * ensuring that this correlator identifier is unique within the bundle.
       */
      asb.cipherFlags |= BSP_ASB_CORR;

      asb.correlator = BAB_CORRELATOR;

      /* Serialize the Abstract Security Block. */
      raw_asb = bsp_serializeASB(&(blk->dataLength), &(asb));

      if((raw_asb == NULL) || (blk->dataLength == 0))
      {
         BAB_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Unable to serialize \
ASB. blk->dataLength = %d", blk->dataLength);
         result = -1;
      }
      else
      {
        BspBabCollaborationBlock collab;

         /*
          * store this block in its bytes array. This is necessary as the
          * block will be added to the bundle when this function exits.
          */
         result = serializeExtBlk(blk, eidRefs, (char *) raw_asb);

         MRELEASE(raw_asb);

         /*
          * Store collaboration information.  This is used by the post-payload
          * BAB block in the bundle to verify that the blocks agree.
          */
         collab.hdr.type = COR_BAB_TYPE;
         collab.hdr.id = asb.correlator;
         collab.hdr.size = sizeof(BspBabCollaborationBlock);
         collab.correlator = asb.correlator;
         collab.cipher = asb.cipher;
         istrcpy(collab.cipherKeyName, secInfo.cipherKeyName, BSP_KEY_NAME_LEN);
         collab.rxFlags = 0;
         collab.hmacLen = 0;
         collab.expectedResult[0] = '\0';

         if (addCollaborationBlock(bundle, (CollabBlockHdr *) &collab) < 0)
	 {
         	BAB_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Can't add \
collaboration block.", NULL);
         	result = -1;
	 }
     }
   }

   /* Clear the eid references if we used them. */
   if(eidRefs != NULL)
   {
      lyst_destroy(eidRefs);
   }

   BAB_DEBUG_PROC("- bsp_babPreProcessOnDequeue(%d)", result);

   return result;
}

int bsp_babProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *parm)
{
	if (blk->occurrence == 1)
	{
		return bsp_babPostProcessOnDequeue(blk, bundle, parm);
	}

	return bsp_babPreProcessOnDequeue(blk, bundle, parm);
}

/******************************************************************************
 *
 * \par Function Name: bsp_babRelease
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
 *      1. This same function is used for pre- and post- payload blocks.
 *      2. It is OK to simply free the entire scratchpad object, without
 *         explicitly freeing the result data part of the ASB.
 *         bsp_babPreProcessOnDequeue does not have a security result, so there
 *         is nothing to free in that case. bsp_babPostProcessOnDequeue
 *         calculates the security result and inserts it directly into the
 *         bytes array of the block, which is freed by the main ION library.
 *         In neither case is the resultData portion of the ASB structure in the
 *         scratchpad allocated.
 *
 *****************************************************************************/

void    bsp_babRelease(ExtensionBlock *blk)
{

   BAB_DEBUG_PROC("+ bsp_babRelease(%x)", (unsigned long) blk);

   CHKVOID(blk);
   if(blk->size > 0)
   {
      sdr_free(getIonsdr(), blk->object);
   }

   BAB_DEBUG_PROC("- bsp_babRelease(%c)", ' ');

   return;
}


/*****************************************************************************
 *                            BAB HELPER FUNCTIONS                           *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_babGetSecResult
 *
 * \par Purpose: Calculates a security result given a key name and a
 *               set of serialized data.
 *
 * \retval unsigned char * - The security result.
 *
 * \param[in]  rawBundle     - The serialized bundle that we wish to hash.
 * \param[in]  rawBundleLen  - Length of the serialized bundle.
 * \param[in]  cipherKeyName - Name of the key to use.
 * \param[out] hashLen       - Lenght of the security result.
 *
 * \par Notes:
 *      1. Currently, only the HMAC-SHA1 is implemented.
 *      2. This function should be updated for a more parameterized security
 *         result based on different ciphersuites.
 *****************************************************************************/

unsigned char *bsp_babGetSecResult(Object dataObj,
                                   unsigned int dataLen,
                                   char *keyValue,
                    unsigned int keyLen,
                                   unsigned int *hashLen)
{
   Sdr bpSdr = getIonsdr();
   unsigned char *hashData = NULL;
   char *dataBuffer;
   int i = 0;
   ZcoReader dataReader;
   char *authContext;
   int authCtxLen = 0;
   unsigned int bytesRemaining = 0;
   unsigned int chunkSize = BSP_BAB_BLOCKING_SIZE;
   unsigned int bytesRetrieved = 0;

   BAB_DEBUG_INFO("+ bsp_babGetSecResult(0x%x, %ld, %s %d, 0x%x)",
                  (unsigned long) dataObj,
                  dataLen,
                  keyValue,
                  keyLen,
                  (unsigned long) hashLen);

   CHKNULL(keyValue);
   CHKNULL(hashLen);

   /*	Allocate a data buffer.	*/

   dataBuffer = MTAKE(BSP_BAB_BLOCKING_SIZE);
   CHKNULL(dataBuffer);

   /* Grab a context for the hmac. A local context allows re-entrant calls to the
      HMAC libraries. */
   if((authCtxLen = hmac_sha1_context_length()) <= 0)
   {
      BAB_DEBUG_ERR("x bsp_babGetSecResult: Bad context length (%d)",
         authCtxLen);
         *hashLen = 0;
      BAB_DEBUG_PROC("- bsp_babGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
   }

   BAB_DEBUG_INFO("i bsp_babGetSecResult: context length is %d", authCtxLen);

   if((authContext = MTAKE(authCtxLen)) == NULL)
   {
      BAB_DEBUG_ERR("x bsp_babGetSecResult: Can't allocate %ld bytes",
         authCtxLen);
         *hashLen = 0;
      BAB_DEBUG_PROC("- bsp_babGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
   }

   /*   Prepare the data for processing. */
   CHKNULL(sdr_begin_xn(bpSdr));
   zco_start_transmitting(dataObj, &dataReader);
   
   hmac_sha1_init(authContext, (unsigned char *)keyValue, keyLen);
   bytesRemaining = dataLen;

   BAB_DEBUG_INFO("i bsp_babGetSecResult: size is %d", bytesRemaining);
   BAB_DEBUG_INFO("i bsp_babGetSecResult: Key value is %s and key length is %d", keyValue, keyLen);

   while(bytesRemaining > 0)
   {

     /* See how many bytes we wish to read in at this time. */
     if(bytesRemaining < chunkSize)
     {
       chunkSize = bytesRemaining;
     }

     /* Read in the appropriate number of bytes. */
     bytesRetrieved = zco_transmit(bpSdr, &dataReader, chunkSize, dataBuffer);

     if(bytesRetrieved != chunkSize)
     {
      BAB_DEBUG_ERR("x bsp_babGetSecResult: Read %d bytes, but expected %d.",
         bytesRetrieved, chunkSize);

      MRELEASE(authContext);
      oK(sdr_end_xn(bpSdr));
      *hashLen = 0;
      BAB_DEBUG_PROC("- bsp_babGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
     }

     /* Print out debugging information, if requested */
     for (i = 0; i < chunkSize; i++)
     {
      //BAB_DEBUG_INFO("Byte %d is %x", i+(dataLen-bytesRemaining), dataBuffer[i]);
     }

     /* Add the data to the hmac_sha1 */
     hmac_sha1_update(authContext, (unsigned char *)dataBuffer, chunkSize);
     bytesRemaining -= bytesRetrieved;
   }

   /* This will store the hash result. */
   if((hashData = MTAKE(BAB_HMAC_SHA1_RESULT_LEN)) == NULL)
   {
      BAB_DEBUG_ERR("x bsp_babGetSecResult: Failed allocating %d bytes.",
                     BAB_HMAC_SHA1_RESULT_LEN);

   MRELEASE(authContext);
      oK(sdr_end_xn(bpSdr));
      *hashLen = 0;
   BAB_DEBUG_PROC("- bsp_babGetSecResult--> NULL", NULL);
   MRELEASE(dataBuffer);
   return NULL;
   }

   BAB_DEBUG_INFO("i bsp_babGetSecResult: allocated hash data.",NULL);

   /* Calculate the hash result. */
   hmac_sha1_final(authContext, hashData, BAB_HMAC_SHA1_RESULT_LEN);
   hmac_sha1_reset(authContext);

   MRELEASE(authContext);
   if ((i = sdr_end_xn(bpSdr)) < 0)
   {
      BAB_DEBUG_ERR("x bsp_babGetSecResult: Failed closing transaction. Result is %d.", i);

      MRELEASE(hashData);
      *hashLen = 0;
   BAB_DEBUG_PROC("- bsp_babGetSecResult--> NULL", NULL);
   MRELEASE(dataBuffer);
   return NULL;
   }

   *hashLen = BAB_HMAC_SHA1_RESULT_LEN;

   for(i = 0; i < BAB_HMAC_SHA1_RESULT_LEN; i++)
   {
      BAB_DEBUG_INFO("Result Byte %d is 0x%x", i, hashData[i]);
   }

   BAB_DEBUG_PROC("- bsp_babGetSecResult(%x)", (unsigned long) hashData);

   MRELEASE(dataBuffer);
   return (unsigned char *) hashData;
}
