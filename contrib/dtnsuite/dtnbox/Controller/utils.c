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
 * utils.c
 *
 * -funzioni di utility varie per adattare la modifica inotify
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "utils.h"
#include "../DBInterface/DBInterface.h"
#include "../Model/definitions.h"
#include "debugger.h"

#include <libgen.h>

#include <signal.h>


static dtnNode localNode;


//restituisce il tempo in millisecondi a partire da EPOCH
unsigned long long getCurrentTime() {
	unsigned long long currentTimeStamp;

	//prendo timestamp
	currentTimeStamp = time(NULL);
	return currentTimeStamp;
}

//controlla che la cartella esista sul filesystem e in caso negativo la crea, utilizzare path assoluto
int checkAndCreateFolder(char* folderName) {
	int error;
	DIR* tempFolder;

	//controllo i parametri passati
	if (folderName == NULL || !strcmp(folderName, "")) {
		error_print("checkAndCreateFolder: error in folderName parameter\n");
		return ERROR_VALUE;
	}

	//controllo che la cartella passata esista
	tempFolder = opendir(folderName);
	if (tempFolder) {
		//la cartella esiste gia'
		closedir(tempFolder);
		return SUCCESS_VALUE;
	} else if (ENOENT == errno) {
		//la cartella non esiste, procedo a crearla
//		debug_print(DEBUG_L1, "checkAndCreateFolder: Creazione cartella %s\n",
//				folderName);
		error = mkdir(folderName, 0700);
		if (error < 0) {
			error_print("checkAndCreateFolder: error while creating folder %s\n",
					folderName);
			return ERROR_VALUE;
		}
	} else {
		error_print("checkAndCreateFolder: error in opendir()\n");
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}

//mi toglie il carattere \n dalla stringa
void stripNewline(char* string) {
	char* tempString = string;
	while (*string != '\0') {
		if (*string != '\n') {
			*tempString++ = *string++;
		} else {
			++string;
		}
	}
	*tempString = '\0';
}

//ricavo il nome owner dall'EID;
void getOwnerFromEID(char* owner, char* EID) {
	char* temp;
	char tempStr[TEMP_STRING_LENGTH];

	strcpy(tempStr, EID);

	temp = strtok(tempStr, ".");
	temp = strtok(temp, ":");
	temp = strtok(NULL, ":");

	if (strstr(EID, "ipn") != NULL) {

		//stringa ipn, es: ipn:17.1
		strcpy(owner, temp);

	} else if (strstr(EID, "dtn") != NULL) {

		//stringa dtn es: dtn://nodo.dtn/DTNbox
		temp = strtok(temp, "//");
		strcpy(owner, temp);

	} else {

		error_print(
				"getOwnerFromEID: error, unknown EID schema: %s\n",
				EID);
	}
}

//ricavo la directory home
int getHomeDir(char* homeFolder) {
	struct passwd* pw;

	//mi ricavo la directory home
	pw = getpwuid(getuid());

	if (pw != NULL && pw != 0) {
		strcpy(homeFolder, pw->pw_dir);
		strcat(homeFolder, "/");
		return SUCCESS_VALUE;

	} else {
		error_print("getHomeDir: error in getpwuid()\n");
		return ERROR_VALUE;
	}

	return ERROR_VALUE;
}

//controlla se un nodo e' valido, 1 se tutto ok 0 se negativo
int isDtnNodeValid(dtnNode node) {

	//controllo che i parametri passati siano validi
	if (node.EID == NULL || !strcmp(node.EID, "")) {
		error_print("isDtnNodeValid: error, empty EID\n");
		return 0;
	}

	//devo controllare se il lifetime e i numRetry sono != da 0?

	return 1;
}

//controlla se una cartella e' valida, 1 se tutto ok 0 se negativo
int isFolderToSyncValid(folderToSync folder) {

	//controllo che i parametri passati siano validi
	if (folder.name == NULL || !strcmp(folder.name, "")) {
		error_print("isFolderValid: error, empty folderName parameter\n");
		return 0;
	}
	if (folder.owner == NULL || !strcmp(folder.owner, "")) {
		error_print("isFolderValid: error, empty owner parameter\n");
		return 0;
	}

	return 1;
}

//controlla se un file e' valido, 1 se tutto ok 0 se negativo
int isFileToSyncValid(fileToSync file) {

	//controllo che i parametri passati siano validi
	if (file.name == NULL || !strcmp(file.name, "")) {
		error_print("isFileValid: error, empty fileName parameter\n");
		return 0;
	}

	return 1;
}

//controlla se una sincronizzazione e' valida, 1 se tutto ok 0 se negativo
int isSynchronizationValid(synchronization sync) {

	//controllo che la cartella passata sia valida
	if (!isFolderToSyncValid(sync.folder)) {
		return 0;
	}

	//controllo che il nodo sia valido
	if (!isDtnNodeValid(sync.node)) {
		return 0;
	}

	return 1;
}

//controlla se un comando e' valido, 1 se tutto ok 0 se negativo
int isCommandValid(command cmd) {
	if (cmd.text == NULL || !strcmp(cmd.text, "")) {
		error_print("isCommandValid: error, cmd text empty\n");
		return 0;
	}

	return 1;
}

//dato il nome assoluto di un tar ne estrae il contenuto nella directory specificata
int tarToFiles(char* tarName, char* destDir) {
	int error;
	char* inizio = "tar xfp ";
	char* destOpt = " -C ";
	char tarCommand[SYSTEM_COMMAND_LENGTH + 2*(DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH)];

	//creo il comando e ci metto costante + nomeTar
	strcpy(tarCommand, inizio);
	strcat(tarCommand, tarName);
	strcat(tarCommand, destOpt);
	strcat(tarCommand, destDir);

	//chiamo tar
	error = system(tarCommand);
	if(error){
		error_print("tarToFiles: error in system(%s)\n", tarCommand);
		return ERROR_VALUE;
	}
	return SUCCESS_VALUE;
}

//controlla se un dato file e' effettivamente un file o una directory
int isFile(const char* path) {
	struct stat pathStat;
	int err = stat(path, &pathStat);
	return (err == -1) ? 0 : S_ISREG(pathStat.st_mode);
}

//controlla se un dato file (su FS) e' una directory
int isFolder(const char* path) {

	//return closedir(opendir(path))==0;
	struct stat s;
	int err = stat(path, &s);

	return (err == -1) ? 0 : S_ISDIR(s.st_mode);
}

//controlla se esiste un file sul filesystem
int fileExists(char* filePath) {
	struct stat fileInfo;
	return (stat(filePath, &fileInfo) == 0);
}

//dato un nodo restituisce la prossima data di ritrasmissione per il comando o il file
unsigned long long getNextRetryDate(dtnNode node) {
	return (getCurrentTime() + (node.lifetime * 2));
}

//restituisce il lifetime per un nodo di default in SECONDI
int getDefaultLifetime() {
	return DEFAULT_LIFETIME;
}

//restituisce il numero minimo di ritrasmissioni di default per un nodo
int getDefaultNumRetry() {
	return DEFAULT_NUM_RETRY;
}

//funzione per il controllo e l'aggiunta delle cartelle da monitorare
int scanFolders(sqlite3* dbConn, char* userFolder, char* currentUser) {
	int isFolderOnDb;
	int error;
	char completeDirName[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	DIR* userFolderDir;
	struct dirent* foldersToChek;
	folderToSync tempFolder;

	userFolderDir = opendir(userFolder);
	debug_print(DEBUG_L1,
			"scanFolders: checking folders to add to database...\n");

	if (userFolderDir != NULL) {
		while ((foldersToChek = readdir(userFolderDir)) != NULL) {

			strcpy(completeDirName, userFolder);
			strcat(completeDirName, foldersToChek->d_name);

			//controllo che sia effettivamente una cartella
			if (strcmp(foldersToChek->d_name, ".") != 0
					&& strcmp(foldersToChek->d_name, "..") != 0
					&& isFolder(completeDirName)) {
				//recupero nome e owner della cartella

				//ho recuperato le info che mi servono della cartella, procedo ad aggiungerla al database
				//non cerco i file in quanto dovrebbe essere compito di fsScanThread di fare il primo giro
				tempFolder.files = NULL;
				strcpy(tempFolder.name, foldersToChek->d_name);
				strcpy(tempFolder.owner, currentUser);

				//controllo che la cartella sia sul database
				error = DBConnection_isFolderOnDb(dbConn, tempFolder,
						&isFolderOnDb);
				if (error) {
					error_print(
							"scanFolders: error in DBConnection_isFolderOnDb() %d\n",
							error);
					return ERROR_VALUE;
				}

				if (isFolderOnDb) {
					//la cartella e' gia presente, non l'aggiungo
				} else {
					//manca la cartella sul db, la aggiungo
					error = DBConnection_addFolderToSync(dbConn, tempFolder);
					if (error) {
						error_print(
								"scanFolders: error while adding a folder %s\n",
								tempFolder.name);
						return ERROR_VALUE;
					}
					debug_print(DEBUG_L1,
							"scanFolders: added new folder %s\n",
							tempFolder.name);
				}
			}
		}
		closedir(userFolderDir);
	} else {
		error_print("scanFolders: unable to open folder %s\n",
				userFolder);
		return ERROR_VALUE;
	}
	debug_print(DEBUG_L1, "scanFolders: folders check completed\n");
	return SUCCESS_VALUE;
}

//a partire dall'eid di invio (es ipn:1.3001) restituisce l'eid di ricezione (es ipn:1.3000)
void getEIDfromSender(char* srcEID, char* destEID) {
	char* temp;

	temp = strtok(srcEID, ".");

	if (strstr(temp, "ipn") != NULL) {

		//stringa ipn, es: ipn:17.1
		sprintf(destEID, "%s.%s", temp, DEMUX_TOKEN_IPN_RCV);

	} else if (strstr(temp, "dtn") != NULL) {

		//stringa dtn es: dtn://nodo.dtn/DTNbox
		sprintf(destEID, "%s.dtn%s", temp, DEMUX_TOKEN_DTN_RCV);

	} else {

		error_print(
				"getEIDfromSender: unknown EID schema: %s\n",
				srcEID);
	}
}

//function called to check the termination of the program
int terminate(pthread_mutex_t* quit) {

	switch (pthread_mutex_trylock(quit)) {

	case 0:
		pthread_mutex_unlock(quit);
		return ERROR_VALUE;

	case EBUSY:
		return SUCCESS_VALUE;
	}

	return ERROR_VALUE;
}

/*
 funzione che crea nel percorso assoluto dest, le cartelle del percorso relativo subPath, dove subPath è il nome di un file...
 usata per parametri del tipo
 dest = /cartella/cartella oppure dest = /cartella/cartella/
 subPath del tipo cartella/cartella/file
 usata per creare albero di cartelle per file da mettere dentro il tar
*/
int createFolderForPath(char *dest, char *subPath) {

	char currentSubfolder[TEMP_STRING_LENGTH];
	int i = 0;
	int current = 0;

	if (dest[strlen(dest) - 1] != '/')
		strcat(dest, "/");

	for (i = 0; i < strlen(subPath); i++) {
		if (subPath[i] == '/') {
			currentSubfolder[current] = '\0';
			strcat(dest, currentSubfolder);
			strcat(dest, "/");
			if (checkAndCreateFolder(dest)) {
				error_print(
						"createFolderForPath: error in checkAndCreateFolder()\n");
				return ERROR_VALUE;
			}
			current = 0;
		} else if (subPath[i] != '/')
			currentSubfolder[current++] = subPath[i];
	}
	return SUCCESS_VALUE;
}

//check if string starts with prefis
int startsWith(char *string, char *prefix) {
	return (strncmp(string, prefix, strlen(prefix)) == 0);
}

//check if string ends with suffix
int endsWith(char *string, char *suffix) {

	int stringLen = strlen(string);
	int suffixLen = strlen(suffix);

	if (suffixLen > stringLen)
		return 0;

	return strncmp(&(string[stringLen - suffixLen]), suffix, suffixLen) == 0;
}

//return ~/DTNbox/foldersToSync/
int getFoldersToSyncPath(char *dest) {

	if (dest == NULL)
		return ERROR_VALUE;

	if (getHomeDir(dest))
		return ERROR_VALUE;

	strcat(dest, DTNBOX_FOLDERSTOSYNCFOLDER);
	return SUCCESS_VALUE;
}

//return ~/DTNbox/foldersToSync/owner/folder/
int getAbsolutePathFromOwnerAndFolder(char *absolutePath, char *owner, char *folder) {

	if (absolutePath == NULL || owner == NULL || folder == NULL
			|| strcmp(owner, "") == 0 || strcmp(folder, "") == 0)
		return ERROR_VALUE;

	if (getFoldersToSyncPath(absolutePath))
		return ERROR_VALUE;

	strcat(absolutePath, owner);
	strcat(absolutePath, "/");
	strcat(absolutePath, folder);
	strcat(absolutePath, "/");
	return SUCCESS_VALUE;
}

//set owner and folder values
void getOwnerAndFolderFromPath(char *path, char *owner, char *folder) {

	char temp[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	int baseLen;
	char current;
	int i;
	getFoldersToSyncPath(temp);

	baseLen = strlen(temp);

	i = 0;
	while ((current = path[baseLen]) != '/' && current != '\0') {
		owner[i++] = current;
		baseLen++;
	}
	owner[i] = '\0';

	baseLen++;	//to skip the /
	i = 0;
	while ((current = path[baseLen]) != '/' && current != '\0') {
		folder[i++] = current;
		baseLen++;
	}
	folder[i] = '\0';
}

//returns a folderToSync with owner and folder fields
folderToSync getEmptyFileListFolderFromOwnerAndFolder(char *owner, char *folder) {

	folderToSync res;
	strcpy(res.owner, owner);
	strcpy(res.name, folder);
	res.files = NULL;
	return res;
}

//relative to the synchronization folder (e.g. subFolderA/subfolderB/file)
void getRelativeFilePathFromAbsolutePath(char *relativeFilePath, char *absolutePath) {

	int baseLen;
	char current;

	getFoldersToSyncPath(relativeFilePath);

	baseLen = strlen(relativeFilePath);

	while ((current = absolutePath[baseLen]) != '/' && current != '\0')
		baseLen++;

	baseLen++;

	while ((current = absolutePath[baseLen]) != '/' && current != '\0')
		baseLen++;

	baseLen++; //to skip '/'

	strcpy(relativeFilePath, &(absolutePath[baseLen]));
}

//copy into syncFolderPath the folder path of the absolute file
void getSyncFolderPathFromAbsoluteFilePath(char* syncFolderPath, char *absolutePath) {

	int len;
	char current;

	getFoldersToSyncPath(syncFolderPath);

	len = strlen(syncFolderPath);

	while ((current = absolutePath[len]) != '/' && current != '\0') {
		syncFolderPath[len] = absolutePath[len];
		len++;
	}

	syncFolderPath[len] = absolutePath[len];
	len++;

	while ((current = absolutePath[len]) != '/' && current != '\0') {
		syncFolderPath[len] = absolutePath[len];
		len++;
	}

	syncFolderPath[len] = absolutePath[len];
	len++;
	syncFolderPath[len] = '\0';
}

//returns the related folderToSync from the absolute path of a file
folderToSync getFolderToSyncFromAbsoluteFilePath(char *absolutePath) {

	folderToSync res;
	char owner[OWNER_LENGTH];
	char folder[SYNC_FOLDER_LENGTH];

	getOwnerAndFolderFromPath(absolutePath, owner, folder);
	res = getEmptyFileListFolderFromOwnerAndFolder(owner, folder);

	return res;
}

//returns the related folder ID from a given absolute file path
int getFolderToSyncIDFromAbsolutePath(sqlite3* conn, int *folderToSyncID,
		char *absolutePath) {

	folderToSync folder = getFolderToSyncFromAbsoluteFilePath(absolutePath);
	int error;
	int res;

	error = DBConnection_getFolderToSyncID(conn, folder, &res);
	if (error) {
		error_print(
				"getFolderToSyncIDFromAbsolutePath(): error in DBConnection_getFolderToSyncID()\b");
		return ERROR_VALUE;
	}

	*folderToSyncID = res;
	return SUCCESS_VALUE;
}

//Siccome nel DB le cartelle le salviamo con un / finale...
//in questo modo le differenziamo dai files normali!!
int fileIsFolderOnDb(char *file) {
	return file[strlen(file) - 1] == '/';
}

//add a command to DB
int DBConnection_controlledAddCommand(sqlite3 *dbConn, command *cmd) {

	int error;
	int commandOnDB;

	commandOnDB = 0;
	error = DBConnection_isCommandOnDb(dbConn, *cmd, &commandOnDB);
	if (error) {
		error_print(
				"DBConnection_controlledAddCommand(): error in DBConnection_isCommandOnDb()\n");
		return ERROR_VALUE;
	}
	if (!commandOnDB) {
		//se non su DB -> lo aggiungo
		error = DBConnection_addCommand(dbConn, *cmd);
		if (error) {
			error_print(
					"DBConnection_controlledAddCommand(): error in DBConnection_addCommand()\n");
			return ERROR_VALUE;
		}
	}

	return SUCCESS_VALUE;
}

//add to DB all the syncOnFile for a given synchronization
//used when a synchronization is established (not the first, as the first syncrhonization need a scan of the folder)
int DBConnection_addSyncsOnFileForSynchronization(sqlite3 *dbConn,
		synchronization sync) {

	int folderToSyncID;
	char absoluteFilePath[FILETOSYNC_ABSOLUTE_PATH_LENGTH];

	int folderNameLength;
	int error;

	fileToSyncList files;	//memoria da liberare
	fileToSyncList toDelete;

	int tempFileTxAttempts;
	unsigned long long tempFileNextRetryDate;

	error = DBConnection_getFolderToSyncID(dbConn, sync.folder,
			&folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_addSyncsOnFileForSynchronization: error in DBConnection_getFolderToSyncID()");
		return ERROR_VALUE;
	}

	getAbsolutePathFromOwnerAndFolder(absoluteFilePath, sync.folder.owner,
			sync.folder.name);
	folderNameLength = strlen(absoluteFilePath);

	files = NULL;
	toDelete = NULL;

	error = DBConnection_getFilesToSyncFromFolder(dbConn, &files,
			folderToSyncID);
	if (error) {
		error_print(
				"DBConnection_addSyncsOnFileForSynchronization: error in DBConnection_getFilesToSyncFromFolder()\n");
		return ERROR_VALUE;
	}

	toDelete = files;
	while (toDelete != NULL) {


		strcat(absoluteFilePath, toDelete->value.name);

		if (/*result && */fileExists(absoluteFilePath)
				&& ((!fileIsFolderOnDb(toDelete->value.name)
						&& isFile(absoluteFilePath))
						|| (fileIsFolderOnDb(toDelete->value.name)
								&& isFolder(absoluteFilePath)))) {
			//aggiungiamo la sincronizzazione su file solo se è sia sul DB che sul FS
			toDelete->value.state = FILE_DESYNCHRONIZED;
			error = DBConnection_addSyncOnFile(dbConn, sync, toDelete->value,
					folderToSyncID);
			if (error) {
				error_print(
						"DBConnection_addSyncsOnFileForSynchronization: error in DBConnection_addSyncOnFile()");
				fileToSyncList_destroy(files);
				return ERROR_VALUE;
			}

			tempFileTxAttempts = sync.node.numTx;
			tempFileNextRetryDate = getNextRetryDate(
					sync.node);
			error = DBConnection_updateSyncOnFileTxLeftAndRetryDate(
					dbConn, sync, toDelete->value,
					folderToSyncID, tempFileTxAttempts,
					tempFileNextRetryDate);
			if (error) {
				error_print(
						"DBConnection_addSyncsOnFileForSynchronization: error in DBConnection_updateSyncOnFileTxAttemptsAndRetryDate()\n");
				fileToSyncList_destroy(files);
				return ERROR_VALUE;
			}
		}

		absoluteFilePath[folderNameLength] = '\0';
		toDelete = toDelete->next;
	}
	fileToSyncList_destroy(files);
	return SUCCESS_VALUE;
}

//the SIGINT signal need to be handled only by the main thread

int disableSigint() {

	sigset_t blockSet, prevMask;

	if (sigemptyset(&blockSet)) {
		error_print("disableSigint: error in sigemptyset()\n");
		return ERROR_VALUE;
	}
	if (sigaddset(&blockSet, SIGINT)) {
		error_print("disableSigint: error in sigaddset()\n");
		return ERROR_VALUE;
	}

	if (pthread_sigmask(SIG_BLOCK, &blockSet, &prevMask)) {
		error_print("disableSigint: error in pthread_sigmask()\n");
		return ERROR_VALUE;
	}

	return SUCCESS_VALUE;
}

//compare funtion used to sort files
//funzione necessaria per mettere prima i files deleted, poi in ordine alfabetico... in modo che le cartelle siano prima dei files dentro le cartelle stesse
int compare(const void *a, const void *b) {

	fileToSync fileA = *((fileToSync*) a);
	fileToSync fileB = *((fileToSync*) b);

	if (fileA.deleted && !fileB.deleted)
		return -1;
	else if (!fileA.deleted && fileB.deleted)
		return 1;
	else{
		if(fileA.deleted)
			return (-1)*strcmp(fileA.name, fileB.name);
		else
			return strcmp(fileA.name, fileB.name);
	}
}

//check if the directory is empty
int isDirectoryEmpty(char *dirname) {
	int n = 0;
	struct dirent *d;
	DIR *dir = opendir(dirname);
	if (dir == NULL) //Not a directory or doesn't exist
		return 1;
	while ((d = readdir(dir)) != NULL) {
		if (++n > 2)
			break;
	}
	closedir(dir);
	if (n <= 2) //Directory Empty
		return 1;
	else
		return 0;
}

//usata in caso di move di una (sotto)cartella per recuperare dal DB tutti i files dentro la sottocartella spostata
//e.g if the (sub)folder absolute path ~/DTNbox/foldersToSync/owner/folderName/subA/subB/subC/ is moved... relativeSubfolderPath will contains subA/subB/subC/
int getRelativeSubfolderPathFromAbsolutePath(char *relativeSubfolderPath,
		char *absolutePath) {

	char tempPath[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];
	char current;
	int len;

	getHomeDir(tempPath);
	strcat(tempPath, DTNBOX_FOLDERSTOSYNCFOLDER);

	len = strlen(tempPath);

	while ((current = absolutePath[len]) != '/' && current != '\0') {
		tempPath[len] = absolutePath[len];
		len++;
	}

	tempPath[len] = absolutePath[len];
	len++;

	while ((current = absolutePath[len]) != '/' && current != '\0') {
		tempPath[len] = absolutePath[len];
		len++;
	}

	tempPath[len] = absolutePath[len];
	len++;
	tempPath[len] = '\0';

	strcpy(relativeSubfolderPath, &(absolutePath[strlen(tempPath)]));
	return SUCCESS_VALUE;
}


//check if folderPath is like "~/DTNbox/foldersToSync/owner/folderName/"
int isSynchronizationFolderPath(char *folderPath){

	char tempString[FOLDERTOSYNC_ABSOLUTE_PATH_LENGTH];

	getSyncFolderPathFromAbsoluteFilePath(tempString, folderPath);

	return
		strcmp(tempString, folderPath) == 0;
}

void setLocalNode(char *EID){
	strcpy(localNode.EID, EID);
	localNode.lifetime = DEFAULT_LIFETIME;
	localNode.numTx = DEFAULT_NUM_RETRY;

	localNode.blackWhite = WHITELIST;
	localNode.blockedBy = NOT_BLOCKEDBY;
	localNode.frozen = NOT_FROZEN;
}

dtnNode getLocalNode(){
	return localNode;
}


int getAckCommandSize(char *receivedCommand) {

	// + 8 for Ack suffix :)
	if(startsWith(receivedCommand, "Sync\t")){
		return SYNC_COMMAND_TEXT_SIZE_LIMIT + 8;

	} else if (startsWith(receivedCommand, "Update\t")){
		return UPDATE_COMMAND_TEXT_SIZE_LIMIT + 8;

	} else if (startsWith(receivedCommand, "CheckUpdate\t")){
		return CHECKUPDATE_COMMAND_TEXT_SIZE_LIMIT + 8;

	} else if (startsWith(receivedCommand, "Unfreeze\t")){
		return UNFREEZE_COMMAND_TEXT_SIZE_LIMIT + 8;

	} else if (startsWith(receivedCommand, "Fin\t")){
		return FIN_COMMAND_TEXT_SIZE_LIMIT + 8;
	} else {
		//for the commands not listed above, there is no ack...
		return 0;
	}
}










