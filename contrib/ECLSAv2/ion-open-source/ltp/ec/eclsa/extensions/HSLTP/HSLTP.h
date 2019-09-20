/*
HSLTP.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Co-author of HSLTP extensions: Azzurra Ciliberti (azzurra.ciliberti@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclso daemon

 * */
#ifndef _HSLTP_H_

#define _HSLTP_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include "HSLTP_def.h"
#include "../../elements/matrix/eclsaMatrix.h"
#include "../../eclso_def.h"
#include "../../eclsi_def.h"


void *fill_matrix_thread_HSLTP_MODE(void *parm);
void passEclsaMatrix_HSLTP_MODE(EclsaMatrix *matrix , EclsiEnvironment * eclsiEnv);
void initEclsoUpperLevel_HSLTP_MODE(int argc, char *argv[], EclsoEnvironment *env);

/* protocol adapter */
void sendSegmentToUpperProtocol_HSLTP_MODE		(char *buffer,int *bufferLength,int abstractCodecStatus,bool isFirst);
int receiveSegmentFromUpperProtocol_HSLTP_MODE	(char *buffer,int *bufferLength);


#ifdef __cplusplus
}
#endif

#endif
