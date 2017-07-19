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
#ifndef _SDRLIST_H_
#define _SDRLIST_H_

#include "sdrmgt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on linked lists in SDR.			*/

typedef int		(*SdrListCompareFn)(Sdr sdr, Address eltData,
				void *dataBuffer);
/*	Note: an SdrListCompareFn operates by comparing some value(s)
	derived from its first argument (which will always be the
	sdr_list_data of some SDR list element) to some value(s)
	derived from its second argument (which may be a pointer
	to an object residing in memory).				*/

typedef void		(*SdrListDeleteFn)(Sdr sdr, Object eltData, void *arg);

#define sdr_list_create(sdr) \
Sdr_list_create(__FILE__, __LINE__, sdr)
extern Object		Sdr_list_create(const char *file, int line,
				Sdr sdr);

#define sdr_list_destroy(sdr, list, deleteFn, argument) \
Sdr_list_destroy(__FILE__, __LINE__, sdr, list, deleteFn, argument)
extern void		Sdr_list_destroy(const char *file, int line,
				Sdr sdr, Object list, SdrListDeleteFn deleteFn,
				void *argument);

extern Address		sdr_list_user_data(Sdr sdr, Object list);

#define sdr_list_user_data_set(sdr, list, userData) \
Sdr_list_user_data_set(__FILE__, __LINE__, sdr, list, userData)
extern void		Sdr_list_user_data_set(const char *file, int line,
				Sdr sdr, Object list, Address userData);

extern size_t		sdr_list_length(Sdr sdr, Object list);

#define sdr_list_insert(sdr, list, data, compare, arg) \
Sdr_list_insert(__FILE__, __LINE__, sdr, list, data, compare, arg)
extern Object		Sdr_list_insert(const char *file, int line,
				Sdr sdr, Object list, Address data,
				SdrListCompareFn compare, void *dataBuffer);

#define sdr_list_insert_first(sdr, list, data) \
Sdr_list_insert_first(__FILE__, __LINE__, sdr, list, data)
extern Object		Sdr_list_insert_first(const char *file, int line,
				Sdr sdr, Object list, Address data);

#define sdr_list_insert_last(sdr, list, data) \
Sdr_list_insert_last(__FILE__, __LINE__, sdr, list, data)
extern Object		Sdr_list_insert_last(const char *file, int line,
				Sdr sdr, Object list, Address data);

#define sdr_list_insert_before(sdr, elt, data) \
Sdr_list_insert_before(__FILE__, __LINE__, sdr, elt, data)
extern Object		Sdr_list_insert_before(const char *file, int line,
				Sdr sdr, Object elt, Address data);

#define sdr_list_insert_after(sdr, elt, data) \
Sdr_list_insert_after(__FILE__, __LINE__, sdr, elt, data)
extern Object		Sdr_list_insert_after(const char *file, int line,
				Sdr sdr, Object elt, Address data);

#define sdr_list_delete(sdr, elt, deleteFn, argument) \
Sdr_list_delete(__FILE__, __LINE__, sdr, elt, deleteFn, argument)
extern void		Sdr_list_delete(const char *file, int line,
				Sdr sdr, Object elt, SdrListDeleteFn deleteFn,
				void *argument);

extern Object		sdr_list_list(Sdr sdr, Object elt);
extern Object		sdr_list_first(Sdr sdr, Object list);
extern Object		sdr_list_last(Sdr sdr, Object list);
extern Object		sdr_list_next(Sdr sdr, Object elt);
extern Object		sdr_list_prev(Sdr sdr, Object elt);

extern Object		sdr_list_search(Sdr sdr, Object elt, int reverse,
				SdrListCompareFn compare, void *dataBuffer);

extern Address		sdr_list_data(Sdr sdr, Object elt);

#define sdr_list_data_set(sdr, elt, data) \
Sdr_list_data_set(__FILE__, __LINE__, sdr, elt, data)
extern Address		Sdr_list_data_set(const char *file, int line,
				Sdr sdr, Object elt, Address data);
#ifdef __cplusplus
}
#endif

#endif  /* _SDRLIST_H_ */
