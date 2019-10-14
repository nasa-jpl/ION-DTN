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
 * fileToSyncList.c
 */

#include <string.h>
#include <stdlib.h>
#include "fileToSync.h"
#include "fileToSyncList.h"
#include "../Controller/debugger.h"
#include <stdio.h>

//crea lista vuota
fileToSyncList fileToSyncList_create() {
	return NULL;
}

//aggiunge elemento alla lista
fileToSyncList fileToSyncList_add(fileToSyncList list, fileToSync file) {
	fileToSyncList newList = malloc(sizeof(fileToSyncListNode));
	newList->value = file;
	newList->next = list;
	return newList;
}

//torna 0 se l'elemento non c'è, 1 altrimenti
int fileToSyncList_isInList(fileToSyncList list, char* file) {

	fileToSyncList toDestroy = list;

	while (toDestroy != NULL) {
		if (strcmp(toDestroy->value.name, file) == 0) {
			return 1;
		}
		toDestroy = toDestroy->next;
	}
	return 0;
}

//destroy list
void fileToSyncList_destroy(fileToSyncList list){

	fileToSyncList current;
	fileToSyncList temp;

	current = list;

	while(current != NULL){
		temp = current->next;
		free(current);
		current = temp;
	}
}

//ribalta una lista
fileToSyncList fileToSyncList_reverse(fileToSyncList list) {
	fileToSyncList new_root = 0;
	while (list) {
		fileToSyncList next = list->next;
		list->next = new_root;
		new_root = list;
		list = next;
	}
	return new_root;
}

int fileToSyncList_size(fileToSyncList list){

	fileToSyncList temp = list;
	int res=0;

	while(temp != NULL){
		res++;
		temp = temp->next;
	}
	return res;
}

void fileToSyncList_print(fileToSyncList list){

	fileToSyncList toScan;

	toScan = list;

	while(toScan != NULL){
		debug_print(DEBUG_OFF, "%s\n", toScan->value.name);
		toScan = toScan->next;
	}
	return;
}

