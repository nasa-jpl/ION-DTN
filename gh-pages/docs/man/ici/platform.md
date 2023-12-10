# NAME

platform - C software portability definitions and functions

# SYNOPSIS

    #include "platform.h"

    [see description for available functions]

# DESCRIPTION

_platform_ is a library of functions that simplify the porting of
software written in C.  It provides an API that enables application 
code to access the resources of an abstract POSIX-compliant
"least common denominator" operating system -- typically a large
subset of the resources of the actual underlying operating system.

Most of the functionality provided by the platform library is
aimed at making communication code portable: common functions for
shared memory, semaphores, and IP sockets are provided.  
The implementation of the abstract O/S API varies according
to the actual operating system on which the application runs, but
the API's behavior is always the same; applications that invoke
the platform library functions rather than native O/S system
calls may forego some O/S-specific capability, but they gain portability 
at little if any cost in performance.

Differences in word size among platforms are implemented by values
of the _SPACE\_ORDER_ macro.  "Space order" is the base 2 log of the
number of octets in a word: for 32-bit machines the space order is
2 (2^2 = 4 octets per word), for 64-bit machines it is 3 (2^3 = 8
octets per word).

A consistent platform-independent representation of large integers is
useful for some applications.  For this purpose, _platform_ defines
new types **vast** and **uvast** (unsigned vast) which are consistently
defined to be 64-bit integers regardless of the platform's native word
size.

The platform.h header file #includes many of the most frequently
needed header files: sys/types.h, errno.h, string.h, stdio.h,
sys/socket.h, signal.h, dirent.h, netinet/in.h, unistd.h,
stdlib.h, sys/time.h, sys/resource.h, malloc.h, sys/param.h,
netdb.h, sys/uni.h, and fcntl.h.  Beyond this, _platform_ attempts 
to enhance compatibility by providing standard macros,
type definitions, external references, or function implementations 
that are missing from a few supported O/S's but supported
by all others.  Finally, entirely new, generic functions are provided 
to establish a common body of functionality that subsumes
significantly different O/S-specific capabilities.

## PLATFORM COMPATIBILITY PATCHES

The platform library "patches" the APIs of supported O/S's to
guarantee that all of the following items may be utilized by application 
software:

    The strchr(), strrchr(), strcasecmp(), and strncasecmp() functions.

    The unlink(), getpid(), and gettimeofday() functions.

    The select() function.

    The FD_BITMAP macro (used by select()).

    The MAXHOSTNAMELEN macro.

    The NULL macro.

    The timer_t type definition.

## PLATFORM GENERIC MACROS AND FUNCTIONS

The generic macros and functions in this section may be used in
place of comparable O/S-specific functions, to enhance the portability 
of code.  (The implementations of these macros and functions are 
no-ops in environments in which they are inapplicable,
so they're always safe to call.)

- FDTABLE\_SIZE

    The FDTABLE\_SIZE macro returns the total number of file
    descriptors defined for the process (or VxWorks target).

- ION\_PATH\_DELIMITER

    The ION\_PATH\_DELIMITER macro returns the ASCII character -- either '/' or
    '\\' -- that is used as a directory name delimiter in path names for the
    file system used by the local platform.

- oK(expression)

    The oK macro simply casts the value of _expression_ to void, a way of
    handling function return codes that are not meaningful in this context.

- CHKERR(condition)

    The CHKERR macro is an "assert" mechanism.  It causes the calling function
    to return -1 immediately if _condition_ is false. 

- CHKZERO(condition)

    The CHKZERO macro is an "assert" mechanism.  It causes the calling function
    to return 0 immediately if _condition_ is false. 

- CHKNULL(condition)

    The CHKNULL macro is an "assert" mechanism.  It causes the calling function
    to return NULL immediately if _condition_ is false. 

- CHKVOID(condition)

    The CHKVOID macro is an "assert" mechanism.  It causes the calling function
    to return immediately if _condition_ is false. 

- void snooze(unsigned int seconds)

    Suspends execution of the invoking task or process for the indicated 
    number of seconds.

- void microsnooze(unsigned int microseconds)

    Suspends execution of the invoking task or process for
    the indicated number of microseconds.

- void getCurrentTime(struct timeval \*time)

    Returns the current local time (ctime, i.e., Unix epoch time) in a timeval
    structure (see gettimeofday(3C)).

- void isprintf(char \*buffer, int bufSize, char \*format, ...)

    isprintf() is a safe, portable implementation of snprintf(); see the
    snprintf(P) man page for details.  isprintf() differs from snprintf() in that
    it always NULL-terminates the string in _buffer_, even if the length of the
    composed string would equal or exceed _bufSize_.  Buffer overruns are
    reported by log message; unlike snprintf(), isprintf() returns void.

- size\_t istrlen(const char \*sourceString, size\_t maxlen)

    istrlen() is a safe implementation of strlen(); see the strlen(3) man 
    page for details.  istrlen() differs from strlen() in that it takes a second
    argument, the maximum valid length of _sourceString_.  The function
    returns the number of non-NULL characters in _sourceString_ preceding
    the first NULL character in _sourceString_, provided that a NULL
    character appears somewhere within the first _maxlen_ characters of
    _sourceString_; otherwise it returns _maxlen_.

- char \*istrcpy(char \*buffer, char \*sourceString, int bufSize)

    istrcpy() is a safe implementation of strcpy(); see the strcpy(3) man
    page for details.  istrcpy() differs from strcpy() in that it takes a
    third argument, the total size of the buffer into which _sourceString_
    is to be copied.  istrcpy() always NULL-terminates the string in _buffer_,
    even if the length of _sourceString_ string would equal or exceed
    _bufSize_ (in which case _sourceString_ is truncated to fit within
    the buffer).

- char \*istrcat(char \*buffer, char \*sourceString, int bufSize)

    istrcat() is a safe implementation of strcat(); see the strcat(3) man
    page for details.  istrcat() differs from strcat() in that it takes a
    third argument, the total size of the buffer for the string that is being
    aggregated. istrcat() always NULL-terminates the string in _buffer_, even
    if the length of _sourceString_ string would equal or exceed the sum of
    _bufSize_ and the length of the string currently occupying the buffer
    (in which case _sourceString_ is truncated to fit within the buffer).

- char \*igetcwd(char \*buf, size\_t size)

    igetcwd() is normally just a wrapper around getcwd(3).  It differs from
    getcwd(3) only when FSWWDNAME is defined, in which case the implementation
    of igetcwd() must be supplied in an included file named "wdname.c"; this
    adaptation option accommodates flight software environments in which the
    current working directory name must be configured rather than discovered
    at run time.

- void isignal(int signbr, void (\*handler)(int))

    isignal() is a portable, simplified interface to signal handling that is
    functionally indistinguishable from signal(P).  It assures that reception
    of the indicated signal will interrupt system calls in SVR4 fashion, even
    when running on a FreeBSD platform.

- void iblock(int signbr)

    iblock() simply prevents reception of the indicated signal by the calling
    thread.  It provides a means of controlling which of the threads in a process
    will receive the signal cited in an invocation of isignal().

- int ifopen(const char \*fileName, int flags, int pmode)

    ifopen() is a portable function for opening "regular" files.  It operates
    in exactly the same way as open() except that it fails (returning -1) if 
    _fileName_ does not identify a regular file, i.e., it's a directory, a
    named pipe, etc.

    **NOTE** that ION also provides iopen() which is nothing more than a
    portable wrapper for open().  iopen() can be used to open a directory, for
    example.

- char \*igets(int fd, char \*buffer, int buflen, int \*lineLen)

    igets() reads a line of text, delimited by a newline character, from _fd_
    into _buffer_ and writes a NULL character at the end of the string.  The
    newline character itself is omitted from the NULL-terminated text line in
    _buffer_; if the newline is immediately preceded by a carriage return
    character (i.e., the line is from a DOS text file), then the carriage return
    character is likewise omitted from the NULL-terminated text line in
    _buffer_.  End of file is interpreted as an implicit newline, terminating
    the line.  If the number of characters preceding the newline is greater
    than or equal to _buflen_, only the first (_buflen_ - 1) characters of
    the line are written into _buffer_.  On error the function sets _\*lineLen_
    to -1 and returns NULL.  On reading end-of-file, the function sets _\*lineLen_
    to zero and returns NULL.  Otherwise the function sets _\*lineLen_ to the
    length of the text line in _buffer_, as if from strlen(3), and returns
    _buffer_.

- int iputs(int fd, char \*string)

    iputs() writes to _fd_ the NULL-terminated character string at _string_.  No
    terminating newline character is appended to _string_ by iputs().  On error
    the function returns -1; otherwise the function returns the length of the
    character string written to _fd_, as if from strlen(3).

- vast strtovast(char \*string)

    Converts the leading characters of _string_, skipping leading white space
    and ending at the first subsequent character that can't be interpreted as
    contributing to a numeric value, to a **vast** integer and returns that integer.

- uvast strtouvast(char \*string)

    Same as strtovast() except the result is an unsigned **vast** integer value.

- void findToken(char \*\*cursorPtr, char \*\*token)

    Locates the next non-whitespace lexical token in a character array, starting
    at _\*cursorPtr_.  The function NULL-terminates that token within the array
    and places a pointer to the token in _\*token_.  Also accommodates tokens
    enclosed within matching single quotes, which may contain embedded spaces
    and escaped single-quote characters.  If no token is found, _\*token_ contains
    NULL on return from this function.

- void \*acquireSystemMemory(size\_t size)

    Uses memalign() to allocate a block of system memory of length _size_,
    starting at an address that is guaranteed to be an integral multiple of
    the size of a pointer to void, and initializes the entire block to binary
    zeroes.  Returns the starting address of the allocated block on success;
    returns NULL on any error.

- int createFile(const char \*name, int flags)

    Creates a file of the indicated name, using the indicated file creation flags.
    This function provides common file creation functionality across VxWorks and
    Unix platforms, invoking creat() under VxWorks and open() elsewhere.  For
    return values, see creat(2) and open(2).

- unsigned int getInternetAddress(char \*hostName)

    Returns the IP address of the indicated host machine, or zero if the
    address cannot be determined.

- char \*getInternetHostName(unsigned int hostNbr, char \*buffer)

    Writes the host name of the indicated host machine into _buffer_ and
    returns _buffer_, or returns NULL on any error.  The size of _buffer_
    should be (MAXHOSTNAMELEN + 1).

- int getNameOfHost(char \*buffer, int bufferLength)

    Writes the first (_bufferLength_ - 1) characters of the
    host name of the local machine into _buffer_.  Returns 0 on success, -1 on
    any error.

- unsigned int getAddressOfHost()

    Returns the IP address for the host name of the local machine, or 0 on any
    error.

- void parseSocketSpec(char \*socketSpec, unsigned short \*portNbr, unsigned int \*hostNbr)

    Parses _socketSpec_, extracting host number (IP address) and port number from
    the string.  _socketSpec_ is expected to be of the form
    "{ @ | hostname }\[:&lt;portnbr>\]", where @ signifies "the host name of the
    local machine".  If host number can be determined, writes it into _\*hostNbr_;
    otherwise writes 0 into _\*hostNbr_.  If port number is supplied and
    is in the range 1024 to 65535, writes it into _\*portNbr_; otherwise writes
    0 into _\*portNbr_.

- void printDottedString(unsigned int hostNbr, char \*buffer)

    Composes a dotted-string (xxx.xxx.xxx.xxx) representation of the IPv4 address
    in _hostNbr_ and writes that string into _buffer_.  The length of _buffer_
    must be at least 16.

- char \*getNameOfUser(char \*buffer)

    Writes the user name of the invoking task or process
    into _buffer_ and returns _buffer_.  The size of _buffer_
    must be at least _L\_cuserid_, a constant defined in the
    stdio.h header file.  Returns _buffer_.

- int reUseAddress(int fd)

    Makes the address that is bound to the socket identified by 
    _fd_ reusable, so that the socket can be closed
    and immediately reopened and re-bound to the same port number.
    Returns 0 on success, -1 on any error.

- int makeIoNonBlocking(int fd)

    Makes I/O on the socket identified by _fd_ non-blocking; returns -1 on
    failure.  An attempt to read on a non-blocking socket when no data are pending, 
    or to write on it when its output buffer is full, will not block; 
    it will instead return -1 and cause errno to be set to EWOULDBLOCK.

- int watchSocket(int fd)

    Turns on the "linger" and "keepalive" options for the
    socket identified by _fd_.  See socket(2) for details.  Returns 0 on
    success, -1 on any failure.

- void closeOnExec(int fd)

    Ensures that _fd_ will NOT be open in any child process
    fork()ed from the invoking process.  Has no effect on a VxWorks platform.

## EXCEPTION REPORTING

The functions in this section offer platform-independent capabilities
for reporting on processing exceptions.

The underlying mechanism for ICI's exception reporting is a pair of
functions that record error messages in a privately managed pool of
static memory.  These functions -- postErrmsg() and postSysErrmsg() --
are designed to return very rapidly with no possibility of failing,
themselves.  Nonetheless they are not safe to call from an interrupt
service routing (ISR).  Although each merely copies its text to the
next available location in the error message memory pool, that pool
is protected by a mutex; multiple processes might be queued up to
take that mutex, so the total time to execute the function is
non-deterministic.

Built on top of postErrmsg() and postSysErrmsg() are the putErrmsg()
and putSysErrmsg() functions, which may take longer to return.  Each
one simply calls the corresponding "post" function but then calls the
writeErrmsgMemos() function, which calls writeMemo() to print (or
otherwise deliver) each message currently posted to the pool and
then destroys all of those posted messages, emptying the pool.

Recommended general policy on using the ICI exception reporting functions
(which the functions in the ION distribution libraries are supposed to
adhere to) is as follows:

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

- char \*system\_error\_msg( )

    Returns a brief text string describing the current system error, as identified
    by the current value of errno.

- void setLogger(Logger usersLoggerName)

    Sets the user function to be used for writing messages to a user-defined "log"
    medium.  The logger function's calling sequence must match the following
    prototype:

            void    usersLoggerName(char *msg);

    The default Logger function simply writes the message to standard output.

- void writeMemo(char \*msg)

    Writes one log message, using the currently defined message logging function.

- void writeMemoNote(char \*msg, char \*note)

    Writes a log message like writeMemo(), accompanied by the user-supplied
    context-specific text in _note_.

- void writeErrMemo(char \*msg)

    Writes a log message like writeMemo(), accompanied by text describing the
    current system error.

- char \*itoa(int value)

    Returns a string representation of the signed integer in _value_, nominally
    for immediate use as an argument to putErrmsg().  \[Note that the string is
    constructed in a static buffer; this function is not thread-safe.\]

- char \*utoa(unsigned int value)

    Returns a string representation of the unsigned integer in _value_, nominally
    for immediate use as an argument to putErrmsg().  \[Note that the string is
    constructed in a static buffer; this function is not thread-safe.\]

- void postErrmsg(char \*text, char \*argument)

    Constructs an error message noting the name of the source file containing
    the line at which this function was called, the line number, the _text_ of
    the message, and -- if not NULL -- a single textual _argument_ that can be
    used to give more specific information about the nature of the reported
    failure (such as the value of one of the arguments to the failed
    function).  The error message is appended to the list of messages in
    a privately managed pool of static memory, ERRMSGS\_BUFSIZE bytes in length.

    If _text_ is NULL or is a string of zero length or begins with a newline
    character (i.e., _\*text_ == '\\0' or '\\n'), the function returns immediately
    and no error message is recorded.

    The errmsgs pool is designed to be large enough to contain error messages
    from all levels of the calling stack at the time that an error is
    encountered.  If the remaining unused space in the pool is less than
    the size of the new error message, however, the error message is silently
    omitted.  In this case, provided at least two bytes of unused space remain
    in the pool, a message comprising a single newline character is appended to
    the list to indicate that a message was omitted due to excessive length.

- void postSysErrmsg(char \*text, char \*arg)

    Like postErrmsg() except that the error message constructed by the function
    additionally contains text describing the current system error.  _text_ is
    truncated as necessary to assure that the sum of its length and that of
    the description of the current system error does not exceed 1021 bytes.

- int getErrmsg(char \*buffer)

    Copies the oldest error message in the message pool into _buffer_ and
    removes that message from the pool, making room for new messages.  Returns
    zero if the message pool cannot be locked for update or there are no more
    messages in the pool; otherwise returns the length of the message copied
    into _buffer_.  Note that, for safety, the size of _buffer_ should be
    ERRMSGS\_BUFSIZE.

    Note that a returned error message comprising only a single newline character
    always signifies an error message that was silently omitted because there
    wasn't enough space left on the message pool to contain it.

- void writeErrmsgMemos( )

    Calls getErrmsg() repeatedly until the message pool is empty, using
    writeMemo() to log all the messages in the pool.  Messages that were
    omitted due to excessive length are indicated by logged lines of the
    form "\[message omitted due to excessive length\]".

- void putErrmsg(char \*text, char \*argument)

    The putErrmsg() function merely calls postErrmsg() and then
    writeErrmsgMemos().

- void putSysErrmsg(char \*text, char \*arg)

    The putSysErrmsg() function merely calls postSysErrmsg() and then
    writeErrmsgMemos().

- void discardErrmsgs( )

    Calls getErrmsg() repeatedly until the message pool is empty, discarding all
    of the messages.

- void printStackTrace( )

    On Linux machines only, uses writeMemo() to print a trace of the process's
    current execution stack, starting with the lowest level of the stack and
    proceeding to the main() function of the executable.

    Note that (a) printStackTrace() is **only** implemented for Linux platforms
    at this time; (b) symbolic names of functions can only be printed if the
    _-rdynamic_ flag was enabled when the executable was linked; (c) only the
    names of non-static functions will appear in the stack trace.

    For more complete information about the state of the executable at the time
    the stack trace snapshot was taken, use the Linux _addr2line_ tool. To do
    this, cd into a directory in which the executable file resides (such as
    /opt/bin) and submit an addr2line command as follows:

    >     addr2line -e _name\_of\_executable_ _stack\_frame\_address_

    where both _name\_of\_executable_ and _stack\_frame\_address_ are taken from
    one of the lines of the printed stack trace.  addr2line will print the source
    file name and line number for that stack frame.

## WATCH CHARACTERS

The functions in this section offer platform-independent capabilities
for recording "watch" characters indicating the occurrence of protocol
events.  See bprc(5), ltprc(5), cfdprc(5), etc. for details of the
watch character production options provided by the protocol packages.

- void setWatcher(Watcher usersWatcherName)

    Sets the user function to be used for recording watch characters to a
    user-defined "watch" medium.  The watcher function's calling sequence
    must match the following prototype:

            void    usersWatcherName(char token);

    The default Watcher function simply writes the token to standard output.

- void iwatch(char token)

    Records one "watch" character, using the currently defined watch character
    recording function.

## SELF-DELIMITING NUMERIC VALUES (SDNV)

The functions in this section encode and decode SDNVs, portable variable-length
numeric variables that expand to whatever size is necessary to contain the
values they contain.  SDNVs are used extensively in the BP and LTP libraries.

- void encodeSdnv(Sdnv \*sdnvBuffer, uvast value)

    Determines the number of octets of SDNV text needed to contain the value,
    places that number in the _length_ field of the SDNV buffer, and encodes
    the value in SDNV format into the first _length_ octets of the _text_ field
    of the SDNV buffer.

- int decodeSdnv(uvast \*value, unsigned char \*sdnvText)

    Determines the length of the SDNV located at _sdnvText_ and returns this
    number after extracting the SDNV's value from those octets and storing it
    in _value_.  Returns 0 if the encoded number value will not fit into an
    unsigned vast integer.

## ARITHMETIC ON LARGE INTEGERS (SCALARS)

The functions in this section perform simple arithmetic operations on
unsigned Scalar objects -- structures encapsulating large positive
integers in a machine-independent way.  Each Scalar comprises two
integers, a count of units \[ranging from 0 to (2^30 - 1), i.e., up
to 1 gig\] and a count of gigs \[ranging from 0 to (2^31 -1)\].  A
Scalar can represent a numeric value up to 2 billion billions,
i.e., 2 million trillions.

- void loadScalar(Scalar \*scalar, signed int value)

    Sets the value of _scalar_ to the absolute value of _value_.

- void increaseScalar(Scalar \*scalar, signed int value)

    Adds to _scalar_ the absolute value of _value_.

- void reduceScalar(Scalar \*scalar, signed int value)

    Adds to _scalar_ the absolute value of _value_.

- void multiplyScalar(Scalar \*scalar, signed int value)

    Multiplies _scalar_ by the absolute value of _value_.

- void divideScalar(Scalar \*scalar, signed int value)

    Divides _scalar_ by the absolute value of _value_.

- void copyScalar(Scalar \*to, Scalar \*from)

    Copies the value of _from_ into _to_.

- void addToScalar(Scalar \*scalar, Scalar \*increment)

    Adds _increment_ (a Scalar rather than a C integer) to _scalar_.

- void subtractFromScalar(Scalar \*scalar, Scalar \*decrement)

    Subtracts _decrement_ (a Scalar rather than a C integer) from _scalar_.

- int scalarIsValid(Scalar \*scalar)

    Returns 1 if the arithmetic performed on _scalar_ has not resulted in
    overflow or underflow.

- int scalarToSdnv(Sdnv \*sdnv, Scalar \*scalar)

    If _scalar_ points to a valid Scalar, stores the value of _scalar_ in
    _sdnv_; otherwise sets the length of _sdnv_ to zero.

- int sdnvToScalar(Scalar \*scalar, unsigned char \*sdnvText)

    If _sdnvText_ points to a sequence of bytes that, when interpreted as
    the text of an Sdnv, has a value that can be represented in a 61-bit
    unsigned binary integer, then this function stores that value in _scalar_
    and returns the detected Sdnv length.  Otherwise returns zero.

    Note that Scalars and Sdnvs are both representations of potentially large
    unsigned integer values.  Any Scalar can alternatively be represented as
    an Sdnv.  However, it is possible for a valid Sdnv to be too large to
    represent in a Scalar.

## PRIVATE MUTEXES

The functions in this section provide platform-independent management of
mutexes for synchronizing operations of threads or tasks in a common private
address space.

- int initResourceLock(ResourceLock \*lock)

    Establishes an inter-thread lock for use in locking some resource.  Returns
    0 if successful, -1 if not.

- void killResourceLock(ResourceLock \*lock)

    Deletes the resource lock referred to by _lock_.

- void lockResource(ResourceLock \*lock)

    Checks the state of _lock_.  If the lock is already
    owned by a different thread, the call blocks until the
    other thread relinquishes the lock.  If the lock is
    unowned, it is given to the current thread and the lock
    count is set to 1.  If the lock is already owned by
    this thread, the lock count is incremented by 1.

- void unlockResource(ResourceLock \*lock)

    If called by the current owner of _lock_, decrements _lock_'s
    lock count by 1; if zero, relinquishes the lock so it may be
    taken by other threads.  Care must be taken to make sure that one, and
    only one, unlockResource() call is issued for each
    lockResource() call issued on a given resource lock.

## SHARED MEMORY IPC DEVICES

The functions in this section provide platform-independent management of
IPC mechanisms for synchronizing operations of threads, tasks, or processes
that may occupy different address spaces but share access to a common system
(nominally, processor) memory.

_NOTE_ that this is distinct from the VxWorks "VxMP" capability enabling
tasks to share access to bus memory or dual-ported board memory from multiple
processors.  The "platform" system will support IPC devices that 
utilize this capability at some time in the future, but that support is
not yet implemented.

- int sm\_ipc\_init( )

    Acquires and initializes shared-memory IPC management resources.  Must be
    called before any other shared-memory IPC function is called.  Returns 0
    on success, -1 on any failure.

- void sm\_ipc\_stop( )

    Releases shared-memory IPC management resources, disabling the shared-memory
    IPC functions until sm\_ipc\_init() is called again.

- int sm\_GetUniqueKey( )

    Some of the "sm\_" (shared memory) functions described
    below associate new communication objects with _key_
    values that uniquely identify them, so that different
    processes can access them independently.  Key values
    are typically defined as constants in application code.
    However, when a new communication object is required
    for which no specific need was anticipated in the application, 
    the sm\_GetUniqueKey() function can be invoked to obtain a new,
    arbitrary key value that is known not to be already in use.

- sm\_SemId sm\_SemCreate(int key, int semType)

    Creates a shared-memory semaphore that can be used to
    synchronize activity among tasks or processes residing
    in a common system memory but possibly multiple address
    spaces; returns a reference handle for that semaphore,
    or SM\_SEM\_NONE on any failure.  If _key_ refers to an existing
    semaphore, returns the handle of that semaphore.  If
    _key_ is the constant value SM\_NO\_KEY, automatically
    obtains an unused key.  On VxWorks platforms, _semType_
    determines the order in which the semaphore
    is given to multiple tasks that attempt to take it while
    it is already taken: if set to SM\_SEM\_PRIORITY then the
    semaphore is given to tasks in task priority sequence (i.e.,
    the highest-priority task waiting for it receives it when
    it is released), while otherwise (SM\_SEM\_FIFO) the semaphore
    is given to tasks in the order in which they attempted to take
    it.  On all other platforms, only SM\_SEM\_FIFO behavior is
    supported and _semType_ is ignored.

- int sm\_SemTake(sm\_SemId semId)

    Blocks until the indicated semaphore is no longer taken by any other
    task or process, then takes it.  Return 0 on success, -1 on any error.

- void sm\_SemGive(sm\_SemId semId)

    Gives the indicated semaphore, so that another task or process can take it.

- void sm\_SemEnd(sm\_SemId semId)

    This function is used to pass a termination signal to whatever task is
    currently blocked on taking the indicated semaphore, if any.  It sets
    to 1 the "ended" flag associated with this semaphore, so that a test for
    sm\_SemEnded() will return 1, and it gives the semaphore so that the
    blocked task will have an opportunity to test that flag.

- int sm\_SemEnded(sm\_SemId semId)

    This function returns 1 if the "ended" flag associated with the
    indicated semaphore has been set to 1; returns zero otherwise.  When
    the function returns 1 it also gives the semaphore so that any other
    tasks that might be pended on the same semaphore are also given an
    opportunity to test it and discover that it has been ended.

- void sm\_SemUnend(sm\_SemId semId)

    This function is used to reset an ended semaphore, so that a restarted
    subsystem can reuse that semaphore rather than delete it and allocate a
    new one.

- int sm\_SemUnwedge(sm\_SemId semId, int timeoutSeconds)

    Used to release semaphores that have been taken but never released, possibly
    because the tasks or processes that took them crashed before releasing them.
    Attempts to take the semaphore; if this attempt does not succeed within
    _timeoutSeconds_ seconds (providing time for normal processing to be
    completed, in the event that the semaphore is legitimately and temporarily
    locked by some task), the semaphore is assumed to be wedged.  In any case,
    the semaphore is then released.  Returns 0 on success, -1 on any error.

- void sm\_SemDelete(sm\_SemId semId)

    Destroys the indicated semaphore.

- sm\_SemId sm\_GetTaskSemaphore(int taskId)

    Returns the ID of the semaphore that is dedicated to the private use of the
    indicated task, or SM\_SEM\_NONE on any error.

    This function implements the concept that for each task there can
    always be one dedicated semaphore, which the task can always use for its
    own purposes, whose key value may be known a priori because the key of the
    semaphore is based on the task's ID.  The design of the function
    rests on the assumption that each task's ID, whether a VxWorks task ID
    or a Unix process ID, maps to a number that is out of the range of all
    possible key values that are arbitrarily produced by sm\_GetUniqueKey().
    For VxWorks, we assume this to be true because task ID is a pointer to
    task state in memory which we assume not to exceed 2GB; the unique key
    counter starts at 2GB.  For Unix, we assume this to be true because
    process ID is an index into a process table whose size is less than 64K;
    unique keys are formed by shifting process ID left 16 bits and adding
    the value of an incremented counter which is always greater than zero.

- int sm\_ShmAttach(int key, int size, char \*\*shmPtr, int \*id)

    Attaches to a segment of memory to which tasks or processes residing in
    a common system memory, but possibly multiple address spaces, all have
    access.

    This function registers the invoking task or process as a user of the
    shared memory segment identified by _key_.  If _key_ is the constant value 
    SM\_NO\_KEY, automatically sets _key_ to some unused key value.
    If a shared memory segment identified by _key_ already exists, then
    _size_ may be zero and the value of _\*shmPtr_ is ignored.
    Otherwise the size of the shared memory segment must be provided
    in _size_ and a new shared memory segment is created in a manner that is
    dependent on _\*shmPtr_: if _\*shmPtr_ is NULL then 
    _size_ bytes of shared memory are dynamically acquired, allocated, and
    assigned to the newly created shared memory segment; otherwise the
    memory located at _shmPtr_ is assumed to have been pre-allocated
    and is merely assigned to the newly created shared memory segment. 

    On success, stores the unique shared memory ID of the segment in _\*id_
    for possible future destruction, stores a pointer to the segment's
    assigned memory in _\*shmPtr_, and returns 1 (if the segment is newly
    created) or 0 (otherwise).  Returns -1 on any error.  

- void sm\_ShmDetach(char \*shmPtr)

    Unregisters the invoking task or process as a user of
    the shared memory starting at _shmPtr_.

- void sm\_ShmDestroy(int id)

    Destroys the shared memory segment identified by _id_, releasing any
    memory that was allocated when the segment was created.

## PORTABLE MULTI-TASKING

- int sm\_TaskIdSelf( )

    Returns the unique identifying number of the invoking task or process.

- int sm\_TaskExists(int taskId)

    Returns non-zero if a task or process identified by
    _taskId_ is currently running on the local processor, zero otherwise.

- void \*sm\_TaskVar(void \*\*arg)

    Posts or retrieves the value of the "task variable" belonging to the
    invoking task.  Each task has access to a single task variable, initialized
    to NULL, that resides in the task's private state; this can be convenient
    for passing task-specific information to a signal handler, for example.  If
    _arg_ is non-NULL, then _\*arg_ is posted as the new value of the task's
    private task variable.  In any case, the value of that task variable is
    returned.

- void sm\_TaskSuspend( )

    Indefinitely suspends execution of the invoking task or
    process.  Helpful if you want to freeze an application
    at the point at which an error is detected, then use a
    debugger to examine its state.

- void sm\_TaskDelay(int seconds)

    Same as snooze(3).

- void sm\_TaskYield( )

    Relinquishes CPU temporarily for use by other tasks.

- int sm\_TaskSpawn(char \*name, char \*arg1, char \*arg2, char
                       \*arg3, char \*arg4, char \*arg5, char \*arg6, char
                       \*arg7, char \*arg8, char \*arg9, char \*arg10, int
                       priority, int stackSize)

    Spawns/forks a new task/process, passing it up to ten
    command-line arguments.  _name_ is the name of the
    function (VxWorks) or executable image (UNIX) to be executed 
    in the new task/process.

    For UNIX, _name_ must be the name of some executable 
    program in the $PATH of the invoking process.

    For VxWorks, _name_ must be
    the name of some function named in an application-defined private
    symbol table (if PRIVATE\_SYMTAB is defined) or the system symbol
    table (otherwise).  If PRIVATE\_SYMTAB is defined, the application must
    provide a suitable adaptation of the symtab.c source file, which
    implements the private symbol table.

    "priority" and "stackSize" are ignored under UNIX.  Under VxWorks, if
    zero they default to the values in the application-defined private
    symbol table if provided, or otherwise to ICI\_PRIORITY (nominally 100)
    and 32768 respectively.

    Returns the task/process ID of the new task/process on
    success, or -1 on any error.

- void sm\_TaskKill(int taskId, int sigNbr)

    Sends the indicated signal to the indicated task or process.

- void sm\_TaskDelete(int taskId)

    Terminates the indicated task or process.

- void sm\_Abort()

    Terminates the calling task or process.  If not called while ION is in
    flight configuration, a stack trace is printed or a core file is written.

- int pseudoshell(char \*script)

    Parses _script_ into a command name and up to 10 arguments, then passes the
    command name and arguments to sm\_TaskSpawn() for execution.
    The sm\_TaskSpawn() function is invoked with priority and stack size both
    set to zero, causing default values (possibly from an application-defined
    private symbol table) to be used.  Tokens in 
    _script_ are normally whitespace-delimited, but a token that is enclosed in
    single-quote characters (') may contain embedded whitespace and may contain
    escaped single-quote characters ("\\'").  On any parsing
    failure returns -1; otherwise returns the value returned by sm\_TaskSpawn().

# USER'S GUIDE

- Compiling an application that uses "platform":

    Just be sure to "#include "platform.h"" at the top of each
    source file that includes any platform function calls.

- Linking/loading an application that uses "platform":

        a.   In a Solaris environment, link with these libraries:

                 -lplatform -socket -nsl -posix4 -c

        b.   In a Linux environment, simply link with platform:

                 -lplatform

        c.   In a VxWorks environment, use

                 ld 1, 0, "libplatform.o"

             to load platform on the target before loading applications.

# SEE ALSO

gettimeofday(3C)
