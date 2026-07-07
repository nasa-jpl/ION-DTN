/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

// TODO: Update documentation
/*****************************************************************************
 **
 ** File Name: bpsec_util.h
 **
 **		Note: name changed from "sbsp.h", which conflicts with the
 **		name of the Board Support Package header file generated on
 **		installation of RTEMS.
 **
 ** Subsystem:
 **             Extensions: bpsec_util_
 **
 ** Description: This file provides all structures, variables, and function
 **              definitions necessary for a full implementation of Bundle
 **		 Protocol Security (bpsec).  This implementation utilizes
 **		 the ION Extension Interface to manage the creation,
 **		 modification, evaluation, and removal of bpsec blocks from
 **		 Bundle Protocol bundles.
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

#ifndef BPSEC_UTIL_H
#define BPSEC_UTIL_H

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/
#include "bp.h"
#include "bpP.h"
#include "ionsec.h"
#include "bei.h"
#include "csi.h"
#include "sci.h"
#include "sci_valmap.h"
#if !USING_BSL
#include "bpsec_policy.h"
#include "bpsec_policy_rule.h"
#endif

/*****************************************************************************
 *                              DEBUG DEFINITIONS                            *
 *****************************************************************************/

#ifndef BPSEC_DEBUGGING
#define BPSEC_DEBUGGING	1  /** Whether to enable (1) or disable (0) debugging */
#endif

#define BPSEC_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define BPSEC_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define BPSEC_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define BPSEC_DEBUG_LVL_ERR  4 /** Error and above debugging */

#ifndef BPSEC_DEBUG_LVL
#define BPSEC_DEBUG_LVL	BPSEC_DEBUG_LVL_ERR
#endif

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

#define BPSEC_DEBUG(level, prefix, format, ...)                          \
	if (level >= BPSEC_DEBUG_LVL)                                    \
	{                                                                \
		int dbg_len = _isprintf(__FILE__, __LINE__, gMsg,        \
				GMSG_BUFLEN, "%s: (%s) ",                \
				__func__, prefix);                       \
		_isprintf(__FILE__, __LINE__, gMsg + dbg_len,            \
				GMSG_BUFLEN - dbg_len, format,           \
				__VA_ARGS__);                            \
		putErrmsg(gMsg, NULL);                                   \
	}

#define BPSEC_DEBUG_PROC(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_PROC, ">", format, __VA_ARGS__)

#define BPSEC_DEBUG_INFO(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_INFO, "i", format, __VA_ARGS__)

#define BPSEC_DEBUG_WARN(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_WARN, "w", format, __VA_ARGS__)

#define BPSEC_DEBUG_ERR(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_ERR, "e", format, __VA_ARGS__)
#else
#define BPSEC_DEBUG(level, format, ...) \
	if (level >= BPSEC_DEBUG_LVL)   \
	{                               \
	}

#define BPSEC_DEBUG_PROC(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_PROC, format, __VA_ARGS__)

#define BPSEC_DEBUG_INFO(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_INFO, format, __VA_ARGS__)

#define BPSEC_DEBUG_WARN(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_WARN, format, __VA_ARGS__)

#define BPSEC_DEBUG_ERR(format, ...) \
	BPSEC_DEBUG(BPSEC_DEBUG_LVL_ERR, format, __VA_ARGS__)

#endif

/*****************************************************************************
 *                      bpsec SPEC VARIABLE DEFINITIONS                      *
 *****************************************************************************/
/** bpsec rule type enumerations */
// TODO: Can we remove these?
#define bpsec_TX		0
#define bpsec_RX		1

/**
 * Context Flags - From bpsec Spec.
 *
 * SEC_SRC:	Block contains a security source EID.
 * PARM:	Block contains context parameters.
 */
#define BPSEC_ASB_PARM		0x01

/**
 * Context and Security Result Item Types - bpsec spec Section 2.7.
 */
// TODO: Can we remove these?
#define BPSEC_CSPARM_IV           0x01
#define BPSEC_CSPARM_KEY_INFO     0x03
#define BPSEC_CSPARM_CONTENT_RNG  0x04
#define BPSEC_CSPARM_INT_SIG      0x05
#define BPSEC_CSPARM_SALT         0x07
#define BPSEC_CSPARM_ICV          0x08

#define BPSEC_KEY_NAME_LEN	  32


/*
 * TODO: Migrate blob_t from nm over into ici?
 */
typedef struct
{
	unsigned int   scSerializedLength;
	unsigned char *scSerializedText;
} BpsecSerializeData;


 /* --- BPSec Transaction Memory Arena --- */
#define MAX_BPSEC_TX_RESOURCES 64

typedef enum {
	RES_HEAP,
	RES_PSM,
	RES_SDR
} BpsecResourceType;

typedef struct {
	BpsecResourceType type;
	union {
		void *heap_ptr;
		PsmAddress psm_addr;
		SdrObject sdr_obj;
	} ref;
} BpsecResource;

typedef struct {
	Sdr sdr;
	PsmPartition wm;
	BpsecResource resources[MAX_BPSEC_TX_RESOURCES];
	int count;
} BpsecTxContext;


/*****************************************************************************
 *                             FUNCTION DEFINITIONS                          *
 *****************************************************************************/


 /* --- BPSec Transaction Memory Arena --- */
void       bpsec_ctx_init(BpsecTxContext *ctx, Sdr sdr, PsmPartition wm);
SdrObject  bpsec_ctx_sdr_malloc(BpsecTxContext *ctx, size_t size);
PsmAddress bpsec_ctx_psm_zalloc(BpsecTxContext *ctx, size_t size);
void* bpsec_ctx_mbuftake(BpsecTxContext *ctx, size_t size);
void       bpsec_ctx_abort(BpsecTxContext *ctx);
void       bpsec_ctx_commit(BpsecTxContext *ctx);


/* Information Retrieval Functions */
// Get long-term-key for a block given a rule and an ASB
// get session key for a block givn a rule and an ASB
// Get a security parm from asb, or rule, or default value. Default value from SC?
// See if target_exists_in_bundle...

//TODO: Fix function names to all confirm to naming conventions.

int           bpsec_util_EIDCopy(EndpointId *toEID, EndpointId *fromEID);
extern int    bpsec_util_canonicalizeOut(Bundle *bundle, uint8_t blkNbr, SdrObject *zcoOut);
extern int    bpsec_util_canonicalizeIn(AcqWorkArea *work, uint8_t blkNbr, SdrObject *zcoOut);
int bpsec_util_eidIsLocalCheck(EndpointId eid);

extern int      bpsec_util_destIsLocalCheck(Bundle *bundle);
extern char     *bpsec_util_localAdminEIDGet(char *eid);
extern void     bpsec_util_outboundItemGet(uint8_t itemNeeded, SdrObject items, SdrObject *tvp);
extern int      bpsec_util_zcoFileSourceTransferTo(Sdr sdr, ZcoAcct acct,
		SdrObject *resultZco, SdrObject *acqFileRef,
		char *fname, char *bytes, uvast length);
SdrObject	bspsec_util_findOutboundBpsecTargetBlock(Bundle *bundle, int tgtBlkNum, BpBlockType sopType);

unsigned char *bpsec_util_primaryBlkSerialize(Bundle *bundle, int *length);

void bpsec_util_inboundBlkClear(AcqExtBlock *blk);
void bpsec_util_outboundBlkRelease(ExtensionBlock *blk);

int bpsec_util_acqBlkDataAsZco(AcqExtBlock *blk, SdrObject *zco);

sc_value bpsec_util_keyRetrieve(char *keyName);

int32_t bpsec_util_sdrBlkConvert(uint32_t suite, uint8_t *context, csi_blocksize_t *blocksize,
                                 ZcoReader *dataReader, uvast outputBufLen, SdrObject *outputZco, uint8_t function);


int32_t bpsec_util_fileBlkConvert(uint32_t suite, uint8_t *csi_ctx, csi_blocksize_t *blocksize,
		ZcoReader *dataReader, uvast outputBufLen, SdrObject *outputZco, char *filename,
		uint8_t function);

/** Instrumentation support functions (needed by NM agent's BPSec ADM) */
int                     bpsec_util_numKeysGet(int *size);
void                    bpsec_util_keysGet(char *buffer, int length);
int                     bpsec_util_numCSNamesGet(int *size);
void                    bpsec_util_cSNamesGet(char *buffer, int length);

#if !USING_BSL
int bpsec_util_swapOutboundSop(Bundle *bundle, ExtensionBlock *src, ExtensionBlock *dest, int tgtBlkNum);

int bpsec_util_attachSecurityBlocks(Bundle *bundle, BpBlockType secBlkType, sc_action action);

LystElt bpsec_util_findInboundTarget(AcqWorkArea *work, int blockNumber, LystElt *bibElt);

int bpsec_util_checkSop(BpBlockType target, BpBlockType sec);
int bpsec_util_checkOutboundSopTarget(Bundle *bundle, sc_Def *def, PsmPartition wm, PsmAddress parms,
                                      BpBlockType sopType, int tgtBlkNum, SdrObject *bibBlk, SdrObject *secBlk);


SdrObject bpsec_util_OutboundBlockCreate(Bundle *bundle, BpBlockType type, sc_Def *def, PsmAddress parms);


int bpsec_util_generateSecurityResults(Bundle *bundle, char *fromEid, ExtensionBlock *secBlk, BpsecOutboundASB *secAsb, sc_action action, int *misconfTgtNum);
#endif /* !USING_BSL */


#endif /* BPSEC_UTIL_H */
