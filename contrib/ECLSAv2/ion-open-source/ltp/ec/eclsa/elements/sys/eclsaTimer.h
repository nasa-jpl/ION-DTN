/*
eclsaTimer.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions of the timer utility
used by eclso and eclsi.

 * */

#ifndef _ECTIMER_H_
#define _ECTIMER_H_

#include <pthread.h> //POSIX threads
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void (*funcPointer)();

typedef struct
{
	unsigned int	ID;
	pthread_t 		thread;
	bool 			rewind;
} EclsaTimer;


void timerInit(funcPointer handler, void *userData); //initialize the timer utility
void timerDestroy(); //destroy the timer utility
void timerStart(EclsaTimer *timer,int expireMillis, void *matrixData, unsigned short *matrixID, unsigned short *engineID);
void timerStop(EclsaTimer *timer); //stop an existing timer
void timerRewind(EclsaTimer *timer); //rewind an existing timer
void timerHandler(unsigned int timerID,void *matrixData,void *userData); //function called when a timer elapses

#ifdef __cplusplus
}
#endif

#endif
