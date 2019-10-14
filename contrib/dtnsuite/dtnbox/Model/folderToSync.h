/********************************************************
  ** Authors: Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * folderToSync.h
 */

#ifndef FOLDERTOSYNC_H_
#define FOLDERTOSYNC_H_

#include "fileToSyncList.h"

//cartella condivisa in una sincronizzazione
typedef struct folderToSync{
	char owner[OWNER_LENGTH];
	char name[SYNC_FOLDER_LENGTH];
	fileToSyncList files; //sarebbe un array di fileToSync*. meglio fare una lista.
} folderToSync;



#endif /* FOLDERTOSYNC_H_ */
