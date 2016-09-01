/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **  10/27/12  E. Birrane     Initial Implementation (JHU/APL)
 **  11/13/12  E. Birrane     Technical review, comment updates. (JHU/APL)
 **  03/11/15  E. Birrane     Removed NN from OID into NN. (Secure DTN - NASA: NNX14CS58P)
 **  08/29/15  E. Birrane     Removed length restriction from OID parms. (Secure DTN - NASA: NNX14CS58P)
 **  06/11/16  E. Birrane     Updated parameters to be of type TDC. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef OID_H_
#define OID_H_

#include "stdint.h"

#include "platform.h"
#include "ion.h"
#include "lyst.h"
#include "dc.h"

#include "../utils/debug.h"
#include "../utils/utils.h"
#include "../primitives/tdc.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * OID TYPES
 */
#define OID_TYPE_FULL 0
#define OID_TYPE_PARAM 1
#define OID_TYPE_COMP_FULL 2
#define OID_TYPE_COMP_PARAM 3
#define OID_TYPE_UNK 4


/**
 * Maximum size, in bytes, supported by any OID in the system.
 * WARNING: Changes to these values must be reflected in the associated MID
 *          data types and associated functions.
 */
#define MAX_OID_SIZE (16)

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
 * The params lyst is a Data Collection of the parameters for this OID.
 *
 * NICKNAME HANDLING
 * The nn_id holds the identifier of the OID nickname.  The nickname expansion
 * is not represented in this structure.
 */
typedef struct {
    
    uint8_t type;					  /**> Type of OID. */

    tdc_t   params;                   /**> Typed data collection of parameters */

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


int8_t   oid_add_param(oid_t *oid, amp_type_e type, blob_t *blob);

int8_t   oid_add_params(oid_t *oid, tdc_t *params);

void     oid_clear(oid_t* oid);

void	 oid_clear_parms(oid_t* oid);

int8_t   oid_compare(oid_t oid1, oid_t oid2, uint8_t use_parms);

oid_t    oid_construct(uint8_t type, tdc_t *parms, uvast nn_id, uint8_t *value, uint32_t size);

oid_t    oid_copy(oid_t src_oid);

uint8_t  oid_copy_parms(oid_t* dest, oid_t* src);

oid_t    oid_deserialize_comp(unsigned char *buffer,
		  	  	  	  	  	   uint32_t size,
		                       uint32_t *bytes_used);

oid_t    oid_deserialize_comp_param(unsigned char *buffer,
    				 	             uint32_t size,
    					             uint32_t *bytes_used);

oid_t    oid_deserialize_full(unsigned char *buffer,
							   uint32_t size,
			                   uint32_t *bytes_used);

oid_t    oid_deserialize_param(unsigned char *buffer,
		    	   	   	   	    uint32_t size,
		                        uint32_t *bytes_used);

oid_t    oid_get_null();

int8_t   oid_get_num_parms(oid_t oid);

blob_t*  oid_get_param(oid_t oid, uint32_t idx, amp_type_e *type);

void	 oid_init(oid_t *oid);

char*    oid_pretty_print(oid_t oid);

void     oid_release(oid_t* oid);

uint8_t* oid_serialize(oid_t oid, uint32_t *size);

char*    oid_to_string(oid_t oid);


#endif /* OID_H_ */
