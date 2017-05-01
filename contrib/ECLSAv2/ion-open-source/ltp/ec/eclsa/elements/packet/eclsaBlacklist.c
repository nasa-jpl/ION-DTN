/*
 eclsaBlacklist.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#include "eclsaBlacklist.h"

#include <semaphore.h>
#define FEC_BLACKLIST_SIZE 10		  //for eclsi

typedef struct
{
	unsigned short engineID;
	unsigned short matrixID;
	bool 		   valid;
} BlackListElement;
typedef struct
{
	BlackListElement array[FEC_BLACKLIST_SIZE];
	sem_t		lock;
	int currentIndex;
} BlacklistCircularBuffer;

static BlacklistCircularBuffer blacklist;

/*Blacklist functions*/
void blacklistInit()
{
	int i;
	sem_init(&(blacklist.lock),0,1);
	blacklist.currentIndex=0;
	for(i=0;i< FEC_BLACKLIST_SIZE;i++)
		{
		blacklist.array[i].engineID=0;
		blacklist.array[i].matrixID=0;
		blacklist.array[i].valid=false;
		}
}
void blacklistDestroy()
{
	sem_destroy(&(blacklist.lock));
}
void addToBlacklist(unsigned short engineID, unsigned short matrixID)
{
	BlackListElement *element;
	sem_wait(&(blacklist.lock));
	element=&(blacklist.array[blacklist.currentIndex]);
	element->engineID=engineID;
	element->matrixID=matrixID;
	element->valid=true;
	blacklist.currentIndex = (blacklist.currentIndex+1) % FEC_BLACKLIST_SIZE;
	sem_post(&(blacklist.lock));
}
bool isBlacklisted(unsigned short engineID,unsigned short matrixID)
{
	int i;
	BlackListElement *element;
	bool result=false;
	sem_wait(&(blacklist.lock));
	for(i=0;i< FEC_BLACKLIST_SIZE; i++)
		{
		element=&(blacklist.array[i]);
		if(element->valid && element->engineID == engineID && element->matrixID == matrixID)
			{
			result=true;
			break;
			}
		}

	sem_post(&(blacklist.lock));
	return result;
}
