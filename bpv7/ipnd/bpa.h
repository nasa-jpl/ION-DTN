/*
 *	bpa.h -- DTN IP Neighbor Discovery (IPND). Main IPND threads. Include:
 *	Send beacons thread.
 *	Receive beacon thread.
 *	Expire neighbors thread.
 *  
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *	Version 2.0 DTN Neighbor Discovery 
 *		- ION IPND Implementation Assembly Part2
 */

#ifndef _NODE_H_
#define _NODE_H_

#define IPND_DEBUG	0

#include "helper.h"
#include "beacon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPND_VERSION2 0x02
#define IPND_VERSION4 0x04

/* Neighbor link. */
typedef struct
{
	int	state;
	char	bidirectional;
} Link;

/* Neighbor link state. */
enum	LinkStates
{
	UP = 1,
	DOWN
};

/* Beacon destination and its context. */
typedef struct
{
	NetAddress	addr;
	char		eid[MAX_EID_LEN];
	Beacon		beacon;

	int		fixed;
	int		beaconInitialized;
	int		announcePeriod;
	long		nextAnnounceTimestamp;
} Destination;

/* IPND neighbor. */
typedef struct
{
	NetAddress	addr;
	Beacon		beacon;
	time_t		beaconReceptionTime;
	Link		link;
} IpndNeighbor;

extern void	*sendBeacons(void *attr);
extern void	*receiveBeacons(void *attr);
extern void	*expireNeighbors(void *attr);

#ifdef __cplusplus
}
#endif

#endif
