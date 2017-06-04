/*

	smrbt.h:	definitions supporting use of red-black trees
			in shared memory.

	Author: Scott Burleigh, JPL
	
	Modification History:
	Date      Who	What
	11-17-11  SCB	Adapted from Julienne Walker tutorial, public
			domain code.
	(http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx)

	Copyright (c) 2011 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#ifndef _SMRBT_H_
#define _SMRBT_H_

#include "psm.h"

#ifndef SMRBT_DEBUG
#define SMRBT_DEBUG	0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on red-black trees in shared memory.	*/

typedef int		(*SmRbtCompareFn)(PsmPartition partition,
				PsmAddress nodeData, void *dataBuffer);
/*	Note: an SmRbtCompareFn operates by comparing some value(s)
	derived from its first argument (which will always be the
	sm_rbt_data of some shared memory rbt node) to some value(s)
	derived from its second argument (which is typically a pointer
	to an object residing in memory).				*/

typedef void		(*SmRbtDeleteFn)(PsmPartition partition,
				PsmAddress nodeData, void *arg);

#define sm_rbt_create(partition) \
Sm_rbt_create(__FILE__, __LINE__, partition)
extern PsmAddress	Sm_rbt_create(const char *file, int line,
				PsmPartition partition);
extern void		sm_rbt_unwedge(PsmPartition partition, PsmAddress rbt,
				int interval);

#define sm_rbt_clear(partition, rbt, deleteFn, argument) \
Sm_rbt_clear(__FILE__, __LINE__, partition, rbt, deleteFn, argument)
extern void		Sm_rbt_clear(const char *file, int line,
				PsmPartition partition, PsmAddress rbt,
				SmRbtDeleteFn deleteFn, void *argument);

#define sm_rbt_destroy(partition, rbt, deleteFn, argument) \
Sm_rbt_destroy(__FILE__, __LINE__, partition, rbt, deleteFn, argument)
extern void		Sm_rbt_destroy(const char *file, int line,
				PsmPartition partition, PsmAddress rbt,
				SmRbtDeleteFn deleteFn, void *argument);

extern PsmAddress	sm_rbt_user_data(PsmPartition partition,
				PsmAddress rbt);
extern void		sm_rbt_user_data_set( PsmPartition partition,
				PsmAddress rbt, PsmAddress userData);
extern size_t		sm_rbt_length(PsmPartition partition, PsmAddress rbt);

#define sm_rbt_insert(partition, rbt, data, compare, dataBuffer) \
Sm_rbt_insert(__FILE__, __LINE__, partition, rbt, data, compare, dataBuffer)
extern PsmAddress	Sm_rbt_insert(const char *file, int line,
				PsmPartition partition, PsmAddress rbt,
				PsmAddress data, SmRbtCompareFn compare,
				void *dataBuffer);

#define sm_rbt_delete(partition, rbt, compare, dataBuffer, deleteFn, \
argument) Sm_rbt_delete(__FILE__, __LINE__, partition, rbt, compare, \
dataBuffer, deleteFn, argument)
extern void		Sm_rbt_delete(const char *file, int line,
				PsmPartition partition, PsmAddress rbt,
				SmRbtCompareFn compare, void *dataBuffer,
				SmRbtDeleteFn deleteFn, void *argument);

extern PsmAddress	sm_rbt_search(PsmPartition partition, PsmAddress rbt,
				SmRbtCompareFn compare, void *dataBuffer,
				PsmAddress *successor);

extern PsmAddress	sm_rbt_first(PsmPartition partition, PsmAddress rbt);
extern PsmAddress	sm_rbt_last(PsmPartition partition, PsmAddress rbt);
#define sm_rbt_prev(partition, node) Sm_rbt_traverse(partition, node, 0)
#define sm_rbt_next(partition, node) Sm_rbt_traverse(partition, node, 1)
extern PsmAddress	Sm_rbt_traverse(PsmPartition partition,
				PsmAddress node, int direction);

extern PsmAddress	sm_rbt_rbt(PsmPartition partition, PsmAddress node);
extern PsmAddress	sm_rbt_data(PsmPartition partition, PsmAddress node);
#ifdef __cplusplus
}
#endif

#endif  /* _SMRBT_H_ */
