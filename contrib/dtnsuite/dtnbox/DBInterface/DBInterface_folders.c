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
 * DBInterface_folders.c
 */
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/fileToSyncList.h"
#include "../Model/folderToSync.h"
#include "../Model/synchronization.h"
#include "DBInterface.h"

// -- Metodi di gestione cartelle da sincronizzare

//aggiunta cartella su database
int DBConnection_addFolderToSync(sqlite3* conn, folderToSync folder) {
	int error;
	int queryLen;
//	int lastRowID;
	sqlite3_stmt* statement;
	const char* query = "INSERT INTO folders(owner,name) VALUES (?,?);";

	if (!isFolderToSyncValid(folder)) {
		error_print("DBConnection_addFolderToSync: error in folder parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFolderToSync: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, folder.owner,
			strlen(folder.owner) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFolderToSync: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, folder.name,
			strlen(folder.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addFolderToSync: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_addFolderToSync: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo la chiave primaria di una cartella
int DBConnection_getFolderToSyncID(sqlite3* conn, folderToSync folder,
		int* folderToSyncID) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT _folderID FROM folders WHERE name == ? AND owner == ?;";

	*folderToSyncID = 0;

	if (!isFolderToSyncValid(folder)) {
		error_print("DBConnection_getFolderToSyncID: error in folder parameter\n");
		 return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFolderToSyncID: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, folder.name,
			strlen(folder.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFolderToSyncID: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, folder.owner,
			strlen(folder.owner) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getFolderToSyncID: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*folderToSyncID = 0;
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*folderToSyncID = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print("DBConnection_getFolderToSyncID: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

/*
 * Restituisce tutte le cartelle (FolderToSync) su cui è attiva almeno una sincronizzazione.
 * Ricordarsi di fare la free dell'array di folders nel chiamante.
 */
int DBConnection_getAllFoldersToSync(sqlite3* conn, folderToSync** folders,
		int* numFolders) {
	int error;
	int i;
	int queryLen;
	folderToSync tempFolder;
	sqlite3_stmt* statement;
	const char* queryCount = "SELECT COUNT(DISTINCT folders._folderID) "
			"FROM folders INNER JOIN synchronizations "
			"ON synchronizations.folderID_ref = folders._folderID WHERE synchronizations.state == ?;";
	//"GROUP BY folders._folderID;";
	const char* queryData =
			"SELECT folders._folderID, folders.owner, folders.name "
					"FROM folders INNER JOIN synchronizations "
					"ON synchronizations.folderID_ref = folders._folderID WHERE synchronizations.state == ? "
					"GROUP BY folders._folderID;";

	*folders = NULL;
	*numFolders = 0;

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToSync: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToSync: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numFolders = 0;
		*folders = NULL;
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numFolders = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getAllFoldersToSync: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//ora che conosco il numero di sincronizzazioni istanzio l'array, sarà il chiamante a doversi occupare della free
	*folders = (folderToSync*) malloc(sizeof(folderToSync) * *numFolders);

	for (i = 0; i < *numFolders; i++)
		(*folders)[i].files = fileToSyncList_create();

	//procedo a leggere tutte le cartelle
	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToSync: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));

		free(*folders);
		*folders = NULL;
		*numFolders = 0;
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToSync: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		free(*folders);
		*folders = NULL;
		*numFolders = 0;
		return DB_BIND_ERROR;
	}

	//leggo i dati
	for (i = 0; i < *numFolders; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getAllFoldersToSync: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);

//			for (; i >= 0; i--)
//				fileToSyncList_destroy((*folders)[i].files);
			free(*folders);
			*folders = NULL;
			*numFolders = 0;
			return DB_STEP_ERROR;
		}

		tempFolder.files = NULL;
		strcpy(tempFolder.owner,
				(const char*) sqlite3_column_text(statement, 1));
		strcpy(tempFolder.name,
				(const char*) sqlite3_column_text(statement, 2));

		//assegno la carella alla lista
		(*folders)[i] = tempFolder;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//controlla se esiste una cartella sul database e salva il risultato nella variabile result
int DBConnection_isFolderOnDb(sqlite3* conn, folderToSync folderToCheck,
		int* result) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "SELECT _folderID FROM folders "
			"WHERE owner == ? AND name == ?;";

	*result = 0;

	//controllo parametri
	if (!isFolderToSyncValid(folderToCheck)) {
		error_print(
				"DBConnection_isFolderOnDb: error in folderToCheck parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isFolderOnDb: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, folderToCheck.owner,
			strlen(folderToCheck.owner) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isFolderOnDb: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, folderToCheck.name,
			strlen(folderToCheck.name) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isFolderOnDb: error in sqlite3_bind_text(): %s\n",
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
		error_print("DBConnection_isFolderOnDb: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//restituisce tutte le cartelle presenti sul database
//lazy, non carica i file
int DBConnection_getAllFolders(sqlite3* conn, folderToSync** folders,
		int* numFolders, char *currentUser) {
	int error;
	int i;
	int queryLen;
	folderToSync tempFolder;
	sqlite3_stmt* statement;
	const char* queryCount = "SELECT COUNT(*) FROM folders, synchronizations WHERE (synchronizations.folderID_ref == folders._folderID AND synchronizations.state == ?) || folders.owner == ?;";
	const char* queryData =
			"SELECT _folderID, owner, name FROM folders, synchronizations WHERE (synchronizations.folderID_ref == folders._folderID AND synchronizations.state == ?) || folders.owner == ?;";

	*folders = NULL;
	*numFolders = 0;

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	error = sqlite3_bind_int(statement, 1, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, currentUser,
			strlen(currentUser) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numFolders = 0;
		*folders = NULL;
		error_print(
				"DBConnection_getAllFolders: data not found on database\n");
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numFolders = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//ora che conosco il numero di sincronizzazioni istanzio l'array, sarà il chiamante a doversi occupare della free
	*folders = (folderToSync*) malloc(sizeof(folderToSync) * *numFolders);

	//procedo a leggere tutte le cartelle
	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));

		free(*folders);
		*folders = NULL;
		numFolders = 0;
		return DB_PREPARE_ERROR;
	}

	error = sqlite3_bind_int(statement, 1, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*folders);
		*folders = NULL;
		numFolders = 0;
		return DB_BIND_ERROR;
	}


	error = sqlite3_bind_text(statement, 2, currentUser,
			strlen(currentUser) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFolders: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*folders);
		*folders = NULL;
		numFolders = 0;
		return DB_BIND_ERROR;
	}

	//leggo i dati
	for (i = 0; i < *numFolders; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getAllFolders: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);

			free(*folders);
			*folders = NULL;
			numFolders = 0;
			return DB_STEP_ERROR;
		}

		//mi popolo la struttura dati
		tempFolder.files = NULL;
		strcpy(tempFolder.owner,
				(const char*) sqlite3_column_text(statement, 1));
		strcpy(tempFolder.name,
				(const char*) sqlite3_column_text(statement, 2));

		//assegno la carella alla lista
		(*folders)[i] = tempFolder;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//cancella una cartella e relativi files dal database
int DBConnection_deleteFolder(sqlite3* conn, folderToSync folder) {
	int error;
	int queryLen;
	int folderToSyncID;
	sqlite3_stmt* statement;
	const char* query = "DELETE FROM folders WHERE _folderID == ?;";

	if (!isFolderToSyncValid(folder)) {
		error_print("DBConnection_deleteFolder: errore parametro folder\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero l'id della cartella
	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error || folderToSyncID == 0) {
		error_print(
				"DBConnection_deleteFolder: error, unable to find the folder on database\n");
		return error;
	}

	//cancello tutti i file che ne fanno parte
	error = DBConnection_deleteFilesForFolder(conn, folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_deleteFolder: error in DBConnection_deleteFilesForFolder()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print("DBConnection_deleteFolder: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//Restituisce le cartelle da monitorare con inotify
int DBConnection_getAllFoldersToMonitor(sqlite3* conn, folderToSync** folders,
		int* numFolders, char *currentUser) {
	int error;
	int i;
	int queryLen;
	folderToSync tempFolder;
	sqlite3_stmt* statement;
	const char* queryCount =
			"SELECT COUNT(DISTINCT folders._folderID) "
					"FROM folders INNER JOIN synchronizations "
					"ON synchronizations.folderID_ref = folders._folderID WHERE (synchronizations.mode == ? OR folders.owner == ?) AND synchronizations.state == ?;";
	//"GROUP BY folders._folderID;";
	const char* queryData =
			"SELECT folders._folderID, folders.owner, folders.name "
					"FROM folders INNER JOIN synchronizations "
					"ON synchronizations.folderID_ref = folders._folderID WHERE (synchronizations.mode == ? OR folders.owner == ?) AND synchronizations.state == ? "
					"GROUP BY folders._folderID;";


	*folders = NULL;
	*numFolders =0;

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	error = sqlite3_bind_int(statement, 1, PUSH_AND_PULL_IN);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, currentUser,
			strlen(currentUser) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 3, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numFolders = 0;
		*folders = NULL;
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numFolders = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//ora che conosco il numero di sincronizzazioni istanzio l'array, sarà il chiamante a doversi occupare della free
	*folders = (folderToSync*) malloc(sizeof(folderToSync) * *numFolders);
	for(i=0; i< *numFolders; i++)
		(*folders)[i].files = fileToSyncList_create();

	//procedo a leggere tutte le cartelle
	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));

		free(*folders);
		*folders=NULL;
		*numFolders=0;

		return DB_PREPARE_ERROR;
	}

	error = sqlite3_bind_int(statement, 1, PUSH_AND_PULL_IN);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		free(*folders);
		*folders=NULL;
		*numFolders=0;

		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, currentUser,
			strlen(currentUser) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		free(*folders);
		*folders=NULL;
		*numFolders=0;

		return DB_BIND_ERROR;
	}


	error = sqlite3_bind_int(statement, 3, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllFoldersToMonitor: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		free(*folders);
		*folders=NULL;
		*numFolders=0;

		return DB_BIND_ERROR;
	}

	//leggo i dati
	for (i = 0; i < *numFolders; i++) {


		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getAllFoldersToMonitor: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			for (; i >= 0; i--)
				fileToSyncList_destroy((*folders)[i].files);

			free(*folders);
			*folders=NULL;
			*numFolders=0;
			return DB_STEP_ERROR;
		}

		//mi popolo la struttura dati

		error = DBConnection_getFilesToSyncFromFolder(conn, &(tempFolder.files),
				sqlite3_column_int(statement, 0));
		if (error) {
			error_print(
					"DBConnection_getAllFoldersToMonitor: error in DBConnection_getFilesToSyncFromFolder(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			for (; i >= 0; i--)
				fileToSyncList_destroy((*folders)[i].files);
			free(*folders);
			*folders=NULL;
			*numFolders=0;

			return error;
		}

		strcpy(tempFolder.owner,
				(const char*) sqlite3_column_text(statement, 1));
		strcpy(tempFolder.name,
				(const char*) sqlite3_column_text(statement, 2));

		//assegno la carella alla lista
		(*folders)[i] = tempFolder;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//Restituisce il numero di sincronizzazioni sulla cartella
int DBConnection_getNumSynchronizationsOnFolder(sqlite3 *conn, folderToSync folder, int *numSyncs){

	int error;
	int folderToSyncID;
	int queryLen;
	sqlite3_stmt* statement;

	const char* queryCount =
			"SELECT COUNT(*) FROM synchronizations WHERE folderID_ref == ? AND state == ? AND mode != ?;";


	if (!isFolderToSyncValid(folder)) {
		error_print("DBConnection_getNumSynchronizationsOnFolder: error in folder parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero l'informazione che mi serve
	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error, unable to find the folder on database\n");
		return error;
	} else if (error) {
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error in DBConnection_getFolderToSyncID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, PULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numSyncs = 0;
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error, no synchronization found\n");
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numSyncs = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getNumSynchronizationsOnFolder: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//compilazione della query
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}
