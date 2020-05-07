/********************************************************
    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
	Lorenzo Tullini, lorenzo.tullini@studio.unibo.it
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

#include "al_bp_extB.h"
#include "definitions.h"

/**
 * Data structures
 */

/*Data structure to describe initialization status
typedef struct proxy_init_status_t {
	int SOCKET;
    int THREADS_STARTED;
	int SEMAPHORETCP;
	int SEMAPHOREBP;
	int BPSEND_REGISTRED;
	int BPRECIVE_REGISTRED;
	int BPINIT;
}proxy_init_status_t;*/

//Data structure for error
typedef enum proxy_error_t {
	SOCKET_ERROR = 0,		//Error after socket creation
	REGISTER_ERROR,			//Error after bp registration
	SEMAPHORE_ERROR,		//Error after sem_init
	THREAD_ERROR			//Error returned by thread
}proxy_error_t;

//Data structure for cricular buffer item
typedef struct circular_buffer_item {
	char fileName[FILE_NAME];
	char eid_dest[AL_BP_MAX_ENDPOINT_ID];
	char ip_dest[IP_LENGTH];
	int port_dest;
}circular_buffer_item;

//Data structure for ring buffer tpc->bp
typedef struct circular_buffer_t{
	circular_buffer_item * buffer;
	circular_buffer_item * buffer_end;
	circular_buffer_item * data_start;
	circular_buffer_item * data_end;
    int count;
    int size;
	pthread_mutex_t mutex;
} circular_buffer_t;

//Data structure for threads TCP-BUNDLE
typedef struct tcp_to_bundle_inf_t {
	int listenSocket_fd;
	al_bp_extB_registration_descriptor rd_send;
	int debug_level;
	char options;
	circular_buffer_t tcp_bp_buffer;
}tcp_to_bundle_inf_t;

//Data structure for threads BUNDLE-TCP
typedef struct bundle_to_tcp_inf_t {
	al_bp_extB_registration_descriptor rd_receive;
	char options;
	int debug_level;
	pthread_mutex_t mutex;
	int error;
	circular_buffer_t bp_tcp_buffer;
}bundle_to_tcp_inf_t;

//Data structure for tcp receiver thread which contains the minimum information to coplete the job
typedef struct tcp_receiver_info_t{
	int connectClientSocket;
	tcp_to_bundle_inf_t * proxy_info;
	struct sockaddr_in cliaddr;
	circular_buffer_t * tcp_bp_buffer;
}tcp_receiver_info_t;




/**
 * Function interfaces
 */
//Thread for TCP receiving
void *tcpReceiving(void * arg);
//Thread for bundle sending
void *bundleSending(void * arg);
//Thread for bundle receiving
void *bundleReceiving(void * arg);
//Thread for TCP sending list
void *tcpSending(void * arg);
//Thread for TCP sender thread
void * tcpSenderThread(void * arg);
//Thread for TCP receiver thread
void * tcpReceiverThread(void * arg);
//Function for program clean exit
void proxy_exit(int error);

void circular_buffer_init(circular_buffer_t * cb, int size);
void circular_buffer_free(circular_buffer_t* cb);
int circular_buffer_isFull(circular_buffer_t* cb);
int circular_buffer_push(circular_buffer_t* cb, circular_buffer_item data);
circular_buffer_item circular_buffer_pop(circular_buffer_t* cb);
int circular_buffer_isEmpty(circular_buffer_t* cb);

#endif /* DTNPROXY_SRC_PROXY_THREAD_H_ */




