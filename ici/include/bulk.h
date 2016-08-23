/*
 *	bulk.h:	definitions for "bulk" data storage abstraction.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, Jet Propulsion Laboratory
 */

#ifndef _BULK_H_
#define _BULK_H_

extern int		bulk_create(unsigned long item);
extern int		bulk_write(unsigned long item, vast offset,
				char *buffer, vast length);
extern int		bulk_read(unsigned long item, char *buffer,
				vast offset, vast length);
extern void		bulk_destroy(unsigned long item);

#endif  /* _BULK_H_ */
