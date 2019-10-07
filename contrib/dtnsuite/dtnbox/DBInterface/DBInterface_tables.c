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
 * DBInterface_tables.c
 */

#include <pthread.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/definitions.h"
#include "DBInterface.h"


static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return DB_SUCCESS;
}


// -- Metodi per la creazione/distruzione di tutte le tabelle

int DBConnection_createTables(sqlite3* conn) {
	int error;
	char* query;
	char* errorMsg;
	const char* data = "Chiamata funzione callback";

	//procedo a creare le tabelle necessarie al programma
	debug_print(DEBUG_L1, "DBConnection_createTables: creating tables...\n");

	//creo tabella cartelle da monitorare
	query = "CREATE TABLE folders("
			"_folderID		INTEGER	PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"name			TEXT NOT NULL,"
			"owner			TEXT NOT NULL,"
			"UNIQUE (owner, name));";

	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_createTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_createTables: FOLDERS table successfully created!\n");
	}

	//creo tabella nodi dtn
	query = "CREATE TABLE nodes("
			"_nodeID	INTEGER	PRIMARY KEY AUTOINCREMENT NOT NULL,"
			"EID		TEXT NOT NULL,"
			"lifetime	INTEGER NOT NULL,"
			"numTx		INTEGER	NOT NULL,"
			"blackWhite INTEGER NOT NULL,"
			"blockedBy	INTEGER NOT NULL,"
			"frozen 	INTEGER NOT NULL,"
			"UNIQUE (EID));";

	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_createTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_createTables: NODES table successfully created!\n");
	}

	//creo tabella sincronizzazioni
	query =
			"CREATE TABLE synchronizations("
					"_synchronizationID	INTEGER	PRIMARY KEY AUTOINCREMENT NOT NULL,"
					"folderID_ref		INTEGER	NOT NULL,"
					"nodeID_ref    		INTEGER	NOT NULL,"
					"mode            	INTEGER NOT NULL,"
					"state				INTEGER NOT NULL,"
					"pwdRead			TEXT,"
					"pwdWrite			TEXT,"
					"FOREIGN KEY(folderID_ref) REFERENCES folders(_folderID),"
					"FOREIGN KEY(nodeID_ref) REFERENCES nodes(_nodeID),"
					"UNIQUE (folderID_ref, nodeID_ref));";

	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_createTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_createTables: SYNCHRONIZATIONS table successfully created!\n");
	}

	//creo tabella files da monitorare
	query =
			"CREATE TABLE files("
					"_fileID			INTEGER	PRIMARY KEY	AUTOINCREMENT NOT NULL,"
					"folderID_ref		INTEGER NOT NULL,"
					"name				TEXT NOT NULL,"
					"lastModified		TEXT NOT NULL,"	//TODO can be int?
					"deleted			INTEGER NOT NULL,"
					"FOREIGN KEY(folderID_ref) REFERENCES folders(_folderID),"
					"UNIQUE (folderID_ref, name));";

	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_createTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_createTables: FILES table successfully created!\n");
	}

	query = "CREATE TABLE commands("
			"_commandID			INTEGER	PRIMARY KEY	AUTOINCREMENT NOT NULL,"
			"nodeID_ref			INTEGER NOT NULL,"
			"text	 			TEXT NOT NULL,"
			"state				INTEGER NOT NULL,"
			"creationTimestamp	TEXT NOT NULL,"	//TODO can be int?
			"nextTxTimestamp	TEXT NOT NULL,"	//TODO can be int?
			"txLeft				INTEGER NOT NULL,"
			"FOREIGN KEY(nodeID_ref) REFERENCES nodes(_nodeID),"
			"UNIQUE (text, nodeID_ref));";

	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_createTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_createTables: COMMANDS table successfully created!\n");
	}

	//creo la tabella che lega i file alle soncronizzazioni
	query =
			"CREATE TABLE fileSyncStates("
					"_fileSyncStateID		INTEGER	PRIMARY KEY	AUTOINCREMENT NOT NULL,"
					"fileID_ref				INTEGER NOT NULL,"
					"synchronizationID_ref	INTEGER NOT NULL,"
					"state					INTEGER NOT NULL,"
					"nextTxTimestamp		TEXT NOT NULL,"	//TODO can be int?
					"txLeft					INTEGER NOT NULL,"
					"FOREIGN KEY(synchronizationID_ref) REFERENCES synchronization(_synchronizationID),"
					"FOREIGN KEY(fileID_ref) REFERENCES files(_fileID),"
					"UNIQUE (synchronizationID_ref, fileID_ref));";

	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_createTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_createTables: SYNCSTATUS table successfully created!\n");
	}

	//tutto ok
	return DB_SUCCESS;
}

int DBConnection_dropTables(sqlite3* conn) {
	int error;
	char* query;
	char* errorMsg;
	const char* data = "Chiamata funzione callback";

	//procedo a eliminare le tabelle
	debug_print(DEBUG_L1, "DBConnection_dropTables: dropping tables...\n");

	//elimino tabella files da monitorare
	query = "DROP TABLE files;";
	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_dropTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_dropTables: FILES table deleted successfully\n");
	}

	//elimino tabella comandi
	query = "DROP TABLE commands;";
	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_dropTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_dropTables: COMMANDS table deleted successfully!\n");
	}
	//elimino tabella sincronizzazioni
	query = "DROP TABLE synchronizations;";
	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_dropTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_dropTables: SYNCHRONIZATIONS table deleted successfully!\n");
	}

	//elimino tabella cartelle
	query = "DROP TABLE folders;";
	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_dropTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_dropTables: FOLDERS table deleted successfully!\n");
	}

	//elimino tabella nodi dtn
	query = "DROP TABLE nodes;";
	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_dropTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_dropTables: NODES table deleted successfully!\n");
	}

	//elimino tabella syncStatus
	query = "DROP TABLE fileSyncStates;";
	error = sqlite3_exec(conn, query, callback, (void*) data, &errorMsg);
	if (error != SQLITE_OK) {
		error_print( "DBConnection_dropTables: error %s\n", errorMsg);
		sqlite3_free(errorMsg);
		return DB_EXEC_ERROR;
	} else {
		debug_print(DEBUG_L1, "DBConnection_dropTables: SYNCSTATUS table deleted successfully!\n");
	}

	//tutto ok
	return DB_SUCCESS;
}

//metodo di debug che stampa su file l'intero database
int DBConnection_dumpDBData(sqlite3* conn) {
	int error;
	int i;
	int queryLen;
	char filePath[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];
	char tempString[2048];
	FILE *fp;
	sqlite3_stmt* statement;
	char* query;

	//mi costruisco il file in ~/DTNbox
	getHomeDir(filePath);
	strcat(filePath, DBDUMPFILENAME);

	fp = fopen(filePath, "w");
	if (fp == NULL) {
		error_print( "DBConnection_dumpDBData: error in fopen() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//per ogni tabella del database stampo sul file i valori

	//FOLDERS

	strcpy(tempString, "--TABLE FOLDERS\n\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	sprintf(tempString, "|%30s|%30s|%30s\n", "_folderID", "name",
			"owner");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	strcpy(tempString, "");
	for (i = 0; i < ((30 * 3) + 4); i++) {
		strcat(tempString, "-");
	}
	strcat(tempString, "\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//ciclo su tutti i dati
	query = "SELECT _folderID, name, owner FROM folders;";
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//stamp dato per dato
		sprintf(tempString, "|%30d|%30s|%30s\n",
				sqlite3_column_int(statement, 0),
				sqlite3_column_text(statement, 1),
				sqlite3_column_text(statement, 2));
		error = fputs(tempString, fp);
		if (error == EOF) {
			error_print(
					"DBConnection_dumpDBData: error in fputs() on file %s\n",
					filePath);
			sqlite3_finalize(statement);
			return DB_DUMP_DB_ON_FILE_ERROR;
		}

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);

	// NODES

	strcpy(tempString, "\n\n--TABLE NODES\n\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	sprintf(tempString, "|%30s|%30s|%30s|%30s|%30s|%30s|%30s\n", "_nodeID", "EID", "lifetime",
			"numTx", "blackWhite", "blockedBy", "frozen");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	strcpy(tempString, "");
	for (i = 0; i < ((30 * 7) + 8); i++) {
		strcat(tempString, "-");
	}
	strcat(tempString, "\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//ciclo su tutti i dati
	query = "SELECT  _nodeID, EID, lifetime, numTx, blackWhite, blockedBy, frozen FROM nodes;";
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//stamp dato per dato
		sprintf(tempString, "|%30d|%30s|%30d|%30d|%30d|%30d|%30d\n",
				sqlite3_column_int(statement, 0),
				sqlite3_column_text(statement, 1),
				sqlite3_column_int(statement, 2),
				sqlite3_column_int(statement, 3),
				sqlite3_column_int(statement, 4),
				sqlite3_column_int(statement, 5),
				sqlite3_column_int(statement, 6));

		error = fputs(tempString, fp);
		if (error == EOF) {
			error_print(
					"DBConnection_dumpDBData: error in fputs() on file %s\n",
					filePath);
			sqlite3_finalize(statement);
			return DB_DUMP_DB_ON_FILE_ERROR;
		}

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);

	// SYNCHRONIZATIONS

	strcpy(tempString, "\n\n--TABLE SYNCHRONIZATIONS\n\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	sprintf(tempString, "|%30s|%30s|%30s|%30s|%30s|%30s|%30s\n",
			"_synchronizationID", "folderID_ref", "nodeID_ref", "mode", "state",
			"pwdRead", "pwdWrite");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	strcpy(tempString, "");
	for (i = 0; i < ((30 * 7) + 8); i++) {
		strcat(tempString, "-");
	}
	strcat(tempString, "\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//ciclo su tutti i dati
	query =
			"SELECT  _synchronizationID,folderID_ref, nodeID_ref,mode, state, pwdRead, pwdWrite FROM synchronizations;";
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//stamp dato per dato
		sprintf(tempString, "|%30d|%30d|%30d|%30d|%30d|%30s|%30s\n",
				sqlite3_column_int(statement, 0),
				sqlite3_column_int(statement, 1),
				sqlite3_column_int(statement, 2),
				sqlite3_column_int(statement, 3),
				sqlite3_column_int(statement, 4),
				sqlite3_column_text(statement, 5),
				sqlite3_column_text(statement, 6));
		error = fputs(tempString, fp);
		if (error == EOF) {
			error_print(
					"DBConnection_dumpDBData: error in fputs() on file %s\n",
					filePath);
			sqlite3_finalize(statement);
			return DB_DUMP_DB_ON_FILE_ERROR;
		}

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);

	// FILES

	strcpy(tempString, "\n\n--TABLE FILES\n\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	sprintf(tempString, "|%30s|%30s|%30s|%30s|%30s\n", "_fileID",
			"folderID_ref", "name", "lastModified", "deleted");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	strcpy(tempString, "");
	for (i = 0; i < ((30 * 5) + 6); i++) {
		strcat(tempString, "-");
	}
	strcat(tempString, "\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//ciclo su tutti i dati
	query =
			"SELECT _fileID, folderID_ref, name, lastModified, deleted FROM files;";
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//stamp dato per dato
		sprintf(tempString, "|%30d|%30d|%30s|%30s|%30d\n",
				sqlite3_column_int(statement, 0),
				sqlite3_column_int(statement, 1),
				sqlite3_column_text(statement, 2),
				sqlite3_column_text(statement, 3),
				sqlite3_column_int(statement, 4));

		error = fputs(tempString, fp);
		if (error == EOF) {
			error_print(
					"DBConnection_dumpDBData: error in fputs() on file %s\n",
					filePath);
			sqlite3_finalize(statement);
			return DB_DUMP_DB_ON_FILE_ERROR;
		}

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);


	strcpy(tempString, "\n\n--TABLE COMMANDS\n\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	sprintf(tempString, "|%30s|%30s|%30s|%30s|%30s|%30s|%30s\n", "_commandID",
			"nodeID_ref", "state", "creationTimestamp", "nextTxTimestamp",
			"txLeft", "text");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	strcpy(tempString, "");
	for (i = 0; i < ((30 * 7) + 8); i++) {
		strcat(tempString, "-");
	}
	strcat(tempString, "\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//ciclo su tutti i dati
	query =
			"SELECT _commandID,nodeID_ref, state,creationTimestamp,nextTxTimestamp,txLeft, text FROM commands;";
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//stamp dato per dato
		sprintf(tempString, "|%30d|%30d|%30d|%30s|%30s|%30d|%30s\n",
				sqlite3_column_int(statement, 0),
				sqlite3_column_int(statement, 1),
				sqlite3_column_int(statement, 2),
				sqlite3_column_text(statement, 3),
				sqlite3_column_text(statement, 4),
				sqlite3_column_int(statement, 5),
				sqlite3_column_text(statement, 6));
		error = fputs(tempString, fp);
		if (error == EOF) {
			error_print(
					"DBConnection_dumpDBData: error in fputs() on file %s\n",
					filePath);
			sqlite3_finalize(statement);
			return DB_DUMP_DB_ON_FILE_ERROR;
		}

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);

	// SYNCSTATUS

	strcpy(tempString, "\n\n--TABLE FILESYNCSTATES\n\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	sprintf(tempString, "|%30s|%30s|%30s|%30s|%30s|%30s\n",
			"_fileSyncStateID", "synchronizationID_ref", "fileID_ref", "state",
			"nextTxTimestamp", "txLeft");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	strcpy(tempString, "");
	for (i = 0; i < ((30 * 6) + 7); i++) {
		strcat(tempString, "-");
	}
	strcat(tempString, "\n");
	error = fputs(tempString, fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fputs() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}

	//ciclo su tutti i dati
	query =
			"SELECT _fileSyncStateID,synchronizationID_ref,fileID_ref,state,nextTxTimestamp,txLeft FROM fileSyncStates;";
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}
	error = sqlite3_step(statement);
	while (error == SQLITE_ROW) {
		//stamp dato per dato
		sprintf(tempString, "|%30d|%30d|%30d|%30d|%30s|%30d\n",
				sqlite3_column_int(statement, 0),
				sqlite3_column_int(statement, 1),
				sqlite3_column_int(statement, 2),
				sqlite3_column_int(statement, 3),
				sqlite3_column_text(statement, 4),
				sqlite3_column_int(statement, 5));
		error = fputs(tempString, fp);
		if (error == EOF) {
			error_print(
					"DBConnection_dumpDBData: error in fputs() on file %s\n",
					filePath);
			sqlite3_finalize(statement);
			return DB_DUMP_DB_ON_FILE_ERROR;
		}

		//rieseguo la step
		error = sqlite3_step(statement);
	}
	if (error == SQLITE_ERROR) {
		error_print(
				"DBConnection_dumpDBData: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	sqlite3_finalize(statement);

	//chiudo il file
	error = fclose(fp);
	if (error == EOF) {
		error_print( "DBConnection_dumpDBData: error in fclose() on file %s\n",
				filePath);
		return DB_DUMP_DB_ON_FILE_ERROR;
	}
	return DB_SUCCESS;
}
