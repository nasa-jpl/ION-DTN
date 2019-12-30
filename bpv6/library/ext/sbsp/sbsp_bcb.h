/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: sbsp_bcb.h
 **
 ** Description: Definitions supporting generic processing of BCB blocks.
 **              This includes both the BCB Interface to the ION SBSP
 **              API as well as a default implementation of the BCB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BIB SBSP Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              sbsp_bcbOffer
 **              sbsp_bcbProcessOnDequeue
 **              sbsp_bcbRelease
 **              sbsp_bcbCopy
 **                                                  sbsp_bcbAcquire
 **                                                  sbsp_bcbReview
 **                                                  sbsp_bcbDecrypt
 **                                                  sbsp_bcbRecord
 **                                                  sbsp_bcbClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    SIGN SIDE                     VERIFY SIDE
 **
 **              sbsp_bcbDefaultConstruct
 **              sbsp_bcbDefaultSign
 **
 **                                              sbsp_bcbDefaultConstruct
 **                                              sbsp_bcbDefaultSign
 **                                              sbsp_bcbDefaultVerify
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/05/10  E. Birrane     Implementation as extsbspbib.c (JHU/APL)
 **            S. Burleigh    Implementation as sbspbib.c for Sbsp
 **  11/07/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef SBSP_BCB_H_
#define SBSP_BCB_H_

#include "sbsp_util.h"
#include "profiles.h"
#include "csi.h"

#define BCB_FILENAME			"bcb_tmpfile"
#define	MAX_TEMP_FILES_PER_SECOND	5

// If sbsp debugging is turned on, then turn on bcb debugging.
#if SBSP_DEBUGGING == 1
#define BCB_DEBUGGING 1
#endif

#ifndef BCB_DEBUGGING
#define BCB_DEBUGGING 0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BCB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BCB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BCB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BCB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BCB_DEBUG_LVL   BCB_DEBUG_LVL_ERR

#define GMSG_BUFLEN     256
#if BCB_DEBUGGING == 1

/**
 * \def BCB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the BCB library and is useful for confirming control flow
 *    through the BCB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by sbsp module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the BCB module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * BCB_DEBUGGING #define.
 */

   #define BCB_DEBUG(level, format,...) if(level >= BCB_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}

   #define BCB_DEBUG_PROC(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BCB_DEBUG_INFO(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BCB_DEBUG_WARN(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BCB_DEBUG_ERR(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define BCB_DEBUG(level, format,...) if(level >= BCB_DEBUG_LVL) \
{}

   #define BCB_DEBUG_PROC(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BCB_DEBUG_INFO(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BCB_DEBUG_WARN(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BCB_DEBUG_ERR(format,...) \
           BCB_DEBUG(BCB_DEBUG_LVL_ERR,format, __VA_ARGS__)

#endif


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/

extern int		sbsp_bcbAcquire(AcqExtBlock *blk,
						AcqWorkArea *wk);

extern void		sbsp_bcbClear(AcqExtBlock *blk);

extern int		sbsp_bcbCopy(ExtensionBlock *newBlk,
						ExtensionBlock *oldBlk);

extern int		sbsp_bcbReview(AcqWorkArea *wk);

extern int		sbsp_bcbDecrypt(AcqExtBlock *blk,
						AcqWorkArea *wk);

extern int		sbsp_bcbDefaultConstruct(uint32_t suite,
						ExtensionBlock *blk,
						SbspOutboundBlock *asb);

extern int		sbsp_bcbDefaultDecrypt(uint32_t suite,
	       					AcqWorkArea *wk,
						AcqExtBlock *blk,
						uvast *bytes);

extern uint32_t		sbsp_bcbDefaultEncrypt(uint32_t suite,
						Bundle *bundle,
						ExtensionBlock *blk,
						SbspOutboundBlock *asb,
						size_t xmitRate,
						uvast *bytes);

extern BcbProfile	*sbsp_bcbGetProfile(char *secSrc,
						char *secDest,
						int secTgtType,
						BspBcbRule *secBcbRule);

extern  int		sbsp_bcbHelper(Object *dataObj,
						uint32_t chunkSize,
						uint32_t suite,
						csi_val_t key,
						csi_cipherparms_t parms,
						uint8_t encryptInPlace,
						size_t xmitRate,
						uint8_t function);

extern int		sbsp_bcbOffer(ExtensionBlock *blk, Bundle *bundle);

extern int		sbsp_bcbProcessOnDequeue(ExtensionBlock *blk,
						Bundle *bundle,
						void *parm);

extern void		sbsp_bcbRelease(ExtensionBlock *blk);

#endif /* SBSP_BCB_H_ */
