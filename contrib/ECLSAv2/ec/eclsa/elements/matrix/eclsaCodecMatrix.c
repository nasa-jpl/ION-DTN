/*
 eclsaCodecMatrix.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
  	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaCodecMatrix.h"
#include "../sys/eclsaLogger.h"
#include "../sys/eclsaMemoryManager.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void codecMatrixInit(CodecMatrix **codecMatrix,int N,int T)
{
	CodecMatrix *res;
	int j;

	if ( codecMatrix == NULL )
		{
		debugPrint("Error on creating codewordBox[ %d x %d ]: pointer is NULL");
		return;
		}

	debugPrint("creating codewordBox[ %d x %d ]",N,T);
	res = (CodecMatrix*) allocateElement(sizeof(CodecMatrix));
	if ( res != NULL )
		{
		res->rows=N;
		res->cols=T;
		res->codewordBox = (uint8_t**) allocateMatrix(sizeof(uint8_t), res->rows, res->cols);
		res->symbolStatus = (uint8_t*) allocateVector(sizeof(uint8_t), res->rows);
		if ( res->codewordBox == NULL || res->symbolStatus == NULL ) 	// one alloc went wrong
			{
			deallocateMatrix(&(res->codewordBox), res->rows); 	// free the matrix (could be allocated)
			deallocateVector(&(res->symbolStatus));			// free the vector (could be allocated)
			deallocateElement(&(res));						// free the res
			res = NULL;
			}
		}
	*codecMatrix=res;
}
void codecMatrixDestroy(CodecMatrix *codecMatrix)
{
	int j;

	if ( codecMatrix == NULL )
		return;

	deallocateMatrix(&(codecMatrix->codewordBox), codecMatrix->rows);
	deallocateVector(&(codecMatrix->symbolStatus));
	deallocateElement(&(codecMatrix));
}
bool addSymbolToCodecMatrix(CodecMatrix *codecMatrix,int symbolID, char *buffer,int bufferLength,bool copyLenght)
{
	// Each segment is a symbol
	uint16_t segmentLength; //2Byte

	if(codecMatrix->symbolStatus[symbolID]==1) //segment already in matrix
		return false;

	if(copyLenght)
		{
		segmentLength=(uint16_t) bufferLength;
		//copy the size of the LTP (or other upper protocol) segment into the first 2B of the codewordBox column
		memcpy(codecMatrix->codewordBox[symbolID], &segmentLength, sizeof(uint16_t) );
		//copy the LTP (or other upper protocol) segment into the other bytes of the codewordBox column
		memcpy(codecMatrix->codewordBox[symbolID]+sizeof(uint16_t), buffer, sizeof(uint8_t)* segmentLength);
		}
	else
		{
		//copy the eclsa payload into one codewordBox column
		memcpy(codecMatrix->codewordBox[symbolID],buffer,sizeof(uint8_t) * bufferLength);
		}
	codecMatrix->symbolStatus[symbolID]=1;

	return true;
}
char *getSymbolFromCodecMatrix(CodecMatrix *codecMatrix,unsigned int symbolID)
{
	return (char *)codecMatrix->codewordBox[symbolID];
}
void flushCodecMatrix(CodecMatrix *codecMatrix)
{
	int i;

	if ( codecMatrix == NULL )
		return;

	// flushing codecMatrix
	memset(codecMatrix->symbolStatus	, 0, sizeof(uint8_t)*codecMatrix->rows);
	for(i=0;i<codecMatrix->rows;i++)
		memset(codecMatrix->codewordBox[i], 0, sizeof(uint8_t)*codecMatrix->cols);
	// end of flushing codecMatrix
}
bool isValidSymbol(CodecMatrix *codecMatrix, int symbolID)
{
	return (bool) codecMatrix->symbolStatus[symbolID];
}

