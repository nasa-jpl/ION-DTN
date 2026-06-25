/*
	Public header file for routines that manage doubly-linked
	lists.  Derived from Jeff Biesiadecki's list (a.k.a. "lyst")
	library, but adapted to be usable in Solaris shared memory.
	All references to data items are expressed as offsets from
	the start of a shared memory partition managed by the PSM
	(Personal Space Management) system.  They can be converted to
	absolute memory pointers by the psp() function provided by PSM.

	Copyright (c) 2001, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#ifndef SMLIST_H
#define SMLIST_H

#include "psm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int		(*SmListCompareFn)(PsmPartition partition,
				PsmAddress eltData, void *dataBuffer);
/*	Note: an SmListCompareFn operates by comparing some value(s)
	derived from its first argument (which will always be the
	sm_list_data of some shared memory list element) to some
	value(s) derived from its second argument (which is typically
	a pointer to an object residing in memory).			*/

typedef void		(*SmListDeleteFn)(PsmPartition partition,
				PsmAddress elt, void *arg);

#define sm_list_create(partition) \
Sm_list_create(__FILE__, __LINE__, partition)
extern PsmAddress	Sm_list_create(const char *file, int line,
				PsmPartition partition);
#define sm_list_create_unlocked(partition) \
Sm_list_create_unlocked(__FILE__, __LINE__, partition)
extern PsmAddress	Sm_list_create_unlocked(const char *file, int line,
				PsmPartition partition);
extern void		sm_list_unwedge(PsmPartition Partition, PsmAddress list,
				int interval);
#define sm_list_clear(partition, list, fn, arg) \
Sm_list_clear(__FILE__, __LINE__, partition, list, fn, arg)
extern int		Sm_list_clear(const char *file, int line,
				PsmPartition partition, PsmAddress list,
				SmListDeleteFn deleteFn, void *argument);
#define sm_list_destroy(partition, list, fn, arg) \
Sm_list_destroy(__FILE__, __LINE__, partition, list, fn, arg)
extern int		Sm_list_destroy(const char *file, int line,
				PsmPartition partition, PsmAddress list,
				SmListDeleteFn deleteFn, void *argument);

extern PsmAddress	sm_list_user_data(PsmPartition partition,
				PsmAddress list);
extern int		sm_list_user_data_set(PsmPartition partition,
				PsmAddress list, PsmAddress userData);
extern size_t		sm_list_length(PsmPartition partition, PsmAddress list);

#define sm_list_insert(partition, list, data, fn, arg) \
Sm_list_insert(__FILE__, __LINE__, partition, list, data, fn, arg)
extern PsmAddress	Sm_list_insert(const char *file, int line,
				PsmPartition partition, PsmAddress list,
				PsmAddress data, SmListCompareFn compare,
				void *dataBuffer);
#define sm_list_insert_first(partition, list, data) \
Sm_list_insert_first(__FILE__, __LINE__, partition, list, data)
extern PsmAddress	Sm_list_insert_first(const char *file, int line,
				PsmPartition partition, PsmAddress list,
				PsmAddress data);
#define sm_list_insert_last(partition, list, data) \
Sm_list_insert_last(__FILE__, __LINE__, partition, list, data)
extern PsmAddress	Sm_list_insert_last(const char *file, int line,
				PsmPartition partition, PsmAddress list,
				PsmAddress data);

#define sm_list_insert_before(partition, elt, data) \
Sm_list_insert_before(__FILE__, __LINE__, partition, elt, data)
extern PsmAddress	Sm_list_insert_before(const char *file, int line,
				PsmPartition partition, PsmAddress elt,
				PsmAddress data);
#define sm_list_insert_after(partition, elt, data) \
Sm_list_insert_after(__FILE__, __LINE__, partition, elt, data)
extern PsmAddress	Sm_list_insert_after(const char *file, int line,
				PsmPartition partition, PsmAddress elt,
				PsmAddress data);

#define sm_list_delete(partition, elt, fn, arg) \
Sm_list_delete(__FILE__, __LINE__, partition, elt, fn, arg)
extern int		Sm_list_delete(const char *file, int line,
				PsmPartition partition, PsmAddress elt,
				SmListDeleteFn deleteFn, void *argument);

extern PsmAddress	sm_list_first(PsmPartition partition, PsmAddress list);
extern PsmAddress	sm_list_last(PsmPartition partition, PsmAddress list);
extern PsmAddress	sm_list_next(PsmPartition partition, PsmAddress elt);
extern PsmAddress	sm_list_prev(PsmPartition partition, PsmAddress elt);
extern PsmAddress	sm_list_search(PsmPartition partition, PsmAddress elt,
				SmListCompareFn compare, void *dataBuffer);

extern PsmAddress	sm_list_list(PsmPartition partition, PsmAddress elt);
extern PsmAddress	sm_list_data(PsmPartition partition, PsmAddress elt);
extern PsmAddress	sm_list_data_set(PsmPartition partition, PsmAddress elt,
				PsmAddress data);

/*	Returns 1 if the SmList header at 'list' is structurally sane
	(lock field is either SM_SEM_NONE or a valid sem index), 0
	otherwise.  Does not take the list's lock and does not assert.
	Use as a UAF / recycled-block guard for handles whose address
	round-trips through psp/psa but whose backing memory may have
	been Psm_freed and reused (#1048).				*/
extern int		sm_list_header_sane(PsmPartition partition,
				PsmAddress list);

#ifdef __cplusplus
}
#endif

#endif /* SMLIST_H */
