  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * syncAckCommand.h
 */

#ifndef MODEL_SYNCACKCOMMAND_H_
#define MODEL_SYNCACKCOMMAND_H_



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
	char response[RESPONSE_COMMAND_SIZE_LIMIT];
	char errorMessage[ERROR_MESSAGE_COMMAND_SIZE_LIMIT];
} syncAckCommand;

//funzioni del comando
void syncAckCommand_createCommand(command** cmd);
void syncAckCommand_destroyCommand(command* cmd);
void syncAckCommand_sendCommand(command* cmd);
void syncAckCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);


#endif /* MODEL_SYNCACKCOMMAND_H_ */
