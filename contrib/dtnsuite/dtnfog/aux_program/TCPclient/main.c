/********************************************************
    TCPclient Copyright (c) 2018, Alma Mater Studiorum, University of Bologna
    This is a simple TCP client designed to be used with DTNproxy

    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

/*
 * Usage: ./TCPclient serverAddress serverPort
 */

#define BUFFER_LENGTH 10

int main(int argc, char * argv[]) {
	struct hostent * host;
	struct sockaddr_in clientaddr, proxyaddr;
	int error;
	int port, sd, index, len;
	char * eid_dest;
	char * file_name;

	//Check command line arguments
	if(argc != 4) {
		fprintf(stderr, "[Client]: Usage: %s file_name proxy_address proxy_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//Init client address
	memset((char *)&clientaddr, 0, sizeof(struct sockaddr_in));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = INADDR_ANY;
	clientaddr.sin_port = 0;
	//Init server address
	memset((char *)&proxyaddr, 0, sizeof(struct sockaddr_in));
	proxyaddr.sin_family = AF_INET;
	host = gethostbyname(argv[2]);
	//Check if port is correct
	index = 0;
	while(argv[3][index] != '\0') {
		if((argv[3][index] < '0') || (argv[3][index] > '9')) {
			fprintf(stderr, "[Client]: TCP port is an integer: %s\n", argv[3]);
			exit(EXIT_FAILURE);
		}
		index++;
	}

	port = atoi(argv[3]);
	//Check if TCP port is in the correct range (1024-65535)
	if(port < 1024 || port > 65535) {
		fprintf(stderr, "[Client]: TCP port isn't correct: %d\n", port);
		exit(EXIT_FAILURE);
	}
	
	file_name = (char *) malloc(sizeof(char) * strlen(argv[1]));
	strcpy(file_name, argv[1]);

	if(host == NULL) {
		fprintf(stderr, "[Client]: host not found\n");
		exit(EXIT_FAILURE);
	} else {
		proxyaddr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr))->s_addr;
		proxyaddr.sin_port = htons(port);
	}

	//Create socket
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0) {
		fprintf(stderr, "[Client]: Error in creating socket\n");
		exit(EXIT_FAILURE);
	}

	shutdown(sd, 0);

	//Try to connect
	if(connect(sd, (struct sockaddr *)&proxyaddr, sizeof(struct sockaddr)) < 0) {
		fprintf(stderr, "[Client]: Error in connect() %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	int i;
	/*len = strlen(eid_dest);
	error = write(sd, &len, sizeof(int));
	if(error < 0) {
		fprintf(stderr, "Error in writing socket\n");
		exit(EXIT_FAILURE);
	}

	int i;
	for(i = 0; i < len; i++) {
		error = write(sd, &eid_dest[i], sizeof(char));
		if(error < 0) {
			fprintf(stderr, "Error in writing socket\n");
			exit(EXIT_FAILURE);
		}
	}*/

	len = strlen(file_name);
	error = write(sd, &len, sizeof(int));
	if(error < 0) {
		fprintf(stderr, "Error in writing socket\n");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < len; i++) {
		error = write(sd, &file_name[i], sizeof(char));
		if(error < 0) {
			fprintf(stderr, "Error in writing socket\n");
			exit(EXIT_FAILURE);
		}
	}

	printf("Sending: %s\n", file_name);
	int fd;
	fd = open(file_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error in opening file\n");
		exit(EXIT_FAILURE);
	}

	char char_read;
	while((error = read(fd, &char_read, sizeof(char))) > 0) {
		error = write(sd, &char_read, sizeof(char));
		if(error < 0) {
			fprintf(stderr, "Error in writing socket\n");
			exit(EXIT_FAILURE);
		}
	}

	//Close socket
	shutdown(sd, 1);
	close(sd);

	printf("File sent\n");

	return 0;
}

