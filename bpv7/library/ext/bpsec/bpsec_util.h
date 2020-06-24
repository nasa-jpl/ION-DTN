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

#if (BPSEC_DEBUGGING == 1)
#define DEBUGGING 1

extern char	gMsg[GMSG_BUFLEN];

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

/*	Notes on data structures:

An inbound bpsec block is an AcqExtBlk, which originally has a serialized form
represented in the "bytes" array (in private working memory) and acquires a
deserialized form - represented as a BpsecInboundBlock (an ASB), residing in
private working memory, in the scratchpad "object" of the AcqExtBlk - when the
AcqExtBlk is passed to bpsec_deserializeASB().

An outbound bpsec block is an ExtensionBlock, which (if created locally)
originally has a deserialized form - represented as a BpsecOutboundBlock
(an ASB), residing in SDR heap space, in the scratchpad "object" of the
ExtensionBlock.  In this case the serialized form of a block, represented
in the block's "bytes" array (in SDR heap space), is generated when the
BpsecOutboundBlock is passed to bpsec_serializeASB().

All inbound bpsec blocks are extracted from bundles created remotely and
received locally.  All bpsec blocks created locally are outbound bpsec blocks.
However, not all outbound bpsec blocks are created locally.  A bpsec block
that was created remotely may need to be copied to an outbound bpsec block
so that it may be forwarded to other nodes, just as locally created bpsec
blocks must be.

For this purpose, ASB serialization is not needed, since the serialized
form of the block is already present in the "bytes" array of the inbound
block.  All that's needed is to copy the inbound block's "bytes" array to
the SDR heap and store the address of the copy in the outbound block's
"bytes" array.  The inbound block's ASB (scratchpad object) need not be
referenced or re-serialized.  (Note that an inbound bpsec block must not
be modified in any way before it is forwarded, as this would invalidate
the block's sourceEID.)							*/

/*****************************************************************************
 *                        Inbound DATA STRUCTURES                            *
 *****************************************************************************/

/** 
 *  \struct BpsecInboundTarget
 *  \brief  The target of one of an inbound bpsec block's security operations.
 * 
 * The BpsecInboundTarget structure characterizes one target of a bpsec block
 * that is being received by a BP node.
 */
typedef struct
{
	uint8_t		targetBlockNumber;
	BpBlockType	targetBlockType;
	uint8_t		metatargetBlockNumber;
	BpBlockType	metatargetBlockType;
	Lyst		results;	/*	Lyst of csi_inbound_tvs	*/
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
	Lyst	   targets;	/*	Lyst of BpsecInboundTargets	*/
	uint8_t	   contextId;
	char	   keyName[BPSEC_KEY_NAME_LEN];
	uint32_t   contextFlags;
	Lyst	   parmsData;	/*	Lyst of csi_inbound_tvs		*/
} BpsecInboundBlock;

/*****************************************************************************
 *                       Outbound DATA STRUCTURES                            *
 *****************************************************************************/

/** 
 *  \struct BpsecOutboundTarget
 *  \brief  The target of one of an outbound bpsec block's security operations.
 * 
 * The BpsecOutboundTarget structure characterizes one target of a bpsec block
 * that is being prepared for transmission from a BP node.
 */
typedef struct
{
	uvast     id;
	uint32_t  length;
	Object    value;	/*	ID-dependent structure		*/
} BpsecOutboundTlv;

typedef struct
{
	uint8_t		targetBlockNumber;
	BpBlockType	targetBlockType;
	uint8_t		metatargetBlockNumber;
	BpBlockType	metatargetBlockType;
	Object		results;	/*	BpsecOutboundTlv sdr_list*/
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
	Object     parmsData;	/*	sdr_list of BpsecOutboundTlv	*/
} BpsecOutboundBlock;

/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/

extern void		bpsec_releaseInboundTlvs(Lyst tlvs);
extern void		bpsec_releaseInboundTargets(Lyst targets);
extern void		bpsec_releaseInboundAsb(BpsecInboundBlock *asb);
extern void		bpsec_releaseOutboundTlvs(Sdr sdr, Object tlvs);
extern void		bpsec_releaseOutboundTargets(Sdr sdr, Object targets);
extern void		bpsec_releaseOutboundAsb(Sdr sdr, Object asb);

extern int		bpsec_recordAsb(ExtensionBlock *newBlk,
				AcqExtBlock *oldBlk);
extern int		bpsec_copyAsb(ExtensionBlock *newBlk,
				ExtensionBlock *oldBlk);

extern int		bpsec_getInboundTarget(Lyst targets,
				BpsecInboundTarget **target);
extern int		bpsec_getOutboundTarget(Sdr sdr, Object targets,
				BpsecOutboundTarget *target);

extern int		bpsec_write_parms(Sdr sdr, BpsecOutboundBlock *asb,
				sci_inbound_parms *parms);

extern int		bpsec_write_one_result(Sdr sdr, BpsecOutboundBlock *asb,
				sci_inbound_tlv *tlv);

extern int		bpsec_insert_target(Sdr sdr, BpsecOutboundBlock *asb,
				uint8_t nbr, uint8_t type, uint8_t mnbr,
				uint8_t mtyp);

extern int		bpsec_deserializeASB(AcqExtBlock *blk, AcqWorkArea *wk);

extern int		bpsec_destinationIsLocal(Bundle *bundle);
#if 0
extern LystElt		bpsec_findAcqBlock(AcqWorkArea *wk, BpBlockType type,
				BpBlockType targetBlockType,
				BpBlockType metatargetBlockType);
#endif
extern Object		bpsec_findBlock(Bundle *bundle, BpBlockType type,
				BpBlockType targetBlockType,
				BpBlockType metatargetBlockType);
#if 0
extern void		bpsec_getInboundItem(int itemNeeded, unsigned char *buf,
				unsigned int bufLen, unsigned char **val,
				unsigned int *len);
#endif
extern int		bpsec_getInboundSecurityEids(Bundle *bundle,
				AcqExtBlock *blk, BpsecInboundBlock *asb,
				char **fromEid, char **toEid);
#if 0
extern int		bpsec_getInboundSecuritySource(AcqExtBlock *blk,
				char *dictionary, char **fromEid);
#endif
extern char		*bpsec_getLocalAdminEid(char *eid);

extern void		bpsec_getOutboundItem(uint8_t itemNeeded, Object items,
				Object *tvp);

extern int		bpsec_getOutboundSecurityEids(Bundle *bundle,
				ExtensionBlock *blk, BpsecOutboundBlock *asb,
				char **fromEid, char **toEid);
#if 0
extern int		bpsec_getOutboundSecuritySource(ExtensionBlock *blk,
				char *dictionary, char **fromEid);
#endif
extern void		bpsec_insertSecuritySource(Bundle *bundle,
				BpsecOutboundBlock *asb);

extern sci_inbound_tlv	bpsec_retrieveKey(char *keyName);

extern int		bpsec_securityPolicyViolated(AcqWorkArea *wk);

extern int		bpsec_requiredBlockExists(AcqWorkArea *wk,
				BpBlockType bpsecBlockType,
				BpBlockType targetBlockType,
				char *secSrcEid);

extern unsigned char	*bpsec_serializeASB(uint32_t *length,
				BpsecOutboundBlock *blk);

extern int		bpsec_transferToZcoFileSource(Sdr sdr,
				Object *resultZco, Object *acqFileRef,
				char *fname, char *bytes, uvast length);

#endif /* _BPSEC_UTIL_H_ */
