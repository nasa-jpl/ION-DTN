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
 * DBInterface_nodes.c
 */
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../Model/command.h"
#include "DBInterface.h"

// -- Metodi di gestione dtnNode

//aggiunta dtnNode su database
int DBConnection_addDtnNode(sqlite3* conn, dtnNode node) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"INSERT INTO nodes(EID,lifetime,numTx, blackWhite, blockedBy, frozen) VALUES (?,?,?,?,?,?);";

	if (!isDtnNodeValid(node)) {
		error_print( "DBConnection_addDtnNode: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, node.lifetime);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 3, node.numTx);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 4, node.blackWhite);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 5, node.blockedBy);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 6, node.frozen);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_addDtnNode: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo la chiave primaria di un nodo, 0 se il nodo non e' presente
int DBConnection_getDtnNodeID(sqlite3* conn, dtnNode node, int* nodeID) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "SELECT _nodeID FROM nodes WHERE EID == ? ;";

	*nodeID = -1;


	if (!isDtnNodeValid(node)) {
		error_print( "DBConnection_getDtnNodeID: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDtnNodeID: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDtnNodeID: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*nodeID = 0;
		error_print(
				"DBConnection_getDtnNodeID: error, data not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*nodeID = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getDtnNodeID: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo un nodo a partire dal suo EID
int DBConnection_getDtnNodeFromEID(sqlite3* conn, dtnNode* node, char* EIDToGet) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT EID,lifetime,numTx, blackWhite, blockedBy, frozen FROM nodes WHERE EID == ? ;";

	strcpy((*node).EID, "");
	(*node).lifetime=-1;
	(*node).numTx=-1;

	if (EIDToGet == NULL || !strcmp(EIDToGet, "")) {
		error_print(
				"DBConnection_getDtnNodeFromEID: error in EIDToGet parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDtnNodeFromEID: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, EIDToGet, strlen(EIDToGet) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDtnNodeFromEID: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è l'EID cercato (supposto univoco, inserire vincolo?)
		error_print(
				"DBConnection_getDtnNodeFromEID: error, node not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//leggo il contenuto della query e lo salvo nel nodo passato come puntatore
		strcpy(node->EID, (const char*) sqlite3_column_text(statement, 0));
		node->lifetime = sqlite3_column_int(statement, 1);
		node->numTx = sqlite3_column_int(statement, 2);

		node->blackWhite = sqlite3_column_int(statement, 3);
		node->blockedBy = sqlite3_column_int(statement, 4);
		node->frozen = sqlite3_column_int(statement, 5);

	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getDtnNodeFromEID: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//ricavo un nodo a partire dal suo ID
int DBConnection_getDtnNodeFromID(sqlite3* conn, dtnNode* node, int nodeIDToGet) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT EID, lifetime, numTx, blackWhite, blockedBy, frozen FROM nodes WHERE _nodeID == ? ;";


	strcpy((*node).EID, "");
	(*node).lifetime=-1;
	(*node).numTx=-1;

	//controllo parametri
	if (nodeIDToGet <= 0) {
		error_print(
				"DBConnection_getDtnNodeFromID: error in nodeIDToGet parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDtnNodeFromID: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, nodeIDToGet);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDtnNodeFromID: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è il nodo
		error_print(
				"DBConnection_getDtnNodeFromID: error, node not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//leggo il contenuto della query e lo salvo nel nodo passato come puntatore
		strcpy(node->EID, (const char*) sqlite3_column_text(statement, 0));
		node->lifetime = sqlite3_column_int(statement, 1);
		node->numTx = sqlite3_column_int(statement, 2);

		node->blackWhite = sqlite3_column_int(statement, 3);
		node->blockedBy = sqlite3_column_int(statement, 4);
		node->frozen = sqlite3_column_int(statement, 5);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getDtnNodeFromID: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//aggiorna il lifetime di un nodo
int DBConnection_updateDTNnodeLifetime(sqlite3* conn, dtnNode node,
		int lifetime) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "UPDATE nodes SET lifetime = ? WHERE EID == ?;";

	//controllo che i parametri passati siano validi
	if (!isDtnNodeValid(node)) {
		error_print(
				"DBConnection_updateDTNnodeLifetime: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeLifetime: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, lifetime);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeLifetime: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeLifetime: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateDTNnodeLifetime: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

//crea array di nodi per i quali c'è almeno un comando da trasmettere (o ritrasmettere)
int DBConnection_getNodesWithCommandsToTransmit(sqlite3 *conn, dtnNode **nodes,
		int *numNodes, unsigned long long currentDate) {

	const char * queryCount =
			"SELECT COUNT(DISTINCT nodeID_ref) FROM commands WHERE txLeft > 0 AND (state == ? OR (state == ? AND nextTxTimestamp < ?))";

	const char *queryData =
			"SELECT DISTINCT nodeID_ref FROM commands WHERE txLeft > 0 AND (state == ? OR (state == ? AND nextTxTimestamp < ?))";

	int error;
	int queryLen;
	sqlite3_stmt* statement;
	int i = 0;

	char lluString[TEMP_STRING_LENGTH];


	(*nodes) = NULL;
	(*numNodes) = 0;

	if (currentDate <= 0) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: currentDate not valid\n");
		return DB_PARAMETER_ERROR;
	}

	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, CMD_DESYNCHRONIZED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, CMD_PENDING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	sprintf(lluString, "%llu", currentDate);
	error = sqlite3_bind_text(statement, 3, lluString, strlen(lluString)+1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}


	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numNodes = 0;
		*nodes = NULL;
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numNodes = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}


	*nodes = (dtnNode*) malloc(sizeof(dtnNode) * *numNodes);

	//procedo a leggere tutti i nodi
	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		free(*nodes);
		(*nodes) = NULL;
		(*numNodes) = 0;
		return DB_PREPARE_ERROR;
	}

	//bind parametri
	error = sqlite3_bind_int(statement, 1, CMD_DESYNCHRONIZED);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*nodes);
		(*nodes) = NULL;
		(*numNodes) = 0;
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_int(statement, 2, CMD_PENDING);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*nodes);
		(*nodes) = NULL;
		(*numNodes) = 0;
		return DB_BIND_ERROR;
	}


	sprintf(lluString, "%llu", currentDate);
	error = sqlite3_bind_text(statement, 3, lluString, strlen(lluString)+1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		free(*nodes);
		(*nodes) = NULL;
		(*numNodes) = 0;
		return DB_BIND_ERROR;
	}

	for (i = 0; i < *numNodes; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getNodesWithCommandsToTransmit: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			free(*nodes);
			(*nodes) = NULL;
			(*numNodes) = 0;
			return DB_STEP_ERROR;
		}

		error = DBConnection_getDtnNodeFromID(conn, &((*nodes)[i]),
				sqlite3_column_int(statement, 0));
		if (error) {
			error_print(
					"DBConnection_getNodesWithCommandsToTransmit: error in DBConnection_getDtnNodeFromID()\n");
			sqlite3_finalize(statement);
			free(*nodes);
			(*nodes) = NULL;
			(*numNodes) = 0;
			return error;
		}
	}
	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}



int DBConnection_updateDTNnodeBlackWhite(sqlite3* conn, dtnNode node, blackWhite blackWhite){

	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "UPDATE nodes SET blackWhite = ? WHERE EID == ?;";

	//controllo che i parametri passati siano validi
	if (!isDtnNodeValid(node)) {
		error_print(
				"DBConnection_updateDTNnodeBlackWhite: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeBlackWhite: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, blackWhite);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeBlackWhite: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeBlackWhite: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateDTNnodeBlackWhite: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

int DBConnection_updateDTNnodeBlockedBy(sqlite3* conn, dtnNode node, blockedBy blockedBy){

	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "UPDATE nodes SET blockedBy = ? WHERE EID == ?;";

	//controllo che i parametri passati siano validi
	if (!isDtnNodeValid(node)) {
		error_print(
				"DBConnection_updateDTNnodeBlockedBy: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeBlockedBy: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, blockedBy);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeBlockedBy: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeBlockedBy: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateDTNnodeBlockedBy: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

int DBConnection_updateDTNnodeFrozen(sqlite3* conn, dtnNode node, frozen frozen){

	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "UPDATE nodes SET frozen = ? WHERE EID == ?;";

	//controllo che i parametri passati siano validi
	if (!isDtnNodeValid(node)) {
		error_print(
				"DBConnection_updateDTNnodeFrozen: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeFrozen: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_int(statement, 1, frozen);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeFrozen: error in sqlite3_bind_int(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}
	error = sqlite3_bind_text(statement, 2, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_updateDTNnodeFrozen: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error != SQLITE_DONE) {
		error_print(
				"DBConnection_updateDTNnodeFrozen: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}


int DBConnection_getDTNnodeBlackWhite(sqlite3* conn, dtnNode node, blackWhite *blackWhite){
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT blackWhite FROM nodes WHERE EID == ? ;";


	*blackWhite = -1;
	if (node.EID == NULL || !strcmp(node.EID, "")) {
		error_print(
				"DBConnection_getDTNnodeBlackWhite: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDTNnodeBlackWhite: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDTNnodeBlackWhite: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è l'EID cercato (supposto univoco, inserire vincolo?)
		error_print(
				"DBConnection_getDTNnodeBlackWhite: error, node not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//leggo il contenuto della query e lo salvo nel nodo passato come puntatore
		*blackWhite = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getDTNnodeBlackWhite: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

int DBConnection_getDTNnodeBlockedBy(sqlite3* conn, dtnNode node, blockedBy *blockedBy){
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT blockedBy FROM nodes WHERE EID == ? ;";

	*blockedBy = -1;

	if (node.EID == NULL || !strcmp(node.EID, "")) {
		error_print(
				"DBConnection_getDTNnodeBlockedBy: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDTNnodeBlockedBy: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDTNnodeBlockedBy: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è l'EID cercato (supposto univoco, inserire vincolo?)
		error_print(
				"DBConnection_getDTNnodeBlockedBy: error, node not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//leggo il contenuto della query e lo salvo nel nodo passato come puntatore
		*blockedBy = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getDTNnodeBlockedBy: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}

int DBConnection_getDTNnodeFrozen(sqlite3* conn, dtnNode node, frozen *frozen){
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query =
			"SELECT frozen FROM nodes WHERE EID == ? ;";

	*frozen = -1;

	if (node.EID == NULL || !strcmp(node.EID, "")) {
		error_print(
				"DBConnection_getDTNnodeFrozen: error in node parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDTNnodeFrozen: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, node.EID, strlen(node.EID) + 1,
	SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getDTNnodeFrozen: error in sqlite3_bind_text(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_BIND_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//se sono qui, query con successo ma non c'è l'EID cercato (supposto univoco, inserire vincolo?)
		error_print(
				"DBConnection_getDTNnodeFrozen: error, node not found\n");
		sqlite3_finalize(statement);
		return DB_DATA_NOT_FOUND_ERROR;
	} else if (error == SQLITE_ROW) {
		//leggo il contenuto della query e lo salvo nel nodo passato come puntatore
		*frozen = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getDTNnodeFrozen: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}
	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}




int DBConnection_isNodeOnDb(sqlite3* conn, char *EID, int* result) {
	int error;
	int queryLen;
	sqlite3_stmt* statement;
	const char* query = "SELECT _nodeID FROM nodes "
			"WHERE EID == ?;";

	(*result) = 0;

	//controllo parametri
	if (EID == NULL || strlen(EID) == 0) {
		error_print(
				"DBConnection_isNodeOnDb: error in EID parameter\n");
		return DB_PARAMETER_ERROR;
	}

	//compilazione della query
	queryLen = strlen(query) + 1;
	error = sqlite3_prepare_v2(conn, query, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isNodeOnDb: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//bind dei parametri
	error = sqlite3_bind_text(statement, 1, EID,
			strlen(EID) + 1, SQLITE_STATIC);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_isNodeOnDb: error in sqlite3_bind_text(): %s\n",
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
		error_print("DBConnection_isNodeOnDb: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	//deallocazione risorse
	sqlite3_finalize(statement);
	return DB_SUCCESS;
}


int DBConnection_getNodesToWhichSendFreezeOrUnfreeze(sqlite3* conn, dtnNode **nodes, int *numNodes){

	int error;
	int i;

	int queryLen;
	sqlite3_stmt* statement;

	//TODO: filtrare con un JOIN con la tabella nodes solo i nodi che non sono FROZEN
	//in quanto a un nodo frozen non dobbiamo inviare nulla!!
	const char * queryCount =
			"SELECT COUNT(DISTINCT nodeID_ref) FROM synchronizations";
	const char *queryData =
			"SELECT DISTINCT nodeID_ref FROM synchronizations";


	(*nodes) = NULL;
	(*numNodes) = 0;

	queryLen = strlen(queryCount) + 1;
	error = sqlite3_prepare_v2(conn, queryCount, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesToWhichSendFreeze: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		return DB_PREPARE_ERROR;
	}

	//esecuzione
	error = sqlite3_step(statement);
	if (error == SQLITE_DONE) {
		//Non ho trovato nessuna corrispondenza sul database
		*numNodes = 0;
		*nodes = NULL;
		sqlite3_finalize(statement);
		return DB_SUCCESS;
	} else if (error == SQLITE_ROW) {
		//Ho corrispondenza sul database, procedo a leggerne il contenuto
		*numNodes = sqlite3_column_int(statement, 0);
	} else if (error == SQLITE_ERROR) {
		//caso di errore
		error_print(
				"DBConnection_getNodesToWhichSendFreeze: error in sqlite3_step(): %s\n",
				sqlite3_errmsg(conn));
		sqlite3_finalize(statement);
		return DB_STEP_ERROR;
	}

	*nodes = (dtnNode*) malloc(sizeof(dtnNode) * *numNodes);

	//procedo a leggere tutti i nodi
	//compilazione della query
	sqlite3_finalize(statement);
	queryLen = strlen(queryData) + 1;
	error = sqlite3_prepare_v2(conn, queryData, queryLen, &statement, NULL);
	if (error != SQLITE_OK) {
		error_print(
				"DBConnection_getNodesToWhichSendFreeze: error in sqlite3_prepare_v2(): %s\n",
				sqlite3_errmsg(conn));
		free(*nodes);
		(*nodes) = NULL;
		(*numNodes) = 0;
		return DB_PREPARE_ERROR;
	}

	for (i = 0; i < *numNodes; i++) {

		//leggo la riga
		error = sqlite3_step(statement);
		if (error == SQLITE_ERROR) {
			error_print(
					"DBConnection_getNodesToWhichSendFreeze: error in sqlite3_step(): %s\n",
					sqlite3_errmsg(conn));
			sqlite3_finalize(statement);
			free(*nodes);
			(*nodes) = NULL;
			(*numNodes) = 0;
			return DB_STEP_ERROR;
		}

		error = DBConnection_getDtnNodeFromID(conn, &((*nodes)[i]),
				sqlite3_column_int(statement, 0));
		if (error) {
			error_print(
					"DBConnection_getNodesToWhichSendFreeze: error in DBConnection_getDtnNodeFromID()\n");
			sqlite3_finalize(statement);
			free(*nodes);
			(*nodes) = NULL;
			(*numNodes) = 0;
			return error;
		}
	}
	//deallocazione risorse
	sqlite3_finalize(statement);

	return DB_SUCCESS;
}


int DBConncetion_deleteNode(sqlite3* conn, dtnNode node){
	//TODO l'eliminazione di un nodo implica che precedentemente siano stati eliminati
	//tutti i comandi verso di lui
	//tutte le sincronizzazioni con lui
	return DB_SUCCESS;
}




