/*
 eclsaMatrixPool.h

 Author: Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2018, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef _ECMATRIXPOOL_H_
#define _ECMATRIXPOOL_H_

#include "eclsaMatrix.h"
#include <stdbool.h>

void matrixPoolInit(const unsigned int standardPool, const unsigned int maxDynamicMatrix);
void matrixPoolDestroy();
EclsaMatrix *getMatrixToFill(const unsigned short matrixID, const unsigned short engineID, int N, int T);
EclsaMatrix *getMatrixToCode();
EclsaMatrix *getMatrixToSend();
bool	forcePreviousMatrixToDecode(EclsaMatrix *currentMatrix);
void flushMatrixFromPool(EclsaMatrix **matrix);


#endif /* CONTRIB_ECLSAV2_ION_OPEN_SOURCE_LTP_EC_ECLSA_ELEMENTS_MATRIX_ECLSAMATRIXPOOL_H_ */
