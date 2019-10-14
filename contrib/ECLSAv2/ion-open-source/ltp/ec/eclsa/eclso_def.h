/*
eclso_def.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclso daemon

 * */
#ifndef _ECLSO_DEF_H_

#define _ECLSO_DEF_H_

#include <stdbool.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ECLSO_DESTROY_MEMORY_LEAK true		// Possibilities: true, false. True: leakage will be deallocated at the end, false it will not
#define ECLSO_MAX_MEMORY_SIZE 1				// Size, linked to MAX_MEMORY_TYPE
#define ECLSO_MAX_MEMORY_TYPE GIGABYTE		// Possibilities: BYTE, KILOBYTE, MEGABYTE, GIGABYTE

#define ECLSO_MAX_DYNAMIC_MATRIX 100
#define FEC_ECLSO_MATRIX_BUFFER 3
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
	bool 			HSLTPModeEnabled;
	bool			HSLTPProactiveFragmentationEnabled;
	unsigned long	globalMatrixID; //matrix arrival order at eclsi, independent of engineID. In eclso is equal to matrixID
	unsigned short 	ownEngineId;
	int 			maxInfoSize;
	float			estimatedSuccessRate; //current estimated success rate

	unsigned short	portNbr ;
	unsigned int	ipAddress;
	int 			txbps;
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


#ifdef __cplusplus
}
#endif

#endif
