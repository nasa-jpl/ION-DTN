/********************************************************
    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
    Carlo Caini (DTNproxy project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

 ********************************************************/

/*
 * proxy_thread.h
 *
 * This file contains
 * 		- library
 * 		- data structures
 * 		- function interface
 * used by all of the threads
 */

#ifndef DTNPROXY_SRC_PROXY_THREAD_H_
#define DTNPROXY_SRC_PROXY_THREAD_H_

/**
 * Includes
 */
#include <al_bp_api.h>
#include <types.h>
#include <includes.h>

#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <signal.h>
#include <semaphore.h>
#include <libgen.h>

#include "../../al_bp/src/al_bp_extB.h"
#include "definitions.h"

/**
 * Data structures
 */

//Data structure for error
typedef enum proxy_error_t {
	SOCKET_ERROR = 0,		//Error after socket creation
	REGISTER_ERROR,			//Error after bp registration
	SEMAPHORE_ERROR,		//Error after sem_init
	THREAD_ERROR			//Error returned by thread
}proxy_error_t;

//Data structure for threads TCP-BUNDLE
typedef struct tcp_to_bundle_inf_t {
	int listenSocket_fd;
	al_bp_extB_registration_descriptor rd_send;
	sem_t tcpRecv;
	sem_t bundleSnd;
	char buffer[MAX_NUM_FILE][FILE_NAME];
	char eid_dest[AL_BP_MAX_ENDPOINT_ID];
	char options;
	int debug_level;
	int * is_running;
	pthread_mutex_t mutex;
	pthread_t tid_recv;
}tcp_to_bundle_inf_t;

//Data structure for threads BUNDLE-TCP
typedef struct bundle_to_tcp_inf_t {
	al_bp_extB_registration_descriptor rd_receive;
	sem_t bundleRecv;
	sem_t tcpSnd;
	char buffer[MAX_NUM_FILE][FILE_NAME];
	char ip_dest[IP_LENGTH];
	int port_dest;
	char options;
	int debug_level;
	int * is_running;
	pthread_mutex_t mutex;
	pthread_t tid_snd;
}bundle_to_tcp_inf_t;

/**
 * Function interfaces
 */
//Thread for TCP receiving
void *tcpReceiving(void * arg);
//Thread for bundle sending
void *bundleSending(void * arg);
//Thread for bundle receiving
void *bundleReceiving(void * arg);
//Thread for TCP sending
void *tcpSending(void * arg);

//Function for program clean exit
void proxy_exit(int error);

#endif /* DTNPROXY_SRC_PROXY_THREAD_H_ */
