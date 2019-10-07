/********************************************************
    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
	Lorenzo Tullini, lorenzo.tullini@studio.unibo.it
    Carlo Caini (DTNfog project supervisor), carlo.caini@unibo.it

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

#ifndef DTNfog_SRC_PROXY_THREAD_H_
#define DTNfog_SRC_PROXY_THREAD_H_

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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
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

//Data structure for error
typedef enum processing_error_t {
	RETURN_OK = 0,		//Well file extraction
	MOVING_ERROR,		//Error moving file to calcol dir
	RETURNVALUE_ERROR,	//Error not valid tar file
	VALIDTAR_ERROR,		//Error in valid tar checking
	DELETE_ERROR,		//Error in deleting files
	COMMAND_NOT_FOUND	//Error command not found in allowed list
}processing_error_t;

typedef enum processing_return_t {
	IS_FOR_CLIENT = 0,		//File tar was sanding by a clint
	IS_FROM_CLOUD,			//File tar was sanding by cloud
	RESULT_FOUND,		//In file tar there is result so dtnfog works like a network switch
	RESULT_NOT_FOUND	//In file tar there is no result so dtnfog traies to execute comand
}processing_return_t;
/*-------------------------------
 *       CIRCULAR BUFFER
 * -----------------------------
 * */


//Data structure for cricular buffer item
typedef struct circular_buffer_item {
	char fileName[FILE_NAME];
	char eid[AL_BP_MAX_ENDPOINT_ID];
	char ip[IP_LENGTH];
	int port;
	int protocol; //BP=0 TCP=1
}circular_buffer_item;

//Data structure for ring buffer
typedef struct circular_buffer_t{
	circular_buffer_item * buffer;
	circular_buffer_item * buffer_end;
	circular_buffer_item * data_start;
	circular_buffer_item * data_end;
    int count;
    int size;
	pthread_mutex_t mutex;
} circular_buffer_t;

/*-------------------------------
 *         CALCO UNIT
 * -----------------------------
 * */

typedef struct calcol_unit_info{
	circular_buffer_t calcol_unit_buffer;
	char options;
	int debug_level;
	circular_buffer_t * tcp_tosend_buffer;
	circular_buffer_t *bp_tosend_buffer;
	int cloud;
	char allowed_comand[(COMMAND_SIZE*MAX_NUMBER_OF_COMMANDS)+COMMAND_SIZE];
	int numberOfCommand;
	char mycloud[IP_LENGTH];
	char communication_address[IP_LENGTH+6];
	char tcp_address[IP_LENGTH+6];
	char bp_address[IP_LENGTH+6];
	char addresso_to_print[IP_LENGTH+6];
	int report;
}calcol_unit_info_t;

typedef struct processor_info{
	circular_buffer_item item;
	circular_buffer_t * tcp_tosend_buffer;
	circular_buffer_t *bp_tosend_buffer;
	int debug_level;
	char options;
	int cloud;
	char allowed_comand[(COMMAND_SIZE*MAX_NUMBER_OF_COMMANDS)+COMMAND_SIZE];
	int numberOfCommand;
	char mycloud[IP_LENGTH];
	char comunication_address[IP_LENGTH+6];
	char tcp_address[IP_LENGTH+6];
	char bp_address[IP_LENGTH+6];
	char addresso_to_print[IP_LENGTH+6];
	int report;
}processor_info_t;

typedef struct command{
	int cmdLen;
	int number_of_file;
	char cmd[EXCOMMAND_SIZE];
	char files[MAX_NUMBER_OF_FILES][FILE_NAME];
}command_t;

/*-------------------------------
 *        SENDER SIDE
 * -----------------------------
 * */

//Data structure for threads TCP SENDER
typedef struct tcp_sender_info {
	int debug_level;
	char options;
	circular_buffer_t tcp_tosend_buffer;
}tcp_sender_info_t;

//Data structure for threads BUNDLE SENDER
typedef struct bp_sender_info {
	al_bp_extB_registration_descriptor rd_send;
	char options;
	int debug_level;
	int error;
	circular_buffer_t bp_tosend_buffer;
}bp_sender_info_t;


/*-------------------------------
 *        RECEIVER SIDE
 * -----------------------------
 * */

typedef struct tcp_receiver_info{
	int listenSocket_fd;
	int debug_level;
	char options;
	circular_buffer_t * calcol_unit_circular_buffer;
}tcp_receiver_info_t;

//Data structure for tcp receiver thread which contains the minimum information to coplete the job
typedef struct tcp_receiver_thread_info{
	int connectClientSocket;
	tcp_receiver_info_t * proxy_info;
	struct sockaddr_in cliaddr;
	circular_buffer_t * calcol_unit_circular_buffer;
}tcp_receiver_thread_info_t;

typedef struct bp_receiver_info{
	al_bp_extB_registration_descriptor rd_receive;
	int debug_level;
	char options;
	int error;
	circular_buffer_t * calcol_unit_circular_buffer;
}bp_receiver_info_t;




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
//Thread of calcol and execution of dtnfog
void * calcolUnitThread(void * arg);
//Thread of calcol processing
void * processorThread(void * arg);
void circular_buffer_init(circular_buffer_t * cb, int size);
void circular_buffer_free(circular_buffer_t* cb);
int circular_buffer_isFull(circular_buffer_t* cb);
int circular_buffer_push(circular_buffer_t* cb, circular_buffer_item data);
circular_buffer_item circular_buffer_pop(circular_buffer_t* cb);
int circular_buffer_isEmpty(circular_buffer_t* cb);

#endif /* DTNfog_SRC_PROXY_THREAD_H_ */




