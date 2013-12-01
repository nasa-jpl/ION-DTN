//
//  nm_debug.h
//  DTN NM Agent
//
//  Created by Birrane, Edward J. on 10/21/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef DEBUG_H_
#define DEBUG_H_

/*****************************************************************************
 *                              DEBUG DEFINITIONS                            *
 *****************************************************************************/

#ifndef DTNMP_DEBUGGING
#define DTNMP_DEBUGGING	1  /** Whether to enable (1) or disable (0) debugging */
#endif

#define DTNMP_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define DTNMP_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define DTNMP_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define DTNMP_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define DTNMP_DEBUG_LVL	DTNMP_DEBUG_LVL_WARN

#define	GMSG_BUFLEN	512
#if DTNMP_DEBUGGING == 1
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif

/**
 * \def DTNMP_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the DTNMP library and is useful for confirming control flow
 *    through the DTNMP module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by DTNMP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the DTNMP module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * DTNMP_DEBUGGING #define.
 */

#define DTNMP_DEBUG(level, type, func, format,...) if(level >= DTNMP_DEBUG_LVL) \
{isprintf(gMsg, GMSG_BUFLEN, (char *) format, __VA_ARGS__); \
fprintf(stderr, "[%s:%d] %c %s %s\n",__FILE__,__LINE__,type, func, gMsg);}

#define DTNMP_DEBUG_ENTRY(func, format,...) \
DTNMP_DEBUG(DTNMP_DEBUG_LVL_PROC,'+',func,format, __VA_ARGS__)

#define DTNMP_DEBUG_EXIT(func, format,...) \
DTNMP_DEBUG(DTNMP_DEBUG_LVL_PROC,'-',func,format, __VA_ARGS__)

#define DTNMP_DEBUG_INFO(func, format,...) \
DTNMP_DEBUG(DTNMP_DEBUG_LVL_INFO,'i',func,format, __VA_ARGS__)

#define DTNMP_DEBUG_WARN(func, format,...) \
DTNMP_DEBUG(DTNMP_DEBUG_LVL_WARN,'w',func,format, __VA_ARGS__)

#define DTNMP_DEBUG_ERR(func, format,...) \
DTNMP_DEBUG(DTNMP_DEBUG_LVL_ERR,'x',func,format, __VA_ARGS__)

#define DTNMP_DEBUG_ALWAYS(func, format,...) \
DTNMP_DEBUG(DTNMP_DEBUG_LVL,':',func,format, __VA_ARGS__)

#endif // DEBUG_H_
