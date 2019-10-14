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
 * MB: receiveCommand fatta da zero
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <utime.h>
#include <sys/stat.h>

#include "updateCommand.h"
#include "../Controller/utils.h"
#include "../DBInterface/DBInterface.h"
#include "definitions.h"
#include "../Controller/debugger.h"

#include "watchList.h"

static int parseUpdateCommand(sqlite3 *dbConn, synchronization sync,
		int **outcomeUpdates, int *numFilesToUpdate, int synchronizationID,
		int folderToSyncID);

static void updateCommand_buildAckCommand(updateCommand* this,
		char *ackCommandText, int outcome, fileToSyncList fileList,
		int numFilesToUpdate, int *outcomeUpdates);

void updateCommand_createCommand(command** cmd) {
	updateCommand* this;
	char* cmdText;
	char *toFree;
	char* token;
	fileToSync tempFile;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(updateCommand));
	this = (updateCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//Update timestamp owner folderName fileName deleted lastModified
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
void updateCommand_destroyCommand(command* cmd) {
	updateCommand* this;
	this = (updateCommand*) cmd;
	if (this->files != NULL)
		fileToSyncList_destroy(this->files);

	else
		error_print(
				"updateCommand_destroyCommand: this->files should never be null\n");

}
void updateCommand_sendCommand(command* cmd) {
	cmd->state = CMD_PENDING;
	return;
}

void updateCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {

	updateCommand* this = (updateCommand*) cmd;
	int error = -1;

	synchronization tmpSync;
	fileToSyncList toDestroy = NULL;

	int numFilesToUpdate = 0;
	int *outcomeUpdates = NULL;

	int folderToSyncID;
	int synchronizationID;

	debug_print(DEBUG_L1,
			"updateCommand_receiveCommand: received and accepted an UPDATE command\n");
	debug_print(DEBUG_L1, "updateCommand_receiveCommand: %s\n",
			this->base.text);

	this->files = fileToSyncList_reverse(this->files);
	toDestroy = this->files;

	//mi costruisco la cartella temporanea
	tmpSync.folder = getEmptyFileListFolderFromOwnerAndFolder(this->owner,
			this->folderName);
	tmpSync.folder.files = toDestroy;

	//mi costruisco la sincro
	tmpSync.node = this->base.msg.source;

	//mi ricavo il suo id
	error = DBConnection_getFolderToSyncID(dbConn, tmpSync.folder,
			&folderToSyncID);
	if (error) {
		error_print(
				"updateCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		error = DB_DATA_NOT_FOUND_ERROR ? NOFOLDER : INTERNALERROR;
		updateCommand_buildAckCommand(this, ackCommandText, error, toDestroy,
				numFilesToUpdate, outcomeUpdates);
		return;
	}

	error = DBConnection_getSynchronizationID(dbConn, tmpSync,
			&synchronizationID);
	if (error) {
		error_print(
				"updateCommand_receiveCommand: error in DBConnection_getSynchronizationID()\n");
		error = DB_DATA_NOT_FOUND_ERROR ? NOSYNC : INTERNALERROR;
		updateCommand_buildAckCommand(this, ackCommandText, error, toDestroy,
				numFilesToUpdate, outcomeUpdates);
		return;
	}

	//tmpSync.folder.files must contains the files of the update
	//error will contains the outcome
	error = parseUpdateCommand(dbConn, tmpSync, &outcomeUpdates,
			&numFilesToUpdate, synchronizationID, folderToSyncID);

	toDestroy = this->files;

	updateCommand_buildAckCommand(this, ackCommandText, error, toDestroy,
			numFilesToUpdate, outcomeUpdates);

	free(outcomeUpdates);

	return;
}	//updateCommand_receiveCommand()

static void updateCommand_buildAckCommand(updateCommand* this,
		char *ackCommandText, int outcome, fileToSyncList fileList,
		int numFilesToUpdate, int *outcomeUpdates) {

	char tempString[TEMP_STRING_LENGTH];
	fileToSyncList temp = fileList;

	strcpy(ackCommandText, "");

	//mi costruisco il comando di risposta
	strcpy(ackCommandText, "UpdateAck");

	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);

	//aggiungo l'owner e il nome
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->owner);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->folderName);

	switch (outcome) {

	case (OK): {
		strcat(ackCommandText, "\tOK");
		break;
	}
	case (INTERNALERROR): {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR");
		break;
	}
	case (NOFOLDER): {
		strcat(ackCommandText, "\tERROR\tNOFOLDER");
		break;
	}
	case (NOSYNC): {
		strcat(ackCommandText, "\tERROR\tNOSYNC");
		break;
	}
	case (INCOMPLETE): {

		int i = 0;
		char *updateMask = NULL;//updateMask is a string like "00110101", cannot be "111111"

		updateMask = (char*) malloc(sizeof(char) * (numFilesToUpdate + 1));	//+1 x il '\0'

		for (i = 0; i < numFilesToUpdate; i++)
			updateMask[i] = outcomeUpdates[i] + '0';
		updateMask[numFilesToUpdate] = '\0';

		strcat(ackCommandText, "\tERROR\tINCOMPLETE");
		//probabilmente il secondo \t è meglio non mettero...
		strcat(ackCommandText, updateMask);

		free(updateMask);
		break;
	}
	default: {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR");
		break;
	}

	}	//switch(outcome)

	//as database key is NODO and CMDTEXT
	//dopo l'ok aggiungo tutti i file
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

//void buildUpdateString(folderToSync folder, fileToSync* files, int numFiles,
//		char* updateString) {
//	char tempString[TEMP_STRING_LENGTH];
//	int i;

//	strcpy(updateString, "Update");

//aggiungo il timestamp
//	sprintf(tempString, "%llu", getCurrentTime());
//	strcat(updateString, "\t");
//	strcat(updateString, tempString);

//aggiungo l'owner e il nome
//	strcat(updateString, "\t");
//	strcat(updateString, folder.owner);
//	strcat(updateString, "\t");
//	strcat(updateString, folder.name);

//	qsort(files, numFiles, sizeof(fileToSync), compare);

//a questo punto per metto di seguito ogni file da aggiornare
//	for (i = 0; i < numFiles; i++) {
//		if (strlen(files[i].name) > 0) {
//			strcat(updateString, "\t");
//			strcat(updateString, files[i].name);
//			strcat(updateString, "\t");
//			sprintf(tempString, "%d", files[i].deleted);
//			strcat(updateString, tempString);
//			sprintf(tempString, "%llu", files[i].lastModified);
//			strcat(updateString, "\t");
//			strcat(updateString, tempString);
//		}
//	}

//metto il newline e termino
//	strcat(updateString, "\n");
//}

int parseUpdateCommand(sqlite3 *dbConn, synchronization sync,
		int **outcomeUpdates, int *numFilesToUpdate, int synchronizationID,
		int folderToSyncID) {

	int error;

	char folderLocation[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH] = "";	//~/DTNbox/foldersToSync/owner/folderName/
	char completeFilePath[FILETOSYNC_ABSOLUTE_PATH_LENGTH] = "";//~/DTNbox/foldersToSync/owner/folderName/fileName (where filename can be subfolderA/subfolderB/fileName)
	char tarExctractDir[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH] = "";		// ~/DTNbox/.tempDir/in/currentProcessed

	fileToSyncList toDestroy;
	fileToSync currentFile;	//of the sender of the update command

	int isFileOnDB;
	int hasSyncStatusOnFile;
	int isSyncPending;
//	unsigned long long fileLastModified;

	fileToSync myFile; //contains my informations about the file

	int mustPropagate = 0; // == 1 <-> aggiornamento sul file valido

	int i = 0;
	int j = 0;

	char sysCommand[SYSTEM_COMMAND_LENGTH + DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH + FILETOSYNC_ABSOLUTE_PATH_LENGTH];

	struct stat fileStat;
	struct utimbuf newTimes;

	synchronization* allSyncOnFolder = NULL;
	int numSync;

	int tempFileTxAttempts;
	unsigned long long tempFileNextRetryDate;

	int numSyncStatusOnFile;

	*numFilesToUpdate = 0;
	*outcomeUpdates = NULL;

	getHomeDir(tarExctractDir);
	strcat(tarExctractDir, DTNBOXFOLDER);
	strcat(tarExctractDir, TEMPDIR);
	strcat(tarExctractDir, INFOLDER);
	strcat(tarExctractDir, CURRENTPROCESSED);

	//mi costruisco la directory
	getAbsolutePathFromOwnerAndFolder(folderLocation, sync.folder.owner,
			sync.folder.name);

	//ciclo su tutti i file del comando per aggiornarli

	i = 0;
	toDestroy = sync.folder.files;

	*numFilesToUpdate = fileToSyncList_size(toDestroy);
	*outcomeUpdates = (int*) malloc(sizeof(int) * (*numFilesToUpdate));
	for (i = 0; i < *numFilesToUpdate; i++)
		(*outcomeUpdates)[i] = OK;

	//must follow this order!!
	//1)fare le varie delete
	//2)creare le varie cartelle
	//3)creare i vari files
	i = 0;
	toDestroy = sync.folder.files;
	while (toDestroy != NULL) {

		//current rappresenta il file corrente della lista del mittente
		//così in caso di modifiche modifichiamo currentFile e non toDestroy->value
		strcpy(currentFile.name, toDestroy->value.name);
		currentFile.deleted = toDestroy->value.deleted;
		currentFile.lastModified = toDestroy->value.lastModified;

		//mi creo il path completo del file
		strcpy(completeFilePath, folderLocation);
		strcat(completeFilePath, currentFile.name);

		strcpy(myFile.name, currentFile.name);

		error = DBConnection_isFileOnDb(dbConn, myFile, folderToSyncID,
				&isFileOnDB);
		if (error) {
			error_print(
					"updateCommand_receiveCommand: error in DBConnection_isFileOnDb()\n");
			(*outcomeUpdates)[i++] = INTERNALERROR;
			toDestroy = toDestroy->next;
			continue;	//INTERNALERROR
		}

		hasSyncStatusOnFile = 0;
		isSyncPending = 0;
		if (isFileOnDB) {
			//only if we have file on DB we can retrieve informations about the file
			//and verify if we have the synchronization state
			error = DBConnection_getFileInfo(dbConn, folderToSyncID, &myFile);
			if (error) {
				error_print(
						"updateCommand_receiveCommand: error in DBConnection_getFileInfo()\n");
				(*outcomeUpdates)[i++] = INTERNALERROR;
				toDestroy = toDestroy->next;
				continue;	//INTERNALERROR
			}

			error = DBConnection_hasSyncStatusOnFile(dbConn, sync, currentFile,
					folderToSyncID, &hasSyncStatusOnFile, &isSyncPending);
			if (error) {
				error_print(
						"updateCommand_receiveCommand: error in DBConnection_hasSyncStatusOnFile()\n");

				(*outcomeUpdates)[i++] = INTERNALERROR;
				toDestroy = toDestroy->next;
				continue;	//INTERNALERROR
			}
		}

		if (currentFile.deleted) {
			//check if file deletion must be propagated
			if (isFileOnDB) {
				if (currentFile.lastModified > myFile.lastModified)
					mustPropagate = 1;
				else if (currentFile.lastModified == myFile.lastModified
						&& currentFile.deleted != myFile.deleted)
					mustPropagate = 1;
			}
		} else {
			//check if update file or new file must be propagated
			if (isFileOnDB && currentFile.lastModified > myFile.lastModified)
				mustPropagate = 1;
			else if (!isFileOnDB)
				mustPropagate = 1;
		}

		//DELETED FILE------------------------------------------------------------------------------------------------------------------------

		if (currentFile.deleted) {

			strcpy(sysCommand, "rm -r \"");
			strcat(sysCommand, completeFilePath);
			strcat(sysCommand, "\"");

			if (!isFileOnDB || mustPropagate) {

				debug_print(DEBUG_L1, "updateCommand_receiveCommand: current file processed %s\n",
						currentFile.name);

				if (mustPropagate) {

					error = DBConnection_updateFileDeleted(dbConn, currentFile,
							folderToSyncID, FILE_DELETED);
					if (error) {
						error_print(
								"updateCommand_receiveCommand: error in DBConnection_updateSyncOnFileDeleted()\n");
						(*outcomeUpdates)[i++] = INTERNALERROR;
						toDestroy = toDestroy->next;
						continue;
					}

					if (currentFile.lastModified > myFile.lastModified) {
						//aggiorna timestamp su db
						error = DBConnection_updateFileToSyncLastModified(
								dbConn, currentFile, folderToSyncID,
								currentFile.lastModified);
						if (error) {
							error_print(
									"updateCommand_receiveCommand: error in DBConnection_updateFileToSyncLastModified()\n");
							(*outcomeUpdates)[i++] = INTERNALERROR;
							toDestroy = toDestroy->next;
							continue;	//INTERNALERROR
						}
					}

					if (hasSyncStatusOnFile) {
						//elimina sync on file con il nodo mittente se esiste...
						error = DBConnection_deleteSyncOnFile(dbConn, sync,
								currentFile, folderToSyncID);
						if (error) {
							error_print(
									"updateCommand_receiveCommand: error in DBConnection_deleteSyncOnFile()\n");
							(*outcomeUpdates)[i++] = INTERNALERROR;
							toDestroy = toDestroy->next;
							continue;	//INTERNALERROR
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
					}
				}					//if file is on DB and must be "updated"

				if (fileExists(completeFilePath)) {
					//try to delete file from FS also if file not on DB
					error = system(sysCommand);
					if (error) {
						error_print(
								"updateCommand_receiveCommand: error in system()\n");
						(*outcomeUpdates)[i++] = INTERNALERROR;
						toDestroy = toDestroy->next;
						continue;	//INTERNALERROR
					}
				}
			}
		}	//if file deleted

		//FINE DELETED FILE------------------------------------------------------------------------------------------------------------------------

		//NEW OR UPDATE FILE------------------------------------------------------------------------------------------------------------------------

		else {

			//copio il file su fs se file nuovo o file di aggiornamento piu nuovo che quello locale
			if (mustPropagate) {

				strcpy(sysCommand, "");
				if (!fileIsFolderOnDb(currentFile.name)) {
					//caso "update file" o "nuovo file"
					sprintf(sysCommand, "cp \"%s%s/%s/%s\" \"%s\"",
							tarExctractDir, sync.folder.owner,
							sync.folder.name, currentFile.name,
							completeFilePath);

				} else if (fileIsFolderOnDb(currentFile.name)
						&& !fileExists(completeFilePath)) {

					//caso "nuova cartella"
					strcpy(sysCommand, "mkdir \"");
					strcat(sysCommand, completeFilePath);
					strcat(sysCommand, "\"");
					//				debug_print(DEBUG_L1,
					//						"updateCommand_receiveCommand: creo cartella %s\n",
					//						currentFile.fileName);
				}
				if (strcmp("", sysCommand) != 0) {
					debug_print(DEBUG_L1,
							"updateCommand_receiveCommand: command %s\n",
							sysCommand);
					error = system(sysCommand);
					if (error) {
						error_print(
								"updateCommand_receiveCommand: error in system()\n");
						(*outcomeUpdates)[i++] = INTERNALERROR;
						toDestroy = toDestroy->next;
						continue;	//INTERNALERROR
					} else {
						//modifico il timestamp di ultima modifica sul File System, cosi non ho il ping pong
						if (stat(completeFilePath, &fileStat) < 0) {
							error_print(
									"updateCommand_receiveCommand: error in stat() on %s\n",
									completeFilePath);
							(*outcomeUpdates)[i++] = INTERNALERROR;
							toDestroy = toDestroy->next;
							continue;	//INTERNALERROR
						}

						newTimes.actime = fileStat.st_atime;
						newTimes.modtime = (time_t) currentFile.lastModified;
						if (utime(completeFilePath, &newTimes) < 0) {
							error_print(
									"updateCommand_receiveCommand: error in utime()\n");
							(*outcomeUpdates)[i++] = INTERNALERROR;
							toDestroy = toDestroy->next;
							continue;	//INTERNALERROR
						}

					}
				}
			}	//modifica sul File System...

			if (!isFileOnDB) {
				//aggiungiamolo al DB, con i dati che abbiamo ricevuto dal mittente ovvero timestamp nuovo
				error = DBConnection_addFileToSync(dbConn, currentFile,
						folderToSyncID);
				if (error) {
					error_print(
							"updateCommand_receiveCommand: error in DBConnection_addFileToSync()\n");
					(*outcomeUpdates)[i++] = INTERNALERROR;
					toDestroy = toDestroy->next;
					continue;	//INTERNALERROR
				}
				//isFileOnDB=1:

			} else {
				if (currentFile.lastModified > myFile.lastModified) {
					//aggiorno il timestamp su DB
					error = DBConnection_updateFileToSyncLastModified(dbConn,
							currentFile, folderToSyncID,
							currentFile.lastModified);
					if (error) {
						error_print(
								"updateCommand_receiveCommand: error in DBConnection_updateFileToSyncLastModified()\n");
						(*outcomeUpdates)[i++] = INTERNALERROR;
						toDestroy = toDestroy->next;
						continue;	//INTERNALERROR
					}
				}
			}

			if (!hasSyncStatusOnFile) {
				//aggiungiamo la sincronizzazione, con stato SYNCHRONIZED e file not deleted.
				currentFile.state = FILE_SYNCHRONIZED;
				error = DBConnection_addSyncOnFile(dbConn, sync, currentFile,
						folderToSyncID);
				if (error) {
					error_print(
							"updateCommand_receiveCommand: error in DBConnection_addSyncOnFile()\n");
					(*outcomeUpdates)[i++] = INTERNALERROR;
					toDestroy = toDestroy->next;
					continue;	//INTERNALERROR
				}
			} else if (currentFile.lastModified > myFile.lastModified) {

				error = DBConnection_updateSyncOnFileState(dbConn, sync,
						currentFile, folderToSyncID, FILE_SYNCHRONIZED);
				if (error) {
					error_print(
							"updateCommand_receiveCommand: error in DBConnection_updateSyncOnFileState()\n");
					(*outcomeUpdates)[i++] = INTERNALERROR;
					toDestroy = toDestroy->next;
					continue;	//INTERNALERROR
				}
			}
		}	//file not deleted

		(*outcomeUpdates)[i] = OK;	//esito solo dell'aggiornamento locale....

		if (mustPropagate) {
			//propago la modifica alle altre sincronizzazioni
			allSyncOnFolder = NULL;
			numSync = 0;
			//prendo solo quelle in PUSH e PUSH&PULL
			error = DBConnection_getSynchronizationsOnFolder(dbConn,
					sync.folder, &allSyncOnFolder, &numSync);
			if (error) {
				error_print(
						"updateCommand_receiveCommand: error in DBConnection_getSynchronizationsOnFolder()\n");
			}
			else {
				for (j = 0; j < numSync; j++) {

					if(allSyncOnFolder[j].node.blackWhite == BLACKLIST || allSyncOnFolder[j].node.blockedBy == BLOCKEDBY)
						continue;

					if (strcmp(allSyncOnFolder[j].node.EID, sync.node.EID)
							!= 0) {

						hasSyncStatusOnFile = -1;
						isSyncPending = -1;
						error = DBConnection_hasSyncStatusOnFile(dbConn,
								allSyncOnFolder[j], currentFile, folderToSyncID,
								&hasSyncStatusOnFile, &isSyncPending);
						if (error) {
							error_print(
									"updateCommand_receiveCommand: error in DBConnection_hasSyncStatusOnFile()\n");
							continue;
						}



						if (!hasSyncStatusOnFile) {
							//la sync su file non c'è
							debug_print(DEBUG_L1,
									"updateCommand_receiveCommand: create the synchronization on file %s\n",
									currentFile.name);
							currentFile.state = FILE_DESYNCHRONIZED;//currentFile.deleted should be already ok!!
							error = DBConnection_addSyncOnFile(dbConn,
									allSyncOnFolder[j], currentFile,
									folderToSyncID);
							if (error) {
								error_print(
										"updateCommand_receiveCommand: error in DBConnection_addSyncOnFile()\n");
								continue;
							}
						} else {
							//la sync su file c'è
							debug_print(DEBUG_L1,
									"updateCommand_receiveCommand: update the synchronization on file %s\n",
									currentFile.name);
							error = DBConnection_updateSyncOnFileState(dbConn,
									allSyncOnFolder[j], currentFile,
									folderToSyncID, FILE_DESYNCHRONIZED);
							if (error) {
								error_print(
										"updateCommand_receiveCommand: error in DBConnection_updateSyncOnFileState()\n");
								continue;
							}
						}

						//in entrambi i casi devo aggiornare i txLeft e la nextTxTimestamp
						tempFileTxAttempts = allSyncOnFolder[j].node.numTx;
						tempFileNextRetryDate = getNextRetryDate(
								allSyncOnFolder[j].node);
						error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(
								dbConn, allSyncOnFolder[j], currentFile,
								folderToSyncID, tempFileTxAttempts,
								tempFileNextRetryDate);
						if (error) {
							error_print(
									"updateCommand_receiveCommand: error in DBConnection_updateSyncOnFileTxAttemptsAndRetryDate()\n");
							continue;
						}

					}//non devo propagare l'aggiornamento a chi me l'ha mandato
				}	//per ogni sincronizzazione a cui propagare l'aggiornamento
			}
			if (allSyncOnFolder != NULL)
				free(allSyncOnFolder);
		}	//if(devoPropagare)

		i++;
		toDestroy = toDestroy->next;
	}	//while sui file

	//altro while per sistemare i timestamp delle cartelle
	i = 0;
	toDestroy = sync.folder.files;
	while (toDestroy != NULL) {

		//TODO vedere se è veramente da aggiornare (vedi timestamp...!!!);

		strcpy(currentFile.name, toDestroy->value.name);
		currentFile.deleted = toDestroy->value.deleted;
		currentFile.lastModified = toDestroy->value.lastModified;

		//mi creo il path completo del file
		strcpy(completeFilePath, folderLocation);
		strcat(completeFilePath, currentFile.name);

		if (!currentFile.deleted && fileIsFolderOnDb(currentFile.name)) {
			if (stat(completeFilePath, &fileStat) < 0) {
				error_print(
						"updateCommand_receiveCommand: error in stat() su %s\n",
						completeFilePath);
				(*outcomeUpdates)[i++] = INTERNALERROR;
				toDestroy = toDestroy->next;
				continue;	//INTERNALERROR
			}

			newTimes.actime = fileStat.st_atime;
			newTimes.modtime = (time_t) currentFile.lastModified;
			if (utime(completeFilePath, &newTimes) < 0) {
				error_print("updateCommand_receiveCommand: error in utime()\n");
				(*outcomeUpdates)[i++] = INTERNALERROR;
				toDestroy = toDestroy->next;
				continue;	//INTERNALERROR
			}
		}
		i++;
		toDestroy = toDestroy->next;
	}

	toDestroy = sync.folder.files;

	error = 0;
	for (i = 0; i < *numFilesToUpdate && !error; i++)
		if ((*outcomeUpdates)[i] == INTERNALERROR)
			error = 1;

	return (error ? INCOMPLETE : OK);
}	//parseUpdateCommand

