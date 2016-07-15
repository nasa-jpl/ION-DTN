/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/


/*****************************************************************************
 **
 ** \file nm_types.h
 **
 ** Description: AMP Structures and Data Types
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/21/11  E. Birrane     Initial Implementation. (JHU/APL)
 *****************************************************************************/
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

#define	DTNMP_GMSG_BUFLEN	256
#if DTNMP_DEBUGGING == 1
extern char		gDtnmpMsg[];		/*	Debug message buffer.	*/
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

#if defined (ION_LWT)

#define DTNMP_DEBUG(level, type, func, format,...) if(level >= DTNMP_DEBUG_LVL) \
{_isprintf(gDtnmpMsg, DTNMP_GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(func, gDtnmpMsg);}

#else

#define DTNMP_DEBUG(level, type, func, format,...) if(level >= DTNMP_DEBUG_LVL) \
{isprintf(gDtnmpMsg, DTNMP_GMSG_BUFLEN, (char *) format, __VA_ARGS__); \
fprintf(stderr, "[%s:%d] %c %s %s\n",__FILE__,__LINE__,type, func, gDtnmpMsg);}

#endif

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
