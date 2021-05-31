/*
 eclsaCodecMatrix.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef _ECLSADEC_H_
#define _ECLSADEC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
	uint8_t *symbolStatus;
	uint8_t **codewordBox;
	unsigned int rows;
	unsigned int cols;
} CodecMatrix;
typedef enum
{
	STATUS_CODEC_NOT_DECODED = 127,
	STATUS_CODEC_SUCCESS = 1,
	STATUS_CODEC_FAILED = 0
} UniversalCodecStatus; //todo lo stato del decodificatore può variare da -128 a 127 essendo un unsigned char

/*Codec Matrix*/
void 	codecMatrixInit(CodecMatrix **codecMatrix,int N,int T);
void 	codecMatrixDestroy(CodecMatrix *codecMatrix);
bool 	addSymbolToCodecMatrix(CodecMatrix *codecMatrix,int symbolID, char *buffer,int bufferLength,bool copyLenght);
char  	*getSymbolFromCodecMatrix(CodecMatrix *codecMatrix,unsigned int symbolID);
void 	flushCodecMatrix(CodecMatrix *codecMatrix);
bool 	isValidSymbol(CodecMatrix *codecMatrix, int symbolID);


#ifdef __cplusplus
}
#endif

#endif /* LTP_EC_ECLSA_ECLSADEC_H_ */
