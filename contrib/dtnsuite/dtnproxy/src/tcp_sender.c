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
 * tcp_sender.c
 *
 * This thread is used to send
 * a file using TCP sockets
 *
 */

#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

#define PORT_SERVER 3000
#define IP_DEST "10.0.2.4"

#define NUMBER_ATTEMPTS 5

//void handlerTCPSender(int signal);

/**
 * Thread code
 */
void * tcpSending(void * arg) {
	int error;

	struct hostent * host;
	struct sockaddr_in clientaddr, servaddr;
	int sd;

	int index = 0;

	int fd;
	char file_name[FILE_NAME];
	char char_read;
	int len, i;

	bundle_to_tcp_inf_t * proxy_inf = (bundle_to_tcp_inf_t *) arg;

	//ClientTCP address
	memset((char *)&clientaddr, 0, sizeof(struct sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = INADDR_ANY;
	clientaddr.sin_port = 0;

//	proxy_inf->tid_snd = pthread_self();
//	signal(SIGUSR1, handlerTCPSender);

	while(*(proxy_inf->is_running)) {
		sem_wait(&proxy_inf->tcpSnd);

		//To be replaced with ION metadata or new DTNperf header (-Fp)
		strcpy(proxy_inf->ip_dest, IP_DEST);
		proxy_inf->port_dest = PORT_SERVER;

		//ServerTCP address
		memset((char *)&servaddr, 0, sizeof(struct sockaddr_in));
		servaddr.sin_family = AF_INET;
		host = gethostbyname(proxy_inf->ip_dest);

		if(host == NULL) {
			error_print("Server not found: %s\n", strerror(errno));
			continue;
		} else {
			servaddr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
			servaddr.sin_port = htons(proxy_inf->port_dest);
		}

		sd = socket(AF_INET, SOCK_STREAM, 0);
		if(sd < 0) {
			error_print("Creating socket failed: %s\n", strerror(errno));
			continue;
		}

		//Try to connect to server
		int attempt;
		for(attempt = 0; attempt < (NUMBER_ATTEMPTS); attempt ++) {
			if(connect(sd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
				error_print("Connect to: %s failed (%s)\n", host->h_name, strerror(errno));
				error_print("Trying to connect, attemp n%d\n", attempt);
				sleep(10);
				continue;
			}
			else
				break;
		}
		if(attempt == (NUMBER_ATTEMPTS - 1)) {
			error_print("Failed to connect to %s\n", host->h_name);
			continue;
		}

		printf("Connect to %s\n", host->h_name);

		shutdown(sd, SHUT_RD);

		strcpy(file_name, proxy_inf->buffer[index]);
		debug_print(proxy_inf->debug_level, "[DEBUG] file_received: %s, index: %d\n",
				basename(file_name), index);

		len = strlen(basename(file_name));
		error = write(sd, &len, sizeof(int));
		if(error < 0) {
			error_print("Writing socket failed: %s\n", strerror(errno));
			close_socket(sd, SHUT_WR);
			continue;
		}

		char * basename_file = (char *) malloc(sizeof(char) * strlen(basename(file_name)));
		strcpy(basename_file, basename(file_name));
		for(i = 0; i < len; i++) {
			error = write(sd, &basename_file[i], sizeof(char));
			if(error < 0) {
				error_print("Writing socket failed: %s\n", strerror(errno));
				close_socket(sd, SHUT_WR);
				continue;
			}
		}
		free(basename_file);

		if(*(proxy_inf->is_running) == 0) {
			close_socket(sd, SHUT_WR);
			break;
		}

		fd = open(file_name, O_RDONLY);
		if(fd < 0) {
			error_print("Error in open() file %s: %s\n", file_name, strerror(errno));
			close_socket(sd, SHUT_WR);
			continue;
		}

		while((error = read(fd, &char_read, sizeof(char))) > 0) {
			error = write(sd, &char_read, sizeof(char));
			if(error < 0) {
				error_print("Writing socket failed: %s\n", strerror(errno));
				close_socket(sd, SHUT_WR);
				close(fd);
				continue;
			}
		}
		close(fd);

		if(*(proxy_inf->is_running) == 0) {
			close_socket(sd, SHUT_WR);
			break;
		}

		printf("File sent\n");

		error = remove(file_name);
		if(error < 0) {
			error_print("Removing file failed: %s\n", strerror(errno));
			close_socket(sd, SHUT_WR);
			continue;
		}

		close_socket(sd, SHUT_WR);

		strcpy(file_name, "");
		index = (index + 1) % MAX_NUM_FILE;

		sem_post(&proxy_inf->bundleRecv);
	}//while

//	printf("TCPSender: i'm here\n");
	//pthread_exit((void *) EXIT_FAILURE);

	return NULL;
}

//void handlerTCPSender(int signal) {
////	printf("TCPSender: signal\n");
////	pthread_exit((void *) EXIT_FAILURE);
//}
