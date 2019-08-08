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
		object in SDR heap) or an item in mass storage
		(identified by item number, with implementation-
		specific semantics, stored in a "bulk reference"
		object in SDR heap) or an object in SDR heap
		space (identified by heap address stored in an
		"object reference" object in SDR heap) or an
		array of bytes in SDR heap space (directly
		identified by heap address).  Each protocol header
		or trailer capsule indicates the length and the
		address (within SDR heap space) of a single protocol
		header or trailer at some layer of the stack.

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
	ZcoInbound = 0,
	ZcoOutbound = 1,
	ZcoUnknown = 2
} ZcoAcct;

typedef enum
{
	ZcoFileSource = 1,
	ZcoBulkSource = 2,
	ZcoObjSource = 3,
	ZcoSdrSource = 4,
	ZcoZcoSource = 5
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

typedef void	(*ZcoCallback)(ZcoAcct);

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
				char *cleanupScript,
				ZcoAcct acct);
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
			 *	255.  Returns SDR heap location of
			 *	file reference object on success, 0
			 *	on any error.				*/

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

extern Object	zco_create_bulk_ref(Sdr sdr,
				unsigned long item,
				vast length,
				ZcoAcct acct);
			/*	The referenced item is automatically
			 *	destroyed at the time that the last
			 *	ZCO that cites this bulk reference is
			 *	destroyed [normally upon delivery
			 *	either down to the "ZCO transition
			 *	layer" of the protocol stack or up to
			 *	a ZCO-capable application].  Returns
			 *	SDR heap location of bulk reference
			 *	object on success, 0 on any error.	*/

extern void	zco_destroy_bulk_ref(Sdr sdr,
				Object bulkRef);
			/*	If bulk reference is no longer in use
			 *	(no longer referenced by any ZCO) then
			 *	it is destroyed immediately.  Otherwise
			 *	it is flagged for destruction as soon
			 *	as the last reference to it is removed.	*/

extern Object	zco_create_obj_ref(Sdr sdr,
				Object object,
				vast length,
				ZcoAcct acct);
			/*	The referenced object is automatically
			 *	freed at the time that the last ZCO
			 *	that cites this object reference is
			 *	destroyed [normally upon delivery
			 *	either down to the "ZCO transition
			 *	layer" of the protocol stack or up to
			 *	a ZCO-capable application].  Returns
			 *	SDR heap location of object reference
			 *	object on success, 0 on any error.	*/

extern void	zco_destroy_obj_ref(Sdr sdr,
				Object objRef);
			/*	If object reference is no longer in use
			 *	(no longer referenced by any ZCO) then
			 *	it is destroyed immediately.  Otherwise
			 *	it is flagged for destruction as soon
			 *	as the last reference to it is removed.	*/

extern void	zco_status(Sdr sdr);
			/*	Writes a report of the current contents
 			 *	of the ZCO database to ion.log.		*/

extern double	zco_get_file_occupancy(Sdr sdr,
				ZcoAcct acct);
			/*	Returns the total number of file
			 *	system space bytes occupied by ZCOs
			 *	in this SDR.				*/

extern void	zco_set_max_file_occupancy(Sdr sdr,
				double occupancy,
				ZcoAcct acct);
			/*	Sets the maximum number of file
			 *	system space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern double	zco_get_max_file_occupancy(Sdr sdr,
				ZcoAcct acct);
			/*	Returns the maximum number of file
			 *	system space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern int	zco_enough_file_space(Sdr sdr,
				vast length,
				ZcoAcct acct);
			/*	Returns 1 if the total remaining file
			 *	system space available for ZCOs is
			 *	greater than length, 0 otherwise.	*/

extern double	zco_get_bulk_occupancy(Sdr sdr,
				ZcoAcct acct);
			/*	Returns the total number of bulk
			 *	storage space bytes occupied by ZCOs
			 *	in this SDR.				*/

extern void	zco_set_max_bulk_occupancy(Sdr sdr,
				double occupancy,
				ZcoAcct acct);
			/*	Sets the maximum number of bulk
			 *	storage space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern double	zco_get_max_bulk_occupancy(Sdr sdr,
				ZcoAcct acct);
			/*	Returns the maximum number of bulk
			 *	storage space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern int	zco_enough_bulk_space(Sdr sdr,
				vast length,
				ZcoAcct acct);
			/*	Returns 1 if the total remaining bulk
			 *	storage space available for ZCOs is
			 *	greater than length, 0 otherwise.	*/

extern double	zco_get_heap_occupancy(Sdr sdr,
				ZcoAcct acct);
			/*	Returns the total number of SDR
			 *	heap space bytes occupied by ZCOs
			 *	in this SDR.				*/

extern void	zco_set_max_heap_occupancy(Sdr sdr,
				double occupancy,
				ZcoAcct acct);
			/*	Sets the maximum number of SDR
			 *	heap space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern double	zco_get_max_heap_occupancy(Sdr sdr,
				ZcoAcct acct);
			/*	Returns the maximum number of SDR
			 *	heap space bytes that may be
			 *	occupied by ZCOs in this SDR.		*/

extern int	zco_enough_heap_space(Sdr sdr,
				vast length,
				ZcoAcct acct);
			/*	Returns 1 if the total remaining SDR
			 *	heap space available for ZCOs is
			 *	greater than length, 0 otherwise.	*/

extern int	zco_extent_too_large(Sdr sdr,
				ZcoMedium sourceMedium,
				vast length,
				ZcoAcct acct);
			/*	Returns 1 if the total remaining space
			 *	available for ZCOs is NOT enough to
			 *	contain a new extent of the indicated
			 *	length in the indicated source medium.
			 *	Returns 0 otherwise.			*/

extern void	zco_get_aggregate_length(Sdr sdr,
				Object location,
				vast offset,
				vast length,
				vast *fileSpaceOccupied,
				vast *bulkSpaceOccupied,
				vast *heapSpaceOccupied);
			/*	Populates the *fileSpaceOccupied,
			 *	*bulkSpaceOccupied, and
			 *	*heapSpaceOccupied fields with the
			 *	total number of ZCO space bytes
			 *	occupied by the extents of the zco
			 *	at "location", from "offset" to
			 *	offset + length.  If offset isn't
			 *	the start of an extent or offset
			 *	+ length isn't the end of an extent,
			 *	returns -1 in all "Occupied" fields.	*/

extern Object	zco_create(	Sdr sdr,
				ZcoMedium firstExtentSourceMedium,
				Object firstExtentLocation,
				vast firstExtentOffset,
				vast firstExtentLength,
				ZcoAcct acct);
			/*	The parameters "firstExtentLocation"
			 *	and "firstExtentLength" must either
			 *	both be zero (indicating that
			 *	zco_append_extent will be used to
			 *	insert the first source data extent
			 *	later) or else both be non-zero.
			 *	A negative value for firstExtentLength
			 *	indicates that the extent is already
			 *	known not to be too large for the
			 *	available ZCO space, and the actual
			 *	length of the extent is the additive
			 *	inverse of this value.
			 *
			 *	If firstExtentSourceMedium is
			 *	ZcoFileSource then firstExtentLocation
			 *	must be the SDR heap location of a
			 *	file reference.
			 *
			 *	If firstExtentSourceMedium is
			 *	ZcoBulkSource then firstExtentLocation
			 *	must be the SDR heap location of a
			 *	bulk reference.
			 *
			 *	If firstExtentSourceMedium is
			 *	ZcoObjSource then firstExtentLocation
			 *	must be the SDR heap location of an
			 *	object reference.
			 *
			 *	If firstExtentSourceMedium is
			 *	ZcoSdrSource then firstExtentLocation
			 *	must be the SDR heap location of an
			 *	SDR heap object.
			 *
			 *	Returns SDR location of a new ZCO
			 *	object on success, 0 if there is
			 *	currently too little available ZCO
			 *	space in the indicated account to
			 *	accommodate the proposed first
			 *	extent, ((Object) -1) on any error.	*/

extern vast	zco_append_extent(Sdr sdr,
				Object zco,
				ZcoMedium sourceMedium,
				Object location,
				vast offset,
				vast length);
			/*	Both location and length must be non-
			 *	zero.  A negative value for length
			 *	indicates that the extent is already
			 *	known not to be too large for the
			 *	available ZCO space, and the actual
			 *	length of the extent is the additive
			 *	inverse of this value.
			 *
			 *	For constraints on the value of
			 *	location, see zco_create().
			 *
			 *	Returns length on success, 0 if there
			 *	is currently too little available ZCO
			 *	space to accommodate the proposed
			 *	extent, -1 on any error.		*/

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

extern Object	zco_header_text(Sdr sdr,
				Object zco,
				int skip,
				vast *length);
			/*	Skips over the first "skip" headers
			 *	of the indicated ZCO and returns the
			 *	address of the text of the next one,
			 *	placing the length of that text in
			 *	*length.  Returns 0 on any error.	*/

extern Object	zco_trailer_text(Sdr sdr,
				Object zco,
				int skip,
				vast *length);
			/*	Skips over the first "skip" trailers
			 *	of the indicated ZCO and returns the
			 *	address of the text of the next one,
			 *	placing the length of that text in
			 *	*length.  Returns 0 on any error.	*/

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

extern int	zco_revise(	Sdr sdr,
				Object zco,
				vast offset,
				char *buffer,
				vast length);
			/*	Writes the contents of buffer, for
			 *	the indicated length, into the 
			 *	indicated ZCO at the indicated offset.
			 *	Return 0 on success, -1 on any error.	*/

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
			 *	total data length cloned, or -1 on
			 *	any error.				*/

extern vast	zco_length(	Sdr sdr,
				Object zco);
			/*	Returns length of entire zero-copy
			 *	object, including all headers and
			 *	trailers and all source data extents.	*/

extern vast	zco_source_data_length(Sdr sdr,
				Object zco);
			/*	Returns current presumptive length of
			 *	this ZCO's encapsulated source data.	*/

extern ZcoAcct	zco_acct(	Sdr sdr,
				Object zco);
			/*	Returns an indicator as to whether
			 *	this ZCO is inbound or outbound.	*/

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
