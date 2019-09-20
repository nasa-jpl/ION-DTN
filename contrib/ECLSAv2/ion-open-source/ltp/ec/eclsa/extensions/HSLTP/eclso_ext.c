/*
eclso_ext.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file implements eclso extension for HSLTP mode

 * */

#include <stdlib.h>
#include "HSLTP.h"
#include "../../eclso.h"
#include "../../elements/sys/eclsaLogger.h"
#include "../../elements/sys/eclsaMemoryManager.h"
#include "../../elements/matrix/eclsaMatrixPool.h"

void *fill_matrix_thread_HSLTP_MODE(void *parm) // thread T1
{
	//This is the main thread, denoted by "T1";
	//this thread is in charge of filling a matrix with the segments received from
	//LTP (or another upper layer)
	EclsoThreadParms	 *thread = (EclsoThreadParms *) parm;
	EclsoEnvironment 	 *eclsoEnv=		  thread->eclsoEnv;
	EclsaMatrix 		 *matrix;

	int processingType;
	int 			segmentLength;
	char			*segment;

	segment = (char*) allocateVector(sizeof(char), eclsoEnv->T -2);
	eclsoEnv->maxInfoSize=getBiggestFEC()->K;

	debugPrint("log: thread T1 running (fill_matrix_thread)");

	while(thread->running)
		{
		processingType = receiveSegmentFromUpperProtocol_HSLTP_MODE(segment,&segmentLength);
		while ( (matrix=getMatrixToFill(eclsoEnv->globalMatrixID,eclsoEnv->ownEngineId,getBiggestFEC()->N,getBiggestFEC()->T))==NULL)
			{
			debugPrint("T1: sleeping... all matrix full");
			sem_wait(&(thread->notifyT1));
			}

		pthread_mutex_lock(&(matrix->lock));

		if( isMatrixEmpty(matrix) )
			{
			matrix->ID=		  		 eclsoEnv->globalMatrixID;
			matrix->globalID= 		 eclsoEnv->globalMatrixID;
			matrix->engineID=		 eclsoEnv->ownEngineId;
			matrix->maxInfoSize=	 eclsoEnv->maxInfoSize;
			matrix->feedbackEnabled= eclsoEnv->feedbackRequestEnabled;
			matrix->HSLTPModeEnabled = eclsoEnv->HSLTPModeEnabled;
// EOB ?
			if(processingType == DEFAULT_PROC || processingType == END_OF_MATRIX)
					{
						matrix->HSLTPMatrixType = ONLY_DATA;
						timerStart(&(matrix->timer),
							eclsoEnv->maxAggregationTime,
							matrix,
							&(matrix->ID),
							&(matrix->engineID));
					debugPrint("T1: start to fill new matrix MID=%d EID=%d default proc", matrix->ID, matrix->engineID);
					}
			if (processingType == SPECIAL_PROC)
					{
						matrix->HSLTPMatrixType = ONLY_SIGNALING;

						timerStart(&(matrix->timer),
								SPECIAL_PROC_TIMER,
								matrix,
								&(matrix->ID),
								&(matrix->engineID));
						debugPrint("T1: start to fill new matrix MID=%d EID=%d special proc", matrix->ID, matrix->engineID);
					}
			}
			else
			{
				if (processingType == SPECIAL_PROC && matrix->HSLTPMatrixType == ONLY_DATA)
					{
					matrix->HSLTPMatrixType = SIGNALING_AND_DATA;
					}
				if(processingType == DEFAULT_PROC && matrix->HSLTPMatrixType == ONLY_SIGNALING)
					{
					matrix->HSLTPMatrixType = SIGNALING_AND_DATA;
					timerStop(&(matrix->timer));
					timerStart(&(matrix->timer),
										eclsoEnv->maxAggregationTime,
										matrix,
										&(matrix->ID),
										&(matrix->engineID));
					debugPrint("T1: change to signaling and data matrix MID=%d EID=%d special proc", matrix->ID, matrix->engineID);
					}
			}
		addSegmentToEclsaMatrix(matrix,segment,segmentLength,matrix->infoSegmentAddedCount,true);
		//debugPrint("T1: segment of type %d added to the matrix", processingType);

		if ( isMatrixInfoPartFull(matrix) || processingType == END_OF_MATRIX)
			{
				timerStop(&(matrix->timer));
				eclsoEnv->globalMatrixID++;
				matrix->clearedToCodec=true;
				sem_notify(&(thread->notifyT2));
				if (processingType == END_OF_MATRIX)
					debugPrint("T1: MID=%d received EOB, stop timer, wake up T2!", matrix->ID);
				else
					debugPrint("T1: MID=%d full, stop timer, wake up T2!", matrix->ID);
			}

		pthread_mutex_unlock(&(matrix->lock));
		}

deallocateVector(&(segment));
return NULL;
}
