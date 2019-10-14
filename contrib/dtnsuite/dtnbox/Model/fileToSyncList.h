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
 * fileToSyncListNode.h
 */

#ifndef FILETOSYNCLIST_H_
#define FILETOSYNCLIST_H_

#include "fileToSync.h"


//lista di file da sincronizzare. Utile per il tipo folderToSync
//e per mantenere una lista dei file da eliminare
typedef struct fileToSyncListNode{
	fileToSync value;
	struct fileToSyncListNode* next;
} fileToSyncListNode;
typedef fileToSyncListNode* fileToSyncList;

//crea lista vuota
fileToSyncList fileToSyncList_create();
//aggiunge elemento alla lista
fileToSyncList fileToSyncList_add(fileToSyncList list, fileToSync file);
//torna 0 se l'elemento non c'è, 1 altrimenti
int fileToSyncList_isInList(fileToSyncList list, char* file);
//dealloca la memoria di una lista
void fileToSyncList_destroy(fileToSyncList list);
//ribalta una lista
fileToSyncList fileToSyncList_reverse(fileToSyncList list);

int fileToSyncList_size(fileToSyncList list);

void fileToSyncList_print(fileToSyncList list);

#endif /* FILETOSYNCLIST_H_ */
