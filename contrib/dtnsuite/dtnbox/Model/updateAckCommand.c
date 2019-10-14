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
 * updateAckCommand.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "updateAckCommand.h"
#include "../DBInterface/DBInterface.h"
#include "../Controller/utils.h"
#include "definitions.h"
#include "../Controller/debugger.h"

void updateAckCommand_createCommand(command** cmd) {
	updateAckCommand* this;
	char* cmdText;
	char* toFree;
	char* token;
	fileToSync tempFile;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(updateAckCommand));
	this = (updateAckCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//UpdateAck timestamp owner folderName response fileName deleted lastModified
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

	//il qunto token e' la risposta
	token = strsep(&cmdText, "\t");
	strcpy(this->response, token);

	if (strcmp(this->response, "OK") != 0) {
		//ho un altro token che mi indica l'errore
		token = strsep(&cmdText, "\t");
		strcpy(this->errorMessage, token);
	}

	//ogni riga successiva e' il file
	this->files = fileToSyncList_create();
	while ((token = strsep(&cmdText, "\t")) != NULL) {
		//il primo token letto e' il nome del file
		strcpy(tempFile.name, token);

		//il secondo token e' lo stato deleted
		token = strsep(&cmdText, "\t");
		tempFile.deleted = atoi(token);

		//il terzo token e' il lastModified
		token = strsep(&cmdText, "\t");
		tempFile.lastModified = strtoull(token, NULL, 0);

		//aggiungo alla lista
		this->files = fileToSyncList_add(this->files, tempFile);
	}

	//libero la memoria della stringa
	free(toFree);

	//assegno lo stato desync in quanto e' stato appena creato
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;
}
void updateAckCommand_destroyCommand(command* cmd) {
	updateAckCommand* this;
	this = (updateAckCommand*) cmd;
	if (this->files != NULL)
		fileToSyncList_destroy(this->files);
	else
		debug_print(DEBUG_L1, "updateAckCommand_destroyCommand: this->files should never be null\n");

}
void updateAckCommand_sendCommand(command* cmd) {
	//aggiorno lo stato del comando a CONFIRMED in quanto non c'è l'ack dell'ack
	cmd->state = CMD_CONFIRMED;
	return;
}

void updateAckCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {

	int error;

	int sourceID;//contiene l'ID del DB del comando di update inviato, sourceID == 0 -> problema
	int folderToSyncID;
	int synchronizationID;

	char updateText[UPDATE_COMMAND_TEXT_SIZE_LIMIT];
	char tempString[TEMP_STRING_LENGTH];
	synchronization tmpSync;
	updateAckCommand* this;
	command tmpCommandSource;

	fileToSyncList toDestroy;

	int numSyncStatusOnFile;

	fileToSync myFile;
	int isFileOnDB = 0;

	//Converto il comando
	this = (updateAckCommand*) cmd;

	//azzero il testo dell'ack
	//strcpy(ackCommandText, "");	XXX note: now ackCommandText memory is not allocated!!

	//mi ricostruisco la stringa del comando di update
	strcpy(updateText, "Update");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(updateText, "\t");
	strcat(updateText, tempString);
	//aggiungo l'owner e il nome
	strcat(updateText, "\t");
	strcat(updateText, this->owner);
	strcat(updateText, "\t");
	strcat(updateText, this->folderName);

	//a questo punto per metto di seguito ogni file da aggiornare
	toDestroy = this->files;
	toDestroy = fileToSyncList_reverse(toDestroy);
	this->files = toDestroy;

	while (toDestroy != NULL) {
		strcat(updateText, "\t");
		strcat(updateText, toDestroy->value.name);
		strcat(updateText, "\t");
		sprintf(tempString, "%d", toDestroy->value.deleted);
		strcat(updateText, tempString);
		sprintf(tempString, "%llu", toDestroy->value.lastModified);
		strcat(updateText, "\t");
		strcat(updateText, tempString);
		toDestroy = toDestroy->next;
	}
	toDestroy = this->files;

	//metto il newline e termino
	strcat(updateText, "\n");

	//costruisco il comando update che avevo inviato
	tmpCommandSource.state = CMD_CONFIRMED;
	tmpCommandSource.msg.destination = this->base.msg.source;
	tmpCommandSource.text = updateText;

	//mi costruisco la cartella temporanea
	strcpy(tmpSync.folder.name, this->folderName);
	strcpy(tmpSync.folder.owner, this->owner);
	tmpSync.folder.files = NULL;

	//mi costruisco la sincro
	tmpSync.node = this->base.msg.source;

	error = DBConnection_isCommandOnDb(dbConn, tmpCommandSource, &sourceID);
	if (error) {
		error_print(
				"updateAckCommand_receiveCommand: error in DBConnection_isCommandOnDb()\n");
		return;
	}

	if (sourceID == 0) {
		error_print(
				"updateAckCommand_receiveCommand: command not found on DB)\n");
		return;
	} else
		debug_print(DEBUG_L1,
				"updateAckCommand_receiveCommand: received ack for the updateCommand with ID %d\n",
				sourceID);

	error = DBConnection_getFolderToSyncID(dbConn, tmpSync.folder,
			&folderToSyncID);
	if (error) {
		//qui anche DB_DATA_NOT_FOUND_ERROR è un problema...
		//se non ho la folderToSync -> non posso avere neanche la sync...
		error_print(
				"updateAckCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		return;
	}

	error = DBConnection_getSynchronizationID(dbConn, tmpSync,
			&synchronizationID);
	if (error) {
		//qui anche DB_DATA_NOT_FOUND_ERROR è un problema...
		error_print(
				"updateAckCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		return;
	}

	//arrivati qui siamo sicuri di avere comando, folder e sync

	//aggiorno lo stato sul database del update che mi ha generato
	error = DBConnection_updateCommandState(dbConn, this->base.msg.source,
			updateText, CMD_CONFIRMED);
	if (error) {
		//caso mi arriva l'ack di un update che non ho inviato io...
		error_print(
				"updateAckCommand_receiveCommand: error in DBConnection_updateCommandState()\n");
		return;	//INTERNALERROR
	}

	//se OK o INCOMPLETE -> possiamo elaborare qualcosa...
	if (strcmp(this->response, "OK") == 0
			|| startsWith(this->response, "INCOMPLETE")) {

		int i = 0;
		int outcomeUpdate = strcmp(this->response, "OK") == 0 ? OK : INCOMPLETE;
		char *outcomeString = NULL;

		//remember ERROR\tINCOMPLETE010101111
		if (outcomeUpdate == INCOMPLETE)
			outcomeString = &((this->response)[strlen("INCOMPLETE")]);

		while (toDestroy != NULL) {

			if (outcomeUpdate == OK
					|| (outcomeUpdate == INCOMPLETE && outcomeString[i++] == '0')) {
				//solo se l'aggiornamento (sul file corrente) in remoto è andato a buon fine possiamo confermare anche in locale

				memset(&myFile, 0, sizeof(fileToSync));
				strcpy(myFile.name, toDestroy->value.name);

				error = DBConnection_isFileOnDb(dbConn, myFile, folderToSyncID,
						&isFileOnDB);
				if (error) {
					error_print(
							"updateAckCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
					toDestroy = toDestroy->next;
					continue;
				}
				if (!isFileOnDB) {
					error_print(
							"updateAckCommand_receiveCommand: file \"%s\" not found on DB\n",
							myFile.name);
					toDestroy = toDestroy->next;
					continue;
				}
				//else, isFileOnDB -> retrieve data (deleted and lastMod
				error = DBConnection_getFileInfo(dbConn, folderToSyncID,
						&myFile);
				if (error) {
					error_print(
							"updateAckCommand_receiveCommand: error in DBConnection_getFileInfo()\n");
					toDestroy = toDestroy->next;
					continue;
				}

				if (toDestroy->value.deleted == myFile.deleted
						&& toDestroy->value.lastModified
								== myFile.lastModified) {
					//solo se l'ack è proprio sull'ultimo aggiornamento lo confermo!!

					//elaboro
					if (toDestroy->value.deleted) {

						//il file e' stato cancellato, tolgo la sua sincro
						error = DBConnection_deleteSyncOnFile(dbConn, tmpSync,
								toDestroy->value, folderToSyncID);
						if (error) {
							error_print(
									"updateAckCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
							toDestroy = toDestroy->next;
							continue;
						}

						//se il file è stato eliminato e non ci sono sync su quel file allora elimino il file dal DB
						numSyncStatusOnFile = -1;
						error = DBConnection_getNumSyncStatusOnFiles(dbConn,
								folderToSyncID, toDestroy->value.name,
								&numSyncStatusOnFile);
						if (error) {
							error_print(
									"updateAckCommand_receiveCommand: error in DBConnection_getNumSyncStatusOnFiles()\n");
							toDestroy = toDestroy->next;
							continue;
						}

						if (numSyncStatusOnFile == 0) {
							error = DBConnection_deleteFileFromDB(dbConn,
									folderToSyncID, toDestroy->value.name);
							if (error) {
								error_print(
										"updateAckCommand_receiveCommand: error in DBConnection_deleteFileFromDB()\n");
								toDestroy = toDestroy->next;
								continue;
							}
						}
					} else {
						//aggiorno il suo stato
						error = DBConnection_updateSyncOnFileState(dbConn,
								tmpSync, toDestroy->value, folderToSyncID,
								FILE_SYNCHRONIZED);
						if (error) {
							error_print(
									"updateCommand_receiveCommand: error in DBConnection_updateSyncOnFileState()\n");
							toDestroy = toDestroy->next;
							continue;
						}
					}

				}//if the ack is about the last update (same informations on file!!)
			} //if update on file OK
			toDestroy = toDestroy->next;
		}
	} //if OK || INCOMPLETE

	//sistemo i comandi precedenti in attesa
	error = DBConnection_setPreviusPendingAsFailed(dbConn,
			this->base.msg.source, this->timestamp);
	if (error) {
		//should never happen
		error_print(
				"updateCommand_receiveCommand: error in DBConnection_setPreviusPendingAsFailed()\n");
		return;	//INTERNALERROR
	}

	return;
}

