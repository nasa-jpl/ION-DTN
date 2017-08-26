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

#define LtpUdpDefaultPortNbr		1113
#define LTP_VERSION	0;

typedef struct
{
	char 		* endpointSpec;
	LtpVspan	*vspan;
} IonLtpEclsoEnvironment;

IonLtpEclsoEnvironment ltpEclsoEnv;

static int	serializeHeaderInternal(LtpXmitSeg *segment, char *segmentBuffer,
			Sdnv *engineIdSdnv)
{
	char		firstByte = LTP_VERSION;
	char		*cursor = segmentBuffer;
	Sdnv		sessionNbrSdnv;
	char		extensionCounts;

	firstByte <<= 4;
	firstByte += segment->pdu.segTypeCode;
	*cursor = firstByte;
	cursor++;

	memcpy(cursor, engineIdSdnv->text, engineIdSdnv->length);
	cursor += engineIdSdnv->length;

	encodeSdnv(&sessionNbrSdnv, segment->sessionNbr);
	memcpy(cursor, sessionNbrSdnv.text, sessionNbrSdnv.length);
	cursor += sessionNbrSdnv.length;

	extensionCounts = segment->pdu.headerExtensionsCount;
	extensionCounts <<= 4;
	extensionCounts += segment->pdu.trailerExtensionsCount;
	*cursor = extensionCounts;

	return 0;
}
static void	serializeReportAckSegmentInternal(LtpXmitSeg *segment, char *buf,Sdnv *ownEngineIdSdnv)
{
	char	*cursor = buf;
	Sdnv	serialNbrSdnv;

	/*	Report is from remote engine, so origin is the local
	 *	engine.							*/

	serializeHeaderInternal(segment, cursor, ownEngineIdSdnv);
	cursor += segment->pdu.headerLength;

	/*	Append report serial number.				*/

	encodeSdnv(&serialNbrSdnv, segment->pdu.rptSerialNbr);
	memcpy(cursor, serialNbrSdnv.text, serialNbrSdnv.length);
}

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
	//todo wait 0.5ms. this is a fix to avoid segments dropping in the interface with the
	//upper protocol. The origin of the dropping has not been thoroughly investigated yet.
	usleep(500);
}
void sendSegmentToUpperProtocol_HSLTP_MODE		(char *buffer,int *bufferLength,int abstractCodecStatus,bool isFirst)
{
	static bool canceledSession;
	LtpSegmentTypeCode segTypeCode;
	bool isSignalingSegment;

	if (isFirst) canceledSession = false;

	//todo wait 0.5ms. this is a fix to avoid segments dropping in the interface with the
	//upper protocol. The origin of the dropping has not been thoroughly investigated yet.
	const int interval=500;

	segTypeCode = buffer[0] & 0x0f;

	isSignalingSegment= (segTypeCode & LTP_CTRL_FLAG) != 0;
	if(abstractCodecStatus == STATUS_CODEC_NOT_DECODED || abstractCodecStatus == STATUS_CODEC_SUCCESS
		|| isSignalingSegment)
		{
			if (ltpHandleInboundSegment(buffer, *bufferLength) < 0)
				{
				putErrmsg("Can't handle inbound segment.", NULL);
				exit(1);
				}
			usleep(interval);
		}

	if(!isSignalingSegment && abstractCodecStatus == STATUS_CODEC_FAILED && !canceledSession)
		{
			debugPrint("Send a pair of miscolored segment");
			canceledSession = true;
			if (ltpHandleInboundSegment(buffer, *bufferLength) < 0)
				{
					putErrmsg("Can't handle inbound segment.", NULL);
					exit(1);
				}
			usleep(interval);
			// change color to the segment
			buffer[0] = buffer[0] ^ LTP_EXC_FLAG;
			if (ltpHandleInboundSegment(buffer, *bufferLength) < 0)
				{
					putErrmsg("Can't handle inbound segment.", NULL);
					exit(1);
				}
			usleep(interval);
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

int receiveSegmentFromUpperProtocol_HSLTP_MODE(char *buffer,int *bufferLength)
{
	char *bufPtr;
	LtpSegmentTypeCode segTypeCode;
	char		*cursor = buffer;
	uvast		sourceEngineId;
	unsigned int	sourceEngineIdLength;
	unsigned int	sessionNbr;
	unsigned int		rptSerialNbr;
	unsigned int	ckptSerialNbr;
	Sdnv		sdnv;
	Sdnv sourceEngineIdSdnv;
	unsigned int	sessionNbrLength;
	unsigned int	serialNbrLength;
	LtpXmitSeg	segment;
	uint16_t tmpSegLen;
	int segmentLength;
	char *segmentBuffer;
	int result;
	int bytesRemaining;
	Sdr		sdr;
	Object		segmentObj;
	Lyst		headerExtensions;
	Lyst		trailerExtensions;
	unsigned int	extensionOffset;
	unsigned int	extensionCounts;
	int i ;
	unsigned headerExtensionsCount;

	int processingType = ERROR_PROC;

	*bufferLength=ltpDequeueOutboundSegment(ltpEclsoEnv.vspan,&bufPtr);
	if(*bufferLength <= 0)
		{
		debugPrint("Can't acquire segment from LTP");
		}
	
	memcpy(buffer,bufPtr,*bufferLength);
	segTypeCode = buffer[0] & 0x0f;

	// Data
	if ((segTypeCode & LTP_CTRL_FLAG) == 0)
		{
			processingType = DEFAULT_PROC;
		}

	// EOB
	if(segTypeCode == LtpDsGreenEOB || segTypeCode == LtpDsRedEOB)
				processingType = END_OF_MATRIX;
	
	// Signaling Segment
	if(segTypeCode == LtpRS || segTypeCode == LtpRAS
		|| segTypeCode == LtpCS || segTypeCode == LtpCAS
		|| segTypeCode == LtpCR || segTypeCode == LtpCAR)
				processingType = SPECIAL_PROC;

	
	if(segTypeCode == LtpRS)
	{	
			bytesRemaining = *bufferLength;
			cursor++;
			bytesRemaining--;
			extractSdnv(&sourceEngineId,&cursor, &bytesRemaining);
			extractSmallSdnv(&sessionNbr, &cursor, &bytesRemaining);
		
			cursor++;
			bytesRemaining--;
	
			extractSmallSdnv(&rptSerialNbr, &cursor, &bytesRemaining);
	
			memset((char *) &segment, 0, sizeof(LtpXmitSeg));
			segment.sessionNbr = sessionNbr;
			segment.remoteEngineId = sourceEngineId;
			encodeSdnv(&sourceEngineIdSdnv, sourceEngineId);
			sourceEngineIdLength = sourceEngineIdSdnv.length;
			encodeSdnv(&sdnv, sessionNbr);
			sessionNbrLength = sdnv.length;
			encodeSdnv(&sdnv, rptSerialNbr);
			serialNbrLength = sdnv.length;
			segment.pdu.headerLength = 1 + sourceEngineIdLength + sessionNbrLength + 1;
			segment.pdu.contentLength = serialNbrLength;
			segment.pdu.trailerLength = 0;
			segment.sessionListElt = 0;
			segment.segmentClass = LtpMgtSeg;
			segment.pdu.segTypeCode = LtpRAS;
			segment.pdu.rptSerialNbr = rptSerialNbr;
			segmentLength = segment.pdu.headerLength + segment.pdu.contentLength;
			segmentBuffer = (char *)malloc(segmentLength);
			serializeReportAckSegmentInternal(&segment,segmentBuffer,&sourceEngineIdSdnv);

	
			if (ltpHandleInboundSegment(segmentBuffer, segmentLength) < 0)
				{
					putErrmsg("Can't handle inbound segment.", NULL);
					exit(1);
				}
			free(segmentBuffer);
		}

	if(processingType == ERROR_PROC)
	{
		debugPrint("Can't read processing type from segment");
		exit(1);
	}
	return processingType;
}

