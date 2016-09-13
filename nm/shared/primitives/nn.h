/*****************************************************************************
 **
 ** File Name: nn.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for a global nickname database useful
 **              for caching frequently-reused portions of OIDs defined in ADMs.
 **
 ** Notes:
 **	     1. In the current implementation, the nickname database is not
 **	        persistent.
 **
 ** Assumptions:
 **      1. We limit the size of a nickname in the system to reduce the amount
 **         of pre-allocated memory in this embedded system. Non-embedded
 **         implementations may wish to dynamically allocate MIDs as they are
 **         received.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation. (Secure DTN - NASA: NNX14CS58P)
 **  03/11/15  E. Birrane     Pulled nicknamed out of OID into NN. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef NN_H_
#define NN_H_

#include "stdint.h"

#include "platform.h"
#include "ion.h"
#include "lyst.h"


#include "../utils/debug.h"



/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * Maximum size, in bytes, supported by any NN in the system.
 */
#define MAX_NN_SIZE (32)

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Describes a nickname database entry.
 *
 * The nickname database maps unique identifiers to a full or partial
 * OID representation.  The encapsulated partial/full OID may be used in place
 * of the nickname when re-constructing an OID from a protocol data unit.
 */
typedef struct {
	uvast id;				   /**> The nickname identifier. */
	char adm_name[16];
	char adm_ver[16];
	uint8_t raw[MAX_NN_SIZE];  /**> The OID representing the expansion*/
	uint32_t raw_size;         /**> Size of the expansion OID. */
} oid_nn_t;


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


int oid_nn_add(oid_nn_t *nn);

int oid_nn_add_parm(uvast id, char *oid, char *name, char *version);

void      oid_nn_cleanup();

int       oid_nn_delete(uvast nn_id);

int 	  oid_nn_equals(oid_nn_t *nn1, oid_nn_t *nn2);

LystElt   oid_nn_exists(uvast nn_id);

oid_nn_t* oid_nn_find(uvast nn_id);

int       oid_nn_init();


#endif /* NN_H_ */
