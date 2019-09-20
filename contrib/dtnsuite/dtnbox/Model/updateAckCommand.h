  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * updateAckCommand.h
 */

#ifndef MODEL_UPDATEACKCOMMAND_H_
#define MODEL_UPDATEACKCOMMAND_H_




#include "command.h"
#include "fileToSyncList.h"


//definizione di un comando di update
typedef struct{
	command base;			//contiene i campi comuni a tutti i comandi. DEVE essere il primo campo
	char owner[OWNER_LENGTH];
	char folderName[SYNC_FOLDER_LENGTH];
	unsigned long long timestamp;
	fileToSyncList files;	//ogni file qui dovrebbe avere stato FILE_DELETED o FILE_NOT_DELETED
	char response[RESPONSE_COMMAND_SIZE_LIMIT];
	char errorMessage[ERROR_MESSAGE_COMMAND_SIZE_LIMIT];	//TODO: this limited size is not compatible with "INCOMPLETE11011001..." solution...
} updateAckCommand;

//funzioni del comando
void updateAckCommand_createCommand(command** cmd);
void updateAckCommand_destroyCommand(command* cmd);
void updateAckCommand_sendCommand(command* cmd);
void updateAckCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText);



#endif /* MODEL_UPDATEACKCOMMAND_H_ */
