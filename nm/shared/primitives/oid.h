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
 **	     1. We do not support a "create" function for OIDs as, so far, any
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
 **  03/11/15  E. Birrane     Removed NN from OID into NN.
 *****************************************************************************/

#ifndef OID_H_
#define OID_H_

#include "stdint.h"

#include "platform.h"
#include "ion.h"
#include "lyst.h"
#include "dc.h"

#include "shared/utils/debug.h"
#include "shared/utils/utils.h"

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

    Lyst    params;                   /**> Of type datacol_entry_t */

    uvast nn_id;                      /**> Optional nickname for this OID. */

    uint8_t value[MAX_OID_SIZE];      /**> OID, sans nickname & parms. */
    uint32_t value_size;		   	  /**> Length in bytes of OID value (incl. room for the size) */
} oid_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int       oid_add_param(oid_t *oid, uint8_t *value, uint32_t len);

int       oid_add_params(oid_t *oid, Lyst params);

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

uint8_t  oid_get_num_parms(oid_t *oid);

datacol_entry_t *oid_get_param(oid_t *oid, int i);

char*     oid_pretty_print(oid_t *oid);

void      oid_release(oid_t *oid);

int       oid_sanity_check(oid_t *oid);

uint8_t*  oid_serialize(oid_t *oid, uint32_t *size);

char*     oid_to_string(oid_t *oid);


#endif /* OID_H_ */
