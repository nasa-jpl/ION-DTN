  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * checkUpdateAckCommand.h
 */

#ifndef DTNBOX11_MODEL_ACHECKUPDATEACKCOMMAND_C_
#define DTNBOX11_MODEL_ACHECKUPDATEACKCOMMAND_C_

#include <sqlite3.h>

#include "command.h"
#include "fileToSyncList.h"
#include "definitions.h"
typedef struct{
	command base;			//contiene i campi comuni a tutti i comandi. DEVE essere il primo campo
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	unsigned long long timestamp;
	fileToSyncList files;	//ogni file qui dovrebbe avere stato FILE_DELETED o FILE_NOT_DELETED
	char response[RESPONSE_COMMAND_SIZE_LIMIT];
	char errorMessage[ERROR_MESSAGE_COMMAND_SIZE_LIMIT];
	char mode[MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT];
	char what[MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT];
} checkUpdateAckCommand;


void checkUpdateAckCommand_createCommand(command** cmd);
void checkUpdateAckCommand_destroyCommand(command* cmd);
void checkUpdateAckCommand_sendCommand(command* cmd);
void checkUpdateAckCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText);


#endif /* DTNBOX11_MODEL_ACHECKUPDATEACKCOMMAND_C_ */
