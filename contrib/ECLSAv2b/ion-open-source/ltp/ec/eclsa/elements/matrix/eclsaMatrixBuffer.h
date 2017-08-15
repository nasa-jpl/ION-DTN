/*
 eclsaMatrixBuffer.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef _ECMATRIXBUFFER_H_
#define _ECMATRIXBUFFER_H_

#include <semaphore.h>
#include "eclsaMatrix.h"

/*Eclsa matrix buffer functions*/
void matrixBufferInit(unsigned int length);
void matrixBufferDestroy();
EclsaMatrix *getMatrixToFill(unsigned short matrixID, unsigned short engineID);
EclsaMatrix *getMatrixToCode();
EclsaMatrix *getMatrixToSend();

bool		 forcePreviousMatrixToDecode(EclsaMatrix *currentMatrix);

#endif
