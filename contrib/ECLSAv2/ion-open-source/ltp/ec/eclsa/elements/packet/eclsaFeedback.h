/*
 eclsaFeedback.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef _ECLSAFEEDBACK_H_
#define _ECLSAFEEDBACK_H_

#include <stdbool.h>
#include "../matrix/eclsaMatrix.h"

/*Feedback */
typedef struct
{
	unsigned short 	matrixID;
	char			codecStatus;
	unsigned short 	totalSegments;
	unsigned short	receivedSegments;
} EclsaFeedback;

void createEclsaFeedback(EclsaFeedback *feedback,EclsaMatrix *matrix);
bool isFeedbackInvalid(EclsaFeedback *feedback,unsigned short minMid, unsigned short maxMid);


#endif
