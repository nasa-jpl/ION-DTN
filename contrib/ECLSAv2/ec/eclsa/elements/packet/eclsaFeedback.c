/*
 eclsaFeedback.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#include "eclsaFeedback.h"

#include <string.h>

bool isFeedbackInvalid(EclsaFeedback *feedback,unsigned short minMid,unsigned short maxMid)
{
	return 	  (feedback->totalSegments == 0 ||
			   feedback->totalSegments < feedback->receivedSegments ||
			   (maxMid>=minMid && (feedback->matrixID<minMid || feedback->matrixID >= maxMid)) ||
			   (maxMid< minMid && (feedback->matrixID<minMid && feedback->matrixID >= maxMid))    //to prevent overflow issues
			   ) ;
}
void createEclsaFeedback(EclsaFeedback *feedback,EclsaMatrix *matrix)
{
	memset(feedback,0,sizeof(EclsaFeedback));
		//feedback.codeID = matrix->encodingCode->ID;
		feedback->codecStatus= matrix->codecStatus;
		feedback->matrixID = matrix->ID;

		//todo
		//If the decoding was skipped because all K info segments arrived,
		//redundancy segments must be neglected.
		if (!isMatrixInfoPartFull(matrix))
			{
			feedback->receivedSegments= matrix->infoSegmentAddedCount + matrix->redundancySegmentAddedCount;
			feedback->totalSegments=matrix->maxInfoSize + matrix->encodingCode->N - matrix->encodingCode->K;
			}
		else
			{
			feedback->receivedSegments= matrix->infoSegmentAddedCount;
			feedback->totalSegments=matrix->maxInfoSize;
			}

}
