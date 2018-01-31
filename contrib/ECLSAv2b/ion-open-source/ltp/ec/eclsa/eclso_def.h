/*
eclso_def.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclso daemon

 * */
#ifndef _ECLSO_DEF_H_

#define _ECLSO_DEF_H_

#include "elements/eclsaBoolean.h"
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
