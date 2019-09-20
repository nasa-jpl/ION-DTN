/********************************************************
    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
    Carlo Caini (DTNfog project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

 ********************************************************/

/*
 * debugger.h

 * This file contains the function interfaces
 * of debugger
 *
 */

#ifndef DTNfog_SRC_DEBUGGER_H_
#define DTNfog_SRC_DEBUGGER_H_

#include <stdarg.h>
#include "proxy_thread.h"

/**
 * Debug structures
 */
typedef enum debug_level {
	DEBUG_OFF=0,
	DEBUG_ON
} debug_level;

/**
 * Debug functions
 */
int debugger_init(int debug, boolean_t create_log, char *log_filename);
void debug_print(debug_level level, const char *string, ...);
void error_print(const char* string, ...);
int debugger_destroy();

#endif /* DTNfog_SRC_DEBUGGER_H_ */
