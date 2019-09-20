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
 * monitorAndSendThreadOperations.h
 */

#ifndef DTNBOX11_CONTROLLER_MONITORANDSENDTHREADOPERATIONS_H_
#define DTNBOX11_CONTROLLER_MONITORANDSENDTHREADOPERATIONS_H_

#include <sqlite3.h>
#include "../../al_bp/src/al_bp_extB.h"
#include "../Model/definitions.h"



int processInotifyEvents(sqlite3 *dbConn);

int buildUpdateCommandsAndCopyTempFiles(sqlite3 *dbConn, dtnNode myNode);

int sendCommandsToDestNodes(sqlite3 *dbConn, dtnNode myNode, al_bp_extB_registration_descriptor register_descriptor);

int clearOldCommandsFromDB(sqlite3 *dbConn);

int createMissingFolders(sqlite3 *dbConn, dtnNode myNode);


int prepareFreezeCommands(sqlite3 *dbConn, dtnNode myNode);

int prepareUnfreezeCommands(sqlite3 *dbConn, dtnNode myNode, int keepRemoteNodeFrozen);


#endif /* DTNBOX11_CONTROLLER_MONITORANDSENDTHREADOPERATIONS_H_ */
