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
 * mainDTNbox.c
 */
//reach
#include "Controller/debugger.h"

#include <getopt.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <types.h>

#include "Controller/al_bp_wrapper.h"
#include "Controller/align.h"
#include "Controller/monitorAndSendThread.h"
#include "Controller/parseTarThread.h"
#include "DBInterface/DBInterface.h"
#include "Controller/receiveThread.h"
#include "Controller/userInterfaceThread.h"
#include "Controller/utils.h"
#include "Model/fileToSyncList.h"
#include "Model/folderToSync.h"
#include "Model/synchronization.h"
#include "Model/watchList.h"
//#include "../al_bp/src/al_bp_extB.h"

#include "Model/definitions.h"
#include "Controller/monitorAndSendThreadOperations.h"

//TODO ???
/*
       #define handle_error_en(en, msg) \
               do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
*/

static int checkAndCreateDTNboxSystemFiles(char *currentUser);
static void cleanTempFolders();
static int initialScan(char *currentUser);
static int parseMainOptions(int argc, char *argv[]);


static pthread_mutex_t quit; //variabile in mutex per terminare i demoni
static pthread_mutex_t generalMutex;	//per la sincronizzazione tra thread
static pthread_mutex_t receiveMutex;//per sincronizzaz tra receiveThread e parseTarThread (uno legge dal DB, ma l'altro legge e scrive... messa solo x sicurezza)

static pthread_t receiveThread; 	//thread di ricezione dei bundle
static pthread_t monitorAndSendThread; //thread di scan del filesystem e invio bundle
static pthread_t userControlThread; //thread di interazione utente
static pthread_t parseTarThread;	//thread di elaborazione dei bundle ricevuti

static sqlite3* userInterfaceThreadDBConn;//connessione al db, usata a regime dall'userInterfaceThread
static sqlite3* parseTarThreadDBConn;
static sqlite3* monitorAndSendThreadDBConn;

static al_bp_extB_registration_descriptor register_receiver=-1;	//connessione DTN receiver
static al_bp_extB_registration_descriptor register_sender=-1;//connessione DTN sender

static sem_t receivable;	//receivable bundles by receive thread
static sem_t processable;	//processable bundle by parse bundle thread

static int userThreadActive = 1;	//default user thread is active
static int keepRemoteNodesFrozen = 0;


void sigHandler(int var) {

	int error;
	dtnNode myNode;

	debug_print(DEBUG_OFF, "mainDTNbox: termination...\n");
	pthread_mutex_unlock(&quit);
	debug_print(DEBUG_L1, "mainDTNbox: quit mutex unlocked\n");

	//chiusura thread di interazione utente
	if (var == SIGINT && userThreadActive) {
		error = pthread_cancel(userControlThread);
		debug_print(DEBUG_L1, "mainDTNbox: sent cancel to userInterfaceThread\n");
		if (error) {
			error_print("mainDTNbox: error in pthread_cancel()\n");
			debug_print(DEBUG_L1, "mainDTNbox: error in pthread_cancel()\n");
		}
	}

	//chiusura thread di ricezione
	pthread_kill(receiveThread, SIGUSR1);
	debug_print(DEBUG_L1, "mainDTNbox: waiting for receiveThread termination\n");
	pthread_join(receiveThread, NULL);
	debug_print(DEBUG_L1, "mainDTNbox: receiveThread terminated\n");

	//chiusura thread di elaborazione bundle ricevuti
	debug_print(DEBUG_L1, "mainDTNbox: waiting for parseTarThread termination\n");
	sem_post(&processable);
	pthread_join(parseTarThread, NULL);
	debug_print(DEBUG_L1, "mainDTNbox: parseTarThread terminated\n");


	//chiusura thread di monitorAndSendThread
	pthread_join(monitorAndSendThread, NULL);
	debug_print(DEBUG_L1, "mainDTNbox: monitorAndSendThread terminated\n");

	myNode = getLocalNode();
	error = DBConnection_updateDTNnodeFrozen(userInterfaceThreadDBConn, myNode, FROZEN);
	if(error){
		error_print("mainDTNbox: error while freezing local node\n");
	}


	pthread_mutex_destroy(&quit);
	pthread_mutex_destroy(&generalMutex);
	pthread_mutex_destroy(&receiveMutex);

	sem_destroy(&processable);
	sem_destroy(&receivable);

	sqlite3_close(userInterfaceThreadDBConn);
	sqlite3_close(monitorAndSendThreadDBConn);
	sqlite3_close(parseTarThreadDBConn);



	debug_print(DEBUG_OFF, "mainDTNbox: closing DTN TX connection\n");
	if (dtnbox_closeConnection(register_sender)) {
		error_print("mainDTNbox: error in dtnbox_closeConnection()\n");
	}
	debug_print(DEBUG_OFF, "mainDTNbox: DTN TX connection closed!\n");


	debug_print(DEBUG_OFF, "mainDTNbox: closing DTN RX connection\n");
	if (dtnbox_closeConnection(register_receiver)) {
		error_print("mainDTNbox: error in dtnbox_closeConnection()\n");
	}
	debug_print(DEBUG_OFF, "mainDTNbox: DTN RX connection closed!\n");


	cleanTempFolders();

	al_bp_extB_destroy();

	watchList_destroy();
	if (debugger_destroy())
		error_print("mainDTNbox: error in debugger_destroy()\n");

	printf("mainDTNbox: DTNbox correctly terminated\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

	/* Code for compatibility with the GUI, does not change in any way the behavior of dtnbox */
		setvbuf(stdout, NULL, _IOLBF, 0);
	/* End code for GUI */

	int error;				//memorizzo il valore restituito dalle funzioni

	al_bp_endpoint_id_t eid;

	char dbFileName[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	//file del database

	//argomenti dei thread

	receiveArgs receiveArguments;
	monitorAndSendArgs scanAndsendArguments;
	userArgs userControlArguments;
	parseTarArgs parseTarArguments;

	//dtnNode myNode;
	char currentUser[OWNER_LENGTH];
	pendingReceivedBundleInfo pendingReceivedBundles[MAX_PENDING_RX_BUNDLES];

	void *res;

	char DTNboxHome[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];

	int result; //check if the local node is on DB

	//check necessario per creare il file di log
	getHomeDir(DTNboxHome);
	strcat(DTNboxHome, DTNBOXFOLDER);
	if(!fileExists(DTNboxHome)){
		error = checkAndCreateFolder(DTNboxHome);
		if(error){
			error_print("mainDTNbox: error in checkAndCreateFolder()\n");
			return error;
		}
	}

	error = parseMainOptions(argc, argv);
	if (error) {
		error_print("mainDTNbox: error in parseMainOptions()\n");
		return error;
	}

	memset(pendingReceivedBundles, 0, sizeof(pendingReceivedBundleInfo)*MAX_PENDING_RX_BUNDLES);


	//specifico l'handler per il ctrl-c

	signal(SIGINT, sigHandler);

	//sistemo le configurazioni per il multithreading
	error = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	if (error != SQLITE_OK) {
		error_print(
				"mainDTNbox: unable to set multithreading in Sqlite, required version >= 3.5.0\n");
		return ERROR_VALUE;
	}

	//apertura connessioni

	error = dtnbox_openConnection(&register_receiver, DEMUX_TOKEN_IPN_RCV,
	DEMUX_TOKEN_DTN_RCV);
	if (error) {
		error_print("mainDTNbox: error in dtnbox_openConnection()\n");
		return error;
	}

	error = dtnbox_openConnection(&register_sender, DEMUX_TOKEN_IPN_SND,
	DEMUX_TOKEN_DTN_SND);
	if (error) {
		error_print("mainDTNbox: error in dtnbox_openConnection()\n");
		return error;
	}

	eid = al_bp_extB_get_local_eid(register_receiver);
	setLocalNode(eid.uri);

	//mi ricavo l'utente dall'EID
	getOwnerFromEID(currentUser, eid.uri);
	debug_print(DEBUG_OFF, "mainDTNbox: current node: %s\n", currentUser);

	error = checkAndCreateDTNboxSystemFiles(currentUser);
	if (error) {
		error_print("mainDTNbox: error in checkAndCreateDTNboxSystemFiles()\n");
		return error;
	}

	cleanTempFolders();

	getHomeDir(dbFileName);
	strcat(dbFileName, DTNBOXFOLDER);
	strcat(dbFileName, DBFILENAME);

	if (sqlite3_open(dbFileName, &parseTarThreadDBConn)
			|| sqlite3_open(dbFileName, &userInterfaceThreadDBConn)
			|| sqlite3_open(dbFileName, &monitorAndSendThreadDBConn)) {

		error_print("mainDTNbox: error while opening a DB connection\n");
		return ERROR_VALUE;
	}

	error = DBConnection_isNodeOnDb(userInterfaceThreadDBConn, eid.uri, &result);
	if(error){
		error_print("mainDTNbox: error in DBConnection_isNodeOnDb()\n");
		return ERROR_VALUE;
	}
	if(result){
		//local node is on DB
		dtnNode localNodeOnDB;

		error = DBConnection_getDtnNodeFromEID(userInterfaceThreadDBConn, &localNodeOnDB, eid.uri);
		if(error){
			error_print("mainDTNbox: error in DBConnection_getDtnNodeFromEID()\n");
			exit(-1);
		}
		//check if local node was frozen
		if(localNodeOnDB.frozen == FROZEN){
			//DTNbox correctly terminated

			//set local node not frozen on DB
			error = DBConnection_updateDTNnodeFrozen(userInterfaceThreadDBConn, localNodeOnDB, NOT_FROZEN);
			if(error){
				error_print("mainDTNbox: error in DBConnection_updateDTNnodeFrozen()\n");
				exit(-1);
			}
			//inform all the paired nodes...
			error = prepareUnfreezeCommands(userInterfaceThreadDBConn, localNodeOnDB, keepRemoteNodesFrozen);
			if(error){
				error_print("mainDTNbox: error in prepareUnfreezeCommands()\n");
				exit(-1);
			}
			//ci penserà poi il monitorAndSendThread a inviare i comandi di Unfreeze

		}else{
			//DTNbox incorrectly terminated
			//TODO what to do ???
		}
	}
	else{
		//node not on DB... only at first DTNbox launch
		dtnNode localNode = getLocalNode();
		error = DBConnection_addDtnNode(userInterfaceThreadDBConn, localNode);
		if(error){
			error_print("mainDTNbox: error in DBConnection_addDtnNode()\n");
			exit(-1);
		}
	}




	//inotify mod
	//init watchList
	error = watchList_create();
	if (error) {
		error_print("mainDTNbox: error in watchList_create()");
		return error;
	}

	error = initialScan(currentUser);
	if (error) {
		error_print("mainDTNbox: error in initialScan()\n");
		return error;
	}

	//infine, avvio i thread e li lascio fare

	if (sem_init(&receivable, 0, MAX_PENDING_RX_BUNDLES)
			|| sem_init(&processable, 0, 0)) {
		error_print(
				"mainDTNbox: error while initializing receivable and processable semaphores\n");
		return ERROR_VALUE;
	}

	//blocco il semaforo di quit
	pthread_mutex_init(&quit, NULL);
	pthread_mutex_lock(&quit);

	pthread_mutex_init(&generalMutex, NULL);
	pthread_mutex_init(&receiveMutex, NULL);

	//creazione del thread di ricezione
	debug_print(DEBUG_L1, "mainDTNbox: creating the receiveThread\n");
	receiveArguments.register_receiver = register_receiver;
	receiveArguments.quit = &quit;
	receiveArguments.processable = &processable;
	receiveArguments.receivable = &receivable;
	receiveArguments.pendingReceivedBundles = pendingReceivedBundles;
	receiveArguments.receiveMutex = &receiveMutex;
	pthread_create(&receiveThread, NULL, receiveDaemon,
			(void*) (&receiveArguments));

	//creazione del thread dei bundle ricevuti
	debug_print(DEBUG_L1, "mainDTNbox: creating the parseTarThread\n");
	parseTarArguments.dbConn = parseTarThreadDBConn;
	parseTarArguments.myNode = getLocalNode();
	parseTarArguments.quit = &quit;
	parseTarArguments.generalMutex = &generalMutex;
	parseTarArguments.processable = &processable;
	parseTarArguments.receivable = &receivable;
	parseTarArguments.pendingReceivedBundles = pendingReceivedBundles;
	parseTarArguments.receiveMutex = &receiveMutex;
	pthread_create(&parseTarThread, NULL, parseTarDaemon,
			(void*) (&parseTarArguments));

	//creazione del thread di scan del filesystem e invio
	debug_print(DEBUG_L1, "mainDTNbox: creating the monitorAndSendThread\n");
	scanAndsendArguments.dbConn = monitorAndSendThreadDBConn;
	scanAndsendArguments.myNode = getLocalNode();
	scanAndsendArguments.register_sender = register_sender;
	scanAndsendArguments.quit = &quit;
	scanAndsendArguments.generalMutex = &generalMutex;
	pthread_create(&monitorAndSendThread, NULL, monitorAndSendDaemon, (void*) (&scanAndsendArguments));

	if(userThreadActive){
		//creazione del thread di interfaccia utente
		debug_print(DEBUG_L1, "mainDTNbox: creating the userInterfaceThread\n");
		userControlArguments.myNode = getLocalNode();
		userControlArguments.dbConn = userInterfaceThreadDBConn;
		userControlArguments.generalMutex = &generalMutex;
		pthread_create(&userControlThread, NULL, userInterfaceDaemon, (void*) (&userControlArguments));


		debug_print(DEBUG_L1, "mainDTNbox: waiting for userInterfaceThread termination\n");
		error = pthread_join(userControlThread, &res);
		debug_print(DEBUG_L1,"mainDTNbox: unlock from userInterfaceThread waiting\n");

		if(error)
			error_print("mainDTNbox: error in pthread_join(userInterfaceThread)\n");
		else{
			if(!error && res == NULL)
				sigHandler(0);
			else if(!error && res == PTHREAD_CANCELED)
				debug_print(DEBUG_L1, "mainDTNbox: userInterfaceThread has been canceled by me\n");
		}
	}else{
		debug_print(DEBUG_L1, "mainDTNbox: waiting for monitorAndSendThread termination\n");
		error = pthread_join(monitorAndSendThread, &res);	//just to stop main thread
		if(error){
			error_print("mainDTNbox: error in pthread_join(monitorAndSendThread)\n");
			exit(EXIT_FAILURE);
		}
	}
	exit(EXIT_SUCCESS);
}

static int checkAndCreateDTNboxSystemFiles(char *currentUser) {


	int error;

	char homeFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/
	char dbFileName[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH]; 	// ~/DTNbox/DTNboxDB.db
	char baseFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/foldersToSync/
	char tempDir[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];		// ~/DTNbox/.tempDir/
	char userFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/foldersToSync/ourNode/

	char outFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/out/
	char inFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];		// ~/DTNbox/.tempDir/in/

	char tempCopyFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/out/tempCopy/

	char waitingBundleFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/in/waitingBunldes/
	char currentProcessedInFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];	// ~/DTNbox/.tempDir/in/currentProcessed/

	//mi ricavo la directory home
	error = getHomeDir(homeFolder);
	if (error) {
		fprintf(stderr, "mainDTNbox: error in getHomeDir()\n");
		return error;
	}
	//mi costruisco i nomi delle directory necessari DTNBox
	strcat(homeFolder, DTNBOXFOLDER);

	strcpy(dbFileName, homeFolder);
	strcat(dbFileName, DBFILENAME);

	strcpy(baseFolder, homeFolder);
	strcat(baseFolder, FOLDERSTOSYNCFOLDER);

	strcpy(tempDir, homeFolder);
	strcat(tempDir, TEMPDIR);

	strcpy(userFolder, baseFolder);
	strcat(userFolder, currentUser);
	strcat(userFolder, "/");

	strcpy(outFolder, tempDir);
	strcat(outFolder, OUTFOLDER);

	strcpy(inFolder, tempDir);
	strcat(inFolder, INFOLDER);

	strcpy(waitingBundleFolder, inFolder);
	strcat(waitingBundleFolder, WAITINGBUNDLES);

	strcpy(currentProcessedInFolder, inFolder);
	strcat(currentProcessedInFolder, CURRENTPROCESSED);

	strcpy(tempCopyFolder, outFolder);
	strcat(tempCopyFolder, TEMPCOPY);

	//controllo che la cartella radice esista
	error = checkAndCreateFolder(homeFolder);
	if (error) {
		error_print("mainDTNbox: error in checkAndCreateFolder()\n");
		return error;
	}

	//controllo che la cartella che contiene le cartelle da sincronizzare esista
	error = checkAndCreateFolder(baseFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	//controllo che la cartella temp esista
	error = checkAndCreateFolder(tempDir);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	//controllo che la cartella dell'utente esista (chiedere)
	error = checkAndCreateFolder(userFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	error = checkAndCreateFolder(inFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	error = checkAndCreateFolder(outFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	error = checkAndCreateFolder(tempCopyFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	error = checkAndCreateFolder(currentProcessedInFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	error = checkAndCreateFolder(waitingBundleFolder);
	if (error) {
		error_print("mainDTNbox: errore checkAndCreateFolder()\n");
		return error;
	}

	//controllo se esiste gia' il database, in caso contrario lo creo
	debug_print(DEBUG_L1, "mainDTNbox: check if database exists %s...\n", dbFileName);
	error = access(dbFileName, F_OK);
	if (error && ENOENT == errno) {
		//il file non essite, procedo a chiamare la funzione di creazione del db
		debug_print(DEBUG_L1, "mainDTNbox: database not found, proceed to creation...\n");
		error = sqlite3_open(dbFileName, &userInterfaceThreadDBConn);
		if (error) {
			error_print("mainDTNbox: error in DBConnection_open()\n");
			return ERROR_VALUE;
		}
		error = DBConnection_createTables(userInterfaceThreadDBConn);
		if (error) {
			error_print("mainDTNbox: unable to create the tables on database\n");
			return ERROR_VALUE;
		}
		//chiudo la connessione al db
		error = sqlite3_close(userInterfaceThreadDBConn);
		if (error != SQLITE_OK) {
			error_print("mainDTNbox: error while closing the database connection\n");
			return ERROR_VALUE;
		}
	} else {
		debug_print(DEBUG_L1, "mainDTNbox: database found\n");
	}

	return SUCCESS_VALUE;
}

//funcion called only by the main at the startup of the program for consistence between FS and DB
static int initialScan(char *currentUser) {

	int error;
	int i;

	folderToSync* folders = NULL;
	int numFolders;

	char absoluteFilePath[LINUX_ABSOLUTE_PATH_LENGTH];

	getHomeDir(absoluteFilePath);
	strcat(absoluteFilePath, DTNBOXFOLDER);
	strcat(absoluteFilePath, FOLDERSTOSYNCFOLDER);
	strcat(absoluteFilePath, currentUser);
	strcat(absoluteFilePath, "/");

	//add to DB all the folders present in ~/DTNbox/foldersToSync/thisNode/ but not in DB
	error = scanFolders(userInterfaceThreadDBConn, absoluteFilePath, currentUser);
	if (error) {
		error_print("initialScan: error in scanFolders()\n");
		return error;
	}

	//get all folders to monitor with inotify
	error = DBConnection_getAllFoldersToMonitor(userInterfaceThreadDBConn, &folders,
			&numFolders, currentUser);
	if (error) {
		error_print(
				"initialScan: error in DBConnection_getAllFoldersToMonitor()\n");
		return ERROR_VALUE;
	}

	for (i = 0; i < numFolders; i++) {

		error = getAbsolutePathFromOwnerAndFolder(absoluteFilePath, (folders[i]).owner,
				(folders[i]).name);
		if (error) {
			for (i = 0; i < numFolders; i++)
				fileToSyncList_destroy(folders[i].files);
			free(folders);
			error_print(
					"initialScan: error in getAbsolutePathFromOwnerAndFolder()\n");
			return error;
		}


		//faccio una scansione di tutta la cartella di sincronizzazione per vedere se ci sono state modifiche mentre DTNbox era off
		error = alignFSandDB(userInterfaceThreadDBConn, folders[i], absoluteFilePath);
		if (error) {
			error_print("initialScan: error in alignFSandDB()\n");
			for (i = 0; i < numFolders; i++)
				fileToSyncList_destroy(folders[i].files);
			free(folders);
			return error;
		}
	}

	//no errors, free the memory
	if (folders != NULL) {
		for (i = 0; i < numFolders; i++)
			fileToSyncList_destroy(folders[i].files);
		free(folders);
	}

	return SUCCESS_VALUE;
}	//initialScan()


static int parseMainOptions(int argc, char *argv[]){

	int error=0;

	int done = 0;
	int option_index = 0;
	char c;

	//config values
	char logFileName[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];
	debug_level debug = DEBUG_OFF;
	boolean_t create_log = FALSE;

	char force_eid = 'N';
	int ipn_local_number = 0;


	getHomeDir(logFileName);
	strcat(logFileName, DTNBOXFOLDER);
	strcat(logFileName, LOGFILENAME);

	keepRemoteNodesFrozen = 0;
	done=0;
	while(!done){

		static struct option long_options[] =
		{
				{"debug", optional_argument, 0, 1},
				{"log", no_argument, 0, 2},
				{"force-eid", required_argument, 0,3},
				{"ipn-local", required_argument, 0, 4},
				{"help", no_argument, 0, 5},
				{"no-input", no_argument, 0, 6},
				{"keep-frozen", no_argument, 0, 7},
				{0,0,0,0}
		};

		c = getopt_long(argc, argv, "", long_options, &option_index);

		switch(c){

		case 1:{
			//debug
			if(optarg){
				int debug_level = atoi(optarg);
				if(debug_level >= 0 && debug_level <=2){
					debug = debug_level;
				}else{
					error_print("main: invalid debug level\n");
					return ERROR_VALUE;
				}
			}
			else
				debug = DEBUG_L1;
			break;
		}
		case 2:{
			//log
			create_log = TRUE;
			break;
		}
		case 3:{
			//force-eid
			if(optarg){
				if(strcmp(optarg, "IPN") != 0 && strcmp(optarg, "DTN") != 0/*(optarg[0] != 'N' && optarg[0] != 'I' && optarg[0] != 'D')*/){
					error_print("main: invalid force-eid parameter\n");
					return ERROR_VALUE;
				}
				else{
					force_eid = optarg[0];
					if(error){
						error_print("main: error in al_bp_extB_init()\n");
						return ERROR_VALUE;
					}
				}
			}
			else{
				error_print("main: force-eid parameter\n");
				return ERROR_VALUE;
			}
			break;
		}
		case 4:{
			//ipn-local
			if(optarg){
				ipn_local_number = 0;
				ipn_local_number = atoi(optarg);
				if(ipn_local_number <= 0){
					error_print("main: wrong --ipn-local argument parameter\n");
					return ERROR_VALUE;
				}
				break;
			}else{
				error_print("main: ipn-local require an integer argument parameter\n");
				return ERROR_VALUE;
			}
		}
		case 5 : {
			//help
			error_print("\n");
			error_print("DTNbox version %s\n", DTNBOX_VERSION);
			error_print("DTNbox\n");
			error_print("options:\n"
					"     --log                        Create a log file. Default log filename is %s.\n"
					"     --debug[=level]              Debug messages [1-2]; if level is not indicated level = 2.\n"
					"     --force-eid <[DTN|IPN]>      Force scheme of registration EID.\n"
					"     --ipn-local <num>            Set ipn local number (Use only on DTN2)\n"
					"     --no-input                   Do not start UserInterfaceThread\n"
					"     --keep-frozen                Keep frozen remote nodes\n"
					"     --help                       This help.\n", LOGFILENAME);
			error_print("\n");
			exit(EXIT_FAILURE);
		}
		case 6 : {
			//no input
			userThreadActive = 0;
			close(0); //close stdin
			break;
		}
		case 7 : {
			//keep-frozen
			keepRemoteNodesFrozen = 1;
			break;
		}
		case (char)(-1):{
			done = 1;
			break;
		}

		default: {
			error_print("main: invalid input parameter, %c\n", c);
			return ERROR_VALUE;
		}
		}//switch
	}//while(!done)

	//if underlying implementation is DTN2 and forced scheme is "ipn", ipn-local must be set
	if (al_bp_get_implementation() == BP_DTN && force_eid == 'I' && ipn_local_number <= 0)
	{
		error_print("main: error in \"--force-eid option\". To use ipn registration in DTN2 implementation,"
				"you must set the local ipn number with the option \"--ipn-local\"\n");
		return ERROR_VALUE;
	}

	error = debugger_init(debug, create_log, create_log ? logFileName : NULL);
	if(error){
		error_print("main: error in debugger_init()\n");
		return error;
	}
	error = al_bp_extB_init(force_eid, ipn_local_number);
	if(error == BP_EXTB_ERRNOIMPLEMENTATION){
		error_print("main: error, no implementation found\n");
		return ERROR_VALUE;
	}
	else if(error){
		error_print("main: error in al_bp_extB_init()\n");
		return ERROR_VALUE;
	}

	switch(al_bp_get_implementation()){
	case(BP_DTN):{
		debug_print(DEBUG_OFF, "DTN2 found\n");
		break;
	}
	case(BP_ION):{
		debug_print(DEBUG_OFF, "ION found\n");
		break;
	}
	case(BP_IBR):{
		debug_print(DEBUG_OFF, "IBR found\n");
		break;
	}
	default:{
		error_print("main: unknown implementation\n");
		return ERROR_VALUE;
	}
	}

	return SUCCESS_VALUE;
}


static void cleanTempFolders() {

	char systemCommand[SYSTEM_COMMAND_LENGTH + 2*DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];

	char tempDirFolder[DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH];

	getHomeDir(tempDirFolder);
	strcat(tempDirFolder, DTNBOXFOLDER);
	strcat(tempDirFolder, TEMPDIR);
	strcat(tempDirFolder, INFOLDER);

	sprintf(systemCommand, "rm -rf %s%s* %s%s*", tempDirFolder,
	CURRENTPROCESSED, tempDirFolder, WAITINGBUNDLES);
	//debug_print(DEBUG_L1, "systemCommand: %s\n", systemCommand);
	system(systemCommand);

	getHomeDir(tempDirFolder);
	strcat(tempDirFolder, DTNBOXFOLDER);
	strcat(tempDirFolder, TEMPDIR);
	strcat(tempDirFolder, OUTFOLDER);
	sprintf(systemCommand, "rm -rf %s%s*", tempDirFolder, TEMPCOPY);
	//debug_print(DEBUG_L1, "systemCommand: %s\n", systemCommand);
	system(systemCommand);

	return;
}

