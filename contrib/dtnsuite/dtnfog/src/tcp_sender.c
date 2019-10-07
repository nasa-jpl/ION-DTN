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
 * tcp_sender.c
 * This thread has the function of handling bp -> tcp requests
 * and managing the pool of senders threads.
 * This uses two static array:
 * -) sender for senders threads
 * -) sendingQueqe to give thread an exceptionally accessible memory space.
 * The two arrays are statically allocated to N_SENDERS macro (definition.h)
 * which indicates the level of parallelization.
 * Shemaphore is used to block BP receiver side until sender thread is started.
 */

#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

/* ---------------------------
 *      Global variables
 * --------------------------- */
pthread_t sender[N_SENDERS];
circular_buffer_item sendingQueqe[N_SENDERS];

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
static void criticalError(void *arg);
void startThread(circular_buffer_item info);


/**
 * Thread code
 */
void * tcpSending(void * arg) {
	//Changing of default exit routine
	pthread_cleanup_push(criticalError, NULL);

	tcp_sender_info_t * proxy_inf = (tcp_sender_info_t *) arg;
	while(1==1) {
		circular_buffer_item info=circular_buffer_pop(&(proxy_inf->tcp_tosend_buffer));
		//Starting sender thread
		startThread(info);
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
	for(i=0;i<N_SENDERS;i++){
		if(sender[i] && (pthread_kill(sender[i], 0 )) == 0){
			//thread is running
			if(pthread_cancel(sender[i])!=0){
				error_print("Error to close thread: %d\n",i);
			}
		}
	}
}

/**/
void startThread(circular_buffer_item info) {
	int i=0;
	while(1==1){
		for(i=0;i<N_SENDERS;i++){
			if(!sender[i]||(pthread_kill(sender[i], 0 )) != 0){
				sendingQueqe[i]=info;
				pthread_create(&(sender[i]), NULL, tcpSenderThread, (void *)&(sendingQueqe[i]));
				printf("Asseggnato al thread %d\n",i);
				return;
			}
		}
		printf("All sender thread are busy retry in 1 sec\n");
		sleep(1);
	}
}
