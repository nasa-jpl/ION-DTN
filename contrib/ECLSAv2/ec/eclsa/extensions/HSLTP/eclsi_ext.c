/*
eclsi_ext.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)


Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file implements eclsi extension for HSLTP mode

 * */

#include <string.h>
#include <stdlib.h>
#include "../../adapters/codec/eclsaCodecAdapter.h"
#include "HSLTP.h"
//#include "../../eclsi.h"

void passEclsaMatrix_HSLTP_MODE(EclsaMatrix *matrix, EclsiEnvironment *eclsiEnv)
{
unsigned int i;
FecElement 	*code= 		  matrix->encodingCode;
char *segment;
uint16_t tmpSegLen;
int segmentLength;
char abstractCodecStatus;
bool isFirst = true;
abstractCodecStatus=convertToAbstractCodecStatus(matrix->codecStatus);

for(i=0;i < code->K;i++)
	{
	segment= getSymbolFromCodecMatrix(matrix->abstractCodecMatrix,i);
	memcpy(&tmpSegLen,segment, sizeof(uint16_t));
	//Only the matrix segments that have a length > 0 (no padding) and have been flagged as valid
	//by the decoder, must be sent to LTP (or to another upper protocol).

	if(tmpSegLen > 0 && isValidSymbol(matrix->abstractCodecMatrix,i) )
		{
			segmentLength=(int)tmpSegLen;
			sendSegmentToUpperProtocol_HSLTP_MODE((char *)(segment+ sizeof(uint16_t)),&segmentLength,abstractCodecStatus,isFirst);
			isFirst = false;
		}
	}
}
