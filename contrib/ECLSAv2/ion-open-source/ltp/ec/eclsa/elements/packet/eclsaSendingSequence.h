/*
 eclsaSendingSequence.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef _ECLSASEQUENCE_H_
#define _ECLSASEQUENCE_H_

#include <stdbool.h>
#include "../fec/eclsaFecManager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	int *values;
	int length;
} SendingSequence;

/*Eclsa matrix segment index sequence functions */
void sequenceInit(SendingSequence *sequence,int maxSize);
void sequenceDestroy(SendingSequence *sequence);
void sequenceReload(SendingSequence *sequence,FecElement *fec,bool addRedundancy,bool interleavingEnabled, bool puncturingEnabled, unsigned int infoSegmentAddedCount);
void sequenceShuffle(SendingSequence *sequence);
int  sequenceGetLength(SendingSequence *seq);
void sequenceFlush(SendingSequence *seq);

#ifdef __cplusplus
}
#endif

#endif
