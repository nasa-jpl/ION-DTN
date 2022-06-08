/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bpsec_asb.h
 **
 ** Subsystem:
 **              BP Security Library (BSP) Abstract Security Block (ASB)
 **
 ** Namespace:
 **              bpsec_asb_
 **
 ** Description: This file provides all structures, variables, and function
 **              definitions necessary for operations associated with the
 **              Abstract Security Block (ASB) structure defined by the
 **              BPSec security extensions.
 **
 **              ASBs are logical structures holding security information for
 **              BIB and BCB security blocks. As such, ABSs must be deserialized
 **              from encoded blocks when processing inbound bundles and
 **              serialized into outbound blocks when transmitting
 **              bundles.
 **
 **              All inbound bpsec blocks are extracted from bundles created
 **              "remotely" (though possibly by the local node via BP loopback)
 **              and received locally. All bpsec blocks created locally are
 **              outbound bpsec blocks.  However, not all outbound bpsec blocks
 **              are created locally.  A bpsec block that was created remotely
 **              and acquired locally may need to be copied to an outbound bpsec
 **              block so that it may be forwarded to other nodes, just as locally
 **              created bpsec blocks must be.
 **
 **              For this purpose, ASB serialization is not needed, since the
 **              serialized form of the block is already present in the "bytes"
 **              array of the inbound block.  All that's needed is to copy the
 **              inbound block's "bytes" array to the SDR heap and store the
 **              address of the copy in the outbound block's "bytes" array.  The
 **              inbound block's ASB (scratchpad object) need not be referenced or
 **              re-serialized.  (Note that an inbound bpsec block must not be
 **              modified in any way before it is forwarded, as this would
 **              invalidate the block's sourceEID.)
 **
 **
 ** Assumptions:
 **      1. We assume that this code is not under such tight profiling
 **         constraints that sanity checks are too expensive.
 **
 ** Modification History:
 **
 **  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 **  --------  ------------  -----------------------------------------------
 **  12/31/21  E. Birrane           Extracted ASB functions from bpsec_util and
 **                                 updated for RFC9172 and RFC9173. (JHU/APL)
 *****************************************************************************/

#ifndef _BPSEC_ASB_H_

#define _BPSEC_ASB_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "lyst.h"
#include "bp.h"
#include "bpP.h"
#include "ionsec.h"
#include "bei.h"


/*
 * +--------------------------------------------------------------------------+
 * |                              CONSTANTS                                   +
 * +--------------------------------------------------------------------------+
 */
// TODO: Update to have BPSEC_ASB?
#define CBOR_MAX_UVAST_ENC_LENGTH (9)

/*
 * +--------------------------------------------------------------------------+
 * |                                MACROS                                    +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |                              DATA TYPES                                  +
 * +--------------------------------------------------------------------------+
 */




/**
 * \struct BpsecInboundTargetResult
 *
 * \brief  Security results associated with a BPSec target block.
 *
 * This structure holds the , &unparsedBytesresults associated with a single target block of
 * a BPSec security operation.
 *
 * Target blocks are identified by their unique block identifier.
 *
 * Each result is represented as a TLV consisting of a result ID, result
 * value length, and result encoding.  The encoding and decoding of security
 * results occurs within the security context used by the BPSec security
 * block.
 *
 */
typedef struct
{
    unsigned char scTargetId;           /* The target block within the bundle.        */
    Lyst          scIndTargetResults;   /* (sci_val *) target security individual results. */
} BpsecInboundTargetResult;



/**
 * \struct BpsecInboundASB
 *
 * \brief The block-type-specific data of an inbound bpsec extension block.
 *
 * This structure describes a BPSec extension block as it is being received
 * by a BP node.
 *
 * The BP node receives the serialized form of the BPSec extension block in
 * the "bytes" array of the AcqExtBlk structure which exists in private
 * working memory.
 *
 * The BpsecInboundASB represents the deserialized form of the BPSec
 * extension block generated from a call to bpsec_util_deserialize_ASB.
 *
 * The BpsecInboundASB structure exists in private working memory in the
 * "scratchpad" object of the AcqExtBlk representing the associated BPSec
 * extension block.
 *
 */
typedef struct
{
    EndpointId scSource;       /* BPSec block security source.              */

    uint16_t   scId;           /* The BPSec Security Context Identifier.    */
    uint32_t   scFlags;        /* BPSec Security Context Flags.             */

    Lyst       scParms;        /* (sci_val) Security Context parameters.    */

    Lyst       scResults;      /* (BpsecInboundTargetResult) Results by target.   */
} BpsecInboundASB;



/**
 * \struct BpsecOutboundTargetResult
 *
 * \brief  Security results associated with a BPSec target block.
 *
 * This structure holds the results associated with a single target block of
 * a BPSec security operation.
 *
 * Target blocks are identified by their unique block identifier.
 *
 * Each result is represented as a TLV consisting of a result ID, result
 * value length, and result encoding.  The encoding and decoding of security
 * results occurs within the security context used by the BPSec security
 * block.
 *
 * TODO: Do we really need scTargetId to be a uvast? What about uint16_t?
 */

typedef struct
{
    uvast  scTargetId; /* The target block within the bundle.     */
    Object scIndTargetResults;  /* sdr_list of security individual results. */
} BpsecOutboundTargetResult;



/**
 * \struct BpsecOutboundASB, &unparsedBytes
 *
 * \brief The block-type-specific data of an outbound BPSec extension block.
 *
 * This structure describes a BPSec extension block as it is being
 * prepared for transmission from a BP node.
 *
 * A BP node will create this outbound extension block using the ExtensionBlock
 * structure. The ExtensionBlock will (if created locally) keep a deserialized
 * ASB that resides in its SDR heap space, in the scratchpad "object" of the
 * ExtensionBlock.
 *
 * The serialized form of the block, resident in the block's "bytes" array
 * (in SDR heap space), is generated when the BpsecOutboundASB is passed
 * to bpsec_util_serialize_ASB.
 *
 */

typedef struct
{
    EndpointId scSource;       /* BPSec block security source.               */

    uint16_t   scId;           /* The BPSec Security Context Identifier.     */
    uint32_t   scFlags;        /* BPSec Security Context Flags.              */

    Object     scParms;        /* (sci_val) Security Context Parms. */

    Object     scResults;      /* (BpsecOutboundResult) sdr_list of results  */
} BpsecOutboundASB;


/*
 * +--------------------------------------------------------------------------+
 * |                          FUNCTION PROTOTYPES                             +
 * +--------------------------------------------------------------------------+
 */

// TODO: Consider which of these to make static and private?
// TODO: Update function names to match conventions.

BpsecInboundASB*              bpsec_asb_inboundAsbCreate();
void                          bpsec_asb_inboundAsbDelete(BpsecInboundASB *asb); // bpsec_releaseInboundAsb...
int                           bpsec_asb_inboundAsbDeserialize(AcqExtBlock *blk, AcqWorkArea *wk); // bpsec_deserializeASB
//BpsecInboundTargetResult*   bpsec_asb_inboundAsbResultCreate(unsigned char tgt_id, int memIdx);

int                           bpsec_asb_inboundAsbRecord(ExtensionBlock *newBlk, AcqExtBlock *oldBlk);
void                          bpsec_asb_inboundTargetResultRelease(LystElt item, void *tag); // bsu_outTgtReleaseCb

BpsecInboundTargetResult      *bpsec_asb_inboundTargetResultCreate(uvast tgt_id, int memIdx);
void bpsec_asb_inboundTargetResultRemove(LystElt tgtResultElt, LystElt secBlkElt);

Object                        bpsec_asb_outboundAsbCreate(Sdr sdr, unsigned int *size);
void                          bpsec_asb_outboundAsbDelete(Sdr sdr, BpsecOutboundASB *asb); // Consider calling this Clear and not Delete.
void                          bpsec_asb_outboundAsbDeleteObj(Sdr sdr, Object obj);
void                          bpsec_asb_outboundTargetResultsRelease(Sdr sdr, Object eltData, void *arg);
void                          bpsec_asb_outboundTargetResultDelete(Sdr sdr, Object result);
int                           bpsec_asb_outboundAsbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk); // bpsec_copyAsb
uint8_t                       *bpsec_asb_outboundAsbSerialize(uint32_t *length, BpsecOutboundASB *blk);// bpsec_serializeASB
int                           bpsec_asb_outboundParmsWrite(Sdr sdr, BpsecOutboundASB *asb, Lyst parms);
void                          bpsec_asb_outboundSecuritySourceInsert(Bundle *bundle, BpsecOutboundASB *asb); // bpsec_insertSecuritySource
int                           bpsec_asb_outboundTargetInsert(Sdr sdr, BpsecOutboundASB *asb, uint8_t nbr); // bpsec_insert_target



#endif /* _BPSEC_ASB_H_ */
