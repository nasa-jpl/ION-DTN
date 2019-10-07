/*
 eclsaFecManager.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#ifndef _ECLSAFECARRAY_H_
#define _ECLSAFECARRAY_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>


#define FEC_MAX_K 16384
#define FEC_MAX_N 24576
#define FEC_MAX_RC (8.0/9.0)
#define FEC_MIN_RC (2.0/3.0)
#define CONTINUOUS_DEFAULT_MARGIN 10 // it must be > N1 (RFC 5170)
#define FEEDBACK_DEFAULT_MARGIN 1.15F //for eclso
#define FEC_CODE_MAX_COUNT 255

typedef struct
{
	unsigned int 	K;
	unsigned int 	N;
	unsigned int	T;
	void 			*codecVars; //variables to codec
	bool			continuous; //todo
} FecElement;


void 		fecManagerInit(bool adaptiveCodingEnabled, bool feedbackAdaptiveEnabled,bool continuousModeEnabled, unsigned int envK,unsigned int envN, unsigned int envT);
void		fecManagerDestroy();

/*Fec Array*/
void createNewFecArray(int length);
bool isFecElementAddable(int K,int N);
void addFecElementToArray(FecElement fec);

FecElement  *startMatrixFecElement(FecElement * encodingCode);
void flushMatrixFecElement(FecElement * encodingCode);

/* Adaptive coding */
FecElement 	*getBiggestFEC();
FecElement 	*getBestFEC(unsigned int infoSegmentAddedCount, float currentRate);
FecElement  *getFecElement(unsigned short K, unsigned short N, bool continuousModeRequested);

#ifdef __cplusplus
}
#endif

#endif
