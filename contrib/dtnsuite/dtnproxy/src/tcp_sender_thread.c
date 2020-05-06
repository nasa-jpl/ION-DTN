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


/**
 * tcp_sender_thread.c
 * Thread launched by tcp_sender.c to manage sending side of bp -> tcp interaction.
 * It sends to server ip file recived by BP protocol.
 * Semaphore (POSIX) grants not concurrency between bp receiver and thread.
 */

#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
static void criticalError(void *arg);

/**
 * Thread code
 */
void * tcpSenderThread(void * arg) {
	circular_buffer_item * info=(circular_buffer_item *) arg;
	int toSleep=0;//debug
	int error;

	struct hostent * host;
	struct sockaddr_in clientaddr, servaddr;
	int sd;

	int fd;
	char file_name[FILE_NAME];
	char char_read;
	int len, i;

	pthread_cleanup_push(criticalError, &sd);
	//printf("\tHellow I'm tcp senderd thread pid: %lu\n\tI'll send %s to %s:%d\n",(long) pthread_self(),info->filename,info->ip,info->port);
	sleep(toSleep); //debug

	//ClientTCP address
	memset((char *)&clientaddr, 0, sizeof(struct sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = INADDR_ANY;
	clientaddr.sin_port = 0;

	//ServerTCP address
	memset((char *)&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	host = gethostbyname(info->ip_dest);

	if(host == NULL) {
		error_print("Server not found: %s\n", strerror(errno));
	} else {
		servaddr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
		servaddr.sin_port = htons(info->port_dest);
	}

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		error_print("Creating socket failed: %s\n", strerror(errno));
	}


	//Try to connect to server
	int attempt;
	for(attempt = 0; attempt < (NUMBER_ATTEMPTS); attempt ++) {
		if(connect(sd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
			error_print("Connect to: %s failed (%s)\n", host->h_name, strerror(errno));
			error_print("Trying to connect, attemp n%d\n", attempt);
			sleep(10);
		}
		else
			break;
	}

	if(attempt == (NUMBER_ATTEMPTS - 1)) {
		error_print("Failed to connect to %s\n", host->h_name);
		return NULL;
	}

	//printf("\t%lu: Connect to %s\n",(long) pthread_self(), host->h_name);

	//Closing socket read side
	shutdown(sd, SHUT_RD);

	//Sending filename
	strcpy(file_name, info->fileName);
	len = strlen(basename(file_name));
	error = write(sd, &len, sizeof(int));
	if(error < 0) {
		error_print("Writing socket failed: %s\n", strerror(errno));
		close_socket(sd, SHUT_WR);
	}

	char * basename_file = (char *) malloc(sizeof(char) * strlen(basename(file_name)));
	strcpy(basename_file, basename(file_name));
	for(i = 0; i < len; i++) {
		error = write(sd, &basename_file[i], sizeof(char));
		if(error < 0) {
			error_print("Writing socket failed: %s\n", strerror(errno));
			close_socket(sd, SHUT_WR);
		}
	}
	free(basename_file);

	//Opening file
	fd = open(file_name, O_RDONLY);
	if(fd < 0) {
		error_print("Error in open() file %s: %s\n", file_name, strerror(errno));
		close_socket(sd, SHUT_WR);
	}
	//Bufferizzare !!!
	while((error = read(fd, &char_read, sizeof(char))) > 0) {
		error = write(sd, &char_read, sizeof(char));
		if(error < 0) {
			error_print("Writing socket failed: %s\n", strerror(errno));
			close_socket(sd, SHUT_WR);
			close(fd);
		}
	}
	close(fd);

	//Removing file
	//printf("\t%lu: File %s sent\n",(long) pthread_self(),info->filename);
	error = remove(file_name);
	if(error < 0) {
		error_print("Removing file failed: %s\n", strerror(errno));
		close_socket(sd, SHUT_WR);
	}


	strcpy(file_name, "");
	pthread_cleanup_pop(1);
	return NULL;
}


/**
 * Custom routine started in case of reception of pthread_cancel by parent. It closes socket.
 */
static void criticalError(void *arg){
	int *sd=(int*) arg;
	//error_print("\t%lu says: Critical error occurrit!!\n",(long) pthread_self());
	//printf("\t%lu TCPsender: exit\n",(long) pthread_self());
	close_socket(*sd, SHUT_WR);
}
