/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: ari.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of AMM Resource Identifiers (ARIs). Every object in
 **              the AMM can be uniquely identified using an ARI.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  09/18/18  E. Birrane     Initial implementation ported from previous
 **                           implementation of MIDs for earlier AMP spec (JHU/APL)
 *****************************************************************************/

#ifndef ARI_H_
#define ARI_H_

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
 * Defines the bits comprising the ARI flags as follows.
 *
 * Bits 0-3:  The type of AMM Object identified by this ARI.
 * Bit 4:     Whether the Tag field for this ARI is present.
 * Bit 5:     Whether the Issue field for this ARI is present.
 * Bit 6:     Whether this ARI contains parameters.
 * Bit 7:     Whether this ARI uses Nicknames to compress its regular name.
 *
 */
#define ARI_FLAG_TYPE     (0x0F)
#define ARI_FLAG_TAG      (0x10)
#define ARI_FLAG_ISS      (0x20)
#define ARI_FLAG_PARM     (0x40)
#define ARI_FLAG_NN       (0x80)

/*
 * When encoding an ARI, the default encoding size to try.
 */
#define ARI_DEFAULT_ENC_SIZE 100



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


/**
 * Describes a "regular" AMM Resource Identifier
 *
 * A regular ARI identifies an object whose value is not itself captured in the
 * identifier. Put another way, a regular ARI is one what identifies anything'
 * other than a literal value.
 *
 * Nicknames, Issuers, and Tags are stored in databases on the Agent and the
 * Manager and the ARI structure stores an index into these databases. This is
 * done for space compression. For example, a nickname may be up to 64 bits in
 * length whereas an index into the NN database can be 16 bits. This results in
 * an average savings of 18 bytes for ARIs that use nicknames, issuers, and
 * 64-bit tags.
 *
 * The name field is an unparsed bytestring whose value is determined by naming
 * rules for the ADM or user that defines the object being identified.
 *
 * \todo: Nicknames, Issuers, Tags, and Names might all more efficiently by
 *        stored as an index into a tree structure, such as a radix tree.
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


/*
 * Defines a general-purpose ARI structure.
 *
 * The ARI being captured here can be either a "regular" ARI,
 * in which case the structure should be interpreted as a
 * ari_reg_t structure or a "literal" ARI, in which case the
 * structure can be interpreted as a type/name/value.
 *
 * The use of a separate type field is redundant in that type
 * information is already captured in both the "regular" ARI
 * flag and the TYPE part of a literal ARI. However, extracting
 * the type information makes processing simpler and any other
 * method of distinguishing a regular and literal ARI would
 * likely use up at least a byte of space anyway.
 */

typedef struct {

	amp_type_e type;

	union {
		tnv_t     as_lit;
		ari_reg_t as_reg;
	};
} ari_t;



/*
 * An ARI Collection (AC) is an ordered collection of ARI structures.
 *
 * This is modeled as a vector acting as a stack.
 */
typedef struct {
	vector_t values; /* (ari_t*) */
} ac_t;



/*
 * +--------------------------------------------------------------------------+
 * |			  FUNCTION PROTOTYPES  				  +
 * +--------------------------------------------------------------------------+
 */


/* ARI Functions */

int       ari_add_parm_set(ari_t *ari, tnvc_t *parms);
int       ari_add_parm_val(ari_t *ari, tnv_t *parm);
int       ari_cb_comp_fn(void *i1, void *i2);
int       ari_cb_comp_no_parm_fn(void *i1, void *i2);
void*     ari_cb_copy_fn(void *item);
void      ari_cb_del_fn(void *item);
rh_idx_t  ari_cb_hash(void *table, void *key);
void      ari_cb_ht_del(rh_elt_t *elt);
int       ari_compare(ari_t *ari1, ari_t *ari2, int parms);
ari_t     ari_copy(ari_t val, int *success);
ari_t*    ari_copy_ptr(ari_t *ari);
ari_t*    ari_create(amp_type_e type);
ari_t     ari_deserialize(QCBORDecodeContext *it, int *success);
ari_t*    ari_deserialize_ptr(QCBORDecodeContext *it, int *success);
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
int       ari_serialize(QCBOREncodeContext *encoder, void *item);
blob_t*   ari_serialize_wrapper(ari_t *ari);


/* ARI Collection Functions */
int       ac_append(ac_t *dest, ac_t *src);
void      ac_clear(ac_t *ac);
ac_t*     ac_create();
//int       ac_compare(ac_t *a1, ac_t *a2);
ac_t      ac_copy(ac_t *src);
ac_t*     ac_copy_ptr(ac_t *src);
ac_t      ac_deserialize(QCBORDecodeContext *it, int *success);
ac_t*     ac_deserialize_ptr(QCBORDecodeContext *it, int *success);
ac_t      ac_deserialize_raw(blob_t *data, int *success);
ari_t*    ac_get(ac_t* ac, uint8_t index);
uint8_t   ac_get_count(ac_t* ac);
int       ac_init(ac_t *ac);
int       ac_insert(ac_t* ac, ari_t *ari);
void      ac_release(ac_t *ac, int destroy);
int        ac_serialize(QCBOREncodeContext *encoder, void *item);
blob_t*   ac_serialize_wrapper(ac_t *ac);

#endif
