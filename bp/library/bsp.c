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
 ** \file bsp.c
 ** 
 ** File Name: bsp.c
 **
 **
 ** Subsystem:
 **          BSP-ION
 **
 ** Description: This file provides a partial implementation of the Bundle
 **              Security Protocol (BSP) Specification, Version 0.8. This
 **              implementation utilizes the ION Extension Interface to
 **              manage the creation, modification, evaluation, and removal
 **              of BSP blocks from Bundle Protocol (RFC 5050) bundles.
 **
 ** Notes:  The current implementation of this file (6/2009) only supports
 **         the Bundle Authentication Block (BAB) with the following
 **         constraints:
 **         - Bundle fragmentation is not considered
 **         - Only the HMAC-SHA1 ciphersuite for BAB is considered
 **         - No ciphersuite parameters are utilized or supported.
 **         - All BAB blocks will utilize both the pre- and post-payload block.
 **
 ** Assumptions:
 **      1. We assume that this code is not under such tight profiling
 **         constraints that sanity checks are too expensive.
 **
 **      2. As a general rule, abstract security block structures are created
 **         and used as the unit of scratchpad information for the extension
 **         block.  While less efficient, this provides easy maintainability.
 **         As such, we assume that the time and space necessary to use the
 **         scratchpad in this way does not exceed available margin.
 **      
 **      3. We assume that the extensions interface never passes us a NULL 
 **         value.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 **  --------  ------------  -----------------------------------------------
 **  06/08/09  E. Birrane           Initial Implementation of BAB blocks.
 **  06/15/09  E. Birrane           BAB Unit Testing and Documentation updates.
 **  06/20/09  E. Birrane           Documentation updates for initial release.
 **  12/04/09  S. Burleigh          Revisions per DINET and DEN testing.
 *****************************************************************************/


/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "ionbsp.h" /** BSP structures and enumerations */
#include "hmac.h"   /** HMAC-SHA1 implementation */ 
#include "ionsec.h" /** ION Security Policy Functions */

/*****************************************************************************
 *                            VARIABLE DEFINITIONS                           *
 *****************************************************************************/

/** \var gMsg
 * Global variable used to hold a constructed error message. NOT RE-ENTRANT! 
 * This is accessed by the BSP_DEBUG macros.
 */
static char gMsg[256];

/*****************************************************************************
 *                           GENERAL BSP FUNCTIONS                           *
 *****************************************************************************/


/******************************************************************************
 *
 * \par Function Name: bsp_addSdnvToStream
 *
 * \par Purpose: This utility function adds the contents of an SDNV to a
 *               character stream and then returns the updated stream pointer.
 *
 * \retval unsigned char * -- The updated stream pointer.
 *
 * \param[in]  stream  The current position of the stream pointer.
 * \param[in]  value   The SDNV value to add to the stream.
 *
 * \par Notes: 
 *      1. Input parameters are passed as pointers to prevent wasted copies.
 *         Therefore, this function must be careful not to modify them.
 *      2. This function assumes that the stream is a character stream.
 *      3. We assume that we are not under such tight profiling constraints
 *         that sanity checks are too expensive.
 *****************************************************************************/

unsigned char *bsp_addSdnvToStream(unsigned char *stream, Sdnv* value)
{
   BSP_DEBUG_PROC("+ bsp_addSdnvToStream(%x, %x)",
                  (unsigned long) stream, (unsigned long) value);
  	
   if((stream != NULL) && (value != NULL) && (value->length > 0))
   {
      BSP_DEBUG_INFO("i bsp_addSdnvToStream: Adding %d bytes", value->length);
      memcpy(stream, value->text, value->length);
      stream += value->length;
   }
 
   BSP_DEBUG_PROC("- bsp_addSdnvToStream --> %x", (unsigned long) stream);
    
   return stream;	
}


/******************************************************************************
 *
 * \par Function Name: bsp_deserializeASB
 *
 * \par Purpose: This utility function accepts a serialized Abstract Security
 *               Block from a bundle during acquisition and places it in a
 *               AbstractSecurityBlock structure stored in the Acquisition
 *               Block's scratchpad area.
 *
 * \retval int -- 1 - An ASB was successfully deserialized into the scratchpad
 *                0 - The deserialized ASB did not pass its sanity check.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  A pointer to the acquisition block holding the
 *                      serialized abstract security block.
 *
 * \par Notes: 
 *      1. This function allocates memory using the MTAKE method.  This
 *         scratchpad must be freed by the caller iff the method does
 *         not return -1.  Any system error will release the memory.
 *
 *      2.  If we return a 1, the ASB is considered corrupted and not usable.
 *          The block should be discarded. It is still returned, though, so that
 *          the caller may examine it.
 *****************************************************************************/

int bsp_deserializeASB(AcqExtBlock *blk)
{
   int result = 1;
   
   BSP_DEBUG_PROC("+ bsp_deserializeASB(%x)", (unsigned long)blk);	
  
   /* Sanity Check 1: We have a block. */
   if(blk == NULL)
   {
      BSP_DEBUG_ERR("x bsp_deserializeASB: Bad parms: blk = %x",
                    (unsigned long) blk);
  	  result = -1;
   }
   /* Sanity Check 2: The block has a scratchpad. */
   else if((blk->object == NULL) || (blk->size == 0))
   {
      BSP_DEBUG_ERR("x bsp_deserializeASB: No Scratchpad: blk->size %d", 
                    blk->size);
      result = -1;    
   }
   /* Sanity Check 3: The block has a known type. */
   else if((blk->type != BSP_BAB_TYPE) &&
      (blk->type != BSP_PIB_TYPE) &&
      (blk->type != BSP_PCB_TYPE) &&
      (blk->type != BSP_ESB_TYPE))
   {
      BSP_DEBUG_WARN("? bsp_deserializeASB: Not a BSP block: %d", blk->type);
      result = 0;
   }
   /* If we pass sanity checks, we are good to deserialize. */
   else
   {
      BspBabScratchpad *scratch = (BspBabScratchpad *) blk->object;
      BspAbstractSecurityBlock *asb = (BspAbstractSecurityBlock *) 
                                      &(scratch->asb);
      unsigned char *cursor = NULL;
      int unparsedBytes = blk->length;

      /*
       * Position cursor to start of security-specific part of the block, which
       * is after the canonical block header area.  The security-specific part
       * of the block has length blk->dataLength, so it is easy to find the
       * start of the part given the overall block length.
       */
      cursor = ((unsigned char*)(blk->bytes)) + (blk->length - blk->dataLength);
 
      /* Extract block specifics, using cipherFlags as necessary. */
      extractSdnv(&(asb->cipher),      &cursor, &unparsedBytes);
      extractSdnv(&(asb->cipherFlags), &cursor, &unparsedBytes);
  
      BSP_DEBUG_INFO("i bsp_deserializeASB: cipher %ld, flags %ld, length %d", 
                     asb->cipher, asb->cipherFlags, blk->dataLength);
  
      if(asb->cipherFlags & BSP_ASB_CORR)
      {
         extractSdnv(&(asb->correlator), &cursor, &unparsedBytes);
         BSP_DEBUG_INFO("i bsp_deserializeASB: corr. = %ld", asb->correlator);
      }

      if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
      {
   	    /* TODO: Not implemented yet. */
      }
   
      if(asb->cipherFlags & BSP_ASB_RES)
      {
         extractSdnv(&(asb->resultLen), &cursor, &unparsedBytes);
         if(asb->resultLen == 0)
         {
            BSP_DEBUG_ERR("x bsp_deserializeASB: ResultLen is 0 with flags %ld",
                          asb->cipherFlags);
      	    result = 0;
         }
      
         else if((asb->resultData = MTAKE(asb->resultLen)) == NULL)
         {
            BSP_DEBUG_ERR("x bsp_deserializeASB: Failed allocating %ld bytes.", 
                          asb->resultLen);
      	    result = -1;
         }
      
         else
         {
      	    BSP_DEBUG_INFO("i bsp_deserializeASB: resultLen %ld", 
                           asb->resultLen);
            memcpy((char *)asb->resultData, cursor, asb->resultLen);
            cursor += asb->resultLen;
         
            result = 1;
         }
      }
   }
   
   BSP_DEBUG_PROC("- bsp_deserializeASB -> %d", result); 
            
   return result;		
}


#if 0
/******************************************************************************
 *
 * \par Function Name: bsp_eidNil
 *
 * \par Purpose: This utility function determines whether a given EID is
 *               "nil".  Nil in this case means that the EID is uninitialized
 *               or will otherwise resolve to the default nullEID. 
 *
 * \retval int - Whether the EID is nil (1) or not (0).
 *
 * \param[in]  eid - The EndpointID being checked.
 *
 * \par Notes: 
 *      1. Nil check is pulled from whether the ION library printEid function
 *         will use the default nullEID when given this EID. 
 *****************************************************************************/

int bsp_eidNil(EndpointId *eid)
{
   int result = 0;
  
   BSP_DEBUG_PROC("+ bsp_eidNil(%x)", (unsigned long) eid);
  
   /*
    * EIDs have two mutually exclusive representations, so pick right one to 
    * check.
    */
   if(eid->cbhe == 0)
   {
      BSP_DEBUG_INFO("i bsp_eidNil: scheme %d, nss %d",
                     eid->d.schemeNameOffset, eid->d.nssOffset);
              
      result = (eid->d.schemeNameOffset == 0) &&
               (eid->d.nssOffset == 0);     
   }
   else
   {
      BSP_DEBUG_INFO("i bsp_eidNil: node %ld, service %ld",
                     eid->c.nodeNbr, eid->c.serviceNbr);

      result = (eid->c.nodeNbr == 0);
   }

   BSP_DEBUG_PROC("- bsp_eidNil --> %d", result);
  
   return result;
}
#endif


/******************************************************************************
 *
 * \par Function Name: bsp_findAcqExtBlk
 *
 * \par Purpose: This utility function finds an acquisition extension block
 *               from within the work area.
 *
 * \retval AcqExtBlock * -- The found block, or NULL.
 *
 * \param[in]  wk      - The work area holding the blocks.
 * \param[in]  listIdx - Whether we want to look in the pre- or post- payload
 *                       area for the block.
 * \param[in[  type    - The block type.
 * 
 * \par Notes: 
 *      1. This function should be moved into libbpP.c
 *****************************************************************************/

AcqExtBlock *bsp_findAcqExtBlk(AcqWorkArea *wk, int listIdx, int type)
{
   LystElt      elt;
   AcqExtBlock  *result = NULL;
    
   BSP_DEBUG_PROC("+ bsp_findAcqExtBlk(%x, %d, %d",
                  (unsigned long) wk, listIdx, type);
                
   for (elt = lyst_first(wk->extBlocks[listIdx]); elt; elt = lyst_next(elt))
   {
      result = (AcqExtBlock *) lyst_data(elt);
     
      BSP_DEBUG_INFO("i bsp_findAcqExtBlk: Found type %d", result->type);
      if(result->type == type)
      {
         break;
      }
     
      result = NULL;
   }

   BSP_DEBUG_PROC("- bsp_findAcqExtBlk -- %x", (unsigned long) result);
            
   return result;
}


/******************************************************************************
 *
 * \par Function Name: bsp_retrieveKey
 *
 * \par Purpose: Retrieves the key associated with a particular keyname.
 *
 * \retval char * -- !NULL - A pointer to a buffer holding the key value.
 *                    NULL - There was a system error.
 *
 * \param[out] keyLen   The length of the key value that was found.
 * \param[in]  keyName  The name of the key to find.
 *
 * \par Notes: 
 *****************************************************************************/

unsigned char *bsp_retrieveKey(int *keyLen, char *keyName)
{
   unsigned char *keyValueBuffer = NULL;
   char c = ' ';
  
   BSP_DEBUG_PROC("+ bsp_retrieveKey(%d, %s)", *keyLen, keyName);	

   /* 
    * We pass in a key length of zero.  This should result in the sec-get_key
    * function populating this value with the key length.  We cannot pass in a 
    * value of NULL for the keyValueBuffer, so we pass in a single char pointer.
    */
   *keyLen = 0;
   if(sec_get_key(keyName, keyLen, &c) != 0)
   {
      BSP_DEBUG_ERR("bsp_retrieveKey:  Unable to return length of key %s.", 
                    keyName);
   }
  
   /* If no key length, the key must not have been found.*/
   else if(*keyLen == 0)
   {
      BSP_DEBUG_ERR("x bsp_retrieveKey: Unable to find key %s", keyName);	
   }
  
   else if((keyValueBuffer = (unsigned char *) MTAKE(*keyLen)) == NULL)
   {
      BSP_DEBUG_ERR("x bsp_retrieveKey: Failed to allocate %d bytes", *keyLen); 
   }
  
   /* Now we have key length and allocated buffer, so get key. */
   else if(sec_get_key(keyName, keyLen, (char *) keyValueBuffer) != *keyLen)
   {
      BSP_DEBUG_ERR("bsp_retrieveKey:  Can't get key %s", keyName);
  	  MRELEASE(keyValueBuffer);
      keyValueBuffer = NULL;
   }
  
   BSP_DEBUG_PROC("- bsp_retrieveKey - (begins with) %.*s", MIN(128, *keyLen),
		   keyValueBuffer);	
  
   return keyValueBuffer;
}


/******************************************************************************
 *
 * \par Function Name: bsp_serializeASB
 *
 * \par Purpose: Serializes an abstract security block and returns the 
 *               serialized representation.
 *
 * \retval unsigned char * - the serialized Abstract Security Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  asb    The BspAbstractSecurityBlock to serialize.
 *
 * \par Notes: 
 *      1. This function uses MTAKE to allocate space for the result. This
 *         result (if not NULL) must be freed using MRELEASE. 
 *      2. This function only serializes the "security specific" ASB, not the
 *         canonical header information of the encapsulating BP extension block.
 *****************************************************************************/

unsigned char *bsp_serializeASB(unsigned int *length,      
                                BspAbstractSecurityBlock *asb)
{
   Sdnv cipherFlags;
   Sdnv cipher;
   Sdnv correlator;
   Sdnv resultLen;
   unsigned char *result = NULL;
  
   BSP_DEBUG_PROC("+ bsp_serializeASB(%x, %x)", 
                  (unsigned long) length, (unsigned long) asb);	
  
   /*
    * Sanity check. We should have an ASB and the block should not already
    * have a serialized version of itself.
    */
   if((asb == NULL) || (length == NULL))
   {
     BSP_DEBUG_ERR("x bsp_serializeASB:  Parameters are missing. Asb is %x", 
                   (unsigned long) asb);
     BSP_DEBUG_PROC("- bsp_serializeASB --> %s","NULL");
     return NULL;
   }
   
   /***************************************************************************
    *                       Calculate the BLock Length                        *
    ***************************************************************************/

   /*
    * We need to assign all of our SDNV values first so we know how many
    * bytes they will take up. We don't want a separate function to calcuate
    * this length as it would result in generating multiple SDNV values 
    * needlessly. 
    */
   
   encodeSdnv(&cipherFlags, asb->cipherFlags);
   encodeSdnv(&cipher, asb->cipher);
   encodeSdnv(&correlator, asb->correlator);
   encodeSdnv(&resultLen, asb->resultLen);

   *length = cipherFlags.length + cipher.length;
   
   if(asb->cipherFlags & BSP_ASB_CORR)
   {
   	  *length += correlator.length;
   }

   if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
   {
   	  /* TODO: Not implemented yet. */
   }
   
   if(asb->cipherFlags & BSP_ASB_RES)
   {
   	  *length += resultLen.length + asb->resultLen;
   }

   /***************************************************************************
    *                Serialize the ASB into the allocated buffer              *
    ***************************************************************************/
   if((result = MTAKE(*length)) == NULL)
   {
      BSP_DEBUG_ERR("x bsp_serializeBlock:  Unable to malloc %d bytes.", 
                    *length);
      *length = 0;
      result = NULL;
   }
   else
   {
      unsigned char *cursor = result;
    
      cursor = bsp_addSdnvToStream(cursor, &cipher);
      cursor = bsp_addSdnvToStream(cursor, &cipherFlags);

      if(asb->cipherFlags & BSP_ASB_CORR)
      {
         cursor = bsp_addSdnvToStream(cursor, &correlator);
      }

      if(asb->cipherFlags & BSP_ASB_HAVE_PARM)
      {
   	     /* TODO: Not implemented yet. */
      }
   
      if(asb->cipherFlags & BSP_ASB_RES)
      {
         cursor = bsp_addSdnvToStream(cursor, &resultLen);
   	 BSP_DEBUG_INFO("i bsp_serializeASB: cursor %x, data %x, length %ld",
   	                (unsigned long)cursor, (unsigned long) asb->resultData, 
                        asb->resultLen);
         if(asb->resultData != NULL)
         {
            memcpy(cursor, (char *)asb->resultData, asb->resultLen);
         }
         else
         {
            memset(cursor,0, asb->resultLen);  
         } 
         cursor += asb->resultLen;
      }

      /* Sanity check */
      if( (unsigned long)(cursor - result) > (*length + 1)) 
      {
         BSP_DEBUG_ERR("x bsp_serializeASB: Check failed. Length is %d not %d", 
                       cursor-result, *length + 1);
   	     MRELEASE(result);
   	     *length = 0;
   	     result = NULL;
      }
   }
    
   BSP_DEBUG_PROC("- bsp_serializeASB -> data: %x, length %d", 
                  (unsigned long) result, *length);	

   return result;
}


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
 * \par Function Name: getBspItem
 *
 * \par Purpose: This function searches within a BSP buffer (a ciphersuite
 *		 parameters field or a security result field) for an
 *		 information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in bsp.h
 *                         as BSP_CSPARM_xxx macros.
 * \param[in]  bspBuf      The data buffer in which to search for the item.
 * \param[in]  bspLen      The length of the data buffer.
 * \param[in]  val         A pointer to a variable in which the function
 *                         should place the location (within the buffer)
 *                         of the first item of specified type that is
 *                         found within the buffer.  On return, this
 *                         variable contains NULL if no such item was found.
 * \param[in]  len         A pointer to a variable in which the function
 *                         should place the length of the first item of
 *                         specified type that is found within the buffer.
 *                         On return, this variable contains 0 if no such
 *                         item was found.
 *
 * \par Notes: 
 *****************************************************************************/

void	getBspItem(int itemNeeded, unsigned char *bspBuf,
			unsigned long bspLen, unsigned char **val,
			unsigned long *len)
{
	unsigned char	*cursor = bspBuf;
	unsigned char	itemType;
	int		sdnvLength;
	unsigned long	itemLength;

	*val = NULL;		/*	Default.			*/
	*len = 0;		/*	Default.			*/

	/*	Walk through all items in the buffer (either a security
	 *	parameters field or a security result field, searching
	 *	for an item of the indicated type.			*/

	while (bspLen > 0)
	{
		itemType = *cursor;
		cursor++;
		bspLen--;
		if (bspLen == 0)	/*	No item length.		*/
		{
			return;		/*	Malformed result data.	*/
		}

		sdnvLength = decodeSdnv(&itemLength, cursor);
		if (sdnvLength == 0 || sdnvLength > bspLen)
		{
			return;		/*	Malformed result data.	*/
		}

		cursor += sdnvLength;
		bspLen -= sdnvLength;
		if (itemLength == 0)	/*	Empty item.		*/
		{
			continue;
		}

		if (itemType == itemNeeded)
		{
			*val = cursor;
			*len = itemLength;
			return;
		}

		/*	Look at next item in result data.		*/

		cursor += itemLength;
		bspLen -= itemLength;
	}
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
 * \par Function Name: bsp_babPreAcquire
 *
 * \par Purpose: This callback is called when a pre-payload bunde received by 
 *               the ION library.  This function will rely on the general
 *               helper function bsp_babAcquire to deserialize the block. 
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
 *      1. The pre-payload BAB block is the FIRST extension block after the
 *         primary block in the bundle. 
 *****************************************************************************/

int	bsp_babPreAcquire(AcqExtBlock *blk, AcqWorkArea *wk)
{
   BSP_DEBUG_PROC("+ bsp_babPreAcquire(%x, %x)", 
                  (unsigned long) blk, (unsigned long) wk);
   return bsp_babAcquire(blk, wk);
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
	BSP_DEBUG_INFO("Byte %d is %x", i, rawBundle[i]);
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
   BSP_DEBUG_PROC("+ bsp_babGetSecurityInfo(%x %d, %s, %x)",
	(unsigned long) bundle,which,eidString,(unsigned long) scratch);

   /* By default, we disable BAB processing */
   scratch->useBab = 0;
   scratch->cipherKeyName[0] = '\0';  	 

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
