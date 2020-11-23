/*
	libtc.c:	common functions for all participants in
			any Trusted Collective deployment.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tc.h"

int	tc_serialize(char *buffer, unsigned int buflen,
		uvast nodeNbr, time_t effectiveTime,
		time_t assertionTime, unsigned short datLength,
		unsigned char *datValue)
{
	int	length = 0;
	char	*cursor;
	Sdnv	nodeNbrSdnv;
	uint32_t u4;
	uint16_t u2;

	CHKERR(buffer);
	CHKERR(buflen);
	cursor = buffer;
	encodeSdnv(&nodeNbrSdnv, nodeNbr);
	CHKERR(buflen > nodeNbrSdnv.length + 14 + datLength);
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
		CHKERR(datValue);
		memcpy(cursor, datValue, datLength);
		length += datLength;
	}

	return length;
}

int	tc_deserialize(char **cursor, int *bytesRemaining,
		unsigned short maxDatLength, uvast *nodeNbr,
		time_t *effectiveTime, time_t *assertionTime,
		unsigned short *datLength, unsigned char *datValue) 
{
	int	originalBytesRemaining;
	uint32_t u4;
	uint16_t u2;
	int	length;

	CHKERR(cursor);
	CHKERR(bytesRemaining);
	originalBytesRemaining = *bytesRemaining;
	CHKERR(nodeNbr);
	CHKERR(effectiveTime);
	CHKERR(assertionTime);
	CHKERR(datLength);
	CHKERR(datValue);
	extractSdnv(nodeNbr, cursor, bytesRemaining);
	if (*bytesRemaining < 10)	/*	Times and data length.	*/
	{
		writeMemo("Malformed TC record: too short.");
		return 0;
	}

	memcpy((char *) &u4, *cursor, 4);
	*cursor += 4;
	*bytesRemaining -= 4;
	u4 = ntohl(u4);
	*effectiveTime = u4;

	memcpy((char *) &u4, *cursor, 4);
	*cursor += 4;
	*bytesRemaining -= 4;
	u4 = ntohl(u4);
	*assertionTime = u4;

	memcpy((char *) &u2, *cursor, 2);
	*cursor += 2;
	*bytesRemaining -= 2;
	u2 = ntohs(u2);
	*datLength = u2;
	if (*datLength > maxDatLength)
	{
		writeMemoNote("Malformed TC record: data too long",
				utoa(*datLength));
		return 0;
	}

	if (*bytesRemaining < *datLength)
	{
		writeMemo("Malformed TC record: data truncated.");
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
