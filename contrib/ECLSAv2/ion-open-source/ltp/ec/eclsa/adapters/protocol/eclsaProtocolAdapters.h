/*
eclsaProtocolAdapters.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions of the adapter between
eclsi|eclso and the upper and lower protocols.

 */

#ifndef _ECLSA_PROT_ADAPTER_H_
#define _ECLSA_PROT_ADAPTER_H_

#include <stdbool.h>
#include "../../eclso_def.h"
#include "../../eclsi_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FEC_LOWERLEVEL_MAX_PACKET_SIZE 				((256 * 256) - 1)

/* upper protocol interface*/
void initEclsoUpperLevel(int argc, char *argv[], EclsoEnvironment *env);
void initEclsiUpperLevel(int argc, char *argv[], EclsiEnvironment *env);
void sendSegmentToUpperProtocol		(char *buffer,int *bufferLength);
void receiveSegmentFromUpperProtocol	(char *buffer,int *bufferLength);

/* lower protocol interface*/
void initEclsoLowerLevel(int argc, char *argv[], EclsoEnvironment *env);
void initEclsiLowerLevel(int argc, char *argv[], EclsiEnvironment *env);
void receivePacketFromLowerProtocol	(char *buffer,int *bufferLength, void **lowerLevelProtocolData, unsigned int *lowerLevelProtocolDataSize);
void sendPacketToLowerProtocol	(char *buffer, int *bufferLength, void *lowerLevelProtocolData);


#ifdef __cplusplus
}
#endif

#endif	/* _ECLSA_PROT_ADAPTER_H_ */
