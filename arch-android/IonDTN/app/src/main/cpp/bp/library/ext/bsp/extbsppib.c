#include "extbsputil.h"
#include "extbsppib.h"

#if (PIB_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

#include "crypto.h"


/*****************************************************************************
 *                     PIB EXTENSIONS INTERFACE FUNCTIONS                    *
 *                                                                           *
 *                                Call Order                                 *
 *                                                                           *
 *       SEND SIDE                                      RECEIVE SIDE         *  
 *                                                                           *
 *   bsp_pibOffer                                                            *
 *   bsp_pibProcessOnDequeue                                                 *
 *   bsp_pibRelease                                                          *
 *                                                  bsp_pibAcquire           *
 *                                                  bsp_pibCheck             *
 *                                                  bsp_pibClear             *
 *                                                                           *
 *****************************************************************************/


/******************************************************************************
 *
 * \par Function Name: bsp_pibAcquire
 *
 * \par Purpose: This callback is called when a serialized PIB bundle is
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

int bsp_pibAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
   int result = -1;

   BspAbstractSecurityBlock *asb = NULL;

   PIB_DEBUG_PROC("+ bsp_pibAcquire(%x, %x)",
                  (unsigned long)blk, (unsigned long)wk);

   CHKERR(blk);

   /* Allocate a scratchpad object to hold a structured view of the block. */
   blk->size = sizeof(BspAbstractSecurityBlock);
   if((blk->object = MTAKE(blk->size)) == NULL)
   {
      PIB_DEBUG_ERR("x bsp_pibAcquire:  MTAKE failed on size %d", blk->size);
      blk->size = 0;
      result = -1;
   }
   else
   {
      /* Clear out the block's scratchpad information */
      asb = (BspAbstractSecurityBlock *) blk->object;
      memset((char *) asb,0, blk->size);

      /* Populate the scratchpad object's ASB. */
      result = bsp_deserializeASB(blk, wk, BSP_PIB_TYPE);

      PIB_DEBUG_INFO("i bsp_pibAcquire: Deserialize result %d", result);
   }

   PIB_DEBUG_PROC("- bsp_pibAcquire -> %d", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_pibClear
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

void	bsp_pibClear(AcqExtBlock *blk)
{
   PIB_DEBUG_PROC("+ bsp_pibClear(%x)", (unsigned long) blk);

   CHKVOID(blk);
   if(blk->size > 0)
   {
      BspAbstractSecurityBlock *asb = (BspAbstractSecurityBlock *) blk->object;
      if(asb->resultLen > 0)
      {
         PIB_DEBUG_INFO("i bsp_pibClear: Release result of len %ld",
                        asb->resultLen);
         MRELEASE(asb->resultData);
         asb->resultData = 0;
         asb->resultLen = 0;
      }

      PIB_DEBUG_INFO("i bsp_pibClear: Release ASB of len %d", blk->size);

      MRELEASE(blk->object);
      blk->object = NULL;
      blk->size = 0;
   }

   PIB_DEBUG_PROC("- bsp_pibClear(%c)", ' ');

   return;
}


/******************************************************************************
 *
 * \par Function Name: bsp_pibCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a PIB block
 * 		 to a new block that is a copy of the original.
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

int bsp_pibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk)
{
   Sdr bpSdr = getIonsdr();
   BspAbstractSecurityBlock asb;
   int result = -1;

   PIB_DEBUG_PROC("+ bsp_pibCopy(%x, %x)",
                  (unsigned long) newBlk, (unsigned long) oldBlk);

   CHKERR(newBlk);
   CHKERR(oldBlk);
   sdr_read(bpSdr, (char *) &asb, oldBlk->object, sizeof asb);

   /*
    * Reserve space for the scratchpad object in the block. We aren't actually
    * sure that we will be using the PIB, so we don't go through the hassle of
    * allocating objects in the SDR yet, as that would incur a larger
    * performance penalty on bundles that do not use the PIB.
    */

   newBlk->size = sizeof asb;
   newBlk->object = sdr_malloc(bpSdr, sizeof asb);
   if (newBlk->object == 0)
   {
      PIB_DEBUG_ERR("x bsp_pibCopy: Failed to SDR allocate of size: %d",
			sizeof asb);
      result = -1;
   }
   else
   {
      sdr_write(bpSdr, newBlk->object, (char *) &asb, sizeof asb);
      result = 0;
   }

   PIB_DEBUG_PROC("- bsp_pibCopy -> %d", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_pibOffer
 *
 * \par Purpose: This callback determines whether a PIB block is necessary for
 *               this particular bundle, based on local security policies.
 *               However, at this point we may not have enough information
 *               (such as EIDs) to query the security policy. Therefore, the
 *               offer callback ALWAYS adds the PIB block.  When we do the
 *               process on dequeue callback we will have enough information
 *               to determine whether or not the PIB extension block should
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
 *      2. If this bundle was forwarded and there was already a PIB,
 *         ION does not offer another block (aka this won't be executed
 *****************************************************************************/

int bsp_pibOffer(ExtensionBlock *blk, Bundle *bundle)
{
   Sdr bpSdr = getIonsdr();
   BspAbstractSecurityBlock asb;
   int result = -1;


   PIB_DEBUG_PROC("+ bsp_pibOffer(%x, %x)",
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
    * sure that we will be using the PIB, so we don't go through the hassle of
    * allocating objects in the SDR yet, as that would incur a larger performance
    * penalty on bundles that do not use the PIB.
    */
   blk->size = sizeof(BspAbstractSecurityBlock);

   blk->object = sdr_malloc(bpSdr, blk->size);
   if (blk->object == 0)
   {
      PIB_DEBUG_ERR("x bsp_pibOffer: Failed to SDR allocate of size: %d",
            blk->size);
      result = -1;
   }
   else
   {
      sdr_write(bpSdr, blk->object, (char *) &asb, blk->size);
      result = 0;
   }

   PIB_DEBUG_PROC("- bsp_pibOffer -> %d", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_pibCheck
 *
 * \par Purpose: This callback checks a post-payload block, upon bundle receipt
 *               to determine whether the block should be considered corrupt.
 *               For a PIB, this implies that the security result encoded in
 *               this block is the correct hash for the bundle.
 *
 * \retval int 0 - The block check succeeded or was inconclusive.
 *             3 - The block check failed.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 *****************************************************************************/

int bsp_pibCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
   BspAbstractSecurityBlock *asb = NULL;
   int temp = 0, retval = 0;
   unsigned char   *digest;
   unsigned int   digestLen;

   PIB_DEBUG_PROC("+ bsp_pibCheck(%x, %x)",
                  (unsigned long) blk, (unsigned long) wk);

   /***************************************************************************
    *                             Sanity Checks                               *
    *                                                                         *
    *  1) No correlator value (by ION security policy, only 1 PIB).           *
    *  2) Security result must be present (as collab block not present).      *
    *  3) Digest exists                                                       *
    *  4) Ciphersuite is recognized and digest lengths are proper.
    ***************************************************************************/

   if((blk == NULL) || (blk->object == NULL) || (wk == NULL))
   {
      PIB_DEBUG_ERR("x bsp_pibCheck:  Blocks are NULL. %x",
                    (unsigned long) blk);
      PIB_DEBUG_PROC("- bsp_pibPostCheck --> %d", -1);
      return -1;
   }

   /* Grab ASB */
   asb = (BspAbstractSecurityBlock *) blk->object;

   // If we are not the security destination, skip but keep the block.
   if(eidIsLocal(asb->secDest, wk->dictionary) == 0)
   {
	   PIB_DEBUG_INFO("- bsp_pibCheck - Node is not security dest.", NULL);
	   PIB_DEBUG_PROC("- bsp_pibCheck(2)", NULL);
           // Set the processing flag "Forwarded without processing"
           blk->blkProcFlags |= BLK_FORWARDED_OPAQUE;
	   return 0;
   }

   getBspItem(BSP_CSPARM_INT_SIG, asb->resultData, asb->resultLen, &digest,
             &digestLen);

   /* Pessimism: assume check failure */
   retval = 3;

   /* Make sure no correlator value. */
   if((asb->cipherFlags & BSP_ASB_CORR) != 0)
   {
	   PIB_DEBUG_ERR("x bsp_pibCheck: Unexpected correlator found. Flags: %ld",
	                 asb->cipherFlags);
   }

   /* Make sure we have a security result. */
   else if((asb->cipherFlags & BSP_ASB_RES) == 0)
   {
	   PIB_DEBUG_ERR("x bsp_pibCheck:  No security results. Flags are %ld",
				     asb->cipherFlags);
   }

   /* Digest exists */
   else if((digest == NULL) || (digestLen <= 0))
   {
	   PIB_DEBUG_ERR("x bsp_pibCheck: No digest found.", NULL);
   }

   /* Digest made with recognized ciphersuite */
   else if(asb->cipher != BSP_CSTYPE_PIB_HMAC_SHA256)
   {
	   PIB_DEBUG_ERR("x bsp_pibCheck: Unsupported ciphersuite: %d",
			         asb->cipher);
   }

   /* Digest of correct size */
   else if(digestLen !=  BSP_PIB_HMAC_SHA256_RESULT_LEN)
   {
	   PIB_DEBUG_ERR("x bsp_pibCheck: Digest length incorrect. %d not %d",
			         digestLen, BSP_PIB_HMAC_SHA256_RESULT_LEN);
   }
   /*
    * Sanity checks passed.  Construct a digest of the current payload and
    * compare it to the signed value.
    */
   else
   {
      /* Compare expected result to the computed digest. */
	  unsigned char   *tmpDigest;
	  unsigned int   tmpDigestLen;
	  char *keyValueBuffer;
	  int keyLen;
	  BspSecurityInfo secInfo;
	  int cmpResult;
          Object payloadData;
          Sdr bpSdr = getIonsdr();
          char *srcNode = NULL;
          char *destNode = NULL;

          // Get the src and dest needed to get key
          oK(printEid(&asb->secSrc, wk->dictionary, &srcNode));
          oK(printEid(&asb->secDest, wk->dictionary, &destNode));
	  if (srcNode == NULL || destNode == NULL)
          {
	      if (srcNode) MRELEASE(srcNode);
	      if (destNode) MRELEASE(destNode);
	      PIB_DEBUG_ERR("x bsp_pibCheck: Error retrieving src and dest nodes to find key.", NULL);
	      PIB_DEBUG_PROC("- bsp_pibCheck --> %d", -1);
	      return -1;
          }

	  /* Get key information */
	  bsp_getSecurityInfo(&(wk->bundle), BSP_PIB_TYPE, 1,
                                srcNode,
                                destNode,
	                        &secInfo);
	  MRELEASE(srcNode); MRELEASE(destNode);

	  if (secInfo.cipherKeyName[0] == '\0')
	  {
		  /*   No rule, or no key.               */
		  PIB_DEBUG_INFO("i bsp_pibCheck: No rule/key for PIB.", NULL);
		  PIB_DEBUG_PROC("- bsp_pibCheck --> 0", NULL);
		  return 0;   /*   No hash computation.         */
	  }

	  keyValueBuffer = (char *) bsp_retrieveKey(&keyLen, secInfo.cipherKeyName);
	  if (keyValueBuffer == NULL)
	  {
		  PIB_DEBUG_ERR("x bsp_pibCheck: Can't retrieve key %s for EID %s",
				        secInfo.cipherKeyName, wk->senderEid);
		  PIB_DEBUG_PROC("- bsp_pibCheck --> 0", NULL);
		  return 0;
	  }

          // The entire bundle currently sits in payload.content,
          // extract just the real payload content
          CHKERR(sdr_begin_xn(bpSdr));
          payloadData = zco_clone(bpSdr, wk->bundle.payload.content, wk->headerLength,
                                  wk->bundle.payload.length);
          // The length should be == to the payload length now
          if((temp = zco_length(bpSdr, payloadData)) != wk->bundle.payload.length) 
          {
              // Something went wrong, bail
              PIB_DEBUG_ERR("x bsp_pibCheck: Failed to isolate payload data \
                             length of zco %d, should be payload length of %d", temp, wk->bundle.payload.length);
              zco_destroy(bpSdr, payloadData);
              oK(sdr_end_xn(bpSdr));
              PIB_DEBUG_PROC("- bsp_pibCheck(%d)", -1);
              return -1;
          }

          if (sdr_end_xn(bpSdr) < 0)
	  {
		  putErrmsg("Transaction failed.", NULL);
		  return -1;
	  }

	  /* Grab the digest of the received payload */
	  tmpDigest = bsp_pibGetSecResult(payloadData, wk->bundle.payload.length,
			                  keyValueBuffer, keyLen, &tmpDigestLen);

          // Destroy the payload object as we don't need it any longer
          CHKERR(sdr_begin_xn(bpSdr));
          zco_destroy(bpSdr, payloadData);
          if (sdr_end_xn(bpSdr) < 0)
	  {
		  putErrmsg("Transaction failed.", NULL);
	  }

      cmpResult = memcmp(digest, tmpDigest, digestLen);

      MRELEASE(keyValueBuffer);
      MRELEASE(tmpDigest);

      if((cmpResult == 0) && (digestLen == tmpDigestLen))
      {
         retval = 0;
      }
      else
      {
         PIB_DEBUG_ERR("x bsp_pibCheck: memcmp failed: %d", cmpResult);
         retval = 3;
      }
   }

   /* We are done with this block. */
   discardExtensionBlock(blk);
   PIB_DEBUG_PROC("- bsp_pibCheck(%d)", retval);

   return retval;
}

/******************************************************************************
 *
 * \par Function Name: bsp_pibProcessOnDequeue
 *
 * \par Purpose: This callback constructs the PIB block for
 *               inclusion in a bundle right before the bundle is transmitted.
 *
 * \retval int 0 - The block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  post_blk  The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 *
 * \par Notes:
 *      1. The payload must not have its content changed after this point. The
 *      payload may be encrypted, but the digest will not match until the
 *      payload is then decrypted.
 *****************************************************************************/
int bsp_pibProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
                            void *parm)
{
   DequeueContext   *ctxt = (DequeueContext *) parm;
   BspAbstractSecurityBlock asb;
   BspSecurityInfo secInfo;
   int result = 0;
   Sdr bpSdr = getIonsdr();
   char *keyValueBuffer = NULL;
   int keyLen = 0;
   unsigned char *digest;
   unsigned int digestLen = 0;
   char *srcNode = NULL, *destNode = NULL;
   Lyst eidRefs = NULL;
   unsigned char *raw_asb;
   Sdnv digestSdnv;
   unsigned int digestOffset = 0;

   PIB_DEBUG_PROC("+ bsp_pibProcessOnDequeue(%x, %x, %x)",
                  (unsigned long) blk,
                  (unsigned long) bundle,
                  (unsigned long) ctxt);

   // Sanity Check...
   if (bundle == NULL || parm == NULL || blk == NULL)
   {
     PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: Bundle or ASB were not \
as expected.", NULL);
     PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue bundle %d, parm %d, blk %d, blk->size %d",
                    bundle, parm, blk, blk->size);
     PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> %d", -1);
     scratchExtensionBlock(blk);
     return -1;
   }

   // Check the processing flag "Forwarded without processing"
   if(blk->blkProcFlags & BLK_FORWARDED_OPAQUE) {
       // do nothing.. the bytes on the block should be correct and ready for transmission,
       // everything was transferred and serialized just in the recordExtensionBlocks() function 
       // so we just pass it along
       return 0;
   } 
   else if(blk->size != sizeof(BspAbstractSecurityBlock)) 
   {
       PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: block scratchpad ASB size is not \
as expected.", NULL);
       PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue blk->size %d", blk->size);
       PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> %d", -1);
       scratchExtensionBlock(blk);
       return -1;
   }

   /** Grab the ASB. */
   sdr_read(bpSdr, (char *) &asb, blk->object, blk->size);

   srcNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);
   destNode = MTAKE(MAX_SCHEME_NAME_LEN + 1 + MAX_EID_LEN);

   if(setSecPointsTrans(blk, bundle, &asb, &eidRefs, BSP_PIB_TYPE, ctxt, srcNode, destNode) != 0)
   {
       MRELEASE(srcNode); MRELEASE(destNode);
       if(eidRefs != NULL) lyst_destroy(eidRefs);
       PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: setSecPointsTrans failed.", NULL);
       PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> %d", -1);
       scratchExtensionBlock(blk);
       return -1;
   }
   else if(srcNode == NULL || destNode == NULL)
   {
       MRELEASE(srcNode); MRELEASE(destNode);
       if(eidRefs != NULL) lyst_destroy(eidRefs);
       PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: a node address is unexpectedly null! \
srcNode:%s, destNode:%s", srcNode, destNode);
       PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> %d", -1);
       scratchExtensionBlock(blk);
       return -1;
   }

   /*
    * Check that we will need a PIB. If we have no key, then there is either
    * no rule or no key associated with PIBs over this hop, so we don't need
    * a PIB block in this bundle.  This is not an error.
    */

   bsp_getSecurityInfo(bundle,
		       BSP_PIB_TYPE,
		       1,
		       srcNode,
		       destNode,
                       &secInfo);
   MRELEASE(srcNode); MRELEASE(destNode);

   if (secInfo.cipherKeyName[0] == '\0')
   {
      int result = 0;
      PIB_DEBUG_INFO("i bsp_pibProcessOnDequeue: No key found for PIB. \
Not using PIB blocks for this bundle.", NULL);
      if(eidRefs != NULL) lyst_destroy(eidRefs);
      scratchExtensionBlock(blk);
      PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> %d", result);
      return result;
   }

   keyValueBuffer = (char *) bsp_retrieveKey(&keyLen, secInfo.cipherKeyName);
   if (keyValueBuffer == NULL)
   {
	   PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: Can't retrieve key %s for dest EID %s",
			   secInfo.cipherKeyName, bundle->destination);
           if(eidRefs != NULL) lyst_destroy(eidRefs);
	   PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> 0", NULL);
	   return 0;
   }

   /* Grab the digest of the received payload */
   digest = bsp_pibGetSecResult(bundle->payload.content,
   				bundle->payload.length,
   			        keyValueBuffer, keyLen, &digestLen);

   MRELEASE(keyValueBuffer);

   /***************************************************************************
    *                      Populate ASB                                       *
    ***************************************************************************/

   // Encode the digest length into an sdnv
   encodeSdnv(&digestSdnv, digestLen);
   digestOffset = 1 + digestSdnv.length;

   asb.cipher = BSP_CSTYPE_PIB_HMAC_SHA256;
   asb.cipherFlags |= BSP_ASB_RES;
   asb.resultLen = digestLen + digestOffset;

   if((asb.resultData = MTAKE(digestLen + digestOffset)) == NULL)
   {
	   PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: Can't alloc result: len %ld.",
			         digestLen + digestOffset);
	   MRELEASE(digest);
           if(eidRefs != NULL) lyst_destroy(eidRefs);
	   PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue --> %d", -1);

	   return -1;
   }

   /* Construct the new security result. */
   *(asb.resultData) = BSP_CSPARM_INT_SIG;
   memcpy(asb.resultData + 1, digestSdnv.text, digestSdnv.length);
   memcpy(asb.resultData + digestOffset, digest, digestLen);
   MRELEASE(digest);

   /*
    * Serialize the block.  This will do everything except populate
    * the security result data.
    */

   raw_asb = bsp_serializeASB(&(blk->dataLength), &(asb));
   if((raw_asb == NULL) || (blk->dataLength == 0))
   {
      PIB_DEBUG_ERR("x bsp_pibProcessOnDequeue: Serialization failed.\
raw_asb = %x", (unsigned long) raw_asb);
      if(eidRefs != NULL) lyst_destroy(eidRefs);
      scratchExtensionBlock(blk);
      result = -1;
   }
   else
   {
      /* Store the serialized ASB in the bytes array */
      result = serializeExtBlk(blk, eidRefs, (char *) raw_asb);

      /* Store the updated ASB for this block */
      sdr_write(bpSdr,
            blk->object,
            (char *) &asb,
            blk->size);
      MRELEASE(raw_asb);
   }

   if(eidRefs != NULL) lyst_destroy(eidRefs);
   PIB_DEBUG_PROC("- bsp_pibProcessOnDequeue(%d)", result);

   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_pibRelease
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
 *
 *****************************************************************************/

void    bsp_pibRelease(ExtensionBlock *blk)
{

   PIB_DEBUG_PROC("+ bsp_pibRelease(%x)", (unsigned long) blk);

   CHKVOID(blk);
   if(blk->size > 0)
   {
	   sdr_free(getIonsdr(), blk->object);
   }

   PIB_DEBUG_PROC("- bsp_pibRelease(%c)", ' ');

   return;
}


/*****************************************************************************
 *                            BAB HELPER FUNCTIONS                           *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_pibGetSecResult
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
 *****************************************************************************/

unsigned char *bsp_pibGetSecResult(Object dataObj,
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
   unsigned int chunkSize = BSP_PIB_BLOCKING_SIZE;
   unsigned int bytesRetrieved = 0;

   PIB_DEBUG_INFO("+ bsp_pibGetSecResult(0x%x, %ld, %s %d, 0x%x)",
                  (unsigned long) dataObj,
                  dataLen,
                  keyValue,
                  keyLen,
                  (unsigned long) hashLen);

   CHKNULL(keyValue);
   CHKNULL(hashLen);

   /*	Allocate a data buffer.	*/
   dataBuffer = MTAKE(BSP_PIB_BLOCKING_SIZE);
   CHKNULL(dataBuffer);

   /* Grab a context for the hmac. A local context allows re-entrant calls
    * to the HMAC libraries. */
   if((authCtxLen = hmac_sha256_context_length()) <= 0)
   {
      PIB_DEBUG_ERR("x bsp_pibGetSecResult: Bad context length (%d)",
         authCtxLen);
      *hashLen = 0;
      PIB_DEBUG_PROC("- bsp_pibGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
   }

   PIB_DEBUG_INFO("i bsp_pibGetSecResult: context length is %d", authCtxLen);

   if((authContext = MTAKE(authCtxLen)) == NULL)
   {
      PIB_DEBUG_ERR("x bsp_pibGetSecResult: Can't allocate %ld bytes",
         authCtxLen);
         *hashLen = 0;
      PIB_DEBUG_PROC("- bsp_pibGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
   }

   /*   Prepare the data for processing. */
   CHKNULL(sdr_begin_xn(bpSdr));
   zco_start_transmitting(dataObj, &dataReader);
   
   hmac_sha256_init(authContext,(unsigned char *)keyValue, keyLen);
   bytesRemaining = dataLen;

   PIB_DEBUG_INFO("i bsp_pibGetSecResult: size is %d", bytesRemaining);
   PIB_DEBUG_INFO("i bsp_pibGetSecResult: Key value is %s and key length is %d",
		          keyValue, keyLen);

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
    	 PIB_DEBUG_ERR("x bsp_pibGetSecResult: Read %d bytes, but expected %d.",
					   bytesRetrieved, chunkSize);

    	 MRELEASE(authContext);
         oK(sdr_end_xn(bpSdr));

         *hashLen = 0;
         PIB_DEBUG_PROC("- bsp_pibGetSecResult--> NULL", NULL);
         MRELEASE(dataBuffer);
         return NULL;
     }

     /* Print out debugging information, if requested */
     for (i = 0; i < chunkSize; i++)
     {
      //PIB_DEBUG_INFO("Byte %d is %x", i+(dataLen-bytesRemaining), dataBuffer[i]);
     }

     /* Add the data to the hmac_sha1 */
     hmac_sha256_update(authContext, (unsigned char *)dataBuffer, chunkSize);
     bytesRemaining -= bytesRetrieved;
   }

   /* This will store the hash result. */
   if((hashData = MTAKE(BSP_PIB_HMAC_SHA256_RESULT_LEN)) == NULL)
   {
      PIB_DEBUG_ERR("x bsp_pibGetSecResult: Failed allocating %d bytes.",
                     BSP_PIB_HMAC_SHA256_RESULT_LEN);

      MRELEASE(authContext);
      oK(sdr_end_xn(bpSdr));

      *hashLen = 0;
      PIB_DEBUG_PROC("- bsp_pibGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
   }
   // set to zero in case hmac_sha1_final doesn't fill up the buffer
   memset(hashData, 0, BSP_PIB_HMAC_SHA256_RESULT_LEN);

   PIB_DEBUG_INFO("i bsp_pibGetSecResult: allocated hash data.",NULL);

   /* Calculate the hash result. */
   hmac_sha256_final(authContext, hashData, BSP_PIB_HMAC_SHA256_RESULT_LEN);
   hmac_sha256_reset(authContext);

   MRELEASE(authContext);
   if ((i = sdr_end_xn(bpSdr)) < 0)
   {
      PIB_DEBUG_ERR("x bsp_pibGetSecResult: Failed closing transaction. Result is %d.", i);

      MRELEASE(hashData);
      *hashLen = 0;
      PIB_DEBUG_PROC("- bsp_pibGetSecResult--> NULL", NULL);
      MRELEASE(dataBuffer);
      return NULL;
   }

   *hashLen = BSP_PIB_HMAC_SHA256_RESULT_LEN;

   for(i = 0; i < BSP_PIB_HMAC_SHA256_RESULT_LEN; i++)
   {
	   PIB_DEBUG_INFO("Result Byte %d is 0x%x", i, hashData[i]);
   }

   PIB_DEBUG_PROC("- bsp_pibGetSecResult(%x)", (unsigned long) hashData);

   MRELEASE(dataBuffer);
   return (unsigned char *) hashData;
}


