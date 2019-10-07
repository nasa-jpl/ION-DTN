/*
eclso_ext.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
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

static void setAggregationSizeLimit(unsigned long aggregationSizeLimit, char *remoteEngineIdStr)
	{
		//update LTP Span with the new aggregation size limit

		Object		spanAddress;
		LtpSpan		spanBuffer;
		Sdr 		sdr = getIonsdr();

		if (sdr_begin_xn(sdr)<0)
		{
			putErrmsg("Failed to initialize SDR.", remoteEngineIdStr);
			exit(0);
		}

		spanAddress = (Object) sdr_list_data(sdr, ltpEclsoEnv.vspan->spanElt);
		sdr_stage(sdr, (char *) &spanBuffer, spanAddress, sizeof(LtpSpan));

		//update
		spanBuffer.aggrSizeLimit=aggregationSizeLimit;

		//commit
		sdr_write(sdr, spanAddress, (char *) &spanBuffer, sizeof(LtpSpan));
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed to end SDR transaction.", remoteEngineIdStr);
			exit(0);
		}

	}

static void setMaxBundlePayloadSize(unsigned long maxBundlePayloadSize,char *remoteEngineIdStr)
	{
			Object bpdbObject =0;
			VOutduct	*vduct;
			PsmAddress	vductElt=0;
			Sdr 		sdr = getIonsdr();
			const long time=100000; // 100ms

			//wait for the initialization of BP database
			while(bpdbObject == 0)
			{
			if (sdr_begin_xn(sdr) < 0)
				{
				putErrmsg("Failed to initialize SDR.", remoteEngineIdStr);
				exit(0);
				}
			bpdbObject = sdr_find(sdr, "bpdb", NULL);
			sdr_exit_xn(sdr);
			if(bpdbObject == 0)
				usleep(time);
			}

			//attach to bundle protocol

			if (bpAttach() < 0)
				{
					putErrmsg("eclso can't attach to BP.", NULL);
					exit(0);
				}

			//wait the Outduct definition for this eclso
			while(vductElt == 0)
			{
			if (sdr_begin_xn(sdr) < 0)
				{
				putErrmsg("Failed to initialize SDR.", remoteEngineIdStr);
				exit(0);
				}
			findOutduct("ltp", remoteEngineIdStr, &vduct, &vductElt);
			sdr_exit_xn(sdr);
			if(vductElt == 0)
				usleep(time);
			}

			//update the Outduct with the new maxBundlePayloadSize
			updateOutduct("ltp",remoteEngineIdStr,NULL,maxBundlePayloadSize);

	}

void initEclsoUpperLevel_HSLTP_MODE(int argc, char *argv[], EclsoEnvironment *env)
{

	/* ION declarations*/
		Sdr					sdr;
		LtpDB				*ltpDb;
		PsmAddress			vspanElt;
		unsigned long 		remoteEngineId; //to find span
		char				ownHostName[MAXHOSTNAMELEN];
		char *remoteEngineIdStr=argv[8];

		ltpEclsoEnv.endpointSpec = argc > 1 ? argv[1] : NULL;
		remoteEngineId = strtoul(remoteEngineIdStr, NULL, 0);

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

		//proactive fragmentation

		if(env->HSLTPProactiveFragmentationEnabled)
		{
			unsigned long primaryBlockSize= 50;
			unsigned long ltpSingleOverhead=50;
			unsigned long maxMatrixSize= env->K * env->T;
			unsigned long ltpTotalOverHead= ltpSingleOverhead * env->K;
			unsigned long maxBundlePayloadSize= maxMatrixSize/2 - primaryBlockSize - ltpTotalOverHead;
			unsigned long aggregationSizeLimit= maxMatrixSize/2 -ltpTotalOverHead;

			debugPrint("HSLTP PROACTIVE FRAGMENTATION");
			debugPrint("BP ESTIMATED PRIMARY BLOCK SIZE = %lu ",primaryBlockSize);
			debugPrint("LTP ESTIMATED SINGLE OVERHEAD= %lu ",ltpSingleOverhead);
			debugPrint("MAX MATRIX SIZE (KxT) = %lu ",maxMatrixSize);
			debugPrint("LTP ESTIMATED TOTAL OVERHEAD= %lu ",ltpTotalOverHead);
			debugPrint("MAX BUNDLE PROTOCOL PAYLOAD SIZE = %lu ",maxBundlePayloadSize);
			debugPrint("AGGREGATION SIZE LIMIT= %lu ",aggregationSizeLimit);

			setAggregationSizeLimit(aggregationSizeLimit,remoteEngineIdStr);
			setMaxBundlePayloadSize(maxBundlePayloadSize,remoteEngineIdStr);
		}
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
			segmentBuffer = (char*) allocateVector(sizeof(char), segmentLength);
			serializeReportAckSegmentInternal(&segment,segmentBuffer,&sourceEngineIdSdnv);


			if (ltpHandleInboundSegment(segmentBuffer, segmentLength) < 0)
				{
					putErrmsg("Can't handle inbound segment.", NULL);
					exit(1);
				}
			deallocateVector(&(segmentBuffer));
		}

	if(processingType == ERROR_PROC)
	{
		debugPrint("Can't read processing type from segment");
		exit(1);
	}
	return processingType;
}
