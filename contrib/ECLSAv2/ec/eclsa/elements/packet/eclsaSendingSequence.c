/*
 eclsaSendingSequence.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#include "eclsaSendingSequence.h"
#include "../sys/eclsaLogger.h"
#include "../sys/eclsaMemoryManager.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

void sequenceInit(SendingSequence *sendingSequence,int maxSize)
{
	sendingSequence->values = (int*) allocateVector(sizeof(int), maxSize);
	sendingSequence->length=0;
	srand(time(NULL));
}
void sequenceDestroy(SendingSequence *sendingSequence)
{
	deallocateVector(sendingSequence);
}
void sequenceReload(SendingSequence *sendingSequence,FecElement *fec,bool addRedundancy,bool interleavingEnabled, bool puncturingEnabled, unsigned int infoSegmentAddedCount)
{
	unsigned int i=0;

	sendingSequence->length=0;

	//Add info segment indexes to the sequence (but padding)
	for (i=0; i< infoSegmentAddedCount ; i++)
		{
			sendingSequence->values[sendingSequence->length]=i;
			sendingSequence->length++;
		}

	//if(matrix->encoded)
	if(addRedundancy)
	{
		//If the matrix has been coded successfully, add redundancy indexes to the sequence.
		for (i= fec->K; i< fec->N; i++)
			{
				sendingSequence->values[sendingSequence->length]=i;
				sendingSequence->length++;
			}

		if (interleavingEnabled) //Change the order if the interleaver (internal to the matrix) is on.
			{
			sequenceShuffle(sendingSequence);
			}
	}

	debugPrint("T3: interleaving sequence reloaded");
}
void sequenceShuffle(SendingSequence *sendingSequence)
{
	int i;
	int randomIndex;
	int tmp;
	//With sequenceSize -1 all indexes are shuffled but the last
	//(it is used as notification of matrix end reception in eclsi).
	int shuffleSize = sendingSequence->length -1;

	for (i=0; i< shuffleSize ;i++)
	{
	randomIndex= rand() % shuffleSize;
	tmp=sendingSequence->values[i];
	sendingSequence->values[i]=sendingSequence->values[randomIndex];
	sendingSequence->values[randomIndex]=tmp;
	}
	debugPrint("T3: sequence shuffled");

/*
	debugPrint("Shuffled: %d");
	for (i=0;i< matrix->sequenceSize;i++)
	{
		debugPrint("%d",matrix->sequence[i]);
	}
*/
}
int  sequenceGetLength(SendingSequence *sendingSequence)
{
	return sendingSequence->length;
}
void sequenceFlush(SendingSequence *sendingSequence)
{
sendingSequence->length=0;
}
