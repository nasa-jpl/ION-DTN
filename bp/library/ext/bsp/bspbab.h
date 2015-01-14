/*
 * bspbab.h
 *
 *  Created on: Jul 5, 2010
 *      Author: birraej1
 */

#ifndef BSPBAB_H_
#define BSPBAB_H_

#include "bsputil.h"
#include "ciphersuites.h"

/** BAB Scratchpad Receive Flags */
#define BSP_BABSCRATCH_RXFLAG_CORR 1 /** Block's correlated block was found */

// If bsp debugging is turned on, then turn on bab debugging.
#if BSP_DEBUGGING == 1
#define BAB_DEBUGGING 1
#endif

#ifndef BAB_DEBUGGING
#define BAB_DEBUGGING 0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BAB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BAB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BAB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BAB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BAB_DEBUG_LVL   BAB_DEBUG_LVL_PROC

#define GMSG_BUFLEN     256
#if BAB_DEBUGGING == 1

/**
 * \def BAB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the BAB library and is useful for confirming control flow
 *    through the BAB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by BSP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the BAB module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * BAB_DEBUGGING #define.
 */

   #define BAB_DEBUG(level, format,...) if(level >= BAB_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}

   #define BAB_DEBUG_PROC(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BAB_DEBUG_INFO(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BAB_DEBUG_WARN(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BAB_DEBUG_ERR(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define BAB_DEBUG(level, format,...) if(level >= BAB_DEBUG_LVL) \
{}

   #define BAB_DEBUG_PROC(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BAB_DEBUG_INFO(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BAB_DEBUG_WARN(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BAB_DEBUG_ERR(format,...) \
           BAB_DEBUG(BAB_DEBUG_LVL_ERR,format, __VA_ARGS__)

#endif

/*****************************************************************************
 *                     BAB EXTENSIONS INTERFACE FUNCTIONS                    *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_babOffer
 *
 * \par Purpose: This callback constructs a BAB block.  See the bspbab.c
 * 		 comments for details.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that is to be added to the bundle.
 * \param[in]      bundle The bundle that will hold this block.
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
 *  06/20/09  E. Birrane           Comment updates for initial release.
 *****************************************************************************/

int	bsp_babOffer(ExtensionBlock *blk, Bundle *bundle);

/******************************************************************************
 *
 * \par Function Name: bsp_babProcessOnDequeue
 *
 * \par Purpose: This callback constructs a BAB block for inclusion
 * 		 in a bundle right before the bundle is transmitted.
 * 		 See bspbab.c comments for details.
 *
 * \par Date Written:  5/30/09
 *
 * \retval int 0 - The BAB block was successfully created or deleted.
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk     The block whose abstract security block structure
 * 			   will be populated and then serialized into the
 * 			   block's bytes array.
 * \param[in]      bundle  The bundle holding the block.
 *
 * \param[in]      ctxt    The dequeue context.
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

int	bsp_babProcessOnDequeue(ExtensionBlock *blk, Bundle *bundle,
		void *ctxt);

/*****************************************************************************
 *
 * \par Function Name: bsp_bab_1_ProcessOnTransmit
 *
 * \par Purpose: This callback is the last operation on each BAB block.
 * 		 See bspbab.c comments for details.
 *
 * \par Date Written:  5/18/09
 *
 * \retval int 0 - The lone BAB block was successfully replaced, or the
 * 		   the first BAB was retained.
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
 *      1. For a lone BAB, this must be the last block modification performed
 *         on the bundle before it is transmitted.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/18/09  E. Birrane           Initial Implementation.
 *  06/20/09  E. Birrane           Comment/Debug updated for initial release.
 *****************************************************************************/

int	bsp_babProcessOnTransmit(ExtensionBlock *blk, Bundle *bundle,
		void *ctxt);

/******************************************************************************
 *
 * \par Function Name: bsp_babRelease
 *
 * \par Purpose: This callback releases heap space allocated to
 * 		 a BSP extension block.
 *
 * \par Date Written:  5/30/09
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated heap space must be
 *                      released.
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

void	bsp_babRelease(ExtensionBlock *blk);

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

int	bsp_babAcquire(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_babCheck
 *
 * \par Purpose: This callback checks a BAB received in a bundle.
 * 		 See the bspbab.c comments for details.
 *
 * \par Date Written:  6/03/09
 *
 * \retval int 0 - The block check was inconclusive.
 *             1 - The block check failed: inauthentic.
 *             2 - The block check succeeded: authentic.
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and
 *                  the rest of the received bundle.
 *
 * \par Notes:
 *      1.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/03/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Cmt/Debug for initial release.
 *****************************************************************************/

int	bsp_babCheck(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_babClear
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
 *  06/06/09  E. Birrane           Documentation Pass.
 *****************************************************************************/

void	bsp_babClear(AcqExtBlock *blk);

#endif /* BSPBAB_H_ */
