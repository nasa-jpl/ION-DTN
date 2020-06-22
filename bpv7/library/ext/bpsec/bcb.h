/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bcb.h
 **
 ** Description: Definitions supporting generic processing of BCB blocks.
 **              This includes both the BCB Interface to the ION bpsec
 **              API as well as a default implementation of the BCB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BCB bpsec Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              bcbOffer
 **              bcbProcessOnDequeue
 **              bcbRelease
 **              bcbCopy
 **                                                  bcbAcquire
 **                                                  bcbReview
 **                                                  bcbDecrypt
 **                                                  bcbRecord
 **                                                  bcbClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    SIGN SIDE                     VERIFY SIDE
 **
 **              bcbDefaultConstruct
 **              bcbDefaultSign
 **
 **                                              bcbDefaultConstruct
 **                                              bcbDefaultSign
 **                                              bcbDefaultVerify
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/05/10  E. Birrane     Implementation as extsbspbcb.c (JHU/APL)
 **            S. Burleigh    Implementation as sbspbcb.c for Sbsp
 **  11/07/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 **  09/02/19  S. Burleigh    Rename everything for bpsec
 *****************************************************************************/

#ifndef BCB_H_
#define BCB_H_

#include "bpsec_util.h"
#include "profiles.h"
#include "csi.h"

#define BCB_FILENAME			"bcb_tmpfile"
#define	MAX_TEMP_FILES_PER_SECOND	5

// If bpsec debugging is turned on, then turn on bcb debugging.
#if DEBUGGING == 1
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
#if (BCB_DEBUGGING == 1)

/**
 * \def BCB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the BCB library and is useful for confirming control flow
 *    through the BCB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by bpsec module software engineers.
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

extern int		bcbAcquire(AcqExtBlock *blk,
						AcqWorkArea *wk);

extern int		bcbRecord(ExtensionBlock *newBlk,
						AcqExtBlock *oldBlk);

extern void		bcbClear(AcqExtBlock *blk);

extern int		bcbCopy(ExtensionBlock *newBlk,
						ExtensionBlock *oldBlk);

extern int		bcbReview(AcqWorkArea *wk);

extern int		bcbDecrypt(AcqExtBlock *blk,
						AcqWorkArea *wk);

extern int		bcbDefaultConstruct(uint32_t suite,
						ExtensionBlock *blk,
						BpsecOutboundBlock *asb);

extern int		bcbDefaultDecrypt(uint32_t suite,
	       					AcqWorkArea *wk,
						AcqExtBlock *blk,
						uvast *bytes,
						char *fromEid);

extern uint32_t		bcbDefaultEncrypt(uint32_t suite,
						Bundle *bundle,
						ExtensionBlock *blk,
						BpsecOutboundBlock *asb,
						size_t xmitRate,
						uvast *bytes,
						char *toEid);

extern BcbProfile	*bcbGetProfile(char *secSrc,
						char *secDest,
						int secTgtType,
						BPsecBcbRule *secBcbRule);

extern  int		bcbHelper(Object *dataObj,
						uint32_t chunkSize,
						uint32_t suite,
						sci_inbound_tlv key,
						sci_inbound_parms parms,
						uint8_t encryptInPlace,
						size_t xmitRate,
						uint8_t function);

extern int		bcbOffer(ExtensionBlock *blk, Bundle *bundle);

extern int		bcbProcessOnDequeue(ExtensionBlock *blk,
						Bundle *bundle,
						void *parm);

extern void		bcbRelease(ExtensionBlock *blk);

#endif /* BCB_H_ */
