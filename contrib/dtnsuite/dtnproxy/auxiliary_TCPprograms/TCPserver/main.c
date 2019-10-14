/********************************************************
    TCPserver Copyright (c) 2018, Alma Mater Studiorum, University of Bologna
    This is a simple TCP server designed to be used with DTNproxy

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
#include <dirent.h>
#include <sys/stat.h>

#define FILE_NAME 256
#define WORD_LENGTH 10
#define PORT 3000
#define DIR "/root/tcp_files/"

int main(int argc, char * argv[]) {
	int listenSocket, connectClientSocket;
	int error;
	struct sockaddr_in cliaddr, servaddr;
	unsigned int len = sizeof(cliaddr);
	const int on = 1;

	int fd;
	char char_read;
	char file_name[FILE_NAME];
	char * temp_file_name;
	int file_name_len;

	opendir(DIR);
	if(errno == ENOENT) {
		printf("Creating %s\n", DIR);
		mkdir(DIR, 0700);
	}		

	//Init server address
	memset((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocket < 0) {
		fprintf(stderr, "Error in socket() (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		fprintf(stderr, "Error in setting socket options (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(bind(listenSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		fprintf(stderr, "Error in bind() (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(listen(listenSocket, 5) < 0) {
		fprintf(stderr, "Error in listen() (%s)\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	printf("Server starts correctly\n");
	for(;;) {

		printf("Waiting for TCP connection...\n");

		//Accept a TCP client
		if((connectClientSocket = accept(listenSocket, (struct sockaddr *)&cliaddr, &len)) < 0) {
			if(errno == EINTR) {
				continue;
			}
			else {
				fprintf(stderr, "Error in accept(): %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}

		shutdown(connectClientSocket, 1);

		printf("Connection accepted\n");
		printf("Waiting for data...\n");

		//Read the filename
		error = read(connectClientSocket, &file_name_len, sizeof(int));
		if(error < 0) {
			fprintf(stderr, "Reading filename length failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		temp_file_name = (char *) malloc(sizeof(char) * file_name_len);
		int i;
		for(i = 0; i < file_name_len; i++) {
			error = read(connectClientSocket, &temp_file_name[i], sizeof(char));
			if(error < 0) {
				fprintf(stderr, "Reading filename failed: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		temp_file_name[i] = '\0';

		//Create filename
		strcat(file_name, DIR);
		strcat(file_name, temp_file_name);


		fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, 0700);
		if(fd < 0) {
			fprintf(stderr, "Opening file %s failed: %s\n", file_name, strerror(errno));
			exit(EXIT_FAILURE);
		}

		while((error = read(connectClientSocket, &char_read, sizeof(char))) > 0) {
			error = write(fd, &char_read, sizeof(char));
			if(error < 0) {
				fprintf(stderr, "Writing the file %s failed: %s\n", file_name, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		close(fd);
		
		printf("Saved file %s\n", file_name);

		strcpy(file_name, "");

		shutdown(connectClientSocket, 0);
		close(connectClientSocket);

	} //for

}//main

