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
 ** File Name: oid.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Object Identifiers (OIDs) and the OID nickname
 **              database.
 **
 ** Notes:
 **	     1. In the current implementation, the nickname database is not
 **	        persistent.
 **	     2. We do not support a "create" function for OIDs as, so far, any
 **	        need to create OIDs can be met by calling the appropriate
 **	        deserialize function.
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
 **  10/27/12  E. Birrane     Initial Implementation
 **  11/13/12  E. Birrane     Technical review, comment updates.
 *****************************************************************************/

#ifndef OID_H_
#define OID_H_

#include "stdint.h"

#include "platform.h"
#include "ion.h"
#include "lyst.h"


#include "shared/utils/debug.h"



/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * OID TYPES
 *
 * FULL: SNMP ASN.1 OID encoded with BER.
 * 	EX: <OID>
 * PARAM: OID with parameters appended.
 * 	EX: <#P><OID ROOT><P1><P2>...<Pn>
 * COMP_FULL: Compressed, full OID
 * 	EX: <nickname><subtree rooted at nickname>
 * COMP_PARAM: COmpressed, parameterized OID
 * 	EX: <#P><nickname><subtree><P1><P2>...<Pn>
 */
#define OID_TYPE_FULL 0
#define OID_TYPE_PARAM 1
#define OID_TYPE_COMP_FULL 2
#define OID_TYPE_COMP_PARAM 3



/**
 * Maximum size, in bytes, supported by any OID in the system.
 * WARNING: Changes to these values must be reflected in the associated MID
 *          data types and associated functions.
 */
#define MAX_OID_SIZE (32)
#define MAX_OID_PARM (5)

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Describes an OID nickname database entry.
 *
 * The OID nickname database maps unique identifiers to a full or partial
 * OID representation.  The encapsulated partial/full OID may be used in place
 * of the nickname when re-constructing an OID from a protocol data unit.
 */
typedef struct {
	uvast id;				/**> The nickname identifier. */
	uint8_t raw[MAX_OID_SIZE];  /**> The OID representing the expansion*/
	uint32_t raw_size;          /**> Size of the expansion OID. */
} oid_nn_t;



/**
 * Describes an Object Identifier (OID).
 *
 * RAW OID Handling:
 * The value and value_size members capture the raw OID as originally
 * communicated when the OID was defined.  For a FULL OID, this is the full OID.
 * For parameterized OIDs, this is the common (non-parameterized) part of the
 * OID.  For a compressed OID, this is the unique part of the OID that is not
 * elided behind the nickname. The value includes the # bytes portion.
 *
 * PARAMETER HANDLING:
 * The num_parm member stores the number of parameters in the OID.
 * The raw_parms and parms_size capture the serialized parameters (including
 * size SDNV) and the size of the serialized parameter buffer, respectively.
 * The parm_idx array holds a series of indices into raw_parms identifying the
 * start of the ith parameter SDNV within raw_parms.  For example, to decode
 * the 3rd parameter, the SDNV string would start at raw_parms[parm_idx[3]].
 *
 * NICKNAME HANDLING
 * The nn_id holds the identifier of the OID nickname.  The nickname expansion
 * is not represented in this structure.
 *
 * \todo
 * 	- This structure is huge. Will need to migrate this to dynamic memory
 * 	once we achieve baseline functionality.
 */
typedef struct {
    
    uint8_t type;					  /**> Type of OID. */

    Lyst    params;

   // uint32_t num_parm;                /**> Number of parameters in the OID */
   // uint32_t parm_idx[MAX_OID_PARM];  /**> Index into raw_parms of ith parm */

   // uint8_t raw_parms[MAX_OID_SIZE];  /**> Raw parms (including # parms) */
   // uint32_t parms_size;			  /**> Size of all parms (w/ # parms) */

    uvast nn_id;                   /**> Optional nickname for this OID. */

    uint8_t value[MAX_OID_SIZE];      /**> OID, sans nickname & parms. */
    uint32_t value_size;		   	  /**> Length in bytes of OID value (incl. room for the size) */
} oid_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */

/**
 * \todo Migrate this to a more efficient structure, and make persistent.
 */
extern Lyst nn_db;
extern ResourceLock nn_db_mutex;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int       oid_add_param(oid_t *oid, uint8_t *value, uint32_t len);

uint32_t  oid_calc_size(oid_t *oid);

void      oid_clear(oid_t *oid);

int       oid_compare(oid_t *oid1, oid_t *oid2, uint8_t use_parms);

oid_t*    oid_copy(oid_t *src_oid);

oid_t*    oid_deserialize_comp(unsigned char *buffer,
		  	  	  	  	  	   uint32_t size,
		                       uint32_t *bytes_used);

oid_t*    oid_deserialize_comp_param(unsigned char *buffer,
    				 	             uint32_t size,
    					             uint32_t *bytes_used);

oid_t*    oid_deserialize_full(unsigned char *buffer,
							   uint32_t size,
			                   uint32_t *bytes_used);

oid_t*    oid_deserialize_param(unsigned char *buffer,
		    	   	   	   	    uint32_t size,
		                        uint32_t *bytes_used);

char*     oid_pretty_print(oid_t *oid);

void      oid_release(oid_t *oid);

int       oid_sanity_check(oid_t *oid);

uint8_t*  oid_serialize(oid_t *oid, uint32_t *size);

char*     oid_to_string(oid_t *oid);

int       oid_nn_add(oid_nn_t *nn);

void      oid_nn_cleanup();

int       oid_nn_delete(uvast nn_id);

LystElt   oid_nn_exists(uvast nn_id);

oid_nn_t* oid_nn_find(uvast nn_id);

int       oid_nn_init();


#endif /* OID_H_ */
