  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * commandParser.c
 */

#include <stdlib.h>
#include <stdio.h>
#include "commandParser.h"
#include "debugger.h"


cmdList cmdParser_getCommands(char* commandFile){
	FILE* fileComandi;
	char* linea = NULL;
	size_t len = 0;
	ssize_t read;
	command* tempCmd;
	cmdList toReturn = NULL;

	toReturn = cmdList_create();

	//apro il file in lettura
	fileComandi = fopen(commandFile, "r");
	if(fileComandi == NULL){
		error_print("cmdParser_getCommands: error while opening file\n");
		return NULL;
	}

	while((read = getline(&linea, &len, fileComandi)) != -1) {
		//Ogni comando e' separato dal \n
		newCommand(&tempCmd, linea);
		//aggiungo il comando alla lista
		toReturn = cmdList_add(toReturn, tempCmd);
	}

	fclose(fileComandi);
	if(linea){
		free(linea);
	}
	return toReturn;
}
