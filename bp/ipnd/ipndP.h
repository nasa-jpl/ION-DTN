/*
 *	ipndpP.h -- DTN IP Neighbor Discovery (IPND). Initializes IPND context,
 *	loads configuration and launches the main threads.
 *
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery
 *		- ION IPND Implementation Assembly Part2
 *	Version 2.1 DTN Neighbor Discovery - ION IPND Fix Defects and Issues
 *	Version 2.2 Shared context ctx passed explicitely to threads to avoid shared library security change implications
 */

#ifndef _IPND_H_
#define _IPND_H_

#include "lyst.h"
#include "llcv.h"
#include "bpP.h"
#include "ipnfw.h"
#include "dtn2fw.h"
#include "platform.h"
#include "bloom.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tag name length limit, more characters are ignored */
#define IPND_MAX_TAG_NAME_LENGTH 20

/* If the maximum size of a beacon is set to less or equal
	than 1024B, the reception buffer will be allocated in the IPND stack
	otherwise, we will allocate the reception buffer in the ION memory */
#define MAX_BEACON_SIZE 1024

/* NBF parameters */
#define NBF_DEFAULT_CAPACITY 20
#define NBF_DEFAULT_ERROR 0.0001

/* IPND Tag definition */
typedef struct
{
	unsigned char	number;
	char		name[IPND_MAX_TAG_NAME_LENGTH + 1];

	/*	-2: variable,
	 *	-1: explicit,
	 *	0: constructed,
	 *	1..127: fixed 1..127	*/

	signed char	lengthType;

	/*	children tags (params)	*/

	Lyst		children;
} IpndTag;

/* IPND Tag children */
typedef struct
{
	IpndTag	*tag;
	char	name[IPND_MAX_TAG_NAME_LENGTH + 1];
	char	*strVal; /* value in human readable form (from config) */
} IpndTagChild;

/* Service definition structure */
typedef struct
{
	unsigned char	number;
	uvast		dataLength;
	unsigned char	*data; /*	will be parsed as needed,
					contains all the bytes.		*/
} ServiceDefinition;

/* IPND context. Stores IPND configuration and
   IPND variables shared by different threads.
*/
typedef struct
{
	/* IPND structures */
	pthread_t	sendBeaconsThread;
	int		haveSendThread;		/*	Boolean		*/
	pthread_t	receiveBeaconsThread;
	int		haveReceiveThread;	/*	Boolean		*/
	pthread_t	expireNeighborsThread;
	int		haveExpireThread;	/*	Boolean		*/

	/* Configuration */
	char		srcEid[MAX_EID_LEN];
	int		port;
	int		announceEid;
	int		multicastTTL;
	int		enabledBroadcastSending;
	int		enabledBroadcastReceiving;

	/* Determines if period should be announced. */
	int		announcePeriod;

	/* Stores period as UNICAST,MULTICAST and BROADCAST addresses */
	int		announcePeriods[3];

	/* NetAddress lyst, unsorted. */
	Lyst		listenAddresses;
	ResourceLock	configurationLock;
	int		numListenSockets;
	int		*listenSockets;

	/* Defined tags */
	IpndTag		tags[256];

	/* Advertised services */
	Lyst		services;

	/* NBF */
	struct bloom	nbf;

	/* Node structures */

	/* Destination lyst, sorted by nextAnnounceTimestamp. */
	Lyst		destinations;
	Llcv		destinationsCV;
	struct llcv_str	destinationsCV_str;

	/* Neighbor list, sorted by neighbor NetAddress. */
	Lyst		neighbors;
	ResourceLock	neighborsLock;
} IPNDCtx;

/* Updates ctx->nbf as well as NBF-Bits service in ctx->services if present */
extern void	updateCtxNbf(char *eid, int len);

extern IPNDCtx	*getIPNDCtx();
extern void	setIPNDCtx(IPNDCtx *);

#ifdef __cplusplus
}
#endif

#endif
