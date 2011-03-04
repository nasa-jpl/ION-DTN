/*
 * extbspbab.h
 *
 *  Created on: Jul 5, 2010
 *      Author: birraej1
 */

#ifndef EXTBSPBAB_H_
#define EXTBSPBAB_H_

#include "extbsputil.h"

/** BAB Scratchpad Receive Flags */
#define BSP_BABSCRATCH_RXFLAG_CORR 1 /** Block's correlated block was found */

/** BAB Constants */
#define BAB_HMAC_SHA1_RESULT_LEN 20
#define BAB_CORRELATOR 0xED
#define BSP_BAB_BLOCKING_SIZE 4096


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
	unsigned long correlator;
	unsigned long cipher;
	char cipherKeyName[BSP_KEY_NAME_LEN]; /** Cipherkey name used by this block.*/
	unsigned long rxFlags;        /** RX-side processing flags for this block. */
	int hmacLen;
	char expectedResult[BAB_HMAC_SHA1_RESULT_LEN];
} BspBabCollaborationBlock;



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
 * \par Date Written:  6/15/09
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
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/15/09  E. Birrane           Initial Implementation.
 *  06/20/09  E. Birrane           Comment updates for initial release.
 *****************************************************************************/

int bsp_babAcquire(AcqExtBlock *blk, AcqWorkArea *wk);


/******************************************************************************
 *
 * \par Function Name: bsp_babClear
 *
 * \par Purpose: This callback removes all memory allocated by the BSP module
 *               during the block's acquisition process. This function is the
 *               same for both PRE and POST payload blocks.
 *
 * \par Date Written:  6/04/09
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
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/04/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *****************************************************************************/

void bsp_babClear(AcqExtBlock *blk);


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
 * \par Date Written:  5/30/09
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
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Comment updates for initial release.
 *****************************************************************************/

int  bsp_babOffer(ExtensionBlock *blk, Bundle *bundle);


/******************************************************************************
 *
 * \par Function Name: bsp_babPostCheck
 *
 * \par Purpose: This callback checks a post-payload block, upon bundle receipt
 *               to determine whether the block should be considered authentic.
 *               For a BAB, this implies that the security result encoded in
 *               this block is the correct hash for the bundle.
 *
 * \par Date Written:  6/03/09
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
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/03/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Comment updated for initial release.
 *****************************************************************************/

int  bsp_babPostCheck(AcqExtBlock *post_blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_babPostProcessOnDequeue
 *
 * \par Purpose: This callback constructs the post-payload BAB block for
 *               inclusion in a bundle right before the bundle is transmitted.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - The post-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  post_blk  The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 * \param[in]      ctxt      Unused.
 *
 * \par Notes:
 *      1. No other blocks will be added to the bundle, and no existing blocks
 *         in the bundle will be modified.  This must be the last block
 *         modification performed on the bundle before it is transmitted.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Comment/Debug updated for initial release.
 *****************************************************************************/

int bsp_babPostProcessOnDequeue(ExtensionBlock *post_blk,
                                Bundle *bundle,
                                void *ctxt);


/*****************************************************************************
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
 * \par Date Written:  5/18/09
 *
 * \retval int 0 - The post-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk     The block whose abstract security block structure
 *                         will be populated and then serialized into the
 *                         bundle.
 * \param[in]      bundle  The bundle holding the block.
 * \param[in]      ctxt    Unused.
 *
 *
 * \par Notes:
 *      1. We assume that the post-payload BAB block is the last block of the
 *         bundle.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/18/09  E. Birrane           Initial Implementation.
 *  06/20/09  E. Birrane           Comment/Debug updated for initial release.
 *****************************************************************************/

int  bsp_babPostProcessOnTransmit(ExtensionBlock *blk,
                                  Bundle *bundle,
                                  void *ctxt);


/******************************************************************************
 *
 * \par Function Name: bsp_babPreCheck
 *
 * \par Purpose: This callback checks a pre-payload block, upon bundle receipt
 *               to determine whether the block should be considered authentic.
 *               For a BAB, this is really just a sanity check on the block,
 *               making sure that the block fields are consistent.
 *
 * \par Date Written:  6/03/09
 *
 * \retval int 0 - The block check was inconclusive
 *             1 - The block check failed.
 *             2 - The block check succeeed.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and
 *                  the rest of the received bundle.
 *
 * \par Notes:
 *      1. We assume that there is only 1 BAB block pair in a bundle.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/03/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Cmt/Debug for initial release.
 *****************************************************************************/

int  bsp_babPreCheck(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_babPreProcessOnDequeue
 *
 * \par Purpose: This callback constructs the pre-payload BAB block for
 *               inclusion in a bundle right before the bundle is transmitted.
 *               This means constructing the abstract security block and
 *               serializing it into the block's bytes array.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - The pre-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk     The block whose abstract security block structure will
 *                         be populated and then serialized into the block's
 *                         bytes array.
 * \param[in]      bundle  The bundle holding the block.
 *
 * \param[in]      ctxt    Unused.
 *
 *
 * \par Notes:
 *      1.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Debug/Cmt update for initial release.
 *****************************************************************************/
int  bsp_babPreProcessOnDequeue(ExtensionBlock *blk,
                                Bundle *bundle,
                                void *ctxt);



/******************************************************************************
 *
 * \par Function Name: bsp_babRelease
 *
 * \par Purpose: This callback removes memory allocated by the BSP module
 *               from a particular extension block.
 *
 * \par Date Written:  5/30/09
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated memory pools must be
 *                      released.
 *
 * \par Notes:
 *      1. This same function is used for pre- and post- payload blocks.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Debug/Cmt update for initial release.
 *****************************************************************************/
void bsp_babRelease(ExtensionBlock *blk);


unsigned char *bsp_babGetSecResult(Object dataObj,
                                   unsigned long dataLen,
                                   char *cipherKeyName,
					     unsigned long keyLen,
                                   unsigned long *hashLen);




#endif /* EXTBSPBAB_H_ */
