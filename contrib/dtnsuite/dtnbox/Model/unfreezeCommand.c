  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

#include "unfreezeCommand.h"

#include <stdio.h>

#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../DBInterface/DBInterface.h"
#include <stdlib.h>
#include <string.h>

#include "definitions.h"

static void unfreezeCommand_buildAckCommand(unfreezeCommand* this, char *ackCommandText,
		int outcome);

void unfreezeCommand_createCommand(command** cmd){

	unfreezeCommand* this;
	char *cmdText;
	char *token;

	char *toFree;


	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(unfreezeCommand));
	this = (unfreezeCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//Unreeze timestamp
	cmdText = strdup(this->base.text);
	toFree = cmdText;

	//brucio il primo token in quanto e' la descrizione del comando
	token = strsep(&cmdText, "\t");

	//il secondo token e' il timestamp
	//visto che e' l'ultimo mi tolgo il \n
	token = strsep(&cmdText, "\t");
//	stripNewline(token);
	this->timestamp = strtoull(token, NULL, 0);

	//libero la memoria della stringa
	free(toFree);

	//assegno lo stato desync in quanto e' stato appena creato
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;

	return;
}
void unfreezeCommand_destroyCommand(command* cmd){
	//deallocare solo i campi specifici allocati con malloc() nella createCommand()
	//in questo caso vuota
	return;
}
void unfreezeCommand_sendCommand(command* cmd){
	cmd->state = CMD_PENDING;
	return;
}
void unfreezeCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText){

	int error;
	//convert the command
	unfreezeCommand* this = (unfreezeCommand*) cmd;

	dtnNode sourceNode;

	debug_print(DEBUG_OFF, "freezeCommand_receiveCommand: %s node woke up\n", this->base.msg.source.EID);


	error = DBConnection_getDtnNodeFromEID(dbConn, &sourceNode, this->base.msg.source.EID);
	if(error){
		//should never happen
		error_print("freezeCommand_receiveCommand: error in DBConnection_getDtnNodeFromEID()\n");
		unfreezeCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
		return;
	}

	error = DBConnection_updateDTNnodeFrozen(dbConn, sourceNode, NOT_FROZEN);
	if(error){
		error_print("freezeCommand_receiveCommand: error in DBConnection_updateDTNnodeFrozen()\n");
		unfreezeCommand_buildAckCommand(this, ackCommandText, INTERNALERROR);
		return;
	}

	unfreezeCommand_buildAckCommand(this, ackCommandText, OK);
	return;
}


static void unfreezeCommand_buildAckCommand(unfreezeCommand* this, char *ackCommandText,
		int outcome) {

	char tempString[TEMP_STRING_LENGTH];

	//azzero il testo dell'ack
	strcpy(ackCommandText, "");

	//Mi preparo la stringa per l'ack
	//FinAck timestamp owner folderName fromOwner response
	strcpy(ackCommandText, "UnfreezeAck");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(ackCommandText, "\t");
	strcat(ackCommandText, tempString);

	switch (outcome) {

	case (OK): {
		strcat(ackCommandText, "\tOK\n");
		break;
	}
	case (INTERNALERROR): {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR\n");
		break;
	}
	default: {
		strcat(ackCommandText, "\tERROR\tINTERNALERROR\n");
		break;
	}

	} //switch(outcome)

	return;
}
