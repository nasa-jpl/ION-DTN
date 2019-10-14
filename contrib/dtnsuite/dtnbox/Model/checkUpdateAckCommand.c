  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * checkUpdateAckCommand.c
 */

#include "checkUpdateAckCommand.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileToSync.h"
#include "fileToSyncList.h"
#include "synchronization.h"
#include "../DBInterface/DBInterface.h"
#include "../Controller/debugger.h"
#include "definitions.h"

void checkUpdateAckCommand_createCommand(command** cmd) {

	//CheckUpdate\ttimestamp\t[OFFER/ASK]\t[LIST/ALL/FILE]\towner\tfolder

	checkUpdateAckCommand* this;

	char* cmdText;	//used to scan, after scan it will be null
	char* toFree;

	char* token;
	fileToSync tempFile;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(checkUpdateAckCommand));
	this = (checkUpdateAckCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//CheckUpdateAck timestamp owner folderName response fileName deleted lastModified
	cmdText = strdup(this->base.text);
	toFree = cmdText;

	//brucio il primo token in quanto e' la descrizione del comando
	token = strsep(&cmdText, "\t");

	//il secondo token e' il timestamp
	token = strsep(&cmdText, "\t");
	this->timestamp = strtoull(token, NULL, 0);

	//il terzo token è il campo mode
	token = strsep(&cmdText, "\t");
	strcpy(this->mode, token);

	//il quarto token è il campo what
	token = strsep(&cmdText, "\t");
	strcpy(this->what, token);

	//il quinto token e' l'owner
	token = strsep(&cmdText, "\t");
	strcpy(this->owner, token);

	//il sesto token e' la cartella
	token = strsep(&cmdText, "\t");
	strcpy(this->folderName, token);

	//il settimo token e' la risposta
	token = strsep(&cmdText, "\t");
	strcpy(this->response, token);

	//necessary when the list of files is empty
	if(cmdText == NULL)
		(this->response)[strlen(this->response) - 1] = '\0';


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
void checkUpdateAckCommand_destroyCommand(command* cmd) {
	checkUpdateAckCommand* this;
	this = (checkUpdateAckCommand*) cmd;
	if (this->files != NULL)
		fileToSyncList_destroy(this->files);
}
void checkUpdateAckCommand_sendCommand(command* cmd) {
	cmd->state = CMD_CONFIRMED;
	return;
}
void checkUpdateAckCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {

	int error;
	int sourceID;

	int synchronizationID;
	int folderToSyncID;

	checkUpdateAckCommand* this = (checkUpdateAckCommand*) cmd;

	char checkUpdateText[CHECKUPDATE_COMMAND_TEXT_SIZE_LIMIT];
	char tempString[TEMP_STRING_LENGTH];

	command tmpCommandSource;
	fileToSyncList toDestroy;

	synchronization tmpSync;


	//strcpy(ackCommandText, "");	XXX note: now ackCommandText memory is not allocated!!

	//mi ricostruisco la stringa del comando di CheckUpdate
	strcpy(checkUpdateText, "CheckUpdate");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(checkUpdateText, "\t");
	strcat(checkUpdateText, tempString);
	strcat(checkUpdateText, "\t");
	strcat(checkUpdateText, this->mode);
	strcat(checkUpdateText, "\t");
	strcat(checkUpdateText, this->what);

	//aggiungo l'owner e il nome
	strcat(checkUpdateText, "\t");
	strcat(checkUpdateText, this->owner);
	strcat(checkUpdateText, "\t");
	strcat(checkUpdateText, this->folderName);

	//a questo punto per metto di seguito ogni file da aggiornare
	toDestroy = this->files;
	toDestroy = fileToSyncList_reverse(toDestroy);
	this->files = toDestroy;

	while (toDestroy != NULL) {
		strcat(checkUpdateText, "\t");
		strcat(checkUpdateText, toDestroy->value.name);
		strcat(checkUpdateText, "\t");
		sprintf(tempString, "%d", toDestroy->value.deleted);
		strcat(checkUpdateText, tempString);
		sprintf(tempString, "%llu", toDestroy->value.lastModified);
		strcat(checkUpdateText, "\t");
		strcat(checkUpdateText, tempString);
		toDestroy = toDestroy->next;
	}
	toDestroy = this->files;

	//metto il newline e termino
	strcat(checkUpdateText, "\n");

	//costruisco il comando update che avevo inviato
	tmpCommandSource.state = CMD_CONFIRMED;
	tmpCommandSource.msg.destination = this->base.msg.source;
	tmpCommandSource.text = checkUpdateText;

	//mi costruisco la cartella temporanea
	strcpy(tmpSync.folder.name, this->folderName);
	strcpy(tmpSync.folder.owner, this->owner);
	tmpSync.folder.files = NULL;

	//mi costruisco la sincro
	tmpSync.node = this->base.msg.source;

	error = DBConnection_isCommandOnDb(dbConn, tmpCommandSource, &sourceID);
	if (error) {
		error_print(
				"checkUpdateAckCommand_receiveCommand: error in DBConnection_isCommandOnDb()\n");
		return;
	}

	if (sourceID == 0) {
		//il comando di update non è su DB...
		error_print("checkUpdateAckCommand_receiveCommand: command not found on DB\n");
		return;
	} else
		debug_print(DEBUG_L1,
				"checkUpdateAckCommand_receiveCommand: received ack for the checkUpdateCommand with ID %d\n",
				sourceID);

	error = DBConnection_getFolderToSyncID(dbConn, tmpSync.folder,
			&folderToSyncID);
	if (error) {
		//qui anche DB_DATA_NOT_FOUND_ERROR è un problema...
		//se non ho la folderToSync -> non posso avere neanche la sync...
		error_print(
				"checkUpdateAckCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		return;
	}

	error = DBConnection_getSynchronizationID(dbConn, tmpSync,
			&synchronizationID);
	if (error) {
		//qui anche DB_DATA_NOT_FOUND_ERROR è un problema...
		error_print(
				"checkUpdateAckCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		return;
	}

	//arrivati qui siamo sicuri di avere comando, folder e sync

	//aggiorno lo stato sul database del update che mi ha generato
	error = DBConnection_updateCommandState(dbConn, this->base.msg.source,
			checkUpdateText, CMD_CONFIRMED);
	if (error) {
		//caso mi arriva l'ack di un update che non ho inviato io...
		error_print(
				"checkUpdateAckCommand_receiveCommand: error in DBConnection_updateCommandState()\n");
		return;	//INTERNALERROR
	}

	if (strcmp(this->response, "OK") == 0) {
		//la ricezione dell'ack di un comando di checkUpdate non fa nulla di speciale
		//sarà l'altro nodo che ha creato il comando opportuno
		debug_print(DEBUG_OFF, "checkUpdateAckCommand_receiveCommand: checkUpdate request received and accepted\n");
	} else {
		error_print("checkUpdateAckCommand_receiveCommand: error in check update request, reason: %s\n", this->errorMessage);
		return;
	}

	//sistemo i comandi precedenti in attesa
	error = DBConnection_setPreviusPendingAsFailed(dbConn,
			this->base.msg.source, this->timestamp);
	if (error) {
		error_print(
				"checkUpdateCommand_receiveCommand: error in DBConnection_setPreviusPendingAsFailed()\n");
		//qui non dovrebbe mai fallire... ci possiamo fare poco.
		return;//INTERNALERROR
	}

	return;
}

