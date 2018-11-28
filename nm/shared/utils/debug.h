/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file debug.h
 **
 ** Description: AMP debugging tools
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/21/11  E. Birrane     Initial Implementation. (JHU/APL)
 **  08/03/16  E. Birrane     Cleanup from DTNMP to AMP (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef DEBUG_H_
#define DEBUG_H_

/*****************************************************************************
 *                              DEBUG DEFINITIONS                            *
 *****************************************************************************/

#ifndef AMP_DEBUGGING
#define AMP_DEBUGGING	1  /** Whether to enable (1) or disable (0) debugging */
#endif

#define AMP_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define AMP_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define AMP_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define AMP_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define AMP_DEBUG_LVL	AMP_DEBUG_LVL_WARN

#define	AMP_GMSG_BUFLEN	256
#if AMP_DEBUGGING == 1
extern char		gAmpMsg[];		/*	Debug message buffer.	*/
#endif

/**
 * \def AMP_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the AMP library and is useful for confirming control flow
 *    through the AMP module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by AMP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the AMP module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * AMP_DEBUGGING #define.
 */

#if defined (ION_LWT)

#define AMP_DEBUG(level, type, func, format,...) if(level >= AMP_DEBUG_LVL) \
{_isprintf(gAmpMsg, AMP_GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(func, gAmpMsg);}

#else

#define AMP_DEBUG(level, type, func, format,...) if(level >= AMP_DEBUG_LVL) \
{isprintf(gAmpMsg, AMP_GMSG_BUFLEN, (char *) format, __VA_ARGS__); \
fprintf(stderr, "[%s:%d] %c %s %s\n",__FILE__,__LINE__,type, func, gAmpMsg);}

#endif

#define AMP_DEBUG_ENTRY(func, format,...) \
AMP_DEBUG(AMP_DEBUG_LVL_PROC,'+',func,format, __VA_ARGS__)

#define AMP_DEBUG_EXIT(func, format,...) \
AMP_DEBUG(AMP_DEBUG_LVL_PROC,'-',func,format, __VA_ARGS__)

#define AMP_DEBUG_INFO(func, format,...) \
AMP_DEBUG(AMP_DEBUG_LVL_INFO,'i',func,format, __VA_ARGS__)

#define AMP_DEBUG_WARN(func, format,...) \
AMP_DEBUG(AMP_DEBUG_LVL_WARN,'w',func,format, __VA_ARGS__)

#define AMP_DEBUG_ERR(func, format,...) \
AMP_DEBUG(AMP_DEBUG_LVL_ERR,'x',func,format, __VA_ARGS__)

#define AMP_DEBUG_ALWAYS(func, format,...) \
AMP_DEBUG(AMP_DEBUG_LVL,':',func,format, __VA_ARGS__)

#endif // DEBUG_H_
