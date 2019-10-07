/********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/


/*
 * receiveThread.h
 */

#ifndef RECEIVEDAEMONTHREAD_H_
#define RECEIVEDAEMONTHREAD_H_

#include "al_bp_wrapper.h"
#include <semaphore.h>
#include "../Model/definitions.h"
//thread che riceve i bundle in ingresso e li deposita in una cartella temporanea

//struttura argomenti in ingresso
typedef struct{
	al_bp_extB_registration_descriptor register_receiver;
	pthread_mutex_t* quit;
	pthread_mutex_t *receiveMutex;
	sem_t *receivable;
	sem_t *processable;
	pendingReceivedBundleInfo *pendingReceivedBundles;
} receiveArgs;

//loop di ricezione
void* receiveDaemon(void* args);
#endif /* RECEIVEDAEMONTHREAD_H_ */
