# Platform Macros & Error Reporting

_From manual page for "platform"_

## Platform Compatibility

The platform library "patches" the APIs of supported OS's to guarantee that all of the following items may be utilized by application software:


    The strchr(), strrchr(), strcasecmp(), and strncasecmp() functions.

    The unlink(), getpid(), and gettimeofday() functions.

    The select() function.

    The FD_BITMAP macro (used by select()).

    The MAXHOSTNAMELEN macro.

    The NULL macro.

    The timer_t type definition.

## Platform Generic Macros & Functions

The generic macros and functions in this section may be used in place of comparable O/S-specific functions, to enhance the portability of code. (The implementations of these macros and functions are no-ops in environments in which they are inapplicable, so they're always safe to call.)

### FDTABLE_SIZE

The **FDTABLE_SIZE** macro returns the total number of file descriptors defined for the process (or VxWorks target).

### ION_PATH_DELIMITER

The **ION_PATH_DELIMITER** macro returns the ASCII character -- either '/' or '\' -- that is used as a directory name delimiter in path names for the file system used by the local platform.

### oK

```c
oK(expression)
```
The oK macro simply casts the value of expression to void, a way of handling function return codes that are not meaningful in this context.

### CHKERR

```c
CHKERR(condition)
```

The CHKERR macro is an "assert" mechanism. It causes the calling function to return -1 immediately if condition is false.

### CHKZERO

```c
CHKZERO(condition)
```

The CHKZERO macro is an "assert" mechanism. It causes the calling function to return 0 immediately if condition is false.

### CHKNULL

```c
CHKNULL(condition)
```
The CHKNULL macro is an "assert" mechanism. It causes the calling function to return NULL immediately if condition is false.

### CHKVOID

```c
CHKVOID(condition)
```

The CHKVOID macro is an "assert" mechanism. It causes the calling function to return immediately if condition is false.

### snooze

```c
void snooze(unsigned int seconds)
```
Suspends execution of the invoking task or process for the indicated number of seconds.

### microsnooze

```c
void microsnooze(unsigned int microseconds)
```

Suspends execution of the invoking task or process for the indicated number of microseconds.

### getCurrentTime

```c
void getCurrentTime(struct timeval *time)
```

Returns the current local time (ctime, i.e., Unix epoch time) in a timeval structure (see gettimeofday(3C)).

### isprintf

```c
void isprintf(char *buffer, int bufSize, char *format, ...)
```

isprintf() is a safe, portable implementation of snprintf(); see the snprintf(P) man page for details. isprintf() differs from snprintf() in that it always NULL-terminates the string in buffer, even if the length of the composed string would equal or exceed bufSize. Buffer overruns are reported by log message; unlike snprintf(), isprintf() returns void.

### istrlen

```c
size_t istrlen(const char *sourceString, size_t maxlen)
```

istrlen() is a safe implementation of strlen(); see the strlen(3) man page for details. istrlen() differs from strlen() in that it takes a second argument, the maximum valid length of sourceString. The function returns the number of non-NULL characters in sourceString preceding the first NULL character in sourceString, provided that a NULL character appears somewhere within the first maxlen characters of sourceString; otherwise it returns maxlen.

### istrcpy

```c
char *istrcpy(char *buffer, char *sourceString, int bufSize)
```

istrcpy() is a safe implementation of strcpy(); see the strcpy(3) man page for details. istrcpy() differs from strcpy() in that it takes a third argument, the total size of the buffer into which sourceString is to be copied. istrcpy() always NULL-terminates the string in buffer, even if the length of sourceString string would equal or exceed bufSize (in which case sourceString is truncated to fit within the buffer).

### istrcat

```c
char *istrcat(char *buffer, char *sourceString, int bufSize)
```

istrcat() is a safe implementation of strcat(); see the strcat(3) man page for details. istrcat() differs from strcat() in that it takes a third argument, the total size of the buffer for the string that is being aggregated. istrcat() always NULL-terminates the string in buffer, even if the length of sourceString string would equal or exceed the sum of bufSize and the length of the string currently occupying the buffer (in which case sourceString is truncated to fit within the buffer).

### igetcwd

```c
char *igetcwd(char *buf, size_t size)
```

igetcwd() is normally just a wrapper around getcwd(3). It differs from getcwd(3) only when FSWWDNAME is defined, in which case the implementation of igetcwd() must be supplied in an included file named "wdname.c"; this adaptation option accommodates flight software environments in which the current working directory name must be configured rather than discovered at run time.

### isignal

```c
void isignal(int signbr, void (*handler)(int))
```

isignal() is a portable, simplified interface to signal handling that is functionally indistinguishable from signal(P). It assures that reception of the indicated signal will interrupt system calls in SVR4 fashion, even when running on a FreeBSD platform.

### iblock

```c
void iblock(int signbr)
```

iblock() simply prevents reception of the indicated signal by the calling thread. It provides a means of controlling which of the threads in a process will receive the signal cited in an invocation of isignal().

### ifopen

```c
int ifopen(const char *fileName, int flags, int pmode)
```

ifopen() is a portable function for opening "regular" files. It operates in exactly the same way as open() except that it fails (returning -1) if fileName does not identify a regular file, i.e., it's a directory, a named pipe, etc.

NOTE that ION also provides iopen() which is nothing more than a portable wrapper for open(). iopen() can be used to open a directory, for example.

### igets

```c
char *igets(int fd, char *buffer, int buflen, int *lineLen)
```

igets() reads a line of text, delimited by a newline character, from fd into buffer and writes a NULL character at the end of the string. The newline character itself is omitted from the NULL-terminated text line in buffer; if the newline is immediately preceded by a carriage return character (i.e., the line is from a DOS text file), then the carriage return character is likewise omitted from the NULL-terminated text line in buffer. End of file is interpreted as an implicit newline, terminating the line. If the number of characters preceding the newline is greater than or equal to buflen, only the first (buflen - 1) characters of the line are written into buffer. On error the function sets *lineLen to -1 and returns NULL. On reading end-of-file, the function sets *lineLen to zero and returns NULL. Otherwise the function sets *lineLen to the length of the text line in buffer, as if from strlen(3), and returns buffer.

### iputs

```c
int iputs(int fd, char *string)
```

iputs() writes to fd the NULL-terminated character string at string. No terminating newline character is appended to string by iputs(). On error the function returns -1; otherwise the function returns the length of the character string written to fd, as if from strlen(3).

### strtovast

```c
vast strtovast(char *string)
```

Converts the leading characters of string, skipping leading white space and ending at the first subsequent character that can't be interpreted as contributing to a numeric value, to a vast integer and returns that integer.

### strtouvast

```c
uvast strtouvast(char *string)
```

Same as strtovast() except the result is an unsigned vast integer value.

### findToken

```c
void findToken(char **cursorPtr, char **token)
```

Locates the next non-whitespace lexical token in a character array, starting at *cursorPtr. The function NULL-terminates that token within the array and places a pointer to the token in *token. Also accommodates tokens enclosed within matching single quotes, which may contain embedded spaces and escaped single-quote characters. If no token is found, *token contains NULL on return from this function.

### acquireSystemMemory

```c
void *acquireSystemMemory(size_t size)
```

Uses memalign() to allocate a block of system memory of length size, starting at an address that is guaranteed to be an integral multiple of the size of a pointer to void, and initializes the entire block to binary zeroes. Returns the starting address of the allocated block on success; returns NULL on any error.

### createFile

```c
int createFile(const char *name, int flags)
```

Creates a file of the indicated name, using the indicated file creation flags. This function provides common file creation functionality across VxWorks and Unix platforms, invoking creat() under VxWorks and open() elsewhere. For return values, see creat(2) and open(2).

### getInternetAddress

```c
unsigned int getInternetAddress(char *hostName)
```

Returns the IP address of the indicated host machine, or zero if the address cannot be determined.

### getInternetHostName

```c
char *getInternetHostName(unsigned int hostNbr, char *buffer)
```

Writes the host name of the indicated host machine into buffer and returns buffer, or returns NULL on any error. The size of buffer should be (MAXHOSTNAMELEN + 1).

### getNameOfHost

```c
int getNameOfHost(char *buffer, int bufferLength)
```

Writes the first (bufferLength - 1) characters of the host name of the local machine into buffer. Returns 0 on success, -1 on any error.

### getAddressOfHost

```c
unsigned int getAddressOfHost()
```

Returns the IP address for the host name of the local machine, or 0 on any error.

### parseSocketSpec

```c
void parseSocketSpec(char *socketSpec, unsigned short *portNbr, unsigned int *hostNbr)
```

Parses socketSpec, extracting host number (IP address) and port number from the string. socketSpec is expected to be of the form "{ @ | hostname }[:<portnbr>]", where @ signifies "the host name of the local machine". If host number can be determined, writes it into *hostNbr; otherwise writes 0 into *hostNbr. If port number is supplied and is in the range 1024 to 65535, writes it into *portNbr; otherwise writes 0 into *portNbr.

### printDottedString

```c
void printDottedString(unsigned int hostNbr, char *buffer)
```

Composes a dotted-string (xxx.xxx.xxx.xxx) representation of the IPv4 address in hostNbr and writes that string into buffer. The length of buffer must be at least 16.

### getNameOfUser

```c
char *getNameOfUser(char *buffer)
```

Writes the user name of the invoking task or process into buffer and returns buffer. The size of buffer must be at least L_cuserid, a constant defined in the stdio.h header file. Returns buffer.

### reUseAddress

```c
int reUseAddress(int fd)
```

Makes the address that is bound to the socket identified by fd reusable, so that the socket can be closed and immediately reopened and re-bound to the same port number. Returns 0 on success, -1 on any error.

### makeIoNonBlocking

```c
int makeIoNonBlocking(int fd)
```

Makes I/O on the socket identified by fd non-blocking; returns -1 on failure. An attempt to read on a non-blocking socket when no data are pending, or to write on it when its output buffer is full, will not block; it will instead return -1 and cause errno to be set to EWOULDBLOCK.

### watchSocket

```c
int watchSocket(int fd)
```

Turns on the "linger" and "keepalive" options for the socket identified by fd. See socket(2) for details. Returns 0 on success, -1 on any failure.

### closeOnExec

```c
void closeOnExec(int fd)
```

Ensures that fd will NOT be open in any child process fork()ed from the invoking process. Has no effect on a VxWorks platform.

-------------------

## Exception Reporting

The functions in this section offer platform-independent capabilities for reporting on processing exceptions.

The underlying mechanism for ICI's exception reporting is a pair of functions that record error messages in a privately managed pool of static memory. These functions -- `postErrmsg()` and `postSysErrmsg()` -- are designed to return very rapidly with no possibility of failing, themselves. Nonetheless they are not safe to call from an interrupt service routing (ISR). Although each merely copies its text to the next available location in the error message memory pool, that pool is protected by a mutex; multiple processes might be queued up to take that mutex, so the total time to execute the function is non-deterministic.

Built on top of `postErrmsg()` and `postSysErrmsg()` are the `putErrmsg()` and `putSysErrmsg()` functions, which may take longer to return. Each one simply calls the corresponding "post" function but then calls the `writeErrmsgMemos()` function, which calls `writeMemo()` to print (or otherwise deliver) each message currently posted to the pool and then destroys all of those posted messages, emptying the pool.

Recommended general policy on using the ICI exception reporting functions (which the functions in the ION distribution libraries are supposed to adhere to) is as follows:


    In the implementation of any ION library function or any ION
    task's top-level driver function, any condition that prevents
    the function from continuing execution toward producing the
    effect it is designed to produce is considered an "error".

    Detection of an error should result in the printing of an
    error message and, normally, the immediate return of whatever
    return value is used to indicate the failure of the function
    in which the error was detected.  By convention this value
    is usually -1, but both zero and NULL are appropriate
    failure indications under some circumstances such as object
    creation.

    The CHKERR, CHKZERO, CHKNULL, and CHKVOID macros are used to
    implement this behavior in a standard and lexically terse
    manner.  Use of these macros offers an additional feature:
    for debugging purposes, they can easily be configured to
    call sm_Abort() to terminate immediately with a core dump
    instead of returning a error indication.  This option is
    enabled by setting the compiler parameter CORE_FILE_NEEDED
    to 1 at compilation time.

    In the absence of either any error, the function returns a
    value that indicates nominal completion.  By convention this
    value is usually zero, but under some circumstances other
    values (such as pointers or addresses) are appropriate
    indications of nominal completion.  Any additional information
    produced by the function, such as an indication of "success",
    is usually returned as the value of a reference argument.
    [Note, though, that database management functions and the
    SDR hash table management functions deviate from this rule:
    most return 0 to indicate nominal completion but functional
    failure (e.g., duplicate key or object not found) and return
    1 to indicate functional success.]

    So when returning a value that indicates nominal completion
    of the function -- even if the result might be interpreted
    as a failure at a higher level (e.g., an object identified
    by a given string is not found, through no failure of the
    search function) -- do NOT invoke putErrmsg().

    Use putErrmsg() and putSysErrmsg() only when functions are
    unable to proceed to nominal completion.  Use writeMemo()
    or writeMemoNote() if you just want to log a message.

    Whenever returning a value that indicates an error:

            If the failure is due to the failure of a system call
            or some other non-ION function, assume that errno
            has already been set by the function at the lowest
            layer of the call stack; use putSysErrmsg (or
            postSysErrmsg if in a hurry) to describe the nature
            of the activity that failed.  The text of the error
            message should normally start with a capital letter
            and should NOT end with a period.

            Otherwise -- i.e., the failure is due to a condition
            that was detected within ION -- use putErrmsg (or
            postErrmg if pressed for time) to describe the nature
            of the failure condition.  This will aid in tracing
            the failure through the function stack in which the
            failure was detected.  The text of the error message
            should normally start with a capital letter and should
            end with a period.

    When a failure in a called function is reported to "driver"
    code in an application program, before continuing or exiting
    use writeErrmsgMemos() to empty the message pool and print a
    simple stack trace identifying the failure.

### system_error_msg() ###

```c
char *system_error_msg( )
```

Returns a brief text string describing the current system error, as identified by the current value of errno.

### setLogger ###

```c
void setLogger(Logger usersLoggerName)
```

Sets the user function to be used for writing messages to a user-defined "log" medium. The logger function's calling sequence must match the following prototype:

    void    usersLoggerName(char *msg);

The default Logger function simply writes the message to standard output.

### writeMemo ###


```c
void writeMemo(char *msg)
```

Writes one log message, using the currently defined message logging function. To construct a more complex string, it is customary and safer to use the isprintf function to build a message string first, and then pass that string as an argument to writeMemo.

### writeMemoNote ###

```c
void writeMemoNote(char *msg, char *note)
```

Writes a log message like writeMemo(), accompanied by the user-supplied context-specific text string in note. The text string can also be build separately using isprintf().

### writeErrMemo ###

```c
void writeErrMemo(char *msg)
```

Writes a log message like writeMemo(), accompanied by text describing the current system error.

### itoa ###

```c
char *itoa(int value)
```

Returns a string representation of the signed integer in value, nominally for immediate use as an argument to putErrmsg(). [Note that the string is constructed in a static buffer; this function is not thread-safe.]

### utoa ###

```c
char *utoa(unsigned int value)
```

Returns a string representation of the unsigned integer in value, nominally for immediate use as an argument to putErrmsg(). [Note that the string is constructed in a static buffer; this function is not thread-safe.]

### postErrmsg ###

```c
void postErrmsg(char *text, char *argument)
```

Constructs an error message noting the name of the source file containing the line at which this function was called, the line number, the text of the message, and -- if not NULL -- a single textual argument that can be used to give more specific information about the nature of the reported failure (such as the value of one of the arguments to the failed function). The error message is appended to the list of messages in a privately managed pool of static memory, ERRMSGS_BUFSIZE bytes in length.

If text is NULL or is a string of zero length or begins with a newline character (i.e., *text == '\0' or '\n'), the function returns immediately and no error message is recorded.

The errmsgs pool is designed to be large enough to contain error messages from all levels of the calling stack at the time that an error is encountered. If the remaining unused space in the pool is less than the size of the new error message, however, the error message is silently omitted. In this case, provided at least two bytes of unused space remain in the pool, a message comprising a single newline character is appended to the list to indicate that a message was omitted due to excessive length.

### postSysErrmsg ###

```c
void postSysErrmsg(char *text, char *arg)
```

Like postErrmsg() except that the error message constructed by the function additionally contains text describing the current system error. text is truncated as necessary to assure that the sum of its length and that of the description of the current system error does not exceed 1021 bytes.

### getErrmsg ###

```c
int getErrmsg(char *buffer)
```

Copies the oldest error message in the message pool into buffer and removes that message from the pool, making room for new messages. Returns zero if the message pool cannot be locked for update or there are no more messages in the pool; otherwise returns the length of the message copied into buffer. Note that, for safety, the size of buffer should be ERRMSGS_BUFSIZE.

Note that a returned error message comprising only a single newline character always signifies an error message that was silently omitted because there wasn't enough space left on the message pool to contain it.

### writeErrmsgMemos ###

```c
void writeErrmsgMemos( )
```

Calls getErrmsg() repeatedly until the message pool is empty, using writeMemo() to log all the messages in the pool. Messages that were omitted due to excessive length are indicated by logged lines of the form "[message omitted due to excessive length]".

### putErrmsg ###

```c
void putErrmsg(char *text, char *argument)
```

The putErrmsg() function merely calls postErrmsg() and then writeErrmsgMemos().

### putSysErrmsg ###

```c
void putSysErrmsg(char *text, char *arg)
```

The putSysErrmsg() function merely calls postSysErrmsg() and then writeErrmsgMemos().

### discardErrmsgs ###

```c
void discardErrmsgs( )
```

Calls getErrmsg() repeatedly until the message pool is empty, discarding all of the messages.

### printStackTrace ###


```c
void printStackTrace( )
```

On Linux machines only, uses writeMemo() to print a trace of the process's current execution stack, starting with the lowest level of the stack and proceeding to the main() function of the executable.

Note that (a) printStackTrace() is only implemented for Linux platforms at this time; (b) symbolic names of functions can only be printed if the -rdynamic flag was enabled when the executable was linked; (c) only the names of non-static functions will appear in the stack trace.

For more complete information about the state of the executable at the time the stack trace snapshot was taken, use the Linux addr2line tool. To do this, cd into a directory in which the executable file resides (such as /opt/bin) and submit an addr2line command as follows:

`addr2line -e name_of_executable stack_frame_address`
where both name_of_executable and stack_frame_address are taken from one of the lines of the printed stack trace. addr2line will print the source file name and line number for that stack frame.