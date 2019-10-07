/*
eclso.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclso daemon

 * */
#ifndef _ECLSO_H_

#define _ECLSO_H_


#include "elements/matrix/eclsaMatrix.h"
#include "eclso_def.h"
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif



/* FUNCTIONS */

/*Thread functions*/
static void *fill_matrix_thread(void *parm);
static void	*encode_matrix_thread(void *parm);
static void	*pass_matrix_thread(void *parm);
static void *feedback_handler_thread(void *parm);

/*Common functions*/
static void parseCommandLineArgument(int argc, char *argv[], EclsoEnvironment *eclsoEnv);

/*Single eclsa matrix functions*/
void encodeEclsaMatrix(EclsoEnvironment *eclsoEnv, EclsaMatrix *matrix);
void passEclsaMatrix(EclsoEnvironment *eclsoEnv, EclsaMatrix *matrix);

/* Synchronization */
void sem_notify(sem_t *threadSem);


#ifdef __cplusplus
}
#endif

#endif /* LTP_EC_ECLSA_ECLSO_H_ */
