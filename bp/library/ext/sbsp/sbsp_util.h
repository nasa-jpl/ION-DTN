/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
 
/*****************************************************************************
 **
 ** File Name: sbsp_util.h
 **
 **		Note: name changed from "sbsp.h", which conflicts with the
 **		name of the Board Support Package header file generated on
 **		installation of RTEMS.
 **
 ** Subsystem:
 **             Extensions: SBSP
 **
 ** Description: This file provides all structures, variables, and function 
 **              definitions necessary for a full implementation of Streamlined
 **		 Bundle Protocol Security (SBSP).  This implementation utilizes
 **		 the ION Extension Interface to manage the creation,
 **		 modification, evaluation, and removal of SBSP blocks from
 **		 Bundle Protocol (RFC 5050) bundles.
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
 **  06/08/09  E. Birrane           Initial Implementation of BAB blocks. (JHU/APL)
 **  06/15/09  E. Birrane           Completed BAB Unit Testing & Documentation (JHU/APL)
 **  06/20/09  E. Birrane           Doc. updates for initial release. (JHU/APL)
 **  01/14/14  S. Burleigh          Revised for "streamlined sbsp".
 **  01/23/16  E. Birrane           Updated for SBSP
 **                                 [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef _SBSP_UTIL_H_
#define _SBSP_UTIL_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpP.h"
#include "bei.h"
#include "bpsec.h"
#include "csi.h"

/*****************************************************************************
 *                              DEBUG DEFINITIONS                            *
 *****************************************************************************/

#ifndef SBSP_DEBUGGING
#define SBSP_DEBUGGING	0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define SBSP_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define SBSP_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define SBSP_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define SBSP_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define SBSP_DEBUG_LVL	SBSP_DEBUG_LVL_ERR

#define	GMSG_BUFLEN	256
#if SBSP_DEBUGGING == 1

/**
 * \def SBSP_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the sbsp library and is useful for confirming control flow
 *    through the sbsp module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by sbsp module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the sbsp module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * SBSP_DEBUGGING #define.
 */

   #define SBSP_DEBUG(level, format,...) if(level >= SBSP_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}

   #define SBSP_DEBUG_PROC(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define SBSP_DEBUG_INFO(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define SBSP_DEBUG_WARN(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define SBSP_DEBUG_ERR(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define SBSP_DEBUG(level, format,...) if(level >= SBSP_DEBUG_LVL) \
{}

   #define SBSP_DEBUG_PROC(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define SBSP_DEBUG_INFO(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define SBSP_DEBUG_WARN(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define SBSP_DEBUG_ERR(format,...) \
           SBSP_DEBUG(SBSP_DEBUG_LVL_ERR,format, __VA_ARGS__)


#endif

/*****************************************************************************
 *                      SBSP SPEC VARIABLE DEFINITIONS                      *
 *****************************************************************************/
/** sbsp rule type enumerations */
#define sbsp_TX			0
#define sbsp_RX			1

/** 
 * Block Types
 */
#define BLOCK_TYPE_PRIMARY	0x00	/*	Primary block type.	*/
#define BLOCK_TYPE_PAYLOAD	0x01	/*	Payload block type.	*/
#define BLOCK_TYPE_BIB  	0x03	/*	SBSP BIB block type.	*/
#define BLOCK_TYPE_BCB  	0x04	/*	SBSP BCB block type.	*/


/**
 * Ciphersuite Flags - From SBSP Spec.
 *
 * SEC_SRC: Block contains a security source EID.
 * PARM: Block contains ciphersuite parameters.
 * RES: Block contains a security result.
 */
#if 0
#define SBSP_ASB_SEC_SRC	0x04
#define SBSP_ASB_PARM		0x02
#define SBSP_ASB_RES		0x01
#endif
#define SBSP_ASB_PARM		0x01

/**
 * Ciphersuite and Security Result Item Types - SBSP spec Section 2.7.
 */
#define SBSP_CSPARM_IV           0x01
#define SBSP_CSPARM_KEY_INFO     0x03
#define SBSP_CSPARM_CONTENT_RNG  0x04
#define SBSP_CSPARM_INT_SIG      0x05
#define SBSP_CSPARM_SALT         0x07
#define SBSP_CSPARM_ICV          0x08

#define SBSP_KEY_NAME_LEN	 32


/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/

/** 
 *  \struct SbspInboundBlock
 *  \brief The block-type-specific data of an inbound SBSP extension block.
 * 
 * The SbspInboundBlock structure describes a SBSP block as it is being
 * received by a BP node.
 */
typedef struct
{
	EndpointId securitySource;
	uint8_t	   targetBlockType;
	uint8_t	   targetBlockOccurrence;
	uint8_t    instance;	    /*  0: 1st, lone.  1: last.		*/
	uint8_t	   ciphersuiteType;
	char	   keyName[SBSP_KEY_NAME_LEN];
	uint32_t   ciphersuiteFlags;
	uint32_t   parmsLen;	    /*  IFF flags & sbsp_ASB_PARM	*/
	uint8_t	  *parmsData;	    /*  IFF flags & sbsp_ASB_PARM	*/
	uint32_t   resultsLen;
	uint8_t	  *resultsData;
} SbspInboundBlock;



/** 
 *  \struct SbspOutboundBlock
 *  \brief The block-type-specific data of an outbound SBSP extension block.
 * 
 * The SbspOutboundBlock structure describes a SBSP block as it is being
 * prepared for transmission from a BP node.
 */
typedef struct
{
	EndpointId securitySource;
	uint8_t	   targetBlockType;
	uint8_t	   targetBlockOccurrence;
	uint8_t	   instance;	/*  0: 1st, lone.  1: last.		*/
	uint8_t	   encryptInPlace;	/*  Boolean			*/
	uint8_t	   ciphersuiteType;
	char       keyName[SBSP_KEY_NAME_LEN];
	uint32_t   ciphersuiteFlags;
	uint32_t   parmsLen;	/** IFF flags & sbsp_ASB_PARM		*/
	Object     parmsData;	/** IFF flags & sbsp_ASB_PARM		*/
	uint32_t   resultsLen;
	Object     resultsData;
} SbspOutboundBlock;


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/


extern unsigned char	*sbsp_addSdnvToStream(unsigned char *stream, Sdnv* val);


extern SdrObject	sbsp_build_sdr_parm(Sdr sdr, csi_cipherparms_t parms,
				uint32_t *len);

extern SdrObject	sbsp_build_sdr_result(Sdr sdr, uint8_t id,
				csi_val_t value, uint32_t *len);

extern int		sbsp_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk);

extern int		sbsp_destinationIsLocal(Bundle *bundle);

extern LystElt		sbsp_findAcqBlock(AcqWorkArea *wk, uint8_t type,
				uint8_t targetBlockType,
				uint8_t targetBlockOccurrence,
				uint8_t instance);

extern Object		sbsp_findBlock(Bundle *bundle, uint8_t type,
				uint8_t targetBlockType,
				uint8_t targetBlockOccurrence,
				uint8_t instance);

extern void		sbsp_getInboundItem(int itemNeeded, unsigned char *buf,
				unsigned int bufLen, unsigned char **val,
				unsigned int *len);

extern int		sbsp_getInboundSecurityEids(Bundle *bundle,
				AcqExtBlock *blk, SbspInboundBlock *asb,
				char **fromEid, char **toEid);

extern int		sbsp_getInboundSecuritySource(AcqExtBlock *blk,
				char *dictionary, char **fromEid);

extern char		*sbsp_getLocalAdminEid(char *eid);

extern void		sbsp_getOutboundItem(uint8_t itemNeeded, Object buf,
				uint32_t bufLen, Address *val, uint32_t *len);

extern int		sbsp_getOutboundSecurityEids(Bundle *bundle,
				ExtensionBlock *blk, SbspOutboundBlock *asb,
				char **fromEid, char **toEid);

extern int		sbsp_getOutboundSecuritySource(ExtensionBlock *blk,
				char *dictionary, char **fromEid);

extern void		sbsp_insertSecuritySource(Bundle *bundle,
				SbspOutboundBlock *asb);

extern csi_val_t	sbsp_retrieveKey(char *keyName);

extern int		sbsp_securityPolicyViolated(AcqWorkArea *wk);

extern int		sbsp_requiredBlockExists(AcqWorkArea *wk,
				uint8_t sbspBlockType, uint8_t targetBlockType,
				char *secSrcEid);

extern unsigned char	*sbsp_serializeASB(uint32_t *length,
				SbspOutboundBlock *blk);

extern int		sbsp_transferToZcoFileSource(Sdr sdr,
				Object *resultZco, Object *acqFileRef,
				char *fname, char *bytes, uvast length);

#endif /* _SBSP_UTIL_H_ */
