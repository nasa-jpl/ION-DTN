/*
eclsaPacketManager.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

 * */
#include "eclsaPacket.h"
#include "../sys/eclsaLogger.h"
#include "../../adapters/codec/eclsaCodecAdapter.h"

#include <string.h>

/*Eclsa packet functions*/
void createEclsaHeader(EclsaMatrix *matrix,EclsaHeader *header)
{
	//Create the eclsa header with PID=0 (once per matrix)
	//The actual PID will be set later by the createEclsaPacket function.
	memset(header,0,sizeof(EclsaHeader));

	header->version=0;
	header->engineID=matrix->engineID;
	header->matrixID = matrix->ID;
	if(matrix->encodingCode!=NULL)
		{
		//header->codeID= matrix->encodingCode->ID;
		header->K = 	matrix->encodingCode->K;
		header->N = 	matrix->encodingCode->N;
		header->T= 		matrix->encodingCode->T;
		if(matrix->encodingCode->continuous)
			header->flags=	header->flags | CONTINUOUS_MODE_MASK;
		}
    if(matrix->HSLTPModeEnabled)
    	header->flags = header->flags | HSLTP_MODE_MASK;

	header->segmentsAdded= matrix->infoSegmentAddedCount;

	if(matrix->feedbackEnabled)
		header->flags= header->flags | FEEDBACK_REQUEST_MASK;
	//todo extensions must be parsed
	header->extensionCount=0;
}
void createEclsaPacket(EclsaMatrix *matrix, EclsaHeader *header, int symbolID, char * buffer,int *bufferLength)
{
	int i;
	int T;
	uint16_t segmentLength; //2Byte
	char *matrixSegment=getSymbolFromCodecMatrix(matrix->abstractCodecMatrix,symbolID);
	int universalCodecStatus = convertToAbstractCodecStatus(matrix->codecStatus);

	//The eclsa header has been created once with packetID=0 by createEclsaHeader();
	header->symbolID = symbolID; // set the symbolID to the actual value
	//copy the eclsa header into the buffer
	memcpy(buffer,header,sizeof(EclsaHeader));

	//if(matrix->encoded && !isInfoSegment(matrix,packetID) )
	if(universalCodecStatus == STATUS_CODEC_SUCCESS  && !isInfoSymbol(matrix,symbolID) )
		{
		//The eclsa payload must be filled with a matrix  redundancy segment, all the segment must be copied;
		T = matrix->encodingCode->T;
		//add the eclsa payload to the buffer
		memcpy(buffer+sizeof(EclsaHeader),matrixSegment,sizeof(uint8_t) * T);
		*bufferLength= sizeof(EclsaHeader) + sizeof(uint8_t) * T ;

		//Optimization: eclsa packets often end with a long run of zeroes.
		//This often happens when the encoding matrix is not full (I<<K).
		//These zeroes can be safely erased to save bandwidth.

		for (i = *bufferLength - 1; i >= 0; i--)
			if (buffer[i] != '\0')
				{
				*bufferLength = i + 1;
				break;
				}

		}
	else
		{
		//The eclsa payload must be filled with a matrix uncoded or info segment,
		//only the actual data must be copied (drop the row padding);

		//extract the LTP (or another upper protocol) segment length from the first 2 bytes of ADT[packetID]
		memcpy(&segmentLength,matrixSegment, sizeof(uint16_t));
		//add the eclsa payload to the buffer
		memcpy(buffer+sizeof(EclsaHeader),matrixSegment, segmentLength + sizeof(uint16_t));
		*bufferLength=(int)segmentLength+sizeof(EclsaHeader) + sizeof(uint16_t);
		}


}
bool parseEclsaIncomingPacket(EclsaHeader *outHeader, FecElement **outEncodingCode, char *inBuffer, int inBufferLength, char **outBuffer, int *outBufferLength, int maxT)
{
	int i;
	uint16_t tmpSegmentLength; //2Byte
	bool continuousModeRequested=false;
	if(inBufferLength < sizeof(EclsaHeader))
		{
		debugPrint("Eclsa Header validation failed. Discarding segment. ( bufferLength < sizeof(EclsaHeader))");
		return false;
		}
	memcpy(outHeader,inBuffer,sizeof(EclsaHeader));

	if(outHeader->version != 0)
		{
		debugPrint("Eclsa Header validation failed. Discarding segment. (header->version != 0)");
		return false;
		}
	if(outHeader->segmentsAdded == 0)
		{
		debugPrint("Eclsa Header validation failed. Discarding segment. (header->segmentsAdded == 0)");
		return false;
		}
	//todo
	//if(header->T == 0)
		//{
		//debugPrint("WARNING: header->T == 0");
		//return false;
		//}
	if(outHeader->T > maxT)
		{
		debugPrint("Eclsa Header validation failed. Discarding segment. (header->T > maxT)");
		return false;
		}

	//remove the header from the buffer
	*outBuffer=inBuffer+sizeof(EclsaHeader);
	*outBufferLength= inBufferLength - sizeof(EclsaHeader);

	if(isUncodedEclsaPacket(outHeader))
		{
		memcpy(&tmpSegmentLength,*outBuffer, sizeof(uint16_t));
		*outBuffer=*outBuffer+sizeof(uint16_t);
		*outBufferLength= tmpSegmentLength;
		return true;
		}
	continuousModeRequested = (outHeader->flags & CONTINUOUS_MODE_MASK) != 0;
	*outEncodingCode = getFecElement(outHeader->K,outHeader->N,continuousModeRequested);

	if(*outEncodingCode == NULL)
		{
		//pointer not found (the wanted code is not in the code array)
		//todo togliendo questo commento si ottiene questa informazione che è utile.
		//essendo mostrata una volta per pacchetto, riempie il log se i segmenti sono tanti
		//debugPrint("Encoding code not found. Discarding segment.");
		return false;
		}
	return true;
}
bool isUncodedEclsaPacket(EclsaHeader *header)
	{
	return header->K == 0 && header->N == 0;
	}

