/*
eclsi.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions and the structures of the eclsi daemon

 * */


#ifndef _ECLSI_H_

#define _ECLSI_H_


#include <semaphore.h>
#include "eclsi_def.h"
#include "elements/matrix/eclsaMatrix.h"



#ifdef __cplusplus
extern "C" {
#endif


/*Thread functions*/
static void *fill_matrix_thread(void *parm);
static void	*decode_matrix_thread(void *parm);
static void	*pass_matrix_thread(void *parm);

/*Common functions*/
static void parseCommandLineArgument(int argc, char *argv[], EclsiEnvironment *eclsiEnv);

/*Single eclsa matrix function*/
void decodeEclsaMatrix(EclsaMatrix *matrix);
void passEclsaMatrix(EclsaMatrix*matrix , EclsiEnvironment *eclsiEnv);
/*Feedback */
void sendFeedback(EclsaMatrix *matrix,EclsiEnvironment *eclsiEnv);

/* Synchronization */
void sem_notify(sem_t *threadSem);


#ifdef __cplusplus
}
#endif

#endif /* LTP_EC_ECLSA_ECLSI_H_ */
