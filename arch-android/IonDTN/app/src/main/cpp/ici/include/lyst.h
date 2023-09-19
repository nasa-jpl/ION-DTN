/*
	public header file for routines that manage Lysts.

	Copyright (c) 1997, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Jeff Biesiadecki, Jet Propulsion Laboratory		*/
/*	Adapted by Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#ifndef _LYST_H_
#define _LYST_H_

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * define types
 */

typedef struct LystStruct *Lyst;
typedef struct LystEltStruct *LystElt;

typedef enum {
  LIST_SORT_ASCENDING,
  LIST_SORT_DESCENDING
} LystSortDirection;

typedef int  (*LystCompareFn)(void *,void *);
typedef void (*LystCallback)(LystElt,void *);

/*
 * function prototypes
 */

#define lyst_create_using(idx)	Lyst_create_using(__FILE__, __LINE__, idx)
Lyst Lyst_create_using(const char*,int,int);
#define lyst_create()		Lyst_create(__FILE__, __LINE__)
Lyst Lyst_create(const char*,int);
#define lyst_clear(list)	Lyst_clear(__FILE__, __LINE__, list)
void Lyst_clear(const char*,int,Lyst);
#define lyst_destroy(list)	Lyst_destroy(__FILE__, __LINE__, list)
void Lyst_destroy(const char*,int,Lyst);

void lyst_compare_set(Lyst,LystCompareFn);
LystCompareFn lyst_compare_get(Lyst);
void lyst_direction_set(Lyst,LystSortDirection);
void lyst_delete_set(Lyst,LystCallback,void *);
void lyst_delete_get(Lyst,LystCallback *,void **);
void lyst_insert_set(Lyst,LystCallback,void *);
void lyst_insert_get(Lyst,LystCallback *,void **);
size_t lyst_length(Lyst);

#define lyst_insert(list, data)	Lyst_insert(__FILE__, __LINE__, list, data)
LystElt Lyst_insert(const char*,int,Lyst,void *);
#define lyst_insert_first(list, data)	Lyst_insert_first(__FILE__, __LINE__, \
list, data)
LystElt Lyst_insert_first(const char*,int,Lyst,void *);
#define lyst_insert_last(list, data)	Lyst_insert_last(__FILE__, __LINE__, \
list, data)
LystElt Lyst_insert_last(const char*,int,Lyst,void *);
#define lyst_insert_before(elt, data)	Lyst_insert_before(__FILE__, __LINE__, \
elt, data)
LystElt Lyst_insert_before(const char*,int,LystElt,void *);
#define lyst_insert_after(elt, data)	Lyst_insert_after(__FILE__, __LINE__, \
elt, data)
LystElt Lyst_insert_after(const char*,int,LystElt,void *);
#define lyst_delete(elt)	Lyst_delete(__FILE__, __LINE__, elt)
void Lyst_delete(const char*,int,LystElt);

LystElt lyst_first(Lyst);
LystElt lyst_last(Lyst);
LystElt lyst_next(LystElt);
LystElt lyst_prev(LystElt);
LystElt lyst_search(LystElt,void *);

Lyst lyst_lyst(LystElt);
void *lyst_data(LystElt);
void *lyst_data_set(LystElt,void *);

void lyst_sort(Lyst);
int lyst_sorted(Lyst);
void lyst_apply(Lyst,LystCallback,void *);


#ifdef __cplusplus
}
#endif

#endif  /* _LYST_H_ */
