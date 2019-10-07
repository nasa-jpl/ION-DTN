/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * file_transfer_tools.h
 */

#ifndef FILE_TRANSFER_TOOLS_H_
#define FILE_TRANSFER_TOOLS_H_

#include "dtnperf_types.h"


typedef struct file_transfer_info
{
	al_bp_endpoint_id_t client_eid;
	int filename_len;
	char * filename;
	char * full_dir;
	u32_t file_dim;
	u32_t bytes_recvd;
	u32_t first_bundle_time; //timestamp secs
	u32_t last_bundle_time; //timestamp secs
	u32_t expiration; //secs
}
file_transfer_info_t;

typedef struct file_transfer_info_list_item
{
	file_transfer_info_t * info;
	struct file_transfer_info_list_item * previous;
	struct file_transfer_info_list_item * next;
} file_transfer_info_list_item_t;

typedef struct file_transfer_info_list
{
	file_transfer_info_list_item_t * first;
	file_transfer_info_list_item_t * last;
	int count;
} file_transfer_info_list_t;

file_transfer_info_list_t file_transfer_info_list_create();
void file_transfer_info_list_destroy(file_transfer_info_list_t * list);

file_transfer_info_t * file_transfer_info_create(al_bp_endpoint_id_t client_eid,
		int filename_len,
		char * filename,
		char * full_dir,
		u32_t file_dim,
		u32_t bundle_time,
		u32_t expiration);

void file_transfer_info_destroy(file_transfer_info_t * info);

void file_transfer_info_put(file_transfer_info_list_t * list, file_transfer_info_t * info);

file_transfer_info_list_item_t *  file_transfer_info_get_list_item(file_transfer_info_list_t * list, al_bp_endpoint_id_t client);

file_transfer_info_t *  file_transfer_info_get(file_transfer_info_list_t * list, al_bp_endpoint_id_t client);

void file_transfer_info_del(file_transfer_info_list_t * list, al_bp_endpoint_id_t client);

void file_transfer_info_list_item_delete(file_transfer_info_list_t * list, file_transfer_info_list_item_t * item);

/*
 * assemble_file() writes the file fragment contained in bundle to the file
 * indicated by info. Returns -1 if an error occurs, 0 if the fragment is written
 * succesfully, 1 if the written fragment is the last fragment of the file.
 */
int assemble_file(file_transfer_info_t * info, FILE * pl_stream,
		u32_t pl_size, u32_t timestamp_secs, u32_t expiration, uint16_t monitor_eid_len, uint32_t *crc);

int process_incoming_file_transfer_bundle(file_transfer_info_list_t *info_list,
		al_bp_bundle_object_t * bundle,
		char * dir, uint32_t *crc);

u32_t get_file_fragment_size(u32_t payload_size, uint16_t filename_len, uint16_t monitor_eid_len);

al_bp_error_t prepare_file_transfer_payload(dtnperf_options_t *opt, FILE * f, int fd,
		char * filename, uint32_t file_dim, al_bp_timeval_t expiration_time, boolean_t * eof, uint32_t *crc, int *bytes_written);




#endif /* FILE_TRANSFER_TOOLS_H_ */
