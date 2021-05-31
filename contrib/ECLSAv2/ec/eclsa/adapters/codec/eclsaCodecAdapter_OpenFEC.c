/*
 eclsaCodecAdapter_OpenFEC.c

 Author: Marco Raminella (marco.raminella@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisors: Carlo Caini (carlo.caini@unibo.it)
 	 	 	 	 	  Nicola Alessi (nicola.alessi@studio.unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo: fare initSession e destroySession

 * */
#include "eclsaCodecAdapter.h"
#include "../../elements/sys/eclsaLogger.h"
#include "../../elements/sys/eclsaMemoryManager.h"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <of_openfec_api.h>

#define VERBOSITY false
#define MAXK 16384
#define MAXN 24576
static const int N1 = 7;
static const int prng_seed = 1234;

typedef struct
{
	of_status_t of_status[2];
	of_session_t * of_session[2];
	of_ldpc_parameters_t * of_parameters;
} OpenFECVars;
// this is an internal type used to define two distinct codecs inside OpenFECVars
enum CODEC { ENCODER, DECODER };
// extra states required to know how the decoder succeeded
enum SUCCESS_STATUS { OF_STATUS_SUCCESS_IT = 4, OF_STATUS_SUCCESS_ML = 5 };

static void initSession(FecElement * fecElement, enum CODEC type)
{
	OpenFECVars *openFECVars = fecElement->codecVars;
		UINT8 codec_type;
		switch(type){
		case ENCODER: codec_type = OF_ENCODER; break;
		case DECODER: codec_type = OF_DECODER; break;
		}
		if(fecElement->codecVars == NULL){
			openFECVars = allocateElement(sizeof(OpenFECVars));
			openFECVars->of_parameters = allocateElement(sizeof(of_ldpc_parameters_t));
			openFECVars->of_parameters->encoding_symbol_length = fecElement->T * sizeof(uint8_t) ; // The number of bytes per symbol
			openFECVars->of_parameters->nb_source_symbols = fecElement->K;
			openFECVars->of_parameters->nb_repair_symbols = fecElement->N - fecElement->K;
			openFECVars->of_parameters->N1 = N1;
			openFECVars->of_parameters->prng_seed = prng_seed;
			openFECVars->of_session[ENCODER] = NULL;
			openFECVars->of_session[DECODER] = NULL;
			fecElement->codecVars = (void *) openFECVars;
		}
		openFECVars->of_status[type] = of_create_codec_instance(&openFECVars->of_session[type], OF_CODEC_LDPC_STAIRCASE_STABLE,codec_type,VERBOSITY);
		openFECVars->of_status[type] = of_set_fec_parameters(openFECVars->of_session[type],(of_parameters_t *)openFECVars->of_parameters);
}
static void destroySession(FecElement * fecElement, enum CODEC type)
{
	((OpenFECVars *) fecElement->codecVars)->of_status[type] = of_release_codec_instance(((OpenFECVars *) fecElement->codecVars)->of_session[type]);
	((OpenFECVars *) fecElement->codecVars)->of_session[type] = NULL;
}


void FECArrayLoad(int envT)
{
	int arrK[9]={512,512,512,2048,2048,2048,16384,16384,16384};
	int arrN[9]={576,640,768,2304,2560,3072,18432,20480,24576};
	int arrLen=9;
	FecElement 	fec;
	FecElement tmp;
	OpenFECVars *temp;
	int i,j;
	int counter=0;

	for(i=0;i<arrLen;i++)
		{
		if( isFecElementAddable(arrK[i],arrN[i]) )
				{
				counter++;
				}
		}

	debugPrint("found %d files in the code folder",counter);

	if(counter > 0)
	{
	createNewFecArray(counter);

	for(i=0;i<arrLen;i++)
			{
		  if( isFecElementAddable(arrK[i],arrN[i]) )
			 	  {
				  fec.K=arrK[i];
				  fec.N=arrN[i];
				  fec.T=envT;
				  fec.codecVars = NULL;
				  fec.continuous=false;
				  addFecElementToArray(fec);
				  }
			}
	}
}
void destroyCodecVars(FecElement *fec)
{
	of_status_t of_status;
	OpenFECVars *openFECVars = (OpenFECVars *) fec->codecVars;
	if (openFECVars == NULL)
		return;
	/* if(openFECVars->of_session[ENCODER] != NULL)
		of_status = of_release_codec_instance(openFECVars->of_session[ENCODER]);
	if(openFECVars->of_session[DECODER] != NULL)
		of_status = of_release_codec_instance(openFECVars->of_session[DECODER]); */
	deallocateElement(&(openFECVars->of_parameters));
	deallocateElement(&(openFECVars));
	openFECVars=NULL;
}
/*codec Matrix
 *
 * openFECMatrixInit allocates the buffer needed for encoding and decoding data.
 *
 * 	- 	ADT is a parity check vector of symbols of size T, composed by total N elements
 * 		which K of them are of data and M are of redundancy, the redundancy symbols are
 * 		generated from the parity check equations. However this vector is abstracted as
 * 		a matrix, in order to see each symbol as a column.
 *
 * 	-	columnStatus says if a certain symbol is valid (1) or not (0).
 *
 * openFECMatrixDestroy frees up the resources allocated before.
 *
 */
int  encodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode)
{
	OpenFECVars *openFECVars;
	of_status_t of_status;
	int i, status;

	initSession(encodingCode,ENCODER);
	openFECVars = (OpenFECVars *) encodingCode->codecVars;
	for(i=encodingCode->K;i<encodingCode->N;i++){
		if(!isValidSymbol(codecMatrix,i))
			openFECVars->of_status[ENCODER] = of_build_repair_symbol(openFECVars->of_session[ENCODER],(void **) codecMatrix->codewordBox,i);
		codecMatrix->symbolStatus[i] = 1;
	}

	if(openFECVars->of_status[ENCODER] == OF_STATUS_OK){
				status = OF_STATUS_SUCCESS_IT;
			} else status = openFECVars->of_status[ENCODER];

	destroySession(encodingCode,ENCODER);
	return status;

}
int  decodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode,int paddingFrom,int paddingTo)
{
	OpenFECVars * openFECVars;
	int i, status = 0;
	uint8_t ** in_data_vector;
	in_data_vector = allocateVector(sizeof(uint8_t*), encodingCode->N);

	initSession(encodingCode,DECODER);
	openFECVars = (OpenFECVars *) encodingCode->codecVars;
	if(openFECVars->of_status[DECODER] != OF_STATUS_OK)
		return openFECVars->of_status[DECODER];

	for(i=paddingFrom ;i<  paddingTo ; i++)
		codecMatrix->symbolStatus[i]=1;

	for(i=0; i < encodingCode-> N; i++){
		if(isValidSymbol(codecMatrix,i)){
			in_data_vector[i] = allocateVector(sizeof(uint8_t), encodingCode->T);
			memcpy(in_data_vector[i],codecMatrix->codewordBox[i],encodingCode->T);
		} else {
			in_data_vector[i] = NULL;
		}
	}
	openFECVars->of_status[DECODER] = of_set_available_symbols(openFECVars->of_session[DECODER],(void **) in_data_vector);

	/*
	 * Decoding with this primitive causes a lot of memory leaking
	 *
	for(i=0;i<encodingCode-> N;i++){
			 if(isValidSegment(openFECMat,i))
				openFECVars->of_status[DECODER] = of_decode_with_new_symbol(openFECVars->of_session[DECODER],openFEC->ADT[i],i);

	}
	*/

	if(of_is_decoding_complete(openFECVars->of_session[DECODER]))
	{
		status = OF_STATUS_SUCCESS_IT;
	}
	else
	{
		// if the iterative decoding has failed then we need to do the ML decoding
		openFECVars->of_status[DECODER] = of_finish_decoding(openFECVars->of_session[DECODER]);
		if(openFECVars->of_status[DECODER] == OF_STATUS_OK){
			status = OF_STATUS_SUCCESS_ML;
		} else status = openFECVars->of_status[DECODER];
	}

	openFECVars->of_status[DECODER] = of_get_source_symbols_tab(openFECVars->of_session[DECODER],(void **) in_data_vector);
	if(openFECVars->of_status[DECODER] != OF_STATUS_OK){
			status = openFECVars->of_status[DECODER];
			}

	for(i=0; i < encodingCode-> N; i++){
		if(in_data_vector[i] != NULL){
			memcpy(codecMatrix->codewordBox[i],in_data_vector[i],sizeof(uint8_t)*encodingCode->T);
			codecMatrix->symbolStatus[i] = 1;
			deallocateVector(&(in_data_vector[i]));

		} else {
			memset(codecMatrix->codewordBox[i],0,sizeof(uint8_t)*encodingCode->T);
			codecMatrix->symbolStatus[i] = 0;
		}
	}

	deallocateVector(&(in_data_vector));
	destroySession(encodingCode,DECODER);
	return status;
}

/**/
char convertToAbstractCodecStatus(char codecStatus)
{
	if(codecStatus == STATUS_CODEC_NOT_DECODED)
			return STATUS_CODEC_NOT_DECODED;

	if(codecStatus == OF_STATUS_SUCCESS_IT || codecStatus == OF_STATUS_SUCCESS_ML)
		return STATUS_CODEC_SUCCESS;
	else
		return STATUS_CODEC_FAILED;
}
char *getCodecStatusString(int codecStatus)
	{
	static char *not_decoded =	"OpenFEC:\"Not decoded\"";
	static char *failure=		"OpenFEC:\"Failed decoding\"";
	static char *error=			"OpenFEC:\"Codec error\"";
	static char *fatal=			"OpenFEC:\"Critical error\"";
	static char *success_it =   "OpenFEC:\"IT okay\"";
	static char *success_ml =   "OpenFEC:\"ML okay\"";
	static char *unknown=		"OpenFEC:\"Unknown Decode Status\"";

	switch (codecStatus)
		{
			case STATUS_CODEC_NOT_DECODED:
				return not_decoded;
			case OF_STATUS_FAILURE:
					return failure;
			case OF_STATUS_ERROR:
					return error;
			case OF_STATUS_FATAL_ERROR:
					return fatal;
			case OF_STATUS_SUCCESS_IT:
					return success_it;
			case OF_STATUS_SUCCESS_ML:
					return success_ml;
			default:
					return unknown;
		}
	}
bool 	isContinuousModeAvailable()
{
return true;
}

