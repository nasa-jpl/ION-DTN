/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2020 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: radixP.h
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
 ** Assumptions:
 **     Keys are strings
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/25/20  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef RADIX_P_H_
#define RADIX_P_H_

#include "stdint.h"
#include "ion.h"
#include "radix.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/**> The default number of child nodes for a given radix node. */
#define RADIX_DEFAULT_CHILDREN (10)

/**> The number of child nodes to add when growing a node's children. */
#define RADIX_CHILD_INCR (5)

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */

#define radix_lock(tree)   if(sm_SemTake(tree->lock) < 0) putErrmsg("Can't lock radix.", NULL);
#define radix_unlock(tree) sm_SemGive(tree->lock)

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * RadixNode
 * 
 * Each node in a radix tree stores information relating to the structure
 * of the tree, its contribution to the held keys, and any user information
 * associated with keys for which the node acts as a leaf. 
 * 
 * The structural information includes references to its parent and zero or more
 * children. 
 * 
 * Key information is stored as a fixed-length string representing a subset of 
 * a larger string.
 * 
 * User information is treated as a vector of information which may be added
 * to over time. 
 */

struct RadixNode_s
{ 
	/* Tree Structure Information */
	PsmAddress parent;     /**> Parent node (RadixNode*) */

    PsmAddress children;   /**> Children node (PsmAddress *) */

	uint8_t order;         /**> The (0-based) order amongst siblings. */
	uint8_t depth;         /**> The 0-based depth of this node. */
	uint8_t num_kids;      /**> Total number of allocated child nodes. */

	/* Key Information */
	PsmAddress key;        /**> Key Information. (char *) */

	/* User Information */
	PsmAddress user_data;  /**> sm_list of user information. */
}; 


/**
 * RadixStats
 * 
 * Simple statistics for measuring the performance of the tree. 
 */
struct RadixStats_s
{
	int nodes;     /**> Number of nodes in the tree. */
};


/**
 * RadixTree
 * 
 * The radix tree contains the tree structure, statistics, protection
 * mechanisms, and information on how to handle individual user data. 
 * 
 */
struct RadixTree_s
{
	/* Tree structure */
	PsmAddress root;   /**> The root node of the tree (RadixNode *). */
	
	/* Statistics */
	struct RadixStats_s stats;  /**> Statistics related to the tree. */
	
	/* Protection */
    sm_SemId lock;          /**> Mutex - locks on write. */
    
    /* User Data Handling */
    //radix_del_fn     del_fn;    /**> Optional user-data delete function. */
    //radix_insert_fn  ins_fn;    /**> Optional user-defined insert function */
};


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


PsmAddress radixP_create_node(PsmPartition partition, char *key, int keyLen, PsmAddress data);

int radixP_del_leafnode(PsmPartition partition, PsmAddress nodeAddr, RadixTree *tree, radix_del_fn del_fn);

void radixP_fix_depth(PsmPartition partition, RadixNode *nodePtr);

PsmAddress radixP_get_child(PsmPartition partition, RadixNode *node, int order);

PsmAddress radixP_get_next_node(PsmPartition partition, PsmAddress nodeAddr, PsmAddress limitAddr, int *delta);

PsmAddress radixP_get_sibling_node(PsmPartition partition, RadixNode *node);

PsmAddress radixP_get_term_node(PsmPartition partition, RadixTree *radixPtr, char *key, int *type, int *key_idx, int *node_idx);

int radixP_grow_children(PsmPartition partition, RadixNode *nodePtr);

int radixP_insert_node(PsmPartition partition, RadixTree *tree, PsmAddress parentAddr, PsmAddress nodeAddr, int order);

int        radixP_node_is_leaf(PsmPartition partition, RadixNode *node);

int        radixP_node_matches_key(char *node_key, char *key, int *idx, int wildcard);

PsmAddress radixP_remove_child(PsmPartition partition, RadixNode *node, int order);

int radixP_split_insert_node(PsmPartition partition, RadixTree *radixPtr, PsmAddress splitNodeAddr, PsmAddress newChildAddr, int offset, radix_del_fn del_fn);

#endif
