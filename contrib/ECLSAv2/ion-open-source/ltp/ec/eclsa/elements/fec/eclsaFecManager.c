/*
 eclsaFecManager.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#include "eclsaFecManager.h"
#include "../sys/eclsaLogger.h"
#include "../sys/eclsaMemoryManager.h"
#include "../../adapters/codec/eclsaCodecAdapter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>


typedef struct
{
	FecElement 		*array;
	unsigned int 	length;
} FecArray;

static FecArray *fecArray=NULL;
static int maxSize=0;

static FecElement continuousFec;

static bool adaptiveCodingEnabled;
static bool	feedbackAdaptiveRcEnabled;
static bool continuousModeEnabled;
static unsigned int eK;
static unsigned int eN;

static void fecArraySort()
{
	//Sort the FEC array by K and after by N
	FecElement tmp;
	FecElement *fec_i, *fec_j;
	int i,j;
	for (i=0;i< fecArray->length -1 ; i++)
	  for(j=i+1; j<fecArray->length; j++)
		  {
		  fec_i=&(fecArray->array[i]);
		  fec_j=&(fecArray->array[j]);
		  //Order by K, if K equals order by N
		  if( fec_j->K < fec_i->K || (fec_j->K == fec_i->K && fec_j->N < fec_i->N  ) )
			  {
			  tmp=*fec_i;
			  *fec_i= *fec_j;
			  *fec_j=tmp;
			  }
		  }
}

void fecManagerInit(bool adaptiveCodingEn, bool feedbackAdaptiveRcEn,bool continuousModeEn, unsigned int envK,unsigned int envN, unsigned int envT)
{
	int i;
	FecElement *fecPtr;
	FecElement fec;

	if( envK > FEC_MAX_K )
	    {
		debugPrint("WARNING: K > FEC_MAX_K. FEC_MAX_K will be used instead of K");
		envK= FEC_MAX_K;
		}
	if( envN > FEC_MAX_N )
		{
		debugPrint("WARNING: N > FEC_MAX_N. FEC_MAX_N will be used instead of N");
		envN = FEC_MAX_N;
		}

	if( envN < envK )
		{
		debugPrint("ERROR: cannot start fecManager because N < K"); //todo
		exit(1);
		}
	if(fecArray != NULL)
		{
		debugPrint("Warning: FecArray already initialized");
		return;
		}
	if( continuousModeEn && !isContinuousModeAvailable())
		{
		debugPrint("Warning: continuousModeEnabled but codec does not support it. Disabling continuousMode...");
		continuousModeEn = false;
		}

	adaptiveCodingEnabled=adaptiveCodingEn;
	feedbackAdaptiveRcEnabled=feedbackAdaptiveRcEn;
	continuousModeEnabled=continuousModeEn;
	eK=envK;
	eN=envN;
	while ( (fecArray = (FecArray*) allocateElement(sizeof(FecArray))) == NULL ) ; //untill it will be allocated
	FECArrayLoad(envT);

	//todo
	if(continuousModeEnabled && fecArray->length == 0)
		{
		  createNewFecArray(1);
		  fec.K=envK;

		  if(feedbackAdaptiveRcEnabled)
		  	  {
			  fec.N=envK / FEC_MIN_RC;
		  	  }
		  else
		  	  {
			  fec.N=envN;
		  	  }

		  fec.T=envT;
		  fec.codecVars = NULL;
		  fec.continuous=true; // does not really matter here...
		  fecArray->array[fecArray->length] = fec;
		  fecArray->length ++;
		}

	if(fecArray->length <= 0)
		{
		debugPrint("ERROR: no FEC code loaded in fecArray");
		exit(1);
		}
	fecArraySort();
	for (i=0;i<fecArray->length;i++)
	    {
	    fecPtr= &(fecArray->array[i]);
	    debugPrint("FEC code: K=%5d N=%5d M=%5d \t", fecPtr->K , fecPtr->N, fecPtr->N - fecPtr->K );
	    }
}
void fecManagerDestroy()
{
	int i;
	if(fecArray == NULL)
		{
		debugPrint("Warning: FecArray not initialized or already destroyed");
		return;
		}
	for (i=0;i<fecArray->length;i++)
		{
			destroyCodecVars(&fecArray->array[i]);
		}

	deallocateVector(&(fecArray->array));
	deallocateElement(&(fecArray));
	fecArray=NULL;
}

void createNewFecArray(int length)
{
	 if( length <= 0)
		  {
		  debugPrint("ERROR: cannot create fecArray with length <= 0. Quitting..");
		  exit(1);
		  }
	 if( length > FEC_CODE_MAX_COUNT)
		  {
		  debugPrint("ERROR: Too much FEC codes, cannot create fecArray");
		  exit(1);
		  }
	if(fecArray->array != NULL)
		  {
		  debugPrint("WARNING: destroying old fecArray->array...");
		  deallocateVector(&(fecArray->array));
		  }

	debugPrint("create array with size %d",length);
	while ( (fecArray->array = allocateVector(sizeof(FecElement), length)) == NULL ) ; // until it will be allocated
	fecArray->length=0;
	maxSize=length;
}
bool isFecElementAddable(int K,int N)
{
 return ( (fecArray->length <= FEC_CODE_MAX_COUNT) &&  ( K<N )&& (
		( adaptiveCodingEnabled && K<=eK ) ||
		(!adaptiveCodingEnabled && !continuousModeEnabled && K==eK && (N==eN || feedbackAdaptiveRcEnabled))));
}
void addFecElementToArray(FecElement fec)
{
	if( !isFecElementAddable(fec.K,fec.N) )
		{
		debugPrint("WARNING: fecElement K=%d N=%d cannot be added to fecArray...",fec.K, fec.N);
		return;
		}
	if( fec.continuous )
		{
		debugPrint("WARNING: cannot add continuous FecElement to fecArray... changing FecElement to non-continuous");
		fec.continuous=false;
		}
	fecArray->array[fecArray->length] = fec;
	fecArray->length ++;
}

FecElement *startMatrixFecElement(FecElement * encodingCode)
{
	FecElement *fec;
	if(encodingCode == &continuousFec)
	{
		while ( (fec = allocateElement(sizeof(FecElement))) == NULL) ; // until it will be allocated
		memcpy(fec,encodingCode,sizeof(FecElement));
	}
	else
	{
		fec = encodingCode;
	}
	return fec;
}
void flushMatrixFecElement(FecElement * encodingCode)
{
	int i;
	for(i=0;i<fecArray->length;i++)
	{
		if(&(fecArray->array[i]) == encodingCode)
			return;
	}
	deallocateElement(&(encodingCode));
}

/*Adaptive FEC functions */
FecElement  *getBiggestFEC()
	{
	return &(fecArray->array[fecArray->length-1]);
	}
FecElement 	*getBestFEC(unsigned int infoSegmentAddedCount, float currentRate) //used only by eclso
{
	FecElement *fec;
	float 		actualCodeRate; // Defined as I/(I+M)
	unsigned int i,N;
	debugPrint("I=%d estimatedSuccesRate=%f, estimatedPER=%f",infoSegmentAddedCount,currentRate, 1-currentRate);

	if( feedbackAdaptiveRcEnabled )
	{
		currentRate= 1 - ( (1-currentRate)* FEEDBACK_DEFAULT_MARGIN);
	}

	if(continuousModeEnabled)
	{
		memset(&continuousFec,0,sizeof(FecElement));
		continuousFec.continuous=true;
		continuousFec.K=infoSegmentAddedCount;

		if( currentRate > FEC_MAX_RC )
			{
			currentRate = FEC_MAX_RC;
			}

		continuousFec.N=ceil(continuousFec.K/currentRate) + CONTINUOUS_DEFAULT_MARGIN;

		if(continuousFec.N > getBiggestFEC()->N)
		{
			debugPrint("Warning: requested N=%d too big, using N=%d",continuousFec.N,getBiggestFEC()->N);
			continuousFec.N = getBiggestFEC()->N;
		}
		continuousFec.T=getBiggestFEC()->T;
		continuousFec.codecVars = NULL;

		while ( (fec = allocateElement(sizeof(FecElement))) == NULL ) ; // until it will be allocated
		memcpy(fec,&continuousFec,sizeof(FecElement));

		return fec;
	}
	else
	{
		for(i=0;i< fecArray->length;i++)
				{
				fec=&(fecArray->array[i]);
				actualCodeRate=(float)infoSegmentAddedCount/(infoSegmentAddedCount + fec->N -fec->K);
				if(actualCodeRate<=currentRate && infoSegmentAddedCount <= fec->K )
					debugPrint("N=%5d K=%5d -> I=%d R=%4d Real rate: %f , works good with PER<=%f",
							fec->N,fec->K,infoSegmentAddedCount,fec->N-fec->K,actualCodeRate, 1-actualCodeRate);
				}

		//The code array is always sorted first by K and then by N, so that we can stop at first occurrence.
		for(i=0;i< fecArray->length;i++)
			{
			fec=&(fecArray->array[i]);
			actualCodeRate=(float)infoSegmentAddedCount/(infoSegmentAddedCount + fec->N -fec->K);
			if(actualCodeRate<=currentRate && infoSegmentAddedCount <= fec->K )
				return fec;
			}
		debugPrint("T2:wanted FEC not found, FEC with largest N selected");
		return fec;
	}


}
FecElement  *getFecElement(unsigned short K, unsigned short N, bool continuousModeRequested) //used only by eclsi
{
	FecElement *outEncodingCode=NULL;
	int i;

	if(continuousModeRequested)
	{
		if(continuousModeEnabled && N > K && N<= getBiggestFEC()->N )
		{
		memset(&continuousFec,0,sizeof(FecElement));
		continuousFec.continuous=true;
		continuousFec.K=K;
		continuousFec.N=N;
		continuousFec.T=getBiggestFEC()->T;
		continuousFec.codecVars = NULL;
		outEncodingCode = &continuousFec;
		}
	}
	else
	{
		for(i=0;i<fecArray->length;i++)
			{
			if(fecArray->array[i].K == K && fecArray->array[i].N == N)
				{
				outEncodingCode=&(fecArray->array[i]);
				break;
				}
			}
	 }

	return outEncodingCode;

}
