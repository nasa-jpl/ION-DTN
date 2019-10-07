/*
eclsaProtocolAdapter_UP_ION_LTP.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
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
#include "../../elements/sys/eclsaLogger.h"

#define LtpUdpDefaultPortNbr		1113
#define LTP_VERSION	0;

typedef struct
{
	char 		* endpointSpec;
	LtpVspan	*vspan;
} IonLtpEclsoEnvironment;

IonLtpEclsoEnvironment ltpEclsoEnv;

void initEclsoUpperLevel(int argc, char *argv[], EclsoEnvironment *env)
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


		env->T=ltpEclsoEnv.vspan->maxXmitSegSize+ sizeof(uint16_t); //2B added for segmentLength
		env->ownEngineId=(unsigned short)ltpDb->ownEngineId;

		ionNoteMainThread("eclso");

		//Getting info for lower layer
		getNameOfHost(ownHostName, MAXHOSTNAMELEN);

		env->portNbr=0;
		env->ipAddress=0;
		parseSocketSpec(ltpEclsoEnv.endpointSpec, &(env->portNbr), &(env->ipAddress));
		if (env->ipAddress == 0)		//	Default to local host.
				env->ipAddress = getInternetAddress(ownHostName);
		if (env->portNbr == 0)
				env->portNbr = LtpUdpDefaultPortNbr;

}
void initEclsiUpperLevel(int argc, char *argv[], EclsiEnvironment *env)
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

	env->portNbr=0;
	env->ipAddress = INADDR_ANY;
	if (endpointSpec && parseSocketSpec(endpointSpec, &(env->portNbr), &(env->ipAddress)) != 0)
		{
			putErrmsg("Can't get IP/port for endpointSpec.",endpointSpec);
			exit(1);
		}

	if (env->portNbr == 0)
		env->portNbr = LtpUdpDefaultPortNbr;

	ionNoteMainThread("eclsi");
}

void sendSegmentToUpperProtocol		(char *buffer,int *bufferLength)
{
if (ltpHandleInboundSegment(buffer, *bufferLength) < 0)
	{
		putErrmsg("Can't handle inbound segment.", NULL);
		exit(1);
	}
	//todo wait 0.5ms. this is a fix to avoid segments dropping in the interface with the
	//upper protocol. The origin of the dropping has not been thoroughly investigated yet.
	usleep(500);
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


