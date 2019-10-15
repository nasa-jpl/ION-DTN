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

/*	For all functions, *cursor is a pointer to the location in
 *	the CBOR coding buffer at which bytes are to be encoded
 *	or decoded.							*/ 

extern int	cbor_encode_integer(	uvast value,
					int class,
					unsigned char **cursor);
			/*	If class is -1, represent in the
			 *	smallest possible integer class;
			 *	otherwise represent in integer
			 *	of the indicated class.  Cursor
			 *	is automatically advanced.
			 *	Returns number of bytes written,
			 *	or -1 on any error.			*/

extern int	cbor_encode_byte_string(unsigned char *value,
					uvast size,
					unsigned char **cursor);
			/*	Size is the number of bytes to
			 *	write.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	written, or -1 on any error.		*/

extern int	cbor_encode_text_string(char *value,
					uvast size,
					unsigned char **cursor);
			/*	Size is the number of bytes to
			 *	write.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	written, or -1 on any error.		*/

extern int	cbor_encode_array_open(	uvast size,
					unsigned char **cursor);
			/*	If size is ((uvast) -1), the array
			 *	is of indefinite size; otherwise
			 *	size indicates the number of items
			 *	in the array.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	written, or -1 on any error.		*/

extern int	cbor_encode_break(	unsigned char **cursor);
			/*	Break code is written at the
			 *	indicated location.  Cursor is
			 *	automatically advanced.  Returns
			 *	number of bytes written, or -1
			 *	on any error.				*/

extern void	cbor_decode_initial_byte(unsigned char *cursor,
					int *majorType,
					int *additionalInfo);
			/*	This function just extracts major
			 *	type and additional info from the
			 *	byte identified by cursor.		*/

extern int	cbor_decode_integer(	uvast *value,
					int class,
					unsigned char **cursor);
			/*	If class is -1, any class of integer
			 *	data item is accepted; otherwise
			 *	only an integer data item of the
			 *	indicated class is accepted.
			 *	Cursor is automatically advanced.
			 *	Returns number of bytes read, or
			 *	-1 on any error.			*/

extern int	cbor_decode_byte_string(unsigned char *value,
					uvast *size,
					unsigned char **cursor);
			/*	Initial value of size is the
			 *	maximum allowable size of the
			 *	decoded byte string; the actual
			 *	number of bytes in the byte string
			 *	(which, NOTE, is less than the
			 *	number of bytes read) is returned
			 *	in size.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	read, or -1 on any error.		*/

extern int	cbor_decode_text_string(char *value,
					uvast *size,
					unsigned char **cursor);
			/*	Initial value of size is the
			 *	maximum allowable size of the
			 *	decoded text string; the actual
			 *	number of bytes in the text string
			 *	(which, NOTE, is less than the
			 *	number of bytes read) is returned
			 *	in size.  Cursor is automatically
			 *	advanced.  Returns number of bytes
			 *	read, or -1 on any error.		*/

extern int	cbor_decode_array_open(	uvast *size,
					unsigned char **cursor);
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
			 *	advanced.  Returns number of
			 *	bytes read, or -1 on any error.		*/

extern int	cbor_decode_break(	unsigned char **cursor);
			/*	Break code is read from the
			 *	indicated location.  Cursor is
			 *	automatically advanced.  Returns
			 *	number of bytes written, or -1
			 *	on any error.				*/

#ifdef __cplusplus
}
#endif

#endif  /* _CBOR_H_ */
