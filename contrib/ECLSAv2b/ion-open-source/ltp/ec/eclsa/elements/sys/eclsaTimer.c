/*
eclsaTimer.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the implementation of the timer utility
used by eclso and eclsi. This implementation is based on
POSIX threads.

 * */


#include "eclsaTimer.h"
#include "eclsaLogger.h"
#include <unistd.h>
#include <pthread.h> //POSIX threads
#include <string.h>  //memcpy
#include <semaphore.h>
#include <stdlib.h> //exit

static funcPointer 	timerHandlerPtr;
static void 		*userDataPtr=NULL;
static sem_t		waitCopySem;
static unsigned int currentID=1;
static bool 		timerUtilityRunning=false;

typedef struct
{
int 			milliseconds;
unsigned int 	timerID;

EclsaTimer *timer;
void 		   *matrixData;
unsigned short *actualEngineID;
unsigned short *actualMatrixID;
unsigned short 	engineID;
unsigned short 	matrixID;
} TimerCoreThreadPar;

static void *timerThreadCore(void *parm)
{
	TimerCoreThreadPar timerEnv;
	memcpy(&timerEnv,parm,sizeof(TimerCoreThreadPar));
	sem_post(&waitCopySem);
	do
	{
		timerEnv.timer->rewind=false;
		sleep(timerEnv.milliseconds/1000);
		usleep( (timerEnv.milliseconds%1000) *1000 );

	} while(timerEnv.timer->rewind && timerUtilityRunning);
	debugPrint("timer elapsed");
	if (//if actual == previous (saved during timerStart)
		timerEnv.timer->ID 		    == timerEnv.timerID   &&
		*timerEnv.actualEngineID 	== timerEnv.engineID  &&
		*timerEnv.actualMatrixID	== timerEnv.matrixID  &&
		timerUtilityRunning)
			{
				timerHandlerPtr(timerEnv.timerID,timerEnv.matrixData,userDataPtr);
			}
	return NULL;
}
void timerInit(funcPointer handler,void *userData)
{
	if(timerUtilityRunning)
		{
		debugPrint("WARNING: cannot start another timer utility");
		return;
		}
	timerHandlerPtr=handler;
	userDataPtr=userData;
	sem_init(&waitCopySem,0,0);
	timerUtilityRunning=true;
}
void timerDestroy()
{
	if(!timerUtilityRunning)
			{
			debugPrint("WARNING: cannot destroy timer utility");
			return;
			}
	sem_destroy(&waitCopySem);
	timerUtilityRunning=false;
}
void timerStart(EclsaTimer *timer,int expireMillis, void *matrixData, unsigned short *matrixID, unsigned short *engineID)
{
	if(!timerUtilityRunning)
		{
		debugPrint("ERROR:cannot start timer because timer utility is not running.");
		exit(1);
		}
	TimerCoreThreadPar parm;
	pthread_attr_t threadAttribute;
	int err;

	//save by pointer
	parm.actualEngineID=engineID;
	parm.actualMatrixID=matrixID;
	parm.timer=timer;
	parm.matrixData=matrixData;

	//save by value
	parm.engineID=*engineID;
	parm.matrixID=*matrixID;
	parm.timerID=currentID;
	parm.milliseconds=expireMillis;

	//update current timer status
	timer->ID=currentID;
	timer->rewind=false;
	currentID++;

	//overflow protection, 0 means timer not set or stopped!
	if(currentID == 0) currentID++;

	//detached to avoid memory leaks (join is not needed)
	pthread_attr_init(&threadAttribute);
	pthread_attr_setdetachstate(&threadAttribute, PTHREAD_CREATE_DETACHED);

	if( (err=pthread_create(&(timer->thread), &threadAttribute, timerThreadCore, &parm)) != 0 )
		{
		debugPrint("ERROR:cannot start timer %d",err);
		exit(1);
		}
	sem_wait(&waitCopySem);
}
void timerStop(EclsaTimer *timer) //to be used during matrix lock to be thread safe
{
if(timer->ID != 0)
	{
	pthread_cancel(timer->thread);
	timer->ID=0;
	timer->thread=0;
	}
}
void timerRewind(EclsaTimer *timer)  //to be used during matrix lock to be thread safe
{
timer->rewind=true;
}
