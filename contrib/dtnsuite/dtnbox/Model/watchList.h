  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/


/*
 * watchList.h
 * Module for the detection of events on monitored folders
 * watchFolder field in watchStruct type is an absolute path whose last character must be '/' (very important!!)
 * watchFolder folder must be a FOLDER path, not regular file.
 *
 */


#ifndef DTNBOX11_MODEL_WATCH_LIST_H_
#define DTNBOX11_MODEL_WATCH_LIST_H_




typedef struct watchStruct{
	int wd;	//watch descriptor returned by inotify_add_watch...
	char *watchFolder;	//...and his related absolute folder path
} watchStruct;


typedef struct watchListNode{
	watchStruct* value;
	struct watchListNode* next;
} watchListNode;

typedef watchListNode* watchList;



//create new list
int watchList_create();
//destroy list by deallocating memory
void watchList_destroy();
//if watch list contains the folder return the wd associated
int watchList_containsFolder(char *folder);
//if watch list contains the wd return the pointer of the path associated, it must not be changed.
char* watchList_containsDescriptor(int wd);
//return inotify watch descriptor
int getIfd();

//add only folderToWatch to the watchList
int watchList_add(char *folderToWatch);
//scan folderToWatch directory and add recursively to the watchList all the folders contained.
int recursive_add(char *absolutePath);
//remove all paths starting with absolutePath from watchList
int recursive_remove(char *absolutePath);

//remove folderToRemove path from watchList
int watchList_remove(char *folderToRemove);

void watchList_print();

#endif /* DTNBOX11_MODEL_WATCH_LIST_H_ */
