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
 * receiveThread.c
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "al_bp_wrapper.h"
#include "../DBInterface/DBInterface.h"
#include "sqlite3.h"
#include "receiveThread.h"
#include "utils.h"
#include <sys/signal.h>
#include "../Model/definitions.h"
#include "debugger.h"

#include <string.h>

#include <libgen.h>
#include "parseTarThread.h"
#include <unistd.h>
static char receivedTar[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	//path where BP place the received bundle
static char sourceEID[NODE_EID_LENGTH];	//sender EID of the received bundle (note: with SEND DEMUX TOKEN)

static void receiveHandler(int signal);

void* receiveDaemon(void* args) {

	receiveArgs* argomenti = (receiveArgs*) args;
	al_bp_extB_registration_descriptor register_descriptor =
			argomenti->register_receiver;

	int error;

	dtnNode sender;			//sender node (with RECEIVE_DEMUX_TOKEN)

	char absoluteWaitingBundlesFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/in/waitingBundles/
	char mvCommand[SYSTEM_COMMAND_LENGTH + 2*(DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH)];	//mv from receivedTar to absoluteWaitingBundlesFolder

	int currentReceiveIndex = 0;	//index for circular array

	//inizializzazione
	error = disableSigint();
	if (error) {
		error_print("parseTarDaemon: error in disableSigint()\n");
		pthread_exit(NULL);
	}

	signal(SIGUSR1, receiveHandler);

	//build absoluteWaitingBundlesFolder absolute path
	getHomeDir(absoluteWaitingBundlesFolder);
	strcat(absoluteWaitingBundlesFolder, DTNBOXFOLDER);
	strcat(absoluteWaitingBundlesFolder, TEMPDIR);
	strcat(absoluteWaitingBundlesFolder, INFOLDER);
	strcat(absoluteWaitingBundlesFolder, WAITINGBUNDLES);

	error = 0;
	memset(sourceEID, 0, sizeof(sourceEID));
	memset(receivedTar, 0, sizeof(receivedTar));


	while (!terminate(argomenti->quit)) {

	 	sem_wait(argomenti->receivable);

		error = dtnbox_receive(register_descriptor, receivedTar, sourceEID);
		if(error){
			memset(sourceEID, 0, sizeof(sourceEID));
			memset(receivedTar, 0, sizeof(receivedTar));

			switch(error){

			case ERROR_VALUE:{
				//CLOSE
				error_print("receiveThread: error in dtnbox_receive()\n");
				kill(getpid(), SIGINT);	//send SIGINT (like pressing CTRL+C) to the process
				break;
			}

			case WARNING_VALUE:{
				error_print("receiveThread: warning in dtnbox_receive()\n");
				break;
			}

			default:{
				//CLOSE, should never enter here...
				error_print("receiveThread: error in dtnbox_receive()\n");
				kill(getpid(), SIGINT); //send SIGINT (like pressing CTRL+C) to the process
				break;
			}

			}//switch
			continue;
		}//if(error)

		debug_print(DEBUG_OFF, "receiveThread: received bundle %s from %s\n", receivedTar,
				sourceEID);

		if (strstr(sourceEID, DEMUX_TOKEN_IPN_SND) != NULL
				|| strstr(sourceEID, DEMUX_TOKEN_DTN_SND) != NULL) {

			getEIDfromSender(sourceEID, sender.EID);

			//move the tar from the folder where BP store the file to the waitingBundles folder
//			sprintf(mvCommand, "mv %s %s%d", receivedTar, absoluteWaitingBundlesFolder, currentReceiveIndex/*basename(receivedTar)*/);
			sprintf(mvCommand, "mv %s %s%d_%d", receivedTar, 
					absoluteWaitingBundlesFolder, getpid(), currentReceiveIndex);
			if (system(mvCommand)) {
				error_print("receiveThread: error in system(%s)\n", mvCommand);
				memset(sourceEID, 0, sizeof(sourceEID));
				memset(receivedTar, 0, sizeof(receivedTar));
				continue;
			}

			//lock and unlock teorically not necessary... and should never be a blocking situation...
			pthread_mutex_lock(argomenti->receiveMutex);
			//copy the relative tar name into the current cell of the array
//			strcpy(
//					((argomenti->pendingReceivedBundles)[currentReceiveIndex]).relativeTarName,
//					basename(receivedTar));
			sprintf(((argomenti->pendingReceivedBundles)[currentReceiveIndex]).relativeTarName, "%d_%d",getpid(), currentReceiveIndex);
			//copy the source node into the current cell of the array
			((argomenti->pendingReceivedBundles)[currentReceiveIndex]).source =
					sender;
			pthread_mutex_unlock(argomenti->receiveMutex);



			//increment the private index of the current cell of the array
			currentReceiveIndex = (currentReceiveIndex + 1)
					% MAX_PENDING_RX_BUNDLES;

			//post to the parseTarThread
			sem_post(argomenti->processable);

			memset(sourceEID, 0, sizeof(sourceEID));
			memset(receivedTar, 0, sizeof(receivedTar));
		}	//bundle from a valid source?!
	} //fine loop ricezione


	debug_print(DEBUG_L1, "receiveThread: terminated correctly\n");
	pthread_exit(NULL);
}

//handler of the SIGUSR1 sent by the main to the receiveThread
static void receiveHandler(int signal) {

	//questo è il massimo che posso fare per gestire l'asincronicità dei segnali...
	if (signal == SIGUSR1 && receivedTar[0] == 0 && sourceEID[0] == 0) {
		debug_print(DEBUG_L1,
				"receiveThread: received signal from main, exit...\n");
		debug_print(DEBUG_L1, "receiveThread: terminated correctly\n");
		pthread_exit(NULL);
	}
}

