/*
eclsi.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclsi daemon

 * */


#ifndef _ECLSI_H_

#define _ECLSI_H_

#include "elements/eclsaBoolean.h"

#include "adapters/protocol/eclsaProtocolAdapters.h"
#include "adapters/codec/eclsaCodecAdapter.h"

#include "elements/matrix/eclsaMatrix.h"
#include "elements/matrix/eclsaCodecMatrix.h"
#include "elements/matrix/eclsaMatrixBuffer.h"
#include "elements/packet/eclsaBlacklist.h"
#include "elements/fec/eclsaFecManager.h"
#include "elements/packet/eclsaPacket.h"
#include "elements/packet/eclsaFeedback.h"
#include "elements/sys/eclsaTimer.h"
#include "elements/sys/eclsaLogger.h"

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif


#define FEC_ECLSI_MATRIX_BUFFER 5


/* STRUCTURES */
typedef struct
{
unsigned int 	maxT;
unsigned int 	maxK;
unsigned int	maxN;
int			 	maxWaitingTime;
unsigned long 	globalMatrixID;
unsigned short  feedbackBurst;

unsigned short	portNbr ;
unsigned int	ipAddress;

} EclsiEnvironment;


typedef struct
{
	sem_t 			 		notifyT1;
	sem_t 			 		notifyT2;
	sem_t 			 		notifyT3;
	EclsiEnvironment 	 	*eclsiEnv;
	bool 				 	running;
} EclsiThreadParms;

/*Thread functions*/
static void *fill_matrix_thread(void *parm);
static void	*decode_matrix_thread(void *parm);
static void	*pass_matrix_thread(void *parm);

/*Common functions*/
static void parseCommandLineArgument(int argc, char *argv[], EclsiEnvironment *eclsiEnv);

/*Single eclsa matrix function*/
void decodeEclsaMatrix(EclsaMatrix *matrix);
void passEclsaMatrix(EclsaMatrix*matrix , EclsiEnvironment *eclsiEnv);
void passEclsaMatrix_HSLTP_MODE(EclsaMatrix*matrix , EclsiEnvironment *eclsiEnv);
/*Feedback */
void sendFeedback(EclsaMatrix *matrix,EclsiEnvironment *eclsiEnv);

/* Synchronization */
void sem_notify(sem_t *threadSem);


#ifdef __cplusplus
}
#endif

#endif /* LTP_EC_ECLSA_ECLSI_H_ */
