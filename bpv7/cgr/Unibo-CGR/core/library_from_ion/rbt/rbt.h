/***************************************************************
 * \file rbt.h
 *
 * \brief In this file there are the declarations of the functions
 *        implemented in "rbt.c".
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

#ifndef RBT_H_
#define RBT_H_

#ifndef RBT_DEBUG
#define RBT_DEBUG	0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "rbt_type.h"
#include "../../library/commonDefines.h"

extern Rbt* rbt_create(RbtDeleteFn deleteFn, RbtCompareFn compareFn);
extern void rbt_clear(Rbt *rbt);
extern void rbt_destroy(Rbt *rbt);
extern void rbt_user_data_set(Rbt *rbt, void *userData);
extern RbtNode* rbt_insert(Rbt *rbt, void *data);
extern void rbt_delete(Rbt *rbt, void *dataBuffer);

extern RbtNode* rbt_search(Rbt *rbt, void *dataBuffer, RbtNode **successor);
extern RbtNode* rbt_first(Rbt *rbt);
extern RbtNode* rbt_last(Rbt *rbt);
#define rbt_prev(node) rbt_traverse(node, 0)
#define rbt_next(node) rbt_traverse(node, 1)
extern RbtNode* rbt_traverse(RbtNode *node, int direction);

extern Rbt* rbt_rbt(RbtNode *node);
extern void* rbt_data(RbtNode *node);
extern void* rbt_user_data(Rbt *rbt);
extern unsigned long int rbt_length(Rbt *rbt);

#if (LOG == 1)
typedef int (*print_tree_node)(FILE*, void*);
extern int printTreeInOrder(Rbt *rbt, FILE *file, print_tree_node print_function);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RBT_H_ */
