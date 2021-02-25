/***************************************************************
 * \file rbt_type.h
 *
 * \brief This file provides the definition of the Rbt and RbtNode type.
 *
 * \details There are some differences with the ION's SmRbt definition:
 *          1. I deleted "lock" field in Rbt type, because the utlity is
 *             only with shared memory and currently I don't need it.
 *          2. I add the delete function and the compare function
 *             directly in two fields of the Rbt type.
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

#ifndef LIBRARY_RBT_RBT_TYPE_H_
#define LIBRARY_RBT_RBT_TYPE_H_

typedef void (*RbtDeleteFn)(void*);
typedef int (*RbtCompareFn)(void*, void*);
typedef struct rbt Rbt;

typedef struct rbtNode
{
	/**
	 * \brief The Rbt to which this RbtNode belongs
	 */
	Rbt *rbt;
	/**
	 * \brief The RbtNode to which this RbtNode is a child
	 */
	struct rbtNode *parent;
	/**
	 * \brief The RbtNodes to which this RbtNode is the parent
	 */
	struct rbtNode *child[2];
	void *data;
	/**
	 * \brief Boolean: 1 if the node is red, 0 otherwise.
	 */
	int isRed;
} RbtNode;

struct rbt
{
	/**
	 * \brief The data to which this Rbt belongs (back-reference)
	 */
	void *userData;
	/**
	 * \brief The root of the Rbt
	 */
	RbtNode *root;
	/**
	 * \brief The number of the elements in this Rbt
	 */
	long unsigned int length;
	/**
	 * \brief The function called to delete the data field of the RbtNode
	 */
	RbtDeleteFn deleteFn;
	/**
	 * \brief The function called to compare the data field of the RbtNodes
	 */
	RbtCompareFn compareFn;
};

#endif /* LIBRARY_RBT_RBT_TYPE_H_ */
