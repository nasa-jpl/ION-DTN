/*
 *  beacon.h -- DTN IP Neighbor Discovery (IPND). Beacon related functions.
 *  These functions allow to manage IPND beacons. *
 *  
 *	Copyright (c) 2015, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *	Author: Gerard Garcia, TrePe
 *	Version 1.0 2015/05/09 Gerard Garcia
 *  Version 2.0 DTN Neighbor Discovery - ION IPND Implementation Assembly Part2
 */

#ifndef _BEACON_H_
#define _BEACON_H_

#include "ipndP.h"
#include "bloom.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Beacon flags */
#define BEAC_SOURCE_EID_PRESENT		(0) 	/*	00000001	*/
#define BEAC_SERVICE_BLOCK_PRESENT	(1)	/*	00000010	*/
#define BEAC_NBF_PRESENT		(2)	/*	00000100	*/
#define BEAC_PERIOD_PRESENT		(3)	/*	00001000	*/

/* Beacon structure. */
typedef struct
{
	unsigned char	version;
	unsigned char	flags;
	unsigned int	sequenceNumber;
	char		canonicalEid[MAX_EID_LEN];
	Lyst		services; // list of ServiceDefinitions
	int		period;
	struct bloom	bloom;
} Beacon;

char *logBeacon(Beacon *beacon);
int beaconChanged(Beacon *beacon, const int period);
int populateBeacon(Beacon *beacon, const int period);
int serializeBeacon(Beacon *beacon, unsigned char **rawBeacon);
int deserializeBeacon(unsigned char *rawBeacon, const int rawBeaconLength,
                      Beacon *deserializedBeacon);
void clearBeacon(Beacon* beacon);
void copyBeacon(Beacon* dest, Beacon* src);

#ifdef __cplusplus
}
#endif

#endif	/* _BEACON_H_ */
