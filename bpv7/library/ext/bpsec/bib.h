/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
// TODO: Update description
/*****************************************************************************
 **
 ** File Name: bib.h
 **
 ** Description: Definitions supporting generic processing of BIB blocks.
 **              This includes both the BIB Interface to the ION bpsec
 **              API as well as a default implementation of the BIB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BIB bpsec Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              bibOffer
 **              bibProcessOnDequeue
 **              bibRelease
 **              bibCopy
 **                                                  bibReview
 **                                                  bibParse
 **                                                  bibCheck
 **                                                  bibRecord
 **                                                  bibClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    SIGN SIDE                     VERIFY SIDE
 **
 **              bibDefaultConstruct
 **              bibDefaultSign
 **
 **                                              bibDefaultConstruct
 **                                              bibDefaultSign
 **                                              bibDefaultVerify
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/05/10  E. Birrane     Implementation as extbspbib.c (JHU/APL)
 **            S. Burleigh    Implementation as bspbib.c for SBSP
 **  11/02/15  E. Birrane     Update for generic proc and profiles
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 **  02/27/16  E. Birrane     Update to CSI interface.
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 **  09/02/19  S. Burleigh    Rename everything for bpsec
 *****************************************************************************/
#ifndef BIB_H_
#define BIB_H_

#include "bpsec_util.h"
#include "bpsec_asb.h"
#include "sci.h"


// TODO: Consider removing BIB debug statements.

// If bpsec debugging is turned on, then turn on bib debugging.
#if DEBUGGING == 1
#define BIB_DEBUGGING	1
#endif

#ifndef BIB_DEBUGGING
#define BIB_DEBUGGING 1  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BIB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BIB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BIB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BIB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BIB_DEBUG_LVL   BIB_DEBUG_LVL_ERR

#define GMSG_BUFLEN     256
#if (BIB_DEBUGGING == 1)

/**
 * \def BIB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the BIB library and is useful for confirming control flow
 *    through the BIB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by bpsec module software engineers.
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

#define BIB_DEBUG(level, prefix, format,...) if(level >= BIB_DEBUG_LVL) \
{                                                                              \
  int len = _isprintf(gMsg, GMSG_BUFLEN, "%s: (%s) ", __func__, prefix);        \
  _isprintf(gMsg+len, GMSG_BUFLEN-len, format, __VA_ARGS__);                   \
   putErrmsg(gMsg, NULL);                                                      \
}

   #define BIB_DEBUG_PROC(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_PROC, ">", format, __VA_ARGS__)

   #define BIB_DEBUG_INFO(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_INFO, "i", format, __VA_ARGS__)

   #define BIB_DEBUG_WARN(format,...) \
           BIB_DEBUG(BIB_DEBUG_LVL_WARN, "w", format, __VA_ARGS__)

   #define BIB_DEBUG_ERR(format,...) \
           BIB_DEBUG(BPSEC_DEBUG_LVL_ERR,  "e", format, __VA_ARGS__)

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

/**
 * \def BIB_TEST_LOGGING
 * Constructs a string message documenting the occurrence of a security
 * operation event and prints it to the ion.log file.
 *
 * BIB TEST LOGGING statements contain the following:
 * - [te] test event indicator
 * - security operation event identifier
 * - bsrc: bundle source
 * - bdest: bundle destination
 * - svc: security service (bib-integrity)
 * - tgt: target block type
 * - msec and count: bundle identifiers
 */

#ifndef BIB_TEST_LOGGING
#define BIB_TEST_LOGGING 0  /** Whether to enable (1) or disable (0) BIB
                              * test-level logging statements         */
#endif
#if (BIB_TEST_LOGGING == 1)

#define BIB_TEST_POINT(event, bundle, num) \
{_isprintf(gMsg, GMSG_BUFLEN, "[te] %s - bsrc:ipn:%i.%i, bdest:ipn:%i.%i,\
svc: bib-integrity, tgt:%u, msec:%u, count: %u", event,\
(bundle) ? bundle->id.source.ssp.ipn.nodeNbr      : 0, \
(bundle) ? bundle->id.source.ssp.ipn.serviceNbr   : 0, \
(bundle) ? bundle->destination.ssp.ipn.nodeNbr    : 0, \
(bundle) ? bundle->destination.ssp.ipn.serviceNbr : 0, \
num, \
(bundle) ? bundle->id.creationTime.msec  : 0, \
(bundle) ? bundle->id.creationTime.count : 0); \
writeMemo(gMsg);}
#else
#define BIB_TEST_POINT(event, bundle, num) {}
#endif

/************************************************************************
 *				FUNCTION DEFINITIONS			*
 ************************************************************************/

extern int	bpsec_sign(Bundle *bundle);

extern int	bpsec_verify(AcqWorkArea *work);


#endif /* BIB_H_ */
