/*
	libdtka.c:	common functions for DTKA implementation.

	Author: Scott Burleigh, JPL

	Copyright (c) 2013, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "dtka.h"

int	dtka_serialize(unsigned char *buffer, unsigned int buflen,
		uvast nodeNbr, time_t effectiveTime,
		time_t assertionTime, unsigned short datLength,
		unsigned char *datValue)
{
	int		length = 0;
	unsigned char	*cursor;
	Sdnv		nodeNbrSdnv;
	unsigned int	u4;
	unsigned short	u2;

	CHKZERO(buffer);
	CHKZERO(buflen);
	CHKZERO(nodeNbr);
	cursor = buffer;
	encodeSdnv(&nodeNbrSdnv, nodeNbr);
	CHKZERO(buflen > nodeNbrSdnv.length + 14 + datLength);
	memcpy(cursor, nodeNbrSdnv.text, nodeNbrSdnv.length);
	cursor += nodeNbrSdnv.length;
	length += nodeNbrSdnv.length;
	u4 = effectiveTime;
	u4 = htonl(u4);
	memcpy(cursor, (char *) &u4, 4);
	cursor += 4;
	length += 4;
	u4 = assertionTime;
	u4 = htonl(u4);
	memcpy(cursor, (char *) &u4, 4);
	cursor += 4;
	length += 4;
	u2 = datLength;
	u2 = htons(u2);
	memcpy(cursor, (char *) &u2, 2);
	cursor += 2;
	length += 2;
	if (datLength > 0)
	{
		CHKZERO(datValue);
		memcpy(cursor, datValue, datLength);
		length += datLength;
	}

	return length;
}

int	dtka_deserialize(unsigned char **cursor, int *bytesRemaining,
		unsigned short maxDatLength, uvast *nodeNbr,
		time_t *effectiveTime, time_t *assertionTime,
		unsigned short *datLength, unsigned char *datValue) 
{
	int	originalBytesRemaining;
	int	length;

	CHKERR(cursor);
	CHKERR(bytesRemaining);
	originalBytesRemaining = *bytesRemaining;
	CHKERR(nodeNbr);
	CHKERR(effectiveTime);
	CHKERR(datLength);
	CHKERR(datValue);
	extractSdnv(nodeNbr, cursor, bytesRemaining);
	if (*bytesRemaining < 10)	/*	Times and key length.	*/
	{
		writeMemo("Malformed DTKA record: too short.");
		return 0;
	}

	memcpy((char *) effectiveTime, *cursor, 4);
	*cursor += 4;
	*bytesRemaining -= 4;
	*effectiveTime = ntohl(*effectiveTime);
	memcpy((char *) assertionTime, *cursor, 4);
	*cursor += 4;
	*bytesRemaining -= 4;
	*assertionTime = ntohl(*assertionTime);
	memcpy((char *) datLength, *cursor, 2);
	*cursor += 2;
	*bytesRemaining -= 2;
	*datLength = ntohs(*datLength);
	if (*datLength > maxDatLength)
	{
		writeMemoNote("Malformed DTKA record: key is too long",
				utoa(*datLength));
		return 0;
	}

	if (*bytesRemaining < *datLength)
	{
		writeMemo("Malformed DTKA record: key truncated.");
		return 0;
	}

	if (*datLength > 0)
	{
		memcpy(datValue, *cursor, *datLength);
		*cursor += *datLength;
		*bytesRemaining -= *datLength;
	}

	length = originalBytesRemaining - *bytesRemaining;
	return length;
}
