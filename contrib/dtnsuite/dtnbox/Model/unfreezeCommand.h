  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

#ifndef DTNBOX11_MODEL_DEFREEZECOMMAND_H_
#define DTNBOX11_MODEL_DEFREEZECOMMAND_H_

#include <sqlite3.h>
#include "command.h"

typedef struct{
	command base;
	unsigned long long timestamp;
} unfreezeCommand;

//funzioni del comando
void unfreezeCommand_createCommand(command** cmd);
void unfreezeCommand_destroyCommand(command* cmd);
void unfreezeCommand_sendCommand(command* cmd);
void unfreezeCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);



#endif /* DTNBOX11_MODEL_UNFREEZECOMMAND_H_ */
