/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * dtnperf_types.h
 */

#ifndef DTNPERF_TYPES_H_
#define DTNPERF_TYPES_H_

#include "definitions.h"
#include <al_bp_types.h>
#include <stdio.h>

typedef enum {
	DTNPERF_SERVER = 1,
	DTNPERF_CLIENT,
	DTNPERF_MONITOR
	//DTNPERF_CLIENT_MONITOR
} dtnperf_mode_t;

typedef enum {
	DTN = 1,
	IPN
} dtnperf_eid_format_t;

// server ack to monitor options
typedef enum {
	ATM_NORMAL = 0x0,
	ATM_FORCE_YES = 0x1,
	ATM_FORCE_NO = 0x2,
} dtnperf_ack_to_mon_options_t;

// options to send to the server by the client
typedef struct dtnperf_bundle_ack_options
{
	boolean_t ack_to_client;
	dtnperf_ack_to_mon_options_t ack_to_mon;
	boolean_t set_ack_expiration;
	al_bp_timeval_t ack_expiration;
	boolean_t set_ack_priority;
	al_bp_bundle_priority_t ack_priority;
	boolean_t crc_enabled;
} dtnperf_bundle_ack_options_t;

/**
 * To change default values go to init_dtnperf_options()
 */
typedef struct dtnperf_options
{
	//general options
	al_bp_implementation_t		 	bp_implementation; 					// Bundle Protocol implementation
	int								debug;								// if true, debug messages are shown [FALSE]
	int 							use_file;							// if set to 1, a file is used instead of memory [1]
	boolean_t		 				use_ip;								// set different values of ip address and port [FALSE]
	char*							ip_addr;							// daemon ip address [127.0.0.1]
	short 							ip_port;							// daemon port [5010]
	char 							eid_format_forced;					// is the format of the eid (U = URI, C = CBHE, N = None) [N]
	int								ipn_local_num;						// local ipn eid number (Used only if dtnperf server or monitor must register with ipn scheme on DTN2) [0]
	//boolean_t 						daemon;								// run as daemon (server and monitor) [FALSE]
	char*							server_output_file;					// stdout and stderr redirect here if daemon is TRUE [SERVER_OUTPUT_FILE]
	char*							monitor_output_file;				// stdout and stderr redirect here if daemon is TRUE [MONITOR_OUTPUT_FILE]
	//client options
	char 							dest_eid[AL_BP_MAX_ENDPOINT_ID];	// destination eid
	char 							mon_eid[AL_BP_MAX_ENDPOINT_ID];		// monitor eid
	char 							op_mode;    						// operative mode (T = time_mode, D = data_mode, F = file_mode) [D]
	double 							data_qty;							// data to be transmitted (bytes) [0]
	char*							D_arg;								// arguments of -D option
	char*							F_arg;								// argument of -F option (filename)
	char*							P_arg;								// arguments of -P option
	char 							data_unit;							// B = bytes, K = kilobytes, M = megabytes [M]
	int 							transmission_time;					// seconds of transmission [0]
	char 							congestion_ctrl;					// W = window based, R = rate based [W]
	int 							window;								// transmission window (bundles) [1]
	char* 							rate_arg;							// argument of -r option
	double 							rate;								// transmission rate [0]
	char 							rate_unit;							// b = bit/sec; B = bundle/sec [b]
	int 							wait_before_exit;					// additional interval before exit [0]
	double 							bundle_payload;  					// quantity of data (in bytes) to send (-p option) [DEFAULT_PAYLOAD]
	dtnperf_bundle_ack_options_t 	bundle_ack_options;					// options to send to the server
	al_bp_bundle_payload_location_t payload_type;						// the type of data source for the bundle [DTN_PAYLOAD_FILE]
	boolean_t 						create_log;							// create log file [FALSE]
	char*						 	log_filename;						// log filename [LOG_FILENAME]
	boolean_t						no_bundle_stop;						// do not send bundle stop and force stop to the monitor [FALSE]
	//server options
	char* 							dest_dir;							// destination dir of bundles [~/dtnperf/bundles]
	char* 							file_dir;							// destination dir of transferred files [~/dtnperf/files]
	boolean_t 						acks_to_mon;						// send ACKs to both source and monitor (if monitor is not the source) [FALSE]
	boolean_t 						no_acks;							// do not send ACKs (for retro-compatibility purpose)
	//monitor options
	char* 							logs_dir;							// dir where are saved monitor logs [LOGS_DIR_DEFAULT]
	int 							expiration_session; 				// expiration time of session log file [120]
	boolean_t						oneCSVonly;							// monitor opens an unique session and an unique csv log file [FALSE]
	boolean_t						rtPrint;							// monitor prints realtime human readable status report info [FASLE]
	FILE*							rtPrintFile;						// monitor prints realtime SR info on this file [stdout]
	char*							uniqueCSVfilename;					// filename of the unique csv log file [MONITOR_UNIQUE_CSV_FILENAME]
	// block options
	u16_t 							num_blocks;  	  				 	// number of extension and metadata blocks
	u64_t 							metadata_type;   					// metadata type code
	uint32_t						crc;								// crc enabled [FALSE]
} dtnperf_options_t;

/**
 * To change default values go to init_dtnperf_connection_options()
 */
typedef struct dtnperf_connection_options
{
	al_bp_timeval_t expiration;			// bundle expiration time (sec)
	al_bp_bundle_priority_t priority;		// bundle priority
	boolean_t unreliable;   // unrelabiable value [FALSE]
	boolean_t critical;		// critical value [FALSE]
	u32_t flow_label;		// flow label value [0]
	boolean_t delivery_receipts;
	boolean_t forwarding_receipts;
	boolean_t custody_transfer;
	boolean_t custody_receipts;
	boolean_t receive_receipts;
	boolean_t deleted_receipts;
	boolean_t wait_for_report;
	boolean_t disable_fragmentation;
} dtnperf_connection_options_t;

typedef struct dtnperf_global_options
{
	dtnperf_mode_t mode;
	dtnperf_options_t * perf_opt;
	dtnperf_connection_options_t * conn_opt;
} dtnperf_global_options_t;

typedef struct dtnperf_server_ack_payload
{
	al_bp_endpoint_id_t bundle_source;
	al_bp_timestamp_t bundle_creation_ts;
} dtnperf_server_ack_payload_t;


#endif /* DTNPERF_TYPES_H_ */
