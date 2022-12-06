/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2020 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: radix.h
 **
 ** Subsystem:
 **          Utilities
 **
 ** Description:
 **     This file contains a custom implementation of a Radix Tree. This tree
 **     stores strings in a space-optimized tree structure. This implementation
 **     is augmented to include a visitor pattern and user-defined 
 **     key-associated data.
 **     
 **     This tree structure supports very fast search times, but additions
 **     and deletions may be very expensive because they (1) lock the structure
 **     and (2) may cause cascading rebalancing.   
 **     
 **     User-defined callbacks provide the ability to manipulate user data
 **     stored in the tree.
 **     
 **     The implementation supports re-entrancy via blocking writes. 
 **
 ** Notes:
 **     This implementation *only* supports wildcard prefix searches (data*)
 **
 **      Locking is a balance between performance and precision. Locking
 **      too specific an area leads to too much locking and unlocking. Locking
 **      too large an area reduces concurrent execution of otherwise paralellizable
 **      functions. Radix balances this by locking based on user-functions and
 **      not locking within private functions.
 **
 **
 ** Assumptions:
 **     Keys are strings
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/25/20  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef RADIX_H_
#define RADIX_H_

#include "stdint.h"
#include "ion.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/**> Wildcard for prefix matching. */
#define RADIX_PREFIX_WILDCARD '*'

#define RADIX_MATCH_NONE     (1)
#define RADIX_MATCH_PARTIAL  (2)
#define RADIX_MATCH_FULL     (3)
#define RADIX_MATCH_SUBSET   (4)
#define RADIX_MATCH_WILDCARD (5)

#define RADIX_FULL_MATCH 1
#define RADIX_PARTIAL_MATCH 2

#define RADIX_STOP_FOREACH 2
/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

typedef struct RadixNode_s RadixNode;
typedef struct RadixStats_s RadixStats;
typedef struct RadixTree_s RadixTree;


/* User-definable delete function. */
typedef void (*radix_del_fn)(PsmPartition partition, PsmAddress user_data);

/* User-definable callback to run on every node in the tree. */
typedef int (*radix_foreach_fn)(PsmAddress user_data, void *tag);

/* User-definable callback to run on every node matching a key. */
typedef int (*radix_match_fn)(PsmPartition partition, PsmAddress user_data, void *tag);

/* User-definable callback to insert data into the radix tree. */
typedef int (*radix_insert_fn)(PsmPartition partition, PsmAddress *node_data, PsmAddress new_item);

/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

//radix_insert_fn ins, radix_del_fn del,
PsmAddress radix_create(PsmPartition partition);

void radix_destroy(PsmPartition partition, PsmAddress radixAddr, radix_del_fn del_fn);

PsmAddress radix_find(PsmPartition partition, PsmAddress radixAddr, char *key, int wildcard);

void  radix_foreach(PsmPartition partition, PsmAddress radixAddr, radix_foreach_fn foreach_fn, void *tag);

void radix_foreach_match(PsmPartition partition, PsmAddress radixAddr, char *key, int flags, radix_match_fn match_fn, void *tag);

int   radix_insert(PsmPartition partition, PsmAddress radixAddr, char *key, PsmAddress data, radix_insert_fn ins_fn, radix_del_fn del_fn);

int  radix_prettyprint(PsmPartition partition, PsmAddress tree, char *buffer, int buf_size);


#endif
