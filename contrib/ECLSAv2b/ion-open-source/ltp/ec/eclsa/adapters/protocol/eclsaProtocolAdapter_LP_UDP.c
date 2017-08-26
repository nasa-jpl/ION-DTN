/*
eclsaProtocolAdapter_LP_UDP.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

 * */

/*
todo: ifdef LW_UDP
	  ifdef UL_ION_LTP
*/

#include "eclsaProtocolAdapters.h"
#include "../../elements/sys/eclsaLogger.h"
#include "platform.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdlib.h>


/* UDP */
typedef struct
{
int 				socketDescriptor;
struct sockaddr		socketName;
struct sockaddr_in	*inetName;
float 				sleepSecPerBit; //eclso only
} UdpEnvironment;

UdpEnvironment udpEnv;
void initEclsoLowerLevel(int argc, char *argv[], unsigned short portNbr, unsigned int ipAddress, int txbps)
{
	//this function initialize UDP for eclso
	udpEnv.sleepSecPerBit = 1.0 / txbps;
	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &udpEnv.socketName, 0, sizeof udpEnv.socketName);
	udpEnv.inetName = (struct sockaddr_in *) &udpEnv.socketName;
	udpEnv.inetName->sin_family = AF_INET;
	udpEnv.inetName->sin_port = portNbr;
	memcpy((char *) &(udpEnv.inetName->sin_addr.s_addr),
			(char *) &ipAddress, 4);

	udpEnv.socketDescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpEnv.socketDescriptor < 0)
	{
		debugPrint("LSO can't open UDP socket");
		exit(1);
	}
}
void initEclsiLowerLevel(int argc, char *argv[], unsigned short portNbr, unsigned int ipAddress)
{
	//this function initialize UDP for eclsi
	socklen_t			nameLength;
	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &udpEnv.socketName, 0, sizeof(udpEnv.socketName));
	udpEnv.inetName = (struct sockaddr_in *) &udpEnv.socketName;
	udpEnv.inetName->sin_family = AF_INET;
	udpEnv.inetName->sin_port = portNbr;
	memcpy((char *) &(udpEnv.inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	udpEnv.socketDescriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpEnv.socketDescriptor < 0)
	{
		debugPrint("LSI can't open UDP socket");
		exit(1);
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(udpEnv.socketDescriptor)
	|| bind(udpEnv.socketDescriptor, &udpEnv.socketName, nameLength) < 0
	|| getsockname(udpEnv.socketDescriptor, &udpEnv.socketName, &nameLength) < 0)
	{
		//closesocket(udpEnv.socketDescriptor);
		close(udpEnv.socketDescriptor);
		//putSysErrmsg("Can't initialize socket", NULL);
		debugPrint("Can't initialize socket");
		exit(1);
	}
}
void sendPacketToLowerProtocol	(char *buffer, int *bufferLength, void *udpProtocolData)
	{
	int					bytesSent;
	float				sleep_secs;
	unsigned int		usecs;

	struct sockaddr * workingSockAddr;
	if(udpProtocolData!=NULL)
		workingSockAddr=udpProtocolData;
	else
		workingSockAddr=(struct sockaddr *)udpEnv.inetName;

	if (*bufferLength > FEC_LOWERLEVEL_MAX_PACKET_SIZE)
		{
		debugPrint("Segment is too big for UDP LSO, skipping... BufferLength: %d",*bufferLength);
		return;
		}

	bytesSent = sendto(udpEnv.socketDescriptor, buffer, *bufferLength, 0, (struct sockaddr *) workingSockAddr, sizeof(struct sockaddr));

	if (bytesSent < *bufferLength)
		{
		debugPrint("UDP: bytesSent < segmentLength, skipping..");
		return;
		}

	sleep_secs = udpEnv.sleepSecPerBit * (((sizeof(struct iphdr) + sizeof(struct udphdr)) + *bufferLength) * 8);
	usecs = sleep_secs * 1000000.0;
	if (usecs == 0)
		{
		usecs = 1;
		}

	usleep(usecs);
	}
void receivePacketFromLowerProtocol	(char *buffer,int *bufferLength, void **udpProtocolData, unsigned int *udpProtocolDataSize)
{
	static struct sockaddr_in	fromAddr;
	socklen_t			fromSize;
	fromSize = sizeof(struct sockaddr_in);
	*bufferLength = recvfrom(udpEnv.socketDescriptor, buffer, FEC_LOWERLEVEL_MAX_PACKET_SIZE,	0, (struct sockaddr *) &fromAddr, &fromSize);
	*udpProtocolData= &fromAddr;
	*udpProtocolDataSize=sizeof(struct sockaddr_in);

	//todo How to manage an error during the reception of an UDP datagram?
	if(*bufferLength <= 0)
		{
		debugPrint("Can't acquire segment from UDP");
		exit(1);
		}
}

