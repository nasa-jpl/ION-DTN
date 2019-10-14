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
 * DBInterface_synchronizations.c
 */
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/folderToSync.h"
#include "../Model/synchronization.h"
#include "DBInterface.h"

// -- Metodi di gestione delle sincronizzazioni

//aggiunta di una sincronizzazione sul database
int DBConnection_addSynchronization(sqlite3* conn, synchronization sync) {
	int error;
	int queryLen;
	int nodeID;
	int folderToSyncID;
	sqlite3_stmt* statement;
	const char* query =
			"INSERT INTO "
					"synchronizations(folderID_ref,mode,state,nodeID_ref,pwdRead,pwdWrite) "
					"VALUES (?,?,?,?,?,?);";

	//controllo che i parametri passati siano validi
	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_addSynchronization: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getDtnNodeID(conn, sync.node, &nodeID);
	if (error != 0) {
		error_print(
				"DBConnection_addSynchronization: error, unable to find the node on database\n");
		return error;
	}
	error = DBConnection_getFolderToSyncID(conn, sync.folder, &folderToSyncID);
	if (error != 0) {
		error_print(
				"DBConnection_addSynchronization: error, unable to find the folder on database\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, sync.mode);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, sync.state);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 4, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 5, sync.pwdRead,
			strlen(sync.pwdRead) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 6, sync.pwdWrite,
			strlen(sync.pwdWrite) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_addSynchronization: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo l'id sul database di una sincronizzazione
int DBConnection_getSynchronizationID(sqlite3* conn, synchronization sync,
		int* synchronizationID) {
	int error;
	int queryLen;
	int folderToSyncID;
	int nodeID;
	sqlite3_stmt* statement;
	const char* query = "SELECT _synchronizationID "
			"FROM synchronizations "
			"WHERE folderID_ref == ? AND nodeID_ref == ?;";

	*synchronizationID=0;

	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_getSynchronizationID: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri
	error = DBConnection_getFolderToSyncID(conn, sync.folder, &folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationID: error, unable to find the folder ID\n");
		return error;
	}
	error = DBConnection_getDtnNodeID(conn, sync.node, &nodeID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationID: error, unable to find the node ID\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationID: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationID: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationID: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*synchronizationID = 0;
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*synchronizationID = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getSynchronizationID: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//elimina una sincronizzazione dal database
int DBConnection_deleteSynchronization(sqlite3* conn, synchronization sync) {
	int error;
	int queryLen;
	int synchronizationID;
	sqlite3_stmt* statement;
	const char* query =
			"DELETE FROM synchronizations WHERE _synchronizationID == ?;";

	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_deleteSynchronization: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}


	//posso procedere ad eliminare la sincronizzazione
	//recupero i parametri necessari
	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if (error) {
		error_print(
				"DBConnection_deleteSynchronization: error, unable to find the folder on database\n");
		return error;
	}

	//cancello le entry all'interno del sync state
	error = DBConnection_deleteSyncOnAllFiles(conn, sync);
	if (error) {
		error_print(
				"DBConnection_deleteSynchronization: error in DBConnection_deleteSyncOnAllFiles()\n");
		return error;
	}


	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSynchronization: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteSynchronization: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_deleteSynchronization: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//malloc: data una cartella, restituisce in un array l'insieme delle sincronizzazioni non in PULL legate alla FolderToSync indicata
int DBConnection_getSynchronizationsOnFolder(sqlite3* conn, folderToSync folder,
		synchronization** syncs, int* numSyncs) {
	int error;
	int i;
	int queryLen;
	int folderToSyncID;
	int tempID;
	synchronization tempSync;
	sqlite3_stmt* statement;
	const char* queryCount = "SELECT COUNT(*) "
			"FROM synchronizations "
			"WHERE folderID_ref == ? AND mode != ? AND state == ?;";
	const char* queryData = "SELECT mode, nodeID_ref, pwdRead, pwdWrite "
			"FROM synchronizations "
			"WHERE folderID_ref == ? AND mode != ? AND state == ?;";

	(*syncs) = NULL;
	*numSyncs = 0;

	if(!isFolderToSyncValid(folder)){
		error_print("DBConnection_getSynchronizationsOnFolder: error in folder parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero l'informazione che mi serve
	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: folder not found\n");
		return error;
	} else if (error) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in DBConnection_getFolderToSyncID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, PULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numSyncs = 0;
		*syncs = NULL;
		error_print(
				"DBConnection_getSynchronizationsOnFolder: no synchronization found\n");
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numSyncs = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print("DBConnection_getSynchronizationsOnFolder: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//ora che conosco il numero di sincronizzazioni istanzio l'array, sarà il chiamante a doversi occupare della free
	*syncs = (synchronization*) malloc(sizeof(synchronization) * *numSyncs);
//	for(i=0; i<*numSyncs; i++)
//		(*syncs)[i].folder.files = fileToSyncList_create();

	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		free(*syncs);
		*syncs = NULL;
		*numSyncs = 0;
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*syncs);
		*syncs = NULL;
		*numSyncs = 0;
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, PULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*syncs);
		*syncs = NULL;
		*numSyncs = 0;
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, SYNCHRONIZATION_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*syncs);
		*syncs = NULL;
		*numSyncs = 0;
		return DB_BIND_ERROR;
	}

	//leggo i dati

	//lascio la cartella fuori dal ciclo tanto e' sempre quella
	tempSync.folder = folder;
	for (i = 0; i < *numSyncs; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getSynchronizationsOnFolder: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			free(*syncs);
			*syncs = NULL;
			*numSyncs = 0;
			return DB_STEP_ERROR;
		}

		//mi popolo la struttura dati
		tempSync.mode = sqlite3_column_int(statement, 0);

		//recupero il nodo
		tempID = sqlite3_column_int(statement, 1);
		error = DBConnection_getDtnNodeFromID(conn, &(tempSync.node), tempID);
		if (error) {
			error_print(
					"DBConnection_getSynchronizationsOnFolder: error, unable to find the node for the synchronization\n");
			sqlite3_finalize(statement);
			free(*syncs);
			*syncs = NULL;
			*numSyncs = 0;
			return error;
		}

		strcpy(tempSync.pwdRead,
				(const char*) sqlite3_column_text(statement, 2));
		strcpy(tempSync.pwdWrite,
				(const char*) sqlite3_column_text(statement, 3));

		//assegno la sincronizzazione
		(*syncs)[i] = tempSync;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//Data una cartella, restituisce in un array l'insieme delle sincronizzazioni legate alla cartella, comprese quelle in PULL.
//usata solo x la fine, per cui ha senso mandarle anche in caso di pending!!
int DBConnection_getAllSynchronizationsOnFolder(sqlite3* conn,
		folderToSync folder, synchronization** syncs, int* numSyncs) {
	int error;
	int i;
	int queryLen;
	int folderToSyncID;
	int tempID;
	synchronization tempSync;
	sqlite3_stmt* statement;
	const char* queryCount =
			"SELECT COUNT(*) FROM synchronizations WHERE folderID_ref == ?;";
	const char* queryData =
			"SELECT mode, nodeID_ref, pwdRead, pwdWrite FROM synchronizations WHERE folderID_ref == ?;";

	*syncs = NULL;
	*numSyncs = 0;

	//recupero l'informazione che mi serve
	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error, unable to find the folder on database\n");
		return error;
	} else if (error) {
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error in DBConnection_getFolderToSyncID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numSyncs = 0;
		*syncs = NULL;
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error, no synchronization found\n");
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numSyncs = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//ora che conosco il numero di sincronizzazioni istanzio l'array, sarà il chiamante a doversi occupare della free
	*syncs = (synchronization*) malloc(sizeof(synchronization) * *numSyncs);

	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));

		free(*syncs);
		*syncs = NULL;
		*numSyncs = 0;

		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getAllSynchronizationsOnFolder: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		free(*syncs);
		*syncs = NULL;
		*numSyncs = 0;
		return DB_BIND_ERROR;
	}


	//leggo i dati

	//lascio la cartella fuori dal ciclo tanto e' sempre quella
	tempSync.folder = folder;
	for (i = 0; i < *numSyncs; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getAllSynchronizationsOnFolder: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			free(*syncs);
			*syncs = NULL;
			*numSyncs = 0;
			return DB_STEP_ERROR;
		}

		//mi popolo la struttura dati
		tempSync.mode = sqlite3_column_int(statement, 0);

		//recupero il nodo
		tempID = sqlite3_column_int(statement, 1);
		error = DBConnection_getDtnNodeFromID(conn, &(tempSync.node), tempID);
		if (error) {
			error_print("DBConnection_getAllSynchronizationsOnFolder: error %s\n",
					error == DB_DATA_NOT_FOUND_ERROR ?
							"node not found" :
							"in DBConnection_getDtnNodeFromID()\n");
			sqlite3_finalize(statement);
			free(*syncs);
			*syncs = NULL;
			*numSyncs = 0;
			return error;
		}

		strcpy(tempSync.pwdRead,
				(const char*) sqlite3_column_text(statement, 2));
		strcpy(tempSync.pwdWrite,
				(const char*) sqlite3_column_text(statement, 3));

		//assegno la sincronizzazione
		(*syncs)[i] = tempSync;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//data una sync restituisce il suo stato
int DBConnection_getSynchronizationStatus(sqlite3 *conn, synchronization sync, SynchronizationState *state){

	int error;
	int queryLen;
	int folderToSyncID;
	int nodeID;
	sqlite3_stmt* statement;
	const char* query = "SELECT state "
			"FROM synchronizations "
			"WHERE folderID_ref == ? AND nodeID_ref == ?;";




	//recupero i parametri
	error = DBConnection_getFolderToSyncID(conn, sync.folder, &folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationStatus: error, unable to find the folder ID\n");
		return error;
	}
	error = DBConnection_getDtnNodeID(conn, sync.node, &nodeID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationStatus: error, unable to find the node ID\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatus: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatus: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatus: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*state = SYNCHRONIZATION_PENDING;
		error_print(
				"DBConnection_getSynchronizationStatus: synchronization not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*state = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getSynchronizationStatus: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//data una sync restituisce la sua modalita
int DBConnection_getSynchronizationMode(sqlite3 *conn, synchronization sync,
		SyncMode *mode) {

	int error;

	int folderToSyncID;
	int nodeID;

	const char *query =
			"SELECT mode FROM synchronizations WHERE folderID_ref = ? AND nodeID_ref = ?";
	int queryLen;
	sqlite3_stmt *statement;


	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_getSynchronizationMode: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}


	error = DBConnection_getFolderToSyncID(conn, sync.folder, &folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationMode: error in DBConnection_getFolderToSyncID()\n");
		return error;
	}

	error = DBConnection_getDtnNodeID(conn, sync.node, &nodeID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationMode: error in DBConnection_getDtnNodeID()\n");
		return error;
	}

	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationMode: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationMode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 2, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationMode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*mode = -1;
		error_print(
				"DBConnection_getSynchronizationMode: synchronization not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*mode = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getSynchronizationMode: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}


//data una cartella e una modalità di sincronizzazione restituisce se esiste almeno una sincronizzazione con quella modalità
//da usare con mode == PULL || mode == PUSH_AND_PULL_INPUT
int DBConnection_hasSynchronizationMode(sqlite3* conn, folderToSync folder, SyncMode mode, int *result){

	int error;
	int queryLen;

	const char *query =
			"SELECT _synchronizationID FROM synchronizations WHERE folderID_ref == ? AND mode == ?";

	sqlite3_stmt *statement;
	int folderToSyncID;


	if(!isFolderToSyncValid(folder)){
		error_print("DBConnection_hasSynchronizationMode: error in folder parameter\n");
		return DB_PARAMETER_ERROR;
	}


	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_hasSynchronizationMode: error in DBConnection_getFolderToSyncID()\n");
		return error;
	}

	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error) {
		error_print(
				"DBConnection_hasSynchronizationMode: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_hasSynchronizationMode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 2, mode);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_hasSynchronizationMode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {

		*result=0;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database
		*result=1;
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_hasSynchronizationMode: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//data una sync ne aggiorna lo stato
int DBConnection_updateSynchronizationStatus(sqlite3 *conn, synchronization sync, SynchronizationState newStatus){

	int error;
	int synchronizationID;

	const char* query = "UPDATE synchronizations SET state = ?"
			"WHERE _synchronizationID == ?;";
	int queryLen;

	if (!isSynchronizationValid(sync)) {
		error_print(
				"DBConnection_updateSynchronizationStatus: error in sync parameter\n");
		return DB_PARAMETER_ERROR;
	}

	error = DBConnection_getSynchronizationID(conn, sync, &synchronizationID);
	if(error){
		error_print("DBConnection_updateSynchronizationStatus: error in DBConnection_getSynchronizationID()\n");
		return error;
	}

	sqlite3_stmt* statement;

	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSynchronizationStatus: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, newStatus);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSynchronizationStatus: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, synchronizationID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateSynchronizationStatus: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateSynchronizationStatus: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//Restituisce lo stato della sincronizzazione con il padre (ed è unica) su una data cartella
int DBConnection_getSynchronizationStatusWithParent(sqlite3 *conn, folderToSync folder, SynchronizationState *stateWithParent){


	int error;
	int queryLen;
	int folderToSyncID;
	sqlite3_stmt* statement;
	const char* query = "SELECT state "
			"FROM synchronizations "
			"WHERE folderID_ref == ? AND (mode == ? OR mode == ?);";


	if (!isFolderToSyncValid(folder)) {
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error in folder parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri
	error = DBConnection_getFolderToSyncID(conn, folder, &folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error, unable to find the folder ID\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, folderToSyncID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, PULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 3, PUSH_AND_PULL_IN);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*stateWithParent = SYNCHRONIZATION_PENDING;
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: synchronization with parent not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*stateWithParent = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getSynchronizationStatusWithParent: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;

}





