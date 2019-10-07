  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * commandParser.h
 */

#ifndef COMMANDPARSER_H_
#define COMMANDPARSER_H_

#include "../Model/cmdList.h"

//legge da un file .dbcmd tutti i comandi e li restituisce in una lista
cmdList cmdParser_getCommands(char* commandFile);

#endif /* COMMANDPARSER_H_ */
