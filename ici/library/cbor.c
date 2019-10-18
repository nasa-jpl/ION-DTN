/*

	cbor.c:	API for encoding and decoding CBOR elements in ION.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "cbor.h"

/*	CBOR data item types						*/
#define	CborUnsignedInteger	0
#define	CborByteString		2
#define	CborTextString		3
#define	CborArray		4
#define	CborSimpleValue		7

static void	encodeFirstByte(unsigned char **cursor, int majorType,
			int additionalInfo)
{
	unsigned char	byte;

	byte = majorType & 0x07;
	byte = (byte << 5) + (additionalInfo & 0x1f);
	**cursor = byte;
	*cursor += 1;
}

static int	encodeVariableLengthInteger(uvast value, int *additionalInfo,
			unsigned char *cursor)
{
	uvast	residue;
	char	bytes[8];
	int	first;

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

static int	encodeInteger(uvast value, int class, int *additionalInfo,
			unsigned char *cursor)
{
	if (class == CborAny)
	{
		return encodeVariableLengthInteger(value, additionalInfo,
				cursor);
	}

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
		if (value > 4294967295)
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

int	cbor_encode_integer(uvast value, int class, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	length = encodeInteger(value, class, &additionalInfo, bytes);
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

	length = encodeInteger(size, CborAny, &additionalInfo, bytes);
	if (length < 0)
	{
		writeMemoNote("[?] CBOR byte string encode failed",
				itoa(size));
		return 0;
	}

	encodeFirstByte(cursor, CborByteString, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	memcpy(*cursor, value, size);
	*cursor += size;
	return 1 + (length + size);
}

int	cbor_encode_text_string(char *value, uvast size, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	length = encodeInteger(size, CborAny, &additionalInfo, bytes);
	if (length < 0)
	{
		writeMemoNote("[?] CBOR text string encode failed",
				itoa(size));
		return 0;
	}

	encodeFirstByte(cursor, CborTextString, additionalInfo);
	if (length > 0)
	{
		memcpy(*cursor, bytes, length);
		*cursor += length;
	}

	memcpy(*cursor, value, size);
	*cursor += size;
	return 1 + (length + size);
}

int	cbor_encode_array_open(uvast size, unsigned char **cursor)
{
	unsigned char	bytes[8];
	int		additionalInfo;
	int		length;

	if (size == ((uvast) -1))
	{
		additionalInfo = 31;	/*	Indefinite-size array.	*/
	}
	else
	{
		length = encodeInteger(size, CborAny, &additionalInfo, bytes);
		if (length < 0)
		{
			writeMemoNote("[?] CBOR array encode failed",
					itoa(size));
			return 0;
		}
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

void	cbor_decode_initial_byte(unsigned char *cursor, int *majorType,
			int *additionalInfo)
{

	CHKVOID(cursor);
	*majorType = (*cursor) >> 5 & 0x07;
	*additionalInfo = (*cursor) & 0x1f;
}

static void	decodeFirstByte(unsigned char **cursor, int *majorType,
			int *additionalInfo)
{
	cbor_decode_initial_byte(*cursor, majorType, additionalInfo);
	*cursor += 1;
}

static int	decodeInteger(uvast *value, int class, int additionalInfo, 
			unsigned char **cursor)
{
	uvast	sum;

	if (additionalInfo < 24)
	{
		if (class == CborAny || class == CborTiny)
		{
			*value = additionalInfo;
			return 0;
		}

		writeMemo("[?] CBOR error: got tiny value.");
		return -1;
	}

	switch (additionalInfo)
	{
	case 24:
		if (class == CborAny || class == CborChar)
		{
			*value = **cursor;
			*cursor += 1;
			return 1;
		}

		writeMemo("[?] CBOR error: got char.");
		return -1;

	case 25:
		if (class == CborAny || class == CborShort)
		{
			sum = **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			return 2;
		}

		writeMemo("[?] CBOR error: got short.");
		return -1;

	case 26:
		if (class == CborAny || class == CborInt)
		{
			sum = **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			return 4;
		}

		writeMemo("[?] CBOR error: got int.");
		return -1;

	case 27:
		if (class == CborAny || class == CborVast)
		{
			sum = **cursor;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			sum = (sum << 8) + **cursor;
			*value = sum;
			*cursor += 1;
			return 8;
		}

		writeMemo("[?] CBOR error: got uvast.");
		return -1;

	default:
		writeMemoNote("[?] CBOR error: invalid integer type",
				itoa(additionalInfo));
		return -1;
	}
}

int	cbor_decode_integer(uvast *value, int class, unsigned char **cursor)
{
	int	majorType;
	int	additionalInfo;
	int	length;

	CHKERR(cursor && *cursor && value);
	decodeFirstByte(cursor, &majorType, &additionalInfo);
	if (majorType != CborUnsignedInteger)
	{
		writeMemo("[?] CBOR error: not integer.");
		return 0;
	}

	length = decodeInteger(value, class, additionalInfo, cursor);
	if (length < 0)
	{
		writeMemo("[?] CBOR integer decode failed.");
		return 0;
	}

	return 1 + length;
}

int	cbor_decode_byte_string(unsigned char *value, uvast *size,
		unsigned char **cursor)
{
	int	majorType;
	int	additionalInfo;
	int	length;
	uvast	stringLength;

	CHKERR(cursor && *cursor && size);
	decodeFirstByte(cursor, &majorType, &additionalInfo);
	if (majorType != CborByteString)
	{
		writeMemo("[?] CBOR error: not byte string.");
		return 0;
	}

	length = decodeInteger(&stringLength, 0, additionalInfo, cursor);
	if (length < 0)
	{
		writeMemo("[?] CBOR byte string decode failed.");
		return 0;
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
	if (value)
	{
		memcpy(value, *cursor, stringLength);
		*cursor += stringLength;
	}
	else
	{
		stringLength = 0;
	}

	return 1 + (length + stringLength);
}

int	cbor_decode_text_string(char *value, uvast *size,
		unsigned char **cursor)
{
	int	majorType;
	int	additionalInfo;
	int	length;
	uvast	stringLength;

	CHKERR(cursor && *cursor && size);
	decodeFirstByte(cursor, &majorType, &additionalInfo);
	if (majorType != CborTextString)
	{
		writeMemo("[?] CBOR error: not text string.");
		return 0;
	}

	length = decodeInteger(&stringLength, 0, additionalInfo, cursor);
	if (length < 0)
	{
		writeMemo("[?] CBOR text string decode failed.");
		return 0;
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
	if (value)
	{
		memcpy(value, *cursor, stringLength);
		*cursor += stringLength;
	}
	else
	{
		stringLength = 0;
	}

	return 1 + (length + stringLength);
}

int	cbor_decode_array_open(uvast *size, unsigned char **cursor)
{
	int	majorType;
	int	additionalInfo;
	int	length;
	uvast	arrayLength;

	CHKERR(cursor && *cursor && size);
	decodeFirstByte(cursor, &majorType, &additionalInfo);
	if (majorType != CborTextString)
	{
		writeMemo("[?] CBOR error: not array.");
		return 0;
	}

	length = decodeInteger(&arrayLength, 0, additionalInfo, cursor);
	if (length < 0)
	{
		writeMemo("[?] CBOR array decode failed.");
		return 0;
	}

	if (*size == 0)			/*	Any size is okay.	*/
	{
		if (additionalInfo == 31)
		{
			*size = ((uvast) -1);
		}
		else
		{
			*size = arrayLength;
		}

		return length + 1;
	}


	if (*size == ((uvast) -1))	/*	Req. indefinite-length.	*/
	{
		if (additionalInfo == 31)
		{
			return 1;
		}

		writeMemo("[?] CBOR array size is not indefinite.");
		return 0;
	}

	/*	Definite-length array is required.			*/

	if (arrayLength == *size)
	{
		return 1 + length;
	}

	writeMemoNote("[?] CBOR array size is wrong", itoa(arrayLength));
	return 0;
}

int	cbor_decode_break(unsigned char **cursor)
{
	int	majorType;
	int	additionalInfo;

	CHKERR(cursor && *cursor);
	decodeFirstByte(cursor, &majorType, &additionalInfo);
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
