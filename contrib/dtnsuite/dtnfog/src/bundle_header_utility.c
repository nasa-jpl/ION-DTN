/********************************************************
    Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
    Carlo Caini (DTNfog project supervisor), carlo.caini@unibo.it

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

 ********************************************************/

/*
 * DTNfog_header_utility.c
 *
 * Functions for DTNperf compability
 *
 */

#include <dirent.h>
#include "bundle_header_utility.h"
#include "debugger.h"

/**
 * Static variables for stream operations
 */
static char * buffer = NULL;
static u32_t buffer_len = 0;

/**
 * Init options operations
 */
void init_dtnperf_options(dtnperf_options_t *opt)
{
	opt->bp_implementation = al_bp_get_implementation();
	opt->debug = DEBUG_OFF;
	opt->use_ip = FALSE;
	opt->ip_addr = "127.0.0.1";
	opt->ip_port = 5010;
	opt->eid_format_forced = 'N';
	opt->ipn_local_num = 0;
	opt->daemon = FALSE;
	opt->server_output_file = SERVER_OUTPUT_FILE;
	opt->monitor_output_file = MONITOR_OUTPUT_FILE;
	memset(opt->dest_eid, 0, AL_BP_MAX_ENDPOINT_ID);
	memset(opt->mon_eid, 0, AL_BP_MAX_ENDPOINT_ID);
	opt->op_mode = 'D';
	opt->data_qty = 0;
	opt->D_arg = NULL;
	opt->F_arg = NULL;
	opt->P_arg = NULL;
	opt->use_file = 1;
	opt->data_unit = 'M';
	opt->transmission_time = 0;
	opt->congestion_ctrl = 'W';
	opt->window = 1;
	opt->rate_arg = NULL;
	opt->rate = 0;
	opt->rate_unit = 'b';
	opt->wait_before_exit = 0;
	opt->bundle_payload = DEFAULT_PAYLOAD;
	opt->payload_type = BP_PAYLOAD_FILE;
	opt->dest_dir = BUNDLE_DIR_DEFAULT;
	opt->file_dir = FILE_DIR_DEFAULT;
	opt->create_log = FALSE;
	opt->log_filename = LOG_FILENAME;
	opt->no_bundle_stop = FALSE;
	opt->acks_to_mon = FALSE;
	opt->no_acks = TRUE;
	opt->logs_dir = LOGS_DIR_DEFAULT;
	opt->bundle_ack_options.ack_to_client = FALSE;
	opt->bundle_ack_options.ack_to_mon = ATM_FORCE_NO;
	opt->bundle_ack_options.set_ack_expiration = FALSE;
	opt->bundle_ack_options.set_ack_priority = FALSE;
	opt->expiration_session = 120;
	opt->oneCSVonly = FALSE;
	opt->rtPrint = FALSE;
	opt->rtPrintFile = stdout;
	opt->uniqueCSVfilename = MONITOR_UNIQUE_CSV_FILENAME;
	opt->num_blocks = 0;
	opt->crc = FALSE;
}

void init_dtnperf_connection_options(dtnperf_connection_options_t* opt)
{
	opt->expiration = BUNDLE_EXPIRATION;				// expiration time (sec)
	opt->delivery_receipts = TRUE;		// request delivery receipts [1]
	opt->forwarding_receipts = FALSE;   // request per hop departure [0]
	opt->custody_transfer = FALSE;   	// request custody transfer [0]
	opt->custody_receipts = FALSE;   	// request per custodian receipts [0]
	opt->receive_receipts = FALSE;   	// request per hop arrival receipt [0]
	opt->deleted_receipts = FALSE;		// request per deleted bndl receipt [0]
	opt->wait_for_report = TRUE;   		// wait for bundle status reports [1]
	opt->disable_fragmentation = FALSE; //disable bundle fragmentation[0]
	opt->priority.priority = BP_PRIORITY_NORMAL; // bundle priority [BP_PRIORITY_NORMAL]
	opt->priority.ordinal = 0;
	opt->unreliable = FALSE;
	opt->critical = FALSE;
	opt->flow_label = 0;
}

/**
 * Stream operations
 */
int dtnperf_open_payload_stream_write(al_bp_bundle_object_t bundle, FILE ** f) {
	al_bp_bundle_payload_location_t pl_location;

	al_bp_bundle_get_payload_location(bundle, &pl_location);


	al_bp_bundle_get_payload_file(bundle, &buffer, &buffer_len);
	*f = fopen(buffer, "wb");

	if (*f == NULL)
		return -1;

	return 0;
}

int dtnperf_open_payload_stream_read(al_bp_bundle_object_t bundle, FILE ** f) {
	al_bp_bundle_payload_location_t pl_location;
	char * buffer;
	u32_t buffer_len;

	al_bp_bundle_get_payload_location(bundle, &pl_location);

	al_bp_bundle_get_payload_file(bundle, &buffer, &buffer_len);
	*f = fopen(buffer, "rb");

	if (*f == NULL)	{
		return -1;
	}

	return 0;
}

int dtnperf_close_payload_stream_write(al_bp_bundle_object_t * bundle, FILE *f) {
	al_bp_bundle_payload_location_t pl_location;
	al_bp_bundle_get_payload_location(*bundle, &pl_location);

	fclose(f);
	al_bp_bundle_set_payload_file(bundle, buffer, buffer_len);

	return 0;
}

int dtnperf_close_payload_stream_read(FILE * f)
{
	return fclose(f);
}

/**
 *  Functions for compatibility with DTNperf --server
 */
u32_t get_dtnperf_header_size(uint16_t filename_len, uint16_t monitor_eid_len) {
	u32_t result = 0;
	//Header Type,  congenstion char,  ack lifetime, crc, monitor eid, monitor eid length
	result = HEADER_SIZE + BUNDLE_OPT_SIZE + sizeof(al_bp_timeval_t) + BUNDLE_CRC_SIZE + sizeof(monitor_eid_len) + monitor_eid_len;
	//Bundle lifetime, filename, filename len, dim file, offset
	result += sizeof(al_bp_timeval_t) + filename_len + sizeof(filename_len) + sizeof(uint32_t) + sizeof(uint32_t);

	return result;
}

u32_t get_dtnperf_file_fragment_size(u32_t payload_size, uint16_t filename_len, uint16_t monitor_eid_len) {
	u32_t result;
	result = payload_size - get_dtnperf_header_size(filename_len, monitor_eid_len);
	return result;
}

al_bp_error_t prepare_dtnperf_payload_header_and_ack_options(dtnperf_options_t * opt,
		al_bp_endpoint_id_t mon_eid, FILE * f, uint32_t *crc, int *bytes_written) {
	if (f == NULL)
		return BP_ENULLPNTR;

	HEADER_TYPE header;
	BUNDLE_OPT_TYPE options;
	uint16_t eid_len;
	uint32_t tmp_crc = 0;
	al_bp_timeval_t ack_timeval = (al_bp_timeval_t) 0;

	header = FILE_HEADER;
	//Options
	// options
	options = 0;
	// ack to client
	if (opt->bundle_ack_options.ack_to_client)
		options |= BO_ACK_CLIENT_YES;
	else
		options |= BO_ACK_CLIENT_NO;

	//ack to monitor
	if (opt->bundle_ack_options.ack_to_mon == ATM_NORMAL)
		options |= BO_ACK_MON_NORMAL;
	else if (opt->bundle_ack_options.ack_to_mon == ATM_FORCE_YES)
		options |= BO_ACK_MON_FORCE_YES;
	else if (opt->bundle_ack_options.ack_to_mon == ATM_FORCE_NO)
		options |= BO_ACK_MON_FORCE_NO;
	// bundle ack expiration time
	if (opt->bundle_ack_options.set_ack_expiration)
		options |= BO_SET_EXPIRATION;
	// bundle ack priority
	if (opt->bundle_ack_options.set_ack_priority)
	{
		options |= BO_SET_PRIORITY;
		switch (opt->bundle_ack_options.ack_priority.priority)
		{
		case BP_PRIORITY_BULK:
			options |= BO_PRIORITY_BULK;
			break;
		case BP_PRIORITY_NORMAL:
			options |= BO_PRIORITY_NORMAL;
			break;
		case BP_PRIORITY_EXPEDITED:
			options |= BO_PRIORITY_EXPEDITED;
			break;
		case BP_PRIORITY_RESERVED:
			options |= BO_PRIORITY_RESERVED;
			break;
		default:
			break;
		}
	}

	//Write in payload
	fwrite(&header, HEADER_SIZE, 1, f);
	*bytes_written+=HEADER_SIZE;

	fwrite(&options, BUNDLE_OPT_SIZE, 1, f);
	*bytes_written+=BUNDLE_OPT_SIZE;

	//Write lifetime of ack
	fwrite(&ack_timeval, sizeof(al_bp_timeval_t), 1, f);
	*bytes_written+=sizeof(al_bp_timeval_t);

	//Write CRC
	fwrite(&tmp_crc, BUNDLE_CRC_SIZE, 1, f);
	*bytes_written+=BUNDLE_CRC_SIZE;

	//Write reply-to eid
	eid_len = strlen(mon_eid.uri);
	fwrite(&eid_len, sizeof(eid_len), 1, f);
	*bytes_written+=sizeof(eid_len);

	fwrite(mon_eid.uri, eid_len, 1, f);
	*bytes_written+=eid_len;

	return BP_SUCCESS;
}

al_bp_error_t prepare_dtnperf_file_transfer_payload(dtnperf_options_t * opt, FILE * f, int fd, char * filename,
		uint32_t file_dim, al_bp_timeval_t expiration_time, boolean_t * eof, uint32_t *crc, int *bytes_written, double bundle_payload) {
	if (f == NULL)
		return BP_ENULLPNTR;

	al_bp_error_t result;
	uint32_t fragment_len;
	char * fragment;
	uint32_t offset;
	long bytes_read;
	uint16_t filename_len = strlen(filename);
	al_bp_endpoint_id_t monitor_eid;
	al_bp_get_none_endpoint(&monitor_eid);
	uint16_t monitor_eid_len = strlen(monitor_eid.uri);

	//Get size of fragment and allocate fragment
	fragment_len = get_dtnperf_file_fragment_size(opt->bundle_payload, filename_len, monitor_eid_len);
	fragment = (char *) malloc(sizeof(char) * fragment_len);
	memset(fragment, 0, fragment_len);
	//Get offset of fragment
	offset = 0; //lseek(fd, 0, SEEK_CUR);

	//Read fragment from file
	bytes_read = read(fd, fragment, fragment_len);
	if (bytes_read < fragment_len)// reached EOF
	{
		*eof = TRUE;
		fragment[bytes_read] = EOF;
	}
	else
		*eof = FALSE;

	//Reset CRC
	*crc= 0;
	*bytes_written=0;

	//Prepare header and congestion control
	result = prepare_dtnperf_payload_header_and_ack_options(opt, monitor_eid, f, crc, bytes_written);

	//Write expiration time
	fwrite(&expiration_time, sizeof(expiration_time), 1, f);
	*bytes_written+=sizeof(expiration_time);
	//Write filename length
	fwrite(&filename_len, sizeof(filename_len), 1, f);
	*bytes_written+=sizeof(filename_len);
	//Write filename
	fwrite(filename, filename_len, 1, f);
	*bytes_written+=filename_len;
	//Write file size
	fwrite(&file_dim, sizeof(file_dim), 1, f);
	*bytes_written+=sizeof(file_dim);
	//Write offset in the bundle
	fwrite(&offset, sizeof(offset), 1, f);
	*bytes_written+=sizeof(offset);
	//Write fragment in the bundle
	fwrite(fragment, fragment_len, 1, f);
	*bytes_written+=fragment_len;

	free(fragment);

	return result;
}

/**
 *  Functions for DTNperf --client compatibility
 */
int get_dtnperf_bundle_header_and_options(al_bp_bundle_object_t * bundle, HEADER_TYPE * header) {
	if (bundle == NULL)
		return -1;

	FILE * pl_stream = NULL;
	dtnperf_open_payload_stream_read(*bundle, &pl_stream);

	if (header != NULL) {
		//Read header
		if(fread(header, HEADER_SIZE, 1, pl_stream) != 1)
		{ return  BP_EINVAL;}
	}
	else {
		//Skip header
		fseek(pl_stream, HEADER_SIZE, SEEK_SET);
	}

	//Skip option
	fseek(pl_stream, BUNDLE_OPT_SIZE, SEEK_SET);

	dtnperf_close_payload_stream_read(pl_stream);

	return 0;
}

int dtnperf_assemble_file(char * filename_to_transfer, int filename_len, char * filename, FILE * pl_stream,
		u32_t pl_size, u32_t timestamp_secs, u32_t expiration, uint16_t monitor_eid_len, uint32_t *crc) {
	char * transfer;
	u32_t transfer_len;
	int fd;
	uint32_t offset;
	//uint32_t local_crc=0;

	// transfer length is total payload length without header,
	// congestion control char and file fragment offset
	transfer_len = get_dtnperf_file_fragment_size(pl_size, filename_len, monitor_eid_len);
	// read file fragment offset
	if(fread(&offset, sizeof(offset), 1, pl_stream) != 1)
		return -1;
	// read remaining file fragment
	transfer = (char*) malloc(transfer_len);
	memset(transfer, 0, transfer_len);
	if (fread(transfer, transfer_len, 1, pl_stream) != 1)
		return -1;

	// calculate CRC
	//if (crc!=NULL)
	//{
	//	local_crc = calc_crc32_d8(local_crc, (uint8_t*) transfer, transfer_len);
	//	if (local_crc!=*crc)
	//		return -2;
	//}

	// open or create destination file
	char * full_dir = (char*) malloc(filename_len + strlen(CLC_DIR) +1);
	memset(full_dir, 0, sizeof(char) * strlen(full_dir));
	strcpy(full_dir, CLC_DIR);
	strcat(full_dir, filename_to_transfer);
	free(filename_to_transfer);
	fd = open(full_dir, O_WRONLY | O_TRUNC | O_CREAT, 0700);
	if (fd < 0)
	{
		return -1;
	}
	// write fragment
	lseek(fd, offset, SEEK_SET);
	if (write(fd, transfer, transfer_len) < 0)
		return -1;
	close(fd);
	free(transfer);

	strcpy(filename, full_dir);

	// deallocate resources
	free(full_dir);

	return 1;
}

int dtnperf_process_incoming_file(char * filename, al_bp_bundle_object_t * bundle)
{
	al_bp_endpoint_id_t client_eid;
	al_bp_timestamp_t timestamp;
	al_bp_timeval_t expiration;
	FILE * pl_stream;
	int result;
	uint16_t filename_len;
	uint32_t file_dim;
	char * filename_to_transfer;
	u32_t pl_size;
	uint32_t * crc = 0;

	// get info from bundle
	al_bp_bundle_get_source(*bundle, &client_eid);
	al_bp_bundle_get_creation_timestamp(*bundle, &timestamp);
	//	if(al_bp_get_implementation() != BP_ION)
	//		al_bp_bundle_get_expiration(*bundle, &expiration);
	al_bp_bundle_get_payload_size(*bundle, &pl_size);

	// create stream from incoming bundle payload
	if (dtnperf_open_payload_stream_read(*bundle, &pl_stream) < 0)
		return -1;

	// skip header - congestion control char - lifetime ack
	fseek(pl_stream, HEADER_SIZE + BUNDLE_OPT_SIZE + sizeof(al_bp_timeval_t) + BUNDLE_CRC_SIZE, SEEK_SET);
	// skip monitor eid
	uint16_t monitor_eid_len;
	char monitor_eid[256];
	if(fread(&monitor_eid_len, sizeof(monitor_eid_len), 1, pl_stream) != 1){return -1;}
	if(fread(monitor_eid, monitor_eid_len, 1, pl_stream) != 1){return -1;}

	// get expiration time
	result = fread(&expiration, sizeof(expiration), 1, pl_stream);
	if (result < 1)
		fprintf(stderr, "Error reading expiration time (%s)\n", strerror(errno));

	// get filename len
	result = fread(&filename_len, sizeof(filename_len), 1, pl_stream);
	// get filename
	filename_to_transfer = (char *) malloc(filename_len + 1);
	memset(filename_to_transfer, 0, filename_len + 1);
	result = fread(filename_to_transfer, filename_len, 1, pl_stream);
	if(result < 1)
		fprintf(stderr, "Error reading filename (%s)\n", strerror(errno));
	filename_to_transfer[filename_len] = '\0';
	//get file size
	if(fread(&file_dim, sizeof(file_dim), 1, pl_stream)!=1){return -1;}

	// assemble file
	result = dtnperf_assemble_file(filename_to_transfer, filename_len, filename, pl_stream, pl_size, timestamp.secs, expiration, monitor_eid_len, crc);
	dtnperf_close_payload_stream_read(pl_stream);

	if (result < 0)// error
		return result;
	if (result == 1) // transfer completed
	{
		printf("Successfully transfered file: %s\n", filename);
		return 1;
	}

	return 0;
}
