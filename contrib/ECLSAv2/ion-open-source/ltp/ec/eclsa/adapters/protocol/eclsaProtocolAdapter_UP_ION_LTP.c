/*
eclsaProtocolAdapter_UP_ION_LTP.c

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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdlib.h>
#include "ltpP.h"
#define LtpUdpDefaultPortNbr		1113

typedef struct
{
	char 		* endpointSpec;
	LtpVspan	*vspan;
} IonLtpEclsoEnvironment;

IonLtpEclsoEnvironment ltpEclsoEnv;
void initEclsoUpperLevel(int argc, char *argv[], unsigned int *T, unsigned short *ownEngineId,unsigned short *portNbr,unsigned int *ipAddress)
{
	/* ION declarations*/
		Sdr					sdr;
		LtpDB				*ltpDb;
		PsmAddress			vspanElt;
		unsigned long 		remoteEngineId; //to find span
		char				ownHostName[MAXHOSTNAMELEN];

		ltpEclsoEnv.endpointSpec = argc > 1 ? argv[1] : NULL;
		remoteEngineId = strtoul(argv[8], NULL, 0);

		if (remoteEngineId == 0 || ltpEclsoEnv.endpointSpec == NULL)
			{
			printf("remoteEngineId %lu", remoteEngineId);
			PUTS("Usage: eclso {<remote engine's host name> | @}[:\
			<its port number>] <txbps (0=unlimited)> <remote engine ID>");
			exit(1);
			}
		if (ltpInit(0) < 0)
			{
			putErrmsg("eclso can't initialize LTP.", NULL);
			exit(1);
			}
		sdr = getIonsdr();

		if (!(sdr_begin_xn(sdr)))
			{
			putErrmsg("failed to begin sdr", NULL);
			exit(1);
			}
		findSpan(remoteEngineId, &ltpEclsoEnv.vspan, &vspanElt);
		if (vspanElt == 0)
			{
			sdr_exit_xn(sdr);
			putErrmsg("No such engine in database.", itoa(remoteEngineId));
			exit(1);
			}
		if (ltpEclsoEnv.vspan->lsoPid != ERROR && ltpEclsoEnv.vspan->lsoPid != sm_TaskIdSelf())
			{
			sdr_exit_xn(sdr);
			putErrmsg("LSO task is already started for this span.",itoa(ltpEclsoEnv.vspan->lsoPid));
			exit(1);
			}
		sdr_exit_xn(sdr);
		ltpDb= getLtpConstants();


		*T=ltpEclsoEnv.vspan->maxXmitSegSize+ sizeof(uint16_t); //2B added for segmentLength
		*ownEngineId=(unsigned short)ltpDb->ownEngineId;

		ionNoteMainThread("eclso");

		//Getting info for lower layer
		getNameOfHost(ownHostName, MAXHOSTNAMELEN);

		*portNbr=0;
		*ipAddress=0;
		parseSocketSpec(ltpEclsoEnv.endpointSpec, portNbr, ipAddress);
		if (*ipAddress == 0)		//	Default to local host.
				*ipAddress = getInternetAddress(ownHostName);
		if (*portNbr == 0)
				*portNbr = LtpUdpDefaultPortNbr;

}
void initEclsiUpperLevel(int argc, char *argv[], unsigned short *portNbr, unsigned int *ipAddress)
{
	LtpVdb	*vdb;

	char	*endpointSpec = (argc > 1 ? argv[1] : NULL);

	if (ltpInit(0) < 0)
		{
			putErrmsg("eclsi can't initialize LTP.", NULL);
			exit(1);
		}

	vdb = getLtpVdb();
	if (vdb->lsiPid != ERROR && vdb->lsiPid != sm_TaskIdSelf())
		{
			putErrmsg("LSI task is already started.", itoa(vdb->lsiPid));
			exit(1);
		}

	*portNbr=0;
	*ipAddress = INADDR_ANY;
	if (endpointSpec && parseSocketSpec(endpointSpec, portNbr, ipAddress) != 0)
		{
			putErrmsg("Can't get IP/port for endpointSpec.",endpointSpec);
			exit(1);
		}

	if (*portNbr == 0)
		*portNbr = LtpUdpDefaultPortNbr;

	ionNoteMainThread("eclsi");
}
void sendSegmentToUpperProtocol		(char *buffer,int *bufferLength)
{
if (ltpHandleInboundSegment(buffer, *bufferLength) < 0)
	{
		putErrmsg("Can't handle inbound segment.", NULL);
		exit(1);
	}
}
void receiveSegmentFromUpperProtocol	(char *buffer,int *bufferLength)
{
char *bufPtr;
*bufferLength=ltpDequeueOutboundSegment(ltpEclsoEnv.vspan,&bufPtr);
if(*bufferLength <= 0)
	{
	debugPrint("Can't acquire segment from LTP");
	}
memcpy(buffer,bufPtr,*bufferLength);
}

