/********************************************************
 **  Authors: Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **  		  Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * dtnperf_debugger.c
 */

#include "dtnperf_debugger.h"

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void logger_print_va_list(FILE* fp, const char *string, va_list argList);

typedef struct {
	int debug;	//debug level
	boolean_t create_log;	//request log_file
	FILE* log_fp;	//file pointer
	sem_t lock;	//semaphore
} debugger_t;

static debugger_t debug_struct;
static boolean_t is_initialized = FALSE;
static boolean_t is_sem_initialized = FALSE;

int debugger_init(int debug, boolean_t create_log, char *log_filename) {
	if (is_initialized || debug < 0) return EXIT_FAILURE;
	is_initialized = TRUE;
	memset(&debug_struct, 0, sizeof(debug_struct));

	debug_struct.debug = debug;
	debug_struct.create_log = create_log;

	if (!is_sem_initialized) {
		sem_init(&(debug_struct.lock), 0, 1);
		is_sem_initialized = TRUE;
	}

	if (debug_struct.create_log) {
		debug_struct.log_fp = fopen(log_filename, "w");
		if (debug_struct.log_fp == NULL) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int debugger_destroy() {
	if (!is_initialized) return EXIT_FAILURE;
	is_initialized = FALSE;
	if (is_sem_initialized) {
		sem_destroy(&(debug_struct.lock));
		is_sem_initialized = FALSE;
	}
	if (debug_struct.create_log) {
		if (fclose(debug_struct.log_fp) == EOF) {
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

void logger_print_va_list(FILE* fp, const char *string, va_list argList) {
	if (fp == NULL) {
		printf("Failed to print debug\n");
		return;
	}

	if (!is_sem_initialized) {
		sem_init(&(debug_struct.lock), 0, 1);
		is_sem_initialized = TRUE;
	}
	sem_wait(&(debug_struct.lock));
	vfprintf(fp, string, argList);
	fflush(fp);
	sem_post(&(debug_struct.lock));
	return;
}

void debug_print(debug_level level, const char *string, ...) {
	if (is_initialized && debug_struct.debug >= level && level > DEBUG_OFF) {
		va_list lp;
		va_start(lp, string);
		logger_print_va_list(stdout, string, lp);
		// print log
		if (debug_struct.create_log)
			logger_print_va_list(debug_struct.log_fp, string, lp);
		va_end(lp);
	}
}

void error_print(const char* string, ...) {
	va_list lp;
	va_start(lp, string);
	logger_print_va_list(stderr, string, lp);
	va_end(lp);
	return;
}

FILE* get_log_fp() {
	if (debug_struct.create_log && is_initialized)
		return debug_struct.log_fp;
	else
		return NULL;
}

boolean_t debug_check_level(debug_level required_level) {
	return (debug_struct.debug >= required_level);
}

/*
 void verbose_print(const char *string, ...) {
 if (debug_struct.verbose) {
 va_list lp;
 va_start(lp, string);
 logger_print_va_list(stdout, string, lp);
 va_end(lp);
 }
 return;
 }
 */

