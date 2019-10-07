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
 * cmdList.c
 */

#include "cmdList.h"
#include "command.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//crea lista vuota
cmdList cmdList_create() {
	return NULL;
}

//aggiunge elemento in testa alla lista
cmdList cmdList_add(cmdList list, command* cmd) {
	cmdList newList = malloc(sizeof(cmdListNode));
	newList->value = cmd;
	newList->next = list;
	return newList;
}

void cmdList_destroy(cmdList list) {
	cmdList current;
	cmdList temp;

	current = list;

	while (current != NULL) {
		destroyCommand(current->value);
		temp = current->next;
		free(current);
		current = temp;
	}
}

cmdList cmdList_reverse(cmdList root) {
	cmdList new_root = 0;
	while (root) {
		cmdList next = root->next;
		root->next = new_root;
		new_root = root;
		root = next;
	}
	return new_root;
}


