/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: rhht.h
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description:
 **     This file contains the functions for a Robin Hood Hash Table.
 **     It uses a "backwards-shift" removal system, which reduces the
 **     max number of probes per find and eliminates the need for
 **     tombstones. The low expected number of removals relative to
 **     the number of insertions & finds makes this a useful tradeoff.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/17/18  L. F. Campbell Initial Implementation (JHU/APL)
 **  09/14/18  E. Birrane     Updates for data structures, logging, reentrance,
 **                           coding standards and documentation.
 *****************************************************************************/

#ifndef RHHT_H_
#define RHHT_H_

#include "stdint.h"
#include "ion.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */
#define RH_ERROR      0
#define RH_OK         1
#define RH_SYSERR    -1
#define RH_DUPLICATE -2
#define RH_NOT_FOUND -3
#define RH_FULL      -4

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

#define RHP_SIZE(ht) (ht->num_elts)
#define RHP_LOAD(ht) (((float) ht->num_elts) / ((float) ht->num_bkts))

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */
typedef uint16_t rh_idx_t;



/**
 * A hash table is a collection of hash table entries.
 */
typedef struct  {
    void *value;
    void *key;
    rh_idx_t delta;
} rh_elt_t;


typedef void (*rh_del_fn)(rh_elt_t *elt); /* Item Delete Function. */
typedef int (*rh_comp_fn)(void *key1, void *key2); /* Semantic compare elements */
typedef rh_idx_t (*rh_hash_fn)(void *table, void *key); /* Hashing function */
typedef void (*rh_foreach_fn)(rh_elt_t *elt, void *tag); /* Foreach callback. */


/**
 * Meta-data for the hash table and a pointer to its first entry.
 */
typedef struct rhht {
	rh_elt_t   *buckets;

	rh_idx_t   num_bkts;
    rh_idx_t   num_elts;
    rh_idx_t   max_delta;

    ResourceLock lock;

    rh_comp_fn compare;
    rh_hash_fn hash;
    rh_del_fn  delete;
} rhht_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


rhht_t  rhht_create(rh_idx_t buckets, rh_comp_fn compare, rh_hash_fn hash, rh_del_fn del, int *success);
void    rhht_del_idx(rhht_t *ht, rh_idx_t idx);
void    rhht_del_key(rhht_t *ht, void *key);
int     rhht_find(rhht_t *ht, void *key, rh_idx_t *idx);
void    rhht_foreach(rhht_t *ht, rh_foreach_fn, void *tag);
int     rhht_insert(rhht_t *ht, void *key, void *value, rh_idx_t *idx);
void    rhht_release(rhht_t *ht, int destroy);
void*   rhht_retrieve_idx(rhht_t *ht, rh_idx_t idx);
void*   rhht_retrieve_key(rhht_t *ht, void *key);

#endif /* RHHT_H_ */
