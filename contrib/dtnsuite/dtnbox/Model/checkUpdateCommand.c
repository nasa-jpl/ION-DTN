  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * checkUpdateCommand.c
 */

#include "checkUpdateCommand.h"
#include "definitions.h"
#include "../DBInterface/DBInterface.h"
#include "../Controller/utils.h"
#include "../Controller/debugger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static void checkUpdateCommand_buildAckCommand(checkUpdateCommand* this,
		char *ackCommandText, int outcome, fileToSyncList fileList);

void checkUpdateCommand_createCommand(command** cmd) {

	//CheckUpdate\ttimestamp\t[OFFER/ASK]\t[LIST/ALL/FILE]\towner\tfolder

	checkUpdateCommand* this;

	char *cmdText;
	char *toFree;

	char *token;
	fileToSync tempFile;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(checkUpdateCommand));
	this = (checkUpdateCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//CheckUpdate\timestamp\t[OFFER/ASK]\t[LIST/ALL/FILE]\towner\tfolder
	cmdText = strdup(this->base.text);
	toFree = cmdText;

	//brucio il primo token in quanto e' la descrizione del comando
	token = strsep(&cmdText, "\t");

	//il secondo token e' il timestamp
	token = strsep(&cmdText, "\t");
	this->timestamp = strtoull(token, NULL, 0);

	//i terzo token è la modalità [ASK/OFFER]
	token = strsep(&cmdText, "\t");
	strcpy(this->mode, token);

	//il quarto token è il contenuto del comando [LIST/ALL/FILE]
	token = strsep(&cmdText, "\t");
	strcpy(this->what, token);

	//il quinto token è l'owner
	token = strsep(&cmdText, "\t");
	strcpy(this->owner, token);

	//il sesto token è la cartella
	token = strsep(&cmdText, "\t");
	strcpy(this->folderName, token);

	//necessary when the list of files sent by the sender is empty
	if(cmdText == NULL)
		(this->folderName)[strlen(this->folderName) -1] = '\0';

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
	}	//while read files from cmdText

	free(toFree);
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;

	return;
}	//checkUpdateCommand_createCommand()

void checkUpdateCommand_destroyCommand(command* cmd) {

	checkUpdateCommand *this;
	this = (checkUpdateCommand*) cmd;
	if (this->files != NULL)
		fileToSyncList_destroy(this->files);
	return;
}
void checkUpdateCommand_sendCommand(command* cmd) {
	cmd->state = CMD_PENDING;
	return;
}

void checkUpdateCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {

	checkUpdateCommand *this = (checkUpdateCommand*) cmd;

	int error;

	synchronization tmpSync;
	fileToSyncList toScan;	//pointer to the list of its files

	int folderToSyncID;
	int synchronizationID;

	int result;

	command *cmdToSend;


	fileToSyncList myFiles;
	fileToSyncList myFilesToScan;

	fileToSync myFile;

	char checkUpdateString[CHECKUPDATE_COMMAND_TEXT_SIZE_LIMIT];
	int checkUpdateStringLength = 0;
	char currentTripleProcessed[FILETOSYNC_LENGTH + 64/*sizeof(unsigned long long) + sizeof(int) + 4*/];


	int txLeft;
	unsigned long long nextTxTimestamp;

	debug_print(DEBUG_OFF,
			"checkUpdateCommand_receiveCommand: received checkUpdateCommand\n");
	debug_print(DEBUG_L1, "checkUpdateCommand_receiveCommand: %s\n",
			this->base.text);

	this->files = fileToSyncList_reverse(this->files);
	toScan = this->files;

	//mi costruisco la cartella temporanea
	tmpSync.folder = getEmptyFileListFolderFromOwnerAndFolder(this->owner,
			this->folderName);
	tmpSync.folder.files = this->files;

	//mi costruisco la sincro
	//tmpSync.node = this->base.msg.source;

	error = DBConnection_getDtnNodeFromEID(dbConn, &(tmpSync.node),
			this->base.msg.source.EID);
	if (error) {
		error_print(
				"updateCommand_receiveCommand: error in DBConnection_getDtnNodeFromEID()\n");
		checkUpdateCommand_buildAckCommand(this, ackCommandText, INTERNALERROR,
				toScan);
		return;
	}

	//mi ricavo il suo id
	error = DBConnection_getFolderToSyncID(dbConn, tmpSync.folder,
			&folderToSyncID);
	if (error) {
		error_print(
				"updateCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		error = (error == DB_DATA_NOT_FOUND_ERROR) ? NOFOLDER : INTERNALERROR;
		checkUpdateCommand_buildAckCommand(this, ackCommandText, error, toScan);
		return;
	}

	error = DBConnection_getSynchronizationID(dbConn, tmpSync,
			&synchronizationID);
	if (error) {
		error_print(
				"updateCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		error = (error == DB_DATA_NOT_FOUND_ERROR) ? NOSYNC : INTERNALERROR;
		checkUpdateCommand_buildAckCommand(this, ackCommandText, error, toScan);
		return;
	}

	if (strcmp(this->mode, OFFER) == 0 && strcmp(this->what, _LIST) == 0) {
		//someone offered its own list of files
		debug_print(DEBUG_OFF, "\n\nRicevuta lista di file: \n");
		while (toScan != NULL) {
			debug_print(DEBUG_OFF, "%s %llu %d\n", toScan->value.name,
					toScan->value.lastModified, toScan->value.deleted);
			toScan = toScan->next;
		}
		debug_print(DEBUG_OFF, "\n\n");
		toScan = this->files;
		checkUpdateCommand_buildAckCommand(this, ackCommandText, OK, toScan);
		return;
	}	//if OFFER LIST

	else if (strcmp(this->mode, ASK) == 0 && strcmp(this->what, _LIST) == 0) {

		//don't care about its list, it only asked my list...
		//we create a checkUpate OFFER LIST command
		myFiles = fileToSyncList_create();
		error = DBConnection_getFilesToSyncFromFolder(dbConn, &myFiles,
				folderToSyncID);
		if (error) {
			error_print(
					"checkUpdateCommand_receiveCommand: error in DBConnection_getFilesToSyncFromFolder()\n");
			checkUpdateCommand_buildAckCommand(this, ackCommandText,
					INTERNALERROR, toScan);
			return;
		}

//		myFilesToScan = myFiles;

		for(myFilesToScan = myFiles; myFilesToScan != NULL; /*myFilesToScan = myFilesToScan->next*/){

			sprintf(checkUpdateString, "CheckUpdate\t%llu\t%s\t%s\t%s\t%s", getCurrentTime(), OFFER, _LIST, tmpSync.folder.owner, tmpSync.folder.name);
			checkUpdateStringLength = strlen(checkUpdateString) + 2;

			while(myFilesToScan != NULL){

				sprintf(currentTripleProcessed, "\t%s\t%d\t%llu", myFilesToScan->value.name, myFilesToScan->value.deleted, myFilesToScan->value.lastModified);
				if(checkUpdateStringLength + strlen(currentTripleProcessed) < UPDATE_COMMAND_TEXT_SIZE_LIMIT){
					//everything fine...
					strcat(checkUpdateString, currentTripleProcessed);
					checkUpdateStringLength += strlen(currentTripleProcessed);
					myFilesToScan = myFilesToScan->next;
				}//if the string can contains also this file...
				else
					break;



			}
			strcat(checkUpdateString, "\n");

			newCommand(&cmdToSend, checkUpdateString);
			cmdToSend->msg.destination = this->base.msg.source;
			cmdToSend->msg.source = this->base.msg.destination;
			cmdToSend->msg.nextTxTimestamp = getNextRetryDate(this->base.msg.source);
			cmdToSend->msg.txLeft = 1;

			error = DBConnection_controlledAddCommand(dbConn, cmdToSend);
			destroyCommand(cmdToSend);
			if (error) {
				error_print(
						"checkUpdateCommand_receiveCommand: error in DBConnection_controlledAddCommand()\n");
				checkUpdateCommand_buildAckCommand(this, ackCommandText,
						INTERNALERROR, toScan);
				return;
			}

		}//external for
		fileToSyncList_destroy(myFiles);


		checkUpdateCommand_buildAckCommand(this, ackCommandText, OK, toScan);
		return;

	} else if (strcmp(this->mode, ASK) == 0 && strcmp(this->what, _ALL) == 0) {

		//he asked us to check all the files

		//retrieve our file from DB
		error = DBConnection_getFilesToSyncFromFolder(dbConn, &myFiles, folderToSyncID);
		if (error) {
			error_print(
					"checkUpdateCommand_receiveCommand: error in DBConnection_getFilesToSyncFromFolder()\n");
			checkUpdateCommand_buildAckCommand(this, ackCommandText,
					INTERNALERROR, toScan);
			return;
		}

		//scan our file list by checking differences with its list
		myFilesToScan = myFiles;
		for (myFilesToScan = myFiles; myFilesToScan != NULL; myFilesToScan = myFilesToScan->next) {
			//toScan contains the list of the files of the sender
			toScan = this->files;
			if (!fileToSyncList_isInList(toScan, myFilesToScan->value.name)) {

				//the sender hasn't the current file -> set DESYNCHRONIZED the syncOnFile with it

				//local file not in Sender list
				txLeft = tmpSync.node.numTx;
				nextTxTimestamp = getNextRetryDate(tmpSync.node);

				error = DBConnection_updateSyncOnFileState(dbConn, tmpSync,
						myFilesToScan->value, folderToSyncID, FILE_DESYNCHRONIZED);
				if (error) {
					error_print(
							"checkUpdateCommand_receiveCommand: error in DBConnection_updateSyncOnFileState()\n");
					continue;
				}
				error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(dbConn,
						tmpSync, myFilesToScan->value, folderToSyncID, txLeft,
						nextTxTimestamp);
				if (error) {
					error_print(
							"checkUpdateCommand_receiveCommand: error in DBConnection_updateSyncOnFileTxLeftAndRetryDate()\n");
					continue;
				}
			} else {
				//local file in Sender list, he has the file
				toScan = this->files;
				while (toScan != NULL
						&& strcmp(toScan->value.name,
								(myFilesToScan->value).name) != 0)
					toScan = toScan->next;

				if (toScan != NULL) {
					//file found
					if ((myFilesToScan->value).lastModified
							> (toScan->value).lastModified
							|| ((myFilesToScan->value).lastModified
									== (toScan->value).lastModified
									&& (myFilesToScan->value).deleted != toScan->value.deleted)) {

						//files not synchronized, set to DESYNCHRONIZED se syncOnFile with the node
						txLeft = tmpSync.node.numTx;
						nextTxTimestamp = getNextRetryDate(tmpSync.node);

						error = DBConnection_updateSyncOnFileState(dbConn,
								tmpSync, myFilesToScan->value, folderToSyncID,
								FILE_DESYNCHRONIZED);
						if (error) {
							error_print(
									"checkUpdateCommand_receiveCommand: error in DBConnection_updateSyncOnFileState()\n");
							continue;
						}
						error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(
								dbConn, tmpSync, myFilesToScan->value, folderToSyncID,
								txLeft, nextTxTimestamp);
						if (error) {
							error_print(
									"checkUpdateCommand_receiveCommand: error in DBConnection_updateSyncOnFileTxLeftAndRetryDate()\n");
							continue;
						}
					}
				}
			}
		}
		fileToSyncList_destroy(myFiles);
		toScan = this->files;
		checkUpdateCommand_buildAckCommand(this, ackCommandText, OK, toScan);
		return;

	} else if (strcmp(this->mode, ASK) == 0 && strcmp(this->what, _FILE) == 0) {

		//sender want to check only some files...

		toScan = this->files;

		while (toScan != NULL) {
			//check if we have the current file it asked to check
			error = DBConnection_isFileOnDb(dbConn, toScan->value,
					folderToSyncID, &result);
			if (error) {
				error_print(
						"checkUpdateCommand_receiveCommand: error in DBConnection_isFileOnDb()\n");
				toScan = toScan->next;
				continue;
			}
			if (result) {
				//we have the file
				strcpy(myFile.name, toScan->value.name);
				error = DBConnection_getFileInfo(dbConn, folderToSyncID,
						&myFile);
				if (error) {
					error_print(
							"checkUpdateCommand_receiveCommand: error in DBConnection_getSyncStatusInfo()\n");
					toScan = toScan->next;
					continue;
				}
				if (myFile.lastModified > (toScan->value).lastModified
						|| (myFile.lastModified == (toScan->value).lastModified
								&& myFile.deleted)) {
					//our file is newer -> set to DESYNCHRONIZED the syncOnFile
					txLeft = tmpSync.node.numTx;
					nextTxTimestamp = getNextRetryDate(tmpSync.node);

					error = DBConnection_updateSyncOnFileState(dbConn, tmpSync,
							myFile, folderToSyncID, FILE_DESYNCHRONIZED);
					if (error) {
						error_print(
								"checkUpdateCommand_receiveCommand: error in DBConnection_updateSyncOnFileState()\n");
						toScan = toScan->next;
						continue;
					}
					error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(
							dbConn, tmpSync, myFile, folderToSyncID, txLeft,
							nextTxTimestamp);
					if (error) {
						error_print(
								"checkUpdateCommand_receiveCommand: error in DBConnection_updateSyncOnFileTxLeftAndRetryDate()\n");
						toScan = toScan->next;
						continue;
					}
				}
			}
			toScan = toScan->next;
		}
		toScan = this->files;
		checkUpdateCommand_buildAckCommand(this, ackCommandText, OK, toScan);
		return;
	} else {
		//should never happen
		error_print("checkUpdateCommand_receiveCommand: not valid (%s %s)\n",
				this->mode, this->what);
		checkUpdateCommand_buildAckCommand(this, ackCommandText, INTERNALERROR,
				toScan);
		return;
	}

	return;
}	//checkUpdateCommand_receiveCommand()

static void checkUpdateCommand_buildAckCommand(checkUpdateCommand* this,
		char *ackCommandText, int outcome, fileToSyncList fileList) {

	char tempString[TEMP_STRING_LENGTH];
	fileToSyncList temp = fileList;

	strcpy(ackCommandText, "");

	//mi costruisco il comando di risposta
	strcpy(ackCommandText, "CheckUpdateAck");

	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);
	strcat(ackCommandText, "\t");

	strcat(ackCommandText, this->mode);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->what);
	strcat(ackCommandText, "\t");

	//aggiungo l'owner e il nome
	strcat(ackCommandText, this->owner);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->folderName);

	switch (outcome) {

	case (OK): {
		strcat(ackCommandText, "\tOK");
		break;
	}
	case (INTERNALERROR): {
		strcat(ackCommandText, "\tINTERNALERROR");
		break;
	}
	case (NOFOLDER): {
		strcat(ackCommandText, "\tNOFOLDER");
		break;
	}
	case (NOSYNC): {
		strcat(ackCommandText, "\tNOSYNC");
		break;
	}

	default: {
		strcat(ackCommandText, "\tINTERNALERROR");
		break;
	}

	}	//switch(outcome)

	//as database key is NODO and CMDTEXT
	//dopo l'esito aggiungo tutti i file
	//a questo punto per metto di seguito ogni file da aggiornare
	while (temp != NULL) {
		strcat(ackCommandText, "\t");
		strcat(ackCommandText, temp->value.name);
		strcat(ackCommandText, "\t");
		sprintf(tempString, "%d", temp->value.deleted);
		strcat(ackCommandText, tempString);
		sprintf(tempString, "%llu", temp->value.lastModified);
		strcat(ackCommandText, "\t");
		strcat(ackCommandText, tempString);
		temp = temp->next;
	}

	//metto il newline e termino
	strcat(ackCommandText, "\n");

	return;

}

//void buildCheckUpdateString(folderToSync folder, fileToSyncList files,
//		char* updateString, char *mode, char *what) {
//	char tempString[TEMP_STRING_LENGTH];
////	int i;
//
//	fileToSyncList toScan;
//
//	strcpy(updateString, "CheckUpdate");
//
////aggiungo il timestamp
//	sprintf(tempString, "%llu", getCurrentTime());
//	strcat(updateString, "\t");
//	strcat(updateString, tempString);
//	strcat(updateString, "\t");
//
//	strcat(updateString, mode);
//	strcat(updateString, "\t");
//	strcat(updateString, what);
//	strcat(updateString, "\t");
//
//	strcat(updateString, folder.owner);
//	strcat(updateString, "\t");
//	strcat(updateString, folder.name);
//
//	//qsort(files, numFiles, sizeof(fileToSync), compare);
//
//	//a questo punto per metto di seguito ogni file da aggiornare
//
//	for (toScan = files; toScan != NULL; toScan = toScan->next) {
//		strcat(updateString, "\t");
//		strcat(updateString, toScan->value.name);
//		strcat(updateString, "\t");
//		sprintf(tempString, "%d", toScan->value.deleted);
//		strcat(updateString, tempString);
//		sprintf(tempString, "%llu", toScan->value.lastModified);
//		strcat(updateString, "\t");
//		strcat(updateString, tempString);
//	}
//
////metto il newline e termino
//	strcat(updateString, "\n");
//}
