  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/


/*
 * updateCommand.h
 */

#ifndef UPDATECOMMAND_H_
#define UPDATECOMMAND_H_




#include "command.h"
#include "fileToSyncList.h"
#include "folderToSync.h"


//definizione di un comando di update
typedef struct{
	command base;			//contiene i campi comuni a tutti i comandi. DEVE essere il primo campo
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	unsigned long long timestamp;
	fileToSyncList files;	//ogni file qui dovrebbe avere stato FILE_DELETED o FILE_NOT_DELETED
} updateCommand;

//funzioni del comando
void updateCommand_createCommand(command** cmd);
void updateCommand_destroyCommand(command* cmd);
void updateCommand_sendCommand(command* cmd);
void updateCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);

//void buildUpdateString(folderToSync folder, fileToSync* files,
//		int numFiles, char* updateString);

#endif /* UPDATECOMMAND_H_ */
