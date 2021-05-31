/*
eclso_ext.c

 Author: Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file implements protocol adapter extension for HSLTP mode

 * */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include "ltpP.h"
#include "bpP.h"
#include "HSLTP.h"
#include "../../elements/sys/eclsaLogger.h"
#include "../../adapters/protocol/eclsaProtocolAdapters.h"
#include "../../elements/sys/eclsaMemoryManager.h"
//#include "../../eclso.h"
//#include "../../elements/matrix/eclsaCodecMatrix.h"

#define LtpUdpDefaultPortNbr		1113

void initEclsoUpperLevel_HSLTP_MODE(int argc, char *argv[], EclsoEnvironment *env)
{
	// Same as ECLSA
	initEclsoUpperLevel(argc, argv, env);
}

void sendSegmentToUpperProtocol_HSLTP_MODE		(char *buffer,int *bufferLength,int abstractCodecStatus,bool isFirst)
{
	// Same as ECLSA
	sendSegmentToUpperProtocol(buffer, bufferLength);
}
int receiveSegmentFromUpperProtocol_HSLTP_MODE(char *buffer,int *bufferLength)
{
	// Same as ECLSA
        receiveSegmentFromUpperProtocol(buffer, bufferLength);

        // First byte is version + type flag
        // Get the type flag using mask 0x0F
        int segmentType = (*buffer) & 0x0F;

        switch ( segmentType ) {
        case 0:
        case 1:
        case 2:
        case 4:
        case 5:
                return DEFAULT_PROC;

        case 3:
        case 6:
        case 7:
                return END_OF_MATRIX;

        case 8:
        case 9:
        case 12:
        case 13:
        case 14:
        case 15:
                return SPECIAL_PROC;

        case 10:
        case 11:
	default:
                return ERROR_PROC;
        }
}
