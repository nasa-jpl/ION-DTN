/*
 *	smrbt.c:	shared memory red-black tree management library.
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

#include "platform.h"
#include "smrbt.h"

/*		Private definitions of shared-memory rbt structures.	*/

#define	LEFT		0
#define	RIGHT		1

typedef struct
{
	PsmAddress	userData;
	PsmAddress	root;		/*	root node of the rbt	*/
	size_t		length;		/*	number of nodes in rbt	*/
	sm_SemId	lock;		/*	mutex for tree		*/
} SmRbt;

typedef struct
{
	PsmAddress	rbt;		/*	rbt that node is in	*/
	PsmAddress	parent;		/*	parent node in tree	*/
	PsmAddress	child[2];	/*	child nodes in tree	*/
	PsmAddress	data;		/*	data for this node	*/
	int		isRed;		/*	Boolean; if 0, is Black	*/
} SmRbtNode;

/*	*	*	Rbt management functions	*	*	*/

static void	eraseTree(SmRbt *rbt)
{
	rbt->userData = 0;
	rbt->root = 0;
	rbt->length = 0;
	rbt->lock = 0;
}

static void	eraseTreeNode(SmRbtNode *node)
{
	node->rbt = 0;
	node->parent = 0;
	node->child[LEFT] = 0;
	node->child[RIGHT] = 0;
	node->data = 0;
	node->isRed = 0;
}

static int	nodeIsRed(PsmPartition partition, PsmAddress node)
{
	SmRbtNode	*nodePtr;

	if (node == 0)
	{
		return 0;
	}

	nodePtr = (SmRbtNode *) psp(partition, node);
	return nodePtr->isRed;
}

static int	lockSmrbt(SmRbt *rbt)
{
	int	result;

	result = sm_SemTake(rbt->lock);
	if (result < 0)
	{
		putErrmsg("Can't lock red-black table.", NULL);
	}

	return result;
}

static void	unlockSmrbt(SmRbt *rbt)
{
	sm_SemGive(rbt->lock);
}

PsmAddress	Sm_rbt_create(const char *file, int line,
			PsmPartition partition)
{
	sm_SemId	lock;
	PsmAddress	rbt;
	SmRbt		*rbtPtr;

	lock = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (lock < 0)
	{
		putErrmsg("Can't create semaphore for rbt.", NULL);
		return 0;
	}

	rbt = Psm_zalloc(file, line, partition, sizeof(SmRbt));
	if (rbt == 0)
	{
		sm_SemDelete(lock);
		putErrmsg("Can't allocate space for rbt object.", NULL);
		return 0;
	}

	rbtPtr = (SmRbt *) psp(partition, rbt);
	eraseTree(rbtPtr);
	rbtPtr->lock = lock;
	return rbt;
}

void	sm_rbt_unwedge(PsmPartition partition, PsmAddress rbt, int interval)
{
	SmRbt	*rbtPtr;

	CHKVOID(partition);
	CHKVOID(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKVOID(rbtPtr);
	sm_SemUnwedge(rbtPtr->lock, interval);
}

static void	destroyRbtNodes(const char *file, int line,
			PsmPartition partition, SmRbt *rbtPtr,
			SmRbtDeleteFn deleteFn, void *arg)
{
	PsmAddress	node;
	SmRbtNode	*nodePtr;
	PsmAddress	parentNode;

	node = rbtPtr->root;
	nodePtr = (SmRbtNode *) psp(partition, node);

	/*	Destroy all nodes of the tree.				*/

	while (node)
	{
		/*	If node has a left subtree, descend into it.	*/

		if (nodePtr->child[LEFT])
		{
			node = nodePtr->child[LEFT];
			nodePtr = (SmRbtNode *) psp(partition, node);
			continue;
		}

		/*	If node has a right subtree, descend into it.	*/

		if (nodePtr->child[RIGHT])
		{
			node = nodePtr->child[RIGHT];
			nodePtr = (SmRbtNode *) psp(partition, node);
			continue;
		}

		/*	Node is a leaf, so can delete it.		*/

		parentNode = nodePtr->parent;
		if (deleteFn)
		{
			deleteFn(partition, nodePtr->data, arg);
		}

		/*	just in case user mistakenly accesses later...	*/
		eraseTreeNode(nodePtr);
		Psm_free(file, line, partition, node);

		/*	Now pop back up to this node's parent.		*/

		if (parentNode == 0)
		{
			rbtPtr->root = 0;
			break;	/*	Have deleted the root node.	*/
		}

		nodePtr = (SmRbtNode *) psp(partition, parentNode);

		/*	Erase the child pointer for the child that
		 *	has now been deleted.				*/

		if (node == nodePtr->child[LEFT])
		{
			nodePtr->child[LEFT] = 0;
		}

		if (node == nodePtr->child[RIGHT])
		{
			nodePtr->child[RIGHT] = 0;
		}

		node = parentNode;
	}
}

void	Sm_rbt_clear(const char *file, int line, PsmPartition partition,
		PsmAddress rbt, SmRbtDeleteFn deleteFn, void *arg)
{
	SmRbt	*rbtPtr;

	CHKVOID(partition);
	CHKVOID(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	oK(lockSmrbt(rbtPtr));
	destroyRbtNodes(file, line, partition, rbtPtr, deleteFn, arg);
	unlockSmrbt(rbtPtr);
}

void	Sm_rbt_destroy(const char *file, int line, PsmPartition partition,
		PsmAddress rbt, SmRbtDeleteFn deleteFn, void *arg)
{
	SmRbt	*rbtPtr;

	CHKVOID(partition);
	CHKVOID(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	oK(lockSmrbt(rbtPtr));
	destroyRbtNodes(file, line, partition, rbtPtr, deleteFn, arg);

	/*	Now destroy the tree itself.				*/

	sm_SemEnd(rbtPtr->lock);
	microsnooze(50000);
	sm_SemDelete(rbtPtr->lock);

	/*	just in case user mistakenly accesses later...		*/
	eraseTree(rbtPtr);
	Psm_free(file, line, partition, rbt);
}

PsmAddress	sm_rbt_user_data(PsmPartition partition, PsmAddress rbt)
{
	SmRbt		*rbtPtr;
	PsmAddress	userData;

	CHKZERO(partition);
	CHKZERO(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKZERO(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return 0;
	}

	userData = rbtPtr->userData;
	unlockSmrbt(rbtPtr);
	return userData;
}

void	sm_rbt_user_data_set(PsmPartition partition, PsmAddress rbt,
		PsmAddress data)
{
	SmRbt	*rbtPtr;

	CHKVOID(partition);
	CHKVOID(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKVOID(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return;
	}

	rbtPtr->userData = data;
	unlockSmrbt(rbtPtr);
}

size_t	sm_rbt_length(PsmPartition partition, PsmAddress rbt)
{
	SmRbt	*rbtPtr;
	size_t	length;

	CHKERR(partition);
	CHKERR(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKERR(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return ERROR;
	}

	length = rbtPtr->length;
	unlockSmrbt(rbtPtr);
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

static PsmAddress	rotateOnce(PsmPartition partition, PsmAddress root,
				int direction)
{
	int		otherDirection = 1 - direction;
	SmRbtNode	*rootPtr;
	PsmAddress	pivot;
	SmRbtNode	*pivotPtr;
	PsmAddress	orphan;
	SmRbtNode	*orphanPtr;

	rootPtr = (SmRbtNode *) psp(partition, root);
	pivot = rootPtr->child[otherDirection];
	pivotPtr = (SmRbtNode *) psp(partition, pivot);
	orphan = pivotPtr->child[direction];
	orphanPtr = (SmRbtNode *) psp(partition, orphan);
	pivotPtr->parent = rootPtr->parent;
	pivotPtr->child[direction] = root;
	pivotPtr->isRed = 0;
	rootPtr->parent = pivot;
	rootPtr->child[otherDirection] = orphan;
	rootPtr->isRed = 1;
	if (orphan)
	{
		orphanPtr->parent = root;
	}

	return pivot;
}

static PsmAddress	rotateTwice(PsmPartition partition, PsmAddress root,
				int direction)
{
	int		otherDirection = 1 - direction;
	SmRbtNode	*rootPtr;

	rootPtr = (SmRbtNode *) psp(partition, root);
	rootPtr->child[otherDirection] =
			rotateOnce(partition, rootPtr->child[otherDirection],
			otherDirection);
	return rotateOnce(partition, root, direction);
}

static PsmAddress	createNode(const char *file, int line,
				PsmPartition partition, PsmAddress rbt,
				PsmAddress parent, PsmAddress data,
				SmRbtNode **buffer)
{
	PsmAddress	node;
	SmRbtNode	*nodePtr;

	node = Psm_zalloc(file, line, partition, sizeof(SmRbtNode));
	if (node == 0)
	{
		putErrmsg("Can't allocate space for rbt node.", NULL);
		return 0;
	}

	nodePtr = (SmRbtNode *) psp(partition, node);
	nodePtr->rbt = rbt;
	nodePtr->parent = parent;
	nodePtr->child[LEFT] = 0;
	nodePtr->child[RIGHT] = 0;
	nodePtr->data = data;
	if (buffer)	/*	Tree already has a root.		*/
	{
		nodePtr->isRed = 1;
		*buffer = nodePtr;
	}
	else		/*	This is the first node in the tree.	*/
	{
		nodePtr->isRed = 0;	/*	Root is always black.	*/
	}

	return node;
}

PsmAddress	Sm_rbt_insert(const char *file, int line,
			PsmPartition partition, PsmAddress rbt, PsmAddress data,
			SmRbtCompareFn compare, void *dataBuffer)
{
	SmRbtNode	dummyRootBuffer = { 0, 0, { 0, 0}, 0, 0 };
				/*	Dummy root is a trick; it
				 *	lets us avoid constantly
				 *	rebalancing at actual root.	*/
	SmRbt		*rbtPtr;
	SmRbtNode	*childPtr[2];
	PsmAddress	node;			//	q
	SmRbtNode	*nodePtr;		//	q
	SmRbtNode	*greatgrandparentPtr;	//	t
	PsmAddress	grandparent;		//	g
	SmRbtNode	*grandparentPtr;	//	g
	PsmAddress	parent;			//	p
	SmRbtNode	*parentPtr;		//	p
	int		direction = LEFT;	//	dir
	int		prevDirection = LEFT;	//	last
	int		subtree;		//	dir2

	CHKZERO(partition);
	CHKZERO(rbt);
	CHKZERO(compare);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKZERO(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return 0;
	}

	if (rbtPtr->root == 0)
	{
		node = createNode(file, line, partition, rbt, 0, data, NULL);
		if (node)
		{
			rbtPtr->root = node;
			rbtPtr->length = 1;
		}

		unlockSmrbt(rbtPtr);
		return node;
	}

	/*	Initialize for top-down insertion pass.  Tree is
	 *	known to be non-empty, so every newly inserted node
	 *	will be a leaf with a non-zero parent (at least at
	 *	first).							*/

	greatgrandparentPtr = &dummyRootBuffer;
	greatgrandparentPtr->child[RIGHT] = rbtPtr->root;
	grandparent = 0;			/*	None.		*/
	grandparentPtr = NULL;
	parent = 0;				/*	None.		*/
	parentPtr = NULL;
	node = rbtPtr->root;
	while (1)
	{
		if (node == 0)	/*	(Can't be tree's root node.)	*/
		{
			/*	Have found the position at which this
			 *	node should be inserted.		*/

			node = createNode(file, line, partition, rbt, parent,
					data, &nodePtr);
			if (node == 0)
			{
				unlockSmrbt(rbtPtr);
				return 0;
			}

			/*	Update parent's child pointer.  Note
			 *	that parent must already exist and be
			 *	known, because this is not the root
			 *	node.					*/

			parentPtr->child[direction] = node;
		}
		else		/*	(Might be the root node.)	*/
		{
			nodePtr = (SmRbtNode *) psp(partition, node);

			/*	Check for need to flip colors.		*/

			if (nodeIsRed(partition, nodePtr->child[LEFT])
			&& nodeIsRed(partition, nodePtr->child[RIGHT]))
			{
				/*	The two children are RED, so
				 *	make current node RED and
				 *	children BLACK.			*/

				nodePtr->isRed = 1;
				childPtr[LEFT] = (SmRbtNode *)
						psp(partition,
						nodePtr->child[LEFT]);
				childPtr[LEFT]->isRed = 0;
				childPtr[RIGHT] = (SmRbtNode *)
						psp(partition,
						nodePtr->child[RIGHT]);
				childPtr[RIGHT]->isRed = 0;
			}
		}

		/*	Now look for a newly introduced red violation;
		 *	if present, fix it.  Note that there can't be
		 *	a violation if this is the root node, because
		 *	the parent is the dummy root, which is black.	*/

		if (nodeIsRed(partition, parent) && nodeIsRed(partition, node))
		{
			if (greatgrandparentPtr->child[RIGHT] == grandparent)
			{
				subtree = RIGHT;
			}
			else
			{
				subtree = LEFT;
			}

			if (node == parentPtr->child[prevDirection])
			{
				greatgrandparentPtr->child[subtree]
					= rotateOnce(partition, grandparent,
						1 - prevDirection);
			}
			else	/*	Child in current direction.	*/
			{
				greatgrandparentPtr->child[subtree]
					= rotateTwice(partition, grandparent,
						1 - prevDirection);
			}
		}

		/*	Stop if this data item is in the tree, either
		 *	from earlier insertion or from this one.	*/

		if (nodePtr->data == data)
		{
			break;
		}

		/*	Decide which branch to take next.		*/

		prevDirection = direction;
		if (compare(partition, nodePtr->data, dataBuffer) < 0)
		{
			direction = RIGHT;
		}
		else
		{
			direction = LEFT;
		}

		/*	Descend one level in the tree and try again.	*/

		if (grandparent)
		{
			greatgrandparentPtr = grandparentPtr;
		}

		grandparent = parent;
		grandparentPtr = parentPtr;
		parent = node;
		parentPtr = nodePtr;
		node = nodePtr->child[direction];
	}

	rbtPtr->length += 1;

	/*	Record the new root node object.			*/

	rbtPtr->root = dummyRootBuffer.child[RIGHT];

	/*	Make sure the root node is black.			*/

	nodePtr = (SmRbtNode *) psp(partition, rbtPtr->root);
	nodePtr->isRed = 0;
	unlockSmrbt(rbtPtr);
	return node;
}

void	Sm_rbt_delete(const char *file, int line, PsmPartition partition,
		PsmAddress rbt, SmRbtCompareFn compare, void *dataBuffer,
		SmRbtDeleteFn deleteFn, void *arg)
{
	SmRbtNode	dummyRootBuffer = { 0, 0, {0, 0}, 0, 0 };
				/*	Dummy root is a trick; it
				 *	lets us avoid special cases.	*/
	SmRbt		*rbtPtr;
	PsmAddress	node;			//	q
	SmRbtNode	*nodePtr;		//	q
	PsmAddress	sibling;		//	s
	SmRbtNode	*siblingPtr;		//	s
	PsmAddress	parent;			//	p
	SmRbtNode	*parentPtr;		//	p
	PsmAddress	stepparent;		//	p
	SmRbtNode	*stepparentPtr;		//	p
	SmRbtNode	*grandparentPtr;	//	g
	PsmAddress	target;			//	f
	SmRbtNode	*targetPtr;		//	f
	int		direction;		//	dir
	int		otherDirection;		//	!dir
	int		prevDirection;		//	last
	int		prevOtherDirection;	//	!last
	int		subtree;		//	dir2
	SmRbtNode	*childPtr[2];
	int		result;
	int		i;
	int		j;

	CHKVOID(partition);
	CHKVOID(rbt);
	CHKVOID(compare);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKVOID(rbtPtr);
	if (rbtPtr->length == 0 || rbtPtr->root == 0
	|| lockSmrbt(rbtPtr) == ERROR)
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
	parent = 0;				/*	None.		*/
	parentPtr = NULL;
	node = 0;
	nodePtr = &dummyRootBuffer;
	nodePtr->child[RIGHT] = rbtPtr->root;
	target = 0;				/*	f		*/
	direction = RIGHT;
	while (nodePtr->child[direction])
	{
		prevDirection = direction;
		grandparentPtr = parentPtr;

		/*	The first time through the loop, parentPtr
		 *	is NULL so grandparentPtr is still NULL.  The
		 *	second time through the loop, nodePtr is a
		 *	pointer to the dummy root, so the dummy root
		 *	becomes the grandparent.  The third time, a
		 *	pointer to the tree's root (a real node) is
		 *	placed in grandparentPtr.			*/

		parent = node;
		parentPtr = nodePtr;

		/*	The first time through the loop, nodePtr is
		 *	a pointer to the dummy root, so the dummy root
		 *	becomes the parent.  The second time, a pointer
		 *	to the tree's root (a real node) is placed in
		 *	parentPtr.					*/

		node = nodePtr->child[direction];
		nodePtr = (SmRbtNode *) psp(partition, node);

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

		result = compare(partition, nodePtr->data, dataBuffer);
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
 
 		if (nodeIsRed(partition, node)
		|| nodeIsRed(partition, nodePtr->child[direction]))
		{
			continue;	/*	No recolor needed.	*/
		}

		/*	Neither the node nor its child (if any) in
		 *	the current direction of search are red, so
		 *	we must introduce some redness.			*/

		otherDirection = 1 - direction;
		if (nodeIsRed(partition, nodePtr->child[otherDirection]))
		{
			/*	This is the "red sibling" case.  Rotate
			 *	current node down/red.			*/

			childPtr[otherDirection] = (SmRbtNode *)
				psp(partition, nodePtr->child[otherDirection]);
			parentPtr->child[prevDirection] =
				rotateOnce(partition, node, direction);

			/*	The new root of the subtree whose root
			 *	before rotation was "node" [the red
			 *	sibling] must be noted as "parent" in
			 *	place of the original parent of "node",
			 *	because it is now the actual parent
			 *	of "node".				*/

			parent = parentPtr->child[prevDirection];
			parentPtr = (SmRbtNode *) psp(partition, parent);

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
		sibling = parentPtr->child[prevOtherDirection];
		if (sibling == 0)	/*	(True of tree root.)	*/
		{
			continue;	/*	No sibling, no problem.	*/
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

		siblingPtr = (SmRbtNode *) psp(partition, sibling);

		/*	Check for need to flip colors.  NOT TREE ROOT.	*/

		if (nodeIsRed(partition, siblingPtr->child[LEFT]) == 0
		&& nodeIsRed(partition, siblingPtr->child[RIGHT]) == 0)
		{
			/*	Sibling has no red children, so it's
			 *	safe to make both the current node and
			 *	its sibling RED and their parent BLACK.	*/

			parentPtr->isRed = 0;
			siblingPtr->isRed = 1;
			nodePtr->isRed = 1;
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

		if (nodeIsRed(partition, siblingPtr->child[prevDirection]))
		{
			grandparentPtr->child[subtree] =
				rotateTwice(partition, parent, prevDirection);
		}
		else
		{
			if (nodeIsRed(partition,
					siblingPtr->child[prevOtherDirection]))
			{
				grandparentPtr->child[subtree] =
				rotateOnce(partition, parent, prevDirection);
			}
		}

		/*	The subtree may now have a new parent, due
		 *	to rotation.  Ensure nodes have correct colors.	*/

		stepparent = grandparentPtr->child[subtree];
		stepparentPtr = (SmRbtNode *) psp(partition, stepparent);
		stepparentPtr->isRed = 1;
		nodePtr->isRed = 1;
		childPtr[LEFT] = (SmRbtNode *) psp(partition,
				stepparentPtr->child[LEFT]);
		childPtr[LEFT]->isRed = 0;
		childPtr[RIGHT] = (SmRbtNode *) psp(partition,
				stepparentPtr->child[RIGHT]);
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

	if (target)
	{
		targetPtr = (SmRbtNode *) psp(partition, target);
		if (deleteFn)
		{
			deleteFn(partition, targetPtr->data, arg);
		}

		targetPtr->data = nodePtr->data;

		/*	To destroy inorder predecessor node, must
		 *	make its parent forget about it.		*/

		if (parentPtr->child[RIGHT] == node)
		{
			i = RIGHT;
		}
		else
		{
			i = LEFT;
		}

		if (nodePtr->child[LEFT] == 0)
		{
			j = RIGHT;
		}
		else
		{
			j = LEFT;
		}

		parentPtr->child[i] = nodePtr->child[j];

		/*	just in case user mistakenly accesses later...	*/
		eraseTreeNode(nodePtr);
		Psm_free(file, line, partition, node);
		rbtPtr->length -= 1;
	}

	/*	Update root in rbt object.				*/
 
	rbtPtr->root = dummyRootBuffer.child[RIGHT];

	/*	Make sure root is BLACK.				*/

	if (rbtPtr->root)
	{
		nodePtr = (SmRbtNode *) psp(partition, rbtPtr->root);
		nodePtr->isRed = 0;
	}

	unlockSmrbt(rbtPtr);
}

PsmAddress	sm_rbt_first(PsmPartition partition, PsmAddress rbt)
{
	SmRbt		*rbtPtr;
	PsmAddress	node;
	PsmAddress	first = 0;
	SmRbtNode	*nodePtr;

	CHKZERO(partition);
	CHKZERO(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKZERO(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return 0;
	}

	node = rbtPtr->root;
	while (node)
	{
		first = node;
		nodePtr = (SmRbtNode *) psp(partition, node);
		if (nodePtr == NULL)
		{
			putErrmsg("Corrupt red-black tree.", NULL);
			first = 0;
			break;
		}

		node = nodePtr->child[LEFT];
	}

	unlockSmrbt(rbtPtr);
	return first;
}

PsmAddress	sm_rbt_last(PsmPartition partition, PsmAddress rbt)
{
	SmRbt		*rbtPtr;
	PsmAddress	node;
	PsmAddress	last = 0;
	SmRbtNode	*nodePtr;

	CHKZERO(partition);
	CHKZERO(rbt);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKZERO(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return 0;
	}

	node = rbtPtr->root;
	while (node)
	{
		last = node;
		nodePtr = (SmRbtNode *) psp(partition, node);
		if (nodePtr == NULL)
		{
			putErrmsg("Corrupt red-black tree.", NULL);
			last = 0;
			break;
		}

		node = nodePtr->child[RIGHT];
	}

	unlockSmrbt(rbtPtr);
	return last;
}

static PsmAddress	traverseRbt(PsmPartition partition,
				PsmAddress fromNode, int direction)
{
	int		otherDirection = 1 - direction;
	SmRbtNode	*nodePtr;
	PsmAddress	next;

	nodePtr = (SmRbtNode *) psp(partition, fromNode);
	if (nodePtr->child[direction] == 0)
	{
		/*	No next neighbor in this direction in the
		 *	subtree of which this node is root.  Time to
		 *	return to parent, popping up one level.		*/

		next = nodePtr->parent;
		while (next)
		{
			nodePtr = (SmRbtNode *) psp(partition, next);
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
	nodePtr = (SmRbtNode *) psp(partition, next);

	/*	Now get leftmost node in that subtree if going RIGHT,
	 *	rightmost node in that subtree if going LEFT.		*/

	while (nodePtr->child[otherDirection] != 0)
	{
		next = nodePtr->child[otherDirection];
		nodePtr = (SmRbtNode *) psp(partition, next);
	}

	return next;
}

PsmAddress	Sm_rbt_traverse(PsmPartition partition, PsmAddress fromNode,
			int direction)
{
	SmRbtNode	*nodePtr;
	SmRbt		*rbtPtr;
	int		nextNode;

	CHKZERO(partition);
	CHKZERO(fromNode);
	nodePtr = (SmRbtNode *) psp(partition, fromNode);
	CHKZERO(nodePtr);
	rbtPtr = (SmRbt *) psp(partition, nodePtr->rbt);
	CHKZERO(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return 0;
	}

	if (direction != LEFT)
	{
		direction = RIGHT;
	}

	nextNode = traverseRbt(partition, fromNode, direction);
	unlockSmrbt(rbtPtr);
	return nextNode;
}

PsmAddress	sm_rbt_search(PsmPartition partition, PsmAddress rbt,
			SmRbtCompareFn compare, void *dataBuffer,
			PsmAddress *successor)
{
	SmRbt		*rbtPtr;
	PsmAddress	node;
	PsmAddress	prevNode;
	int		direction = LEFT;
	SmRbtNode	*nodePtr;
	int		result;

	CHKZERO(partition);
	CHKZERO(rbt);
	CHKZERO(compare);
	rbtPtr = (SmRbt *) psp(partition, rbt);
	CHKZERO(rbtPtr);
	if (lockSmrbt(rbtPtr) == ERROR)
	{
		return 0;
	}

	node = rbtPtr->root;
	prevNode = 0;
	while (node)
	{
		nodePtr = (SmRbtNode *) psp(partition, node);
		result = compare(partition, nodePtr->data, dataBuffer);
		if (result == 0)	/*	Found the node.		*/
		{
			break;
		}

		prevNode = node;
		if (result < 0)	/*	Haven't reached that node yet.	*/
		{
			direction = RIGHT;
		}
		else		/*	Seeking an earlier node.	*/
		{
			direction = LEFT;
		}

		node = nodePtr->child[direction];
	}

	if (successor)
	{
		if (node == 0)	/*	Didn't find it; note position.	*/
		{
			if (direction == LEFT)
			{
				*successor = prevNode;
			}
			else
			{
				if (prevNode == 0)
				{
					*successor = 0;
				}
				else
				{
					*successor = traverseRbt(partition,
							prevNode, RIGHT);
				}
			}
		}
		else		/*	Successor is moot.		*/
		{
			*successor = 0;
		}
	}

	unlockSmrbt(rbtPtr);
	return node;	/*	If zero, didn't find matching node.	*/
}

PsmAddress	sm_rbt_rbt(PsmPartition partition, PsmAddress node)
{
	SmRbtNode	*nodePtr;

	CHKZERO(partition);
	CHKZERO(node);
	nodePtr = (SmRbtNode *) psp(partition, node);
	CHKZERO(nodePtr);
	return nodePtr->rbt;
}

PsmAddress	sm_rbt_data(PsmPartition partition, PsmAddress node)
{
	SmRbtNode	*nodePtr;

	CHKZERO(partition);
	CHKZERO(node);
	nodePtr = (SmRbtNode *) psp(partition, node);
	CHKZERO(nodePtr);
	return nodePtr->data;
}

#if	SMRBT_DEBUG
void	printTree(PsmPartition partition, PsmAddress rbt)
{
	SmRbt		*rbtPtr;
	PsmAddress	node;
	SmRbtNode	*nodePtr;
	int		depth;
	int		descending;
	int		i;
	PsmAddress	prevNode;

	rbtPtr = (SmRbt *) psp(partition, rbt);
	node = rbtPtr->root;
	depth = 0;
	descending = 1;
	prevNode = 0;
	while (node)
	{
		nodePtr = (SmRbtNode *) psp(partition, node);
		if (descending)
		{
			for (i = 0; i < depth; i++)
			{
				printf("\t");
			}

			printf("%lu (%c)\n", nodePtr->data,
					nodePtr->isRed ? 'R' : 'B');
			if (nodePtr->child[LEFT])
			{
				depth++;
				node = nodePtr->child[LEFT];
				continue;
			}

			if (nodePtr->child[RIGHT])
			{
				depth++;
				node = nodePtr->child[RIGHT];
				continue;
			}

			/*	Time to pop up one level.		*/

			descending = 0;
			prevNode = node;
			depth--;
			node = nodePtr->parent;
			continue;
		}

		/*	Ascending in tree.				*/

		if (nodePtr->child[RIGHT]
		&& nodePtr->child[RIGHT] != prevNode)
		{
			/*	Descend other subtree.			*/

			descending = 1;
			depth++;
			node = nodePtr->child[RIGHT];
			continue;
		}

		/*	Pop up one more level.				*/

		prevNode = node;
		depth--;
		node = nodePtr->parent;
	}

	fflush(stdout);
	puts("Tree printed.");
}

static int	subtreeBlackHeight(PsmPartition partition, PsmAddress node)
{
	SmRbtNode	*nodePtr;
	SmRbtNode	*childPtr[2];
	int		blackHeight[2];

	if (node == 0)
	{
		return 1;	/*	Black pseudo-node.		*/
	}

	nodePtr = (SmRbtNode *) psp(partition, node);

	/*	Test for vertically consecutive red nodes.		*/

	if (nodePtr->child[LEFT])
	{
		childPtr[LEFT] = (SmRbtNode *) psp(partition,
				nodePtr->child[LEFT]);
		if (nodePtr->isRed && childPtr[LEFT]->isRed)
		{
			puts("Red violation (left).");
			return 0;
		}
	}

	if (nodePtr->child[RIGHT])
	{
		childPtr[RIGHT] = (SmRbtNode *) psp(partition,
				nodePtr->child[RIGHT]);
		if (nodePtr->isRed && childPtr[RIGHT]->isRed)
		{
			puts("Red violation (right).");
			return 0;
		}
	}

	blackHeight[LEFT] = subtreeBlackHeight(partition,
			nodePtr->child[LEFT]);
	blackHeight[RIGHT] = subtreeBlackHeight(partition,
			nodePtr->child[RIGHT]);
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

	if (nodePtr->isRed == 0)	/*	This is a black node.	*/
	{
		blackHeight[0] += 1;
	}

	return blackHeight[0];
}

int	treeBroken(PsmPartition partition, PsmAddress rbt)
{
	SmRbt	*rbtPtr;

	rbtPtr = (SmRbt *) psp(partition, rbt);
	return (subtreeBlackHeight(partition, rbtPtr->root) == 0);
}
#endif
