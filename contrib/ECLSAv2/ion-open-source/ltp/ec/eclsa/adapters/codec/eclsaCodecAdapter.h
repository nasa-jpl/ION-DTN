/*
eclsaCodecAdapter.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.



 * */

#ifndef _ECLSA_CODEC_ADAPTER_H_

#define _ECLSA_CODEC_ADAPTER_H_

#include "../../elements/fec/eclsaFecManager.h"
#include "../../elements/matrix/eclsaCodecMatrix.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/*Specifiche per il decoder*/
void 	FECArrayLoad(int envT);
void	destroyCodecVars(FecElement *fec);
int		encodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode);
int 	decodeCodecMatrix(CodecMatrix *codecMatrix,FecElement *encodingCode,int paddingFrom,int paddingTo);
char	convertToAbstractCodecStatus(char codecStatus);
char	*getCodecStatusString(int codecStatus);
bool 	isContinuousModeAvailable();


#ifdef __cplusplus
}
#endif

#endif
