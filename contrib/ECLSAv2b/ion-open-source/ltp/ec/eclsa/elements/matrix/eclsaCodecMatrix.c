/*
 eclsaCodecMatrix.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaCodecMatrix.h"
#include "../sys/eclsaLogger.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void codecMatrixInit(CodecMatrix **codecMatrix,int N,int T)
{
	CodecMatrix *res;
	int j;
	debugPrint("creating codewordBox[ %d x %d ]",N,T);
	res = malloc(sizeof(CodecMatrix));
	res->codewordBox=(uint8_t **)calloc(N,sizeof(uint8_t *));
	for(j=0;j<N;j++)
		res->codewordBox[j]=(uint8_t *)calloc(T,sizeof(uint8_t));
	res->symbolStatus=(uint8_t *)calloc(N,sizeof(uint8_t));
	*codecMatrix=res;
}
void codecMatrixDestroy(CodecMatrix *codecMatrix,int N)
{
	int j;
	for(j=0;j<N;j++)
		free(codecMatrix->codewordBox[j]);

	free(codecMatrix->codewordBox);
	free(codecMatrix->symbolStatus);
	free(codecMatrix);
}
bool addSymbolToCodecMatrix(CodecMatrix *codecMatrix,int symbolID, char *buffer,int bufferLength,unsigned char copyLenght)
{
	// Each segment is a symbol
	uint16_t segmentLength; //2Byte

	if(codecMatrix->symbolStatus[symbolID]==1) //segment already in matrix
		{
		return false;
		}

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
void flushCodecMatrix(CodecMatrix *codecMatrix,int N,int T)
{
	int i;
	// flushing codecMatrix
	memset(codecMatrix->symbolStatus	, 0, sizeof(uint8_t)*N);
	for(i=0;i<N;i++)
		memset(codecMatrix->codewordBox[i], 0, sizeof(uint8_t)*T);
	// end of flushing codecMatrix
}
bool isValidSymbol(CodecMatrix *codecMatrix, int symbolID)
{
	return (bool) codecMatrix->symbolStatus[symbolID];
}

