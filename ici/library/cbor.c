/*

	cbor.c:	API for encoding and decoding CBOR elements in ION.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "cbor.h"

static void	encodeFirstByte(unsigned char **cursor, int majorType,
			int additionalInfo)
{
	unsigned char	byte;

	byte = majorType & 0x07;
	byte = (byte << 5) + (additionalInfo & 0x1f);
	**cursor = byte;
	*cursor += 1;
}

static int	encodeInteger(uvast value, int *additionalInfo,
			unsigned char *cursor)
{
	uvast		residue;
	unsigned char	bytes[8];
	int		first;

	if (value < 24)
	{
		*additionalInfo = value;
		return 0;
	}

	residue = value;
	first = 7;

	/*	Try to fit into 1 byte of length.			*/

	bytes[first] = residue & 0xff;
	residue /= 256;
	if (residue == 0)
	{
		*additionalInfo = 24;
		*cursor = bytes[first];
		return 1;
	}

	/*	Try to fit into 2 bytes of length.			*/

	first--;
	bytes[first] = residue & 0xff;
	residue /= 256;
	if (residue == 0)
	{
		*additionalInfo = 25;
		*cursor = bytes[first];
		cursor += 1;
		first++;
		*cursor = bytes[first];
		return 2;
	}

	/*	Try to fit into 4 bytes of length.			*/

	first--;
	bytes[first] = residue & 0xff;
	residue /= 256;
	first--;
	bytes[first] = residue & 0xff;
	residue /= 256;
	if (residue == 0)
	{
		*additionalInfo = 26;
		*cursor = bytes[first];
		cursor += 1;
		first++;
		*cursor = bytes[first];
		cursor += 1;
		first++;
		*cursor = bytes[first];
		cursor += 1;
		first++;
		*cursor = bytes[first];
		return 4;
	}

	/*	Can only fit into 8 bytes of length.			*/

	first--;
	bytes[first] = residue & 0xff;
	residue /= 256;
	first--;
	bytes[first] = residue & 0xff;
	residue /= 256;
	first--;
	bytes[first] = residue & 0xff;
	residue /= 256;
	first--;
	bytes[first] = residue & 0xff;
	*additionalInfo = 27;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	cursor += 1;
	first++;
	*cursor = bytes[first];
	return 8;
}

int	cbor_encode_integer(uvast value, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	length = encodeInteger(value, &additionalInfo, bytes);
	encodeFirstByte(cursor, CborUnsignedInteger, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	return 1 + length;
}

static int	encodeFixedLengthInteger(uvast value, int class,
			int *additionalInfo, unsigned char *cursor)
{
	switch (class)
	{
	case CborTiny:
		if (value > 23)
		{
			writeMemoNote("[?] CBOR integer not tiny",
					itoa(value));
			return -1;
		}

		*additionalInfo = value;
		return 0;

	case CborChar:
		if (value > 255)
		{
			writeMemoNote("[?] CBOR integer not char",
					itoa(value));
			return -1;
		}

		*cursor = value;
		*additionalInfo = 24;
		return 1;

	case CborShort:
		if (value > 65535)
		{
			writeMemoNote("[?] CBOR integer not short",
					itoa(value));
			return -1;
		}

		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		*additionalInfo = 25;
		return 2;

	case CborInt:
		if (value > 4294967295UL)
		{
			writeMemoNote("[?] CBOR integer not int",
					itoa(value));
			return -1;
		}

		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		*additionalInfo = 26;
		return 4;

	case CborVast:
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		cursor += 1;
		value /= 256;
		*cursor = value & 0xff;
		*additionalInfo = 27;
		return 8;

	default:
		writeMemoNote("[?] Invalid integer class for CBOR",
				itoa(class));
		return -1;
	}
}

int	cbor_encode_fixed_int(uvast value, int class, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	length = encodeFixedLengthInteger(value, class, &additionalInfo, bytes);
	if (length < 0)
	{
		writeMemoNote("[?] CBOR integer encode failed", itoa(value));
		return 0;
	}

	encodeFirstByte(cursor, CborUnsignedInteger, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	return 1 + length;
}

int	cbor_encode_byte_string(unsigned char *value, uvast size,
		unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	length = encodeInteger(size, &additionalInfo, bytes);
	encodeFirstByte(cursor, CborByteString, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	if (value)
	{
		memcpy(*cursor, value, size);
		*cursor += size;
		length += size;
	}

	return 1 + length;
}

int	cbor_encode_text_string(char *value, uvast size, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	length = encodeInteger(size, &additionalInfo, bytes);
	encodeFirstByte(cursor, CborTextString, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	if (value)
	{
		memcpy(*cursor, value, size);
		*cursor += size;
		length += size;
	}

	return 1 + length;
}

int	cbor_encode_array_open(uvast size, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	if (size == ((uvast) -1))
	{
		additionalInfo = 31;	/*	Indefinite-size array.	*/
		length = 0;
	}
	else
	{
		length = encodeInteger(size, &additionalInfo, bytes);
	}

	encodeFirstByte(cursor, CborArray, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	return 1 + length;
}

int	cbor_encode_break(unsigned char **cursor)
{
	encodeFirstByte(cursor, CborSimpleValue, 31);
	return 1;
}

static int	decodeFirstByte(unsigned char **cursor,
			unsigned int *bytesBuffered, int *majorType,
			int *additionalInfo)
{
	if (*bytesBuffered < 1)
	{
		writeMemo("[?] No initial byte.");
		return 0;
	}

	*majorType = (**cursor) >> 5 & 0x07;
	*additionalInfo = (**cursor) & 0x1f;
	*cursor += 1;
	*bytesBuffered -= 1;
	return 1;
}

int	cbor_decode_initial_byte(unsigned char **cursor,
		unsigned int *bytesBuffered, int *majorType,
		int *additionalInfo)
{
	CHKZERO(cursor && bytesBuffered && majorType && additionalInfo);
	return decodeFirstByte(cursor, bytesBuffered, majorType,
			additionalInfo);
}

static int	decodeInteger(uvast *value, int class, int additionalInfo, 
			unsigned char **cursor, unsigned int *bytesBuffered)
{
	uvast	sum;

	if (additionalInfo < 24)	/*	Value already read.	*/
	{
		if (class == CborAny || class == CborTiny)
		{
			*value = additionalInfo;
			return 0;
		}

		writeMemo("[?] CBOR error: got Tiny value.");
		return -1;
	}

	switch (additionalInfo)
	{
	case 24:
		if (class == CborAny || class == CborChar)
		{
			if (*bytesBuffered < 1)
			{
				writeMemo("[?] Not enough bytes for Char.");
				return -1;
			}

			*value = **cursor;
			*cursor += 1;
			*bytesBuffered -= 1;
			return 1;
		}

		writeMemo("[?] CBOR error: got Char value.");
		return -1;

	case 25:
		if (class == CborAny || class == CborShort)
		{
			if (*bytesBuffered < 2)
			{
				writeMemo("[?] Not enough bytes for Short.");
				return -1;
			}

			sum = **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			*value = sum;
			*bytesBuffered -= 2;
			return 2;
		}

		writeMemo("[?] CBOR error: got Short value.");
		return -1;

	case 26:
		if (class == CborAny || class == CborInt)
		{
			if (*bytesBuffered < 4)
			{
				writeMemo("[?] Not enough bytes for Int.");
				return -1;
			}

			sum = **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			*value = sum;
			*bytesBuffered -= 4;
			return 4;
		}

		writeMemo("[?] CBOR error: got Int value.");
		return -1;

	case 27:
		if (class == CborAny || class == CborVast)
		{
			if (*bytesBuffered < 8)
			{
				writeMemo("[?] Not enough bytes for Vast.");
				return -1;
			}

			sum = **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*cursor += 1;
			*value = sum;
			*bytesBuffered -= 8;
			return 8;
		}

		writeMemo("[?] CBOR error: got Vast value.");
		return -1;

	default:
		writeMemoNote("[?] CBOR error: invalid integer class",
				itoa(additionalInfo));
		return -1;
	}
}

int	cbor_decode_integer(uvast *value, int class, unsigned char **cursor,
		unsigned int *bytesBuffered)
{
	int	majorType;
	int	additionalInfo;
	int	length;

	CHKZERO(value && cursor && *cursor && bytesBuffered);
	if (decodeFirstByte(cursor, bytesBuffered, &majorType, &additionalInfo)
			< 1)
	{
		return 0;
	}

	if (majorType != CborUnsignedInteger)
	{
		writeMemo("[?] CBOR error: not integer.");
		return 0;
	}

	length = decodeInteger(value, class, additionalInfo, cursor,
			bytesBuffered);
	if (length < 0)
	{
		writeMemo("[?] CBOR integer decode failed.");
		return 0;
	}

	return 1 + length;
}

int	cbor_decode_byte_string(unsigned char *value, uvast *size,
		unsigned char **cursor, unsigned int *bytesBuffered)
{
	int	majorType;
	int	additionalInfo;
	int	length;
	uvast	stringLength;

	CHKZERO(cursor && *cursor && size && bytesBuffered);
	if (decodeFirstByte(cursor, bytesBuffered, &majorType, &additionalInfo)
			< 1)
	{
		return 0;
	}

	if (majorType != CborByteString)
	{
		writeMemo("[?] CBOR error: not byte string.");
		return 0;
	}

	if (additionalInfo < 24)
	{
		length = 0;
		stringLength = additionalInfo;
	}
	else
	{
		length = decodeInteger(&stringLength, CborAny, additionalInfo,
				cursor, bytesBuffered);
		if (length < 0)
		{
			writeMemo("[?] CBOR byte string decode failed.");
			return 0;
		}
	}

	if (stringLength > *size)
	{
		writeMemoNote("[?] CBOR byte string too long",
				itoa(stringLength));
		return 0;
	}

	/*	Cursor has been advanced to the beginning of the
	 *	text string at this point.				*/

	*size = stringLength;
	if (value)	/*	Okay to copy bytes into buffer.		*/
	{
		memcpy(value, *cursor, stringLength);
		*cursor += stringLength;
		*bytesBuffered -= stringLength;
	}
	else
	{
		stringLength = 0;
	}

	return 1 + (length + stringLength);
}

int	cbor_decode_text_string(char *value, uvast *size,
		unsigned char **cursor, unsigned int *bytesBuffered)
{
	int	majorType;
	int	additionalInfo;
	int	length;
	uvast	stringLength;

	CHKZERO(cursor && *cursor && size && bytesBuffered);
	if (decodeFirstByte(cursor, bytesBuffered, &majorType, &additionalInfo)
			< 1)
	{
		return 0;
	}

	if (majorType != CborTextString)
	{
		writeMemo("[?] CBOR error: not text string.");
		return 0;
	}

	if (additionalInfo < 24)
	{
		length = 0;
		stringLength = additionalInfo;
	}
	else
	{
		length = decodeInteger(&stringLength, CborAny, additionalInfo,
				cursor, bytesBuffered);
		if (length < 0)
		{
			writeMemo("[?] CBOR text string decode failed.");
			return 0;
		}
	}

	if (stringLength > *size)
	{
		writeMemoNote("[?] CBOR text string too long",
				itoa(stringLength));
		return 0;
	}

	/*	Cursor has been advanced to the beginning of the
	 *	text string at this point.				*/

	*size = stringLength;
	if (value)	/*	Okay to copy bytes into buffer.		*/
	{
		memcpy(value, *cursor, stringLength);
		*cursor += stringLength;
		*bytesBuffered -= stringLength;
	}
	else
	{
		stringLength = 0;
	}

	return 1 + (length + stringLength);
}

int	cbor_decode_array_open(uvast *size, unsigned char **cursor,
		unsigned int *bytesBuffered)
{
	int	majorType;
	int	additionalInfo;
	int	length;
	uvast	arrayLength;

	CHKZERO(cursor && *cursor && size && bytesBuffered);
	if (decodeFirstByte(cursor, bytesBuffered, &majorType, &additionalInfo)
			< 1)
	{
		return 0;
	}

	if (majorType != CborArray)
	{
		writeMemo("[?] CBOR error: not array.");
		return 0;
	}

	if (additionalInfo == 31)	/*	Indefinite length.	*/
	{
		if (*size == 0			/*	Any size okay.	*/
		|| *size == ((uvast) -1))	/*	Req indefinite.	*/
		{
			*size = ((uvast) -1);
			return 1;
		}
	}

	/*	This is an array of definite length.			*/

	if (*size == ((uvast) -1))	/*	Req. indefinite-length.	*/
	{
		writeMemo("[?] CBOR array size is not indefinite.");
		return 0;
	}

	/*	An array of definite length is expected.		*/

	length = decodeInteger(&arrayLength, CborAny, additionalInfo, cursor,
			bytesBuffered);
	if (length < 0)
	{
		writeMemo("[?] CBOR array decode failed.");
		return 0;
	}

	if (*size == 0)			/*	Any size is okay.	*/
	{
		*size = arrayLength;
		return 1 + length;
	}

	/*	Array size must match.					*/

	if (arrayLength == *size)
	{
		return 1 + length;	/*	Size is correct.	*/
	}

	writeMemoNote("[?] CBOR array size is wrong", itoa(arrayLength));
	return 0;
}

int	cbor_decode_break(unsigned char **cursor, unsigned int *bytesBuffered)
{
	int	majorType;
	int	additionalInfo;

	CHKZERO(cursor && *cursor && bytesBuffered);
	decodeFirstByte(cursor, bytesBuffered, &majorType, &additionalInfo);
	if (majorType != CborSimpleValue)
	{
		writeMemo("[?] CBOR error: not 'simple' value.");
		return 0;
	}

	if (additionalInfo != 31)
	{
		writeMemo("[?] CBOR error: not a break code.");
		return 0;
	}

	return 1;			/*	Only initial byte.	*/
}
