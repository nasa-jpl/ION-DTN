# ION Coding Guide

- [ION Coding Guide](#ion-coding-guide)
  - [Preface](#preface)
  - [Application Behavior](#application-behavior)
  - [Function Design Guidelines](#function-design-guidelines)
  - [Error Checking](#error-checking)
  - [Error and Status Reporting](#error-and-status-reporting)
  - [‘C’ Coding Style](#c-coding-style)
    - [Naming Conventions](#naming-conventions)
    - [Indentation, Bracketing, Whitespace](#indentation-bracketing-whitespace)
    - [Comment Formatting](#comment-formatting)
    - [Miscellaneous Rules](#miscellaneous-rules)

## Preface

The following coding guidelines apply to all software delivered as part of the Interplanetary Overlay Network (ION) distribution, except:

* Where the delivered software is legacy code rather than code developed specifically for ION.
* Where conformance to some other standard is clearly appropriate. For example, when using a framework library like Motif it may be appropriate to modify these guidelines so as to be consistent with the practices of the framework. 
* Where, in the judgment of the programmer, deviating from the guidelines in a particular case results in manifestly clearer code. This is not a license to ignore the guidelines; it is intended to cover special circumstances. 
Adherence to these guidelines is the responsibility of the individual programmer but will be considered during peer reviews of new ION code.

## Application Behavior

Every process should return an exit code on termination. 
* On normal termination, the exit code should be 0. 
* On abnormal or error termination, the exit code should be a non-zero number in the range 1-255. 
    * In this case the code should be 1 unless specific codes are used to distinguish between different kinds of errors.

## Function Design Guidelines

All file I/O should be performed using POSIX functions rather than the buffered I/O functions `fopen`, `fread`, `fseek`, etc.  This is because buffered I/O entails the dynamic allocation of system memory, which some missions may prohibit in flight software.  

The iputs function provided in `platform.c` should be used in place of `fputs`, and the `igets` function should be used in place of `fgets`.  Rather than `fscanf`, use `igets` and `sscanf`; rather than `fprintf`, use `isprintf` and `iputs`.

All varargs-based string composition should be performed using `isprintf` rather than `sprintf`, to minimize the chance of overrunning string composition buffers.  (`isprintf` is similar to `snprintf`.  Since VxWorks 5.4 does not support `snprintf`, `isnprintf` is included in `platform.c`.)  

Similarly, all string copying should be performed using `istrcpy` rather than `strcpy`, `strncpy`, and `strcat`.

The `isignal` function should be used instead of `signal`; it ensures that reception of a signal will always interrupt system calls in SVR4 fashion even when running on a FreeBSD platform. 

The `iblock` function provides a simple, portable means of preventing reception of the indicated signal by the calling thread.

Data objects larger than 1024 bytes should not be declared in stack space.  This is to 
  * Minimize complaints by Coverity, and 
  * Minimize the chance of overrunning allocated stack space when running on a VxWorks platform.

__Static variables that must be made globally accessible should be declared within external functions, rather than declared as external variables.__  This is per the JPL C Coding Standard, but it also has the useful property of providing an easy way to track all access to a global static variable in `gdb`: you just set a breakpoint at the start of the function in which the variable is declared.

## Error Checking

In the implementation of any ION library function or any ION task’s top-level driver function, any condition that prevents the function from continuing execution toward producing the effect it is designed to produce is considered an “error”.

Detection of an error should result in the printing of an error message and, normally, the immediate return of whatever return value is used to indicate the failure of the function in which the error was detected. 

By convention this value is usually -1, but both zero and NULL are appropriate failure indications under some circumstances such as object  creation.

The `CHKERR`, `CHKZERO`, `CHKNULL`, and `CHKVOID` macros are used to implement this behavior in a standard and lexically terse manner. Use of these macros offers an additional feature: for debugging purposes, they can easily be configured to call `sm_Abort()` to terminate immediately with a core dump instead of returning a error indication.  This option is enabled by setting the compiler parameter `CORE_FILE_NEEDED` to 1 at compilation time.

In the absence of any error, the function returns a value that indicates nominal completion. By convention this value is usually zero, but under some circumstances other values (such as pointers or addresses) are appropriate indications of nominal completion. Any additional information produced by the function, such as an indication of “success”, is usually returned as the value of a reference argument.  

However, database management functions and the SDR hash table management functions deviate from this rule: most return 0 to indicate nominal completion but functional failure (e.g., duplicate key or object not found) and return 1 to indicate functional success.

Whenever returning a value that indicates an error:
* If the failure is due to the failure of a system call or some other non-ION function, assu=me that errno has already been set by the function at the lowest layer of the call stack; use `putSysErrmsg` (or `postSysErrmsg` if in a hurry) as described below.
* Otherwise – i.e., the failure is due to a condition that was detected within ION –use `putErrmsg` (or `postErrmg` if pressed for time) as described below; this will aid in tracing the failure through the function stack in which the failure was detected.

When a failure in a called function is reported to “driver” code in an application program, before continuing or exiting use `writeErrmsgMemos()` to empty the message pool and print a simple stack trace identifying the failure.

Calling code may choose to ignore the error indication returned by a function (e.g., when an error returned by an sdr function is subsumed by a future check of the error code returned by `sdr_end_xn`).  To do so without incurring the wrath of a static analysis tool, pass the entire function call as the sole argument to the `oK` macro; the macro operates on the return code, casting it to `(void)` and thus placating static analysis.

## Error and Status Reporting

To write a simple status message, use `writeMemo`.  To write a status message and annotate that message with some other context-dependent string, use `writeMemoNote`.  (The `itoa` and `utoa` functions may be used to express signed and unsigned integer values, respectively, as strings for this purpose.)  Note that adhering to ION’s conventions for tagging status messages will simplify any automated status message processing that the messages might be delivered to, i.e., the first four characters of the status message should be as follows:

* [i] – informational
* [?] – warning
* [s] – reserved for bundle status reports
* [x] – reserved for communication statistics

To write a simple diagnostic message, use `putErrmsg`; the source file name and line number will automatically be inserted into the message text, and a context-dependent string may be provided.  (Again the `itoa` and `utoa` functions may be helpful here.)  The diagnostic message should normally begin with a capital letter and end with a period.

To write a diagnostic message in response to the failure of a system call or some other non-ION function that sets errno, use `putSysErrmsg` instead.  In this case, the diagnostic message should normally begin with a capital letter and not end with a period.
 
## ‘C’ Coding Style

This page contains guidelines for programming in the C language.

### Naming Conventions

Names of global variables, local variables, structure fields, and function arguments are in mixed upper and lower case, without embedded underscores, and beginning with a lowercase letter. 
```c
int	numItems;
```
Private function names are in mixed upper and lower case, without embedded underscores, and beginning with a lowercase letter. 

```c
void	computeSomething(int firstArg, int secondArg);
```
Public function names are in lower case with tokens separated by underscores.  The first token of each public function name is the name of the package whose “include” directory contains the .h file in which the function prototype is defined.
```c
	extern int	ltp_open(unsigned long clientId);
```
Macro names are written in upper case with tokens separated by underscores. 
```c
#define SYMBOLIC_CONSTANT 5
```

Unions are not used.

Typedef names are in mixed upper and lower case, with the first token capitalized.  Type names are never the same as the structure or enum tags for the structures and enums that they name. 
```c
typedef struct gloplist_str
{
int	thing1;
int	thing2;
} GlopList;
```

### Indentation, Bracketing, Whitespace

No line of source text is ever more than 80 characters long.  When the length of a line of code exceeds 80 characters, the line of code is wrapped across two or more lines of text.  Whenever the point at which the text must be wrapped is within a literal, a newline character (\) is inserted at the wrap point and the continuation of the literal begins in the first column of the next text line.  Otherwise, each continuation line is normally indented two tab stops from the first text line of the long line of code; when indenting just one tab stop (rather than two) seems to make the code more readable, indenting one tab stop is okay.  When a single meaningful clause of a source code line must be wrapped across multiple lines of text, each text line after the first line in that clause is normally indented one additional tab stop.

In the declaration of a function or variable, the type name and function/variable name are normally separated by a single tab.  They may be separated by multiple tabs when this is necessary in order to have the variable names in multiple consecutive variable declarations line up, which is always preferred.

Functions are written with the return type, function name, and arguments as a single line of code, subject to the code line wrapping guidelines given above.  The opening brace of the function definition appears in the first column of the next line.

The opening brace of a structure definition likewise appears in the first column of the next line after the structure name.

A control statement (starting with `if`, `else`, `while`, or `switch`) begins a new line of code.  The opening brace for the control statement always appears on the next line, at the same indentation as the control statement keyword.

The first line of code appearing after an opening brace (whether for a structure definition, for a function definition, or in the scope of a control statement) always appears on the next line, indented one tab stop.  From that point on, every subsequent line of code is indented the same number of tabs as the preceding line of code, subject to the code line wrapping guidelines given above.

Every closing brace always appears in the same column as the corresponding opening brace.

Every closing brace is always followed by a single blank line, except when it is immediately followed either by another closing brace (which will be indented one less tab stop) or by an else (which will be indented by the same number of tab stops as the closing brace and, therefore, the corresponding if).

```c
static void 	computeSomething(int numItems, Item *items)
{
	unsigned int	x;
	int		i;

	while (x > 0)
	{
		x--;
	}

	for (i = 0; i < numItems; i++)
	{
		x += items[i].field1;
	}

	if (numItems == 0)
	{
		doThis();
	}
	else
	{
		doThat();
	}
}
```

The case labels in switch statements line up with the braces.  Every case (or default) label in the switch, after the first case, is preceded by a blank line.  Cases which do not include a break or return statement either contain no code at all  or else end with a comment along the lines of:

```c
/* Intentional fall-through to next case. */
For example:
switch (ch)
{
case 'A':
.
.
.
break;

case 'B':
case 'C':
.
.
.
break;

case 'D':
.
.
.
/* Intentionally falls through. */

case 'E':
.
.
.
break;

default:
.
.
.
break;
}
```

### Comment Formatting

Comments are so rare and valuable that we hesitate to risk discouraging them by overly constraining their format.  In general, comments should be inserted in such a way as to be as easy as possible to read in relevant context.  The multi-line comment formatting performed automatically by vim is particularly acceptable.

```c
/*	Here is the beginning of an extremely long comment, so long
 * 	that it has to wrap over two lines of source code text.	*/
```
### Miscellaneous Rules

Use – and write – thread-safe library functions where possible.  E.g., normally prefer `strtok_r()` to `strtok()`. 

Avoid writing non-portable code, e.g., prefer POSIX library calls to OS-specific library calls. 

Template for ".c" files
```c
 1 2 3 4 5 6 7  
123456789012345678901234567890123456789012345678901234567890123456789012
/*
	platform_sm.c:	platform-dependent implementation of common
			functions, to simplify porting.

	Author:  Alan Schlutsmeyer, JPL

	Copyright 1997, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government sponsorship
	acknowledged.
									                                */
```

Each file should have a header comment like the one shown above. 

```c
#include <stdio.h>
#include <locallib.h>
#include "appheader.h"
  .
  .
  .
```

.h files are included just after the header.  System-provided headers should be specified with angle brackets; ION-provided headers should be specified with double-quotes.
```c
#define SYMBOLIC_CONSTANT 5
```

Next, symbolic constants and macros (if any) are defined. They normally go first, because they might be used in the definitions of data types and static variables.  However, symbolic constants and macros may be inserted later in the source text if that will improve the readability of the file. 

```c
typedef struct fb_str
{
	int	field1;
	in	field2;
} Foobar;
```

Data types are defined next because they might be used by static variables. 
```c
static int numFoobars = 0;  /* Number of foobars in the program. */
  .
  .
  .
```

Global functions used only within the program should be declared static. 
Public function prototypes should be in a header file; the definitions of those functions, with their headers, are included in the corresponding .c file.  Low-level functions, such as commonly-used utility functions, appear first in the .c file.  They are followed by the functions that call those functions directly, followed by higher-level-functions that call those functions, and so on.

Template for ".h" Files
```c
 1 2 3 4 5 6 7  
123456789012345678901234567890123456789012345678901234567890123456789012
/*
	platform_sm.h:	portable definitions of types and functions.

	Author:  Alan Schlutsmeyer, JPL

	Copyright 1997, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government sponsorship
	acknowledged.
								                                	*/
```

Each header file begins with a standard header comment like the one shown above.
```c
#ifndef _PLATFORM_SM_H_
#define _PLATFORM_SM_H_
```

Each header file must have an "include" guard. 
```c
#include "platform.h"
  .
  .
  .
```
Next come any includes required by the declarations in the header. 
```c
#ifdef __cplusplus
extern "C" {
#endif
```
Next comes the beginning of the C++ guard. This allows the header to be included in a C++ program without error. 

Next come declarations of various sorts, followed by the ends of the C++ and “include” guards. 
```c
#ifdef __cplusplus
}
#endif

#endif
```

Nothing should go after the "#endif" of the include guard. 

## SDR Transaction

All writing to an SDR heap must occur during a transaction that was initiated by the task issuing the write.  Transactions are single-threaded; if task B wants to start a transaction while a transaction begun by task A is still in progress, it must wait until A's transaction is either ended or cancelled.  

A transaction is begun by calling `sdr_begin_xn()`, and the current transaction is normally ended by calling the `sdr_end_xn()` function, which returns an error code in the event that any serious SDR-related processing error was encountered in the course of the transaction Transactions may safely be nested, provided that every level of transaction activity that is begun
is properly ended.

Another way to terminate a transaction is using the `sdr_exit_xn()` call, as a way to implement a critical section within which SDR data is read while no data modifications occurred. Using the `sdr_exit_xn()` to end a transaction indicates that no SDR modification should have occurred during the critical section. Therefore the `sdr_exit_xn` function will check whether any SDR modifications were made during the transaction and if the current transaction is the outer most layer (depth = 1) then it will result in a unrecoverable SDR error.

Given that `sdr_end_xn()` is designed to handle transaction with SDR modification while `sdr_exit_xn()` does not, why would one want to use `sdr_exit_xn()` to end a transaction? This is useful as a way to detect SDR changes that is not supposed to occur but does occur - it catches both logical error and implementation error in the code. It is a useful "check" whether the SDR behavior is as expected. In case  `sdr_exit_xn()` detected a SDR modification as the outer most layer of transaction, an error message will be loggedin ion.log file to document a potential issue with the implementation logic. This is usedul for debugging and improving the overall SDR transaction management software.

The choice between `sdr_end_xn()` and `sdr_exit_xn()` depends on the intent of the transaction and the outcome of a transaction:

* Situation A: If a fault occurred that requires reverting SDR modifications (including those made by all nested layers) leading up to the fault event, then one should call sdr_cancel_xn to trigger SDR reversibility, if configured, and allow ion to reload the volatile protocol state to restore the SDR heap and working memory state.

* Situation B: If a fault occurred but the implementation has the proper procedure and logic to handle handling it such as:

  * The fault did not result in any SDR heap changes, or
  * The modifications that occurred before the fault and as part of fault handling afterward do not require reversal. For example, if the SDR modifications made before the fault may still be valid despite the occurrence of the fault, and the SDR modifications made after the fault, intentionally as part of fault handling procedure such as updating bundle error counters are successful, __AND__
  * In either case, the integrity of the protocol state is not affected or can be resotred. For example, an invalid bundle was detected and can be safety removed from ION by making appropriate updates to the heap and the working memory. Then one can still call `sdr_end_xn` since the failure was handled nominally.

* Situation C: All transactions and modifications are nominal. In this case, call `sdr_end_xn.`

The implementation must be able to discern between situations A and B. When one cannot make certain that the SDR will be in operation state due to a complex transaction failure case, or due to system errors, one one should default to A and issue transaction cancellation and rely on reversibility in order to avoid leaving the SDR in an inconsistent state.