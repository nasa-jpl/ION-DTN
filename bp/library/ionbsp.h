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
 **
 ** File Name: ionbsp.h
 **
 **		Note: name changed from "bsp.h", which conflicts with the
 **		name of the Board Support Package header file generated on
 **		installation of RTEMS.
 **
 ** Subsystem:
 **          BSP-ION
 **
 ** Description: This file provides all structures, variables, and function 
 **              definitions necessary for a full implementation of the 
 **              Bundle Security Protocol (BSP) Specification, Version 8. This
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
 ** Modification History:
 **
 **  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 **  --------  ------------  -----------------------------------------------
 **  06/08/09  E. Birrane           Initial Implementation of BAB blocks.
 **  06/15/09  E. Birrane           Completed BAB Unit Testing & Documentation
 **  06/20/09  E. Birrane           Doc. updates for initial release.
 *****************************************************************************/

#ifndef _IONBSP_H_
#define _IONBSP_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

/** 
 * \file bpP.h
 * Private include functions for the ION Bundle Protocol library.
 * 
 * The file bpP.h is necessary to include functions and variables necessary
 * for the processing of an RFC5050 bundle. Unlike other extension blocks,
 * the BSP must cross-reference blocks and otherwise examine data external
 * to any BSP block (such as the case where a bundle-wide hash must be
 * calculated.
 */ 
#include "bpP.h" 

/*****************************************************************************
 *                              DEBUG DEFINITIONS                            *
 *****************************************************************************/

/** \todo remove this and require that it be passed in on the command line. */
#define BSP_DEBUGGING 1  /** Whether to enable (1) or disable (0) debugging */

#define BSP_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BSP_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BSP_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BSP_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BSP_DEBUG_LVL BSP_DEBUG_LVL_WARN

/**
 * \def BSP_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the BSP library and is useful for confirming control flow
 *    through the BSP module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by BSP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the BSP module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * BSP_DEBUGGING #define.
 */
#ifdef TORNADO_2_0_2
#ifdef BSP_DEBUGGING
   #define BSP_DEBUG(level, format, args...) if(level >= BSP_DEBUG_LVL) \
{isprintf(gMsg, sizeof gMsg, format, args); putErrmsg(gMsg, NULL);}
                                       
   #define BSP_DEBUG_PROC(format, args...) \
           BSP_DEBUG(BSP_DEBUG_LVL_PROC,format, args)

   #define BSP_DEBUG_INFO(format, args...) \
           BSP_DEBUG(BSP_DEBUG_LVL_INFO,format, args)

   #define BSP_DEBUG_WARN(format, args...) \
           BSP_DEBUG(BSP_DEBUG_LVL_WARN,format, args)

   #define BSP_DEBUG_ERR(format, args...) \
           BSP_DEBUG(BSP_DEBUG_LVL_ERR,format, args)
                                        
#else
   #define BSP_DEBUG(format, args...)
   #define BSP_DEBUG_PROC(format, args...) 
   #define BSP_DEBUG_INFO(format, args...) 
   #define BSP_DEBUG_WARN(format, args...) 
   #define BSP_DEBUG_ERR(format, args...)
#endif
#else		/*	Not TORNADO_2_0_2				*/
#ifdef BSP_DEBUGGING
   #define BSP_DEBUG(level, format,...) if(level >= BSP_DEBUG_LVL) \
{isprintf(gMsg, sizeof gMsg, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}
                                       
   #define BSP_DEBUG_PROC(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BSP_DEBUG_INFO(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BSP_DEBUG_WARN(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BSP_DEBUG_ERR(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_ERR,format, __VA_ARGS__)
                                        
#else
   #define BSP_DEBUG(format,...)
   #define BSP_DEBUG_PROC(format,...) 
   #define BSP_DEBUG_INFO(format,...) 
   #define BSP_DEBUG_WARN(format,...) 
   #define BSP_DEBUG_ERR(format,...)
#endif
#endif		/*	End of #ifdef TORNADO_2_0_2			*/


/*****************************************************************************
 *                        BSP SPEC VARIABLE DEFINITIONS                      *
 *****************************************************************************/

/** 
 * BAB Block Type Fields 
 * 
 * \todo: 
 * 1: Consider making these enum fields
 */
#define BSP_BAB_TYPE  0x02 /** pre-payload bab block type.  */
#define BSP_PIB_TYPE  0x03 /** BSP PIB block type.          */
#define BSP_PCB_TYPE  0x04 /** BSP PCB block type.          */
#define BSP_ESB_TYPE  0x09 /** BSP ESB block type.          */


/** Ciphersuite types - From BSP Spec. Version 8. */
#define BSP_CSTYPE_BAB_HMAC 0x001
#define BSP_CSTYPE_PIB_RSA_SHA256 0x002
#define BSP_CSTYPE_PCB_RSA_AES128_PAYLOAD_PIB_PCB 0x003
#define BSP_CSTYPE_ESB_RSA_AES128_EXT 0x004


/** Ciphersuite Flags - From BSP Spec. Version 8. */
#define BSP_ASB_SEC_SRC   0x10 /** ASB contains a security source EID      */
#define BSP_ASB_SEC_DEST  0x08 /** ASB contains a security destination EID */
#define BSP_ASB_HAVE_PARM 0x04 /** ASB has ciphersuite parameters.         */
#define BSP_ASB_CORR      0x02 /** ASB has a correlator field.             */
#define BSP_ASB_RES       0x01 /** ASB contains a result length and data.  */


/** Ciphersuite Parameter Types - From BSP Spec Version 8. */
#define BSP_CSPARM_IV           0x01
#define BSP_CSPARM_KEY_INFO     0x03
#define BSP_CSPARM_FRAG_RNG     0x04
#define BSP_CSPARM_INT_SIG      0x05
#define BSP_CSPARM_SALT         0x07
#define BSP_CSPARM_ICV          0x08
#define BSP_CSPARM_ENC_BLK      0x0A
#define BSP_CSPARM_ENV_BLK_TYPE 0x0B


/*****************************************************************************
 *                        BSP MODULE VARIABLE DEFINITIONS                    *
 *****************************************************************************/

/** BAB rule type enumerations */
#define BAB_TX 0
#define BAB_RX 1 

/** BAB Scratchpad Receive Flags */
#define BSP_BABSCRATCH_RXFLAG_CORR 1 /** Block's correlated block was found */

/** BAB Constants */
#define BAB_KEY_NAME_LEN     32
#define BAB_HMAC_SHA1_RESULT_LEN 20
#define BAB_CORRELATOR 0xED

/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/

/** 
 *  \struct BspAbstractSecurityBlock
 *  \brief Canonical Abstract Security Block as defined in the Bundle Security
 *  Specification, version 8.
 * 
 * The Abstract Security Block (ASB) structure encapsulates the security-
 * specific part of the canonical ASB structure as defined in the BSP
 * Specification, version 8. 
 */
typedef struct {
	unsigned long cipher;         /** Ciphersuite Type Field              */
	unsigned long cipherFlags;    /** Ciphersuite Flags Field             */
	unsigned long correlator;     /** IFF cipherFlags & BSP_ASB_CORR      */
	unsigned long cipherParmsLen; /** IFF cipherFlags & BSP_ASB_HAVE_PARM */
	char *cipherParmsData;        /** IFF cipherFlags & BSP_ASB_HAVE_PARM */
	unsigned long resultLen;      /** IFF cipherFlags & BSP_ASB_RES       */
	unsigned char *resultData;    /** IFF cipherFlags & BSP_ASB_RES       */
} BspAbstractSecurityBlock;

/** 
 *  \struct BspBabScratchpad
 *  \brief scratchpad object used to carry structured block information prior
 *  to block serialization.
 * 
 *  The scratchpad object carries meta-data associated with BAB block
 *  processing and is used to both ease the construction of individual
 *  pre- and post- payload BAB blocks as well as to facilitate communication
 *  between BAB blocks in the BSP module.
 */
 
typedef struct
{
  BspAbstractSecurityBlock asb; /** The ASB associated with this block.      */
  char cipherKeyName[BAB_KEY_NAME_LEN]; /** Cipherkey name used by this block.*/
  int useBab;                   /** Whether this block uses BAB blocks at all.*/
  unsigned long rxFlags;        /** RX-side processing flags for this block. */
  int hmacLen;
  char expectedResult[BAB_HMAC_SHA1_RESULT_LEN];
} BspBabScratchpad;

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/


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
 * \par Date Written:  5/29/09
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
 * 
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/29/09  E. Birrane           Initial Implementation.
 *  06/08/09  E. Birrane           Documentation updates.
 *  06/20/09  E. Birrane           Added Debugging Stmt, cmts for initial rel.
 *****************************************************************************/

unsigned char *bsp_addSdnvToStream(unsigned char *stream, Sdnv* value);


/******************************************************************************
 *
 * \par Function Name: bsp_deserializeASB
 *
 * \par Purpose: This utility function accepts a serialized Abstract Security
 *               Block from a bundle during acquisition and places it in a
 *               AbstractSecurityBlock structure stored in the Acquisition
 *               Block's scratchpad area.
 *
 * \par Date Written:  5/30/09
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
 * 
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  05/30/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/20/09  E. Birrane           Cmts and code cleanup for initial release.
 *****************************************************************************/

int bsp_deserializeASB(AcqExtBlock *blk);


#if 0
/******************************************************************************
 *
 * \par Function Name: bsp_eidNil
 *
 * \par Purpose: This utility function determines whether a given EID is
 *               "nil".  Nil in this case means that the EID is uninitialized
 *               or will otherwise resolve to the default nullEID. 
 *
 * \par Date Written:  6/18/09
 *
 * \retval int - Whether the EID is nil (1) or not (0).
 *
 * \param[in]  eid - The EndpointID being checked.
 *
 * \par Notes: 
 *      1. Nil check is pulled from whether the ION library printEid function
 *         will use the default nullEID when given this EID. 
 *
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/18/09  E. Birrane           Initial Implementation.
 *  06/20/09  E. Birrane           Cmt/Debug updates for initial release.
 *****************************************************************************/

int bsp_eidNil(EndpointId *eid);
#endif


/******************************************************************************
 *
 * \par Function Name: bsp_findAcqExtBlk
 *
 * \par Purpose: This utility function finds an acquisition extension block
 *               from within the work area.
 *
 * \par Date Written:  6/13/09
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
 *  
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/13/09  E. Birrane           Initial Implementation.
 *  06/20/09  E. Birrane           Cmt/Debug updates for initial release.
 *****************************************************************************/

AcqExtBlock *bsp_findAcqExtBlk(AcqWorkArea *wk, int listIdx, int type);


/******************************************************************************
 *
 * \par Function Name: bsp_retrieveKey
 *
 * \par Purpose: Retrieves the key associated with a particular keyname.
 *
 * \par Date Written:  6/01/09
 *
 * \retval char * -- !NULL - A pointer to a buffer holding the key value.
 *                    NULL - There was a system error.
 *
 * \param[out] keyLen   The length of the key value that was found.
 * \param[in]  keyName  The name of the key to find.
 *
 * \par Notes: 
 *
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/01/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/13/09  E. Birrane           Added debugging statements.
 *  06/15/09  E. Birrane           Formatting and comment updates.
 *  06/20/09  E. Birrane           Change to use ION primitives, Added cmts for
 *                                 initial release.
 *****************************************************************************/

unsigned char *bsp_retrieveKey(int *keyLen, char *keyName);


/******************************************************************************
 *
 * \par Function Name: bsp_serializeASB
 *
 * \par Purpose: Serializes an abstract security block and returns the 
 *               serialized representation.
 *
 * \par Date Written:  6/03/09
 *
 * \retval unsigned char * - the serialized Abstract Security Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  asb The BspAbstractSecurityBlock to serialize.
 *
 * \par Notes: 
 *      1. This function uses MTAKE to allocate space for the result. This
 *         result (if not NULL) must be freed using MRELEASE. 
 *      2. This function only serializes the "security specific" ASB, not the
 *         canonical header information of the encapsulating BP extension block.
 *  
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/03/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/13/09  E. Birrane           Added debugging statements.
 *  06/15/09  E. Birrane           Comment updates for DINET-2 release.
 *  06/20/09  E. Birrane           Fixed Debug stmts, pre for initial release.
 *****************************************************************************/

unsigned char *bsp_serializeASB(unsigned int *length,      
                                BspAbstractSecurityBlock *asb);


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
 * \par Function Name: bsp_babPreAcquire
 *
 * \par Purpose: This callback is called when a pre-payload bunde received by 
 *               the ION library.  This function will rely on the general
 *               helper function: bsp_babAcquire to deserialize the block. 
 *               Additionally, this function will grab the overall serialized
 *               version of the bundle.  This is due to the act that the act of
 *               deserializing the bundle is destructive and would require 
 *               re-serializing the bundle later if we do not capture the 
 *               bundle at this point.
 *
 *               The serialized bundle is stored in the scratchpad. Currently,
 *               we cannot and should not serialize the bundle at this point
 *               to calculate the security result because blocks that may
 *               affect the security result (like the previous hop insertion
 *               block) will not have been processed yet.  Also, the acquire 
 *               callback should likely just acquire information, and not do
 *               data processing.
 * 
 * \par Date Written:  6/20/09
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
 * 
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/20/09  E. Birrane           Initial Implementation.
 *****************************************************************************/

int bsp_babPreAcquire(AcqExtBlock *blk, AcqWorkArea *wk);


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


/*****************************************************************************
 *                            BAB UTILITY FUNCTIONS                          *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_babGetSecResult
 *
 * \par Purpose: Calculates a security result given a ciphersuite name and a 
 *               set of serialized data. 
 *
 * \par Date Written:  6/02/09
 *
 * \retval unsigned char * - The security result.
 *
 * \param[in]  rawBundle     - The serialized bundle that we wish to hash.
 * \param[in]  rawBundleLen  - Length of the serialized bundle.
 * \param[in]  cipherKeyName - Name of the key to use to hash the bundle.
 * \param[out] hashLen       - Length of the security result.
 * 
 * \par Notes: 
 *      1. Currently, only the HMAC-SHA1 ciphersuite is implemented.
 *      2. This function should be updated for a more parameterized security
 *         result based on different ciphersuites.
 *
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/02/09  E. Birrane           Initial Implementation.
 *  06/06/09  E. Birrane           Documentation Pass.
 *  06/13/09  E. Birrane           Updated to use scratchpads.
 *  06/18/09  E. Birrane           Major re-write to not be coupled to blk.
 *  06/20/09  E. Birrane           Cmt/Debug updated for initial release.  
 *****************************************************************************/

unsigned char *bsp_babGetSecResult(char *rawBundle, 
                                   unsigned long rawBundleLen, 
                                   char *cipherKeyName, 
                                   unsigned long *hashLen);
 
 
 /******************************************************************************
 *
 * \par Function Name: bsp_babGetSecurityInfo
 *
 * \par Purpose: This utility function retrieves security information for a
 *               given BAB block from the ION security manager.
 *
 * \par Date Written:  6/13/09
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
 *
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/13/09  E. Birrane           Initial Implementation.
 *  06/20/09  E. Birrane           Updated cmts for initial release.
 *****************************************************************************/

void bsp_babGetSecurityInfo(Bundle *bundle, 
                            int which, 
                            char *eidString, 
                            BspBabScratchpad *scratch);
 

#endif /* _IONBSP_H_ */
