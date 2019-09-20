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
 * parseTarThread.h
 */

#ifndef EXECUTETHREAD_H_
#define EXECUTETHREAD_H_

#include <semaphore.h>
#include "../Model/definitions.h"
#include <sqlite3.h>


//argomenti in ingresso
typedef struct parseTarArgs{

	sqlite3* dbConn;
	dtnNode myNode;

	pthread_mutex_t* quit;
	pthread_mutex_t *generalMutex;

	pthread_mutex_t *receiveMutex;

	sem_t *receivable;
	sem_t *processable;
	pendingReceivedBundleInfo *pendingReceivedBundles;

}parseTarArgs;

void* parseTarDaemon(void* args);


//argomenti in uscita

#endif /* EXECUTETHREAD_H_ */
