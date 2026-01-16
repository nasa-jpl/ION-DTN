/*
 *	bulk.h:	definitions for "bulk" data storage abstraction.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, Jet Propulsion Laboratory
 */

#ifndef BULK_H
#define BULK_H

#include "platform.h"

extern int		bulk_create(unsigned long item);
extern int		bulk_write(unsigned long item, vast offset,
				char *buffer, vast length);
extern int		bulk_read(unsigned long item, char *buffer,
				vast offset, vast length);
extern void		bulk_destroy(unsigned long item);

#endif /* BULK_H */
