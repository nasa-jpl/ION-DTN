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
 * tcp_receiver.c
 *
 * This thread receives a file from a Client TCP
 */

#include "proxy_thread.h"
#include "debugger.h"
#include "utility.h"

//void handlerTCPReceiver(int signal);

/**
 * Thread code
 */
void * tcpReceiving(void * arg) {
	int error;

	int connectClientSocket;
	struct sockaddr_in cliaddr;
	uint len = sizeof(cliaddr);
	int eid_dest_len;
	char * eid_dest;

	tcp_to_bundle_inf_t * proxy_inf = (tcp_to_bundle_inf_t *) arg;

	int fd;
	char char_read;
	char file_name[FILE_NAME];
	char * temp_file_name;
	int file_name_len;

	int index = 0;
	int i;

	strcpy(file_name, "");

	//Check if the directory exists
	opendir(TCP_DTN_DIR);
	if(errno == ENOENT) {
		debug_print(proxy_inf->debug_level, "[DEBUG] Creating %s\n", TCP_DTN_DIR);
		mkdir(TCP_DTN_DIR, 0700);
	}

//	proxy_inf->tid_recv = pthread_self();
//	signal(SIGUSR1, handlerTCPReceiver);

	while(*(proxy_inf->is_running)) {
		sem_wait(&proxy_inf->tcpRecv);

		printf("Waiting for TCP connection...\n");

		if(*(proxy_inf->is_running) == 0)
			break;

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

		shutdown(connectClientSocket, SHUT_WR);

		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}

		printf("Connection accepted\n");
		printf("Waiting for data...\n");

		//Read the EID dest
		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}

		error = read(connectClientSocket, &eid_dest_len, sizeof(int));
		if(error < 0) {
			error_print("Reading EID dest length failed: %s\n", strerror(errno));
			close_socket(connectClientSocket, SHUT_RD);
			continue;
		}

		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}
		eid_dest = (char *) malloc(sizeof(char) * eid_dest_len);
		for(i = 0; i < eid_dest_len; i++) {
			error = read(connectClientSocket, &eid_dest[i], sizeof(char));
			if(error < 0) {
				error_print("Reading EID dest failed: %s\n", strerror(errno));
				close_socket(connectClientSocket, SHUT_RD);
				continue;
			}
		}
		eid_dest[i] = '\0';

		strcpy(proxy_inf->eid_dest, eid_dest);

		//Read the filename
		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}
		error = read(connectClientSocket, &file_name_len, sizeof(int));
		if(error < 0) {
			error_print("Reading filename length failed: %s\n", strerror(errno));
			close_socket(connectClientSocket, SHUT_RD);
			continue;
		}
		temp_file_name = (char *) malloc(sizeof(char) * file_name_len);
		for(i = 0; i < file_name_len; i++) {
			error = read(connectClientSocket, &temp_file_name[i], sizeof(char));
			if(error < 0) {
				error_print("Reading filename failed: %s\n", strerror(errno));
				close_socket(connectClientSocket, SHUT_RD);
				continue;
			}
		}
		temp_file_name[i] = '\0';

		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}

		//Create filename
		strcat(file_name, "/tmp/tcpToBundle/");
		strcat(file_name, basename(temp_file_name));
		//		strcat(file_name, "_");
		//		char address16];
		//		char ip[16];
		//		sprintf(file_name, "%s", inet_ntop(AF_INET, &(cliaddr.sin_addr.s_addr), ip, INET_ADDRSTRLEN));
		//		strcat(file_name, address);
		//		char porta[4];
		//		sprintf(porta, "%d", ntohs(cliaddr.sin_port));
		//		strcat(file_name, "_");
		//		char timestamp[5];
		//		sprintf(timestamp, "%d", (int)time(NULL));
		//		strcat(file_name, timestamp);
		//		strcat(file_name, "file_");
		//		char indice[2];
		//		sprintf(indice, "%d", index);
		//		strcat(file_name, indice);

		fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, 0700);
		if(fd < 0) {
			error_print("Opening file %s failed: %s\n", file_name, strerror(errno));
			close_socket(connectClientSocket, SHUT_RD);
			continue;
		}

		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}

		while((error = read(connectClientSocket, &char_read, sizeof(char))) > 0) {
			error = write(fd, &char_read, sizeof(char));
			if(error < 0) {
				error_print("Writing the file %s failed: %s\n", file_name, strerror(errno));
				close_socket(connectClientSocket, SHUT_RD);
				close(fd);
				continue;
			}
		}
		close(fd);

		if(*(proxy_inf->is_running) == 0) {
			close_socket(connectClientSocket, SHUT_RD);
			break;
		}
		strcpy(proxy_inf->buffer[index], file_name);
		debug_print(proxy_inf->debug_level,
				"[DEBUG]: index: %d %s\n", index, basename(proxy_inf->buffer[index]));
		index = (index + 1) % MAX_NUM_FILE;

		strcpy(file_name, "");

		close_socket(connectClientSocket, SHUT_RD);

		printf("Socket closed\n");

		sem_post(&proxy_inf->bundleSnd);
	}//while

//	printf("TCPReceiver: i'm here\n");
	//pthread_exit((void *) EXIT_FAILURE);

	return NULL;
}

//void handlerTCPReceiver(int signal) {
//
//}
