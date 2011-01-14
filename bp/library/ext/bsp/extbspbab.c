/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

//#include "ionbsp.h" /** BSP structures and enumerations */
#include "extbsputil.h"
#include "extbspbab.h"
#include "bei.h"	/* BVB */

/* #include "hmac.h" */  /** HMAC-SHA1 implementation */ 
#include "../../crypto/crypto.h"
#include "ionsec.h" /** ION Security Policy Functions */


/*****************************************************************************
 *                     BAB EXTENSIONS INTERFACE FUNCTIONS                    *
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
   BspBabScratchpad *scratch = NULL;
  
   BSP_DEBUG_PROC("+ bsp_babAcquire(%x, %x)", 
                  (unsigned long)blk, (unsigned long)wk);
  
   /* Allocate a scratchpad object to hold a structured view of the block. */  
   blk->size = sizeof(BspBabScratchpad);
   if((blk->object = MTAKE(blk->size)) == NULL)
   {
      BSP_DEBUG_ERR("x bsp_babAcquire:  MTAKE failed on size %d", blk->size);
      blk->size = 0;
      result = -1;
   }
   else
   {
      /* Clear out the block's scratchpad information */  
      scratch = (BspBabScratchpad *) blk->object;
      memset((char *) scratch,0,sizeof(BspBabScratchpad));

      /* Populate the scratchpad object's ASB. */
      result = bsp_deserializeASB(blk);  
      
      BSP_DEBUG_INFO("i bsp_babAcquire: Deserialize result %d", result);      
   }

   /** BVB **/
   //wk->bundle.id.source.d.schemeNameOffset = 1;
   //wk->bundle.id.source.d.nssOffset = 1;
   BSP_DEBUG_WARN("   (BVB) bsp_babAcquire: wk->senderEid = '%s'", wk->senderEid);

   BSP_DEBUG_PROC("- bsp_babAcquire -> %d", result); 
     
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

void    bsp_babClear(AcqExtBlock *blk)
{
   BSP_DEBUG_PROC("+ bsp_babClear(%x)", (unsigned long) blk);
  
   if(blk->size > 0)
   {
      BspBabScratchpad *scratch = (BspBabScratchpad *) blk->object;
      if(scratch->asb.resultLen > 0)
      {
         BSP_DEBUG_INFO("i bsp_babClear: Release result of len %ld",
                        scratch->asb.resultLen);
         MRELEASE(scratch->asb.resultData);
         scratch->asb.resultData = 0;
         scratch->asb.resultLen = 0;    
      }

      BSP_DEBUG_INFO("i bsp_babClear: Release scratchpad of len %d", blk->size);
    
      MRELEASE(blk->object);
      blk->object = NULL;
      blk->size = 0;
   }

   BSP_DEBUG_PROC("- bsp_babClear(%c)", ' ');

   return;
}



/******************************************************************************
 *
 * \par Function Name: bsp_babOffer
 *
 * \par Purpose: This callback determines whether a BAB block is necessary for
 *               this particular bundle, based on local security policies. If
 *               a BAB block is necessary, a scratchpad structure will be  
 *               allocated and stored in the block's scratchpad to ease the 
 *               construction of the block downstream.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error. 
 *
 * \param[in,out]  blk    The block that may/may not be added to the bundle.
 * \param[in]      bundle The bundle that might hold this block.
 *
 * \par Notes: 
 *      1. Setting the length and size fields to 0 will result in the block
 *         NOT being added to the bundle.  This is how we "reject" the block
 *         in the absence of a specific flag to that effect.
 *      2. All block memory is allocated using sdr_malloc.
 *****************************************************************************/

int bsp_babOffer(ExtensionBlock *blk, Bundle *bundle)
{
   Sdr bpSdr = getIonsdr();
   BspBabScratchpad scratch;
   int result = -1;
   
   BSP_DEBUG_PROC("+ bsp_babOffer(%x, %x)", 
                  (unsigned long) blk, (unsigned long) bundle);

   memset((char *) &scratch,0,sizeof(BspBabScratchpad));
   scratch.useBab = 1;
   
   BSP_DEBUG_INFO("i bsp_babOffer: useBab is %d. KeyName %s", 
                  scratch.useBab, scratch.cipherKeyName);
  
   if(scratch.useBab == 1) 
   { 
      /*
       * Populate the type information for the extension block. We do not 
       * alter the block processing flags at this point, as it might be 
       * too "early" to know which flags we want.
       * 
       * Also, we do not allocate the bytes for this block. Again, it might
       * be too early to know the size for this block.
       */
      blk->length = 0;
      blk->bytes = 0;
      
      /* 
       * Allocate the scratchpad that holds the structured representation of
       * this block while it is being constructed and cross-referenced.
       */

      blk->size = sizeof(BspBabScratchpad);
     
      blk->object = sdr_malloc(bpSdr, blk->size);
      if (blk->object == 0)
      {
         BSP_DEBUG_ERR("x bsp_babOffer: Failed to SDR allocate of size: %d", 
                       blk->size);
         result = -1;
      }
      else
      {
         sdr_write(bpSdr, blk->object, (char *) &scratch, blk->size);
         result = 0;
      }
   }
   
   /* If we get here, then this bundle should not have a BAB block. */
   else
   {
      /* 
       * A side-effect from the OFFER callback is that an extension block
       * whose length AND size are 0 will NOT be added to the bundle. As
       * such, no future callbacks for the block will be processed.
       * 
       * This is currently the only way to "decline" the offer of adding this
       * block to the bundle.
       */
      blk->length = 0;
      blk->size = 0;
      result = 0;   
   }

   BSP_DEBUG_PROC("- bsp_babOffer -> %d", result);

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
 * \retval int 0 - The block check was inconclusive
 *             1 - The block check failed.
 *             2 - The block check succeeed. 
 *            -1 - There was a system error. 
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes: 
 *****************************************************************************/

int bsp_babPostCheck(AcqExtBlock *blk, AcqWorkArea *wk)
{
   BspBabScratchpad *scratch = NULL;
   BspAbstractSecurityBlock *asb = NULL;
   int retval = 0;
   unsigned char	*digest;
   unsigned long	digestLen;
   int			cmpResult;
  
   BSP_DEBUG_PROC("+ bsp_babPostCheck(%x, %x)", 
                  (unsigned long) blk, (unsigned long) wk);
  
   /***************************************************************************
    *                             Sanity Checks                               *
    ***************************************************************************/

   /* We need blocks! */
   if((blk == NULL) || (blk->object == NULL) || (wk == NULL))
   {
      BSP_DEBUG_ERR("x bspBabPostCheck:  Blocks are NULL. %x", 
                    (unsigned long) blk);
      BSP_DEBUG_PROC("- bsp)babPostCheck --> %d", -1);
      return -1;    
   }

   if (wk->authentic)
   {
	discardExtensionBlock(blk);
	return 0;	 /*	Authenticity asserted; bail.	*/
   }

   scratch = (BspBabScratchpad *) blk->object;
   asb = &(scratch->asb);
 

   BSP_DEBUG_WARN(" (BVB) babPostCheck: wk->senderEid = '%s'", wk->senderEid);

  
   /* The post-payload BAB block *must* have a security result. */
   if((asb->cipherFlags & BSP_ASB_RES) == 0)
   {
	BSP_DEBUG_ERR("x bspBabPostCheck:  No security results. Flags are %ld", 
                    asb->cipherFlags);
	discardExtensionBlock(blk);
	return 1;
   } 
  
   /* Make sure that we found the correlated pre-payload block. */
   if((scratch->rxFlags & BSP_BABSCRATCH_RXFLAG_CORR) == 0)
   {
	BSP_DEBUG_ERR("x bsp_babPostCheck: No pre-payload block found.%c",' '); 
	discardExtensionBlock(blk);
	return 1;
   }

   /* Passed sanity checks. Get ready to check security result. */

   if (scratch->hmacLen == 0)
   {
      BSP_DEBUG_ERR("x bsp_babPostCheck: SHA1 result not computed %s",
			       scratch->cipherKeyName);
      retval = 1;
   }
   else 
   {
      getBspItem(BSP_CSPARM_INT_SIG, asb->resultData, asb->resultLen,
	&digest, &digestLen);
      if (digest == NULL || digestLen != BAB_HMAC_SHA1_RESULT_LEN)
      {
         BSP_DEBUG_ERR("x bsp_babPostCheck: no integrity signature: security \
result len %lu data %x", asb->resultLen, (unsigned long) asb->resultData);
            retval = 1;   
      }
      else
      {
         cmpResult = memcmp(scratch->expectedResult, digest, digestLen);
         if(cmpResult == 0)
         {
            retval = 2;   
         }
         else
         {
            BSP_DEBUG_ERR("x bsp_babPostCheck: memcmp failed: %d",
                       cmpResult);
            retval = 1;   
         }
      }
   }
   
   /* We are done with this block. */  

   discardExtensionBlock(blk);
   BSP_DEBUG_PROC("- bsp_babPostCheck(%d)", retval);

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

int bsp_babPostProcessOnDequeue(ExtensionBlock *post_blk, 
                                Bundle *bundle, 
                                void *parm)
{
   DequeueContext	*ctxt = (DequeueContext *) parm;
   BspBabScratchpad pre_scratch;
   BspBabScratchpad post_scratch;
   OBJ_POINTER(ExtensionBlock, pre_blk);
   unsigned char *raw_asb = NULL;
   int result = 0;
   Sdr bpSdr = getIonsdr();
   Object elt = 0;
  
   BSP_DEBUG_PROC("+ bsp_babPostProcessOnDequeue(%x, %x, %x)", 
                  (unsigned long) post_blk, 
                  (unsigned long) bundle, 
                  (unsigned long) ctxt);

   if (bundle == NULL || parm == NULL || post_blk == NULL
   || post_blk->size != sizeof(BspBabScratchpad))
   {
	BSP_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Bundle or ASB were not \
as expected.", NULL);
	BSP_DEBUG_PROC("- bsp_babPostProcessOnDequeue --> %d", -1);
	return -1;
   } 

   /* Populate the post-security block fields that are self-evident... */
   sdr_read(bpSdr, (char *) &post_scratch, post_blk->object, post_blk->size);
   bsp_babGetSecurityInfo(bundle, BAB_TX, ctxt->proxNodeEid, &post_scratch);
   if (post_scratch.cipherKeyName[0] == '\0')
   {
	/*	No rule, or no key.					*/

	scratchExtensionBlock(post_blk);
	return 0;
   }

   /***************************************************************************
    *                      Populate Post-Payload ASB                          *
    ***************************************************************************/

  
   /* post-payload BAB must be the last one in the bundle. */ 
   post_blk->blkProcFlags |= BLK_IS_LAST;
   
   /* Update scratchpad with items not added by the acquire callback. */
   post_scratch.asb.cipher = BSP_CSTYPE_BAB_HMAC;
   post_scratch.asb.cipherFlags = BSP_ASB_CORR | BSP_ASB_RES;
   post_scratch.asb.correlator = BAB_CORRELATOR;
   post_scratch.asb.resultLen = BAB_HMAC_SHA1_RESULT_LEN + 2;


   /***************************************************************************
    *                       Verify Pre-Payload Block                          *
    ***************************************************************************/

   /* Find the extension block ELT element. */
   if((elt = findExtensionBlock(bundle, BSP_BAB_TYPE, PRE_PAYLOAD)) == 0)
   {
      BSP_DEBUG_ERR("x bsp_babPostProcessOnDequeue:  Can't find pre block in \
bundle %x", (unsigned long) bundle);
      result = -1;
   }
   else
   {
      Object addr = sdr_list_data(bpSdr, elt);
      GET_OBJ_POINTER(bpSdr, ExtensionBlock, pre_blk, addr);
     
      BSP_DEBUG_INFO("i bsp_babPostProcessOnDequeue: found pre_blk at %x",
                     (unsigned long) pre_blk);  

      /* Grab the pre-payload scratchpad. */
      sdr_read(bpSdr, (char *) &pre_scratch, pre_blk->object, pre_blk->size);
    
      /* Confirm correlator */
      if((pre_scratch.asb.correlator != post_scratch.asb.correlator) ||
         (pre_scratch.asb.cipher != post_scratch.asb.cipher))
      {
         BSP_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Found blocks do not \
correlator: pre %ld, post %ld cipher: pre %ld, post %ld",
                       pre_scratch.asb.correlator, 
                       post_scratch.asb.correlator,
                       pre_scratch.asb.cipher,
                       post_scratch.asb.cipher);
         result = -1;   
      }

      /* post-block looks OK to serialize. */
      else
      {
         /*
          * Serialize the block.  This will do everything except populate
          * the security result data.
          */
         raw_asb = bsp_serializeASB(&(post_blk->dataLength),
                                    &(post_scratch.asb));
         if((raw_asb == NULL) || (post_blk->dataLength == 0)) 
         {
            /* TODO: Remove the pre and post block. */
            BSP_DEBUG_ERR("x bsp_babPostProcessOnDequeue: Serialization failed.\
raw_asb = %x", (unsigned long) raw_asb);
            result = -1;
         }
         else
         {
            /* Store the serialized ASB in the bytes array */
            result = serializeExtBlk(post_blk, NULL, (char *) raw_asb);

            /* Store the updated scratchpad for this block */
            sdr_write(bpSdr, 
                      post_blk->object, 
                      (char *) &post_scratch, 
                      post_blk->size);

            MRELEASE(raw_asb);
         }
      }
   }

   BSP_DEBUG_PROC("- bsp_babPostProcessOnDequeue(%d)", result);

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

int bsp_babPostProcessOnTransmit(ExtensionBlock *blk, Bundle *bundle,void *ctxt)
{
   Object bundleRef = 0;
   ZcoReader bundleReader;
   Sdr bpSdr = getIonsdr();
   char *rawBundle = NULL;
   unsigned int rawBundleLength = 0;
   BspAbstractSecurityBlock *asb = NULL;
   BspBabScratchpad scratch;
   unsigned char	*digest;
   unsigned long	digestLen;
   unsigned char *raw_asb = NULL;
   int result = 0;
  
   BSP_DEBUG_PROC("+ bsp_babPostProcessOnTransmit: %x, %x, %x",
	(unsigned long) blk, (unsigned long) bundle,(unsigned long) ctxt);
  
   memset(&scratch,0,sizeof(BspBabScratchpad));
  
   if((blk == NULL) || (bundle == NULL))
   {
      BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Bad Parms: Bundle %x",
                    (unsigned long) bundle);
      BSP_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", -1);
      return -1;
   }
  
   sdr_read(bpSdr, (char *) &scratch, blk->object, blk->size);
   asb = (BspAbstractSecurityBlock *) &(scratch.asb);
  
   /* 
    * Grab the serialized bundle. It lives in bundle.payload. 
    * Get it ready to serialize, and grab the bundle length.
    */
   bundleRef = zco_add_reference(bpSdr, bundle->payload.content);
   zco_start_transmitting(bpSdr, bundleRef, &bundleReader);
   rawBundleLength = zco_length(bpSdr, bundleRef) - asb->resultLen;
  
   BSP_DEBUG_INFO("i bsp_babPostProcessOnTransmit: bundle len sans payload= %d",
                  rawBundleLength);
  
   /* Allocate bundle storage */
   if((rawBundle = MTAKE(rawBundleLength)) == NULL)
   {
     BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Unable to allocate %d \
bytes", rawBundleLength);
     /** \todo: cleanup zco? */          
     result = -1;
   }
   else
   {
      /* Suck up the serialized bundle */
      int bytesCopied = zco_transmit(bpSdr, 
                                     &bundleReader, 
                                     rawBundleLength, 
                                     rawBundle);
      zco_stop_transmitting(bpSdr, &bundleReader);
      zco_destroy_reference(bpSdr, bundleRef);
      BSP_DEBUG_INFO("i bsp_babPostProcessOnTransmit: transmit copied %d bytes",
                     bytesCopied);

      /* Make sure we got as many bytes as we expected */
      if(bytesCopied != rawBundleLength)
      {
         BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: transmit failed Got \
%d and wanted %d.", bytesCopied, rawBundleLength);
         MRELEASE(rawBundle);
         result = -1;
      }
  
      /* Calculate the security result. */
      else
      {
         digest = bsp_babGetSecResult(rawBundle, rawBundleLength, 
		scratch.cipherKeyName, &digestLen);
	 if(digest == NULL || digestLen != BAB_HMAC_SHA1_RESULT_LEN)
	 {
            BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Unable to calculate \
security result. Digest len is %ld.", digestLen);     
            MRELEASE(rawBundle);
	    if (digest) MRELEASE(digest);
            result = 0;
         }
         else
         {
            asb->resultData = MTAKE(digestLen + 2);
	    if (asb->resultData == NULL)
	    {
               BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Can't allocate \
	    	   ASB result, len %ld.", digestLen + 2);
               MRELEASE(rawBundle);
	       MRELEASE(digest);
               result = 0;
	    }
	    else
	    {
	       *(asb->resultData) = BSP_CSPARM_INT_SIG;
	       *(asb->resultData + 1) = digestLen;
	       memcpy(asb->resultData + 2, digest, digestLen);
	       MRELEASE(digest);
         
               /* 
                * serialize the ASB portion of the block. Note, the asb now has
                * the bundle's security result.
                */
               raw_asb = bsp_serializeASB(&(blk->dataLength), &(scratch.asb)); 
               if((raw_asb == NULL) || (blk->dataLength == 0)) 
               {
                  /* TODO: Remove the pre and post block. */
                  BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Serialization \
failed. raw_asb = %x", (unsigned long) raw_asb);
                  result = -1;
               }
               else
               {
                  char *temp = NULL;

                  /* Serialize the block into its bytes array. */
                  result = serializeExtBlk(blk, NULL, (char *) raw_asb);
                  if(result < 0)
                  {
                      BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: Failed \
serializing block. Result is %d", result);
                  }
                  /* 
                   * Put the new block into place.  First, grab the text
	           * of the newly serialized block, from bytes array.  The
	           * serializeExtBlk function dropped the serialized length
	           * of the bytes array into blk.
                   */  
                  else if((temp = MTAKE(blk->length)) == NULL)
                  {
                     BSP_DEBUG_ERR("x bsp_babPostProcessOnTransmit: \
Allocation of %d bytes failed.", blk->length);
                     result = -1;   
                  }
                  else
                  {
                     sdr_read(bpSdr, temp, blk->bytes, blk->length);
                     BSP_DEBUG_INFO("i bsp_babPostProcessOnTransmit: New \
trailer has length %d", blk->length);
                     /*
                      * Throw away the *old* post-payload block and insert the
                      * one we just serialized.
                      */
                     zco_discard_last_trailer(bpSdr, bundle->payload.content);
                     oK(zco_append_trailer(bpSdr, bundle->payload.content,
	       			      (char *) temp, blk->length));
                     MRELEASE(temp);
                  }
            
                  /* Done with the raw ASB now. */
                  MRELEASE(raw_asb);
	       }

               /* Done with the abstract security block. */
               MRELEASE(asb->resultData);
               asb->resultData = NULL;
               asb->resultLen = 0;
	    }
         }
      }   
       
      /* Done with the raw bundle */   
      MRELEASE(rawBundle);
   }

   BSP_DEBUG_PROC("- bsp_babPostProcessOnTransmit --> %d", result);

   return result;  
}




/******************************************************************************
 *
 * \par Function Name: bsp_babPreCheck
 *
 * \par Purpose: This callback checks a pre-payload block, upon bundle receipt
 *               to determine whether the block should be considered authentic. 
 *               For a BAB, this is really just a sanity check on the block, 
 *               making sure that the block fields are consistent.
 *		 Also, at this point the entire bundle has been received
 *		 into a zero-copy object, so we can compute the security
 *		 result for the bundle and pass it on to the post-payload
 *		 BAB check function.
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

int bsp_babPreCheck(AcqExtBlock *pre_blk, AcqWorkArea *wk)
{
   BspBabScratchpad *pre_scratch = NULL;
   BspAbstractSecurityBlock *pre_asb = NULL;
   Sdr			bpSdr = getIonsdr();
   int			digestLen = BAB_HMAC_SHA1_RESULT_LEN;
   int			resultLen = digestLen + 2;	/*	Item.	*/
   unsigned long	rawBundleLen;
   int			bytesRetrieved;
   int			lengthToHash;
   char			*keyValueBuffer;
   int			keyLen;
   char			*rawBundle;
   Object		bundleRef;
   ZcoReader		bundleReader;
   int			i;
   int retval = 0;
    
   BSP_DEBUG_PROC("+ bsp_babPreCheck(%x,%x)", 
                  (unsigned long) pre_blk, (unsigned long) wk);
 
   /***************************************************************************
    *                             Sanity Checks                               *
    ***************************************************************************/

   /* We need blocks! */
   if((pre_blk == NULL) || (pre_blk->object == NULL))
   {
      BSP_DEBUG_ERR("x bsp_babPreCheck:  No blocks. pre_blk is %x", 
                    (unsigned long) pre_blk);
      BSP_DEBUG_PROC("- bsp_babPreCheck --> %d", -1);              
      return -1;    
   }

   BSP_DEBUG_WARN(" (BVB) babPreCheck: wk->senderEid = '%s'", wk->senderEid);

   pre_scratch = (BspBabScratchpad *) pre_blk->object;
   pre_asb = &(pre_scratch->asb);

	/*	The incoming serialized bundle, unaltered, is now
	 *	in the bundle payload content of the work area; its
	 *	BP header and trailer have not yet been stripped off.
	 *	So we can now compute the correct security result
	 *	for this bundle's BABs.					*/

   rawBundleLen = zco_length(bpSdr, wk->bundle.payload.content);
   if ((lengthToHash = rawBundleLen - resultLen) < 0)
   {
	BSP_DEBUG_ERR("x bsp_babPreCheck: Can't hash %d bytes",
			lengthToHash);
	return 0;
   }

   BSP_DEBUG_INFO("i bsp_babPreCheck: len %d", resultLen);
   pre_scratch = (BspBabScratchpad *) (pre_blk->object); 
   bsp_babGetSecurityInfo(&(wk->bundle), BAB_RX, wk->senderEid, pre_scratch);
   if (pre_scratch->cipherKeyName[0] == '\0')
   {
	/*	No rule, or no key.					*/

	return 0;	/*	No hash computation.			*/
   }

   if ((keyValueBuffer = (char *) bsp_retrieveKey(&keyLen,
   		pre_scratch->cipherKeyName)) == NULL)
   {
	BSP_DEBUG_ERR("x bsp_babPreAcquire: Can't retrieve key %s for EID %s",
			pre_scratch->cipherKeyName, wk->senderEid);

	/*	Note that pre_scratch->hmacLen remains zero.  This is
	 *	the indication, during the post-payload BAB check,
	 *	that the key was not retrieved.				*/

	return 0;
   }

   if ((rawBundle = MTAKE(rawBundleLen)) == NULL)
   {
	BSP_DEBUG_ERR("x bsp_babPreCheck: Can't allocate %ld bytes",
			rawBundleLen);
	MRELEASE(keyValueBuffer);
	return -1;
   }

   /*	Retrieve entire bundle into buffer.				*/

   /**	\todo: watch pointer arithmetic if sizeof(char) != 1		*/

   sdr_begin_xn(bpSdr);
   bundleRef = zco_add_reference(bpSdr, wk->bundle.payload.content);
   zco_start_transmitting(bpSdr, bundleRef, &bundleReader);
   bytesRetrieved = zco_transmit(bpSdr, &bundleReader, rawBundleLen, rawBundle);
   zco_stop_transmitting(bpSdr, &bundleReader);
   zco_destroy_reference(bpSdr, bundleRef);
   if (sdr_end_xn(bpSdr) < 0 || bytesRetrieved != rawBundleLen)
   {
	BSP_DEBUG_ERR("x bsp_babPreCheck: Can't receive %ld bytes",
			rawBundleLen);
	MRELEASE(keyValueBuffer);
	MRELEASE(rawBundle);
	return -1;
   }

   for (i = 0; i < lengthToHash; i++)
   {
	/* BSP_DEBUG_INFO("Byte %d is %x", i, rawBundle[i]);  BVB */
   }

   pre_scratch->hmacLen = hmac_authenticate(pre_scratch->expectedResult,
		   digestLen, keyValueBuffer, keyLen, rawBundle, lengthToHash);
   MRELEASE(rawBundle);
   MRELEASE(keyValueBuffer);
   BSP_DEBUG_INFO("i bsp_babPreCheck: Calculated length is %d",
		pre_scratch->hmacLen);
   if (pre_scratch->hmacLen != digestLen)
   {
	BSP_DEBUG_ERR("x bsp_babPreCheck: digestLen %d, hmacLen %d",
			digestLen, pre_scratch->hmacLen);
	return 0;
   }

   /*	Security result for this bundle has now been calculated and
    *	can be used for post-payload BAB check.				*/

   /* The pre-payload block must have a correlator and no result. */
   if(((pre_asb->cipherFlags & BSP_ASB_CORR) == 0) ||
            ((pre_asb->cipherFlags & BSP_ASB_RES) != 0))
   {
      BSP_DEBUG_ERR("x bsp_babPreCheck: Bad Flags! Correlator missing and/or\
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
         BSP_DEBUG_ERR("x bsp_babPreCheck:  Could not find post-payload \
BAB block! %c",  ' ');
         retval = 1;        
      }
      else
      {
         BspBabScratchpad *post_scratch = NULL;
         post_scratch = (BspBabScratchpad *) post_blk->object; 

         /* Odd if the post-payload block has no scratchpad. */
         if(post_scratch == NULL)
         {
            BSP_DEBUG_ERR("x bsp_babPreCheck:  Could not find post-payload \
BAB scratchpad! %c",  ' ');
            retval = 1;                    
         }
         
         /* Make sure this is the right post-payload BAB block. */
         else if(post_scratch->asb.correlator != pre_asb->correlator)
         {
            BSP_DEBUG_ERR("x bsp_babPreCheck:  Could not find post-payload \
BAB block with correlator %ld.", pre_asb->correlator);
            retval = 1;
         }
         
         /* Populate post-payload block with information from this block. */
         else
         {

            /* 
             * The pre-block checks out. Let the post-block know this. We need 
             * to handle things this way since we will be removing the pre-check
             * block before processing the post-payload block, therefore, the
             * post-payload block will not have a chance to inspect the pre-
             * payload block anymore and will need to rely on this flag.
             */

		post_scratch->rxFlags |= BSP_BABSCRATCH_RXFLAG_CORR;  
	    /*
	     * This block has the computed expected security result for
	     * the bundle.  We must copy that result from the pre-payload
	     * block scratchpad to the post-payload block scratchpad.
	     */

		memcpy(post_scratch->cipherKeyName,
				pre_scratch->cipherKeyName, BAB_KEY_NAME_LEN);
		post_scratch->hmacLen = pre_scratch->hmacLen;
		memcpy(post_scratch->expectedResult,
				pre_scratch->expectedResult,
				BAB_HMAC_SHA1_RESULT_LEN);
         }   
      }
   }
  
   /* 
    * We are done with the pre-block. Either the post-block was not found, or
    * we did find it and told it that it did, at one point, have a pre-block.
    * Either way, time to drop the pre-payload block.
    */

   discardExtensionBlock(pre_blk);
   BSP_DEBUG_PROC("- bsp_babPreCheck(%d)", retval);

   return retval;
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

int bsp_babPreProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle, void *parm)
{
   DequeueContext	*ctxt = (DequeueContext *) parm;
   Lyst eidRefs = NULL;
   BspBabScratchpad scratch;
   int result = 0;
   unsigned char *raw_asb = NULL;
   Sdr bpSdr = getIonsdr();
   unsigned long tmp;
 

   BSP_DEBUG_WARN(" (BVB) bsp_babPreProcessOnDequeue called. ctxt=0x%08X", (unsigned long)ctxt );/*BVB*/ 
   BSP_DEBUG_PROC("+ bsp_babPreProcessOnDequeue(%x, %x, %x", 
                  (unsigned long) blk, 
                  (unsigned long) bundle, 
                  (unsigned long) ctxt);

   if((bundle == NULL) || (parm == NULL) || (blk == NULL) || 
   		(blk->size != sizeof(BspBabScratchpad)))
   {
      BSP_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Bundle or ASB were not as \
expected.", NULL);
                    
      BSP_DEBUG_PROC("- bsp_babPreProcessOnDequeue --> %d", -1);
      return -1;
   }


   BSP_DEBUG_WARN("  --> %d passed NULL checking", 1);
  
   /*
    * Grab the scratchpad object and now get security information for
    * the final selected security destination.		//	SB 3 Aug 2009
    */ 

   sdr_read(bpSdr, (char *) &scratch, blk->object, blk->size);
   bsp_babGetSecurityInfo(bundle, BAB_TX, ctxt->proxNodeEid, &scratch);
   if (scratch.cipherKeyName[0] == '\0')
   {
	/*	No rule, or no key.					*/

	scratchExtensionBlock(blk);
	return 0;
   }

   /* XXX BVB 4,6 */
   BSP_DEBUG_WARN("  --> %d passed rule/key checking; ctxt->proxNodeEid = '%s'", 2, ctxt->proxNodeEid);
   

   /* 
    * If we are using EID references, we will populate the BAB with the
    * security source and security destination.  This is "trivial" for the
    * BAB, where the security source is the source of this bundle (i.e., us)
    * and the security destination is the destination of the bundle.  Since
    * we are looking for these values in the process on dequeue callback, they
    * have been added to the provided bundle structure. 
    */
  
   if(bundle->dictionaryLength != 0)
   {

      BSP_DEBUG_WARN(" (BVB) bundle->dictionaryLength > %d", 0); /* BVB */

      /* 
       * If using EIDs, we will always specify both a security source and a
       * security destination.
       */       
      blk->blkProcFlags |= BLK_HAS_EID_REFERENCES;
      scratch.asb.cipherFlags |= (BSP_ASB_SEC_SRC | BSP_ASB_SEC_DEST); 
            
      if((eidRefs = lyst_create_using(getIonMemoryMgr())) == NULL)
      {
        BSP_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Can't allocate eidRefs%c.",
                      ' ');
        result = -1;
      }
      else
      {
         /* Check return values */
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->id.source.d.schemeNameOffset));
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->id.source.d.nssOffset));
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->destination.d.schemeNameOffset));
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->destination.d.nssOffset));
      }   
   }

   /* BVB Remove this */
#if 0
   else
   {
      /* XXX BVB */
      BSP_DEBUG_WARN(" (BVB) bundle->dictionaryLength == %d", 0);
      
      scratch.asb.cipherFlags |= (BSP_ASB_SEC_SRC | BSP_ASB_SEC_DEST); 
            
      if((eidRefs = lyst_create_using(getIonMemoryMgr())) == NULL)
      {
        BSP_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Can't allocate eidRefs%c.",
                      ' ');
        result = -1;
      }
      else
      {
         /* Check return values */
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->id.source.d.schemeNameOffset));
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->id.source.d.nssOffset));
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->destination.d.schemeNameOffset));
         lyst_insert_last(eidRefs,
		(void *) (tmp = bundle->destination.d.nssOffset));


         BSP_DEBUG_WARN(" (BVB %d) pre-process/dequeue:", 0);
         BSP_DEBUG_WARN("       bundle->id.source.d.*:   (%u, %u)", bundle->id.source.d.schemeNameOffset, bundle->id.source.d.nssOffset);
         BSP_DEBUG_WARN("       bundle->destination.d.*: (%u, %u)", bundle->destination.d.schemeNameOffset, bundle->destination.d.nssOffset);
         BSP_DEBUG_WARN("       bundle->clDossier.senderEid = '%s'", getBpString(&(bundle->clDossier.senderEid)));
         BSP_DEBUG_WARN("       bundle->clDossier.senderNodeNbr = %u", bundle->clDossier.senderNodeNbr);\
         BSP_DEBUG_WARN("       bundle->id.source.c.nodeNbr = %u", bundle->id.source.c.nodeNbr);
         BSP_DEBUG_WARN("       bundle->id.source.c.serviceNbr = %u", bundle->id.source.c.serviceNbr);
      }

   }
   
#endif
   if(result == 0)
   {
      /* We always use the HMAC-SHA1 cipher, for now. */
      scratch.asb.cipher = BSP_CSTYPE_BAB_HMAC;

      /*
       * All BAB blocks that we create will be correlated. Since each bundle 
       * can only have 1 BAB set of blocks in it, we do not need to worry about
       * ensuring that this correlator identifier is unique within the bundle.
       */ 
      scratch.asb.cipherFlags |= BSP_ASB_CORR;
      scratch.asb.correlator = BAB_CORRELATOR;

      /* Serialize the Abstract Security Block. */

      raw_asb = bsp_serializeASB(&(blk->dataLength), &(scratch.asb));
      if((raw_asb == NULL) || (blk->dataLength == 0))
      {
         BSP_DEBUG_ERR("x bsp_babPreProcessOnDequeue: Unable to serialize \
ASB. blk->dataLength = %d", blk->dataLength);
         result = -1;
      }
      else
      {
         /* 
          * store this block in its bytes array. This is necessary as the 
          * block will be added to the bundle when this function exits.
          */
         result = serializeExtBlk(blk, eidRefs, (char *) raw_asb);
         
         /* Store ASB in the scratchpad so post-payload blk can use it later. */
         sdr_write(bpSdr, blk->object, (char *) &scratch, blk->size);

         MRELEASE(raw_asb);
      }
   }

   /* Clear the eid references if we used them. */   
   if(eidRefs != NULL)
   {
      lyst_destroy(eidRefs);
   }

   BSP_DEBUG_PROC("- bsp_babPreProcessOnDequeue(%d)", result);  

   return result;
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
 *****************************************************************************/

void    bsp_babRelease(ExtensionBlock *blk)
{

  BSP_DEBUG_PROC("+ bsp_babRelease(%x)", (unsigned long) blk);
  
   /*
    * If we have allocated an abstract security block, then free that memory.
    */   
   
   if(blk->size > 0)
   {
     /*
      * It is OK to simply free the entire scratchpad object, without
      * explicitly freeing the result data part of the ASB. 
      * 
      * bsp_babPreProcessOnDequeue does not have a security result, so there
      * is nothing to free int hat case.
      * 
      * bsp_babPostProcessOnDequeue calculates the security result and 
      * inserts it directly into the bytes array of the block, which is freed
      * by the main ION library.
      * 
      * In neither case is the resultData portion of the ASB structure in the
      * scratchpad allocated.  
      */
     sdr_free(getIonsdr(), blk->object);
   }
    
   BSP_DEBUG_PROC("- bsp_babRelease(%c)", ' ');

   return;
}




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

unsigned char *bsp_babGetSecResult(char *rawBundle, 
                                   unsigned long rawBundleLen, 
                                   char *cipherKeyName, 
                                   unsigned long *hashLen)
{
   unsigned long sha1Len = BAB_HMAC_SHA1_RESULT_LEN;
   char *keyValueBuffer = NULL;
   char *hashData = NULL;
   int keyLen = 0;
   int hmac_len;
   int i = 0; 
   
   BSP_DEBUG_PROC("+ bsp_babGetSecResult(%x, %ld, %s %x)", 
                  (unsigned long) rawBundle, 
                  rawBundleLen, 
                  cipherKeyName, 
                  (unsigned long) hashLen);
  
   *hashLen = 0;
  
   /* Grab the key value */
   if((keyValueBuffer = (char*) bsp_retrieveKey(&keyLen,cipherKeyName)) == NULL)
   {
      BSP_DEBUG_ERR("x bsp_babGetSecResult:  Failed to retrieve EID key %s.", 
                    cipherKeyName);
   }
   else
   {
      BSP_DEBUG_INFO("i bsp_babGetSecResult: key begins with %.*s",
		      MIN(128, keyLen), keyValueBuffer);

      /* Generate the HMAC-SHA1 Hash. */
      if((hashData = MTAKE(sha1Len)) == NULL)
      {
         BSP_DEBUG_ERR("x bsp_babGetSecResult:  Malloc of %ld failed.",
	 		sha1Len);
      }
      else
      {
         for(i = 0; i < rawBundleLen; i++)
         {
            BSP_DEBUG_INFO("Byte %d is %x", i, rawBundle[i]);   
         }

         hmac_len = hmac_authenticate(hashData, 
                                      sha1Len,
                                      keyValueBuffer, 
                                      keyLen, 
                                      rawBundle, 
                                      rawBundleLen);
         if(hmac_len != sha1Len)
         {
            BSP_DEBUG_ERR("x bsp_babGetSecResult: Hash Length Invalid. \
Expected %ld and got %d.", sha1Len, hmac_len);
            MRELEASE(hashData);
         }         
      }
      
      /* Get rid of key */
      MRELEASE(keyValueBuffer);
   }
   
   BSP_DEBUG_PROC("- bsp_babGetSecResult(%x)", (unsigned long) hashData);

   *hashLen = sha1Len;
   return (unsigned char *) hashData;
}


/******************************************************************************
 *
 * \par Function Name: bsp_babGetSecurityInfo
 *
 * \par Purpose: This utility function retrieves security information for a
 *               given BAB block from the ION security manager.
 *
 * \retval void 
 *
 * \param[in]  bundle    The bundle that holding the block whose security
 *                       information is being requested.
 * \param[in]  which     Whether we are receiving or transmitting the block.
 * \param[in]  eidString The name of the source endpoint.
 * \param[out] scratch   The block scratchpad holding security information for 
 *                       this block.
 * 
 * \par Notes: 
 *****************************************************************************/

void bsp_babGetSecurityInfo(Bundle *bundle, 
                            int which, 
                            char *eidString, 
                            BspBabScratchpad *scratch)
{
   char * bvbEidString;

   BSP_DEBUG_PROC("+ bsp_babGetSecurityInfo(%x %d, %s, %x)",
	(unsigned long) bundle,which,eidString,(unsigned long) scratch);


   BSP_DEBUG_WARN(" (BVB) getting sec info for EID '%s'", eidString);


   /* By default, we disable BAB processing */
   scratch->useBab = 0;
   scratch->cipherKeyName[0] = '\0';  	 


   /* BVB */
   BSP_DEBUG_WARN("  +- BVB ---->  Bundle Source EID (offset): %u.%u", bundle->id.source.d.schemeNameOffset,
                                           bundle->id.source.d.nssOffset);

   /* BVB */
   bvbEidString = getBpString(&(bundle->clDossier.senderEid)); 
   BSP_DEBUG_WARN("  +- BVB ---->  Bundle Source EID (BpString): '%s'", bvbEidString);


   /* Since we look up key information by EndPointID, if we do not have the
    * EID, we cannot get any security information.  We will assume, then, that
    * we are not using the BAB.
    */          
   if(eidString == NULL)
   {
      BSP_DEBUG_WARN("? bsp_babGetSecurityInfo: Can't get EID from bundle \
%x. Not using BAB.", (unsigned long) bundle); 
   }
   else
   {
      Object ruleAddr;
      Object eltp;
      
      /*
       * Grab the security rule (BAB use, cipherKeyName) based on whether this
       * is a TX or an RX rule.
       */
      if(which == BAB_TX)
      {
         OBJ_POINTER(BabTxRule, rule);
    	
         if((sec_get_babTxRule(eidString, &ruleAddr, &eltp) == -1)
	 || (eltp == 0))
         {
   		BSP_DEBUG_INFO("x bsp_babGetSecurityInfo: No TX entry for \
EID %s.", eidString);
         }
         else
         {
      	    /** \todo: Check ciphersuite name */
            GET_OBJ_POINTER(getIonsdr(), BabTxRule, rule, ruleAddr);
            scratch->useBab = 1;
	    if (rule->ciphersuiteName[0] != '\0')
	    {
           	 istrcpy(scratch->cipherKeyName,rule->keyName,
				 sizeof scratch->cipherKeyName);
	    }

            BSP_DEBUG_INFO("i bsp_babGetSecurityInfo: get TX key name of %s", 
                           scratch->cipherKeyName);
         }
      }
      else
      {
         OBJ_POINTER(BabRxRule, rule);

         if((sec_get_babRxRule(eidString, &ruleAddr, &eltp) == -1)
	 || (eltp == 0))
         {
		BSP_DEBUG_INFO("x bsp_babGetSecurityInfo: No RX entry for \
EID %s.", eidString);
         }
         else
         {
      	    /** \todo: Check ciphersuite name */
            GET_OBJ_POINTER(getIonsdr(), BabRxRule, rule, ruleAddr);
            scratch->useBab = 1;
	    if (rule->ciphersuiteName[0] != '\0')
	    {
            	istrcpy(scratch->cipherKeyName,rule->keyName,
				 sizeof scratch->cipherKeyName);
	    }

            BSP_DEBUG_INFO("i bsp_babGetSecurityInfo: get RX key name of %s", 
                           scratch->cipherKeyName);
         }
      }
   }
  
   BSP_DEBUG_PROC("- bsp_babGetSecurityInfo %c", ' ');
}
