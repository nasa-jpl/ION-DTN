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
 * tcp_receiver_thread.c
 * Thread launched by tcp_receiver.c to manage the reception of a file from the client:
 * it receives the file independently then in a mutually exclusive phase requires sending via bundle BP.
 * Semaphore (POSIX) grants not concurrency between bp sender and thread.
 * Mutex varable (POSIX) grants not concurrency between receiver threads
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
	int eid_dest_len;
	char * eid_dest;
	//sleep(rand()%10);
	tcp_receiver_info_t * info=(tcp_receiver_info_t *) arg;
	//tcp_to_bundle_inf_t * proxy_inf = info->proxy_info;
	int connectClientSocket=info->connectClientSocket;

	//Changing of default exit routine
	pthread_cleanup_push(criticalError, &connectClientSocket);

	printf("Waiting for data...\n");

	//Read the EID dest
	error = read(connectClientSocket, &eid_dest_len, sizeof(int));
	if(error < 0) {
		error_print("Reading EID dest length failed: %s\n", strerror(errno));
		close_socket(connectClientSocket, SHUT_RD);
	}

	eid_dest = (char *) malloc(sizeof(char) * eid_dest_len);
	for(i = 0; i < eid_dest_len; i++) {
		error = read(connectClientSocket, &eid_dest[i], sizeof(char));
		if(error < 0) {
			error_print("Reading EID dest failed: %s\n", strerror(errno));
			close_socket(connectClientSocket, SHUT_RD);
		}
	}
	eid_dest[i] = '\0';

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
	strcat(file_name, "/tmp/tcpToBundle/");
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

	if(circular_buffer_isFull(info->tcp_bp_buffer)==0) printf("\t%lu says: bp to send buffer full, wait....\n",(long) pthread_self());
	while(circular_buffer_isFull(info->tcp_bp_buffer)==0) sleep(1);
	circular_buffer_item toSend;
	strcpy(toSend.eid_dest, eid_dest);
	strcpy(toSend.fileName, file_name);
	pthread_mutex_lock(&(info->tcp_bp_buffer->mutex));
	circular_buffer_push(info->tcp_bp_buffer,toSend);
	pthread_mutex_unlock(&(info->tcp_bp_buffer->mutex));

	/*Mutex phase
	sem_wait(&proxy_inf->tcpRecv);
	pthread_mutex_lock(&proxy_inf->mutex);
	printf("\t%lu says: send %s\n",(long) pthread_self(),file_name);
	strcpy(proxy_inf->eid_dest, eid_dest);
	strcpy(proxy_inf->fileName, file_name);
	pthread_mutex_unlock(&proxy_inf->mutex);
	sem_post(&proxy_inf->bundleSnd);
	end of mutex phase*/

	free(eid_dest);
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

