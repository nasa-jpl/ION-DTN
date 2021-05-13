/*
eclsaLogger.h

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 	 	 Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file contains the definitions of the logger utility
used by eclso and eclsi.

 * */

#ifndef _ECLOGGER_H_

#define _ECLOGGER_H_

#include <stdarg.h>     // va_list, va_start, va_arg, va_end
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_ENABLED true

void loggerInit(int maxLogFiles); //initialize the logger utility
void loggerDestroy(); //destroy the logger utility
void loggerStartLog(int loggerID,char *filename, bool printTimestamp, bool printNewLine);
void loggerPrint(int loggerID,const char *string, ...);
void loggerPrintVaList(int loggerID,const char *string,va_list lp);

/*Debug logger shortcuts todo: need macro for these two*/
void debugPrint(const char *string, ...);
void packetLogger(const char *string, ...);

#ifdef __cplusplus
}
#endif

#endif
