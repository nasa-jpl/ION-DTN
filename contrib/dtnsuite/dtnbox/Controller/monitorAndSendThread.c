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
 * monitorAndSendThread.c
 */

#include "monitorAndSendThread.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "debugger.h"

#include "../Model/definitions.h"
#include "../DBInterface/DBInterface.h"
#include "monitorAndSendThreadOperations.h"

void* monitorAndSendDaemon(void* args) {

	//argument of this thread
	monitorAndSendArgs* argomenti = (monitorAndSendArgs*) args;

	//error check
	int error;

	//connection to BP
	al_bp_extB_registration_descriptor register_descriptor;

	//connection to DB
	sqlite3* dbConn;

	char tempCopyFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// "~/DTNbox/.tempDir/out/tempCopy"
	char cleanCommand[SYSTEM_COMMAND_LENGTH + DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];		//"rm -rf ~/DTNbox/.tempDir/out/tempCopy/*"


	//apro connessione al db
	dbConn = argomenti->dbConn;

	register_descriptor = argomenti->register_sender;

	getHomeDir(tempCopyFolder);
	strcat(tempCopyFolder, DTNBOXFOLDER);
	strcat(tempCopyFolder, TEMPDIR);
	strcat(tempCopyFolder, OUTFOLDER);
	strcat(tempCopyFolder, TEMPCOPY);

	sprintf(cleanCommand, "rm -rf %s*", tempCopyFolder);


	error = disableSigint();
	if(error){
		error_print("monitorAndSendThread: error in disableSigint()\n");
		pthread_exit(NULL);
	}


	//ciclo infinito
	while (!terminate(argomenti->quit)) {

		//sleep
		sleep(SLEEP_TIME);

		pthread_mutex_lock(argomenti->generalMutex);

		//rilevazione modifiche sul file system
		error = processInotifyEvents(dbConn);
		if (error) {
			error_print(
					"monitorAndSendThread: error in processInotifyEvent()\n");
			system(cleanCommand);
			error_print("monitorAndSendThread: error, unlock the mutex\n");
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}

		//creazione dei comandi di update e copia dei file di aggiornamento nella cartella temporanea
		error = buildUpdateCommandsAndCopyTempFiles(dbConn, argomenti->myNode);
		if(error){
			error_print("monitorAndSendThread: error in buildUpdateCommands()\n");
			system(cleanCommand);
			error_print( "monitorAndSendThread: error, unlock the mutex\n");
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}

		//creazione dei tar e invio dei bundle
		error = sendCommandsToDestNodes(dbConn, argomenti->myNode, register_descriptor);
		if(error){
			error_print("monitorAndSendThread: error in sendCommandsToDestNodes()\n");
			system(cleanCommand);
			error_print("monitorAndSendThread: error, unlock the mutex\n");
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}

		//eliminazione dal DB dei comandi conclusi (inviati e ricevuti)
		//TODO definire politica migliore...
		error = clearOldCommandsFromDB(dbConn);
		if(error){
			error_print("monitorAndSendThread: error in clearOldCommandsFromDB()\n");
			system(cleanCommand);
			error_print("monitorAndSendThread: error, unlock the mutex\n");
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}

		//creazione delle cartelle di sincronizzazione mancanti
		//TODO migliorabile con inotify
		error = createMissingFolders(dbConn, argomenti->myNode);
		if(error){
			error_print("monitorAndSendThread: error in createMissingFolders()\n");
			system(cleanCommand);
			error_print("monitorAndSendThread: error, unlock the mutex\n");
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}

		pthread_mutex_unlock(argomenti->generalMutex);
	}//while(!terminate)

	//prepare and send "Freeze" commands to inform everyone that I go to sleep...
	pthread_mutex_lock(argomenti->generalMutex);
	error = prepareFreezeCommands(dbConn, argomenti->myNode);
	if(error){
		error_print("monitorAndSendThread: error in prepareFreezeCommands()\n");
		system(cleanCommand);
	}
	else{
		error = sendCommandsToDestNodes(dbConn, argomenti->myNode, register_descriptor);
		if(error){
			error_print("monitorAndSendThread: error in sendCommandsToDestNodes()\n");
			system(cleanCommand);
		}
	}
	pthread_mutex_unlock(argomenti->generalMutex);


	debug_print(DEBUG_L1, "monitorAndSendThread: correctly terminated\n");
	pthread_exit(NULL);
}
