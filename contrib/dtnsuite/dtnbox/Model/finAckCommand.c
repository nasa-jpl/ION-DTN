  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * finAckCommand.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "finAckCommand.h"
#include "../DBInterface/DBInterface.h"
#include "../Controller/utils.h"
#include "watchList.h"
#include "../Controller/debugger.h"


void finAckCommand_createCommand(command** cmd) {
	finAckCommand* this;
	char* cmdText;
	char* toFree;
	char* token;

	//primissima cosa da fare SEMPRE, realloc() del comando
	(*cmd) = realloc((*cmd), sizeof(finAckCommand));
	this = (finAckCommand*) (*cmd);

	//Recupero dal testo tutti gli attributi
	//FinAck timestamp owner folderName fromOwner response
	cmdText = strdup(this->base.text);
	toFree = cmdText;

	//brucio il primo token in quanto e' la descrizione del comando
	token = strsep(&cmdText, "\t");

	//timestamp
	token = strsep(&cmdText, "\t");
	this->timestamp = strtoull(token, NULL, 0);

	//owner
	token = strsep(&cmdText, "\t");
	strcpy(this->owner, token);

	//cartella
	token = strsep(&cmdText, "\t");
	strcpy(this->folderName, token);

	//se proviene dall'owner
	token = strsep(&cmdText, "\t");
	this->fromOwner = atoi(token);

	//il nono token e' la risposta
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

void finAckCommand_destroyCommand(command* cmd) {
	//dovrei fare la free delle strutture allocate nella new command ma non serve per questo comando
}

void finAckCommand_sendCommand(command* cmd) {
	//aggiorno lo stato del comando a CONFIRMED in quanto non c'è l'ack dell'ack
	cmd->state = CMD_CONFIRMED;
	return;
}

void finAckCommand_receiveCommand(command* cmd, sqlite3* dbConn,
		char* ackCommandText) {
	int error;

	char finText[FIN_COMMAND_TEXT_SIZE_LIMIT];

	char tempString[TEMP_STRING_LENGTH];
	finAckCommand* this;

	command tmpCommandSource;	//il comando fin che io ho inviato
	int sourceID;

	//Converto il comando
	this = (finAckCommand*) cmd;

	//azzero il testo dell'ack
	//strcpy(ackCommandText, "");	XXX note: now ackCommandText memory is not allocated!!

	//ho ricevuto l'ack, aggiorno lo stato sul database del comando fin
	//Fin timestamp owner folderName fromOwner
	//genero il comando di fin
	strcpy(finText, "Fin");
	//aggiungo il timestamp
	sprintf(tempString, "%llu", this->timestamp);
	strcat(finText, "\t");
	strcat(finText, tempString);
	//owner e folderName
	strcat(finText, "\t");
	strcat(finText, this->owner);
	strcat(finText, "\t");
	strcat(finText, this->folderName);
	//inserisco il mode
	sprintf(tempString, "%d", this->fromOwner);
	strcat(finText, "\t");
	strcat(finText, tempString);
	strcat(finText, "\n");

	tmpCommandSource.state = CMD_CONFIRMED;
	tmpCommandSource.msg.destination = this->base.msg.source;
	tmpCommandSource.text = finText;

	//controllo se ricevo un finAck di una fin che non avevo mandato o che non ho nel DB...

	error = DBConnection_isCommandOnDb(dbConn, tmpCommandSource, &sourceID);
	if (error) {
		error_print( "finAckCommand_receiveCommand: error in DBConnection_isCommandOnDb()\n");
		return;	//INTERNALERROR
	}
	if (sourceID == 0) {
		//TODO manage problem
		error_print( "finAckCommand_receiveCommand: command not found\n");
		return;
	}

	//aggiorno lo stato sul database del sync che mi ha generato
	error = DBConnection_updateCommandState(dbConn, this->base.msg.source,
			finText, CMD_CONFIRMED);
	if (error) {
		error_print(
				"finAckCommand_receiveCommand: error in DBConnection_updateCommandState()\n");
		return;	//INTERNALERROR
	}

	//getOwnerFromEID(myOwnerFromEID, this->base.msg.destination.EID);

	//guardo il messaggio
	if (strcmp(this->response, "OK") == 0) {
		debug_print(DEBUG_OFF, "finAckCommand_receiveCommand: synchronization terminated correctly!\n");
	} else {

		//la richiesta non e' andata a buon fine, gestisco i possibili errori

		debug_print(DEBUG_OFF, "finAckCommand_receiveCommand: request refused, reason %s\n",
				this->errorMessage);

		//se sono stato bloccato mi aggiungo in blockedByList
		if (strcmp(this->errorMessage, "BLACKLISTED") == 0) {

			error = DBConnection_updateDTNnodeBlockedBy(dbConn, this->base.msg.source, BLOCKEDBY);
			if(error){
				error_print("finAckCommand_receiveCommand: error in DBConnection_updateDTNnodeBlockedBy()\n");
				return; //INTERNALERROR
			}
		}
	}

	//sistemo i comandi precedenti in attesa
	error = DBConnection_setPreviusPendingAsFailed(dbConn,
			this->base.msg.source, this->timestamp);
	if (error) {
		error_print(
				"finAckCommand_receiveCommand: error in DBConnection_setPreviusPendingAsFailed()\n");
		return;	//ci possiamo fare poco... INTERNALERROR
	}

	return;
}


