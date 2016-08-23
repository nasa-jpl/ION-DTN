/*****************************************************************************
 **
 ** File Name: csi_debug.h
 **
 ** Description: Implements a debug logging system for users of the CSI interface
 **
 **
 ** Notes:
 **    1. ION does not ship with any security ciphersuites.
 **    2. The structures and functions defined in this interface do not imply
 **       any particular ciphersuite implementation and should not be considered
 **       as more or less appropriate for any ciphersuite library.
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTIONuint8_t *out
 **  --------  ------------   ---------------------------------------------
 **  09/20/15  E. Birrane     Update to generic interfaces [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/


#ifndef _CSI_DEBUG_H_
#define _CSI_DEBUG_H_

#include "platform.h"
#include "ion.h"

#ifndef CSI_DEBUGGING
#define CSI_DEBUGGING 1  /** Whether to enable (1) or disable (0) debugging */
#endif

#define CSI_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define CSI_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define CSI_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define CSI_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define CSI_DEBUG_LVL   CSI_DEBUG_LVL_ERR

#define GMSG_BUFLEN     256

extern char	gCsiMsg[GMSG_BUFLEN];

#if CSI_DEBUGGING == 1

/**
 * \def CSI_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the ciphersuite library and is useful for confirming control flow
 *    through the ciphersuite modules.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by ciphersuite module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software.
 *
 * Error logging within the ciphersuite module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 *
 * Debugging can be turned off at compile time by removing the
 * CSI_DEBUGGING #define.
 */

   #define CSI_DEBUG(level, format,...) if(level >= CSI_DEBUG_LVL) \
{_isprintf(gCsiMsg, GMSG_BUFLEN, format, __VA_ARGS__); putErrmsg(gCsiMsg, NULL);}

   #define CSI_DEBUG_PROC(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define CSI_DEBUG_INFO(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define CSI_DEBUG_WARN(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define CSI_DEBUG_ERR(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define CSI_DEBUG(level, format,...) if(level >= CSI_DEBUG_LVL) \
{}

   #define CSI_DEBUG_PROC(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define CSI_DEBUG_INFO(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define CSI_DEBUG_WARN(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define CSI_DEBUG_ERR(format,...) \
           CSI_DEBUG(CSI_DEBUG_LVL_ERR,format, __VA_ARGS__)

#endif




#endif
