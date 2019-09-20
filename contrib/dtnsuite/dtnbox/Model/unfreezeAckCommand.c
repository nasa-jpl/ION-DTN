  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

#include "unfreezeAckCommand.h"

#include <stdio.h>
#include "../Controller/debugger.h"
#include "../Controller/utils.h"
#include "../DBInterface/DBInterface.h"
#include <stdlib.h>
#include <string.h>

void unfreezeAckCommand_createCommand(command** cmd){

	unfreezeAckCommand *this;
	char* cmdText;
	char* toFree;
	char* token;


	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(unfreezeAckCommand));
	this = (unfreezeAckCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//UnfreezeAck timestamp response
	cmdText = strdup(this->base.text);
	toFree = cmdText;

	//brucio il primo token in quanto e' la descrizione del comando
	token = strsep(&cmdText, "\t");

	//timestamp
	token = strsep(&cmdText, "\t");
	this->timestamp = strtoull(token, NULL, 0);

	//risposta
	token = strsep(&cmdText, "\t");
	strcpy(this->response, token);

	if (strcmp(this->response, "OK\n") != 0) {
		//ho anche il messaggio di errore
		token = strsep(&cmdText, "\t");
		//tolgo il newline
		stripNewline(token);
		strcpy(this->errorMessage, token);

	} else {
		//era un ok tolgo il newline
		stripNewline(this->response);
	}

	//libero la memoria della stringa
	free(toFree);

	//assegno lo stato desync in quanto e' stato appena creato
	this->base.state = CMD_DESYNCHRONIZED;
	this->base.creationTimestamp = this->timestamp;

	return;
}
void unfreezeAckCommand_destroyCommand(command* cmd){
	//deallocare solo i campi specifici allocati con malloc() nella createCommand()
	//in questo caso vuota
	return;
}
void unfreezeAckCommand_sendCommand(command* cmd){
	cmd->state = CMD_PENDING;
	return;
}
void unfreezeAckCommand_receiveCommand(command* cmd, sqlite3* dbConn, char* ackCommandText){

	int error;
	char unfreezeText[UNFREEZE_COMMAND_TEXT_SIZE_LIMIT];
	char tempString[TEMP_STRING_LENGTH];

	unfreezeAckCommand *this;
	command tmpCommandSource;

	int sourceID;


	this = (unfreezeAckCommand*) cmd;

	//azzero il testo dell'ack
//	strcpy(ackCommandText, "");	XXX note: now ackCommandText memory is not allocated!!

	//ho ricevuto l'ack, aggiorno lo stato sul database del comando unfreeze
	//Unreeze timestamp
	//genero il comando di unfreeze
	strcpy(unfreezeText, "Unfreeze");
	sprintf(tempString, "%llu", this->timestamp);
	strcat(unfreezeText, "\t");
	strcat(unfreezeText, tempString);
	strcat(unfreezeText, "\n");

	tmpCommandSource.state = CMD_CONFIRMED;
	tmpCommandSource.msg.destination = this->base.msg.source;
	tmpCommandSource.text = unfreezeText;

	//controllo se ricevo un unfreezeAck di un unfreeze che non avevo mandato o che non ho nel DB...


	error = DBConnection_isCommandOnDb(dbConn, tmpCommandSource, &sourceID);
	if (error) {
		error_print( "unfreezeAckCommand_receiveCommand: error in DBConnection_isCommandOnDb()\n");
		return;	//INTERNALERROR
	}
	if (sourceID == 0) {
		//TODO manage problem
		error_print( "unfreezeAckCommand_receiveCommand: command not found\n");
		return;
	}

	//aggiorno lo stato sul database del comando che avevo inviato
	error = DBConnection_updateCommandState(dbConn, this->base.msg.source,
			unfreezeText, CMD_CONFIRMED);
	if (error) {
		error_print(
				"unfreezeAckCommand_receiveCommand: error in DBConnection_updateCommandState()\n");
		return;	//INTERNALERROR
	}

	if(strcmp(this->response, "OK") == 0){
		debug_print(DEBUG_OFF, "unfreezeAckCommand_receiveCommand: source confirmed unfreeze command!\n");
	}else{
		//la richiesta non e' andata a buon fine, gestisco i possibili errori
		debug_print(DEBUG_OFF, "unfreezeAckCommand_receiveCommand: request refused, reason %s\n",
				this->errorMessage);
	}


	//sistemo i comandi precedenti in attesa
	error = DBConnection_setPreviusPendingAsFailed(dbConn,
			this->base.msg.source, this->timestamp);
	if (error) {
		error_print(
				"unfreezeAckCommand_receiveCommand: error in DBConnection_setPreviusPendingAsFailed()\n");
		return;	//ci possiamo fare poco... INTERNALERROR
	}

	return;
}

