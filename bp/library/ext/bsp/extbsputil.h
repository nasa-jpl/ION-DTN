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
 ** File Name: extbsputil.h
 **
 **		Note: name changed from "bsp.h", which conflicts with the
 **		name of the Board Support Package header file generated on
 **		installation of RTEMS.
 **
 ** Subsystem:
 **          Extensions: BSP
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
#include "bei.h"
#include "bpsec.h"

int extensionBlockTypeToInt(char *blockType);
int extensionBlockTypeToString(unsigned char blockType, char *retVal,
		unsigned int retValLength);
int bspTypeToString(int bspType, char *s, int buflen);
int bspTypeToInt(char *bspType);

/*****************************************************************************
 *                              DEBUG DEFINITIONS                            *
 *****************************************************************************/

#ifndef BSP_DEBUGGING
#define BSP_DEBUGGING	0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BSP_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BSP_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BSP_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BSP_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BSP_DEBUG_LVL	BSP_DEBUG_LVL_PROC

#define	GMSG_BUFLEN	256
#if BSP_DEBUGGING == 1

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

   #define BSP_DEBUG(level, format,...) if(level >= BSP_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); printf("%s\n", gMsg);}

   #define BSP_DEBUG_PROC(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BSP_DEBUG_INFO(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BSP_DEBUG_WARN(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BSP_DEBUG_ERR(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define BSP_DEBUG(level, format,...) if(level >= BSP_DEBUG_LVL) \
{}

   #define BSP_DEBUG_PROC(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BSP_DEBUG_INFO(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BSP_DEBUG_WARN(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BSP_DEBUG_ERR(format,...) \
           BSP_DEBUG(BSP_DEBUG_LVL_ERR,format, __VA_ARGS__)


#endif


/*****************************************************************************
 *                        BSP SPEC VARIABLE DEFINITIONS                      *
 *****************************************************************************/
/** BSP rule type enumerations */
#define BSP_TX 0
#define BSP_RX 1

/** 
 * BAB Block Type Fields 
 */
#define BLOCK_TYPE_PAYLOAD	0x01 /*		Payload block type.	*/
#define BSP_BAB_TYPE		0x02 /*		BSP BAB block type.	*/
#define BSP_PIB_TYPE		0x03 /*		BSP PIB block type.	*/
#define BSP_PCB_TYPE		0x04 /*		BSP PCB block type.	*/
#define BSP_ESB_TYPE		0x05 /*		BSP ESB block type.	*/

/** Ciphersuite types - From BSP Spec. Version 8. */
#define BSP_CSTYPE_BAB_HMAC 					0x001
#define BSP_CSTYPE_PIB_RSA_SHA256 				0x002
#define BSP_CSTYPE_PCB_RSA_AES128_PAYLOAD_PIB_PCB		0x003
#define BSP_CSTYPE_ESB_RSA_AES128_EXT				0x004
#define BSP_CSTYPE_PIB_HMAC_SHA256 				0x005
#define BSP_CSTYPE_PCB_ARC4 					0x006
#define BSP_CSTYPE_PCB_AES128 					0x007

/** Ciphersuite Flags - From BSP Spec. Version 8. */
#define BSP_ASB_SEC_SRC   0x10 /** ASB contains a security source EID      */
#define BSP_ASB_SEC_DEST  0x08 /** ASB contains a security destination EID */
#define BSP_ASB_HAVE_PARM 0x04 /** ASB has ciphersuite parameters.         */
#define BSP_ASB_CORR      0x02 /** ASB has a correlator field.             */
#define BSP_ASB_RES       0x01 /** ASB contains a result length and data.  */

/** Ciphersuite Parameter Types - RFC6257 Section 2.6. */
#define BSP_CSPARM_IV           0x01
#define BSP_CSPARM_KEY_INFO     0x03
#define BSP_CSPARM_FRAG_RNG     0x04
#define BSP_CSPARM_INT_SIG      0x05
#define BSP_CSPARM_SALT         0x07
#define BSP_CSPARM_ICV          0x08
#define BSP_CSPARM_ENC_BLK      0x0A
#define BSP_CSPARM_ENV_BLK_TYPE 0x0B

#define BSP_KEY_NAME_LEN	32

/** Misc **/
#define BSP_ZCO_TRANSFER_BUF_SIZE 4096

/*****************************************************************************
 *                        BSP MODULE VARIABLE DEFINITIONS                    *
 *****************************************************************************/

/*
 * At times it is useful to identify which block types manage correlation
 * information.  For BSP we set up specific enumerations for these so as to
 * not confuse them with the block types themselves and, thus, imply a
 * correlation.
 */
#define COR_BAB_TYPE 0
#define COR_PIB_TYPE 1
#define COR_PCB_TYPE 2
#define COR_ESB_TYPE 3


/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/

typedef struct
{
	char	cipherKeyName[BSP_KEY_NAME_LEN];
} BspSecurityInfo;

/** 
 *  \struct BspAbstractSecurityBlock
 *  \brief Canonical Abstract Security Block as defined in the Bundle Security
 *  Specification, version 8.
 * 
 * The Abstract Security Block (ASB) structure encapsulates the security-
 * specific part of the canonical ASB structure as defined in the BSP
 * Specification, version 8. 
 */
typedef struct
{
	EndpointId    secSrc;         /** Optional security source            */
	EndpointId    secDest;        /** Optional security destination       */
	unsigned int cipher;         /** Ciphersuite Type Field              */
	unsigned int cipherFlags;    /** Ciphersuite Flags Field             */
	unsigned int correlator;     /** IFF cipherFlags & BSP_ASB_CORR      */
	unsigned int cipherParmsLen; /** IFF cipherFlags & BSP_ASB_HAVE_PARM */
	char *cipherParmsData;        /** IFF cipherFlags & BSP_ASB_HAVE_PARM */
	unsigned int resultLen;      /** IFF cipherFlags & BSP_ASB_RES       */
	unsigned char *resultData;    /** IFF cipherFlags & BSP_ASB_RES       */
} BspAbstractSecurityBlock;


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/


/*****************************************************************************
 *
 * \par Function Name: getCustodianEid
 *
 * \par Purpose: This utility function returns the custodian node number
 *               for the peer ID given. The EID is used to identify a scheme,
 *               and the custodian EID for the scheme is returned.  Note: The
 *               peer ID may actually refer to a different node than
 *               the current node.  For example, if we are communicating
 *               with a "far" node in scheme "A", we may pass in the far
 *               node's EID to determine our own, local custodian EID
 *               for scheme "A".
 *
 * \par Date Written: 2/28/2011
 *
 * \retval char * -- Custodian EID (e.g., ipn:1.0)
 *
 * \param[in] peerEid - EID string of the destination(?) endpoint.
 *
 * \par Revision History:
 *
 * MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 * --------  ------------  ------------------------------------------------
 * 02/28/11  E. Birrane            Initial implementation
 * **************************************************************************/
char * getCustodianEid(char *peerEid);


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
 * \param[in]      wk   Work area holding bundle information.
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
 *  07/26/11  E. Birrane           Added useCbhe and EID ref/deref
 *****************************************************************************/

int bsp_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk, int blockType);


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
 *                            BAB UTILITY FUNCTIONS                          *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_getSecurityInfo
 *
 * \par Purpose: This utility function retrieves security information for a
 *               given BAB block from the ION security manager.
 *
 * \par Date Written:  6/02/09
 *
 * \retval void 
 *
 * \param[in]  bundle    The bundle that holding the block whose security
 *                       information is being requested.
 * \param]in]  blockType The block whose key information is being requested.
 * \param[in]  bspType   The type of BSP block whose key is being requested.
 * \param[in]  blockType The type of block affected by the BSP. 0 = whole bundle
 * \param[in]  eidSourceString The name of the source endpoint.
 * \param[in]  eidDestString The name of the destination endpoint.
 * \param[out] secInfo   The block scratchpad holding security information for
 *                       this block.
 * 
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  06/02/09  E. Birrane           Initial Implementation.
 *  06/18/09  E. Birrane           Re-write to incorporate Other BSP blocks
 *  02/10/11  E. Birrane           Updated based on ionsecadmin changes.
 *  07/27/11  E. Birrane           Updated for PIB.
 *****************************************************************************/

void bsp_getSecurityInfo(Bundle * bundle,
			int bspType,
			int blockType,
			char * eidSourceString,
			char * eidDestString,
			BspSecurityInfo * secInfo);

/******************************************************************************
 *
 * \par Function Name: getBspItem
 *
 * \par Purpose: This function searches within a BSP buffer (a ciphersuite
 *               parameters field or a security result field) for an
 *               information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in bsp.h
 *                         as BSP_CSPARM_xxx macros.
 * \param[in]  bspBuf      The data buffer in which to search for the item.
 * \param[in]  bspLen      The length of the data buffer.
 * \param[in]  val         A pointer to a variable in which the function
 *                         should place the location (within the buffer)
 *                         of the first item of specified type that is
 *                         found within the buffer.  On return, this
 *                         variable contains NULL if no such item was found.
 * \param[in]  len         A pointer to a variable in which the function
 *                         should place the length of the first item of
 *                         specified type that is found within the buffer.
 *                         On return, this variable contains 0 if no such
 *                         item was found.
 *
 * \par Notes: 
 *****************************************************************************/

void getBspItem(int itemNeeded, unsigned char *bspBuf,
		unsigned int bspLen, unsigned char **val,
		unsigned int *len);

char *getLocalCustodianEid(DequeueContext *ctxt);

int setSecPointsRecv(AcqExtBlock *blk, AcqWorkArea *wk, int blockType);

int setSecPointsTrans(ExtensionBlock *blk, Bundle *bundle, BspAbstractSecurityBlock *asb,
                      Lyst *eidRefs, int blockType, DequeueContext *ctxt, char *srcNode, char *destNode);

/******************************************************************************
 *
 * \par Function Name: transferToZcoFileSource
 *
 * \par Purpose: This utility function attains a zco object, a file reference, a 
 *               character string and appends the string to a file. A file
 *               reference to the new data is appended to the zco object. If given
 *               an empty zco object- it will create a new one on the empty pointer.
 *               If given an empty file reference, it will create a new file.
 *
 * \par Date Written:  8/15/11
 *
 * \retval int - 0 indicates success, -1 is an error
 *
 * \param[in]  sdr        ion sdr
 * \param]in]  resultZco  Object where the file references will go
 * \param[in]  acqFileRef A file references pointing to the file
 * \param[in]  fname      A string to be used as the base of the filename
 * \param[in]  bytes      The string data to write in the file
 * \param[in]  length     Length of the string data
 * \par Revision History:
 * 
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/20/11  R. Brown           Initial Implementation.
 *****************************************************************************/

int transferToZcoFileSource(Sdr sdr, Object *resultZco, Object *acqFileRef, 
                            char *fname, char *bytes, int length); 
#endif /* _IONBSP_H_ */
