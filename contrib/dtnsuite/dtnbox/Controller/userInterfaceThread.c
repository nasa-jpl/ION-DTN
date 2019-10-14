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
 * userInterfaceThread.c
 */
#include "userInterfaceThread.h"

#include "debugger.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "../Model/command.h"
#include "utils.h"
#include "../Model/synchronization.h"
#include "../DBInterface/DBInterface.h"
#include "../Model/syncCommand.h"
#include "../Model/watchList.h"

#include "../Model/checkUpdateCommand.h"
#include "../Model/definitions.h"

#include "../Controller/utils.h"

static int addNodeUserOperation(sqlite3 *dbConn, pthread_mutex_t *generalMutex);
static int addSyncUserOperation(sqlite3* dbConn, pthread_mutex_t *generalMutex,
		dtnNode myNode);
static int addFolderUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode);
static int deleteSyncUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode);
static int deleteFolderUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode);
static int checkUpdateUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode);
static int forceUpdateUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex);

static int resetDatabaseUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode);
static int dumpDatabaseUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex);
static int printWatchListUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex);

static int deleteSynchronizationsOnFolderUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode);

void* userInterfaceDaemon(void* args) {

	userArgs* argomenti = (userArgs*) args;
	sqlite3* dbConn = argomenti->dbConn;

	int error;
	int exitFlag;

	char userInput[USER_INPUT_LENGTH];

	error = disableSigint();
	if (error) {
		error_print("userInterfaceDaemon: error in disableSigint()\n");
		pthread_exit(NULL);
	}

	exitFlag = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &error);
	if (exitFlag)
		error_print("userThread: error in pthread_setcancelstate()\n");
//	if (error == PTHREAD_CANCEL_ENABLE)
//		debug_print(DEBUG_L1, "Thread already cancellable\n");

	debug_print(DEBUG_OFF, "userThread: Waiting for commands: \n");
	exitFlag = 0;

	while (!exitFlag && (fgets(userInput, sizeof(userInput), stdin) != NULL)) {

		//mangio il newline
		userInput[strlen(userInput) - 1] = '\0';

		//controllo che comando ho inviato
		if (strcmp(userInput, "help") == 0) {
			debug_print(DEBUG_OFF,
					"userThread: Valid commands: add node, add sync, add folder, check update, reset database, dump database, force update, delete sync, delete synchronizations, delete folder, print watchlist, exit\n");
		}

		//controllo che comando ho inviato
		if (strcmp(userInput, "add node") == 0) {
			error = addNodeUserOperation(dbConn, argomenti->generalMutex);
			if (error)
				error_print("userThread: failure. Error in addNodeUserOperation()\n");
		} //fine add node
//--------------------------------------------------------------------------------------------------------------------------------------FINE ADD NODE
		if (strcmp(userInput, "add sync") == 0) {
			error = addSyncUserOperation(dbConn, argomenti->generalMutex,
					argomenti->myNode);
			if (error)
				error_print("userThread: failure. Error in addSyncUserOperation()\n");
		}
//---------------------------------------------------------------------------------------------------------------------------------------FINE ADD SYNC
		if (strcmp(userInput, "add folder") == 0) {
			error = addFolderUserOperation(dbConn, argomenti->generalMutex,
					argomenti->myNode);
			if (error)
				error_print("userThread: failure. Error in addFolderUserOperation()\n");
		}
//--------------------------------------------------------------------------------------------------------------------------------------FINE ADD FOLDER
		if (strcmp(userInput, "reset database") == 0) {
			error = resetDatabaseUserOperation(dbConn, argomenti->generalMutex,
					argomenti->myNode);
			if (error) {
				error_print(
						"userThread: failure. Error in resetDatabaseUserOperation()\n");
			}
		}
//-----------------------------------------------------------------------------------------------------------------------------FINE RESET DATABASE

		if (strcmp(userInput, "dump database") == 0) {
			error = dumpDatabaseUserOperation(dbConn, argomenti->generalMutex);
			if (error) {
				error_print(
						"userThread: failure. Error in dumpDatabaseUserOperation()\n");
			}
		}
//---------------------------------------------------------------------------------------------------------------------------------------FINE DUMP DATABASE
		if (strcmp(userInput, "force update") == 0) {
			error = forceUpdateUserOperation(dbConn, argomenti->generalMutex);
			if (error) {
				error_print(
						"userThread: failure. Error in forceUpdateUserOperation()\n");
			}
		}
//--------------------------------------------------------------------------------------------------------------------------------------FINE FORCE UPDATE

		if (strcmp(userInput, "delete sync") == 0) {
			error = deleteSyncUserOperation(dbConn, argomenti->generalMutex,
					argomenti->myNode);
			if (error) {
				error_print("userThread: failure. Error in deleteSyncUserOperation()\n");
			}
		}
//-------------------------------------------------------------------------------------------------------------------------------------------FINE DELETE SYNC
		if (strcmp(userInput, "delete folder") == 0) {
			error = deleteFolderUserOperation(dbConn, argomenti->generalMutex,
					argomenti->myNode);
			if (error) {
				error_print(
						"userThread: failure. Error in deleteFolderUserOperation()\n");
			}
		}		//fine delete folder

//----------------------------------------------------------------------------------------------------------------------------------------FINE DELETE FOLDER
		if (strcmp(userInput, "check update") == 0) {
			error = checkUpdateUserOperation(dbConn, argomenti->generalMutex,
					argomenti->myNode);
			if (error) {
				error_print(
						"userThread: failure. Error in checkUpdateUserOperation()\n");
			}
		}
//----------------------------------------------------------------------------------------------------------------------------------------FINE CHECK UPDATE

		if (strcmp(userInput, "print watchlist") == 0) {
			error = printWatchListUserOperation(dbConn,
					argomenti->generalMutex);
			if (error)
				error_print(
						"userThread: failure. Error in printWatchListUserOperation()\n");
		}

//----------------------------------------------------------------------------------------------------------------------------------------DELETE SYNC FOLDER
		if (strcmp(userInput, "delete synchronizations") == 0) {
			error = deleteSynchronizationsOnFolderUserOperation(dbConn,
					argomenti->generalMutex, argomenti->myNode);
			if (error) {
				error_print(
						"userThread: failure. Error in deleteSynchronizationsOnFolderUserOperation()\n");			
			}
		}

		if (strcmp(userInput, "exit") == 0) {
			exitFlag = 1;
		}

		//torno al ciclo
		if (!exitFlag)
			debug_print(DEBUG_OFF, "userThread: Waiting for commands: \n");
	}

//	kill(getpid(), SIGINT);
	debug_print(DEBUG_L1, "userThread: Terminated correctly\n");
	pthread_exit(NULL);
}

//add a node to DB and (to whitelist or blacklist)
static int addNodeUserOperation(sqlite3 *dbConn, pthread_mutex_t *generalMutex) {

	int error;
	int ok;

	char userInput[USER_INPUT_LENGTH] = "";

	char checkString1[TEMP_STRING_LENGTH] = "";
	char checkString2[TEMP_STRING_LENGTH] = "";

	int checkInt1;
	int checkInt2;

	dtnNode tempNode;

//get user input--------------------------------------------------------------------------------------------------

	//get node EID from user
	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the EID: (e.g. ipn:5.3000 or dtn://node_name.dtn/DTNbox)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (sscanf(userInput, "dtn://%s.dtn/%s", checkString1, checkString2)
				!= 1
				&& sscanf(userInput, "ipn:%d.%d", &checkInt1, &checkInt2) != 2)
			continue;
		else {
			ok = 1;
			strcpy(tempNode.EID, userInput);
		}

	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: \"add node\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	//get node lifetime from user
	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the bundle lifetime for this node (in seconds), default: %d\n", DEFAULT_LIFETIME);
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if(strlen(userInput) == 0){
			//user pressed return key
			ok=1;
			tempNode.lifetime = DEFAULT_LIFETIME;
		}
		else if (sscanf(userInput, "%d", &checkInt1) != 1)
			continue;
		else {
			ok = 1;
			tempNode.lifetime = atoi(userInput);
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add node\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	//get node tx attemps
	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF, "userThread: Enter the number of Tx attemps for this node, default: %d\n", DEFAULT_NUM_RETRY);
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if(strlen(userInput) == 0){
			ok=1;
			tempNode.numTx = DEFAULT_NUM_RETRY;
		}
		if (sscanf(userInput, "%d", &checkInt1) != 1)
			continue;
		else {
			ok = 1;
			tempNode.numTx = atoi(userInput);
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add node\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	//node in whitelist or blacklist?
	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter W to add the node to the whiteList, B to the blackList, default: W\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if(strlen(userInput) == 0){
			ok=1;
			tempNode.blackWhite = WHITELIST;
		}
		else if (strcmp(userInput, "W") == 0 || strcmp(userInput, "B") == 0){
			ok = 1;
			switch(userInput[0]){
			case 'W':{
				tempNode.blackWhite = WHITELIST;
				break;
			}
			case 'B':{
				tempNode.blackWhite = BLACKLIST;
				break;
			}
			default:{
				ok=0;
				continue;
			}
			}
		}
		else
			continue;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add node\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	tempNode.blockedBy = NOT_BLOCKEDBY;
	tempNode.frozen = NOT_FROZEN;


	pthread_mutex_lock(generalMutex);
	debug_print(DEBUG_OFF, "userThread: Adding the node...\n");
	//aggiungo il nodo al database
	error = DBConnection_addDtnNode(dbConn, tempNode);
	if (error) {
		error_print(
				"userThread: failure. addNodeUserOperation: error in DBConnection_addDtnNode()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	debug_print(DEBUG_OFF, "userThread: ok. Node successfully added!\n");
	pthread_mutex_unlock(generalMutex);

	return SUCCESS_VALUE;
}

//add a synchronization
static int addSyncUserOperation(sqlite3* dbConn, pthread_mutex_t *generalMutex,
		dtnNode myNode) {

	int error;
	int ok;

	char userInput[USER_INPUT_LENGTH];

//	int result;
	char EIDNodo[NODE_EID_LENGTH];
	char commandText[SYNC_COMMAND_TEXT_SIZE_LIMIT];

	char myOwnerFromEID[OWNER_LENGTH];
	char destOwnerFromEID[OWNER_LENGTH];

	char absoluteFolderPath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	synchronization tempSync;
	int folderToSyncID;
	int synchronizationID;

	SynchronizationState synchronizationStatus;
	SyncMode syncMode;

	SynchronizationState synchronizationStatusWithParent;

	int hasPullMode = 0;
	int hasPushAndPullInputMode = 0;

	command* cmd;

	char checkString1[TEMP_STRING_LENGTH];
	char checkString2[TEMP_STRING_LENGTH];

	int checkInt1;
	int checkInt2;

//getUserInput----------------------------------------------------------------------------------------------------------------

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the folder owner: (from its EID e.g. [ipn:5.3000, owner: 5] or [dtn://node_name.dtn/DTNbox, owner: node_name])\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (sscanf(userInput, "%d", &checkInt1) != 1
				&& sscanf(userInput, "%s", checkString1) != 1)
			continue;
		else {
			ok = 1;
			strcpy(tempSync.folder.owner, userInput);
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF, "userThread: Enter the folder name: (e.g. photos)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (strlen(userInput) > 0) {
			strcpy(tempSync.folder.name, userInput);
			ok = 1;
		} else
			continue;

	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the EID of the synchronization pair node: (e.g. ipn:5.3000 or dtn://node_name.dtn/DTNbox)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (sscanf(userInput, "dtn://%s.dtn/%s", checkString1, checkString2)
				!= 1
				&& sscanf(userInput, "ipn:%d.%d", &checkInt1, &checkInt2) != 2)
			continue;
		else {
			strcpy(EIDNodo, userInput); //mettiamo in EIDnodo
			ok = 1;
		}

	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the synchronization mode: (PUSH, PULL, PUSH&PULL_IN, PUSH&PULL_OUT)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		//default value deleted
		if (strcmp(userInput, "PUSH") == 0) {
			tempSync.mode = PUSH;
			ok = 1;
		} else if (strcmp(userInput, "PULL") == 0) {
			tempSync.mode = PULL;
			ok = 1;
		} else if (strcmp(userInput, "PUSH&PULL_IN") == 0) {
			tempSync.mode = PUSH_AND_PULL_IN;
			ok = 1;
		} else if (strcmp(userInput, "PUSH&PULL_OUT") == 0) {
			tempSync.mode = PUSH_AND_PULL_OUT;
			ok = 1;
		} else {
			debug_print(DEBUG_OFF, "userThread: Unknown synchronization mode\n");
			continue;
		}
	} //while !ok
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	tempSync.pwdRead[0] = '\0';
	tempSync.pwdWrite[0] = '\0';
	//TODO not yet implemented
//	debug_print(DEBUG_OFF, "userThread: Inserisci password lettura (opzionale)\n");
//	if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
//		debug_print(DEBUG_OFF, "userThread: operazione di \"add sync\" abortita\n");
//		return SUCCESS_VALUE;
//	}
//	userInput[strlen(userInput) - 1] = '\0';
//	if (strlen(userInput) == 0) {
//		strcpy(tempSync.pwdRead, "nopassword");
//	} else {
//		strcpy(tempSync.pwdRead, userInput);
//	}
//
//	debug_print(DEBUG_OFF, "userThread: Inserisci password scrittura (opzionale)\n");
//	if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
//		debug_print(DEBUG_OFF, "userThread: operazione di \"add sync\" abortita\n");
//		return SUCCESS_VALUE;
//	}
//	userInput[strlen(userInput) - 1] = '\0';
//	if (strlen(userInput) == 0) {
//		strcpy(tempSync.pwdWrite, "nopassword");
//	} else {
//		strcpy(tempSync.pwdWrite, userInput);
//	}

//end getUserInput----------------------------------------------------------------------------------------------------------------

	getOwnerFromEID(myOwnerFromEID, myNode.EID);
	getOwnerFromEID(destOwnerFromEID, EIDNodo);

	getAbsolutePathFromOwnerAndFolder(absoluteFolderPath, tempSync.folder.owner,
			tempSync.folder.name);

	tempSync.folder.files = fileToSyncList_create();

	error = 0;

	pthread_mutex_lock(generalMutex);

//retrieve data from DB to check if the sinchronization is valid------------------------------------------------------------------------------

	//recupero il nodo dal db
	error = DBConnection_getDtnNodeFromEID(dbConn, &(tempSync.node), EIDNodo);
	if (error && error == DB_DATA_NOT_FOUND_ERROR) {
		error_print("userThread: failure. addSyncUserOperation: %s not found\n", EIDNodo);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	} else if (error) {
		error_print(
				"userThread: failure. addSyncUserOperation: error in DBConnection_getDtnNodeFromEID()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if(tempSync.node.blackWhite == BLACKLIST){
		error_print("userThread: failure. addSyncUserOperation: error, you cannot send a request of synchronization to a node you blocked\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	if(tempSync.node.blockedBy == BLOCKEDBY){
		error_print("userThread: failure. addSyncUserOperation: error, you cannot send a request of synchronization to a node that blocked you\n");	
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	//TODO can we send a sync request to a frozen node? Note: the request will be sent only after its unfreeze notice
//	if(tempSync.node.frozen == FROZEN){
//		error_print("addSyncUserOperation: error, you cannot send a request of synchronization to a node that is frozen\n");
//		pthread_mutex_unlock(generalMutex);
//		return ERROR_VALUE;
//	}

	//getFolderToSyncID (if exists)
	folderToSyncID = 0;
	error = DBConnection_getFolderToSyncID(dbConn, tempSync.folder,
			&folderToSyncID);
	if (error && error != DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"userThread: failure. addSyncUserOperation: error in DBConnection_getFolderToSyncID()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	synchronizationID = 0;
	hasPullMode = 0;
	hasPushAndPullInputMode = 0;
	if (folderToSyncID != 0) {
		//if(folderToSyncID != 0) <-> if I have the folder on DB

		//only if I have the folder I can have the synchronization!!

		//getSynchronizationID (if exists)
		error = DBConnection_getSynchronizationID(dbConn, tempSync,
				&synchronizationID);
		if (error && error != DB_DATA_NOT_FOUND_ERROR) {
			error_print(
					"userThread: failure. addSyncUserOperation: error in DBConnection_getSynchronizationID()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		if (!error) {
			//if we have this sinchronization
			error = DBConnection_getSynchronizationStatus(dbConn, tempSync,
					&synchronizationStatus);
			if (error) {
				error_print(
						"userThread: failure. addSyncUserOperation: error in DBConnection_getSynchronizationStatus()\n");
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
			error = DBConnection_getSynchronizationMode(dbConn, tempSync,
					&syncMode);
			if (error) {
				error_print(
						"userThread: failure. addSyncUserOperation: error in DBConnection_getSynchronizationMode()\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}

			debug_print(DEBUG_OFF,
					"userThread: failure. addSyncUserOperation: You already have a synchronization (confirmed or pending) on this folder with this node\n");
			debug_print(DEBUG_L1,
					"addSyncUserOperation: STATUS: %d, MODE: %d\n",
					synchronizationStatus, syncMode);
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}

		//check if already sync... with this node...
		error = DBConnection_hasSynchronizationMode(dbConn, tempSync.folder,
				PULL, &hasPullMode);
		if (error) {
			error_print(
					"userThread: failure. addSyncUserOperation: error in DBConnection_hasSynchronizationMode()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}

		error = DBConnection_hasSynchronizationMode(dbConn, tempSync.folder,
				PUSH_AND_PULL_IN, &hasPushAndPullInputMode);
		if (error) {
			error_print(
					"userThread: failure. addSyncUserOperation: error in DBConnection_hasSynchronizationMode()\n");			
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}

		if (strcmp(tempSync.folder.owner, myOwnerFromEID) != 0) {
			//if I am not the owner of the folder but I have the folder on DB, then I must have a sync (CONFIRMED or PENDING) with my parent, else error!
			error = DBConnection_getSynchronizationStatusWithParent(dbConn,
					tempSync.folder, &synchronizationStatusWithParent);
			if (error) {
				error_print(
						"userThread: failure. addSyncUserOperation: error in DBConnection_getSynchronizationStatusWithParent()\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
		}

	} //if(folderToSyncID != 0) <-> if I have the folder on DB

//END retrieve data from DB to check if the sinchronization is valid------------------------------------------------------------------------------

//constraints---------------------------------------------------------------------------------------------
	if (strcmp(myOwnerFromEID, destOwnerFromEID) == 0) {
		debug_print(DEBUG_OFF,
				"userThread: failure. addSyncUserOperation: You cannot send the request to yourself\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if (strcmp(myOwnerFromEID, tempSync.folder.owner) == 0) {
		//I am the owner
		if (tempSync.mode == PULL || tempSync.mode == PUSH_AND_PULL_IN) {
			debug_print(DEBUG_OFF,
					"userThread: failure. addSyncUserOperation: You cannot synchronize with a folder of yours\n");	
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		} else if (tempSync.mode == PUSH
				|| tempSync.mode == PUSH_AND_PULL_OUT) {
			//we must have the folder...
			if (folderToSyncID == 0 || !fileExists(absoluteFolderPath)) {
				debug_print(DEBUG_OFF, "userThread: failure. addSyncUserOperation: This folder does not exist\n");
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
		}
	} else {
		if(strcmp(destOwnerFromEID, tempSync.folder.owner) == 0 &&
				(tempSync.mode == PUSH || tempSync.mode == PUSH_AND_PULL_OUT)) {
			debug_print(DEBUG_OFF,
					"userThread: failure. addSyncUserOperation: You cannot offer this folder to its owner\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		if((tempSync.mode == PULL
				|| tempSync.mode == PUSH_AND_PULL_IN) && (hasPullMode || hasPushAndPullInputMode)){
			debug_print(DEBUG_OFF,
					"userThread: failure. addSyncUserOperation: You already have a  %s synchronization confirmed or pending\n",
					hasPullMode ? "PULL" : "PUSH&PULL_IN");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		if(tempSync.mode == PUSH || tempSync.mode == PUSH_AND_PULL_OUT){
			if (synchronizationStatusWithParent
					== SYNCHRONIZATION_PENDING) {
				debug_print(DEBUG_OFF,
						"userThread: failure. addSyncUserOperation: You cannot offer a folder whose synchronization is still pending\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
			if (folderToSyncID == 0 || !fileExists(absoluteFolderPath)) {
				debug_print(DEBUG_OFF,
						"userThread: failure. addSyncUserOperation: You cannot offer (PUSH or PUSH&PULL_OUT) a folder you don't have\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
			if(tempSync.mode == PUSH_AND_PULL_OUT){
				if (hasPullMode || !hasPushAndPullInputMode) {
					debug_print(DEBUG_OFF,
							"userThread: failure. addSyncUserOperation: You cannot offer a folder synchronized with you in PULL mode only\n");					
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
			}
		}
	}
//end constraints---------------------------------------------------------------------------------------------

	//if (folderToSyncID == 0) {
	if (tempSync.mode == PULL || tempSync.mode == PUSH_AND_PULL_IN) {
		error = DBConnection_addFolderToSync(dbConn, tempSync.folder);
		if (error) {
			//should never happen...
			error_print(
					"userThread: failure. addSyncUserOperation: error in DBConnection_addFolderToSync()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
	}

	tempSync.state = SYNCHRONIZATION_PENDING;
	error = DBConnection_addSynchronization(dbConn, tempSync);
	if (error) {
		//should never happen...
		error_print(
				"userThread: failure. addSyncUserOperation: error in DBConnection_addSynchronization()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	//here we can send the sync command
	//mi costruisco il testo del comando
	createSyncText(tempSync, tempSync.node.lifetime, commandText);

	debug_print(DEBUG_L1, "userThread: command has been created=%s\n",
			commandText);

	//procedo a creare il comando effettivo
	newCommand(&cmd, commandText);

	//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
	cmd->msg.destination = tempSync.node;
	cmd->msg.source = myNode;
	cmd->msg.txLeft = tempSync.node.numTx;
	cmd->msg.nextTxTimestamp = getNextRetryDate(tempSync.node);

	error = DBConnection_controlledAddCommand(dbConn, cmd);
	destroyCommand(cmd);
	if (error) {
		error_print(
				"userThread: failure. addSyncUserOperation: error in DBConnection_controlledAddCommand()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	debug_print(DEBUG_OFF, "userThread: ok. Synchronization request created!\n");
	pthread_mutex_unlock(generalMutex);
	return SUCCESS_VALUE;
}

//add a folder to FS and to DB
static int addFolderUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode) {

	int ok;
	int error;
	char userInput[USER_INPUT_LENGTH];

	folderToSync tempFolder;

	char absoluteFolderPath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	int isFolderOnDB;

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF, "userThread: Enter the folder name: (e.g. photos)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (strlen(userInput) == 0)
			continue;
		else {
			ok = 1;
			strcpy(tempFolder.name, userInput);
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"add folder\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	//mi ricavo l'owner visto che sono io
	getOwnerFromEID(tempFolder.owner, myNode.EID);
	getAbsolutePathFromOwnerAndFolder(absoluteFolderPath, tempFolder.owner,
			tempFolder.name);

	pthread_mutex_lock(generalMutex);
	debug_print(DEBUG_OFF, "userThread: Creating folder...\n");
	//controllo se esiste e in caso negativo la creo
	error = checkAndCreateFolder(absoluteFolderPath);
	if (error) {
		error_print(
				"userThread: failure. addFolderUserOperation: error in checkAndCreateFolder()\n");
		pthread_mutex_unlock(generalMutex);
		return error;
	}

	//creata la cartella la aggiungo al db se non è già presente
	error = DBConnection_isFolderOnDb(dbConn, tempFolder, &isFolderOnDB);
	if (error) {
		error_print(
				"userThread: failure. addFolderUserOperation: error in DBConnection_isFolderOnDb()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if (!isFolderOnDB) {
		tempFolder.files = NULL;
		error = DBConnection_addFolderToSync(dbConn, tempFolder);
		if (error) {
			error_print(
					"userThread: failure. addFolderUserOperation: error in DBConnection_addFolderToSync()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
	}
	debug_print(DEBUG_OFF, "userThread: ok. Folder successfully created and added!\n");
	pthread_mutex_unlock(generalMutex);

	return SUCCESS_VALUE;
}

//delete a sync and apply... see code
static int deleteSyncUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode) {

	int error;
	int ok;
	char userInput[USER_INPUT_LENGTH];
	int i;

	synchronization sync;
	int synchronizationID;
	int folderToSyncID;
	SyncMode mode;

	command* cmd;
	char commandText[FIN_COMMAND_TEXT_SIZE_LIMIT];

	char removeCommand[SYSTEM_COMMAND_LENGTH + FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];	//rm -rf ~/DTNbox/foldersToSync/owner/folder/

	int fromOwner;

	synchronization* syncsOnFolder;
	int numSync;

	char myOwnerFromEID[OWNER_LENGTH];
	char dirName[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	char EIDNodo[NODE_EID_LENGTH];

	//variables to check user input
	char checkString1[TEMP_STRING_LENGTH];
	char checkString2[TEMP_STRING_LENGTH];
	int checkInt1;
	int checkInt2;

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the folder owner: (from its EID e.g. [ipn:5.3000, owner: 5] or [dtn://node_name.dtn/DTNbox, owner: node_name])\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (sscanf(userInput, "%d", &checkInt1) != 1
				&& sscanf(userInput, "%s", checkString1) != 1)
			continue;
		else {
			ok = 1;
			strcpy(sync.folder.owner, userInput);
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the folder name: (relative name, e.g. folder)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';
		if (strlen(userInput) > 0) {
			strcpy(sync.folder.name, userInput);
			ok = 1;
		} else
			continue;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the EID of the paired node: (e.g. ipn:5.3000 or dtn://node_name.dtn/DTNbox)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;

		userInput[strlen(userInput) - 1] = '\0';

		if (sscanf(userInput, "dtn://%s.dtn/%s", checkString1, checkString2)
				!= 1
				&& sscanf(userInput, "ipn:%d.%d", &checkInt1, &checkInt2) != 2)
			continue;
		else
			ok = 1;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

//END user input --------------------------------------------------------------------------------------------------------------------------------

	strcpy(EIDNodo, userInput);

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Warning: by deleting the synchronization with the parent (PULL or PUSH&PULL_IN) your local folder will be deleted, are you sure? (YES, upper case)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;

		userInput[strlen(userInput) - 1] = '\0';

		if (strlen(userInput) == 0)
			continue;
		else
			ok = 1;
	}
	if (!ok || strcmp("YES", userInput) != 0) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete sync\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	getAbsolutePathFromOwnerAndFolder(dirName, sync.folder.owner,
			sync.folder.name);

	getOwnerFromEID(myOwnerFromEID, myNode.EID);

	fromOwner = 0;
	if (strcmp(myOwnerFromEID, sync.folder.owner) == 0)
		fromOwner = 1;

//END USER INPUT-----------------------------------------------------------------------------------------------------------------
	pthread_mutex_lock(generalMutex);
	//recupero il nodo dal db
	error = DBConnection_getDtnNodeFromEID(dbConn, &(sync.node), EIDNodo);
	if (error) {
		error_print("userThread: failure. Error in DBConnection_getDtnNodeFromEID()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	//TODO 	can I delete a sync with a blocked or blocked-by node? YES, but ...
	//		...with the current implementation,a sync should not exists with this node, because, currenty, a node cannot be moved from BLACKLIST to WHITELIST and viceversa
	//		can I delete a sync with a frozen node? Yes, but the command will be send only when the node inform us that he unfreezed...

	error = DBConnection_getFolderToSyncID(dbConn, sync.folder,
			&folderToSyncID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print("userThread: failure. folder not found\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	} else if (error) {
		error_print("userThread: failure. Error in DBConnection_getFolderToSyncID()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	synchronizationID = 0;
	error = DBConnection_getSynchronizationID(dbConn, sync, &synchronizationID);
	if (error && error != DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"userThread: failure. Error in DBConnection_getSynchronizationID()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if (!error && synchronizationID > 0) {

		error = DBConnection_getSynchronizationMode(dbConn, sync, &mode);
		if (error) {
			error_print(
					"userThread: failure. Error in DBConnection_getSynchronizationMode()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}

		if (mode == PULL || mode == PUSH_AND_PULL_IN) {
			//sto eliminando la sincronizzazione chiave, la radice dell'albero...

			//elimino la cartella dalla watchList
			if (mode == PUSH_AND_PULL_IN) {
				if (watchList_containsFolder(dirName)) {
					error = recursive_remove(dirName);
					if (error) {
						error_print("userThread: failure. Error in recursive_remove()\n");
						pthread_mutex_unlock(generalMutex);
						return error;
					}
				}
			}
			//elimina la cartella dal filesystem
			sprintf(removeCommand, "rm -rf %s", dirName);
			error = system(removeCommand);
			if (error) {
				error_print("userThread: failure. Error in system(%s)", removeCommand);
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
			//if empty
			getFoldersToSyncPath(dirName);
			strcat(dirName, sync.folder.owner);
			if (isDirectoryEmpty(dirName)) {
				sprintf(removeCommand, "rm -rf %s", dirName);
				error = system(removeCommand);
				if (error) {
					error_print("userThread: failure. Error in system(%s)",
							removeCommand);
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
			}

			//propago la fine a tutti
			syncsOnFolder = NULL;
			numSync = 0;
			error = DBConnection_getAllSynchronizationsOnFolder(dbConn,
					sync.folder, &syncsOnFolder, &numSync);
			if (error) {
				error_print(
						"userThread: failure. deleteSyncUserOperation: error in DBConnection_getAllSynchronizationsOnFolder()\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
			for (i = 0; i < numSync; i++) {
				//Per ogni sync creo il comando di FIN

				//mi costruisco il testo del comando
				//Fin timestamp owner folderName fromOwner
				sprintf(commandText, "Fin\t%llu\t%s\t%s\t%d\n",
						getCurrentTime(), syncsOnFolder[i].folder.owner,
						syncsOnFolder[i].folder.name, fromOwner);

				debug_print(DEBUG_L1,
						"userThread: Command created=%s\n",
						commandText);

				//procedo a creare il comando effettivo
				newCommand(&cmd, commandText);

				//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
				cmd->msg.destination = syncsOnFolder[i].node;
				cmd->msg.source = myNode;
				cmd->msg.txLeft = syncsOnFolder[i].node.numTx;
				cmd->msg.nextTxTimestamp = getNextRetryDate(
						syncsOnFolder[i].node);

				error = DBConnection_controlledAddCommand(dbConn, cmd);
				destroyCommand(cmd);
				if (error) {
					free(syncsOnFolder);
					error_print(
							"userThread: failure. deleteSyncUserOperation: error in DBConnection_controlledAddCommand()\n");					
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}

				//cancello la sincronizzazione per il nodo
				error = DBConnection_deleteSynchronization(dbConn,
						syncsOnFolder[i]);
				if (error) {
					free(syncsOnFolder);
					error_print(
							"userThread: failure. deleteSyncUserOperation: error in DBConnection_deleteSynchronization()\n");					
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
			}
			if (syncsOnFolder != NULL)
				free(syncsOnFolder);

			//delete folder from DB
			error = DBConnection_deleteFolder(dbConn, sync.folder);
			if (error) {
				error_print(
						"userThread: failure. deleteSyncUserOperation: error in DBConnection_deleteFolder()\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}

		}				//if I want to delete main sync
		else {
			//I want to delete another sync (not the main sync)
			sprintf(commandText, "Fin\t%llu\t%s\t%s\t%d\n", getCurrentTime(),
					sync.folder.owner, sync.folder.name, fromOwner);

			debug_print(DEBUG_L1, "userThread: Created command=%s\n",
					commandText);

			//procedo a creare il comando effettivo
			newCommand(&cmd, commandText);

			//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
			cmd->msg.destination = sync.node;
			cmd->msg.source = myNode;
			cmd->msg.txLeft = sync.node.numTx;
			cmd->msg.nextTxTimestamp = getNextRetryDate(sync.node);

			error = DBConnection_controlledAddCommand(dbConn, cmd);
			destroyCommand(cmd);
			if (error) {
				error_print(
						"userThread: failure. Error in DBConnection_controlledAddCommand()\n");
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}

			//Mi è stata chiesta la fine di una sincro, come prima cosa cancello i dati sul database
			error = DBConnection_deleteSynchronization(dbConn, sync);
			if (error) {
				error_print(
						"userThread: failure. Error in DBConnection_deleteSynchronization()\n");
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}

			if (strcmp(sync.folder.owner, myOwnerFromEID) == 0
					&& watchList_containsFolder(dirName)) {

				error = DBConnection_getNumSynchronizationsOnFolder(dbConn,
						sync.folder, &numSync);
				if (error) {
					error_print(
						"userThread: failure. Error in DBConnection_getNumSynchronizationsOnFolder()\n");					
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
				if (numSync == 0 && watchList_containsFolder(dirName)) {
					error = recursive_remove(dirName);
					if (error) {
						error_print(
							"userThread: failure. Error in recursive_remove()\n");
						pthread_mutex_unlock(generalMutex);
						return error;
					}
					error = DBConnection_deleteFilesForFolder(dbConn,
							folderToSyncID);
					if (error) {
						error_print(
								"userThread: failure. Error in DBConnection_deleteFilesForFolder()\n");						
						pthread_mutex_unlock(generalMutex);
						return ERROR_VALUE;
					}

				}
			}
		}

		debug_print(DEBUG_OFF,
				"userThread: ok. Synchronization deleted, the node will be notified!\n");
	} else {
		debug_print(DEBUG_OFF, "userThread: failure. This synchronization does not exist!\n");
	}
	pthread_mutex_unlock(generalMutex);

	return SUCCESS_VALUE;
}

//delete a folder from FS and DB
static int deleteFolderUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode) {

	int error;
	int ok;

	char userInput[USER_INPUT_LENGTH];
	folderToSync tempFolder;

	char userFolder[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	char rmCommand[SYSTEM_COMMAND_LENGTH + FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	int result;

	int i = 0;
	synchronization * syncsOnFolder = NULL;
	int numSyncs = 0;
	char commandText[FIN_COMMAND_TEXT_SIZE_LIMIT]; //contains the fin command
	command *cmd;

	int folderToSyncID;

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF, "userThread: Enter the folder to be deleted: (e.g. photos)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';
		if (strlen(userInput) > 0) {
			strcpy(tempFolder.name, userInput);
			ok = 1;
		} else
			continue;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete folder\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	//mi ricavo l'owner dal mio EID
	getOwnerFromEID(tempFolder.owner, myNode.EID);

	getAbsolutePathFromOwnerAndFolder(userFolder, tempFolder.owner,
			tempFolder.name);

	//controllo se la cartella esiste
	if (fileExists(userFolder)) {

		//cancello la cartella dal mio fs
		sprintf(rmCommand, "rm -rf %s", userFolder);

		//visto che in caso l'utente inserisca */* fondamentalmente formatterei l'hd chiedo prima all'utente la conferma del comando
		debug_print(DEBUG_OFF,
				"userThread: The deletion command \"%s\" will be executed and all the synchronizations on the folder will be terminated, are you sure? (YES, upper case)\n",
				rmCommand);
		if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
			debug_print(DEBUG_OFF, "userThread: failure. \"delete folder\" operation aborted\n");
			return SUCCESS_VALUE;
		}
		userInput[strlen(userInput) - 1] = '\0';
		if (strcmp(userInput, "YES") == 0) {

			pthread_mutex_lock(generalMutex);

			if (watchList_containsFolder(userFolder)) {
				error = recursive_remove(userFolder);
				if (error) {
					error_print(
							"userThread: failure. deleteFolderUserOperation: error in recursive_remove()\n",
							rmCommand);
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
			}

			//lancio il comando
			error = system(rmCommand);
			if (error) {
				error_print(
						"userThread: failure. deleteFolderUserOperation: error while executing system command %s\n",
						rmCommand);
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}

			//cancello la cartella dal DB
			error = DBConnection_isFolderOnDb(dbConn, tempFolder, &result);
			if (error) {
				error_print(
						"userThread: failure. deleteFolderUserOperation: error in DBConnection_isFolderOnDb\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
			if (result) {

				error = DBConnection_getFolderToSyncID(dbConn, tempFolder,
						&folderToSyncID);
				if (error) {
					error_print(
							"userThread: failure. userThread: error in DBConnection_getFolderToSyncID()\n");;
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}

				syncsOnFolder = NULL;
				numSyncs = 0;
				error = DBConnection_getAllSynchronizationsOnFolder(dbConn,
						tempFolder, &syncsOnFolder, &numSyncs);
				if (error) {
					error_print(
							"userThread: failure. deleteSyncUserOperation: error in DBConnection_getAllSynchronizationsOnFolder()\n");					
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
				for (i = 0; i < numSyncs; i++) {

					//TODO 	can I delete a sync with a blocked or blocked-by node? YES, but ...
					//		...with the current implementation,a sync should not exists with this node, because, currenty, a node cannot be moved from BLACKLIST to WHITELIST and viceversa

					//		can I delete a sync with a frozen node? Yes, but the command will be send only when the node inform us that he unfreezed...


					//Per ogni sync creo il comando di FIN
					//mi costruisco il testo del comando
					//Fin timestamp owner folderName fromOwner
					sprintf(commandText, "Fin\t%llu\t%s\t%s\t%d\n",
							getCurrentTime(), syncsOnFolder[i].folder.owner,
							syncsOnFolder[i].folder.name, 1);

					debug_print(DEBUG_L1, "userThread: Command created=%s\n",
							commandText);

					//procedo a creare il comando effettivo
					newCommand(&cmd, commandText);

					//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
					cmd->msg.destination = syncsOnFolder[i].node;
					cmd->msg.source = myNode;
					cmd->msg.txLeft = syncsOnFolder[i].node.numTx;
					cmd->msg.nextTxTimestamp = getNextRetryDate(
							syncsOnFolder[i].node);

					error = DBConnection_controlledAddCommand(dbConn, cmd);
					destroyCommand(cmd);
					if (error) {
						free(syncsOnFolder);
						error_print(
								"userThread: failure. deleteSyncUserOperation: error in DBConnection_controlledAddCommand()\n");
						pthread_mutex_unlock(generalMutex);
						return ERROR_VALUE;
					}

					//cancello la sincronizzazione per il nodo
					error = DBConnection_deleteSynchronization(dbConn,
							syncsOnFolder[i]);
					if (error) {
						free(syncsOnFolder);
						error_print(
								"deleteSyncUserOperation: error in DBConnection_deleteSynchronization()\n");
						pthread_mutex_unlock(generalMutex);
						return ERROR_VALUE;
					}
				}
				if (syncsOnFolder != NULL)
					free(syncsOnFolder);

				error = DBConnection_deleteFolder(dbConn, tempFolder);
				if (error) {
					error_print(
							"userThread: failure. deleteSyncUserOperation: error in DBConnection_deleteSynchronization()\n");					
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
			}					//if folder is on DB
			pthread_mutex_unlock(generalMutex);
		}					//if YES
	} else {
		debug_print(DEBUG_OFF, "userThread: failure. %s folder doesn't exists\n", userFolder);
	}
	return SUCCESS_VALUE;
}

//force update to all nodes
static int forceUpdateUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex) {

	char userInput[USER_INPUT_LENGTH];
	int error;

	debug_print(DEBUG_OFF,
			"userThread: The \"force update\" command will put all files in DESYNCHRONIZED, are you sure? (YES, upper case)\n");
	if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
		debug_print(DEBUG_OFF, "userThread: failure. \"force update\" aborted\n");
		return SUCCESS_VALUE;
	}
	userInput[strlen(userInput) - 1] = '\0';
	if (strcmp(userInput, "YES") == 0) {

		pthread_mutex_lock(generalMutex);
		debug_print(DEBUG_OFF,
				"userThread: Files are going to be put in DESYNCHRONIZED state...\n");
		error = DBConnection_forceUpdate(dbConn);
		if (error) {
			error_print(
					"userThread: failure. forceUpdateUserOperation: error in DBConnection_forceUpdate()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		debug_print(DEBUG_OFF, "userThread: ok. \"force update\" command successfully executed.\n");
		pthread_mutex_unlock(generalMutex);
	}
	return SUCCESS_VALUE;
}

//reset database
static int resetDatabaseUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode) {

	int error;
	char userInput[USER_INPUT_LENGTH];

	char currentUser[OWNER_LENGTH];
	char userFolder[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	debug_print(DEBUG_OFF,
			"userThread: This command will delete and recreate all the database tables, are you sure you want to continue (YES, in uppercase)\n");
	if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
		debug_print(DEBUG_OFF, "userThread: failure. \"reset database\" operation aborted\n");
		return SUCCESS_VALUE;
	}
	userInput[strlen(userInput) - 1] = '\0';
	if (strcmp(userInput, "YES") == 0) {

		pthread_mutex_lock(generalMutex);
		debug_print(DEBUG_OFF, "userThread: Proceed to delete tables...\n");
		error = DBConnection_dropTables(dbConn);
		if (error) {
			error_print(
					"userThread: failure. resetDatabaseUserOperation: error in DBConnection_dropTables()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		debug_print(DEBUG_OFF, "userThread: Tables deleted, proceed to recreate them\n");
		error = DBConnection_createTables(dbConn);
		if (error) {
			error_print(
					"userThread: failure. resetDatabaseUserOperation: error in DBConnection_createTables()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		debug_print(DEBUG_OFF, "userThread: Check your synchronization folders...\n");

		getOwnerFromEID(currentUser, myNode.EID);

		getFoldersToSyncPath(userFolder);
		strcat(userFolder, currentUser);
		strcat(userFolder, "/");

		error = scanFolders(dbConn, userFolder, currentUser);
		if (error) {
			error_print("userThread: failure. resetDatabaseUserOperation: error in scanFolders()\n");
			pthread_mutex_unlock(generalMutex);
			return error;
		}

		watchList_destroy();
		watchList_create();

		pthread_mutex_unlock(generalMutex);
	}

	return SUCCESS_VALUE;
}

//dump database
static int dumpDatabaseUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex) {

	int error;

	pthread_mutex_lock(generalMutex);
	debug_print(DEBUG_OFF, "userThread: Creating DTNboxDB.txt file...\n");
	error = DBConnection_dumpDBData(dbConn);
	if (error) {
		error_print(
				"userThread: failure. dumpDatabaseUserOperation: error in DBConnection_dumpDBData()\n");
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	debug_print(DEBUG_OFF, "userThread: ok. DTNboxDB.txt creation terminated!\n");
	pthread_mutex_unlock(generalMutex);
	return SUCCESS_VALUE;
}

static int checkUpdateUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode) {

	int error;
	int ok = 0;

	char userInput[FILETOSYNC_LENGTH];	//user input could exceed this size if the user want to manually check a file which size is more than 256...
										//to solve this we can use as size the maximum length allowed for a file
										//so don't use USER_INPUT_LENGTH
	int checkInt1;
	int checkInt2;

	char checkString1[TEMP_STRING_LENGTH];
	char checkString2[TEMP_STRING_LENGTH];

	char destEID[NODE_EID_LENGTH];
	synchronization tempSync;

	int synchronizationID;
	int folderToSyncID;

	char mode[MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT];
	char what[MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT];

	fileToSyncList files = fileToSyncList_create();
	fileToSyncList toScan = fileToSyncList_create();

	fileToSync tempFile;

	char checkUpdateString[CHECKUPDATE_COMMAND_TEXT_SIZE_LIMIT];
	int checkUpdateStringLength = 0;
	char currentTripleProcessed[FILETOSYNC_LENGTH + 64/*sizeof(unsigned long long) + sizeof(int) + 4*/];




	command *cmd;

	char folderToSyncPath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	int result;

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the folder owner: (from its EID e.g. [ipn:5.3000, owner: 5] or [dtn://node_name.dtn/DTNbox, owner: node_name])\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (strlen(userInput) > 0) {
			strcpy(tempSync.folder.owner, userInput);
			ok = 1;
		} else
			continue;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"check update\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF, "userThread: Enter the folder name: (e.g. photos)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';
		if (strlen(userInput) > 0) {
			strcpy(tempSync.folder.name, userInput);
			ok = 1;
		} else
			continue;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"check update\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"userThread: Enter the EID of the synchronization pair node: (e.g. ipn:5.3000 or dtn://node_name.dtn/DTNbox) to which send the checkUpdate command\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;

		userInput[strlen(userInput) - 1] = '\0';

		if (sscanf(userInput, "dtn://%s.dtn/%s", checkString1, checkString2)
				!= 1
				&& sscanf(userInput, "ipn:%d.%d", &checkInt1, &checkInt2) != 2)
			continue;
		else
			ok = 1;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"check update\" operation aborted\n");
		return SUCCESS_VALUE;
	}
	strcpy(destEID, userInput);

	ok = 0;
	while (!ok) {
		//mode
		debug_print(DEBUG_OFF, "userThread: Enter the mode: [%s/%s]\n", OFFER, ASK);
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';
		if (strlen(userInput) > 0
				&& (strcmp(userInput, OFFER) == 0 || strcmp(userInput, ASK) == 0)) {
			strcpy(mode, userInput);
			ok = 1;
		} else
			continue;
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"check update\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	if (strcmp(mode, ASK) == 0) {
		ok = 0;
		while (!ok) {
			//what
			debug_print(DEBUG_OFF, "userThread: Enter what you want to ask: [%s/%s/%s]\n",
			_LIST, _ALL,
			_FILE);
			if (fgets(userInput, sizeof(userInput), stdin) == NULL)
				break;
			userInput[strlen(userInput) - 1] = '\0';
			if (strlen(userInput) > 0
					&& (strcmp(userInput, _LIST) == 0
							|| strcmp(userInput, _ALL) == 0
							|| strcmp(userInput, _FILE) == 0)) {
				strcpy(what, userInput);
				ok = 1;
			} else
				continue;
		}
		if (!ok) {
			debug_print(DEBUG_OFF, "userThread: failure. \"check update\" operation aborted\n");
			return SUCCESS_VALUE;
		}
	} else if (strcmp(mode, OFFER) == 0)
		strcpy(what, _LIST);
	else {
		//shold never happen...
		error_print("userThread: failure. Mode not valid\n");
		return ERROR_VALUE;
	}

	if (strcmp(mode, ASK) == 0 && strcmp(what, _FILE) == 0) {
		//which file
		getHomeDir(folderToSyncPath);
		strcat(folderToSyncPath, DTNBOX_FOLDERSTOSYNCFOLDER);
		strcat(folderToSyncPath, tempSync.folder.owner);
		strcat(folderToSyncPath, "/");
		strcat(folderToSyncPath, tempSync.folder.name);
		strcat(folderToSyncPath, "/");

		files = fileToSyncList_create();
		toScan = fileToSyncList_create();

		/* Simple modification for compatibility with the GUI.
		 * In the following "while(...)", dtnbox is reading user input until
		 * it finds EOF, but with GUI this approach can't be used because EOF
		 * is read only when the input pipe used to send data to DTNbox is closed.
		 * For this reason i use instead a keyword to exit from this cicle.
		 * Keyword used:   "end-filename-list"   */

		debug_print(DEBUG_OFF,
				"userThread: Enter the filename of the file relative to the path %s (Insert \"end-filename-list\" to stop)\n",
				folderToSyncPath);
		while (fgets(userInput, sizeof(userInput), stdin) != NULL) {

			if (strcmp(userInput, "end-filename-list\n") ==0)
				break;

			userInput[strlen(userInput) - 1] = '\0';
			strcpy(tempFile.name, userInput);
			files = fileToSyncList_add(files, tempFile);
			debug_print(DEBUG_OFF,
					"userThread: Enter the filename of the file relative to the path %s (Insert \"end-filename-list\" to stop)\n",
					folderToSyncPath);
		}

		if (fileToSyncList_size(files) == 0) {
			debug_print(DEBUG_OFF, "userThread: failure. \"check update\" operation aborted\n");
			return SUCCESS_VALUE;
		}
	}

//end user input-------------------------------------------------------------------------------------------------------------------------------

	pthread_mutex_lock(generalMutex);

	error = DBConnection_getDtnNodeFromEID(dbConn, &(tempSync.node), destEID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print("userThread: failure. Node %s not found\n", destEID);
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	} else if (error) {
		error_print("userThread: failure. error in DBConnection_getDtnNodeFromEID()\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if(tempSync.node.blackWhite == BLACKLIST){
		error_print("userThread: failure. You cannot send a checkUpdate command to a blocked node\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	if(tempSync.node.blockedBy == BLOCKEDBY){
		error_print("userThread: failure. You cannot send a checkUpdate command to a node that blocked you\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	if(tempSync.node.frozen == FROZEN){
		//the checkUpdate command could became not more valid... as files are updated!!
		error_print("userThread: failure. You cannot send a checkUpdate command to a frozen node\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	error = DBConnection_getFolderToSyncID(dbConn, tempSync.folder,
			&folderToSyncID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print("userThread: failure. Error folder (owner: %s, folder: %s) not found\n",
				tempSync.folder.owner, tempSync.folder.name);
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	} else if (error) {
		error_print("userThread: failure. Error in DBConnection_getFolderToSyncID()\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	error = DBConnection_getSynchronizationID(dbConn, tempSync,
			&synchronizationID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print("userThread: failure. Error sync with %s on folder (owner: %s, folder: %s) not found\n",
				tempSync.node.EID, tempSync.folder.owner,
				tempSync.folder.name);
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	} else if (error) {
		error_print(
				"userThread: failure. Error in DBConnection_getSynchronizationID()\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	error = DBConnection_getSynchronizationStatus(dbConn, tempSync,
			&(tempSync.state));
	if (error) {
		error_print(
				"userThread: failure. Error in DBConnection_getSynchronizationStatus()\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	error = DBConnection_getSynchronizationMode(dbConn, tempSync,
			&(tempSync.mode));
	if (error) {
		error_print(
				"userThread: failure. Error in DBConnection_getSynchronizationMode()\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if (tempSync.state == SYNCHRONIZATION_PENDING) {
		error_print("userThread: failure. This synchronization is pending, you cannot send the checkUpdate command\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}
	if (tempSync.mode == PUSH
			&& (strcmp(what, _FILE) == 0 || strcmp(what, _ALL) == 0)) {
		error_print("userThread: failure. Your synchronization is in PUSH mode, you cannot ask files\n");
		fileToSyncList_destroy(files);
		pthread_mutex_unlock(generalMutex);
		return ERROR_VALUE;
	}

	if (strcmp(what, _FILE) != 0) {
		//only if what != FILE, list is populated with all files in out DB
		error = DBConnection_getFilesToSyncFromFolder(dbConn, &files,
				folderToSyncID);
		if (error) {
			error_print("userThread: failure. Error in DBConnection_getFilesToSyncFromFolder()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
	} else {
		//fill the list
		for (toScan = files; toScan != NULL; toScan = toScan->next) {
			error = DBConnection_isFileOnDb(dbConn, toScan->value,
					folderToSyncID, &result);
			if (error) {
				error_print("userThread: failure. Error in DBConnection_isFileOnDb()\n");				
				pthread_mutex_unlock(generalMutex);
				fileToSyncList_destroy(files);
				return ERROR_VALUE;
			}
			if (result) {
				//can retrieve both information
				error = DBConnection_getFileInfo(dbConn, folderToSyncID,
						&(toScan->value));
				if (error) {
					error_print(
							"userThread: failure. Error in DBConnection_getSyncStatusInfo()\n");
					fileToSyncList_destroy(files);
					pthread_mutex_unlock(generalMutex);
					return ERROR_VALUE;
				}
			} else {
				//allow to ask files not present on the local DB
				toScan->value.deleted = FILE_DELETED;
				toScan->value.lastModified = 0;
			}
		}
	}

//	toScan = files;
//	while (toScan != NULL) {
//		debug_print(DEBUG_OFF, "%s %d %llu\n", toScan->value.name, toScan->value.deleted,
//				toScan->value.lastModified);
//		toScan = toScan->next;
//	}


//--------------------------------------------------------------------------------------------------


	for(toScan = files; toScan != NULL; /*toScan = toScan->next*/){

		sprintf(checkUpdateString, "CheckUpdate\t%llu\t%s\t%s\t%s\t%s", getCurrentTime(), mode, what, tempSync.folder.owner, tempSync.folder.name);
		checkUpdateStringLength = strlen(checkUpdateString) + 2;

		while(toScan != NULL){

			sprintf(currentTripleProcessed, "\t%s\t%d\t%llu", toScan->value.name, toScan->value.deleted, toScan->value.lastModified);
			if(checkUpdateStringLength + strlen(currentTripleProcessed) < UPDATE_COMMAND_TEXT_SIZE_LIMIT){
				//everything fine...
				strcat(checkUpdateString, currentTripleProcessed);
				checkUpdateStringLength += strlen(currentTripleProcessed);
				toScan = toScan->next;
			}//if the string can contains also this file...
			else
				break;
		}

		strcat(checkUpdateString, "\n");

		debug_print(DEBUG_L1, "%s\n", checkUpdateString);
		newCommand(&cmd, checkUpdateString);

		cmd->msg.destination = tempSync.node;
		cmd->msg.source = myNode;
		cmd->msg.txLeft = 1;
		cmd->msg.nextTxTimestamp = getNextRetryDate(tempSync.node);

		error = DBConnection_controlledAddCommand(dbConn, cmd);
		destroyCommand(cmd);
		if (error) {
			error_print(
					"userThread: failure. Error in DBConnection_controlledAddCommand()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
	}//extern loop

	fileToSyncList_destroy(files);

//--------------------------------------------------------------------------------------------------


	pthread_mutex_unlock(generalMutex);
	return SUCCESS_VALUE;
}	//checkUpdateUserOperation()

//print che folders monitored with inotify
static int printWatchListUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex) {

	pthread_mutex_lock(generalMutex);
	watchList_print();
	pthread_mutex_unlock(generalMutex);

	return SUCCESS_VALUE;
}

static int deleteSynchronizationsOnFolderUserOperation(sqlite3 *dbConn,
		pthread_mutex_t *generalMutex, dtnNode myNode) {

	int error;
	int ok;
	char userInput[USER_INPUT_LENGTH];

	folderToSync tempFolder;
	int folderToSyncID;

	synchronization *syncsOnFolder = NULL;
	int numSyncs;

	command *cmd;
	char commandText[FIN_COMMAND_TEXT_SIZE_LIMIT];

	int i;

	getOwnerFromEID(tempFolder.owner, myNode.EID);

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF, "userThread: Enter the folder name: (e.g. photos)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (strlen(userInput) == 0)
			continue;
		else {
			ok = 1;
			strcpy(tempFolder.name, userInput);
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete synchronizations\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	ok = 0;
	while (!ok) {
		debug_print(DEBUG_OFF,
				"Are you sure that you want to delete all the synchronizations on this folder? (YES, upper case)\n");
		if (fgets(userInput, sizeof(userInput), stdin) == NULL)
			break;
		userInput[strlen(userInput) - 1] = '\0';

		if (strlen(userInput) == 0)
			continue;
		else {
			ok = 1;
		}
	}
	if (!ok) {
		debug_print(DEBUG_OFF, "userThread: failure. \"delete synchronizations\" operation aborted\n");
		return SUCCESS_VALUE;
	}

	if (strcmp("YES", userInput) == 0) {

		pthread_mutex_lock(generalMutex);

		error = DBConnection_getFolderToSyncID(dbConn, tempFolder,
				&folderToSyncID);
		if (error) {
			error_print(
					"userThread: failure. Error in DBConnection_getFolderToSyncID()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}

		syncsOnFolder = NULL;
		numSyncs = 0;
		error = DBConnection_getAllSynchronizationsOnFolder(dbConn, tempFolder,
				&syncsOnFolder, &numSyncs);
		if (error) {
			error_print(
					"userThread: failure. deleteSynchronizationsOnFolderUserOperation: error in DBConnection_getAllSynchronizationsOnFolder()\n");
			pthread_mutex_unlock(generalMutex);
			return ERROR_VALUE;
		}
		for (i = 0; i < numSyncs; i++) {

			//TODO 	can I delete a sync with a blocked or blocked-by node? YES, but ...
			//		...with the current implementation,a sync should not exists with this node, because, currenty, a node cannot be moved from BLACKLIST to WHITELIST and viceversa

			//		can I delete a sync with a frozen node? Yes, but the command will be send only when the node inform us that he unfreezed...


			//Per ogni sync creo il comando di FIN
			//mi costruisco il testo del comando
			//Fin timestamp owner folderName fromOwner
			sprintf(commandText, "Fin\t%llu\t%s\t%s\t%d\n", getCurrentTime(),
					syncsOnFolder[i].folder.owner,
					syncsOnFolder[i].folder.name, 1);

			debug_print(DEBUG_L1, "userThread: Command created=%s\n",
					commandText);

			//procedo a creare il comando effettivo
			newCommand(&cmd, commandText);

			//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
			cmd->msg.destination = syncsOnFolder[i].node;
			cmd->msg.source = myNode;
			cmd->msg.txLeft = syncsOnFolder[i].node.numTx;
			cmd->msg.nextTxTimestamp = getNextRetryDate(syncsOnFolder[i].node);

			error = DBConnection_controlledAddCommand(dbConn, cmd);
			destroyCommand(cmd);
			if (error) {
				free(syncsOnFolder);
				error_print(
						"userThread: failure. deleteSynchronizationsOnFolderUserOperation: error in DBConnection_controlledAddCommand()\n");
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}

			//cancello la sincronizzazione per il nodo
			error = DBConnection_deleteSynchronization(dbConn,
					syncsOnFolder[i]);
			if (error) {
				free(syncsOnFolder);
				error_print(
						"userThread: failure. deleteSynchronizationsOnFolderUserOperation: error in DBConnection_deleteSynchronization()\n");				
				pthread_mutex_unlock(generalMutex);
				return ERROR_VALUE;
			}
		}
		if (syncsOnFolder != NULL)
			free(syncsOnFolder);

		pthread_mutex_unlock(generalMutex);
	}			//if delete synchronizations
	else {
		//ci ha ripensato...
	}

	return SUCCESS_VALUE;
}

