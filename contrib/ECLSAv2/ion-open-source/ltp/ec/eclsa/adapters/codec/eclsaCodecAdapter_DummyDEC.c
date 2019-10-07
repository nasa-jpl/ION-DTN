/*
 eclsaCodecAdapter_DummyDEC.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */
#include "eclsaCodecAdapter.h"
#include <unistd.h>
#include <stdlib.h> //exit
#include <stdint.h>

//change this for debug testing...
#define CONTINUOUS_MODE_SUPPORT true

/*Fec Array*/
void FECArrayLoad(int envT)
{
	int arrK[9]={512,512,512,2048,2048,2048,16384,16384,16384};
	int arrN[9]={576,640,768,2304,2560,3072,18432,20480,24576};
	int arrLen=9;
	FecElement fec;
	int i,j;
	int counter=0;

	//count addable fec elements
	for(i=0;i<arrLen;i++)
		{
		if( isFecElementAddable(arrK[i] , arrN[i] ) )
				{
				counter++;
				}
		}

	debugPrint("found %d files in the code folder",counter);

	if(counter > 0)
	{
    createNewFecArray(counter);
    //add fec elements to fec array inside fecManager
    for(i=0;i<arrLen;i++)
		{
			if( isFecElementAddable(arrK[i] , arrN[i] ) )
			 {
				  fec.K=arrK[i];
				  fec.N=arrN[i];
				  fec.T=envT;
				  fec.codecVars=NULL;
				  fec.continuous=false;
				  addFecElementToArray(fec);
			 }
		}
	}
}
void	destroyCodecVars(FecElement *fec)
{
	//nothing to do because codec vars is not allocated by dummyDEC
	return;
}

/*Codec Matrix*/
int  encodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode)
{
	int i;
	for (i=0;i<encodingCode->N;i++)
		{
		codecMatrix->symbolStatus[i]=1;
		}
	return STATUS_CODEC_SUCCESS;
}
int  decodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode,int paddingFrom,int paddingTo)
{
	int i;

	for(i=paddingFrom ;i<  paddingTo ; i++)
		codecMatrix->symbolStatus[i]=1;

	for (i=0;i<encodingCode->K;i++)
			{
				if(codecMatrix->symbolStatus[i]==0)
					{
					return STATUS_CODEC_FAILED;
					}
			}
	return STATUS_CODEC_SUCCESS;
	}

/**/
char convertToAbstractCodecStatus(char codecStatus)
{
return  codecStatus;
}
char *getCodecStatusString(int codecStatus)
	{
	static char *not_decoded=	"DummyDEC:\"Not decoded\"";
	static char *success_it=	"DummyDEC:\"Success\"";
	static char *failed_it=		"DummyDEC:\"Failed\"";
	static char *unknown=		"DummyDEC:\"Unknown Decode Status\"";

	switch (codecStatus)
		{
			case STATUS_CODEC_NOT_DECODED:
					return not_decoded;
			case STATUS_CODEC_SUCCESS:
					return success_it;
			case STATUS_CODEC_FAILED:
					return failed_it;
			default:
					return unknown;
		}
	}
bool 	isContinuousModeAvailable()
{
return CONTINUOUS_MODE_SUPPORT;
}
