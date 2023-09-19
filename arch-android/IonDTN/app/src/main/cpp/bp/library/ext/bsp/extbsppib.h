/*
 * extbsppib.h
 */

#ifndef EXTBSPPIB_H_
#define EXTBSPPIB_H_

#include "extbsputil.h"


/** PIB Constants */
#define BSP_PIB_HMAC_SHA256_RESULT_LEN 32
#define BSP_PIB_BLOCKING_SIZE     4096

// If bsp debugging is turned on, then turn on pib
#if BSP_DEBUGGING == 1
#define PIB_DEBUGGING 1
#endif

#ifndef PIB_DEBUGGING
#define PIB_DEBUGGING 0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define PIB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define PIB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define PIB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define PIB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define PIB_DEBUG_LVL   PIB_DEBUG_LVL_PROC

#define GMSG_BUFLEN     256
#if PIB_DEBUGGING == 1

/**
 * \def PIB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the PIB library and is useful for confirming control flow
 *    through the PIB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by BSP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the PIB module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * PIB_DEBUGGING #define.
 */

   #define PIB_DEBUG(level, format,...) if(level >= PIB_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); printf("%s\n", gMsg);}

   #define PIB_DEBUG_PROC(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define PIB_DEBUG_INFO(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define PIB_DEBUG_WARN(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define PIB_DEBUG_ERR(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define PIB_DEBUG(level, format,...) if(level >= PIB_DEBUG_LVL) \
{}

   #define PIB_DEBUG_PROC(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define PIB_DEBUG_INFO(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define PIB_DEBUG_WARN(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define PIB_DEBUG_ERR(format,...) \
           PIB_DEBUG(PIB_DEBUG_LVL_ERR,format, __VA_ARGS__)

#endif



/*****************************************************************************
 *                     PIB EXTENSIONS INTERFACE FUNCTIONS                    *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_pibAcquire
 *
 * \par Purpose: This callback is called when a serialized PIB bundle is
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
 *****************************************************************************/

int bsp_pibAcquire(AcqExtBlock *blk, AcqWorkArea *wk);


/******************************************************************************
 *
 * \par Function Name: bsp_pibClear
 *
 * \par Purpose: This callback removes all memory allocated by the BSP module
 *               during the block's acquisition process.
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
 *****************************************************************************/

void bsp_pibClear(AcqExtBlock *blk);


/******************************************************************************
 *
 * \par Function Name: bsp_pibCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a PIB block
 * 		 to a new block that is a copy of the original.
 *
 * \par Date Written:  4/02/12
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  newBlk The new copy of this extension block.
 * \param[in]      oldBlk The original extension block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  04/02/12  S. Burleigh           Initial Implementation.
 *****************************************************************************/

extern int bsp_pibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk);


/******************************************************************************
 *
 * \par Function Name: bsp_pibOffer
 *
 * \par Purpose: This callback determines whether a PIB block is necessary for
 *               this particular bundle, based on local security policies. If
 *               a PIB block is necessary, a scratchpad structure will be
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
 *****************************************************************************/

int  bsp_pibOffer(ExtensionBlock *blk, Bundle *bundle);


/******************************************************************************
 *
 * \par Function Name: bsp_pibCheck
 *
 * \par Purpose: This callback checks a PIB block, upon bundle receipt,
 *               to determine whether the block should be considered authentic.
 *               For a PIB, this implies that the security result encoded in
 *               this block is the correct hash for the payload.
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
 *****************************************************************************/

int  bsp_pibCheck(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_pibProcessOnDequeue
 *
 * \par Purpose: This callback constructs the PIB block for
 *               inclusion in a bundle right before the bundle is transmitted.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - The block was successfully created
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
 *****************************************************************************/

int bsp_pibProcessOnDequeue(ExtensionBlock *blk,
                            Bundle *bundle,
                            void *ctxt);

/******************************************************************************
 *
 * \par Function Name: bsp_pibRelease
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
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *****************************************************************************/
void bsp_pibRelease(ExtensionBlock *blk);


unsigned char *bsp_pibGetSecResult(Object dataObj,
                                   unsigned int dataLen,
                                   char *cipherKeyName,
					               unsigned int keyLen,
                                   unsigned int *hashLen);




#endif /* EXTBSPPIB_H_ */
