/*

	sdr.h:	definitions supporting use of an abstract Spacecraft
		Data Recorder.  The underlying principle is that an
		SDR provides standardized support for user data
		organization at object granularity and direct
		access to persistent user data objects, rather than
		supporting user data organization only at file
		granularity and requiring the user to implement
		access to the data objects accreted within those
		files.  An SDR is a preallocated region of notionally
		persistent shared memory with a flat address space.

	Author: Scott Burleigh, JPL
	
	Modification History:
	Date      Who	What
	06-05-07  SCB	Split into multiple separable modules.
	09-01-02  SCB	Added "trace" functions.
	08-01-01  APS	Initial delivery to Ball.

	Copyright (c) 2001-2007 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#ifndef _SDR_H_
#define _SDR_H_

#include "sdrstring.h"
#include "sdrlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on the SDR Object catalogue.		*/

#define sdr_catlg(sdr, name, type, object) \
Sdr_catlg(__FILE__, __LINE__, sdr, name, type, object)
extern void		Sdr_catlg(const char *file, int line,
				Sdr sdr, char *name, int type, Object object);

extern Object		sdr_find(Sdr sdr, char *name, int *type);

#define sdr_uncatlg(sdr, name) \
Sdr_uncatlg(__FILE__, __LINE__, sdr, name)
extern void		Sdr_uncatlg(const char *file, int line,
				Sdr sdr, char *name);

extern Object		sdr_read_catlg(Sdr sdr, char *name, int *type,
				Object *object, Object previous_entry);
			/*	Returns address of catalogue entry, a
				list element; content of entry is
				copied into name, type, object.  The
				returned address can be supplied as
				previous_entry in a subsequent
				sdr_read_catlg call to get the
				next entry in the catalog.  If
				previous_entry is zero, first
				catalogue entry is read.		*/
#ifdef __cplusplus
}
#endif

#endif  /* _SDR_H_ */
