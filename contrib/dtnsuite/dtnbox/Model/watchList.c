  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/

/*
 * watchList.c
 * Module for the detection of events on monitored folders
 */

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <dirent.h>

#include "watchList.h"
#include "../Controller/utils.h"

#include "definitions.h"
#include "../Controller/debugger.h"

static int ifd = -1;	//inotify file descriptor
static watchList this;//list of watchStruct -> this must always be the first element of the list

static pthread_mutex_t lock;	//recursive mutex

static void newWatchStruct(watchStruct **ws, int wd, char *folderPath) {

	int len = strlen(folderPath);

	(*ws) = (watchStruct*) malloc(sizeof(watchStruct));

	if (folderPath[len - 1] == '/') {
		(*ws)->watchFolder = (char*) malloc(len + 1);
		strcpy((*ws)->watchFolder, folderPath);
	} else {
		(*ws)->watchFolder = (char*) malloc(len + 2);
		strcpy((*ws)->watchFolder, folderPath);
		((*ws)->watchFolder)[len] = '/';
		((*ws)->watchFolder)[len + 1] = '\0';
	}
	(*ws)->wd = wd;
}

static void destroyWatchStruct(watchStruct *ws) {
	free(ws->watchFolder);
	free(ws);
	ws=NULL;
}

//create new watchList
int watchList_create() {
	this = NULL;
	ifd = inotify_init1(IN_NONBLOCK);

	pthread_mutexattr_t mtxAttr;

	if (ifd < 0 || pthread_mutexattr_init(&mtxAttr)
			|| pthread_mutexattr_settype(&mtxAttr, PTHREAD_MUTEX_RECURSIVE)
			|| pthread_mutex_init(&lock, &mtxAttr)
			|| pthread_mutexattr_destroy(&mtxAttr))
		return ERROR_VALUE;
	else
		return SUCCESS_VALUE;
}

//add to watchList a ***single*** folderToWatch
int watchList_add(char *folderToWatch) {

//	int error = 0;
	int iwd;
	watchStruct *toAdd = NULL;

	if (folderToWatch == NULL || strlen(folderToWatch) == 0
			|| folderToWatch[0] != '/' || !isFolder(folderToWatch))
		return ERROR_VALUE;

//	if(strlen(relative_filetosync_path) > FILETOSYNC_LENGTH - 1 || strlen(absolutePath) >= (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH)){
//
//		char sysCommand[SYSTEM_COMMAND_LENGTH + LINUX_ABSOLUTE_PATH_LENGTH];
//
//		sprintf(sysCommand, "rm -rf \"%s\"", absolutePath);
//
//		error_print("recursive_add: the folder path size exceeds the DTNbox \"file to sync\" limit of %d characters, DTNbox delete the folder\n", FILETOSYNC_LENGTH);
//		if(closedir(dir)){
//			error_print("recursive_add: error in closedir(%s)\n", absolutePath);
//			return ERROR_VALUE;
//		}
//
//		error = system(sysCommand);
//		if(error){
//			error_print("recursive_add: error in system(%s)\n", sysCommand);
//			return ERROR_VALUE;
//		}
//
//		return SUCCESS_VALUE;
//	}



	pthread_mutex_lock(&lock);

	iwd = inotify_add_watch(ifd, folderToWatch, MASK);

	if (iwd < 0) {
		pthread_mutex_unlock(&lock);
		return ERROR_VALUE;
	}

	newWatchStruct(&toAdd, iwd, folderToWatch);

	watchList newList = (watchList) malloc(sizeof(watchListNode));
	newList->value = toAdd;
	newList->next = this;
	this = newList;

	debug_print(DEBUG_L1, "Set: %s, watchDescriptor: %d\n", toAdd->watchFolder,
			toAdd->wd);
	pthread_mutex_unlock(&lock);
	return SUCCESS_VALUE;
}

//remove "folderToRemove" from the watchList (if present)
int watchList_remove(char *folderToRemove) {

	watchList temp = this;
	watchList padre = NULL;

	temp = this;

	pthread_mutex_lock(&lock);

	while (temp != NULL) {
		//remove based on the same name
		if (strcmp(temp->value->watchFolder, folderToRemove) == 0) {

			inotify_rm_watch(ifd, temp->value->wd);
			debug_print(DEBUG_L1, "Removed: %s, watchDescriptor: %d\n",
					temp->value->watchFolder, temp->value->wd);

			if (padre == NULL) {
				//first element
				watchList ret = temp->next;

				destroyWatchStruct(temp->value);
				free(temp);
				//return ret;
				this = ret;
				pthread_mutex_unlock(&lock);
				return SUCCESS_VALUE;
			} else {
				//other element
				padre->next = temp->next;
				destroyWatchStruct(temp->value);
				free(temp);
				//return padre;
				pthread_mutex_unlock(&lock);
				return SUCCESS_VALUE;
			}
		}
		padre = temp;
		temp = temp->next;
	}
	//"folderToRemove" not present in watchList
	pthread_mutex_unlock(&lock);
	return ERROR_VALUE;
}

void watchList_destroy() {
	watchList current = this;
	watchList temp = NULL;

	while (current != NULL) {
		destroyWatchStruct(current->value);
		temp = current->next;
		free(current);
		current = temp;
	}
	pthread_mutex_destroy(&lock);
	return;
}

//if watch list contains the folder return the wd associated
int watchList_containsFolder(char *folder) {

	int res = 0;
	watchList temp = this;
	pthread_mutex_lock(&lock);
	while (temp != NULL) {
		if (strcmp(temp->value->watchFolder, folder) == 0) {
			res = temp->value->wd;
			pthread_mutex_unlock(&lock);
			return res;
		}
		temp = temp->next;
	}
	pthread_mutex_unlock(&lock);
	return res;
}

char* watchList_containsDescriptor(int wd) {

	char *res = NULL;
	watchList temp = this;
	pthread_mutex_lock(&lock);
	while (temp != NULL) {
		if (temp->value->wd == wd) {
			res = temp->value->watchFolder;
			pthread_mutex_unlock(&lock);
			return res;
		}
		temp = temp->next;
	}
	pthread_mutex_unlock(&lock);
	return res;
}

int getIfd() {
	return ifd;
}

int getPathFromWatchDescriptor(int wd, char *dest) {

	int res = 0;
	watchList temp = this;
	pthread_mutex_lock(&lock);

	while (temp != NULL && res == 0) {
		if (temp->value->wd == wd) {
			strcpy(dest, temp->value->watchFolder);
			res = 1;
		}
		temp = temp->next;
	}

	pthread_mutex_unlock(&lock);
	return res;
}

//recursive function to add to the watchList absolutePath and all subdirectories
int recursive_add(char *absolutePath) {

	DIR *dir;
	struct dirent* current;
	int error;

//	char relative_filetosync_path[LINUX_ABSOLUTE_PATH_LENGTH];


	if (!isFolder(absolutePath))
		return ERROR_VALUE;

	if ((dir = opendir(absolutePath)) == NULL)
		return ERROR_VALUE;

	if (absolutePath[strlen(absolutePath) - 1] != '/')
		strcat(absolutePath, "/");

//	getRelativeFilePathFromAbsolutePath(relative_filetosync_path, absolutePath);

//	if(strlen(relative_filetosync_path) > FILETOSYNC_LENGTH - 1 || strlen(absolutePath) >= (DTNBOX_SYSTEM_FILES_ABSOLUTE_PATH_LENGTH + OWNER_LENGTH + SYNC_FOLDER_LENGTH + FILETOSYNC_LENGTH)){
//
//		char sysCommand[SYSTEM_COMMAND_LENGTH + LINUX_ABSOLUTE_PATH_LENGTH];
//
//		sprintf(sysCommand, "rm -rf \"%s\"", absolutePath);
//
//		error_print("recursive_add: the folder path size exceeds the DTNbox \"file to sync\" limit of %d characters, DTNbox delete the folder\n", FILETOSYNC_LENGTH);
//		if(closedir(dir)){
//			error_print("recursive_add: error in closedir(%s)\n", absolutePath);
//			return ERROR_VALUE;
//		}
//
//		error = system(sysCommand);
//		if(error){
//			error_print("recursive_add: error in system(%s)\n", sysCommand);
//			return ERROR_VALUE;
//		}
//
//		return SUCCESS_VALUE;
//	}


	pthread_mutex_lock(&lock);

	if (watchList_add(absolutePath))
		return ERROR_VALUE;

	while ((current = readdir(dir)) != NULL) {

		if ((current->d_name)[0] == '.')
			continue;

		strcat(absolutePath, current->d_name);

		if (isFolder(absolutePath)) {
			strcat(absolutePath, "/");

			error = recursive_add(absolutePath);
			if (error)
				return error;

			absolutePath[strlen(absolutePath) - 1] = '\0';
		}

		absolutePath[strlen(absolutePath) - strlen(current->d_name)] = '\0';
	}
	pthread_mutex_unlock(&lock);

	if (closedir(dir))
		return ERROR_VALUE;

	return SUCCESS_VALUE;
}

//this isn't a recursive function, it removes from watchList all the paths starting with absolutePath
int recursive_remove(char *absolutePath) {

	watchList current = this;
	watchList temp;

	pthread_mutex_lock(&lock);

	//la lista funziona solo con stringhe che terminano con '/'
	if (!watchList_containsFolder(absolutePath) || absolutePath[strlen(absolutePath)-1] != '/')
		return ERROR_VALUE;

	while (current != NULL) {
		//debug_print(DEBUG_OFF, "%s %d\n", current->value->watchFolder, current->value->wd);
		if (startsWith(current->value->watchFolder, absolutePath)) {
			temp = current;
			current = current->next;
			watchList_remove(temp->value->watchFolder);
		} else
			current = current->next;
	}
	pthread_mutex_unlock(&lock);

	return SUCCESS_VALUE;
}

//print watchList
void watchList_print() {
	watchList current = this;
	pthread_mutex_lock(&lock);
	while (current != NULL) {
		debug_print(DEBUG_L1, "path: %s, wd: %d\n", current->value->watchFolder,
				current->value->wd);
		current = current->next;
	}
	pthread_mutex_unlock(&lock);
}

