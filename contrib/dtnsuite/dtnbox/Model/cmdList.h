/********************************************************
  ** Authors: Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * cmdList.h
 */

#ifndef CMDLIST_H_
#define CMDLIST_H_

#include "command.h"

//lista di comandi
typedef struct cmdListNode{
	command* value;
	struct cmdListNode* next;
} cmdListNode;
typedef cmdListNode* cmdList;

//crea lista vuota
cmdList cmdList_create();
//aggiunge elemento alla lista
cmdList cmdList_add(cmdList list, command* cmd);
//svuota la lista e dealloca tutti gli elementi
void cmdList_destroy(cmdList list);

cmdList cmdList_reverse(cmdList list);

#endif /* CMDLIST_H_ */
