/*
eclso.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclso daemon

 * */
#ifndef _ECLSO_H_

#define _ECLSO_H_

#include "adapters/protocol/eclsaProtocolAdapters.h"
#include "adapters/codec/eclsaCodecAdapter.h"
#include "elements/eclsaBoolean.h"
#include "elements/matrix/eclsaMatrix.h"
#include "elements/matrix/eclsaMatrixBuffer.h"
#include "elements/matrix/eclsaCodecMatrix.h"
#include "elements/sys/eclsaLogger.h"
#include "elements/sys/eclsaTimer.h"
#include "elements/packet/eclsaFeedback.h"
#include "elements/packet/eclsaPacket.h"
#include "elements/packet/eclsaSendingSequence.h"
#include "elements/fec/eclsaFecManager.h"

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FEC_ECLSO_MATRIX_BUFFER 5
#define FEEDBACK_DEFAULT_WEIGHT 0.5F  //for eclso
#define FEEDBACK_DEFAULT_RELIABILITY_THRESHOLD 65 //for eclso




/* SRTUCTURE DEFINITIONS */

typedef struct
{
	unsigned int 	K; //info elements
	unsigned int 	N; //info + redundancy elements
	unsigned int 	T; //length of a matrix row
	unsigned int 	maxAggregationTime; //in seconds
	unsigned int 	codingThreshold;
	bool 			adaptiveModeEnabled;
	bool			feedbackRequestEnabled;  //ask eclsi for a feedback
	bool			feedbackAdaptiveRcEnabled; //adaptive code rate based on feedback
	bool			staticMidEnabled; //first MID random
	bool 			interleavingEnabled;
	bool			puncturingEnabled; //puncture redundancy to decrease the code rate
	bool 			continuousModeEnabled;
	unsigned long	globalMatrixID; //matrix arrival order at eclsi, independent of engineID. In eclso is equal to matrixID
	unsigned short 	ownEngineId;
	int 			maxInfoSize;
	float			estimatedSuccessRate; //current estimated success rate

	unsigned short	portNbr ;
	unsigned int	ipAddress;
	int 			txbps;
	bool 			HSLTP_mode;
} EclsoEnvironment;

typedef struct
{
	sem_t				 notifyT1;
	sem_t 	    		 notifyT2;
	sem_t   			 notifyT3;
	EclsoEnvironment 	 *eclsoEnv;
	bool 				 running;
} EclsoThreadParms;

/* END OF STRUCTURE DEFINIIONS*/

typedef enum
	{
	MASK_ADAPTIVE_MODE 			= 1  ,
	MASK_CONTINUOUS_MODE		= 2  ,
	MASK_FEEDBACK_REQUEST 		= 4  ,
	MASK_FEEDBACK_ADAPTIVE_RC 	= 8  ,
	MASK_INTERLEAVING 			= 16 ,
	MASK_STATIC_MID				= 32 ,
	MASK_HSLTP					= 64 ,
	} EclsoOptionFlags;

/* FUNCTIONS */

/*Thread functions*/
static void *fill_matrix_thread(void *parm);
static void *fill_matrix_thread_HSLTP_MODE(void *parm);
static void	*encode_matrix_thread(void *parm);
static void	*pass_matrix_thread(void *parm);
static void *feedback_handler_thread(void *parm);

/*Common functions*/
static void parseCommandLineArgument(int argc, char *argv[], EclsoEnvironment *eclsoEnv);

/*Single eclsa matrix functions*/
void encodeEclsaMatrix(EclsoEnvironment *eclsoEnv, EclsaMatrix *matrix);
void passEclsaMatrix(EclsoEnvironment *eclsoEnv, EclsaMatrix *matrix);

/* Synchronization */
void sem_notify(sem_t *threadSem);


#ifdef __cplusplus
}
#endif

#endif /* LTP_EC_ECLSA_ECLSO_H_ */
