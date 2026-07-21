/*

	sdrlist.h:	definitions supporting use of SDR-based
			linked lists.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who	What
	06-05-07  SCB	Initial abstraction from original SDR API.

	Copyright (c) 2001-2007 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/

#ifndef SDRLIST_H
#define SDRLIST_H

#include "sdrmgt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on linked lists in SDR.			*/

typedef int (*SdrListCompareFn)(Sdr sdr, SdrAddress eltData, void *dataBuffer);
/*	Note: an SdrListCompareFn operates by comparing some value(s)
	derived from its first argument (which will always be the
	sdr_list_data of some SDR list element) to some value(s)
	derived from its second argument (which may be a pointer
	to an object residing in memory).				*/

typedef void (*SdrListDeleteFn)(Sdr sdr, SdrObject eltData, void *arg);

#define sdr_list_create(sdr) \
Sdr_list_create(__FILE__, __LINE__, sdr)
extern SdrObject Sdr_list_create(const char *file, int line, Sdr sdr);

#define sdr_list_destroy(sdr, list, deleteFn, argument) \
Sdr_list_destroy(__FILE__, __LINE__, sdr, list, deleteFn, argument)
extern void		Sdr_list_destroy(const char *file, int line,
				Sdr sdr, SdrObject list, SdrListDeleteFn deleteFn,
				void *argument);

extern SdrAddress		sdr_list_user_data(Sdr sdr, SdrObject list);

#define sdr_list_user_data_set(sdr, list, userData) \
Sdr_list_user_data_set(__FILE__, __LINE__, sdr, list, userData)
extern void		Sdr_list_user_data_set(const char *file, int line,
				Sdr sdr, SdrObject list, SdrAddress userData);

extern size_t		sdr_list_length(Sdr sdr, SdrObject list);

#define sdr_list_insert(sdr, list, data, compare, arg) \
Sdr_list_insert(__FILE__, __LINE__, sdr, list, data, compare, arg)
extern SdrObject	Sdr_list_insert(const char *file, int line,
				Sdr sdr, SdrObject list, SdrAddress data,
				SdrListCompareFn compare, void *dataBuffer);

#define sdr_list_insert_first(sdr, list, data) \
Sdr_list_insert_first(__FILE__, __LINE__, sdr, list, data)
extern SdrObject	Sdr_list_insert_first(const char *file, int line,
				Sdr sdr, SdrObject list, SdrAddress data);

#define sdr_list_insert_last(sdr, list, data) \
Sdr_list_insert_last(__FILE__, __LINE__, sdr, list, data)
extern SdrObject	Sdr_list_insert_last(const char *file, int line,
				Sdr sdr, SdrObject list, SdrAddress data);

#define sdr_list_insert_before(sdr, elt, data) \
Sdr_list_insert_before(__FILE__, __LINE__, sdr, elt, data)
extern SdrObject	Sdr_list_insert_before(const char *file, int line,
				Sdr sdr, SdrObject elt, SdrAddress data);

#define sdr_list_insert_after(sdr, elt, data) \
Sdr_list_insert_after(__FILE__, __LINE__, sdr, elt, data)
extern SdrObject	Sdr_list_insert_after(const char *file, int line,
				Sdr sdr, SdrObject elt, SdrAddress data);

#define sdr_list_delete(sdr, elt, deleteFn, argument) \
Sdr_list_delete(__FILE__, __LINE__, sdr, elt, deleteFn, argument)
extern void		Sdr_list_delete(const char *file, int line,
				Sdr sdr, SdrObject elt, SdrListDeleteFn deleteFn,
				void *argument);

extern SdrObject	sdr_list_list(Sdr sdr, SdrObject elt);
extern SdrObject	sdr_list_first(Sdr sdr, SdrObject list);
extern SdrObject	sdr_list_last(Sdr sdr, SdrObject list);
extern SdrObject	sdr_list_next(Sdr sdr, SdrObject elt);
extern SdrObject	sdr_list_prev(Sdr sdr, SdrObject elt);

extern SdrObject	sdr_list_search(Sdr sdr, SdrObject elt, int reverse,
				SdrListCompareFn compare, void *dataBuffer);

extern SdrAddress	sdr_list_data(Sdr sdr, SdrObject elt);

#define sdr_list_data_set(sdr, elt, data) \
Sdr_list_data_set(__FILE__, __LINE__, sdr, elt, data)
extern SdrAddress	Sdr_list_data_set(const char *file, int line,
				Sdr sdr, SdrObject elt, SdrAddress data);
#ifdef __cplusplus
}
#endif

#endif /* SDRLIST_H */
