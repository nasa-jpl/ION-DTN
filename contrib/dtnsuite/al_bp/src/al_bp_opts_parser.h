/********************************************************
 **  Authors: Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Federico Domenicali, federico.domenicali@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

#ifndef AL_BP_PARSER_H_
#define AL_BP_PARSER_H_

#include "includes.h"
#include "types.h"
#include "al_bp_types.h"
#include "al_bp_api.h"

typedef struct {
	//al_bp_implementation_t bp_implementation;
	al_bp_timeval_t lifetime;
	al_bp_bundle_priority_t priority;
	boolean_t custody_transfer;
	boolean_t disable_fragmentation;
	boolean_t delivery_reports;
	boolean_t forwarding_reports;
	boolean_t custody_reports;
	boolean_t reception_reports;
	boolean_t deletion_reports;
	u64_t metadata_type;
	string_t metadata_string;
	boolean_t unreliable;
	boolean_t critical;
	u32_t flow_label;
	u32_t ipn_local;
	al_bp_scheme_t eid_format_forced;
} bundle_options_t;



typedef enum{
	OK=1,
	WRONG_IMPLEMENTATION=-1,
	WRONG_VALUE=-2,
} result_codes;

typedef struct {
	char* option;
	char* option_arg;
	result_codes result_code;
} checked_bp_option_t;

typedef struct unknown_opts{
	char** opts;
	int opts_size;
} unknown_options_t;

typedef struct {
	checked_bp_option_t* opts;
	int opts_size;
} recognized_bp_options_t;

typedef struct {
	recognized_bp_options_t recognized_options;
	unknown_options_t unknown_options;
} result_options_t;

result_codes al_bp_check_bp_options(int argc, char **argv, bundle_options_t* bundle_options, result_options_t* result_options);
void al_bp_free_result_options(result_options_t result_options);
char * get_help_bp_options(void);
al_bp_error_t al_bp_create_bundle_with_option(al_bp_bundle_object_t *bundle, bundle_options_t bundle_options);

#endif
