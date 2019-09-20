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
 * finCommand.c
 *
 * modificato tutto per l'implementazione della nuova sync multilaterale
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../DBInterface/DBInterface.h"
#include "finCommand.h"
#include "synchronization.h"
#include "../Controller/utils.h"
#include "watchList.h"
#include "../Controller/debugger.h"

#include "definitions.h"

static void finCommand_buildAckCommand(finCommand* this, char *ackCommandText,
		int outcome);

void finCommand_createCommand(command** cmd) {
	finCommand* this;
	char* cmdText;
	char* toFree;

	char* token;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(finCommand));
	this = (finCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//Fin timestamp owner folderName fromOwner
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

	//il quinto token e' se proviene dall'owner
	token = strsep(&cmdText, "\t");
	this->fromOwner = atoi(token);

	//libero la memoria della stringa
	free(toFree);

	//assegno lo stato desync in quanto e' stato appena creato
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;
}
void finCommand_destroyCommand(command* cmd) {
	//deallocare solo i campi specifici allocati con malloc() nella createCommand()
	//in questo caso vuota
}

void finCommand_sendCommand(command* cmd) {
	cmd->state = CMD_PENDING;
	return;
}
void finCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {
	int error;

	int synchronizationID;
	int folderToSyncID;

	synchronization sync;

	SyncMode mode;

	finCommand* this;

	synchronization* syncsOnFolder;
	int numSync;

	int i;
	char testoComando[FIN_COMMAND_TEXT_SIZE_LIMIT];
	command *finSyncCmd;
	char dirName[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	char systemCommand[SYSTEM_COMMAND_LENGTH + FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH]; //rm -rf command

	char myOwnerFromEID[OWNER_LENGTH];	//il nome del nostro nodo a partire dal suo EID

	//Converto il comando
	this = (finCommand*) cmd;

	debug_print(DEBUG_OFF,
			"finCommand_receiveCommand: received and accepted FIN command\n");

	//Ricostruisco la sincro a cui sto facendo riferimento
	sync.folder = getEmptyFileListFolderFromOwnerAndFolder(this->owner,
			this->folderName);

	sync.node = this->base.msg.source;

	getOwnerFromEID(myOwnerFromEID, this->base.msg.destination.EID);

	getAbsolutePathFromOwnerAndFolder(dirName, this->owner, this->folderName);

	//no folder -> no sync

	error = DBConnection_getFolderToSyncID(dbConn, sync.folder,
			&folderToSyncID);
	if (error) {
		error_print(
				"finCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		finCommand_buildAckCommand(this, ackCommandText,
				error == DB_DATA_NOT_FOUND_ERROR ? NOFOLDER : INTERNALERROR);
		return;
	}

	error = DBConnection_getSynchronizationID(dbConn, sync, &synchronizationID);
	if (error) {
		error_print(
				"finCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		finCommand_buildAckCommand(this, ackCommandText,
				error == DB_DATA_NOT_FOUND_ERROR ? NOSYNC : INTERNALERROR);
		return;
	}

	error = DBConnection_getSynchronizationMode(dbConn, sync, &mode);
	if (error) {
		error_print(
				"finCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
		return;
	}


	if (mode == PULL || mode == PUSH_AND_PULL_IN) {
		//fine su sync di cartella di input (non sono owner, per forza)
		//send the fin to all the syncs
		numSync = 0;
		syncsOnFolder = NULL;
		error = DBConnection_getAllSynchronizationsOnFolder(dbConn, sync.folder,
				&syncsOnFolder, &numSync);
		if (error) {
			error_print(
					"finCommand_receiveCommand: error in DBConnection_getAllSynchronizationsOnFolder()\n");
			finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
			//finCommand_errorHandler();
			return;//INTERNALERROR
		}
		for (i = 0; i < numSync; i++) {
			//Per ogni sync creo il comando di FIN
			//mi costruisco il testo del comando
			//Fin timestamp owner folderName fromOwner
			if (strcmp(syncsOnFolder[i].node.EID, cmd->msg.source.EID) != 0) {
				sprintf(testoComando, "Fin\t%llu\t%s\t%s\t%d\n",
						getCurrentTime(), syncsOnFolder[i].folder.owner,
						syncsOnFolder[i].folder.name, this->fromOwner);

				//procedo a creare il comando effettivo
				newCommand(&finSyncCmd, testoComando);

				//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
				finSyncCmd->msg.destination = syncsOnFolder[i].node;
				finSyncCmd->msg.source = this->base.msg.destination;
				finSyncCmd->msg.txLeft = syncsOnFolder[i].node.numTx;
				finSyncCmd->msg.nextTxTimestamp = getNextRetryDate(
						syncsOnFolder[i].node);

				error = DBConnection_controlledAddCommand(dbConn, finSyncCmd);
				destroyCommand(finSyncCmd);
				if (error) {
					error_print(
							"finCommand_receiveCommand: error in DBConnection_controlledAddCommand()\n");
					free(syncsOnFolder);
					finCommand_buildAckCommand(this, ackCommandText,
					INTERNALERROR);
					//finCommand_errorHandler();
					return;//INTERNALERROR
				}
			}
			//cancello la sincronizzazione per il nodo
			error = DBConnection_deleteSynchronization(dbConn,
					syncsOnFolder[i]);
			if (error) {
				error_print(
						"finCommand_receiveCommand: error in DBConnection_deleteSynchronization()\n");
				free(syncsOnFolder);
				finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
				//finCommand_errorHandler();
				return;//INTERNALERROR... non può essere altro...
			}
		}
		if (syncsOnFolder != NULL)
			free(syncsOnFolder);

		//cancello la cartella e i file dal database
		error = DBConnection_deleteFolder(dbConn, sync.folder);
		if (error) {
			error_print("finCommand_receiveCommand: error in DBConnection_deleteFolder()\n");
			finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
			//finCommand_errorHandler();
			//se sono qua la folder deve esserci x forza...
			return;//INTERNALERROR
		}

		sprintf(systemCommand, "rm -rf %s", dirName);

		//delete paths from watchList
		if (mode == PUSH_AND_PULL_IN) {
			if (watchList_containsFolder(dirName)) {
				error = recursive_remove(dirName);
				if (error) {
					error_print("finCommand_receiveCommand: error in recursive_remove()\n");
					finCommand_buildAckCommand(this, ackCommandText,
					INTERNALERROR);
					return;
				}
			}
		}

		//delete folder from filesystem
		error = system(systemCommand);
		if (error) {
			error_print("finCommand_receiveCommand: error in system(%s)\n", systemCommand);
			finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
			return;//INTERNALERROR
		}

		//if empty, delete also the owner folder
		getFoldersToSyncPath(dirName);
		strcat(dirName, sync.folder.owner);
		if(isDirectoryEmpty(dirName)){
			sprintf(systemCommand, "rm -rf %s", dirName);
			error = system(systemCommand);
			if (error) {
				error_print("finCommand_receiveCommand: error in system(%s)\n", systemCommand);
				finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
				return;//INTERNALERROR
			}
		}
	} else {
		//delete sync, not the main sync with the parent
		error = DBConnection_deleteSynchronization(dbConn, sync);
		if (error) {
			error_print(
					"finCommand_receiveCommand: error in DBConnection_deleteSynchronization()\n");

			finCommand_buildAckCommand(this, ackCommandText,
					error == DB_DATA_NOT_FOUND_ERROR ? NOSYNC : INTERNALERROR);
			return;	//INTERNALERROR sempre siccome ho già controllato sopra che folder e sync ci siano...
		}

		if (strcmp(myOwnerFromEID, sync.folder.owner) == 0) {
			//i am the owner

			//retrieve from DB the number of sync where sync != PULL ( && to add sync != PUSH_AND_PULL_INPUT...)
			error = DBConnection_getNumSynchronizationsOnFolder(dbConn,
					sync.folder, &numSync);
			if (error) {
				error_print(
						"finCommand_receiveCommand: error in DBConnection_getNumSynchronizationsOnFolder()\n");
				finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
				return;
			}

			//if noone is interested to our folder, we can remove it from DB
			if (numSync == 0 && watchList_containsFolder(dirName)) {
				error = recursive_remove(dirName);
				if (error) {
					error_print("finCommand_receiveCommand: error in recursive_remove()\n");
					finCommand_buildAckCommand(this, ackCommandText,
					INTERNALERROR);
					return;
				}
				error = DBConnection_deleteFilesForFolder(dbConn, folderToSyncID);
				if (error) {
					error_print(
							"finCommand_receiveCommand: error in DBConnection_deleteFilesForFolder()\n");
					finCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
					return;
				}
			}
		}
	}

//a questo punto invio il comando di ack, aggiungo l'ok in questo caso
	finCommand_buildAckCommand(this, ackCommandText, OK);
	return; //OK
}

static void finCommand_buildAckCommand(finCommand* this, char *ackCommandText,
		int outcome) {

	char tempString[TEMP_STRING_LENGTH];

//azzero il testo dell'ack
	strcpy(ackCommandText, "");

//Mi preparo la stringa per l'ack
//FinAck timestamp owner folderName fromOwner response
	strcpy(ackCommandText, "FinAck");
//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);
//owner e folderName
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->owner);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->folderName);
//fromOwner
	sprintf(tempString, "%d", this->fromOwner);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);

	switch (outcome) {

	case (OK): {
		strcat(ackCommandText, "\tOK\n");
		break;
	}
	case (INTERNALERROR): {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR\n");
		break;
	}
	case (NOFOLDER): {
		strcat(ackCommandText, "\tERROR\tNOFOLDER\n");
		break;
	}
	case (NOSYNC): {
		strcat(ackCommandText, "\tERROR\tNOSYNC\n");
		break;
	}
	default: {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR\n");
		break;
	}

	} //switch(outcome)

	return;
}
