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
 * syncCommand.c
 * MB: modificato tutto per l'implementazione della nuova sync multilaterale
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../DBInterface/DBInterface.h"
#include "../Controller/utils.h"
#include "syncCommand.h"
#include "watchList.h"
#include "../Controller/align.h"
#include "definitions.h"
#include "../Controller/debugger.h"
#include "../Controller/monitorAndSendThread.h"

static void syncCommand_buildAckCommand(syncCommand* this, char *ackCommandText,
		int outcome);
static void syncCommand_errorHandler(sqlite3 *conn, synchronization sync,
		int folderAddedToFS, int folderAddedToDB, int syncAddedToDB,
		int folderAddedToWatchList, int syncsOnFileAddedToDB);

void syncCommand_createCommand(command** cmd) {
	syncCommand* this;
	char* cmdText;
	char* token;

	char * toFree;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(syncCommand));
	this = (syncCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//Sync timestamp owner folderName mode nochat pwdRead pwdWrite
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

	//il quinto token e' il modo
	token = strsep(&cmdText, "\t");
	this->mode = atoi(token);

	//il sesto token e' il chat
	token = strsep(&cmdText, "\t");
	this->nochat = atoi(token);

	//leggo il lifetime
	token = strsep(&cmdText, "\t");
	this->lifetime = atoi(token);

	//pwdRead
	token = strsep(&cmdText, "\t");
	strcpy(this->pwdRead, token);

	//pwdWrite
	token = strsep(&cmdText, "\t");

	//visto che e' l'ultimo mi tolgo il \n
	stripNewline(token);

	strcpy(this->pwdWrite, token);

	//libero la memoria della stringa
	free(toFree);

	//assegno lo stato desync in quanto e' stato appena creato
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;
}

//invio richiesta in una sincronizzazione, si limita ad aggiornare lo stato del comando
void syncCommand_sendCommand(command* cmd) {
	cmd->state = CMD_PENDING;
	return;
}

void syncCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {

	//convert the command
	syncCommand* this = (syncCommand*) cmd;

	int error;
	int folderToSyncID;
	int synchronizationID;
	char dirName[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	synchronization sync;
	dtnNode sourceNode;

	SynchronizationState synchronizationStatus;
	SyncMode syncMode;

	SynchronizationState synchronizationStatusWithParent;

	char myOwnerFromEID[OWNER_LENGTH];	//il nome del nostro nodo a partire dal suo EID
	char sourceOwnerFromEID[OWNER_LENGTH]; //il nome del nodo del mittente

	int folderAddedToFS = 0;
	int folderAddedToDB = 0;
	int syncAddedToDB = 0;
	int folderAddedToWatchList = 0;
	int syncsOnFileAddedToDB = 0;

	int hasPullMode = 0;
	int hasPushAndPullInputMode = 0;

	int syncError;

	debug_print(DEBUG_OFF,
			"syncCommand_receiveCommand: received and accepted SYNC command\n");

//update DTNnode lifetime----------------------------------------------------------------------------------------------------------------------------

	//aggiorno il lifetime se il nodo non era sul db
	error = DBConnection_getDtnNodeFromEID(dbConn, &sourceNode,
			this->base.msg.source.EID);
	if (!error) {

		//ho il nodo sul db
		if (sourceNode.lifetime == -1) {
			debug_print(DEBUG_OFF,
					"syncCommand_receiveCommand: update source DTN node %s lifetime to %d\n",
					sourceNode.EID, this->lifetime);
			error = DBConnection_updateDTNnodeLifetime(dbConn,
					this->base.msg.source, this->lifetime);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in DBConnection_updateDTNnodeLifetime()\n");
				syncCommand_buildAckCommand(this, ackCommandText,
				INTERNALERROR);	//INTERNALERROR
				//syncCommand_errorHandler(dbConn, sync, folderAddedToFS,folderAddedToDB, syncAddedToDB, folderAddedToWatchList);
				return;
			}
			sourceNode.lifetime = this->lifetime;
		}
	} else {
		//può essere solo un INTERNALERROR... il nodo qui deve esserci per forza...
		error_print(
				"syncCommand_receiveCommand: error in DBConnection_getDtnNodeFromEID()\n");
		syncCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);//INTERNALERROR
		return;
	}

	//end update DTNnode lifetime----------------------------------------------------------------------------------------------------------------------------

	 if(sourceNode.blackWhite == BLACKLIST){
			syncCommand_buildAckCommand(this, ackCommandText,
			BLACKLISTED);
			//syncCommand_errorHandler(dbConn, sync, folderAddedToFS,folderAddedToDB, syncAddedToDB, folderAddedToWatchList);
			return;
		}

	//retrieve data to check the constraints and the validity of the request

	getOwnerFromEID(myOwnerFromEID, this->base.msg.destination.EID);
	getOwnerFromEID(sourceOwnerFromEID, this->base.msg.source.EID);

	sync.folder = getEmptyFileListFolderFromOwnerAndFolder(this->owner,
			this->folderName);
	sync.node = sourceNode;

	getAbsolutePathFromOwnerAndFolder(dirName, sync.folder.owner,
			sync.folder.name);

	strcpy(sync.pwdRead, this->pwdRead);
	strcpy(sync.pwdWrite, this->pwdWrite);

	//controllo di avere la cartella sul db
	folderToSyncID = 0;
	error = DBConnection_getFolderToSyncID(dbConn, sync.folder,
			&folderToSyncID);
	if (error && error != DB_DATA_NOT_FOUND_ERROR) {
		//si è verificato un errore e non è la mancanza della cartella
		error_print(
				"syncCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
		syncCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
		return;
	}

	if (folderToSyncID != 0) {
		//have folder on DB

		error = DBConnection_getSynchronizationID(dbConn, sync,
				&synchronizationID);
		if (error && error != DB_DATA_NOT_FOUND_ERROR) {
			//error in DBConnection_getSynchronizationID()
			error_print(
					"syncCommand_receiveCommand: error in DBConnection_getFolderToSyncID()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			return;
		} else if (!error) {
			//have the sync with this node, must handle this situation

			//get the synchronization state from DB
			error = DBConnection_getSynchronizationStatus(dbConn, sync,
					&synchronizationStatus);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in DBConnection_getSynchronizationStatus()\n");
				syncCommand_buildAckCommand(this, ackCommandText,
				INTERNALERROR);
				return;
			}

			//already synch with this node, get the mode from DB
			error = DBConnection_getSynchronizationMode(dbConn, sync,
					&syncMode);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in DBConnection_getSynchronizationMode()\n");
				syncCommand_buildAckCommand(this, ackCommandText,
				INTERNALERROR);
				return;
			}

			///TODO define behaviour if is the same sync mode or different sync mode...
			if ((this->mode == PULL && syncMode == PUSH)
					|| (this->mode == PUSH && syncMode == PULL)
					|| (this->mode == PUSH_AND_PULL_IN
							&& syncMode == PUSH_AND_PULL_OUT)
					|| (this->mode == PUSH_AND_PULL_OUT
							&& syncMode == PUSH_AND_PULL_IN)) {

				//match with syncs...
			}else{

			}
		} //have the sync with this node

		//folderToSyncID == 0 <-> error == DB_DATA_NOT_FOUND_ERROR
		//NOFOLDER -> NOSYNC
		//so if I have folderToSyncID != 0 -> I have the sync with the parent (confirmed or pending)

		//check if input mode is present (PULL || PUSH_AND_PULL_IN)
		hasPullMode = 0;
		error = DBConnection_hasSynchronizationMode(dbConn, sync.folder,
				PULL, &hasPullMode);
		if (error) {
			//INTERNALERROR
			error_print(
					"syncCommand_receiveCommand: error in DBConnection_hasSynchronizationMode()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			return;
		}

		hasPushAndPullInputMode = 0;
		error = DBConnection_hasSynchronizationMode(dbConn, sync.folder,
				PUSH_AND_PULL_IN, &hasPushAndPullInputMode);
		if (error) {
			//INTERNALERROR
			error_print(
					"syncCommand_receiveCommand: error in DBConnection_hasSynchronizationMode()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			return;
		}

		if (strcmp(myOwnerFromEID, sync.folder.owner) != 0) {
			//if I am not the owner of the folder but I have the folder on DB, then I must have a sync (CONFIRMED or PENDING) with my parent, else error!
			error = DBConnection_getSynchronizationStatusWithParent(dbConn,
					sync.folder, &synchronizationStatusWithParent);
			if (error) {
				//INTERNALERROR
				error_print(
						"syncCommand_receiveCommand: error in DBConnection_getSynchronizationStatusWithParent()\n");
				syncCommand_buildAckCommand(this, ackCommandText,
				INTERNALERROR);
				return;
			}
		}
	}

//--------------------------------------------------------------------------
	//CONSTRAINTS
	syncError = OK;
	if (strcmp(myOwnerFromEID, sync.folder.owner) == 0) {
		//someone want one of my folders

		if (this->mode == PUSH || this->mode == PUSH_AND_PULL_OUT) {
			//OWNERVIOLATION
			syncError = OWNERVIOLATION;
		} else if (this->mode == PULL || this->mode == PUSH_AND_PULL_IN) {
			//check if I have the folder, if no folder -> NOFOLDER
			if (folderToSyncID == 0 || !fileExists(dirName)) {
				syncError = NOFOLDER;
			}
		}
	}		//someone want one of my folders
	else{
		//not my folder
		if(strcmp(sourceOwnerFromEID, sync.folder.owner) == 0){
			//request from owner
			if (this->mode == PULL || this->mode == PUSH_AND_PULL_IN) {
				//OWNERVIOLATION
				syncError = OWNERVIOLATION;
			}
		}
		if((folderToSyncID != 0 /*|| !fileExists(dirName)*/) &&
			(this->mode == PUSH || this->mode == PUSH_AND_PULL_OUT) &&
			(hasPullMode || hasPushAndPullInputMode)){
			//REMEMBER: no folder on DB -> no sync
			//so folder on DB -> must have the sync with the parent

			//here we already have a sync (confirmed or pending) with the parent
			if (hasPullMode)
				syncError = ALREADYSYNC_PULL;
			else if (hasPushAndPullInputMode)
				syncError = ALREADYSYNC_PUSHANDPULL;
		}
		if(this->mode == PULL || this->mode == PUSH_AND_PULL_IN){
			//someone asked a folder and I am not the owner
			if(folderToSyncID == 0 || !fileExists(dirName)){
				syncError = NOFOLDER;
			}else{
				//we have the folder
				if(synchronizationStatusWithParent == SYNCHRONIZATION_PENDING ||
					//TODO choose only one of the next check (first is the best...)
					(this->mode == PUSH_AND_PULL_IN && !hasPushAndPullInputMode) ||
					(this->mode == PUSH_AND_PULL_IN && hasPullMode) ||
					(this->mode == PULL && !(hasPullMode || hasPushAndPullInputMode))
				)
					syncError = NOSYNC;
			}
		}
	}//sync not on my folder
//END CONSTRAINT--------------------------------------------------------------------------





	if (syncError) {
		//insert a sort of switch... eventually a called function... specific syncError
		error_print(
				"syncCommand_receiveCommand: can't accept sync because of (%d)\n",
				syncError);
		syncCommand_buildAckCommand(this, ackCommandText, syncError);
		return;
	}

	//this check should be not necessary but can help to find bug...
	if (folderToSyncID != 0
			&& (this->mode == PUSH || this->mode == PUSH_AND_PULL_OUT)) {
		error_print("syncCommand_receiveCommand: BUG!!\n");
		syncCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
		return;
	}

	//here I can accept the sync request

	//fundamental, in local we invert the mode
	if (this->mode == PUSH) {
		sync.mode = PULL;

	} else if (this->mode == PULL) {
		sync.mode = PUSH;

	} else if (this->mode == PUSH_AND_PULL_IN) {
		sync.mode = PUSH_AND_PULL_OUT;
	} else {
		sync.mode = PUSH_AND_PULL_IN;
	}

	sync.state = SYNCHRONIZATION_CONFIRMED;

	//from this moment we have to handle the errors by calling "syncCommand_errorHandler()"
	folderAddedToDB = 0;
	folderAddedToFS = 0;
	syncAddedToDB = 0;
	folderAddedToWatchList = 0;
	syncsOnFileAddedToDB = 0;

	//&& !fileExists(dirName))
	if ((sync.mode == PULL || sync.mode == PUSH_AND_PULL_IN)
			&& folderToSyncID == 0) {

		//add folder to DB
		error = DBConnection_addFolderToSync(dbConn, sync.folder);
		if (error) {
			error_print(
					"syncCommand_receiveCommand: error in DBConnection_addFolderToSync()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
					folderAddedToDB, syncAddedToDB, folderAddedToWatchList,
					syncsOnFileAddedToDB);
			return;
		}
		folderAddedToDB = 1;

		//add folder to FS
		getHomeDir(dirName);
		strcat(dirName, DTNBOX_FOLDERSTOSYNCFOLDER);
		strcat(dirName, sync.folder.owner);
		strcat(dirName, "/");
		if (!fileExists(dirName))
			folderAddedToFS = 1;

		error = checkAndCreateFolder(dirName);
		if (error) {
			error_print(
					"syncCommand_receiveCommand: error in checkAndCreateFolder()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
					folderAddedToDB, syncAddedToDB, folderAddedToWatchList,
					syncsOnFileAddedToDB);
			return;
		}

		strcat(dirName, sync.folder.name);
		strcat(dirName, "/");

		if (!folderAddedToFS && !fileExists(dirName))
			folderAddedToFS = 1;
		error = checkAndCreateFolder(dirName);
		if (error) {
			error_print(
					"syncCommand_receiveCommand: error in checkAndCreateFolder()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
					folderAddedToDB, syncAddedToDB, folderAddedToWatchList,
					syncsOnFileAddedToDB);
			return;
		}
	}

	error = DBConnection_addSynchronization(dbConn, sync);
	if (error) {
		//should neve happen...
		error_print(
				"syncCommand_receiveCommand: error in DBConnection_addSynchronization()\n");
		syncCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
		syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
				folderAddedToDB, syncAddedToDB, folderAddedToWatchList,
				syncsOnFileAddedToDB);
		return;
	}
	syncAddedToDB = 1;

	//OK...

	// add path to watch list if
	// 1) I am the owner && path is not being watched
	//or
	// 2) sync.mode == PUSH_AND_PULL_IN (<-> !watchList_containsFolder && <-> I am not the owner)
//	if ((strcmp(myOwnerFromEID, sync.folder.owner) == 0
//			&& !watchList_containsFolder(dirName))
//			|| sync.mode == PUSH_AND_PULL_IN) {
//		error = recursive_add(dirName);
//		if (error) {
//			error_print(
//					"syncCommand_receiveCommand: error in recursive_add()\n");
//			syncCommand_buildAckCommand(this, ackCommandText,
//			INTERNALERROR);
//			syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
//					folderAddedToDB, syncAddedToDB, folderAddedToWatchList,
//					syncsOnFileAddedToDB);
//			return;
//		}
//		folderAddedToWatchList = 1;
//	}

	//if I am the owner
	if ((strcmp(myOwnerFromEID, sync.folder.owner) == 0 && !watchList_containsFolder(dirName)) || sync.mode == PUSH_AND_PULL_IN) {

		if (!watchList_containsFolder(dirName) ||  sync.mode == PUSH_AND_PULL_IN) {
			//necessary a static scan of dirName path
			error = alignFSandDB(dbConn, sync.folder, dirName);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in alignFSandDB()\n");
				syncCommand_buildAckCommand(this, ackCommandText,
				INTERNALERROR);
				syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
						folderAddedToDB, syncAddedToDB,
						folderAddedToWatchList, syncsOnFileAddedToDB);

				return;
			}
			folderAddedToWatchList = 1;
			syncsOnFileAddedToDB = 1;
		} else {
			//add only the syncOnFile for this sinchronization
			error = DBConnection_addSyncsOnFileForSynchronization(dbConn,
					sync);
			if (error) {
				error_print(
						"syncCommand_receiveCommand: error in DBConnection_addSyncsOnFileForSynchronization()\n");
				syncCommand_buildAckCommand(this, ackCommandText,
				INTERNALERROR);
				syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
						folderAddedToDB, syncAddedToDB,
						folderAddedToWatchList, syncsOnFileAddedToDB);

				return;
			}
			syncsOnFileAddedToDB = 1;
		}
	} else if (sync.mode == PUSH || sync.mode == PUSH_AND_PULL_OUT) {
		//ricevo richiesta di sync di una cartella di cui io non sono owner
		//add only the syncOnFile for this synchronization
		error = DBConnection_addSyncsOnFileForSynchronization(dbConn, sync);
		if (error) {
			error_print(
					"syncCommand_receiveCommand: error in DBConnection_addSyncsOnFileForSynchronization()\n");
			syncCommand_buildAckCommand(this, ackCommandText,
			INTERNALERROR);
			syncCommand_errorHandler(dbConn, sync, folderAddedToFS,
					folderAddedToDB, syncAddedToDB, folderAddedToWatchList,
					syncsOnFileAddedToDB);

			return;
		}
		syncsOnFileAddedToDB = 1;
	}

	//OK, finally!!!
	debug_print(DEBUG_OFF, "syncCommand_receiveCommand: synchronization added\n");
	syncCommand_buildAckCommand(this, ackCommandText,
	OK);


	return;
} //syncCommand_receiveCommand()

void syncCommand_destroyCommand(command* cmd) {

	//deallocare solo i campi specifici allocati con malloc() nella createCommand()
	//in questo caso vuota
	return;
}

//costruisce la stringa ACK sia in caso di esito positivo che negativo
static void syncCommand_buildAckCommand(syncCommand* this, char *ackCommandText,
		int outcome) {

	char tempString[TEMP_STRING_LENGTH];

	//azzero il testo dell'ack
	strcpy(ackCommandText, "");

	//Mi preparo la stringa per l'ack
	//SyncAck timestamp owner folderName mode nochat pwdRead pwdWrite resp
	strcpy(ackCommandText, "SyncAck");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);
	//owner e folderName
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->owner);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->folderName);
	//inserisco il mode
	sprintf(tempString, "%d", this->mode);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);
	//metto il no chat
	sprintf(tempString, "%d", this->nochat);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);
	//metto il lifetime
	sprintf(tempString, "%d", this->lifetime);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);
	//inserisco pass di lettura e scrittura
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->pwdRead);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, this->pwdWrite);

	switch (outcome) {

	case (OK): {
		//a questo punto invio il comando di ack, aggiungo l'ok in questo caso
		strcat(ackCommandText, "\tOK\n");
		break;
	}

	case (INTERNALERROR): {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR\n");
		break;
	}
	case (BLACKLISTED): {
		//segnalo nell'ack che la richiesta e' stata rifiutata
		strcat(ackCommandText, "\tERROR\tBLACKLISTED\n");
		break;
	}
	case (NOFOLDER): {
		//segnalo nell'ack che la richiesta e' stata rifiutata
		strcat(ackCommandText, "\tERROR\tNOFOLDER\n");
		break;
	}
	case (NOSYNC): {
		//segnalo nell'ack che la richiesta e' stata rifiutata
		strcat(ackCommandText, "\tERROR\tNOSYNC\n");
		break;
	}
	case (OWNERVIOLATION): {
		//segnalo nell'ack che la richiesta e' stata rifiutata
		strcat(ackCommandText, "\tERROR\tOWNERVIOLATION\n");
		break;
	}
	case (ALREADYSYNC_PULL): {
		strcat(ackCommandText, "\tERROR\tALREADYSYNC_PULL\n");
		break;
	}
	case (ALREADYSYNC_PUSHANDPULL): {
		strcat(ackCommandText, "\tERROR\tALREADYSYNC_PUSHANDPULL\n");
		break;
	}

	default: {
		//non dovremmo mai arrivarci, in tal caso è un INTERNALERROR
		strcat(ackCommandText, "\tERROR\tINTERNALERROR\n");
		break;
	}
	}
	return;
}

//annulla tutte le operazioni di sincronizzazione locali... tipo "rollback"
static void syncCommand_errorHandler(sqlite3 *conn, synchronization sync,
		int folderAddedToFS, int folderAddedToDB, int syncAddedToDB,
		int folderAddedToWatchList, int syncsOnFileAddedToDB) {
	//annulla l'operazione di sync locale... ovvero la addsync, la watchList_add, ...

	//in caso di errori nell'handler non ci posso fare nulla... teoricamente non si dovrebbero mai verificare...
	//cannot hande errors in errorHander...

	char rmCommand[SYSTEM_COMMAND_LENGTH + FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	char tempString[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	getAbsolutePathFromOwnerAndFolder(tempString, sync.folder.owner,
			sync.folder.name);

	if (folderAddedToWatchList)
		recursive_remove(tempString);
	if (folderAddedToFS) {
		getAbsolutePathFromOwnerAndFolder(tempString, sync.folder.owner,
				sync.folder.name);
		strcpy(rmCommand, "rm -rf \"");
		strcat(rmCommand, tempString);
		strcat(rmCommand, "\"");
		system(rmCommand);
	}

	if (syncsOnFileAddedToDB)
		DBConnection_deleteSyncOnAllFiles(conn, sync);
	if (syncAddedToDB)
		DBConnection_deleteSynchronization(conn, sync);
	if (folderAddedToDB)
		DBConnection_deleteFolder(conn, sync.folder);
	return;
}

//crea il testo di un comando SYNC a partire dall'oggetto
void createSyncText(synchronization sync, int sourceLifetime, char* commandText) {
	char tempString[TEMP_STRING_LENGTH];

	//Sync timestamp owner folderName mode nochat pwdRead pwdWrite
	//genero il comando di sync
	strcpy(commandText, "Sync");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", getCurrentTime());
	strcat(commandText, "\t");
	strcat(commandText, tempString);
	//aggiungo la cartella con owner e nome
	strcat(commandText, "\t");
	strcat(commandText, sync.folder.owner);
	strcat(commandText, "\t");
	strcat(commandText, sync.folder.name);
	//inserisco il mode
	sprintf(tempString, "%d", sync.mode);
	strcat(commandText, "\t");
	strcat(commandText, tempString);
	//metto il no chat a false
	strcat(commandText, "\t0");
	//metto il lifetime
	sprintf(tempString, "%d", sourceLifetime);
	strcat(commandText, "\t");
	strcat(commandText, tempString);
	//inserisco pass di lettura e scrittura
	strcat(commandText, "\t");
	strcat(commandText, sync.pwdRead);
	strcat(commandText, "\t");
	strcat(commandText, sync.pwdWrite);
	strcat(commandText, "\n");
}
