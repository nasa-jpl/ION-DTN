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
 * bundle_receiver.c
 * Thread to receive file vai BP protocol,
 * it uses Semaphores (POSIX) to avoid concurrency whith tcp sender side.
 */

#include "proxy_thread.h"
#include "bundle_header_utility.h"
#include "debugger.h"
#include "utility.h"

/* ---------------------------
 *      Global variables
 * --------------------------- */
static al_bp_bundle_object_t bundle;

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
al_bp_error_t read_dtnperf_header(char* filename);
int setTCPdestParameter (al_bp_extension_block_t *metadata,circular_buffer_item * toSend,int debugLeavel);
static void criticalError(void *arg);

/**
 * Thread code
 */
void * bundleReceiving(void * arg) {
	al_bp_extB_error_t utility_error;
	al_bp_error_t error;
	bundle_to_tcp_inf_t * proxy_inf = (bundle_to_tcp_inf_t *) arg;

	al_bp_bundle_payload_location_t location = BP_PAYLOAD_FILE;
	al_bp_endpoint_id_t source_eid;

	char * file_name_payload;
	uint file_name_payload_len;
	char filename[FILE_NAME];
	char temp_filename[256];

	int fd, fdNew;
	char char_read;

	int index = 0;

	//Changing of default exit routine
	pthread_cleanup_push(criticalError, NULL);

	opendir(DTN_TCP_DIR);
	if(errno == ENOENT) {
		debug_print(proxy_inf->debug_level, "[DEBUG] Creating %s\n", DTN_TCP_DIR);
		mkdir(DTN_TCP_DIR, 0700);
	}

	//Start daemon like execution
	while(1==1) {
		//Create bundle
		utility_error = al_bp_bundle_create(&bundle);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_create() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}
		printf("Waiting for a bundle...\n");


		/*Receiving bundle
		while (al_bp_extB_receive(proxy_inf->rd_receive, &bundle, location, -1)!= BP_EXTB_SUCCESS) {
			printf("Registration busy\n");
			break;
		}
		printf("Bundle received\n");*/

		utility_error = al_bp_extB_receive(proxy_inf->rd_receive, &bundle, location, -1);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_get_file_name_payload_file() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			if(utility_error==BP_EXTB_ERRRECEIVE){
				break;
			}
			continue;
		}
		printf("Bundle received\n");

		utility_error = al_bp_bundle_get_payload_file(bundle, &file_name_payload, &file_name_payload_len);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_get_file_name_payload_file() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}

		utility_error = al_bp_bundle_get_source(bundle, &source_eid);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_get_source() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}

		debug_print(proxy_inf->debug_level,
				"[DEBUG] file_name: %s EID_src: %s\n", basename(file_name_payload), source_eid.uri);

		circular_buffer_item toSend;
		//Set tcp ip dest parameter if it's not correct abort reception
		if((setTCPdestParameter(bundle.spec->metadata.metadata_val,&toSend,proxy_inf->debug_level))==0){
			if(proxy_inf->options == 'm') {
				fd = open(file_name_payload, O_RDONLY);
				if (fd < 0) {
					error_print("Opening file %s failed (%s)\n", file_name_payload, strerror(errno));
					al_bp_bundle_free(&bundle);
					continue;
				}

				strcpy(temp_filename, DTN_TCP_DIR);
				strcat(temp_filename, &source_eid.uri[4]);
				strcat(temp_filename, "_");
				strcat(temp_filename, &file_name_payload[5]);
				sprintf(filename, "%s_%d", temp_filename, index);
//				sprintf(filename, "%s_%d", filename, index);

				fdNew = open(filename, O_WRONLY | O_CREAT, 0700);
				if (fd < 0) {
					error_print("Reading file %s failed (%s)\n", file_name_payload, strerror(errno));
					al_bp_bundle_free(&bundle);
					close(fd);
					continue;
				}


				while(read(fd, &char_read, sizeof(char)) > 0) {
					write(fdNew, &char_read, sizeof(char));
				}
				close(fd);
				close(fdNew);


				strcpy(toSend.fileName, filename);
			}
			else { //DTNperf compatibility
				HEADER_TYPE bundle_header;
				if (get_dtnperf_bundle_header_and_options(&bundle, &bundle_header) < 0) {
					error_print("Error in getting bundle header and options\n");
					al_bp_bundle_free(&bundle);
					continue;
				}

				error = read_dtnperf_header(toSend.fileName);

				if(error != BP_SUCCESS) {
					error_print("Error in reading dtnperf header (%s)\n", al_bp_strerror(error));
					al_bp_bundle_free(&bundle);
					continue;
				}
			}
			utility_error = al_bp_bundle_free(&bundle);
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_bundle_free() (%s)\n", al_bp_strerror(utility_error));
				al_bp_bundle_free(&bundle);
				//set_is_running_to_false(proxy_inf->mutex, proxy_inf->is_running);
				//kill(proxy_inf->tid_snd, SIGUSR1);
				break;
			}
			strcpy(filename, "");
			if(circular_buffer_isFull(&(proxy_inf->bp_tcp_buffer))==0) printf("BP->TCP buffer full\n");
			while(circular_buffer_isFull(&(proxy_inf->bp_tcp_buffer))==0) sleep(1);
			pthread_mutex_lock(&(proxy_inf->bp_tcp_buffer.mutex));
			circular_buffer_push(&(proxy_inf->bp_tcp_buffer),toSend);
			pthread_mutex_unlock(&(proxy_inf->bp_tcp_buffer.mutex));
		}else{
			//there is an error in metedata tcp dest
			utility_error = al_bp_bundle_free(&bundle);
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_bundle_free() (%s)\n", al_bp_strerror(utility_error));
				al_bp_bundle_free(&bundle);
				//set_is_running_to_false(proxy_inf->mutex, proxy_inf->is_running);
				//kill(proxy_inf->tid_snd, SIGUSR1);
				break;
			}
			strcpy(filename, "");
		}


	}//while

	//Signaling to main an error in daemon like execution
	proxy_inf->error=utility_error;
	kill(getpid(),SIGINT);
	pthread_cleanup_pop(1);
	return NULL;
}

/**
 * Function for dtnperf compability
 */
al_bp_error_t read_dtnperf_header(char * filename) {
	FILE *pl_stream;
	char *transfer;
	int transfer_len;
	u32_t pl_size;

	//Get info about bundle size
	al_bp_bundle_get_payload_size(bundle, &pl_size);

	if (dtnperf_open_payload_stream_read(bundle, &pl_stream) < 0){
		error_print("Error in opening file transfer bundle (%s)\n", strerror(errno));
		return BP_ERRBASE;
	}

	transfer_len = HEADER_SIZE + BUNDLE_OPT_SIZE+sizeof(al_bp_timeval_t);
	transfer = (char*) malloc(transfer_len);
	memset(transfer, 0, transfer_len);

	if (fread(transfer, transfer_len, 1, pl_stream) != 1 && ferror(pl_stream)!=0){
		error_print("Error in processing file transfer bundle (%s)\n", strerror(errno));
		return BP_ERRBASE;
	}
	free(transfer);

	fseek(pl_stream, BUNDLE_CRC_SIZE, SEEK_CUR);

	transfer_len = pl_size-transfer_len-BUNDLE_CRC_SIZE;
	transfer = (char*) malloc(transfer_len);
	memset(transfer, 0, transfer_len);

	if (fread(transfer, transfer_len, 1, pl_stream) != 1 && ferror(pl_stream)!=0){
		error_print("Error in processing file transfer bundle (%s)\n", strerror(errno));
		return BP_ERRBASE;
	}

	free(transfer);
	dtnperf_close_payload_stream_read(pl_stream);

	dtnperf_process_incoming_file(filename, &bundle);

	return BP_SUCCESS;
}

/**
 * Funciton:
 * -) uses metedata to set ip and port of the tcp destination
 * -) tests ip tcp dest format
 * -) parses address
 */
 //da sistemare i valori di ritorno con macro
int setTCPdestParameter (al_bp_extension_block_t *metadata,circular_buffer_item * toSend,int debugLeavel){
	debug_print(debugLeavel,"[DEBUG] Metadata size: %llu\n",metadata->data.data_len);
	debug_print(debugLeavel,"[DEBUG] Metadata val: %s\n",metadata->data.data_val);
	//printf("Metadata size: %i\n",metadata->data.data_len);
	//printf("Metadata val: %s\n",metadata->data.data_val);
	strcpy(toSend->ip_dest,metadata->data.data_val);

	if(strstr(toSend->ip_dest, ":") == NULL){//test if ip_dest contains ip and port of server
		error_print("Error use %s is an invalid IPv4 address format (use ip:port)\n",toSend->ip_dest);
		return WRONG_TCP_IP_ADDRESS;
	}
	int nbytes=numberOfChar('.',toSend->ip_dest);
	printf("Numero di bytes: %i\n",nbytes);
	if(nbytes<4){ //check if ip tcp dest has 4 bytes and no more or less
	   error_print("Error use %s is an invalid IPv4 address format (too short)\n",toSend->ip_dest);
	   return WRONG_TCP_IP_ADDRESS;
	}
	if(nbytes>4){ //check if ip tcp dest has 4 bytes and no more or less
		error_print("Error use %s is an invalid IPv4 address format (too long)\n",toSend->ip_dest);
		return WRONG_TCP_IP_ADDRESS;
	}
	char* token = strtok(toSend->ip_dest, ":");
	strcpy(toSend->ip_dest,token);
	token = strtok(NULL, ":");
	toSend->port_dest=atoi(token);
	if(toSend->port_dest==0 || (toSend->port_dest)>65535){//test dest port
		error_print("Error use %s is an invalid TCP port\n", token);
		return WRONG_TCP_IP_ADDRESS;
	}
	//check if all bytes of ip address is valid
	char temp[20];
	strcpy(temp,toSend->ip_dest);
	char *token2 = strtok(temp, ".");
	int ipIndex=0;
	while (token2 != NULL || ipIndex>4) {
	   int byte=atoi(token2);
	   if(byte>255){//test every bytes of dest ip
		   error_print("Error use %s is an invalid IPv4 address format (in byte %i)\n",toSend->ip_dest, ipIndex);
		   return WRONG_TCP_IP_ADDRESS;
	   }
	   token2 = strtok(NULL, ".");
	   ipIndex++;
	}
	debug_print(debugLeavel,"[DEBUG] ip sintax checked: %s\n",toSend->ip_dest);
	debug_print(debugLeavel,"[DEBUG] port checked: %lu\n",toSend->port_dest);
	return 0;
}

/**
 * Custom routine started in case of reception of pthread_cancel by parent.
 */
static void criticalError(void *arg){
	//al_bp_bundle_free(&bundle);
}

