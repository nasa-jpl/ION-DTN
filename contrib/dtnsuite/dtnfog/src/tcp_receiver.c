/********************************************************
    Authors:
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

/**
 * tcp_receiver.c
 * This thread has the function of handling tcp -> bp requests
 * and managing the pool of reception threads.
 * This uses two static array:
 * -) receiver for receiver threads
 * -) receiverQueque to give thread an exceptionally accessible memory space.
 * The two arrays are statically allocated to N_RECEIVERS macro (definition.h)
 * which indicates the level of parallelization.
 */

#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

/* ---------------------------
 *      Global variables
 * --------------------------- */
pthread_t receiver[N_RECEIVERS];
tcp_receiver_thread_info_t receiverQueque[N_RECEIVERS];

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
void startReceiverThread(tcp_receiver_thread_info_t info);
static void criticalError(void *arg);

/**
 * Thread code
 */

void * tcpReceiving(void * arg) {
	//tcp_receiver_info_t toReceiver_info;

	int errno;
	int connectClientSocket;
	struct sockaddr_in cliaddr;
	tcp_receiver_info_t * proxy_inf = (tcp_receiver_info_t *) arg;
	uint len = sizeof(cliaddr);

	//Changing of default exit routine
	pthread_cleanup_push(criticalError, NULL);

	//Check if the directory exists
	opendir(TCP_DIR);
	if(errno == ENOENT) {
		debug_print(proxy_inf->debug_level, "[DEBUG] Creating %s\n", TCP_DIR);
		mkdir(TCP_DIR, 0700);
	}

	//Start tcp_receiver like a demon
	while(1==1) {
		printf("Waiting for TCP connection...\n");

		//Accept a TCP client
		if((connectClientSocket = accept(proxy_inf->listenSocket_fd, (struct sockaddr *)&cliaddr, &len)) < 0) {
			if(errno == EINTR) {
				continue;
			}
			else {
				error_print("Error in accept(): %s\n", strerror(errno));
				sleep(10);
				continue;
			}
		}
		//Closing connectClientSocket's write side
		shutdown(connectClientSocket, SHUT_WR);
		printf("Connection accepted\n");

		//Preparation of thread argument
		tcp_receiver_thread_info_t info;
		info.cliaddr=cliaddr;
		info.connectClientSocket=connectClientSocket;
		info.proxy_info=proxy_inf;
		info.calcol_unit_circular_buffer=proxy_inf->calcol_unit_circular_buffer;

		//Starting receiver thread
		startReceiverThread(info);
	}//while

	//Signaling to main an error in daemon like execution
	kill(getpid(),SIGINT);
	pthread_cleanup_pop(1);
	return NULL;
}

/**
 * Custom routine started in case of reception of pthread_cancel by parent. It kills all running threads.
 */
static void criticalError(void *arg){
	int i;
	for(i=0;i<N_RECEIVERS;i++){
		if(receiver[i] && (pthread_kill(receiver[i], 0 )) == 0){
			//thread is running
			if(pthread_cancel(receiver[i])!=0){
				error_print("Error to close thread: %d\n",i);
			}
		}
	}
}

/**
 * Function uses to start a new thread, this has two different behaviors:
 * -)If there are not running threads, assign reception to a new thread
 * -)If all threads are busy it becomes blocking and cycles until one is freed.
 * it uses i index to reserve i memory space to i-thread in an exclusive manner thus avoiding concurrency.
 */
void startReceiverThread(tcp_receiver_thread_info_t info) {
	int i=0;
	while(1==1){
		for(i=0;i<N_RECEIVERS;i++){
			if(!receiver[i]||(pthread_kill(receiver[i], 0 )) != 0){
				receiverQueque[i]=info;
				pthread_create(&(receiver[i]), NULL, tcpReceiverThread, (void *)&(receiverQueque[i]));
				printf("Asseggnato al receiver thread %d\n",i);
				return;
			}
		}
		printf("All receiver thread are busy retry in 1 sec\n");
		sleep(1);
	}
}
