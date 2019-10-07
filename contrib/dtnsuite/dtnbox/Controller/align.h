  /********************************************************
  ** Authors: Nicolò Castellazzi, nicolo.castellazzi@studio.unibo.it
  **          Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * align.h
 */
#include <sqlite3.h>
#include "../Model/folderToSync.h"
#include <sys/stat.h>

#ifndef DTNBOX11_CONTROLLER_ALIGN_H_
#define DTNBOX11_CONTROLLER_ALIGN_H_


#define DIM_TEMP_COPY_BUFFER 4096

//function called when new file is found (both scanning and events)
//function called when update on file is found (both scanning and events)
int newOrUpdateFileOnFS(sqlite3 *dbConn, char *absoluteFilePath);



//function called when delete file is found (both scanning and events)
int deleteFileOnFS(sqlite3* dbConn, char *absoluteFilePath);

//function called when delete folder is found (only for events)
int deleteFolderOnFs(sqlite3 *dbConn, char *absoluteFilePath);

//function called when new folder is found (only for events)
int newFolderOnFS(sqlite3* dbConn, char *absoluteFilePath);

//function called when a monitored folder is moved is found (only for events)
int moveFolderOnFs(sqlite3 *dbConn, char *absoluteFilePath);


//function called to align DB and FS, it scans all folderPath directory
int alignFSandDB(sqlite3* dbConn, folderToSync folder, char* folderPath);

int fileLockingCopy(folderToSync folder, char *subPathFile,
		struct stat fileInfo, char *absoluteFilePath);


#endif /* DTNBOX11_CONTROLLER_ALIGN_H_ */
