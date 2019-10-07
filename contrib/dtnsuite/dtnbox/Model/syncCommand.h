  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * syncCommand.h
 */

#ifndef MODEL_SYNCCOMMAND_H_
#define MODEL_SYNCCOMMAND_H_




#include <sqlite3.h>
#include "command.h"
#include "synchronization.h"

//definizione di un comando di sync
typedef struct{
	command base;			//contiene i campi comuni a tutti i comandi. DEVE essere il primo campo
	unsigned long long timestamp;
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	SyncMode mode;
	int nochat;
	int lifetime;
	char pwdRead[SYNC_PASSWORD_SIZE_LIMIT];
	char pwdWrite[SYNC_PASSWORD_SIZE_LIMIT];
} syncCommand;

//funzioni del comando
void syncCommand_createCommand(command** cmd);
void syncCommand_destroyCommand(command* cmd);
void syncCommand_sendCommand(command* cmd);
void syncCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);

//crea il testo di un comando SYNC a partire dall'oggetto
void createSyncText(synchronization sync, int sourceLifetime, char* commandText);

#endif /* MODEL_SYNCCOMMAND_H_ */
