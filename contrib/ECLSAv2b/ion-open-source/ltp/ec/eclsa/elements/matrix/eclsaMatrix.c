/*
 eclsaMatrix.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaMatrix.h"
#include "../sys/eclsaLogger.h"
#include <string.h>
#include <stdlib.h>

/*Single eclsa matrix functions*/
bool isMatrixInfoPartFull(EclsaMatrix *matrix)
	{
	return matrix->infoSegmentAddedCount == matrix->maxInfoSize;
	}
bool isMatrixEmpty(EclsaMatrix *matrix)
	{
	return (matrix->infoSegmentAddedCount == 0 && matrix->redundancySegmentAddedCount ==0);
	}
bool isInfoSymbol(EclsaMatrix *matrix, int symbolID)
	{
	return symbolID < matrix->encodingCode->K;
	}
void eclsaMatrixInit(EclsaMatrix *matrix)
{
	FecElement *biggestFec=getBiggestFEC();
	int N = biggestFec->N;
	int T = biggestFec->T;

	memset(matrix,0,sizeof(EclsaMatrix));
	matrix->codecStatus= STATUS_CODEC_NOT_DECODED;
	sem_init(&(matrix->lock),0,1);

	codecMatrixInit(&(matrix->abstractCodecMatrix),N,T);
}
void eclsaMatrixDestroy(EclsaMatrix *matrix)
{
	FecElement *biggestFec=getBiggestFEC();
	int N = biggestFec->N;

	sem_destroy(&(matrix->lock));
	codecMatrixDestroy(matrix->abstractCodecMatrix,N);
}
void addSegmentToEclsaMatrix(EclsaMatrix *matrix, char *buffer, int bufferLength, int symbolID,bool copyLength)
{
	if(!addSymbolToCodecMatrix(matrix->abstractCodecMatrix,symbolID,buffer,bufferLength,copyLength))
		{
		debugPrint("WARNING:Received redundant segment.");
		return;
		}

	//If encodingCode==NULL we are in eclso and the code has not been selected yet,
	//thus the segment is always an info segment.

	if( matrix->encodingCode==NULL || isInfoSymbol(matrix,symbolID) )
		matrix->infoSegmentAddedCount++;
	else
		matrix->redundancySegmentAddedCount++;
}
void flushEclsaMatrix(EclsaMatrix *matrix)
	{
	/*flushing ADT data*/
	FecElement *biggestFec=getBiggestFEC();
	int N = biggestFec->N;
	int T = biggestFec->T;
	int matrixID=matrix->ID;
	int engineID=matrix->engineID;

	matrix->engineID=0;
	matrix->ID=0;
	matrix->globalID=0;
	matrix->maxInfoSize=0;
	matrix->infoSegmentAddedCount=0;
	matrix->redundancySegmentAddedCount=0;
	flushMatrixFecElement(matrix->encodingCode);
	matrix->encodingCode=NULL;
	matrix->timer.ID=0;
	matrix->timer.thread=0;
	matrix->timer.rewind=false;
	matrix->clearedToCodec=false;
	matrix->clearedToSend=false;
	matrix->feedbackEnabled=false;
	matrix->workingT=0;
	matrix->HSLTPMatrixType=ONLY_DATA;

	sequenceFlush(&matrix->sequence);

	matrix->codecStatus=STATUS_CODEC_NOT_DECODED;

	if(matrix->lowerProtocolData != NULL)
		{
		free(matrix->lowerProtocolData);
		matrix->lowerProtocolData=NULL;
		}

	flushCodecMatrix(matrix->abstractCodecMatrix,N,T);
	debugPrint("T3: Matrix flushed MID=%d EID=%d", matrixID, engineID);
	}
