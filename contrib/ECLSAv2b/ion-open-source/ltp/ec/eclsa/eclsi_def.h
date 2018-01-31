/*
eclsi_def.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclsi daemon

 * */


#ifndef _ECLSI_DEF_H_

#define _ECLSI_DEF_H_


#include "elements/eclsaBoolean.h"
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


#ifdef __cplusplus
}
#endif

#endif
