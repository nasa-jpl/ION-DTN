/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2020 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: radix.c
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
 **     This implementation *only* supports wildcard prefix searches (data~)
 **
 **     This tree can support either a fixed or variable number of child nodes.
 **
 **     If child nodes can be fixed, less memory is used and searches are faster.
 **
 **     If child nodes need to be variable, searches take about 2x as long and
 **     about 4x as much memory is used.
 **
 **		TODO: Add function to compact tree.
 **		TODO: Add function to split large key into multiple nodes.
 **		TODO: Add function to delete nodes given a key.
 **		TODO: Rebalance if one node has too many children.
 **		TODO: Document naming convention
 **		TODO: Check unlock/returns
 **		TODO: Add unwedge
 **		TODO: Consider sorting the child node arrays and short-circuiting the
 **		      call to get_sibling if you are out of sort order.
 **
 ** Assumptions:
 **     Keys are strings
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  11/25/20  E. Birrane     Initial Implementation
 **  01/05/21  E. Birrane     Migrate to using shared memory
 *****************************************************************************/

#include <string.h>

#include "platform.h"
#include "platform_sm.h"
#include "psm.h"

#include "radixP.h"

PsmAddress radix_alloc(PsmPartition partition, int size)
{
	PsmAddress addr = psm_zalloc(partition, size);
	void *ptr = psp(partition, addr);
	CHKZERO(ptr);
	memset(ptr,0,size);
	return addr;
}

/******************************************************************************
 * @brief Initializes an instance of a radix tree in shared memory.
 *
 * @param[in]  ins       - User function to insert user data
 * @param[in]  del       - User function to free user data
 * @param[out] partition - The shared memory partition
 *
 * @retval  0 - Failure
 * @retval  !0 - The shared memory address of the created radix tree
 *****************************************************************************/
PsmAddress radix_create(PsmPartition partition)
{
	sm_SemId	lock;
	PsmAddress	radixAddr;
	RadixTree  *radixPtr;

	/* Step 1: Start by creating a shared-memory lock for the radix. */
	if((lock = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO)) < 0)
	{
		putErrmsg("Can't create semaphore for radix tree.", NULL);
		return 0;
	}

	/* Step 2: Allocate and zero the share dmemory for the tree. */
	if((radixAddr = radix_alloc(partition, sizeof(RadixTree))) == 0)
	{
		sm_SemDelete(lock);
		putErrmsg("Can't allocate space for rbt object.", NULL);
		return 0;
	}

	/* Step 3: Initialize the tree in shared memory. */
	radixPtr = (RadixTree *) psp(partition, radixAddr);
	radixPtr->lock = lock;
//	radixPtr->del_fn = del;
//	radixPtr->ins_fn = ins;

	/* Step 4: Return the shared memory address of the tree. */
	return radixAddr;
}



/******************************************************************************
 * @brief Releases shared memory associated with a radix tree.
 *
 * @param[in|out] partition - The shared memory partition holding the tree.
 * @param[in]     radixAddr - The shared memory address of the tree.
 * @param[in]     del_fn    - user-defined delete function.
 *
 * @note
 * The caller MUST NOT attempt to use the tree after this function is called.
 *****************************************************************************/

void radix_destroy(PsmPartition partition, PsmAddress radixAddr, radix_del_fn del_fn)
{
	RadixTree *radixPtr = NULL;
	PsmAddress nodeAddr = 0;
	RadixNode *nodePtr = NULL;
	PsmAddress parentAddr = 0;

	/* Step 1 - If the tree doesn't exist, there is nothing to free. */
	CHKVOID(partition);
	CHKVOID(radixAddr);

	if((radixPtr = (RadixTree *) psp(partition, radixAddr)) == NULL)
	{
		return;
	}

	/*
	 * Step 2 - Lock the tree. It would be unfortunate if someone were
	 *          using the tree while we were destroying it.
	 */
	radix_lock(radixPtr);

	/*
	 * Step 3 - Walk the tree by drilling down to leaf nodes and deleting
	 *          them.
	 */
	nodeAddr = radixPtr->root;
	while(nodeAddr != 0)
	{
		/*
		 * Step 3.1 - If this is a leaf node, we can easily remove it because
		 *            it has no children. Keep doing this until the parent
		 *            to the leaf node is also a child, etc... eventually
		 *            the root itself will be a leaf node.
		 */
		nodePtr = psp(partition, nodeAddr);

		if(radixP_node_is_leaf(partition, nodePtr))
		{
			parentAddr = nodePtr->parent;
			radixP_del_leafnode(partition, nodeAddr, radixPtr, del_fn);
			nodeAddr = parentAddr;
		}
		/* Step 3.2 - If this is not a leaf node, push into a child. */
		else
		{
			nodeAddr = radixP_get_child(partition, nodePtr, 0);
		}
	}

	/* Step 4 - Unlock the (now emptied) tree and remove the lock. */
	radix_unlock(radixPtr);

	sm_SemEnd(radixPtr->lock);
	microsnooze(50000);
	sm_SemDelete(radixPtr->lock);

	memset(radixPtr, 0, sizeof(RadixTree));
	psm_free(partition, radixAddr);

}



/******************************************************************************
 * @brief Return the shared memory address of the user data matching the key.
 *
 * @param[in|out] partition - The shared memory partition holding the tree.
 * @param[in]     radixAddr - The shared memory address of the tree.
 * @param[in]     key       - The key whose exact match is being searched for
 *
 * @retval  0 - Item not found
 * @retval !0 - Address of the list holding exact matches for this key.
 *****************************************************************************/

PsmAddress radix_find(PsmPartition partition, PsmAddress radixAddr, char *key, int wildcard)
{
	RadixTree *radixPtr = NULL;
	char *nodeKeyPtr = NULL;
	int offset = 0;
	int delta = 0;
	int len = 0;
	int idx = 0;
	int match = 0;
	PsmAddress nodeAddr = 0;
	PsmAddress tmpAddr = 0;

	RadixNode *nodePtr = NULL;

	PsmAddress user_data = 0;

	/* Step 1: Sanity Checks. */
	CHKZERO(partition);
	CHKZERO(radixAddr);
	CHKZERO(key);

	radixPtr = (RadixTree*) psp(partition, radixAddr);
	CHKZERO(radixPtr);

	/*
	 * Step 2: Remember the key length. As we iterate a radix tree we will
	 *         be comparing substrings.
	 */
	len = strlen(key);

	/*
	 * Step 3: Lock the tree. While this iteration is read-only, changes
	 *         may cause nodes to be skipped.
	 */
	radix_lock(radixPtr);

	/* Step 4: Start the pre-order traversal at the root. */
	nodeAddr = radixPtr->root;

	while(nodeAddr != 0)
	{
		nodePtr = (RadixNode *) psp(partition, nodeAddr);
		nodeKeyPtr = (char *) psp(partition, nodePtr->key);

		/* Step 4.1: Assess how the current node matches the appropriate
		 *           key subset. key[offset] is the start of the key subset
		 *           being compared to the current node. The returned idx
		 *           is the end of the match within the key. On a full
		 *           or partial match, "node" matches the substring of
		 *           key[offset] to key[idx].
		 */
		match = radixP_node_matches_key(nodeKeyPtr, &(key[offset]), &idx, wildcard);

		switch(match)
		{
			/*
			 * Step 4.1.1: If we full match, store user data for this node and
			 *             then note that we are finished traversing. Setting
			 *             node to NULl will break us out of the while loop.
			 */
			case RADIX_MATCH_FULL:
				user_data = nodePtr->user_data;
				nodeAddr = 0;
			    break;

			/* Step 4.1.2: If we partial match... */
			case RADIX_MATCH_PARTIAL:
				/*
				 * Step 4.1.2.2: Find the next node to visit. Since this is a
				 *               partial match, we need to check child nodes.
				 *               There are 2 reasons to "skip" a child node:
				 *               1 - No child node exists (node is a leaf)
				 *               2 - The child node exceeds the "depth" of the
				 *                   key, meaning that the key is not long
				 *                   enough to possibly match the child and,
				 *                   therefore, any nodes under the child.
				 *               If we skip the child node, we can find a
				 *               sibling or above node to visit instead.
				 */
				tmpAddr = radixP_get_child(partition, nodePtr, 0);
				if((tmpAddr == 0) ||
				   (offset + strlen(nodeKeyPtr) >= len))
				{
					/* Step 4.1.2.2.1: Get next node and adjust key offset. */
					nodeAddr = radixP_get_next_node(partition, nodeAddr, 0, &delta);
					offset -= delta;
				}
				else
				{
					/*
					 * Step 4.1.2.2.2: Advance key offset by current node's
					 *                 length to pass the correct substring
					 *                 to the child on the next loop iteration.
					 */
					offset += strlen(nodeKeyPtr);
					nodeAddr = tmpAddr;
				}
				break;

			/* Step 4.1.3: If we have no useful match...*/
			case RADIX_MATCH_SUBSET:
			case RADIX_MATCH_NONE:
				/*
				 * Step 4.1.3.1: If we don't match then no child node will
				 *               either.  A subset match is not considered
				 *               a partial match.
				 */
				nodeAddr = radixP_get_next_node(partition, nodeAddr, 0, &delta);
				offset -= delta;
				break;

			default:
				break;
		}
	}

	/* Step 5 - Unlock the tree. */
	radix_unlock(radixPtr);
	return user_data;
}


/******************************************************************************
 * @brief Calls a user-defined function for each item in the tree based on a
 *        pre-ordered traversal.
 *
 * @param[in]     partition  - The shared memory partition
 * @param[in]     radixAddr  - The tree being iterated
 * @param[in]     foreach_fn - The user function to call on each item.
 * @param[in,out] tag        - User-supplied data for the function.
 *
 * @note
 * Subsequent calls to this function might NOT provide the same ordering if
 * the tree has had nodes added to it since the last call.
 *****************************************************************************/

void  radix_foreach(PsmPartition partition, PsmAddress radixAddr, radix_foreach_fn foreach_fn, void *tag)
{
	RadixTree *radixPtr = NULL;
	RadixNode *nodePtr = NULL;
	PsmAddress nodeAddr = 0;
	PsmAddress tmpAddr = 0;

	/* Step 1 - Sanity checks. */
	CHKVOID(partition);
	CHKVOID(radixAddr);
	CHKVOID(foreach_fn);

	radixPtr = (RadixTree*)psp(partition, radixAddr);
	CHKVOID(radixPtr);

	/*
	 * Step 2 - While the iteration does not change the tree, the tree
	 *          must not be altered mid-iteration otherwise nodes
	 *          might be missed by this foreach function.
	 */
	radix_lock(radixPtr);

	/* Step 3 - Perform a pre-order traversal starting at the root. */
	nodeAddr = radixPtr->root;
	while(nodeAddr != 0)
	{
		nodePtr = (RadixNode*) psp(partition, radixPtr->root);

		/*
		 * Step 3.1 - When visiting node, we don't assess the success of the
		 *            user function, nor do we keep a return value. */
		if(foreach_fn(nodePtr->user_data, tag) == RADIX_STOP_FOREACH)
		{
			break;
		}

		/*
		 * Step 3.2 - The pre-order traversal selects the order 0 child. If
		 *            there is none, we are a leaf node and can find a sibling
		 *            or above to visit next. If we do have a child, we select
		 *            the "leftmost" child and visit it. Eventually the leftmost
		 *            child will be a leaf node.
		 */
		tmpAddr = radixP_get_child(partition, nodePtr, 0);

		if(tmpAddr == 0)
		{
			nodeAddr = radixP_get_next_node(partition, nodeAddr, 0, NULL);
		}
		else
		{
			nodeAddr = tmpAddr;
		}
	}

	/* Step 4 - Unlock the tree. */
	radix_unlock(radixPtr);
}



/******************************************************************************
 * @brief Calls a user-defined function for each item in the tree that matches
 *        a given key.
 *
 * @param[in]     partition  - The shared memory partition
 * @param[in]     radixAddr  - The tree being iterated
 * @param[in]     key        - The key being matched to items in the tree
 * @param[in]     flags      - The types of matches the user is interested in
 * @param[in]     match_fn   - The user function to call on each matched item.
 * @param[in,out] tag        - User-supplied data for the match function.
 *
 * @note
 * Subsequent calls to this function might NOT provide the same ordering if
 * the tree has had nodes added to it since the last call.
 * \pr
 * The flags field is a bitmask supporting 2 flags: RADIX_FULL_MATCH and
 * RADIX_PARTIAL_MATCH. Either or both may be set in the flags field.
 * \par
 * RADIX_FULL_MATCH causes the match function to be called whenever an
 * item in the tree represents a "complete" match to the key, including
 * wildcards. A full match in this case means an item which matches such
 * that child nodes of that item need to be searched for further matches.
 * \par
 * RADIX_PARTIAL_MATCH causes the match function to be called whenever an
 * item in the tree represents a "partial" but not a full match. This means
 * any node which is an ancestor of the eventual full-match item.
 *****************************************************************************/

void radix_foreach_match(PsmPartition partition, PsmAddress radixAddr, char *key, int flags, radix_match_fn match_fn, void *tag)
{
	int offset = 0;
	int delta = 0;
	int len = 0;
	int idx;

	RadixTree *radixPtr = NULL;
	RadixNode *nodePtr = NULL;
	PsmAddress nodeAddr = 0;
	PsmAddress tmpAddr = 0;
	char *nodeKeyPtr = NULL;

	/* Step 1: Sanity Checks. */
	CHKVOID(partition);
	CHKVOID(radixAddr);
	CHKVOID(match_fn);

	radixPtr = (RadixTree*) psp(partition, radixAddr);
	CHKVOID(radixPtr);

	/* Giving a NULL key isn't treated as an error. */
	if(key == NULL)
	{
		return;
	}

	/*
	 * Step 2: Remember the key length. As we iterate a radix tree we will
	 *         be comparing substrings.
	 */
	len = strlen(key);

	/*
	 * Step 3: Lock the tree. While this itertion is read-only, changes
	 *         may cause nodes to be skipped.
	 */
	radix_lock(radixPtr);

	/* Step 4: Start the pre-order traversal at the root. */
	nodeAddr = radixPtr->root;
	while(nodeAddr != 0)
	{
		nodePtr = (RadixNode*) psp(partition, nodeAddr);
		nodeKeyPtr = (char *) psp(partition, nodePtr->key);


		/* Step 4.1: Assess how the current node matches the appropriate
		 *           key subset. key[offset] is the start of the key subset
		 *           being compared to the current node. The returned idx
		 *           is the end of the match within the key. On a full
		 *           or partial match, "node" matches the substring of
		 *           key[offset] to key[idx].
		 */
		switch(radixP_node_matches_key(nodeKeyPtr, &(key[offset]), &idx, 1))
		{
			/* Step 4.1.1 - If this is a full match...*/
			case RADIX_MATCH_WILDCARD:
			case RADIX_MATCH_FULL:
				/*
				 * Step 4.1.1.1: Visit if full matches are requested and there
				 *               exists user data at he node. A node may exist
				 *               without user data when the node is created by
				 *               the radix tree itself as part of a split on
				 *               insert.
				 */
				if((flags & RADIX_MATCH_FULL) && (nodePtr->user_data != 0))
				{
				   if(match_fn(partition, nodePtr->user_data, tag) == RADIX_STOP_FOREACH)
				   {
					   radix_unlock(radixPtr);
					   return;
				   }
				}
				/* Step 4.1.1.2: On a full match (including wildcards) we stop
				 *               descending. No child nodes will offer anything
				 *               beyond this full match. It is the terminal node.
				 */
				nodeAddr = radixP_get_next_node(partition, nodeAddr, 0, &delta);

				/*
				 * Step 4.1.1.3: Since we are moving "up" in the tree, the
				 *               key substring changes as well. Delta
				 *               captures the substring difference between
				 *               the current node and the new sibling-or-
				 *               above next node to visit.
				 */
				offset -= delta;
				break;

			/* Step 4.1.2: If we partial match... */
			case RADIX_MATCH_PARTIAL:

				/*
				 * Step 4.1.2.1: Visit if partial matches are requested and
				 *               there exists user data at the node. A node
				 *               may exist without user data when the node
				 *               is created by the radix tree itself as part
				 *               of a split on insert.
				 */
				if(flags & RADIX_MATCH_PARTIAL)
				{
				  if(match_fn(partition, nodePtr->user_data, tag) == RADIX_STOP_FOREACH)
				  {
					  radix_unlock(radixPtr);
					  return;
				  }
				}

				/*
				 * Step 4.1.2.2: Find the next node to visit. Since this is a
				 *               partial match, we need to check child nodes.
				 *               There are 2 reasons to "skip" a child node:
				 *               1 - No child node exists (node is a leaf)
				 *               2 - The child node exceeds the "depth" of the
				 *                   key, meaning that the key is not long
				 *                   enough to possibly match the child and,
				 *                   therefore, any nodes under the child.
				 *               If we skip the child node, we can find a
				 *               sinbling or above node to visit instead.
				 */
				tmpAddr = radixP_get_child(partition, nodePtr, 0);
				if((tmpAddr == 0) ||
				   (offset + strlen(nodeKeyPtr) >= len))
				{
					/* Step 4.1.2.2.1: Get next node and adjust key offset. */
					nodeAddr = radixP_get_next_node(partition, nodeAddr, 0, &delta);
					offset -= delta;
				}
				else
				{
					/*
					 * Step 4.1.2.2.2: Advance key offset by current node's
					 *                 length to pass the correct substring
					 *                 to the child on the next loop iteration.
					 */
					offset += strlen(nodeKeyPtr);
					nodeAddr = tmpAddr;
				}
				break;

			/* Step 4.1.3: If we have no useful match...*/
			case RADIX_MATCH_SUBSET:
			case RADIX_MATCH_NONE:
				/*
				 * Step 4.1.3.1: If we don't match then no child node will
				 *               either.  A subset match is not considered
				 *               a partial match.
				 */
				nodeAddr = radixP_get_next_node(partition, nodeAddr, 0, &delta);
				offset -= delta;
				break;

			default:
				break;
		}
	}

	/* Step 5 - Unlock the tree. */
	radix_unlock(radixPtr);
}



/******************************************************************************
 * @brief Inserts a piece of user data into the tree at a unqie key.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[out]    radixAddr - The tree being updated
 * @param[in]     key       - The key being matched to items in the tree
 * @param[in]     data      - The types of matches the user is interested in
 * @param[in]     ins_fn    - User-specified insert function.
 * @param[in]     del_fn    - User-specified delete function.
 *
 * @note
 * A user may not insert an item with NO user data.
 *
 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int   radix_insert(PsmPartition partition, PsmAddress radixAddr, char *key, PsmAddress data, radix_insert_fn ins_fn, radix_del_fn del_fn)
{
	RadixTree *radixPtr = NULL;

	RadixNode *termPtr = NULL;
	PsmAddress termAddr = 0;

	RadixNode *nodePtr = NULL;
	PsmAddress nodeAddr = 0;

	int type = 0;
	int key_idx;
	int node_idx;
	int result = 0;

	/* Step 1: Sanity Checks. */
	CHKERR(partition);
	CHKERR(radixAddr);
	CHKERR(key);
	CHKERR(data);

	radixPtr = (RadixTree*) psp(partition, radixAddr);
	CHKERR(radixPtr);

	/* Step 2: Lock access to the tree. */
	radix_lock(radixPtr);

	/*
	 * Step 3: Find the terminal node for this key. This is the node that
	 *         is either a FULL, PARTIAL, or SUBSET match for the given
	 *         key.
	 */
	termAddr = radixP_get_term_node(partition, radixPtr, key, &type, &key_idx, &node_idx);

	/* Step 4: Make sure we found the node without issue. */
	if((type == -1) || (key_idx > strlen(key)) || (node_idx > strlen(key)))
	{
		radix_unlock(radixPtr);
		return -1;
	}

	termPtr = (RadixNode *) psp(partition, termAddr);

	/* Step 5: insert the new node based on the key match type. */
	switch(type)
	{

		/* Step 5.1: If the terminal node is an exact partial match it
		 *           means the children of the terminal node have no
		 *           partial match and the new node must be a child of
		 *           this node.
		 *
		 *           If the terminal node is no match, it means we have
		 *           a child under the root level or no root. If we
		 *           have no root this is the first insert so make
		 *           the new node the root. Otherwise we presume the
		 *           returned terminal node is the root node and
		 *           the new node is a direct child under it.
		 *
		 */
		case RADIX_MATCH_PARTIAL:
		case RADIX_MATCH_NONE:
			if(ins_fn != NULL)
			{
				PsmAddress insert_data = 0;
				if (ins_fn(partition, &insert_data, data) == 0)
				{
					radix_unlock(radixPtr);
					return 0;
				}
				data = insert_data;
			}

			/* Step 5.1.1: Create the new node.*/

			nodeAddr = radixP_create_node(partition, &(key[key_idx]), strlen(&(key[key_idx])), data);
			if(nodeAddr == 0)
			{
				radix_unlock(radixPtr);
				return 0;
			}
			nodePtr = (RadixNode *) psp(partition, nodeAddr);

			/* Step 5.1.2: Special case of empty tree: insert root.*/
			if(radixPtr->root == 0)
			{
				nodePtr->order = 0;
				nodePtr->depth = 0;
				radixPtr->root = nodeAddr;
				radixPtr->stats.nodes++;
				result = 1;
			}
			/* Step 5.1.3: Insert as a child node to the terminal node. */
			else
			{
				result = radixP_insert_node(partition, radixPtr, termAddr, nodeAddr, -1);
			}
			break;

		/* Step 5.2: If the terminal node is a full match, then this node
		 *           either was inserted by a user already and is a duplicate
		 *           or was created by an internal split and can accept
		 *           user data.
		 */
		case RADIX_MATCH_FULL:
			if(ins_fn != NULL)
			{
				PsmAddress insert_data = termPtr->user_data;
				if (ins_fn(partition, &insert_data, data) == 0)
				{
					radix_unlock(radixPtr);
					return 0;
				}
				termPtr->user_data = insert_data;
				result = 1;
			}
			else
			{
				/* Step 5.2.1: If this node was created by an internal split. */
				if(termPtr->user_data == 0)
				{
					/* Step 5.2.1.1: "insert" by assigning data. */
					termPtr->user_data = data;
					result = 1;
				}
				/* Step 5.2.2: Otherwise, this is a duplicate. No Insert. */
				else
				{
					result = 0;
				}
			}

			break;

		/*
		 * Step 5.3: If we match only some of the terminal node key, then
		 *           we need to split the terminal node.
		 *
		 *           A subset match means part of the terminal node's key
		 *           matched and we need to split the terminal node to
		 *           make a common node at the point of the subset match.
		 *
		 *           A wildcard match means the key only matched because
		 *           of the wildcard and we need to split the terminal node
		 *           to make a common node at the point of the wildcard
		 *           character.
		 */
		case RADIX_MATCH_SUBSET:
		case RADIX_MATCH_WILDCARD:
			if(ins_fn != NULL)
			{
				PsmAddress insert_data = 0;
				if (ins_fn(partition, &insert_data, data) == 0)
				{
					radix_unlock(radixPtr);
					return 0;
				}
				data = insert_data;
			}

			/* Step 5.3.1: Make the new node with the subset key. */
			nodeAddr = radixP_create_node(partition, &(key[key_idx]), strlen(&(key[key_idx])), data);

			/* Step 5.3.2: Split insert noting the split-point of the key. */
			result = radixP_split_insert_node(partition, radixPtr, termAddr, nodeAddr, node_idx, del_fn);

			break;
	}

	/* Step 6: Unlock and return. */
	radix_unlock(radixPtr);

	return result;
}



/******************************************************************************
 * @brief Generates a string representation of the tree.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     radixAddr - The tree being converted to a string.
 * @param[out]    buffer    - The user-provided buffer.
 * @param[in]     buf_len   - The size of the user-provided buffer.
 *
 * @note
 *
 * @retval The number of bytes written to the user buffer.
 *****************************************************************************/

int radix_prettyprint(PsmPartition partition, PsmAddress radixAddr, char *buffer, int buf_size)
{
	int offset = 0;
	int delta = 0;
	char *cursor = buffer;
	PsmAddress curNodeAddr = 0;
	RadixNode *curNodePtr = NULL;

	char *curNodeKeyPtr = NULL;

	PsmAddress childAddr = 0;
	RadixTree *radixPtr = NULL;

	/* Step 1: Sanity check. */
	CHKZERO(radixAddr);
	CHKZERO(buffer);
	CHKZERO(buf_size > 0);

	radixPtr = (RadixTree*) psp(partition, radixAddr);

	/* Step 2: Lock the tree... */
	radix_lock(radixPtr);

	/*
	 * Step 3: Start a pre-order traversal. We iterate until we
	 *          run out of tree or we run out of string.
	 */
	curNodeAddr = radixPtr->root;
	while((curNodeAddr != 0) && (offset < buf_size))
	{
		curNodePtr = (RadixNode*) psp(partition, curNodeAddr);
		curNodeKeyPtr = (char *) psp(partition, curNodePtr->key);

		/* Step 3.1: Set the cursor into the string buffer. */
		cursor = &(buffer[offset]);

		/* Step 3.2: Calculate how much we want to add. */
		delta = 3 + curNodePtr->depth + strlen(curNodeKeyPtr)+50;

		/* Step 3.3: Trim if we would overwrite user buffer. */
		if((offset + delta) > buf_size)
		{
			delta = buf_size - offset;
		}

		/* Step 3.4: Write node to cursor, noting written length. */
		offset += snprintf(cursor, delta, "%*s[%s]("ADDR_FIELDSPEC_INT"->"ADDR_FIELDSPEC_INT") ["ADDR_FIELDSPEC_INT"]\n", curNodePtr->depth, "", curNodeKeyPtr, curNodePtr->parent, curNodeAddr, curNodePtr->user_data);

		/* Step 3.5: Get the next node in the pre-order traversal. */
		childAddr = radixP_get_child(partition, curNodePtr, 0);
		if(childAddr == 0)
		{
			curNodeAddr = radixP_get_next_node(partition, curNodeAddr, 0, NULL);
		}
		else
		{
			curNodeAddr = childAddr;
		}
	}

	/* Step 4: Unlock and return. */
	radix_unlock(radixPtr);
	return offset;
}



/******************************************************************************
 * @brief Allocate a new node for the tree.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in] key           - The key being added
 * @param[in] keyLen        - Length of the key
 * @param[in] data          - The user data associated with the key
 *
 * @note
 * This function does not add a node to the list, it just makes a node structure.
 * \par
 * While a user may not enter NULL data, the data field here CAN be NULL, as
 * this function is also used internally when splitting a node, in which case
 * the internally-sourced node will not have user data associated with it.
 * \par
 * This function does not set node order, node depth, or parent because it does
 * not exist in the tree yet.
 *
 * @retval  !NULL - The created node
 * @retval  NULL  - Failure
 *****************************************************************************/

PsmAddress radixP_create_node(PsmPartition partition, char *key, int keyLen, PsmAddress data)
{
	PsmAddress nodeAddr = 0;
	RadixNode *nodePtr = NULL;
	char *keyPtr = NULL;

	/* Step 1: Sanity Checks. */
	CHKZERO(partition);

	/* Step 2: Allocate space for the new node. */
	nodeAddr = radix_alloc(partition, sizeof(RadixNode));
	CHKZERO(nodeAddr);
	nodePtr = (RadixNode*) psp(partition, nodeAddr);

	/* Step 3: Populate other node items and return. */
	if((key != NULL) && (keyLen > 0))
	{
	  nodePtr->key = radix_alloc(partition, keyLen+1);
	  CHKZERO(nodePtr->key);
	  keyPtr = (char *) psp(partition, nodePtr->key);
	  memcpy(keyPtr, key, keyLen);
	}
	else
	{
		psm_free(partition, nodeAddr);
		return 0;
	}

	nodePtr->user_data = data;

	/* Step 4: Allocate some space for child nodes. */
	radixP_grow_children(partition, nodePtr);

	return nodeAddr;
}



/******************************************************************************
 * @brief Remove a leaf node from the tree.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in]     nodeAddr  - Shared memory address of the node being deleted.
 * @param[in,out] tree      - Radix tree pointer.
 * @param[in]     del_fn    - User-supplied delete function.
 *
 * @note
 * Failure of the user data delete function is not considered here.
 *
 * @retval  0 - Error
 * @retval  1 - Success
 *****************************************************************************/

int radixP_del_leafnode(PsmPartition partition, PsmAddress nodeAddr, RadixTree *tree, radix_del_fn del_fn)
{
	RadixNode *nodePtr = psp(partition, nodeAddr);

	/* Step 1: Sanity Checks. */
	CHKZERO(radixP_node_is_leaf(partition, nodePtr));

	/* Step 2: Clean up the node user data. */
	if(del_fn)
	{
		del_fn(partition, nodePtr->user_data);
	}
	nodePtr->user_data = 0;

	/* Step 3: Remove node from parent and release memory. */
	if(nodePtr->parent)
	{
		radixP_remove_child(partition, psp(partition, nodePtr->parent), nodePtr->order);
	}

	/* Step 4: free shared memory in use by the node. */

	if(nodePtr->children)
	{
		psm_free(partition, nodePtr->children);
	}
	if(nodePtr->key)
	{
		psm_free(partition, nodePtr->key);
	}

	memset(nodePtr, 0, sizeof(RadixNode));
	psm_free(partition, nodeAddr);

	/* Step 4: Update tree statistics. */
	tree->stats.nodes -= 1;

	return 1;
}



/******************************************************************************
 * @brief Corrects the depth assignments of nodes in a subtree.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in,out] node      - The root of the subtree being corrected
 *
 * @note
 * This is a pre-order traversal, just limited to the subtree rooted at the
 * given node.
 *****************************************************************************/

void radixP_fix_depth(PsmPartition partition, RadixNode *nodePtr)
{
	PsmAddress curNodeAddr = 0;
	RadixNode *curNodePtr = NULL;
	RadixNode *curNodeParentPtr = NULL;

	PsmAddress limitAddr = 0;
	PsmAddress nextChildAddr = 0;

	/* Step 1: Sanity Checks. */
	CHKVOID(partition);
	CHKVOID(nodePtr);

	/*
	 * Step 2: Set a "limit" at the node's parent to restrict
	 *         any iterations to this node. Since this function is
	 *         not recursive, this limit will prevent accidental
	 *         iteration beyond the subtree.
	 */
	limitAddr = nodePtr->parent;

	/* Step 3: Start with the first node's child. */
	curNodeAddr = radixP_get_child(partition, nodePtr, 0);
	while(curNodeAddr != 0)
	{
		curNodePtr = (RadixNode*) psp(partition, curNodeAddr);
		curNodeParentPtr = (RadixNode*) psp(partition, curNodePtr->parent);

		/* Step 3.1: Depth is one more than the parent's. */
		curNodePtr->depth = curNodeParentPtr->depth + 1;

		/* Step 3.2: Get the next node. */
		nextChildAddr = radixP_get_child(partition, curNodePtr, 0);
		if(nextChildAddr == 0)
		{
			/*
			 * Step 3.2.1: If we can't go to a child, make sure we
			 *             do not go up past the subtree root.
			 */
			curNodeAddr = radixP_get_next_node(partition, curNodeAddr, limitAddr, NULL);
		}
		else
		{
			curNodeAddr = nextChildAddr;
		}
	}
}



/******************************************************************************
 * @brief Retrieve a specific child address from a non-leaf node.
 *
 * @param[in] partition - The shared memory partition
 * @param[in] node      - The node whose child is being queried.
 * @param[in] order     - The specific child being queried.
 *
 * @retval !0 - The child node
 * @retval 0  - Failure
 *****************************************************************************/

PsmAddress radixP_get_child(PsmPartition partition, RadixNode *node, int order)
{
	PsmAddress *childArrayPtr = NULL;

	/* Step 1: Sanity checks. */
	CHKZERO(partition);
	CHKZERO(node);

	/* Step 2: Return NULL if asked for non-existent child. */
	if(order >= node->num_kids)
	{
		return 0;
	}

	/* Step 3: Return the child. */
	childArrayPtr = (PsmAddress*) psp(partition, node->children);
	return childArrayPtr[order];
}



/******************************************************************************
 * @brief Retrieve the next "equal or higher" node in the tree, up to a limit.
 *
 * @param[in]  partition - The share dmemory partition
 * @param[in]  nodeAddr  - The node whose child is being queried.
 * @param[in]  limitAddr - The highest available node.
 * @param[out] delta     - The key index shift (optional)
 *
 * @notes
 * This function will first check for any siblings and then siblings of the
 * parent node, and so on up to a limit node. This function will never
 * return a child node.
 * \par
 * The returned node may have a different depth and a different key length. The
 * delta variable captures differences in the key length of the given node and
 * the key length of the returned next node. This delta value is optional.
 * \par
 * A limit value of NULL implies the root node is the limit.
 * \par
 * The next node will not be a node already in the path to this node.
 *
 * @retval !0 - The next node
 * @retval 0  - Failure
 *****************************************************************************/

PsmAddress radixP_get_next_node(PsmPartition partition, PsmAddress nodeAddr, PsmAddress limitAddr, int *delta)
{
	RadixNode *curNodePtr = NULL;
	PsmAddress nextNodeAddr = 0;

	/* Step 1: Sanity Checks and initialization. */
	CHKZERO(partition);
	CHKZERO(nodeAddr);

	if(delta)
	{
		*delta = 0;
	}

	/* Step 2: Iterate through the subtree looking for the next node. */
	curNodePtr = (RadixNode*) psp(partition, nodeAddr);

	while(nodeAddr != 0)
	{

		/* Step 2.1: If we hit the limit there is no other next node. */
		if(curNodePtr->parent == limitAddr)
		{
			return 0;
		}

		/* Step 2.2: Look for siblings first. If we find one, return it.
		 *           There is no delta for a sibling node because the
		 *           sibling node shares a parent so the parent node key
		 *           length is unchanged.
		 *
		 *           If we can't find a sibling, go to the parent. This
		 *           will change the delta value because the new parent
		 *           reduces the key index by the length of the current
		 *           parent's key.
		 *
		 *           We do not consider returning the parent because the
		 *           parent is on the path to this node and, therefore,
		 *           is not a "next" node.
		 */
		if((nextNodeAddr = radixP_get_sibling_node(partition, curNodePtr)) == 0)
		{
			nodeAddr = curNodePtr->parent;
			curNodePtr = (RadixNode*) psp(partition, nodeAddr);

			if(delta)
			{
				char *nodeKey = (char *) psp(partition, curNodePtr->key);
				*delta += strlen(nodeKey);
			}
		}
		else
		{
			return nextNodeAddr;
		}
	}

	/* Step 3: If we get here, we found no next node. */
	return 0;
}



/******************************************************************************
 * @brief Retrieve the next sibling to a node.
 *
 * @param[in] partition - The shared memory partition
 * @param[in] node      - The node whose sibling is being queried.
 *
 * @notes
 * The next sibling is the parent child with the next higher order than the
 * given node.
 *
 * @retval !0 - The next sibling
 * @retval 0  - Failure
 *****************************************************************************/

PsmAddress radixP_get_sibling_node(PsmPartition partition, RadixNode *node)
{
	CHKZERO(node);
	CHKZERO(node->parent);

	return radixP_get_child(partition, psp(partition, node->parent), node->order+1);
}




/******************************************************************************
 * @brief Retrieve the terminal node for a given key.
 *
 * A terminal node for a key is on where we either have:
 *
 * 1. FULL MATCH NODE
 *    In this case, the user key exactly matches to a node in the tree.
 *    This item is a duplicate.
 *
 * 2. EXACT PARTIAL MATCH NODE
 *    In this case, a prefix subset of the user key exactly matches to a
 *    string in the tree. In this case, a child node to the exact
 *    partial match must be added.
 *
 * 3. SPLITTABLE MATCH NODE
 *    In this case, a prefix subset of a portion of the user key matches
 *    to a node in the tree. Since the match is not a full match at
 *    the node, the node must be "split" around the common subset
 *    and 2 children node created. One to hold the original node info
 *    and one to hold the new remainder of the user key.
 *
 * Consider nodes N1 ("ipn:"), N2("1.") and N3("1")
 * Consider the tree T1 as N1->N2->N3
 *
 * The user key "ipn:1.1" is a full match at node N3.
 * The user key "ipn:1.2" is an exact partial match at node N2.
 * The user key "ipn~" is a splittable match at node N1.
 *
 *
 * @param[in]     partition - The shared memory partition
 * @param[in]     radixPtr  - The node whose sibling is being queried.
 * @param[in]     key       - The key whose terminal node we are finding
 * @param[out]    type      - The type of match that was made
 * @param[in,out] key_idx   - The index of the given key match at the terminal node
 * @param[out]    node_idx  - The index of the node key match at the terminal node
 *
 * @notes
 * Index management is important. The key index is the index into the key
 * value used for matching the current node's key. The node_idx is the
 * index into the node's key where a match ends (for a substring match).
 * At the end of the function, key[key_idx] is the character that matches at
 * node->key[node_idx].
 *
 * @retval !0 - The terminal node for the key
 * @retval 0  - There is no match (terminal node is root)
 *****************************************************************************/

PsmAddress radixP_get_term_node(PsmPartition partition, RadixTree *radixPtr, char *key, int *type, int *key_idx, int *node_idx)
{
	int len = 0;
	PsmAddress nodeAddr = 0;
	RadixNode *nodePtr = NULL;
	PsmAddress tmpAddr = 0;
	char *nodeKeyPtr = NULL;


	/* Step 1: Sanity Checks and Initialization. */
	CHKZERO(partition);
	CHKZERO(type);
	*type = -1;

	CHKZERO(radixPtr);
	CHKZERO(key);
	CHKZERO(key_idx);
	CHKZERO(node_idx);

	len = strlen(key);
	*key_idx = 0;
	*node_idx = 0;
	*type = RADIX_MATCH_NONE;

	/* Step 2: Start the pre-order traversal... */
	nodeAddr = radixPtr->root;
	while(nodeAddr != 0)
	{
		nodePtr = (RadixNode*) psp(partition, nodeAddr);
		nodeKeyPtr = (char *) psp(partition, nodePtr->key);

		/* Step 2.1: Determine how the current node matches the key. */
		switch((*type = radixP_node_matches_key(nodeKeyPtr, &(key[*key_idx]), node_idx, 0)))
		{
			/*
			 * Step 2.1.1: If we are a full match, a wildcard match, or are a
			 *             superset of the key, then we are the terminal node.
			 */
			case RADIX_MATCH_WILDCARD:
			case RADIX_MATCH_FULL:
			case RADIX_MATCH_SUBSET:
				/* Step 2.1.1.1; We matched up to the node index. */
				*key_idx += *node_idx;
				/* Step 2.1.1.2: Return the terminal node. */
				return nodeAddr;
				break;

			/*
			 * Step 2.1.2: If we are a partial match, we might not be
			 * the terminal node - check the child nodes.
			 */
			case RADIX_MATCH_PARTIAL:
				/* Step 2.1.2.1: We matches up to the node index. */
				*key_idx += *node_idx;

				/*
				 * Step 2.1.2.2: A child node *might* be the terminal node
				 *               if (a) there is a child node and (b) there
				 *               is still some key left to match.
				 *
				 *               If a child node cannot be ther terminal node,
				 *               then the current node must be the terminal node.
				 */
				tmpAddr = radixP_get_child(partition, nodePtr, 0);
				if((tmpAddr == 0) || (*key_idx >= len))
				{
					return nodeAddr;
				}
				else
				{
					nodeAddr = tmpAddr;
				}
				break;

			/*
			 * Step 2.1.3: If this not is not a match at all, we can check a
			 *             sibling. If no siblings match, then our parent must
			 *             be the terminal node and the parent match is of
			 *             type partial (since when we looked at the parent
			 *             on the way to us we didn't get a full
			 *             match/wildcard/split match).
			 */
			case RADIX_MATCH_NONE:
				nodeAddr = radixP_get_sibling_node(partition, nodePtr);
				if(nodeAddr == 0)
				{
					*type = RADIX_MATCH_PARTIAL;
					return nodePtr->parent;
				}
				break;

			default:
				break;
		}
	}

	/* Step 3 - If we get here then ourb est guess is the root node. */
	return 0;
}



/******************************************************************************
 * @brief Allocates more children to a given node.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in,out] nodePtr   - The node getting more children. Congrats!
 *
 * @notes
 * Adding a single child node is inefficient because it could cause this
 * expensive function to be called multiple times. Instead, this function
 * will create RADIC_CILD_INCR number of children.
 * \par
 * When called on a node with no children, this function creates the initial
 * set of children.
 * @retval -1 - system error
 * @retval  1 - success
 *****************************************************************************/

int radixP_grow_children(PsmPartition partition, RadixNode *nodePtr)
{
	PsmAddress newArrayAddr = 0;
	int added;

	/* Step 0: Sanity Checks. */
	CHKERR(partition);
	CHKERR(nodePtr);

	/*
	 * Step 1: Figure out how many children nodes to allocate. If max-kids
	 *         is 0 there are 0 maximum children slots, so we are doing the
	 *         default allocation upon creating a new node. Otherwise, we are
	 *         incrementing by adding to a previously allocated set of
	 *         child nodes.
	 */
	added = (nodePtr->num_kids) ? RADIX_CHILD_INCR : RADIX_DEFAULT_CHILDREN;

	/* Step 2: Allocate space for kids + incr new child pointers. */

	newArrayAddr = radix_alloc(partition, ((nodePtr->num_kids + added) * sizeof (PsmAddress)));
	CHKERR(newArrayAddr);



	/*
	 * Step 2: If the node had children already, copy them into the new
	 *         storage area and then release the existing memory.
	 */
	if(nodePtr->num_kids)
	{
		PsmAddress *newArrayPtr = (PsmAddress*) psp(partition, newArrayAddr);
		PsmAddress *curArrayPtr = (PsmAddress*) psp(partition, nodePtr->children);
		int i;

		for(i = 0; i < nodePtr->num_kids; i++)
		{
			newArrayPtr[i] = curArrayPtr[i];
		}
		psm_free(partition, nodePtr->children);
	}

	/* Step 3: Initialize newly added children spots and add to node. */
	nodePtr->children = newArrayAddr;

	/* Step 4: Update node statistics. */
	nodePtr->num_kids += added;

	return 1;
}



/******************************************************************************
 * @brief Insert a node into the tree uder the given parent.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[out] tree         - The tree getting the new node.
 * @param[in]  parentAddr   - The parent for the inserted node
 * @param[in]  nodeAddr     - The node being inserted
 * @param[in]  order        - Which child to make the node.
 *
 * @notes
 * If a non-negative order is provided, this implies that the inserted node is
 * replacing some other node in the parent (which occurs when splitting a node).
 * In this case, the new node must become that specific, ordered child of the
 * parent.
 * \par
 * If a negative order is provided, then the node is to be added as the
 * next available child under the parent. In the special case where this is the
 * first node in the tree, an order of 0 will be assumed.
 * \par
 * If a parent has no space for children, the parent will "grow" more children
 * spots.
 *
 * @retval 1 - Success
 * @retval <=0 - Failure
 *****************************************************************************/

int radixP_insert_node(PsmPartition partition, RadixTree *tree, PsmAddress parentAddr, PsmAddress nodeAddr, int order)
{
	PsmAddress *childArrayPtr = NULL;
	RadixNode *nodePtr = NULL;
	RadixNode *parentPtr = NULL;

	/* Step 0: Sanity Checks. */
	CHKERR(tree);
	CHKERR(parentAddr);
	CHKERR(nodeAddr);

	nodePtr = (RadixNode*) psp(partition, nodeAddr);
	parentPtr = (RadixNode*) psp(partition, parentAddr);

	childArrayPtr = (PsmAddress *) psp(partition, parentPtr->children);
	CHKERR(childArrayPtr);

	/* Step 1: If we just want the next child slot...*/
	if(order == -1)
	{
		/* Step 1.1: Find the next available child slot. */
		for(order = 0; order < parentPtr->num_kids; order++)
		{
			if(childArrayPtr[order] == 0)
			{
				break;
			}
		}

		/*
		 * Step 1.2: If the parent is full, grow the parent to be able to hold
		 *           extra children. After this call, the index "order" should
		 *           point to the index of the first newly added child slot.
		 */
		if(order == parentPtr->num_kids)
		{
			CHKERR(radixP_grow_children(partition, parentPtr));
			childArrayPtr = (PsmAddress *) psp(partition, parentPtr->children);
			CHKERR(childArrayPtr);
		}
	}

	/* Step 2: Ensure that the order selected is valid for this parent. */
	CHKERR(order < parentPtr->num_kids);

	/* Step 3: Insert the child node. */
	childArrayPtr[order] = nodeAddr;
	nodePtr->parent = parentAddr;
	nodePtr->order = order;
	nodePtr->depth = parentPtr->depth + 1;
	tree->stats.nodes++;
	return 1;
}



/******************************************************************************
 * @brief Determiens if a node is a leaf node.
 *
 * @param[in] partition - The shared memory partition
 * @param[in] nodePtr   - The node being checked.
 *
 * @notes
 * A node is a leaf node if it's first possible child entry is empty.
 *
 * @retval  1 This is a leaf node
 * @retval  0 This is not a leaf node
 * @retval -1 Error
 *****************************************************************************/

int radixP_node_is_leaf(PsmPartition partition, RadixNode *node)
{
	PsmAddress *nodeChildPtr = NULL;

	CHKERR(partition);
	CHKERR(node);

	nodeChildPtr = (PsmAddress*) psp(partition, node->children);

	return (nodeChildPtr[0] == 0) ? 1 : 0;
}



/******************************************************************************
 * @brief Characterize the relationship between a node and a given key.
 *
 * @param[in]  node_key - The node key being characterized
 * @param[in]  key      - The key to evlauate the node against
 * @param[out] idx      - The key index associated with any special circumstances
 *
 * @notes
 * This function assumes that both the given key and the node key are NULL-terminated.
 *
 * @retval RADIX_MATCH_NONE     - There is no overlap between the node and key
 * @retval RADIX_MATCH_PARTIAL  - The node is an exact partial match with the key
 * @retval RADIX_MATCH_FULL     - The node is a complete match to the key
 * @retval RADIX_MATCH_SUBSET   - The node is a superset of the key
 * @retval RADIX_MATCH_WILDCARD - The node matches the key by wildcard.
 * @retval -1 Error
 *****************************************************************************/

int radixP_node_matches_key(char *node_key, char *key, int *idx, int wildcard)
{
	int key_len = 0;
	int i = 0;

	/* Step 1: Sanity check and initialization */
	CHKERR(node_key);
	CHKERR(key);
	CHKERR(idx);

	key_len = strlen(node_key);

	/* Step 2: Compare the key and the node key. */
	for(i = 0; i < key_len; i++)
	{
		/*
		 * Step 2.1: If we hit a wildcard character on one but not both keys
		 *           then this is a wildcard match. If we hit the wildcard
		 *           on the same position of both keys, this is an exact
		 *           match instead.
		 */
		if(wildcard)
		{
//			if(((node_key[i] == RADIX_PREFIX_WILDCARD) && (key[i] != RADIX_PREFIX_WILDCARD)) ||
//((key[i] == RADIX_PREFIX_WILDCARD) && (node_key[i] != RADIX_PREFIX_WILDCARD)))
		  if((node_key[i] == RADIX_PREFIX_WILDCARD) && (key[i] != RADIX_PREFIX_WILDCARD))
			{
				/* Step 2.1.1: The key index is just before the wildcard. */
				*idx = i-1;
				return RADIX_MATCH_WILDCARD;
			}
		}
		/*
		 * Step 2.2: If the keys differ, we either have a subset match or
		 *           no match.
		 *
		 *           A subset match happens if the inequality happens on
		 *           something other than the first character. This is
		 *           also the case when the given key is shorter than the
		 *           node key and key[i] is the NULL terminator.
		 *
		 *           If the mismatch happens on the first character, we
		 *           matched nothing and this is a no match.
		 */
		if(key[i] != node_key[i])
		{
			/* Step 2.2.1: The key index is this current index. */
			*idx = i;
			return (i == 0) ? RADIX_MATCH_NONE : RADIX_MATCH_SUBSET;
		}
	}

	/* Step 3: If we get here there was no mismatch between the node key
	 *         and the given key, and no wildcrd was found. This is
	 *         either a full or partial match.
	 *
	 *         If the index is the full given key length, then this is
	 *         a full match.
	 *
	 *         If the index is not the full key length, then this was
	 *         only a partial match.
	 */
	*idx = key_len;
	return (strlen(key) == key_len) ? RADIX_MATCH_FULL : RADIX_MATCH_PARTIAL;
}



/******************************************************************************
 * @brief Removes a node from the tree.
 *
 * @param[in,out] partition - The shared memory partition
 * @param[in,out] node      - The node losing a child.
 * @param[in]     order     - Which child node to remove
 *
 * @notes
 * This function does not free the memory associated with the node.
 *
 * @retval !0 - The address of the removed child
 * @retval 0  - No child was removed.
 *****************************************************************************/

PsmAddress radixP_remove_child(PsmPartition partition, RadixNode *node, int order)
{
	PsmAddress childAddr = 0;
	PsmAddress *childArrayPtr = NULL;
	RadixNode  *childPtr = NULL;


	/* Step 0: Sanity Checks. */
	CHKZERO(node);
	CHKZERO(order < node->num_kids);

	/*
	 * Step 1: Starting at the child to remove, iterate through remaining
	 *         children shifting everything back 1.
	 *
	 *         This can stop when we hit the first NULL child or when we
	 *         have shifted all remaining children of order greater than
	 *         the child being removed.
	 */
	childArrayPtr = (PsmAddress *) psp(partition, node->children);
	childAddr = childArrayPtr[order];
	while(order < node->num_kids)
	{
		/* Step 1.1: We hit the end of the "child list" for this node. */
		if(childArrayPtr[order] == 0)
		{
			break;
		}
		/*
		 * Step 1.2: If this is the last non-NULL child, set it to NULL.
		 *           This child was already "shifted left" by a previous
		 *           iteration (or is the child being removed.
		 */
		else if((order == node->num_kids - 1) || (childArrayPtr[order+1] == 0))
		{
			childArrayPtr[order] = 0;
		}
		/*
		 * Step 1.3: If there are more children "to the right", shift the
		 *           rightmost child into this spot. On the first iteration
		 *           this will "forget" the node being removed.
		 */
		else
		{
			childArrayPtr[order] = childArrayPtr[order+1];
			childPtr = (RadixNode *) psp(partition, childArrayPtr[order]);
			childPtr->order = order;
		}

		/* Step 1.4: Handle the next child node...*/
		order++;
	}

	/* Step 2: Return the node removed from the parent. */
	return childAddr;
}



/******************************************************************************
 * @brief Splits a single node into a split-parent and split-child, and then
 *        optionally adds a new child to the split parent.
 *
 * The algorithm is:
 *   - Make a new "parent" node and insert it into the grandparent.
 *   - Take split node and make it a child under the new parent.
 *   - Add any extra new children under the new parent.
 *
 * @param[in,out] partition     - The shared memory partition
 * @param[in,out] radixPre      - The tree doing the split/insert
 * @param[in,out] splitNodeAddr - node being split
 * @param[in]     newChildAddr  - The new child to add.
 * @param[in]     offset        - The index where we split he key of the split-node.
 * @param[in]     del_fn        - User-supplied delete function
 *
 * @note
 * If the new_child is NULL, we are just splitting the split_node.
 *
 * @retval  1 - Success
 * @retval  0 - Failure
 * @retval -1 - System error
 *****************************************************************************/

int radixP_split_insert_node(PsmPartition partition, RadixTree *radixPtr, PsmAddress splitNodeAddr, PsmAddress newChildAddr, int offset, radix_del_fn del_fn)
{
	PsmAddress splitParentAddr = 0;
	RadixNode *splitParentPtr = NULL;
	RadixNode *splitNodePtr = NULL;
	char *splitNodeKeyPtr = NULL;
	char *newKeyPtr = NULL;
	PsmAddress newKeyAddr = 0;
	int len;

	/* Step 1: Sanity checks and initialization */
	CHKERR(partition);
	CHKERR(radixPtr);
	CHKERR(splitNodeAddr);

	splitNodePtr = (RadixNode *)psp(partition, splitNodeAddr);

	/*
	 * Step 2: Create the split-node parent and insert it in place of the
	 *         split-node in the split-node's original parent. The key
	 *         for the split parent is the prefix of the split node's key
	 *         up to the offset and represents the part of the key shared
	 *         by the split-node and the new-child under the split-parent.
	 *
	 *         The split node parent has no data in it, unless the new
	 *         child was a perfect match for the split node, which is
	 *         handled later.
	 */
	splitNodeKeyPtr = (char *) psp(partition, splitNodePtr->key);
	splitParentAddr = radixP_create_node(partition, splitNodeKeyPtr, offset, 0);
	if(splitNodePtr->parent != 0)
	{
		radixP_insert_node(partition, radixPtr, splitNodePtr->parent, splitParentAddr, splitNodePtr->order);
	}
	else
	{
		radixPtr->root = splitParentAddr;
	}

	/*
	 * Step 3: Shorten the split-node's key to be just the part of the key after
	 *         the offset. Everything before the offset is not part of the key of
	 *         the split-parent node.
	 */
	len = strlen(splitNodeKeyPtr);
	newKeyAddr = radix_alloc(partition, len-offset+1);
	CHKERR(newKeyAddr);
	newKeyPtr = (char *) psp(partition, newKeyAddr);

	memcpy(newKeyPtr, splitNodeKeyPtr+offset, len-offset);

	psm_free(partition, splitNodePtr->key);
	splitNodePtr->key = newKeyAddr;

	/* Step 5: Add the split node under the new parent node as first child. */
	radixP_insert_node(partition, radixPtr, splitParentAddr, splitNodeAddr, 0);

	/* Step 6: This changes all the depths, so fix them. */
	radixP_fix_depth(partition, splitNodePtr);

	/*
	 * Step 7: If there is a new child node, add that as the next child
	 *         under the split parent.
	 */
	if(newChildAddr != 0)
	{
		RadixNode *newChildPtr = (RadixNode*) psp(partition, newChildAddr);
		//char *childKeyPtr = (char *) psp(partition, newChildPtr->key);

		/* Step 7.1: If the split-parent happens to be an exact match for the
		 *           new-child's key then the split-parent is, actually, the
		 *           right place for the child node's data.
		 *
		 *           The split parent has no user data because we just created
		 *           it in this function. So, in this special circumstance just
		 *           shallow-copy the user data to the split-parent and then
		 *           drop the new-child node without inserting it.
		 */
		if(newChildPtr->key == 0)
		{
			splitParentPtr = (RadixNode*) psp(partition, splitParentAddr);
			splitParentPtr->user_data = newChildPtr->user_data;

			radixP_del_leafnode(partition, newChildAddr, radixPtr, del_fn);
//			psm_free(partition, newChildAddr);
		}
		else
		{
			// TODO Consider making this an offset insert of -1 instead of 1.
			radixP_insert_node(partition, radixPtr, splitParentAddr, newChildAddr, -1);
		}
	}

	return 1;
}