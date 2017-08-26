/*
eclsaProtocolAdapters.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions of the adapter between
eclsi|eclso and the upper and lower protocols.

 */
#include "../../elements/eclsaBoolean.h"
#ifndef _ECLSA_PROT_ADAPTER_H_
#define _ECLSA_PROT_ADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FEC_LOWERLEVEL_MAX_PACKET_SIZE 				((256 * 256) - 1)

/* upper protocol interface*/
void initEclsoUpperLevel(int argc, char *argv[], unsigned int *T, unsigned short *ownEngineId,unsigned short *portNbr,unsigned int *ipAddress);
void initEclsiUpperLevel(int argc, char *argv[], unsigned short *portNbr, unsigned int *ipAddress);
void sendSegmentToUpperProtocol		(char *buffer,int *bufferLength);
void receiveSegmentFromUpperProtocol	(char *buffer,int *bufferLength);
void sendSegmentToUpperProtocol_HSLTP_MODE		(char *buffer,int *bufferLength,int abstractCodecStatus,bool isFirst);
int receiveSegmentFromUpperProtocol_HSLTP_MODE	(char *buffer,int *bufferLength);

/* lower protocol interface*/
void initEclsoLowerLevel(int argc, char *argv[], unsigned short portNbr, unsigned int ipAddress, int txbps);
void initEclsiLowerLevel(int argc, char *argv[], unsigned short portNbr, unsigned int ipAddress);
void receivePacketFromLowerProtocol	(char *buffer,int *bufferLength, void **lowerLevelProtocolData, unsigned int *lowerLevelProtocolDataSize);
void sendPacketToLowerProtocol	(char *buffer, int *bufferLength, void *lowerLevelProtocolData);


#ifdef __cplusplus
}
#endif

#endif	/* _ECLSA_PROT_ADAPTER_H_ */
