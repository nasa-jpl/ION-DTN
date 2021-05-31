/*
eclsi_def.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclsi daemon

 * */


#ifndef _ECLSI_DEF_H_

#define _ECLSI_DEF_H_


#include <stdbool.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ECLSI_DESTROY_MEMORY_LEAK true		// Possibilities: true, false. True: leakage will be deallocated at the end, false it will not
#define ECLSI_MAX_MEMORY_SIZE 1				// Size, linked to MAX_MEMORY_TYPE
#define ECLSI_MAX_MEMORY_TYPE GIGABYTE		// Possibilities: BYTE, KILOBYTE, MEGABYTE, GIGABYTE

#define ECLSI_MAX_DYNAMIC_MATRIX 100
#define FEC_ECLSI_MATRIX_BUFFER 1			// for ideal configuration this should be low, because a dinamic matrix is better then a static matric in ECLSI


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
