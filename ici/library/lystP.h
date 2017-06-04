/*
	Private header file for routines that manage doubly-linked
	lists, called "lysts".
									*/
/*									*/
/*	Copyright (c) 1997, California Institute of Technology.		*/
/*	ALL RIGHTS RESERVED.  U.S. Government Sponsorship		*/
/*	acknowledged.							*/
/*	Author: Jeff Biesiadecki, Jet Propulsion Laboratory		*/
/*	Adapted by Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _LYSTP_H_
#define _LYSTP_H_

#include "lyst.h"

/* define a lyst */
struct LystStruct {
   LystElt           first,   /* points to the first element in the lyst */
                     last;    /* points to the last element in the lyst */
   size_t	     length;  /* the number of elements in the lyst */

   LystCompareFn     compare; /* element comparison function for sorted lysts */
   LystSortDirection dir;     /* direction of sort */

   LystCallback      delete_cb;   /* function when deleting an element */
   void              *delete_arg; /* argument to pass to delete function */

   LystCallback      insert_cb;   /* function when inserting an element */
   void              *insert_arg; /* argument to pass to insert function */

   int               alloc_idx;   /* element memory allocation function index */
};

/* define an element of a lyst */
struct LystEltStruct {
   Lyst    lyst; /* lyst that this element belongs to */
   LystElt prev, /* previous element in lyst */
           next; /* next element in lyst */
   void    *data; /* data for this element */
};

#endif  /* _LYSTP_H_ */
