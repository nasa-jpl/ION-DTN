/*
 eclsaMatrix.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef LTP_EC_ECLSA_TOOLS_ECLSAMATRIX_H_

#define LTP_EC_ECLSA_TOOLS_ECLSAMATRIX_H_

#include "eclsaCodecMatrix.h"
#include <stdbool.h>
#include "../fec/eclsaFecManager.h"
#include "../packet/eclsaSendingSequence.h"
#include "../sys/eclsaTimer.h"
#include "../../extensions/HSLTP/HSLTP_def.h"
#include <semaphore.h>
#include <pthread.h>


typedef struct
{
	/*COMMON*/
	unsigned short 	engineID; //todo manage overflows
	unsigned short  ID; //ex bid , matrix id
	unsigned long 	globalID; //todo manage overflows
	unsigned int	maxInfoSize; //max K
	unsigned int	infoSegmentAddedCount; // <= maxInfoSize
	unsigned int	redundancySegmentAddedCount; // <= N-K
	FecElement 		*encodingCode; //code used for encoding
	CodecMatrix 	*abstractCodecMatrix; //ex universalCodecMatrix
	EclsaTimer 		timer;
	bool			clearedToCodec;
	bool			clearedToSend;
	pthread_mutex_t	lock;
	bool			feedbackEnabled;
	char 			codecStatus;

	/*ONLY FOR ECLSI*/
    int 			workingT;
    void 			*lowerProtocolData;

    /*ONLY FOR ECLSO*/
    SendingSequence sequence;

	/* HSLTP adds */
    HSLTPMatrixType HSLTPMatrixType;
    bool HSLTPModeEnabled;

} EclsaMatrix;



/*Single eclsa matrix functions*/
bool isMatrixInfoPartFull(EclsaMatrix *matrix);
bool isMatrixEmpty(EclsaMatrix *matrix);
bool isInfoSymbol(EclsaMatrix *matrix, int symbolID);
void eclsaMatrixInit(EclsaMatrix *matrix, unsigned int N, unsigned int T);
void eclsaMatrixDestroy(EclsaMatrix *matrix);
void addSegmentToEclsaMatrix(EclsaMatrix *matrix, char *buffer, int bufferLength, int symbolID,bool copyLength);
void flushEclsaMatrix(EclsaMatrix *matrix);

#endif
