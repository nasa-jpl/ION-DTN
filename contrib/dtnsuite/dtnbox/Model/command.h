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
 * command.h
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <sqlite3.h>
#include "message.h"

//definisco un enum per gli stati di un comando
typedef enum CmdState {CMD_DESYNCHRONIZED, CMD_PENDING, CMD_CONFIRMED, CMD_FAILED, CMD_PROCESSING} CmdState;

typedef struct command command;

//definisco le funzioni polimorfiche per i comandi in una struttura
//la signature delle funzioni è ancora da definire
typedef struct commandFuncTable{
	void (*createCommand)(command** cmd);	//usata con newCommand() per creare command da stringa di testo
	void (*destroyCommand)(command* cmd);	//dealloca le parti specifiche del command
	void (*receiveCommand)(command* cmd, sqlite3* dbConn, char* ackCommandText);
	void (*sendCommand)(command* cmd);
} commandFuncTable;

//comando presente in un messaggio
struct command {
	char* text;
	CmdState state;
	unsigned long long creationTimestamp;
	message msg;
	commandFuncTable funcTable;
};

typedef struct {
	char* cmdName;
	commandFuncTable funcs;
} cmdTableEntry;


//questa funzione crea il comando base. Delega alla createCommand() opportuna
//le operazioni specifiche di un tipo di comando
void newCommand(command** cmd, char* text);
//questa funzione serve a deallocare la memoria comune a tutti i comandi
//da chiamare nella destroyCommand() una volta deallocate le
//parti specifiche di un tipo di comando
void destroyCommand(command* cmd);

#endif /* COMMAND_H_ */
