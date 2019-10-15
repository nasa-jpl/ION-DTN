/*
 eclsaMatrix.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaMatrix.h"
#include "../sys/eclsaLogger.h"
#include "../sys/eclsaMemoryManager.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

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
void eclsaMatrixInit(EclsaMatrix *matrix, unsigned int N, unsigned int T)
{
	pthread_mutexattr_t recursiveAttribute;
	pthread_mutexattr_init(&recursiveAttribute);
	pthread_mutexattr_settype(&recursiveAttribute, PTHREAD_MUTEX_RECURSIVE);
	memset(matrix,0,sizeof(EclsaMatrix));

	matrix->codecStatus= STATUS_CODEC_NOT_DECODED;
	pthread_mutex_init(&(matrix->lock), &recursiveAttribute);

	codecMatrixInit(&(matrix->abstractCodecMatrix),N,T);
}
void eclsaMatrixDestroy(EclsaMatrix *matrix)
{
	if ( matrix == NULL )
		return;
	pthread_mutex_destroy(&(matrix->lock));
	codecMatrixDestroy(matrix->abstractCodecMatrix);
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
	int matrixID;
	int engineID;

	if ( matrix == NULL )
		return;

	/*flushing ADT data*/
	matrixID=matrix->ID;
	engineID=matrix->engineID;

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
		deallocateElement((Pointer*)&(matrix->lowerProtocolData));
		matrix->lowerProtocolData=NULL;
		}

	flushCodecMatrix(matrix->abstractCodecMatrix);
	pthread_mutex_unlock(&(matrix->lock));
	debugPrint("T3: Matrix flushed MID=%d EID=%d", matrixID, engineID);
	}
