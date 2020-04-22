/********************************************************
    DTNproxy Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
	Lorenzo Tullini, lorenzo.tullini@studio.unibo.it
    Carlo Caini (DTNproxy project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 ********************************************************/

/*
 * main.c
 *
 * This file contains the main of
 * DTNproxy
 *
 */

#include "debugger.h"
#include "definitions.h"
#include "proxy_thread.h"


#define IPN_DEMUX_SENDER 5001
#define IPN_DEMUX_RECEIVER 5000
#define N_PORT 2500

/* ---------------------------
 *      Global variables
 * --------------------------- */

int listenSocket;
al_bp_extB_error_t utility_error;
al_bp_extB_registration_descriptor rd_send;
al_bp_extB_registration_descriptor rd_receive;
pthread_t tcpReceiver;
pthread_t bundleSender;
pthread_t bundleReceiver;
pthread_t tcpSender;
//sem_t tcpRecv;
//sem_t bundleSnd;
sem_t bundleRecv;
sem_t tcpSnd;
proxy_error_t error;
tcp_to_bundle_inf_t tcp_to_bundle_inf;
bundle_to_tcp_inf_t bundle_to_tcp_inf;
int level_debug = 0;
char mode;
const char * program_name;
//circular_buffer_t tcp_bp_buffer;
//circular_buffer_t bp_tcp_buffer;

/**
 * Shared variables used in case of thread termination
 * Threads check if this variable is TRUE, otherwise they terminate themselves
 * It is used with a pthread_mutex
 */
int is_running = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
void proxy_exit(int error);
void print_usage(const char * program_name);
void parse_options(int argc, char * argv[]);

/**
 * Main code
 */
int main(int argc, char *argv[]){
	printf("DTNproxy PID: %d\n",getpid());
	const int on = 1;
	struct sockaddr_in proxyaddr;
	sigset_t set;
	int sig;

	//Parse command line options
	parse_options(argc, argv);
	debugger_init(level_debug, TRUE, LOG_FILENAME);


	//Init TCP side
	memset((char *)&proxyaddr, 0, sizeof(proxyaddr));
	proxyaddr.sin_family = AF_INET;
	proxyaddr.sin_addr.s_addr = INADDR_ANY;
	proxyaddr.sin_port = htons(N_PORT);

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocket < 0) {
		error_print("Error in creating the socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	debug_print(level_debug, "[DEBUG] listen socket create with fd: %d\n", listenSocket);

	if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		error_print("Error in setting socket option: %s\n", strerror(errno));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}

	if(bind(listenSocket, (struct sockaddr *)&proxyaddr, sizeof(proxyaddr)) < 0) {
		error_print("Error in bind(): %s\n", strerror(errno));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}

	if(listen(listenSocket, 5) < 0) {
		error_print("Error in listen(): %s\n", strerror(errno));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] init TCP side: success\n");

	//Init DTN side
	utility_error = al_bp_extB_init('N',0);
	if (utility_error != BP_EXTB_SUCCESS) {
		error_print("Error in al_bp_extB_init(): (%s)\n", al_bp_strerror(utility_error));
		error = SOCKET_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] al_bp_extB_init: success\n");

	//Register BundleSender to BP
	utility_error = al_bp_extB_register(&rd_send, "proxy_sender", IPN_DEMUX_SENDER);
	if (utility_error != BP_EXTB_SUCCESS) {
		error_print("Error in al_bp_extB_register() for sending thread (%s)\n", al_bp_strerror(utility_error));
		error = REGISTER_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] al_bp_extB_register for sending thread, rd: %d\n", rd_send);

	//Register BundleReceiver to BP
	utility_error = al_bp_extB_register(&rd_receive, "proxy_receiver", IPN_DEMUX_RECEIVER);
	if (utility_error != BP_EXTB_SUCCESS) {
		error_print("Error in al_bp_extB_register() for receiving thread (%s)\n", al_bp_strerror(utility_error));
		error = REGISTER_ERROR;
		proxy_exit(error);
	}
	debug_print(level_debug, "[DEBUG] al_bp_extB_register for receiving thread, rd: %d\n", rd_receive);
	debug_print(level_debug, "[DEBUG] init DTN side: success\n");

	/*Init semaphores
	if(sem_init(&tcpRecv, 0, 1) || sem_init(&bundleSnd, 0, 0)) {
		error_print("Error in sem_init: (%s)\n", strerror(errno));
		error = SEMAPHORE_ERROR;
		proxy_exit(error);
	}*/

	if(sem_init(&bundleRecv, 0, 1) || sem_init(&tcpSnd, 0, 0)) {
		error_print("Error in sem_init: (%s)\n", strerror(errno));
		error = SEMAPHORE_ERROR;
		proxy_exit(error);
	}

	//Init data structure
	circular_buffer_init(&(tcp_to_bundle_inf.tcp_bp_buffer),MAX_BUFFER_SIZE);
	tcp_to_bundle_inf.listenSocket_fd = listenSocket;
	tcp_to_bundle_inf.rd_send = rd_send;
	tcp_to_bundle_inf.debug_level = level_debug;
	if (pthread_mutex_init(&(tcp_to_bundle_inf.tcp_bp_buffer.mutex), NULL) != 0){
		        printf("\n mutex init failed\n");
		        proxy_exit(SIGINT);
	}

	circular_buffer_init(&(bundle_to_tcp_inf.bp_tcp_buffer),MAX_BUFFER_SIZE);
	bundle_to_tcp_inf.rd_receive = rd_receive;
	bundle_to_tcp_inf.debug_level = level_debug;
	bundle_to_tcp_inf.mutex = mutex;
	if (pthread_mutex_init(&(bundle_to_tcp_inf.bp_tcp_buffer.mutex), NULL) != 0){
	        printf("\n mutex init failed\n");
	        proxy_exit(SIGINT);
	}

	//Init mode
	if(mode == 'n') {
		tcp_to_bundle_inf.options = mode;
		bundle_to_tcp_inf.options = mode;

		printf("DTNperf compatibility: none\n");
	}
	else {
		tcp_to_bundle_inf.options = 'd';
		bundle_to_tcp_inf.options = 'd';

		printf("DTNperf compatibility: ok\n");
	}

	//Assing proxy_exit to SIGINT and init mask to critic error signal from threads
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, NULL);


	//Init threads
	pthread_create(&tcpReceiver, NULL, tcpReceiving, (void *)&tcp_to_bundle_inf);
	pthread_create(&bundleSender, NULL, bundleSending, (void *)&tcp_to_bundle_inf);
	pthread_create(&bundleReceiver, NULL, bundleReceiving, (void *)&bundle_to_tcp_inf);
	pthread_create(&tcpSender, NULL, tcpSending, (void *)&bundle_to_tcp_inf);

	//Wait for children return,in this case, since the children are daemons, dnproxy ends with an error
	sigwait(&set, &sig);
	//printf("Main had recived a %d from threads\n",sig);
	proxy_exit(SIGINT);
	return EXIT_FAILURE;
}


/**
 * Function for clean exit
 */
void proxy_exit(int error) {
	circular_buffer_free(&(tcp_to_bundle_inf.tcp_bp_buffer));
	circular_buffer_free(&(bundle_to_tcp_inf.bp_tcp_buffer));
	if((error == THREAD_ERROR) || error == SIGINT) {
		//Close threads
		pthread_cancel(tcpReceiver);
		printf("TCPreceiver: exit\n");
		pthread_cancel(bundleSender);
		printf("BPsender: exit\n");
		pthread_cancel(bundleReceiver);
		printf("BPreceiver: exit\n");
		pthread_cancel(tcpSender);
		printf("TCPsender: exit\n");
	}

	//Close socket
	if(error==SOCKET_ERROR || error == SIGINT){
		shutdown(listenSocket, 0);
		shutdown(listenSocket, 1);
		close(listenSocket);
	}

	if(error == REGISTER_ERROR) {
		al_bp_extB_destroy();
		exit(EXIT_FAILURE);
	}

	if(((error != SOCKET_ERROR && error != REGISTER_ERROR)) || error == SIGINT) {
		if(bundle_to_tcp_inf.error!=BP_EXTB_ERRRECEIVE){
			//Unregister BundleSender and BundleReceiver
			utility_error = al_bp_extB_unregister(rd_send);
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_extB_unregister() for sending thread (%s)\n", al_bp_strerror(utility_error));
				exit(EXIT_FAILURE);
			}
			debug_print(level_debug, "[DEBUG] al_bp_extB_unregister for sending thread\n");

			utility_error = al_bp_extB_unregister(rd_receive);
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_extB_unregister() for receiving thread (%s)\n", al_bp_strerror(utility_error));
				exit(EXIT_FAILURE);
			}
			debug_print(level_debug, "[DEBUG] al_bp_extB_unregister for receiving thread\n");

			al_bp_extB_destroy();
		}

		//Destroy semaphores
		if(((error != SOCKET_ERROR && error != REGISTER_ERROR && error != SEMAPHORE_ERROR)) || error == SIGINT) {
			//sem_destroy(&tcpRecv);
			//sem_destroy(&bundleSnd);
			sem_destroy(&bundleRecv);
			sem_destroy(&tcpSnd);
		}
	}

	if(error == SIGINT) {
		printf("Exit...\n");
		exit(EXIT_SUCCESS);
	}


	error_print("Exit...\n");
	exit(EXIT_FAILURE);
}

/**
 * Function for printing program usage
 */
void print_usage(const char* progname){
	fprintf(stderr, "\n");
	fprintf(stderr, "DTNproxy\n");
	fprintf(stderr, "SYNTAX: %s [options]\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, " --no-header             Bundles do not contain DTNperf header\n");
	fprintf(stderr, " --debug                 Debug messages\n");
	fprintf(stderr, " --help                  Print this screen.\n");
	fprintf(stderr, "\n");
}

/**
 * Function for parsing command line options
 */
void parse_options(int argc, char * argv[]) {
	program_name = basename(argv[0]);

	char c, done = 0;

	while(!done) {
		static struct option long_options[] =
		{
				{"help", no_argument, 0, 'h'},
				{"no-header", no_argument, 0, 'n'},
				{"debug", no_argument, 0, 'd'},
				{0, 0, 0, 0}
		};

		int option_index = 0;
		c = getopt_long(argc, argv, "hnd", long_options, &option_index);

		switch(c)
		{
		case 'h':
			print_usage(program_name);
			exit(0);
			return;
		case 'n':
			mode = 'n';
			break;
		case 'd':
			level_debug = DEBUG_ON;
			break;
		case (char) -1:
				done = 1;
		break;
		default:
			print_usage(program_name);
			exit(0);
		}
	}

}

