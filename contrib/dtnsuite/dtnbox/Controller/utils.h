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
 * utils.h
 */

#ifndef CONTROLLER_UTILS_H_
#define CONTROLLER_UTILS_H_

#include <pthread.h>
#include "../Model/folderToSync.h"
#include "../Model/fileToSync.h"
#include "../Model/synchronization.h"
#include "../Model/command.h"

//restituisce il tempo in millisecondi a partire da EPOCH
unsigned long long getCurrentTime();
//controlla che la cartella esista sul filesystem e in caso negativo la crea, utilizzare path assoluto
int checkAndCreateFolder(char* folderName);
//mi toglie il carattere \n dalla stringa
void stripNewline(char* string);
//ricavo il nome owner dall'EID
void getOwnerFromEID(char* owner, char* EID);
//ricavo la directory home
int getHomeDir(char* homeFolder);
//controlla se un nodo e' valido, 1 se tutto ok 0 se negativo
int isDtnNodeValid(dtnNode node);
//controlla se una cartella e' valida, 1 se tutto ok 0 se negativo
int isFolderToSyncValid(folderToSync folder);
//controlla se un file e' valido, 1 se tutto ok 0 se negativo
int isFileToSyncValid(fileToSync file);
//controlla se una sincronizzazione e' valida, 1 se tutto ok 0 se negativo
int isSynchronizationValid(synchronization sync);
//controlla se un comando e' valido, 1 se tutto ok 0 se negativo
int isCommandValid(command cmd);
//dato il nome assoluto di un tar ne estrae il contenuto nella directory specificata
int tarToFiles(char* tarName, char* destDir);
//controlla se un dato file e' effettivamente un file o una directory
int isFile(const char* path);
//controlla se un dato file (su FS) e' una directory
int isFolder(const char* path);
//controlla se esiste un file sul filesystem
int fileExists(char* filePath);
//dato un nodo restituisce la prossima data di ritrasmissione per il comando o il file
unsigned long long getNextRetryDate(dtnNode node);
//restituisce il lifetime per un nodo di default in SECONDI
int getDefaultLifetime();
//restituisce il numero minimo di ritrasmissioni di default per un nodo
int getDefaultNumRetry();
//funzione per il controllo e l'aggiunta delle cartelle da monitorare
int scanFolders(sqlite3* dbConn, char* userFolder, char* currentUser);
//funzione che restituisce l'EID del nodo in ascolto a partire dal mittente
void getEIDfromSender(char* srcEID, char* destEID);
//funzione per segnalare l'uscita ai thread
int terminate(pthread_mutex_t* quit);
/*
 funzione che crea nel percorso assoluto dest, le cartelle del percorso relativo subPath, dove subPath è il nome di un file...
 usata per parametri del tipo
 dest = /cartella/cartella oppure dest = /cartella/cartella/
 subPath del tipo cartella/cartella/file
 usata per creare albero di cartelle per file da mettere dentro il tar
*/
int createFolderForPath(char *dest, char *subPath);
//check if string starts with prefis
int startsWith(char *string, char *prefix);
//check if string ends with suffix
int endsWith(char *string, char *suffix);
//return ~/DTNbox/foldersToSync/
int getFoldersToSyncPath(char *dest);
//return ~/DTNbox/foldersToSync/owner/folder/
int getAbsolutePathFromOwnerAndFolder(char *absolutePath, char *owner, char *folder);
//returns a folderToSync with owner and folder fields
folderToSync getEmptyFileListFolderFromOwnerAndFolder(char *owner, char *folder);
//relative to the synchronization folder (e.g. subFolderA/subfolderB/file)
void getRelativeFilePathFromAbsolutePath(char *relativeFilePath, char *absolutePath);
//copy into syncFolderPath the folder path of the absolute file
void getSyncFolderPathFromAbsoluteFilePath(char* syncFolderPath, char *absolutePath);
//returns the related folderToSync from the absolute path of a file
folderToSync getFolderToSyncFromAbsoluteFilePath(char *absolutePath);
//returns the related folder ID from a given absolute file path
int getFolderToSyncIDFromAbsolutePath(sqlite3* conn, int *folderToSyncID, char *absolutePath);
//Siccome nel DB le cartelle le salviamo con un / finale...
//in questo modo le differenziamo dai files normali!!
int fileIsFolderOnDb(char *file);
//add a command to DB
int DBConnection_controlledAddCommand(sqlite3 *dbConn, command *cmd);
//add to DB all the syncOnFile for a given synchronization
//used when a synchronization is established (not the first, as the first syncrhonization need a scan of the folder)
int DBConnection_addSyncsOnFileForSynchronization(sqlite3 *dbConn,
		synchronization sync);
//the SIGINT signal need to be handled only by the main thread
int disableSigint();
//compare funtion used to sort files
//funzione necessaria per mettere prima i files deleted, poi in ordine alfabetico... in modo che le cartelle siano prima dei files dentro le cartelle stesse
int compare(const void *a, const void *b);
//check if the directory is empty
int isDirectoryEmpty(char *dirname);
//usata in caso di move di una (sotto)cartella per recuperare dal DB tutti i files dentro la sottocartella spostata
//e.g if the (sub)folder absolute path ~/DTNbox/foldersToSync/owner/folderName/subA/subB/subC/ is moved... relativeSubfolderPath will contains subA/subB/subC/
int getRelativeSubfolderPathFromAbsolutePath(char *relativeSubfolderPath, char *absolutePath);
//check if folderPath is like "~/DTNbox/foldersToSync/owner/folderName/"
int isSynchronizationFolderPath(char *folderPath);

void setLocalNode(char *EID);
dtnNode getLocalNode();

int getAckCommandSize(char *receivedCommand);

#endif /* CONTROLLER_UTILS_H_ */
