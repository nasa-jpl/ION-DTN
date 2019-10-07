/********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * parseTarThread.c
 */

#include "parseTarThread.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "utils.h"
#include "../Model/cmdList.h"
#include "commandParser.h"
#include "../DBInterface/DBInterface.h"
#include "../Model/finAckCommand.h"
#include "debugger.h"

#include <unistd.h>

#include <sys/signal.h>

#include <sqlite3.h>

#include "../Model/freezeCommand.h"
#include "../Model/syncCommand.h"

//check the source of the command and add it to DB if not present
static int checkSourceNode(sqlite3* dbConn, char *sourceEID);

//extract tar, then extract commands to parse from the current processed bundle
static int getCommandsFromCurrentTarPath(parseTarArgs *argomenti,
		int currentProcessIndex, char *absoluteWaitingBundlesFolder,
		char *absoluteCurrentProcessedFolder, cmdList *commands);

//for each command the specific receiveCommand() function is called, then if necessary the ack command is created and added to DB
static void processReceivedCommands(sqlite3 *dbConn, dtnNode source,
		dtnNode myNode, cmdList receivedCommands);

//clean the currentProcess cell of the buffer, clean the currentProcessedFolder, increment the private index of the buffer, and send post to receiveThread
static void setProcessed(parseTarArgs *argomenti, char *cleanCommand,
		int *currentProcessIndex);

void* parseTarDaemon(void* args) {

	parseTarArgs* argomenti = (parseTarArgs*) args;
	sqlite3* dbConn = argomenti->dbConn;

	int error;

	char absoluteWaitingBundlesFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/in/waitingBundles/
	char absoluteCurrentProcessedFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH]; // ~/DTNbox/.tempDir/in/currentProcessed/

	char cleanCommand[SYSTEM_COMMAND_LENGTH + DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH]; // rm -rf ~/DTNbox/.tempDir/in/absoluteCurrentProcessedFolder/*

	cmdList commands = NULL;
	cmdList toDestroy = NULL;

	int currentProcessIndex = 0;

	int semValue = 0;



	error = disableSigint();
	if (error) {
		error_print("parseTarDaemon: error in disableSigint()\n");
		pthread_exit(NULL);
	}

	getHomeDir(absoluteWaitingBundlesFolder);
	strcat(absoluteWaitingBundlesFolder, DTNBOXFOLDER);
	strcat(absoluteWaitingBundlesFolder, TEMPDIR);
	strcat(absoluteWaitingBundlesFolder, INFOLDER);

	strcpy(absoluteCurrentProcessedFolder, absoluteWaitingBundlesFolder);

	strcat(absoluteWaitingBundlesFolder, WAITINGBUNDLES);
	strcat(absoluteCurrentProcessedFolder, CURRENTPROCESSED);

	sprintf(cleanCommand, "rm -rf %s*", absoluteCurrentProcessedFolder);


	//if semValue > 0 -> then there are some bundles waiting to be processed
	while (!terminate(argomenti->quit)
			|| (!sem_getvalue(argomenti->processable, &semValue) && semValue)) {


		sem_wait(argomenti->processable);


		if(((argomenti->pendingReceivedBundles)[currentProcessIndex]).source.EID[0] == 0){
			//ricevuta post dall'handler di chiusura... terminiamo!!
			continue;
			//oppure break;
		}

		pthread_mutex_lock(argomenti->generalMutex);

		//check the source of the tar
		error =
				checkSourceNode(dbConn,
						((argomenti->pendingReceivedBundles)[currentProcessIndex]).source.EID);
		if (error) {
			error_print("parseTarThread: error in checkSourceNode()\n");
			setProcessed(argomenti, cleanCommand, &currentProcessIndex);
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}


		//se sono qua significa che posso elaborare questo bundle (sono sicuro che il nodo è presente e non è in black list
		commands = cmdList_create();
		//retrive commands from the tar
		error = getCommandsFromCurrentTarPath(argomenti, currentProcessIndex, absoluteWaitingBundlesFolder, absoluteCurrentProcessedFolder, &commands);
		if (error) {
			error_print(
					"parseTarThread: error in getCommandsFromCurrentTarPath()\n");
			setProcessed(argomenti, cleanCommand, &currentProcessIndex);
			pthread_mutex_unlock(argomenti->generalMutex);
			continue;
		}

		toDestroy = commands;

		//process the commands, the core is the call of the specific receiveCommand() function of the specific command
		processReceivedCommands(dbConn,
				((argomenti->pendingReceivedBundles)[currentProcessIndex]).source,
				argomenti->myNode, toDestroy);
		cmdList_destroy(commands);

		//unlock general mutex
		pthread_mutex_unlock(argomenti->generalMutex);

		//send post to receiveThread
		setProcessed(argomenti, cleanCommand, &currentProcessIndex);
	}	//while(!terminate)


	debug_print(DEBUG_L1, "parseTarThread: terminated correctly\n");
	pthread_exit(NULL);
}

//check the source of the command and add it to DB if not present
static int checkSourceNode(sqlite3* dbConn, char *sourceEID) {

	int error;
	dtnNode sender;
	blackWhite result;

	//known node?
	error = DBConnection_getDtnNodeFromEID(dbConn, &sender, sourceEID);
	if (error == DB_DATA_NOT_FOUND_ERROR) {

		//se ricevo un bundle da un nodo sconosciuto
		//il nodo non e' presente, imposto valori di default
		debug_print(DEBUG_L1,
				"checkSourceNode: node %s not on database, set the default value in numTx field\n",
				sourceEID);
		strcpy(sender.EID, sourceEID);
		sender.lifetime = -1;
		sender.numTx = getDefaultNumRetry();

		sender.blackWhite = WHITELIST;
		sender.blockedBy = NOT_BLOCKEDBY;
		sender.frozen = NOT_FROZEN;

		error = DBConnection_addDtnNode(dbConn, sender);
		if (error) {
			error_print(
					"checkSourceNode: error in DBConnection_addDtnNode()\n");
			return ERROR_VALUE;
		}

	} else if (error) {
		debug_print(DEBUG_L1,
				"checkSourceNode: errore di DBConnection_getDtnNodeFromEID()\n");
		return ERROR_VALUE;
	}

	error = DBConnection_getDTNnodeFrozen(dbConn, sender, &(sender.frozen));
	if(error){
		error_print("checkSourceNode: error in DBConnection_getDTNnodeFrozen()\n");
		return ERROR_VALUE;
	}
	if(sender.frozen == FROZEN){
		//unfreeze it as we received a bundle.
		error = DBConnection_updateDTNnodeFrozen(dbConn, sender, NOT_FROZEN);
		if(error){
			error_print("checkSourceNode: error in DBConnection_updateDTNnodeFrozen()\n");
			return ERROR_VALUE;
		}
	}

	//is node in black list?
	result = -1;
	error = DBConnection_getDTNnodeBlackWhite(dbConn, sender, &result);
	if(error){
		error_print("checkSourceNode: error in DBConnection_getDTNnodeBlackWhite()\n");
		return ERROR_VALUE;
	}
	if(result == BLACKLIST){
		//l'eid e' nella blacklist, non elaboro il comando
		//TODO per adesso lo processiamo... e rispondiamo con BLACKLISTED
		debug_print(DEBUG_OFF, "checkSourceNode: received bundle from %s but is blacklisted\n", sourceEID);
	}

	return SUCCESS_VALUE;
}

//extract tar, then extract commands to parse from the current processed bundle
static int getCommandsFromCurrentTarPath(parseTarArgs *argomenti,
		int currentProcessIndex, char *absoluteWaitingBundlesFolder,
		char *absoluteCurrentProcessedFolder, cmdList *commands) {

	DIR* extractDirEntity;
	struct dirent* foundFile;

	char dbcmdFile[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH]; 	// ~/DTNbox/.tempDir/in/currentProcesed/ ... .dbcmd
	char tarCompleteName[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH]; // ~/DTNbox/.tempDir/in/currentProcessed/ ... .tar.gz

	char systemCommand[SYSTEM_COMMAND_LENGTH + 2*(DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH)]; // "mv ~/DTNbox/.tempDir/in/waitingBundles/ ... .tar.gz ~/DTNbox/.tempDir/in/currentProcessed/ ... .tar.gz"
	int error;

	//sposto il tar da processare dalla cartella waitingBundles alla cartella currentProcessed
	sprintf(systemCommand, "mv \"%s%s\" \"%s%s\"", absoluteWaitingBundlesFolder,
			((argomenti->pendingReceivedBundles)[currentProcessIndex]).relativeTarName,
			absoluteCurrentProcessedFolder,
			((argomenti->pendingReceivedBundles)[currentProcessIndex]).relativeTarName);
	//debug_print(DEBUG_L1, "parseTarThread: mvCommand: %s\n", mvCommand);

	error = system(systemCommand);
	if (error) {
		error_print("getCommandsFromCurrentTarPath(): error in system(%s)\n", systemCommand);
		return ERROR_VALUE;

	}

	sprintf(tarCompleteName, "%s%s", absoluteCurrentProcessedFolder,
			((argomenti->pendingReceivedBundles)[currentProcessIndex]).relativeTarName);
	//debug_print(DEBUG_L1, "parseTarThread: tarCompleteName: %s\n", tarCompleteName);

	//estraggo i file del tar
	error = tarToFiles(tarCompleteName, absoluteCurrentProcessedFolder);
	if (error) {
		char dtnboxHomeFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];

		error_print("getCommandsFromCurrentTarPath(): error in tarToFiles()\n");

		//in case of error we copy the tar to the DTNbox home folder
		getHomeDir(dtnboxHomeFolder);
		strcat(dtnboxHomeFolder, DTNBOXFOLDER);
		sprintf(systemCommand, "cp %s %s", tarCompleteName, dtnboxHomeFolder);
		error = system(systemCommand);
		if(error){
			error_print("getCommandsFromCurrentTarPath(): error in system(%s)\n", systemCommand);
		}
		return ERROR_VALUE;
	}

	//prendi file .dbcmd
	extractDirEntity = opendir(absoluteCurrentProcessedFolder);
	if (extractDirEntity == NULL) {
		error_print("getCommandsFromCurrentTarPath(): error in opendir()\n");
		return ERROR_VALUE;
	}

	strcpy(dbcmdFile, "");
	while ((foundFile = readdir(extractDirEntity)) != NULL) {
		if (endsWith(foundFile->d_name, ".dbcmd")) {
			//ho trovato il file .dbcmd
			strcpy(dbcmdFile, absoluteCurrentProcessedFolder);
			strcat(dbcmdFile, foundFile->d_name);
			break;
		}
	}
	closedir(extractDirEntity);
	if (!strcmp(dbcmdFile, "")) {
		error_print("getCommandsFromCurrentTarPath(): dbcmd file not found\n");
		return ERROR_VALUE;
	}
	debug_print(DEBUG_L1, "dbcmdFile: %s\n", dbcmdFile);

	//parsa file .dbcmd e recupera i comandi da elaborare
	*commands = cmdParser_getCommands(dbcmdFile);
	if (*commands == NULL) {
		error_print("getCommandsFromCurrentTarPath(): error in cmdParser_getCommands()\n");
		return ERROR_VALUE;
	}

	//necessaria reverse siccome dobbiamo prima eseguire ad esempio synAck e poi update!!
	*commands = cmdList_reverse(*commands);

	return SUCCESS_VALUE;
}

//for each command the specific receiveCommand() function is called, then if necessary the ack command is created and added to DB
static void processReceivedCommands(sqlite3 *dbConn, dtnNode source,
		dtnNode myNode, cmdList receivedCommands) {

	int error;

	command* ackCommand;
	cmdList commands = receivedCommands;

	char *ackText;
	int ackTextSize;

	dtnNode updatedSender;

	//esegue i singoli comandi
	while (commands != NULL) {

		//assegno ai comandi alcuni parametri
		commands->value->state = CMD_PROCESSING;
		commands->value->msg.source = source;
		commands->value->msg.destination = myNode;

		//aggiungo il comando sul db
		error = DBConnection_controlledAddCommand(dbConn, commands->value);
		if (error) {
			error_print(
					"processReceivedCommands: error in DBConnection_controlledAddCommand()\n");
			commands = commands->next;
			continue;
		}

		ackText = NULL;
		ackTextSize = getAckCommandSize(commands->value->text);
		if(ackTextSize > 0){
			ackText = malloc(sizeof(char) * ackTextSize);
			memset(ackText, 0, sizeof(char)*ackTextSize);
		}
		//execute the command, this line is the core of this function, understand the behaviour
		commands->value->funcTable.receiveCommand(commands->value, dbConn,
				ackText);

		if (ackTextSize > 0 && ackText != NULL) {
			//the received command need an ack

			//recupero le info aggiornate per il mittente
			error = DBConnection_getDtnNodeFromEID(dbConn, &updatedSender,
					source.EID);
			if (error) {
				error_print(
						"parseTar: error in DBConnection_getDtnNodeFromEID()\n");
				//parseTarThreadstatic void sig_handler(int signal)_cleanExit(argomenti, cleanCommand, toDestroy, dbConn);
				commands = commands->next;
				free(ackText);
				continue;
			}

			debug_print(DEBUG_L1, "processReceivedCommands: ack created %s\n", ackText);

			//Il comando prevede un ack, provvedo a inserirlo
			newCommand(&ackCommand, ackText);
			//note: now I can free the memory as new command already copied it;
			free(ackText);


			//assegno il nodo destinatario e sorgente e inizializzo i valori di msg
			ackCommand->msg.destination = updatedSender;
			ackCommand->msg.source = myNode;
			ackCommand->msg.txLeft = updatedSender.numTx;	//TODO qui ci dovrebbe andare 1... verificare e correggere...
																//probabilmente il bug non sorge in quanto la clausola di trasmissione
																//è data da numTx (ovvero txLeft) MA ANCHE DALLO STATO DEL COMANDO
																//quindi se txLeft è > 1, ma lo stato è PROCESSING/CONFIRMED
																//allora l'ACK non verrà ritrasmesso...
																
																//per una chiarezza del protocollo si potrebbe mettere:
																//ackCommand->msg.txLeft = 1;
																
																
			ackCommand->msg.nextTxTimestamp = getNextRetryDate(updatedSender);

			error = DBConnection_controlledAddCommand(dbConn, ackCommand);
			destroyCommand(ackCommand);
			if (error) {
				error_print(
						"parseTarThread: error in DBConnection_controlledAddCommand()\n");
				//parseTarThread_cleanExit(argomenti, cleanCommand, toDestroy, dbConn);
				commands = commands->next;
				continue;
			}
		}
		commands = commands->next;
	}
	return;
}

//clean the currentProcess cell of the buffer, clean the currentProcessedFolder, increment the private index of the buffer,  and send post to receiveThread
static void setProcessed(parseTarArgs *argomenti, char *cleanCommand,
		int *currentProcessIndex) {

	//this lock should not be necessary
	pthread_mutex_lock(argomenti->receiveMutex);
	//clean the current processed array cell
	memset(&((argomenti->pendingReceivedBundles)[*currentProcessIndex]), 0, sizeof(pendingReceivedBundleInfo));
	pthread_mutex_unlock(argomenti->receiveMutex);

	//clean the currentProcessed folder
	if (system(cleanCommand)) {
		error_print("parseTarThread: error in system(%s)\n", cleanCommand);
	}

	//increment the private index of the circular buffer
	*currentProcessIndex = (*currentProcessIndex + 1) % MAX_PENDING_RX_BUNDLES;
	//post to receiveThread
	sem_post(argomenti->receivable);
}

