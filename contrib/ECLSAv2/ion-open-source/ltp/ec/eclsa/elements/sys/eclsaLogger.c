/*
eclsaLogger.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the implementation of the logger utility
used by eclso and eclsi.
 * */

#include "eclsaLogger.h"

#include "eclsaMemoryManager.h"

#include <string.h>  //memcpy
#include <semaphore.h> //semaphores
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct 
{
char 	*filename;
sem_t	lock;
unsigned char init;
bool 	printTimestamp;
bool	printNewLine;
} LogFile;

static int logArrayLength;
static LogFile *logArray;

void loggerInit(int maxLogFiles)
{
if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato

int i;
logArrayLength=maxLogFiles;
logArray = calloc(maxLogFiles, sizeof(LogFile));
//logArray = allocateVector(sizeof(LogFile), maxLogFiles);
for(i=0;i<maxLogFiles;i++)
	{
	sem_init(&(logArray[i].lock),0,1);
	logArray[i].filename=NULL;
	logArray[i].init=0;
	}
}
void loggerDestroy()
{
if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato

int i;
for(i=0;i<logArrayLength;i++)
	{
	sem_destroy(&(logArray[i].lock));
	if(logArray[i].filename!=NULL)
		free(logArray[i].filename);
		//deallocateVector(&(logArray[i].filename));
	}
free(logArray);
//deallocateElement(&(logArray));
}
void loggerStartLog(int loggerID,char *filename,bool printTimestamp, bool printNewLine)
{
	if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato

	if(loggerID<0 || loggerID>=logArrayLength || logArray[loggerID].init)
		{
		printf("Failed to start log: loggerID=%d filename=%s",loggerID,filename);
		return;
		}
	logArray[loggerID].filename = calloc(strlen(filename)+1, sizeof(char));
	//logArray[loggerID].filename = allocateVector(sizeof(char), strlen(filename)+1);
	strcpy(logArray[loggerID].filename,filename);
	logArray[loggerID].init=1;
	logArray[loggerID].printTimestamp=printTimestamp;
	logArray[loggerID].printNewLine=printNewLine;
}
void loggerStopLog(int loggerID,char *filename)
{
	if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato

	if(loggerID<0 || loggerID>=logArrayLength || !logArray[loggerID].init)
		{
		printf("Failed to stop log: loggerID=%d",loggerID);
		return;
		}
	free(logArray[loggerID].filename);
	//deallocateVector(&(logArray[loggerID].filename));
	logArray[loggerID].filename=NULL;
	logArray[loggerID].init=0;
}
void loggerPrintVaList(int loggerID,const char *string,va_list argList)
{
	if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato

	if(loggerID<0 || loggerID>=logArrayLength || !logArray[loggerID].init )
		{
		printf("Failed to print log: loggerID=%d ",loggerID);
		return;
		}

	FILE *fp;
	struct timespec spec;
	unsigned long msTimestamp;
	sem_wait(&(logArray[loggerID].lock));

	fp=fopen(logArray[loggerID].filename, "a");

	if(logArray[loggerID].printTimestamp)
		{
		clock_gettime(CLOCK_REALTIME, &spec);
		msTimestamp =  (spec.tv_sec) * 1000 + (spec.tv_nsec) / 1000000 ;
		fprintf(fp, "[%lu]# ",msTimestamp);
		}

	vfprintf(fp, string, argList);

	if(logArray[loggerID].printNewLine)
		{
		fprintf(fp, "\n");
		}

	fclose(fp);
	sem_post(&(logArray[loggerID].lock));
}
void loggerPrint(int loggerID,const char *string, ...)
{
	if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato
	va_list argList;
	va_start(argList, string);
	loggerPrintVaList(loggerID,string, argList);
	va_end(argList);
}

/*Debug logger shortcuts todo: need macro for these two*/
void debugPrint(const char *string, ...)
{
	if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato
	va_list lp;
	va_start(lp, string);
	loggerPrintVaList(0,string, lp);
	va_end(lp);
}
void packetLogger(const char *string, ...)
{
	if(!LOGGER_ENABLED) return; //todo non fare niente se il logger è disabilitato
	va_list lp;
	va_start(lp, string);
	loggerPrintVaList(1,string, lp);
	va_end( lp );
}
