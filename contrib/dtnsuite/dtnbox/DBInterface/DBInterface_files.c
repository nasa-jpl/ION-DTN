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
 * DBInterface_files.c
 */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/fileToSync.h"
#include "../Model/fileToSyncList.h"
#include "DBInterface.h"

// -- Metodi di gestione file da sincronizzare

//aggiunta file su database
int DBConnection_addFileToSync(sqlite3* conn, fileToSync file,
		int folderToSyncID) {
	int error;
	int queryLen;
	char tempText[TEMP_STRING_LENGTH];
	sqlite3_stmt* statement;
	const char* query =
			"INSERT INTO files(folderID_ref,name,lastModified,deleted) VALUES (?,?,?,?);";

	if (!isFileToSyncValid(file)) {
		error_print("DBConnection_addFileToSync: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFileToSync: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFileToSync: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, file.name,
			strlen(file.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFileToSync: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	sprintf(tempText, "%llu", file.lastModified);
	error = sqlite3_bind_text(statement, 3, tempText, strlen(tempText) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFileToSync: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 4, file.deleted);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFileToSync: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_addFileToSync: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//controlla se esiste una file sul database e salva il risultato nella variabile result
int DBConnection_isFileOnDb(sqlite3* conn, fileToSync fileToCheck,
		int folderToSyncID, int* result) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "SELECT _fileID FROM files "
			"WHERE folderID_ref == ? AND name == ?;";

	(*result) = 0;

	//controllo parametri
	if (!isFileToSyncValid(fileToCheck)) {
		error_print(
				"DBConnection_isFileOnDb: error in folderToCheck parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isFileOnDb: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isFileOnDb: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, fileToCheck.name,
			strlen(fileToCheck.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isFileOnDb: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è il record cercato
		*result = 0;
		//non segnalo l'errore con il return -1 in quanto e' nella semantica dell'operazione controllare il result
	} else if (error == SQLITE_ROW) {
		//se sono qui, ho trovato una riga
		*result = 1;
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print("DBConnection_isFileOnDb: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//restituisce la lista di file per una data cartella (allocazione dinamica)
int DBConnection_getFilesToSyncFromFolder(sqlite3* conn, fileToSyncList* files,
		int folderToSyncID) {
	int error;
	int queryLen;
	fileToSync tempFile;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT name, lastModified, deleted FROM files WHERE folderID_ref == ?;";

	*files = fileToSyncList_create();

	//controllo parametro passato
	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getFilesToSyncFromFolder: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFilesToSyncFromFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFilesToSyncFromFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//faccio la prima step
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//se sono qua ho letto i dati
		strcpy(tempFile.name,
				(const char*) sqlite3_column_text(statement, 0));
		tempFile.lastModified = strtoull(
				(const char*) sqlite3_column_text(statement, 1), NULL, 0);
		tempFile.deleted = sqlite3_column_int(statement, 2);

		//li aggiungo alla lista
		*files = fileToSyncList_add(*files, tempFile);

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_getFilesToSyncFromFolder: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		fileToSyncList_destroy(*files);
		*files = fileToSyncList_create();

		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo l'id sul database di un file
int DBConnection_getFileToSyncID(sqlite3* conn, fileToSync file,
		int folderToSyncID, int* fileID) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT _fileID FROM files WHERE name == ? AND folderID_ref == ?;";

	*fileID = -1;

	if (!isFileToSyncValid(file)) {
		error_print("DBConnection_getFileToSyncID: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getFileToSyncID: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;

	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFileToSyncID: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, file.name,
			strlen(file.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFileToSyncID: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFileToSyncID: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*fileID = 0;
		error_print("DBConnection_getFileToSyncID: file not found on DB\n");
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*fileID = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getFileToSyncID: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo la data di modifica sul database di un file
int DBConnection_getFileToSyncLastModified(sqlite3* conn, fileToSync file,
		int folderToSyncID, unsigned long long* lastModified) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT lastModified FROM files WHERE name == ? AND folderID_ref == ?;";

	(*lastModified) = -1;

	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_getFileToSyncLastModified: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getFileToSyncLastModified: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;

	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFileToSyncLastModified: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, file.name,
			strlen(file.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFileToSyncLastModified: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFileToSyncLastModified: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*lastModified = 0;
		error_print(
				"DBConnection_getFileToSyncLastModified: file not found on DB\n");
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*lastModified = strtoull(
				(const char*) sqlite3_column_text(statement, 0), NULL, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getFileToSyncLastModified: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}


//aggiorna l'ultima modifica sul file
int DBConnection_updateFileToSyncLastModified(sqlite3* conn, fileToSync file,
		int folderToSyncID, unsigned long long lastModified) {
	int error;
	int queryLen;
	char tempText[TEMP_STRING_LENGTH];
	sqlite3_stmt* statement;
	const char* query = "UPDATE files SET lastModified = ?"
			"WHERE folderID_ref == ? AND name == ?;";

	//controllo che i parametri passati siano validi
	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_updateFileToSyncLastModified: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileToSyncLastModified: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	sprintf(tempText, "%llu", lastModified);
	error = sqlite3_bind_text(statement, 1, tempText, strlen(tempText) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileToSyncLastModified: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileToSyncLastModified: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 3, file.name,
			strlen(file.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileToSyncLastModified: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateFileToSyncLastModified: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//cancello tutti i file per una cartella
int DBConnection_deleteFilesForFolder(sqlite3* conn, int folderToSyncID) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "DELETE FROM files WHERE folderID_ref == ?;";

	//controllo che i parametri passati siano validi
	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_deleteFilesForFolder: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFilesForFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFilesForFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_deleteFilesForFolder: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//restituisce i file il cui nome relativo inizia per relativeSubfoldePath (allocazione dinamica)
int DBConnection_getFilenamesToDeleteFromSubfolderPath(sqlite3* conn,
		int folderToSyncID, char *relativeSubfolderPath, char ***fileNames,
		int *numFiles) {

	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* queryCount =
			"SELECT COUNT(*) FROM files WHERE folderID_ref == ? AND name LIKE ?;";

	const char* queryData =
			"SELECT name FROM files WHERE folderID_ref == ? AND name LIKE ?;";

	int i = 0;
	char currentName[FILETOSYNC_LENGTH];	//note: this contains file names...!!!

	*fileNames = NULL;
	*numFiles = 0;

	//controllo che i parametri passati siano validi
	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getFilenamesToDeleteFromSubfolderPath: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	strcpy(currentName, relativeSubfolderPath);
	strcat(currentName, "%");

	error = sqlite3_bind_text(statement, 2, currentName,
			strlen(currentName) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database (non dovrebbe capitare)
		*numFiles = 0;
		*fileNames = NULL;
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error, no data found on database\n");
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numFiles = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//allocaz memoria necessaria...
	*fileNames = (char**) malloc(sizeof(char*) * (*numFiles));
	for (i = 0; i < *numFiles; i++)
		(*fileNames)[i] = (char*) malloc(sizeof(char) * FILETOSYNC_LENGTH);

	//---------------------------------------------------------------------
	//queryData
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		for (i = 0; i < *numFiles; i++)
			free((*fileNames)[i]);
		free(*fileNames);
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		for (i = 0; i < *numFiles; i++)
			free((*fileNames)[i]);
		free(*fileNames);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, currentName,
			strlen(currentName) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		for (i = 0; i < *numFiles; i++)
			free((*fileNames)[i]);
		free(*fileNames);
		return DB_BIND_ERROR;
	}

	for (i = 0; i < *numFiles; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print("DBConnection_getFilenamesToDeleteFromSubfolderPath: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			for (i = 0; i < *numFiles; i++)
				free((*fileNames)[i]);
			free(*fileNames);
			return DB_STEP_ERROR;
		}
		//debug_print(DEBUG_OFF, "%s\n", (const char*) sqlite3_column_text(statement, 0));
		strcpy(currentName, (const char*) sqlite3_column_text(statement, 0));
		strcpy((*fileNames)[i], currentName);
		//debug_print(DEBUG_OFF, "%s\n", (*fileNames)[i]);
	}
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//restituisce il numero di sincronizzazione sul file
int DBConnection_getNumSyncStatusOnFiles(sqlite3 *conn, int folderToSyncID,
		char *filename, int *numSyncStatusOnFile) {

	int error;

	sqlite3_stmt* statement;

	int queryLen;
	const char* queryCount =
			"SELECT COUNT(*) FROM files INNER JOIN fileSyncStates ON files._fileID = fileSyncStates.fileID_ref WHERE files.folderID_ref == ? AND files.name == ?;";

	//controllo che i parametri passati siano validi
	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getNumSyncStatusOnFiles: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSyncStatusOnFiles: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSyncStatusOnFiles: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, filename, strlen(filename) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSyncStatusOnFiles: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numSyncStatusOnFile = 0;
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numSyncStatusOnFile = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getNumSyncStatusOnFiles: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//compilazione della query
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//elimina un file dal database
int DBConnection_deleteFileFromDB(sqlite3* conn, int folderToSyncID,
		char *filename) {

	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"DELETE FROM files WHERE folderID_ref == ? AND name == ?;";

	//controllo che i parametri passati siano validi
	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_deleteFileFromDB: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFileFromDB: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFileFromDB: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, filename, strlen(filename) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFileFromDB: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_deleteFileFromDB: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//aggiorna lo stato DELETED o meno di un file
int DBConnection_updateFileDeleted(sqlite3* conn, fileToSync file,
		int folderToSyncID, FileDeletionState newState) {
	int error;
	int queryLen;
	int fileID;
	sqlite3_stmt* statement;
	const char* query = "UPDATE files SET deleted = ?"
			"WHERE _fileID == ?;";

	//controllo che i parametri passati siano validi
	if (!isFileToSyncValid(file)) {
		error_print(
				"DBConnection_updateFileDeleted: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}

	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_updateFileDeleted: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getFileToSyncID(conn, file, folderToSyncID, &fileID);
	if (error || fileID == 0) {
		error_print(
				"DBConnection_updateFileDeleted: error, unable to find the file on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileDeleted: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, newState);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileDeleted: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, fileID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateFileDeleted: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateFileDeleted: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//recupera dal database le informazioni sul file (lastModified e deleted)
int DBConnection_getFileInfo(sqlite3 *conn, int folderToSyncID, fileToSync *file){

	const char* queryData =
			"SELECT lastModified, deleted "
					"FROM files "
					"WHERE folderID_ref == ? AND name == ?;";

	int queryLen;
	int error;
	sqlite3_stmt* statement;

	//check parametri
	if (!isFileToSyncValid(*file)) {
		error_print("DBConnection_getFileInfo: error in file parameter\n");
		return DB_PARAMETER_ERROR;
	}
	if (folderToSyncID <= 0) {
		error_print(
				"DBConnection_getFileInfo: error in folderToSyncID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSyncStatusInfo: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSyncStatusInfo: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, file->name, strlen(file->name) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSyncStatusInfo: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//leggo la riga
	error = sqlite3_step(statement);
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_getSyncStatusInfo: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//mi popolo la struttura dati
	file->lastModified = strtoull(
			(const char*) sqlite3_column_text(statement, 0), NULL, 0);
	file->deleted = sqlite3_column_int(statement, 1);

	sqlite3_finalize(statement);
	return DB_SUCCESS;
}
