/*
eclsi.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file implements eclsi, the Forward Error Correction Link
Service Induct daemon.

 * */

#include "eclsi.h"

#include <stdlib.h>		// exit, ..
#include <unistd.h> 	// usleep
#include <pthread.h>	// POSIX threads
#include <string.h>		// memset
#include <stdint.h> //uint_16t, uint_8...

#include "adapters/protocol/eclsaProtocolAdapters.h"
#include "adapters/codec/eclsaCodecAdapter.h"
#include "elements/matrix/eclsaMatrixPool.h"
#include "elements/packet/eclsaBlacklist.h"
#include "elements/packet/eclsaPacket.h"
#include "elements/packet/eclsaFeedback.h"
#include "elements/sys/eclsaLogger.h"
#include "elements/sys/eclsaMemoryManager.h"
#include "extensions/HSLTP/HSLTP.h"
//#include "elements/sys/eclsaTimer.h"
//#include "elements/fec/eclsaFecManager.h"
//#include "elements/matrix/eclsaCodecMatrix.h"


#define PARSE_PARAMETER(PAR_NO) 	(argc > (PAR_NO) ? (int) strtoll(argv[PAR_NO], NULL, 10) : -1)

int	main(int argc, char *argv[])
{
	EclsiEnvironment 	eclsiEnv;
	EclsiThreadParms 	thread;
	pthread_t			decodeMatrixThread;
	pthread_t			sendMatrixThread;

	initMemoryManager(ECLSI_MAX_MEMORY_SIZE, ECLSI_MAX_MEMORY_TYPE);

	loggerInit(2);
	loggerStartLog(0,"outputECLSI.log",true,true); //can be used debugPrint() shortcut instead of loggerPrint()
	loggerStartLog(1,"eclsiReceivedPackets.log",true,true); //can be used packetLogger() shortcut instead of loggerPrint()

	parseCommandLineArgument(argc,argv,&eclsiEnv);

	initEclsiUpperLevel(argc,argv,&eclsiEnv);
	initEclsiLowerLevel(argc,argv,&eclsiEnv);

	fecManagerInit(true,false,isContinuousModeAvailable(),eclsiEnv.maxK,eclsiEnv.maxN,eclsiEnv.maxT);

	matrixPoolInit(FEC_ECLSI_MATRIX_BUFFER, ECLSI_MAX_DYNAMIC_MATRIX);
	blacklistInit();
	timerInit(timerHandler,&thread);

	sem_init(&(thread.notifyT1),0,0);
	sem_init(&(thread.notifyT2),0,0);
	sem_init(&(thread.notifyT3),0,0);

	thread.eclsiEnv=&eclsiEnv;
	thread.running= true;

	if (pthread_create(&decodeMatrixThread, NULL, decode_matrix_thread, &thread))
	{
		debugPrint("ERROR: eclso can't create encode matrix thread");
		exit(1);
	}
	if (pthread_create(&sendMatrixThread, NULL, pass_matrix_thread, &thread))
	{
		debugPrint("ERROR: eclso can't create send matrix thread");
		exit(1);
	}

	//Once entered the following function, the program shoud never return until active.
	fill_matrix_thread(&thread);

	//We can be here only if an error has occurred in fill_matrix_thread;
	//hence, we must wait for the termination of the other threads (join),
	//then we have to free the memory and exit.

	debugPrint("T1: Join with thread T2");
	pthread_join(decodeMatrixThread,NULL);
	debugPrint("T1: Join with thread T3");
	pthread_join(sendMatrixThread,NULL);

	sem_destroy(&(thread.notifyT1));
	sem_destroy(&(thread.notifyT2));
	sem_destroy(&(thread.notifyT3));

	blacklistDestroy();
	matrixPoolDestroy();
	fecManagerDestroy();
	timerDestroy();
	debugPrint("T1: memory cleared");
	destroyMemoryManager(ECLSI_DESTROY_MEMORY_LEAK);
	loggerDestroy();

	return 0;
}

/*Thread functions */
static void *fill_matrix_thread(void *parm) // thread T1
{
	//This is the main thread, denoted by "T1"
	//this thread is in charge of filling a matrix with the eclsa packets received from
	//UDP (or another lower layer).
	EclsiThreadParms		*thread = (EclsiThreadParms *) parm;
	EclsiEnvironment	 	*eclsiEnv= 	  	 thread->eclsiEnv;

	EclsaHeader header;
	FecElement 	*encodingCode;
	EclsaMatrix *matrix;

	char buffer[FEC_LOWERLEVEL_MAX_PACKET_SIZE];
	int  bufferLength;
	char *upperLevelBuffer;
	int  upperLevelBufferLength;

	void *lowerProtocolData;
	unsigned int lowerProtocolDataLenght;

	debugPrint("thread T1 running (fill_matrix_thread)");

	while (thread->running)
		{
		receivePacketFromLowerProtocol(buffer,&bufferLength,&lowerProtocolData,&lowerProtocolDataLenght);

		if(!parseEclsaIncomingPacket(&header,&encodingCode,buffer,bufferLength,&upperLevelBuffer,&upperLevelBufferLength,eclsiEnv->maxT))
			{
			//todo: remove comment only for debug purposes!
			//packetLogger("EC Packet Received. Parsing Failed. BufferLength: %d",bufferLength);
			continue;
			}

		packetLogger("EC Length=%d T=%d EID=%d extCount=%d MID=%d PID=%d Added=%d K=%d N=%d",
				bufferLength,header.T,header.engineID,header.extensionCount,header.matrixID,header.symbolID,header.segmentsAdded,header.K,header.N);

		//todo remove comments only for debug purposes.
		//todo decommentando alcune di queste righe verranno scartati i pacchetti indicati. questo
		//pu� servire per fare dei test con un certo numero di perdite in modo deterministico
		//if(header.symbolID %2 == 0 || header.symbolID == 575 || header.symbolID==18431) continue;
	    //if(header.symbolID %2 == 0 ) continue;
		//if(header.symbolID  == 0 ) continue;
		//if(header.symbolID  <15) continue;

		if(isUncodedEclsaPacket(&header))
			{
			if((header.flags & HSLTP_MODE_MASK) != 0)
				sendSegmentToUpperProtocol_HSLTP_MODE(upperLevelBuffer,&upperLevelBufferLength,STATUS_CODEC_NOT_DECODED,false);
			else
				sendSegmentToUpperProtocol(upperLevelBuffer,&upperLevelBufferLength);
			
			debugPrint("Uncoded Eclsa Packet received: payload sent to the upper level protocol.");
			continue;
			}

		if(isBlacklisted(header.engineID,header.matrixID))
			{
				continue;
			}


		while( (matrix=getMatrixToFill(header.matrixID,header.engineID, header.N, header.T)) == NULL )
			{
				debugPrint("T1: matrix buffer full");
				sem_wait(&(thread->notifyT1));
			}

		pthread_mutex_lock(&(matrix->lock));

		//This is the same check as before; it must be repeated because a long time may elapse
		//waiting for a free matrix in the previous while.
		if(isBlacklisted(header.engineID,header.matrixID))
			{
			pthread_mutex_unlock(&(matrix->lock));
			continue;
			}

		timerRewind(&(matrix->timer));

		if(isMatrixEmpty(matrix))
			{
				matrix->ID= 			header.matrixID;
				matrix->engineID = 		header.engineID;
				matrix->encodingCode =	startMatrixFecElement(encodingCode);
				matrix->workingT=		header.T;
				matrix->maxInfoSize = 	header.segmentsAdded;
				matrix->globalID= 		eclsiEnv->globalMatrixID;
				matrix->feedbackEnabled=(header.flags & FEEDBACK_REQUEST_MASK) != 0;
				matrix->HSLTPModeEnabled = (header.flags & HSLTP_MODE_MASK) != 0;

				matrix->lowerProtocolData = allocateElement(lowerProtocolDataLenght);
				memcpy(matrix->lowerProtocolData,lowerProtocolData,lowerProtocolDataLenght);

				eclsiEnv->globalMatrixID++;
				//timerStart(matrix,eclsiEnv->maxWaitingTime);
				timerStart(&(matrix->timer),
									eclsiEnv->maxWaitingTime,
									matrix,
									&(matrix->ID),
									&(matrix->engineID));

				debugPrint("T1: start to fill a new matrix MID=%d EID=%d I=%d/%d R=%d/%d",
						matrix->ID, matrix->engineID,
						matrix->infoSegmentAddedCount,matrix->maxInfoSize,
						matrix->redundancySegmentAddedCount,matrix->encodingCode->N - matrix->encodingCode->K);
			}

		//Check for possible information inconsistency: if the eclsa packet has the same matrixID and engineID
		//as previous ones, but different additional information, this packet and all matrix segments must be
		//discarded and the couple (matrixID,engineID) put in blacklist

		if( matrix->encodingCode->K != encodingCode->K || matrix->encodingCode-> N != encodingCode->N ||
			//	matrix->encodingCode->T != encodingCode->T ||  todo
				 matrix->maxInfoSize != header.segmentsAdded || header.T != matrix->workingT)
			{
				debugPrint("T1: ERROR! Discarding matrix and putting (MID=%d,EID=%d) into blacklist");
				timerStop(&(matrix->timer));
				addToBlacklist(matrix->engineID,matrix->ID);
				debugPrint("T1: added (MID=%d,EID=%d) to blacklist",matrix->ID,matrix->engineID);
				flushMatrixFromPool(&matrix);
				continue;
			}

		addSegmentToEclsaMatrix(matrix,upperLevelBuffer,upperLevelBufferLength,header.symbolID,false);

		// Segment added to decoding matrix. If necessary, notify either T2 or T3

		if (isMatrixInfoPartFull(matrix) && isInfoSymbol(matrix,header.symbolID) )
			{
			debugPrint("T1: matrix filled. Status:\"Got all info, skip dec\" MID=%d EID=%d GID=%d N=%d K=%d I=%d/%d R=%d/%d",matrix->ID,matrix->engineID,matrix->globalID,matrix->encodingCode->N,matrix->encodingCode->K,matrix->infoSegmentAddedCount,matrix->maxInfoSize,matrix->redundancySegmentAddedCount,matrix->encodingCode->N-matrix->encodingCode->K);
			timerStop(&(matrix->timer));
			///By setting clearedToCode=true the matrix is passed to T2, although T2 has actually nothing
			//to do in this case but possing to T3, in order to mantain the matrix arrival order.
			matrix->clearedToCodec=true;
			sem_notify(&(thread->notifyT2));
			//todo: se l'arrivo fuori ordine non interessa bisogna usare questo:
			//matrix->clearedToSend=true;
			//sem_notify(&(thread->notifyT3));
			}

		if(header.symbolID == matrix->encodingCode->N -1) //last redundancy segment
			{
			debugPrint("T1: matrix received. Status:\"Got last R\" MID=%d EID=%d GID=%d N=%d K=%d I=%d/%d R=%d/%d",matrix->ID,matrix->engineID,matrix->globalID,matrix->encodingCode->N,matrix->encodingCode->K,matrix->infoSegmentAddedCount,matrix->maxInfoSize,matrix->redundancySegmentAddedCount,matrix->encodingCode->N-matrix->encodingCode->K);
			timerStop(&(matrix->timer));
			matrix->clearedToCodec=true;
			sem_notify(&(thread->notifyT2));
			}

		if(forcePreviousMatrixToDecode(matrix))
			{
			sem_notify(&(thread->notifyT2));
			}

		pthread_mutex_unlock(&(matrix->lock));
		}

	return NULL;
}
static void	*decode_matrix_thread(void *parm) 	// thread T2
{
	//This thread is in charge of matrix decoding
	EclsiThreadParms	*thread = (EclsiThreadParms *) parm;
	EclsaMatrix 		*matrix;
	debugPrint("log: thread T2 running (decode_matrix_thread)");

	while(thread->running)
	{
	debugPrint("T2: sleeping...");
	sem_wait(&(thread->notifyT2));
	debugPrint("T2: awake!");

	while( (matrix=getMatrixToCode()) != NULL)
		{
		debugPrint("T2: iteration...");
		pthread_mutex_lock(&(matrix->lock));

		if (!isMatrixInfoPartFull(matrix))
			decodeEclsaMatrix(matrix);

		matrix->clearedToSend=true;
		sem_notify(&(thread->notifyT3));
		pthread_mutex_unlock(&(matrix->lock));
		}
	}

	return NULL;
}
static void	*pass_matrix_thread(void *parm) 	//thread T3
{
	//This thread is in charge of passing the info segments of a matrix to LTP (or to another upper layer)
	EclsiThreadParms		*thread = (EclsiThreadParms *) parm;
	EclsiEnvironment 		*eclsiEnv = thread->eclsiEnv;
	EclsaMatrix 			*matrix;
	debugPrint("log: thread T3 running (pass_matrix_thread)");

	while(thread->running)
	{
	debugPrint("T3: sleeping...");
	sem_wait(&(thread->notifyT3));
	debugPrint("T3: awake!");

	while( (matrix=getMatrixToSend()) != NULL)
		{
		debugPrint("T3: iteration");
		pthread_mutex_lock(&(matrix->lock));
		debugPrint("T3: passing MID=%d EID=%d GID=%d to upper protocol", matrix->ID,matrix->engineID,matrix->globalID);
		addToBlacklist(matrix->engineID,matrix->ID);
		debugPrint("T3: Added (MID=%d,EID=%d) to blacklist",matrix->ID,matrix->engineID);

		if(matrix->HSLTPModeEnabled)
			passEclsaMatrix_HSLTP_MODE(matrix,eclsiEnv);
		else
			passEclsaMatrix(matrix,eclsiEnv);

		if(matrix->feedbackEnabled)
			sendFeedback(matrix,eclsiEnv);
		flushMatrixFromPool(&matrix);
		sem_notify(&(thread->notifyT1));
		}
	}
	return NULL;
}

/*Common functions*/
static void parseCommandLineArgument(int argc, char *argv[], EclsiEnvironment *eclsiEnv)
{
	const int parameters=5;
	if(argc < parameters)
		{
		debugPrint("wrong usage. missing some parameters..");
		exit(1);
		}
	if(argc > parameters + 1)
		{
		debugPrint("wrong usage. too much parameters..");
		exit(1);
		}
	memset(eclsiEnv,0,sizeof(EclsiEnvironment));

	eclsiEnv->maxWaitingTime= PARSE_PARAMETER(2);
	eclsiEnv->feedbackBurst = PARSE_PARAMETER(3);
	eclsiEnv->maxT= PARSE_PARAMETER(4);
	eclsiEnv->maxN=FEC_MAX_N;

	if(argc >= parameters + 1)
		eclsiEnv->maxK = PARSE_PARAMETER(5);
	else
		eclsiEnv->maxK= FEC_MAX_K;
}

/*Single eclsa matrix functions*/
void decodeEclsaMatrix(EclsaMatrix *matrix)
{
	void *codecMatrix= matrix->abstractCodecMatrix;
	FecElement 	*code= matrix->encodingCode;
	code->T=matrix->workingT;

	matrix->codecStatus=decodeCodecMatrix(codecMatrix,code,matrix->maxInfoSize,code->K);

	debugPrint("T2: decoder info: %s "
				"MID=%d EID=%d GID=%d "
				"N=%d K=%d "
				"I=%d/%d "
				"R=%d/%d "				,getCodecStatusString(matrix->codecStatus),
										 matrix->ID,matrix->engineID,matrix->globalID,
										 code->N,code->K,
										 matrix->infoSegmentAddedCount,matrix->maxInfoSize,
										 matrix->redundancySegmentAddedCount,code->N-code->K);

}
void passEclsaMatrix(EclsaMatrix*matrix , EclsiEnvironment *eclsiEnv)
{
unsigned int i;
FecElement 	*code= 		  matrix->encodingCode;
char *segment;
uint16_t tmpSegLen;
int segmentLength;
debugPrint("T3: LTP standard");

for(i=0;i < code->K;i++)
	{
	segment= getSymbolFromCodecMatrix(matrix->abstractCodecMatrix,i);
	memcpy(&tmpSegLen,segment, sizeof(uint16_t));
	//Only the matrix segments that have a length > 0 (no padding) and have been flagged as valid
	//by the decoder, must be sent to LTP (or to another upper protocol).
	if(tmpSegLen > 0 && isValidSymbol(matrix->abstractCodecMatrix,i) )
		{
			segmentLength=(int)tmpSegLen;
			sendSegmentToUpperProtocol((char *)(segment+ sizeof(uint16_t)),&segmentLength);
		}
	}
}

/*Feedback functions*/
void sendFeedback(EclsaMatrix *matrix,EclsiEnvironment *eclsiEnv)
{
	EclsaFeedback feedback;
	int size= sizeof(EclsaFeedback);
	int i;

	createEclsaFeedback(&feedback,matrix);

	//todo endianness conversions.

	for(i=0;i<eclsiEnv->feedbackBurst;i++)
		sendPacketToLowerProtocol((char *)&feedback,&size,matrix->lowerProtocolData);

	debugPrint("T3: feedback sent decodingState=%s receivedSegments=%d totalSegments=%d",getCodecStatusString(feedback.codecStatus),feedback.receivedSegments,feedback.totalSegments);
	}

/*Timer functions */
void timerHandler(unsigned int timerID,void *matrixData,void *userData)
{
/*
To be thread-safe, a timer handler must contain the following instructions:
 sem_wait(&(matrix->lock));
 if(matrix->timer.ID == timerID)
	 {
	 ...
	 }
  sem_post(&(matrix->lock));
 */
debugPrint("timer handler");
EclsiThreadParms *thread=(EclsiThreadParms *) userData;
EclsaMatrix *matrix=(EclsaMatrix *) matrixData;

pthread_mutex_lock(&(matrix->lock));
if(!matrix->clearedToCodec && matrix->timer.ID == timerID)
	{
	debugPrint("T1: matrix received.Status:\"Timeout\" MID=%d EID=%d GID=%d N=%d K=%d I=%d/%d R=%d/%d",matrix->ID,matrix->engineID,matrix->globalID,matrix->encodingCode->N,matrix->encodingCode->K,matrix->infoSegmentAddedCount,matrix->maxInfoSize,matrix->redundancySegmentAddedCount,matrix->encodingCode->N-matrix->encodingCode->K);
	matrix->clearedToCodec=true;
	sem_notify(&(thread->notifyT2));
	}
pthread_mutex_unlock(&(matrix->lock));
}


/*Synchronization */
void sem_notify(sem_t *threadSem)
{
//todo: is this the best way?
int val;
sem_getvalue(threadSem,&val);
if(val==0)
	sem_post(threadSem);
}
