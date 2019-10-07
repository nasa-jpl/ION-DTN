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
 * DBInterface_syncStatus.c
 */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/fileToSync.h"
#include "../Model/folderToSync.h"
#include "../Model/synchronization.h"
#include "DBInterface.h"

// -- Metodi di gestione della syncStatus

//aggiunta di una sincronizzazione su di un file
int DBConnection_addSyncOnFile(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID) {
	int error;
	int queryLen;
	int synchronizationID;
	int fileID;
	char tempText[TEMP_STRING_LENGTH];
	sqlite3_stmt* statement;
	const char* query =
			"INSERT INTO "
					"fileSyncStates(synchronizationID_ref, fileID_ref, state, nextTxTimestamp, txLeft) "
					"VALUES (?,?,?,?,?);";

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print( "DBConnection_addSyncOnFile: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFileToSyncValid(file)) {
		error_print( "DBConnection_addSyncOnFile: errore in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_addSyncOnFile: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;

	}

	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_addSyncOnFile: error, unable to find the synchronization on dabatase\n");
		return error;
	}
	error = DBConnection_getFileToSyncID(conn, file, folderToSyncID, &fileID);
	if (error || fileID == 0) {
		error_print(
				"DBConnection_addSyncOnFile: error, unable to find the file on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, file.state);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	sprintf(tempText, "%llu", (unsigned long long) 0);
	error = sqlite3_bind_text(statement, 4, tempText, strlen(tempText) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 5, 0);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_addSyncOnFile: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//aggiorna lo stato di un file per una sincro
int DBConnection_updateSyncOnFileState(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID, FileState newState) {
	int error;
	int queryLen;
	int synchronizationID;
	int fileID;
	sqlite3_stmt* statement;
	const char* query = "UPDATE fileSyncStates SET state = ?"
			"WHERE synchronizationID_ref == ? AND fileID_ref == ?;";

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_updateSyncOnFileState: error, unable to find the synchronization ID on database\n");
		return error;
	}
	error = DBConnection_getFileToSyncID(conn, file, folderToSyncID, &fileID);
	if (error || fileID == 0) {
		error_print(
				"DBConnection_updateSyncOnFileState: error, unable to find the file ID on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, newState);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateSyncOnFileState: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//aggiorna il numero di trasmissioni per file e la data di prossima ritrasmissione
int DBConnection_updateSyncOnFileTxLeftAndRetryDate(sqlite3* conn,
		synchronization sync, fileToSync file, int folderToSyncID, int txLeft,
		unsigned long long nextTxTimestamp) {
	int error;
	int queryLen;
	int synchronizationID;
	int fileID;
	char tempText[TEMP_STRING_LENGTH];
	sqlite3_stmt* statement;
	const char* query = "UPDATE fileSyncStates SET txLeft = ?, nextTxTimestamp = ? "
			"WHERE synchronizationID_ref == ? AND fileID_ref == ?;";

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error, unable to find the synchronization\n");
		return error;
	}
	error = DBConnection_getFileToSyncID(conn, file, folderToSyncID, &fileID);
	if (error || fileID == 0) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error, unable to find the file on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, txLeft);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	strcpy(tempText, "");
	sprintf(tempText, "%llu", nextTxTimestamp);
	error = sqlite3_bind_text(statement, 2, tempText, strlen(tempText) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 4, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateSyncOnFileTxLeftAndRetryDate: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//cancello la sincro da un file
int DBConnection_deleteSyncOnFile(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID) {
	int error;
	int queryLen;
	int synchronizationID;
	int fileID;
	sqlite3_stmt* statement;
	const char* query = "DELETE FROM fileSyncStates "
			"WHERE synchronizationID_ref == ? AND fileID_ref == ?;";

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_deleteSyncOnFile: error, unable to find the synchronization on database\n");
		return error;
	}
	error = DBConnection_getFileToSyncID(conn, file, folderToSyncID, &fileID);
	if (error || fileID == 0) {
		error_print(
				"DBConnection_deleteSyncOnFile: error, unable to find the file on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_deleteSyncOnFile: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//cancello la sincro da tutti i file
int DBConnection_deleteSyncOnAllFiles(sqlite3* conn, synchronization sync) {
	int error;
	int queryLen;
	int synchronizationID;
	sqlite3_stmt* statement;
	const char* query =
			"DELETE FROM fileSyncStates WHERE synchronizationID_ref == ?;";

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_deleteSyncOnAllFiles: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_deleteSyncOnAllFiles: error, unable to find the synchronization on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSyncOnAllFiles: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSyncOnAllFiles: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_deleteSyncOnAllFiles: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//recupero il numero di trasmissioni di un file
int DBConnection_getSyncOnFileTxLeft(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID, int* txLeft) {
	int error;
	int queryLen;
	int synchronizationID;
	int fileID;
	sqlite3_stmt* statement;
	const char* query = "SELECT txLeft FROM fileSyncStates "
			"WHERE synchronizationID_ref == ? AND fileID_ref == ?;";

	*txLeft = -1;

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error, unable to find the synchronization on database\n");
		return error;
	}
	error = DBConnection_getFileToSyncID(conn, file, folderToSyncID, &fileID);
	if (error || fileID == 0) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error, unable to find the file on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*txLeft = 0;
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error, data not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*txLeft = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getSyncOnFileTxLeft: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

/*
 * Ricordarsi di fare la free dell'array di files nel chiamante.
 */
//malloc: controllare chiamanti... errori interni OK
// getAllFilesToSend() (send and re-send)
int DBConnection_getAllFilesToUpdate(sqlite3* conn, synchronization sync,
		folderToSync folder, fileToSync** files, int* numFiles, unsigned long long currentTime) {
	int error;
	int i;
	int synchronizationID;
	int folderToSyncID;
	int queryLen;
	fileToSync tempFile;
	sqlite3_stmt* statement;

	char lluString[TEMP_STRING_LENGTH];

	const char* queryCount =
			"SELECT COUNT(*) "
					"FROM files "
					"INNER JOIN fileSyncStates ON fileSyncStates.fileID_ref = files._fileID "
					"WHERE fileSyncStates.synchronizationID_ref == ? AND txLeft > 0 AND (fileSyncStates.state == ? OR (fileSyncStates.state == ? AND fileSyncStates.nextTxTimestamp < ?)) "
					"AND files.folderID_ref == ?;";
	const char* queryData =
			"SELECT files.name, files.lastModified, files.deleted "
					"FROM files "
					"INNER JOIN fileSyncStates ON fileSyncStates.fileID_ref = files._fileID "
					"WHERE fileSyncStates.synchronizationID_ref == ? AND txLeft > 0 AND (fileSyncStates.state == ? OR (fileSyncStates.state == ? AND fileSyncStates.nextTxTimestamp < ?)) "
					"AND files.folderID_ref == ?;";
					//TODO ORDER BY... cosi non se ne occupa il programmatore

	*files = NULL;
	*numFiles=0;

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFolderToSyncValid(folder)) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in folder parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//ricavo i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error || synchronizationID == 0) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error, unable to find the synchronization on database\n");
		return error;
	}
	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error != 0) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error, unable to find the folder on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, FILE_DESYNCHRONIZED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 3, FILE_PENDING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	sprintf(lluString, "%llu", currentTime);
	error = sqlite3_bind_text(statement, 4, lluString, strlen(lluString)+1, SQLITE_STATIC);
	if(error != SQLITE_OK){
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 5, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numFiles = 0;
		*files = NULL;
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numFiles = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//ora che conosco il numero di sincronizzazioni istanzio l'array, sarà il chiamante a doversi occupare della free
	*files = (fileToSync*) malloc(sizeof(fileToSync) * *numFiles);

	//procedo a leggere tutte le cartelle
	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		free(*files);
		*files = NULL;
		*numFiles=0;
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*files);
		*files = NULL;
		*numFiles=0;
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, FILE_DESYNCHRONIZED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*files);
		*files = NULL;
		*numFiles=0;
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 3, FILE_PENDING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*files);
		*files = NULL;
		*numFiles=0;
		return DB_BIND_ERROR;
	}

	sprintf(lluString, "%llu", currentTime);
	error = sqlite3_bind_text(statement, 4, lluString, strlen(lluString)+1, SQLITE_STATIC);
	if(error != SQLITE_OK){
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*files);
		*files = NULL;
		*numFiles=0;
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 5, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFilesToUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*files);
		*files = NULL;
		*numFiles=0;
		return DB_BIND_ERROR;
	}
	//leggo i dati
	for (i = 0; i < *numFiles; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getAllFilesToUpdate: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			free(*files);
			*files = NULL;
			*numFiles=0;
			return DB_STEP_ERROR;
		}

		//mi popolo la struttura dati
		strcpy(tempFile.name,
				(const char*) sqlite3_column_text(statement, 0));
		tempFile.lastModified = strtoull(
				(const char*) sqlite3_column_text(statement, 1), NULL, 0);
		tempFile.deleted = sqlite3_column_int(statement, 2);

		//assegno la carella alla lista
		(*files)[i] = tempFile;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//controlla se esiste la syncstate sul file
int DBConnection_hasSyncStatusOnFile(sqlite3* conn, synchronization sync,
		fileToSync fileToCheck, int folderToSyncID, int* result, int* isPending) {
	int error;
	int queryLen;
	int fileID;
	int syncID;
	sqlite3_stmt* statement;
	const char* query = "SELECT _fileSyncStateID, state FROM fileSyncStates "
			"WHERE synchronizationID_ref == ? AND fileID_ref == ?;";

	*result=0;
	*isPending=0;

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (!isFileToSyncValid(fileToCheck)) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error in fileToCheck parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//ricavo i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &syncID);
	if (error || syncID == 0) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error, unable to find the synchronization on database\n");
		return error;
	}
	error = DBConnection_getFileToSyncID(conn, fileToCheck, folderToSyncID,
			&fileID);
	if (error != 0) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error, unable to find the folder on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, syncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_hasSyncStatusOnFile: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è il record cercato
		*result = 0;
		*isPending = 0;
		//non segnalo l'errore con il return -1 in quanto e' nella semantica dell'operazione controllare il result
	} else if (error == SQLITE_ROW) {
		//se sono qui, ho trovato una riga
		*result = 1;
		if (sqlite3_column_int(statement, 1) == FILE_PENDING) {
			*isPending = 1;
		} else {
			*isPending = 0;
		}
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_hasSyncStatusOnFile: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//imposta tutti i file a DESYNC forzandone l'update indipendentemente dallo stato in cui si trovano
int DBConnection_forceUpdate(sqlite3* conn) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "UPDATE fileSyncStates SET state = ?;";

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_forceUpdate: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, FILE_DESYNCHRONIZED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_forceUpdate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_forceUpdate: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}
