/** \file commonDefines.h
 *
 *  \brief  The purpose of this file is to keep in one place
 *          the definitions and includes used by many files.
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


#ifndef LIBRARY_COMMONDEFINES_H_
#define LIBRARY_COMMONDEFINES_H_

#include <sys/time.h>
#include <stdlib.h>

#include "../config.h"

#ifdef __cplusplus
extern "C"
{
#endif

// just to use with function pointer
extern void MDEPOSIT_wrapper(void *addr);

#ifdef __cplusplus
}
#endif

#define	MAX_POSIX_TIME	2147483647

#define CLEAR_FLAGS(flags) ((flags) = 0)

#if (CGR_DEBUG_FLUSH == 1)
/**
 * \brief   Flush the stream (fflush), compiled only with CGR_DEBUG_FLUSH enabled.
 *
 * \hideinitializer
 */
#define debug_fflush(file) fflush(file)
#else
#define debug_fflush(file) do {  } while(0)
#endif

#if (DEBUG_CGR == 1)

/**
 * \brief   Debug utility function: print the file, line and function that called this macro
 *          and then print the string passed as arguments.
 *
 * \details Use exactly like a printf function.
 *
 * \hideinitializer
 */
#define verbose_debug_printf(f_, ...) do { \
	fprintf(stdout, "At line %d of %s, %s(): ", __LINE__, __FILE__, __FUNCTION__); \
	fprintf(stdout, (f_), ##__VA_ARGS__); \
	fputc('\n', stdout); \
	debug_fflush(stdout); \
} while(0)

/**
 * \brief   Debug utility function: print the file, line and function that called this macro
 *          and then print the string passed as arguments. At the end flush the stream.
 *
 * \details Use exactly like a printf function.
 *
 * \hideinitializer
 */
#define flush_verbose_debug_printf(f_, ...) do { \
	fprintf(stdout, "At line %d of %s, %s(): ", __LINE__, __FILE__, __FUNCTION__); \
	fprintf(stdout, (f_), ##__VA_ARGS__); \
	fputc('\n', stdout); \
	fflush(stdout); \
} while(0)

/**
 * \brief   Debug utility function: print the function that called this macro
 *          and then print the string passed as arguments. At the end flush the stream;
 *
 * \details Use exactly like a printf function.
 *
 * \hideinitializer
 */
#define flush_debug_printf(f_, ...) do { \
	fprintf(stdout, "%s(): ", __FUNCTION__); \
	fprintf(stdout, (f_), ##__VA_ARGS__); \
	fputc('\n', stdout); \
	fflush(stdout); \
} while(0)

/**
 * \brief   Debug utility function: print the function that called this macro
 *          and then print the string passed as arguments.
 *
 * \details Use exactly like a printf function.
 *
 * \hideinitializer
 */
#define debug_printf(f_, ...) do { \
	fprintf(stdout, "%s(): ", __FUNCTION__); \
	fprintf(stdout, (f_), ##__VA_ARGS__); \
	fputc('\n', stdout); \
	debug_fflush(stdout); \
} while(0)

#else
	// empty define
#define debug_printf(f_, ...) do { } while(0)
#define verbose_debug_printf(f_, ...) do { } while(0)
#define flush_debug_printf(f_, ...) do {  } while(0)
#define flush_verbose_debug_printf(f_, ...) do {  } while(0)
#endif

#include "log/log.h"

#endif /* LIBRARY_COMMONDEFINES_H_ */
