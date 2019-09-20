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
 * syncAckCommand.c
 * MB: modificato tutto per l'implementazione della nuova sync multilaterale
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syncAckCommand.h"
#include "../DBInterface/DBInterface.h"
#include "../Controller/utils.h"
#include "watchList.h"
#include "../Controller/align.h"
#include "definitions.h"
#include "../Controller/debugger.h"
#include "../Controller/monitorAndSendThread.h"

void syncAckCommand_createCommand(command** cmd) {
	syncAckCommand* this;
	char* cmdText;
	char *toFree;
	char* token;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(syncAckCommand));
	this = (syncAckCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//SyncAck timestamp owner folderName mode nochat pwdRead pwdWrite resp
	cmdText = strdup(this->base.text);
	toFree = cmdText;

	//brucio il primo token in quanto e' la descrizione del comando
	token = strsep(&cmdText, "\t");

	//il secondo token e' il timestamp
	token = strsep(&cmdText, "\t");
	this->timestamp = strtoull(token, NULL, 0);

	//il terzo token e' l'owner
	token = strsep(&cmdText, "\t");
	strcpy(this->owner, token);

	//il quarto token e' la cartella
	token = strsep(&cmdText, "\t");
	strcpy(this->folderName, token);

	//il quinto token e' il modo
	token = strsep(&cmdText, "\t");
	this->mode = atoi(token);

	//il sesto token e' il chat
	token = strsep(&cmdText, "\t");
	this->nochat = atoi(token);

	//lifetime
	token = strsep(&cmdText, "\t");
	this->lifetime = atoi(token);

	//il settimo token e' la pwdRead
	token = strsep(&cmdText, "\t");
	strcpy(this->pwdRead, token);

	//l'ottavo token e' la pwdWrite
	token = strsep(&cmdText, "\t");
	strcpy(this->pwdWrite, token);

	//il nono token e' la risposta
	token = strsep(&cmdText, "\t");
	strcpy(this->response, token);

	if (strcmp(this->response, "OK\n") != 0) {
		//ho anche il messaggio di errore
		token = strsep(&cmdText, "\t");
		//tolgo il newline
		stripNewline(token);
		strcpy(this->errorMessage, token);

	} else {

		//era un ok tolgo il newline
		stripNewline(this->response);
	}

	//libero la memoria della stringa
	free(toFree);

	//assegno lo stato desync in quanto e' stato appena creato
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;
}

void syncAckCommand_destroyCommand(command* cmd) {
	//dovrei fare la free delle strutture allocate nella new command ma non serve per questo comando
}

void syncAckCommand_sendCommand(command* cmd) {
	//aggiorno lo stato del comando a CONFIRMED in quanto non c'è l'ack dell'ack
	cmd->state = CMD_CONFIRMED;
	return;
}

void syncAckCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {

	//convert the command
	syncAckCommand* this = (syncAckCommand*) cmd;

	int error;

	int folderToSyncID;
	int synchronizationID;

	char syncText[SYNC_COMMAND_TEXT_SIZE_LIMIT];
	char tempString[TEMP_STRING_LENGTH];
	char dirName[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	synchronization sync;

	char myOwnerFromEID[OWNER_LENGTH];
	char sourceOwnerFromEID[OWNER_LENGTH];

//	int folderAddedToWatchList = 0;

	//azzero il testo dell'ack
	//strcpy(ackCommandText, "");	XXX note: now ackCommandText memory is not allocated!!

	//ho ricevuto l'ack, aggiorno lo stato sul database del comando sync
	//Sync timestamp owner folderName mode nochat pwdRead pwdWrite
	//genero il comando di sync
	strcpy(syncText, "Sync");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(syncText, "\t");
	strcat(syncText, tempString);
	//owner e folderName
	strcat(syncText, "\t");
	strcat(syncText, this->owner);
	strcat(syncText, "\t");
	strcat(syncText, this->folderName);
	//inserisco il mode
	sprintf(tempString, "%d", this->mode);
	strcat(syncText, "\t");
	strcat(syncText, tempString);
	//metto il no chat
	sprintf(tempString, "%d", this->nochat);
	strcat(syncText, "\t");
	strcat(syncText, tempString);
	//lifetime
	sprintf(tempString, "%d", this->lifetime);
	strcat(syncText, "\t");
	strcat(syncText, tempString);
	//inserisco pass di lettura e scrittura
	strcat(syncText, "\t");
	strcat(syncText, this->pwdRead);
	strcat(syncText, "\t");
	strcat(syncText, this->pwdWrite);
	strcat(syncText, "\n");

	getOwnerFromEID(myOwnerFromEID, this->base.msg.destination.EID);
	getOwnerFromEID(sourceOwnerFromEID, this->base.msg.source.EID);

	getAbsolutePathFromOwnerAndFolder(dirName, sync.folder.owner,
			sync.folder.name);

	sync.folder = getEmptyFileListFolderFromOwnerAndFolder(this->owner,
			this->folderName);
	sync.folder.files = fileToSyncList_create();
	sync.node = this->base.msg.source;
	sync.mode = this->mode;

	strcpy(sync.pwdRead, this->pwdRead);
	strcpy(sync.pwdWrite, this->pwdWrite);

	//aggiorno lo stato sul database del sync che mi ha generato
	error = DBConnection_updateCommandState(dbConn, this->base.msg.source,
			syncText, CMD_CONFIRMED);
	if (error) {
		error_print(
				"syncAckCommand_receiveCommand: error in DBConnection_updateCommandState()\n");
		return;	//NOSYNCREQUESTED
	}

	//retrieve synchronization informations from database
	//TODO handle problems
	error = DBConnection_getDtnNodeFromEID(dbConn, &(sync.node),
			this->base.msg.source.EID);
	if (error) {
		error_print(
				"syncAckCommand_receiveCommand: error in DBConnection_getDtnNodeFromEID()\n");
		return;
	}

	error = DBConnection_getFolderToSyncID(dbConn, sync.folder,
			&folderToSyncID);
	if (error) {
		//DB_DATA_NOT_FOUND is an ERROR, we must have the folder on DB
		error_print(
				"syncAckCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		return;
	}

	error = DBConnection_getSynchronizationID(dbConn, sync, &synchronizationID);
	if (error) {
		//DB_DATA_NOT_FOUND is an ERROR!
		//here we should have the sync... in PENDING state...
		//send FIN
		error_print(
				"syncAckCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		return;
	}

	error = DBConnection_getSynchronizationStatus(dbConn, sync, &sync.state);
	if (error) {
		error_print(
				"syncAckCommand_receiveCommand: error in DBConnection_getSynchronizationStatus()\n");
		return;
	}
	if (sync.state == SYNCHRONIZATION_CONFIRMED) {
		//dovrebbe essere pending!!
		//ad esempio se ci arriva l'ack due volte...
	}

	//if PUSH || PUSH&PULL_OUT, we must have have the folder on FS and DB
	//else if PULL || PUSH&PULL_IN we mustn't have the folder on FS... if present, remove it!!

	//folderToSyncID && syncrhonizationID must be != 0, check addSyncUserOperation!!!

	if (strcmp(this->response, "OK") == 0) {

		debug_print(DEBUG_OFF, "syncAckCommand_receiveCommand: sync request has been accepted\n");

		error = DBConnection_updateSynchronizationStatus(dbConn, sync,
				SYNCHRONIZATION_CONFIRMED);
		if (error) {
			error_print(
					"syncCommand_receiveCommand: error in DBConnection_updateSynchronizationStatus()\n");
			return; //INTERNALERROR
		}

		//if (!fileExists(dirName)) {
		//better...
		if (this->mode == PUSH_AND_PULL_IN || this->mode == PULL) {

			//if the folder exists, we should delete it...
			getHomeDir(dirName);
			strcat(dirName, DTNBOX_FOLDERSTOSYNCFOLDER);

			strcat(dirName, sync.folder.owner);
			strcat(dirName, "/");

			error = checkAndCreateFolder(dirName);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in checkAndCreateFolder()\n");
				return; //INTERNALERROR
			}

			strcat(dirName, sync.folder.name);
			strcat(dirName, "/");

			error = checkAndCreateFolder(dirName);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in checkAndCreateFolder()\n");
				return; //INTERNALERROR
			}
		}

		getAbsolutePathFromOwnerAndFolder(dirName, sync.folder.owner,
				sync.folder.name);

		//se sono owner e non c'è su watchList la aggiungo e allineo, altrimenti
//		if ((strcmp(myOwnerFromEID, sync.folder.owner) == 0
//				&& !watchList_containsFolder(dirName))
//				|| sync.mode == PUSH_AND_PULL_IN) {
//			//I sent a request about my folder
//			if (!watchList_containsFolder(dirName)) {
//				error = recursive_add(dirName);
//				if (error) {
//					error_print(
//							"syncCommand_receiveCommand: error in recursive_add()\n");
//					return; //INTERNALERROR
//				}
//			}
//			folderAddedToWatchList = 1;
//		}

		if ((strcmp(myOwnerFromEID, sync.folder.owner) == 0 && !watchList_containsFolder(dirName)) || sync.mode == PUSH_AND_PULL_IN) {

			//sono owner
			if (!watchList_containsFolder(dirName) ||  sync.mode == PUSH_AND_PULL_IN) {
				//alignFSandDB (static scan of the content of the sync folder)
				error = alignFSandDB(dbConn, sync.folder, dirName);
				if (error) {
					error_print(
							"syncCommand_receiveCommand: error in updateFilesFromFS()\n");
					return; //INTERNALERROR
				}
			} else {
				error = DBConnection_addSyncsOnFileForSynchronization(dbConn,
						sync);
				if (error) {
					error_print(
							"syncCommand_receiveCommand: error in DBConnection_addSyncsOnFileForSynchronization()\n");
					return; //INTERNALERROR
				}
			}


		} else if (sync.mode == PUSH_AND_PULL_OUT || sync.mode == PUSH) {
			//non sono owner e ho proposto la cartella a un altro
			error = DBConnection_addSyncsOnFileForSynchronization(dbConn, sync);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in DBConnection_addSyncsOnFileForSynchronization()\n");
				return; //INTERNALERROR
			}
		}
	} //if OK
	else if (strcmp(this->response, "ERROR") == 0) {

		debug_print(DEBUG_OFF,
				"syncAckCommand_receiveCommand: request has been refused, reason: %s\n",
				this->errorMessage);

		error = DBConnection_deleteSynchronization(dbConn, sync);
		if (error) {
			error_print(
					"syncAckCommand_receiveCommand: error in DBConnection_deleteSynchronization()\n");
			return;
		}
		if (this->mode == PULL || this->mode == PUSH_AND_PULL_IN) {
			error = DBConnection_deleteFolder(dbConn, sync.folder);
			if (error) {
				error_print(
						"syncAckCommand_receiveCommand: error in DBConnection_deleteFolder()\n");
				return;
			}
		}

		//la richiesta non e' andata a buon fine, se sono stato bloccato mi aggiungo in blockedByList
		if (strcmp(this->errorMessage, "BLACKLISTED") == 0) {
			//mi hanno blacklistato, aggiungo il nodo sorgente alla blockedByList
			error = DBConnection_updateDTNnodeBlockedBy(dbConn, this->base.msg.source, BLOCKEDBY);
			if(error){
				error_print("syncAckCommand_receiveCommand: error in DBConnection_updateDTNnodeBlockedBy()\n");
				return; //INTERNALERROR
			}
		}
	} //if error

	//sistemo i comandi precedenti in attesa
	error = DBConnection_setPreviusPendingAsFailed(dbConn,
			this->base.msg.source, this->timestamp);
	if (error) {
		error_print(
				"syncAckCommand_receiveCommand: error in DBConnection_setPreviusPendingAsFailed()\n");
		return; //INTERNALERROR
	}

	return;
}
