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
 * DBInterface.h
 */

#ifndef DBINTERFACE_H_
#define DBINTERFACE_H_

#include <stdlib.h>

#include <sqlite3.h>
#include "../Model/folderToSync.h"
#include "../Model/synchronization.h"
#include "../Model/command.h"
#include "../Model/cmdList.h"

typedef enum {
	DB_SUCCESS=0,				//success
	DB_BIND_ERROR,				//error in sqlite3_bind_XYZ() function
	DB_PREPARE_ERROR,			//error in sqlite3_prepare_v2() function
	DB_STEP_ERROR,				//error in sqlite3_step() function
	DB_EXEC_ERROR,				//error in sqlite3_exec() function
	DB_DUMP_DB_ON_FILE_ERROR,	//error in function on file during db dump
	DB_PARAMETER_ERROR,			//not valid input
	DB_DATA_NOT_FOUND_ERROR		//valid input, but not found on db
} DB_ERROR;

// -- Metodi per la creazione/distruzione di tutte le tabelle

//creo per la prima volte le tabelle necessarie al funzionamento del programma
int DBConnection_createTables(sqlite3* conn);

//elimino tutte le tabelle presenti sul database
int DBConnection_dropTables(sqlite3* conn);

//metodo di debug che stampa su file l'intero database
int DBConnection_dumpDBData(sqlite3* conn);




// -- Metodi di gestione dtnNode

//aggiunta dtnNode su database
int DBConnection_addDtnNode(sqlite3* conn, dtnNode node);

//ricavo la chiave primaria di un nodo
int DBConnection_getDtnNodeID(sqlite3* conn, dtnNode node, int* nodeID);

//ricavo un nodo partendo dall'EID
int DBConnection_getDtnNodeFromEID(sqlite3* conn, dtnNode* node,
		char* EIDToGet);

//ricavo un nodo dall'id univoco
int DBConnection_getDtnNodeFromID(sqlite3* conn, dtnNode* node,
		int nodeIDToGet);

//aggiorna il lifetime di un nodo
int DBConnection_updateDTNnodeLifetime(sqlite3* conn, dtnNode node,
		int lifetime);

//crea array di nodi per i quali c'è almeno un comando da trasmettere (o ritrasmettere)
int DBConnection_getNodesWithCommandsToTransmit(sqlite3 *conn, dtnNode **nodes,
		int *numNodes, unsigned long long currentDate);


int DBConnection_updateDTNnodeBlackWhite(sqlite3* conn, dtnNode node, blackWhite blackWhite);
int DBConnection_updateDTNnodeBlockedBy(sqlite3* conn, dtnNode node, blockedBy blockedBy);
int DBConnection_updateDTNnodeFrozen(sqlite3* conn, dtnNode node, frozen frozen);

int DBConnection_getDTNnodeBlackWhite(sqlite3* conn, dtnNode node, blackWhite *blackWhite);
int DBConnection_getDTNnodeBlockedBy(sqlite3* conn, dtnNode node, blockedBy *blockedBy);
int DBConnection_getDTNnodeFrozen(sqlite3* conn, dtnNode node, frozen *frozen);


int DBConnection_isNodeOnDb(sqlite3* conn, char *EID, int* result);

int DBConnection_getNodesToWhichSendFreezeOrUnfreeze(sqlite3* conn, dtnNode **nodes, int *numNodes);

// -- Metodi di gestione file da sincronizzare

//aggiunta di un nuovo file sul database
int DBConnection_addFileToSync(sqlite3* conn, fileToSync file,
		int folderToSyncID);

//controlla se esiste una file sul database e salva il risultato nella variabile result
int DBConnection_isFileOnDb(sqlite3* conn, fileToSync fileToCheck,
		int folderToSyncID, int* result);

//restituisco la lista di file da sincronizzare per una data cartella (allocazione dinamica)
int DBConnection_getFilesToSyncFromFolder(sqlite3* conn, fileToSyncList* files,
		int folderToSyncID);

//ricavo l'id sul database di un file
int DBConnection_getFileToSyncID(sqlite3* conn, fileToSync file,
		int folderToSyncID, int* fileID);

//ricavo la data di modifica sul database di un file
int DBConnection_getFileToSyncLastModified(sqlite3* conn, fileToSync file,
		int folderToSyncID, unsigned long long* lastModified);

//aggiorno l'ultima modifica sul file
int DBConnection_updateFileToSyncLastModified(sqlite3* conn, fileToSync file,
		int folderToSyncID, unsigned long long lastModified);

//cancello tutti i file per una cartella
int DBConnection_deleteFilesForFolder(sqlite3* conn, int folderToSyncID);

//restituisce i file il cui nome relativo inizia per relativeSubfoldePath (allocazione dinamica)
int DBConnection_getFilenamesToDeleteFromSubfolderPath(sqlite3* conn,
		int folderToSyncID, char *relativeSubfolderPath, char ***fileNames,
		int *numFiles);

//restituisce il numero di sincronizzazione sul file
int DBConnection_getNumSyncStatusOnFiles(sqlite3 *conn, int folderToSyncID, char *filename, int *numSyncStatusOnFile);

//elimina un file dal database
int DBConnection_deleteFileFromDB(sqlite3* conn, int folderToSyncID, char *filename);

//aggiorna lo stato DELETED o meno di un file
int DBConnection_updateFileDeleted(sqlite3* conn, fileToSync file,
		int folderToSyncID, FileDeletionState newState);

//recupera dal database le informazioni sul file (lastModified e deleted)
int DBConnection_getFileInfo(sqlite3 *conn, int folderToSyncID, fileToSync *file);










// -- Metodi di gestione cartelle da sincronizzare

//aggiunta di una cartella sul database
int DBConnection_addFolderToSync(sqlite3* conn, folderToSync folder);

//ricavo l'id della cartella
int DBConnection_getFolderToSyncID(sqlite3* conn, folderToSync folder,
		int* folderToSyncID);

//restituisce tutte le cartelle (FolderToSync) di cui fare una scansione
int DBConnection_getAllFoldersToSync(sqlite3* conn, folderToSync** folders,
		int* numFolders);

//controlla se sul database e' gia presente la cartella
int DBConnection_isFolderOnDb(sqlite3* conn, folderToSync folderToCheck,
		int* result);

//restituisce tutte le cartelle presenti sul database
int DBConnection_getAllFolders(sqlite3* conn, folderToSync** folders,
		int* numFolders, char *currentUser);

//cancella una cartella e relativi files dal database
int DBConnection_deleteFolder(sqlite3* conn, folderToSync folder);

//Restituisce le cartelle da monitorare con inotify
int DBConnection_getAllFoldersToMonitor(sqlite3* conn, folderToSync** folders,
		int* numFolders, char *currentUser);

//Restituisce il numero di sincronizzazioni sulla cartella
int DBConnection_getNumSynchronizationsOnFolder(sqlite3 *conn, folderToSync folder, int *numSyncs);







// -- Metodi di gestione delle sincronizzazioni

//aggiunta di una sincronizzazione sul db
int DBConnection_addSynchronization(sqlite3* conn, synchronization sync);

//recupero la chiave primaria di una sincronizzazione
int DBConnection_getSynchronizationID(sqlite3* conn, synchronization sync,
		int* synchronizationID);

//elimina una sincronizzazione dal database
int DBConnection_deleteSynchronization(sqlite3* conn, synchronization sync);

//data una cartella, restituisce in un array l'insieme delle sincronizzazioni non in PULL legate alla FolderToSync indicata
int DBConnection_getSynchronizationsOnFolder(sqlite3* conn, folderToSync folder,
		synchronization** syncs, int* numSyncs);

//data una cartella restituisce in un array TUTTE le sincronizzazioni su di essa, comprese quelle in PULL.
int DBConnection_getAllSynchronizationsOnFolder(sqlite3* conn,
		folderToSync folder, synchronization** syncs, int* numSyncs);

//data una sync restituisce il suo stato
int DBConnection_getSynchronizationStatus(sqlite3* dbConn, synchronization tempSync, SynchronizationState *synchronizationStatus);

//data una sync restituisce la sua modalita
int DBConnection_getSynchronizationMode(sqlite3 *conn, synchronization sync,
		SyncMode *mode);

//data una cartella e una modalità di sincronizzazione restituisce se esiste almeno una sincronizzazione con quella modalità
//da usare con mode == PULL || mode == PUSH_AND_PULL_INPUT
int DBConnection_hasSynchronizationMode(sqlite3* conn, folderToSync folder, SyncMode mode, int *result);

//data una sync ne aggiorna lo stato
int DBConnection_updateSynchronizationStatus(sqlite3 *conn, synchronization sync, SynchronizationState newStatus);

//Restituisce lo stato della sincronizzazione con il padre (ed è unica) su una data cartella
int DBConnection_getSynchronizationStatusWithParent(sqlite3 *conn, folderToSync folder, SynchronizationState *stateWithParent);







// -- Metodi di gestione dei comandi

//aggiunta di un comandp sul db
int DBConnection_addCommand(sqlite3* conn, command cmd);

//controlla se esiste una comando sul database
int DBConnection_isCommandOnDb(sqlite3* conn, command cmd, int* result);

//aggiorna lo stato di un comando sul database, utilizzato negli ack
int DBConnection_updateCommandState(sqlite3* conn, dtnNode node, char* text,
		CmdState newState);

//recupero il numero di trasmissioni di un comando
int DBConnection_getCommandTxLeft(sqlite3* conn, dtnNode node, char* text,
		int* txLeft);

//aggiorna il numero di trasmissioni per comando e la data di prossima ritrasmissione
int DBConnection_updateCommandTxLeftAndRetryDate(sqlite3* conn, dtnNode node,
		char* text, int txLeft, unsigned long long nextTxTimestamp);

//elimina tutti i comandi con stato CMD_PROCESSING o CMD_CONFIRMED piu' vecchi della data limite specificata
int DBConnection_clearCommandsOlderThan(sqlite3* conn,
		unsigned long long limitDate);

//elimina il comando all'id specificato
int DBConnection_deleteCommand(sqlite3* conn, int commandID);

//mette a failed tutti i comandi precedenti, da invocare alla ricezione dell'ack
int DBConnection_setPreviusPendingAsFailed(sqlite3* conn, dtnNode sourceNode,
		unsigned long long ackDate);

//recupera tutti i comandi da trasmettere (o ritrasmettere) per il nodo destNode (alloca dinamicamente memoria)
int DBConnection_getCommandsToTransmitForDestNode(sqlite3 *conn,
		dtnNode localNode, dtnNode destNode, cmdList *cmdListReturn,
		unsigned long long currentDate);


// -- Metodi di gestione della syncStatus

//aggiunta di una sincronizzazione su di un file
int DBConnection_addSyncOnFile(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID);

//aggiorna lo stato di un file per una sincro
int DBConnection_updateSyncOnFileState(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID, FileState newState);

//aggiorna il numero di trasmissioni per file e la data di prossima ritrasmissione
int DBConnection_updateSyncOnFileTxLeftAndRetryDate(sqlite3* conn,
		synchronization sync, fileToSync file, int folderToSyncID, int txLeft,
		unsigned long long nextTxTimestamp);

//cancello la sincro da un file
int DBConnection_deleteSyncOnFile(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID);

//cancello la sincro da tutti i file
int DBConnection_deleteSyncOnAllFiles(sqlite3* conn, synchronization sync);

//recupero il numero di trasmissioni di un file
int DBConnection_getSyncOnFileTxLeft(sqlite3* conn, synchronization sync,
		fileToSync file, int folderToSyncID, int* txLeft);

/*
 * Restituisce tutti i file di una sincronizzazione su una cartella che sono in stato DESYNC
 * Ricordarsi di fare la free dell'array di files nel chiamante.
 */
int DBConnection_getAllFilesToUpdate(sqlite3* conn, synchronization sync,
		folderToSync folder, fileToSync** files, int* numFiles, unsigned long long currentTime);

//controlla se esiste la syncstate sul file
int DBConnection_hasSyncStatusOnFile(sqlite3* conn, synchronization sync,
		fileToSync fileToCheck, int folderToSyncID, int* result,
		int* isPending);

//imposta tutti i file a DESYNC forzandone l'update indipendentemente dallo stato in cui si trovano
int DBConnection_forceUpdate(sqlite3* conn);


#endif /* DBINTERFACE_H_ */
