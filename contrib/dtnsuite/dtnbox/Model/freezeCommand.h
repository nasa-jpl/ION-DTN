  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

#ifndef DTNBOX11_MODEL_FREEZECOMMAND_H_
#define DTNBOX11_MODEL_FREEZECOMMAND_H_

#include <sqlite3.h>
#include "command.h"

typedef struct{
	command base;
	unsigned long long timestamp;
} freezeCommand;

//funzioni del comando
void freezeCommand_createCommand(command** cmd);
void freezeCommand_destroyCommand(command* cmd);
void freezeCommand_sendCommand(command* cmd);
void freezeCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);



#endif /* DTNBOX11_MODEL_FREEZECOMMAND_H_ */
