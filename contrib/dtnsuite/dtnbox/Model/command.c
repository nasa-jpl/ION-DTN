/********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * command.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "command.h"
#include "syncCommand.h"
#include "syncAckCommand.h"
#include "updateCommand.h"
#include "updateAckCommand.h"
#include "finCommand.h"
#include "finAckCommand.h"

#include "checkUpdateCommand.h"
#include "checkUpdateAckCommand.h"
#include "freezeCommand.h"
#include "unfreezeAckCommand.h"
#include "unfreezeCommand.h"

#include "../Controller/debugger.h"


//tabella per associare a ogni comando le funzioni per gestirlo
//ordine: nomecomando createCommand() destroyCommand() receiveCommand() sendCommand()
//notare che deve essere presente sempre l'ultima riga con nome comando "" e puntatori a NULL, che
//serve a indicare la fine della tabella
const cmdTableEntry CMD_TABLE[] = {
	{"Sync", {syncCommand_createCommand, syncCommand_destroyCommand, syncCommand_receiveCommand, syncCommand_sendCommand}},
	{"SyncAck", {syncAckCommand_createCommand, syncAckCommand_destroyCommand, syncAckCommand_receiveCommand, syncAckCommand_sendCommand}},
	{"Update", {updateCommand_createCommand, updateCommand_destroyCommand, updateCommand_receiveCommand, updateCommand_sendCommand}},
	{"UpdateAck", {updateAckCommand_createCommand, updateAckCommand_destroyCommand, updateAckCommand_receiveCommand, updateAckCommand_sendCommand}},
	{"Fin", {finCommand_createCommand, finCommand_destroyCommand, finCommand_receiveCommand, finCommand_sendCommand}},
	{"FinAck", {finAckCommand_createCommand, finAckCommand_destroyCommand, finAckCommand_receiveCommand, finAckCommand_sendCommand}},
	{"CheckUpdate", {checkUpdateCommand_createCommand, checkUpdateCommand_destroyCommand, checkUpdateCommand_receiveCommand, checkUpdateCommand_sendCommand}},
	{"CheckUpdateAck", {checkUpdateAckCommand_createCommand, checkUpdateAckCommand_destroyCommand, checkUpdateAckCommand_receiveCommand, checkUpdateAckCommand_sendCommand}},
	{"Freeze", {freezeCommand_createCommand, freezeCommand_destroyCommand, freezeCommand_receiveCommand, freezeCommand_sendCommand}},
	{"Unfreeze", {unfreezeCommand_createCommand, unfreezeCommand_destroyCommand, unfreezeCommand_receiveCommand, unfreezeCommand_sendCommand}},
	{"UnfreezeAck", {unfreezeAckCommand_createCommand, unfreezeAckCommand_destroyCommand, unfreezeAckCommand_receiveCommand, unfreezeAckCommand_sendCommand}},
	{"", {0, 0, 0, 0}}
};


void newCommand(command** cmd, char* text){
	char* cmdString;
	int i;
	int cmdStringLen = 0;
	cmdString = text;

	//conto lunghezza primo elemento fino allo spazio
	while(cmdString[cmdStringLen] != '\0' && cmdString[cmdStringLen] != '\n' && cmdString[cmdStringLen] != ' ' && cmdString[cmdStringLen] != '\t'){
		cmdStringLen++;
	}

	//copio il primo elemento
	cmdString = malloc(cmdStringLen+1);		//+1 per il terminatore
	for(i = 0; i < cmdStringLen; i++){
		cmdString[i] = text[i];
	}
	cmdString[cmdStringLen] = '\0';

	i=0;
	(*cmd) = NULL;
	//cerco il comando nella tabella
	while(strcmp(CMD_TABLE[i].cmdName,"") != 0){
		if(strcmp(CMD_TABLE[i].cmdName, cmdString) == 0){

			//se sono qui ho trovato il comando, popolo la funcTable e inizializzo

			//per ora alloco solo l'oggetto base. Se il comando specifico occupa
			//più memoria, si farà una realloc()
			(*cmd) = malloc(sizeof(command));
			//copio il testo
			(*cmd)->text = malloc(strlen(text)+1);
			strcpy((*cmd)->text, text);

			//non assegno uno stato per ora, se ne occuperà chi ha chiamato questa funzione
			//lo stesso vale per il messaggio associato
			//il timestamp verrà dato dalla createCommand() in base al text
			//inizializzo la tabella delle funzioni
			(*cmd)->funcTable.createCommand = CMD_TABLE[i].funcs.createCommand;
			(*cmd)->funcTable.destroyCommand = CMD_TABLE[i].funcs.destroyCommand;
			(*cmd)->funcTable.receiveCommand = CMD_TABLE[i].funcs.receiveCommand;
			(*cmd)->funcTable.sendCommand = CMD_TABLE[i].funcs.sendCommand;
			//chiamo la createCommand() per inizializzare le parti specifiche del tipo di comando
			(*cmd)->funcTable.createCommand(cmd);
			break;
		}
		i++;
	}
	//se cmd==NULL, non è stato trovato il comando e non è stato inizializzato nulla
	if((*cmd) == NULL){
		error_print( "newCommand: command '%s' not found\n", cmdString);
	}
	//dealloco risorse
	free(cmdString);
}


void destroyCommand(command* cmd){
	//dealloco ciò che è stato fatto dalla createCommand()
	cmd->funcTable.destroyCommand(cmd);
	//dealloco ciò che è stato allocato dalla newCommand()
	free(cmd->text);
	free(cmd);
}


