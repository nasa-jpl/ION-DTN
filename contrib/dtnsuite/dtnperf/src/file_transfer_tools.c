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
 * file_transfer_tools.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <al_bp_api.h>
#include "file_transfer_tools.h"
#include "bundle_tools.h"
#include "utils.h"

file_transfer_info_list_t file_transfer_info_list_create()
{
	file_transfer_info_list_t * list;
	list = (file_transfer_info_list_t *) malloc(sizeof(file_transfer_info_list_t));
	list->first = NULL;
	list->last = NULL;
	list->count = 0;
	return * list;
}

void file_transfer_info_list_destroy(file_transfer_info_list_t * list)
{
	free(list);
}

file_transfer_info_t * file_transfer_info_create(al_bp_endpoint_id_t client_eid,
		int filename_len,
		char * filename,
		char * full_dir,
		u32_t file_dim,
		u32_t bundle_time,
		u32_t expiration)
{
	file_transfer_info_t * info;
	info = (file_transfer_info_t *) malloc(sizeof(file_transfer_info_t));
	al_bp_copy_eid(&(info->client_eid), &client_eid);
	info->filename_len = filename_len;
	info->full_dir = (char*) malloc(strlen(full_dir) + 1);
	memcpy(info->full_dir, full_dir, strlen(full_dir) + 1);
 	info->filename = (char*) malloc(filename_len + 1);
	strncpy(info->filename, filename, filename_len +1);
	info->file_dim = file_dim;
	info->bytes_recvd = 0;
	info->first_bundle_time = bundle_time;
	info->last_bundle_time = bundle_time;
	info->expiration = expiration;
	return info;
}

void file_transfer_info_destroy(file_transfer_info_t * info)
{
	free(info->filename);
	free(info->full_dir);
	free(info);
}

void file_transfer_info_put(file_transfer_info_list_t * list, file_transfer_info_t * info)
{
	file_transfer_info_list_item_t * new;
	new = (file_transfer_info_list_item_t *) malloc(sizeof(file_transfer_info_list_item_t));
	new->info = info;
	new->next = NULL;
	if (list->first == NULL) // empty list
	{
		new->previous = NULL;
		list->first = new;
		list->last = new;
	}
	else
	{
		new->previous = list->last;
		list->last->next = new;
		list->last = new;
	}
	list->count ++;
}

file_transfer_info_list_item_t *  file_transfer_info_get_list_item(file_transfer_info_list_t * list, al_bp_endpoint_id_t client)
{
	file_transfer_info_list_item_t * item = list->first;
	while (item != NULL)
	{
		if (strcmp(item->info->client_eid.uri, client.uri) == 0)
		{
			return item;
		}
		item = item->next;
	}
	return NULL;
}

file_transfer_info_t *  file_transfer_info_get(file_transfer_info_list_t * list, al_bp_endpoint_id_t client)
{
	file_transfer_info_list_item_t * item;
	item = file_transfer_info_get_list_item(list, client);
	if (item != NULL)
		return item->info;
	return NULL;
}

void file_transfer_info_del(file_transfer_info_list_t * list, al_bp_endpoint_id_t client)
{
	file_transfer_info_list_item_t * item;
	item = file_transfer_info_get_list_item(list, client);
	if (item != NULL)
	{
		file_transfer_info_list_item_delete(list, item);
	}
}

void file_transfer_info_list_item_delete(file_transfer_info_list_t * list, file_transfer_info_list_item_t * item)
{
	if (item->next == NULL && item->previous == NULL) // unique element of the list
	{
		list->first = NULL;
		list->last = NULL;
	}
	else if (item->next == NULL)  // last element of list
	{
		item->previous->next = NULL;
		list->last = item->previous;
	}
	else if (item->previous == NULL) // first element of list
	{
		item->next->previous = NULL;
		list->first = item->next;
	}
	else // generic element of list
	{
		item->next->previous = item->previous;
		item->previous->next = item->next;
	}
	file_transfer_info_destroy(item->info);
	free(item);
	list->count --;
}

int assemble_file(file_transfer_info_t * info, FILE * pl_stream,
		u32_t pl_size, u32_t timestamp_secs, u32_t expiration, uint16_t monitor_eid_len, uint32_t *crc)
{
	char * transfer;
	u32_t transfer_len;
	int fd;
	uint32_t offset;
	//uint32_t local_crc=0;

	// transfer length is total payload length without header,
	// congestion control char and file fragment offset
	transfer_len = get_file_fragment_size(pl_size, info->filename_len, monitor_eid_len);
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
	char* filename = (char*) malloc(info->filename_len + strlen(info->full_dir) +1);
	strcpy(filename, info->full_dir);
	strcat(filename, info->filename);
	fd = open(filename, O_WRONLY | O_CREAT, 0755);
	if (fd < 0)
	{
		return -1;
	}
	// write fragment
	lseek(fd, offset, SEEK_SET);
	if (write(fd, transfer, transfer_len) < 0)
		return -1;
	close(fd);

	// deallocate resources
	free(filename);
	free(transfer);

	// update info
	info->bytes_recvd += transfer_len;
	info->expiration = expiration;
	info->last_bundle_time = timestamp_secs;

	// if transfer completed return 1
	if (info->bytes_recvd >= info->file_dim)
		return 1;
	return 0;

}

int process_incoming_file_transfer_bundle(file_transfer_info_list_t *info_list,
		al_bp_bundle_object_t * bundle,
		char * dir, uint32_t *crc)
{
	al_bp_endpoint_id_t client_eid;
	al_bp_timestamp_t timestamp;
	al_bp_timeval_t expiration;
	file_transfer_info_t * info;
	FILE * pl_stream;
	int result;
	uint16_t filename_len;
	char * filename;
	uint32_t file_dim;
	u32_t pl_size;
	char * eid, temp[256];
	char * full_dir;

	// get info from bundle
	al_bp_bundle_get_source(*bundle, &client_eid);
	al_bp_bundle_get_creation_timestamp(*bundle, &timestamp);
//	if(al_bp_get_implementation() != BP_ION)
//		al_bp_bundle_get_expiration(*bundle, &expiration);
	al_bp_bundle_get_payload_size(*bundle, &pl_size);

	// create stream from incoming bundle payload
	if (open_payload_stream_read(*bundle, &pl_stream) < 0)
		return -1;

	// skip header - congestion control char - lifetime ack
	fseek(pl_stream, HEADER_SIZE + BUNDLE_OPT_SIZE + sizeof(al_bp_timeval_t) + BUNDLE_CRC_SIZE, SEEK_SET);
	// skip monitor eid
	uint16_t monitor_eid_len;
	char monitor_eid[256];
	if(fread(&monitor_eid_len, sizeof(monitor_eid_len), 1, pl_stream) != 1){return -1;}
	if(fread(monitor_eid, monitor_eid_len, 1, pl_stream) != 1){return -1;}

	info = file_transfer_info_get(info_list, client_eid);
	// get expiration time
	result = fread(&expiration, sizeof(expiration), 1, pl_stream);
	if( result < 1)
		perror("fread expiration time");
	if (info == NULL) // this is the first bundle
	{
		// get filename len
		result = fread(&filename_len, sizeof(filename_len), 1, pl_stream);
		// get filename
		filename = (char *) malloc(filename_len + 1);
		memset(filename, 0, filename_len + 1);
		result = fread(filename, filename_len, 1, pl_stream);
		if(result < 1 )
			perror("fread filename");
		filename[filename_len] = '\0';
		//get file size
		if(fread(&file_dim, sizeof(file_dim), 1, pl_stream)!=1){return -1;}
		// create destination dir for file
		memcpy(temp, client_eid.uri, strlen(client_eid.uri) + 1);
		// if is a URI endpoint remove service tag
		if(strncmp(temp,"ipn",3) !=0 )
		{
			strtok(temp, "/");
			eid = strtok(NULL, "/");
		}
		else
		{
			strtok(temp, ":");
			eid = strtok(NULL, "");
		}
		full_dir = (char*) malloc(strlen(dir) + strlen(eid) + 20);
		sprintf(full_dir, "%s%s/", dir, eid);
		if(mkpath(full_dir)<0){
			printf(" Error: Couldn't create temporary file\n");
			return -1;
		}
		sprintf(temp, "%u_", timestamp.secs);
		strcat(full_dir, temp);

		// create file transfer info object
		info = file_transfer_info_create(client_eid, filename_len, filename, full_dir, file_dim, timestamp.secs, expiration);
		// insert info into info list
		file_transfer_info_put(info_list, info);
		free(full_dir);
	}
	else  // first bundle of transfer already received
	{
		// skip filename_len and filename
		fseek(pl_stream, sizeof(filename_len) + strlen(info->filename), SEEK_CUR);

		// skip file_size
		fseek(pl_stream, sizeof(file_dim), SEEK_CUR);

	}

	// assemble file
	result = assemble_file(info, pl_stream, pl_size, timestamp.secs, expiration, monitor_eid_len, crc);
	close_payload_stream_read(pl_stream);
	if (result < 0)// error
		return result;
	if (result == 1) // transfer completed
	{
		printf(" Successfully transfered file: %s%s\n", info->full_dir, info->filename);
		// remove info from list
		file_transfer_info_del(info_list, client_eid);
		return 1;
	}
	return 0;


}

u32_t get_file_fragment_size(u32_t payload_size, uint16_t filename_len, uint16_t monitor_eid_len)
{
	u32_t result;
/*	long header_dtnperf, header_file;
	// size header + congestion ctrl char + ack lifetime + monitor eid len + monitor eid
	header_dtnperf = HEADER_SIZE + BUNDLE_OPT_SIZE + sizeof(al_bp_timeval_t) + monitor_eid_len + sizeof(monitor_eid_len);
	// bundle lifetime + filename + filename len + dim file + offset
	header_file = sizeof(al_bp_timeval_t) + filename_len + sizeof(filename_len) + sizeof(uint32_t) + sizeof(uint32_t);
	// fragment size is without dtnperf header and file header*/
	result = payload_size - get_header_size('F', filename_len, monitor_eid_len);
	return result;
}

al_bp_error_t prepare_file_transfer_payload(dtnperf_options_t *opt, FILE * f, int fd,
		char * filename, uint32_t file_dim, al_bp_timeval_t expiration_time, boolean_t * eof, uint32_t *crc, int *bytes_written)
{
	if (f == NULL)
		return BP_ENULLPNTR;

	al_bp_error_t result;
	uint32_t fragment_len;
	char * fragment;
	uint32_t offset;
	long bytes_read;
	uint16_t filename_len = strlen(filename);
	uint16_t monitor_eid_len = strlen(opt->mon_eid);

	// get size of fragment and allocate fragment
	fragment_len = get_file_fragment_size(opt->bundle_payload, filename_len, monitor_eid_len);
	fragment = (char *) malloc(fragment_len);
	memset(fragment, 0, fragment_len);
	// get offset of fragment
	offset = lseek(fd, 0, SEEK_CUR);

	// read fragment from file
	bytes_read = read(fd, fragment, fragment_len);
	if (bytes_read < fragment_len)// reached EOF
	{
		*eof = TRUE;
		fragment[bytes_read] = EOF;
	}
	else
		*eof = FALSE;

	// RESET CRC
	*crc= 0;
	*bytes_written=0;

	// prepare header and congestion control
	result = prepare_payload_header_and_ack_options(opt, f, crc, bytes_written);

	// write expiration time
	fwrite(&expiration_time, sizeof(expiration_time), 1, f);
	*bytes_written+=sizeof(expiration_time);
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &expiration_time, sizeof(expiration_time));
	// write filename length
	fwrite(&filename_len, sizeof(filename_len), 1, f);
	*bytes_written+=sizeof(filename_len);
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &filename_len, sizeof(filename_len));
	// write filename
	fwrite(filename, filename_len, 1, f);
	*bytes_written+=filename_len;
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) filename, filename_len);
	//write file size
	fwrite(&file_dim, sizeof(file_dim), 1, f);
	*bytes_written+=sizeof(file_dim);
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &file_dim, sizeof(file_dim));
	// write offset in the bundle
	fwrite(&offset, sizeof(offset), 1, f);
	*bytes_written+=sizeof(offset);
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &offset, sizeof(offset));
	// write fragment in the bundle
	fwrite(fragment, bytes_read, 1, f);
	*bytes_written+=bytes_read;
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) fragment, bytes_read);

	//printf("\n\tFRAGMENT:\n\t%s\n", fragment);
	return result;
}

int mkpath(char* dir)
{
	char* p;
	int offset = 0;
	char* temp;
	if (dir == NULL) return -1;
	if (dir[0] == '/') offset = 1;
	temp = (char*) malloc(sizeof(char) * (strlen(dir) + 1));
	for (p = strchr(dir + offset, '/'); p; p = strchr(p+1, '/'))
	{
		memset(temp, '\0', sizeof(char) * (strlen(dir) + 1));
		memcpy(temp, dir, p - dir);
		if (mkdir(temp, S_IRWXU) == -1)
		{
			if (errno != EEXIST) { free(temp); return -1; }
		}
	}
	free(temp);
	return 0;
}

