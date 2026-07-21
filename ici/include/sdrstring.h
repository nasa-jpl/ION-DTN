/*

	sdrstring.h:	definitions supporting use of SDR self-
			delimited strings.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who	What
	06-05-07  SCB	Initial abstraction from original SDR API.

	Copyright (c) 2001-2007 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/

#ifndef SDRSTRING_H
#define SDRSTRING_H

#include "sdrmgt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on self-delimited strings in SDR.	*/

#define	MAX_SDRSTRING	(255)
#define SDRSTRING_BUFSZ	(MAX_SDRSTRING + 1)

#define sdr_string_create(sdr, from) \
Sdr_string_create(__FILE__, __LINE__, sdr, from)
#define sdr_string_dup(sdr, from) \
Sdr_string_dup(__FILE__, __LINE__, sdr, from)

extern SdrObject	Sdr_string_create(const char *file, int line,
				Sdr sdr, char *from);
			/*	strlen of buffer must not exceed 255;
				if it does, or if insufficient SDR
				space is available, 0 is returned.
				Else returns address of newly created
				SDR string object.  To destroy, just
				use sdr_free().				*/

extern SdrObject	Sdr_string_dup(const char *file, int line,
				Sdr sdr, SdrObject from);
			/*	If insufficient SDR space is available,
				0 is returned.  Else returns address
				of newly created copy of original
				SDR string object.  To destroy, just
				use sdr_free().				*/

extern int		sdr_string_length(Sdr sdr, SdrObject string);
			/*	Returns length of indicated SDR string
				object (strlen), or -1 on any error.	*/

extern int		sdr_string_read(Sdr sdr, char *into, SdrObject string);
			/*	Buffer must be SDRSTRING_BUFSZ bytes
				in length to allow for the maximum
				possible SDR string.  Returns length
				of string (strlen), or -1 on any error.	*/
#ifdef __cplusplus
}
#endif

#endif /* SDRSTRING_H */
