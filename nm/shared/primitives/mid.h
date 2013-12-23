/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
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
 ** File Name: mid.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of DTNMP Managed Identifiers (MIDs).
 **
 ** Notes:
 **
 ** Assumptions:
 **      1. We limit the size of an OID in the system to reduce the amount
 **         of pre-allocated memory in this embedded system. Non-embedded
 **         implementations may wish to dynamically allocate MIDs as they are
 **         received.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/21/11  E. Birrane     Code comments and functional updates.
 **  10/22/12  E. Birrane     Update to latest version of DTNMP. Cleanup.
 **  06/25/13  E. Birrane     New spec. rev. Remove priority from MIDs
 *****************************************************************************/

#ifndef MID_H_
#define MID_H_

#include "stdint.h"

#include "ion.h"

#include "shared/utils/debug.h"
#include "shared/primitives/oid.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Defines the bits comprising the MID flags as follows.
 *
 * Bits 0-1:  The type of component identified by the MID
 * Bits 2-3:  The category of component identified by the MID
 * Bit 4: The issuer flag associated with the MID
 * Bit 5: The tag flag associated with the MID
 * Bits 6-7: The OID type encapsulated within the MID
 *
 */
#define MID_FLAG_TYPE       (0x03)
#define MID_FLAG_CAT        (0x0C)
#define MID_FLAG_ISS        (0x10)
#define MID_FLAG_TAG        (0x20)
#define MID_FLAG_OID        (0xC0)


/**
 * MID TYPE DEFINITIONS
 *
 * DATA: Values sampled directly by the agent, and combinations thereof.
 * CONTROL: Functions that may be called by the agent.
 * LITERAL: Well-named Constants.
 * OPERATOR: Coded mathematical expression (special case of control).
 */
#define MID_TYPE_DATA 0
#define MID_TYPE_CONTROL 1
#define MID_TYPE_LITERAL 2
#define MID_TYPE_OPERATOR 3


/**
 * MID CATEGORY DEFINITIONS
 *
 * ATOMIC: Proper-named data from specification.
 * COMPUTED: Computed by mathematical expression of atomic/computed data values.
 * COLLECTION: List of items, atomic or computed.
 */
#define MID_CAT_ATOMIC 0
#define MID_CAT_COMPUTED 1
#define MID_CAT_COLLECTION 2



/**
 * Maximum size, in bytes, supported for MID fields.
 * WARNING: Changes to these values must be reflected in the associated MID
 *          data type and associated functions.
 */
#define MAX_TAG (8)
#define MAX_ISSUER (8)


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

#define MID_GET_FLAG_TYPE(flag) (flag & MID_FLAG_TYPE)
#define MID_GET_FLAG_CAT(flag)  ((flag & MID_FLAG_CAT) >> 2)
#define MID_GET_FLAG_ISS(flag)  ((flag & MID_FLAG_ISS) >> 4)
#define MID_GET_FLAG_TAG(flag)  ((flag & MID_FLAG_TAG) >> 5)
#define MID_GET_FLAG_OID(flag)  ((flag & MID_FLAG_OID) >> 6)


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Describes a Managed Identifier object.
 *
 * Notably with this structure is the fact that we keep a completely
 * serialized version of the MID in addition to bit-busting the values into
 * the other members of the structure.  This makes re-serialization of the MID
 * for re-use a very fast endeavor.
 *
 * The mid_internal_serialize function may be used to re-serialize this
 * internal representation if something changes (such changing the OID type
 * to expand a nickname).
 */
typedef struct {

    /** Flags describing optional MID elements */
    uint8_t flags;

    /** Type of the MID, one of MID_TYPE_[DATA|CONTROL|LITERAL|OPERATOR]. */
    uint8_t type;

    /** Category of the MID, one of MID_CAT_[ATOMIC|COMPUTED|COLLECTION]. */
    uint8_t category;
    
    /** Issuer, capped to largest atomic data type allowed by architecture */
    uvast issuer;    
    
    /** OID */
    oid_t *oid;
    
    /** Tag, capped to largest atomic data type allowed by architecture */
    uvast tag;
        
    /** The complete SDNV-encoded serialized MID. */
    uint8_t* raw;
    
    /** Size of the serialized mid in bytes. */
    uint32_t raw_size;
    
} mid_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int      mid_add_param(mid_t *mid, uint8_t *value, uint32_t len);

void     mid_clear(mid_t *mid);

int      mid_compare(mid_t *mid1, mid_t *mid2, uint8_t use_parms);

mid_t*   mid_construct(uint8_t type, uint8_t cat,
		               uvast *issuer, uvast *tag, oid_t *oid);

mid_t*   mid_copy(mid_t *src_mid);

mid_t*   mid_deserialize(unsigned char *buffer,
		                 uint32_t buffer_size,
		                 uint32_t *bytes_used);

int      mid_internal_serialize(mid_t *mid);

char*    mid_pretty_print(mid_t *mid);

void     mid_release(mid_t *mid);

int      mid_sanity_check(mid_t *mid);

uint8_t* mid_serialize(mid_t *mid, uint32_t *size);

char*    mid_to_string(mid_t *mid);

Lyst     midcol_copy(Lyst mids);

void     midcol_destroy(Lyst *mids);

Lyst     midcol_deserialize(unsigned char *buffer,
		                    uint32_t buffer_size,
		                    uint32_t *bytes_used);

char*    midcol_pretty_print(Lyst mc);

char*    midcol_to_string(Lyst mc);

uint8_t* midcol_serialize(Lyst mids, uint32_t *size);


#endif
