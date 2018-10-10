/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: ari.h
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
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/21/11  E. Birrane     Code comments and functional updates. (JHU/APL)
 **  10/22/12  E. Birrane     Update to latest version of DTNMP. Cleanup. (JHU/APL)
 **  06/25/13  E. Birrane     New spec. rev. Remove priority from MIDs (JHU/APL)
 **  04/19/16  E. Birrane     Put OIDs on stack and not heap. (Secure DTN - NASA: NNX14CS58P)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef MID_H_
#define MID_H_

#include "stdint.h"

#include "ion.h"

#include "../utils/debug.h"
#include "../utils/rhht.h"
#include "../utils/vector.h"
#include "../primitives/tnv.h"



/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Defines the bits comprising the MID flags as follows.
 *
 * Bits 0-3:  The struct id of the component identified by the MID
 * Bit 4: The issuer flag associated with the MID
 * Bit 5: The tag flag associated with the MID
 * Bits 6-7: The OID type encapsulated within the MID
 *
 */
#define ARI_FLAG_TYPE     (0x0F)
#define ARI_FLAG_TAG      (0x10)
#define ARI_FLAG_ISS      (0x20)
#define ARI_FLAG_PARM     (0x40)
#define ARI_FLAG_NN       (0x80)

/**
 * Maximum size, in bytes, supported for ARI fields.
 * WARNING: Changes to these values must be reflected in the associated ARI
 *          data type and associated functions.
 */

#define MAX_NAME_SIZE (4)
#define MAX_TAG_SIZE  (4)
#define MAX_ISS_SIZE  (8)


#define ARI_DEFAULT_ENC_SIZE 100
/**
 * Maximum size, in bytes, supported for MID fields.
 * WARNING: Changes to these values must be reflected in the associated MID
 *          data type and associated functions.
 */
#define MAX_TAG_LEN (4)
#define MAX_NAME_LEN (4)


/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

#define ARI_GET_FLAG_TYPE(flag) (flag & ARI_FLAG_TYPE)
#define ARI_GET_FLAG_TAG(flag)  ((flag & ARI_FLAG_TAG) >> 4)
#define ARI_GET_FLAG_ISS(flag)  ((flag & ARI_FLAG_ISS) >> 5)
#define ARI_GET_FLAG_PARM(flag) ((flag & ARI_FLAG_PARM) >> 6)
#define ARI_GET_FLAG_NN(flag)   ((flag & ARI_FLAG_NN) >> 7)


#define ARI_SET_FLAG_TYPE(flag, type) (flag |= (type & ARI_FLAG_TYPE))
#define ARI_SET_FLAG_TAG(flag)  (flag |= ARI_FLAG_TAG)
#define ARI_SET_FLAG_ISS(flag)  (flag |= ARI_FLAG_ISS)
#define ARI_SET_FLAG_PARM(flag) (flag |= ARI_FLAG_PARM)
#define ARI_SET_FLAG_NN(flag)   (flag |= ARI_FLAG_NN)


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



//Define the NN  array, the Issuer Array, the Tag Array.

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


typedef struct
{
	uint8_t flags;

	vec_idx_t nn_idx;

	vec_idx_t iss_idx;

	vec_idx_t tag_idx;  // Might make this a radix tree.

	blob_t name; // Might make this a radix tree.

	tnvc_t parms;

} ari_reg_t;


typedef struct {

	amp_type_e type;

	union {
		tnv_t     as_lit;
		ari_reg_t as_reg;
	};
} ari_t;


typedef struct {
	vector_t values; // Types as ari_t pointers.
} ac_t;

/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


/* ARI Functions */

int       ari_add_param(ari_t *ari, tnv_t parm);
int       ari_add_parms(ari_t *ari, tnvc_t *parms);

int       ari_cb_comp_fn(void *i1, void *i2);
void*     ari_cb_copy_fn(void *item);
void      ari_cb_del_fn(void *item);
rh_idx_t  ari_cb_hash(void *table, void *key);
void      ari_cb_ht_del(rh_elt_t *elt);

int       ari_compare(ari_t *ari1, ari_t *ari2);

ari_t     ari_copy(ari_t val, int *success);
ari_t*    ari_copy_ptr(ari_t ari);
ari_t*    ari_create();

ari_t     ari_deserialize(CborValue *it, int *success);
ari_t*    ari_deserialize_ptr(CborValue *it, int *success);
ari_t*    ari_deserialize_raw(blob_t *data, int *success);

ari_t*    ari_from_uvast(uvast val);
ari_t*    ari_from_parm_reg(uint8_t flags, uvast nn, uvast iss, blob_t *tag, blob_t *name, tnvc_t *parms);
tnv_t*    ari_get_param(ari_t *id, int i);

uint8_t   ari_get_num_parms(ari_t *ari);

void      ari_init(ari_t *ari);

ari_t     ari_null();

void      ari_release(ari_t *ari, int destroy);

int       ari_replace_parms(ari_t *ari, tnvc_t *new_parms);
tnvc_t*   ari_resolve_parms(tnvc_t *src_parms, tnvc_t *cur_parms);

CborError ari_serialize(CborEncoder *encoder, void *item);
blob_t*   ari_serialize_wrapper(ari_t *ari);

char*     ari_to_string(ari_t *ari);


/* ARI Collection Functions */
void      ac_clear(ac_t *ac);
ac_t*     ac_create();
int       ac_compare(ac_t *a1, ac_t *a2);
ac_t      ac_copy(ac_t *src);
ac_t*     ac_copy_ptr(ac_t *src);

ac_t      ac_deserialize(CborValue *it, int *success);
ac_t*     ac_deserialize_ptr(CborValue *it, int *success);

ari_t*    ac_get(ac_t* ac, uint8_t index);
uint8_t   ac_get_count(ac_t* ac);

int       ac_init(ac_t *ac);
int       ac_insert(ac_t* ac, ari_t *ari);

void      ac_release(ac_t *ac, int destroy);

CborError ac_serialize(CborEncoder *encoder, void *item);
blob_t*   ac_serialize_wrapper(ac_t *ac);





#endif
