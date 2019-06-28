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
 ** File Name: bsputil.h
 **
 **		Note: name changed from "bsp.h", which conflicts with the
 **		name of the Board Support Package header file generated on
 **		installation of RTEMS.
 **
 ** Subsystem:
 **             Extensions: BSP
 **
 ** Description: This file provides all structures, variables, and function 
 **              definitions necessary for a full implementation of the 
 **              "Streamlined" Bundle Security Protocol (SBSP) Specification.
 **		 This implementation utilizes the ION Extension Interface to
 **              manage the creation, modification, evaluation, and removal
 **              of BSP blocks from Bundle Protocol (RFC 5050) bundles.
 **
 ** Notes:  The original implementation of this file (6/2009) only supported
 **         the Bundle Authentication Block (BAB) with the following
 **         constraints:
 **         - Bundle fragmentation is not considered.
 **         - Only the HMAC-SHA1 ciphersuite for BAB is considered.
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
 **  01/14/14  S. Burleigh          Revised for "streamlined BSP".
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
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}

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
#define BSP_TX			0
#define BSP_RX			1

/** 
 * BAB Block Type Fields 
 */
#define BLOCK_TYPE_PRIMARY	0x00	/*	Primary block type.	*/
#define BLOCK_TYPE_PAYLOAD	0x01	/*	Payload block type.	*/
#define EXTENSION_TYPE_BAB	0x02	/*	BSP BAB block type.	*/
#define EXTENSION_TYPE_BIB	0x03	/*	BSP BIB block type.	*/
#define EXTENSION_TYPE_BCB	0x04	/*	BSP BCB block type.	*/

/** Ciphersuite types - From RFC 6257. */
#define BSP_CSTYPE_BAB_HMAC_SHA1	0x001
#define BSP_CSTYPE_BIB_HMAC_SHA256 	0x005
#define BSP_CSTYPE_BCB_ARC4		0x006

/** Ciphersuite Flags - From SBSP Spec. */
#define BSP_ASB_SEC_SRC		0x04
			/*	Block contains a security source EID	*/
#define BSP_ASB_PARM		0x02
			/*	Block contains ciphersuite parameters.	*/
#define BSP_ASB_RES		0x01
			/*	Block contains security result.		*/

/** Ciphersuite and Security Result Item Types - SBSP spec Section 2.7.	*/
#define BSP_CSPARM_IV           0x01
#define BSP_CSPARM_KEY_INFO     0x03
#define BSP_CSPARM_CONTENT_RNG	0x04
#define BSP_CSPARM_INT_SIG      0x05
#define BSP_CSPARM_SALT         0x07
#define BSP_CSPARM_ICV          0x08

#define BSP_KEY_NAME_LEN	32

/** Misc **/
#define BSP_ZCO_TRANSFER_BUF_SIZE 4096

/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/

/** 
 *  \struct BspInboundBlock
 *  \brief The block-type-specific data of an inbound BSP extension block.
 * 
 * The BspInboundBlock structure encapsulates the block-type-specific
 * data of an inbound BSP extension block.
 */
typedef struct
{
	EndpointId	securitySource;
	unsigned char	targetBlockType;
	unsigned char	targetBlockOccurrence;
	unsigned char	instance;	/*  0: 1st, lone.  1: last.	*/
	unsigned char	ciphersuiteType;
	char		keyName[BSP_KEY_NAME_LEN];
	unsigned int	ciphersuiteFlags;
	unsigned int	parmsLen;	/*  IFF flags & BSP_ASB_PARM	*/
	unsigned char	*parmsData;	/*  IFF flags & BSP_ASB_PARM	*/
	unsigned int	resultsLen;	/*  IFF flags & BSP_ASB_RES	*/
	unsigned char	*resultsData;	/*  IFF flags & BSP_ASB_RES	*/
} BspInboundBlock;

/** 
 *  \struct BspOutboundBlock
 *  \brief The block-type-specific data of an outbound BSP extension block.
 * 
 * The BspOutboundBlock structure encapsulates the block-type-specific
 * data of an outbound BSP extension block.
 */
typedef struct
{
	EndpointId	securitySource;
	unsigned char	targetBlockType;
	unsigned char	targetBlockOccurrence;
	unsigned char	instance;	/*  0: 1st, lone.  1: last.	*/
	unsigned char	ciphersuiteType;
	char		keyName[BSP_KEY_NAME_LEN];
	unsigned int	ciphersuiteFlags;
	unsigned int	parmsLen;	/** IFF flags & BSP_ASB_PARM	*/
	Object		parmsData;	/** IFF flags & BSP_ASB_PARM	*/
	unsigned int	resultsLen;	/** IFF flags & BSP_ASB_RES	*/
	Object		resultsData;	/** IFF flags & BSP_ASB_RES	*/
} BspOutboundBlock;

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
 * \param[in]  val     The SDNV value to add to the stream.
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

extern unsigned char	*bsp_addSdnvToStream(unsigned char *stream, Sdnv* val);

/******************************************************************************
 *
 * \par Function Name: bsp_deserializeASB
 *
 * \par Purpose: This utility function accepts a serialized Abstract Security
 *               Block from a bundle during acquisition and places it in a
 *               BspInboundBlock structure stored in the Acquisition Extension
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
 *      2.  If we return a 1, the ASB is considered invalid and not usable.
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

extern int	bsp_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk);

extern void	bsp_insertSecuritySource(Bundle *bundle, BspOutboundBlock *asb);

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

extern unsigned char	*bsp_retrieveKey(int *keyLen, char *keyName);

/******************************************************************************
 *
 * \par Function Name: bsp_serializeASB
 *
 * \par Purpose: Serializes an outbound bundle security block and returns the 
 *               serialized representation.
 *
 * \par Date Written:  6/03/09
 *
 * \retval unsigned char * - the serialized outbound bundle Security Block.
 *
 * \param[out] length The length of the serialized block.
 * \param[in]  asb    The BspOutboundBlock to serialize.
 *
 * \par Notes: 
 *      1. This function uses MTAKE to allocate space for the serialized ASB.
 *         This serialized ASB (if not NULL) must be freed using MRELEASE. 
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

extern unsigned char	*bsp_serializeASB(unsigned int *length,
				BspOutboundBlock *blk);

/*****************************************************************************
 *                            BAB UTILITY FUNCTIONS                          *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_getInboundBspItem
 *
 * \par Purpose: This function searches within a buffer (a ciphersuite
 *               parameters field or a security results field) of an
 *               inbound BSP block for an information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in
 *                         bsputil.h as BSP_CSPARM_xxx macros.
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

extern void	bsp_getInboundBspItem(int itemNeeded, unsigned char *bspBuf,
			unsigned int bspBufLen, unsigned char **val,
			unsigned int *len);

/******************************************************************************
 *
 * \par Function Name: bsp_getOutboundBspItem
 *
 * \par Purpose: This function searches within a buffer (a ciphersuite
 *               parameters field or a security result field) of an outbound
 *               BSP block for an information item of specified type.
 *
 * \retval void
 *
 * \param[in]  itemNeeded  The code number of the type of item to search
 *                         for.  Valid item type codes are defined in 
 *                         bsputil.h as BSP_CSPARM_xxx macros.
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

extern void	bsp_getOutboundBspItem(int itemNeeded, Object bspBuf,
			unsigned int bspBufLen, Address *val,
			unsigned int *len);

extern Object	bsp_findBspBlock(Bundle *bundle, unsigned char type,
			unsigned char targetBlockType,
			unsigned char targetBlockOccurrence,
			unsigned char instance);

extern LystElt	bsp_findAcqBspBlock(AcqWorkArea *wk, unsigned char type,
			unsigned char targetBlockType,
			unsigned char targetBlockOccurrence,
			unsigned char instance);

extern int	bsp_getInboundSecurityEids(Bundle *bundle, AcqExtBlock *blk,
			BspInboundBlock *asb, char **fromEid, char **toEid);

extern int	bsp_getOutboundSecurityEids(Bundle *bundle, ExtensionBlock *blk,
			BspOutboundBlock *asb, char **fromEid, char **toEid);

extern int	bsp_destinationIsLocal(Bundle *bundle);

extern char	*bsp_getLocalAdminEid(char *eid);

extern int	bsp_securityPolicyViolated(AcqWorkArea *wk);

#endif /* _IONBSP_H_ */
