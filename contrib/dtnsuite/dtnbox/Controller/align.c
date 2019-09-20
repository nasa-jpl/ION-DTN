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
 * align.c
 * routine invocate quando inotify notifica modifiche sul FS
 * funzione di scansione completa delle cartelle (in fase di inizializzazione o di prima sincronizzazione
 * funzione di copia dei file nella cartella temporanea
 */

#include "align.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "../Model/fileToSync.h"
#include "../Model/fileToSyncList.h"
#include "../Model/synchronization.h"
#include "../Model/watchList.h"
#include "../DBInterface/DBInterface.h"
#include "./utils.h"

#include <libgen.h>
#include "debugger.h"

#include "../Model/definitions.h"

#include "fcntl.h"
#include "unistd.h"

static int ufff_rec(sqlite3* dbConn, char* absoluteFilePath);

//funzione chiamata quando viene rilevato un nuovo file o un aggiornamento su un file
int newOrUpdateFileOnFS(sqlite3 *dbConn, char *absoluteFilePath) {

	int error;

	fileToSync tempFile;
	struct stat fileInfo;
	unsigned long long lastModOnDB;

	folderToSync folder;
	int folderToSyncID;

	int result;
	int isSyncOnFilePending = 0;

	synchronization* syncsOnFolder = NULL;
	int numSyncs;

	int i;

	unsigned long long tempFileNextRetryDate;
	int tempFileTxAttempts;

	char relative_fileToSync_path[LINUX_ABSOLUTE_PATH_LENGTH];


	getRelativeFilePathFromAbsolutePath(relative_fileToSync_path, absoluteFilePath);
	if(strlen(relative_fileToSync_path) >= FILETOSYNC_LENGTH -1 || strlen(absoluteFilePath) >= (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH)){

		char sysCommand[SYSTEM_COMMAND_LENGTH + LINUX_ABSOLUTE_PATH_LENGTH];
		sprintf(sysCommand, "rm -rf \"%s\"", absoluteFilePath);

		error_print("newOrUpdateFileOnFS: the folder path size exceeds the DTNbox \"file to sync\" limit of %d characters, DTNbox delete the folder\n", FILETOSYNC_LENGTH);

		error = system(sysCommand);
		if(error){
			error_print("newOrUpdateFileOnFS: error in system(%s)\n", sysCommand);
			return ERROR_VALUE;
		}

		return SUCCESS_VALUE;
	}




	//retrieve data from absolute path (fileName and lastMod)
	getRelativeFilePathFromAbsolutePath(tempFile.name, absoluteFilePath);
	folder = getFolderToSyncFromAbsoluteFilePath(absoluteFilePath);

	error = stat(absoluteFilePath, &fileInfo);
	if (error) {
		error_print("newOrUpdateFileOnFS: error in stat() on file %s\n",
				absoluteFilePath);
		return ERROR_VALUE;
	}
	tempFile.lastModified = fileInfo.st_mtim.tv_sec;
	tempFile.deleted = FILE_NOTDELETED;
	tempFile.state = FILE_DESYNCHRONIZED;

	//retrieve data from DB
	error = DBConnection_getFolderToSyncID(dbConn, folder, &folderToSyncID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print("newOrUpdateFileOnFS: folder not found on DB\n");
		return ERROR_VALUE;
	} else if (error) {
		error_print(
				"newOrUpdateFileOnFS: error in DBConnection_getFolderToSyncID()\n");
		return ERROR_VALUE;
	}

	result = 0;
	error = DBConnection_isFileOnDb(dbConn, tempFile, folderToSyncID, &result);
	if (error) {
		error_print("newOrUpdateFileOnFS: error in DBConnection_isFileOnDb()\n");
		return ERROR_VALUE;
	}

	if (!result) {
		//file not on DB -> new file
		debug_print(DEBUG_L1, "newOrUpdateFileOnFS: new file not on DB: %s\n",
				tempFile.name);
		error = DBConnection_addFileToSync(dbConn, tempFile, folderToSyncID);
		if (error) {
			error_print(
					"newOrUpdateFileOnFS: error in DBConnection_addFileToSync()\n");
			return ERROR_VALUE;
		}
	} else {
		//file on DB -> update
		error = DBConnection_getFileToSyncLastModified(dbConn, tempFile,
				folderToSyncID, &lastModOnDB);
		if (error) {
			error_print(
					"updateFileOnFS: error in DBConnection_getFileToSyncLastModified()\n");
			return ERROR_VALUE;
		}
		if (lastModOnDB >= tempFile.lastModified) {

			//questo check è necessario per alcuni eventi spuri di inotify
			// ">" should never happen...
			return SUCCESS_VALUE;
		}
		debug_print(DEBUG_L1,
				"newOrUpdateFileOnFS: detected update on file %s\n",
				tempFile.name);
		//aggiorno l'ultima modifica sul file
		error = DBConnection_updateFileToSyncLastModified(dbConn, tempFile,
				folderToSyncID, tempFile.lastModified);
		if (error) {
			error_print(
					"newOrUpdateFileOnFS: error in DBConnection_updateFileToSyncLastModified()\n");
			return ERROR_VALUE;
		}
	}

	//propago il nuovo file o l'aggiornamento a tutte le sincronizzazioni sulla cartella
	syncsOnFolder = NULL;
	numSyncs = 0;
	error = DBConnection_getSynchronizationsOnFolder(dbConn, folder,
			&syncsOnFolder, &numSyncs);
	if (error) {
		error_print(
				"newOrUpdateFileOnFS: error in DBConnection_getSynchronizationsOnFolder()\n");
		return ERROR_VALUE;
	}

	for (i = 0; i < numSyncs; i++) {


		if(syncsOnFolder[i].node.blackWhite == BLACKLIST || syncsOnFolder[i].node.blockedBy == BLOCKEDBY)
			continue;

		//controllo di avere la sincronizzazione sul file
		result = 0;
		error = DBConnection_hasSyncStatusOnFile(dbConn, syncsOnFolder[i],
				tempFile, folderToSyncID, &result, &isSyncOnFilePending);
		if (error) {
			error_print(
					"newOrUpdateFileOnFS: error in DBConnection_hasSyncStatusOnFile()\n");
			free(syncsOnFolder);
			return ERROR_VALUE;
		}

		if (!result) {
			//non ho la sincronizzazione sul file con quel nodo -> la aggiungo
			debug_print(DEBUG_L1, "newOrUpdateFileOnFS: create the sync on file %s\n",
					tempFile.name);

			error = DBConnection_addSyncOnFile(dbConn, syncsOnFolder[i],
					tempFile, folderToSyncID);
			if (error) {
				error_print(
						"newOrUpdateFileOnFS: error in DBConnection_addSyncOnFile()\n");
				free(syncsOnFolder);
				return ERROR_VALUE;
			}
		} else {
			//avevo già la sync sul file -> ne setto lo stato a DESYNCHRONIZED
			debug_print(DEBUG_L1,
					"newOrUpdateFileOnFS: update sync on file %s\n",
					tempFile.name);

			error = DBConnection_updateSyncOnFileState(dbConn, syncsOnFolder[i],
					tempFile, folderToSyncID, FILE_DESYNCHRONIZED);
			if (error) {
				error_print(
						"newOrUpdateFileOnFS: error in DBConnection_updateSyncOnFileState()\n");
				free(syncsOnFolder);
				return ERROR_VALUE;
			}
		}

		//set dei tentativi di ritrasmissione e del tentativo di prossima ritrasmissione
		tempFileTxAttempts = syncsOnFolder[i].node.numTx;
		tempFileNextRetryDate = getNextRetryDate(syncsOnFolder[i].node);
		error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(dbConn,
				syncsOnFolder[i], tempFile, folderToSyncID, tempFileTxAttempts,
				tempFileNextRetryDate);
		if (error) {
			error_print(
					"newOrUpdateFileOnFS: error in DBConnection_updateSyncOnFileTxAttemptsAndRetryDate()\n");
			free(syncsOnFolder);
			return ERROR_VALUE;
		}
	}
	if (syncsOnFolder != NULL)
		free(syncsOnFolder);

	return SUCCESS_VALUE;
}

int deleteFileOnFS(sqlite3* dbConn, char *absoluteFilePath) {

	synchronization* syncsOnFolder = NULL;
	int error = 0;
	int numSyncs;
	int i;
	int result;
	int isSyncOnFilePending;
	int folderToSyncID;
	fileToSync tempFile;
	folderToSync folder;

	int tempFileTxAttempts;
	int tempFileNextRetryDate;

	char relative_fileToSync_path[LINUX_ABSOLUTE_PATH_LENGTH];



	//this happens when we delete a file which name is too long... we don't have to handle this deletion...
	getRelativeFilePathFromAbsolutePath(relative_fileToSync_path, absoluteFilePath);
	if(strlen(relative_fileToSync_path) >= FILETOSYNC_LENGTH -1 || strlen(absoluteFilePath) > FILETOSYNC_ABSOLUTE_PATH_LENGTH)
		return SUCCESS_VALUE;


	//retrieve data from absolute path (fileName)
	getRelativeFilePathFromAbsolutePath(tempFile.name, absoluteFilePath);
	folder = getFolderToSyncFromAbsoluteFilePath(absoluteFilePath);

	//retrieve data from DB
	error = DBConnection_getFolderToSyncID(dbConn, folder, &folderToSyncID);
	if (error) {
		error_print(
				"deleteFileOnFS: error in DBConnection_getFolderToSyncID()\n");
		return ERROR_VALUE;
	}
	error = DBConnection_isFileOnDb(dbConn, tempFile, folderToSyncID, &result);
	if (error) {
		error_print("deleteFileOnFS: error in DBConnection_isFileOnDb()\n");
		return ERROR_VALUE;
	}

	if (!result) {
		debug_print(DEBUG_L1,
				"deleteFileOnFS: detected file deletion but the file is not on database, probably it has been deleted by DTNbox\n");
		return SUCCESS_VALUE;
	} else {
		error = DBConnection_getFileInfo(dbConn, folderToSyncID, &tempFile);
		if (error) {
			error_print(
					"deleteFileOnFS: error in DBConnection_getFileInfo()\n");
			return ERROR_VALUE;
		}
	}

	if (tempFile.deleted) {
		debug_print(DEBUG_L1,
				"deleteFileOnFS: detected file deletion but the file is already in DELETED state on database...\n");
		return SUCCESS_VALUE;
	}

	error = DBConnection_updateFileDeleted(dbConn, tempFile, folderToSyncID,
			FILE_DELETED);
	if (error) {
		error_print(
				"deleteFileOnFS: error in DBConnection_updateSyncOnFileDeleted()\n");
		free(syncsOnFolder);
		return ERROR_VALUE;
	}

	//propago le modifiche a tutte le sincronizzazioni sulla cartella
	syncsOnFolder = NULL;
	numSyncs = 0;
	error = DBConnection_getSynchronizationsOnFolder(dbConn, folder,
			&syncsOnFolder, &numSyncs);
	if (error) {
		error_print(
				"deleteFileOnFS: error in DBConnection_getSynchronizationsOnFolder()\n");
		return ERROR_VALUE;
	}
	for (i = 0; i < numSyncs; i++) {

		if(syncsOnFolder[i].node.blackWhite == BLACKLIST || syncsOnFolder[i].node.blockedBy == BLOCKEDBY)
			continue;

		//per ogni sincro aggiorno lo stato a DESYNC e DELETED
		result = 0;
		error = DBConnection_hasSyncStatusOnFile(dbConn, syncsOnFolder[i],
				tempFile, folderToSyncID, &result, &isSyncOnFilePending);
		if (error) {
			error_print(
					"deleteFileOnFS: error in DBConnection_hasSyncStatusOnFile()\n");
			free(syncsOnFolder);
			return ERROR_VALUE;
		}

		if (result) {
			debug_print(DEBUG_L1,
					"deleteFileOnFS: detected file deletion on file %s, update synchronization with %s\n",
					tempFile.name, syncsOnFolder[i].node.EID);
			error = DBConnection_updateSyncOnFileState(dbConn, syncsOnFolder[i],
					tempFile, folderToSyncID, FILE_DESYNCHRONIZED);
			if (error) {
				error_print(
						"deleteFileOnFS: error in DBConnection_updateSyncOnFileState()\n");
				free(syncsOnFolder);
				return ERROR_VALUE;
			}

			//assegno il numero di trasmissioni
			tempFileTxAttempts = syncsOnFolder[i].node.numTx;
			tempFileNextRetryDate = getNextRetryDate(syncsOnFolder[i].node);
			error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(dbConn,
					syncsOnFolder[i], tempFile, folderToSyncID,
					tempFileTxAttempts, tempFileNextRetryDate);
			if (error) {
				error_print(
						"deleteFileOnFS: error in DBConnection_updateSyncOnFileTxAttemptsAndRetryDate()\n");
				free(syncsOnFolder);
				return ERROR_VALUE;
			}
		}
	}
	//faccio la free
	if (syncsOnFolder != NULL) {
		free(syncsOnFolder);
	}
	return SUCCESS_VALUE;
}

int deleteFolderOnFs(sqlite3 *dbConn, char *absoluteFilePath) {

	int error;

	if (!endsWith(absoluteFilePath, "/")) {
		error_print(
				"deleteFolderOnFs: error, file path must end with '/'\n");
		return ERROR_VALUE;
	}
	//remove the folder path  from the watchlist
	error = watchList_remove(absoluteFilePath);
	if (error) {
		error_print("deleteFolderOnFs: error in watchList_remove()\n");
		return error;
	}
	//remove the folder from DB, like a normal file
	error = deleteFileOnFS(dbConn, absoluteFilePath);
	if (error) {
		error_print("deleteFolderOnFS: errore di deleteFileOnFS()\n");
		return error;
	}

	return SUCCESS_VALUE;
}

int newFolderOnFS(sqlite3* dbConn, char *absoluteFilePath) {

	DIR *dir;
	struct dirent* current;
	int error;

	char relative_fileToSync_path[LINUX_ABSOLUTE_PATH_LENGTH];


	if (!isFolder(absoluteFilePath)) {
		error_print("newFolderOnFs: is not a folder %s\n",
				absoluteFilePath);
		return ERROR_VALUE;
	}




	getRelativeFilePathFromAbsolutePath(relative_fileToSync_path, absoluteFilePath);
	if(strlen(relative_fileToSync_path) >= FILETOSYNC_LENGTH -1 || strlen(absoluteFilePath) >= (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH)){

		char sysCommand[SYSTEM_COMMAND_LENGTH + LINUX_ABSOLUTE_PATH_LENGTH];
		sprintf(sysCommand, "rm -rf \"%s\"", absoluteFilePath);

		error_print("newFolderOnFS: the folder path size exceeds the DTNbox \"file to sync\" limit of %d characters, DTNbox delete the folder\n", FILETOSYNC_LENGTH);

		error = system(sysCommand);
		if(error){
			error_print("newFolderOnFS: error in system(%s)\n", sysCommand);
			return ERROR_VALUE;
		}

		return SUCCESS_VALUE;
	}



	//add folder to whatchlist
	error = watchList_add(absoluteFilePath);
	if (error) {
		error_print("newFolderOnFS: error in watchList_add()\n");
		return error;
	}

	if ((dir = opendir(absoluteFilePath)) == NULL) {
		error_print("newFolderOnFs: error while opening %s folder\n",
				absoluteFilePath);
		return ERROR_VALUE;
	}

	//add (sub)folder to DB, like a normal file
	error = newOrUpdateFileOnFS(dbConn, absoluteFilePath);
	if (error) {
		error_print("newFolderOnFs: error in newFileOnFS()\n");
		return error;
	}

	while ((current = readdir(dir)) != NULL) {
		//scan the folder
		if (strcmp(current->d_name, ".") == 0
				|| strcmp(current->d_name, "..") == 0)
			continue;

		strcat(absoluteFilePath, current->d_name);

		if (isFolder(absoluteFilePath)) {
			strcat(absoluteFilePath, "/");
			//recursion
			error = newFolderOnFS(dbConn, absoluteFilePath);
			if (error) {
				error_print("newFolderOnFs: error in newFolderOnFS()\n");
				return error;
			}
			absoluteFilePath[strlen(absoluteFilePath) - 1] = '\0';
		} else if (isFile(absoluteFilePath)) {
			error = newOrUpdateFileOnFS(dbConn, absoluteFilePath);
			if (error) {
				error_print("newFolderOnFs: error in newFileOnFS()\n");
				return error;
			}
		}
		absoluteFilePath[strlen(absoluteFilePath) - strlen(current->d_name)] = '\0';
	}

	if (closedir(dir)) {
		error_print("newFolderOnFs: error while closing directory\n");
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}

//function called when inotify detect the move of a (sub)folder
int moveFolderOnFs(sqlite3 *dbConn, char *absoluteFilePath) {

	int folderToSyncID;

	char relativeSubfolderPath[FILETOSYNC_LENGTH];		//note, this is a RELATIVE (sub)path, relative to syncFolderPath, e.g. sea/sardinia/friends.jpg
	char syncFolderPath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];		//absolute path, e.g. ~/DTNbox/foldersToSync/5/photos/
	char toRemoveFilePath[FILETOSYNC_ABSOLUTE_PATH_LENGTH];	//absolute path, e.g. ~/DTNbox/foldersToSync/5/photos/sea/sardinia/friends.jpg

	char **filesInSubfolder = NULL;
	int numFilesInSubfolder = 0;
	int error;
	int i;

	//rimuoviamo tutti i relativi watch dalla lista
	error = recursive_remove(absoluteFilePath);

	if (error) {
		error_print("moveFolderOnFs: error in recursive_remove()\n");
		return error;
	}

	error = getFolderToSyncIDFromAbsolutePath(dbConn, &folderToSyncID,
			absoluteFilePath);
	if (error) {
		error_print(
				"moveFolderOnFs: error in getFolderToSyncIDFromAbsolutePath()\n");
		return error;
	}
	error = getRelativeSubfolderPathFromAbsolutePath(relativeSubfolderPath,
			absoluteFilePath);
	if (error) {
		error_print(
				"moveFolderOnFs: error in getRelativeSubfolderPathFromAbsolutePath()\n");
		return error;
	}

	error = DBConnection_getFilenamesToDeleteFromSubfolderPath(dbConn,
			folderToSyncID, relativeSubfolderPath, &filesInSubfolder,
			&numFilesInSubfolder);
	if (error) {
		error_print(
				"moveFolderOnFs: errore in DBConnection_deleteFilesAndFilesSyncsForSubfolder()\n");
		return ERROR_VALUE;
	}

	getSyncFolderPathFromAbsoluteFilePath(syncFolderPath, absoluteFilePath);

	//debug_print(DEBUG_OFF, "%s\n", syncFolderPath);

	for (i = 0; i < numFilesInSubfolder; i++) {

		strcpy(toRemoveFilePath, syncFolderPath);
		strcat(toRemoveFilePath, filesInSubfolder[i]);

		error = deleteFileOnFS(dbConn, toRemoveFilePath);
		if (error) {
			for (i = 0; i < numFilesInSubfolder; i++)
				free(filesInSubfolder[i]);
			free(filesInSubfolder);

			error_print("moveFolderOnFs: error in deleteFileOnFS()\n");
			return error;
		}
	}

	for (i = 0; i < numFilesInSubfolder; i++)
		free(filesInSubfolder[i]);
	free(filesInSubfolder);

	return SUCCESS_VALUE;
}

//funzione usata nel main per allineare FS<->DB, ovvero per aggiornare il DB all'avvio del programma...
//infatti all'avvio del programma è necessaria una scansione generale per ridare consistenza al DB
//all'avvio non possiamo lavorare ad eventi ma dobbiamo fare una scansione generale
//usata anche quando c'è una nuova sincronizzazione

//folderPath del tipo ~/DTNbox/foldersToSync/owner/folder/
int alignFSandDB(sqlite3* dbConn, folderToSync folder, char* folderPath) {

	int error;
	int folderToSyncID;
	char absoluteFilePath[LINUX_ABSOLUTE_PATH_LENGTH];

	fileToSyncList toDestroy;

	//mi ricavo l'id della cartella
	error = DBConnection_getFolderToSyncID(dbConn, folder, &folderToSyncID);
	if (error) {
		error_print(
				"alignFSandDB: unable to find the folder on database\n");
		return ERROR_VALUE;
	}
	strcpy(absoluteFilePath, folderPath);

	//all'uscita della ufff_rec absoluteFilePath dovrebbe essere nella forma ~/DTNbox/foldersToSync/owner/folder/

	//questo ciclo invece setta (sul DB) a DELETED i files che sono su DB ma non su FS
	//cerco i file che sono sul database ma non sul filesystem e li metto a DELETED
	toDestroy = folder.files;
	while (toDestroy != NULL) {
		//ciclo su tutti i file
		strcat(absoluteFilePath, toDestroy->value.name);

		if (!fileExists(absoluteFilePath)
				|| (fileIsFolderOnDb(absoluteFilePath) && isFile(absoluteFilePath))
				|| (!fileIsFolderOnDb(absoluteFilePath) && isFolder(absoluteFilePath))) {

			//in questo caso anche se viene eliminata una cartella non dovrebbe esserci bisogno della deleteFolderOnFs();
			error = deleteFileOnFS(dbConn, absoluteFilePath);
			if (error) {
				error_print(
						"alignFSandDB: error in deleteFileOnFS(): %d\n",
						error);
				return error;
			}
		}
		//passo al file sul db successivo
		absoluteFilePath[strlen(absoluteFilePath) - strlen(toDestroy->value.name)] =
				'\0';
		toDestroy = toDestroy->next;
	}

	//ufff_rec propaga gli aggiornamenti del FS sul DB (nuovi files o modifiche a files)
	strcpy(absoluteFilePath, folderPath);
	error = ufff_rec(dbConn, absoluteFilePath);
	if (error) {
		error_print("alignFSandDB: error in ufff_rec()\n",
				error);
		return error;
	}

	//tutto ok
	return SUCCESS_VALUE;
}

//recursive function only called by alignFSandDB to recursively scan the absoluteFilePath
static int ufff_rec(sqlite3* dbConn, char* absoluteFilePath) {

	DIR* dir;
	int error = 0;
	struct dirent* fileInFolder;

	char relative_fileToSync_path[LINUX_ABSOLUTE_PATH_LENGTH];
	//char sysCommand[SYSTEM_COMMAND_LENGTH + LINUX_ABSOLUTE_PATH_LENGTH];


	getRelativeFilePathFromAbsolutePath(relative_fileToSync_path, absoluteFilePath);
	if(strlen(relative_fileToSync_path) >= FILETOSYNC_LENGTH -1 || strlen(absoluteFilePath) >= (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH)){

		char sysCommand[SYSTEM_COMMAND_LENGTH + LINUX_ABSOLUTE_PATH_LENGTH];
		sprintf(sysCommand, "rm -rf \"%s\"", absoluteFilePath);

		error_print("ufff_rec: the folder path size exceeds the DTNbox \"file to sync\" limit of %d characters, DTNbox delete the folder\n", FILETOSYNC_LENGTH);

		error = system(sysCommand);
		if(error){
			error_print("ufff_rec: error in system(%s)\n", sysCommand);
			return ERROR_VALUE;
		}

		return SUCCESS_VALUE;
	}


	error = (dir = opendir(absoluteFilePath)) == NULL;
	if (error) {
		error_print("ufff_rec: directory not valid %s\n", absoluteFilePath);
		return ERROR_VALUE;
	}

	error = watchList_add(absoluteFilePath);
	if(error){
		error_print("ufff_rec: error in watchList_add()\n");
		return error;
	}



	while ((fileInFolder = readdir(dir)) != NULL) {

		if (startsWith(fileInFolder->d_name, "."))
			continue;

		strcat(absoluteFilePath, fileInFolder->d_name);

		if (isFolder(absoluteFilePath))
			strcat(absoluteFilePath, "/");

		//controllo di averlo sulla lista data dal db
		error = newOrUpdateFileOnFS(dbConn, absoluteFilePath);
		if (error) {
			error_print("ufff_rec: error in newFileOnFS()\n");
			return error;
		}

		if (isFolder(absoluteFilePath)) {



			//recursion
			error = ufff_rec(dbConn, absoluteFilePath);
			if (error) {
				error_print("ufff_rec: error in ufff_rec\n");
				return error;
			}
			absoluteFilePath[strlen(absoluteFilePath) - 1] = '\0';
		} //if folder

		absoluteFilePath[strlen(absoluteFilePath) - strlen(fileInFolder->d_name)] =
				'\0';
	} //WHILE CI SONO ELEMENTI IN CARTELLA

	error = closedir(dir);
	return error;
}

//change file permissions and copy the file ti the tempDir folder
int fileLockingCopy(folderToSync folder, char *subPathFile,
		struct stat fileInfo, char *sourceFile) {

	char destFile[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH];

	//previous mask
	mode_t previousMask = fileInfo.st_mode;
	mode_t newMask = previousMask;


	int sourceFd;
	int destFd;

	int nread;
	int nwrite;

	char buffer[DIM_TEMP_COPY_BUFFER];	//read from the source file and write to the dest file up to DIM_TEMP_COPY_BUFFER per time

	if (!fileExists(sourceFile)) {
		error_print("fileLockingCopy: error, %s not exists\n", sourceFile);
		return ERROR_VALUE;
	}

	if (isFile(sourceFile)) {

		//rimuoviamo i diritti di scrivere all'utente, al gruppo e agli altri;
		newMask &= ~S_IWUSR;
		newMask &= ~S_IWGRP;
		newMask &= ~S_IWOTH;

		if (chmod(sourceFile, newMask)) {
			error_print("fileLockingCopy: error in chmod()\n");
			return ERROR_VALUE;
		}

		getHomeDir(destFile);
		strcat(destFile, DTNBOXFOLDER);
		strcat(destFile, TEMPDIR);
		strcat(destFile, OUTFOLDER);
		strcat(destFile, TEMPCOPY);
		//qui tempFileDir vale ~/DTNbox/.tempDir/

		strcat(destFile, folder.owner);
		strcat(destFile, "/");
		checkAndCreateFolder(destFile);

		strcat(destFile, folder.name);
		strcat(destFile, "/");
		checkAndCreateFolder(destFile);

		//create the tree of folders
		createFolderForPath(destFile, subPathFile);
		strcat(destFile, basename(subPathFile));


		sourceFd = open(sourceFile, O_RDONLY);
		if(sourceFd < 0){
			error_print("fileLockingCopy: error in open()\n");
			return ERROR_VALUE;
		}

		destFd = open (destFile, O_WRONLY | O_CREAT | O_EXCL, 0700);
		if(destFd < 0){
			error_print("fileLockingCopy: error in open()\n");
			close(sourceFd);
			return ERROR_VALUE;
		}

		while((nread = read(sourceFd, buffer, DIM_TEMP_COPY_BUFFER)) > 0){

			nwrite = write(destFd, buffer, nread);
			if(nwrite != nread){
				error_print("fileLockingCopy: error while writing bytes to source file\n");
				close(sourceFd);
				close(destFd);
				return ERROR_VALUE;
			}

		}
		if(nread < 0){
			error_print("fileLockingCopy: error while reading bytes from source file\n");
			close(sourceFd);
			close(destFd);
			return ERROR_VALUE;
		}


		close(sourceFd);
		close(destFd);

		//il file deve avere gli stessi diritti anche nella cartella temporanea...
		if (chmod(sourceFile, previousMask) || chmod(destFile, previousMask)) {
			error_print("fileLockingCopy: error in chmod()\n");
			return ERROR_VALUE;
		}
	}
	return SUCCESS_VALUE;
}

