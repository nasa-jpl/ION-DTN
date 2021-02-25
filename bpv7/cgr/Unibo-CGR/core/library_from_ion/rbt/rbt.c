/**************************************************************
 * \file rbt.c
 *
 * \brief Red-Black Tree library implementation.
 *
 * \details There are some differences with the ION's implementation:
 *          - This rbt can't be used with shared memory.
 *          - I add a function to print the rbt.
 *          - Restyling in some functions.
 *          - Memory allocated with MWITHDRAW and deallocated with MDEPOSIT.
 *
 * \par Ported from ION 3.7.0 by
 *      Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 * 
 * \par Supervisor
 *      Carlo Caini, carlo.caini@unibo.it
 *
 * \par Date:
 *      05/01/20
 ***************************************************************/

/*	SmRbt.c:	shared-memory red-black tree management library.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	11-17-11  SCB	Adapted from Julienne Walker tutorial, public
 *			domain code.
 *	(http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx)
 *
 *	Copyright (c) 2011, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 */

/*		Private definitions of shared-memory rbt structures.	*/

#include "rbt.h"

#include <stdlib.h>

#define LEFT 0
#define RIGHT 1

/*	*	*	Rbt management functions	*	*	*/

/******************************************************************************
 *
 * \par Function Name:
 *      eraseTree
 *
 * \brief Reset the Rbt's fields
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbt   The Rbt that we want to reset
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static void eraseTree(Rbt *rbt)
{
	rbt->userData = NULL;
	rbt->root = NULL;
	rbt->length = 0;
}

/******************************************************************************
 *
 * \par Function Name:
 *      eraseTreeNode
 *
 * \brief Reset the RbtNode's fields
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *node   The RbtNode that we want to reset
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static void eraseTreeNode(RbtNode *node)
{
	node->rbt = NULL;
	node->parent = NULL;
	node->child[LEFT] = NULL;
	node->child[RIGHT] = NULL;
	node->data = NULL;
	node->isRed = 0;
}

/******************************************************************************
 *
 * \par Function Name:
 *      nodeIsRed
 *
 * \brief Reset the Rbt's fields
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return int
 *
 * \retval   1  If the node is red
 * \retval   0  If the node isn't red (black), or the node points to NULL
 *
 * \param[in]  *node   The node for which we want to check if it is red.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static int nodeIsRed(RbtNode *node)
{
	int result;

	if (node == NULL)
	{
		result = 0;
	}
	else
	{
		result = node->isRed;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_create
 *
 * \brief Allocate memory for an Rbt type.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return Rbt*     The allocated Rbt
 * \return NULL     MWITHDRAW error
 *
 * \param[in]  deleteFn    The function that we want to use to delete the RbtNode->data
 * \param[in]  compareFn   The function that we want to use to compare the RbtNode->data
 *
 * \par Notes:
 *      1. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
Rbt* rbt_create(RbtDeleteFn deleteFn, RbtCompareFn compareFn)
{
	Rbt *rbt;

	rbt = (Rbt*) MWITHDRAW(sizeof(Rbt));

	if (rbt != NULL)
	{
		eraseTree(rbt);
		rbt->deleteFn = deleteFn;
		rbt->compareFn = compareFn;
	}

	return rbt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      destroyRbtNodes
 *
 * \brief Remove all the nodes belonging to the tree
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbtPtr    The rbt from which we want to Remove all the nodes
 * \param[in]  deleteFn   The function that we want to use to delete the data pointed by the nodes.
 *
 * \par Notes:
 *      1. The deleteFn will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static void destroyRbtNodes(Rbt *rbtPtr, RbtDeleteFn deleteFn)
{
	RbtNode *nodePtr;
	RbtNode *parentNode;

	nodePtr = rbtPtr->root;
	/*	Destroy all nodes of the tree.				*/

	while (nodePtr != NULL)
	{
		/*	If node has a left subtree, descend into it.	*/

		if (nodePtr->child[LEFT] != NULL)
		{
			nodePtr = nodePtr->child[LEFT];
			continue;
		}

		/*	If node has a right subtree, descend into it.	*/

		if (nodePtr->child[RIGHT] != NULL)
		{
			nodePtr = nodePtr->child[RIGHT];
			continue;
		}

		/*	Node is a leaf, so can delete it.		*/

		parentNode = nodePtr->parent;
		if (deleteFn != NULL)
		{
			deleteFn(nodePtr->data);
		}

		/*	just in case user mistakenly accesses later...	*/
		eraseTreeNode(nodePtr);
		MDEPOSIT(nodePtr);

		/*	Now pop back up to this node's parent.		*/

		if (parentNode == NULL)
		{
			rbtPtr->root = NULL;
			break; /*	Have deleted the root node.	*/
		}

		/*	Erase the child pointer for the child that
		 *	has now been deleted.				*/

		if (nodePtr == parentNode->child[LEFT])
		{
			parentNode->child[LEFT] = NULL;
		}

		if (nodePtr == parentNode->child[RIGHT])
		{
			parentNode->child[RIGHT] = NULL;
		}

		nodePtr = parentNode;

	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_clear
 *
 * \brief Remove all the nodes belonging to the tree
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbt  The rbt from which we want to Remove all the nodes (and not the rbt itself)
 *
 * \par Notes:
 *      1. The rbt->deleteFn will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void rbt_clear(Rbt *rbt)
{
	if (rbt != NULL)
	{
		destroyRbtNodes(rbt, rbt->deleteFn);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_destroy
 *
 * \brief Remove all the nodes belonging to the tree, and the rbt itself
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbt  The rbt we want to delete (including all the nodes)
 *
 * \par Notes:
 *      1. The rbt->deleteFn will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void rbt_destroy(Rbt *rbt)
{
	if (rbt != NULL)
	{
		destroyRbtNodes(rbt, rbt->deleteFn);

		/*	Now destroy the tree itself.				*/

		/*	just in case user mistakenly accesses later...		*/
		eraseTree(rbt);
		MDEPOSIT(rbt);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_user_Data
 *
 * \brief Get the userData field of the rbt
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbt   The rbt from which we want to get the userData field (rbt->userData)
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void* rbt_user_data(Rbt *rbt)
{
	void *userData = NULL;

	if (rbt != NULL)
	{

		userData = rbt->userData;
	}
	return userData;
}

/******************************************************************************
 *
 * \par Function Name: rbt_user_data_set
 *
 * \brief Set the userData field of the rbt
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbt   The rbt for which we want to set the userData field
 * \param[in]  *data  The data to which the rbt->userData will points
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void rbt_user_data_set(Rbt *rbt, void *data)
{

	if (rbt != NULL && data != NULL)
	{
		rbt->userData = data;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_length
 *
 * \brief Get the number of the nodes belonging to the rbt
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return long unsigned int
 *
 * \retval ">= 0"  The number of the nodes belonging to the rbt
 *
 * \param[in]  *rbt  The rbt from which we want to known the length (rbt->length)
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
long unsigned int rbt_length(Rbt *rbt)
{
	long unsigned int length = 0;

	if (rbt != NULL)
	{
		length = rbt->length;
	}

	return length;
}

/*	Rotation to the RIGHT is a clockwise rotation: the root node
 *	of the subtree becomes the RIGHT child of its former LEFT
 *	child, which becomes the new root of the subtree, and that
 *	child's former right child becomes the left child of the
 *	subtree's original root node.  The new root of the subtree
 *	is returned.
 *
 *	Rotation to the LEFT is a counter-clockwise rotation: the
 *	root node of the subtree becomes the LEFT child of its former
 *	RIGHT child, which becomes the new root of the subtree, and
 *	that child's former left child becomes the right child of the
 *	subtree's original root node.  The new root node of the
 *	subtree is returned.
 *
 *	Both rotations leave the tree's traversal order unchanged.	*/

/******************************************************************************
 *
 * \par Function Name:
 *      rotateOnce
 *
 * \brief Rotate the subtree in some direction (LEFT or RIGHT)
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*  The new subtree's root
 *
 * \param[in]  *root       The current root of the subtree
 * \param[in]  *direction  The rotation (LEFT or RIGHT)
 *
 * \warning root doesn't have to be NULL.
 *
 * \par Notes:
 *      1. The current root->parent after the rotation will not
 *         be aware of the rotation (parent->child[x] unchanged)
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static RbtNode* rotateOnce(RbtNode *root, int direction)
{
	int otherDirection = 1 - direction;
	RbtNode *pivot;
	RbtNode *orphan;

	pivot = root->child[otherDirection];
	orphan = pivot->child[direction];
	pivot->parent = root->parent;
	pivot->child[direction] = root;
	pivot->isRed = 0;
	root->parent = pivot;
	root->child[otherDirection] = orphan;
	root->isRed = 1;
	if (orphan != NULL)
	{
		orphan->parent = root;
	}

	return pivot;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rotateOnce
 *
 * \brief Rotate the subtree two times in some direction (LEFT or RIGHT)
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return 	RbtNode* The new subtree's root
 *
 * \param[in]  *root		-	The current root of the subtree
 * \param[in]  *direction	-	The rotation (LEFT or RIGHT)
 *
 * \warning root doesn't have to be NULL.
 *
 * \par Notes:
 *      1. The current root->parent after the rotation will not
 *         be aware of the rotation (parent->child[x] unchanged)
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static RbtNode* rotateTwice(RbtNode *root, int direction)
{
	int otherDirection = 1 - direction;

	root->child[otherDirection] = rotateOnce(root->child[otherDirection], otherDirection);
	return rotateOnce(root, direction);
}

/******************************************************************************
 *
 * \par Function Name:
 *      createNode
 *
 * \brief Allocate memory (with MWITHDRAW) for an RbtNode
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*
 *
 * \retval RbtNode*  The new allocated RbtNode
 * \retval NULL      MWITHDRAW error
 *
 * \param[in]     *rbt        The tree to which the new RbtNode will belongs.
 * \param[in]     *parent     The parent of the new RbtNode
 * \param[in]     *data       The data pointed by the new RbtNode
 * \param[in,out] **buffer    This field has to be NULL if the new RbtNode
 *                            has to be the root of the tree; if != NULL
 *                            *buffer will point to the new RbtNode
 *
 * \par Notes:
 *      1. The new RbtNode is always red, if and only if the new RbtNode
 *         has to be the new root of the tree it will be black. (Root is always black).
 *      2. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static RbtNode* createNode(Rbt *rbt, RbtNode *parent, void *data, RbtNode **buffer)
{
	RbtNode *node;

	node = (RbtNode*) MWITHDRAW(sizeof(RbtNode));

	if (node != NULL)
	{
		node->rbt = rbt;
		node->parent = parent;
		node->child[LEFT] = NULL;
		node->child[RIGHT] = NULL;
		node->data = data;
		if (buffer != NULL) /*	Tree already has a root.		*/
		{
			node->isRed = 1;
			*buffer = node;
		}
		else /*	This is the first node in the tree.	*/
		{
			node->isRed = 0; /*	Root is always black.	*/
		}
	}

	return node;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_insert
 *
 * \brief  Insert an RbtNode to the tree.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*
 *
 * \retval RbtNode*  The new inserted RbtNode
 * \retval NULL      MWITHDRAW error
 *
 * \param[in]  *rbt   The tree to which the new RbtNode will belongs.
 * \param[in]  *data  The data pointed by the new RbtNode
 *
 * \par Notes:
 * 			1. The compareFn will be used to find the position where the new node will be inserted.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
RbtNode* rbt_insert(Rbt *rbt, void *data)
{
	RbtNode dummyRootBuffer = { NULL, NULL, { NULL, NULL }, NULL, 0 };
	/*	Dummy root is a trick; it
	 *	lets us avoid constantly
	 *	rebalancing at actual root.	*/
	RbtNode *child[2] = { NULL, NULL };
	RbtNode *node = NULL;		//	q
	RbtNode *greatgrandparent = NULL;		//	t
	RbtNode *grandparent = NULL;		//	g
	RbtNode *parent = NULL;				//	p
	int direction = LEFT;	//	dir
	int prevDirection = LEFT;	//	last
	int subtree;		//	dir2
	RbtCompareFn compare;

	if (rbt != NULL && data != NULL)
	{
		compare = rbt->compareFn;

		if (rbt->root == NULL)
		{
			node = createNode(rbt, NULL, data, NULL);
			if (node != NULL)
			{
				rbt->root = node;
				rbt->length = 1;
			}

			return node;
		}

		/*	Initialize for top-down insertion pass.  Tree is
		 *	known to be non-empty, so every newly inserted node
		 *	will be a leaf with a non-zero parent (at least at
		 *	first).							*/

		greatgrandparent = &dummyRootBuffer;
		greatgrandparent->child[RIGHT] = rbt->root;
		grandparent = NULL; /*	None.		*/
		parent = NULL; /*	None.		*/
		node = rbt->root;
		while (1)
		{
			if (node == NULL) /*	(Can't be tree's root node.)	*/
			{
				/*	Have found the position at which this
				 *	node should be inserted.		*/

				node = createNode(rbt, parent, data, &node);
				if (node == NULL)
				{
					return NULL;
				}

				/*	Update parent's child pointer.  Note
				 *	that parent must already exist and be
				 *	known, because this is not the root
				 *	node.					*/

				parent->child[direction] = node;
			}
			else /*	(Might be the root node.)	*/
			{

				/*	Check for need to flip colors.		*/

				if (nodeIsRed(node->child[LEFT]) && nodeIsRed(node->child[RIGHT]))
				{
					/*	The two children are RED, so
					 *	make current node RED and
					 *	children BLACK.			*/

					node->isRed = 1;
					child[LEFT] = node->child[LEFT];
					child[LEFT]->isRed = 0;
					child[RIGHT] = node->child[RIGHT];
					child[RIGHT]->isRed = 0;
				}
			}

			/*	Now look for a newly introduced red violation;
			 *	if present, fix it.  Note that there can't be
			 *	a violation if this is the root node, because
			 *	the parent is the dummy root, which is black.	*/

			if (nodeIsRed(parent) && nodeIsRed(node))
			{
				if (greatgrandparent->child[RIGHT] == grandparent)
				{
					subtree = RIGHT;
				}
				else
				{
					subtree = LEFT;
				}

				if (node == parent->child[prevDirection])
				{
					greatgrandparent->child[subtree] = rotateOnce(grandparent, 1 - prevDirection);
				}
				else /*	Child in current direction.	*/
				{
					greatgrandparent->child[subtree] = rotateTwice(grandparent, 1 - prevDirection);
				}
			}

			/*	Stop if this data item is in the tree, either
			 *	from earlier insertion or from this one.	*/

			if (node->data == data)
			{
				break;
			}

			/*	Decide which branch to take next.		*/

			prevDirection = direction;
			if (compare(node->data, data) < 0)
			{
				direction = RIGHT;
			}
			else
			{
				direction = LEFT;
			}

			/*	Descend one level in the tree and try again.	*/

			if (grandparent != NULL)
			{
				greatgrandparent = grandparent;
			}

			grandparent = parent;
			parent = node;
			node = node->child[direction];
		}

		rbt->length += 1;

		/*	Record the new root node object.			*/

		rbt->root = dummyRootBuffer.child[RIGHT];

		/*	Make sure the root node is black.			*/

		node = rbt->root;
		node->isRed = 0;
	}
	return node;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_delete
 *
 * \brief Delete the node that has as data equals to dataBuffer (if any).
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in]  *rbt         The tree for which we want to search the node to delete.
 * \param[in]  *dataBuffer  The data that we use to search the node to delete.
 *
 * \par Notes:
 *      1. The compareFn will be used to search the node to delete
 *      2. The deleteFn will be called, if not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void rbt_delete(Rbt *rbt, void *dataBuffer)
{
	RbtNode dummyRootBuffer = { NULL, NULL, { NULL, NULL }, NULL, 0 };
	/*	Dummy root is a trick; it
	 *	lets us avoid special cases.	*/
	RbtNode *node;		//	q
	RbtNode *sibling;		//	s
	RbtNode *parent;		//	p
	RbtNode *stepparent;		//	p
	RbtNode *grandparentPtr;	//	g
	RbtNode *target;		//	f
	int direction;		//	dir
	int otherDirection;		//	!dir
	int prevDirection;		//	last
	int prevOtherDirection;	//	!last
	int subtree;		//	dir2
	RbtNode *childPtr[2];
	int result;
	int i;
	int j;
	RbtCompareFn compare;
	RbtDeleteFn deleteFn;

	if (rbt != NULL)
	{
		compare = rbt->compareFn;
		deleteFn = rbt->deleteFn;
		if (rbt->length == 0 || rbt->root == NULL)
		{
			return;
		}

		/*	We descend the tree, searching for the node that is
		 *	to be deleted (rather than requiring its address),
		 *	because we want to ensure that the node has been
		 *	recolored red -- if necessary -- by the time we
		 *	reach it.  This is because deleting a red node never
		 *	violates any red-black tree rules, so there is no
		 *	subsequent tree fix-up needed.  Our general strategy
		 *	is to push "red-ness" all the way down to the target
		 *	node, rotating as necessary to ensure that the target
		 *	node is transformed into a red leaf.
		 *
		 *	Note: a special case is when the node that is to be
		 *	deleted is the only node in the tree, hence the root.
		 *	In this case (only), we can delete a black node
		 *	without violating any red-black tree rules.
		 *
		 *	Note 2: this searching strategy is the reason we need
		 *	the compare function provided on node deletion just
		 *	as on node insertion.					*/

		grandparentPtr = NULL;
		parent = NULL;
		node = &dummyRootBuffer;
		node->child[RIGHT] = rbt->root;
		target = NULL; /*	f		*/
		direction = RIGHT;
		while (node->child[direction] != NULL)
		{
			prevDirection = direction;
			grandparentPtr = parent;

			/*	The first time through the loop, parentPtr
			 *	is NULL so grandparentPtr is still NULL.  The
			 *	second time through the loop, nodePtr is a
			 *	pointer to the dummy root, so the dummy root
			 *	becomes the grandparent.  The third time, a
			 *	pointer to the tree's root (a real node) is
			 *	placed in grandparentPtr.			*/

			parent = node;

			/*	The first time through the loop, nodePtr is
			 *	a pointer to the dummy root, so the dummy root
			 *	becomes the parent.  The second time, a pointer
			 *	to the tree's root (a real node) is placed in
			 *	parentPtr.					*/

			node = node->child[direction];

			/*	The first time through the loop, a pointer
			 *	to the tree's root (a real node) is placed in
			 *	nodePtr before any other processing happens.
			 *
			 *	Note that the loop continues PAST the node
			 *	that is to be deleted, locating that node's
			 *	inorder predecessor.  Then we come back to
			 *	move the predecessor's data into the target
			 *	node (erasing the target node's data) and
			 *	destroy to predecessor node.			*/

			result = compare(node->data, dataBuffer);
			if (result < 0)
			{
				direction = RIGHT;
			}
			else
			{
				direction = LEFT;
				if (result == 0)
				{
					target = node;
				}
			}

			if (nodeIsRed(node) || nodeIsRed(node->child[direction]))
			{
				continue; /*	No recolor needed.	*/
			}

			/*	Neither the node nor its child (if any) in
			 *	the current direction of search are red, so
			 *	we must introduce some redness.			*/

			otherDirection = 1 - direction;
			if (nodeIsRed(node->child[otherDirection]))
			{
				/*	This is the "red sibling" case.  Rotate
				 *	current node down/red.			*/

				childPtr[otherDirection] = node->child[otherDirection];
				parent->child[prevDirection] = rotateOnce(node, direction);

				/*	The new root of the subtree whose root
				 *	before rotation was "node" [the red
				 *	sibling] must be noted as "parent" in
				 *	place of the original parent of "node",
				 *	because it is now the actual parent
				 *	of "node".				*/

				parent = parent->child[prevDirection];

				/*	We've introduced the necessary redness;
				 *	time to descend again.			*/

				continue;
			}

			/*	Node and all of its children (if any) are now
			 *	known to be black.
			 *
			 *	Remaining cases concern the children (if any)
			 *	of the sibling (if any) of the current node.	*/

			prevOtherDirection = 1 - prevDirection;
			sibling = parent->child[prevOtherDirection];
			if (sibling == NULL) /*	(True of tree root.)	*/
			{
				continue; /*	No sibling, no problem.	*/
			}

			/*	Node has got a sibling.  Therefore it can't be
			 *	the root of the tree, so the root of the tree
			 *	must be above this node.  Therefore this node's
			 *	parent cannot be the dummy node, so both the
			 *	node's parent and the dummy node are above this
			 *	node in the tree.  So the node must have a
			 *	grandparent (the dummy node or some real node
			 *	lower in the tree).  So grandparentPtr cannot
			 *	be NULL.					*/

			/*	Check for need to flip colors.  NOT TREE ROOT.	*/

			if (nodeIsRed(sibling->child[LEFT]) == 0 && nodeIsRed(sibling->child[RIGHT]) == 0)
			{
				/*	Sibling has no red children, so it's
				 *	safe to make both the current node and
				 *	its sibling RED and their parent BLACK.	*/

				parent->isRed = 0;
				sibling->isRed = 1;
				node->isRed = 1;
				continue;
			}

			if (parent == grandparentPtr->child[RIGHT])
			{
				subtree = RIGHT;
			}
			else
			{
				subtree = LEFT;
			}

			/*	Note: grandparent is known to have a child
			 *	on the "subtree" side ("parent"), even if
			 *	grandparent is dummy.  And that child is
			 *	known to have two children -- "node" and
			 *	"sibling".					*/

			if (nodeIsRed(sibling->child[prevDirection]))
			{
				grandparentPtr->child[subtree] = rotateTwice(parent, prevDirection);
			}
			else
			{
				if (nodeIsRed(sibling->child[prevOtherDirection]))
				{
					grandparentPtr->child[subtree] = rotateOnce(parent, prevDirection);
				}
			}

			/*	The subtree may now have a new parent, due
			 *	to rotation.  Ensure nodes have correct colors.	*/

			stepparent = grandparentPtr->child[subtree];
			stepparent->isRed = 1;
			node->isRed = 1;
			childPtr[LEFT] = stepparent->child[LEFT];
			childPtr[LEFT]->isRed = 0;
			childPtr[RIGHT] = stepparent->child[RIGHT];
			childPtr[RIGHT]->isRed = 0;
		}

		/*	At this point the current node is NOT the one we're
		 *	trying to delete.  It is the inorder predecessor of
		 *	the node we're trying to delete -- a node whose
		 *	content we want to retain.  The data that's in this
		 *	predecessor is retained by copying it into the
		 *	target node -- which effectively deletes the target
		 *	node without actually removing it from the tree.
		 *	Then the current node (the inorder predecessor of
		 *	the target node, which is now redundant) is removed
		 *	from the tree.						*/

		if (target != NULL)
		{
			if (deleteFn != NULL)
			{
				deleteFn(target->data);
			}

			target->data = node->data;

			/*	To destroy inorder predecessor node, must
			 *	make its parent forget about it.		*/

			if (parent->child[RIGHT] == node)
			{
				i = RIGHT;
			}
			else
			{
				i = LEFT;
			}

			if (node->child[LEFT] == NULL)
			{
				j = RIGHT;
			}
			else
			{
				j = LEFT;
			}

			parent->child[i] = node->child[j];

			/*	just in case user mistakenly accesses later...	*/
			eraseTreeNode(node);
			MDEPOSIT(node);
			rbt->length -= 1;
		}

		/*	Update root in rbt object.				*/

		rbt->root = dummyRootBuffer.child[RIGHT];

		/*	Make sure root is BLACK.				*/

		if (rbt->root != NULL)
		{
			node = rbt->root;
			node->isRed = 0;
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_first
 *
 * \brief Get the first node of the rbt
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*
 *
 * \retval RbtNode*  The first node of the rbt
 * \retval NULL      The tree is empty
 *
 * \param[in]  *rbt  The tree from which we want to get the first node
 *
 * \par Notes:
 *			1. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
RbtNode* rbt_first(Rbt *rbt)
{
	RbtNode *first = NULL;
	RbtNode *node;

	if (rbt != NULL)
	{
		node = rbt->root;

		while (node != NULL)
		{
			first = node;
			node = node->child[LEFT];
		}

	}
	return first;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_last
 *
 * \brief Get the last node of the rbt
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*
 *
 * \retval RbtNode*  The last node of the rbt
 * \retval NULL      The tree is empty)
 *
 * \param[in]  *rbt  The tree from which we want to get the last node
 *
 * \par Notes:
 * 			1. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
RbtNode* rbt_last(Rbt *rbt)
{

	RbtNode *last = NULL;
	RbtNode *node;

	if (rbt != NULL)
	{
		node = rbt->root;

		while (node != NULL)
		{
			last = node;
			node = node->child[RIGHT];
		}
	}
	return last;
}

/******************************************************************************
 *
 * \par Function Name:
 *      traverseRbt
 *
 * \brief Get the next/prev node referring to the fromNode
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*
 *
 * \retval RbtNode*   The next/prev node found
 * \retval NULL       Next/prev node not found
 *
 * \param[in]  *fromNode  The node from which we want to start to search the next/prev node
 * \param[in]  direction  If 0 (LEFT) we search the prev node, otherwise we search the next node
 *
 * \warning fromNode doesn't have to be NULL.
 *
 * \par Notes:
 * 			1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
static RbtNode* traverseRbt(RbtNode *fromNode, int direction)
{
	int otherDirection = 1 - direction;
	RbtNode *nodePtr;
	RbtNode *next;

	nodePtr = fromNode;
	if (nodePtr->child[direction] == NULL)
	{
		/*	No next neighbor in this direction in the
		 *	subtree of which this node is root.  Time to
		 *	return to parent, popping up one level.		*/

		next = nodePtr->parent;
		while (next != NULL)
		{
			nodePtr = next;
			if (fromNode == nodePtr->child[otherDirection])
			{
				/*	If the previously traversed
				 *	node was the child on the
				 *	reverse side of the direction
				 *	of traversal, then the parent
				 *	is the next node.  The parent
				 *	is always the next node going
				 *	RIGHT after its left child's
				 *	subtree is exited (i.e.,
				 *	following the last node in
				 *	the left child's subtree),
				 *	and it's the next node going
				 *	LEFT after its right child's
				 *	subtree is exited (i.e.,
				 *	following the first node in
				 *	the right child's subtree).	*/

				break;
			}

			/*	If previous node was the node on the
			 *	side of the direction of traversal,
			 *	then there are no more un-traversed
			 *	nodes in this parent node's subtree.
			 *	So must exit this subtree, popping
			 *	up one more level.			*/

			fromNode = next;
			next = nodePtr->parent;
		}

		return next;
	}

	/*	Next neighbor going RIGHT is the leftmost node in
	 *	this node's right-hand subtree; next neighbor going
	 *	LEFT is the rightmost node in this node's left-hand
	 *	subtree.  First get root of that subtree, the node's
	 *	child in the direction of traversal.			*/

	next = nodePtr->child[direction];
	nodePtr = next;

	/*	Now get leftmost node in that subtree if going RIGHT,
	 *	rightmost node in that subtree if going LEFT.		*/

	while (nodePtr->child[otherDirection] != NULL)
	{
		next = nodePtr->child[otherDirection];
		nodePtr = next;
	}

	return next;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_traverse
 *
 * \brief Get the next/prev node referring to the fromNode
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return RbtNode*
 *
 * \retval RbtNode*  The next/prev node found
 * \retval NULL      Next/prev node not found
 *
 * \param[in]  *fromNode   The node from which we want to start to search the next/prev node
 * \param[in]  direction   If 0 (LEFT) we search the prev node, otherwise we search the next node
 *
 * \par Notes:
 * 			1. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
RbtNode* rbt_traverse(RbtNode *fromNode, int direction)
{
	RbtNode *nextNode = NULL;
	if (fromNode != NULL)
	{
		if (direction != LEFT)
		{
			direction = RIGHT;
		}

		nextNode = traverseRbt(fromNode, direction);
	}
	return nextNode;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_search
 *
 * \brief Get the node that has data equals to dataBuffer
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \retval RbtNode*  The node found, if any, otherwise NULL
 *
 * \param[in]   *rbtPtr      The node from which we want to search the node
 * \param[in]   *dataBuffer  The data that has to be equals to an RbtNode->data
 * \param[out]  **successor  At the end we have as result even the successor of the node found.
 *                           If we didn't find the node, anyway, we know who is the successor.
 *
 * \par Notes:
 * 			1. You must check if the return value of this function is not NULL.
 * 			2. The compareFn is used to find the equal node.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
RbtNode* rbt_search(Rbt *rbtPtr, void *dataBuffer, RbtNode **successor)
{

	RbtNode *node = NULL;
	RbtNode *prevNode = NULL;
	int direction = LEFT;
	int result;
	RbtCompareFn compare;

	if (rbtPtr != NULL)
	{
		compare = rbtPtr->compareFn;
		node = rbtPtr->root;
		prevNode = NULL;
		while (node != NULL)
		{
			result = compare(node->data, dataBuffer);
			if (result == 0) /*	Found the node.		*/
			{
				break;
			}

			prevNode = node;
			if (result < 0) /*	Haven't reached that node yet.	*/
			{
				direction = RIGHT;
			}
			else /*	Seeking an earlier node.	*/
			{
				direction = LEFT;
			}

			node = node->child[direction];
		}

		if (successor != NULL)
		{
			if (node == NULL) /*	Didn't find it; note position.	*/
			{
				if (direction == LEFT)
				{
					*successor = prevNode;
				}
				else
				{
					if (prevNode == NULL)
					{
						*successor = NULL;
					}
					else
					{
						*successor = traverseRbt(prevNode, RIGHT);
					}
				}
			}
			else /*	Successor is moot.		*/
			{
				*successor = NULL;
			}
		}

	}
	return node; /*	If NULL, didn't find matching node.	*/
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_rbt
 *
 * \brief Get the rbt to which the node belongs.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return Rbt*
 *
 * \retval Rbt*  The rbt to which the node belongs
 * \retval NULL  Rbt not found
 *
 * \param[in]  *node  The node from which we want to know to what rbt is belonging
 *
 * \par Notes:
 * 			1. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
Rbt* rbt_rbt(RbtNode *node)
{
	Rbt *rbt = NULL;
	if (node != NULL)
	{
		rbt = node->rbt;
	}
	return rbt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      rbt_data
 *
 * \brief Get the data pointed by the node
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void*
 *
 * \retval void*  The data pointed by the node (node->data)
 * \retval NULL   Data not found.
 *
 * \param[in]  *node  The node from which we want to get the data
 *
 * \par Notes:
 * 			1. You must check if the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void* rbt_data(RbtNode *node)
{
	void *data = NULL;
	if (node != NULL)
	{
		data = node->data;
	}
	return data;
}

#if	RBT_DEBUG
void	printTree(Rbt* rbt)
{
	RbtNode	*node;
	int		depth;
	int		descending;
	int		i;
	RbtNode*	prevNode;

	if(rbt != NULL){
	node = rbt->root;
	depth = 0;
	descending = 1;
	prevNode = NULL;
	while (node != NULL)
	{
		if (descending)
		{
			for (i = 0; i < depth; i++)
			{
				printf("\t");
			}

			printf("%lu (%c)\n", node->data,
					node->isRed ? 'R' : 'B');
			if (node->child[LEFT] != NULL)
			{
				depth++;
				node = node->child[LEFT];
				continue;
			}

			if (node->child[RIGHT])
			{
				depth++;
				node = node->child[RIGHT];
				continue;
			}

			/*	Time to pop up one level.		*/

			descending = 0;
			prevNode = node;
			depth--;
			node = node->parent;
			continue;
		}

		/*	Ascending in tree.				*/

		if (node->child[RIGHT] != NULL
		&& node->child[RIGHT] != prevNode)
		{
			/*	Descend other subtree.			*/

			descending = 1;
			depth++;
			node = node->child[RIGHT];
			continue;
		}

		/*	Pop up one more level.				*/

		prevNode = node;
		depth--;
		node = node->parent;
	}

	fflush(stdout);
	puts("Tree printed.");
	}
}

static int	subtreeBlackHeight(RbtNode* node)
{
	RbtNode	*childPtr[2];
	int		blackHeight[2];

	if (node == NULL)
	{
		return 1;	/*	Black pseudo-node.		*/
	}

	/*	Test for vertically consecutive red nodes.		*/

	if (node->child[LEFT])
	{
		childPtr[LEFT] = node->child[LEFT];
		if (node->isRed && child[LEFT]->isRed)
		{
			puts("Red violation (left).");
			return 0;
		}
	}

	if (nodePtr->child[RIGHT] != NULL)
	{
		childPtr[RIGHT] = node->child[RIGHT];
		if (node->isRed && child[RIGHT]->isRed)
		{
			puts("Red violation (right).");
			return 0;
		}
	}

	blackHeight[LEFT] = subtreeBlackHeight(node->child[LEFT]);
	blackHeight[RIGHT] = subtreeBlackHeight(nodePtr->child[RIGHT]);
	if (blackHeight[LEFT] == 0 || blackHeight[RIGHT] == 0)
	{
		puts("No black nodes.");
		return 0;
	}

	/*	Note: can't verify preservation of order in tree
	 *	because only the application can correctly extract
	 *	argData for presentation to the compare function.	*/

	if (blackHeight[LEFT] != blackHeight[RIGHT])
	{
		puts("Black violation.");
		return 0;
	}

	if (node->isRed == 0)	/*	This is a black node.	*/
	{
		blackHeight[0] += 1;
	}

	return blackHeight[0];
}

int	treeBroken(Rbt* rbt)
{
	int result = -1;

	if(rbt != NULL){
		result = (subtreeBlackHeight(rbt->root) == 0);
	}

	return result;
}
#endif

#if (LOG == 1)
/******************************************************************************
 *
 * \par Function Name:
 *      printTreeInOrder
 *
 * \brief Print the tree from the first element to the last element
 *
 *
 * \par Date Written:
 *      05/01/20
 *
 * \return int
 *
 * \retval  1   Success (tree printed)
 * \retval  0   Error in some parameter (rbt NULL, file NULL, print_function NULL)
 *
 * \param[in]  *rbt             The tree that we want to print
 * \param[in]  file             The file where we want the tree will be printed
 * \param[in]  print_function   The function to print the data of an RbtNode
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  05/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *******************************************************************************/
int printTreeInOrder(Rbt *rbt, FILE *file, print_tree_node print_function)
{
	int result = 0;
	RbtNode *elt;

	if (rbt != NULL && print_function != NULL && file != NULL)
	{
		result = 1;
		for (elt = rbt_first(rbt); elt != NULL; elt = rbt_traverse(elt, 1))
		{
			print_function(file, elt->data);
		}
	}

	return result;
}
#endif
