/*

	zco.h:	definitions enabling the use of ZCOs (zero-copy
		objects), abstract data access representations
		designed to minimize I/O in the encapsulation of
		application source data within one or more layers
		of communication protocol structure.  ZCOs are
		constructed within the heap space of an SDR to
		which all layers of the stack must have access.
		Each ZCO contains information enabling access to
		one or more source data objects, together with
		(a) a linked list of zero or more "extents" that
		reference portions of the source data object(s)
		and (b) linked lists of protocol header and
		trailer capsules.  The concatentation of the
		headers (in ascending stack sequence), source
		data object extents, and trailers (in descending
		stack sequence) is what is to be transmitted or
		has been received.

		Each source data object may be either a file
		(identified by pathname stored in a "file reference"
		object in SDR heap) or an array of bytes in SDR
		heap space (identified by SDR address).  Each
		protocol header or trailer capsule indicates
		the length and the address (within SDR heap space)
		of a single protocol header or trailer at some
		layer of the stack.

		The extents of multiple ZCOs may reference the
		same files and/or SDR source data objects.  The
		source data objects are reference-counted to
		ensure that they are deleted automatically when
		(and only when) all ZCO extents that reference
		them have been deleted.

		Note that the safety of shared access to a ZCO is
		protected by the fact that the ZCO resides in SDR
		and therefore cannot be modified other than in the
		course of an SDR transaction, which serializes access.
		Moreover, extraction of data from a ZCO may entail
		the reading of file-based source data extents, which
		may cause file progress to be updated in one or
		more file reference objects in the SDR heap.
		For this reason, all ZCO "transmit" and "receive"
		functions must be performed within SDR transactions.

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
	ZcoSdrSource = 2,
	ZcoZcoSource = 3
} ZcoMedium;

typedef struct
{
	Object	zco;
	int	trackFileOffset;		/*	Boolean control	*/
	vast	headersLengthCopied;		/*	within extents	*/
	vast	sourceLengthCopied;		/*	within extents	*/
	vast	trailersLengthCopied;		/*	within extents	*/
	vast	lengthCopied;			/*	incl. capsules	*/
} ZcoReader;

/*	Commonly used functions for building, accessing, managing,
 	and destroying a ZCO.						*/

typedef void	(*ZcoCallback)();

extern void	zco_register_callback(ZcoCallback notify);
			/*	Provides the callback function that
			 *	the ZCO system will invoke every time
			 *	a ZCO is destroyed, making ZCO space
			 *	available for new ZCO creation.		*/

extern void	zco_unregister_callback();
			/*	Removes the currently registered
			 *	ZCO-space-available callback.		*/

extern Object	zco_create_file_ref(Sdr sdr,
				char *pathName,
				char *cleanupScript);
			/*	cleanupScript, if not NULL, is invoked
			 *	at the time that the last ZCO that
			 *	cites this file reference is destroyed
			 *	[normally upon delivery either down to
			 *	the "ZCO transition layer" of the
			 *	protocol stack or up to a ZCO-capable
			 *	application]; a zero-length string
			 *	is interpreted as implicit direction
			 *	to delete the referenced file when
			 *	the file reference is destroyed.
			 *	Maximum length of cleanupScript is
			 *	255.  Returns SDR location of file
			 *	reference object on success, 0 on any
			 *	error.					*/

extern int	zco_revise_file_ref(Sdr sdr,
				Object fileRef,
				char *pathName,
				char *cleanupScript);
			/*	Changes the pathName and cleanupScript
			 *	of the indicated file reference.  The
			 *	new values of these fields are validated
			 *	as for zco_create_file_ref.  Returns 0
			 *	on success, -1 on any error.		*/

extern char	*zco_file_ref_path(Sdr sdr,
				Object fileRef,
				char *buffer,
				int buflen);
			/*	Returns the NULL-terminated pathName
			 *	associated with the indicated file
			 *	reference, stored in buffer and
			 *	truncated to buflen as necessary.
			 *	Returns NULL on any error.		*/

extern int	zco_file_ref_xmit_eof(Sdr sdr,
				Object fileRef);
			/*	Returns 1 if the last octet of the
			 *	referenced file (as determined at the
			 *	time the file reference object was
			 *	created) has been read by ZCO via a
			 *	reader with file offset tracking
			 *	turned on.  Otherwise returns zero.	*/

extern void	zco_destroy_file_ref(Sdr sdr,
				Object fileRef);
			/*	If file reference is no longer in use
			 *	(no longer referenced by any ZCO) then
			 *	it is destroyed immediately.  Otherwise
			 *	it is flagged for destruction as soon
			 *	as the last reference to it is removed.	*/

extern vast	zco_get_file_occupancy(Sdr sdr);
			/*	Returns the total number of file
			 *	system space bytes occupied by ZCOs
			 *	in this SDR.				*/

extern void	zco_set_max_file_occupancy(Sdr sdr,
				vast occupancy);
			/*	Sets the maximum number of file
			 *	system space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern vast	zco_get_max_file_occupancy(Sdr sdr);
			/*	Returns the maximum number of file
			 *	system space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern int	zco_enough_file_space(Sdr sdr,
				vast length);
			/*	Returns 1 if the total remaining file
			 *	system space available for ZCOs is
			 *	greater than length, 0 otherwise.	*/

extern vast	zco_get_heap_occupancy(Sdr sdr);
			/*	Returns the total number of SDR
			 *	heap space bytes occupied by ZCOs
			 *	in this SDR.				*/

extern void	zco_set_max_heap_occupancy(Sdr sdr,
				vast occupancy);
			/*	Sets the maximum number of SDR
			 *	heap space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern vast	zco_get_max_heap_occupancy(Sdr sdr);
			/*	Returns the maximum number of SDR
			 *	heap space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern int	zco_enough_heap_space(Sdr sdr,
				vast length);
			/*	Returns 1 if the total remaining SDR
			 *	heap space available for ZCOs is
			 *	greater than length, 0 otherwise.	*/

extern Object	zco_create(	Sdr sdr,
				ZcoMedium firstExtentSourceMedium,
				Object firstExtentLocation,
				vast firstExtentOffset,
				vast firstExtentLength);
			/*	The parameters "firstExtentLocation"
			 *	and "firstExtentLength" must either
			 *	both be zero (indicating that
			 *	zco_append_extent will be used to
			 *	insert the first source data extent
			 *	later) or else both be non-zero.
			 *	Returns SDR location of a new ZCO
			 *	object on success, 0 if there is
			 *	currently too little available ZCO
			 *	space to accommodate the proposed
			 *	first extent, ((Object) -1) on any
			 *	error.					*/

extern vast	zco_append_extent(Sdr sdr,
				Object zco,
				ZcoMedium sourceMedium,
				Object location,
				vast offset,
				vast length);
			/*	Both location and length must be non-
			 *	zero.  Returns length on success, 0
			 *	if there is currently too little
			 *	available ZCO space to accommodate
			 *	the proposed first extent, -1 on any
			 *	error.					*/

extern int	zco_prepend_header(Sdr sdr,
				Object zco,
				char *header,
				vast length);

extern void	zco_discard_first_header(Sdr sdr,
				Object zco);

extern int	zco_append_trailer(Sdr sdr,
				Object zco,
				char *trailer,
				vast length);

extern void	zco_discard_last_trailer(Sdr sdr,
				Object zco);

extern void	zco_destroy(	Sdr sdr,
				Object zco);
			/*	Explicitly destroys the indicated ZCO.
			 *	This reduces the reference counts for
			 *	all files and SDR objects referenced
			 *	in the ZCO's extents, resulting in the
			 *	freeing of SDR objects and (optionally)
			 *	the deletion of files as those
			 *	reference counts drop to zero.		*/

extern int	zco_bond(	Sdr sdr,
				Object zco);
			/*	Converts all headers and trailers to
			 *	source data extents.  Use this function
			 *	to prevent header and trailer data
			 *	from being omitted when the ZCO is
			 *	cloned.					*/

extern Object	zco_clone(	Sdr sdr,
				Object zco,
				vast offset,
				vast length);
			/*	Creates a new ZCO that is a copy of a
			 *	subset of the indicated ZCO.  This
			 *	procedure is required whenever it is
			 *	necessary to process the ZCO's source
			 *	data in multiple different ways, for
			 *	different purposes, and therefore the
			 *	ZCO must be in multiple states at the
			 *	same time.  Copies portions of the
			 *	extents of the original ZCO as needed,
			 *	adding to the reference counts of the
			 *	file and SDR source data objects
			 *	referenced by those extents.  Returns
			 *	the SDR location of the new ZCO on
			 *	success, ((Object) -1) on any error.	*/

extern vast	zco_clone_source_data(Sdr sdr,
				Object toZco,
				Object fromZco,
				vast offset,
				vast length);
			/*	Same as zco_clone except that the
			 *	cloned source data extents are appended
			 *	to an existing ZCO ("toZco") rather
			 *	than to a newly created ZCO.  Returns
			 *	length on success, -1 on any error.	*/

extern vast	zco_length(	Sdr sdr,
				Object zco);
			/*	Returns length of entire zero-copy
			 *	object, including all headers and
			 *	trailers and all source data extents.	*/

extern vast	zco_source_data_length(Sdr sdr,
				Object zco);
			/*	Returns current presumptive length of
			 *	the ZCO's encapsulated source data.	*/

/*	*	Functions for copying ZCO source data.	*	*	*/

extern void	zco_start_transmitting(Object zco,
				ZcoReader *reader);
			/*	Used by underlying protocol layer to
			 *	start extraction of outbound ZCO bytes
			 *	(both from header and trailer capsules
			 *	and from source data extents) for
			 *	transmission, i.e., the copying of
			 *	bytes into a memory buffer for delivery
			 *	to some non-ZCO-aware protocol
			 *	implementation.  Initializes reading
			 *	at the first byte of the concatenated
			 *	ZCO object.  Populates "reader" object,
			 *	which is required.
			 *
			 *	Note that this function can be called
			 *	multiple times to restart reading at
			 *	the start of the ZCO.  Note also that
			 *	multiple ZcoReader objects may be
			 *	used concurrently, by the same task
			 *	or different tasks, to advance through
			 *	the ZCO independently.			*/

extern void	zco_track_file_offset(ZcoReader *reader);
			/*	Turn on file offset tracking for this
			 *	reader.					*/

extern vast	zco_transmit(	Sdr sdr,
				ZcoReader *reader,
				vast length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of the total concatenated ZCO object
			 *	into "buffer"; if buffer is NULL,
			 *	simply skips over "length" bytes of
			 *	this ZCO.  Returns the number of bytes
			 *	copied, or -1 on any error.		*/

extern void	zco_start_receiving(Object zco,
				ZcoReader *reader);
			/*	Used by overlying protocol layer to
			 *	start extraction of inbound ZCO bytes
			 *	for reception, i.e., the copying of
			 *	bytes into a memory buffer for delivery
			 *	to a protocol header parser, to a
			 *	protocol trailer parser, or to the
			 *	ultimate recipient (application).
			 *	Initializes reading of headers, source
			 *	data, and trailers at the first byte
			 *	of the concatenated ZCO object.
			 *
			 *	Populates "reader" object, which is
			 *	required.				*/

extern vast	zco_receive_headers(Sdr sdr,
				ZcoReader *reader,
				vast length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of presumptive protocol header text
			 *	from ZCO source data extents into
			 *	"buffer".  Returns number of bytes
			 *	copied, or -1 on any error.		*/

extern void	zco_delimit_source(Sdr sdr,
				Object zco,
				vast offset,
				vast length);
			/*	Sets the computed offset and length
			 *	of actual source data in the ZCO,
			 *	thereby implicitly establishing the
			 *	total length of the ZCO's concatenated
			 *	protocol headers as "offset" and
			 *	the location of the ZCO's innermost
			 *	protocol trailer as the sum of
			 *	"offset" and "length".  Offset and
			 *	length are typically determined from
			 *	the information carried in received
			 *	presumptive protocol header text.	*/

extern vast	zco_receive_source(Sdr sdr,
				ZcoReader *reader,
				vast length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of source data from ZCO extents into
			 *	"buffer"; if buffer is NULL, simply
			 *	skips over "length" bytes of this ZCO's
			 *	source data.  Returns number of bytes
			 *	copied, or -1 on any error.		*/

extern vast	zco_receive_trailers(Sdr sdr,
				ZcoReader *reader,
				vast length,
				char *buffer);
			/*	Copies "length" as-yet-uncopied bytes
			 *	of trailer data from ZCO extents into
			 *	"buffer".  Returns number of bytes
			 *	copied, or -1 on any error.		*/

extern void	zco_strip(	Sdr sdr,
				Object zco);
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

#ifdef __cplusplus
}
#endif

#endif  /* _ZCO_H_ */
