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
 * monitorAndSendThread.h
 */

#ifndef FSSCANTHREAD_H_
#define FSSCANTHREAD_H_

#include "../Model/cmdList.h"
#include "al_bp_wrapper.h"
#include "../Model/folderToSync.h"
#include "al_bp_extB.h"
#include "pthread.h"

//thread di scansione del file system. Si occupa anche dell'invio dei comandi in coda
typedef struct {
	sqlite3* dbConn;
	dtnNode myNode;
	pthread_mutex_t* quit;
	al_bp_extB_registration_descriptor register_sender;
	pthread_mutex_t *generalMutex;
} monitorAndSendArgs;

void* monitorAndSendDaemon(void* args);

#endif /* FSSCANTHREAD_H_ */
