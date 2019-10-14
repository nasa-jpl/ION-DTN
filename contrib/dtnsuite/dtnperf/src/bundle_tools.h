/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

#ifndef BUNDLE_TOOLS_H_
#define BUNDLE_TOOLS_H_

#include "includes.h"
#include "dtnperf_types.h"
#include <al_bp_types.h>
#include "dtnperf_debugger.h"


typedef struct
{
    al_bp_bundle_id_t bundle_id;
    struct timeval send_time;
    u_int relative_id;
}
send_information_t;

/*
 * extension/metadata block information
 */
typedef struct extension_block_info
{
    boolean_t metadata;
    u_int64_t metadata_type;
    al_bp_extension_block_t block;
} extension_block_info_t;



long bundles_needed (long data, long pl);
void print_eid(char * label, al_bp_endpoint_id_t *eid);

void destroy_info(send_information_t *send_info);
void init_info(send_information_t *send_info, int window);
long add_info(send_information_t* send_info, al_bp_bundle_id_t bundle_id, struct timeval p_start, int window);
int is_in_info(send_information_t* send_info, al_bp_timestamp_t timestamp, int window);
int is_in_info_timestamp(send_information_t* send_info, al_bp_timestamp_t timestamp, int window);
int count_info(send_information_t* send_info, int window);
void remove_from_info(send_information_t* send_info, int position);
void set_bp_options(al_bp_bundle_object_t *bundle, dtnperf_connection_options_t *opt);

int open_payload_stream_read(al_bp_bundle_object_t bundle, FILE ** f);
int close_payload_stream_read(FILE * f);
int open_payload_stream_write(al_bp_bundle_object_t bundle, FILE ** f);
int close_payload_stream_write(al_bp_bundle_object_t * bundle, FILE * f);

al_bp_error_t prepare_payload_header_and_ack_options(dtnperf_options_t *opt, FILE * f, uint32_t *crc, int *bytes_written);
int get_bundle_header_and_options(al_bp_bundle_object_t * bundle, HEADER_TYPE * header, dtnperf_bundle_ack_options_t * options);
u32_t get_header_size(char mode, uint16_t filename_len, uint16_t monitor_eid_len);

al_bp_error_t prepare_generic_payload(dtnperf_options_t *opt, FILE * f, uint32_t *crc, int *bytes_written);
al_bp_error_t prepare_force_stop_bundle(al_bp_bundle_object_t * start, al_bp_endpoint_id_t monitor,
				al_bp_timeval_t expiration, al_bp_bundle_priority_t priority);
al_bp_error_t prepare_stop_bundle(al_bp_bundle_object_t * stop, al_bp_endpoint_id_t monitor,
		al_bp_timeval_t expiration, al_bp_bundle_priority_t priority, int sent_bundles);
al_bp_error_t get_info_from_stop(al_bp_bundle_object_t * stop, int * sent_bundles);
al_bp_error_t prepare_server_ack_payload(dtnperf_server_ack_payload_t ack, dtnperf_bundle_ack_options_t *bundle_ack_options, char ** payload, size_t * payload_size);

/**
 * Get reported eid and timestamp from bundle ack
 * If you don't need either eid or timestamp, just put NULL in eid or timestamp.
 */
al_bp_error_t get_info_from_ack(al_bp_bundle_object_t * ack, al_bp_endpoint_id_t
		* reported_eid, al_bp_timestamp_t * report_timestamp, uint32_t *extension_ack);

/**
 * MetaBlock utility funtions
 */
boolean_t check_metadata(extension_block_info_t* ext_block);
void set_metadata_type(extension_block_info_t* ext_block, u_int64_t metadata_type);
void get_extension_block(extension_block_info_t* ext_block,
			al_bp_extension_block_t * extension_block);
void set_block_buf(extension_block_info_t* ext_block, char * buf, u32_t len);


u32_t get_current_dtn_time();
/**
 * Prints bundle id in the format
 * source_uri, creation_timestamp.creation_seqno,fragment_offset,orig_length
 * Returns the total number of character written on dest, or a negative value
 * if an error is encountered.
 */
int bundle_id_sprintf(char * dest, al_bp_bundle_id_t * bundle_id);

/*
 * Same as "mkdir -p"
 */
int mkpath(char* dir);

void print_bundle(al_bp_bundle_object_t * bundle);

void print_ext_or_metadata_blocks(u32_t blocks_len, al_bp_extension_block_t *blocks_val);

#endif /*BUNDLE_TOOLS_H_*/
