/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

//TODO: Update description
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
#include "bpsec_asb.h"
#include "sci.h"
#include "csi.h"


// TODO: Consider removing these debug macros?

// If bpsec debugging is turned on, then turn on bcb debugging.
#if DEBUGGING == 1
#define BCB_DEBUGGING			1
#endif

#ifndef BCB_DEBUGGING
#define BCB_DEBUGGING 1  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BCB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BCB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BCB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BCB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BCB_DEBUG_LVL            BCB_DEBUG_LVL_ERR

#define GMSG_BUFLEN			256
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
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); writeMemo(gMsg);}

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

/**
 * \def BCB_TEST_LOGGING
 * Constructs a string message documenting the occurrence of a security
 * operation event and prints it to the ion.log file.
 *
 * BCB TEST LOGGING statements contain the following:
 * - [te] test event indicator
 * - security operation event identifier
 * - bsrc: bundle source
 * - bdest: bundle destination
 * - svc: security service (bcb-confidentiality)
 * - tgt: target block type
 * - msec and count: bundle identifiers
 */

#ifndef BCB_TEST_LOGGING
#define BCB_TEST_LOGGING 0  /** Whether to enable (1) or disable (0) BCB
                              * test-level logging statements         */
#endif
#if (BCB_TEST_LOGGING == 1)

#define BCB_TEST_POINT(event, bundle, blktype) \
{_isprintf(gMsg, GMSG_BUFLEN, "[te] %s - bsrc:ipn:%i.%i, bdest:ipn:%i.%i,\
svc: bcb-confidentiality, tgt:%u, msec:%u, count: %u", event,\
(bundle) ? bundle->id.source.ssp.ipn.nodeNbr      : 0, \
(bundle) ? bundle->id.source.ssp.ipn.serviceNbr   : 0, \
(bundle) ? bundle->destination.ssp.ipn.nodeNbr    : 0, \
(bundle) ? bundle->destination.ssp.ipn.serviceNbr : 0, \
blktype, \
(bundle) ? bundle->id.creationTime.msec  : 0, \
(bundle) ? bundle->id.creationTime.count : 0); \
writeMemo(gMsg);}
#else
#define BCB_TEST_POINT(event, bundle, blktype) {}
#endif

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/


extern int		bpsec_encrypt(Bundle *bundle);

extern int		bpsec_decrypt(AcqWorkArea *work);

#endif /* BCB_H_ */
