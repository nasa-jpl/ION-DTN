  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * finAckCommand.h
 */

#ifndef MODEL_FINACKCOMMAND_H_
#define MODEL_FINACKCOMMAND_H_

#include <sqlite3.h>
#include "command.h"
#include "definitions.h"
//definizione di un comando di sync
typedef struct{
	command base;			//contiene i campi comuni a tutti i comandi. DEVE essere il primo campo
	unsigned long long timestamp;
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	int fromOwner;
	char response[RESPONSE_COMMAND_SIZE_LIMIT];
	char errorMessage[ERROR_MESSAGE_COMMAND_SIZE_LIMIT];
} finAckCommand;

//funzioni del comando
void finAckCommand_createCommand(command** cmd);
void finAckCommand_destroyCommand(command* cmd);
void finAckCommand_sendCommand(command* cmd);
void finAckCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);

#endif /* MODEL_FINACKCOMMAND_H_ */
