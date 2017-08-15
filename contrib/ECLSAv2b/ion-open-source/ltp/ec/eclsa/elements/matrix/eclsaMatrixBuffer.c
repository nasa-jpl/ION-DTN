/*
 eclsaMatrixBuffer.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaMatrixBuffer.h"
#include "../sys/eclsaLogger.h"
#include <string.h>
#include <stdlib.h> //malloc, free

/*Matrix buffer */
typedef struct
{
	EclsaMatrix *array;
	unsigned int length;
	sem_t		lock;
} MatrixBuffer;

static MatrixBuffer matrixBuffer;

/*Eclsa matrix buffer functions*/
void matrixBufferInit(unsigned int length)
{
	int i;
	memset(&matrixBuffer,0,sizeof(MatrixBuffer));
	matrixBuffer.array=calloc(length,sizeof(EclsaMatrix));
	matrixBuffer.length=length;
	sem_init(&(matrixBuffer.lock),0,1);

	for(i=0;i< matrixBuffer.length; i++)
		{
		eclsaMatrixInit(&(matrixBuffer.array[i]));
		//todo
		sequenceInit(&(matrixBuffer.array[i].sequence),getBiggestFEC()->N);
		}
}
void matrixBufferDestroy()
{
	int i;
	for(i=0;i< matrixBuffer.length; i++)
		{
		eclsaMatrixDestroy(&(matrixBuffer.array[i]));
		//todo
		sequenceDestroy(&(matrixBuffer.array[i].sequence));
		}
	free(matrixBuffer.array);
	sem_destroy(&(matrixBuffer.lock));
}
EclsaMatrix *getMatrixToFill(unsigned short matrixID, unsigned short engineID)
{
	static int currentIndex=0;
	int i;
	EclsaMatrix *matrix;

	sem_wait(&(matrixBuffer.lock));
	//Look if the previous matrix is still the requested matrix
	matrix=&(matrixBuffer.array[currentIndex]);
	if(matrix->ID == matrixID && matrix->engineID == engineID )
		{
		sem_post(&(matrixBuffer.lock));
		return matrix;
		}

	//otherwise looks for the requested matrix
	for(i=0;i<matrixBuffer.length;i++)
		{
		matrix=&(matrixBuffer.array[i]);
		if(matrix->ID == matrixID && matrix->engineID == engineID )
			{
			sem_post(&(matrixBuffer.lock));
			currentIndex=i;
			return matrix;
			}
		}

	//if the requested matrix is not found, return the first free matrix
	for(i=0;i<matrixBuffer.length;i++)
		{
		matrix=&(matrixBuffer.array[i]);
		if(isMatrixEmpty(matrix) )
			{
			sem_post(&(matrixBuffer.lock));
			currentIndex=i;
			return matrix;
			}
		}

	//if there is not any free matrix, the buffer is full. By returning NULL the thread is put to sleep
	sem_post(&(matrixBuffer.lock));
	return NULL;
}
EclsaMatrix *getMatrixToCode()
{
	int i;
	unsigned long 	minGlobalID;
	EclsaMatrix		*matrix_i;
	EclsaMatrix 	*returnMatrix;
	bool 			found=false;

	sem_wait(&(matrixBuffer.lock));

	//Find the next matrix to code (i.e. that with the lower globalID)
	for(i=0;i< matrixBuffer.length;i++)
		{
		matrix_i=&(matrixBuffer.array[i]);

		if( !matrix_i->clearedToSend && matrix_i->clearedToCodec && ( !found || matrix_i->globalID < minGlobalID))
			{
			minGlobalID= matrix_i->globalID;
			returnMatrix=matrix_i;
			found=true;
			}
		}

	sem_post(&(matrixBuffer.lock));
	if(found)
		return returnMatrix;
	else
		return NULL;
}
EclsaMatrix *getMatrixToSend()
{
	int i;
	unsigned long 	minGlobalID;
	EclsaMatrix 	*matrix_i;
	EclsaMatrix 	*returnMatrix;
	bool 			found=false;

	sem_wait(&(matrixBuffer.lock));

	//Find the next matrix to send (with the lower globalID)
	for(i=0;i< matrixBuffer.length;i++)
		{
		matrix_i=&(matrixBuffer.array[i]);
		if(matrix_i->clearedToSend && ( !found || matrix_i->globalID < minGlobalID))
			{
			minGlobalID= matrix_i->globalID;
			returnMatrix=matrix_i;
			found=true;
			}
		}

	sem_post(&(matrixBuffer.lock));
	if(found)
		return returnMatrix;
	else
		return NULL;
}

bool	forcePreviousMatrixToDecode(EclsaMatrix *currentMatrix)
{
	//todo
	EclsaMatrix *matrix_i;
	unsigned int i;
	bool found=false;
	sem_wait(&(matrixBuffer.lock));
		for(i=0;i<matrixBuffer.length;i++)
			{
			matrix_i=&(matrixBuffer.array[i]);
			if( !matrix_i->clearedToCodec &&
				!matrix_i->clearedToSend &&
				 matrix_i->engineID == currentMatrix->engineID &&
				 matrix_i->ID < currentMatrix->ID)
				{
				timerStop(&(matrix_i->timer));
				matrix_i->clearedToCodec=true;
				debugPrint("T1: matrix received.Status:\"Forced\" MID=%d EID=%d N=%d K=%d I=%d/%d R=%d/%d",matrix_i->ID,matrix_i->engineID,matrix_i->encodingCode->N,matrix_i->encodingCode->K,matrix_i->infoSegmentAddedCount,matrix_i->maxInfoSize,matrix_i->redundancySegmentAddedCount,matrix_i->encodingCode->N-matrix_i->encodingCode->K);
				found=true;
				}
			}
	sem_post(&(matrixBuffer.lock));
	return found;
}
