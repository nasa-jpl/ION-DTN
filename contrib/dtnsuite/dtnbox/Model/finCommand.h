  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * finCommand.h
 */

#ifndef MODEL_FINCOMMAND_H_
#define MODEL_FINCOMMAND_H_




#include <sqlite3.h>
#include "command.h"

//definizione di un comando di fin
typedef struct{
	command base;			//contiene i campi comuni a tutti i comandi. DEVE essere il primo campo
	unsigned long long timestamp;
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	int fromOwner;	//TODO remove fromOwner
} finCommand;

//funzioni del comando
void finCommand_createCommand(command** cmd);
void finCommand_destroyCommand(command* cmd);
void finCommand_sendCommand(command* cmd);
void finCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);

#endif /* MODEL_FINCOMMAND_H_ */
