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
 * monitorAndSendThreadOperations.c
 *
 * operazioni eseguite dallo monitorAndSendThread
 *
 * -elaborazione eventi inotify
 * -costruzione comandi di aggiornamento
 * -copia dei file da trasmettere nella cartella temporanea
 * -creazione dei file .dbcmd, .tar e invio dei bundle
 * -eliminazione vecchi comandi
 * -creazione cartelle di sync mancanti
 */


#include "monitorAndSendThreadOperations.h"

#include "../Model/folderToSync.h"
#include "../Model/fileToSync.h"
#include "../Model/cmdList.h"
#include "../Model/watchList.h"
#include "../Model/updateCommand.h"
#include "../Model/synchronization.h"
#include "../DBInterface/DBInterface.h"
#include "utils.h"
#include "align.h"
#include "../Model/definitions.h"
#include "al_bp_wrapper.h"
#include "debugger.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/inotify.h>
#include <libgen.h>

#include "../Model/syncCommand.h"
#include <sys/stat.h>
#include <sys/signal.h>

//this function creates and sends a tar to destNode
static int sendOperation(sqlite3 *dbConn, dtnNode myNode, dtnNode destNode,
		al_bp_extB_registration_descriptor register_descriptor);

//this function creates a tar with ".dbcmd" file and files to transmit
static char* createTarFile(cmdList commands, sqlite3* dbConn,
		dtnNode dtnNodeDest, char* myOwnerName, unsigned long long currentTime);

//update tx attemps, next retry date and state for a command
static int updateCommandInfoAfterSend(sqlite3 *dbConn, dtnNode dtnNodeDest,
		command *value);

//update tx attemps, next retry date and state for a syncOnFile
static int updateSyncOnFileInfoAfterSend(sqlite3* dbConn, dtnNode dtnNodeDest,
		char *owner, char *folderName, fileToSync value);

//this operation sets syncsOnFile in DESYNC state
int processInotifyEvents(sqlite3 *dbConn) {

	int error;

	int ifd = getIfd();	//inotify file descriptor
	int nread;	//number of bytes read from ifd
	char *buffer;//buffer dinamically allocated containing all inotify event structs
	char *ptr;	//byte pointer to scan buffer
	const struct inotify_event *event;	//current event processed
	char *eventFolder;	//event folder path <-> wd in event

	char absoluteFilePath[LINUX_ABSOLUTE_PATH_LENGTH];
	char parentDirectoryAbsFilePath[LINUX_ABSOLUTE_PATH_LENGTH];


	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	//we must read all the events in one time, else problems

	if (ioctl(ifd, FIONREAD, &nread) < 0) {
		error_print(
				"processInotifyEvents(): error while reading bytes availables in inotify file descriptor\n");
		nread = 0;
		buffer = NULL;
	} else if (nread > 0) {
		//debug_print(DEBUG_OFF, "monitorAndSendThread: bytes disponibili: %d\n", nread);
		buffer = (char*) malloc(sizeof(char) * nread);
		nread = read(ifd, buffer, nread);
	}

	if (nread > 0) {
		//debug_print(DEBUG_OFF, "monitorAndSendThread: bytes letti per evento inotify %d\n", nread);

		for (ptr = buffer; ptr < buffer + nread;
				ptr += sizeof(struct inotify_event) + event->len) {

			event = (const struct inotify_event*) ptr;

			/*
			 debug_print(DEBUG_OFF, "watchDescriptor: %d, file: %s, mask: %d, nameLen: %d\n",
			 event->wd, event->len ? event->name : "", event->mask,
			 event->len);
			 */
			eventFolder = watchList_containsDescriptor(event->wd);
			if (eventFolder != NULL && !(event->mask & IN_IGNORED)) {

				//filter changes in progress (example: .filename.swp)
				if ((event->name)[0] == '.')
					continue;
				strcpy(absoluteFilePath, eventFolder);
				strcpy(parentDirectoryAbsFilePath, eventFolder);

				if (event->len)
					strcat(absoluteFilePath, event->name);
				//debug_print(DEBUG_OFF, "monitorAndSendThread: event on %s\n", eventPath);

				//delete cartella controllata
				if ((event->mask & IN_DELETE_SELF)) {

//					debug_print(DEBUG_OFF,
//							"processInotifyEvent: detected delete of a monitored folder\n");

					if(event->mask & IN_DELETE_SELF){
						//IN_DELETE_SELF
						error = deleteFolderOnFs(dbConn, absoluteFilePath);
						if (error) {
							error_print(
									"processInotifyEvents(): error in deleteFolderOnFs()\n");
							continue;
						}
					}

					if (1/*cartellaEliminataE'DiSync*/) {
						//TODO propago al fine a tutti!!!!
					}

				} else {

					if ((event->mask & IN_ISDIR)) {
						//event on monitored subfolder
						if (absoluteFilePath[strlen(absoluteFilePath) - 1] != '/')
							strcat(absoluteFilePath, "/");

						if ((event->mask & IN_CREATE)
								|| (event->mask & IN_MOVED_TO)) {

//							debug_print(DEBUG_OFF,
//									"processInotifyEvent: detected folder creation\n");

							error = newFolderOnFS(dbConn, absoluteFilePath);
							if (error) {
								error_print(
										"processInotifyEvents(): error in newFolderOnFS()\n");
								continue;
							}
						}else if(event->mask & IN_MOVED_FROM){
							error = moveFolderOnFs(dbConn, absoluteFilePath);
							if (error) {
								error_print(
										"processInotifyEvents(): error in deleteFolderOnFs()\n");
								continue;
							}
						}
					}
					else {
						//event su file
						if ((event->mask & IN_CLOSE_WRITE) || (event->mask & IN_MOVED_TO)) {
//							debug_print(DEBUG_OFF,
//									"processInotifyEvent: detected file creation: %s\n",
//									absoluteFilePath);
							error = newOrUpdateFileOnFS(dbConn, absoluteFilePath);
							if (error) {
								error_print(
										"processInotifyEvents(): error in newOrUpdateFileOnFS()\n");
								continue;
							}
						}
						else if ((event->mask & IN_DELETE)
								|| (event->mask & IN_MOVED_FROM)) {
							error = deleteFileOnFS(dbConn, absoluteFilePath);
							if (error) {
								error_print(
										"processInotifyEvents(): error in deleteFileOnFS()\n");
								continue;
							}
						}
					}			//event on file
				}

				//to update parent folder timestamp on DB (indeed all the events above change the parent directory timestamp)
				if(fileExists(parentDirectoryAbsFilePath) && !isSynchronizationFolderPath(parentDirectoryAbsFilePath)){
					error = newOrUpdateFileOnFS(dbConn, parentDirectoryAbsFilePath);
					if(error){
						error_print(
								"processInotifyEvents(): error in newOrUpdateFileOnFS()\n");
						continue;
					}
				}
			}	//if we have folder <->wd

			//this should never happen
			else if (eventFolder == NULL && !(event->mask & IN_IGNORED)) {
				error_print(
						"processInotifyEvents(): BIG ERROR, related path not found in watchList\n");
				error_print("watchDescriptor: %d, file: %s, mask: %d\n",
						event->wd, event->name, event->mask);
				continue;
			}
		}	//for() events on FS
		free(buffer);
	}	//there was something to read
	else if (nread < 0 && errno != EAGAIN) {
		error_print(
				"processInotifyEvents(): BIG ERROR error while reading inotify events %s\n",
				strerror(errno));
		return ERROR_VALUE;
	}	//devastating mistake

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	return SUCCESS_VALUE;
}

//this operation get syncsOnFile in DESYNC state, build relative update commands and copy files in tempCopy folder
int buildUpdateCommandsAndCopyTempFiles(sqlite3 *dbConn, dtnNode myNode) {

	int error;
	int i;
	int k;
	int j = 0;

	folderToSync* folders;
	int numFolders;

	synchronization* syncsOnFolder;
	int numSyncs;

	fileToSync* filesToUpdate;
	int numFilesToUpdate;

	command* cmd;
	char updateString[UPDATE_COMMAND_TEXT_SIZE_LIMIT];	//"Update ....."
	int updateStringCurrentSize = 0;
	int updateStringBaseSize = 0;


	char currentTripleProcessed[FILETOSYNC_LENGTH + sizeof(unsigned long long) + sizeof(int) + 4];

	char folderToSyncPath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	char absoluteFilePath[FILETOSYNC_ABSOLUTE_PATH_LENGTH];
	struct stat fileInfo;

	char tempCopyFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];
	char tempCopyFolderFilePath[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH];

	getHomeDir(tempCopyFolder);
	strcat(tempCopyFolder, DTNBOXFOLDER);
	strcat(tempCopyFolder, TEMPDIR);
	strcat(tempCopyFolder, OUTFOLDER);
	strcat(tempCopyFolder, TEMPCOPY);

	int folderToSyncID = 0;
	fileToSync fileToSyncFromDB;


	//vedo nel database quali cartelle controllare
	numFolders = 0;
	folders = NULL;
	error = DBConnection_getAllFoldersToSync(dbConn, &folders, &numFolders);
	if (error) {
		error_print(
				"buildUpdateCommandsAndCopyTempFiles():error in DBConnection_getAllFoldersToSync()\n");
		return ERROR_VALUE;
	}

	//scan delle cartelle e rilevazione delle modifiche
	for (i = 0; i < numFolders; i++) {

		error = DBConnection_getFolderToSyncID(dbConn, folders[i], &folderToSyncID);
		if(error){
			error_print(
					"buildUpdateCommandsAndCopyTempFiles: error in DBConnection_getFolderToSyncID()\n");
			continue;
		}

		//adesso lavoro a livello di sincro, per ogni nodo creero' il relativo comando di update
		//come prima cosa chiedo tutte le sincro associate a questa cartella
		syncsOnFolder = NULL;
		numSyncs = 0;
		error = DBConnection_getSynchronizationsOnFolder(dbConn, folders[i],
				&syncsOnFolder, &numSyncs);
		if (error) {
			error_print(
					"buildUpdateCommandsAndCopyTempFiles: error in DBConnection_getSynchronizationsOnFolder()\n");
			continue;
		}

		getHomeDir(folderToSyncPath);
		strcat(folderToSyncPath, DTNBOX_FOLDERSTOSYNCFOLDER);
		strcat(folderToSyncPath, folders[i].owner);
		strcat(folderToSyncPath, "/");
		strcat(folderToSyncPath, folders[i].name);
		strcat(folderToSyncPath, "/");

		//per ogni sincronizzazione creero' il relativo comando di update
		for (k = 0; k < numSyncs; k++) {

			if(syncsOnFolder[k].node.frozen == FROZEN || syncsOnFolder[k].node.blackWhite == BLACKLIST  || syncsOnFolder[k].node.blockedBy == BLOCKEDBY)
				continue;

			//recupero tutti i file che devo aggiornare per questa cartella e questa sincro
			filesToUpdate = NULL;

			error = DBConnection_getAllFilesToUpdate(dbConn, syncsOnFolder[k],
					folders[i], &filesToUpdate, &numFilesToUpdate, getCurrentTime());
			if (error) {
				error_print(
						"buildUpdateCommandsAndCopyTempFiles: error in DBConnection_getAllFilesToUpdate()\n");
				continue;
			}
			if (numFilesToUpdate > 0) {

				//first we order the files... TODO: we should order using a SQL query...
				qsort(filesToUpdate, numFilesToUpdate, sizeof(fileToSync), compare);

				updateStringCurrentSize = 0;



				for(j = 0 ; j < numFilesToUpdate; ) {

					sprintf(updateString, "Update\t%llu\t%s\t%s", getCurrentTime(), folders[i].owner, folders[i].name);
					updateStringBaseSize = strlen(updateString);
					updateStringCurrentSize = strlen(updateString) + 2;


					for( ; j < numFilesToUpdate; j++){

						sprintf(currentTripleProcessed, "\t%s\t%d\t%llu", filesToUpdate[j].name, filesToUpdate[j].deleted, filesToUpdate[j].lastModified);
						if(updateStringCurrentSize + strlen(currentTripleProcessed) < UPDATE_COMMAND_TEXT_SIZE_LIMIT){


							//copia dei file nella cartella temporanea
							if (!filesToUpdate[j].deleted) {
								strcpy(absoluteFilePath, folderToSyncPath);
								strcat(absoluteFilePath, filesToUpdate[j].name);
								if (fileExists(absoluteFilePath)) {

									sprintf(tempCopyFolderFilePath, "%s%s/%s/%s",
											tempCopyFolder, folders[i].owner,
											folders[i].name,
											filesToUpdate[j].name);

									if (!fileExists(tempCopyFolderFilePath)) {

										strcpy(fileToSyncFromDB.name, filesToUpdate[j].name);
										error = DBConnection_getFileInfo(dbConn, folderToSyncID, &fileToSyncFromDB);
										if(error){
											error_print("buildUpdateCommandsAndCopyTempFiles: error in DBConnection_getFileInfo()\n");
											continue;
										}
										stat(absoluteFilePath, &fileInfo);

										if (fileInfo.st_mtim.tv_sec == fileToSyncFromDB.lastModified) {

											error = fileLockingCopy(folders[i],
													filesToUpdate[j].name, fileInfo,
													absoluteFilePath);
											if (error) {
												error_print("buildUpdateCommandsAndCopyTempFiles: error in fileLockingCopy()\n");
												memset(filesToUpdate[j].name, 0, sizeof(filesToUpdate[j].name));
												continue;
												//TODO cosa fare?
												//mettiamo il syncStatus a failed...
											}
										}//if stesso timestamp
										else{
											error_print("buildUpdateCommandsAndCopyTempFiles: different timestamp for %s, skip the file\n", filesToUpdate[j].name);
											memset(filesToUpdate[j].name, 0, sizeof(filesToUpdate[j].name));
											continue;
										}
									} else {
										//file gia copiato precedentemente, no problem!!
									}

								} else {
									error_print(
											"buildUpdateCommandsAndCopyTempFiles: error, file not found...\n");
									memset(filesToUpdate[j].name, 0, sizeof(filesToUpdate[j].name));
									continue;
									//TODO fare altro?
									//mettere a failed la syncStatus? mettere a deleted il file?
								}
							}

							//everything fine...
							strcat(updateString, currentTripleProcessed);
							updateStringCurrentSize += strlen(currentTripleProcessed);

						}else{
							//debug_print(DEBUG_L1, "buildUpdateCommandsAndCopyTempFiles: reached limit for this command string!!\n");
							break;
						}
					} //for updateString can contains tripes...

					if(strlen(updateString) > updateStringBaseSize){
						//only if updateString contains at least a file...
						strcat(updateString, "\n");

						//creo il comando
						newCommand(&cmd, updateString);

						//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
						cmd->msg.destination = syncsOnFolder[k].node;
						cmd->msg.source = myNode;

						//to avoid loop in loop, and a lot of other problems
						//retransmissions managed with syncStatus
						cmd->msg.txLeft = 1;
						cmd->msg.nextTxTimestamp = getNextRetryDate(
								syncsOnFolder[k].node);

						error = DBConnection_controlledAddCommand(dbConn, cmd);
						destroyCommand(cmd);
						if (error) {
							error_print(
									"buildUpdateCommandsAndCopyTempFiles: error in DBConnection_controlledAddCommand()\n");
							continue;
						}
					}
				}//for numFilesToUpdate
				free(filesToUpdate);
			}
		}
		//faccio la free
		if (syncsOnFolder != NULL) {
			free(syncsOnFolder);
		}
	}					//fine ciclo di scan cartelle
						//ora posso liberare folders
	if (folders != NULL) {
		free(folders);
	}

	return SUCCESS_VALUE;
}

//this operation send commands to relative dest nodes
int sendCommandsToDestNodes(sqlite3 *dbConn, dtnNode myNode,
		al_bp_extB_registration_descriptor register_descriptor) {

	int error;

	int i;

	dtnNode* destNodes = NULL;
	int numDestNodes = 0;

	char tempCopy[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/out/tempCopy/
	char cleanCommand[SYSTEM_COMMAND_LENGTH + 2*(DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH)];	//rm -rf ~/DTNbox/.tempDir/out/tempCopy/*.tar ~/DTNbox/.tempDir/out/tempCopy/*.dbcmd

	getHomeDir(tempCopy);
	strcat(tempCopy, DTNBOXFOLDER);
	strcat(tempCopy, TEMPDIR);
	strcat(tempCopy, OUTFOLDER);
	strcat(tempCopy, TEMPCOPY);

	sprintf(cleanCommand, "rm -rf %s*.tar %s*.dbcmd", tempCopy, tempCopy);
	//debug_print(DEBUG_OFF, "cleanCommand: %s\n", cleanCommand);

	//Fase di invio comandi
	//recupero dal database tutti i nodi a cui devo trasmettere uno o più comandi
	error = DBConnection_getNodesWithCommandsToTransmit(dbConn, &destNodes,
			&numDestNodes, getCurrentTime());
	if (error) {
		error_print(
				"sendCommandsToDestNodes(): error in DBConnection_getNodesWithCommandsToTransmit\n");
		return ERROR_VALUE;
	}
	if (numDestNodes > 0) {
		for (i = 0; i < numDestNodes; i++) {

			//il nodo è frozen... non gli dobbiamo inviare nulla!!
			//e in caso di BLOCKEDLIST (si, xke gli dobbiamo inviare ad esempio l'syncAck con esito negativo)
			//e in caso di BLOCKEDBY (solo fin, ma AL MOMENTO con un nodo bloccato non dovrei avere sync...)
			if(destNodes[i].frozen == FROZEN /*TODO verificare se necessario || destNodes[i].blockedBy == BLOCKEDBY*/)
				continue;

			//in caso di errore durante l'invio non esco dal ciclo ma provo ad inviare anche gli altri nodi...
			error = sendOperation(dbConn, myNode, destNodes[i],
					register_descriptor);
			if (error) {
				error_print(
						"sendCommandsToDestNodes(): error in sendOperation() for destNode: %s\n",
						destNodes[i].EID);
			}
			if (system(cleanCommand)) {
				//error while cleaning .tar and .dbcmd files for destNodes[i]
				error_print("sendCommandsToDestNodes(): error in system(%s)\n",
						cleanCommand);
			}
		}
		free(destNodes);
	}

	sprintf(cleanCommand, "rm -rf %s*", tempCopy);
	error = system(cleanCommand);
	if (error) {
		error_print("sendCommandsToDestNodes(): error in system(%s)\n",
				cleanCommand);
		error_print(
				"sendCommandsToDestNodes(): error while cleaning tempCopy content\n");
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}

//this opeation remove from DB ***old*** commands in CONFIRMED or PROCESSING state
int clearOldCommandsFromDB(sqlite3 *dbConn) {

	int error;

	unsigned long long deleteDate;

	//pulisco tutti i comandi che sono più vecchi di 5 minuti
	deleteDate = getCurrentTime() - OLDER_THAN_SECONDS;
	error = DBConnection_clearCommandsOlderThan(dbConn, deleteDate);
	if (error) {
		error_print(
				"clearOldCommandsFromDB(): error in DBConnection_clearCommandsOlderThan()\n");
		return ERROR_VALUE;
	}

	return SUCCESS_VALUE;
}

//this function creates on FS folders present in DB, but not present in FS... why?
int createMissingFolders(sqlite3 *dbConn, dtnNode myNode) {

	int error;
	int i;

	folderToSync* folders;
	int numFolders;

	char absoluteFilePath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	char myOwnerFromEID[OWNER_LENGTH];

	getOwnerFromEID(myOwnerFromEID, myNode.EID);

	//prendo tutte le cartelle sul db, in caso non siano presenti sul fs le ricreo
	numFolders = 0;
	folders = NULL;
	error = DBConnection_getAllFolders(dbConn, &folders, &numFolders,
			myOwnerFromEID);
	if (error) {
		error_print(
				"createMissingFolders(): error in DBConnection_getAllFolders()\n");
		return ERROR_VALUE;
	} else {
		for (i = 0; i < numFolders; i++) {

			//mi costruisco il path assoluto
			getAbsolutePathFromOwnerAndFolder(absoluteFilePath, folders[i].owner,
					folders[i].name);

			//controllo esista la cartella in caso contrario la creo
			error = checkAndCreateFolder(absoluteFilePath);
			if (error) {
				error_print(
						"createMissingFolders(): error in checkAndCreateFolder()\n");
				free(folders);
				return ERROR_VALUE;
			}
		}
	}
	if (folders != NULL)
		free(folders);

	return SUCCESS_VALUE;
}



int prepareUnfreezeCommands(sqlite3 *dbConn, dtnNode myNode, int keepRemoteNodesFrozen){

	int error;
	int i;

	dtnNode *nodes;
	int numNodes;

	command *cmd;

	char commandText[UNFREEZE_COMMAND_TEXT_SIZE_LIMIT];

	//i nodi a cui mandare il comando di Unfreeze sono i nodi
	//-non bloccati
	//-non congelati
	//-per i quali c'è almeno attiva una sincronizzazione (TODO sync confermata, o sia confermata che pendente?)

	nodes = NULL;
	numNodes = 0;
	error = DBConnection_getNodesToWhichSendFreezeOrUnfreeze(dbConn, &nodes, &numNodes);
	if(error){
		error_print(
				"prepareUnfreezeCommands: error in DBConnection_getNodesToWhichSendFreezeOrUnfreeze()\n");
		return ERROR_VALUE;
	}

	if(!keepRemoteNodesFrozen){
		//we locally consider Unfreezed remote nodes.
		for(i=0; i<numNodes; i++){
			if(nodes[i].frozen == FROZEN){
				error = DBConnection_updateDTNnodeFrozen(dbConn, nodes[i], NOT_FROZEN);
				if(error){
					error_print("prepareUnfreezeCommands: error in DBConnection_updateDTNnodeFrozen()\n");
					free(nodes);
					return ERROR_VALUE;
				}
				nodes[i].frozen = NOT_FROZEN;
			}
		}
	}


	for(i=0; i<numNodes; i++){

		//TODO ottimizzabile...
		if(nodes[i].frozen == FROZEN || nodes[i].blockedBy == BLOCKEDBY || nodes[i].blackWhite == BLACKLIST)
			continue;

		sprintf(commandText, "Unfreeze\t%llu\n", getCurrentTime());
		debug_print(DEBUG_L1, "prepareUnfreezeCommands: Command created=%s\n",
				commandText);

		//procedo a creare il comando effettivo
		newCommand(&cmd, commandText);

		//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
		cmd->msg.destination = nodes[i];
		cmd->msg.source = myNode;
		cmd->msg.txLeft = nodes[i].numTx;
		cmd->msg.nextTxTimestamp = getNextRetryDate(nodes[i]);	//qui invece finché non ci confrerma che sa che ci siamo svegliati glielo continuiamo a dire

		error = DBConnection_controlledAddCommand(dbConn, cmd);
		destroyCommand(cmd);
		if (error) {
			free(nodes);
			error_print(
					"prepareUnfreezeCommands: error in DBConnection_controlledAddCommand()\n");
			return ERROR_VALUE;
		}
	}//for
	if(nodes != NULL)
		free(nodes);
	return SUCCESS_VALUE;
}


int prepareFreezeCommands(sqlite3 *dbConn, dtnNode myNode){

	int error;
	int i;

	dtnNode *nodes;
	int numNodes;

	command *cmd;

	char commandText[FREEZE_COMMAND_TEXT_SIZE_LIMIT];

	//i nodi a cui mandare il comando di Unfreeze sono i nodi
	//-non bloccati
	//-non congelati
	//-per i quali c'è almeno attiva una sincronizzazione (TODO sync confermata, o sia confermata che pendente?)
	nodes = NULL;
	numNodes = 0;
	error = DBConnection_getNodesToWhichSendFreezeOrUnfreeze(dbConn, &nodes, &numNodes);
	if(error){
		error_print(
				"prepareFreezeCommands: error in DBConnection_getNodesToWhichSendFreezeOrUnfreeze()\n");
		return ERROR_VALUE;
	}

	for(i=0; i<numNodes; i++){

		//TODO ottimizzabile...
		if(nodes[i].frozen == FROZEN || nodes[i].blockedBy == BLOCKEDBY || nodes[i].blackWhite == BLACKLIST)
			continue;

		sprintf(commandText, "Freeze\t%llu\n", getCurrentTime());
		debug_print(DEBUG_L1, "prepareFreezeCommands: Command created=%s\n",
				commandText);

		//procedo a creare il comando effettivo
		newCommand(&cmd, commandText);

		//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
		cmd->msg.destination = nodes[i];
		cmd->msg.source = myNode;
		cmd->msg.txLeft = 1; //note: we send only one time the Freeze command, because then we go to sleep...
		cmd->msg.nextTxTimestamp = getNextRetryDate(nodes[i]);

		error = DBConnection_controlledAddCommand(dbConn, cmd);
		destroyCommand(cmd);
		if (error) {
			free(nodes);
			error_print(
					"prepareFreezeCommands: error in DBConnection_controlledAddCommand()\n");
			return ERROR_VALUE;
		}
	}//for
	if(nodes != NULL)
		free(nodes);
	return SUCCESS_VALUE;
}





static int sendOperation(sqlite3 *dbConn, dtnNode myNode, dtnNode destNode,
		al_bp_extB_registration_descriptor register_descriptor) {

	int error;
	cmdList commands = cmdList_create();//all commands that need to be transmitted
	cmdList toScan;							//variable toScan the list

//	char tarFolder[ABSOLUTE_PATH]; //dovrà contenere "~/DTNbox/.tempDir/out/currentProcessed/"
	char *currentTarToSend;

	char myOwnerFromEID[OWNER_LENGTH];

	command* tempCommand;
	updateCommand *tempUpdateCommand;
	fileToSyncList filesToScan = fileToSyncList_create();

//	getHomeDir(tarFolder);
//	strcat(tarFolder, DTNBOXFOLDER);
//	strcat(tarFolder, TEMPDIR);
//	strcat(tarFolder, OUTFOLDER);
//	strcat(tarFolder, TEMPCOPY);

	getOwnerFromEID(myOwnerFromEID, myNode.EID);

	//recupero i comandi da tramsettere per il nodo corrente (ovvero destNode)
	error = DBConnection_getCommandsToTransmitForDestNode(dbConn, myNode,
			destNode, &commands, getCurrentTime());
	if (error) {
		error_print(
				"sendOperation(): error in DBConnection_getCommandsToRetransmitForDestNode()\n");
		return ERROR_VALUE;
	}
	commands = cmdList_reverse(commands);

	//create tar file to send to destNode
	toScan = commands;
	currentTarToSend = createTarFile(toScan, dbConn, destNode, myOwnerFromEID, getCurrentTime());

	if (currentTarToSend == NULL) {
		//TODO
		error_print("sendOperation(): error in createTarFile()\n");
		return ERROR_VALUE;
	}

	error = dtnbox_send(register_descriptor, currentTarToSend, destNode);
	if (error) {
		free(currentTarToSend);
		error_print("sendOperation(): error in dtnbox_send()\n");
		//send to myself the CTRL+C signal
		kill(getpid(), SIGINT);	//if(error) in BP operation then close the application
	} else {

		debug_print(DEBUG_L1, "monitorAndSendThread: sent bundle %s to %s\n", currentTarToSend,
				destNode.EID);
		free(currentTarToSend);

		toScan = commands;
		while (toScan != NULL) {
			//settiamo lo stato del comando a PENDING/CONFIRMED
			toScan->value->funcTable.sendCommand(toScan->value);
			debug_print(DEBUG_L1, "monitorAndSendThread: sent command to %s: %s\n",destNode.EID, toScan->value->text);

			//aggiorniamo STATO, TENTATIVI RIMASTI e NEXT RETRY DATE per questo comando
			error = updateCommandInfoAfterSend(dbConn, destNode, toScan->value);
			if (error) {
				error_print(
						"sendOperation(): error in updateCommandInfoAfterSend()\n");
			}

			//se sono il comando update ho un po' di roba da fare
			if (startsWith(toScan->value->text, "Update") && !startsWith(toScan->value->text, "UpdateAck")){

				//so di essere un comando di update
				newCommand(&tempCommand, toScan->value->text);
				tempUpdateCommand = (updateCommand*) tempCommand;

				filesToScan = tempUpdateCommand->files;

				while (filesToScan != NULL) {

					//aggiorniamo STATO, TENTATIVI RIMASTI e NEXT RETRY DATE per questa syncOnFile
					error = updateSyncOnFileInfoAfterSend(dbConn, destNode,
							tempUpdateCommand->owner,
							tempUpdateCommand->folderName, filesToScan->value);
					if (error) {
						error_print(
								"sendOperation(): error in updateSyncOnFileInfoAfterSend()\n");
					}
					filesToScan = filesToScan->next;
				}
				updateCommand_destroyCommand(tempCommand);

			} //if UpdateCommand

			toScan = toScan->next;
		}
	}
	cmdList_destroy(commands);

	return SUCCESS_VALUE;
}

//funzione presa dalla vecchia synchronizationCmdList
static char* createTarFile(cmdList commands, sqlite3* dbConn,
		dtnNode dtnNodeDest, char* myOwnerName, unsigned long long currentTime) {

	//error
	int error;

	char currentFile[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH];		//  ~/DTNbox/.tempDir/out/tempCopy/ and see below... ./owner/folder/fileName


	char tempCopyFolderAbsolutePath[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];

	char dbcmdFile[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH];// ~/DTNbox/.tempDir/out/tempCopy/EIDMittente-dataMillisec.dbcmd
	char tarFileName[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH];// ~/DTNbox/.tempDir/out/tempCopy/EIDMittente-dataMillisec.tar"

	char* res = NULL;
	FILE* f = NULL;
	cmdList toScan;
	updateCommand* updateC;
	fileToSyncList updateFiles;

	char sysCommand[TAR_COMMAND_SIZE_LIMIT];//"cd ~/DTNbox/.tempDir/out/tempCopy/ && tar cfp ./tarName ./dbcmdFile ./fileA ./fileB ./...
	int currentSysCommandLength = 0;


	getHomeDir(tempCopyFolderAbsolutePath);
	strcat(tempCopyFolderAbsolutePath, DTNBOXFOLDER);
	strcat(tempCopyFolderAbsolutePath, TEMPDIR);
	strcat(tempCopyFolderAbsolutePath, OUTFOLDER);
	strcat(tempCopyFolderAbsolutePath, TEMPCOPY);

	//nome del file (formato: EIDMittente-dataMillisec.dbcmd)
	sprintf(dbcmdFile, "%s%s-%llu.dbcmd", tempCopyFolderAbsolutePath, myOwnerName, currentTime);
	sprintf(tarFileName, "%s%s-%llu.tar", tempCopyFolderAbsolutePath, myOwnerName, currentTime);


	//create empty tar
	sprintf(sysCommand, "cd \"%s\" && tar cfp ./%s -T /dev/null", tempCopyFolderAbsolutePath, basename(tarFileName));
	error = system(sysCommand);
	if(error){
		error_print("createTarFile: error in system(%s)\n", sysCommand);
		return NULL;
	}




	//creo e apro il file
	f = fopen(dbcmdFile, "w");
	if (f == NULL) {
		error_print("createTarFile(): error in fopen()\n");
		return NULL;
	}

	//prepare add files to the archive...
	sprintf(sysCommand, "cd \"%s\" && tar rfp ./%s", tempCopyFolderAbsolutePath, basename(tarFileName));
	currentSysCommandLength = strlen(sysCommand) + 2;

	toScan = commands;
	while (toScan != NULL) {

		//scriviamo su file il comando corrente
		error = fprintf(f, "%s", toScan->value->text); //il text dovrebbe includere anche abbastanza \n da separare il comando dai successivi
		if (error != strlen(toScan->value->text)) {
			error_print("createTarFile(): error in fprintf()\n");
			if (f != NULL) {
				fclose(f);
				if (fileExists(tarFileName))
					remove(tarFileName);
				if (fileExists(dbcmdFile))
					remove(dbcmdFile);
			}
			return NULL;
		}



		//se sono il comando update ho un po' di roba da fare
		if (startsWith(toScan->value->text, "Update")
				&& !startsWith(toScan->value->text, "UpdateAck")) {

			//so di essere un comando di update
			updateC = (updateCommand*) toScan->value;
			updateFiles = updateC->files;

			while (updateFiles != NULL) {

				//faccio il tar solo dei file da trasmettere
				if (!updateFiles->value.deleted
						&& !fileIsFolderOnDb(updateFiles->value.name)) {

					sprintf(currentFile, " \"./%s/%s/%s\"", updateC->owner,
							updateC->folderName, updateFiles->value.name);

					if(currentSysCommandLength + strlen(currentFile) < TAR_COMMAND_SIZE_LIMIT - 2){
						//the string has enough space
						strcat(sysCommand, currentFile);
						currentSysCommandLength += strlen(currentFile);
					} else {
						//the sysCommand string is full, add files to the tar and "restart" :)
						error = system(sysCommand);
						if(error){
							error_print("createTarFile(): error in system(%s)\n", sysCommand);
							if (fileExists(tarFileName))
								remove(tarFileName);
							if (fileExists(dbcmdFile))
								remove(dbcmdFile);
							return NULL;
						}
						//prepare new system command
						sprintf(sysCommand, "cd \"%s\" && tar rfp ./%s %s",tempCopyFolderAbsolutePath, basename(tarFileName), currentFile);
						currentSysCommandLength = strlen(sysCommand) + 2;
					}
				}
				updateFiles = updateFiles->next;
			}
		} // if updateCommand
		//passo al comando successivo
		toScan = toScan->next;
	} //while(commands)

	fclose(f);

	//add dbcmd file to the archive
	sprintf(currentFile," ./%s", basename(dbcmdFile));
	if(currentSysCommandLength + strlen(currentFile) < TAR_COMMAND_SIZE_LIMIT - 2){
		//the string can contains also the dbcmd file :)
		strcat(sysCommand, currentFile);
		error = system(sysCommand);
		if(error){
			error_print("createTarFile(): error in system(%s)\n", sysCommand);
			if (fileExists(tarFileName))
				remove(tarFileName);
			if (fileExists(dbcmdFile))
				remove(dbcmdFile);
			return NULL;
		}
	} else {
		//the string cannot contains also the dbcmd file :)
		//add files to the tar
		error = system(sysCommand);
		if(error){
			error_print("createTarFile(): error in system(%s)\n", sysCommand);
			if (fileExists(tarFileName))
				remove(tarFileName);
			if (fileExists(dbcmdFile))
				remove(dbcmdFile);
			return NULL;
		}

		//add dbcmd file to archive...
		sprintf(sysCommand, "cd \"%s\" && tar rfp ./%s", tempCopyFolderAbsolutePath, basename(dbcmdFile));
		error = system(sysCommand);
		if(error){
			error_print("createTarFile(): error in system(%s)\n", sysCommand);
			if (fileExists(tarFileName))
				remove(tarFileName);
			if (fileExists(dbcmdFile))
				remove(dbcmdFile);
			return NULL;
		}
	}

	res = (char*) malloc(strlen(tarFileName) + 1);
	strcpy(res, tarFileName);
	return res;
}

//update tx attemps, next retry date and state for a command
static int updateCommandInfoAfterSend(sqlite3 *dbConn, dtnNode dtnNodeDest,
		command *value) {

	int error;

	int tempTxAttempts;
	unsigned long long tempNextRetyDate;

	//visto che sto creado il file sono in procinto di inviare i comandi, provvedo ad aggiornare il db
	error = DBConnection_updateCommandState(dbConn, dtnNodeDest, value->text,
			value->state);

	if (error) {
		error_print(
				"updateCommandInfoAfterSend(): error in DBConnection_updateCommandState()\n");
		return ERROR_VALUE;
	}

	//aggiorno di uno il numero di trasmissioni
	error = DBConnection_getCommandTxLeft(dbConn, dtnNodeDest, value->text,
			&tempTxAttempts);

	if (error) {
		error_print(
				"updateCommandInfoAfterSend(): error in DBConnection_getCommandTxAttempts()\n");
		return ERROR_VALUE;
	}

	//consumo il numero di trasmissioni
	tempTxAttempts--;
	value->msg.txLeft = tempTxAttempts;

	//sposto in avanti la data di retry
	tempNextRetyDate = getNextRetryDate(dtnNodeDest);

	//aggiorna il numero di trasmissioni per comando e la data di prossima ritrasmissione
	error = DBConnection_updateCommandTxLeftAndRetryDate(dbConn, dtnNodeDest,
			value->text, tempTxAttempts, tempNextRetyDate);

	if (error) {
		error_print(
				"updateCommandInfoAfterSend(): error in DBConnection_updateCommandTxAttemptsAndRetryDate()\n");
		return ERROR_VALUE;
	}

	if (tempTxAttempts == 0) {

		error = DBConnection_updateCommandState(dbConn, dtnNodeDest,
				value->text, CMD_FAILED);

		if (error) {
			error_print(
					"updateCommandInfoAfterSend(): error in DBConnection_updateCommandState()\n");
			return ERROR_VALUE;
		}
	}
	return SUCCESS_VALUE;
}

//update tx attemps, next retry date and state for a syncOnFile
static int updateSyncOnFileInfoAfterSend(sqlite3* dbConn, dtnNode dtnNodeDest,
		char *owner, char *folderName, fileToSync value) {

	int error;

	int folderToSyncID;

	synchronization tmpSync;

	FileState tempFileState;
	int tempFileTxAttempts;
	unsigned long long tempNextRetyDate;

	tmpSync.folder = getEmptyFileListFolderFromOwnerAndFolder(owner,
			folderName);
	tmpSync.node = dtnNodeDest;

	error = DBConnection_getFolderToSyncID(dbConn, tmpSync.folder,
			&folderToSyncID);
	if (error) {
		error_print(
				"updateSyncOnFileInfoAfterSend(): error in DBConnection_getFolderToSyncID()\n");
		return ERROR_VALUE;
	}
	error = DBConnection_getSyncOnFileTxLeft(dbConn, tmpSync, value,
			folderToSyncID, &tempFileTxAttempts);
	if (error) {
		error_print(
				"updateSyncOnFileInfoAfterSend(): error in DBConnection_getSyncOnFileTxAttempts()\n");
		return ERROR_VALUE;
	}
	tempFileTxAttempts--;
	tempNextRetyDate = getNextRetryDate(tmpSync.node);

	//update tx attemps e next retry date of syncOnFile
	error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(dbConn, tmpSync,
			value, folderToSyncID, tempFileTxAttempts, tempNextRetyDate);

	if (error) {
		error_print(
				"updateSyncOnFileInfoAfterSend(): error in DBConnection_updateSyncOnFileTxAttemptsAndRetryDate()\n");
		return ERROR_VALUE;
	}
	if (tempFileTxAttempts <= 0) {
		tempFileState = FILE_FAILED;
	} else {
		tempFileState = FILE_PENDING;
	}

	//update state of syncOnFile
	error = DBConnection_updateSyncOnFileState(dbConn, tmpSync, value,
			folderToSyncID, tempFileState);
	if (error) {
		error_print(
				"updateSyncOnFileInfoAfterSend(): error in DBConnection_updateSyncOnFileState()\n");
		return ERROR_VALUE;
	}

	return SUCCESS_VALUE;
}
