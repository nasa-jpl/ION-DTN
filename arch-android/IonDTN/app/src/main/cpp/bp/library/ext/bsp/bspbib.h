/*
 * extbspbib.h
 */

#ifndef BSPBIB_H_
#define BSPBIB_H_

#include "bsputil.h"
#include "ciphersuites.h"

// If bsp debugging is turned on, then turn on bib debugging.
#if BSP_DEBUGGING == 1
#define BIB_DEBUGGING 1
#endif

#ifndef BIB_DEBUGGING
#define BIB_DEBUGGING 0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BIB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BIB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BIB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BIB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BIB_DEBUG_LVL   BIB_DEBUG_LVL_PROC

#define GMSG_BUFLEN     256
#if BIB_DEBUGGING == 1

/**
 * \def BIB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the BIB library and is useful for confirming control flow
 *    through the BIB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by BSP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the BIB module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * BIB_DEBUGGING #define.
 */

   #define BIB_DEBUG(level, format,...) if(level >= BIB_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}

   #define BIB_DEBUG_PROC(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BIB_DEBUG_INFO(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BIB_DEBUG_WARN(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BIB_DEBUG_ERR(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define BIB_DEBUG(level, format,...) if(level >= BIB_DEBUG_LVL) \
{}

   #define BIB_DEBUG_PROC(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BIB_DEBUG_INFO(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BIB_DEBUG_WARN(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BIB_DEBUG_ERR(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_ERR,format, __VA_ARGS__)

#endif

/*****************************************************************************
 *                     BIB EXTENSIONS INTERFACE FUNCTIONS                    *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_bibOffer
 *
 * \par Purpose: This callback aims to ensure the the bundle contains a
 * 		 BIB for a specified block.  See the bspbib.c comments
 * 		 for details.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - A tentative BIB was successfully created, or not needed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be added to the bundle.
 * \param[in]      bundle The bundle that would hold this block.
 *
 * \par Notes:
 *      1. Setting the length and size fields to 0 will result in the block
 *         NOT being added to the bundle.  This is how we refuse creation
 *         of the new BIB.
 *      2. All block memory is allocated using sdr_malloc.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *****************************************************************************/

int	bsp_bibOffer(ExtensionBlock *blk, Bundle *bundle);

/******************************************************************************
 *
 * \par Function Name: bsp_bibProcessOnDequeue
 *
 * \par Purpose: This callback determines whether or not a block of the
 * 		 type that is the target of this proposed BIB exists in
 * 		 the bundle and discards the BIB if it does not.  If the
 * 		 target block exists, the BIB is constructed.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - The block was successfully constructed or deleted.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that might be constructed.
 * \param[in]      bundle The bundle holding the block.
 * \param[in]      parm   The dequeue context.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *****************************************************************************/

int	bsp_bibProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
		void *parm);

/******************************************************************************
 *
 * \par Function Name: bsp_bibRelease
 *
 * \par Purpose: This callback removes heap space allocated by the BSP module
 *               for a particular block integrity block.
 *
 * \par Date Written:  5/30/09
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated heap space must be
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

void	bsp_bibRelease(ExtensionBlock *blk);

/******************************************************************************
 *
 * \par Function Name: bsp_bibCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a BIB
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

extern int bsp_bibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk);

/******************************************************************************
 *
 * \par Function Name: bsp_bibParse
 *
 * \par Purpose: This callback is called for each serialized BIB that was
 * 		 encountered during bundle reception and subsequently
 *               decrypted (as necessary).  This callback will deserialize
 *               the block into a scratchpad object.
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
 * \param[in]      wk   The work area associated with this block acquisition.
 *
 * \par Notes:
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/15/09  E. Birrane           Initial Implementation.
 *****************************************************************************/

int bsp_bibParse(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_bibCheck
 *
 * \par Purpose: This callback determine whether or not a BIB's security
 * 		 target block has been corrupted en route.  Specifically,
 *               a newly computed hash for the security target block
 *               must match the security result encoded in the BIB.
 *
 * \par Date Written:  6/03/09
 *
 * \retval int 1 - The block was not found to be corrupt.
 *             0 - The block was found to be corrupt.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The BIB whose target must be checked.
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

int  bsp_bibCheck(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_bibClear
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

void bsp_bibClear(AcqExtBlock *blk);

#endif /* BSPBIB_H_ */
