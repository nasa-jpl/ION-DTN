/*
eclsaPacketManager.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

 * */

#ifndef _ECPACKETMANAGER_H_
#define _ECPACKETMANAGER_H_
#include "../matrix/eclsaMatrix.h"
#include <stdbool.h>


/*Header */
typedef struct
{
	unsigned char 	version;		//1Byte
	unsigned char 	extensionCount;	//1Byte
	unsigned char 	flags;
	unsigned short 	engineID; 		//2Byte
	unsigned short 	matrixID;		//BID,Matrix ID, 2Byte
	unsigned short 	symbolID;		//PID, 2Byte
	unsigned short 	segmentsAdded;	//number of added segments, 2Byte
	unsigned short 	K;
	unsigned short 	N;
	unsigned short 	T; 				//2Byte
} EclsaHeader;

typedef enum
{
	FEEDBACK_REQUEST_MASK 	= 1,
	CONTINUOUS_MODE_MASK 	= 2,
	HSLTP_MODE_MASK 		= 4
} HeaderFlags; //2^n , n= 0,1,2, ...


/*Eclsa packet functions*/
void createEclsaHeader(EclsaMatrix *matrix,EclsaHeader *header);
void createEclsaPacket(EclsaMatrix *matrix, EclsaHeader *header, int symbolID, char *buffer,int *bufferLength);
bool isUncodedEclsaPacket(EclsaHeader *header);
bool parseEclsaIncomingPacket(EclsaHeader *outHeader, FecElement **outEncodingCode,char *inBuffer, int inBufferLength, char **outBuffer, int *outBufferLength, int maxT);



#endif
