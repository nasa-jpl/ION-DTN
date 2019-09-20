  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * checkUpdateCommand.h
 */

#ifndef DTNBOX11_MODEL_ACHECKUPDATECOMMAND_H_
#define DTNBOX11_MODEL_ACHECKUPDATECOMMAND_H_

#include "command.h"
#include "fileToSyncList.h"
#include "folderToSync.h"

//mode
#define ASK "ASK"
#define OFFER "OFFER"

//what
#define _FILE "FILE"
#define _LIST "LIST"
#define _ALL "ALL"

typedef struct {
	command base;
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	unsigned long long timestamp;
	fileToSyncList files;
	char mode[MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT];
	char what[MODE_WHAT_CHECK_UPDATE_SIZE_LIMIT];
} checkUpdateCommand;

void checkUpdateCommand_createCommand(command** cmd);
void checkUpdateCommand_destroyCommand(command* cmd);
void checkUpdateCommand_sendCommand(command* cmd);
void checkUpdateCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText);

//void buildCheckUpdateString(folderToSync folder, fileToSyncList files,
//		char* updateString, char *mode,
//		char *what);

#endif /* DTNBOX11_MODEL_ACHECKUPDATECOMMAND_H_ */



