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
 * dtnperf_client.h
 */

#ifndef DTNPERF_CLIENT_H_
#define DTNPERF_CLIENT_H_

#include "../dtnperf_types.h"
#include <stdio.h>

void run_dtnperf_client(dtnperf_global_options_t * global_options);
void print_client_usage(char* progname);
void parse_client_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt);
void check_options(dtnperf_global_options_t * global_options);
void print_final_report(FILE * f);
void client_handler(int sig);
void client_clean_exit(int status);

//create payload function
void create_fill_payload_buf(int num_bundle);

//pthread functions
void * send_bundles(void * opt);
void * congestion_control(void * opt);
void * congestion_window_expiration_timer(void * opt);
//void * start_dedicated_monitor(void * params);
void * wait_for_sigint(void * arg);
#endif /* DTNPERF_CLIENT_H_ */
