/********************************************************
  ** Authors: Marcello Ballanti, marcello.ballanti@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * fileToSync.h
 */

#ifndef FILETOSYNC_H_
#define FILETOSYNC_H_

#include "definitions.h"

//definisco un enum per gli stati di un file
typedef enum FileState {FILE_DESYNCHRONIZED, FILE_PENDING, FILE_SYNCHRONIZED, FILE_FAILED} FileState;

//enumerazione per indicare se il file è cancellato o meno
typedef enum FileDeletionState {FILE_NOTDELETED, FILE_DELETED} FileDeletionState;

//file presente in una o più sincronizzazioni
typedef struct fileToSync{
	char name[FILETOSYNC_LENGTH];	//note: THIS IS A RELATIVE NAME (relative to its synchronization folder), but to be strong to any name length we use the total space available
									//for example, given the synchronization folder path "/a/b/DTNbox/foldersToSync/5/a/", all the remaining characters are available for this name...
	unsigned long long lastModified;
	FileState state;
	FileDeletionState deleted;	//vale 1 se è cancellato, 0 altrimenti
} fileToSync;


#endif /* FILETOSYNC_H_ */
