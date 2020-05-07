/*
 *	helper.c -- DTN IP Neighbor Discovery (IPND). IPND miscellaneous
 *	helper functions.  Includes functions used in several parts of IPND.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery
 *		- ION IPND Implementation Assembly Part2
 *	Version 2.1 DTN Neighbor Discovery - ION IPND Fix Defects and Issues
 */

#include <stdint.h> 
#include "platform.h"
#include "eureka.h"
#include "helper.h"
#include "bpa.h"
#include "ipndP.h"

static int	toBinary(const char *string, char *buf)
{
	struct hostent	*hostInfo;

	CHKZERO(string);
	CHKZERO(buf);
	hostInfo = gethostbyname(string);
	if (hostInfo == NULL)
	{
		return 0;
	}

	memcpy(buf, hostInfo->h_addr, 4);
	return 1;
}

static void	toChar(unsigned char *data, char *buf, int buflen)
{
	unsigned int	hostNbr;

	CHKVOID(data);
	CHKVOID(buf);
	CHKVOID(buflen > 15);
	memcpy((char *) &hostNbr, data, 4);
	hostNbr = ntohl(hostNbr);
	printDottedString(hostNbr, buf);
}

/**
 * Handles echo state.
 * @param  newValue New echo state.
 * @return          Echo state.
 */
static int	_echo(int *newValue)
{
	static int	state = 1;

	if (newValue)
	{
		if (*newValue == 0)
		{
			state = 0;
		}
		else
		{
			state = 1;
		}
	}

	return state;
}

/**
 * Print text to console. If echo is enabled it echoes
 * text to the log file.
 * @param text Text to print and/or log.
 */
void	printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

/**
 * Switch if messages should be echoed to log or not.
 * @param tokenCount Number of tokens in tokens.
 * @param tokens     Command tokens.
 */
void	switchEcho(int tokenCount, char **tokens)
{
	int	state;

	if (tokenCount < 2)
	{
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		state = 0;
		break;

	case '1':
		state = 1;
		break;

	default:
		printText("Echo on or off?");
		return;
	}

	oK(_echo(&state));
}

/**
 * Checks if a node has an active connection with node eid.
 * @param  eid    Eid of node to check.
 * @param  period Beacon period to Eid.
 * @return 1 if there is an active connection to this node, 0 otherwise.
 */
int	hasAnActiveConnection(char *eid, int period)
{
	PsmPartition	bpwm = getIonwm();
	PsmAddress	discoveryElt;
	Discovery	*discovery;

	discoveryElt = bp_find_discovery(eid);
	if (discoveryElt)
	{
		discovery = (Discovery *) psp(bpwm, sm_list_data(bpwm,
				discoveryElt));
		if (discovery->lastContactTime + period > getCtime())
		{
			return 1;
		}
	}

	return 0;
}

/**
 * Gets address type.
 * @param  ip Ip of address to determine type.
 * @return    Address type: UNICAST, BROADCAST, MULTICAST
 *            -1 on unsupported address.
 */
int	getIpv4AddressType(const char *ip)
{
	unsigned char	binaryAddr[4];

	/* Convert address to binary */
	if (!toBinary(ip, (char *) binaryAddr))
	{
		return -1;
	}

	/* 0xFF == 0b1111 */
	/* 0xE == 0b1110 */

	if (binaryAddr[0] & 0xFF &&
			binaryAddr[1] & 0xFF &&
			binaryAddr[2] & 0xFF &&
			binaryAddr[3] & 0xFF)
	{
		return BROADCAST;
	}
	else if ((binaryAddr[0]  >> 4 & 0xE) == 0xE)
	{
		if (binaryAddr[1] == 0 && binaryAddr[2] == 0)
		{
			return MULTICAST;
		}
		else
		{
			putErrmsg("Multicast address not in IPND spec.", NULL);
			return -1;
		}
	}
	else
	{
		return UNICAST;
	}
}

/**
 * Finds an address with ip <ip> in addresses lyst (lyst of NetAddress)
 * @param  ip        Ip to look for.
 * @param  addresses lyst of NetAddress structs
 * @return           NetADdress if found, NULL if not found.
 */
NetAddress	*findAddr(const char *ip, Lyst addresses)
{
	int		i;
	LystElt		addrElt;
	NetAddress	*addr;

	CHKNULL(addresses);
	addrElt = lyst_first(addresses);
	for (i = 0; i < lyst_length(addresses); i++)
	{
		addr = (NetAddress *) lyst_data(addrElt);
		if (strncasecmp(addr->ip, ip, INET_ADDRSTRLEN) == 0)
		{
			break;
		}

		addrElt = lyst_next(addrElt);
	}

	if (addrElt != NULL && addrElt != lyst_last(addresses))
	{
		return lyst_data(addrElt);
	}

	return NULL;
}

/**
 * Determines order of two IpndNeighbor structs.
 * @param  data1 First IpndNeighbor.
 * @param  data2 Sedond IpndNeighbor.
 * @return  1 if nb data1 should be first,
 *          0 if the two neighbors are equal.
 *          -1 if nb data2 should be first.
 */
int	compareIpndNeighbor(void *data1, void *data2)
{
	IpndNeighbor	*nb1 = data1;
	IpndNeighbor	*nb2 = data2;
	int		addrCmp;

	addrCmp = strncasecmp(nb1->addr.ip, nb2->addr.ip, INET_ADDRSTRLEN);
	if (addrCmp == 0)
	{
		if (nb1->addr.port == nb2->addr.port)
		{
			return 0;
		}
		else if (nb1->addr.port > nb2->addr.port)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	return addrCmp;
}

/**
 * Finds an ip neighbor with ip <ip> and port <port> in a lyst
 * of IpndNeighbor structs.
 * @param  ip        Ip to look for.
 * @param  port      Port to look for.
 * @param  neighbors Lyst of IpndNeighbors structs.
 * @return           LystElt if found, NULL otherwise.
 */
LystElt	findIpndNeighbor(const char *ip, const int port, Lyst neighbors)
{
	IpndNeighbor	nb;

	if (lyst_length(neighbors) == 0)
	{
		return NULL;
	}

	istrcpy(nb.addr.ip, ip, INET_ADDRSTRLEN);
	nb.addr.port = port;
	return lyst_search(lyst_first(neighbors), (void *) &nb);
}

/**
 * Determines order of two Destination structs.
 * @param  data1 First Destiantion.
 * @param  data2 Sedond Destiantion.
 * @return  1 if Destination data1 should be first,
 *          0 if the two Destination structs are equal.
 *          -1 if Destination data2 should be first.
 */
int	compareDestination(void *data1, void *data2)
{
	Destination	*dest1 = (Destination *) data1;
	Destination	*dest2 = (Destination *) data2;

	if (dest1->nextAnnounceTimestamp < dest2->nextAnnounceTimestamp)
	{
		return -1;
	}

	if (dest1->nextAnnounceTimestamp > dest2->nextAnnounceTimestamp)
	{
		return 1;
	}

	return 0;
}


/**
 * Finds a Destination with NetAddress <addr> in a lyst
 * of Destination structs.
 * @param  addr         Netaddr to look for.
 * @param  destinations Lyst of Destination structs.
 * @return LystElt if found, NULL otherwise.
 */
LystElt	findDestinationByAddr(NetAddress *addr, Lyst destinations)
{
	int		i;
	LystElt		destinationElt;
	Destination	*dest;

	CHKNULL(destinations);

	destinationElt = lyst_first(destinations);
	for (i = 0; i < lyst_length(destinations); i++)
	{
		dest = (Destination *) lyst_data(destinationElt);
		if (strncasecmp(dest->addr.ip, addr->ip, INET_ADDRSTRLEN) == 0
		&& dest->addr.port == addr->port)
		{
			break;
		}

		destinationElt = lyst_next(destinationElt);
	}

	return destinationElt;
}

/**
 * Releases the contents of a Lyst.
 * @param  lyst Lyst to release.
 */
void	releaseLystElements(Lyst lyst)
{
	int	i;
	LystElt	lystElt;

	if (lyst == NULL)
	{
		return;
	}

	lystElt = lyst_first(lyst);
	for (i = 0; i < lyst_length(lyst); i++)
	{
		MRELEASE(lyst_data(lystElt));
		lystElt = lyst_next(lystElt);
	}
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToBooleanBytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 1)
	{
		return -1;
	}

	*buf = (strcasecmp(str, "true") == 0 || strtol(str, NULL, 0));
	return 1;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToUint64Bytes(char *str, char *buf, int maxLen)
{
	static Sdnv	sdnvTmp;

	encodeSdnv(&sdnvTmp, strtouvast(str));
	if (maxLen < sdnvTmp.length) return -1;
	memcpy(buf, sdnvTmp.text, sdnvTmp.length);
	return sdnvTmp.length;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToSint64Bytes(char *str, char *buf, int maxLen)
{
	static Sdnv	sdnvTmp;

	encodeSdnv(&sdnvTmp, strtovast(str));
	if (maxLen < sdnvTmp.length) return -1;
	memcpy(buf, sdnvTmp.text, sdnvTmp.length);
	return sdnvTmp.length;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToFixed16Bytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 2) return -1;
	uint16_t i = htons(strtouvast(str));
	memcpy(buf, &i, 2);
	return 2;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToFixed32Bytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 4) return -1;
	uint32_t i = htonl(strtouvast(str));
	memcpy(buf, &i, 4);
	return 4;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToFixed64Bytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 8) return -1;
	uvast i = strtouvast(str);
#ifndef RTEMS
	i = (1 == htonl(1) ? (i)
		: ((uvast)htonl(i & 0xFFFFFFFF) << 32) | htonl(i >> 32));
#endif
	memcpy(buf, &i, sizeof i);
	return sizeof i;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToFloatBytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 4 || sizeof(float) != 4) return -1;

	float		f = (float) strtod(str, NULL);
	uint32_t	i;

	memcpy(&i, &f, 4);
	i = htonl(i);
	memcpy(buf, &i, 4);
	return 4;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToDoubleBytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 8 || sizeof(double) != 8) return -1;

	double	f = strtod(str, NULL);

#ifdef RTEMS
	memcpy(buf, &f, 8);
#else
	uvast	i;
	memcpy(&i, &f, 8);
	i = (1 == htonl(1) ? (i)
		: ((uvast)htonl(i & 0xFFFFFFFF) << 32) | htonl(i >> 32));
	memcpy(buf, &i, 8);
#endif
	return 8;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToStringBytes(char *str, char *buf, int maxLen)
{
	static Sdnv	sdnvTmp;
	static int	len;

	/* Internet-Draft DTN-IPND November 2012
	 * Note that a special case exists for representing the empty string and
	 * the empty byte array for the "string" and "bytes" data types,
	 * respectively. In both cases, "empty" is represented by an explicit
	 * length value of 1 and content of a single null byte. */

	if (str[0] == '\0')
	{
		if (maxLen < 2) return -1;
		buf[0] = 1;
		buf[1] = 0;
		return 2;
	}

	len = strlen(str);
	encodeSdnv(&sdnvTmp, len);
	if (sdnvTmp.length + len > maxLen) return -1;
	memcpy(buf, sdnvTmp.text, sdnvTmp.length);
	memcpy(buf + sdnvTmp.length, str, len);
	return sdnvTmp.length + len;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringToBytesBytes(char *str, char *buf, int maxLen)
{
	static char	val[] = "0x00";
	static Sdnv	sdnvTmp;
	static int	len, i;

	if (str[0] == '\0')
	{
		if (maxLen < 2) return -1;
		buf[0] = 1;
		buf[1] = 0;
		return 2;
	}

	len = (strlen(str) + 1) / 2; // there can be odd number of hex digits
	encodeSdnv(&sdnvTmp, len);
	if (sdnvTmp.length + len > maxLen) return -1;

	memcpy(buf, sdnvTmp.text, sdnvTmp.length);
	for (i = 0; i < len; i++)
	{
		val[2] = str[2*i];
		val[3] = str[2*i+1];
		*(buf + sdnvTmp.length + i) = strtol(val, NULL, 16);
	}

	return sdnvTmp.length + len;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringIP4ToFixed32Bytes(char *str, char *buf, int maxLen)
{
	if (maxLen < 4 || toBinary(str, buf) != 1) return -1;
	return 4;
}

/* Parse human readable string into IPND protocol bytes
 * @param str Human readable string
 * @param buf Where to write bytes
 * @param maxLen Capacity of buf
 * @return number of bytes written or -1 on error
 */
int	stringIP6ToBytesBytes(char *str, char *buf, int maxLen)
{
	/*	No portable support for IPV6 at this time.		*/

	if (maxLen < 1 + 16) return -1;
	buf[0] = 16;
	memset(buf + 1, 0, 16);
	return 17;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToBooleanString(unsigned char *data, char *buf, int maxLen)
{
	isprintf(buf, maxLen, "%s", (*data) ? "true" : "false");
	return 1;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToUint64String(unsigned char *data, char *buf, int maxLen)
{
	uvast	ret;
	int	len = decodeSdnv(&ret, data);

	isprintf(buf, maxLen, UVAST_FIELDSPEC, ret);
	return len;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToSint64String(unsigned char *data, char *buf, int maxLen)
{
	uvast	ret;
	int	len = decodeSdnv(&ret, data);

	isprintf(buf, maxLen, VAST_FIELDSPEC, (vast) ret);
	return len;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToFixed16String(unsigned char *data, char *buf, int maxLen)
{
	uint16_t	ret;

	memcpy(&ret, data, 2);
	isprintf(buf, maxLen, "%u", ntohs(ret));
	return 2;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToFixed32String(unsigned char *data, char *buf, int maxLen)
{
	uint32_t	ret;

	memcpy(&ret, data, 4);
	isprintf(buf, maxLen, "%u", ntohl(ret));
	return 4;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToFixed64String(unsigned char *data, char *buf, int maxLen)
{
	uvast	ret;

	memcpy(&ret, data, sizeof ret);
#ifndef RTEMS
	ret = (1 == htonl(1) ? ret
		: ((uvast)ntohl(ret & 0xFFFFFFFF) << 32) | ntohl(ret >> 32));
#endif
	isprintf(buf, maxLen, UVAST_FIELDSPEC, ret);
	return 8;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToFloatString(unsigned char *data, char *buf, int maxLen)
{
	uint32_t	i;
	float		ret;

	memcpy(&i, data, 4);
	i = ntohl(i);
	memcpy(&ret, &i, 4);
	isprintf(buf, maxLen, "%f", ret);
	return 4;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToDoubleString(unsigned char *data, char *buf, int maxLen)
{
	double	ret;

#ifdef RTEMS
	memcpy(&ret, data, 8);
#else
	uvast	i;
	memcpy(&i, data, 8);
	i = (1 == htonl(1) ? (i)
		: ((uvast)ntohl(i & 0xFFFFFFFF) << 32) | ntohl(i >> 32));
	memcpy(&ret, &i, 8);
#endif
	isprintf(buf, maxLen, "%lf", ret);
	return 8;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToStringString(unsigned char *data, char *buf, int maxLen)
{
	uvast	len, sdnvLength;

	/* Internet-Draft DTN-IPND November 2012
	 * Note that a special case exists for representing the empty string and
	 * the empty byte array for the "string" and "bytes" data types,
	 * respectively. In both cases, "empty" is represented by an explicit
	 * length value of 1 and content of a single null byte. */

	if (data[0] == 1 && data[1] == 0)
	{
		return 2;
	}

	sdnvLength = decodeSdnv(&len, data);
	if (maxLen >= len + 1)
	{
		memcpy(buf, data + sdnvLength, len);
		buf[len] = '\0';
	}

	return sdnvLength + len;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesToBytesString(unsigned char *data, char *buf, int maxLen)
{
	uvast		len, sdnvLength;
	static char	hex[] = "0123456789ABCDEF";
	int		i;

	if (data[0] == 1 && data[1] == 0)
	{
		return 2;
	}

	sdnvLength = decodeSdnv(&len, data);
	if (maxLen >= len * 2 + 1)
	{
		for (i = 0; i < len; i++)
		{
			buf[2 * i] = hex[data[i+sdnvLength] >> 4];
			buf[2 * i + 1] = hex[data[i+sdnvLength] & 0x0f];
		}

		buf[len * 2] = '\0';
	}

	return sdnvLength + len;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesIP4ToFixed32String(unsigned char *data, char *buf, int maxLen)
{
	toChar(data, buf, maxLen);
	return 4;
}

/* Change IPND protocol bytes into human readable string
 * @param data Bytes to convert
 * @param buf Where to write human readable string
 * @param maxLen Capacity of buf
 * @return number of bytes read
 */
int	bytesIP6ToBytesString(unsigned char *data, char *buf, int maxLen)
{
	/* IP6 is encoded as byte array	*/

	if (data[0] != 16)
	{
		return bytesToBytesString(data, buf, maxLen);
	}

	/*	No portable support for IPV6 at this time.		*/

	memset(buf, 0, maxLen);
	return 1 + 16;
}
