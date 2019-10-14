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
 * DBInterface_commands.c
 */
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/cmdList.h"
#include "../Model/command.h"
#include "DBInterface.h"
#include "../Model/definitions.h"

// -- Metodi di gestione dei comandi

//aggiunta di un comando sul db
int DBConnection_addCommand(sqlite3* conn, command cmd) {
	int error;
	int queryLen;
	int nodeID;
	int txLeft;
	unsigned long long nextTxTimestamp;
	char tempTextCreation[TEMP_STRING_LENGTH];
	char tempTextRetry[TEMP_STRING_LENGTH];
	sqlite3_stmt* statement;
	const char* query =
			"INSERT INTO "
					"commands(nodeID_ref, text, state,creationTimestamp,nextTxTimestamp, txLeft) "
					"VALUES (?,?,?,?,?,?);";

	//controllo che i parametri passati siano validi
	if (!isCommandValid(cmd)) {
		error_print("DBConnection_addCommand: error in cmd parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	if (cmd.state == CMD_PROCESSING) {
		error = DBConnection_getDtnNodeID(conn, cmd.msg.source, &nodeID);
		txLeft = -1;
		nextTxTimestamp = 0;
	} else {
		error = DBConnection_getDtnNodeID(conn, cmd.msg.destination, &nodeID);

		txLeft = cmd.msg.txLeft;
		nextTxTimestamp = cmd.msg.nextTxTimestamp;
	}

	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_addCommand: error, unable to find the node on database\n");
		return error;
	}
	else if(error){
		error_print("DBConnection_addCommand: error in DBConnection_getDtnNodeID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_text(statement, 2, cmd.text, strlen(cmd.text) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 3, cmd.state);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	strcpy(tempTextCreation, "");
	sprintf(tempTextCreation, "%llu", cmd.creationTimestamp);
	error = sqlite3_bind_text(statement, 4, tempTextCreation,
			strlen(tempTextCreation) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	strcpy(tempTextRetry, "");
	sprintf(tempTextRetry, "%llu", nextTxTimestamp);
	error = sqlite3_bind_text(statement, 5, tempTextRetry,
			strlen(tempTextRetry) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 6, txLeft);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addCommand: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print("DBConnection_addCommand: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//controlla se esiste una comando sul database
int DBConnection_isCommandOnDb(sqlite3* conn, command cmd, int* result) {
	int error;
	int queryLen;
	int nodeID;
	sqlite3_stmt* statement;
	const char* query = "SELECT _commandID FROM commands "
			"WHERE nodeID_ref == ? AND text == ?;";

	*result = 0;

	//controllo parametri
	if (!isCommandValid(cmd)) {
		error_print("DBConnection_isCommandOnDb: error in cmd parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	if (cmd.state == CMD_PROCESSING) {
		error = DBConnection_getDtnNodeID(conn, cmd.msg.source, &nodeID);
	} else {
		error = DBConnection_getDtnNodeID(conn, cmd.msg.destination, &nodeID);
	}

	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_isCommandOnDb: error, unable to find the node on database\n");
		return error;
	}else if(error){
		error_print("DBConnection_isCommandOnDb: error in DBConnection_getDtnNodeID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isCommandOnDb: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isCommandOnDb: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, cmd.text, strlen(cmd.text) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isCommandOnDb: error in sqlite3_bind_text(): %s\n",
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
		*result = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_isCommandOnDb: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//aggiorna lo stato di un comando sul database, utilizzato negli ack
int DBConnection_updateCommandState(sqlite3* conn, dtnNode node, char* text,
		CmdState newState) {
	int error;
	int queryLen;
	int nodeID;
	sqlite3_stmt* statement;
	const char* query = "UPDATE commands SET state = ?"
			"WHERE nodeID_ref == ? AND text == ?;";

	//controllo che i parametri passati siano validi
	if (!isDtnNodeValid(node)) {
		error_print(
				"DBConnection_updateCommandState: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getDtnNodeID(conn, node, &nodeID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_updateCommandState: error, unable to find the node on database\n");
		return error;	//utile se error == DB_DATA_NOT_FOUND_ERROR
	}else if(error){
		error_print("DBConnection_updateCommandState: error in DBConnection_getDtnNodeID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandState: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, newState);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandState: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandState: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 3, text, strlen(text) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandState: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateCommandState: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//recupero il numero di trasmissioni di un comando
int DBConnection_getCommandTxLeft(sqlite3* conn, dtnNode node, char* text,
		int* txLeft) {
	int error;
	int queryLen;
	int nodeID;
	sqlite3_stmt* statement;
	const char* query = "SELECT txLeft FROM commands "
			"WHERE nodeID_ref == ? AND text == ? ;";

	*txLeft = -1;

	if (!isDtnNodeValid(node)) {
		error_print("DBConnection_getCommandTxLeft: error in parameter node\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero parametri
	error = DBConnection_getDtnNodeID(conn, node, &nodeID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_getCommandTxLeft: error, unable to find the node on database\n");
		return error;
	}else if(error){
		error_print("DBConnection_getCommandTxLeft: error in DBConnection_getDtnNodeID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandTxLeft: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandTxLeft: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, text, strlen(text) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandTxLeft: error in sqlite3_bind_text(): %s\n",
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
				"DBConnection_getCommandTxLeft: error, data not found on database\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*txLeft = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getCommandTxLeft: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//aggiorna il numero di trasmissioni per comando e la data di prossima ritrasmissione
int DBConnection_updateCommandTxLeftAndRetryDate(sqlite3* conn, dtnNode node,
		char* text, int txLeft, unsigned long long nextTxTimestamp) {
	int error;
	int queryLen;
	int nodeID;
	char tempText[TEMP_STRING_LENGTH];
	sqlite3_stmt* statement;
	const char* query = "UPDATE commands SET txLeft = ?, nextTxTimestamp = ? "
			"WHERE nodeID_ref == ? AND text == ?;";

	//controllo che i parametri passati siano validi
	if (!isDtnNodeValid(node)) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//recupero i parametri necessari
	error = DBConnection_getDtnNodeID(conn, node, &nodeID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error, unable to find the node on database\n");
		return error;
	}else if(error){
		error_print("DBConnection_updateCommandTxLeftAndRetryDate: error in DBConnection_getDtnNodeID()\n");
		return error;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, txLeft);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error in sqlite3_bind_int(): %s\n",
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
				"DBConnection_updateCommandTxLeftAndRetryDate: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, nodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 4, text, strlen(text) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateCommandTxLeftAndRetryDate: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//restituisce tutti i comandi da ritrasmettere
int DBConnection_clearCommandsOlderThan(sqlite3* conn,
		unsigned long long limitDate) {
	int error;
	int queryLen;
	unsigned long long tempCreationTimestamp;
	sqlite3_stmt* statement;
	const char* query = "SELECT _commandID, creationTimestamp "
			"FROM commands WHERE (state == ? OR state == ?);";

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_clearCommandsOlderThan: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, CMD_CONFIRMED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_clearCommandsOlderThan: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, CMD_PROCESSING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_clearCommandsOlderThan: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//ciclo sui dati
	while ((error = sqlite3_step(statement)) == SQLITE_ROW) {

		//controllo che il comando sia da cancellare
		tempCreationTimestamp = strtoull(
				(const char*) sqlite3_column_text(statement, 1), NULL, 0);
		if (tempCreationTimestamp < limitDate) {

			//procedo ad eliminarlo
			//debug_print(DEBUG_L1, "DBConnection_clearCommandsOlderThan: eliminazione comando ID %d\n",	sqlite3_column_int(statement, 0));
			error = DBConnection_deleteCommand(conn,
					sqlite3_column_int(statement, 0));
			if (error) {
				error_print(
						"DBConnection_clearCommandsOlderThan: error in DBConnection_deleteCommand(): %s\n",
						sqlite3_errmsg(conn));
				sqlite3_finalize(statement);
				return error;
			}
		}
	}
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_clearCommandsOlderThan: error in sqlite3_step()\n");
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//elimina il comando all'id specificato
int DBConnection_deleteCommand(sqlite3* conn, int commandID) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "DELETE FROM commands WHERE _commandID = ?;";

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteCommand: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, commandID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_deleteCommand: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//eseguo la query
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_deleteCommand: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//mette a failed tutti i comandi precedenti, da invocare alla ricezione dell'ack
int DBConnection_setPreviusPendingAsFailed(sqlite3* conn, dtnNode sourceNode,
		unsigned long long ackDate) {
	int error;
	int queryLen;
	unsigned long long tempCreationTimestamp;
	sqlite3_stmt* statement;
	const char* query = "SELECT _commandID, creationTimestamp, text "
			"FROM commands WHERE state == ?;";

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_setPreviusPendingAsFailed: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, CMD_PENDING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_setPreviusPendingAsFailed: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//ciclo sui dati
	while ((error = sqlite3_step(statement)) == SQLITE_ROW) {

		//controllo che il comando sia piuù vecchio dell'ack appena arrivato
		tempCreationTimestamp = strtoull(
				(const char*) sqlite3_column_text(statement, 1), NULL, 0);
		if (tempCreationTimestamp < ackDate) {

			//procedo ad assegnargli lo stato di FAILED
			error = DBConnection_updateCommandState(conn, sourceNode,
					(char*) sqlite3_column_text(statement, 2), CMD_FAILED);
			if (error) {
				error_print(
						"DBConnection_setPreviusPendingAsFailed: error in DBConnection_deleteCommand(): %s\n",
						sqlite3_errmsg(conn));
				sqlite3_finalize(statement);
				return error;
			}

			//azzero il numero di ritrasmissioni
			error = DBConnection_updateCommandTxLeftAndRetryDate(conn,
					sourceNode, (char*) sqlite3_column_text(statement, 2), 0,
					getNextRetryDate(sourceNode));
			if (error) {
				error_print(
						"DBConnection_setPreviusPendingAsFailed: error in DBConnection_deleteCommand(): %s\n",
						sqlite3_errmsg(conn));
				sqlite3_finalize(statement);
				return error;
			}
		}
	}
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_setPreviusPendingAsFailed: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//recupera tutti i comandi da trasmettere (o ritrasmettere) per il nodo destNode (alloca dinamicamente memoria)
int DBConnection_getCommandsToTransmitForDestNode(sqlite3 *conn,
		dtnNode localNode, dtnNode destNode, cmdList *cmdListReturn,
		unsigned long long currentTime) {

	int destNodeID;
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	command* tempCommand;

	char lluString[TEMP_STRING_LENGTH];

	const char* query =
			"SELECT creationTimestamp, nextTxTimestamp, text, txLeft, state "
					"FROM commands WHERE txLeft > 0 AND (state == ? OR (state == ? AND nextTxTimestamp < ?)) AND nodeID_ref == ?;";
					//" ORDER BY creationTimestamp ASC";
	*cmdListReturn = cmdList_create();

	//check parametri
	if (!isDtnNodeValid(localNode) || !isDtnNodeValid(destNode)
			|| currentTime <= 0) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in one or more parameters\n");
		return DB_PARAMETER_ERROR;
	}

	error = DBConnection_getDtnNodeID(conn, destNode, &destNodeID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: node not found\n");
		return error;
	}
	else if (error){
		error_print("DBConnection_getCommandsToTransmitForDestNode: error in DBConnection_getDtnNodeID()\n");
		return error;
	}
	//compilazione query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, CMD_DESYNCHRONIZED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, CMD_PENDING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	sprintf(lluString, "%llu", currentTime);
	error = sqlite3_bind_text(statement, 3, lluString, strlen(lluString)+1, SQLITE_STATIC);
	if(error != SQLITE_OK){
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	error = sqlite3_bind_int(statement, 4, destNodeID);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//ciclo sui dati
	while ((error = sqlite3_step(statement)) == SQLITE_ROW) {

		newCommand(&tempCommand, (char*) sqlite3_column_text(statement, 2));

		tempCommand->msg.destination = destNode;
		tempCommand->msg.source = localNode;
		tempCommand->creationTimestamp = strtoull(
				(const char*) sqlite3_column_text(statement, 0), NULL, 0);
		tempCommand->msg.nextTxTimestamp = strtoull(
				(const char*) sqlite3_column_text(statement, 1), NULL, 0);
		tempCommand->msg.txLeft = sqlite3_column_int(statement, 3);
		tempCommand->state = sqlite3_column_int(statement, 4);

		//aggiungo il comando alla lista
		*cmdListReturn = cmdList_add(*cmdListReturn, tempCommand);
	}
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_getCommandsToTransmitForDestNode: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);

		cmdList_destroy(*cmdListReturn);
		*cmdListReturn = cmdList_create();
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}
