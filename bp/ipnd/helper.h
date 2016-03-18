/*
 *	helper.h -- DTN IP Neighbor Discovery (IPND). IPND miscellaneous
 *	helper functions.
 *	Includes functions used in several parts of IPND.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery
 *		- ION IPND Implementation Assembly Part2
 */

#ifndef _HELPER_H_
#define _HELPER_H_

#include "lyst.h"
#include "platform.h"
#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CHKCTX(e) if (!e) {putErrmsg("Error Getting IPND context.", NULL); return -1;}

/* Network address. */
typedef struct
{
	char ip[INET_ADDRSTRLEN];
	int port;
} NetAddress;

/* Address type. */
enum	addressType
{
	UNICAST,
	MULTICAST,
	BROADCAST
};

void		switchEcho(int tokenCount, char **tokens);
void		printText(char *text);

int		hasAnActiveConnection(char *eid, int period);
int		getIpv4AddressType(const char *ip);
NetAddress*	findAddr(const char *ip, Lyst addresses);

int		compareIpndNeighbor(void *data1, void *data2);
LystElt		findIpndNeighbor(const char *ip, const int port,
			Lyst neighbors);

int		compareDestination(void *data1, void *data2);
LystElt		findDestinationByAddr(NetAddress *addr, Lyst destinations);

void		releaseLystElements(Lyst lyst);

/* Functions to parse human readable string into IPND protocol bytes */
int	stringToBooleanBytes(char *str, char *buf, int maxLen);
int	stringToUint64Bytes(char *str, char *buf, int maxLen);
int	stringToSint64Bytes(char *str, char *buf, int maxLen);
int	stringToFixed16Bytes(char *str, char *buf, int maxLen);
int	stringToFixed32Bytes(char *str, char *buf, int maxLen);
int	stringToFixed64Bytes(char *str, char *buf, int maxLen);
int	stringToFloatBytes(char *str, char *buf, int maxLen);
int	stringToDoubleBytes(char *str, char *buf, int maxLen);
int	stringToStringBytes(char *str, char *buf, int maxLen);
int	stringToBytesBytes(char *str, char *buf, int maxLen);
int	stringIP4ToFixed32Bytes(char *str, char *buf, int maxLen);
int	stringIP6ToBytesBytes(char *str, char *buf, int maxLen);

/* Functions to convert IPND protocol bytes */
int	bytesToBooleanString(unsigned char *data, char *buf, int maxLen);
int	bytesToUint64String(unsigned char *data, char *buf, int maxLen);
int	bytesToSint64String(unsigned char *data, char *buf, int maxLen);
int	bytesToFixed16String(unsigned char *data, char *buf, int maxLen);
int	bytesToFixed32String(unsigned char *data, char *buf, int maxLen);
int	bytesToFixed64String(unsigned char *data, char *buf, int maxLen);
int	bytesToFloatString(unsigned char *data, char *buf, int maxLen);
int	bytesToDoubleString(unsigned char *data, char *buf, int maxLen);
int	bytesToStringString(unsigned char *data, char *buf, int maxLen);
int	bytesToBytesString(unsigned char *data, char *buf, int maxLen);
int	bytesIP4ToFixed32String(unsigned char *data, char *buf, int maxLen);
int	bytesIP6ToBytesString(unsigned char *data, char *buf, int maxLen);

#ifdef __cplusplus
}
#endif

#endif
