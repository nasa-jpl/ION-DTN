/*
eclsaProtocolAdapter_UP_UniboLTP.c

 Author: Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

 * */

/*
todo: ifdef LW_UDP
	  ifdef UL_ION_LTP
*/
#include "eclsaProtocolAdapters.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include "ltpP.h"
#include "../../eclso.h"

#define LtpUdpDefaultPortNbr		11113

void initEclsoUpperLevel(int argc, char *argv[], EclsoEnvironment *env)
{
	char ownHostName[MAXHOSTNAMELEN];

	// TODO find a way to get the T value dynamically
	env->T = 1024 + sizeof(uint16_t); //2B added for segmentLength & some byte added for LTP header

	unsigned long long ownID;
	read(3, &ownID, sizeof(unsigned long long));
	env->ownEngineId = (unsigned short) ownID;

	// TODO find a way to do this not using ION functions (in order to let this project to be ION indipendent)
	//Getting info for lower layer
	getNameOfHost(ownHostName, MAXHOSTNAMELEN);

	env->portNbr=0;
	env->ipAddress=0;
	parseSocketSpec(argc > 1 ? argv[1] : NULL, &(env->portNbr), &(env->ipAddress));
	if (env->ipAddress == 0)		//	Default to local host.
		env->ipAddress = getInternetAddress(ownHostName);
	if (env->portNbr == 0)
		env->portNbr = LtpUdpDefaultPortNbr;
}

void initEclsiUpperLevel(int argc, char *argv[], EclsiEnvironment *env)
{
	env->portNbr=0;
	env->ipAddress = INADDR_ANY;

	char	*endpointSpec = (argc > 1 ? argv[1] : NULL);

	if (endpointSpec && parseSocketSpec(endpointSpec, &(env->portNbr), &(env->ipAddress)) != 0)
		{
			putErrmsg("Can't get IP/port for endpointSpec.",endpointSpec);
			exit(1);
		}

	if (env->portNbr == 0)
		env->portNbr = LtpUdpDefaultPortNbr;
}

void sendSegmentToUpperProtocol		(char *buffer,int *bufferLength)
{
	// Write the int for size
	write(3, bufferLength, sizeof(*bufferLength));

	write(3, buffer, *bufferLength);
}

void receiveSegmentFromUpperProtocol	(char *buffer,int *bufferLength)
{
	read(3, (void*) bufferLength, sizeof(*bufferLength));

	read(3, buffer, *bufferLength);
}


