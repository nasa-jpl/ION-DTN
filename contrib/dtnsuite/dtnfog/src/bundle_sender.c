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

/*
 * bundle_sender.c
 *
 * This thread sends a bundle
 * to a DTN application
 *
 */

#include "proxy_thread.h"
#include "bundle_header_utility.h"
#include "debugger.h"
#include "utility.h"

/* ---------------------------
 *      Global variables
 * --------------------------- */
static char source_file[256];
static char * transfer_file_name;			//Basename of the file to transfer
static u32_t file_total_length;				//Size of the file to transfer
static int transfer_fd;

static al_bp_bundle_object_t bundle;
static al_bp_timeval_t bundle_expiration;

/* -------------------------------
 *       Function interfaces
 * ------------------------------- */
al_bp_error_t prepare_dtnperf_header();
static void criticalError(void *arg);
//void handlerBundleSender(int signal);

/**
 * Thread code
 */
void * bundleSending(void * arg) {
	al_bp_extB_error_t utility_error;
	al_bp_endpoint_id_t to;
	al_bp_endpoint_id_t reply_to;
	bp_sender_info_t * proxy_inf = (bp_sender_info_t *) arg;

	al_bp_bundle_payload_location_t location = BP_PAYLOAD_FILE;
	al_bp_bundle_priority_t bundle_priority;

	char file_name[FILE_NAME];
	int error;

	//Changing of default exit routine
	pthread_cleanup_push(criticalError, NULL);

	//Start daemon like execution
	while(1==1) {
		//sem_wait(&proxy_inf->bundleSnd);
		circular_buffer_item toSend=circular_buffer_pop(&(proxy_inf->bp_tosend_buffer));

		memset((char *)&to.uri, 0, strlen(to.uri));
		memset((char *)&reply_to.uri, 0, strlen(reply_to.uri));

		//Create bundle
		bundle_priority.priority = BP_PRIORITY_NORMAL;
		bundle_priority.ordinal = 0;
		bundle_expiration = (al_bp_timeval_t) BUNDLE_EXPIRATION;

		utility_error = al_bp_bundle_create(&bundle);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_create() (%s)\n", al_bp_strerror(utility_error));
			continue;
		}

		utility_error = al_bp_bundle_set_payload_location(&bundle, location);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_set_payload_location() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}

		strcpy(file_name, toSend.fileName);

		//Check if DTNfog is in --no-header mode
		if(proxy_inf->options == 'd') {
			struct stat file;
			if (stat(file_name, &file) < 0) {
				error_print("Error in stat of file: %s (%s)\n", file_name, strerror(errno));
				al_bp_bundle_free(&bundle);
				continue;
			}

			file_total_length = file.st_size;

			transfer_file_name = malloc(strlen(file_name) + 1);
			strcpy(transfer_file_name, basename(file_name));

			if ((transfer_fd = open(file_name, O_RDONLY)) < 0)
			{
				error_print("Error in opening file: %s (%s)", file_name, strerror(errno));
				al_bp_bundle_free(&bundle);
				continue;
			}

			//Init dtnperf options
			dtnperf_options_t dtnperf_opt;
			init_dtnperf_options(&dtnperf_opt);

			dtnperf_connection_options_t conn_opt;
			init_dtnperf_connection_options(&conn_opt);

			//Prepare bundle for DTNperf --server compatibility
			al_bp_error_t error = prepare_dtnperf_header(&dtnperf_opt, &conn_opt);
			if(error != BP_SUCCESS) {
				error_print("Error in prepare_dtnperf_header() (%s)\n", al_bp_strerror(utility_error));
				al_bp_bundle_free(&bundle);
				close(transfer_fd);
				continue;
			}

			close(transfer_fd);
		}

		utility_error = al_bp_bundle_set_priority(&bundle, bundle_priority);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_set_priority() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}
		bundle.spec->priority.ordinal = 0;
		bundle.spec->critical = FALSE;
		bundle.spec->flow_label = 0;
		bundle.spec->unreliable = FALSE;

		utility_error = al_bp_bundle_set_expiration(&bundle, bundle_expiration);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_set_expiration (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}

		utility_error = al_bp_bundle_set_delivery_opts(&bundle, (al_bp_bundle_delivery_opts_t) BP_DOPTS_NONE);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_set_delivery_opts() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			continue;
		}

		if(proxy_inf->options == 'n') {
			utility_error = al_bp_bundle_set_payload_file(&bundle, file_name, strlen(file_name));
			if (utility_error != BP_EXTB_SUCCESS) {
				error_print("Error in al_bp_bundle_set_payload_file() (%s)\n", al_bp_strerror(utility_error));
				al_bp_bundle_free(&bundle);
				continue;
			}
		}

		printf("Sending bundle with file %s\n", basename(file_name));


		al_bp_parse_eid_string(&to, toSend.eid);
		al_bp_parse_eid_string(&reply_to, "ipn:5.0");
//		al_bp_get_none_endpoint(&reply_to);
		debug_print(proxy_inf->debug_level, "filename: %s\n", bundle.payload->filename.filename_val);
		utility_error = al_bp_extB_send(proxy_inf->rd_send, bundle, to, reply_to);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_extB_send_bundle() (%s)\n", al_bp_strerror(utility_error));
			al_bp_bundle_free(&bundle);
			free(transfer_file_name);
			break;
		}
		debug_print(proxy_inf->debug_level, "[DEBUG] al_bp_extB_send_bundle: %s\n", al_bp_strerror(utility_error));

		utility_error = al_bp_bundle_free(&bundle);
		if (utility_error != BP_EXTB_SUCCESS) {
			error_print("Error in al_bp_bundle_free() (%s)\n", al_bp_strerror(utility_error));
			free(transfer_file_name);
			break;
		}

		error = remove(file_name);
		if(error < 0) {
			error_print("Error in removing file %s (%s)\n", file_name, strerror(errno));
			continue;
		}
		//sem_post(&proxy_inf->tcpRecv);
		strcpy(file_name, "");

		free(transfer_file_name);
	}//for

	kill(getpid(),SIGINT);
	pthread_cleanup_pop(1);
	return NULL;
}

/**
 * Function for writing DTNperf --client -F header
 * into bundle
 */
al_bp_error_t prepare_dtnperf_header(dtnperf_options_t * opt, dtnperf_connection_options_t * conn_opt) {
	FILE * stream, *buf_stream;
	char *buf;
	boolean_t eof_reached;
	uint32_t crc;
	int bytes_written;
	al_bp_error_t error;

	sprintf(source_file, "%s_%d_%d", SOURCE_FILE, getpid(), 1);

	stream = fopen(source_file, "wb");
	if (stream == NULL)
	{
		error_print("Error in creating file %s (%s)\n", source_file, strerror(errno));
		return BP_EOPEN;
	}

	fclose(stream);

	//Fill the payload
	error = al_bp_bundle_set_payload_file(&bundle, source_file, strlen(source_file));

	if (error != BP_SUCCESS)
	{
		error_print("Error in setting bundle payload (%s)\n", strerror(errno));
		return BP_ERRBASE;
	}

	//Open payload stream in write mode
	if (dtnperf_open_payload_stream_write(bundle, &stream) < 0)
	{
		error_print("Error in opening payload stream write mode (%s)\n", strerror(errno));
		return BP_EOPEN;
	}

	al_bp_endpoint_id_t monitor_eid;
	al_bp_get_none_endpoint(&monitor_eid);
	uint16_t monitor_eid_len = strlen(monitor_eid.uri);
	uint16_t filename_len = strlen(transfer_file_name);

	u32_t header_file_size = get_dtnperf_header_size(filename_len, monitor_eid_len);

	opt->bundle_payload = file_total_length + header_file_size;
	buf = (char *) malloc(opt->bundle_payload);
	double temp_bundle_payload = opt->bundle_payload;
	buf_stream = open_memstream(&buf, (size_t *) &temp_bundle_payload);//Prepare the payload
	error = prepare_dtnperf_file_transfer_payload(opt, buf_stream, transfer_fd,
			transfer_file_name, file_total_length, conn_opt->expiration , &eof_reached, &crc, &bytes_written,
			opt->bundle_payload);

	if(error != BP_SUCCESS)
	{
		error_print("Error in preparing file transfer payload (%s)\n", strerror(errno));
		return BP_ERRBASE;
	}

	fclose(buf_stream);

	memcpy(buf+HEADER_SIZE+BUNDLE_OPT_SIZE+sizeof(al_bp_timeval_t), &crc, BUNDLE_CRC_SIZE);

	fwrite(buf, bytes_written, 1, stream);

	//Close the stream
	dtnperf_close_payload_stream_write(&bundle, stream);

	free(buf);

	return BP_SUCCESS;
}

/**
 * Custom routine started in case of reception of pthread_cancel by parent.
 */
static void criticalError(void *arg){}
