/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: bpsec_bib.h
 **
 ** Description: Definitions supporting generic processing of BIB blocks.
 **              This includes both the BIB Interface to the ION BPSEC
 **              API as well as a default implementation of the BIB
 **              ASB processing for standard ciphersuite profiles.
 **
 **              BIB BPSEC Interface Call Order:
 **              -------------------------------------------------------------
 **
 **                     SEND SIDE                    RECEIVE SIDE
 **
 **              bpsec_bibOffer
 **              bpsec_bibProcessOnDequeue
 **              bpsec_bibRelease
 **              bpsec_bibCopy
 **                                                  bpsec_bibParse
 **                                                  bpsec_bibCheck
 **                                                  bpsec_bibRecord
 **                                                  bpsec_bibClear
 **
 **
 **              Default Ciphersuite Profile Implementation
 **              -------------------------------------------------------------
 **
 **                    SIGN SIDE                     VERIFY SIDE
 **
 **              bpsec_bibDefaultConstruct
 **              bpsec_bibDefaultSign
 **
 **                                              bpsec_bibDefaultConstruct
 **                                              bpsec_bibDefaultSign
 **                                              bpsec_bibDefaultVerify
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
 *****************************************************************************/
#ifndef BPSEC_BIB_H_
#define BPSEC_BIB_H_

#include "bpsec_util.h"
#include "profiles.h"

// If bpsec debugging is turned on, then turn on bib debugging.
#if BPSEC_DEBUGGING == 1
#define BIB_DEBUGGING 1
#endif

#ifndef BIB_DEBUGGING
#define BIB_DEBUGGING 0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BIB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BIB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BIB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BIB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BIB_DEBUG_LVL   BIB_DEBUG_LVL_ERR

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
 *    points considered useful by BPSEC module software engineers.
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
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); writeMemo(gMsg);}

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



extern int	   bpsec_bibAttach(Bundle *bundle,
		                           ExtensionBlock *bibBlk,
			                       BpsecOutboundBlock *bibAsb);

extern int         bpsec_bibCheck(AcqExtBlock *blk, AcqWorkArea *wk);

extern void        bpsec_bibClear(AcqExtBlock *blk);

extern int         bpsec_bibCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk);

extern int      bpsec_bibDefaultCompute(Object dataObj,
 				   	                       uint32_t chunkSize,
 						                   uint32_t suite,
 						                   void *context,
 						                   csi_svcid_t svc);

extern int	   bpsec_bibDefaultConstruct(uint32_t suite,
 		                                     ExtensionBlock *blk,
 											 BpsecOutboundBlock *asb);


extern uint32_t    bpsec_bibDefaultResultLen(uint32_t suite,
 		                                     uint8_t tlv);

extern int      bpsec_bibDefaultSign(uint32_t suite,
 		                                Bundle *bundle,
 		                                ExtensionBlock *blk,
 		                                BpsecOutboundBlock *asb,
										uvast *bytes);

extern int      bpsec_bibDefaultVerify(uint32_t suite,
 		                                  AcqWorkArea *wk,
 					     				  AcqExtBlock *blk,
										  uvast *bytes);

extern BibProfile *bpsec_bibGetProfile(char *securitySource,
		                               char *securityDest,
			                           int8_t targetBlkType,
							           BspBibRule *bibRule);

extern int	       bpsec_bibOffer(ExtensionBlock *blk, Bundle *bundle);


extern int         bpsec_bibParse(AcqExtBlock *blk, AcqWorkArea *wk);

extern int	   bpsec_bibProcessOnDequeue(ExtensionBlock *blk,
		                              Bundle *bundle,
		                              void *parm);


extern void	       bpsec_bibRelease(ExtensionBlock *blk);


#endif /* BPSEC_BIB_H_ */
