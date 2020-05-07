/*

	cbor.h:	definition of the application programming interface
		for ION's limited implementation of Concise Binary
		Object Representation (CBOR).

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _CBOR_H_
#define _CBOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ion.h"

/*	CBOR integer classes						*/
#define	CborAny			-1
#define	CborTiny		0
#define	CborChar		1
#define	CborShort		2
#define	CborInt			4
#define	CborVast		8

/*	CBOR data item types						*/
#define	CborUnsignedInteger	0
#define	CborByteString		2
#define	CborTextString		3
#define	CborArray		4
#define	CborSimpleValue		7

/*	For all functions, *cursor is a pointer to the location in
 *	the CBOR coding buffer at which bytes are to be encoded
 *	or decoded.							*/ 

extern int	cbor_encode_integer(	uvast value,
					unsigned char **cursor);
			/*	Represent this value in an integer
			 *	of the smallest possible integer
			 *	class.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	written.				*/

extern int	cbor_encode_fixed_int(	uvast value,
					int class,
					unsigned char **cursor);
			/*	Represent this value in an integer
			 *	of the indicated class.  Cursor
			 *	is automatically advanced.
			 *	Returns number of bytes written,
			 *	0 on encoding error.			*/

extern int	cbor_encode_byte_string(unsigned char *value,
					uvast size,
					unsigned char **cursor);
			/*	Size is the number of bytes to
			 *	write.  If value is NULL, only
			 *	the size of the byte string is
			 *	written; otherwise the byte string
			 *	itself is written as well.  Cursor
			 *	is advanced by the number of bytes
			 *	written in either case.  Returns
			 *	number of bytes written.		*/

extern int	cbor_encode_text_string(char *value,
					uvast size,
					unsigned char **cursor);
			/*	Size is the number of bytes to
			 *	write.  If value is NULL, only
			 *	the size of the text string is
			 *	written; otherwise the text string
			 *	itself is written as well.  Cursor
			 *	is advanced by the number of bytes
			 *	written in either case.  Returns
			 *	number of bytes written.		*/

extern int	cbor_encode_array_open(	uvast size,
					unsigned char **cursor);
			/*	If size is ((uvast) -1), the array
			 *	is of indefinite size; otherwise
			 *	size indicates the number of items
			 *	in the array.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	written.				*/

extern int	cbor_encode_break(	unsigned char **cursor);
			/*	Break code is written at the
			 *	indicated location.  Cursor is
			 *	automatically advanced.  Returns
			 *	number of bytes written (always 1).	*/

extern int	cbor_decode_initial_byte(unsigned char **cursor,
					unsigned int *bytesBuffered,
					int *majorType,
					int *additionalInfo);
			/*	This function just extracts major
			 *	type and additional info from the
			 *	byte identified by cursor.  Cursor
			 *	is automatically advanced.  Returns
			 *	number of bytes decoded (always 1)
			 *	or 0 on decoding error.			*/

extern int	cbor_decode_integer(	uvast *value,
					int class,
					unsigned char **cursor,
					unsigned int *bytesBuffered);
			/*	If class is CborAny, any class of
			 *	integer data item is accepted;
			 *	otherwise only an integer data
			 *	item of the indicated class is
			 *	accepted.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	read, 0 on decoding error.		*/

extern int	cbor_decode_byte_string(unsigned char *value,
					uvast *size,
					unsigned char **cursor,
					unsigned int *bytesBuffered);
			/*	Initial value of size is the
			 *	maximum allowable size of the
			 *	decoded byte string; the actual
			 *	number of bytes in the byte string
			 *	(which, NOTE, is less than the
			 *	number of bytes read) is returned
			 *	in size.  If value is non-NULL, the
			 *	decoded byte string is copied
			 *	into value and cursor is automatically
			 *	advanced to the end of the byte
			 *	string; otherwise, cursor is advanced
			 *	only to the beginning of the byte
			 *	string.  Returns number of bytes
			 *	read, 0 on decoding error.		*/

extern int	cbor_decode_text_string(char *value,
					uvast *size,
					unsigned char **cursor,
					unsigned int *bytesBuffered);
			/*	Initial value of size is the
			 *	maximum allowable size of the
			 *	decoded text string; the actual
			 *	number of bytes in the text string
			 *	(which, NOTE, is less than the
			 *	number of bytes read) is returned
			 *	in size.  If value is non-NULL,
			 *	the decoded text string is copied
			 *	into value and cursor is automatically
			 *	advanced to the end of the text
			 *	string; otherwise, cursor is advanced
			 *	only to the beginning of the text
			 *	string.  Returns number of bytes
			 *	read, 0 on decoding error.		*/

extern int	cbor_decode_array_open(	uvast *size,
					unsigned char **cursor,
					unsigned int *bytesBuffered);
			/*	If size is zero, any array
			 *	is accepted and the actual size
			 *	of the decoded array is returned
			 *	in size; ((uvast) -1) is returned
			 *	in size if the array is of
			 *	indefinite size.  If size is
			 *	((uvast) -1), only an array of
			 *	indefinite length is accepted.
			 *	Otherwise, size indicates the
			 *	required number of items in the
			 *	array.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	read, 0 on decoding error.		*/

extern int	cbor_decode_break(	unsigned char **cursor,
					unsigned int *bytesBuffered);
			/*	Break code is read from the
			 *	indicated location.  Cursor is
			 *	automatically advanced.  Returns
			 *	number of bytes read, 0 on decoding
			 *	error.					*/

#ifdef __cplusplus
}
#endif

#endif  /* _CBOR_H_ */
