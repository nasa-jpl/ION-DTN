/*

	zco.h:	definitions enabling the use of ZCOs (zero-copy
		objects), abstract data access representations
		designed to minimize I/O in the encapsulation of
		application source data within one or more layers
		of communication protocol structure.  ZCOs are
		constructed within the heap space of an SDR to
		which all layers of the stack must have access.
		Each ZCO contains information enabling access to
		the source data object, together with (a) a
		linked list of zero or more "extents" that
		reference portions of this source data object
		and (b) linked lists of protocol header and
		trailer capsules.  The concatentation of the
		headers (in ascending stack sequence), source
		data object extents, and trailers (in descending
		stack sequence) is what is to be transmitted.

		The source data object may be either a file
		(identified by pathname stored in a "file reference"
		object in SDR heap) or an array of bytes in SDR
		heap space (identified by SDR address).  Each
		protocol header or trailer capsule indicates
		the length and the address (within SDR heap space)
		of a single protocol header or trailer at some
		layer of the stack.

		ZCOs are not directly exposed to applications.
		Instead, applications operate on ZcoReferences;
		multiple ZcoReferences may, invisibly to the
		applications, refer to the same ZCO.  When the
		last reference to a given ZCO is destroyed --
		either explicitly or (when the entire ZCO has
		been read and copied via this reference)
		implicitly -- the ZCO itself is automatically
		destroyed.

		But NOTE: to reduce code size and complexity and
	       	minimize processing overhead, the ZCO functions
		are NOT mutexed.  It is in general NOT SAFE for
		multiple threads or processes to be operating
		on the same ZCO concurrently, whether using the
		same Zco reference or multiple references.
		However, multiple threads or processes of overlying
		protocol (e.g, multiple final destination endpoints)
		MAY safely use different ZcoReferences to receive
		the source data of a ZCO concurrently once the
		length of that data -- with regard to that layer
		of protocol -- has been firmly established.


	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _ZCO_H_
#define _ZCO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdr.h"

#define	ZCO_FILE_FILL_CHAR	' '

typedef enum
{
	ZcoFileSource = 1,
	ZcoSdrSource
} ZcoMedium;

typedef struct
{
	Object	reference;	/*	ZcoReference			*/
	Object	fileRefObj;
	int	fd;
} ZcoReader;

/*		Commonly used functions for building, managing,
 *		and destroying a ZCO.					*/

Object		zco_create_file_ref(Sdr sdr,
				char *pathName,
				char *cleanupScript);
			/*	cleanupScript, if not NULL, is invoked
			 *	at the time that the last ZCO that
			 *	cites this file reference is destroyed
			 *	[normally upon delivery either down to
			 *	the "ZCO transition layer" of the
			 *	protocol stack or up to a ZCO-capable
			 *	application].  Maximum length of
			 *	cleanupScript is 255.  Returns SDR
			 *	location of file reference object
			 *	on success, 0 on any error.		*/

char		*zco_file_ref_path(Sdr sdr,
				Object fileRef,
				char *buffer,
				int buflen);
			/*	Returns the NULL-terminated pathName
			 *	associated with the indicated file
			 *	reference, stored in buffer and
			 *	truncated to buflen as necessary.
			 *	Returns NULL on any error.		*/

unsigned int	zco_file_ref_occupancy(Sdr sdr,
				Object fileRef);
			/*	Returns number of bytes of SDR space
			 *	occupied by this file reference object.
			 *	If fileRef is zero, returns the maximum
			 *	possible SDR space occupancy of any
			 *	single file reference object.		*/

void		zco_destroy_file_ref(Sdr sdr,
				Object fileRef);
			/*	If file reference is no longer in use
			 *	(no longer referenced by any ZCO) then
			 *	it is destroyed immediately.  Otherwise
			 *	it is flagged for destruction as soon
			 *	as the last reference to it is removed.	*/

Object		zco_create(	Sdr sdr,
				ZcoMedium firstExtentSourceMedium,
				Object firstExtentLocation,
				unsigned int firstExtentOffset,
				unsigned int firstExtentLength);
			/*	The parameters "firstExtentLocation"
			 *	and "firstExtentLength" must either
			 *	both be zero (indicating that
			 *	zco_append_extent will be used to
			 *	insert the first source data extent
			 *	later) or else both be non-zero.
			 *	Returns SDR location of a new ZCO
			 *	reference on success, 0 on any error.	*/

int		zco_append_extent(Sdr sdr,
				Object zcoRef,
				ZcoMedium sourceMedium,
				Object location,
				unsigned int offset,
				unsigned int length);
			/*	Both location and length must be non-
			 *	zero.					*/

int		zco_prepend_header(Sdr sdr,
				Object zcoRef,
				char *header,
				unsigned int length);

void		zco_discard_first_header(Sdr sdr,
				Object zcoRef);

int		zco_append_trailer(Sdr sdr,
				Object zcoRef,
				char *trailer,
				unsigned int length);

void		zco_discard_last_trailer(Sdr sdr,
				Object zcoRef);

Object		zco_add_reference(Sdr sdr,
				Object zcoRef);
			/*	Creates an additional reference to
			 *	the referenced ZCO and adds 1 to its
			 *	reference count.  Returns SDR location
			 *	of ZcoReference on success, 0 on any
			 *	error.					*/

void		zco_destroy_reference(Sdr sdr,
				Object zcoRef);
			/*	Explicitly destroys the indicated Zco
			 *	reference.  When the ZCO's last
			 *	reference is destroyed, the ZCO
			 *	itself is automatically destroyed.	*/

Object		zco_clone(	Sdr sdr,
				Object zcoRef,
				unsigned int offset,
				unsigned int length);
			/*	Creates a new ZCO that is a copy of a
			 *	subset of the indicated Zco reference.  
			 *	Copies portions of the extents of the
			 *	original ZCO as necessary.  Returns
			 *	SDR location of a new ZCO reference
			 *	on success, 0 on any error.		*/

unsigned int	zco_length(	Sdr sdr,
				Object zcoRef);
			/*	Returns length of entire object,
			 *	including all headers and trailers
			 *	and all source data extents.		*/

unsigned int	zco_source_data_length(Sdr sdr,
				Object zcoRef);
			/*	Returns current presumptive length of
			 *	ZCO's encapsulated source data.		*/

unsigned int	zco_occupancy(	Sdr sdr,
				Object zcoRef);
			/*	Returns number of bytes of SDR space
			 *	occupied by the referenced ZCO and all
			 *	references to it.			*/

unsigned int	zco_nbr_of_refs(Sdr sdr,
				Object zcoRef);
			/*	Returns number of ZCO reference objects
			 *	that currently refer to the referenced
			 *	ZCO.					*/

/*	*	Functions for copying ZCO source data.	*	*	*/

void		zco_start_transmitting(Sdr sdr,
				Object zcoRef,
				ZcoReader *reader);
			/*	Used by underlying protocol layer to
			 *	start extraction of outbound ZCO bytes
			 *	(both from header and trailer capsules
			 *	and from source data extents) for
			 *	transmission, i.e., the copying of
			 *	bytes into a memory buffer for delivery
			 *	to some non-ZCO-aware protocol
			 *	implementation.  Initializes reading
			 *	after the last byte of the total
			 *	concatenated ZCO object that has
			 *	already been read via this ZCO
			 *	reference, if any.  Populates "reader"
			 *	object, which is required.		*/

int		zco_transmit(	Sdr sdr,
				ZcoReader *reader,
				unsigned int length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of the total concatenated ZCO object
			 *	into "buffer"; if buffer is NULL,
			 *	simply skips over "length" bytes of
			 *	this ZCO.  Returns the number of bytes
			 *	copied, or -1 on any error.		*/

void		zco_stop_transmitting(Sdr sdr,
				ZcoReader *reader);
			/*	Terminate extraction of outbound ZCO
			 *	bytes for transmission.			*/

void		zco_start_receiving(Sdr sdr,
				Object zcoRef,
				ZcoReader *reader);
			/*	Used by overlying protocol layer to
			 *	start extraction of inbound ZCO bytes
			 *	for reception, i.e., the copying of
			 *	bytes into a memory buffer for delivery
			 *	to a protocol header parser, to a
			 *	protocol trailer parser, or to the
			 *	ultimate recipient (application).
			 *	Initializes reading of headers,
			 *	source data, and trailers after the
			 *	last byte of such content that has
			 *	already been read via this ZCO
			 *	reference, if any.  Populates "reader"
			 *	object, which is required.		*/

int		zco_receive_headers(Sdr sdr,
				ZcoReader *reader,
				unsigned int length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied
			 *	bytes of header data from ZCO source
			 *	extents into "buffer", advancing the
			 *	presumptive start of actual source
			 *	data (and increasing the presumptive
			 *	length of the ZCO's concatenated
			 *	protocol headers) by "length".  Returns
			 *	number of bytes copied, or -1 on any
			 *	error.					*/

void		zco_restore_source(Sdr sdr,
				ZcoReader *reader,
				unsigned int length);
			/*	Backs off the presumptive start of
			 *	actual source data in the ZCO, i.e.,
			 *	reduces by "length" the presumptive
			 *	length of the ZCO's concatenated
			 *	protocol headers and thereby increases
			 *	the presumptive length of the ZCO's
			 *	actual source data.  Use this function
			 *	to readjust internal boundaries within
			 *	the ZCO when the trailing "length"
			 *	bytes of data previously read as
			 *	protocol header data are determined
			 *	instead to be source data.		*/

void		zco_delimit_source(Sdr sdr,
				ZcoReader *reader,
				unsigned int length);
			/*	Sets the presumptive length of actual
			 *	source data in the ZCO to "length",
			 *	thereby establishing the offset from
			 *	the start of the ZCO at which the
			 *	innermost protocol trailer begins.
			 *	Use this function to establish the
			 *	location of the ZCO's innermost
			 *	trailer, based on total source data
			 *	length as determined from the contents
			 *	of the innermost protocol header.	*/

int		zco_receive_source(Sdr sdr,
				ZcoReader *reader,
				unsigned int length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of source data from ZCO extents into
			 *	"buffer"; if buffer is NULL, simply
			 *	skips over "length" bytes of this ZCO's
			 *	source data.  Returns number of bytes
			 *	copied, or -1 on any error.		*/

int		zco_receive_trailers(Sdr sdr,
				ZcoReader *reader,
				unsigned int length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of trailer data from ZCO extents into
			 *	"buffer".  Returns number of bytes
			 *	copied, or -1 on any error.		*/

void		zco_stop_receiving(Sdr sdr,
				ZcoReader *reader);
			/*	Terminates extraction of inbound ZCO
			 *	bytes for reception.			*/

void		zco_strip(	Sdr sdr,
				Object zcoRef);
			/*	Deletes all source data extents that
			 *	contain only header or trailer data,
			 *	adjusts offsets and/or lengths of
			 *	remaining extents to exclude any
			 *	known header or trailer data.  Use
			 *	this function before concatenating
			 *	with another ZCO, before starting
			 *	the transmission of a ZCO that was
			 *	received from an underlying protocol
			 *	layer rather than from an overlying
			 *	application or protocol layer, and
			 *	before enqueuing the ZCO for reception
			 *	by an overlying application or
			 *	protocol layer.				*/

/*		ZCO functions for future use.				*/
#if 0

void		zco_concatenate(Sdr sdr,
				Object aggregateZcoRef,
				Object atomicZcoRef);
			/*	Appends all source data extents of the
			 *	atomic Zco to the source data of the
			 *	aggregate Zco.  Destroys the atomic
			 *	Zco reference, which will implicitly
			 *	destroy the atomic Zco itself.  Fails
			 *	if either Zco contains any identified
			 *	header or trailer data, or if there
			 *	are multiple references to either Zco.	*/

Object		zco_copy(	Sdr sdr,
				Object zcoRef,
				ZcoMedium sourceMedium,
				char *buffer,
				unsigned int bufferLength,
				unsigned int offset,
				unsigned int length);
			/*	Creates a new ZCO containing the
			 *	"length" bytes, starting at "offset",
			 *	of the current presumptive source data
			 *	within the referenced ZCO; no header
			 *	or trailer capsules.  The old ZCO's
			 *	source data is copied in segments of
			 *	at most "bufferLength" bytes each;
			 *	"buffer" is used as the temporary
			 *	staging area for copying the segments
			 *	of source data.  The new ZCO's source
			 *	data will be stored in a newly created
			 *	file or in one or more blocks of SDR
			 *	space, as specified by "sourceMedium";
			 *	if "sourceMedium" is ZcoFileSource,
			 *	then the path name of the file to be
			 *	created must be supplied as the initial
			 *	contents of "buffer".  Returns SDR
			 *	location of reference to new ZCO on
			 *	success, 0 on any error.
			 *
			 *	Use this function when it is necessary
			 *	to fragment the source data content of
			 *	a ZCO.					*/
#endif
#ifdef __cplusplus
}
#endif

#endif  /* _ZCO_H_ */
