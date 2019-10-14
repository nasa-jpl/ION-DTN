  /********************************************************
  ** Authors: Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
  **          Carlo Caini (DTNbox project supervisor), carlo.caini@unibo.it
  **
  ** Copyright 2018 University of Bologna
  ** Released under GPLv3. See LICENSE.txt for details.
  **
  ********************************************************/
/*
 * debugger.h
 */

#ifndef DTNPERF_SRC_DTNPERF_DEBUGGER_H_
#define DTNPERF_SRC_DTNPERF_DEBUGGER_H_

#include "../../al_bp/src/types.h"
#include <stdio.h>
// Be careful: 0 means DEBUG_OFF!
typedef enum debug_level {DEBUG_OFF=0, DEBUG_L1, DEBUG_L2} debug_level;

#include <stdarg.h>
int debugger_init(int debug, boolean_t create_log, char *log_filename);
int debugger_destroy();

boolean_t debug_check_level(debug_level required_level);

void debug_print(int level, const char *string, ...);

void error_print(const char* string, ...);

FILE* get_log_fp();

//void verbose_print(const char *string, ...);

#endif /* DTNPERF_SRC_DTNPERF_DEBUGGER_H_ */
