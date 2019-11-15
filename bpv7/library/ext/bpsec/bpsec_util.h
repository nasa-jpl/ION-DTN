/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
 
/*****************************************************************************
 **
 ** File Name: bpsec_util.h
 **
 **		Note: name changed from "sbsp.h", which conflicts with the
 **		name of the Board Support Package header file generated on
 **		installation of RTEMS.
 **
 ** Subsystem:
 **             Extensions: bpsec
 **
 ** Description: This file provides all structures, variables, and function 
 **              definitions necessary for a full implementation of Bundle
 **		 Protocol Security (bpsec).  This implementation utilizes
 **		 the ION Extension Interface to manage the creation,
 **		 modification, evaluation, and removal of bpsec blocks from
 **		 Bundle Protocol bundles.
 **
 ** Notes:  The original implementation of this file (6/2009) only supported
 **         the Bundle Authentication Block (BAB) with the following
 **         constraints:
 **         - Bundle fragmentation is not considered.
 **         - Only the HMAC-SHA1 context for BAB is considered.
 **         - No context parameters are utilized or supported.
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
 **  09/02/19  S. Burleigh          Rename everything for bpsec
 **                                 [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef _BPSEC_UTIL_H_
#define _BPSEC_UTIL_H_

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

#ifndef BPSEC_DEBUGGING
#define BPSEC_DEBUGGING	0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BPSEC_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BPSEC_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BPSEC_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BPSEC_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define BPSEC_DEBUG_LVL	BPSEC_DEBUG_LVL_ERR

#define	GMSG_BUFLEN	256
#if BPSEC_DEBUGGING == 1

/**
 * \def BPSEC_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the bpsec library and is useful for confirming control flow
 *    through the bpsec module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by bpsec module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on runtime context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the bpsec module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * BPSEC_DEBUGGING #define.
 */

   #define BPSEC_DEBUG(level, format,...) if(level >= BPSEC_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gMsg, NULL);}

   #define BPSEC_DEBUG_PROC(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BPSEC_DEBUG_INFO(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BPSEC_DEBUG_WARN(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BPSEC_DEBUG_ERR(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define BPSEC_DEBUG(level, format,...) if(level >= BPSEC_DEBUG_LVL) \
{}

   #define BPSEC_DEBUG_PROC(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define BPSEC_DEBUG_INFO(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define BPSEC_DEBUG_WARN(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define BPSEC_DEBUG_ERR(format,...) \
           BPSEC_DEBUG(BPSEC_DEBUG_LVL_ERR,format, __VA_ARGS__)


#endif

/*****************************************************************************
 *                      bpsec SPEC VARIABLE DEFINITIONS                      *
 *****************************************************************************/
/** bpsec rule type enumerations */
#define bpsec_TX		0
#define bpsec_RX		1

/** 
 * Block Types
 */
#define BLOCK_TYPE_PRIMARY	0x00	/*	Primary block type.	*/
#define BLOCK_TYPE_PAYLOAD	0x01	/*	Payload block type.	*/
#define BLOCK_TYPE_BIB  	0x03	/*	bpsec BIB block type.	*/
#define BLOCK_TYPE_BCB  	0x04	/*	bpsec BCB block type.	*/


/**
 * Context Flags - From bpsec Spec.
 *
 * SEC_SRC:	Block contains a security source EID.
 * PARM:	Block contains context parameters.
 */
#define BPSEC_ASB_SEC_SRC	0x02
#define BPSEC_ASB_PARM		0x01

/**
 * Context and Security Result Item Types - bpsec spec Section 2.7.
 */
#define BPSEC_CSPARM_IV           0x01
#define BPSEC_CSPARM_KEY_INFO     0x03
#define BPSEC_CSPARM_CONTENT_RNG  0x04
#define BPSEC_CSPARM_INT_SIG      0x05
#define BPSEC_CSPARM_SALT         0x07
#define BPSEC_CSPARM_ICV          0x08

#define BPSEC_KEY_NAME_LEN	 32


/*****************************************************************************
 *                        Inbound DATA STRUCTURES                            *
 *****************************************************************************/

typedef struct
{
	uvast	   id;
	uint32_t   length;
	void	   *value;	/*	ID-dependent structure		*/
} BpsecInboundTv;

/** 
 *  \struct BpsecInboundTarget
 *  \brief  The target of one of an inbound bpsec block's security operations.
 * 
 * The BpsecInboundTarget structure characterizes one target of a bpsec block
 * that is being received by a BP node.
 */
typedef struct
{
	uint8_t	   targetBlockNumber;
	uint8_t	   targetBlockType;
	uint8_t	   metatargetBlockNumber;
	uint8_t	   metatargetBlockType;
	Lyst	   results;	/*	Lyst of BpsecInboundTv		*/
} BpsecInboundTarget;

/** 
 *  \struct BpsecInboundBlock
 *  \brief The block-type-specific data of an inbound bpsec extension block.
 * 
 * The BpsecInboundBlock structure describes a bpsec block as it is being
 * received by a BP node.
 */
typedef struct
{
	EndpointId securitySource;
	Lyst	   targets;	    /*	Lyst of BpsecInboundTarget	*/
	uint8_t	   contextId;
	char	   keyName[BPSEC_KEY_NAME_LEN];
	uint32_t   contextFlags;
	Lyst	   parmsData;	/*	Lyst of BpsecInboundTv		*/
} BpsecInboundBlock;

/*****************************************************************************
 *                       Outbound DATA STRUCTURES                            *
 *****************************************************************************/

typedef struct
{
	uvast	   id;
	uint32_t   length;
	Object	   value;	 /*	ID-dependent structure		*/
} BpsecOutboundTv;

/** 
 *  \struct BpsecOutboundTarget
 *  \brief  The target of one of an outbound bpsec block's security operations.
 * 
 * The BpsecOutboundTarget structure characterizes one target of a bpsec block
 * that is being prepared for transmission from a BP node.
 */
typedef struct
{
	uint8_t	   targetBlockNumber;
	uint8_t	   targetBlockType;
	uint8_t	   metatargetBlockNumber;
	uint8_t	   metatargetBlockType;
	Object	   results;	/*	sdr_list of BpsecOutboundTv	*/
} BpsecOutboundTarget;

/** 
 *  \struct BpsecOutboundBlock
 *  \brief The block-type-specific data of an outbound bpsec extension block.
 * 
 * The BpsecOutboundBlock structure describes a bpsec block as it is being
 * prepared for transmission from a BP node.
 */
typedef struct
{
	EndpointId securitySource;
	Object	   targets;	/*	sdr_list of BpsecOutboundTarget	*/
	uint8_t	   encryptInPlace;	/*  Boolean			*/
	uint8_t	   contextId;
	char       keyName[BPSEC_KEY_NAME_LEN];
	uint32_t   contextFlags;
	Object     parmsData;	/*	sdr_list of BpsecOutboundTv	*/
} BpsecOutboundBlock;


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/


extern SdrObject	bpsec_build_sdr_parm(Sdr sdr, csi_cipherparms_t parms,
				uint32_t *len);

extern SdrObject	bpsec_build_sdr_result(Sdr sdr, uint8_t id,
				csi_val_t value, uint32_t *len);

extern int		bpsec_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk);

extern int		bpsec_destinationIsLocal(Bundle *bundle);

extern LystElt		bpsec_findAcqBlock(AcqWorkArea *wk, uint8_t type,
				uint8_t targetBlockType,
				uint8_t metatargetBlockOccurrence);

extern Object		bpsec_findBlock(Bundle *bundle, uint8_t type,
				uint8_t targetBlockType,
				uint8_t metatargetBlockType);

extern void		bpsec_getInboundItem(int itemNeeded, unsigned char *buf,
				unsigned int bufLen, unsigned char **val,
				unsigned int *len);

extern int		bpsec_getInboundSecurityEids(Bundle *bundle,
				AcqExtBlock *blk, BpsecInboundBlock *asb,
				char **fromEid, char **toEid);

extern int		bpsec_getInboundSecuritySource(AcqExtBlock *blk,
				char *dictionary, char **fromEid);

extern char		*bpsec_getLocalAdminEid(char *eid);

extern void		bpsec_getOutboundItem(uint8_t itemNeeded, Object buf,
				uint32_t bufLen, Address *val, uint32_t *len);

extern int		bpsec_getOutboundSecurityEids(Bundle *bundle,
				ExtensionBlock *blk, BpsecOutboundBlock *asb,
				char **fromEid, char **toEid);

extern int		bpsec_getOutboundSecuritySource(ExtensionBlock *blk,
				char *dictionary, char **fromEid);

extern void		bpsec_insertSecuritySource(Bundle *bundle,
				BpsecOutboundBlock *asb);

extern csi_val_t	bpsec_retrieveKey(char *keyName);

extern int		bpsec_securityPolicyViolated(AcqWorkArea *wk);

extern int		bpsec_requiredBlockExists(AcqWorkArea *wk,
				uint8_t bpsecBlockType, uint8_t targetBlockType,
				char *secSrcEid);

extern unsigned char	*bpsec_serializeASB(uint32_t *length,
				BpsecOutboundBlock *blk);

extern int		bpsec_transferToZcoFileSource(Sdr sdr,
				Object *resultZco, Object *acqFileRef,
				char *fname, char *bytes, uvast length);

#endif /* _BPSEC_UTIL_H_ */
