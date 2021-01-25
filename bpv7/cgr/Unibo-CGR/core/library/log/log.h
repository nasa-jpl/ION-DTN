/** \file log.h
 *
 *  \brief  This file provides the declaration of functions used by
 *          this cgr's implementation to print log files.
 *
 ** \copyright Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
 **
 ** \par License
 **
 **    This file is part of Unibo-CGR.                                            <br>
 **                                                                               <br>
 **    Unibo-CGR is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation, either version 3 of the License, or
 **    (at your option) any later version.                                        <br>
 **    Unibo-CGR is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.                               <br>
 **                                                                               <br>
 **    You should have received a copy of the GNU General Public License
 **    along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  \author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *  \par Supervisor
 *       Carlo Caini, carlo.caini@unibo.it
 */

#ifndef SOURCES_LIBRARY_LOG_H_
#define SOURCES_LIBRARY_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>

#include "../list/list_type.h"
#include "../commonDefines.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if (LOG == 1)
extern int cleanLogDir();
extern FILE* openBundleFile(unsigned int num);
extern void closeBundleFile(FILE **file_call);
extern int openLogFile();
extern void closeLogFile();
extern int createLogDir();
extern void writeLog(const char *format, ...);
extern void writeLogFlush(const char *format, ...);
extern void setLogTime(time_t time);
extern void printCurrentState();
extern int print_string(FILE *file, char *string);
extern int print_ull_list(FILE *file, List list, char *brief, char *separator);
extern void log_fflush();

#else

//empty defines for some functions

#define closeBundleFile(file_call) do { } while(0)
#define closeLogFile() do { } while(0)
#define writeLog(f_, ...) do { } while(0)
#define writeLogFlush(f_, ...) do {  } while(0)
#define setLogTime(time) do { } while(0)
#define printCurrentState() do { } while(0)
#define print_string(file, string) do { } while(0)
#define print_ull_list(file,list,brief,separator) do { } while(0)
#define log_fflush() do {  } while(0)

#endif

#if (DEBUG_CGR == 1 && LOG == 1)
/**
 * \brief Print a log in the main log file, compiled only with DEBUG_CGR and LOG enabled.
 *
 * \hideinitializer
 */
#define debugLog(f_, ...) writeLog((f_), ##__VA_ARGS__)
/**
 * \brief Print a log in the main log file and flush the stream, compiled only with DEBUG_CGR and LOG enabled.
 *
 * \hideinitializer
 */
#define debugLogFlush(f_, ...) writeLogFlush((f_), ##__VA_ARGS__)
#else
#define debugLog(f_, ...) do {  } while(0)
#define debugLogFlush(f_, ...) do {  } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOURCES_LIBRARY_LOG_H_ */
