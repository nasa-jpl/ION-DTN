/*
 * freezeCommand.c
 *
 *  Created on: Nov 26, 2018
 *      Author: ubunto
 */

#include "freezeCommand.h"

#include <stdio.h>
#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../DBInterface/DBInterface.h"
#include <stdlib.h>
#include <string.h>

//funzioni del comando


void freezeCommand_createCommand(command** cmd){
	freezeCommand* this;
	char *cmdText;
	char *token;

	char *toFree;


	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(freezeCommand));
	this = (freezeCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//Freeze timestamp
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
void freezeCommand_destroyCommand(command* cmd){
	//deallocare solo i campi specifici allocati con malloc() nella createCommand()
	//in questo caso vuota
	return;
}


void freezeCommand_sendCommand(command* cmd){
	cmd->state = CMD_PENDING;
	return;
}
void freezeCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText){

	int error;
	//convert the command
	freezeCommand* this = (freezeCommand*) cmd;

	dtnNode sourceNode;

	debug_print(DEBUG_OFF, "freezeCommand_receiveCommand: %s node is going to sleep\n", this->base.msg.source.EID);

	//no ack
	//strcpy(ackCommandText, "");	XXX note: now ackCommandText memory is not allocated!!


	error = DBConnection_getDtnNodeFromEID(dbConn, &sourceNode, this->base.msg.source.EID);
	if(error){
		//should never happen
		error_print("freezeCommand_receiveCommand: error in DBConnection_getDtnNodeFromEID()\n");
		return;
	}

	error = DBConnection_updateDTNnodeFrozen(dbConn, sourceNode, FROZEN);
	if(error){
		error_print("freezeCommand_receiveCommand: error in DBConnection_updateDTNnodeFrozen()\n");
		return;
	}

	return;
}
