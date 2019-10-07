  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * userInterfaceThread.h
 */

#ifndef USERTHREAD_H_
#define USERTHREAD_H_

#include <sqlite3.h>
#include <pthread.h>
#include "../Model/definitions.h"

typedef struct{
	dtnNode myNode;
	sqlite3* dbConn;
	pthread_mutex_t *generalMutex;
} userArgs;

void* userInterfaceDaemon(void* args);

#endif /* USERTHREAD_H_ */
