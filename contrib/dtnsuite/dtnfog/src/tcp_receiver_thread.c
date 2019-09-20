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


/**
 * tcp_receiver_thread.c
 * Thread launched by tcp_receiver.c to manage the reception of a file from the client:
 * it receives the file independently then in a mutually exclusive phase requires sending via bundle BP.
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
void * tcpReceiverThread(void * arg){
	int fd,error;
	char char_read;
	char file_name[FILE_NAME];
	char * temp_file_name;
	int file_name_len;
	int i;
	//int eid_dest_len;
	//char * eid_dest;
	//sleep(rand()%10);
	tcp_receiver_thread_info_t * info=(tcp_receiver_thread_info_t *) arg;
	//tcp_to_bundle_inf_t * proxy_inf = info->proxy_info;
	int connectClientSocket=info->connectClientSocket;

	//Changing of default exit routine
	pthread_cleanup_push(criticalError, &connectClientSocket);

	printf("Waiting for data...\n");

	//Read the filename
	error = read(connectClientSocket, &file_name_len, sizeof(int));
	if(error < 0) {
		error_print("Reading filename length failed: %s\n", strerror(errno));
		close_socket(connectClientSocket, SHUT_RD);
	}
	temp_file_name = (char *) malloc(sizeof(char) * file_name_len);
	for(i = 0; i < file_name_len; i++) {
		error = read(connectClientSocket, &temp_file_name[i], sizeof(char));
		if(error < 0) {
			error_print("Reading filename failed: %s\n", strerror(errno));
			close_socket(connectClientSocket, SHUT_RD);
		}
	}
	temp_file_name[i] = '\0';

	//Create filename
	strcat(file_name, CLC_DIR);
	strcat(file_name, basename(temp_file_name));
	fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, 0700);
	if(fd < 0) {
		error_print("Opening file %s failed: %s\n", file_name, strerror(errno));
		close_socket(connectClientSocket, SHUT_RD);

	}

	//Receiving the file
	while((error = read(connectClientSocket, &char_read, sizeof(char))) > 0) {
		error = write(fd, &char_read, sizeof(char));
		if(error < 0) {
			error_print("Writing the file %s failed: %s\n", file_name, strerror(errno));
			close_socket(connectClientSocket, SHUT_RD);
			close(fd);
		}
	}
	close(fd);
	close_socket(connectClientSocket, SHUT_RD);
	printf("Socket closed\n");

	circular_buffer_item toSend;
	toSend.protocol=1;
	//Salving of source ip
	strcpy(toSend.ip,inet_ntoa(info->cliaddr.sin_addr));
	toSend.port= (int) ntohs(info->cliaddr.sin_port);
	strcpy(toSend.fileName, file_name);

	circular_buffer_push(info->calcol_unit_circular_buffer,toSend);
	printf("TCPreceiver: Send to calcol unit %s\n",toSend.fileName);

	//free(eid_dest);
	free(temp_file_name);
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

