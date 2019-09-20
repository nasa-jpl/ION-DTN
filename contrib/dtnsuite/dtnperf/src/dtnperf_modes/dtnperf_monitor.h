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
 * dtnperf_monitor.h
 */

#ifndef DTNPERF_MONITOR_H_
#define DTNPERF_MONITOR_H_

#include "../dtnperf_types.h"
#include <stdio.h>

typedef struct monitor_parameters
{
	dtnperf_global_options_t * perf_g_opt;
	//boolean_t dedicated_monitor;
	int client_id;
} monitor_parameters_t;

typedef enum
{
	NONE,
	STATUS_REPORT,
	SERVER_ACK,
	CLIENT_STOP,
	CLIENT_FORCE_STOP
} bundle_type_t;

typedef struct session
{
	al_bp_endpoint_id_t client_eid;
	char * full_filename;
	FILE * file;
	struct timeval * start;
	u32_t last_bundle_time; // secs of bp creation timestamp
	u32_t expiration;
	int delivered_count;
	int total_to_receive;
	struct timeval * stop_arrival_time;
	u32_t wait_after_stop;
	struct session * next;
	struct session * prev;
	long int wrong_crc;
}session_t;

typedef struct session_list
{
	session_t * first;
	session_t * last;
	int count;
}session_list_t;

session_list_t * session_list_create();
void session_list_destroy(session_list_t * list);

/**
 * Create an unique session. Used with --oneCSVonly option
 */
session_t * unique_session_create(char * full_filename, FILE * file, struct timeval start, u32_t bundle_timestamp_secs);
session_t * session_create(al_bp_endpoint_id_t client_eid, char * full_filename, FILE * file, struct timeval start,
		u32_t bundle_timestamp_secs, u32_t bundle_expiration_time);
void session_destroy(session_t * session);

void session_put(session_list_t * list, session_t * session);

session_t * session_get(session_list_t * list, al_bp_endpoint_id_t client);

void session_del(session_list_t * list, session_t * session);
void session_close(session_list_t * list, session_t * session);
/**
 * Used by realtime human readable status report information print
 */
void printRealtimeStatusReport(FILE *f, al_bp_endpoint_id_t sr_source, al_bp_bundle_status_report_t * status_report);
void printSingleRealtimeStatusReport(FILE *f, al_bp_endpoint_id_t sr_source, al_bp_bundle_status_report_t * status_report,
		al_bp_status_report_flags_t type, al_bp_timestamp_t timestamp);

void run_dtnperf_monitor(monitor_parameters_t * parameters);

//session expiration timer thread
void * session_expiration_timer(void * opt);

void print_monitor_usage(char* progname);
void parse_monitor_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt);

void monitor_clean_exit(int status);
void monitor_handler(int signo);

#endif /* DTNPERF_MONITOR_H_ */
