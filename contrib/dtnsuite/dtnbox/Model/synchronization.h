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
 * synchronization.h
 */

#ifndef SYNCHRONIZATION_H_
#define SYNCHRONIZATION_H_

#include "folderToSync.h"

typedef enum SyncMode {

	PUSH = 1,	//both request and sync mode
	PULL = 2,	//both request and sync mode

	//INPUT and OUTPUT are needed to remember the unique dtnNode source (INPUT)
	PUSH_AND_PULL_IN = 4,
	PUSH_AND_PULL_OUT = 8
} SyncMode;

typedef enum SynchronizationState {
	SYNCHRONIZATION_PENDING,
	SYNCHRONIZATION_CONFIRMED,
} SynchronizationState;

//rappresenta una sincronizzazione verso un nodo
typedef struct synchronization{
	folderToSync folder;
	dtnNode node;

	SyncMode mode;
	SynchronizationState state;

	char pwdRead[SYNC_PASSWORD_SIZE_LIMIT];
	char pwdWrite[SYNC_PASSWORD_SIZE_LIMIT];
} synchronization;

#endif /* SYNCHRONIZATION_H_ */
