  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

#ifndef DTNBOX11_MODEL_UNFREEZEACKCOMMAND_H_
#define DTNBOX11_MODEL_UNFREEZEACKCOMMAND_H_

#include <sqlite3.h>
#include "command.h"
#include "definitions.h"

typedef struct{
	command base;
	unsigned long long timestamp;
	char response[RESPONSE_COMMAND_SIZE_LIMIT];
	char errorMessage[ERROR_MESSAGE_COMMAND_SIZE_LIMIT];
} unfreezeAckCommand;

//funzioni del comando
void unfreezeAckCommand_createCommand(command** cmd);
void unfreezeAckCommand_destroyCommand(command* cmd);
void unfreezeAckCommand_sendCommand(command* cmd);
void unfreezeAckCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);



#endif /* DTNBOX11_MODEL_UNFREEZEACKCOMMAND_H_ */
