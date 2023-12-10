# NAME

ltpmeter - LTP daemon task for aggregating and segmenting transmission blocks

# SYNOPSIS

**ltpmeter** _remote\_engine\_nbr_

# DESCRIPTION

**ltpmeter** is a background "daemon" task that manages the presentation of
LTP segments to link service output tasks.  Each "span" of LTP data interchange
between the local LTP engine and a neighboring LTP engine requires its own
**ltpmeter** task.  All **ltpmeter** tasks are spawned automatically by
**ltpadmin** in response to the 's' command that starts operation of the
LTP protocol, and they are all terminated by **ltpadmin** in response to an
'x' (STOP) command.

**ltpmeter** waits until its span's current transmission block (the data
to be transmitted during the transmission session that is currently being
constructed) is ready for transmission, then divides the data in the
span's block buffer into segments and enqueues the segments for transmission
by the span's link service output task (giving the segments semaphore to
unblock the link service output task as necessary), then reinitializes the
span's block buffer and starts another session (giving the "buffer empty"
semaphore to unblock the client service task -- nominally **ltpclo**, the
LTP convergence layer output task for Bundle Protocol -- as necessary).

**ltpmeter** determines that the current transmission block is ready for
transmission by waiting until either (a) the aggregate size of all service
data units in the block's buffer exceeds the aggregation size limit for
this span or (b) the length of time that the first service data unit in
the block's buffer has been awaiting transmission exceeds the aggregation
time limit for this span.  The "buffer full" semaphore is given when ION
(either the ltp\_send() function or the **ltpclock** daemon) determines
that one of these conditions is true; **ltpmeter** simply waits for this
semaphore to be given.

The initiation of a new session may also be blocked: the total number of
transmission sessions that the local LTP engine may have open at a single
time is limited (this is LTP flow control), and while the engine is at this
limit no new sessions can be started.  Availability of a session from the
session pool is signaled by the "session" semaphore, which is given whenever
a session is completed or canceled.

# EXIT STATUS

- "0"

    **ltpmeter** terminated normally, for reasons noted in the **ion.log** file.  If
    this termination was not commanded, investigate and solve the problem identified
    in the log file and use **ltpadmin** to restart **ltpmeter**.

- "1"

    **ltpmeter** terminated abnormally, for reasons noted in the **ion.log** file.
    Investigate and solve the problem identified in the log file, then use
    **ltpadmin** to restart **ltpmeter**.

# FILES

No configuration files are needed.

# ENVIRONMENT

No environment variables apply.

# DIAGNOSTICS

The following diagnostics may be issued to the **ion.log** log file:

- ltpmeter can't initialize LTP.

    **ltpadmin** has not yet initialized LTP protocol operations.

- No such engine in database.

    _remote\_engine\_nbr_ is invalid, or the applicable span has not yet
    been added to the LTP database by **ltpadmin**.

- ltpmeter task is already started for this engine.

    Redundant initiation of **ltpmeter**.

- ltpmeter can't start new session.

    An unrecoverable database error was encountered.  **ltpmeter** terminates.

- Can't take bufClosedSemaphore.

    An unrecoverable database error was encountered.  **ltpmeter** terminates.

- Can't create extents list.

    An unrecoverable database error was encountered.  **ltpmeter** terminates.

- Can't post ExportSessionStart notice.

    An unrecoverable database error was encountered.  **ltpmeter** terminates.

- Can't finish session.

    An unrecoverable database error was encountered.  **ltpmeter** terminates.

# BUGS

Report bugs to <ion-dtn-support@lists.sourceforge.net>

# SEE ALSO

ltpadmin(1), ltpclock(1)
