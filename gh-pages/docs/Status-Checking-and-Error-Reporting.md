# Error Checking and Status Reporting in ION

ION provides set of shorthand macros for error checking and functions for reporting errors and status information resulting in standardized format and capture in the ion.log.

## Error Checking

In the implementation of any ION library function or any ION task’s top-level driver function, any condition that prevents the function from continuing execution toward producing the effect it is designed to produce is considered an “error”.

Detection of an error should result in the printing of an  error message and, normally, the immediate return of whatever return value is used to indicate the failure of the function in which the error was detected. By convention this value is usually -1, but both zero and NULL are appropriate failure indications under some circumstances such as object creation.

The `CHKERR`, `CHKZERO`, `CHKNULL`, and `CHKVOID` macros are used to implement this behavior in a standard and lexically terse manner.  Use of these macros offers an additional feature: for debugging purposes, they can easily be configured to call `sm_Abort()` to terminate immediately with a core dump instead of returning a error indication.  This option is enabled by setting the compiler parameter `CORE_FILE_NEEDED` to 1 at compilation time.

In the absence of any error, the function returns a value that indicates nominal completion. By convention this value is usually zero, but under some circumstances other values (such as pointers or addresses) are appropriate indications of nominal completion.  Any additional information produced by the function, such as an indication of “success”, is usually returned as the value of a reference argument.  Please be aware that database management functions and the SDR hash table management functions deviate from this rule: most will return 0 to indicate nominal completion but functional failure (e.g., duplicate key or object not found) and return 1 to indicate functional success.

Whenever returning a value that indicates an error:

* If the failure is due to the failure of a system call or some other non-ION function, assume that `errno` has already been set by the function at the lowest layer of the call stack; use `putSysErrmsg` (or `postSysErrmsg` if in a hurry) as described below.
* Otherwise – i.e., the failure is due to a condition that was detected within ION – use `putErrmsg` (or `postErrmg` if pressed for time) as described below; this will aid in tracing the failure through the function stack in which the failure was detected.

When a failure in a called function is reported to “driver” code in an application program, before continuing or exiting, please use `writeErrmsgMemos()` to empty the message pool and print a simple stack trace identifying the failure.

Calling code may choose to ignore the error indication returned by a function (e.g., when an error returned by an sdr function is subsumed by a future check of the error code returned by sdr_end_xn).  To do so without incurring the wrath of a static analysis tool, pass the entire function call as the sole argument to the “oK” macro; the macro operates on the return code, casting it to (void) and thus placating static analysis.

## Error and Status Reporting

To write a simple status message, use `writeMemo`.  To write a status message and annotate that message with some other context-dependent string, use `writeMemoNote`.  (The itoa and utoa functions may be used to express signed and unsigned integer values, respectively, as strings for this purpose.)  Adhering to ION’s conventions for tagging status messages will simplify any automated status message processing that the messages might be delivered to, i.e., the first four characters of the status message should be as follows:

* “[i] ” –  informational
* “[?] ” –  warning
* “[s] ” –  (reserved for bundle status reports)
* “[x] ” –  (reserved for communication statistics)

To write a simple diagnostic message, use `putErrmsg`; the source file name and line number will automatically be inserted into the message text, and a context-dependent string may be provided.  (Again the itoa and utoa functions may be helpful here.)  The diagnostic message should normally begin with a capital letter and end with a period.

To write a diagnostic message in response to the failure of a system call or some other non-ION function that sets errno, use `putSysErrmsg` instead.  In this case, the diagnostic message should normally begin with a capital letter and not end with a period.